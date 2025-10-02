// Destructor Exception Safety Test - Demonstrates crash prevention
// This script tests that XmlAutosaveObserver destructors handle exceptions gracefully

console.log("=== Destructor Exception Safety Test ===");
console.log("Testing that application doesn't crash when destructors encounter errors");

function testMassCreationDestruction() {
    console.log("\n1. Testing mass object creation and destruction:");
    
    try {
        for (let batch = 0; batch < 10; batch++) {
            console.log(`  Creating batch ${batch + 1}/10...`);
            
            // Create many nodes to trigger autosave observers
            let nodes = [];
            for (let i = 0; i < 20; i++) {
                let nodeId = Graph.createNode("SOURCE", i * 15, batch * 50);
                if (nodeId) {
                    nodes.push(nodeId);
                    // Move nodes to trigger pending changes in autosave observers
                    Graph.moveNode(nodeId, i * 15 + 5, batch * 50 + 5);
                }
            }
            
            // Force destruction of all objects and their observers
            // This will trigger XmlAutosaveObserver destructors with pending changes
            Graph.clear();
            
            console.log(`  ✅ Batch ${batch + 1} cleared successfully - no crashes`);
        }
        
        console.log("✅ Mass creation/destruction test PASSED");
        return true;
        
    } catch (error) {
        console.log("❌ Mass creation/destruction test FAILED:", error.message);
        return false;
    }
}

function testPendingChangesCleanup() {
    console.log("\n2. Testing cleanup with pending autosave changes:");
    
    try {
        // Create objects that will have pending autosave changes
        let sourceId = Graph.createNode("SOURCE", 100, 100);
        let transformId = Graph.createNode("TRANSFORM", 300, 100);
        let sinkId = Graph.createNode("SINK", 500, 100);
        
        // Create connections and modifications to ensure pending changes
        if (sourceId && transformId && sinkId) {
            Graph.connect(sourceId, 0, transformId, 0);
            Graph.connect(transformId, 0, sinkId, 0);
            
            // Move nodes to trigger autosave pending state
            Graph.moveNode(sourceId, 150, 150);
            Graph.moveNode(transformId, 350, 150);
            Graph.moveNode(sinkId, 550, 150);
            
            console.log("  Created graph with pending autosave changes");
        }
        
        // Force immediate cleanup while changes are pending
        // This tests the destructor exception handling
        Graph.clear();
        
        console.log("✅ Pending changes cleanup test PASSED - no crashes during destruction");
        return true;
        
    } catch (error) {
        console.log("❌ Pending changes cleanup test FAILED:", error.message);
        return false;
    }
}

function testErrorDuringCleanup() {
    console.log("\n3. Testing error handling during cleanup:");
    
    try {
        // Create a complex graph
        let nodes = [];
        for (let i = 0; i < 15; i++) {
            let nodeId = Graph.createNode("TRANSFORM", i * 30, 200);
            if (nodeId) {
                nodes.push(nodeId);
                
                // Create connections to make cleanup more complex
                if (i > 0) {
                    Graph.connect(nodes[i-1], 0, nodeId, 0);
                }
                
                // Multiple moves to ensure autosave observers have pending changes
                Graph.moveNode(nodeId, i * 30 + 10, 210);
                Graph.moveNode(nodeId, i * 30 + 20, 220);
            }
        }
        
        console.log("  Created complex connected graph with multiple pending changes");
        
        // Simulate potential error conditions by forcing rapid cleanup
        // The destructors should handle any exceptions gracefully
        Graph.clear();
        
        console.log("✅ Error handling during cleanup test PASSED");
        return true;
        
    } catch (error) {
        console.log("❌ Error handling during cleanup test FAILED:", error.message);
        return false;
    }
}

function testNestedExceptionScenarios() {
    console.log("\n4. Testing nested exception scenarios:");
    
    try {
        // Create scenario where multiple things might fail
        let nodeId = Graph.createNode("MERGE", 400, 300);
        
        if (nodeId) {
            // Multiple rapid changes
            for (let i = 0; i < 10; i++) {
                Graph.moveNode(nodeId, 400 + i * 5, 300 + i * 5);
            }
        }
        
        // Try to load invalid file (should fail gracefully)
        // Then clear (destructors should still work safely)
        try {
            Graph.loadXml("nonexistent_file.xml");
        } catch (loadError) {
            console.log("  Expected load error occurred:", loadError.message);
        }
        
        // Clear should work even after previous errors
        Graph.clear();
        
        console.log("✅ Nested exception scenarios test PASSED");
        return true;
        
    } catch (error) {
        console.log("❌ Nested exception scenarios test FAILED:", error.message);
        return false;
    }
}

// Run all tests
function runDestructorSafetyTests() {
    console.log("Running comprehensive destructor safety tests...\n");
    
    let tests = [
        testMassCreationDestruction,
        testPendingChangesCleanup,
        testErrorDuringCleanup,
        testNestedExceptionScenarios
    ];
    
    let passed = 0;
    let total = tests.length;
    
    for (let i = 0; i < tests.length; i++) {
        if (tests[i]()) {
            passed++;
        }
    }
    
    console.log(`\n=== Test Results ===`);
    console.log(`Passed: ${passed}/${total} tests`);
    
    if (passed === total) {
        console.log("🎉 ALL DESTRUCTOR SAFETY TESTS PASSED!");
        console.log("✅ Application handles destructor exceptions gracefully");
        console.log("✅ No crashes occur during cleanup scenarios");
        console.log("✅ Pending changes are handled safely during destruction");
    } else {
        console.log("⚠️ Some tests failed - destructor safety needs attention");
    }
    
    return passed === total;
}

// Execute the test suite
let success = runDestructorSafetyTests();

// Save final state
Graph.saveXml("destructor_safety_test_result.xml");
console.log("\nTest completed. Results saved to destructor_safety_test_result.xml");

console.log("\n=== Destructor Safety Test Complete ===");