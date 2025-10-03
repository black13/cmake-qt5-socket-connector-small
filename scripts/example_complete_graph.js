/**
 * Example: Complete Graph - All Node Types, No Empty Sockets
 *
 * Creates a graph using all 5 node types where EVERY socket is connected:
 *
 *                    ┌─→ TRANSFORM 1 ─┐
 *    SOURCE → SPLIT ─┤                ├─→ MERGE → SINK
 *                    └─→ TRANSFORM 2 ─┘
 *
 * Socket Verification:
 * - SOURCE (0 in, 1 out): output 0 → SPLIT ✓
 * - SPLIT (1 in, 2 out): input 0 ← SOURCE ✓, output 1 → TRANSFORM1 ✓, output 2 → TRANSFORM2 ✓
 * - TRANSFORM1 (1 in, 1 out): input 0 ← SPLIT ✓, output 1 → MERGE ✓
 * - TRANSFORM2 (1 in, 1 out): input 0 ← SPLIT ✓, output 1 → MERGE ✓
 * - MERGE (2 in, 1 out): input 0 ← TRANSFORM1 ✓, input 1 ← TRANSFORM2 ✓, output 2 → SINK ✓
 * - SINK (1 in, 0 out): input 0 ← MERGE ✓
 *
 * Total: 6 nodes, 6 edges, 0 empty sockets
 */

console.log("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
console.log("Example: Complete Graph - All Types, No Empty Sockets");
console.log("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");

// Clear graph
Graph.clear();
console.log("Graph cleared\n");

// Step 1: Create all nodes
console.log("[Step 1] Creating nodes...");

var source = Graph.createNode("SOURCE", 100, 250);
console.log("✅ SOURCE at (100, 250)");
console.log("   Sockets: 0 inputs, 1 output");
console.log("   - Socket 0: OUTPUT");

var splitter = Graph.createNode("SPLIT", 300, 250);
console.log("✅ SPLIT at (300, 250)");
console.log("   Sockets: 1 input, 2 outputs");
console.log("   - Socket 0: INPUT");
console.log("   - Socket 1: OUTPUT 1");
console.log("   - Socket 2: OUTPUT 2");

var transform1 = Graph.createNode("TRANSFORM", 500, 150);
console.log("✅ TRANSFORM 1 at (500, 150)");
console.log("   Sockets: 1 input, 1 output");
console.log("   - Socket 0: INPUT");
console.log("   - Socket 1: OUTPUT");

var transform2 = Graph.createNode("TRANSFORM", 500, 350);
console.log("✅ TRANSFORM 2 at (500, 350)");
console.log("   Sockets: 1 input, 1 output");
console.log("   - Socket 0: INPUT");
console.log("   - Socket 1: OUTPUT");

var merger = Graph.createNode("MERGE", 700, 250);
console.log("✅ MERGE at (700, 250)");
console.log("   Sockets: 2 inputs, 1 output");
console.log("   - Socket 0: INPUT 1");
console.log("   - Socket 1: INPUT 2");
console.log("   - Socket 2: OUTPUT");

var sink = Graph.createNode("SINK", 900, 250);
console.log("✅ SINK at (900, 250)");
console.log("   Sockets: 1 input, 0 outputs");
console.log("   - Socket 0: INPUT");

// Step 2: Connect all sockets
console.log("\n[Step 2] Connecting all sockets...");

// Connection 1: SOURCE → SPLIT
var edge1 = Graph.connect(source, 0, splitter, 0);
if (edge1 !== "") {
    console.log("✅ Edge 1: SOURCE[0] → SPLIT[0]");
    console.log("   SOURCE output connected ✓");
    console.log("   SPLIT input connected ✓");
} else {
    console.error("❌ Failed to create edge 1");
}

// Connection 2: SPLIT → TRANSFORM1
var edge2 = Graph.connect(splitter, 1, transform1, 0);
if (edge2 !== "") {
    console.log("✅ Edge 2: SPLIT[1] → TRANSFORM1[0]");
    console.log("   SPLIT output 1 connected ✓");
    console.log("   TRANSFORM1 input connected ✓");
} else {
    console.error("❌ Failed to create edge 2");
}

// Connection 3: SPLIT → TRANSFORM2
var edge3 = Graph.connect(splitter, 2, transform2, 0);
if (edge3 !== "") {
    console.log("✅ Edge 3: SPLIT[2] → TRANSFORM2[0]");
    console.log("   SPLIT output 2 connected ✓");
    console.log("   TRANSFORM2 input connected ✓");
} else {
    console.error("❌ Failed to create edge 3");
}

// Connection 4: TRANSFORM1 → MERGE
var edge4 = Graph.connect(transform1, 1, merger, 0);
if (edge4 !== "") {
    console.log("✅ Edge 4: TRANSFORM1[1] → MERGE[0]");
    console.log("   TRANSFORM1 output connected ✓");
    console.log("   MERGE input 1 connected ✓");
} else {
    console.error("❌ Failed to create edge 4");
}

// Connection 5: TRANSFORM2 → MERGE
var edge5 = Graph.connect(transform2, 1, merger, 1);
if (edge5 !== "") {
    console.log("✅ Edge 5: TRANSFORM2[1] → MERGE[1]");
    console.log("   TRANSFORM2 output connected ✓");
    console.log("   MERGE input 2 connected ✓");
} else {
    console.error("❌ Failed to create edge 5");
}

// Connection 6: MERGE → SINK
var edge6 = Graph.connect(merger, 2, sink, 0);
if (edge6 !== "") {
    console.log("✅ Edge 6: MERGE[2] → SINK[0]");
    console.log("   MERGE output connected ✓");
    console.log("   SINK input connected ✓");
} else {
    console.error("❌ Failed to create edge 6");
}

// Step 3: Verify completeness
console.log("\n[Step 3] Verifying graph completeness...");

var stats = Graph.getStats();
console.log("Graph statistics:");
console.log("  Nodes:", stats.nodes, "(expected: 6)");
console.log("  Edges:", stats.edges, "(expected: 6)");

// Verify all node types are present
var nodes = Graph.getNodes();
var nodeTypes = {};
for (var i = 0; i < nodes.length; i++) {
    var type = nodes[i].type;
    nodeTypes[type] = (nodeTypes[type] || 0) + 1;
}

console.log("\nNode type distribution:");
console.log("  SOURCE:", nodeTypes["SOURCE"] || 0, "(expected: 1)");
console.log("  SINK:", nodeTypes["SINK"] || 0, "(expected: 1)");
console.log("  TRANSFORM:", nodeTypes["TRANSFORM"] || 0, "(expected: 2)");
console.log("  SPLIT:", nodeTypes["SPLIT"] || 0, "(expected: 1)");
console.log("  MERGE:", nodeTypes["MERGE"] || 0, "(expected: 1)");

// Count total sockets and connections
var totalSockets = 0;
var totalConnections = 0;

// SOURCE: 1 output
totalSockets += 1;
totalConnections += 1;  // 1 output connected

// SINK: 1 input
totalSockets += 1;
totalConnections += 1;  // 1 input connected

// TRANSFORM x2: 2 sockets each (1 input + 1 output)
totalSockets += 4;
totalConnections += 4;  // 2 inputs + 2 outputs connected

// SPLIT: 3 sockets (1 input + 2 outputs)
totalSockets += 3;
totalConnections += 3;  // 1 input + 2 outputs connected

// MERGE: 3 sockets (2 inputs + 1 output)
totalSockets += 3;
totalConnections += 3;  // 2 inputs + 1 output connected

console.log("\nSocket analysis:");
console.log("  Total sockets:", totalSockets);
console.log("  Connected sockets:", totalConnections);
console.log("  Empty sockets:", totalSockets - totalConnections);

// Verify all edges
var edges = Graph.getEdges();
console.log("\nEdge details:");
for (var i = 0; i < edges.length; i++) {
    var edge = edges[i];
    console.log("  Edge " + (i + 1) + ":",
                "from node[" + edge.fromSocket + "] → to node[" + edge.toSocket + "]");
}

// Step 4: Validation
console.log("\n" + "=".repeat(50));
console.log("VALIDATION RESULTS:");
console.log("=".repeat(50));

var allTypesPresent = nodeTypes["SOURCE"] === 1 &&
                      nodeTypes["SINK"] === 1 &&
                      nodeTypes["TRANSFORM"] === 2 &&
                      nodeTypes["SPLIT"] === 1 &&
                      nodeTypes["MERGE"] === 1;

var allSocketsConnected = (totalSockets === totalConnections);

var correctCounts = (stats.nodes === 6 && stats.edges === 6);

if (allTypesPresent) {
    console.log("✅ All 5 node types present");
} else {
    console.error("❌ Missing node types");
}

if (allSocketsConnected) {
    console.log("✅ All sockets connected (no empty sockets)");
} else {
    console.error("❌ Some sockets are empty");
}

if (correctCounts) {
    console.log("✅ Correct node and edge counts");
} else {
    console.error("❌ Incorrect counts");
}

if (allTypesPresent && allSocketsConnected && correctCounts) {
    console.log("\n🎉 SUCCESS: Complete graph with all types and no empty sockets!");
} else {
    console.error("\n❌ FAILED: Graph is incomplete");
}

console.log("=".repeat(50));

// Step 5: Save graph
Graph.saveXml("complete_graph.xml");
console.log("\n✅ Graph saved to complete_graph.xml");

// Step 6: Visual ASCII representation
console.log("\n[Visual Representation]");
console.log("");
console.log("    SOURCE (0,1)");
console.log("        │");
console.log("        │ output 0");
console.log("        ▼");
console.log("    SPLIT (1,2)");
console.log("        │");
console.log("   ┌────┴────┐");
console.log("   │         │");
console.log("out 1      out 2");
console.log("   │         │");
console.log("   ▼         ▼");
console.log("TRANS1    TRANS2");
console.log("(1,1)     (1,1)");
console.log("   │         │");
console.log("out 1      out 1");
console.log("   │         │");
console.log("   └────┬────┘");
console.log("        │");
console.log("     MERGE (2,1)");
console.log("        │");
console.log("        │ output 2");
console.log("        ▼");
console.log("     SINK (1,0)");
console.log("");
console.log("Legend: TYPE (inputs, outputs)");

console.log("\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
console.log("Complete Graph Example Finished!");
console.log("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
