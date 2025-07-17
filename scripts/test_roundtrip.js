// Test Script: XML Roundtrip Validation
// This script tests the integrity of XML save/load operations

function test_roundtrip_basic() {
    console.log("=== Basic Roundtrip Test ===");
    
    // Clear and create a test graph
    Graph.clear();
    
    let source = Graph.createNode("Source", 100, 100);
    let processor = Graph.createNode("1-to-2", 250, 100);
    let sink1 = Graph.createNode("Sink", 400, 50);
    let sink2 = Graph.createNode("Sink", 400, 150);
    
    Graph.connect(source, 0, processor, 0);
    Graph.connect(processor, 0, sink1, 0);
    Graph.connect(processor, 1, sink2, 0);
    
    // Get initial state
    let initialStats = Graph.getStats();
    let initialXml = Graph.getXmlString();
    
    console.log("Initial graph stats:", JSON.stringify(initialStats));
    console.log("Initial XML length:", initialXml.length);
    
    // Save to file
    Graph.saveXml("roundtrip_test_1.xml");
    
    // Clear and reload
    Graph.clear();
    let emptyStats = Graph.getStats();
    console.log("After clear:", JSON.stringify(emptyStats));
    
    Graph.loadXml("roundtrip_test_1.xml");
    
    // Get final state
    let finalStats = Graph.getStats();
    let finalXml = Graph.getXmlString();
    
    console.log("Final graph stats:", JSON.stringify(finalStats));
    console.log("Final XML length:", finalXml.length);
    
    // Compare results
    let statsMatch = (initialStats.nodes === finalStats.nodes && 
                     initialStats.edges === finalStats.edges);
    let xmlMatch = (initialXml.length === finalXml.length);
    
    console.log("Stats match:", statsMatch);
    console.log("XML length match:", xmlMatch);
    
    console.log("=== Basic Roundtrip Test Complete ===");
    
    return {
        success: statsMatch && xmlMatch,
        message: "Basic roundtrip test " + (statsMatch && xmlMatch ? "passed" : "failed"),
        initialStats: initialStats,
        finalStats: finalStats
    };
}

function test_roundtrip_with_modifications() {
    console.log("=== Roundtrip Test with Modifications ===");
    
    // Create initial graph
    Graph.clear();
    
    let node1 = Graph.createNode("Source", 50, 50);
    let node2 = Graph.createNode("Sink", 200, 50);
    Graph.connect(node1, 0, node2, 0);
    
    // Save initial state
    Graph.saveXml("roundtrip_before_mod.xml");
    let beforeStats = Graph.getStats();
    
    // Make modifications
    let node3 = Graph.createNode("1-to-2", 125, 100);
    Graph.deleteNode(node2);
    
    let node4 = Graph.createNode("Sink", 200, 75);
    let node5 = Graph.createNode("Sink", 200, 125);
    
    Graph.connect(node1, 0, node3, 0);
    Graph.connect(node3, 0, node4, 0);
    Graph.connect(node3, 1, node5, 0);
    
    // Save modified state
    Graph.saveXml("roundtrip_after_mod.xml");
    let afterStats = Graph.getStats();
    
    console.log("Before modifications:", JSON.stringify(beforeStats));
    console.log("After modifications:", JSON.stringify(afterStats));
    
    // Test roundtrip of modified graph
    Graph.clear();
    Graph.loadXml("roundtrip_after_mod.xml");
    Graph.saveXml("roundtrip_final.xml");
    
    let finalStats = Graph.getStats();
    let finalXml = Graph.getXmlString();
    
    console.log("Final state:", JSON.stringify(finalStats));
    
    // Verify the modifications persisted correctly
    let success = (finalStats.nodes === 4 && finalStats.edges === 3);
    
    console.log("=== Roundtrip Test with Modifications Complete ===");
    
    return {
        success: success,
        message: "Modification roundtrip test " + (success ? "passed" : "failed"),
        beforeStats: beforeStats,
        afterStats: afterStats,
        finalStats: finalStats
    };
}

function test_roundtrip_stress() {
    console.log("=== Stress Roundtrip Test ===");
    
    Graph.clear();
    
    let nodeIds = [];
    let edgeIds = [];
    
    // Create many nodes
    console.log("Creating multiple nodes...");
    for (let i = 0; i < 10; i++) {
        let x = 100 + (i % 5) * 150;
        let y = 100 + Math.floor(i / 5) * 100;
        let type = ["Source", "Sink", "1-to-2", "2-to-1"][i % 4];
        
        let nodeId = Graph.createNode(type, x, y);
        nodeIds.push(nodeId);
        console.log(`Created node ${i}: ${nodeId} (${type})`);
    }
    
    // Create connections where possible
    console.log("Creating connections...");
    for (let i = 0; i < nodeIds.length - 1; i++) {
        try {
            let edgeId = Graph.connect(nodeIds[i], 0, nodeIds[i + 1], 0);
            if (edgeId) {
                edgeIds.push(edgeId);
                console.log(`Connected ${i} -> ${i + 1}: ${edgeId}`);
            }
        } catch (error) {
            console.log(`Failed to connect ${i} -> ${i + 1}: ${error.toString()}`);
        }
    }
    
    let beforeStats = Graph.getStats();
    console.log("Before roundtrip:", JSON.stringify(beforeStats));
    
    // Save and reload multiple times
    for (let round = 1; round <= 3; round++) {
        console.log(`Roundtrip round ${round}`);
        
        Graph.saveXml(`stress_test_round_${round}.xml`);
        Graph.clear();
        Graph.loadXml(`stress_test_round_${round}.xml`);
        
        let roundStats = Graph.getStats();
        console.log(`Round ${round} stats:`, JSON.stringify(roundStats));
    }
    
    let finalStats = Graph.getStats();
    let success = (finalStats.nodes === beforeStats.nodes && 
                  finalStats.edges === beforeStats.edges);
    
    console.log("=== Stress Roundtrip Test Complete ===");
    
    return {
        success: success,
        message: "Stress roundtrip test " + (success ? "passed" : "failed"),
        beforeStats: beforeStats,
        finalStats: finalStats,
        rounds: 3
    };
}

function test_xml_integrity() {
    console.log("=== XML Integrity Test ===");
    
    Graph.clear();
    
    // Create a known graph structure
    let source = Graph.createNode("Source", 100, 100);
    let proc1 = Graph.createNode("1-to-2", 250, 100);
    let proc2 = Graph.createNode("2-to-1", 400, 100);
    let sink = Graph.createNode("Sink", 550, 100);
    
    Graph.connect(source, 0, proc1, 0);
    Graph.connect(proc1, 0, proc2, 0);
    Graph.connect(proc1, 1, proc2, 1);
    Graph.connect(proc2, 0, sink, 0);
    
    // Get XML and verify it contains expected elements
    let xmlString = Graph.getXmlString();
    
    console.log("XML string length:", xmlString.length);
    
    // Check for expected XML elements
    let hasGraphRoot = xmlString.includes('<graph');
    let hasNodes = xmlString.includes('<node');
    let hasEdges = xmlString.includes('<edge');
    let hasValidEncoding = xmlString.includes('UTF-8');
    
    console.log("Has graph root:", hasGraphRoot);
    console.log("Has nodes:", hasNodes);
    console.log("Has edges:", hasEdges);
    console.log("Has valid encoding:", hasValidEncoding);
    
    // Count occurrences
    let nodeCount = (xmlString.match(/<node/g) || []).length;
    let edgeCount = (xmlString.match(/<edge/g) || []).length;
    
    console.log("XML node count:", nodeCount);
    console.log("XML edge count:", edgeCount);
    
    let stats = Graph.getStats();
    let xmlCountsMatch = (nodeCount === stats.nodes && edgeCount === stats.edges);
    
    console.log("XML counts match stats:", xmlCountsMatch);
    
    let success = hasGraphRoot && hasNodes && hasEdges && hasValidEncoding && xmlCountsMatch;
    
    console.log("=== XML Integrity Test Complete ===");
    
    return {
        success: success,
        message: "XML integrity test " + (success ? "passed" : "failed"),
        xmlLength: xmlString.length,
        nodeCount: nodeCount,
        edgeCount: edgeCount,
        statsMatch: xmlCountsMatch
    };
}

function run_all_roundtrip_tests() {
    console.log("=== Running All Roundtrip Tests ===");
    
    let results = [];
    
    try {
        results.push(test_roundtrip_basic());
        results.push(test_roundtrip_with_modifications());
        results.push(test_roundtrip_stress());
        results.push(test_xml_integrity());
    } catch (error) {
        console.log("Error during roundtrip testing:", error.toString());
        results.push({
            success: false,
            message: "Roundtrip test suite failed: " + error.toString()
        });
    }
    
    console.log("=== Roundtrip Test Results Summary ===");
    let passedCount = 0;
    let totalCount = results.length;
    
    for (let i = 0; i < results.length; i++) {
        let result = results[i];
        let status = result.success ? "✅ PASS" : "❌ FAIL";
        console.log(`Roundtrip Test ${i + 1}: ${status} - ${result.message}`);
        
        if (result.success) {
            passedCount++;
        }
    }
    
    console.log(`=== Final Roundtrip Result: ${passedCount}/${totalCount} tests passed ===`);
    
    return {
        passed: passedCount,
        total: totalCount,
        success: passedCount === totalCount
    };
}

// Auto-run all roundtrip tests
run_all_roundtrip_tests();