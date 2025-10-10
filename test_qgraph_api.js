/**
 * QGraph API Test Script
 *
 * Comprehensive JavaScript test for QGraph facade API
 * Target: Improve qgraph.cpp coverage from 22% to 80%+
 *
 * Run: NodeGraph --test-script=test_qgraph_api.js
 */

console.log("========================================");
console.log("QGraph API Test Suite");
console.log("========================================\n");

var testsPassed = 0;
var testsFailed = 0;

function assertTrue(testName, condition, message) {
    if (condition) {
        console.log("✓ PASS: " + testName);
        testsPassed++;
    } else {
        console.log("✗ FAIL: " + testName);
        if (message) {
            console.log("  Reason: " + message);
        }
        testsFailed++;
    }
}

function assertFalse(testName, condition, message) {
    assertTrue(testName, !condition, message);
}

function assertEquals(testName, actual, expected, message) {
    if (actual === expected) {
        console.log("✓ PASS: " + testName);
        testsPassed++;
    } else {
        console.log("✗ FAIL: " + testName);
        console.log("  Expected: " + expected + ", Got: " + actual);
        if (message) {
            console.log("  " + message);
        }
        testsFailed++;
    }
}

// ========================================
// Phase 1: Node Type Validation
// ========================================

console.log("\n--- Testing Node Type Validation ---");

assertTrue("isValidNodeType SOURCE", Graph.isValidNodeType("SOURCE"));
assertTrue("isValidNodeType SINK", Graph.isValidNodeType("SINK"));
assertTrue("isValidNodeType TRANSFORM", Graph.isValidNodeType("TRANSFORM"));
assertTrue("isValidNodeType MERGE", Graph.isValidNodeType("MERGE"));
assertTrue("isValidNodeType SPLIT", Graph.isValidNodeType("SPLIT"));

assertFalse("isValidNodeType invalid", Graph.isValidNodeType("INVALID_TYPE"));
assertFalse("isValidNodeType empty", Graph.isValidNodeType(""));
assertFalse("isValidNodeType lowercase", Graph.isValidNodeType("source"));

var validTypes = Graph.getValidNodeTypes();
assertTrue("getValidNodeTypes not empty", validTypes.length > 0);
assertTrue("getValidNodeTypes has SOURCE", validTypes.indexOf("SOURCE") >= 0);
assertTrue("getValidNodeTypes has SINK", validTypes.indexOf("SINK") >= 0);

// ========================================
// Phase 2: Node Creation
// ========================================

console.log("\n--- Testing Node Creation ---");

var sourceId = Graph.createNode("SOURCE", 100, 100);
assertTrue("createNode SOURCE returns ID", sourceId !== "" && sourceId !== undefined);

var sinkId = Graph.createNode("SINK", 200, 200);
assertTrue("createNode SINK returns ID", sinkId !== "" && sinkId !== undefined);

var transformId = Graph.createNode("TRANSFORM", 300, 300);
assertTrue("createNode TRANSFORM returns ID", transformId !== "" && transformId !== undefined);

var mergeId = Graph.createNode("MERGE", 400, 400);
assertTrue("createNode MERGE returns ID", mergeId !== "" && mergeId !== undefined);

var splitId = Graph.createNode("SPLIT", 500, 500);
assertTrue("createNode SPLIT returns ID", splitId !== "" && splitId !== undefined);

// Test invalid node type
var invalidId = Graph.createNode("INVALID_TYPE", 0, 0);
assertTrue("createNode invalid type returns empty", invalidId === "" || invalidId === undefined);

// ========================================
// Phase 3: Node Query Operations
// ========================================

console.log("\n--- Testing Node Query Operations ---");

var nodeData = Graph.getNode(sourceId);
assertTrue("getNode returns data", nodeData !== undefined && nodeData !== null);
assertTrue("getNode has id", nodeData.id !== undefined);
assertTrue("getNode has type", nodeData.type !== undefined);
assertEquals("getNode correct type", nodeData.type, "SOURCE");

var invalidNodeData = Graph.getNode("invalid-uuid-12345");
assertTrue("getNode invalid ID returns empty", invalidNodeData === undefined || Object.keys(invalidNodeData).length === 0);

var allNodes = Graph.getNodes();
assertEquals("getNodes count", allNodes.length, 5, "Expected 5 nodes created");

var stats = Graph.getStats();
assertTrue("getStats has nodes", stats.nodes !== undefined);
assertTrue("getStats has edges", stats.edges !== undefined);
assertEquals("getStats node count", stats.nodes, 5);
assertEquals("getStats edge count", stats.edges, 0);

// ========================================
// Phase 4: Node Movement
// ========================================

console.log("\n--- Testing Node Movement ---");

var moved = Graph.moveNode(sourceId, 50, 75);
assertTrue("moveNode valid ID", moved === true);

var movedNodeData = Graph.getNode(sourceId);
// Note: moveNode adds delta, so new position is 100+50=150, 100+75=175

var invalidMove = Graph.moveNode("invalid-uuid", 10, 10);
assertFalse("moveNode invalid ID", invalidMove === true);

// ========================================
// Phase 5: Edge Creation
// ========================================

console.log("\n--- Testing Edge Creation ---");

var edge1 = Graph.connect(sourceId, 0, transformId, 0);
assertTrue("connect SOURCE->TRANSFORM", edge1 !== "" && edge1 !== undefined);

var edge2 = Graph.connect(transformId, 0, sinkId, 0);
assertTrue("connect TRANSFORM->SINK", edge2 !== "" && edge2 !== undefined);

// Test merge node with 2 inputs
var edge3 = Graph.connect(sourceId, 0, mergeId, 0);
assertTrue("connect to MERGE input 0", edge3 !== "" && edge3 !== undefined);

var source2Id = Graph.createNode("SOURCE", 50, 400);
var edge4 = Graph.connect(source2Id, 0, mergeId, 1);
assertTrue("connect to MERGE input 1", edge4 !== "" && edge4 !== undefined);

// Test split node with 2 outputs
var sink2Id = Graph.createNode("SINK", 700, 500);
var sink3Id = Graph.createNode("SINK", 700, 600);
var edge5 = Graph.connect(splitId, 0, sink2Id, 0);
assertTrue("connect SPLIT output 0", edge5 !== "" && edge5 !== undefined);
var edge6 = Graph.connect(splitId, 1, sink3Id, 0);
assertTrue("connect SPLIT output 1", edge6 !== "" && edge6 !== undefined);

// Test invalid connections
var invalidEdge1 = Graph.connect("invalid-from", 0, sinkId, 0);
assertTrue("connect invalid from returns empty", invalidEdge1 === "" || invalidEdge1 === undefined);

var invalidEdge2 = Graph.connect(sourceId, 0, "invalid-to", 0);
assertTrue("connect invalid to returns empty", invalidEdge2 === "" || invalidEdge2 === undefined);

var invalidEdge3 = Graph.connect(sourceId, 99, sinkId, 0);
assertTrue("connect invalid socket returns empty", invalidEdge3 === "" || invalidEdge3 === undefined);

// Test duplicate edge (same connection twice)
var duplicateEdge = Graph.connect(sourceId, 0, transformId, 0);
assertTrue("connect duplicate returns empty", duplicateEdge === "" || duplicateEdge === undefined);

// ========================================
// Phase 6: Edge Query Operations
// ========================================

console.log("\n--- Testing Edge Query Operations ---");

var allEdges = Graph.getEdges();
assertTrue("getEdges returns array", allEdges !== undefined && allEdges.length > 0);
assertEquals("getEdges count", allEdges.length, 6, "Expected 6 edges created");

var updatedStats = Graph.getStats();
assertEquals("getStats edge count after creation", updatedStats.edges, 6);

// ========================================
// Phase 7: Edge Deletion
// ========================================

console.log("\n--- Testing Edge Deletion ---");

var deleteEdgeSuccess = Graph.deleteEdge(edge1);
assertTrue("deleteEdge valid ID", deleteEdgeSuccess === true);

var edgesAfterDelete = Graph.getEdges();
assertEquals("getEdges after delete", edgesAfterDelete.length, 5);

var deleteEdgeAgain = Graph.deleteEdge(edge1);
assertFalse("deleteEdge already deleted", deleteEdgeAgain === true);

var deleteInvalidEdge = Graph.deleteEdge("invalid-uuid");
assertFalse("deleteEdge invalid ID", deleteInvalidEdge === true);

// ========================================
// Phase 8: Node Deletion
// ========================================

console.log("\n--- Testing Node Deletion ---");

// Delete a node that has edges connected to it
var deleteNodeSuccess = Graph.deleteNode(transformId);
assertTrue("deleteNode with edges", deleteNodeSuccess === true);

var nodeAfterDelete = Graph.getNode(transformId);
assertTrue("getNode after delete returns empty", nodeAfterDelete === undefined || Object.keys(nodeAfterDelete).length === 0);

// Edges connected to deleted node should be removed
var edgesAfterNodeDelete = Graph.getEdges();
assertTrue("edges removed when node deleted", edgesAfterNodeDelete.length < 5);

var deleteNodeAgain = Graph.deleteNode(transformId);
assertFalse("deleteNode already deleted", deleteNodeAgain === true);

var deleteInvalidNode = Graph.deleteNode("invalid-uuid");
assertFalse("deleteNode invalid ID", deleteInvalidNode === true);

// ========================================
// Phase 9: XML Save/Load Operations
// ========================================

console.log("\n--- Testing XML Save/Load ---");

// Save current graph
Graph.saveXml("test_output.xml");
console.log("  Saved graph to test_output.xml");

var xmlString = Graph.getXmlString();
assertTrue("getXmlString not empty", xmlString !== "" && xmlString.length > 0);
assertTrue("getXmlString has XML header", xmlString.indexOf("<?xml") >= 0);
assertTrue("getXmlString has graph tag", xmlString.indexOf("<graph") >= 0);

// Clear and reload
var statsBeforeClear = Graph.getStats();
console.log("  Stats before clear: " + statsBeforeClear.nodes + " nodes, " + statsBeforeClear.edges + " edges");

Graph.clear();

var statsAfterClear = Graph.getStats();
assertEquals("clear removes nodes", statsAfterClear.nodes, 0);
assertEquals("clear removes edges", statsAfterClear.edges, 0);

var nodesAfterClear = Graph.getNodes();
assertEquals("getNodes after clear", nodesAfterClear.length, 0);

// Load from file
var loaded = Graph.loadXml("test_output.xml");
assertTrue("loadXml success", loaded === true);

var statsAfterLoad = Graph.getStats();
assertTrue("loadXml restores nodes", statsAfterLoad.nodes > 0);
console.log("  Stats after load: " + statsAfterLoad.nodes + " nodes, " + statsAfterLoad.edges + " edges");

// Test loading nonexistent file
var loadInvalid = Graph.loadXml("nonexistent_file.xml");
assertFalse("loadXml nonexistent file", loadInvalid === true);

// ========================================
// Phase 10: Load State Tracking
// ========================================

console.log("\n--- Testing Load State Tracking ---");

var isLoading = Graph.isLoadingXml();
assertFalse("isLoadingXml after load complete", isLoading === true);

var isStable = Graph.isStable();
assertTrue("isStable after load complete", isStable === true);

var unresolvedCount = Graph.getUnresolvedEdgeCount();
assertEquals("getUnresolvedEdgeCount", unresolvedCount, 0);

// ========================================
// Phase 11: Complex Graph Scenario
// ========================================

console.log("\n--- Testing Complex Graph Scenario ---");

Graph.clear();

// Build a complex graph: 2 sources -> merge -> transform -> split -> 2 sinks
var src1 = Graph.createNode("SOURCE", 0, 0);
var src2 = Graph.createNode("SOURCE", 0, 100);
var mrg = Graph.createNode("MERGE", 200, 50);
var tran = Graph.createNode("TRANSFORM", 400, 50);
var spl = Graph.createNode("SPLIT", 600, 50);
var snk1 = Graph.createNode("SINK", 800, 0);
var snk2 = Graph.createNode("SINK", 800, 100);

Graph.connect(src1, 0, mrg, 0);
Graph.connect(src2, 0, mrg, 1);
Graph.connect(mrg, 0, tran, 0);
Graph.connect(tran, 0, spl, 0);
Graph.connect(spl, 0, snk1, 0);
Graph.connect(spl, 1, snk2, 0);

var complexStats = Graph.getStats();
assertEquals("complex graph nodes", complexStats.nodes, 7);
assertEquals("complex graph edges", complexStats.edges, 6);

// Save and reload complex graph
Graph.saveXml("test_complex.xml");
Graph.clear();
Graph.loadXml("test_complex.xml");

var reloadedStats = Graph.getStats();
assertEquals("complex graph reloaded nodes", reloadedStats.nodes, 7);
assertEquals("complex graph reloaded edges", reloadedStats.edges, 6);

// Delete middle node (should cascade edge deletions)
Graph.deleteNode(tran);
var afterDeleteStats = Graph.getStats();
assertEquals("complex graph after delete nodes", afterDeleteStats.nodes, 6);
assertTrue("complex graph after delete edges reduced", afterDeleteStats.edges < 6);

// ========================================
// Test Summary
// ========================================

console.log("\n========================================");
console.log("Test Results:");
console.log("  PASSED: " + testsPassed);
console.log("  FAILED: " + testsFailed);
console.log("  TOTAL:  " + (testsPassed + testsFailed));
console.log("========================================");

if (testsFailed === 0) {
    console.log("\n✓ All tests PASSED!");
} else {
    console.log("\n✗ Some tests FAILED");
}

console.log("\nQGraph API test complete.");
