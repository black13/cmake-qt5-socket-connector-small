/**
 * Example: Simple Chain
 *
 * Creates a simple linear chain: SOURCE -> TRANSFORM -> SINK
 * Demonstrates basic node creation and connection.
 */

console.log("----------------------------------------------------");
console.log("Example: Simple Chain");
console.log("----------------------------------------------------");

// Clear any existing graph
Graph.clear();
console.log("Graph cleared");

// Create nodes
console.log("\nCreating nodes...");
var source = Graph.createNode("SOURCE", 100, 200);
console.log("[OK] Created SOURCE at (100, 200)");

var transform = Graph.createNode("TRANSFORM", 300, 200);
console.log("[OK] Created TRANSFORM at (300, 200)");

var sink = Graph.createNode("SINK", 500, 200);
console.log("[OK] Created SINK at (500, 200)");

// Connect nodes
console.log("\nConnecting nodes...");

// SOURCE output (socket 0) -> TRANSFORM input (socket 0)
var edge1 = Graph.connect(source, 0, transform, 0);
if (edge1 !== "") {
    console.log("[OK] Connected SOURCE -> TRANSFORM");
} else {
    console.error("[FAIL] Failed to connect SOURCE -> TRANSFORM");
}

// TRANSFORM output (socket 1) -> SINK input (socket 0)
var edge2 = Graph.connect(transform, 1, sink, 0);
if (edge2 !== "") {
    console.log("[OK] Connected TRANSFORM -> SINK");
} else {
    console.error("[FAIL] Failed to connect TRANSFORM -> SINK");
}

// Show final stats
console.log("\nFinal graph:");
var stats = Graph.getStats();
console.log("  Nodes:", stats.nodes);
console.log("  Edges:", stats.edges);

// Save graph
Graph.saveXml("simple_chain.xml");
console.log("\n[OK] Graph saved to simple_chain.xml");

console.log("----------------------------------------------------");
