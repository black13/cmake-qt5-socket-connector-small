// Complete Graph Production Test
// Tests node creation and connection to produce a working graph

console.log("=== COMPLETE GRAPH PRODUCTION TEST ===");

try {
    // Step 1: Clear any existing graph
    console.log("Step 1: Clearing existing graph");
    Graph.clear();
    
    var stats = Graph.getStats();
    console.log("Graph cleared. Stats:", JSON.stringify(stats));
    
    // Step 2: Create processing pipeline nodes
    console.log("Step 2: Creating processing pipeline");
    
    var sourceId = Graph.createNode("SOURCE", 100, 100);
    console.log("Created SOURCE node ID:", sourceId);
    
    var transform1Id = Graph.createNode("TRANSFORM", 300, 100);
    console.log("Created TRANSFORM node 1 ID:", transform1Id);
    
    var transform2Id = Graph.createNode("TRANSFORM", 500, 100);
    console.log("Created TRANSFORM node 2 ID:", transform2Id);
    
    var sinkId = Graph.createNode("SINK", 700, 100);
    console.log("Created SINK node ID:", sinkId);
    
    // Step 3: Verify nodes created
    stats = Graph.getStats();
    console.log("After node creation. Stats:", JSON.stringify(stats));
    
    if (stats.nodes < 4) {
        console.error("FAILED: Expected 4 nodes, got", stats.nodes);
        var errorResult = { success: false, message: "Node creation failed" };
        errorResult;
    }
    
    // Step 4: Create connections
    console.log("Step 4: Creating connections");
    
    var edge1Id = Graph.connect(sourceId, 0, transform1Id, 0);
    console.log("Created edge 1 ID:", edge1Id, "connecting", sourceId, "->", transform1Id);
    
    var edge2Id = Graph.connect(transform1Id, 0, transform2Id, 0);
    console.log("Created edge 2 ID:", edge2Id, "connecting", transform1Id, "->", transform2Id);
    
    var edge3Id = Graph.connect(transform2Id, 0, sinkId, 0);
    console.log("Created edge 3 ID:", edge3Id, "connecting", transform2Id, "->", sinkId);
    
    // Step 5: Verify complete graph
    console.log("Step 5: Verifying complete graph");
    stats = Graph.getStats();
    console.log("Final graph stats:", JSON.stringify(stats));
    
    // Step 6: Validation
    if (stats.nodes >= 4 && stats.edges >= 3) {
        console.log("SUCCESS: Complete graph created!");
        console.log("Nodes:", stats.nodes, "Edges:", stats.edges);
        
        var result = {
            success: true,
            message: "Complete graph production successful",
            nodes: stats.nodes,
            edges: stats.edges,
            pipeline: ["SOURCE", "TRANSFORM", "TRANSFORM", "SINK"]
        };
        result;
    } else {
        console.error("FAILED: Incomplete graph");
        console.error("Expected: 4+ nodes, 3+ edges");
        console.error("Got:", stats.nodes, "nodes,", stats.edges, "edges");
        
        var errorResult = {
            success: false,
            message: "Graph production failed - insufficient connections",
            expectedNodes: 4,
            actualNodes: stats.nodes,
            expectedEdges: 3,
            actualEdges: stats.edges
        };
        errorResult;
    }
    
} catch (error) {
    console.error("COMPLETE_GRAPH_TEST: Exception occurred:", error.message);
    var errorResult = {
        success: false,
        message: "Test failed with exception: " + error.message
    };
    errorResult;
}