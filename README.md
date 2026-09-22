NodeGraph
=========

Typed, Qt-based node/edge graph editor with a C++ facade and planned JavaScript integration. The current focus is eliminating qgraphicsitem_cast usage and consolidating working docs into a single plan file.

Quick Links
- Development plan and session resume: `PLAN.MD`
- Build (Linux/WSL): `mkdir -p build_linux && cd build_linux && cmake .. && make -j`
- Build (Windows): Generate with CMake and build in Visual Studio
- Run (Windows): `.\run.ps1` — puts the build's Qt runtime on PATH and forwards
  arguments, e.g. `.\run.ps1 --script scripts/drop_demo.js` or
  `.\run.ps1 -Debug`. The executables are directly runnable too (Qt is deployed
  next to them); `run.ps1` also works before deployment / for a bare Qt install.
- Run (Linux/WSL): `./NodeGraph`
- Concatenate sources for review: `bash concat.sh` (outputs `concatenated_code.txt`)

Status
- Core cleanup: `Scene` now keeps UUID registries in sync with Qt ownership via `notifyNodeDestroyed/notifyEdgeDestroyed`, so File → New / script clears leave no dangling pointers.
- Autosave validation: the legacy O(n²) scan is disabled by default; re-enable it for diagnostics by compiling with `-DNG_ENABLE_AUTOSAVE_VALIDATION=1`.
- JavaScript roadmap: Graph facade is scriptable today (`./NodeGraph --script …`). Every node can persist a `<script>` block + payload (exposed through `graph.setNodeScript`, `graph.setNodePayload`, `graph.executeNodeScript`) and call the new `graph.runSyntheticWork()` helper so “heavy” work still happens in C++; upcoming branches (`feature/node-javascript-behavior`, `feature/edge-javascript-expressions`, etc.) will expand this into full dataflow execution—track progress in `PLAN.MD`.
- See `PLAN.MD` for detailed checklists, UI verification scenarios, and current branch instructions.

File loading validates the complete replacement graph before changing the open
document. Failed loads preserve the graph, selection, and undo history. Both
manual-save XML and autosave `<nodes>` / `<connections>` wrappers are supported.

Regression tests run headlessly through CTest. From the repository root:

```text
python scripts/build_and_test.py
```

CTest runs the two Qt Test binaries plus two JavaScript suites
(`tests/js_regression_suite.js`, `tests/coverage_suite.js`) through
`NodeGraph --script`; the app exits nonzero when a JS check fails, so ctest
reports it like any other test.

The helper normalizes the Windows process environment for MSBuild and enables
`BUILD_TESTING`. Use `--build-dir build_review` to choose the build directory,
or `--libxml2-source <checkout>` to reuse a local libxml2 source tree offline.
For an existing build, run `ctest --test-dir <build-dir> -C Debug --output-on-failure`.
