/**
 * Example: Grid Pattern
 *
 * Creates a 3x3 grid of nodes with horizontal connections:
 *
 * S -> T -> K
 * S -> T -> K
 * S -> T -> K
 *
 * Where: S=SOURCE, T=TRANSFORM, K=SINK
 *
 * Demonstrates programmatic graph creation with loops.
 */

console.log("----------------------------------------------------");
console.log("Example: Grid Pattern");
console.log("----------------------------------------------------");

// Clear graph
Graph.clear();

// Configuration
var rows = 3;
var cols = 3;
var spacing = 200;
var startX = 100;
var startY = 100;

console.log(`\nCreating ${rows}x${cols} grid...`);

// Create grid of nodes
var grid = [];
for (var row = 0; row < rows; row++) {
    grid[row] = [];
    for (var col = 0; col < cols; col++) {
        var x = startX + col * spacing;
        var y = startY + row * spacing;

        // Determine node type based on column
        var nodeType;
        if (col === 0) {
            nodeType = "SOURCE";
        } else if (col === cols - 1) {
            nodeType = "SINK";
        } else {
            nodeType = "TRANSFORM";
        }

        grid[row][col] = Graph.createNode(nodeType, x, y);
        console.log(`  Created ${nodeType} at row ${row}, col ${col}`);
    }
}

console.log("\nConnecting grid horizontally...");

// Connect nodes horizontally
var edgeCount = 0;
for (var row = 0; row < rows; row++) {
    for (var col = 0; col < cols - 1; col++) {
        var fromNode = grid[row][col];
        var toNode = grid[row][col + 1];

        // Determine from socket index
        // SOURCE uses socket 0 (output)
        // TRANSFORM uses socket 1 (output, after input at socket 0)
        var fromSocketIdx = (col === 0) ? 0 : 1;

        // To socket is always 0 (input)
        var toSocketIdx = 0;

        var edge = Graph.connect(fromNode, fromSocketIdx, toNode, toSocketIdx);
        if (edge !== "") {
            edgeCount++;
        }
    }
    console.log(`  Row ${row}: ${cols - 1} connections`);
}

// Show final stats
var stats = Graph.getStats();
console.log("\nFinal graph:");
console.log("  Nodes:", stats.nodes, `(expected: ${rows * cols})`);
console.log("  Edges:", stats.edges, `(expected: ${edgeCount})`);
console.log("  Density:", (stats.edges / stats.nodes).toFixed(2), "edges per node");

Graph.saveXml("grid_pattern.xml");
console.log("\n[OK] Graph saved to grid_pattern.xml");

console.log("----------------------------------------------------");
