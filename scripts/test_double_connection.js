// Test Double Connection Prevention
console.log("=== DOUBLE CONNECTION PREVENTION TEST ===");

try {
    // Clear any existing graph
    Graph.clear();
    
    // Create test nodes
    var sourceId = Graph.createNode("SOURCE", 100, 100);
    var transform1Id = Graph.createNode("TRANSFORM", 300, 100); 
    var transform2Id = Graph.createNode("TRANSFORM", 500, 100);
    
    console.log("Created nodes:");
    console.log("  SOURCE:", sourceId);
    console.log("  TRANSFORM1:", transform1Id);
    console.log("  TRANSFORM2:", transform2Id);
    
    console.log("\n=== TESTING DOUBLE CONNECTION ===");
    
    // First connection - should succeed
    console.log("1. Attempting: SOURCE(0) -> TRANSFORM1(0) [FIRST CONNECTION]");
    var edge1Id = Graph.connect(sourceId, 0, transform1Id, 0);
    if (edge1Id) {
        console.log("   SUCCESS:", edge1Id);
    } else {
        console.log("   FAILED: No edge ID returned");
    }
    
    // Second connection to same input socket - should fail
    console.log("2. Attempting: TRANSFORM2(1) -> TRANSFORM1(0) [DOUBLE CONNECTION - SHOULD FAIL]");
    var edge2Id = Graph.connect(transform2Id, 1, transform1Id, 0);
    if (edge2Id) {
        console.log("   PROBLEM: Double connection succeeded! Edge ID:", edge2Id);
    } else {
        console.log("   CORRECT: Double connection blocked as expected");
    }
    
    // Check final graph stats
    var stats = Graph.getStats();
    console.log("\nFinal graph stats:", JSON.stringify(stats));
    
    var result = {
        success: !edge2Id, // Success if second connection was blocked
        message: edge2Id ? "Double connection prevention FAILED" : "Double connection prevention WORKING",
        edge1: edge1Id,
        edge2: edge2Id,
        stats: stats
    };
    result;
    
} catch (error) {
    console.error("TEST FAILED:", error.message);
    var errorResult = {
        success: false,
        message: "Test script failed: " + error.message
    };
    errorResult;
}