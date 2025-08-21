// Test Palette System - Test all 5 palette node types
// This script tests the core node types from the node palette

console.log("=== Palette System Test Starting ===");
console.log("Testing all 5 core node types: SOURCE, SINK, TRANSFORM, SPLIT, MERGE");

function logTestStart(testName) {
    console.log(`\nTEST: ${testName}`);
    console.log("─".repeat(50));
}

function logTestResult(success, message) {
    const status = success ? "PASS" : "FAIL";
    console.log(`${status}: ${message}`);
}

function logNodeCreation(nodeType, nodeId, x, y) {
    console.log(`   Created ${nodeType} node: ${nodeId} at (${x}, ${y})`);
}

function logConnection(fromNode, fromSocket, toNode, toSocket, edgeId) {
    console.log(`   Connected ${fromNode}[${fromSocket}] → ${toNode}[${toSocket}] = ${edgeId}`);
}

function logGraphStats() {
    try {
        const stats = Graph.getStats();
        console.log(`   Graph stats: ${stats.nodes} nodes, ${stats.edges} edges`);
        return stats;
    } catch (error) {
        console.log(`   Failed to get graph stats: ${error}`);
        return null;
    }
}

// Test 1: SOURCE Node Creation
function testSourceNode() {
    logTestStart("SOURCE Node Creation");
    
    try {
        const sourceId = Graph.createNode("SOURCE", 100, 100);
        logNodeCreation("SOURCE", sourceId, 100, 100);
        
        if (sourceId && sourceId.length > 0) {
            logTestResult(true, "SOURCE node created successfully");
            return sourceId;
        } else {
            logTestResult(false, "SOURCE node creation returned invalid ID");
            return null;
        }
    } catch (error) {
        logTestResult(false, `SOURCE node creation failed: ${error}`);
        return null;
    }
}

// Test 2: SINK Node Creation
function testSinkNode() {
    logTestStart("SINK Node Creation");
    
    try {
        const sinkId = Graph.createNode("SINK", 500, 100);
        logNodeCreation("SINK", sinkId, 500, 100);
        
        if (sinkId && sinkId.length > 0) {
            logTestResult(true, "SINK node created successfully");
            return sinkId;
        } else {
            logTestResult(false, "SINK node creation returned invalid ID");
            return null;
        }
    } catch (error) {
        logTestResult(false, `SINK node creation failed: ${error}`);
        return null;
    }
}

// Test 3: Processing Node Creation (TRANSFORM, SPLIT, MERGE)
function testProcessingNodes() {
    logTestStart("Processing Node Types");
    
    const processingTypes = ["TRANSFORM", "SPLIT", "MERGE"];
    const createdNodes = [];
    
    for (let i = 0; i < processingTypes.length; i++) {
        const nodeType = processingTypes[i];
        const x = 200 + (i * 100);
        const y = 200;
        
        try {
            const nodeId = Graph.createNode(nodeType, x, y);
            logNodeCreation(nodeType, nodeId, x, y);
            
            if (nodeId && nodeId.length > 0) {
                createdNodes.push({ type: nodeType, id: nodeId });
                logTestResult(true, `${nodeType} node created successfully`);
            } else {
                logTestResult(false, `${nodeType} node creation returned invalid ID`);
            }
        } catch (error) {
            logTestResult(false, `${nodeType} node creation failed: ${error}`);
        }
    }
    
    return createdNodes;
}

// Test 4: Node Connection Testing
function testNodeConnections(sourceId, sinkId, processingNodes) {
    logTestStart("Node Connection Testing");
    
    if (!sourceId || !sinkId || processingNodes.length === 0) {
        logTestResult(false, "Cannot test connections - missing nodes");
        return false;
    }
    
    let connectionsSucceeded = 0;
    let connectionsAttempted = 0;
    
    // Test basic Source → Sink connection
    try {
        connectionsAttempted++;
        const edge1 = Graph.connect(sourceId, 0, sinkId, 0);
        logConnection(sourceId, 0, sinkId, 0, edge1);
        
        if (edge1) {
            connectionsSucceeded++;
            logTestResult(true, "Direct Source → Sink connection successful");
        } else {
            logTestResult(false, "Direct Source → Sink connection failed");
        }
    } catch (error) {
        logTestResult(false, `Direct connection failed: ${error}`);
    }
    
    // Test Source → Processing → Sink chain
    if (processingNodes.length > 0) {
        const processor = processingNodes[0]; // Use first processing node
        
        try {
            // Clear previous connection first
            Graph.clear();
            
            // Recreate nodes for clean test
            const newSource = Graph.createNode("SOURCE", 100, 300);
            const newProcessor = Graph.createNode(processor.type, 300, 300);
            const newSink = Graph.createNode("SINK", 500, 300);
            
            // Connect Source → Processor
            connectionsAttempted++;
            const edge2 = Graph.connect(newSource, 0, newProcessor, 0);
            logConnection(newSource, 0, newProcessor, 0, edge2);
            
            if (edge2) {
                connectionsSucceeded++;
                
                // Connect Processor → Sink
                connectionsAttempted++;
                const edge3 = Graph.connect(newProcessor, 0, newSink, 0);
                logConnection(newProcessor, 0, newSink, 0, edge3);
                
                if (edge3) {
                    connectionsSucceeded++;
                    logTestResult(true, "Processing chain connection successful");
                } else {
                    logTestResult(false, "Processor → Sink connection failed");
                }
            } else {
                logTestResult(false, "Source → Processor connection failed");
            }
        } catch (error) {
            logTestResult(false, `Processing chain connection failed: ${error}`);
        }
    }
    
    const successRate = connectionsAttempted > 0 ? (connectionsSucceeded / connectionsAttempted * 100).toFixed(1) : 0;
    logTestResult(
        connectionsSucceeded === connectionsAttempted,
        `Connection success rate: ${connectionsSucceeded}/${connectionsAttempted} (${successRate}%)`
    );
    
    return connectionsSucceeded === connectionsAttempted;
}

// Test 5: Graph State Validation
function testGraphState() {
    logTestStart("Graph State Validation");
    
    try {
        const stats = Graph.getStats();
        const nodes = Graph.getNodes();
        const edges = Graph.getEdges();
        
        console.log(`   Final graph state:`);
        console.log(`   - Nodes: ${stats.nodes} (${nodes.length} in array)`);
        console.log(`   - Edges: ${stats.edges} (${edges.length} in array)`);
        
        // Validate node details
        if (nodes.length > 0) {
            console.log(`   Node details:`);
            for (let i = 0; i < nodes.length && i < 5; i++) { // Show first 5 nodes
                const node = nodes[i];
                console.log(`     ${i + 1}. ${node.type || "Unknown"} [${node.id || "No ID"}] at (${node.x || "?"}, ${node.y || "?"})`);
            }
            if (nodes.length > 5) {
                console.log(`     ... and ${nodes.length - 5} more nodes`);
            }
        }
        
        // Validate edge details
        if (edges.length > 0) {
            console.log(`   Edge details:`);
            for (let i = 0; i < edges.length && i < 5; i++) { // Show first 5 edges
                const edge = edges[i];
                console.log(`     ${i + 1}. ${edge.fromNode || "?"}[${edge.fromIndex || "?"}] → ${edge.toNode || "?"}[${edge.toIndex || "?"}]`);
            }
            if (edges.length > 5) {
                console.log(`     ... and ${edges.length - 5} more edges`);
            }
        }
        
        const isValid = stats.nodes >= 0 && stats.edges >= 0 && 
                       stats.nodes === nodes.length && stats.edges === edges.length;
        
        logTestResult(isValid, isValid ? "Graph state is consistent" : "Graph state inconsistency detected");
        return isValid;
    } catch (error) {
        logTestResult(false, `Graph state validation failed: ${error}`);
        return false;
    }
}

// Test 6: Save/Load Test
function testSaveLoad() {
    logTestStart("Save/Load Functionality");
    
    try {
        const beforeStats = Graph.getStats();
        console.log(`   Before save: ${beforeStats.nodes} nodes, ${beforeStats.edges} edges`);
        
        // Save the graph
        Graph.saveXml("palette_test_graph.xml");
        console.log("   Saved graph to palette_test_graph.xml");
        
        // Clear the graph
        Graph.clear();
        const afterClear = Graph.getStats();
        console.log(`   After clear: ${afterClear.nodes} nodes, ${afterClear.edges} edges`);
        
        // Load the graph back
        Graph.loadXml("palette_test_graph.xml");
        const afterLoad = Graph.getStats();
        console.log(`   After load: ${afterLoad.nodes} nodes, ${afterLoad.edges} edges`);
        
        const saveLoadWorking = (beforeStats.nodes === afterLoad.nodes && 
                                beforeStats.edges === afterLoad.edges);
        
        logTestResult(saveLoadWorking, saveLoadWorking ? 
            "Save/Load cycle successful" : 
            `Save/Load mismatch - expected ${beforeStats.nodes}/${beforeStats.edges}, got ${afterLoad.nodes}/${afterLoad.edges}`
        );
        
        return saveLoadWorking;
    } catch (error) {
        logTestResult(false, `Save/Load test failed: ${error}`);
        return false;
    }
}

// Main Test Execution
function runPaletteSystemTests() {
    console.log("🏁 Starting comprehensive palette system test...");
    
    // Clear graph to start fresh
    try {
        Graph.clear();
        console.log("Cleared graph for fresh start");
    } catch (error) {
        console.log(`Warning: Could not clear graph: ${error}`);
    }
    
    const results = {
        total: 0,
        passed: 0,
        failed: 0,
        tests: []
    };
    
    // Run individual tests
    const sourceId = testSourceNode();
    results.total++;
    if (sourceId) results.passed++; else results.failed++;
    results.tests.push({name: "SOURCE Node", passed: !!sourceId});
    
    const sinkId = testSinkNode();
    results.total++;
    if (sinkId) results.passed++; else results.failed++;
    results.tests.push({name: "SINK Node", passed: !!sinkId});
    
    const processingNodes = testProcessingNodes();
    results.total++;
    const processingPassed = processingNodes.length > 0;
    if (processingPassed) results.passed++; else results.failed++;
    results.tests.push({name: "Processing Nodes", passed: processingPassed});
    
    const connectionsWorking = testNodeConnections(sourceId, sinkId, processingNodes);
    results.total++;
    if (connectionsWorking) results.passed++; else results.failed++;
    results.tests.push({name: "Node Connections", passed: connectionsWorking});
    
    const stateValid = testGraphState();
    results.total++;
    if (stateValid) results.passed++; else results.failed++;
    results.tests.push({name: "Graph State", passed: stateValid});
    
    const saveLoadWorking = testSaveLoad();
    results.total++;
    if (saveLoadWorking) results.passed++; else results.failed++;
    results.tests.push({name: "Save/Load", passed: saveLoadWorking});
    
    // Final results
    console.log("\n" + "=".repeat(60));
    console.log("🏆 PALETTE SYSTEM TEST RESULTS");
    console.log("=".repeat(60));
    console.log(`Total tests: ${results.total}`);
    console.log(`Passed: ${results.passed}`);
    console.log(`Failed: ${results.failed}`);
    console.log(`Success rate: ${(results.passed / results.total * 100).toFixed(1)}%`);
    
    console.log("\nDetailed results:");
    for (const test of results.tests) {
        const status = test.passed ? "PASS" : "FAIL";
        console.log(`  ${status} ${test.name}`);
    }
    
    const finalStats = logGraphStats();
    if (finalStats) {
        console.log(`\nFinal graph: ${finalStats.nodes} nodes, ${finalStats.edges} edges`);
    }
    
    console.log("\nPalette test files generated:");
    console.log("   - palette_test_graph.xml (test graph for inspection)");
    
    const overallSuccess = results.failed === 0;
    console.log(`\nOverall result: ${overallSuccess ? "ALL TESTS PASSED" : "SOME TESTS FAILED"}`);
    console.log("=== Palette System Test Complete ===");
    
    return {
        success: overallSuccess,
        passed: results.passed,
        total: results.total,
        message: `Palette System Test: ${results.passed}/${results.total} tests passed`
    };
}

// Execute the tests
runPaletteSystemTests();