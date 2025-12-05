Clean Reinstall (Windows): Ghidra + NI-VISA
===========================================

Overview
- Goal: remove all local Ghidra traces and uninstall NI-VISA, then install a clean, current NI-VISA.
- All steps require an elevated PowerShell (Run as Administrator) for full effect.

1) Stop Ghidra and wipe local configs
- Optional dry-run (shows intent):
  - `pwsh -File .\scripts\cleanup_ghidra.ps1 -WhatIf`
- Remove configs and (optionally) your local projects:
  - Keep projects: `pwsh -File .\scripts\cleanup_ghidra.ps1`
  - Also delete %USERPROFILE%\ghidra_projects: `pwsh -File .\scripts\cleanup_ghidra.ps1 -PurgeProjects`
- If you installed Ghidra in custom folders, also delete them:
  - `pwsh -File .\scripts\cleanup_ghidra.ps1 -InstallDirs 'C:\ghidra_10.4.2' -PurgeProjects`

2) Uninstall NI-VISA via NI Package Manager (nipkg)
- List installed packages (GUI is fine too):
  - Start Menu → NI Package Manager
- CLI (elevated):
  - Dry-run: `pwsh -File .\scripts\ni_visa_reset.ps1 -WhatIf`
  - Remove VISA (and optional GPIB):
    - `pwsh -File .\scripts\ni_visa_reset.ps1`
    - Include GPIB: `pwsh -File .\scripts\ni_visa_reset.ps1 -IncludeGPIB`
- Reboot after uninstall.

3) Install clean NI Package Manager and NI-VISA
- Download NI Package Manager (NIPM) from NI’s site and install.
- Using NIPM, install:
  - NI-VISA 64-bit (latest) — "Full" or Runtime as needed
  - Optional: NI-488.2 (if you need GPIB) and NI I/O Trace (helpful for debugging)
- Reboot after install.

4) Verify installation and probe
- Check that VISA DLLs exist:
  - `C:\Program Files\IVI Foundation\VISA\Win64\Bin\nivisa64.dll`
- Build/run the visa-probe (no Qt needed):
  - `cmake -S . -B vp_build -G "Visual Studio 17 2022" -A x64 -DWITH_VISA_SHIM=ON -DWITH_VISA_SHIM_ONLY=ON`
  - `cmake --build vp_build --config Debug --target visa-probe`
  - `vp_build\visa-probe.exe`
  - Or specify: `set VISA_DLL_PATH="C:\Program Files\IVI Foundation\VISA\Win64\Bin\nivisa64.dll"` then run.

5) Proceed with project build (optional)
- If building NodeGraph on Windows, ensure Qt paths in `build.bat` match your install.
- In a Developer Command Prompt for VS 2022: `cmd /c build.bat debug`

Notes
- Scripts are safe-by-default. Use `-WhatIf` to preview changes. Run elevated for Machine-scope changes.
- Do not redistribute NI binaries. Keep research bins local and ignored by git.

