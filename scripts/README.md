# NodeGraph JavaScript Examples

This directory contains JavaScript examples demonstrating how to programmatically create and manipulate graphs using the NodeGraph API.

## Documentation

📖 **[GRAPH_API.md](GRAPH_API.md)** - Complete API reference with all available methods and examples

## Example Scripts

### Basic Patterns

1. **[example_simple_chain.js](example_simple_chain.js)**
   - Simple linear pipeline: SOURCE → TRANSFORM → SINK
   - Perfect starting point for beginners
   - Demonstrates basic node creation and connection

2. **[example_fan_out.js](example_fan_out.js)**
   - Fan-out topology using SPLIT node
   - One source splits to multiple sinks
   - Pattern: SOURCE → SPLIT → [SINK1, SINK2]

3. **[example_fan_in.js](example_fan_in.js)**
   - Fan-in topology using MERGE node
   - Multiple sources merge to one sink
   - Pattern: [SOURCE1, SOURCE2] → MERGE → SINK

4. **[example_diamond_pattern.js](example_diamond_pattern.js)**
   - Diamond-shaped workflow
   - Combines both split and merge
   - Pattern: SOURCE → SPLIT → [TRANSFORM1, TRANSFORM2] → MERGE → SINK

### Advanced Patterns

5. **[example_grid_pattern.js](example_grid_pattern.js)**
   - Programmatic 3x3 grid creation
   - Demonstrates loops and dynamic node placement
   - Useful for understanding algorithmic graph generation

6. **[example_stress_test.js](example_stress_test.js)**
   - Large graph creation (50+ nodes)
   - Performance measurement
   - Multiple parallel chains

### Test Scripts

7. **[test_template_system.js](test_template_system.js)**
   - Verifies template system works correctly
   - Tests socket configuration
   - Validates node types

8. **[test_edge_cases.js](test_edge_cases.js)**
   - Edge case testing (empty files, invalid operations)
   - Error handling validation
   - 16 test scenarios

## Quick Start

### Running an Example

To run any example script in the NodeGraph application:

1. Launch NodeGraph
2. Go to File → Run Script (or press Ctrl+R)
3. Select one of the example files
4. Watch the graph being created!

### Creating Your Own Script

```javascript
// 1. Clear the graph
Graph.clear();

// 2. Create nodes
var source = Graph.createNode("SOURCE", 100, 100);
var sink = Graph.createNode("SINK", 300, 100);

// 3. Connect them
var edge = Graph.connect(source, 0, sink, 0);

// 4. Save your work
Graph.saveXml("my_graph.xml");

// 5. Check stats
var stats = Graph.getStats();
console.log("Created graph with", stats.nodes, "nodes");
```

## Node Types Reference

| Type | Inputs | Outputs | Description |
|------|--------|---------|-------------|
| **SOURCE** | 0 | 1 | Data source / generator |
| **SINK** | 1 | 0 | Data consumer / terminator |
| **TRANSFORM** | 1 | 1 | 1-to-1 data processor |
| **SPLIT** | 1 | 2 | 1-to-2 data splitter |
| **MERGE** | 2 | 1 | 2-to-1 data merger |

## Socket Index Reference

Sockets are indexed sequentially: **inputs first**, then **outputs**.

### Examples:

**SOURCE** (0 inputs, 1 output):
- Socket 0 = OUTPUT

**SINK** (1 input, 0 outputs):
- Socket 0 = INPUT

**TRANSFORM** (1 input, 1 output):
- Socket 0 = INPUT
- Socket 1 = OUTPUT

**MERGE** (2 inputs, 1 output):
- Socket 0 = INPUT 1
- Socket 1 = INPUT 2
- Socket 2 = OUTPUT

**SPLIT** (1 input, 2 outputs):
- Socket 0 = INPUT
- Socket 1 = OUTPUT 1
- Socket 2 = OUTPUT 2

## Common Patterns

### Sequential Chain
```javascript
var n1 = Graph.createNode("SOURCE", 100, 100);
var n2 = Graph.createNode("TRANSFORM", 300, 100);
var n3 = Graph.createNode("SINK", 500, 100);

Graph.connect(n1, 0, n2, 0);  // SOURCE output → TRANSFORM input
Graph.connect(n2, 1, n3, 0);  // TRANSFORM output → SINK input
```

### Parallel Branches
```javascript
var source = Graph.createNode("SOURCE", 100, 200);
var split = Graph.createNode("SPLIT", 300, 200);
var sink1 = Graph.createNode("SINK", 500, 100);
var sink2 = Graph.createNode("SINK", 500, 300);

Graph.connect(source, 0, split, 0);    // SOURCE → SPLIT
Graph.connect(split, 1, sink1, 0);     // SPLIT output 1 → SINK 1
Graph.connect(split, 2, sink2, 0);     // SPLIT output 2 → SINK 2
```

### Convergent Flows
```javascript
var source1 = Graph.createNode("SOURCE", 100, 100);
var source2 = Graph.createNode("SOURCE", 100, 300);
var merge = Graph.createNode("MERGE", 300, 200);
var sink = Graph.createNode("SINK", 500, 200);

Graph.connect(source1, 0, merge, 0);   // SOURCE 1 → MERGE input 1
Graph.connect(source2, 0, merge, 1);   // SOURCE 2 → MERGE input 2
Graph.connect(merge, 2, sink, 0);      // MERGE output → SINK
```

## Tips & Best Practices

1. **Always clear first**: Call `Graph.clear()` before creating new graphs
2. **Check return values**: Empty string or `false` indicates failure
3. **Use console.log**: Debug your scripts with console output
4. **Save regularly**: Use `Graph.saveXml()` to preserve your work
5. **Validate node types**: Use `Graph.isValidNodeType()` before creating
6. **Check socket indices**: Remember inputs come before outputs
7. **Test incrementally**: Build complex graphs step by step

## Error Handling

```javascript
// Always check if node creation succeeded
var nodeId = Graph.createNode("SOURCE", 100, 100);
if (nodeId === "") {
    console.error("Failed to create node!");
    return;
}

// Always check if connection succeeded
var edgeId = Graph.connect(source, 0, sink, 0);
if (edgeId === "") {
    console.error("Failed to create edge!");
    return;
}

// Verify final graph state
var stats = Graph.getStats();
if (stats.nodes < expectedNodes) {
    console.error("Some nodes were not created!");
}
```

## Debugging

Use `console.log()` liberally to understand what's happening:

```javascript
console.log("Creating SOURCE node...");
var source = Graph.createNode("SOURCE", 100, 100);
console.log("Created node with ID:", source);

var stats = Graph.getStats();
console.log("Current graph:", JSON.stringify(stats));

var nodes = Graph.getNodes();
console.log("Node count:", nodes.length);
for (var i = 0; i < nodes.length; i++) {
    console.log("  Node", i, ":", nodes[i].type, "at", nodes[i].x, nodes[i].y);
}
```

## Next Steps

1. **Read the API reference**: [GRAPH_API.md](GRAPH_API.md)
2. **Run the examples**: Start with `example_simple_chain.js`
3. **Experiment**: Modify the examples to create your own patterns
4. **Create custom scripts**: Build workflows specific to your needs
5. **Share your creations**: Save interesting graphs as XML files

## Contributing

To add new examples to this directory:

1. Follow the naming convention: `example_*.js` or `test_*.js`
2. Include a header comment explaining what the script does
3. Use clear variable names and add comments
4. Test your script before committing
5. Update this README with your new example

---

**Happy scripting!** 🚀
