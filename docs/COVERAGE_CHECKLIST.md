# NodeGraph — LLVM Coverage Preflight Checklist

Aircraft-style preflight checklist for manual LLVM source-based code coverage
collection. Run the instrumented binary, work through every item, close the app.
The instrumented binary records every code path you touch into `.profraw`.

**Traceability:** Each set targets specific low-coverage files from the JS-only
baseline (undo_commands.cpp 0%, ghost_edge.cpp 0%, view.cpp 9%, window.cpp 12%).

**Legend:** `[RMB-drag]` = right-click-and-hold on a socket, drag to target.

---

## SETUP — Before you begin

- [ ] **S.1** Open PowerShell, navigate to repo root.
- [ ] **S.2** Run: `.\scripts\run_coverage_manual.ps1`
  → Window opens with "LLVM Coverage Manual Run" in the title bar.
  → profraw path shown in the console.
- [ ] **S.3** Confirm status bar shows "Nodes: 0  Edges: 0".

---

## SET A — NODE CREATION (targets: node.cpp, node_templates.cpp, graph_factory.cpp)

- [ ] **A.1** Double-click an empty area → SOURCE node appears near click. [createNode]
- [ ] **A.2** Drag "SINK" from palette onto canvas → SINK at drop point. [palette drag]
- [ ] **A.3** Drag "TRANSFORM" from palette → TRANSFORM appears.
- [ ] **A.4** Drag "SCRIPT" from palette → SCRIPT node appears.
- [ ] **A.5** Drag "SPLIT" from palette → SPLIT node appears.
- [ ] **A.6** Drag "MERGE" from palette → MERGE node appears.
- [ ] **A.7** Status bar: Nodes = 6.

---

## SET B — CONNECTIONS / GHOST EDGES (targets: edge.cpp, ghost_edge.cpp, socket.cpp)

- [ ] **B.1** [RMB-drag] from SOURCE output (right dot) to TRANSFORM input (left dot).
  → During drag: ghost edge follows mouse cursor. [ghost_edge.cpp]
  → On release: solid bezier edge appears. socket highlight clears. [edge.cpp]
- [ ] **B.2** [RMB-drag] from TRANSFORM output to SINK input → second edge.
- [ ] **B.3** [RMB-drag] from a SOCKET to **empty canvas** and release.
  → Ghost edge disappears. Source socket is NOT stuck highlighted. [ghost_edge destructor]
- [ ] **B.4** [RMB-drag] from SINK **input** backward to TRANSFORM **output**.
  → Refused (wrong role). No edge created.
- [ ] **B.5** [RMB-drag] from SOURCE output to TRANSFORM input while it's already connected.
  → Refused (socket already connected). Status bar shows error.
- [ ] **B.6** [RMB-drag] from TRANSFORM output back to TRANSFORM input (self-loop).
  → Refused. No edge.

---

## SET C — NODE MOVE / SELECTION (targets: scene.cpp, node.cpp, view.cpp)

- [ ] **C.1** Left-click a node → node highlights (selection glow). [node::paint selected]
- [ ] **C.2** Left-click-drag a node to a new position → node follows mouse.
- [ ] **C.3** Left-click empty canvas → selections clear.
- [ ] **C.4** Rubber-band select (left-click empty area, drag box) over 2+ nodes →
  multiple nodes highlighted. [scene selection]
- [ ] **C.5** Ctrl+click a node → toggles its selection without clearing others.
- [ ] **C.6** Hold Shift, left-click-drag all 6 nodes to a corner of the view →
  all selected nodes move together.

---

## SET D — DELETE / UNDO / REDO (targets: undo_commands.cpp, graph.cpp)

- [ ] **D.1** Select one TRANSFORM node, press **Delete**.
  → Node + its edges disappear. Status bar updates. [deleteNode cascade]
- [ ] **D.2** Press **Ctrl+Z** (Undo).
  → Node AND both edges return. Edges reconnect to same sockets. [undo_commands.cpp]
- [ ] **D.3** Press **Ctrl+Y** (Redo).
  → Node + edges deleted again.
- [ ] **D.4** Ctrl+Z again to restore. Move a node 100px right. Ctrl+Z.
  → Node moves back to original position. [moveNode undo]
- [ ] **D.5** Rubber-band select everything, Delete → all gone. Ctrl+Z → all return.
- [ ] **D.6** Do 5 mixed operations (create, delete, move, connect).
  Press Ctrl+Z 5 times. Press Ctrl+Y 5 times.
  → Each undo/redo step restores exactly the right state. Counts stay consistent.

---

## SET E — FILE SAVE / LOAD / NEW (targets: graph_factory.cpp, xml_autosave_observer.cpp, window.cpp)

- [ ] **E.1** Create a chain: SOURCE → TRANSFORM → SINK (2 edges).
  Ctrl+S → "Save As" dialog. Save as `logs\preflight_test.xml`. [saveToFile]
- [ ] **E.2** Ctrl+N (File→New) → confirm dialog, scene clears, counts 0/0. [clearGraph]
- [ ] **E.3** Ctrl+O → open `logs\preflight_test.xml`.
  → 3 nodes, 2 edges return at original positions. [loadFromFile]
- [ ] **E.4** Ctrl+O again → no change, stable reload.
- [ ] **E.5** Wait 3 seconds → check `autosave.xml` was written in repo root.
  [xml_autosave_observer.cpp]

---

## SET F — ZOOM / VIEW (targets: view.cpp)

- [ ] **F.1** Scroll wheel up → scene zooms in. [view::wheelEvent]
- [ ] **F.2** Scroll wheel down → scene zooms out.
- [ ] **F.3** Ctrl+0 (if bound) or View→Reset Zoom menu → zoom resets to 1:1.

---

## SET G — CONTEXT MENUS (targets: window.cpp, scene.cpp)

- [ ] **G.1** Right-click a SCRIPT node → context menu appears.
  → "Edit Script..." option visible. [scene::contextMenuEvent]
- [ ] **G.2** Click "Edit Script..." → script dialog opens.
  → Type `42;`, click OK. Dialog closes.
- [ ] **G.3** Right-click the SCRIPT node again → "Run Script" → log message appears.
  [window::onRunScript]
- [ ] **G.4** Right-click the SCRIPT node → "Delete" → node removed.
  → Same as pressing Delete — verify undo works.

---

## SET H — MENU BAR (targets: window.cpp)

- [ ] **H.1** File → New → confirm → clears scene.
- [ ] **H.2** Edit → Undo → restores previous state (verify menu Undo works too).
- [ ] **H.3** Edit → Redo → re-does the clear.
- [ ] **H.4** Help → About → dialog shows, click OK.

---

## SHUTDOWN

- [ ] **Z.1** Close the window (X button or Alt+F4).
  → Console shows "LLVM coverage profraw written".
- [ ] **Z.2** The PowerShell script automatically merges + generates HTML report.
  → Open: `build_llvm\coverage_report\index.html`

---

## SIGN-OFF

| Set | Items | Passed | Tester | Date | Build |
|-----|-------|--------|--------|------|-------|
| S   | 3     | __ / 3 |        |      |       |
| A   | 7     | __ / 7 |        |      |       |
| B   | 6     | __ / 6 |        |      |       |
| C   | 6     | __ / 6 |        |      |       |
| D   | 6     | __ / 6 |        |      |       |
| E   | 5     | __ / 5 |        |      |       |
| F   | 3     | __ / 3 |        |      |       |
| G   | 4     | __ / 4 |        |      |       |
| H   | 4     | __ / 4 |        |      |       |
| Z   | 2     | __ / 2 |        |      |       |
| **TOTAL** | **46** | __ / 46 |

Failures (set.item — what happened): _________________________________________

---

## Expected coverage improvement (vs JS-only baseline)

| File | JS-only | After preflight | Why |
|------|---------|-----------------|-----|
| `undo_commands.cpp` | 0% | **~70%+** | D.1-D.6 exercises all undo/redo |
| `ghost_edge.cpp` | 0% | **~80%+** | B.1-B.3 exercises ghost edge lifecycle |
| `view.cpp` | 9% | **~40%+** | zoom, rubber-band, scroll |
| `window.cpp` | 12% | **~50%+** | menus, dialogs, shortcuts |
| `scene.cpp` | 33% | **~55%+** | selection, context menus |
| `node.cpp` | 68% | **~80%+** | double-click create, select paint |
| `socket.cpp` | 20% | **~50%+** | connected state, highlight |
| `graph_factory.cpp` | 51% | **~70%+** | save/load round-trip via menus |
