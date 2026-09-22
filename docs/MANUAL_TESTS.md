# NodeGraph Manual Test Checklists

Aircraft-style checklists for hands-on verification of NodeGraph (Debug build,
post-remediation state 2026-07-22). Work top to bottom. Tick each box only
when the EXPECTED result is exactly what you see. If a step fails, STOP —
note the set/item number and the newest file in `logs/` before continuing.

Traceability: bracketed codes reference the fix each step verifies
(see `ADVISORY.md` Entry 004).

Legend: `[RMB-drag]` = hold right mouse button on a socket and drag.

---

## PREFLIGHT

- [ ] Build is current: `cmake --build build_Debug --config Debug --parallel` → success.
- [ ] Launch: `build_Debug\Debug\NodeGraph.exe` → window opens, no error dialogs.
- [ ] Status bar shows node count 0, edge count 0.

---

## SET A — STARTUP / SHUTDOWN

- [ ] A.1 App opens with empty scene, palette docked on the left, menus present.
- [ ] A.2 Close with the X button → exits cleanly, no crash, no hang. [B4]
- [ ] A.3 Relaunch with `NodeGraph.exe --script scripts\smoke_fixes.js` → wait ~20 s, close → newest `logs\NodeGraph_*.log` contains `SMOKE DONE PASS=12 FAIL=0`. [regression gate]

---

## SET B — NODE CREATION

- [ ] B.1 Press Ctrl+2 → SINK node appears near view center; status bar nodes = 1.
       (Use Ctrl+2/Ctrl+3 here; Ctrl+1 has a known shortcut conflict — see KNOWN LIMITATIONS.)
- [ ] B.2 Drag "SOURCE" from the palette onto the canvas → node appears at the drop point.
- [ ] B.3 Press Ctrl+3 → TRANSFORM node appears; node height fits its socket rows.

---

## SET C — CONNECTIONS (RMB-drag)

- [ ] C.1 RMB-drag from SOURCE output (right side) to TRANSFORM input (left side) → edge appears; both sockets show connected state.
- [ ] C.2 RMB-drag from TRANSFORM output to SINK input → second edge appears. You now have a pass-through chain. [A1]
- [ ] C.3 Try a second edge from the same SOURCE output to anywhere → refused; log shows "socket already connected". [B2]
- [ ] C.4 Try RMB-drag from TRANSFORM **input** to SOURCE **output** (wrong roles) → refused, no edge.
- [ ] C.5 Try RMB-drag from a TRANSFORM output back into the same node's input (self-loop) → refused.
- [ ] C.6 RMB-drag and release over empty canvas → ghost edge disappears; source socket is NOT stuck highlighted. [A3]
- [ ] C.7 Start an RMB-drag, keep holding it, press Ctrl+N (File→New) with the other hand, keep moving the mouse, then release → **no crash**; scene is empty; no ghost remains. [A3 — the critical manual check]

---

## SET D — SAVE / LOAD

- [ ] D.1 Build the chain from SET C. Ctrl+S → save as `test1.xml`. Dialog confirms save.
- [ ] D.2 Ctrl+N → scene empties, counts 0/0.
- [ ] D.3 Ctrl+O → open `test1.xml` → exactly 3 nodes / 2 edges return, positions preserved. [A1]
- [ ] D.4 Ctrl+S again, Ctrl+N, Ctrl+O again → identical result (round-trip stability).
- [ ] D.5 Add one extra node, then Ctrl+O `test1.xml` → the extra node is GONE; the file's 3 nodes are the whole scene (load replaces, never merges). [C6]

---

## SET E — DELETE / UNDO / REDO

- [ ] E.1 Select the TRANSFORM, press Del → node and BOTH its edges disappear (cascade).
- [ ] E.2 Ctrl+Z → node and both edges restored; edges reconnect to the same sockets.
- [ ] E.3 Ctrl+Y (or Ctrl+Shift+Z) → deleted again.
- [ ] E.4 Rubber-band select everything, Del, then Ctrl+Z → all nodes and edges restored.
- [ ] E.5 Drag a node to a new spot, Ctrl+Z → it returns to the old spot (move undo).
- [ ] E.6 After several mixed operations, undo to the bottom, then redo to the top → no zombie items, no duplicates, counts consistent throughout.

---

## SET F — SCRIPTING

- [ ] F.1 Shift+Left-click a node → context menu offers script actions → "Run Scripts (All Nodes)" → log shows `NODE SUMMARY {...}` lines from the default node scripts.
- [ ] F.2 Hand-edit `test1.xml`: add `<script language="javascript">graph.deleteNode(node.nodeId());</script>` inside one node. Load it, run scripts → the node SURVIVES; log shows the refusal error. [B3]
- [ ] F.3 Same, with script `while (1) {}` → run → UI freezes ~5 s, log shows "Error: Interrupted", app is responsive again. [C2]
- [ ] F.4 Same, with script `graph.executeNodeScript(node.nodeId(), {});` → run → warnings in log (depth cap), app alive. [C1]

---

## SET G — AUTOSAVE

- [ ] G.1 Create/change a node, wait ~3 s → log shows an autosave write; `autosave.xml` exists and contains the node. [C5]
- [ ] G.2 Close the app cleanly → open `autosave.xml` in a text editor → it STILL contains your graph (not an empty `<graph/>`). [A2 — the big one]
- [ ] G.3 Relaunch, Ctrl+O `autosave.xml` → your graph loads back.

---

## SET H — VISUAL / RENDER

- [ ] H.1 Move a connected node so a socket sits exactly at scene coordinate (0,0) (top-left corner) → its edge stays drawn. [A6]
- [ ] H.2 Drag nodes around vigorously → edges follow, no stale edge lines.
- [ ] H.3 Zoom in/out with the wheel → grid and items scale; app stays responsive. (Do not zoom to extremes — see KNOWN LIMITATIONS.)

---

## KNOWN LIMITATIONS (do not report — deferred, ADVISORY.md Entry 004 §4)

- Ctrl+1 is double-bound (Add Input vs Reset Zoom) → Reset Zoom unreachable by key.
- Palette button single/double-click does nothing; only drag works.
- Five menu actions are unconnected (Select All, Deselect, Delete, Validate, Statistics).
- Selected nodes can leave faint glow fragments when moved (paint outside boundingRect).
- Zoom is unclamped; extreme zoom-out can slow repaints.
- `autosave.xml` is written in the locale codec — non-ASCII scripts/labels may corrupt.
- Files containing duplicate node/edge UUIDs are accepted (should be rejected).

---

## SIGN-OFF

| Set | Items passed | Tester | Date | Build |
|---|---|---|---|---|
| A | __ / 3 | | | |
| B | __ / 3 | | | |
| C | __ / 7 | | | |
| D | __ / 5 | | | |
| E | __ / 6 | | | |
| F | __ / 4 | | | |
| G | __ / 3 | | | |
| H | __ / 3 | | | |

Failures (set.item — what happened — log file): ______________________
