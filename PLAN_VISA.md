VISA/Ghidra Research Track Plan
===============================

Branch
- Create from up-to-date `main`:
  - `git checkout -b feature/ghidra-visa-tooling`

Deliverables
- `VISA_GHIDRA.md` (overview, phases, legal)
- `scripts/collect_visa_binaries.ps1` (local-only helper)
- Optional (future): `tools/visa-probe` CLI behind `WITH_VISA_SHIM` CMake option

Tasks
1) Inventory exports
   - Run `dumpbin /exports` (x86/x64) and save output under `research_bins/visa/exports/` (ignored by git).
   - Identify the minimal API subset for probing.
2) Ghidra project setup
   - Import DLLs, run auto-analysis, label the subset functions.
   - Record calling convention and parameter sizes in a notes file.
3) Shim design (optional)
   - Draft a tiny loader that resolves the minimal subset at runtime and prints availability/version.
   - Keep OFF by default (`-DWITH_VISA_SHIM=OFF`).
4) Integration notes
   - Define what the app/tooling needs: enumerate resources, open session, read/write, set timeout/termchar.
   - Document risks and EULA constraints.

NI MAX Decompile Summary (from Ghidra exports)
----------------------------------------------

- App Type
  - 32-bit Windows GUI built with MFC 14.0 (VS2015 CRT/UCRT).
  - Typical MSVC startup path: `entry -> __scrt_common_main_seh -> WinMain-like thunk -> app init`.

- Control Flow
  - Entrypoint delegates to CRT main and then to the app runner:
    - `fresh_clone/ghidra/exports/NIMax/004017b5_entry.c:1` calls `__scrt_common_main_seh()`.
    - `fresh_clone/ghidra/exports/NIMax/0040164d___scrt_common_main_seh.c:1` runs CRT init, configures atexit, parses cmdline, then calls `FUN_00401090((HMODULE)&IMAGE_DOS_HEADER_00400000)`.
    - `fresh_clone/ghidra/exports/NIMax/00401090_FUN_00401090.c:1` is the handoff:
      - Calls `FUN_004010e0(hModule)` to cache the executable directory.
      - Calls `NiMaxInit()`, `NiMaxRun(NULL, NULL)`, then `NiMaxCleanup()`.
      - Calls `AfxSetModuleState(FUN_004011b0())` to set the MFC module state.

- Implication
  - The EXE is a bootstrap; primary logic/resources live in `NIMaxImp.dll` (MFC extension). Focus detailed analysis on `NIMaxImp.dll` and loaded vendor/VISA DLLs.

Push
- `git push -u origin feature/ghidra-visa-tooling`
