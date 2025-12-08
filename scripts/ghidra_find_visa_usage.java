// Ghidra headless analysis: detect NIVISA usage patterns
// - Checks for static imports of nivisa64.dll
// - Finds LoadLibrary*/GetProcAddress call xrefs
// - Finds string references to VISA API names and nivisa64.dll
// - Emits a concise report to console (and optional file)
//
// Usage with analyzeHeadless (similar to ghidra_run_headless.ps1):
//   -postScript ghidra_find_visa_usage.java dll=nivisa64.dll regex=^vi[A-Za-z0-9_]+$ report=C:\out\visa-usage.txt

import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.*;
import ghidra.program.model.lang.Register;
import ghidra.program.model.symbol.*;
import ghidra.program.model.address.*;
import ghidra.program.model.data.*;
import ghidra.program.model.mem.Memory;
import ghidra.util.task.TaskMonitor;

import java.io.*;
import java.nio.charset.StandardCharsets;
import java.util.*;
import java.util.regex.Pattern;

public class ghidra_find_visa_usage extends GhidraScript {
    private String targetDll = "nivisa64.dll"; // case-insensitive match
    private Pattern visaNameRegex = Pattern.compile("(?i)^vi[A-Za-z0-9_]+$");
    private String reportPath = null;
    private boolean reportAppend = false;

    private final List<String> commonVisaApis = Arrays.asList(
            // Core
            "viOpenDefaultRM", "viOpen", "viClose", "viFindRsrc", "viFindNext",
            "viRead", "viWrite", "viPrintf", "viQueryf",
            // Attributes
            "viGetAttribute", "viSetAttribute",
            // Events & status
            "viEnableEvent", "viDisableEvent", "viDiscardEvents", "viWaitOnEvent",
            // Locking, IO control
            "viLock", "viUnlock", "viFlush", "viStatusDesc",
            // Others commonly seen
            "viGpibControlREN", "viGpibControlATN", "viGpibSendIFC",
            "viGpibCommand", "viGpibPassControl"
    );

    @Override
    protected void run() throws Exception {
        parseArgs();

        StringBuilder out = new StringBuilder();
        out.append("visa-usage report for ").append(currentProgram.getName()).append("\n");
        out.append("target dll: ").append(targetDll).append("\n\n");

        // 1) Static imports: does the binary import nivisa64.dll directly?
        boolean hasStatic = false;
        Set<String> importedLibs = new TreeSet<>(String.CASE_INSENSITIVE_ORDER);
        try {
            ExternalManager em = currentProgram.getExternalManager();
            for (String lib : em.getExternalLibraryNames()) {
                if (lib != null && !lib.isEmpty()) importedLibs.add(lib);
            }
            for (String lib : importedLibs) {
                if (lib.equalsIgnoreCase(targetDll) || lib.toLowerCase().contains("/" + targetDll.toLowerCase())
                        || lib.toLowerCase().contains("\\" + targetDll.toLowerCase())) {
                    hasStatic = true;
                    break;
                }
            }
        } catch (Throwable t) {
            // Fallback when ExternalManager APIs differ by Ghidra version
        }
        out.append("static import of ").append(targetDll).append(": ").append(hasStatic ? "yes" : "no").append("\n");
        if (!importedLibs.isEmpty()) {
            out.append("imported libraries (top-level): ").append(String.join(", ", importedLibs)).append("\n");
        }
        out.append("\n");

        // 2) Find LoadLibrary*/GetProcAddress call xrefs (by symbol name)
        List<String> apisToCheck = Arrays.asList("LoadLibraryA", "LoadLibraryW", "LoadLibraryExA", "LoadLibraryExW", "GetProcAddress", "LdrLoadDll");
        Map<String, List<String>> callSites = new LinkedHashMap<>();
        for (String api : apisToCheck) {
            List<String> sites = findCallSitesByName(api);
            if (!sites.isEmpty()) callSites.put(api, sites);
        }
        if (!callSites.isEmpty()) {
            out.append("dynamic import helpers (call sites):\n");
            for (Map.Entry<String, List<String>> e : callSites.entrySet()) {
                out.append("  ").append(e.getKey()).append(": ").append(e.getValue().size()).append(" hits\n");
                int shown = 0;
                for (String s : e.getValue()) {
                    out.append("    ").append(s).append("\n");
                    if (++shown >= 20) { // cap to keep output reasonable
                        out.append("    ... (" + (e.getValue().size() - shown) + " more)\n");
                        break;
                    }
                }
            }
            out.append("\n");
        } else {
            out.append("no obvious LoadLibrary/GetProcAddress xrefs found (may be heavily inlined or obfuscated)\n\n");
        }

        // 2a) Detailed GetProcAddress argument recovery and usage mapping
        try {
            out.append("GetProcAddress details (recovered arguments):\n");
            List<CallDetail> gpCalls = findGetProcAddressCalls();
            if (gpCalls.isEmpty()) {
                out.append("  <none found>\n\n");
            } else {
                for (CallDetail cd : gpCalls) {
                    out.append("  ").append(cd.funcDesc).append(" -> ").append(cd.callAddr.toString())
                       .append(" : ");
                    if (cd.procName != null) out.append('"').append(cd.procName).append('"');
                    else if (cd.ordinal >= 0) out.append("ORD:").append(Long.toString(cd.ordinal));
                    else out.append("<unknown>");
                    if (!cd.usageCalls.isEmpty()) {
                        out.append("  usage:");
                        for (Address u : cd.usageCalls) out.append(' ').append(u.toString());
                    }
                    out.append("\n");
                }
                out.append("\n");
            }
        } catch (Throwable t) {
            out.append("[warn] detailed GetProcAddress analysis failed: ").append(t.getMessage()).append("\n\n");
        }

        // 3) Find strings for the target DLL and known VISA API names
        List<String> dllRefs = findStringRefs(targetDll);
        out.append("string refs containing '" + targetDll + "': " + dllRefs.size() + "\n");
        for (int i = 0; i < Math.min(20, dllRefs.size()); i++) {
            out.append("  ").append(dllRefs.get(i)).append("\n");
        }
        if (dllRefs.size() > 20) out.append("  ... (" + (dllRefs.size() - 20) + " more)\n");
        out.append("\n");

        // 3a) Regex-identified VISA names (vi...)
        List<String> viRegexRefs = findRegexStringRefs(visaNameRegex);
        out.append("string refs matching regex '" + visaNameRegex.pattern() + "': " + viRegexRefs.size() + "\n");
        for (int i = 0; i < Math.min(40, viRegexRefs.size()); i++) {
            out.append("  ").append(viRegexRefs.get(i)).append("\n");
        }
        if (viRegexRefs.size() > 40) out.append("  ... (" + (viRegexRefs.size() - 40) + " more)\n");
        out.append("\n");

        // 3b) Known VISA API name hits
        List<String> explicitRefs = new ArrayList<>();
        for (String name : commonVisaApis) {
            explicitRefs.addAll(findStringRefs(name));
        }
        out.append("string refs for common VISA APIs: " + explicitRefs.size() + "\n");
        for (int i = 0; i < Math.min(40, explicitRefs.size()); i++) {
            out.append("  ").append(explicitRefs.get(i)).append("\n");
        }
        if (explicitRefs.size() > 40) out.append("  ... (" + (explicitRefs.size() - 40) + " more)\n");
        out.append("\n");

        // 4) Quick heuristic: functions that likely do dynamic VISA resolve
        //    If a function contains refs to both target dll and vi* names, flag it.
        Set<String> suspectFuncs = new TreeSet<>();
        for (String dllRef : dllRefs) {
            String func = extractFunctionNameFromRefLine(dllRef);
            if (func == null) continue;
            for (String viRef : viRegexRefs) {
                String f2 = extractFunctionNameFromRefLine(viRef);
                if (func.equals(f2)) {
                    suspectFuncs.add(func);
                }
            }
        }
        if (!suspectFuncs.isEmpty()) {
            out.append("suspect dynamic resolve functions (dll + vi* strings present):\n");
            for (String s : suspectFuncs) out.append("  ").append(s).append("\n");
            out.append("\n");
        }

        // emit
        String text = out.toString();
        println(text);
        if (reportPath != null) writeUtf8(new File(reportPath), text, reportAppend);
    }

    private void parseArgs() {
        for (String arg : getScriptArgs()) {
            int idx = arg.indexOf('=');
            if (idx <= 0) continue;
            String k = arg.substring(0, idx).trim();
            String v = arg.substring(idx + 1).trim();
            if (k.equalsIgnoreCase("dll")) targetDll = v;
            if (k.equalsIgnoreCase("regex")) visaNameRegex = Pattern.compile(v);
            if (k.equalsIgnoreCase("report")) reportPath = v;
            if (k.equalsIgnoreCase("append")) reportAppend = Boolean.parseBoolean(v);
        }
    }

    private List<String> findCallSitesByName(String apiName) {
        List<String> hits = new ArrayList<>();
        try {
            SymbolTable st = currentProgram.getSymbolTable();
            SymbolIterator it = st.getSymbolIterator(apiName, true);
            while (it.hasNext() && !monitor.isCancelled()) {
                Symbol s = it.next();
                if (!s.getName().equalsIgnoreCase(apiName)) continue;
                Address target = s.getAddress();
                if (target == null) continue;
                Reference[] refs = getReferencesTo(target);
                for (Reference r : refs) {
                    Address from = r.getFromAddress();
                    Function f = getFunctionContaining(from);
                    String fdesc = f != null ? (f.getName() + " @ " + f.getEntryPoint().toString()) : "<no-func>";
                    hits.add(String.format("%s -> %s", fdesc, from.toString()));
                }
            }
        } catch (Throwable t) {
            // Ghidra versions differ; tolerate partial failure
        }
        return hits;
    }

    private List<String> findStringRefs(String needle) {
        List<String> hits = new ArrayList<>();
        if (needle == null || needle.isEmpty()) return hits;
        String needleLower = needle.toLowerCase(Locale.ROOT);
        Listing listing = currentProgram.getListing();
        DataIterator it = listing.getDefinedData(true);
        while (it.hasNext() && !monitor.isCancelled()) {
            Data d = it.next();
            DataType dt = d.getDataType();
            String sVal = null;
            try {
                if (dt instanceof StringDataType || dt instanceof TerminatedStringDataType) {
                    sVal = d.getDefaultValueRepresentation();
                } else if (dt instanceof UnicodeDataType || dt instanceof TerminatedUnicodeDataType) {
                    sVal = d.getDefaultValueRepresentation();
                }
            } catch (Throwable t) {
                // robust to data exceptions
            }
            if (sVal == null) continue;
            String normalized = stripQuotes(sVal);
            if (normalized.toLowerCase(Locale.ROOT).contains(needleLower)) {
                hits.add(formatRef(d));
            }
        }
        return hits;
    }

    private List<String> findRegexStringRefs(Pattern p) {
        List<String> hits = new ArrayList<>();
        Listing listing = currentProgram.getListing();
        DataIterator it = listing.getDefinedData(true);
        while (it.hasNext() && !monitor.isCancelled()) {
            Data d = it.next();
            DataType dt = d.getDataType();
            String sVal = null;
            try {
                if (dt instanceof StringDataType || dt instanceof TerminatedStringDataType) {
                    sVal = d.getDefaultValueRepresentation();
                } else if (dt instanceof UnicodeDataType || dt instanceof TerminatedUnicodeDataType) {
                    sVal = d.getDefaultValueRepresentation();
                }
            } catch (Throwable t) { }
            if (sVal == null) continue;
            String normalized = stripQuotes(sVal);
            if (p.matcher(normalized).find()) {
                hits.add(formatRef(d));
            }
        }
        return hits;
    }

    private String formatRef(Data d) {
        Address a = d.getAddress();
        String sval = stripQuotes(safe(d.getDefaultValueRepresentation()));
        Function f = null;
        // Collate by first xref's function, if any
        Reference[] refs = getReferencesTo(a);
        for (Reference r : refs) {
            f = getFunctionContaining(r.getFromAddress());
            if (f != null) break;
        }
        String fname = f != null ? (f.getName() + " @ " + f.getEntryPoint().toString()) : "<no-func>";
        return String.format("%s : %s (data @ %s)", fname, sval, a.toString());
    }

    private static String stripQuotes(String s) {
        if (s == null) return null;
        if (s.startsWith("\"") && s.endsWith("\"") && s.length() >= 2) return s.substring(1, s.length() - 1);
        return s;
    }

    private static String safe(String s) { return s == null ? "" : s; }

    private static String extractFunctionNameFromRefLine(String line) {
        if (line == null) return null;
        int sep = line.indexOf(" : ");
        if (sep <= 0) return null;
        String lhs = line.substring(0, sep).trim(); // e.g. "FuncName @ 00123456" or "<no-func>"
        if (lhs.equals("<no-func>")) return null;
        return lhs; // preserve entrypoint for disambiguation
    }

    // ----- Detailed GetProcAddress analysis helpers -----

    private static class CallDetail {
        String funcDesc;
        Address callAddr;
        String procName; // if resolved
        long ordinal = -1; // if ordinal
        List<Address> usageCalls = new ArrayList<>(); // indirect calls using returned pointer
    }

    private List<CallDetail> findGetProcAddressCalls() {
        List<CallDetail> out = new ArrayList<>();
        try {
            SymbolTable st = currentProgram.getSymbolTable();
            SymbolIterator it = st.getSymbolIterator("GetProcAddress", true);
            while (it.hasNext() && !monitor.isCancelled()) {
                Symbol s = it.next();
                if (!s.getName().equalsIgnoreCase("GetProcAddress")) continue;
                Address target = s.getAddress();
                if (target == null) continue;
                Reference[] refs = getReferencesTo(target);
                for (Reference r : refs) {
                    Address from = r.getFromAddress();
                    if (from == null) continue;
                    Instruction callInstr = getInstructionAt(from);
                    if (callInstr == null) continue;
                    Function f = getFunctionContaining(from);
                    String fdesc = f != null ? (f.getName() + " @ " + f.getEntryPoint().toString()) : "<no-func>";
                    CallDetail cd = new CallDetail();
                    cd.funcDesc = fdesc;
                    cd.callAddr = from;
                    recoverGetProcArgs(callInstr, cd);
                    mapUsageCalls(callInstr, cd);
                    out.add(cd);
                }
            }
        } catch (Throwable t) {
            // tolerate partial failure
        }
        return out;
    }

    private void recoverGetProcArgs(Instruction callInstr, CallDetail cd) {
        // Heuristics for x86 stdcall: look backward for MOV [ESP+4], <addr> or PUSH <addr> (second push before call)
        Address cur = callInstr.getAddress();
        int maxBack = 25;
        int pushesSeen = 0;
        Address pushProcNameAddr = null;
        Long procOrdinal = null;
        boolean foundMovEsp4 = false;

        Instruction it = callInstr.getPrevious();
        for (int i = 0; i < maxBack && it != null; i++, it = it.getPrevious()) {
            String mnem = it.getMnemonicString().toUpperCase(Locale.ROOT);
            try {
                if (mnem.equals("MOV")) {
                    // Check if dest is [ESP+4]
                    String op0 = it.getDefaultOperandRepresentation(0).toUpperCase(Locale.ROOT);
                    if (op0.contains("[ESP+") && (op0.contains("+0X4") || op0.matches(".*\\[ESP\\s*\\+\\s*4\\].*"))) {
                        Address to = firstToAddressRef(it);
                        if (to != null) {
                            pushProcNameAddr = to;
                            foundMovEsp4 = true;
                            break;
                        }
                        // maybe immediate ordinal in op1
                        Long imm = firstImmediate(it, 1);
                        if (imm != null && imm < 0x10000L) {
                            procOrdinal = imm;
                            foundMovEsp4 = true;
                            break;
                        }
                    }
                }
                if (mnem.equals("PUSH")) {
                    // Collect push candidates (right-to-left arg order)
                    // The procName should be the second push before call
                    Address to = firstToAddressRef(it);
                    if (to != null) {
                        pushesSeen++;
                        if (pushesSeen == 2) {
                            pushProcNameAddr = to;
                            break;
                        }
                        continue;
                    }
                    Long imm = firstImmediate(it, 0);
                    if (imm != null && imm < 0x10000L) {
                        pushesSeen++;
                        if (pushesSeen == 2) {
                            procOrdinal = imm;
                            break;
                        }
                    }
                }
            } catch (Throwable t) {
                // ignore and continue
            }
        }

        if (pushProcNameAddr != null) {
            String s = tryReadString(pushProcNameAddr);
            if (s != null && !s.isEmpty()) cd.procName = s;
        } else if (procOrdinal != null) {
            cd.ordinal = procOrdinal;
        }
    }

    private void mapUsageCalls(Instruction callInstr, CallDetail cd) {
        // Heuristic: return value in EAX; track registers assigned from EAX and find subsequent call reg
        Set<String> regsHolding = new HashSet<>();
        regsHolding.add("EAX");
        Instruction it = callInstr.getNext();
        for (int i = 0; i < 40 && it != null; i++, it = it.getNext()) {
            String mnem = it.getMnemonicString().toUpperCase(Locale.ROOT);
            if (mnem.equals("MOV")) {
                Register dst = it.getRegister(0);
                Register src = it.getRegister(1);
                if (dst != null && src != null) {
                    if (regsHolding.contains(src.getName().toUpperCase(Locale.ROOT))) {
                        regsHolding.add(dst.getName().toUpperCase(Locale.ROOT));
                    }
                }
            }
            if (mnem.startsWith("CALL")) {
                // Operand 0 may be register or memory [reg]
                String op0 = it.getDefaultOperandRepresentation(0).toUpperCase(Locale.ROOT);
                for (String r : regsHolding) {
                    if (op0.equals(r) || op0.contains("[" + r + "]") || op0.contains("[" + r + "+") || op0.contains("[" + r + "-")) {
                        cd.usageCalls.add(it.getAddress());
                        // do not break early; there may be multiple uses
                        break;
                    }
                }
            }
            if (mnem.equals("RET") || mnem.equals("RETN")) break;
        }
    }

    private Address firstToAddressRef(Instruction instr) {
        try {
            Reference[] refs = instr.getReferencesFrom();
            for (Reference r : refs) {
                Address to = r.getToAddress();
                if (to != null && to.isMemoryAddress()) return to;
            }
        } catch (Throwable t) { }
        return null;
    }

    private Long firstImmediate(Instruction instr, int opIndex) {
        try {
            Object[] objs = instr.getOpObjects(opIndex);
            for (Object o : objs) {
                if (o instanceof ghidra.program.model.scalar.Scalar) {
                    ghidra.program.model.scalar.Scalar s = (ghidra.program.model.scalar.Scalar)o;
                    return Long.valueOf(s.getUnsignedValue());
                }
            }
        } catch (Throwable t) { }
        return null;
    }

    private String tryReadString(Address addr) {
        try {
            // Prefer existing defined data
            Data d = getDataAt(addr);
            if (d == null) d = getCurrentProgram().getListing().getDefinedDataContaining(addr);
            if (d != null) {
                DataType dt = d.getDataType();
                if (dt instanceof StringDataType || dt instanceof TerminatedStringDataType) {
                    return stripQuotes(d.getDefaultValueRepresentation());
                }
                if (dt instanceof UnicodeDataType || dt instanceof TerminatedUnicodeDataType) {
                    String u = stripQuotes(d.getDefaultValueRepresentation());
                    return u;
                }
            }
            // Fallback: raw memory read as ASCII
            Memory mem = currentProgram.getMemory();
            byte[] buf = new byte[256];
            int i = 0;
            while (i < buf.length) {
                byte b = mem.getByte(addr.add(i));
                if (b == 0) break;
                buf[i++] = b;
            }
            if (i == 0) return null;
            return new String(buf, 0, i, java.nio.charset.StandardCharsets.ISO_8859_1);
        } catch (Throwable t) {
            return null;
        }
    }

    private static void writeUtf8(File file, String content, boolean append) throws IOException {
        File parent = file.getParentFile();
        if (parent != null && !parent.exists()) parent.mkdirs();
        try (OutputStream os = new FileOutputStream(file, append)) {
            os.write(content.getBytes(StandardCharsets.UTF_8));
        }
    }
}
