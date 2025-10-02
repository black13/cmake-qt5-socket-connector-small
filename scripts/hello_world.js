// Hello World - Simple JavaScript Test
// The most basic script to test JavaScript execution

console.log("Hello World from JavaScript!");

console.log("This is a simple test script.");
console.log("Current date: " + new Date().toLocaleString());

// Test basic math
let a = 5;
let b = 3;
let sum = a + b;

console.log(`Basic math: ${a} + ${b} = ${sum}`);

// Test simple enumeration
console.log("Counting to 3:");
for (let i = 1; i <= 3; i++) {
    console.log(`${i}. Hello World #${i}`);
}

// Test graph access
let stats = Graph.getStats();
console.log(`Graph has ${stats.nodes} nodes and ${stats.edges} edges`);

console.log("Hello World script completed successfully!");

// Return a simple result
"Hello World script executed successfully!";