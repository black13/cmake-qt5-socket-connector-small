/**
 * Example: Stress Test
 *
 * Creates a large graph to test performance:
 * - Multiple parallel chains
 * - Many nodes and edges
 * - Programmatic generation
 *
 * Demonstrates handling of larger graphs.
 */

console.log("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
console.log("Example: Stress Test");
console.log("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");

// Clear graph
Graph.clear();

// Configuration
var chains = 5;          // Number of parallel chains
var nodesPerChain = 10;  // Length of each chain
var chainSpacing = 150;  // Vertical spacing between chains
var nodeSpacing = 150;   // Horizontal spacing between nodes

console.log(`\nCreating ${chains} chains of ${nodesPerChain} nodes each...`);
console.log(`Total nodes to create: ${chains * nodesPerChain}`);

var startTime = new Date().getTime();

// Create chains
var createdNodes = 0;
var createdEdges = 0;

for (var chain = 0; chain < chains; chain++) {
    var chainNodes = [];
    var y = 100 + chain * chainSpacing;

    // Create nodes for this chain
    for (var i = 0; i < nodesPerChain; i++) {
        var x = 100 + i * nodeSpacing;

        // Determine node type
        var nodeType;
        if (i === 0) {
            nodeType = "SOURCE";
        } else if (i === nodesPerChain - 1) {
            nodeType = "SINK";
        } else {
            nodeType = "TRANSFORM";
        }

        var nodeId = Graph.createNode(nodeType, x, y);
        chainNodes.push(nodeId);
        createdNodes++;
    }

    // Connect nodes in this chain
    for (var i = 0; i < nodesPerChain - 1; i++) {
        var fromSocketIdx = (i === 0) ? 0 : 1;  // SOURCE uses 0, TRANSFORM uses 1
        var edgeId = Graph.connect(chainNodes[i], fromSocketIdx, chainNodes[i + 1], 0);

        if (edgeId !== "") {
            createdEdges++;
        }
    }

    // Progress indicator
    var progress = ((chain + 1) / chains * 100).toFixed(0);
    console.log(`  Chain ${chain + 1}/${chains} completed (${progress}%)`);
}

var endTime = new Date().getTime();
var duration = endTime - startTime;

// Verify results
var stats = Graph.getStats();
console.log("\n" + "=".repeat(50));
console.log("Stress Test Results:");
console.log("=".repeat(50));
console.log("Nodes created:", createdNodes);
console.log("Edges created:", createdEdges);
console.log("Graph stats - Nodes:", stats.nodes);
console.log("Graph stats - Edges:", stats.edges);
console.log("Creation time:", duration, "ms");
console.log("Average time per node:", (duration / createdNodes).toFixed(2), "ms");
console.log("=".repeat(50));

// Validation
if (stats.nodes === createdNodes && stats.edges === createdEdges) {
    console.log("✅ All nodes and edges created successfully!");
} else {
    console.error("❌ Mismatch in node/edge counts!");
}

Graph.saveXml("stress_test.xml");
console.log("\n✅ Graph saved to stress_test.xml");

console.log("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
