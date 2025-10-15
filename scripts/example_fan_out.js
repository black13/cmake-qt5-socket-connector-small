/**
 * Example: Fan-Out Pattern
 *
 * Creates a fan-out topology where one source splits to multiple sinks:
 *
 *        +--> SINK 1
 * SOURCE-+
 *        +--> SINK 2
 *
 * Demonstrates the SPLIT node with multiple output connections.
 */

console.log("----------------------------------------------------");
console.log("Example: Fan-Out Pattern");
console.log("----------------------------------------------------");

// Clear graph
Graph.clear();

// Create source
var source = Graph.createNode("SOURCE", 100, 200);
console.log("[OK] Created SOURCE at (100, 200)");

// Create splitter
var splitter = Graph.createNode("SPLIT", 300, 200);
console.log("[OK] Created SPLIT at (300, 200)");

// Create sinks
var sink1 = Graph.createNode("SINK", 500, 100);
console.log("[OK] Created SINK 1 at (500, 100)");

var sink2 = Graph.createNode("SINK", 500, 300);
console.log("[OK] Created SINK 2 at (500, 300)");

console.log("\nConnecting nodes...");

// SOURCE output (0) -> SPLIT input (0)
Graph.connect(source, 0, splitter, 0);
console.log("[OK] SOURCE -> SPLIT");

// SPLIT has 1 input and 2 outputs
// Socket layout: [0=INPUT, 1=OUTPUT1, 2=OUTPUT2]

// SPLIT output 1 (socket 1) -> SINK 1 input (0)
Graph.connect(splitter, 1, sink1, 0);
console.log("[OK] SPLIT (output 1) -> SINK 1");

// SPLIT output 2 (socket 2) -> SINK 2 input (0)
Graph.connect(splitter, 2, sink2, 0);
console.log("[OK] SPLIT (output 2) -> SINK 2");

// Show final stats
var stats = Graph.getStats();
console.log("\nFinal graph:");
console.log("  Nodes:", stats.nodes);
console.log("  Edges:", stats.edges);

Graph.saveXml("fan_out.xml");
console.log("\n[OK] Graph saved to fan_out.xml");

console.log("----------------------------------------------------");
