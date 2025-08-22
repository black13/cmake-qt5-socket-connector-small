// Debug Socket Configuration - Investigate socket availability
console.log("=== DEBUG SOCKET CONFIGURATION ===");

try {
    // Create the same nodes as the failing test
    Graph.clear();
    
    var sourceId = Graph.createNode("SOURCE", 100, 100);
    var transform1Id = Graph.createNode("TRANSFORM", 300, 100);
    var transform2Id = Graph.createNode("TRANSFORM", 500, 100);
    var sinkId = Graph.createNode("SINK", 700, 100);
    
    console.log("Created nodes:");
    console.log("  SOURCE:", sourceId);
    console.log("  TRANSFORM1:", transform1Id);
    console.log("  TRANSFORM2:", transform2Id);
    console.log("  SINK:", sinkId);
    
    // Detailed socket introspection for each node
    console.log("\n=== DETAILED SOCKET INTROSPECTION ===");
    
    // SOURCE node
    console.log("SOURCE node sockets:");
    var sourceInputs = Graph.getInputSockets(sourceId);
    var sourceOutputs = Graph.getOutputSockets(sourceId);
    console.log("  Inputs (" + sourceInputs.length + "):", JSON.stringify(sourceInputs));
    console.log("  Outputs (" + sourceOutputs.length + "):", JSON.stringify(sourceOutputs));
    
    // TRANSFORM1 node
    console.log("TRANSFORM1 node sockets:");
    var transform1Inputs = Graph.getInputSockets(transform1Id);
    var transform1Outputs = Graph.getOutputSockets(transform1Id);
    console.log("  Inputs (" + transform1Inputs.length + "):", JSON.stringify(transform1Inputs));
    console.log("  Outputs (" + transform1Outputs.length + "):", JSON.stringify(transform1Outputs));
    
    // TRANSFORM2 node
    console.log("TRANSFORM2 node sockets:");
    var transform2Inputs = Graph.getInputSockets(transform2Id);
    var transform2Outputs = Graph.getOutputSockets(transform2Id);
    console.log("  Inputs (" + transform2Inputs.length + "):", JSON.stringify(transform2Inputs));
    console.log("  Outputs (" + transform2Outputs.length + "):", JSON.stringify(transform2Outputs));
    
    // SINK node
    console.log("SINK node sockets:");
    var sinkInputs = Graph.getInputSockets(sinkId);
    var sinkOutputs = Graph.getOutputSockets(sinkId);
    console.log("  Inputs (" + sinkInputs.length + "):", JSON.stringify(sinkInputs));
    console.log("  Outputs (" + sinkOutputs.length + "):", JSON.stringify(sinkOutputs));
    
    // Test individual socket lookups
    console.log("\n=== INDIVIDUAL SOCKET TESTS ===");
    
    // Test SOURCE socket 0
    if (sourceOutputs.length > 0) {
        var sourceSocket0 = Graph.getSocketInfo(sourceId, 0);
        console.log("SOURCE socket 0:", JSON.stringify(sourceSocket0));
    }
    
    // Test TRANSFORM1 sockets
    if (transform1Inputs.length > 0) {
        var transform1Socket0 = Graph.getSocketInfo(transform1Id, 0);
        console.log("TRANSFORM1 socket 0:", JSON.stringify(transform1Socket0));
    }
    if (transform1Outputs.length > 0) {
        var transform1Socket1 = Graph.getSocketInfo(transform1Id, 1);
        console.log("TRANSFORM1 socket 1:", JSON.stringify(transform1Socket1));
    }
    
    // Test connections with validation
    console.log("\n=== CONNECTION VALIDATION TESTS ===");
    
    // Test 1: SOURCE -> TRANSFORM1
    console.log("Testing SOURCE(0) -> TRANSFORM1(0):");
    var canConnect1 = Graph.canConnect(sourceId, 0, transform1Id, 0);
    console.log("  Can connect:", canConnect1);
    
    // Test 2: TRANSFORM1 -> TRANSFORM2
    console.log("Testing TRANSFORM1(1) -> TRANSFORM2(0):");
    var canConnect2 = Graph.canConnect(transform1Id, 1, transform2Id, 0);
    console.log("  Can connect:", canConnect2);
    
    // Test 3: TRANSFORM2 -> SINK
    console.log("Testing TRANSFORM2(1) -> SINK(0):");
    var canConnect3 = Graph.canConnect(transform2Id, 1, sinkId, 0);
    console.log("  Can connect:", canConnect3);
    
    var result = {
        success: true,
        message: "Socket debug information collected",
        nodes: 4,
        socketInvestigation: "Check console output for details"
    };
    result;
    
} catch (error) {
    console.error("DEBUG_SOCKETS: Exception:", error.message);
    var errorResult = {
        success: false,
        message: "Socket debug failed: " + error.message
    };
    errorResult;
}