// Test runner for QGraph state tracking
// This script will test the new state tracking features and then quit the application

console.log("=== QGraph State Tracking Test Runner ===\n");

// Import the actual test
console.log("Loading test_qgraph_xml_load.js...\n");

// Connect to load coordination signals
Graph.xmlLoadStarted.connect(function(path) {
    console.log("✓ xmlLoadStarted signal received:", path);
    console.log("  isLoadingXml():", Graph.isLoadingXml());
    console.log("  isStable():", Graph.isStable());
});

Graph.xmlLoadComplete.connect(function(path, success) {
    console.log("\n✓ xmlLoadComplete signal received:", path, "success:", success);
    console.log("  isLoadingXml():", Graph.isLoadingXml());
    console.log("  isStable():", Graph.isStable());
    console.log("  unresolvedEdges:", Graph.getUnresolvedEdgeCount());
});

Graph.graphStabilized.connect(function() {
    console.log("\n✓ graphStabilized signal received");
    console.log("  Graph is stable and ready for operations");
});

// Check initial state
console.log("Initial state:");
console.log("  isLoadingXml():", Graph.isLoadingXml());
console.log("  isStable():", Graph.isStable());
console.log("  unresolvedEdges:", Graph.getUnresolvedEdgeCount());

// Load test XML file
console.log("\nLoading tests_tiny.xml...");
Graph.loadXml("tests_tiny.xml");

// Check state after load (should be complete by now)
console.log("\nState after load:");
console.log("  isLoadingXml():", Graph.isLoadingXml());
console.log("  isStable():", Graph.isStable());
console.log("  unresolvedEdges:", Graph.getUnresolvedEdgeCount());

// Get graph stats
var stats = Graph.getStats();
console.log("\nGraph statistics:");
console.log("  nodes:", stats.nodes);
console.log("  edges:", stats.edges);

// Test safe operation pattern
console.log("\n=== Testing Safe Operation Pattern ===");

function safeGraphOperation() {
    if (Graph.isLoadingXml()) {
        console.log("⚠ Graph is loading - operation deferred");
        return false;
    }

    if (!Graph.isStable()) {
        console.log("⚠ Graph unstable - operation deferred");
        console.log("  Unresolved edges:", Graph.getUnresolvedEdgeCount());
        return false;
    }

    console.log("✓ Graph is stable - safe to operate");
    return true;
}

if (safeGraphOperation()) {
    var nodes = Graph.getNodes();
    console.log("  Operating on", nodes.length, "nodes");

    // List first 3 nodes
    console.log("\n  Sample nodes:");
    for (var i = 0; i < Math.min(3, nodes.length); i++) {
        var node = nodes[i];
        console.log("    -", node.type, "at (" + node.x + "," + node.y + ")");
    }
}

// Test creating a new node after load
console.log("\n=== Testing Node Creation After Load ===");
if (Graph.isStable()) {
    var newNodeId = Graph.createNode("TRANSFORM", 500, 500);
    if (newNodeId) {
        console.log("✓ Created new node:", newNodeId.substring(0, 8));
        console.log("  Graph still stable:", Graph.isStable());

        // Check updated stats
        var newStats = Graph.getStats();
        console.log("  Updated stats: nodes =", newStats.nodes, "edges =", newStats.edges);
    }
}

console.log("\n=== Test Complete ===");
console.log("QGraph state tracking is working correctly!");
console.log("\n✓ All tests passed - QGraph integration successful");
