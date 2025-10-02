// Simple Counter Script
// This script demonstrates basic enumeration and counting

console.log("=== Simple Counter Script ===");

// Simple counter that enumerates from 1 to N
function countToNumber(max) {
    console.log(`Counting from 1 to ${max}:`);
    
    let items = [];
    for (let i = 1; i <= max; i++) {
        let item = {
            number: i,
            label: `Item #${i}`,
            isEven: i % 2 === 0,
            isOdd: i % 2 === 1,
            square: i * i
        };
        
        items.push(item);
        console.log(`${i}. Item #${i} (${item.isEven ? 'even' : 'odd'}) - square: ${item.square}`);
    }
    
    console.log(`Counted ${items.length} items`);
    return items;
}

// Enumerate letters of the alphabet
function enumerateAlphabet(count = 26) {
    console.log(`Enumerating alphabet (first ${count} letters):`);
    
    let letters = [];
    for (let i = 0; i < count && i < 26; i++) {
        let letter = String.fromCharCode(65 + i); // A-Z
        let item = {
            number: i + 1,
            letter: letter,
            asciiCode: 65 + i,
            lowercase: letter.toLowerCase()
        };
        
        letters.push(item);
        console.log(`${i + 1}. ${letter} (ascii: ${item.asciiCode}, lower: ${item.lowercase})`);
    }
    
    console.log(`Enumerated ${letters.length} letters`);
    return letters;
}

// Simple statistics counter
function generateStats() {
    console.log("Generating simple statistics...");
    
    let stats = {
        timestamp: new Date().toISOString(),
        random_numbers: [],
        total: 0,
        count: 10
    };
    
    console.log("Generating 10 random numbers:");
    for (let i = 1; i <= 10; i++) {
        let randomNum = Math.floor(Math.random() * 100) + 1;
        stats.random_numbers.push({
            position: i,
            value: randomNum
        });
        stats.total += randomNum;
        
        console.log(`${i}. Random number: ${randomNum}`);
    }
    
    stats.average = stats.total / stats.count;
    
    console.log(`Total: ${stats.total}`);
    console.log(`Average: ${stats.average.toFixed(2)}`);
    
    return stats;
}

// Count graph elements (if any exist)
function countGraphElements() {
    console.log("Counting elements in the current graph...");
    
    let graphStats = Graph.getStats();
    let summary = {
        nodes: graphStats.nodes || 0,
        edges: graphStats.edges || 0,
        total: (graphStats.nodes || 0) + (graphStats.edges || 0)
    };
    
    console.log(`Graph contains:`);
    console.log(`1. Nodes: ${summary.nodes}`);
    console.log(`2. Edges: ${summary.edges}`);
    console.log(`3. Total elements: ${summary.total}`);
    
    if (summary.total === 0) {
        console.log("Graph is empty - no elements to count");
    }
    
    return summary;
}

// Main execution
console.log("Starting simple enumeration demonstrations...");

console.log("\n--- Demo 1: Count to 5 ---");
let counting = countToNumber(5);

console.log("\n--- Demo 2: First 10 letters ---");
let alphabet = enumerateAlphabet(10);

console.log("\n--- Demo 3: Random statistics ---");
let stats = generateStats();

console.log("\n--- Demo 4: Graph element count ---");
let graphCount = countGraphElements();

// Final summary
console.log("\n=== Summary ===");
console.log(`Counted numbers: ${counting.length}`);
console.log(`Enumerated letters: ${alphabet.length}`);
console.log(`Random numbers generated: ${stats.count}`);
console.log(`Graph elements: ${graphCount.total}`);

let totalEnumerated = counting.length + alphabet.length + stats.count + graphCount.total;
console.log(`Total items enumerated: ${totalEnumerated}`);

console.log("\n=== Simple Counter Script Complete ===");

// Return summary for script execution
let result = {
    success: true,
    demos_completed: 4,
    total_items_enumerated: totalEnumerated,
    details: {
        counting: counting.length,
        alphabet: alphabet.length,
        statistics: stats.count,
        graph_elements: graphCount.total
    }
};

console.log(`Script completed successfully! Enumerated ${result.total_items_enumerated} items total.`);

result;