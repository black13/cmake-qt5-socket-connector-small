// tests/js_regression_suite.js
//
// JavaScript regression tests for the bugs described in the code review and
// fixed since: global socket indexing, script error reporting, script safety
// guards, scripted drops, Unicode saves, load-preserves-graph, edge policy,
// synthetic-work error handling, and batch semantics.
//
// Run from the repo root:
//   NodeGraph.exe --script tests/js_regression_suite.js
// or through CTest (registers as JsRegressionSuite).
//
// The process exits with code 0 only when every check passes, so CTest and
// shells can detect failures. console.log takes exactly one argument.

var passes = 0;
var fails = 0;
function test(name, condition, detail) {
    if (condition) { passes++; console.log("PASS " + name); }
    else { fails++; console.log("FAIL " + name + (detail ? " (" + detail + ")" : "")); }
}
function counts() { var s = graph.getGraphStats(); return [s.nodeCount, s.edgeCount]; }
function alive(id) { return graph.getAllNodes().indexOf(id) !== -1; }

// ── Global socket indices (fix: Graph::connectNodes) ───────────────────────
console.log("--- socket indices ---");
graph.clearGraph();
var src = graph.createNode("SOURCE", 0, 0);
var tr = graph.createNode("TRANSFORM", 200, 0);
var snk = graph.createNode("SINK", 400, 0);
var e1 = graph.connectNodes(src, 0, tr, 0);      // SOURCE output is global 0
test("socket.source_out_is_0", e1 !== "");
var e2 = graph.connectNodes(tr, 1, snk, 0);      // TRANSFORM output is global 1
test("socket.transform_out_is_1", e2 !== "");
var wrongRole = graph.connectNodes(tr, 0, snk, 0); // global 0 is the input
test("socket.role_mismatch_refused", wrongRole === "");
var edgeData = graph.getEdgeData(e2);
test("socket.get_round_trips",
     edgeData.fromSocketIndex === 1 && edgeData.toSocketIndex === 0,
     "from=" + edgeData.fromSocketIndex + " to=" + edgeData.toSocketIndex);

// ── Script error reporting (fix: ScriptedNode::lastError) ──────────────────
console.log("--- script errors ---");
graph.clearGraph();
var scripted = graph.createNode("TRANSFORM", 0, 0);
graph.setNodeScript(scripted, "throw new Error('boom');");
graph.executeNodeScript(scripted, {});
test("errors.runtime_reported", graph.getNodeScriptError(scripted).length > 0);
graph.setNodeScript(scripted, "return 1 + 1;");
graph.executeNodeScript(scripted, {});
test("errors.cleared_on_success", graph.getNodeScriptError(scripted) === "");
graph.setNodeScript(scripted, "function ( {");
graph.executeNodeScript(scripted, {});
test("errors.compile_reported", graph.getNodeScriptError(scripted).length > 0);
test("errors.missing_node_empty",
     graph.getNodeScriptError("deadbeef-dead-beef-dead-beefdeadbeef") === "");

// ── Script safety guards (self-delete, recursion) ──────────────────────────
console.log("--- script safety ---");
graph.setNodeScript(scripted, "graph.deleteNode(node.nodeId()); 'done';");
graph.executeNodeScript(scripted, {});
test("safety.self_delete_refused", alive(scripted));
graph.setNodeScript(scripted,
    "graph.executeNodeScript(node.nodeId(), {}); 'done';");
graph.executeNodeScript(scripted, {});
test("safety.recursion_capped", alive(scripted));

// ── Scripted drop-in (feature: Graph::dropNode) ────────────────────────────
console.log("--- dropNode ---");
graph.clearGraph();
var dropped = graph.dropNode("SINK", 300, 200, 40);
test("drop.created", dropped !== "" && alive(dropped));
var droppedDefault = graph.dropNode("SOURCE", 100, 100); // 3-arg overload
test("drop.default_duration", droppedDefault !== "");
test("drop.bad_type", graph.dropNode("NOPE", 0, 0, 40) === "");
test("drop.count", counts()[0] === 2, "nodes=" + counts()[0]);

// ── Unicode save/load (fix: Graph::saveToFile via QFile) ───────────────────
console.log("--- unicode save ---");
graph.clearGraph();
var u1 = graph.createNode("SOURCE", 0, 0);
var u2 = graph.createNode("SINK", 200, 0);
graph.connectNodes(u1, 0, u2, 0);
var unicodePath = "logs/regression-\u00e9-\u65e5\u672c.xml";
test("unicode.save", graph.saveToFile(unicodePath) === true);
graph.clearGraph();
test("unicode.load", graph.loadFromFile(unicodePath) === true);
test("unicode.roundtrip", counts()[0] === 2 && counts()[1] === 1,
     "counts=" + counts().join("/"));

// ── Failed load preserves the document ─────────────────────────────────────
console.log("--- load failure ---");
test("load.missing_file_false", graph.loadFromFile("logs/definitely-missing.xml") === false);
test("load.missing_file_preserves", counts()[0] === 2 && counts()[1] === 1);

// ── Edge policy: duplicates refused, delete cascades ───────────────────────
console.log("--- edge policy ---");
graph.clearGraph();
var a = graph.createNode("SOURCE", 0, 0);
var b = graph.createNode("SINK", 200, 0);
var c = graph.createNode("SINK", 400, 0);
test("edge.connect", graph.connectNodes(a, 0, b, 0) !== "");
test("edge.duplicate_refused", graph.connectNodes(a, 0, c, 0) === "");
test("edge.cascade_delete", graph.deleteNode(a) === true && counts()[1] === 0);
test("edge.delete_twice_refused", graph.deleteNode(a) === false);

// ── Synthetic work: real results and loud failures ─────────────────────────
console.log("--- synthetic work ---");
var workHash = graph.runSyntheticWork({task: "hash", payload: "hello world"});
test("work.hash_result", workHash.status === "ok" && workHash.result.length > 0);
var workLoop = graph.runSyntheticWork({task: "loop", iterations: 200000});
test("work.duration_measured", workLoop.durationMs > 0, "durationMs=" + workLoop.durationMs);
var workBad = graph.runSyntheticWork({task: "bogus"});
test("work.unknown_task_error",
     workBad.status === "error" && workBad.error !== undefined, "error=" + workBad.error);

// ── Serialization and batches ──────────────────────────────────────────────
console.log("--- serialization ---");
graph.clearGraph();
var serialized = graph.createNode("TRANSFORM", 10, 20);
graph.setNodePayload(serialized, {answer: 42});
var xml = graph.toXml();
test("xml.real", xml.indexOf("<graph") !== -1 && xml.indexOf("TRANSFORM") !== -1 &&
     xml !== "<graph></graph>", "len=" + xml.length);

console.log("--- batch ---");
graph.clearGraph();
graph.beginBatch();
graph.createNode("SOURCE", 0, 0);
graph.createNode("SINK", 200, 0);
test("batch.active", graph.isBatchMode() === true);
graph.endBatch();
test("batch.ended", graph.isBatchMode() === false && counts()[0] === 2,
     "nodes=" + counts()[0]);

// ── Done ───────────────────────────────────────────────────────────────────
console.log("");
console.log("=== JS REGRESSION SUITE COMPLETE ===");
console.log("PASS: " + passes + "  FAIL: " + fails);
graph.quit(fails === 0 ? 0 : 1);
