NodeGraph
=========

Typed, Qt-based node/edge graph editor with a C++ facade and planned JavaScript integration. The current focus is eliminating qgraphicsitem_cast usage and consolidating working docs into a single plan file.

Quick Links
- Development plan and session resume: `PLAN.MD`
- Build (Linux/WSL): `mkdir -p build_linux && cd build_linux && cmake .. && make -j`
- Build (Windows): Generate with CMake and build in Visual Studio
- Run: `./NodeGraph`
- Concatenate sources for review: `bash concat.sh` (outputs `concatenated_code.txt`)

Code Coverage (macOS)
---------------------
Use Apple clang/LLVM’s built-in tooling:

```
export COVERAGE_FLAGS="-fprofile-instr-generate -fcoverage-mapping"
cmake -S . -B build_coverage \
      -DCMAKE_BUILD_TYPE=Debug \
      "-DCMAKE_CXX_FLAGS=${COVERAGE_FLAGS}" \
      "-DCMAKE_C_FLAGS=${COVERAGE_FLAGS}"
cmake --build build_coverage

# Run the instrumented binary (pass CLI args as needed)
LLVM_PROFILE_FILE="nodegraph-%p.profraw" ./build_coverage/NodeGraph

# Merge profiles and generate a report
xcrun llvm-profdata merge -sparse nodegraph-*.profraw -o nodegraph.profdata
xcrun llvm-cov report ./build_coverage/NodeGraph -instr-profile=nodegraph.profdata
```

Helper: `scripts/coverage_report.sh` automates build → run → report. Run it without arguments to launch the instrumented app (close the window manually when finished), or pass `--script …` if you want to drive a headless scenario:
```
./scripts/coverage_report.sh                   # interactive run, no startup script
./scripts/coverage_report.sh --script scripts/coverage_scenario.js  # automated scenario
```
Set `QT_QPA_PLATFORM=offscreen` yourself if you need a headless session.

Status
- Core cleanup: `Scene` now keeps UUID registries in sync with Qt ownership via `notifyNodeDestroyed/notifyEdgeDestroyed`, so File → New / script clears leave no dangling pointers.
- Autosave validation: the legacy O(n²) scan is disabled by default; re-enable it for diagnostics by compiling with `-DNG_ENABLE_AUTOSAVE_VALIDATION=1`.
- JavaScript roadmap: Graph facade is scriptable today (`./NodeGraph --script …`). Every node can persist a `<script>` block + payload (exposed through `graph.setNodeScript`, `graph.setNodePayload`, `graph.executeNodeScript`) and call the new `graph.runSyntheticWork()` helper so “heavy” work still happens in C++; upcoming branches (`feature/node-javascript-behavior`, `feature/edge-javascript-expressions`, etc.) will expand this into full dataflow execution—track progress in `PLAN.MD`.
- See `PLAN.MD` for detailed checklists, UI verification scenarios, and current branch instructions.
