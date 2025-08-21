// Complete Node Graph - Every node type with maximum socket connectivity
console.log("=== COMPLETE NODE GRAPH CONFIGURATION ===");
console.log("Creating comprehensive graph with all 5 node types fully connected");

try {
    // Step 1: Clear and start fresh
    Graph.clear();
    console.log("Graph cleared");
    
    // Step 2: Create all 5 node types with strategic positioning
    console.log("\n=== CREATING ALL NODE TYPES ===");
    
    // Layer 1: Sources (left side)
    var source1Id = Graph.createNode("SOURCE", 100, 100);
    var source2Id = Graph.createNode("SOURCE", 100, 250);
    console.log("Created SOURCE nodes:", source1Id, source2Id);
    
    // Layer 2: Processing nodes (middle)
    var transformId = Graph.createNode("TRANSFORM", 300, 150);  
    var splitId = Graph.createNode("SPLIT", 300, 300);
    console.log("Created processing nodes:", transformId, splitId);
    
    // Layer 3: Merge node (right-middle)
    var mergeId = Graph.createNode("MERGE", 500, 200);
    console.log("Created MERGE node:", mergeId);
    
    // Layer 4: Final sink (right side)
    var sinkId = Graph.createNode("SINK", 700, 200);
    console.log("Created SINK node:", sinkId);
    
    // Step 3: Socket introspection for all nodes
    console.log("\n=== SOCKET INTROSPECTION ===");
    
    var allNodes = [
        {id: source1Id, type: "SOURCE"},
        {id: source2Id, type: "SOURCE"},
        {id: transformId, type: "TRANSFORM"},
        {id: splitId, type: "SPLIT"},
        {id: mergeId, type: "MERGE"},
        {id: sinkId, type: "SINK"}
    ];
    
    allNodes.forEach(function(node) {
        var inputs = Graph.getInputSockets(node.id);
        var outputs = Graph.getOutputSockets(node.id);
        console.log(node.type + " [" + node.id.substr(1, 8) + "...] - Inputs:", inputs.length, "Outputs:", outputs.length);
        console.log("  Input sockets:", JSON.stringify(inputs));
        console.log("  Output sockets:", JSON.stringify(outputs));
    });
    
    // Step 4: Create comprehensive connection network
    console.log("\n=== CREATING COMPREHENSIVE CONNECTIONS ===");
    
    var connections = [];
    var connectionCount = 0;
    
    function tryConnection(fromId, fromIndex, toId, toIndex, description) {
        console.log("Attempting: " + description);
        if (Graph.canConnect(fromId, fromIndex, toId, toIndex)) {
            var edgeId = Graph.connect(fromId, fromIndex, toId, toIndex);
            if (edgeId && edgeId.length > 0) {
                connections.push({
                    from: fromId,
                    fromIndex: fromIndex,
                    to: toId,
                    toIndex: toIndex,
                    edge: edgeId,
                    description: description
                });
                console.log("  ✅ SUCCESS: " + edgeId);
                connectionCount++;
                return true;
            } else {
                console.log("  ❌ FAILED: Connection returned empty");
                return false;
            }
        } else {
            console.log("  ❌ VALIDATION FAILED");
            return false;
        }
    }
    
    // Connection plan: Create a complex data flow network
    console.log("\n--- PRIMARY DATA FLOW ---");
    
    // SOURCE1 -> TRANSFORM -> MERGE
    tryConnection(source1Id, 0, transformId, 0, "SOURCE1(out:0) -> TRANSFORM(in:0)");
    tryConnection(transformId, 1, mergeId, 0, "TRANSFORM(out:1) -> MERGE(in:0)");
    
    // SOURCE2 -> SPLIT -> MERGE + SINK
    tryConnection(source2Id, 0, splitId, 0, "SOURCE2(out:0) -> SPLIT(in:0)");
    tryConnection(splitId, 1, mergeId, 1, "SPLIT(out:1) -> MERGE(in:1)");
    tryConnection(splitId, 2, sinkId, 0, "SPLIT(out:2) -> SINK(in:0) [ALTERNATE PATH]");
    
    // MERGE -> SINK (MAIN PATH)
    // Check if sink is already connected
    var sinkInputs = Graph.getInputSockets(sinkId);
    var sinkAvailable = false;
    if (sinkInputs.length > 0 && !sinkInputs[0].connected) {
        tryConnection(mergeId, 2, sinkId, 0, "MERGE(out:2) -> SINK(in:0) [MAIN PATH]");
    } else {
        console.log("SINK already connected via alternate path");
    }
    
    // Step 5: Verify complete network
    console.log("\n=== NETWORK VERIFICATION ===");
    
    var finalStats = Graph.getStats();
    console.log("Final graph statistics:", JSON.stringify(finalStats));
    
    console.log("\nConnection Summary:");
    connections.forEach(function(conn, index) {
        console.log((index + 1) + ". " + conn.description + " [" + conn.edge + "]");
    });
    
    console.log("\nNode Type Distribution:");
    Object.keys(finalStats.nodeTypes).forEach(function(type) {
        console.log("  " + type + ": " + finalStats.nodeTypes[type] + " nodes");
    });
    
    // Step 6: Comprehensive validation
    console.log("\n=== COMPREHENSIVE VALIDATION ===");
    
    var expectedNodes = 6; // 2 SOURCE, 1 TRANSFORM, 1 SPLIT, 1 MERGE, 1 SINK
    var minimumEdges = 4;  // At least 4 connections for basic flow
    var maximumEdges = 6;  // Maximum possible with our configuration
    
    var validationResults = {
        nodeCount: finalStats.nodes === expectedNodes,
        edgeCount: finalStats.edges >= minimumEdges && finalStats.edges <= maximumEdges,
        allNodeTypes: Object.keys(finalStats.nodeTypes).length === 5,
        sourceNodes: finalStats.nodeTypes.SOURCE === 2,
        processingNodes: (finalStats.nodeTypes.TRANSFORM || 0) + (finalStats.nodeTypes.SPLIT || 0) + (finalStats.nodeTypes.MERGE || 0) >= 3,
        sinkNodes: finalStats.nodeTypes.SINK === 1
    };
    
    console.log("Validation Results:");
    Object.keys(validationResults).forEach(function(test) {
        console.log("  " + test + ": " + (validationResults[test] ? "✅ PASS" : "❌ FAIL"));
    });
    
    var allTestsPassed = Object.values(validationResults).every(function(result) { return result; });
    
    if (allTestsPassed) {
        console.log("\n🎉 SUCCESS: COMPLETE NODE GRAPH CREATED!");
        console.log("✅ All 5 node types present and connected");
        console.log("✅ Complex data flow network established");
        console.log("✅ " + connectionCount + " connections successfully created");
        
        var result = {
            success: true,
            message: "Complete node graph with all types and connections",
            nodesCreated: finalStats.nodes,
            edgesCreated: finalStats.edges,
            nodeTypes: finalStats.nodeTypes,
            connections: connectionCount,
            networkComplexity: "HIGH",
            dataFlowPaths: "MULTIPLE"
        };
        result;
    } else {
        console.error("❌ VALIDATION FAILED: Graph incomplete");
        console.error("Expected: " + expectedNodes + " nodes, " + minimumEdges + "-" + maximumEdges + " edges");
        console.error("Actual: " + finalStats.nodes + " nodes, " + finalStats.edges + " edges");
        
        var errorResult = {
            success: false,
            message: "Complete node graph validation failed",
            nodesCreated: finalStats.nodes,
            edgesCreated: finalStats.edges,
            validationResults: validationResults
        };
        errorResult;
    }
    
} catch (error) {
    console.error("COMPLETE_NODE_GRAPH: Exception occurred:", error.message);
    var errorResult = {
        success: false,
        message: "Complete node graph failed with exception: " + error.message
    };
    errorResult;
}