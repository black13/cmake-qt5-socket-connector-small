// tests/perf_large_graph.js
//
// Large-graph mutation benchmark: build a big graph, then remove small parts
// and measure how long each operation takes. Run headless from the repo root:
//
//   NodeGraph.exe --script tests/perf_large_graph.js
//
// Each metric is printed as a single "PERF ..." line in logs/NodeGraph_*.log,
// and the app exits via graph.quit() when done. Times come from
// Date().getTime(), so treat sub-millisecond values as below the clock grain.
//
// Note: console.log glue in this app takes exactly one argument.

var NODE_COUNT = 400;       // long TRANSFORM chain: 400 nodes, 399 edges
var SMALL_PART = 25;        // "remove a small part" ~6% of the graph
var MEDIUM_PART = 100;      // scaling check

function now() { return new Date().getTime(); }
function counts() { var s = graph.getGraphStats(); return s.nodeCount + "/" + s.edgeCount; }
function report(name, ms, detail) {
    console.log("PERF " + name + " " + ms + "ms " + (detail || "") + " counts=" + counts());
}

// ── Phase 1: create a large graph (batched, like a pasta operation) ─────────
graph.beginBatch();
var t = now();
var nodes = [];
for (var i = 0; i < NODE_COUNT; ++i) {
    nodes.push(graph.createNode("TRANSFORM", (i % 20) * 220, Math.floor(i / 20) * 140));
}
var createMs = now() - t;

var edges = 0;
t = now();
for (var i = 1; i < NODE_COUNT; ++i) {
    if (graph.connectNodes(nodes[i - 1], 1, nodes[i], 0) !== "") { edges++; }
}
var connectMs = now() - t;
graph.endBatch();
report("create_nodes", createMs, "n=" + NODE_COUNT + " avg=" + (createMs / NODE_COUNT).toFixed(2) + "ms");
report("connect_chain", connectMs, "edges=" + edges + " avg=" + (connectMs / Math.max(edges, 1)).toFixed(2) + "ms");

// ── Phase 2: remove a small part, one node at a time (unbatched) ────────────
var victims = [];
for (var i = 0; i < NODE_COUNT; i += Math.floor(NODE_COUNT / SMALL_PART)) { victims.push(nodes[i]); }
t = now();
var removed = 0;
for (var i = 0; i < victims.length; ++i) {
    if (graph.deleteNode(victims[i])) { removed++; }
}
var smallMs = now() - t;
report("delete_small_unbatched", smallMs, "removed=" + removed + " avg=" + (smallMs / Math.max(removed, 1)).toFixed(2) + "ms");

// ── Phase 3: same size, batched (what deleteSelection does internally) ──────
function isAlive(id) { return graph.getAllNodes().indexOf(id) !== -1; }
var victims2 = [];
var step = Math.floor(NODE_COUNT / SMALL_PART);
for (var i = Math.floor(step / 2); i < NODE_COUNT && victims2.length < SMALL_PART; i += step) {
    var idx = Math.floor(i);
    if (isAlive(nodes[idx])) { victims2.push(nodes[idx]); }
}
graph.beginBatch();
t = now();
var removed2 = 0;
for (var i = 0; i < victims2.length; ++i) {
    if (graph.deleteNode(victims2[i])) { removed2++; }
}
var batchedMs = now() - t;
graph.endBatch();
report("delete_small_batched", batchedMs, "removed=" + removed2 + " avg=" + (batchedMs / Math.max(removed2, 1)).toFixed(2) + "ms");

// ── Phase 4: medium part, batched ──────────────────────────────────────────
var alive = [];
var all = graph.getAllNodes();
for (var i = 0; i < all.length; ++i) { alive.push(all[i]); }
var medium = [];
for (var i = 0; i < MEDIUM_PART && i < alive.length; i += 2) { medium.push(alive[i]); }
graph.beginBatch();
t = now();
var removed3 = 0;
for (var i = 0; i < medium.length; ++i) {
    if (graph.deleteNode(medium[i])) { removed3++; }
}
var mediumMs = now() - t;
graph.endBatch();
report("delete_medium_batched", mediumMs, "removed=" + removed3 + " avg=" + (mediumMs / Math.max(removed3, 1)).toFixed(2) + "ms");

// ── Phase 5: move + serialize + save the remaining large graph ─────────────
var remaining = graph.getAllNodes();
t = now();
for (var i = 0; i < remaining.length; i += 5) {
    graph.moveNode(remaining[i], 10, 10);
}
var moveMs = now() - t;
report("move_fifth_of_nodes", moveMs, "moved=" + Math.floor((remaining.length + 4) / 5));

t = now();
var xml = graph.toXml();
report("toXml", now() - t, "len=" + xml.length);

t = now();
var saved = graph.saveToFile("logs/perf_large_graph.xml");
report("saveToFile", now() - t, "ok=" + saved);

// ── Phase 6: full clear + final state ──────────────────────────────────────
t = now();
graph.clearGraph();
report("clearGraph", now() - t, "");

console.log("=== PERF LARGE GRAPH COMPLETE ===");
graph.quit();
