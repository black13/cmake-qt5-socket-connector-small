/**
 * Regression Test: Edge Deletion Socket Clearing (Simplified)
 *
 * Verifies Action A1: "Lock edge-delete path - ensure socket clearing"
 *
 * Tests that reconnection works after edge deletion, which proves sockets were cleared.
 */

console.log("----------------------------------------------------");
console.log("Edge Deletion Socket Clearing Test (Simplified)");
console.log("----------------------------------------------------");

var testsPassed = 0;
var testsFailed = 0;

function assert(condition, message) {
    if (condition) {
        console.log("[OK] " + message);
        testsPassed++;
    } else {
        console.log("[FAIL] FAIL: " + message);
        testsFailed++;
    }
}

// Test 1: Create and connect nodes
console.log("\n[Test 1] Creating and connecting nodes...");

var node1 = Graph.createNode("SOURCE", 100, 100);
var node2 = Graph.createNode("SINK", 300, 100);

assert(node1 !== "", "Node 1 created");
assert(node2 !== "", "Node 2 created");

// Connect nodes (socket 0 to socket 0)
var edge1 = Graph.connect(node1, 0, node2, 0);
assert(edge1 !== "", "Initial edge created");

// Test 2: Delete edge and verify stats
console.log("\n[Test 2] Deleting edge...");

var deletedOk = Graph.deleteEdge(edge1);
assert(deletedOk, "Edge deleted successfully");

var stats = Graph.getStats();
assert(stats.edges === 0, "Edge count is 0 after deletion");

// Test 3: Reconnect same sockets (this fails if sockets weren't cleared!)
console.log("\n[Test 3] Reconnecting same sockets (critical test)...");

var edge2 = Graph.connect(node1, 0, node2, 0);
assert(edge2 !== "", "Reconnection successful - sockets were cleared!");
assert(edge2 !== edge1, "New edge has different ID");

stats = Graph.getStats();
assert(stats.edges === 1, "Edge count is 1 after reconnection");

// Test 4: Multiple delete/reconnect cycles
console.log("\n[Test 4] Testing 5 delete-reconnect cycles...");

for (var i = 0; i < 5; i++) {
    var deleted = Graph.deleteEdge(edge2);
    assert(deleted, "Cycle " + (i+1) + ": Edge deleted");

    edge2 = Graph.connect(node1, 0, node2, 0);
    assert(edge2 !== "", "Cycle " + (i+1) + ": Reconnected successfully");
}

// Test 5: Node deletion cascade-deletes edges
console.log("\n[Test 5] Testing node deletion cascade...");

// Ensure we have an edge
if (edge2 === "") {
    edge2 = Graph.connect(node1, 0, node2, 0);
}

var nodeDeletedOk = Graph.deleteNode(node1);
assert(nodeDeletedOk, "Node 1 deleted successfully");

stats = Graph.getStats();
assert(stats.nodes === 1, "Node count is 1 after deletion");
assert(stats.edges === 0, "Edge cascade-deleted with node");

// Clean up
Graph.deleteNode(node2);

// Summary
console.log("\n----------------------------------------------------");
console.log("Test Summary");
console.log("----------------------------------------------------");
console.log("Tests Passed: " + testsPassed);
console.log("Tests Failed: " + testsFailed);

if (testsFailed === 0) {
    console.log("[OK] ALL TESTS PASSED - Socket clearing works correctly!");
    console.log("Action A1 verified: Edge deletion properly clears endpoint sockets");
} else {
    console.log("[FAIL] SOME TESTS FAILED - Review edge deletion implementation");
}

console.log("----------------------------------------------------");
