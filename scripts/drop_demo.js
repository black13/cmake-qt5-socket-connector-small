// scripts/drop_demo.js
//
// Visual check for Graph::dropNode(): the nodes are created through the same
// factory/template path as a palette drag, but instead of a mouse drag they
// slide in from a small offset with a fade, so a script can make creation
// look like a drop onto the canvas.
//
// Run from the repo root:
//   build_Release\Release\NodeGraph.exe --script scripts/drop_demo.js
//
// The app stays open so the animation is visible; close it when done.
// (console.log glue takes exactly one argument.)

graph.clearGraph();

var source = graph.dropNode("SOURCE", -250, 0, 320);
var transform = graph.dropNode("TRANSFORM", 0, 0, 460);
var sink = graph.dropNode("SINK", 250, 0, 600);

console.log("dropNode: " + source + " " + transform + " " + sink);
console.log("Nodes now on canvas: " + graph.getGraphStats().nodeCount);
