# NodeGraph Development Plan
**Updated:** 2025-11-06
**PRIMARY GOAL:** Eliminate qgraphicsitem_cast - architectural rot (0 remaining of 17)
**SECONDARY GOAL:** Full JavaScript integration for programmable nodes/edges

---

## 🎯 NEXT STEP (Session Resume)

**Current Status (Nov 24, 2025):**
- Branch: `main` (feature/node-javascript-behavior merged)
- Scripting UX: Shift+Left context menu, per-node JS logging, default summary scripts
- Diagnostics: Scene cleanup logging, scripted-node API docs, TEMP helper script `thescript.sh`

**Polish Cycle – Branch Per Goal**
1. `polish/snap-grid` – refine grid rendering & snapping indicator
2. `polish/view-navigation` – smoother zoom/pan, recenter shortcuts
3. `polish/context-menu` – UX tweaks for scripted-node menu (disabled states, log hygiene)
4. `polish/status-logging` – consolidate log spam, add concise status HUD for scripts/saves
5. `polish/selection-copy` – prep selection HUD for future copy/paste/undo work

**Tooling / Samples**
- `scripts/generate_graph.py` produces sample XML graphs (pipeline, split-merge) so we can quickly load larger scratch-pad layouts and test snapping/selection behavior.

Each branch: `git checkout -b polish/<topic>`, run targeted fixes, build/test, then PR into `main`.
---

## Observer & Rendering Next Steps (Nov 2025)

### Current Architecture Snapshot
- `Graph` is the public facade coordinating `Scene` and `GraphFactory` (shared API for C++ and JavaScript).
- `Scene` owns QGraphics items but exposes typed collections (`Node*`, `Edge*`) without QObject inheritance.
- `GraphObserver`/`GraphSubject` deliver plain-C++ notifications; observers must not touch QGraphics items.
- Rendering is moving toward snapshot feeds so the QGraphics view consumes value objects, not live scene pointers.

### Guiding Principles
- Keep Node/Edge/Socket free of QObject; rely on explicit registration and deterministic teardown.
- Observers exchange UUIDs, geometry structs, and payload data—never `QGraphicsItem*`.
- `Graph` remains the sole surface for scripting/UI; `Scene` stays an internal detail.
- Batch operations defer notifications via `GraphSubject::beginBatch()` / `endBatch()`.

### Phase 1 – Event Hub Stabilization
1. Introduce a `GraphEventHub` (or extend `GraphSubject`) that publishes typed event structs:
   - `NodeCreated { QUuid id, QString type, QPointF position }`
   - `NodeMoved { QUuid id, QPointF from, QPointF to }`
   - `EdgeCreated { QUuid id, QUuid fromNode, int fromSocket, QUuid toNode, int toSocket }`
2. Route all `Graph` facade mutations (create/delete/connect/disconnect/move/load/save) through the hub.
3. Ensure batch mode coalesces events before emission and document observer expectations.

### Phase 2 – Snapshot-Based Rendering Bridge
1. Build a `GraphSnapshotBuilder` that walks typed collections and produces immutable render data:
   - `SceneSnapshot { QVector<NodeRenderData>, QVector<EdgeRenderData>, GhostEdgeState }`
2. Create a rendering observer (`RenderBridge`) that listens for hub events and requests snapshots when state changes.
3. Update the QGraphics view ("QGraph") to consume snapshots instead of raw `Scene` pointers.
4. Keep ghost-edge interactions typed: add `Scene::socketAt()` and maintain magnet snapping without `itemAt()` casts.

### Phase 3 – Autosave & Persistence Observers
1. Port `XmlAutosaveObserver` to the event hub—remove any direct `Scene::items()` iteration.
2. Fire autosave once per batch when `GraphSubject::endBatch()` runs.
3. Add failure telemetry (path, error messages) without exposing QGraphics objects to observers.

### Phase 4 – Diagnostics, Logging, Undo Preparation
1. Implement a logging observer that records event structs (human-readable plus optional JSON for tooling).
2. Draft undo/redo command wrappers that store event snapshots rather than graphics references.
3. Document the observer/snapshot interplay in `GRAPH_SPECIFICATION.md` and cross-link from `RULES.md`.

### Phase 5 – JavaScript Validation & Examples
1. Add facade test scripts under `scripts/` (and future `examples/`) covering node create/move/delete, edge connect/disconnect, and autosave verification.
2. Keep scripts asynchronous-safe by querying state through facade methods (`Graph::getNodeData`, `Graph::getAllEdges`, etc.).

### Immediate Follow-Up Items
- Validate ghost-edge typed lookup in UI (hover/release) and magnet snapping
- Update existing `GraphObserver` implementations (autosave, future undo) to consume the new event structs
- Proceed to Branch 1.6 (window typed queries) or Phase 2 bug fixes (e.g., 2.1 graph save)
- Refresh documentation once the event hub and snapshot pipeline are in place


---

## Session Resume Checklist

**Use this checklist when resuming work after computer restart or context loss:**

1. **Understand Current State:**
   - [ ] Run `git status` to see uncommitted changes
   - [ ] Run `git log --oneline -10` to see recent commits
   - [ ] Check current branch with `git branch`
   - [ ] Review ISSUE.md for known bugs
   - [ ] Review this PLAN.MD Progress Tracking section

2. **Rebuild Project:**
   - [ ] Windows: Open Visual Studio solution, rebuild
   - [ ] WSL/Linux: `cd build_linux && cmake .. && make`
   - [ ] Verify build succeeds before making changes

3. **Validate Current Code:**
   - [ ] Run concatenation: `bash concat.sh` (generates concatenated_code.txt)
   - [ ] Check for dead code with grep: `git grep -n "qgraphicsitem_cast"`
   - [ ] Check test files exist: `ls -la test_*.xml test_*.js`

4. **Memory Leak Testing (WSL/Linux only):**
   - [ ] Build with ASAN: `cmake -DCMAKE_BUILD_TYPE=Debug .. && make`
   - [ ] Run leak test: `LSAN_OPTIONS=suppressions=../asan_suppressions.txt ./NodeGraph test_corrupt_duplicate.xml`
   - [ ] Verify no leaks from graph_factory.cpp

5. **Quick Smoke Test:**
   - [ ] Launch application: `./NodeGraph` (Linux) or run from Visual Studio (Windows)
   - [ ] Test basic operations: create nodes, connect sockets, save/load
   - [ ] Test JavaScript CLI: `./NodeGraph --script test_memory_leak.js`

6. **Identify Next Task:**
   - [ ] Look at Progress Tracking section below
   - [ ] Pick next unchecked [ ] branch
   - [ ] Read the branch description and implementation plan
   - [ ] Create branch: `git checkout -b <branch-name>`

---

## Project Vision

**Architecture First:**
- [ ] ZERO qgraphicsitem_cast violations (17 found - CRITICAL)
- [ ] Typed collections everywhere (Node*, Edge*, Socket*)
- [ ] Objects manage their own lifecycle
- [ ] No type checking loops

**JavaScript Integration:**
- [x] Graph operations callable from JavaScript (Graph facade with QJSEngine)
- [x] Individual nodes have custom JavaScript behavior (any node may persist a `<script>` block + payload via `graph.setNodeScript`, `graph.setNodePayload`, `graph.executeNodeScript`)
- [x] Synthetic workload helper: expose a C++ `SyntheticWork::run(request)` utility so scripts can trigger deterministic “heavy” tasks (hashing, math loops, sleeps) without reimplementing them in JS; wire it through `graph.runSyntheticWork()` and the ScriptNode API so both nodes and edges share the same pathway.
- [x] Context menu tooling: from the graph view you can right-click a node (or selection) (or Shift+Left click for quick access) to run its script via the Graph facade; logs show `[ScriptRunner]` entries for every invocation, and the status bar surfaces a short summary.
- [ ] Scripted node metadata: extend `ScriptNodeApi` with helpers (`id()`, `type()`, `socketCount(role)`, `edgeCount()`, etc.) plus default template scripts so every new node logs its identity and sockets without manual setup. Document these helpers inline so future script authors know what's available. Remember: `ScriptedNode::write` fires on autosave/save (immediately after palette drops), so the starter script is persisted right away and can be triggered via context menu or CLI.
- [x] Window instrumentation: `Window::showContextMenu` now logs which node/selection triggered the action so scripted-node debugging has traceable context in the main log (easily stripped/macro'd later).
- [ ] macOS polish: add an app bundle icon (NodeGraph.icns + Info.plist tweaks) so the Dock shows a branded icon when running on macOS; document the asset location and CMake tweaks once implemented.
- [ ] Nodes/edges have dynamic payloads (properties, weights, metadata)
- [ ] Users script node logic without recompiling C++
- [ ] Complete application scriptable end-to-end

---

## UI Verification & Stability Plan (Nov 2025)

**Goal:** Provide repeatable scenarios so every visible UI surface and script entry point can be validated quickly after merges or environment resets.

### Component Coverage Map
- **Window chrome & menus** – File (New, Open, Save, Save As), Edit shortcuts (Ctrl+Z placeholder), View controls (Zoom Fit/Reset), Help/About dialogs, status bar text.
- **Node Palette Widget** – category filter/search, tooltip descriptions, drag-and-drop from every template button, keyboard navigation, scrollbar behavior when resized.
- **Graph View / Scene** – node placement, rubber-band selection, Delete key handling, context menu disablement, zoom/pan gestures, ghost-edge snapping, socket hover highlights.
- **Autosave & Logging** – autosave timer toggles, manual save triggers, log rolling per session, message consistency across UI actions.
- **JavaScript Interface** – `--script` CLI path, startup script set via window, console logging, error propagation to log and UI dialogs.
- **File IO dialogs** – Qt file pickers for open/save, overwrite prompts, handling of missing files, default directories.

### Functional Verification Checklist
1. **Baseline Start**
   - [ ] Launch app without arguments; confirm empty scene, palette populated, log created.
   - [ ] Cycle window resize, DPI scaling, and menubar shortcuts (Cmd vs Ctrl) on macOS/WSL.
2. **Palette to Scene**
   - [ ] Drag every built-in node (SOURCE/SINK/TRANSFORM/MERGE/SPLIT) into the scene; verify nodes land at cursor, tooltips match NodeTypeTemplates.
   - [ ] Use palette search to isolate a node type and ensure filtering updates button grid.
3. **Graph Editing**
   - [ ] Create Source→Transform→Sink chain using drag-drop; connect sockets by dragging; confirm ghost edge highlights valid targets only.
   - [ ] Multi-select via rubber-band drag (Qt modifiers Shift/Cmd/Ctrl supported); move selection and ensure edges follow.
   - [ ] Press Delete to remove selection; confirm graph stats/log updates and selection label refresh.
   - [ ] TODO: Add graticule/grid overlay toggle in View so node placement has visual reference; log when toggled for verification.
   - [ ] Validate delete flow end-to-end:
     - [ ] Rubber-band select multiple nodes/edges, hit Delete, and save to `rubberbanding1.xml` (see `logs/NodeGraph_2025-11-15_23-13-00.log` for `Window::deleteSelection` + `Graph::deleteSelection` entries).
     - [ ] Reopen the saved file and repeat to ensure selection survives across load/save cycles.
     - [ ] Add a JavaScript smoke test that uses `graph.getSelectedNodes`/`graph.deleteSelection()` to mirror the UI workflow.

4. **Scriptable Nodes & Localization (Future)**
   - [ ] Ensure every node type (including Source/Sink) can carry a script payload so behavior (start/stop triggers, processing) is injectable end-to-end.

   - [ ] Add a dummy script payload (e.g., placeholder JS) per ScriptNode so serialization + execution can be wired before real behaviors exist.
   - [ ] Extend `NodeTypeTemplates` to carry arbitrary metadata (script module, localization keys) that gets stamped into each node instance.
   - [ ] Expose Graph facade APIs to set/get script payloads per node/edge and to invoke hooks (e.g., `graph.runNodeScript(nodeId, "onDelete")`).
   - [ ] Provide iteration helpers (`graph.forEachNode`, `graph.forEachEdge`) so JavaScript can walk the graph, update labels/tooltips, or inject runtime logic without new C++ subclasses.
   - [ ] Prototype a localization script: traverse nodes, map labels via locale tables, log changes, and persist results alongside the graph.

4. **Persistence**
   - [ ] Save graph via Ctrl+S and File > Save As; inspect XML for node/edge count.
   - [ ] Reopen saved file; confirm positions preserved; verify autosave writes between edits.
   - [ ] Attempt open on invalid XML; ensure error dialog + scene remains clean.

5. **JavaScript Behavior in Nodes (Future)**
   - [ ] Introduce `ScriptedNode` that wraps `Node` but carries a `QString m_script` and uses a shared `QJSEngine` for evaluation.
   - [ ] Define a JS interface `function(node, inputs)` and expose a `ScriptNodeApi` QObject (setOutput/input) to interact with sockets.
   - [ ] Extend XML serialization with a `<script language="javascript">...</script>` child element per node; update `write/read` accordingly.
   - [ ] Provide C++ hooks (evaluate on input change, on double-click, etc.) and consider UI integration for editing scripts.

5. **JavaScript**
   - [ ] Run `./NodeGraph --script hello.js` to verify console/log output, available node types, and stats API.
   - [ ] Run regression scripts (`test_load_xmls.js`, `test_memory_leak.js`) and capture pass/fail counts in log.
   - [ ] Set `Window::setStartupScript` (CLI `--script`) and confirm execution after UI shows (watch `=== Executing startup script` log marker).
6. **Autosave**
   - [ ] Confirm autosave toggles on startup (log line `Autosave: enabled`), writes on drag/drop, and produces valid `autosave.xml`.
   - [ ] Simulate crash (force quit) and relaunch; ensure autosave prompt/resume path works (manual for now: inspect `autosave.xml` then load).

### Stability & Stress Scenarios
1. **Long Session Burn-in**
   - Create/delete 200+ nodes via JS batch script; pan/zoom continuously for 10 minutes; monitor log for warnings and check memory footprint.
2. **Repeated Load/Save Loop**
   - Alternate between `tests_small.xml` and `tests_stress.xml` in a loop (scripted) to ensure `Graph::clearGraph()` leaves no orphaned items and autosave keeps up.
3. **Autosave Pressure Test**
   - Lower autosave interval to 1s, spam node movements, and verify file locking/errors don’t appear; check `autosave.xml` writes stay <1 KB for small scenes.
4. **Invalid Input Fuzzing**
   - Feed malformed XML and scripts that call `graph.connectNodes` with bad indices; ensure errors surface via `Graph::errorOccurred` and UI does not crash.
5. **Script Flood**
   - Execute back-to-back scripts (hello.js, test_load_xmls.js, custom create_graph.js) to verify QJSEngine context reuse and log readability.
6. **Resource Cleanup**
   - Open/close window repeatedly (automated via Qt test or manual) to confirm `Scene::prepareForShutdown()` frees objects and subsequent launches remain stable.

### Integration Steps
- [ ] Track completion of each scenario per release in PLAN.MD (append dated checklist entries).
- [ ] Automate a subset via `scripts/` JS harness; aim to cover Palette→Scene, Save/Load, and Autosave flows first.
- [ ] Add CI note: once headless mode exists, convert these manual tests into scripted regressions.

### UI → Graph Surface → JavaScript Validation Chain
For every UI feature below, complete three passes in order: manual UI, Graph facade invocation, and JavaScript script.

| Feature Area | UI Actions | Graph Surface Checks | JavaScript Mirror |
|--------------|-----------|----------------------|-------------------|
| Palette drag/drop | Drag SOURCE/SINK into scene, confirm placement + autosave | Call `graph.createNode` for each type, compare `graph.getNodeData` to UI coords | Script `create_nodes.js` that calls `graph.createNode` with varying positions and logs stats |
| Selection + move | Shift+drag multi-select, move, verify edges follow | `graph.getSelectedNodes`, `graph.moveNode`, `graph.setNodePosition` maintain layout | Script `move_nodes.js` applies deterministic offsets via Graph API |
| Edge connections | Mouse drag between sockets, ensure ghost-edge + snap | `graph.connectNodes`, `graph.getNodeEdges`, `graph.deleteEdge` produce same topology | Script `connect_chain.js` creates chain and asserts stats |
| Persistence | File > Save/Load plus autosave recovery | `graph.saveToFile`, `graph.loadFromFile`, `graph.clearGraph`, `graph.toXml` (future) | Script `roundtrip.js` saves to temp file, clears, reloads, compares stats |
| Logging & errors | Trigger load failure/open invalid file, confirm dialogs/log entries | Watch `Graph::errorOccurred` signal or log output when calling facade with bad data | Script `bad_inputs.js` purposefully calls APIs with invalid IDs/indices |
| Startup script | Set CLI `--script`, confirm executes after UI shows | `Window::setStartupScript` uses `Graph::evalFile` | Self-test: script writes nodes then UI verifies presence |

Execution order:
1. Run through UI scenarios to confirm behavior manually.
2. Immediately run equivalent Graph-surface Qt test (can be via REPL or small harness) to ensure facade alone reproduces results.
3. Capture the same steps in `.js` scripts under `scripts/` so regressions can be automated later.

**Clean-state rule:** No leftovers—after each pass, clear the graph (`graph.clearGraph()` / File > New) so the next scenario begins in a pristine scene and autosave/logs are easy to audit.

**Palette expansion ideas:** When new node concepts emerge (custom templates, JS-driven nodes), register them via `NodeTypeTemplates` and add matching palette buttons as part of the verification chain above (UI → Graph → JS); the plan’s table doubles as the checklist for onboarding future palette entries.

### Verification Run Checklist (with Logging Evidence)
For each release or major change, run the following sequence; attach log timestamps or excerpts so results are auditable.

1. **Baseline Startup**
   - Launch app, note log lines `=== NodeGraph Application Started ===` and `NodePalette: Starting population...`.
   - Verify logfile path (logs/NodeGraph_*.log) is created and writable.
2. **Palette Drag/Drop**
   - Drag each node type once; capture log lines showing drag/drop (e.g., `NodeButton: Starting drag operation` & autosave write).
   - Record node IDs from `Graph::createNode` debug statements if available.
3. **Selection & Move**
   - Multi-select + move nodes; confirm log entries (`Graph::setNodePosition` or movement traces) and autosave flushes at new positions.
4. **Edge Connection**
   - Connect/disconnect edges; capture log lines `Graph::connectNodes` and `[AUTOSAVE]` summarizing node/edge counts.
5. **Persistence**
   - Save via UI (log `Saving graph to:`) and load the same file (log `Graph::loadFromFile: Successfully loaded`); inspect resulting XML and reference in notes.
6. **Error Handling**
   - Attempt to open invalid XML or connect invalid sockets via Graph API; ensure `Graph::errorOccurred` emits a message and log shows warning without crashing.
7. **JavaScript Mirror**
   - Run the companion script(s) (e.g., `scripts/create_nodes.js`, `scripts/connect_chain.js`); verify log entries `=== Executing startup script:` and `Script executed successfully`.
8. **Clean State**
   - Clear graph (UI or `graph.clearGraph()`), confirm log `Graph::clearGraph` and autosave `nodes=0`, then close the app with `=== NodeGraph Application Ending ===`.

Document the log file name plus timestamps for each checklist item so reviewers can cross-check behavior without rerunning.

### Scenario A – Save → Load → Edit → Save → Delete (UI + Graph + JS)
**Purpose:** Prove the full workflow (create node, save, reload, keep editing, clean shutdown) works through every surface.

**UI Path**
1. Launch app (empty scene).
2. Drag a SOURCE node into the scene.
3. File → Save As… (or Ctrl+S) → write `scenario_a_step1.xml`; note log lines (`Saving graph to:` etc.).
4. File → New (or `graph.clearGraph()` via command palette) to clear; confirm autosave nodes=0.
5. File → Open… → load `scenario_a_step1.xml`.
6. Drag an additional node (e.g., SINK) into the scene.
7. Ctrl+S to overwrite the same file; confirm log shows nodes incremented.
8. File → Open… (same file) to verify both nodes reappear.
9. Select all nodes (Shift+drag) and Delete; ensure autosave nodes=0 and log records `Graph::deleteNode` or observer events.

**Graph Facade Path**
1. From a Qt/CLI harness (or via script console), call:
   - `graph.clearGraph()`
   - `const id1 = graph.createNode("SOURCE", -100, -50);`
   - `graph.saveToFile("scenario_a_facade.xml");`
   - `graph.clearGraph(); graph.loadFromFile("scenario_a_facade.xml");`
   - `const id2 = graph.createNode("SINK", 150, 25);`
   - `graph.saveToFile("scenario_a_facade.xml");`
   - `graph.clearGraph(); graph.loadFromFile("scenario_a_facade.xml");`
   - `graph.deleteNode(id1); graph.deleteNode(id2); graph.saveToFile("scenario_a_facade.xml");`
2. Verify `graph.getGraphStats()` matches expectations after each step (log or console).

**JavaScript Path**
1. Create `scripts/scenario_a.js` replicating the facade steps above (use `console.log` for checkpoints).
2. Run `./NodeGraph --script scripts/scenario_a.js` and confirm log shows script execution, Graph API calls, and save/load cycle without UI interaction.
3. Keep the resulting log file references next to the checklist entry.
4. Use `scripts/scripted_node_demo.js` to exercise per-node scripting (payload mutations, label updates, save/load) and capture logs for regression tracking.

---

## Qt JavaScript Integration Roadmap

**Branch Workflow Reminder**
- Always create a named branch per feature (e.g., `feature/js-palette-hotkeys`).
- Do **not** delete local branches after review; push them (`git push origin <branch>`), open PR/merge to `main`, then `git checkout main && git merge <branch>` locally.
- After merging, push `main` (`git push origin main`) so history mirrors remote; keep branches around for traceability.

### Planned Branches / Phases
1. **Branch 4.1 – feature/node-javascript-behavior**
   - Expose node payload + callbacks to scripts.
   - Add JS examples for per-node logic.
   - Verification: script toggles node colors/labels and persists via save/load.
2. **Branch 4.2 – feature/edge-javascript-expressions**
   - Allow edges to compute values or validation via JS.
   - Provide Graph API hooks for reading edge metadata from scripts.
3. **Branch 5.x – JS-driven UI actions**
   - `graph.runFile()` helper, script-defined toolbar buttons, console pane.
   - Stress-test QJSEngine with sequential scripts (reuse `Script Flood` scenario).
4. **Branch 6.1 – test/javascript-integration-examples**
   - Curated examples under `examples/` (01_basic_graph.js, 02_node_scripting.js, etc.).
   - Integrate into documentation and README quick start.

Each branch should land with:
- Updated JS samples.
- Notes in this roadmap marking completion date + verification steps executed.
- Cross-link to UI Verification scenarios (especially JavaScript section) to keep manual and automated coverage aligned.

---

## Completed Work ✅

### Facade Migration (100% Complete)
- [x] Branch 1: Graph loading migrated to facade (commit: 4d55d48)
- [x] Branch 2: Graph clearing migrated to facade (commit: 770ac19)
- [x] Branch 3: Query operations migrated to facade (commit: b5ec77d)
- [x] Branch 4: Edge creation migrated to facade (commit: acfe762)
- [x] All window.cpp operations use Graph facade API
- [x] JavaScript CLI execution via `--script` option (commit: a88c25a)

### Test Code Cleanup
- [x] Removed scene.cpp test code (debugForceLayout3Nodes) (commit: 3d55337)
- [x] Removed ~713 lines of C++ test functions from window.cpp (commit: 462e3df)
- [x] Removed test menus and #if ENABLE_JS blocks
- [x] All testing now via JavaScript + Graph facade
- [x] Deleted obsolete plan files (commit: 0221c2d)
  - FACADE_MIGRATION_PLAN.md (completed)
  - plan_response.md (merged into this plan)
  - ISSUE.md (bugs merged into Phase 1)
  - SESSION_STATE.md (session notes)

**Net cleanup:** -2,711 lines, +1,225 lines

---

## Phase 1: Architecture - Eliminate qgraphicsitem_cast 🔥

**CRITICAL:** 17 violations found - fix them ALL

**Principle:** Objects manage their own lifecycle through typed collections, never through type checking or casting.

### Branch 1.1: Socket Parent Linkage
**Branch:** `refactor/socket-typed-parent`

**Problem:** socket.cpp:42 uses `qgraphicsitem_cast<Node*>(parentItem())`

**Changes:**
- [ ] socket.h: Add `Node* m_parentNode` private member
- [ ] socket.cpp: Initialize in constructor from parent
- [ ] socket.cpp: Remove qgraphicsitem_cast, use m_parentNode directly
- [ ] node.cpp: Pass `this` to Socket constructor

**Eliminates:** 1 cast violation

**Git Workflow:**
```bash
git checkout main
git checkout -b refactor/ghost-edge-typed
# ... make changes ...
git add scene.h scene.cpp
git commit -m "refactor: replace ghost-edge casts with typed lookup

- add Scene::socketAt() helper using typed collections
- route ghost-edge hover/release through typed socket lookup
- keep magnet snapping + state reset paths on typed sockets
- eliminates final qgraphicsitem_cast occurrences"
git push origin refactor/ghost-edge-typed
git checkout main
git merge refactor/ghost-edge-typed --no-ff
git push origin main
git branch -d refactor/ghost-edge-typed
```

---

### Branch 1.2: Node Socket Collections
**Branch:** `refactor/node-typed-collections`

**Problem:** node.cpp:188, 426 iterate childItems() with qgraphicsitem_cast<Socket*>

**Changes:**
- [ ] node.h: Add `QList<Socket*> m_inputSockets` and `QList<Socket*> m_outputSockets`
- [ ] node.h: Add `QList<Socket*> inputSockets()` and `outputSockets()` accessors
- [ ] node.cpp: Store sockets in typed lists when creating them
- [ ] node.cpp: Remove childItems() iteration with casts
- [ ] node.cpp: Use typed lists instead

**Eliminates:** 2 cast violations

**Git Workflow:**
```bash
git checkout main
git checkout -b refactor/ghost-edge-typed
# ... make changes ...
git add scene.h scene.cpp
git commit -m "refactor: replace ghost-edge casts with typed lookup

- add Scene::socketAt() helper using typed collections
- route ghost-edge hover/release through typed socket lookup
- keep magnet snapping + state reset paths on typed sockets
- eliminates final qgraphicsitem_cast occurrences"
git push origin refactor/ghost-edge-typed
git checkout main
git merge refactor/ghost-edge-typed --no-ff
git push origin main
git branch -d refactor/ghost-edge-typed
```

---

### Branch 1.3: Scene Typed Collections ✅ COMPLETE
**Branch:** `refactor/scene-typed-collections` → **Status:** MERGED (commit: 43b4ccb)

**Implementation:**
- [x] scene.h: QHash<QUuid, Node*> m_nodes and QHash<QUuid, Edge*> m_edges (already existed)
- [x] scene.h: Added QList<Node*> selectedNodes() and QList<Edge*> selectedEdges() helpers
- [x] scene.cpp: Implemented typed selection helpers
- [x] window.cpp: Updated updateSelectionInfo() to use typed helpers

**Note:** Scene already had QHash-based typed collections from earlier work. This branch added selection helpers and cleaned up window.cpp. No new qgraphicsitem_cast violations eliminated (window.cpp was already clean). Auto-layout casts (scene.cpp:538, 641) were eliminated by Branch 2.4 (remove/unused-auto-layout).

---

### Branch 1.4: Delete Key Overhaul 🔥 CRITICAL
**Branch:** `refactor/delete-key-self-managed`

**Problem:** scene.cpp:454-458 loops selectedItems() casting to Node*/Edge* for deletion

**Changes:**
- [ ] node.h: Add `void keyPressEvent(QKeyEvent* event) override;`
- [ ] node.cpp: Implement - if Delete key, call `deleteLater()`, accept event
- [ ] edge.h: Add `void keyPressEvent(QKeyEvent* event) override;`
- [ ] edge.cpp: Implement - if Delete key, call `deleteLater()`, accept event
- [ ] scene.cpp: REMOVE Scene::keyPressEvent deletion logic (lines 435-480)

**Eliminates:** 3 cast violations
**Architectural win:** Objects manage their own lifecycle

**Git Workflow:**
```bash
git checkout main
git checkout -b refactor/ghost-edge-typed
# ... make changes ...
git add scene.h scene.cpp
git commit -m "refactor: replace ghost-edge casts with typed lookup

- add Scene::socketAt() helper using typed collections
- route ghost-edge hover/release through typed socket lookup
- keep magnet snapping + state reset paths on typed sockets
- eliminates final qgraphicsitem_cast occurrences"
git push origin refactor/ghost-edge-typed
git checkout main
git merge refactor/ghost-edge-typed --no-ff
git push origin main
git branch -d refactor/ghost-edge-typed
```

---

### Branch 1.5: Ghost Edge Typed Access ✅ COMPLETE
**Branch:** `refactor/ghost-edge-typed` → Status: MERGED (local)

**Problem:** ghost-edge hover/release paths still depend on `itemAt()` + `qgraphicsitem_cast`

**Changes:**
- [x] scene.h / scene.cpp: add `Socket* socketAt(const QPointF& scenePos) const` helper that iterates typed node/socket collections
- [x] scene.cpp: use `socketAt` inside `updateGhostEdge()` and `mouseReleaseEvent()`; remove direct `itemAt()` usage
- [x] scene.cpp: keep magnet snapping and `resetAllSocketStates()` logic but ensure they rely exclusively on typed sockets
- [x] Record a manual ghost-edge test checklist (below) covering drag, invalid targets, snapping, and cancel flows

**Test Plan:**
- [ ] Right-click drag from output to input creates an edge and clears highlights
- [ ] Drag to invalid targets (output->output, same node) turns ghost edge red and cancels cleanly
- [ ] Release near (not on) an input to confirm magnet snapping selects the closest valid socket
- [ ] Cancel mid-drag (Esc or secondary click) to verify the ghost edge and socket states reset

**Eliminates:** Final scene.cpp `qgraphicsitem_cast` usage (lines 326, 504)

**Git Workflow:**
```bash
git checkout main
git checkout -b refactor/ghost-edge-typed
# ... make changes ...
git add scene.h scene.cpp
git commit -m "refactor: replace ghost-edge casts with typed lookup

- add Scene::socketAt() helper using typed collections
- route ghost-edge hover/release through typed socket lookup
- keep magnet snapping + state reset paths on typed sockets
- eliminates final qgraphicsitem_cast occurrences"
git push origin refactor/ghost-edge-typed
git checkout main
git merge refactor/ghost-edge-typed --no-ff
git push origin main
git branch -d refactor/ghost-edge-typed
```

---

### Branch 1.6: Window Typed Queries ✅ COMPLETE
**Branch:** `refactor/window-typed-queries` → Status: MERGED (local)

**Problem:** window.cpp remaining uses of scene item iteration with casts

**Changes:**
- [x] window.cpp: Replace all scene->items() loops with Scene::nodes()/edges()
- [x] window.cpp: Use typed collections for selection counting
- [x] window.cpp: Remove any remaining qgraphicsitem_cast calls

**Eliminates:** Remaining window.cpp casts

**Git Workflow:**
```bash
git checkout main
git checkout -b refactor/ghost-edge-typed
# ... make changes ...
git add scene.h scene.cpp
git commit -m "refactor: replace ghost-edge casts with typed lookup

- add Scene::socketAt() helper using typed collections
- route ghost-edge hover/release through typed socket lookup
- keep magnet snapping + state reset paths on typed sockets
- eliminates final qgraphicsitem_cast occurrences"
git push origin refactor/ghost-edge-typed
git checkout main
git merge refactor/ghost-edge-typed --no-ff
git push origin main
git branch -d refactor/ghost-edge-typed
```

---

### Branch 1.7: Factory Typed Serialization ✅ COMPLETE
**Branch:** `refactor/factory-typed-serialization` → Status: MERGED (local)

**Problem:** graph_factory.cpp:708 uses qgraphicsitem_cast for serialization

**Changes:**
- [x] graph_factory.cpp: Use Node::inputSockets()/outputSockets() for validation/iteration
- [x] graph_factory.cpp: Remove childItems() casting
- [x] Verify XML load/validation paths

**Eliminates:** 1 cast violation (final one!)

**Git Workflow:**
```bash
git checkout main
git checkout -b refactor/ghost-edge-typed
# ... make changes ...
git add scene.h scene.cpp
git commit -m "refactor: replace ghost-edge casts with typed lookup

- add Scene::socketAt() helper using typed collections
- route ghost-edge hover/release through typed socket lookup
- keep magnet snapping + state reset paths on typed sockets
- eliminates final qgraphicsitem_cast occurrences"
git push origin refactor/ghost-edge-typed
git checkout main
git merge refactor/ghost-edge-typed --no-ff
git push origin main
git branch -d refactor/ghost-edge-typed
```

---

## Phase 2: Critical Bug Fixes (Parallel to Phase 1)

**Note:** Bugs identified by Codex (from ISSUE.md analysis). Can be done in parallel with architecture work.

### Branch 2.1: Fix Graph::saveToFile() 🔥 ✅ COMPLETE
**Branch:** `fix/graph-save-to-file` → Status: MERGED (local)

**Problem:** `graph.cpp:371-379` - Graph::saveToFile() returns true but never writes file

**Changes (Option A - direct):**
- [x] Implement save loop in Graph (mirror `window.cpp:saveGraph`): create XML doc, call `node->write()` and `edge->write()`, write to disk
- [x] Remove TODO comment; emit `graphSaved(filePath)` on success
- [x] Add error handling/return false on failure

**Changes (Option B - factory):**
- [ ] Add `GraphFactory::saveToXmlFile(const QString&)` and call from Graph
- [ ] Keep window and facade save paths consistent

**Implementation:**
```cpp
// Option A sketch (facade-local)
bool Graph::saveToFile(const QString& filePath)
{
    qDebug() << "Graph::saveToFile:" << filePath;
    xmlDocPtr doc = xmlNewDoc(BAD_CAST "1.0");
    if (!doc) return false;
    xmlNodePtr root = xmlNewNode(nullptr, BAD_CAST "graph");
    xmlDocSetRootElement(doc, root);
    for (Node* node : m_scene->getNodes().values()) node->write(doc, root);
    for (Edge* edge : m_scene->getEdges().values()) edge->write(doc, root);
    int result = xmlSaveFormatFileEnc(filePath.toUtf8().constData(), doc, "UTF-8", 1);
    xmlFreeDoc(doc);
    if (result == -1) return false;
    emit graphSaved(filePath);
    return true;
}
```

**Test Plan:**
- [ ] Build and run application
- [ ] Create test graph with 3 nodes, 2 edges
- [ ] Call `graph.saveToFile("test_save.xml")` from JavaScript CLI
- [ ] Verify file exists in filesystem
- [ ] Verify file size > 0
- [ ] Load file via `graph.loadFromFile("test_save.xml")`
- [ ] Verify graph restored correctly

**Git Workflow:**
```bash
git checkout main
git checkout -b refactor/ghost-edge-typed
# ... make changes ...
git add scene.h scene.cpp
git commit -m "refactor: replace ghost-edge casts with typed lookup

- add Scene::socketAt() helper using typed collections
- route ghost-edge hover/release through typed socket lookup
- keep magnet snapping + state reset paths on typed sockets
- eliminates final qgraphicsitem_cast occurrences"
git push origin refactor/ghost-edge-typed
git checkout main
git merge refactor/ghost-edge-typed --no-ff
git push origin main
git branch -d refactor/ghost-edge-typed
```

---

### Branch 2.2: Fix Node Position Precision ✅ COMPLETE
**Branch:** `fix/node-position-precision` → Status: MERGED (local)

**Problem:** `node.cpp:123` only updates m_lastPos if movement > 5px, but `node.cpp:434-435` saves m_lastPos. Small nudges lost on save/reload.

**Changes:**
- [x] `node.cpp:write()` - Serialize `pos()` instead of `m_lastPos`

**Implementation:**
```cpp
// node.cpp:434-435 - Change from:
xmlSetProp(node, BAD_CAST "x", BAD_CAST QString::number(m_lastPos.x()).toUtf8().constData());
xmlSetProp(node, BAD_CAST "y", BAD_CAST QString::number(m_lastPos.y()).toUtf8().constData());

// To:
QPointF currentPos = pos();
xmlSetProp(node, BAD_CAST "x", BAD_CAST QString::number(currentPos.x()).toUtf8().constData());
xmlSetProp(node, BAD_CAST "y", BAD_CAST QString::number(currentPos.y()).toUtf8().constData());
```

**Test Plan:**
- [ ] Build and run application
- [ ] Create node at (100, 100)
- [ ] Nudge node 2px to (102, 100)
- [ ] Save graph
- [ ] Clear graph
- [ ] Load graph
- [ ] Verify node at (102, 100) not (100, 100)

**Git Workflow:**
```bash
git checkout main
git checkout -b refactor/ghost-edge-typed
# ... make changes ...
git add scene.h scene.cpp
git commit -m "refactor: replace ghost-edge casts with typed lookup

- add Scene::socketAt() helper using typed collections
- route ghost-edge hover/release through typed socket lookup
- keep magnet snapping + state reset paths on typed sockets
- eliminates final qgraphicsitem_cast occurrences"
git push origin refactor/ghost-edge-typed
git checkout main
git merge refactor/ghost-edge-typed --no-ff
git push origin main
git branch -d refactor/ghost-edge-typed
```

---

### Branch 2.3: Fix Load Rollback on Validation Failure ✅ COMPLETE
**Branch:** `fix/load-rollback` → **Status:** MERGED (commit: fix/load-rollback-memory-leak)

**Problem:** `graph_factory.cpp:461-535` created nodes/edges in Phase 2, but Phase 3 validation could fail (duplicate sockets). On failure, objects allocated but not deleted → memory leak.

**Implementation:**
- [x] Added cleanup loops to 4 error paths in graph_factory.cpp
- [x] Lines 463-466: Delete allNodes on node creation failure
- [x] Lines 486-493: Delete allNodes + allEdges on edge creation failure
- [x] Lines 530-535: Delete allNodes + allEdges on duplicate output socket validation failure
- [x] Lines 548-553: Delete allNodes + allEdges on duplicate input socket validation failure
- [x] Each cleanup loop explicitly calls `delete` before `return false`

**Validation:**
- [x] Created test_corrupt_duplicate.xml with duplicate socket connections
- [x] Created test_memory_leak.js to load corrupt file 50 times
- [x] Added ASAN support to CMakeLists.txt (Linux/WSL Debug builds)
- [x] Created asan_suppressions.txt for OpenGL false positives
- [x] Ran ASAN test on WSL: NO leaks from graph_factory.cpp ✓
- [x] Updated ISSUE.md to document the fix

**Key Learning:** Manual `delete` is correct pattern for Qt graphics items. Smart pointers (std::unique_ptr) would conflict with QGraphicsScene's parent-child ownership system.

---

### Branch 2.4: Remove Unused Auto-Layout Code
**Branch:** `remove/unused-auto-layout`

**Problem:** Scene contains two unused auto-layout functions that don't work and should be removed entirely. These functions also contain 2 qgraphicsitem_cast violations.

**Functions to Remove:**
- `Scene::autoLayoutAnneal` (scene.cpp:526) - attempted simulated annealing, doesn't work
- `Scene::autoLayoutForceDirected` (scene.cpp:629) - attempted force-directed layout, doesn't work

**Changes:**
- [ ] scene.h: Remove method declarations for both functions
- [ ] scene.cpp: Delete both function implementations
- [ ] window.cpp: Remove menu items/shortcuts that call these functions (if any)
- [ ] Verify no other callers exist with `git grep "autoLayout"`

**Eliminates:** 2 qgraphicsitem_cast violations (scene.cpp:532, 635)

**Test Plan:**
- [ ] Build and run application
- [ ] Verify no compilation errors
- [ ] Verify no runtime crashes
- [ ] Verify menu/shortcuts still work for remaining features

**Git Workflow:**
```bash
git checkout main
git checkout -b refactor/ghost-edge-typed
# ... make changes ...
git add scene.h scene.cpp
git commit -m "refactor: replace ghost-edge casts with typed lookup

- add Scene::socketAt() helper using typed collections
- route ghost-edge hover/release through typed socket lookup
- keep magnet snapping + state reset paths on typed sockets
- eliminates final qgraphicsitem_cast occurrences"
git push origin refactor/ghost-edge-typed
git checkout main
git merge refactor/ghost-edge-typed --no-ff
git push origin main
git branch -d refactor/ghost-edge-typed
```

---

### Branch 2.5: Enhanced Edge Visual Ordering
**Branch:** `feature/edge-z-order-enhancements`

**Status:** Basic implementation MERGED (commit: feature/edge-visual-ordering)
**Current Implementation:**
- [x] Selection z-order boost: selected edges jump to z=10
- [x] Connectivity-based stacking: edges with more connections get higher z-values (z = 2.0 + connections * 0.02)

**Remaining Improvements:**
- [ ] Cache z-value before selection to restore properly (currently always restores to z=2)
- [ ] Trigger `updateZOrderFromConnections()` when node connectivity changes (add/remove edges)
- [ ] Add perpendicular offset to control points for parallel edges between same nodes (fan-out effect)
- [ ] Add smoke test for deterministic z-ordering

**Changes Needed:**

**edge.h:**
```cpp
private:
    qreal m_normalZValue;  // Cache z-value before selection
```

**edge.cpp:**
```cpp
// In itemChange():
if (change == ItemSelectedHasChanged) {
    if (value.toBool()) {
        m_normalZValue = zValue();  // Cache before boosting
        setZValue(10);
    } else {
        setZValue(m_normalZValue);  // Restore cached value
    }
}

// Call updateZOrderFromConnections() after edge creation/deletion
// Add to Node::addEdge() and Node::removeEdge() if they exist
```

**scene.cpp:**
```cpp
// After creating edge in connectSockets():
edge->updateZOrderFromConnections();

// After deleting edge:
// Update z-order of remaining edges connected to affected nodes
```

**Test Plan:**
- [ ] Load tests_large.xml with MERGE nodes
- [ ] Verify edges stack by connectivity
- [ ] Select edge, verify it comes to front at z=10
- [ ] Deselect edge, verify it returns to correct stacked z-value (not always z=2)
- [ ] Add new edge, verify all connected edges update their z-order
- [ ] Delete edge, verify remaining edges update their z-order

**Git Workflow:**
```bash
git checkout main
git checkout -b refactor/ghost-edge-typed
# ... make changes ...
git add scene.h scene.cpp
git commit -m "refactor: replace ghost-edge casts with typed lookup

- add Scene::socketAt() helper using typed collections
- route ghost-edge hover/release through typed socket lookup
- keep magnet snapping + state reset paths on typed sockets
- eliminates final qgraphicsitem_cast occurrences"
git push origin refactor/ghost-edge-typed
git checkout main
git merge refactor/ghost-edge-typed --no-ff
git push origin main
git branch -d refactor/ghost-edge-typed
```

---

### Branch 2.6: Improve View/Scene Spacing and Navigation
**Branch:** `feature/improved-view-navigation`

**Problem:** Multiple viewport and navigation issues
- **Fixed scene rect:** scene.cpp:32 has fixed 2000x2000 area, large graphs go outside bounds
- **No auto-center:** Loading XML doesn't center view on content
- **No panning:** setDragMode(NoDrag) prevents middle-click pan
- **No auto-fit:** Can't zoom to fit entire graph
- **Tight node spacing:** 100px spacing causes overlaps (nodes are ~120-150px wide)

**Changes:**

**scene.cpp:**
- [ ] Replace fixed sceneRect with dynamic calculation based on content
- [ ] Add `updateSceneRect()` method that expands to fit all nodes + margin
- [ ] Call updateSceneRect() after loading graph, adding nodes, moving nodes

**view.cpp:**
- [ ] Change `setDragMode(QGraphicsView::ScrollHandDrag)` for middle-click pan
- [ ] Add Ctrl+0 shortcut to fit entire graph in view
- [ ] Add `centerOnGraph()` method to center view on content bounding box

**window.cpp:**
- [ ] Call `view->centerOnGraph()` after loading XML files
- [ ] Add menu item "View > Fit Graph" (Ctrl+0)
- [ ] Add constants for node spacing (HORIZONTAL_SPACING = 300, VERTICAL_SPACING = 180)
- [ ] Update node creation positions to use proper spacing

**generate_test_files.py (for testing):**
- [ ] Update spacing from 100px to 300px horizontal, 180px vertical
- [ ] Reduce jitter from ±20 to ±10 for cleaner layouts
- [ ] Fix UUID format: remove braces (use str(uuid.uuid4()) not "{...}")

**Test Plan:**
- [ ] Generate new test files with improved spacing
- [ ] Load tests_tiny.xml, tests_small.xml, tests_medium.xml
- [ ] Verify nodes don't overlap
- [ ] Verify edges have clear routing space
- [ ] Test with different node types (MERGE, SPLIT have more sockets)

**Git Workflow:**
```bash
git checkout main
git checkout -b refactor/ghost-edge-typed
# ... make changes ...
git add scene.h scene.cpp
git commit -m "refactor: replace ghost-edge casts with typed lookup

- add Scene::socketAt() helper using typed collections
- route ghost-edge hover/release through typed socket lookup
- keep magnet snapping + state reset paths on typed sockets
- eliminates final qgraphicsitem_cast occurrences"
git push origin refactor/ghost-edge-typed
git checkout main
git merge refactor/ghost-edge-typed --no-ff
git push origin main
git branch -d refactor/ghost-edge-typed
```

---

## Phase 3: Payload Infrastructure (Data Layer)

### Branch 3.1: Add Payload Storage to Node and Edge
**Branch:** `feature/node-edge-payload-storage`

**Purpose:** Add QVariantMap-based payload storage to nodes/edges with XML persistence

**Changes:**

**node.h:**
- [ ] Add `QVariantMap m_payload;` private member
- [ ] Add payload API methods
- [ ] Add serialization methods

**node.cpp:**
- [ ] Implement payload getters/setters
- [ ] Implement writePayload()/readPayload() for XML
- [ ] Wire into Node::write() and Node::read()

**edge.h / edge.cpp:**
- [ ] Same payload infrastructure as Node

**Test Plan:**
- [ ] Create node, set payload, save, reload
- [ ] Verify payload persists through XML round-trip
- [ ] Test multiple types: string, int, double, bool

**Git Workflow:**
```bash
git checkout main
git checkout -b refactor/ghost-edge-typed
# ... make changes ...
git add scene.h scene.cpp
git commit -m "refactor: replace ghost-edge casts with typed lookup

- add Scene::socketAt() helper using typed collections
- route ghost-edge hover/release through typed socket lookup
- keep magnet snapping + state reset paths on typed sockets
- eliminates final qgraphicsitem_cast occurrences"
git push origin refactor/ghost-edge-typed
git checkout main
git merge refactor/ghost-edge-typed --no-ff
git push origin main
git branch -d refactor/ghost-edge-typed
```

---

### Branch 3.2: Expose Payload API through Graph Facade
**Branch:** `feature/payload-facade-api`

**Purpose:** Make payload operations callable from JavaScript via Graph facade

**Changes:**

**graph.h:**
- [ ] Add Q_INVOKABLE setNodePayload/getNodePayload/getNodeAllPayload
- [ ] Add Q_INVOKABLE setEdgePayload/getEdgePayload/getEdgeAllPayload
- [ ] Add signals: nodePayloadChanged, edgePayloadChanged

**graph.cpp:**
- [ ] Implement node/edge payload methods using Scene::findNodeById/findEdgeById
- [ ] Emit signals on payload changes
- [ ] Trigger autosave on mutations

**Test Plan:**
- [ ] Test from JavaScript CLI
- [ ] Set/get payloads via facade
- [ ] Verify save/load persistence
- [ ] Verify autosave triggers

**Git Workflow:**
```bash
git checkout main
git checkout -b refactor/ghost-edge-typed
# ... make changes ...
git add scene.h scene.cpp
git commit -m "refactor: replace ghost-edge casts with typed lookup

- add Scene::socketAt() helper using typed collections
- route ghost-edge hover/release through typed socket lookup
- keep magnet snapping + state reset paths on typed sockets
- eliminates final qgraphicsitem_cast occurrences"
git push origin refactor/ghost-edge-typed
git checkout main
git merge refactor/ghost-edge-typed --no-ff
git push origin main
git branch -d refactor/ghost-edge-typed
```

---

## Phase 4: JavaScript Scripting Layer

### Branch 4.1: Node JavaScript Behavior Execution
**Branch:** `feature/node-javascript-behavior`

**Purpose:** Allow nodes to execute custom JavaScript code with access to their own context

**Design:**
- Nodes store JavaScript source as special payload key: `"__jsBehavior"`
- Graph facade passes its QJSEngine reference to nodes for execution
- Nodes get `node` object in JS context with id, type, payload

**Changes:**

**node.h:**
- [ ] Add setJavaScriptBehavior/getJavaScriptBehavior/executeJavaScript methods

**node.cpp:**
- [ ] Implement JavaScript execution with context injection
- [ ] Expose node object (id, type, payload, position) to scripts

**graph.h:**
- [ ] Add Q_INVOKABLE setNodeBehavior/getNodeBehavior/executeNodeScript

**graph.cpp:**
- [ ] Implement facade methods calling Node::executeJavaScript with m_jsEngine

**Test Plan:**
- [ ] Create test_node_scripting.js
- [ ] Set node behavior from JavaScript
- [ ] Execute scripts with context parameters
- [ ] Verify scripts persist through save/load
- [ ] Test error handling for bad scripts

**Git Workflow:**
```bash
git checkout main
git checkout -b refactor/ghost-edge-typed
# ... make changes ...
git add scene.h scene.cpp
git commit -m "refactor: replace ghost-edge casts with typed lookup

- add Scene::socketAt() helper using typed collections
- route ghost-edge hover/release through typed socket lookup
- keep magnet snapping + state reset paths on typed sockets
- eliminates final qgraphicsitem_cast occurrences"
git push origin refactor/ghost-edge-typed
git checkout main
git merge refactor/ghost-edge-typed --no-ff
git push origin main
git branch -d refactor/ghost-edge-typed
```

---

### Branch 4.2: Edge JavaScript Expressions
**Branch:** `feature/edge-javascript-expressions`

**Purpose:** Edges can evaluate JavaScript expressions with access to connected nodes

**Use Case:** Dynamic edge weights based on node properties

**Changes:**

**edge.h:**
- [ ] Add setExpression/getExpression/evaluateExpression methods

**edge.cpp:**
- [ ] Implement expression evaluation
- [ ] Expose fromNode, toNode, edge objects to expressions

**graph.h:**
- [ ] Add Q_INVOKABLE setEdgeExpression/getEdgeExpression/evaluateEdge

**graph.cpp:**
- [ ] Implement facade methods

**Test Plan:**
- [ ] Create nodes with weights
- [ ] Set edge expression: `fromNode.payload.weight + toNode.payload.weight`
- [ ] Verify evaluation returns correct sum
- [ ] Update node payload, verify re-evaluation works

**Git Workflow:**
```bash
git checkout main
git checkout -b refactor/ghost-edge-typed
# ... make changes ...
git add scene.h scene.cpp
git commit -m "refactor: replace ghost-edge casts with typed lookup

- add Scene::socketAt() helper using typed collections
- route ghost-edge hover/release through typed socket lookup
- keep magnet snapping + state reset paths on typed sockets
- eliminates final qgraphicsitem_cast occurrences"
git push origin refactor/ghost-edge-typed
git checkout main
git merge refactor/ghost-edge-typed --no-ff
git push origin main
git branch -d refactor/ghost-edge-typed
```

---

## Phase 5: Template Integration

### Branch 5.1: Default Payloads and Scripts in Templates
**Branch:** `feature/template-default-payloads`

**Purpose:** NodeTypeTemplates can specify default payloads and JavaScript behaviors

**Changes:**

**node_type_templates.h:**
- [ ] Update registerTemplate to accept defaultPayload parameter

**node_type_templates.cpp:**
- [ ] Define default behaviors for SOURCE, TRANSFORM, SINK
- [ ] Apply default payloads on node creation

**Test Plan:**
- [ ] Create SOURCE node, verify default payload/behavior applied
- [ ] Execute default behavior, verify it works
- [ ] Override default, verify custom behavior takes precedence

**Git Workflow:**
```bash
git checkout main
git checkout -b refactor/ghost-edge-typed
# ... make changes ...
git add scene.h scene.cpp
git commit -m "refactor: replace ghost-edge casts with typed lookup

- add Scene::socketAt() helper using typed collections
- route ghost-edge hover/release through typed socket lookup
- keep magnet snapping + state reset paths on typed sockets
- eliminates final qgraphicsitem_cast occurrences"
git push origin refactor/ghost-edge-typed
git checkout main
git merge refactor/ghost-edge-typed --no-ff
git push origin main
git branch -d refactor/ghost-edge-typed
```

---

## Phase 6: Examples and Documentation

### Branch 6.1: JavaScript Integration Examples
**Branch:** `test/javascript-integration-examples`

**Purpose:** Create comprehensive examples demonstrating JavaScript integration

**Changes:**
- [ ] Create examples/ directory
- [ ] examples/01_basic_payload.js
- [ ] examples/02_node_scripting.js
- [ ] examples/03_edge_expressions.js
- [ ] examples/04_data_pipeline.js
- [ ] Create JAVASCRIPT_API.md documenting all facade methods
- [ ] Update README.md with JavaScript integration section

**Git Workflow:**
```bash
git checkout main
git checkout -b refactor/ghost-edge-typed
# ... make changes ...
git add scene.h scene.cpp
git commit -m "refactor: replace ghost-edge casts with typed lookup

- add Scene::socketAt() helper using typed collections
- route ghost-edge hover/release through typed socket lookup
- keep magnet snapping + state reset paths on typed sockets
- eliminates final qgraphicsitem_cast occurrences"
git push origin refactor/ghost-edge-typed
git checkout main
git merge refactor/ghost-edge-typed --no-ff
git push origin main
git branch -d refactor/ghost-edge-typed
```

---

## Progress Tracking

### Phase 1: Architecture (qgraphicsitem_cast Elimination) 🔥 CRITICAL
- [x] Branch 1.1: refactor/socket-typed-parent (commit: 489305e) - 1 cast eliminated
- [x] Branch 1.2: refactor/node-typed-collections (commit: 8975361) - 2 casts eliminated
- [x] Branch 1.3: refactor/scene-typed-collections (commit: 43b4ccb) - infrastructure, 0 casts (already clean)
- [ ] Branch 1.4: refactor/delete-key-self-managed 🔥 (SKIP - risky, not required for final goal)
- [x] **Branch 1.5: refactor/ghost-edge-typed** (this branch) - eliminated 2 remaining casts (scene.cpp:326, 504)
- [ ] Branch 1.6: refactor/window-typed-queries (likely already done)
- [x] Branch 1.7: refactor/factory-typed-serialization (commit: 542a5e8, 128a1b3) - 1 cast eliminated + VS debugger fix

### Phase 2: Bug Fixes (Parallel to Phase 1)
- [ ] Branch 2.1: fix/graph-save-to-file
- [ ] Branch 2.2: fix/node-position-precision
- [x] Branch 2.3: fix/load-rollback (commit: fix/load-rollback-memory-leak) - ASAN validated ✓
- [x] Branch 2.4: remove/unused-auto-layout (commit: 8f0ad98) - 2 casts eliminated, 306 lines removed
- [x] Branch 2.5: feature/edge-z-order-enhancements (basic implementation, commit: 2b7bb36)

### Phase 3: Payload Infrastructure
- [ ] Branch 3.1: feature/node-edge-payload-storage
- [ ] Branch 3.2: feature/payload-facade-api

### Phase 4: JavaScript Scripting
- [ ] Branch 4.1: feature/node-javascript-behavior
- [ ] Branch 4.2: feature/edge-javascript-expressions

### Phase 5: Template Integration
- [ ] Branch 5.1: feature/template-default-payloads

### Phase 6: Examples & Documentation
- [ ] Branch 6.1: test/javascript-integration-examples

---

## Success Criteria

**JavaScript integration is complete when:**
- [x] Graph facade exists with QJSEngine (DONE)
- [ ] Nodes can execute custom JavaScript
- [ ] Edges can evaluate expressions
- [ ] Payloads persist through save/load
- [ ] All APIs callable from `--script`
- [ ] Comprehensive examples work
- [ ] Documentation complete

**Final Vision:**
```javascript
// Create fully programmable graph from JavaScript
var source = graph.createNode("SOURCE", 0, 0, 1);
graph.setNodeBehavior(source, `
    this.value = (this.value || 0) + 1;
    return this.value;
`);

var transform = graph.createNode("TRANSFORM", 200, 0, 1);
graph.setNodePayload(transform, "multiplier", 2);
graph.setNodeBehavior(transform, `
    return input * node.payload.multiplier;
`);

var edge = graph.connectNodes(source, 0, transform, 0);
graph.setEdgeExpression(edge, `
    fromNode.payload.weight || 1.0
`);

// Execute graph
for (var i = 0; i < 5; i++) {
    var val = graph.executeNodeScript(source);
    var result = graph.executeNodeScript(transform, {input: val});
    console.log("Step", i, ":", val, "->", result);
}

// Save entire scripted graph
graph.saveToFile("my_program.xml");
```

---

## Deferred Low-Priority Items

**From ISSUE.md (Codex analysis):**

1. **Harmonize Template Names**
   - [ ] Audit callers of GraphFactory::createNode for wrong type strings
   - [ ] Likely already fixed - wrong names were only in removed test code
   - [ ] Verify palette and shortcuts use correct uppercase names

2. **Tame Diagnostic Noise**
   - [ ] Create NG_VERBOSE environment variable for logging control
   - [ ] Route qDebug() calls through logging helper
   - [ ] Replace QMessageBox notifications with status bar updates
   - Low priority - current logging useful for debugging

---

## Operational Rules

1. **Build/test after every branch** - Never merge broken code
2. **Commit frequently** - After each logical change
3. **Tag milestones** - After each phase complete
4. **One feature per branch** - Keep changes focused
5. **Always test from JavaScript** - Use `--script` to verify APIs
6. **Update this plan** - Mark [x] when complete, add notes

---

**Next Step:** Start Phase 1, Branch 1.1 - Socket Typed Parent (eliminate qgraphicsitem_cast)

- [x] `fix/memory-safety-step-2`: temporarily instrument node/edge constructors/destructors to log instance counts and warn when items are destroyed while still registered; remove once cleanup notifications land.
- [x] `fix/memory-safety-step-3`: route all destruction through `Scene::notifyNodeDestroyed/notifyEdgeDestroyed`, let `Scene::clear()` rely on those callbacks instead of pre-clearing hashes, and verify logs stay clean before removing the stop-gap validation.
- [x] `fix/memory-safety-step-4`: remove (or permanently disable) the O(n^2) autosave validation loop, rely on the new notifications for integrity, and keep a compile-time switch only for emergency diagnostics.
- [x] `fix/memory-safety-step-5`: keep the UUID hash maps (documented decision) because they provide O(1) lookups for scripts/undo; no further refactor needed now that destruction notifications keep them consistent.
- [ ] `fix/memory-safety-step-6`: documentation & cleanup pass—remove temporary diagnostics once scripting scenarios prove stability, describe the destruction/observer model in README/PLAN, and note the optional autosave guard.
- Static `Node::s_instanceCount` / `Edge::s_instanceCount` diagnostics stay enabled until we have long-running scripted tests; do not remove them without replacing with an equivalent leak detector.
## CMake & Build System Follow-up (Nov 2025)
- [] Replace hard-coded Qt Windows paths with user-configurable variables or documented environment overrides (QT_ROOT_DIR, Qt5_DIR, CMAKE_PREFIX_PATH).
- [] Move away from global add_definitions()/BUILD_SHARED_LIBS; prefer target_compile_definitions and isolate FetchContent options.
- [] Reintroduce a minimal test target (even a CLI smoke test) and wire add_test() back up.
- [] Standardize CMAKE_BUILD_TYPE handling for MSVC/multi-config generators via generator expressions.


## UI Workflow Improvements (branches)
- [] Define a shared selection stack (scratchpad) that receives copies/deletes so undo/redo and copy/paste work off the same structure; document workflow for contributors.
- [] `feature/ui-undo-redo`: integrate a QUndoStack (or custom command pattern) so Delete/Insert operations can be undone/redone.
- [] `feature/ui-copy-paste`: implement selection copy/paste plus a scratchpad flow for storing/restoring mini-graphs.
- [] `feature/ui-graticule`: add a grid overlay toggle, persistent preference, and log entry when toggled.
- [] `feature/ui-polish`: selection highlighting, status bar cues, better tooltips, and professional theming.
