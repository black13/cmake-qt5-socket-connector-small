// Simple Enumeration Script
// This script enumerates and numbers items in the graph

console.log("=== Simple Enumeration Script ===");

function enumerateNodes() {
    console.log("Enumerating all nodes in the graph...");
    
    let nodes = Graph.getNodes();
    
    if (nodes.length === 0) {
        console.log("No nodes found in the graph");
        return [];
    }
    
    let enumeratedNodes = [];
    
    for (let i = 0; i < nodes.length; i++) {
        let node = nodes[i];
        let enumerated = {
            number: i + 1,
            id: node.id,
            type: node.type,
            position: { x: node.x, y: node.y },
            label: `Node #${i + 1}: ${node.type}`
        };
        
        enumeratedNodes.push(enumerated);
        console.log(`${i + 1}. ${node.type} (${node.id}) at (${node.x}, ${node.y})`);
    }
    
    console.log(`Total: ${nodes.length} nodes enumerated`);
    return enumeratedNodes;
}

function enumerateEdges() {
    console.log("Enumerating all edges in the graph...");
    
    let edges = Graph.getEdges();
    
    if (edges.length === 0) {
        console.log("No edges found in the graph");
        return [];
    }
    
    let enumeratedEdges = [];
    
    for (let i = 0; i < edges.length; i++) {
        let edge = edges[i];
        let enumerated = {
            number: i + 1,
            id: edge.id,
            from: edge.fromNode,
            to: edge.toNode,
            fromSocket: edge.fromIndex,
            toSocket: edge.toIndex,
            label: `Edge #${i + 1}: ${edge.fromNode}[${edge.fromIndex}] → ${edge.toNode}[${edge.toIndex}]`
        };
        
        enumeratedEdges.push(enumerated);
        console.log(`${i + 1}. ${edge.fromNode}[${edge.fromIndex}] → ${edge.toNode}[${edge.toIndex}]`);
    }
    
    console.log(`Total: ${edges.length} edges enumerated`);
    return enumeratedEdges;
}

function enumerateByType() {
    console.log("Enumerating nodes by type...");
    
    let nodes = Graph.getNodes();
    let nodeTypes = {};
    
    // Group nodes by type
    for (let node of nodes) {
        if (!nodeTypes[node.type]) {
            nodeTypes[node.type] = [];
        }
        nodeTypes[node.type].push(node);
    }
    
    let enumeration = {};
    
    // Enumerate each type
    for (let type in nodeTypes) {
        console.log(`\n${type} nodes:`);
        enumeration[type] = [];
        
        for (let i = 0; i < nodeTypes[type].length; i++) {
            let node = nodeTypes[type][i];
            let enumerated = {
                number: i + 1,
                id: node.id,
                position: { x: node.x, y: node.y },
                label: `${type} #${i + 1}`
            };
            
            enumeration[type].push(enumerated);
            console.log(`  ${i + 1}. ${node.id} at (${node.x}, ${node.y})`);
        }
        
        console.log(`  Total ${type} nodes: ${nodeTypes[type].length}`);
    }
    
    return enumeration;
}

function numberNodesSequentially() {
    console.log("Numbering nodes sequentially...");
    
    let nodes = Graph.getNodes();
    let numberedNodes = [];
    
    for (let i = 0; i < nodes.length; i++) {
        let node = nodes[i];
        let numbered = {
            originalId: node.id,
            sequentialNumber: i + 1,
            newLabel: `${i + 1}_${node.type}`,
            type: node.type,
            position: { x: node.x, y: node.y }
        };
        
        numberedNodes.push(numbered);
        console.log(`Node ${node.id} → #${i + 1} (${node.type})`);
    }
    
    return numberedNodes;
}

function createNumberedList() {
    console.log("Creating numbered list of all graph elements...");
    
    let list = [];
    let counter = 1;
    
    // Add nodes
    let nodes = Graph.getNodes();
    for (let node of nodes) {
        list.push({
            number: counter++,
            type: "Node",
            element: node,
            description: `${node.type} node at (${node.x}, ${node.y})`
        });
    }
    
    // Add edges
    let edges = Graph.getEdges();
    for (let edge of edges) {
        list.push({
            number: counter++,
            type: "Edge", 
            element: edge,
            description: `Connection from ${edge.fromNode}[${edge.fromIndex}] to ${edge.toNode}[${edge.toIndex}]`
        });
    }
    
    console.log("Complete numbered list:");
    for (let item of list) {
        console.log(`${item.number}. ${item.type}: ${item.description}`);
    }
    
    console.log(`Total items: ${list.length}`);
    return list;
}

function generateInventory() {
    console.log("Generating graph inventory...");
    
    let stats = Graph.getStats();
    let inventory = {
        summary: {
            totalNodes: stats.nodes,
            totalEdges: stats.edges,
            inventoryDate: new Date().toISOString()
        },
        nodes: enumerateNodes(),
        edges: enumerateEdges(),
        byType: enumerateByType()
    };
    
    console.log("\n=== GRAPH INVENTORY ===");
    console.log(`Date: ${inventory.summary.inventoryDate}`);
    console.log(`Total Nodes: ${inventory.summary.totalNodes}`);
    console.log(`Total Edges: ${inventory.summary.totalEdges}`);
    
    console.log("\nNode Types Summary:");
    for (let type in inventory.byType) {
        console.log(`  ${type}: ${inventory.byType[type].length} nodes`);
    }
    
    return inventory;
}

// Main execution
console.log("Starting enumeration...");

// Check if graph has content
let stats = Graph.getStats();
if (stats.nodes === 0) {
    console.log("Graph is empty. Creating sample nodes for demonstration...");
    
    // Create some sample nodes
    Graph.createNode("Source", 100, 100);
    Graph.createNode("1-to-2", 250, 100);
    Graph.createNode("Sink", 400, 80);
    Graph.createNode("Sink", 400, 120);
    
    console.log("Created sample nodes for enumeration");
}

// Run all enumeration functions
console.log("\n1. Enumerating nodes:");
let nodes = enumerateNodes();

console.log("\n2. Enumerating edges:");
let edges = enumerateEdges();

console.log("\n3. Enumerating by type:");
let byType = enumerateByType();

console.log("\n4. Creating numbered sequence:");
let numbered = numberNodesSequentially();

console.log("\n5. Creating complete numbered list:");
let completeList = createNumberedList();

console.log("\n6. Generating inventory:");
let inventory = generateInventory();

console.log("\n=== Enumeration Complete ===");
console.log("All items have been enumerated and numbered!");

// Return the complete enumeration results
let results = {
    nodes: nodes,
    edges: edges,
    byType: byType,
    numbered: numbered,
    completeList: completeList,
    inventory: inventory
};

console.log(`Enumeration script completed successfully with ${results.completeList.length} total items.`);

// For script execution, return a summary
results;