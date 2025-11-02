# NodeGraph Improvement Plan

## Harmonize Template Names
- Audit every caller of `GraphFactory::createNode` (smoke tests, palette, shortcuts) for literal node-type strings.
- Pick a single canonical naming style (likely uppercase) and either normalize inputs centrally or add alias support in `NodeTypeTemplates`.
- **Test:** Execute the force-layout and palette smoke routines; they should successfully instantiate nodes without unknown-type warnings.

## Guarantee Load Rollback ⚠️ PARTIAL - Memory Leak
- **Status:** Phase 2 implementation complete (objects created in memory), but cleanup missing on validation failure.
- `graph_factory.cpp:463, 482, 517, 528` - validation failure paths call `endBatch()` and `return false` but don't delete the `allNodes` and `allEdges` vectors.
- **Memory Leak:** On validation failure, Node* and Edge* objects remain allocated but orphaned (not in scene, not deleted).
- **Fix Needed:** Add cleanup loops before each `return false`: `for (Node* n : allNodes) delete n; for (Edge* e : allEdges) delete e;`
- `GraphSubject::beginBatch()`/`endBatch()` pairing is correct.
- **Test:** Load an XML containing duplicate socket connections and confirm the scene remains unchanged after a reported failure; follow with a valid file to verify normal loading. Use Valgrind/ASAN to verify no memory leaks.

## Preserve Precise Node Positions
- `node.cpp:123` only updates `m_lastPos` if movement exceeds a 5px manhattan delta, yet `Node::write()` (`node.cpp:434-435`) persists `m_lastPos`; small drags or scripted nudges serialize stale coordinates.
- Drop the dead zone or serialize `pos()` directly so saved XML matches the live scene.
- **Test:** Place a node, nudge it <5px, save, reload, and confirm the node stays where the user left it.

## Make `Graph::saveToFile` Real
- `graph.cpp:371-377` exposes `Graph::saveToFile` through the facade/JS API but it only emits `graphSaved` and returns `true`; no document ever hits disk.
- Wire this call into the real serialization path (or fail fast) so scripts and future integrations can depend on it.
- **Test:** Invoke `graph.saveToFile("tmp.xml")` from the facade/JS environment and verify the file exists and round-trips via `loadFromFile`.

## Tame Diagnostic Noise
- Introduce a controllable logging helper (e.g., `NG_VERBOSE`) and route current `qDebug()` calls through it.
- Default the helper to quieter behavior for regular builds while keeping a verbose mode for agent-driven debugging; replace routine `QMessageBox` notifications with status-bar updates unless verbose mode is enabled.
- **Test:** Run smoke/test flows twice—once with verbose mode enabled to confirm rich diagnostics, once with it disabled to ensure a quiet UX.

## Extend Node & Edge Payloads
- Capture a minimal payload contract (e.g., display name, numeric weight) and add storage members to `Node`/`Edge`.
- Expand XML templates and serializers so payload defaults flow from `NodeTypeTemplates` and persist through `Node::write/read` and `Edge::write/read`.
- Update `GraphFactory` creation paths and the `Graph` facade to accept/return payload data, emitting observer signals on mutation.
- Surface payload editing in the UI (property inspector or status details) using the existing selection hooks.
- **Test:** Create fixture XML with payload fields, load/save to verify round-tripping; drive facade methods (unit or QTest) to mutate payloads and confirm observer callbacks plus serialization updates.

## Improve View Scaling & Scrollbars
- Enable antialiasing and smooth pixmap transforms on the `View` so zooming looks clean.
- Track zoom level, clamp wheel-based scaling between sensible min/max values, and anchor zoom under the mouse so the scene stays in view.
- Ensure horizontal/vertical scrollbars appear as needed and the scene rect updates so users can reach all items after large zoom operations.
- **Test:** Load `tests_large.xml`, zoom/pan, resize the window, and verify the graph remains visible with usable scrollbars.

## Improve Edge Visual Ordering & Separation
- **Problem:** Multiple edges converging on nodes (e.g., MERGE nodes with many inputs) overlap perfectly, making it hard to distinguish individual connections (see `edgeoverlap.png`).
- All edges currently at z-level 2, causing random overlap ordering.
- **Quick Wins:**
  1. **Selection Z-Order Boost:** Selected edges jump to z-level 4, bringing them to front for visibility (`edge.cpp` - 5 min fix).
  2. **Stacked Edge Z-Order:** Edges with more connections get slightly higher z-values for natural stacking (`edge.cpp` - 15 min fix).
  3. **Curve Offset for Parallel Edges:** Add perpendicular offset to control points for edges between same nodes, creating fan-out effect (`edge.cpp` - 30-60 min fix).
- **Note:** Full edge routing (path planning around obstacles) is deferred - these are visual ordering improvements only.
- **Test:** Load graph with MERGE/SPLIT nodes with multiple connections, verify edges are distinguishable; select edge, verify it comes to front.
