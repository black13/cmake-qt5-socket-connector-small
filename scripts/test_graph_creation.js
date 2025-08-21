// Test Script: Graph Creation and Basic Operations
// This script tests the fundamental graph creation capabilities

function test_graph_creation() {
    console.log("=== Graph Creation Test ===");
    
    // Clear any existing graph
    Graph.clear();
    console.log("Graph cleared");
    
    // Get initial stats
    let initialStats = Graph.getStats();
    console.log("Initial stats:", JSON.stringify(initialStats));
    
    // Create a simple 3-node pipeline
    console.log("Creating 3-node pipeline...");
    
    let sourceNode = Graph.createNode("Source", 100, 100);
    console.log("Created Source node:", sourceNode);
    
    let processorNode = Graph.createNode("1-to-2", 250, 100);
    console.log("Created 1-to-2 node:", processorNode);
    
    let sinkNode1 = Graph.createNode("Sink", 400, 50);
    console.log("Created Sink node 1:", sinkNode1);
    
    let sinkNode2 = Graph.createNode("Sink", 400, 150);
    console.log("Created Sink node 2:", sinkNode2);
    
    // Connect the nodes
    console.log("Connecting nodes...");
    
    let edge1 = Graph.connect(sourceNode, 0, processorNode, 0);
    console.log("Connected source to processor:", edge1);
    
    let edge2 = Graph.connect(processorNode, 0, sinkNode1, 0);
    console.log("Connected processor to sink 1:", edge2);
    
    let edge3 = Graph.connect(processorNode, 1, sinkNode2, 0);
    console.log("Connected processor to sink 2:", edge3);
    
    // Get final stats
    let finalStats = Graph.getStats();
    console.log("Final stats:", JSON.stringify(finalStats));
    
    // Validate the graph structure
    let nodes = Graph.getNodes();
    let edges = Graph.getEdges();
    
    console.log("Node count:", nodes.length);
    console.log("Edge count:", edges.length);
    
    // List all nodes
    console.log("All nodes:");
    for (let i = 0; i < nodes.length; i++) {
        let node = nodes[i];
        console.log(`  Node ${i}: ${node.id} (${node.type}) at (${node.x}, ${node.y})`);
    }
    
    // List all edges
    console.log("All edges:");
    for (let i = 0; i < edges.length; i++) {
        let edge = edges[i];
        console.log(`  Edge ${i}: ${edge.id} from ${edge.fromNode}[${edge.fromIndex}] to ${edge.toNode}[${edge.toIndex}]`);
    }
    
    // Save the graph
    Graph.saveXml("test_graph_creation.xml");
    console.log("Graph saved to test_graph_creation.xml");
    
    console.log("=== Graph Creation Test Complete ===");
    
    return {
        success: true,
        nodesCreated: nodes.length,
        edgesCreated: edges.length,
        message: "Graph creation test passed"
    };
}

function test_node_deletion() {
    console.log("=== Node Deletion Test ===");
    
    // Start with a simple graph
    Graph.clear();
    
    let node1 = Graph.createNode("Source", 100, 100);
    let node2 = Graph.createNode("Sink", 300, 100);
    let edge = Graph.connect(node1, 0, node2, 0);
    
    console.log("Created test graph with 2 nodes and 1 edge");
    
    let beforeStats = Graph.getStats();
    console.log("Before deletion:", JSON.stringify(beforeStats));
    
    // Delete the source node (should also delete the edge)
    let deleteResult = Graph.deleteNode(node1);
    console.log("Delete result:", deleteResult);
    
    let afterStats = Graph.getStats();
    console.log("After deletion:", JSON.stringify(afterStats));
    
    // Verify the edge was also deleted
    let remainingEdges = Graph.getEdges();
    console.log("Remaining edges:", remainingEdges.length);
    
    console.log("=== Node Deletion Test Complete ===");
    
    return {
        success: deleteResult && remainingEdges.length === 0,
        message: "Node deletion test " + (deleteResult ? "passed" : "failed")
    };
}

function test_edge_operations() {
    console.log("=== Edge Operations Test ===");
    
    Graph.clear();
    
    // Create a more complex graph
    let source = Graph.createNode("Source", 50, 100);
    let processor1 = Graph.createNode("1-to-2", 200, 100);
    let processor2 = Graph.createNode("2-to-1", 350, 100);
    let sink = Graph.createNode("Sink", 500, 100);
    
    // Create connections
    let edge1 = Graph.connect(source, 0, processor1, 0);
    let edge2 = Graph.connect(processor1, 0, processor2, 0);
    let edge3 = Graph.connect(processor1, 1, processor2, 1);
    let edge4 = Graph.connect(processor2, 0, sink, 0);
    
    console.log("Created complex graph with 4 nodes and 4 edges");
    
    let beforeStats = Graph.getStats();
    console.log("Before edge deletion:", JSON.stringify(beforeStats));
    
    // Delete one edge
    let deleteResult = Graph.deleteEdge(edge2);
    console.log("Edge deletion result:", deleteResult);
    
    let afterStats = Graph.getStats();
    console.log("After edge deletion:", JSON.stringify(afterStats));
    
    // Verify edge count
    let edges = Graph.getEdges();
    console.log("Remaining edges:", edges.length);
    
    console.log("=== Edge Operations Test Complete ===");
    
    return {
        success: deleteResult && edges.length === 3,
        message: "Edge operations test " + (deleteResult ? "passed" : "failed")
    };
}

function test_xml_operations() {
    console.log("=== XML Operations Test ===");
    
    Graph.clear();
    
    // Create a test graph
    let node1 = Graph.createNode("Source", 100, 100);
    let node2 = Graph.createNode("Sink", 300, 100);
    let edge = Graph.connect(node1, 0, node2, 0);
    
    console.log("Created test graph for XML operations");
    
    // Get XML string
    let xmlString = Graph.getXmlString();
    console.log("XML string length:", xmlString.length);
    console.log("XML preview:", xmlString.substring(0, 200) + "...");
    
    // Save to file
    Graph.saveXml("test_xml_operations.xml");
    console.log("Graph saved to test_xml_operations.xml");
    
    // Clear and reload
    Graph.clear();
    let emptyStats = Graph.getStats();
    console.log("After clear:", JSON.stringify(emptyStats));
    
    Graph.loadXml("test_xml_operations.xml");
    let loadedStats = Graph.getStats();
    console.log("After load:", JSON.stringify(loadedStats));
    
    console.log("=== XML Operations Test Complete ===");
    
    return {
        success: loadedStats.nodes === 2 && loadedStats.edges === 1,
        message: "XML operations test " + (loadedStats.nodes === 2 ? "passed" : "failed")
    };
}

function run_all_tests() {
    console.log("=== Running All Graph Tests ===");
    
    let results = [];
    
    try {
        results.push(test_graph_creation());
        results.push(test_node_deletion());
        results.push(test_edge_operations());
        results.push(test_xml_operations());
    } catch (error) {
        console.log("Error during testing:", error.toString());
        results.push({
            success: false,
            message: "Test suite failed: " + error.toString()
        });
    }
    
    console.log("=== Test Results Summary ===");
    let passedCount = 0;
    let totalCount = results.length;
    
    for (let i = 0; i < results.length; i++) {
        let result = results[i];
        let status = result.success ? "PASS" : "FAIL";
        console.log(`Test ${i + 1}: ${status} - ${result.message}`);
        
        if (result.success) {
            passedCount++;
        }
    }
    
    console.log(`=== Final Result: ${passedCount}/${totalCount} tests passed ===`);
    
    return {
        passed: passedCount,
        total: totalCount,
        success: passedCount === totalCount
    };
}

// Auto-run all tests
run_all_tests();