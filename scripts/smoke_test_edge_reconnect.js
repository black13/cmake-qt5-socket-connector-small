// Smoke Test: Edge Deletion and Reconnection
// Tests that deleting an edge properly clears socket pointers
// allowing the same sockets to be reconnected

console.log("=== Smoke Test: Edge Deletion and Reconnection ===");

try {
    // Clean slate
    Graph.clear();

    // Create 2 nodes
    console.log("Creating 2 nodes...");
    let node1 = Graph.createNode("SOURCE", 100, 100);
    let node2 = Graph.createNode("SINK", 300, 100);

    if (!node1 || !node2) {
        console.log("❌ FAILED: Could not create nodes");
        "FAILED";
    }

    console.log("✓ Created nodes: " + node1 + ", " + node2);

    // Connect them
    console.log("Connecting nodes...");
    let edge1 = Graph.connect(node1, 0, node2, 0);

    if (!edge1) {
        console.log("❌ FAILED: Could not create edge");
        "FAILED";
    }

    console.log("✓ Connected: edge " + edge1);

    // Verify graph state
    let stats1 = Graph.getStats();
    if (stats1.nodes !== 2 || stats1.edges !== 1) {
        console.log("❌ FAILED: Expected 2 nodes, 1 edge. Got " + stats1.nodes + " nodes, " + stats1.edges + " edges");
        "FAILED";
    }

    console.log("✓ Graph state correct: 2 nodes, 1 edge");

    // Delete the edge
    console.log("Deleting edge...");
    let deleteResult = Graph.deleteEdge(edge1);

    if (!deleteResult) {
        console.log("❌ FAILED: Could not delete edge");
        "FAILED";
    }

    console.log("✓ Deleted edge");

    // Verify edge is gone
    let stats2 = Graph.getStats();
    if (stats2.nodes !== 2 || stats2.edges !== 0) {
        console.log("❌ FAILED: Expected 2 nodes, 0 edges after deletion. Got " + stats2.nodes + " nodes, " + stats2.edges + " edges");
        "FAILED";
    }

    console.log("✓ Edge deleted successfully");

    // CRITICAL TEST: Reconnect the same sockets
    console.log("Reconnecting same sockets (tests socket pointer cleanup)...");
    let edge2 = Graph.connect(node1, 0, node2, 0);

    if (!edge2) {
        console.log("❌ FAILED: Could not reconnect - socket pointers not cleared!");
        "FAILED - SOCKET POINTER LEAK";
    }

    console.log("✓ Reconnected successfully: edge " + edge2);

    // Verify final state
    let stats3 = Graph.getStats();
    if (stats3.nodes !== 2 || stats3.edges !== 1) {
        console.log("❌ FAILED: Expected 2 nodes, 1 edge after reconnect. Got " + stats3.nodes + " nodes, " + stats3.edges + " edges");
        "FAILED";
    }

    console.log("✓ Final graph state correct: 2 nodes, 1 edge");
    console.log("✅ PASSED: Edge deletion properly clears socket pointers");

    "PASSED";

} catch (e) {
    console.log("❌ EXCEPTION: " + e.toString());
    "EXCEPTION";
}
