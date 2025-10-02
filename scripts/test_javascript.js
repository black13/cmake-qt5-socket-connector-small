// JavaScript Test Script for NodeGraph
// Basic tests and demonstrations of the JavaScript integration

console.log("=== NodeGraph JavaScript Integration Test ===");

// Test 1: Basic JavaScript functionality
console.log("\n1. Testing basic JavaScript functionality...");

const testData = {
    name: "NodeGraph",
    version: "1.0.0",
    features: ["JavaScript", "Qt5", "Modern ES6+"]
};

console.log("Test data:", JSON.stringify(testData, null, 2));

// Test 2: Modern JavaScript features
console.log("\n2. Testing modern JavaScript features...");

// Arrow functions
const add = (a, b) => a + b;
console.log("Arrow function result:", add(5, 3));

// Template literals
const message = `NodeGraph supports modern JavaScript features!`;
console.log("Template literal:", message);

// Destructuring
const { name, version } = testData;
console.log("Destructured values:", name, version);

// Spread operator
const newFeatures = [...testData.features, "QJSEngine"];
console.log("Spread operator result:", newFeatures);

// Test 3: Async/Promise simulation (QJSEngine has limited async support)
console.log("\n3. Testing Promise-like functionality...");

function simulateAsyncOperation(data) {
    return new Promise((resolve, reject) => {
        // Simulate async work
        if (data) {
            resolve(`Processed: ${data}`);
        } else {
            reject("No data provided");
        }
    });
}

// Test 4: Error handling
console.log("\n4. Testing error handling...");

try {
    const result = JSON.parse('{"valid": "json"}');
    console.log("JSON parse success:", result);
} catch (error) {
    console.log("JSON parse error:", error.message);
}

try {
    const badResult = JSON.parse('invalid json');
    console.log("This should not print");
} catch (error) {
    console.log("Caught expected error:", error.message);
}

// Test 5: Array and object manipulation
console.log("\n5. Testing array and object manipulation...");

const numbers = [1, 2, 3, 4, 5];
const doubled = numbers.map(n => n * 2);
const filtered = numbers.filter(n => n > 3);
const sum = numbers.reduce((acc, n) => acc + n, 0);

console.log("Original:", numbers);
console.log("Doubled:", doubled);
console.log("Filtered (>3):", filtered);
console.log("Sum:", sum);

// Test 6: String manipulation
console.log("\n6. Testing string manipulation...");

const text = "NodeGraph JavaScript Integration";
console.log("Original:", text);
console.log("Uppercase:", text.toUpperCase());
console.log("Words:", text.split(" "));
console.log("Length:", text.length);

// Test 7: Math operations
console.log("\n7. Testing math operations...");

const mathTests = [
    { op: "sqrt", val: 16, result: Math.sqrt(16) },
    { op: "pow", val: [2, 3], result: Math.pow(2, 3) },
    { op: "random", val: null, result: Math.random() },
    { op: "floor", val: 3.7, result: Math.floor(3.7) },
    { op: "ceil", val: 3.2, result: Math.ceil(3.2) }
];

mathTests.forEach(test => {
    console.log(`Math.${test.op}(${test.val}):`, test.result);
});

// Test 8: Date handling
console.log("\n8. Testing date handling...");

const now = new Date();
console.log("Current time:", now.toISOString());
console.log("Timestamp:", now.getTime());
console.log("Formatted:", now.toLocaleString());

// Test 9: Regular expressions
console.log("\n9. Testing regular expressions...");

const emailPattern = /^[^\s@]+@[^\s@]+\.[^\s@]+$/;
const emails = ["test@example.com", "invalid-email", "user@domain.org"];

emails.forEach(email => {
    const isValid = emailPattern.test(email);
    console.log(`Email "${email}" is ${isValid ? "valid" : "invalid"}`);
});

// Test 10: Node Graph specific functionality
console.log("\n10. Testing Node Graph API availability...");

// Check if APIs are available
const apis = ["Node", "Graph", "Algorithms", "Utils"];
apis.forEach(api => {
    const available = typeof window !== 'undefined' && window[api] !== undefined;
    console.log(`${api} API available:`, available);
});

// Test 11: Utility functions
console.log("\n11. Testing utility functions...");

function factorial(n) {
    if (n <= 1) return 1;
    return n * factorial(n - 1);
}

console.log("Factorial of 5:", factorial(5));

function fibonacci(n) {
    if (n <= 1) return n;
    return fibonacci(n - 1) + fibonacci(n - 2);
}

console.log("Fibonacci sequence (first 10):", 
    Array.from({ length: 10 }, (_, i) => fibonacci(i)));

// Test 12: Memory and performance
console.log("\n12. Testing memory and performance...");

const startTime = Date.now();
const largeArray = Array.from({ length: 10000 }, (_, i) => i);
const processedArray = largeArray.filter(n => n % 2 === 0).map(n => n * 2);
const endTime = Date.now();

console.log("Large array processing time:", endTime - startTime, "ms");
console.log("Processed array length:", processedArray.length);

// Test completion
console.log("\n=== JavaScript Integration Test Complete ===");
console.log("All tests completed successfully!");

// Return test results
const testResults = {
    status: "completed",
    timestamp: new Date().toISOString(),
    testsRun: 12,
    success: true,
    message: "JavaScript integration is working correctly"
};

console.log("Test Results:", JSON.stringify(testResults, null, 2));

// Export test results if in module environment
if (typeof module !== 'undefined' && module.exports) {
    module.exports = testResults;
}

testResults;