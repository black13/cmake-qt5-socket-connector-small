// Test: XML Load and Graph Enumeration
// Purpose: Validate JavaScript system health by loading XML and enumerating all graph elements
// Expected: 4 nodes (SOURCE, TRANSFORM, SINK, LOGGER), 3 edges
// Graph structure: SOURCE -> TRANSFORM -> SINK
//                    |
//                    +------> LOGGER

console.log("=== XML Load and Enumeration Test ===");
console.log("");

try {
    // Step 1: Clear existing graph
    console.log("Step 1: Clearing graph...");
    Graph.clear();

    let stats = Graph.getStats();
    if (stats.nodes !== 0 || stats.edges !== 0) {
        console.log("❌ FAILED: Graph not cleared. Nodes: " + stats.nodes + ", Edges: " + stats.edges);
        "FAILED";
    }
    console.log("✓ Graph cleared");
    console.log("");

    // Step 2: Load XML file
    console.log("Step 2: Loading test_graph.xml...");
    Graph.loadXml("test_graph.xml");

    // Small delay to allow loading (synchronous operation should complete immediately)
    stats = Graph.getStats();
    console.log("✓ XML loaded");
    console.log("");

    // Step 3: Verify graph statistics
    console.log("Step 3: Graph Statistics");
    console.log("  Nodes: " + stats.nodes);
    console.log("  Edges: " + stats.edges);
    console.log("");

    if (stats.nodes !== 4) {
        console.log("❌ FAILED: Expected 4 nodes, got " + stats.nodes);
        "FAILED";
    }

    if (stats.edges !== 3) {
        console.log("❌ FAILED: Expected 3 edges, got " + stats.edges);
        "FAILED";
    }

    console.log("✓ Graph statistics correct (4 nodes, 3 edges)");
    console.log("");

    // Step 4: Enumerate nodes
    console.log("Step 4: Node Enumeration");
    let nodes = Graph.getNodes();

    if (!nodes || nodes.length === 0) {
        console.log("❌ FAILED: getNodes() returned empty or null");
        "FAILED";
    }

    console.log("  Total nodes: " + nodes.length);
    console.log("");

    for (let i = 0; i < nodes.length; i++) {
        let node = nodes[i];
        let num = i + 1;

        console.log("  [" + num + "/" + nodes.length + "] Node:");
        console.log("    ID: " + node.id);
        console.log("    Type: " + node.type);
        console.log("    Position: (" + node.x + ", " + node.y + ")");

        if (node.inputs !== undefined && node.outputs !== undefined) {
            console.log("    Sockets: " + node.inputs + " inputs, " + node.outputs + " outputs");
        }

        console.log("");
    }

    console.log("✓ All nodes enumerated");
    console.log("");

    // Step 5: Enumerate edges
    console.log("Step 5: Edge Enumeration");
    let edges = Graph.getEdges();

    if (!edges || edges.length === 0) {
        console.log("❌ FAILED: getEdges() returned empty or null");
        "FAILED";
    }

    console.log("  Total edges: " + edges.length);
    console.log("");

    for (let i = 0; i < edges.length; i++) {
        let edge = edges[i];
        let num = i + 1;

        console.log("  [" + num + "/" + edges.length + "] Edge:");
        console.log("    ID: " + edge.id);
        console.log("    From: " + edge.fromNode + " [socket " + edge.fromSocket + "]");
        console.log("    To: " + edge.toNode + " [socket " + edge.toSocket + "]");
        console.log("");
    }

    console.log("✓ All edges enumerated");
    console.log("");

    // Step 6: Basic integrity validation
    console.log("Step 6: Integrity Validation");

    // Build node ID map for lookup
    let nodeIds = {};
    for (let i = 0; i < nodes.length; i++) {
        nodeIds[nodes[i].id] = true;
    }

    let orphanedEdges = 0;
    let invalidSockets = 0;

    for (let i = 0; i < edges.length; i++) {
        let edge = edges[i];

        // Check if fromNode exists
        if (!nodeIds[edge.fromNode]) {
            console.log("  ❌ Edge " + edge.id + " references non-existent fromNode: " + edge.fromNode);
            orphanedEdges++;
        }

        // Check if toNode exists
        if (!nodeIds[edge.toNode]) {
            console.log("  ❌ Edge " + edge.id + " references non-existent toNode: " + edge.toNode);
            orphanedEdges++;
        }

        // Check socket indices are non-negative
        if (edge.fromSocket < 0 || edge.toSocket < 0) {
            console.log("  ❌ Edge " + edge.id + " has invalid socket index");
            invalidSockets++;
        }
    }

    if (orphanedEdges > 0) {
        console.log("  ❌ Found " + orphanedEdges + " orphaned edge(s)");
    } else {
        console.log("  ✓ All edges reference valid nodes");
    }

    if (invalidSockets > 0) {
        console.log("  ❌ Found " + invalidSockets + " invalid socket index(es)");
    } else {
        console.log("  ✓ All socket indices are valid");
    }

    console.log("");

    // Final result
    if (orphanedEdges === 0 && invalidSockets === 0 && nodes.length === 4 && edges.length === 3) {
        console.log("✅ TEST PASSED");
        console.log("   - XML loaded successfully");
        console.log("   - 4 nodes enumerated");
        console.log("   - 3 edges enumerated");
        console.log("   - Graph integrity validated");
        console.log("   - JavaScript system is healthy");
        "PASSED";
    } else {
        console.log("❌ TEST FAILED");
        console.log("   - Issues found during validation");
        "FAILED";
    }

} catch (e) {
    console.log("");
    console.log("❌ EXCEPTION: " + e.toString());
    console.log("   JavaScript system encountered an error");
    "EXCEPTION";
}
