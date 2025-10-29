// create_merge.js - Create a merge structure (multiple inputs -> one output)
console.log("=== Creating Merge Structure ===");

graph.clearGraph();

// Two sources
var source1 = graph.createNode("SOURCE", 200, 100);
console.log("Created SOURCE 1: " + source1);

var source2 = graph.createNode("SOURCE", 600, 100);
console.log("Created SOURCE 2: " + source2);

// Transform nodes
var transform1 = graph.createNode("TRANSFORM", 200, 250);
console.log("Created TRANSFORM 1: " + transform1);

var transform2 = graph.createNode("TRANSFORM", 600, 250);
console.log("Created TRANSFORM 2: " + transform2);

// Connect sources to transforms
var edge1 = graph.connectNodes(source1, 0, transform1, 0);
console.log("Connected SOURCE1 -> TRANSFORM1: " + edge1);

var edge2 = graph.connectNodes(source2, 0, transform2, 0);
console.log("Connected SOURCE2 -> TRANSFORM2: " + edge2);

// Merge node
var merge = graph.createNode("MERGE", 400, 400);
console.log("Created MERGE: " + merge);

// Connect transforms to merge
var edge3 = graph.connectNodes(transform1, 0, merge, 0);  // Output socket 0 -> input 0
console.log("Connected TRANSFORM1 -> MERGE[0]: " + edge3);

var edge4 = graph.connectNodes(transform2, 0, merge, 1);  // Output socket 0 -> input 1
console.log("Connected TRANSFORM2 -> MERGE[1]: " + edge4);

// Final sink
var sink = graph.createNode("SINK", 400, 550);
console.log("Created SINK: " + sink);

var edge5 = graph.connectNodes(merge, 0, sink, 0);  // MERGE output socket 0
console.log("Connected MERGE -> SINK: " + edge5);

// Report stats
var stats = graph.getGraphStats();
console.log("");
console.log("=== Merge Structure Complete ===");
console.log("Nodes: " + stats.nodeCount + " (2 sources merge into 1 sink)");
console.log("Edges: " + stats.edgeCount);
