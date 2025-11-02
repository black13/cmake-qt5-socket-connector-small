// test_load_xmls.js - Test loading working XML files via JavaScript
console.log("=== Testing XML Loading via JavaScript ===");
console.log("");

// List of working XML files to test (uppercase types)
var testFiles = [
    "tests_tiny.xml",
    "tests_small.xml",
    "tests_medium.xml",
    "tests_large.xml",
    "tests_stress.xml"
];

console.log("Testing " + testFiles.length + " XML files...");
console.log("");

var successCount = 0;
var failCount = 0;

for (var i = 0; i < testFiles.length; i++) {
    var filename = testFiles[i];
    console.log("--- Test " + (i + 1) + "/" + testFiles.length + ": " + filename + " ---");

    // Clear graph before loading
    graph.clearGraph();

    // Attempt to load
    var success = graph.loadFromFile(filename);

    if (success) {
        // Get stats
        var stats = graph.getGraphStats();
        console.log("✓ SUCCESS: Loaded " + stats.nodeCount + " nodes, " + stats.edgeCount + " edges");
        successCount++;
    } else {
        console.log("✗ FAILED to load " + filename);
        failCount++;
    }
    console.log("");
}

console.log("=== Test Results ===");
console.log("Passed: " + successCount + "/" + testFiles.length);
console.log("Failed: " + failCount + "/" + testFiles.length);
console.log("");
console.log("=== Test Complete ===");
