// Hello World - Simple JavaScript test for CLI --script option
console.log("=== Hello from JavaScript! ===");
console.log("Graph facade is available as 'graph' global");

// Check what's available
console.log("Available node types: " + graph.getAvailableNodeTypes());

// Get current graph stats
var stats = graph.getGraphStats();
console.log("Current graph: " + stats.nodeCount + " nodes, " + stats.edgeCount + " edges");

console.log("=== Hello.js complete ===");
