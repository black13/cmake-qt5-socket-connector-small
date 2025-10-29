// create_pipeline.js - Create a simple linear processing pipeline
console.log("=== Creating Linear Pipeline ===");

graph.clearGraph();

// Create a chain: SOURCE -> TRANSFORM -> TRANSFORM -> SINK
var source = graph.createNode("SOURCE", 100, 200);
console.log("Created SOURCE: " + source);

var transform1 = graph.createNode("TRANSFORM", 300, 200);
console.log("Created TRANSFORM 1: " + transform1);

var transform2 = graph.createNode("TRANSFORM", 500, 200);
console.log("Created TRANSFORM 2: " + transform2);

var sink = graph.createNode("SINK", 700, 200);
console.log("Created SINK: " + sink);

// Connect them in sequence
// Note: Socket indices are per-type (outputs: 0,1,2... inputs: 0,1,2...)
var edge1 = graph.connectNodes(source, 0, transform1, 0);
console.log("Connected SOURCE -> TRANSFORM1: " + edge1);

var edge2 = graph.connectNodes(transform1, 0, transform2, 0);  // Output socket 0 (not 1!)
console.log("Connected TRANSFORM1 -> TRANSFORM2: " + edge2);

var edge3 = graph.connectNodes(transform2, 0, sink, 0);  // Output socket 0 (not 1!)
console.log("Connected TRANSFORM2 -> SINK: " + edge3);

// Report stats
var stats = graph.getGraphStats();
console.log("");
console.log("=== Pipeline Complete ===");
console.log("Nodes: " + stats.nodeCount);
console.log("Edges: " + stats.edgeCount);
