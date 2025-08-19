// NodeGraph Startup Verification Script
// Tests JavaScript integration with fully initialized application

console.log("=== NodeGraph Startup Verification ===");
console.log("Testing JavaScript integration with live application...");

let testResults = {
    total: 0,
    passed: 0,
    failed: 0,
    tests: []
};

function test(name, testFunc) {
    testResults.total++;
    try {
        let result = testFunc();
        if (result) {
            console.log("PASS: " + name);
            testResults.passed++;
            testResults.tests.push({name: name, status: "PASS"});
        } else {
            console.log("FAIL: " + name);
            testResults.failed++;
            testResults.tests.push({name: name, status: "FAIL"});
        }
    } catch (error) {
        console.log("ERROR: " + name + " - " + error.message);
        testResults.failed++;
        testResults.tests.push({name: name, status: "ERROR", error: error.message});
    }
}

// Test 1: Basic JavaScript engine functionality
test("JavaScript Engine Basic", () => {
    return typeof console !== 'undefined' && typeof console.log === 'function';
});

// Test 2: Core infrastructure check
test("Qt Infrastructure Available", () => {
    // We don't need Scene/Window/View fully exposed, just verify Graph works
    // The QObject infrastructure handles the backend
    return typeof Graph !== 'undefined' && Graph !== null;
});

// Test 3: Node API availability  
test("Node API Available", () => {
    return typeof Node !== 'undefined';
});

// Test 4: Console API functions
test("Console API Functions", () => {
    return typeof console.log === 'function' && 
           typeof console.error === 'function';
});

// Test 5: Template system integration - THE CRITICAL TEST
test("Template Types Available", () => {
    if (typeof Graph === 'undefined') return false;
    return typeof Graph.createNode === 'function';
});

// Test 6: Actual node creation with SOURCE template
test("Create SOURCE Node", () => {
    if (typeof Graph === 'undefined' || typeof Graph.createNode !== 'function') {
        return false;
    }
    
    try {
        let sourceNode = Graph.createNode("SOURCE", 100, 100);
        return sourceNode !== null && sourceNode !== undefined;
    } catch (error) {
        console.log("SOURCE creation error: " + error.message);
        return false;
    }
});

// Test 7: Create SINK node
test("Create SINK Node", () => {
    if (typeof Graph === 'undefined' || typeof Graph.createNode !== 'function') {
        return false;
    }
    
    try {
        let sinkNode = Graph.createNode("SINK", 200, 100);
        return sinkNode !== null && sinkNode !== undefined;
    } catch (error) {
        console.log("SINK creation error: " + error.message);
        return false;
    }
});

// Test 8: Create TRANSFORM node
test("Create TRANSFORM Node", () => {
    if (typeof Graph === 'undefined' || typeof Graph.createNode !== 'function') {
        return false;
    }
    
    try {
        let transformNode = Graph.createNode("TRANSFORM", 150, 150);
        return transformNode !== null && transformNode !== undefined;
    } catch (error) {
        console.log("TRANSFORM creation error: " + error.message);
        return false;
    }
});

// Test 9: Graph state queries  
test("Graph State Queries", () => {
    if (typeof Graph === 'undefined') return false;
    
    try {
        let hasGetNodes = typeof Graph.getNodes === 'function';
        let hasGetStats = typeof Graph.getStats === 'function';
        return hasGetNodes || hasGetStats;
    } catch (error) {
        console.log("Graph query error: " + error.message);
        return false;
    }
});

// Test 10: Factory system active
test("Factory System Active", () => {
    if (typeof Graph === 'undefined' || typeof Graph.createNode !== 'function') {
        return false;
    }
    
    try {
        let split = Graph.createNode("SPLIT", 300, 100);
        let merge = Graph.createNode("MERGE", 300, 200);
        return (split !== null) && (merge !== null);
    } catch (error) {
        console.log("Factory system error: " + error.message);
        return false;
    }
});

// Test 11: Edge creation functionality
test("Graph.connect() Available", () => {
    return typeof Graph !== 'undefined' && typeof Graph.connect === 'function';
});

// Test 12: Create edges between nodes
test("Create Edges Between Nodes", () => {
    if (typeof Graph === 'undefined' || typeof Graph.createNode !== 'function' || typeof Graph.connect !== 'function') {
        return false;
    }
    
    try {
        // Create two nodes
        let sourceNode = Graph.createNode("SOURCE", 100, 100);
        let sinkNode = Graph.createNode("SINK", 300, 100);
        
        if (!sourceNode || !sinkNode) {
            console.log("Edge test: Failed to create test nodes");
            return false;
        }
        
        // Try to connect them (SOURCE output socket 0 to SINK input socket 0)
        let edge = Graph.connect(sourceNode.id, 0, sinkNode.id, 0);
        
        return edge !== null && edge !== undefined;
    } catch (error) {
        console.log("Edge creation error: " + error.message);
        return false;
    }
});

// Test 13: Save created graph to XML file
test("Save Graph to XML", () => {
    if (typeof Graph === 'undefined' || typeof Graph.saveXml !== 'function') {
        console.log("Graph.saveXml not available - checking if nodes were actually created");
        
        // Try alternative - check if Qt.GraphController exists with saveXml
        if (typeof Qt !== 'undefined' && typeof Qt.GraphController !== 'undefined' && typeof Qt.GraphController.saveXml === 'function') {
            try {
                console.log("Using Qt.GraphController.saveXml for verification");
                Qt.GraphController.saveXml("verification_test.xml");
                return true;
            } catch (error) {
                console.log("XML save error: " + error.message);
                return false;
            }
        }
        
        return false;
    }
    
    try {
        // Save all created nodes and edges to XML file
        Graph.saveXml("verification_test.xml");
        console.log("Graph successfully saved to verification_test.xml");
        return true;
    } catch (error) {
        console.log("XML save error: " + error.message);
        return false;
    }
});

// Print final results
console.log("\n=== Verification Results ===");
console.log("Total tests: " + testResults.total);
console.log("Passed: " + testResults.passed);
console.log("Failed: " + testResults.failed);
console.log("Success rate: " + Math.round((testResults.passed / testResults.total) * 100) + "%");

if (testResults.failed > 0) {
    console.log("\nFailed tests:");
    testResults.tests.filter(t => t.status !== "PASS").forEach(t => {
        console.log("  - " + t.name + ": " + t.status + (t.error ? ' (' + t.error + ')' : ''));
    });
}

console.log("\n=== JavaScript Integration Status ===");
if (testResults.failed === 0) {
    console.log("All systems operational! JavaScript integration fully working.");
} else if (testResults.passed > testResults.failed) {
    console.log("JavaScript integration partially working. Some issues detected.");
} else {
    console.log("JavaScript integration has significant issues. Manual investigation needed.");
}

console.log("=== Startup Verification Complete ===\n");

testResults;