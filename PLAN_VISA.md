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

Push
- `git push -u origin feature/ghidra-visa-tooling`

