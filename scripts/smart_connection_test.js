// Smart Connection Test - Uses socket introspection for dynamic connections
console.log("=== SMART CONNECTION TEST ===");

try {
    // Step 1: Clear graph
    Graph.clear();
    console.log("Graph cleared");
    
    // Step 2: Create nodes
    console.log("Creating nodes...");
    var sourceId = Graph.createNode("SOURCE", 100, 100);
    var transformId = Graph.createNode("TRANSFORM", 300, 100);  
    var sinkId = Graph.createNode("SINK", 500, 100);
    
    console.log("Created nodes:", sourceId, transformId, sinkId);
    
    // Step 3: Introspect socket configurations
    console.log("\n=== SOCKET INTROSPECTION ===");
    
    var sourceInputs = Graph.getInputSockets(sourceId);
    var sourceOutputs = Graph.getOutputSockets(sourceId);
    console.log("SOURCE - Inputs:", JSON.stringify(sourceInputs), "Outputs:", JSON.stringify(sourceOutputs));
    
    var transformInputs = Graph.getInputSockets(transformId);
    var transformOutputs = Graph.getOutputSockets(transformId);
    console.log("TRANSFORM - Inputs:", JSON.stringify(transformInputs), "Outputs:", JSON.stringify(transformOutputs));
    
    var sinkInputs = Graph.getInputSockets(sinkId);
    var sinkOutputs = Graph.getOutputSockets(sinkId);
    console.log("SINK - Inputs:", JSON.stringify(sinkInputs), "Outputs:", JSON.stringify(sinkOutputs));
    
    // Step 4: Smart connection using discovered sockets
    console.log("\n=== SMART CONNECTIONS ===");
    
    // Find first available output from source and first available input from transform
    if (sourceOutputs.length > 0 && transformInputs.length > 0) {
        var sourceOutIndex = sourceOutputs[0].index;
        var transformInIndex = transformInputs[0].index;
        
        console.log("Connecting SOURCE[" + sourceOutIndex + "] -> TRANSFORM[" + transformInIndex + "]");
        
        // Validate connection first
        if (Graph.canConnect(sourceId, sourceOutIndex, transformId, transformInIndex)) {
            var edge1 = Graph.connect(sourceId, sourceOutIndex, transformId, transformInIndex);
            console.log("Connection 1 successful:", edge1);
        } else {
            console.error("Connection 1 validation failed");
        }
    } else {
        console.error("Cannot find compatible sockets for SOURCE -> TRANSFORM");
    }
    
    // Find first available output from transform and first available input from sink
    if (transformOutputs.length > 0 && sinkInputs.length > 0) {
        var transformOutIndex = transformOutputs[0].index;
        var sinkInIndex = sinkInputs[0].index;
        
        console.log("Connecting TRANSFORM[" + transformOutIndex + "] -> SINK[" + sinkInIndex + "]");
        
        // Validate connection first
        if (Graph.canConnect(transformId, transformOutIndex, sinkId, sinkInIndex)) {
            var edge2 = Graph.connect(transformId, transformOutIndex, sinkId, sinkInIndex);
            console.log("Connection 2 successful:", edge2);
        } else {
            console.error("Connection 2 validation failed");
        }
    } else {
        console.error("Cannot find compatible sockets for TRANSFORM -> SINK");
    }
    
    // Step 5: Test invalid connections (should be caught by validation)
    console.log("\n=== TESTING INVALID CONNECTIONS ===");
    
    // Try to connect INPUT to INPUT (should fail)
    if (transformInputs.length > 0 && sinkInputs.length > 0) {
        console.log("Testing invalid connection: TRANSFORM[INPUT] -> SINK[INPUT]");
        if (Graph.canConnect(transformId, transformInputs[0].index, sinkId, sinkInputs[0].index)) {
            console.error("VALIDATION BUG: INPUT->INPUT connection should be rejected");
        } else {
            console.log("SUCCESS: INPUT->INPUT connection properly rejected");
        }
    }
    
    // Step 6: Final verification
    console.log("\n=== FINAL VERIFICATION ===");
    var stats = Graph.getStats();
    console.log("Final stats:", JSON.stringify(stats));
    
    if (stats.nodes >= 3 && stats.edges >= 2) {
        console.log("SUCCESS: Smart connection test completed");
        var result = {
            success: true,
            message: "Smart connection test successful",
            nodesCreated: stats.nodes,
            edgesCreated: stats.edges,
            socketIntrospectionWorked: true
        };
        result;
    } else {
        console.error("FAILED: Expected 3+ nodes, 2+ edges. Got:", stats.nodes, "nodes,", stats.edges, "edges");
        var errorResult = {
            success: false,
            message: "Smart connection test failed",
            nodesCreated: stats.nodes,
            edgesCreated: stats.edges
        };
        errorResult;
    }
    
} catch (error) {
    console.error("SMART_CONNECTION_TEST: Exception:", error.message);
    var errorResult = {
        success: false,
        message: "Test failed with exception: " + error.message
    };
    errorResult;
}