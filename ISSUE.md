# NodeGraph Improvement Plan

## Harmonize Template Names
- Audit every caller of `GraphFactory::createNode` (smoke tests, palette, shortcuts) for literal node-type strings.
- Pick a single canonical naming style (likely uppercase) and either normalize inputs centrally or add alias support in `NodeTypeTemplates`.
- **Test:** Execute the force-layout and palette smoke routines; they should successfully instantiate nodes without unknown-type warnings.

## Guarantee Load Rollback
- `graph_factory.cpp:61` + `graph_factory.cpp:517-528` currently push nodes/edges into `Scene` during Phase 2, but `loadFromXmlFile` returns `false` on Phase-3 validation failures without unwinding; we end up with half-loaded objects while the caller believes the load failed.
- Keep nodes/edges created during `GraphFactory::loadFromXmlFile` isolated until validation finishes, or explicitly roll back the scene/registries before returning `false`.
- Always pair `GraphSubject::beginBatch()`/`endBatch()` even when aborting so observers resume in a known state.
- **Test:** Load an XML containing duplicate socket connections and confirm the scene remains unchanged after a reported failure; follow with a valid file to verify normal loading.

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
