// Ghidra headless export: write either per-function .c files or one combined .c
// Usage (via analyzeHeadless -postScript):
//   ghidra_export_decomp.java outDir=C:\out\dir
//   ghidra_export_decomp.java outFile=C:\out\single.c

import ghidra.app.decompiler.*;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.*;
import ghidra.util.task.TaskMonitor;
import java.io.*;
import java.nio.charset.StandardCharsets;

public class ghidra_export_decomp extends GhidraScript {
    @Override
    protected void run() throws Exception {
        String outDir = null;
        String outFile = null;
        for (String arg : getScriptArgs()) {
            int idx = arg.indexOf('=');
            if (idx > 0) {
                String k = arg.substring(0, idx).trim();
                String v = arg.substring(idx + 1).trim();
                if (k.equalsIgnoreCase("outDir")) outDir = v;
                if (k.equalsIgnoreCase("outFile")) outFile = v;
            }
        }
        if (outDir == null && outFile == null) {
            printerr("Specify outDir=... or outFile=...");
            return;
        }

        DecompInterface di = new DecompInterface();
        di.toggleCCode(true);
        di.toggleSyntaxTree(true);
        // Use default simplification style for this Ghidra version
        di.openProgram(currentProgram);

        Listing listing = currentProgram.getListing();
        FunctionIterator funcs = listing.getFunctions(true);

        StringBuilder combined = outFile != null ? new StringBuilder() : null;
        if (combined != null) {
            combined.append("/*\n");
            combined.append("Ghidra single-file decompilation export\n");
            combined.append("Program: ").append(currentProgram.getName()).append("\n");
            combined.append("Language: ").append(currentProgram.getLanguage().getLanguageDescription()).append("\n");
            combined.append("*/\n\n");
        }

        int exported = 0;
        while (funcs.hasNext() && !monitor.isCancelled()) {
            Function f = funcs.next();
            DecompileResults res = di.decompileFunction(f, 30, monitor);
            if (!res.decompileCompleted()) continue;
            String code = res.getDecompiledFunction().getC();

            if (outFile != null) {
                combined.append("/* Function ").append(f.getName()).append(" @ ")
                        .append(f.getEntryPoint().toString()).append(" */\n");
                combined.append(code).append("\n\n");
            } else {
                String safeName = f.getName().replaceAll("[^A-Za-z0-9_]+", "_");
                String fname = safeName + "_" + f.getEntryPoint().toString().replace(':', '_') + ".c";
                File out = new File(outDir, fname);
                writeUtf8(out, code);
                exported++;
            }
        }

        if (outFile != null) {
            writeUtf8(new File(outFile), combined.toString());
            println("ghidra_export_decomp.java> Exported single file to: " + outFile);
        } else {
            println("ghidra_export_decomp.java> Exported functions: " + exported);
        }
    }

    private static void writeUtf8(File file, String content) throws IOException {
        File parent = file.getParentFile();
        if (parent != null && !parent.exists()) parent.mkdirs();
        try (OutputStream os = new FileOutputStream(file)) {
            os.write(content.getBytes(StandardCharsets.UTF_8));
        }
    }
}
