// create_batch.js - Create large graph using batch mode for performance
console.log("=== Creating Large Graph with Batch Mode ===");

graph.clearGraph();

// Use batch mode to suppress observer notifications during bulk creation
graph.beginBatch();
console.log("Batch mode started");

var nodeCount = 20;
var nodes = [];

// Create a long chain of nodes
for (var i = 0; i < nodeCount; i++) {
    var type;
    if (i === 0) {
        type = "SOURCE";
    } else if (i === nodeCount - 1) {
        type = "SINK";
    } else {
        type = "TRANSFORM";
    }

    var nodeId = graph.createNode(type, 100 + i * 80, 300);
    nodes.push(nodeId);
}

console.log("Created " + nodeCount + " nodes");

// Connect them in a chain
var edgeCount = 0;
for (var i = 0; i < nodes.length - 1; i++) {
    var edgeId = graph.connectNodes(nodes[i], 0, nodes[i + 1], 0);
    if (edgeId) {
        edgeCount++;
    }
}

console.log("Created " + edgeCount + " edges");

// End batch mode - triggers all observer notifications at once
graph.endBatch();
console.log("Batch mode ended");

// Report stats
var stats = graph.getGraphStats();
console.log("");
console.log("=== Large Graph Complete ===");
console.log("Nodes: " + stats.nodeCount);
console.log("Edges: " + stats.edgeCount);
console.log("Performance: Batch mode minimized observer overhead");
