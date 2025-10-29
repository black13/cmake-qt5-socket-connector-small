// create_tree.js - Create a tree structure (SPLIT nodes)
console.log("=== Creating Tree Structure ===");

graph.clearGraph();

// Root: SOURCE that splits into two branches
var source = graph.createNode("SOURCE", 400, 100);
console.log("Created SOURCE (root): " + source);

var split = graph.createNode("SPLIT", 400, 250);
console.log("Created SPLIT: " + split);

// Connect source to split
var edge1 = graph.connectNodes(source, 0, split, 0);
console.log("Connected SOURCE -> SPLIT: " + edge1);

// Left branch
var leftTransform = graph.createNode("TRANSFORM", 200, 400);
console.log("Created TRANSFORM (left): " + leftTransform);

var leftSink = graph.createNode("SINK", 200, 550);
console.log("Created SINK (left): " + leftSink);

var edge2 = graph.connectNodes(split, 0, leftTransform, 0);
console.log("Connected SPLIT[0] -> TRANSFORM(left): " + edge2);

var edge3 = graph.connectNodes(leftTransform, 0, leftSink, 0);  // Output socket 0
console.log("Connected TRANSFORM(left) -> SINK(left): " + edge3);

// Right branch
var rightTransform = graph.createNode("TRANSFORM", 600, 400);
console.log("Created TRANSFORM (right): " + rightTransform);

var rightSink = graph.createNode("SINK", 600, 550);
console.log("Created SINK (right): " + rightSink);

var edge4 = graph.connectNodes(split, 1, rightTransform, 0);  // SPLIT output 1 (second output)
console.log("Connected SPLIT[1] -> TRANSFORM(right): " + edge4);

var edge5 = graph.connectNodes(rightTransform, 0, rightSink, 0);  // Output socket 0
console.log("Connected TRANSFORM(right) -> SINK(right): " + edge5);

// Report stats
var stats = graph.getGraphStats();
console.log("");
console.log("=== Tree Complete ===");
console.log("Nodes: " + stats.nodeCount + " (1 root, 1 split, 2 branches)");
console.log("Edges: " + stats.edgeCount);
