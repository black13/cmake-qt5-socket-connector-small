// Test __FUNCTION__ Logging Enhancement
// This script specifically tests that GraphController::createNode shows function names

console.log("=== Testing __FUNCTION__ Logging Enhancement ===");
console.log("Creating nodes to verify function-level logging works...");

// Clear graph first
Graph.clear();
console.log("PASS: Graph cleared for function logging test");

console.log("\n--- Creating each node type to trigger __FUNCTION__ logging ---");

// Create SOURCE node - should show "createNode: Creating node SOURCE at 100, 200"
console.log("1. Creating SOURCE node...");
const source = Graph.createNode("SOURCE", 100, 200);
console.log(`   Result: ${source}`);

// Create SINK node  
console.log("2. Creating SINK node...");
const sink = Graph.createNode("SINK", 500, 200);
console.log(`   Result: ${sink}`);

// Create TRANSFORM node
console.log("3. Creating TRANSFORM node...");
const transform = Graph.createNode("TRANSFORM", 300, 200);
console.log(`   Result: ${transform}`);

// Create SPLIT node
console.log("4. Creating SPLIT node...");
const split = Graph.createNode("SPLIT", 400, 300);
console.log(`   Result: ${split}`);

// Create MERGE node
console.log("5. Creating MERGE node...");
const merge = Graph.createNode("MERGE", 400, 100);
console.log(`   Result: ${merge}`);

console.log("\n--- Function Logging Test Complete ---");
console.log("Check the NodeGraph log file for entries like:");
console.log("  'createNode: Validating node type SOURCE'");
console.log("  'createNode: VALID node type: SOURCE'");
console.log("  'createNode: Creating node SOURCE at 100, 200'");
console.log("  'createNode: Node created successfully!'");
console.log("\nIf you see these patterns, the __FUNCTION__ macro is working correctly!");

const result = {
    success: true,
    message: "__FUNCTION__ logging test completed",
    nodesCreated: 5,
    expectedLogPattern: "createNode: [action]"
};

result;