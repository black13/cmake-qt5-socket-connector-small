// Minimal test to prove JavaScript→C++ bridge works
console.log("=== MINIMAL TEST SCRIPT ===");
console.log("Script: minimal_test.js");
console.log("Purpose: Prove JavaScript→C++ bridge functionality");
console.log("JavaScript: Starting minimal test");
Graph.clear();
console.log("JavaScript: Called Graph.clear() successfully");
var stats = Graph.getStats();
console.log("JavaScript: Got stats:", JSON.stringify(stats));
console.log("JavaScript: Test complete");
console.log("=== MINIMAL TEST COMPLETE ===");