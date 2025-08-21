// Construct All Node Types - Show Every Node Type Construction
// This script creates one instance of every registered node type

console.log("=== Constructing All 5 Core Node Types ===");

// Clear graph first
Graph.clear();
console.log("Graph cleared");

// The 5 core node types
const nodeTypes = ["SOURCE", "SINK", "TRANSFORM", "SPLIT", "MERGE"];

console.log("Creating one instance of each node type:");

let x = 100;
let y = 100;
const spacing = 150;

for (let i = 0; i < nodeTypes.length; i++) {
    const nodeType = nodeTypes[i];
    console.log(`${i + 1}. Constructing ${nodeType} node...`);
    
    const node = Graph.createNode(nodeType, x, y);
    console.log(`   ${nodeType} node created: ${node}`);
    
    // Position next node
    x += spacing;
    if (x > 800) {
        x = 100;
        y += spacing;
    }
}

console.log("=== All 5 Core Node Types Constructed ===");

// Save the constructed graph to XML file named after the test
const xmlFileName = "construct_all_node_types_test.xml";
console.log(`Saving constructed nodes to: ${xmlFileName}`);
Graph.saveXml(xmlFileName);
console.log("XML file saved successfully!");

// Return summary
const result = {
    typesConstructed: nodeTypes.length,
    nodeTypes: nodeTypes,
    savedToFile: xmlFileName
};

result;