/**
 * Example: Fan-In Pattern
 *
 * Creates a fan-in topology where multiple sources merge to one sink:
 *
 * SOURCE 1 ─┐
 *           ├─→ SINK
 * SOURCE 2 ─┘
 *
 * Demonstrates the MERGE node with multiple input connections.
 */

console.log("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
console.log("Example: Fan-In Pattern");
console.log("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");

// Clear graph
Graph.clear();

// Create sources
var source1 = Graph.createNode("SOURCE", 100, 100);
console.log("✅ Created SOURCE 1 at (100, 100)");

var source2 = Graph.createNode("SOURCE", 100, 300);
console.log("✅ Created SOURCE 2 at (100, 300)");

// Create merger
var merger = Graph.createNode("MERGE", 300, 200);
console.log("✅ Created MERGE at (300, 200)");

// Create sink
var sink = Graph.createNode("SINK", 500, 200);
console.log("✅ Created SINK at (500, 200)");

console.log("\nConnecting nodes...");

// MERGE has 2 inputs and 1 output
// Socket layout: [0=INPUT1, 1=INPUT2, 2=OUTPUT]

// SOURCE 1 output (0) → MERGE input 1 (socket 0)
Graph.connect(source1, 0, merger, 0);
console.log("✅ SOURCE 1 → MERGE (input 1)");

// SOURCE 2 output (0) → MERGE input 2 (socket 1)
Graph.connect(source2, 0, merger, 1);
console.log("✅ SOURCE 2 → MERGE (input 2)");

// MERGE output (socket 2) → SINK input (0)
Graph.connect(merger, 2, sink, 0);
console.log("✅ MERGE (output) → SINK");

// Show final stats
var stats = Graph.getStats();
console.log("\nFinal graph:");
console.log("  Nodes:", stats.nodes);
console.log("  Edges:", stats.edges);

Graph.saveXml("fan_in.xml");
console.log("\n✅ Graph saved to fan_in.xml");

console.log("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
