/**
 * Example: Diamond Pattern
 *
 * Creates a diamond-shaped workflow:
 *
 *         ┌─→ TRANSFORM 1 ─┐
 * SOURCE─┤                 ├─→ SINK
 *         └─→ TRANSFORM 2 ─┘
 *
 * Demonstrates both split and merge in one graph.
 */

console.log("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
console.log("Example: Diamond Pattern");
console.log("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");

// Clear graph
Graph.clear();

// Create nodes
console.log("\nCreating nodes...");

var source = Graph.createNode("SOURCE", 100, 250);
console.log("✅ SOURCE at (100, 250)");

var splitter = Graph.createNode("SPLIT", 250, 250);
console.log("✅ SPLIT at (250, 250)");

var transform1 = Graph.createNode("TRANSFORM", 400, 150);
console.log("✅ TRANSFORM 1 at (400, 150)");

var transform2 = Graph.createNode("TRANSFORM", 400, 350);
console.log("✅ TRANSFORM 2 at (400, 350)");

var merger = Graph.createNode("MERGE", 550, 250);
console.log("✅ MERGE at (550, 250)");

var sink = Graph.createNode("SINK", 700, 250);
console.log("✅ SINK at (700, 250)");

console.log("\nConnecting diamond pattern...");

// SOURCE → SPLIT
Graph.connect(source, 0, splitter, 0);
console.log("✅ SOURCE → SPLIT");

// SPLIT → TRANSFORM branches
// SPLIT socket layout: [0=INPUT, 1=OUTPUT1, 2=OUTPUT2]
Graph.connect(splitter, 1, transform1, 0);
console.log("✅ SPLIT (output 1) → TRANSFORM 1");

Graph.connect(splitter, 2, transform2, 0);
console.log("✅ SPLIT (output 2) → TRANSFORM 2");

// TRANSFORM → MERGE convergence
// MERGE socket layout: [0=INPUT1, 1=INPUT2, 2=OUTPUT]
Graph.connect(transform1, 1, merger, 0);
console.log("✅ TRANSFORM 1 → MERGE (input 1)");

Graph.connect(transform2, 1, merger, 1);
console.log("✅ TRANSFORM 2 → MERGE (input 2)");

// MERGE → SINK
Graph.connect(merger, 2, sink, 0);
console.log("✅ MERGE → SINK");

// Show final stats
var stats = Graph.getStats();
console.log("\nFinal graph:");
console.log("  Nodes:", stats.nodes);
console.log("  Edges:", stats.edges);
console.log("  Pattern: Diamond (split → parallel → merge)");

Graph.saveXml("diamond_pattern.xml");
console.log("\n✅ Graph saved to diamond_pattern.xml");

console.log("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
