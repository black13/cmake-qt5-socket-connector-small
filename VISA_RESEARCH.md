# VISA Research

Branch: `feature/ghidra-visa-tooling`

## Objective

Build a **mock server that NI/VISA tools can communicate with** by reverse-engineering NI runtime behavior and understanding the VISA protocol over TCP/IP SOCKET.

## Key Finding - December 8, 2025

**Problem**: Mock VISA server communication timed out despite correct responses.

**Root Cause**: `VI_ATTR_TERMCHAR_EN` defaults to `VI_FALSE` for TCPIP::SOCKET resources.

**Solution**: Set `VI_ATTR_TERMCHAR_EN = VI_TRUE` in VISA I/O Settings.

### Working Configuration

```
Resource: TCPIP::127.0.0.1::5025::SOCKET
VI_ATTR_TERMCHAR = 0x0A (\n)
VI_ATTR_TERMCHAR_EN = VI_TRUE  <-- CRITICAL (disabled by default!)
Timeout = 2000ms
```

## TCPIP::SOCKET Protocol

| Item | Value |
|------|-------|
| Terminator | `\n` (0x0A) |
| Message framing | None - plain text with terminator |
| Basic query | `*IDN?\n` |
| Default termchar | 0x0A but **disabled by default** |

### Binary Data Transfers

For binary data (waveforms, spectra), terminators don't work because binary data may contain 0x0A.

**IEEE 488.2 Block Format**: `#<digits><length><binary data>`

Example: `#3256` = 3 digits for length, length is 256, followed by 256 raw bytes.

For binary: set `VI_ATTR_TERMCHAR_EN = VI_FALSE`, read exact byte count from header.

## Mock VISA Server

Location: `mock_visa_server.py`

```bash
python mock_visa_server.py
# Add in NI MAX: TCPIP::127.0.0.1::5025::SOCKET
# Enable termination character in I/O Settings!
```

## NI Process Architecture

- **NIMax.exe** - 32-bit MFC bootstrap (just a launcher)
- **NIMaxImp.dll** - Actual NI MAX implementation
- **NIvisaic.exe** - Loads VISA transport DLLs

### VISA Transport DLLs (loaded by NIvisaic.exe)

| DLL | Purpose |
|-----|---------|
| NiViEnet.dll | Ethernet/TCP-IP |
| NiViAsrl.dll | Serial |
| NiVi488.dll | GPIB |
| NiViUsb.dll | USB |

Location: `C:\Program Files (x86)\IVI Foundation\VISA\WinNT\Bin\`

## NI Documentation References

- [Termination Characters in NI-VISA](https://www.ni.com/en/support/documentation/supplemental/06/termination-characters-in-ni-visa.html)
- [VI_ATTR_TERMCHAR](https://www.ni.com/docs/en-US/bundle/ni-visa-api-ref/page/ni-visa-api-ref/vi_attr_termchar.html)
- [VI_ATTR_TERMCHAR_EN](https://www.ni.com/docs/en-US/bundle/ni-visa-api-ref/page/ni-visa-api-ref/vi_attr_termchar_en.html)
- [Data Blocks - NI](https://www.ni.com/docs/en-US/bundle/ni-visa/page/data-blocks.html)

---

## Architecture: Instrument Simulation Server

### Design Philosophy

- **Python** = Prototyping, quick iteration (mock_visa_server.py)
- **C/C++** = Production server for instrument simulation
- **Goal** = Simulate expensive test equipment without hardware

### Why C/C++?

1. Performance for binary data handling (waveforms, traces)
2. Integration with existing Qt/CMake codebase
3. Direct memory control for IEEE 488.2 block transfers
4. Can link against VISA SDK headers for type correctness
5. Real instruments use C - matching semantics helps accuracy

### Multi-Instrument Test Stack

```
Port 5025 - Spectrum Analyzer (e.g., Keysight N9010A)
Port 5026 - Signal Generator (e.g., Keysight N5182B)
Port 5027 - Oscilloscope (e.g., Keysight DSOX3104T)
Port 5028 - Power Supply (e.g., Keysight E36313A)
```

Each simulated instrument needs:
1. Unique `*IDN?` response matching real instrument format
2. Instrument-specific SCPI command set
3. Binary block support (IEEE 488.2) for waveform/trace data
4. Realistic measurement data generation
5. Proper `VI_ATTR_TERMCHAR` handling

### What We Need to Build

| Component | Language | Purpose |
|-----------|----------|---------|
| `mock_visa_server.py` | Python | Prototyping, protocol testing |
| `visa_sim_server` | C++ | Production multi-instrument server |
| `instrument_*.cpp` | C++ | Per-instrument SCPI handlers |
| `scpi_parser.cpp` | C++ | SCPI command parsing |
| `ieee488_block.cpp` | C++ | Binary block encode/decode |

### What We Need to Determine

| Question | Status |
|----------|--------|
| What SCPI commands do real instruments respond to? | TODO - get manuals |
| What binary formats do spectrum analyzers use? | TODO - IEEE 488.2 blocks |
| How to simulate measurement data realistically? | TODO - math models |
| Instrument timing characteristics? | TODO - response delays |

---

## Ghidra Decompilation Research

### Decompiled Binaries

| File | Relevance | Description |
|------|-----------|-------------|
| `NiSpyVV.dll.c` | **High** | VISA call spy - shows `CVISACall` structures |
| `analyzer.exe.c` | **High** | GPIB Analyzer - shows `niGpibAnalyzer_*` client API |
| `NI IO Trace.exe.c` | Medium | I/O tracing - logging patterns |
| `NIMax.exe.c` | Low | 32-bit MFC bootstrap, calls `NIMaxImp.dll` |
| `NIMaxImp.dll.c` | Medium | NI MAX main implementation |
| `nicntsae.dll.c` | Low | PXI controller infra (not VISA protocol) |
| `NiSpy*.dll.c` | Low | IVI/LabVIEW spy modules |

## Key APIs Discovered

**GPIB Analyzer API** (from `analyzer.exe`):
```c
niGpibAnalyzer_Open(interface)
niGpibAnalyzer_Close(handle)
niGpibAnalyzer_StartCapture(handle, &config, &buffer, flag)
niGpibAnalyzer_StopCapture(handle)
niGpibAnalyzer_ReadBus(handle, &data)
niGpibAnalyzer_WriteBus(handle, data)
niGpibAnalyzer_SetCallback(handle, type, func, userdata)
niGpibAnalyzer_FindFirstInterface(buffer, size, ...)
```

**NI Spy Classes** (from `NiSpyVV.dll`):
- `INiSpyLogger` / `INiSpyLoggerImpl`
- `CVISACall` - wraps VISA API calls
- `CIviCall` - wraps IVI API calls
- `CVisaGenericFormat` - formats call data

**VISA Core Functions** (targets for compatibility):
- `viOpenDefaultRM`, `viOpen`, `viClose`
- `viRead`, `viWrite`
- `viSetAttribute`, `viGetAttribute`
- `viFindRsrc`

## NI MAX Bootstrap Flow

From Ghidra analysis of `NIMax.exe`:
```
entry -> __scrt_common_main_seh -> FUN_00401090
  -> NiMaxInit()
  -> NiMaxRun(NULL, NULL)
  -> NiMaxCleanup()
  -> AfxSetModuleState(...)
```
The EXE is just a launcher; real logic is in `NIMaxImp.dll`.

## Communication Architecture

NI tools use:
1. **Registry config** - paths in `HKLM\SOFTWARE\National Instruments\NI-488.2\CurrentVersion`
2. **COM/MFC** - `IUnknown`, `IDispatch` for component communication
3. **Shared memory** - `MapViewOfFile`/`CreateFileMappingA` (in analyzer)
4. **DLL APIs** - not sockets; calls into vendor DLLs

## Binary Locations

| Component | Path |
|-----------|------|
| MAX EXE | `C:\Program Files (x86)\National Instruments\MAX\NIMax.exe` |
| MAX DLL | `C:\Program Files (x86)\National Instruments\MAX\NIMaxImp.dll` |
| VISA 32-bit | `C:\Windows\SysWOW64\visa32.dll` |
| VISA 64-bit | `C:\Windows\System32\visa64.dll` |
| NI VISA 64-bit | `C:\Windows\System32\nivisa64.dll` |
| NI Tulip | `C:\Program Files (x86)\IVI Foundation\VISA\WinNT\Bin\NiVisaTulip.dll` |

## Tools

**visa-probe CLI** (optional, no Qt):
```bash
cmake -S . -B vp_build -G "Visual Studio 17 2022" -A x64 \
  -DWITH_VISA_SHIM=ON -DWITH_VISA_SHIM_ONLY=ON
cmake --build vp_build --config Debug --target visa-probe
.\vp_build\Debug\visa-probe.exe
```

**Ghidra headless export**:
```bash
pwsh .\scripts\decompile_nimax.ps1 -GhidraHome "D:\ghidra_11.3.1_PUBLIC" -Single
```

**Ghidra project**: `ghidra/exports/_gh_proj/NIResearch.gpr`

## Next Steps

1. [ ] Document `niGpibAnalyzer_*` and `CVISACall` structures with Doxygen
2. [ ] Build compatible server implementing same DLL exports/callbacks
3. [ ] Focus on callback types (0-3), settings IDs, buffer formats

## Legal

- Respect vendor licenses and EULAs
- Do not redistribute proprietary binaries
- Prefer published VISA specs (VPP-4.3) and official SDK headers
- Keep research artifacts local and git-ignored
