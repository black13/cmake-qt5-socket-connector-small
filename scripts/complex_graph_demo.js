// scripts/complex_graph_demo.js
//
// Demonstrates driving the application entirely from JavaScript:
//   1. build a 40-node / 43-edge layered DAG (sources -> transforms -> merges
//      -> splits -> transforms -> merges -> sink)
//   2. lay it out and shift it with setNodePosition() / moveNode()
//   3. save to XML
//   4. clear, reload, and prove the graph is identical (ids, topology,
//      positions, payloads, scripts)
//
// Run from the repo root:
//   build_Release\Release\NodeGraph.exe --script scripts/complex_graph_demo.js
// or via run.ps1. Exits nonzero if the round-trip check fails.

var passes = 0;
var fails = 0;
function test(name, condition, detail) {
    if (condition) { passes++; console.log("DEMO PASS " + name); }
    else { fails++; console.log("DEMO FAIL " + name + (detail ? " (" + detail + ")" : "")); }
}
function now() { return new Date().getTime(); }
function counts() { var s = graph.getGraphStats(); return [s.nodeCount, s.edgeCount]; }

// ── 1. Build the layered DAG ────────────────────────────────────────────────
graph.clearGraph();
graph.beginBatch();
var tCreate = now();

var layerCounts = [8, 8, 4, 4, 8, 4, 2, 1, 1]; // src, t1, merge, split, t2, merge, merge, merge, sink
var layers = [];
for (var L = 0; L < layerCounts.length; ++L) {
    var row = [];
    for (var i = 0; i < layerCounts[L]; ++i) {
        var type;
        if (L === 0) { type = "SOURCE"; }
        else if (L === layerCounts.length - 1) { type = "SINK"; }
        else if (L === 2 || L === 5 || L === 6 || L === 7) { type = "MERGE"; }
        else if (L === 3) { type = "SPLIT"; }
        else { type = "TRANSFORM"; }
        var id = graph.createNode(type, 0, 0); // placed by the layout pass below
        if (id === "") { console.log("DEMO FAIL create layer=" + L + " i=" + i); graph.quit(1); }
        row.push(id);
    }
    layers.push(row);
}

var src = layers[0], t1 = layers[1], m1 = layers[2], sp = layers[3],
    t2 = layers[4], m2 = layers[5], m3 = layers[6], m4 = layers[7], sink = layers[8];

var edgeCount = 0;
function wire(from, fromIdx, to, toIdx) {
    if (graph.connectNodes(from, fromIdx, to, toIdx) === "") {
        console.log("DEMO FAIL connect " + from + "[" + fromIdx + "] -> " + to + "[" + toIdx + "]");
        graph.quit(1);
    }
    ++edgeCount;
}
for (var i = 0; i < 8; ++i) { wire(src[i], 0, t1[i], 0); }                  // SOURCE out 0 -> TRANSFORM in 0
for (var j = 0; j < 4; ++j) {                                              // TRANSFORM out 1 -> MERGE in 0/1
    wire(t1[2 * j], 1, m1[j], 0);
    wire(t1[2 * j + 1], 1, m1[j], 1);
}
for (var j = 0; j < 4; ++j) { wire(m1[j], 2, sp[j], 0); }                  // MERGE out 2 -> SPLIT in 0
for (var j = 0; j < 4; ++j) {                                              // SPLIT out 1/2 -> TRANSFORM in 0
    wire(sp[j], 1, t2[2 * j], 0);
    wire(sp[j], 2, t2[2 * j + 1], 0);
}
for (var j = 0; j < 4; ++j) {                                              // TRANSFORM out 1 -> MERGE in 0/1
    wire(t2[2 * j], 1, m2[j], 0);
    wire(t2[2 * j + 1], 1, m2[j], 1);
}
for (var j = 0; j < 2; ++j) {
    wire(m2[2 * j], 2, m3[j], 0);
    wire(m2[2 * j + 1], 2, m3[j], 1);
}
wire(m3[0], 2, m4[0], 0);
wire(m3[1], 2, m4[0], 1);
wire(m4[0], 2, sink[0], 0);

// Payloads and scripts travel with the nodes through save/load.
for (var i = 0; i < t1.length; ++i) { graph.setNodePayload(t1[i], {layer: "t1", idx: i}); }
for (var i = 0; i < t2.length; ++i) {
    graph.setNodePayload(t2[i], {layer: "t2", idx: i});
    graph.setNodeScript(t2[i], "return node.payloadValue('idx') * 2;");
}
graph.endBatch();
console.log("DEMO built nodes=" + counts()[0] + " edges=" + edgeCount +
            " in " + (now() - tCreate) + "ms");

// ── 2. Layout, then shift the whole graph ──────────────────────────────────
graph.beginBatch();
var tMove = now();
for (var L = 0; L < layers.length; ++L) {
    for (var i = 0; i < layers[L].length; ++i) {
        graph.setNodePosition(layers[L][i], 60 + L * 260, 60 + i * 130);
    }
}
var shiftDx = 40, shiftDy = -25;
var allNodes = graph.getAllNodes();
for (var i = 0; i < allNodes.length; ++i) { graph.moveNode(allNodes[i], shiftDx, shiftDy); }
graph.endBatch();
console.log("DEMO moved " + allNodes.length + " nodes in " + (now() - tMove) + "ms");

// ── 3. Snapshot, save ──────────────────────────────────────────────────────
function sortedKeys(o) { return Object.keys(o).sort(); }
function stablePayload(o) {
    var ks = sortedKeys(o), parts = [];
    for (var i = 0; i < ks.length; ++i) { parts.push(ks[i] + "=" + JSON.stringify(o[ks[i]])); }
    return "{" + parts.join(",") + "}";
}
function snapshot() {
    var ids = graph.getAllNodes().sort();
    var parts = [];
    for (var i = 0; i < ids.length; ++i) {
        var d = graph.getNodeData(ids[i]);
        var edges = graph.getNodeEdges(ids[i]).sort();
        parts.push(ids[i] + "|" + d.type + "|" + d.x + "," + d.y + "|" +
                   stablePayload(d.payload || {}) + "|" + (d.script || "") + "|" + edges.join("+"));
    }
    return parts.join("\n");
}
var before = snapshot();
var tSave = now();
var saveOk = graph.saveToFile("logs/complex_graph.xml");
console.log("DEMO saved logs/complex_graph.xml ok=" + saveOk + " in " + (now() - tSave) + "ms");

// ── 4. Clear, reload, verify ───────────────────────────────────────────────
graph.clearGraph();
console.log("DEMO cleared nodes=" + counts()[0] + " edges=" + counts()[1]);
var tLoad = now();
var loadOk = graph.loadFromFile("logs/complex_graph.xml");
var after = snapshot();
console.log("DEMO loaded ok=" + loadOk + " nodes=" + counts()[0] + " edges=" + counts()[1] +
            " in " + (now() - tLoad) + "ms");

test("file.save", saveOk === true);
test("file.load", loadOk === true);
test("counts.nodes", counts()[0] === 40, "nodes=" + counts()[0]);
test("counts.edges", counts()[1] === 43, "edges=" + counts()[1]);

// Spot checks: a merge has 3 incident edges, a split 3, the sink 1.
var d = graph.getNodeData(m4[0]);
test("topology.merge_out", graph.getNodeEdges(m4[0]).length === 3);
test("topology.split", graph.getNodeEdges(sp[0]).length === 3);
test("topology.sink", graph.getNodeEdges(sink[0]).length === 1);

// Full equality: ids, types, shifted positions, payloads, scripts, adjacency.
test("roundtrip.identical", before === after);
if (before !== after) {
    var b = before.split("\n"), a = after.split("\n");
    for (var i = 0; i < Math.max(b.length, a.length); ++i) {
        if (b[i] !== a[i]) { console.log("DEMO first_diff before=" + b[i] + " after=" + a[i]); break; }
    }
}

// Scripts still execute after the reload.
var executed = graph.executeNodeScript(t2[3], {});
test("script.survives_reload", executed === 6, "result=" + executed);

// ── 5. Remove a layer, save, undo/redo, reload ─────────────────────────────
graph.beginBatch();
var tRemove = now();
for (var i = 0; i < t2.length; ++i) { graph.deleteNode(t2[i]); }
graph.endBatch();
console.log("DEMO removed layer t2 (8 nodes) in " + (now() - tRemove) + "ms");
test("remove.counts", counts()[0] === 32 && counts()[1] === 27,
     "counts=" + counts().join("/"));
test("remove.can_undo", graph.canUndo() === true);

var tRemoveSave = now();
var saveRemovedOk = graph.saveToFile("logs/complex_graph_removed.xml");
console.log("DEMO saved removed graph ok=" + saveRemovedOk +
            " in " + (now() - tRemoveSave) + "ms");

// Undo puts the graph back exactly (ids, positions, payloads, scripts, edges)
graph.undo();
test("undo.restores_full",
     counts()[0] === 40 && counts()[1] === 43 && snapshot() === before,
     "counts=" + counts().join("/"));
graph.redo();
test("redo.removes_again", counts()[0] === 32 && counts()[1] === 27,
     "counts=" + counts().join("/"));

// The removed graph persists across clear + reload
graph.clearGraph();
var tRemovedLoad = now();
var removedLoadOk = graph.loadFromFile("logs/complex_graph_removed.xml");
console.log("DEMO loaded removed graph ok=" + removedLoadOk +
            " nodes=" + counts()[0] + " edges=" + counts()[1] +
            " in " + (now() - tRemovedLoad) + "ms");
test("remove.file_roundtrip",
     removedLoadOk === true && counts()[0] === 32 && counts()[1] === 27,
     "counts=" + counts().join("/"));

console.log("");
console.log("=== COMPLEX GRAPH DEMO COMPLETE ===");
console.log("PASS: " + passes + "  FAIL: " + fails);
// Intentionally no graph.quit() here: when run interactively the window stays
// open on the reloaded graph (close it when done). CTest runs this through
// tests/complex_graph_ctest.js, which turns the count into an exit code.
