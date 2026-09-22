// scripts/build_align_demo.js
//
// Build a small graph from JavaScript, connect it, then align + center it:
//   1. create six nodes anywhere (deliberately messy coordinates)
//   2. connect them into a diamond: 2 sources -> 2 transforms -> merge -> sink
//   3. Graph::alignGraph() lays it out by topology on the grid (undoable)
//   4. view.centerOnGraph() brings it into view
//
// Run from the repo root:
//   .\run.ps1 --script scripts/build_align_demo.js
//
// The window stays open on the aligned graph; close it when done.

graph.clearGraph();

var s1 = graph.createNode("SOURCE", 400, 500);
var s2 = graph.createNode("SOURCE", -200, 123);
var t1 = graph.createNode("TRANSFORM", 50, -300);
var t2 = graph.createNode("TRANSFORM", 777, 42);
var merge = graph.createNode("MERGE", 12, 900);
var sink = graph.createNode("SINK", -500, -500);

graph.connectNodes(s1, 0, t1, 0);
graph.connectNodes(s2, 0, t2, 0);
graph.connectNodes(t1, 1, merge, 0);
graph.connectNodes(t2, 1, merge, 1);
graph.connectNodes(merge, 2, sink, 0);

console.log("built nodes=" + graph.getGraphStats().nodeCount +
            " edges=" + graph.getGraphStats().edgeCount);

var moved = graph.alignGraph();
console.log("alignGraph moved=" + moved + " nodes");

view.centerOnGraph();
console.log("centered on graph");
