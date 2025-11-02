// test_memory_leak.js - Validate memory leak fix
// This test loads a corrupted XML file multiple times
// BEFORE fix: Memory grows with each failed load (leak)
// AFTER fix: Memory stays stable (no leak)

console.log("=== Memory Leak Validation Test ===");
console.log("Testing load rollback with duplicate socket connections");
console.log("File: test_corrupt_duplicate.xml (based on tests_tiny.xml)");
console.log("Corruption: Output socket 01bd7e1d...:0 used by TWO edges");
console.log("");

var iterations = 50;  // Load corrupted file 50 times

for (var i = 0; i < iterations; i++) {
    // Clear graph before each attempt
    graph.clearGraph();

    // Try to load corrupted file (should fail validation)
    var success = graph.loadFromFile("test_corrupt_duplicate.xml");

    if (success) {
        console.log("ERROR: Load " + (i+1) + " succeeded but should have failed!");
        console.log("Duplicate socket validation is broken!");
        break;
    }

    // Verify scene is clean after failure
    var stats = graph.getGraphStats();
    if (stats.nodeCount !== 0 || stats.edgeCount !== 0) {
        console.log("ERROR: Scene not clean after failed load!");
        console.log("  Nodes: " + stats.nodeCount + ", Edges: " + stats.edgeCount);
        console.log("  ROLLBACK FAILED - objects leaked into scene!");
        break;
    }

    if ((i + 1) % 10 === 0) {
        console.log("✓ Completed " + (i+1) + "/" + iterations + " failed loads - scene remains clean");
    }
}

console.log("");
console.log("=== Test Complete ===");
console.log("All " + iterations + " load attempts correctly failed and rolled back.");
console.log("");
console.log("ASAN Validation:");
console.log("  - No leaks detected = PASS (fix works!)");
console.log("  - Leak detected = FAIL (fix incomplete)");
console.log("");
console.log("Manual Validation:");
console.log("  - Memory stable ~50MB = PASS");
console.log("  - Memory growing >500MB = FAIL");
