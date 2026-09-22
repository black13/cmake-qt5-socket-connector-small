// tests/chaos_suite.js
//
// Adversarial tests: deliberately misuse every script-facing API and assert
// the app degrades gracefully (returns false/empty, keeps invariants, never
// crashes). Hostile calls are wrapped so a JS exception is recorded rather
// than aborting the suite.
//
// Run from the repo root:
//   NodeGraph.exe --script tests/chaos_suite.js
//
// If the app dies mid-suite there is no "CHAOS SUITE COMPLETE" line in the
// log - that is itself the failure signal. Exits nonzero when a check fails.

var passes = 0;
var fails = 0;
function test(name, condition, detail) {
    if (condition) { passes++; console.log("PASS " + name); }
    else { fails++; console.log("FAIL " + name + (detail ? " (" + detail + ")" : "")); }
}
function counts() { var s = graph.getGraphStats(); return [s.nodeCount, s.edgeCount]; }
function alive(id) { return graph.getAllNodes().indexOf(id) !== -1; }

// Run a hostile call: a JS exception counts as graceful refusal too.
function attempt(f) { try { return f(); } catch (e) { return "__threw__"; } }
// Refused: false / empty string / empty collection / null / threw.
function refused(v) {
    return v === false || v === "" || v === null || v === undefined ||
           v === "__threw__" ||
           (typeof v === "object" && v !== null && v.length === 0);
}
// Empty/missing result value (empty QVariantMap lands as either).
function nothing(v) { return v === null || v === undefined || v === "__threw__"; }

var BAD_IDS = ["", "bad", "00000000-0000-0000-0000-000000000000",
               "not-a-uuid", "{}", "null", "123"];

console.log("--- createNode argument abuse ---");
graph.clearGraph();
test("create.empty_type_refused", refused(attempt(function() { return graph.createNode("", 0, 0); })));
test("create.lowercase_type_refused", refused(attempt(function() { return graph.createNode("source", 0, 0); })));
test("create.garbage_type_refused", refused(attempt(function() { return graph.createNode("<script>", 0, 0); })));
test("create.null_type_refused", refused(attempt(function() { return graph.createNode(null, 0, 0); })));
test("create.number_type_refused", refused(attempt(function() { return graph.createNode(123, 0, 0); })));
test("create.no_crash_survivors", counts()[0] === 0, "nodes=" + counts()[0]);

console.log("--- coordinate abuse ---");
graph.clearGraph();
var nanNode = attempt(function() { return graph.createNode("SOURCE", 0 / 0, 0); });
var infNode = attempt(function() { return graph.createNode("SOURCE", 1 / 0, 0); });
var negInfNode = attempt(function() { return graph.createNode("SOURCE", -1 / 0, 0); });
test("coords.nan_refused", refused(nanNode));
test("coords.pos_inf_refused", refused(infNode));
test("coords.neg_inf_refused", refused(negInfNode));

var finite = graph.createNode("SOURCE", 5, 5);
test("coords.finite_accepted", finite !== "");
test("coords.move_nan_refused", refused(attempt(function() { return graph.moveNode(finite, 0 / 0, 0); })));
test("coords.setpos_inf_refused", refused(attempt(function() { return graph.setNodePosition(finite, 1 / 0, 0); })));
// Finite inputs can still overflow the sum: 1e308 + 1e308 = Infinity.
test("coords.huge_finite_accepted", graph.setNodePosition(finite, 1e308, 5) === true);
test("coords.move_overflow_refused", refused(attempt(function() { return graph.moveNode(finite, 1e308, 0); })));
test("coords.restore_finite",
     graph.setNodePosition(finite, 5, 5) === true &&
     graph.getNodeData(finite).x === 5 && graph.getNodeData(finite).y === 5);

// A graph with only finite positions must always round-trip.
test("coords.save", graph.saveToFile("logs/chaos_coords.xml") === true);
graph.clearGraph();
test("coords.load", graph.loadFromFile("logs/chaos_coords.xml") === true && counts()[0] === 1,
     "nodes=" + counts()[0]);

console.log("--- connectNodes argument abuse ---");
graph.clearGraph();
var src = graph.createNode("SOURCE", 0, 0);
var tr = graph.createNode("TRANSFORM", 200, 0);
var snk = graph.createNode("SINK", 400, 0);
test("connect.negative_index", refused(attempt(function() { return graph.connectNodes(src, -1, tr, 0); })));
test("connect.huge_index", refused(attempt(function() { return graph.connectNodes(src, 999999, tr, 0); })));
test("connect.fractional_index_no_crash", attempt(function() { return graph.connectNodes(src, 0.5, tr, 0); }) !== "__threw__");
test("connect.input_as_output", refused(attempt(function() { return graph.connectNodes(tr, 0, snk, 0); })));
test("connect.same_node", refused(attempt(function() { return graph.connectNodes(tr, 1, tr, 0); })));
test("connect.bad_node_ids", refused(attempt(function() { return graph.connectNodes("bad", 0, snk, 0); })));
test("connect.bad_ids_both", refused(attempt(function() { return graph.connectNodes("bad", 0, "worse", 0); })));
test("connect.output_to_output", refused(attempt(function() { return graph.connectNodes(src, 0, tr, 1); })));
test("connect.to_self_refused", refused(attempt(function() { return graph.connectNodes(src, 0, src, 0); })));

console.log("--- bad-id API sweep ---");
var survived = true;
for (var i = 0; i < BAD_IDS.length; i++) {
    var id = BAD_IDS[i];
    var ok = refused(attempt(function() { return graph.deleteNode(id); }))
        && refused(attempt(function() { return graph.deleteEdge(id); }))
        && refused(attempt(function() { return graph.moveNode(id, 1, 1); }))
        && refused(attempt(function() { return graph.setNodePosition(id, 1, 1); }))
        && refused(attempt(function() { return graph.setNodeScript(id, "1"); }))
        && refused(attempt(function() { return graph.getNodeScript(id); }))
        && refused(attempt(function() { return graph.getNodeScriptError(id); }))
        && refused(attempt(function() { return graph.setNodePayload(id, {}); }))
        && nothing(attempt(function() { return graph.executeNodeScript(id, {}); }))
        && refused(attempt(function() { return graph.getNodeEdges(id); }));
    if (!ok) { survived = false; break; }
}
test("badids.every_call_refused", survived);
test("badids.survivors_intact", counts()[0] === 3, "nodes=" + counts()[0]);

console.log("--- weird UUID shapes ---");
var braced = attempt(function() { return graph.getNodeData("{" + src + "}"); });
test("uuid.braced_no_crash", braced !== "__threw__");
var upper = attempt(function() { return graph.getNodeData(src.toUpperCase()); });
test("uuid.uppercase_no_crash", upper !== "__threw__");

console.log("--- batch misuse ---");
graph.endBatch();
test("batch.stray_end_safe", graph.isBatchMode() === false);
graph.beginBatch();
graph.beginBatch();
graph.endBatch();
test("batch.nested_still_active", graph.isBatchMode() === true);
graph.endBatch();
test("batch.nested_ended", graph.isBatchMode() === false);
graph.endBatch();
test("batch.triple_end_safe", graph.isBatchMode() === false);

console.log("--- payload abuse ---");
var pn = graph.createNode("TRANSFORM", 0, 0);
test("payload.number_and_bool", graph.setNodePayload(pn, {n: 1, b: true, s: "x"}) === true);
test("payload.roundtrip_intact", graph.getNodePayload(pn).s === "x");
test("payload.nan_value", graph.setNodePayload(pn, {bad: 0 / 0}) === true);
var big = new Array(10000).join("x");
test("payload.large_string", graph.setNodePayload(pn, {blob: big}) === true);
var deep = {}; var cursor = deep;
for (var d = 0; d < 200; d++) { cursor.next = {}; cursor = cursor.next; }
test("payload.deep_nesting", graph.setNodePayload(pn, deep) === true);

console.log("--- save/load path abuse ---");
test("save.directory_path", refused(attempt(function() { return graph.saveToFile("logs"); })));
test("save.empty_path", refused(attempt(function() { return graph.saveToFile(""); })));
test("save.illegal_chars", refused(attempt(function() { return graph.saveToFile("logs/a<b>c|d?.xml"); })));
test("load.directory_path", refused(attempt(function() { return graph.loadFromFile("logs"); })));
test("load.empty_path", refused(attempt(function() { return graph.loadFromFile(""); })));
test("load.missing_file", refused(attempt(function() { return graph.loadFromFile("logs/no-such-file-xyz.xml"); })));
test("paths.graph_survived", counts()[0] > 0);

console.log("--- script abuse ---");
var scripted = graph.createNode("TRANSFORM", 500, 0);
test("script.non_string_handled", attempt(function() { return graph.setNodeScript(scripted, 12345); }) !== "__threw__");
graph.setNodeScript(scripted, "return {a: 1, b: [1, 2, 3]};");
var objResult = attempt(function() { return graph.executeNodeScript(scripted, {}); });
test("script.returns_object", objResult !== "__threw__" && objResult !== null);
graph.setNodeScript(scripted, "return function() { return 1; };");
test("script.returns_function_survives", attempt(function() { return graph.executeNodeScript(scripted, {}); }) !== "__threw__");
graph.setNodeScript(scripted, "graph.clearGraph(); return 'cleared';");
attempt(function() { return graph.executeNodeScript(scripted, {}); });
test("script.clear_during_run_refused", counts()[0] > 0);

console.log("--- stress: create/delete churn ---");
graph.clearGraph();
var churnOk = true;
for (var i = 0; i < 300; i++) {
    var n = graph.createNode("TRANSFORM", (i % 30) * 40, Math.floor(i / 30) * 40);
    if (n === "" || !graph.deleteNode(n)) { churnOk = false; break; }
}
test("stress.churn_300", churnOk && counts()[0] === 0, "nodes=" + counts()[0]);

console.log("--- stress: script flood ---");
var floodNode = graph.createNode("TRANSFORM", 0, 0);
graph.setNodeScript(floodNode, "return node.payloadValue('i') || 0;");
var floodOk = true;
for (var r = 0; r < 500; r++) {
    if (graph.executeNodeScript(floodNode, {}) === null) { floodOk = false; break; }
}
test("stress.script_flood_500", floodOk);

console.log("--- stress: save/load loop ---");
graph.clearGraph();
var loopSrc = graph.createNode("SOURCE", 0, 0);
var loopSnk = graph.createNode("SINK", 200, 0);
graph.connectNodes(loopSrc, 0, loopSnk, 0);
var loopOk = true;
for (var l = 0; l < 25; l++) {
    if (!graph.saveToFile("logs/chaos_loop.xml") || !graph.loadFromFile("logs/chaos_loop.xml")) {
        loopOk = false; break;
    }
}
test("stress.roundtrip_25", loopOk && counts()[0] === 2 && counts()[1] === 1,
     "counts=" + counts().join("/"));

console.log("");
console.log("=== CHAOS SUITE COMPLETE ===");
console.log("PASS: " + passes + "  FAIL: " + fails);
graph.quit(fails === 0 ? 0 : 1);
