/**
 * Edge Case Testing - Things an inexperienced user might try
 *
 * Tests defensive programming - system should handle gracefully:
 * - Loading empty files
 * - Deleting non-existent items
 * - Operating on empty graph
 * - Invalid IDs and indices
 * - Nonsensical operations
 */

console.log("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
console.log("Edge Case Testing - Inexperienced User Scenarios");
console.log("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");

var testsPassed = 0;
var testsFailed = 0;

function testCase(description, fn) {
    try {
        console.log("\n[Test] " + description);
        var result = fn();
        if (result !== false) {
            console.log("  ✅ Handled gracefully");
            testsPassed++;
        } else {
            console.log("  ❌ Test failed");
            testsFailed++;
        }
    } catch (e) {
        console.log("  ❌ CRASH: " + e);
        testsFailed++;
    }
}

// Test 1: Load empty XML file
testCase("Load empty XML file", function() {
    Graph.loadXml("tests/empty.xml");
    var stats = Graph.getStats();
    console.log("  Stats after loading empty file: nodes=" + stats.nodes + ", edges=" + stats.edges);
    return stats.nodes === 0 && stats.edges === 0;
});

// Test 2: Delete all edges when graph is empty
testCase("Delete all edges from empty graph", function() {
    var edges = Graph.getEdges();
    console.log("  Edge count: " + edges.length);
    for (var i = 0; i < edges.length; i++) {
        Graph.deleteEdge(edges[i].id);
    }
    console.log("  Attempted to delete " + edges.length + " edges (should be 0)");
    return true;
});

// Test 3: Delete all nodes when graph is empty
testCase("Delete all nodes from empty graph", function() {
    var nodes = Graph.getNodes();
    console.log("  Node count: " + nodes.length);
    for (var i = 0; i < nodes.length; i++) {
        Graph.deleteNode(nodes[i].id);
    }
    console.log("  Attempted to delete " + nodes.length + " nodes (should be 0)");
    return true;
});

// Test 4: Clear already empty graph
testCase("Clear an already empty graph", function() {
    Graph.clear();
    var stats = Graph.getStats();
    console.log("  After clear: nodes=" + stats.nodes + ", edges=" + stats.edges);
    return stats.nodes === 0 && stats.edges === 0;
});

// Test 5: Delete non-existent node
testCase("Delete node with fake UUID", function() {
    var result = Graph.deleteNode("{00000000-0000-0000-0000-000000000000}");
    console.log("  Delete returned: " + result);
    // Should return false but not crash
    return true;
});

// Test 6: Delete non-existent edge
testCase("Delete edge with fake UUID", function() {
    var result = Graph.deleteEdge("{fake-edge-id-12345}");
    console.log("  Delete returned: " + result);
    // Should return false but not crash
    return true;
});

// Test 7: Try to connect with invalid node IDs
testCase("Connect non-existent nodes", function() {
    var result = Graph.connect("{fake-node-1}", 0, "{fake-node-2}", 0);
    console.log("  Connect returned: " + result);
    // Should return empty string or handle gracefully
    return true;
});

// Test 8: Create nodes then try invalid operations
testCase("Create nodes, then try invalid socket indices", function() {
    var node1 = Graph.createNode("SOURCE", 100, 100);
    var node2 = Graph.createNode("SINK", 300, 100);

    console.log("  Created 2 nodes");

    // SOURCE has 0 inputs, 1 output (index 0)
    // SINK has 1 input (index 0), 0 outputs
    // Try to connect output 5 to input 10 (invalid indices)
    var badEdge = Graph.connect(node1, 5, node2, 10);
    console.log("  Attempted invalid connection: " + badEdge);

    // Clean up
    Graph.deleteNode(node1);
    Graph.deleteNode(node2);
    return true;
});

// Test 9: Delete node twice
testCase("Delete same node twice", function() {
    var node = Graph.createNode("SOURCE", 200, 200);
    console.log("  Created node: " + node);

    var result1 = Graph.deleteNode(node);
    console.log("  First delete: " + result1);

    var result2 = Graph.deleteNode(node);
    console.log("  Second delete (already gone): " + result2);

    return true;
});

// Test 10: Delete edge twice
testCase("Delete same edge twice", function() {
    var node1 = Graph.createNode("SOURCE", 100, 300);
    var node2 = Graph.createNode("SINK", 300, 300);
    var edge = Graph.connect(node1, 0, node2, 0);

    console.log("  Created edge: " + edge);

    var result1 = Graph.deleteEdge(edge);
    console.log("  First delete: " + result1);

    var result2 = Graph.deleteEdge(edge);
    console.log("  Second delete (already gone): " + result2);

    // Clean up
    Graph.deleteNode(node1);
    Graph.deleteNode(node2);
    return true;
});

// Test 11: Create node with invalid type
testCase("Create node with invalid/unknown type", function() {
    var result = Graph.createNode("SUPER_MEGA_NODE_9000", 400, 400);
    console.log("  Create unknown type returned: " + result);
    // Should return empty string
    return result === "";
});

// Test 12: Rapid create/delete cycles (stress test)
testCase("Rapid create/delete cycles (10 iterations)", function() {
    for (var i = 0; i < 10; i++) {
        var n1 = Graph.createNode("SOURCE", 50, 50);
        var n2 = Graph.createNode("SINK", 100, 50);
        var e = Graph.connect(n1, 0, n2, 0);

        Graph.deleteEdge(e);
        Graph.deleteNode(n1);
        Graph.deleteNode(n2);
    }

    var stats = Graph.getStats();
    console.log("  After 10 cycles: nodes=" + stats.nodes + ", edges=" + stats.edges);
    return stats.nodes === 0 && stats.edges === 0;
});

// Test 13: Load empty file multiple times
testCase("Load empty XML file 5 times in a row", function() {
    for (var i = 0; i < 5; i++) {
        Graph.loadXml("tests/empty.xml");
    }
    var stats = Graph.getStats();
    console.log("  After 5 loads: nodes=" + stats.nodes + ", edges=" + stats.edges);
    return stats.nodes === 0 && stats.edges === 0;
});

// Test 14: Delete nodes while they have edges
testCase("Delete node that has connected edges", function() {
    var node1 = Graph.createNode("SOURCE", 500, 100);
    var node2 = Graph.createNode("SINK", 600, 100);
    var node3 = Graph.createNode("SINK", 700, 100);

    var edge1 = Graph.connect(node1, 0, node2, 0);
    var edge2 = Graph.connect(node1, 0, node3, 0);

    console.log("  Created 1 source with 2 outgoing edges");

    // Delete the source node - edges should cascade delete
    Graph.deleteNode(node1);

    var stats = Graph.getStats();
    console.log("  After deleting source: nodes=" + stats.nodes + ", edges=" + stats.edges);

    // Should have 2 nodes left (the sinks), 0 edges (cascade deleted)
    var expected = (stats.nodes === 2 && stats.edges === 0);

    // Clean up
    Graph.clear();
    return expected;
});

// Test 15: Save empty graph to XML
testCase("Save empty graph to XML", function() {
    Graph.clear();
    Graph.saveXml("tests/output_empty.xml");
    console.log("  Saved empty graph to tests/output_empty.xml");
    return true;
});

// Test 16: Get XML string of empty graph
testCase("Get XML string of empty graph", function() {
    Graph.clear();
    var xml = Graph.getXmlString();
    console.log("  XML length: " + xml.length + " characters");
    console.log("  XML preview: " + xml.substring(0, 100));
    return xml.length > 0; // Should have at least the root element
});

// Summary
console.log("\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
console.log("Edge Case Test Summary");
console.log("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
console.log("Tests Passed: " + testsPassed);
console.log("Tests Failed: " + testsFailed);

if (testsFailed === 0) {
    console.log("✅ ALL EDGE CASES HANDLED GRACEFULLY!");
    console.log("System is robust against inexperienced user mistakes");
} else {
    console.log("❌ SOME EDGE CASES FAILED");
    console.log("Review error handling for defensive programming");
}

console.log("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
