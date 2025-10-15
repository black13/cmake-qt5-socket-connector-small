/**
 * Regression Test: Edge Deletion Socket Clearing
 *
 * Verifies Action A1: "Lock edge-delete path - ensure socket clearing"
 *
 * Tests that when an edge is deleted:
 * 1. Both endpoint sockets have their connectedEdge cleared
 * 2. Reconnection to the same sockets works correctly
 * 3. No dangling edge references remain
 */

console.log("----------------------------------------------------");
console.log("Edge Deletion Socket Clearing Test");
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

// Test 1: Create two nodes and connect them
console.log("\n[Test 1] Creating nodes and connecting sockets...");

var node1Id = Graph.createNode("SOURCE", 100, 100);
var node2Id = Graph.createNode("SINK", 300, 100);

assert(node1Id !== "", "Node 1 created");
assert(node2Id !== "", "Node 2 created");

// Get nodes and their sockets
var node1 = Graph.getNode(node1Id);
var node2 = Graph.getNode(node2Id);

assert(node1 !== null, "Node 1 retrieved");
assert(node2 !== null, "Node 2 retrieved");

// Get output socket from node1 and input socket from node2
var outputSocket = node1.getOutputSocketByIndex(0);
var inputSocket = node2.getInputSocketByIndex(0);

assert(outputSocket !== null, "Node 1 output socket exists");
assert(inputSocket !== null, "Node 2 input socket exists");

// Connect the sockets
var edgeId = Graph.connectSockets(node1Id, 0, node2Id, 0);
assert(edgeId !== "", "Edge created successfully");

// Test 2: Verify sockets are connected
console.log("\n[Test 2] Verifying socket connections after edge creation...");

// Get the edge
var edge = Graph.getEdgeById(edgeId);
assert(edge !== null, "Edge retrieved successfully");

// Verify sockets are connected to the edge
var fromSocket = edge.getFromSocket();
var toSocket = edge.getToSocket();

assert(fromSocket !== null, "Edge has from-socket");
assert(toSocket !== null, "Edge has to-socket");

// Verify socket->edge references exist
assert(outputSocket.getConnectedEdge() !== null, "Output socket has connected edge");
assert(inputSocket.getConnectedEdge() !== null, "Input socket has connected edge");

var outputEdge = outputSocket.getConnectedEdge();
var inputEdge = inputSocket.getConnectedEdge();

assert(outputEdge.getId() === edgeId, "Output socket connected to correct edge");
assert(inputEdge.getId() === edgeId, "Input socket connected to correct edge");

// Test 3: Delete edge and verify socket clearing
console.log("\n[Test 3] Deleting edge and verifying socket clearing...");

var deleteSuccess = Graph.deleteEdge(edgeId);
assert(deleteSuccess, "Edge deleted successfully");

// CRITICAL: Verify sockets are cleared
assert(outputSocket.getConnectedEdge() === null, "Output socket cleared after edge deletion");
assert(inputSocket.getConnectedEdge() === null, "Input socket cleared after edge deletion");

// Test 4: Reconnect the same sockets
console.log("\n[Test 4] Reconnecting same sockets after edge deletion...");

var newEdgeId = Graph.connectSockets(node1Id, 0, node2Id, 0);
assert(newEdgeId !== "", "Reconnection successful");
assert(newEdgeId !== edgeId, "New edge has different ID");

// Verify new connection
var newEdge = Graph.getEdgeById(newEdgeId);
assert(newEdge !== null, "New edge retrieved");

assert(outputSocket.getConnectedEdge() !== null, "Output socket connected to new edge");
assert(inputSocket.getConnectedEdge() !== null, "Input socket connected to new edge");

var reconnectedEdge1 = outputSocket.getConnectedEdge();
var reconnectedEdge2 = inputSocket.getConnectedEdge();

assert(reconnectedEdge1.getId() === newEdgeId, "Output socket references new edge");
assert(reconnectedEdge2.getId() === newEdgeId, "Input socket references new edge");

// Test 5: Multiple deletion cycles
console.log("\n[Test 5] Testing multiple delete-reconnect cycles...");

for (var i = 0; i < 3; i++) {
    // Delete
    var success = Graph.deleteEdge(newEdgeId);
    assert(success, "Cycle " + (i+1) + ": Edge deleted");
    assert(outputSocket.getConnectedEdge() === null, "Cycle " + (i+1) + ": Output socket cleared");
    assert(inputSocket.getConnectedEdge() === null, "Cycle " + (i+1) + ": Input socket cleared");

    // Reconnect
    newEdgeId = Graph.connectSockets(node1Id, 0, node2Id, 0);
    assert(newEdgeId !== "", "Cycle " + (i+1) + ": Reconnected successfully");
}

// Clean up final edge
Graph.deleteEdge(newEdgeId);

// Test 6: Node deletion clears connected edges
console.log("\n[Test 6] Testing node deletion clears connected edges...");

// Reconnect one more time
var finalEdgeId = Graph.connectSockets(node1Id, 0, node2Id, 0);
assert(finalEdgeId !== "", "Final edge created for node deletion test");

// Delete node1 (should cascade delete the edge)
var nodeDeleteSuccess = Graph.deleteNode(node1Id);
assert(nodeDeleteSuccess, "Node 1 deleted successfully");

// Verify edge was cascade-deleted
var orphanedEdge = Graph.getEdgeById(finalEdgeId);
assert(orphanedEdge === null, "Edge cascade-deleted with node");

// Verify remaining socket is cleared
assert(inputSocket.getConnectedEdge() === null, "Remaining socket cleared after node deletion");

// Clean up
Graph.deleteNode(node2Id);

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

// Exit cleanly
// Test complete;
