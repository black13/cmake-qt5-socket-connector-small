VISA + Ghidra Research Track
============================

Objective
- Explore VISA runtime behavior (e.g., NI‑VISA) to enable optional tooling and informed integration without creating a hard build‑time dependency.

Phase 1: Inventory and Triage
- Collect local VISA binaries (not committed):
  - Typical paths: `C:\Program Files\IVI Foundation\VISA\Win64\Bin`, `C:\Program Files (x86)\IVI Foundation\VISA\WinNT\Bin`
  - Example targets: `visa32.dll`, `visa64.dll`, `nivisa64.dll`, `nivisa32.dll`
- Export map:
  - Run `dumpbin /exports <dll>` (from a Developer Command Prompt) or `llvm-objdump --exports` and store text outputs in a private folder.
- Identify priority APIs:
  - `viOpenDefaultRM`, `viOpen`, `viClose`, `viRead`, `viWrite`, `viSetAttribute`, `viGetAttribute`, `viFindRsrc`

Phase 2: Ghidra Analysis
- Create a new Ghidra project and import x64/x86 DLLs as appropriate.
- Enable auto‑analysis (default options). For key functions above, record:
  - Calling convention, parameter sizes, pointer/handle patterns
  - Frequently used attributes (timeouts, termination char, buffering)
  - Common status codes and error flows

Phase 3: Optional Runtime Shim (OFF by default)
- Add a small dynamic loader that resolves VISA functions at runtime via `LoadLibrary`/`GetProcAddress` and prints version/availability.
- Keep behind CMake option `WITH_VISA_SHIM` and build it as a separate, opt‑in CLI for local testing only.

Legal / Ethical
- Respect vendor licenses and EULAs; do not redistribute proprietary binaries or derived signatures.
- Prefer the published VISA specifications (VPP‑4.3) and official SDK headers when available.

Deliverables
- This document (track overview and steps).
- A brief call‑map markdown with notes per function (private if needed).
- An optional `visa-probe` CLI (built only when `WITH_VISA_SHIM=ON`).

Next Steps
- Decide the minimal probing set required by our tooling.
- If we proceed with the shim, integrate a tiny abstraction that the app can call to query “is VISA available?” and “list resources”, without hard linking.

Ghidra Install (Windows)
------------------------

Recommended layout
- Avoid Program Files to reduce UAC friction; prefer a tools folder:
  - Example: `D:\Tools\Ghidra\ghidra_10.4.x` (or `C:\Tools\Ghidra\ghidra_10.4.x`)
- Ghidra “installs” by unzip; no MSI. Keep versions side‑by‑side.

Chosen Path
- We will use: `D:\ghidra_11.3.1_PUBLIC`
- Extract the official zip directly into that folder so `D:\ghidra_11.3.1_PUBLIC\ghidraRun.bat` exists.

Steps
1) Download Ghidra zip from NSA’s GitHub release page.
2) Extract to your tools folder, e.g.: `D:\ghidra_11.3.1_PUBLIC`
3) Ensure Java 17 is available (Temurin JDK 17 recommended).
   - Optional env: `setx GHIDRA_JDK "C:\\Program Files\\Eclipse Adoptium\\jdk-17.x.x"`
4) Set convenience env var (optional):
   - `setx GHIDRA_HOME "D:\\ghidra_11.3.1_PUBLIC"`
5) Run via: `"%GHIDRA_HOME%\ghidraRun.bat"` (or double‑click `ghidraRun.bat`).

Project location
- Default is `%USERPROFILE%\ghidra_projects`. You can create a clean workspace path such as `D:\ghidra_projects` and point Ghidra to it on first run.

Verification
- Check version: `"%GHIDRA_HOME%\support\analyzeHeadless.bat" -help`
- If launching fails, verify Java 17 is on PATH or `GHIDRA_JDK` is set.

Cleanup reference
- See `scripts/cleanup_ghidra.ps1` for safe removal of configs (`.ghidra*` under the user profile), optional project purge, and optional install folder deletion.
