// tests/coverage_suite.js
// Comprehensive test suite for LLVM source-based coverage instrumentation.
// Exercises every Q_INVOKABLE method on the Graph facade.
// Run: NodeGraph.exe --script tests/coverage_suite.js
// Exits cleanly via graph.quit() so .profraw is written.

var passes = 0, fails = 0;
function test(name, cond, detail) {
    if (cond) { passes++; console.log("PASS " + name); }
    else      { fails++;  console.log("FAIL " + name + " " + (detail || "")); }
}
function assert(cond, msg) { if (!cond) throw new Error(msg || "assertion failed"); }

console.log("=== LLVM COVERAGE TEST SUITE ===");

// ── Validation ──────────────────────────────────────────
(function() {
    console.log("--- Validation ---");
    var types = graph.getAvailableNodeTypes();
    test("valid_types", types.indexOf("SOURCE") !== -1 && types.indexOf("SINK") !== -1 &&
                         types.indexOf("TRANSFORM") !== -1 && types.indexOf("SPLIT") !== -1 &&
                         types.indexOf("MERGE") !== -1 && types.indexOf("SCRIPT") !== -1,
         "got: " + types.join(","));
    test("valid_node_type_SOURCE", graph.isValidNodeType("SOURCE") === true);
    test("valid_node_type_BOGUS", graph.isValidNodeType("BOGUS") === false);
    test("valid_node_type_empty", graph.isValidNodeType("") === false);
})();

// ── Node Creation ───────────────────────────────────────
(function() {
    console.log("--- Node Creation ---");
    graph.clearGraph();

    var src = graph.createNode("SOURCE", 100, 150);
    test("create_SOURCE", src !== "");
    test("create_BOGUS", graph.createNode("BOGUS", 0, 0) === "");

    var snk = graph.createNode("SINK", 700, 150);
    test("create_SINK", snk !== "");

    var tr = graph.createNode("TRANSFORM", 300, 150);
    test("create_TRANSFORM", tr !== "");

    var sp = graph.createNode("SPLIT", 300, 300);
    test("create_SPLIT", sp !== "");

    var mg = graph.createNode("MERGE", 500, 300);
    test("create_MERGE", mg !== "");

    var sc = graph.createNode("SCRIPT", 500, 450);
    test("create_SCRIPT", sc !== "");

    var stats = graph.getGraphStats();
    test("create_count_6", stats.nodeCount === 6, "got " + stats.nodeCount);

    // getNodeData
    var data = graph.getNodeData(src);
    test("getNodeData_type", data.type === "SOURCE");
    test("getNodeData_id", data.id === src);
    test("getNodeData_pos", data.x === 100 && data.y === 150);

    // getNodeData nonexistent
    var bad = graph.getNodeData("00000000-0000-0000-0000-000000000000");
    test("getNodeData_bad", bad === null || Object.keys(bad).length === 0);
})();

// ── Edge Connection ─────────────────────────────────────
(function() {
    console.log("--- Edge Connection ---");
    graph.clearGraph();

    var src = graph.createNode("SOURCE", 0, 0);
    var tr  = graph.createNode("TRANSFORM", 200, 0);
    var snk = graph.createNode("SINK", 400, 0);

    var e1 = graph.connectNodes(src, 0, tr, 0);   // SOURCE out 0
    test("edge_create", e1 !== "");

    var e2 = graph.connectNodes(tr, 1, snk, 0);   // TRANSFORM out is global 1
    test("edge_create2", e2 !== "");

    // Duplicate output connection refused
    var dup = graph.connectNodes(src, 0, snk, 0);
    test("edge_dup_refused", dup === "");

    // Invalid indices
    var bad1 = graph.connectNodes(src, 99, tr, 0);
    test("edge_bad_srcIdx", bad1 === "");
    var bad2 = graph.connectNodes(src, 0, tr, 99);
    test("edge_bad_dstIdx", bad2 === "");
    // Role mismatch: TRANSFORM's global 0 is its INPUT, not an output
    var badRole = graph.connectNodes(tr, 0, snk, 0);
    test("edge_bad_role", badRole === "");

    // Nonexistent nodes
    var bad3 = graph.connectNodes("deadbeef-dead-beef-dead-beefdeadbeef", 0, tr, 0);
    test("edge_bad_srcNode", bad3 === "");

    var stats = graph.getGraphStats();
    test("edge_count_2", stats.edgeCount === 2);

    // getEdgeData reports the same global indices connectNodes consumes
    var ed = graph.getEdgeData(e1);
    test("getEdgeData_fromNode", ed.fromNode === src);
    test("getEdgeData_toNode", ed.toNode === tr);
    test("getEdgeData_fromSocketIndex", ed.fromSocketIndex === 0);
    test("getEdgeData_toSocketIndex", ed.toSocketIndex === 0);
    // Nonexistent edge: empty map (undefined/null/{} depending on the bridge)
    var badEdge = graph.getEdgeData("00000000-0000-0000-0000-000000000000");
    test("getEdgeData_bad", badEdge === null || badEdge === undefined || Object.keys(badEdge).length === 0);

    // getNodeEdges
    var edges = graph.getNodeEdges(tr);
    test("getNodeEdges_count", edges.length === 2, "got " + edges.length);

    // deleteEdge
    test("deleteEdge_ok", graph.deleteEdge(e2) === true);
    test("deleteEdge_gone", graph.deleteEdge(e2) === false);

    graph.deleteNode(src); // cascades to e1
    var stats2 = graph.getGraphStats();
    test("deleteNode_cascade", stats2.edgeCount === 0);
})();

// ── Scripted Drop ───────────────────────────────────────
(function() {
    console.log("--- Scripted Drop ---");
    graph.clearGraph();

    var dropped = graph.dropNode("SINK", 640, 360, 60);
    test("dropNode_created", dropped !== "" && graph.getNodeData(dropped).id === dropped);

    var droppedDefault = graph.dropNode("SOURCE", 100, 100);
    test("dropNode_default_duration", droppedDefault !== "");
    test("dropNode_bad_type", graph.dropNode("NOPE", 0, 0, 60) === "");
})();

// ── Node Mutation ───────────────────────────────────────
(function() {
    console.log("--- Node Mutation ---");
    graph.clearGraph();

    var n = graph.createNode("TRANSFORM", 50, 50);

    test("moveNode_ok", graph.moveNode(n, 10, 20) === true);
    var d1 = graph.getNodeData(n);
    test("moveNode_pos", d1.x === 60 && d1.y === 70);

    test("setNodePosition_ok", graph.setNodePosition(n, 200, 300) === true);
    var d2 = graph.getNodeData(n);
    test("setNodePosition_pos", d2.x === 200 && d2.y === 300);

    test("moveNode_bad", graph.moveNode("bad-bad-bad", 0, 0) === false);
    test("setNodePosition_bad", graph.setNodePosition("bad", 0, 0) === false);

    test("deleteNode_ok", graph.deleteNode(n) === true);
    test("deleteNode_twice", graph.deleteNode(n) === false);
})();

// ── Batch Operations ────────────────────────────────────
(function() {
    console.log("--- Batch Operations ---");
    graph.clearGraph();

    test("batch_not_active", graph.isBatchMode() === false);

    graph.beginBatch();
    test("batch_active", graph.isBatchMode() === true);

    var n1 = graph.createNode("SOURCE", 0, 0);
    var n2 = graph.createNode("SINK", 100, 0);
    var e1 = graph.connectNodes(n1, 0, n2, 0);

    test("batch_creates_work", n1 !== "" && n2 !== "" && e1 !== "");

    graph.endBatch();
    test("batch_ended", graph.isBatchMode() === false);

    var stats = graph.getGraphStats();
    test("batch_post_count", stats.nodeCount === 2 && stats.edgeCount === 1);
})();

// ── Scripted Nodes ──────────────────────────────────────
(function() {
    console.log("--- Scripted Nodes ---");
    graph.clearGraph();

    var sc = graph.createNode("SCRIPT", 100, 100);
    test("script_set", graph.setNodeScript(sc, "1 + 1;") === true);
    test("script_get", graph.getNodeScript(sc) === "1 + 1;");

    var payload = {key: "val", num: 42};
    test("payload_set", graph.setNodePayload(sc, payload) === true);
    var got = graph.getNodePayload(sc);
    test("payload_get", got.key === "val" && got.num === 42);

    // Execute: write to payload. node.payload() is a property map, not JSON,
    // and script bodies need an explicit return (they compile as a function).
    graph.setNodeScript(sc, "node.setPayloadValue('executed', true); return 'ok';");
    var result = graph.executeNodeScript(sc, {input: "hello"});
    test("script_execute", result === "ok");
    var updated = graph.getNodePayload(sc);
    test("script_execute_sidefx", updated.executed === true);

    // Execute on non-scripted node
    var src = graph.createNode("SOURCE", 300, 100);
    test("script_on_source", graph.setNodeScript(src, "return 42;") === true);
    var r2 = graph.executeNodeScript(src, {});
    test("script_source_exec", r2 === 42);

    // Bad node
    test("script_bad_node", graph.setNodeScript("bad", "x") === false);
    test("payload_bad_node", graph.setNodePayload("bad", {}) === false);
})();

// ── Synthetic Work ──────────────────────────────────────
(function() {
    console.log("--- Synthetic Work ---");

    var h = graph.runSyntheticWork({task: "hash", payload: "hello world"});
    test("work_hash", h.result && h.result.length > 0, "result=" + h.result);
    test("work_hash_status", h.status === "ok");

    var l = graph.runSyntheticWork({task: "loop", iterations: 200000});
    test("work_loop", l.durationMs > 0, "durationMs=" + l.durationMs);
    test("work_loop_iterations", l.iterations === 200000);

    var d = graph.runSyntheticWork({task: "delay", delayMs: 10});
    test("work_delay", d.delayMs === 10);

    var bad = graph.runSyntheticWork({task: "bogus"});
    test("work_bad", bad.status === "error" && bad.error !== undefined, "error=" + bad.error);
})();

// ── XML Serialization ───────────────────────────────────
(function() {
    console.log("--- XML Serialization ---");
    graph.clearGraph();

    var src = graph.createNode("SOURCE", 100, 200);
    var tr  = graph.createNode("TRANSFORM", 300, 200);
    var snk = graph.createNode("SINK", 500, 200);
    graph.connectNodes(src, 0, tr, 0);
    graph.connectNodes(tr, 1, snk, 0);

    var xml = graph.toXml();
    test("toXml_not_empty", xml.length > 40);
    test("toXml_has_graph", xml.indexOf("<graph") !== -1);
    test("toXml_has_nodes", xml.indexOf("<node") !== -1);
    test("toXml_has_edges", xml.indexOf("<edge") !== -1);

    var saved = graph.saveToFile("logs/cov_roundtrip.xml");
    test("saveToFile", saved === true);

    graph.clearGraph();
    test("clearGraph", graph.getGraphStats().nodeCount === 0);

    var loaded = graph.loadFromFile("logs/cov_roundtrip.xml");
    test("loadFromFile", loaded === true);
    var stats = graph.getGraphStats();
    test("roundtrip_count", stats.nodeCount === 3 && stats.edgeCount === 2,
         "nodes=" + stats.nodeCount + " edges=" + stats.edgeCount);

    test("loadFromFile_bad", graph.loadFromFile("nonexistent.xml") === false);
})();

// ── Complex Topologies ──────────────────────────────────
(function() {
    console.log("--- Complex Topologies ---");
    graph.clearGraph();

    // SPLIT: 1 input -> 2 outputs; global indices: in 0, out 1, out 2
    var src  = graph.createNode("SOURCE", 0, 0);
    var split = graph.createNode("SPLIT", 200, 0);
    var t1   = graph.createNode("TRANSFORM", 400, -50);
    var t2   = graph.createNode("TRANSFORM", 400, 50);
    graph.connectNodes(src, 0, split, 0);
    graph.connectNodes(split, 1, t1, 0);
    graph.connectNodes(split, 2, t2, 0);
    test("split_topo", graph.getNodeEdges(split).length === 3, "got " + graph.getNodeEdges(split).length);

    // MERGE: 2 inputs -> 1 output; global indices: in 0/1, out 2
    var merge = graph.createNode("MERGE", 600, 0);
    var snk   = graph.createNode("SINK", 800, 0);
    graph.connectNodes(t1, 1, merge, 0);
    graph.connectNodes(t2, 1, merge, 1);
    graph.connectNodes(merge, 2, snk, 0);
    test("merge_topo", graph.getNodeEdges(merge).length === 3);

    // Move nodes around
    graph.moveNode(split, 100, 0);
    graph.setNodePosition(merge, 650, 50);
})();

// ── Queries ─────────────────────────────────────────────
(function() {
    console.log("--- Queries ---");
    var nodes = graph.getAllNodes();
    test("getAllNodes_arr", Array.isArray(nodes) && nodes.length > 0);

    var edges = graph.getAllEdges();
    test("getAllEdges_arr", Array.isArray(edges) && edges.length > 0);

    var stats = graph.getGraphStats();
    test("getGraphStats_obj", stats.nodeCount > 0 && stats.edgeCount > 0);

    // Selection queries (nothing selected in script mode)
    var selNodes = graph.getSelectedNodes();
    test("getSelectedNodes_empty", Array.isArray(selNodes));
    var selEdges = graph.getSelectedEdges();
    test("getSelectedEdges_empty", Array.isArray(selEdges));
})();

// ── Error Handling ─────────────────────────────────────
(function() {
    console.log("--- Error Handling ---");

    test("deleteNode_nonexistent", graph.deleteNode("nonexistent-uuid") === false);
    test("deleteEdge_nonexistent", graph.deleteEdge("nonexistent-uuid") === false);
    test("deleteSelection_empty", graph.deleteSelection() === false);
})();

// ── Done ────────────────────────────────────────────────
console.log("");
console.log("=== COVERAGE SUITE COMPLETE ===");
console.log("PASS: " + passes + "  FAIL: " + fails);
console.log("Exiting via graph.quit(" + (fails === 0 ? 0 : 1) + ") ...");
graph.quit(fails === 0 ? 0 : 1);
