// Simple Node Creation Test
// Basic test to verify JavaScript can create nodes

console.log("=== Simple Node Creation Test ===");

try {
    // Clear any existing graph
    Graph.clear();
    console.log("Cleared graph");
    
    // Test creating each node type
    const source = Graph.createNode("SOURCE", 100, 100);
    console.log("Created SOURCE node:", source);
    
    const sink = Graph.createNode("SINK", 300, 100);
    console.log("Created SINK node:", sink);
    
    const transform = Graph.createNode("TRANSFORM", 200, 200);
    console.log("Created TRANSFORM node:", transform);
    
    const split = Graph.createNode("SPLIT", 100, 300);
    console.log("Created SPLIT node:", split);
    
    const merge = Graph.createNode("MERGE", 300, 300);
    console.log("Created MERGE node:", merge);
    
    // Get final stats
    const stats = Graph.getStats();
    console.log("Final stats:", stats.nodes, "nodes,", stats.edges, "edges");
    
    if (stats.nodes === 5) {
        console.log("SUCCESS: All 5 node types created");
        var result = { success: true, message: "All 5 node types created successfully" };
        result;
    } else {
        console.log("FAILURE: Expected 5 nodes, got", stats.nodes);
        var result = { success: false, message: `Expected 5 nodes, got ${stats.nodes}` };
        result;
    }
    
} catch (error) {
    console.error("Test failed:", error.message);
    var errorResult = { success: false, message: `Test failed: ${error.message}` };
    errorResult;
}