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
    return typeof Graph !== 'undefined' && Graph !== null;
});

// Test 3: Node API availability  
test("Node API Available", () => {
    return typeof Node !== 'undefined';
});

// Test 4: Graph API methods
test("Graph API Methods", () => {
    return typeof Graph.createNode === 'function' && 
           typeof Graph.clear === 'function' &&
           typeof Graph.getStats === 'function';
});

console.log("\n=== Test Results ===");
console.log("Total tests:", testResults.total);
console.log("Passed:", testResults.passed);
console.log("Failed:", testResults.failed);

// Return results for C++ integration
testResults;