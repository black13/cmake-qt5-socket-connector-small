NodeGraph
=========

Typed, Qt-based node/edge graph editor with a C++ facade and planned JavaScript integration. The current focus is eliminating qgraphicsitem_cast usage and consolidating working docs into a single plan file.

Quick Links
- Development plan and session resume: `PLAN.MD`
- Build (Linux/WSL): `mkdir -p build_linux && cd build_linux && cmake .. && make -j`
- Build (Windows): Generate with CMake and build in Visual Studio
- Run: `./NodeGraph`
- Concatenate sources for review: `bash concat.sh` (outputs `concatenated_code.txt`)

Status
- Core cleanup: `Scene` now keeps UUID registries in sync with Qt ownership via `notifyNodeDestroyed/notifyEdgeDestroyed`, so File → New / script clears leave no dangling pointers.
- Autosave validation: the legacy O(n²) scan is disabled by default; re-enable it for diagnostics by compiling with `-DNG_ENABLE_AUTOSAVE_VALIDATION=1`.
- JavaScript roadmap: Graph facade is scriptable today (`./NodeGraph --script …`). Every node can persist a `<script>` block + payload (exposed through `graph.setNodeScript`, `graph.setNodePayload`, `graph.executeNodeScript`); upcoming branches (`feature/node-javascript-behavior`, `feature/edge-javascript-expressions`, etc.) will expand this into full dataflow execution—track progress in `PLAN.MD`.
- See `PLAN.MD` for detailed checklists, UI verification scenarios, and current branch instructions.
