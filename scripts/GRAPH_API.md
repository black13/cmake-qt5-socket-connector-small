# NodeGraph JavaScript API Reference

Complete API reference for creating and manipulating graphs via JavaScript.

## Overview

The `Graph` object provides a complete API for programmatic graph creation, inspection, and manipulation. All methods are exposed from the `QGraph` C++ class via Qt's meta-object system.

## Node Operations

### `Graph.createNode(type, x, y)`
Create a new node at the specified position.

**Parameters:**
- `type` (String): Node type ("SOURCE", "SINK", "TRANSFORM", "MERGE", "SPLIT")
- `x` (Number): X coordinate in scene space
- `y` (Number): Y coordinate in scene space

**Returns:** String (UUID) - The created node's ID

**Example:**
```javascript
var sourceId = Graph.createNode("SOURCE", 100, 100);
var sinkId = Graph.createNode("SINK", 300, 100);
```

---

### `Graph.deleteNode(nodeId)`
Delete a node and all its connected edges.

**Parameters:**
- `nodeId` (String): UUID of the node to delete

**Returns:** Boolean - true if deletion succeeded

**Example:**
```javascript
var nodeId = Graph.createNode("SOURCE", 100, 100);
var success = Graph.deleteNode(nodeId);
```

---

### `Graph.moveNode(nodeId, dx, dy)`
Move a node by a relative offset.

**Parameters:**
- `nodeId` (String): UUID of the node
- `dx` (Number): Horizontal offset
- `dy` (Number): Vertical offset

**Returns:** Boolean - true if move succeeded

**Example:**
```javascript
// Move node 50 pixels right, 100 pixels down
Graph.moveNode(nodeId, 50, 100);
```

---

### `Graph.getNode(nodeId)`
Get information about a specific node.

**Parameters:**
- `nodeId` (String): UUID of the node

**Returns:** Object with properties:
- `id` (String): Node UUID
- `type` (String): Node type
- `x` (Number): X coordinate
- `y` (Number): Y coordinate
- `socketCount` (Number): Total socket count

**Example:**
```javascript
var node = Graph.getNode(nodeId);
console.log(`Node ${node.type} at (${node.x}, ${node.y})`);
```

---

### `Graph.getNodes()`
Get all nodes in the graph.

**Returns:** Array of node objects (same structure as `getNode()`)

**Example:**
```javascript
var nodes = Graph.getNodes();
for (var i = 0; i < nodes.length; i++) {
    console.log(`Node ${i}: ${nodes[i].type} at (${nodes[i].x}, ${nodes[i].y})`);
}
```

---

## Edge Operations

### `Graph.connect(fromNodeId, fromSocketIdx, toNodeId, toSocketIdx)`
Create an edge connecting two sockets.

**Parameters:**
- `fromNodeId` (String): Source node UUID
- `fromSocketIdx` (Number): Source socket index
- `toNodeId` (String): Target node UUID
- `toSocketIdx` (Number): Target socket index

**Returns:** String (UUID) - The created edge's ID, or empty string on failure

**Socket Index Rules:**
- Sockets are indexed sequentially: inputs first (0,1,2...), then outputs
- SOURCE (0 inputs, 1 output): socket 0 = OUTPUT
- SINK (1 input, 0 outputs): socket 0 = INPUT
- TRANSFORM (1 input, 1 output): socket 0 = INPUT, socket 1 = OUTPUT
- MERGE (2 inputs, 1 output): sockets 0,1 = INPUTS, socket 2 = OUTPUT
- SPLIT (1 input, 2 outputs): socket 0 = INPUT, sockets 1,2 = OUTPUTS

**Example:**
```javascript
var source = Graph.createNode("SOURCE", 100, 100);
var sink = Graph.createNode("SINK", 300, 100);

// Connect SOURCE output (socket 0) to SINK input (socket 0)
var edgeId = Graph.connect(source, 0, sink, 0);
```

---

### `Graph.deleteEdge(edgeId)`
Delete an edge.

**Parameters:**
- `edgeId` (String): UUID of the edge to delete

**Returns:** Boolean - true if deletion succeeded

**Example:**
```javascript
Graph.deleteEdge(edgeId);
```

---

### `Graph.getEdges()`
Get all edges in the graph.

**Returns:** Array of edge objects with properties:
- `id` (String): Edge UUID
- `fromNode` (String): Source node UUID
- `toNode` (String): Target node UUID
- `fromSocket` (Number): Source socket index
- `toSocket` (Number): Target socket index

**Example:**
```javascript
var edges = Graph.getEdges();
for (var i = 0; i < edges.length; i++) {
    var e = edges[i];
    console.log(`Edge: ${e.fromNode}[${e.fromSocket}] → ${e.toNode}[${e.toSocket}]`);
}
```

---

## Graph-Wide Operations

### `Graph.clear()`
Delete all nodes and edges.

**Example:**
```javascript
Graph.clear();
console.log("Graph cleared");
```

---

### `Graph.deleteSelected()`
Delete all currently selected items.

**Returns:** Boolean - true if deletion succeeded

**Example:**
```javascript
Graph.deleteSelected();
```

---

### `Graph.getStats()`
Get graph statistics.

**Returns:** Object with properties:
- `nodes` (Number): Node count
- `edges` (Number): Edge count

**Example:**
```javascript
var stats = Graph.getStats();
console.log(`Graph has ${stats.nodes} nodes and ${stats.edges} edges`);
```

---

## XML Persistence

### `Graph.saveXml(path)`
Save graph to XML file.

**Parameters:**
- `path` (String): File path (relative to executable or absolute)

**Example:**
```javascript
Graph.saveXml("my_graph.xml");
```

---

### `Graph.loadXml(path)`
Load graph from XML file.

**Parameters:**
- `path` (String): File path to load

**Example:**
```javascript
Graph.loadXml("my_graph.xml");
```

---

### `Graph.getXmlString()`
Get graph as XML string.

**Returns:** String - XML representation of the graph

**Example:**
```javascript
var xml = Graph.getXmlString();
console.log("Graph XML:", xml);
```

---

## Load State Tracking

### `Graph.isLoadingXml()`
Check if XML load is in progress.

**Returns:** Boolean - true if currently loading XML

---

### `Graph.isStable()`
Check if graph is stable (not loading, no unresolved edges).

**Returns:** Boolean - true if graph is stable

---

### `Graph.getUnresolvedEdgeCount()`
Get count of edges that haven't resolved their socket connections.

**Returns:** Number - Count of unresolved edges

---

## Utility Functions

### `Graph.isValidNodeType(type)`
Check if a node type is valid.

**Parameters:**
- `type` (String): Node type to check

**Returns:** Boolean - true if type is valid

**Example:**
```javascript
if (Graph.isValidNodeType("SOURCE")) {
    console.log("SOURCE is a valid node type");
}
```

---

### `Graph.getValidNodeTypes()`
Get list of all valid node types.

**Returns:** Array of strings

**Example:**
```javascript
var types = Graph.getValidNodeTypes();
console.log("Available node types:", types.join(", "));
// Output: "Available node types: SOURCE, SINK, TRANSFORM, MERGE, SPLIT"
```

---

## Console API

### `console.log(message)`
Print message to application console.

**Example:**
```javascript
console.log("Hello from JavaScript!");
```

---

### `console.error(message)`
Print error message to application console.

**Example:**
```javascript
console.error("Something went wrong!");
```

---

## Complete Examples

### Example 1: Simple Pipeline
```javascript
// Create a simple source → transform → sink pipeline
Graph.clear();

var source = Graph.createNode("SOURCE", 100, 100);
var transform = Graph.createNode("TRANSFORM", 250, 100);
var sink = Graph.createNode("SINK", 400, 100);

Graph.connect(source, 0, transform, 0);  // SOURCE output → TRANSFORM input
Graph.connect(transform, 1, sink, 0);     // TRANSFORM output → SINK input

Graph.saveXml("pipeline.xml");
console.log("Pipeline created and saved!");
```

---

### Example 2: Split Workflow
```javascript
// Create a source that splits to multiple sinks
Graph.clear();

var source = Graph.createNode("SOURCE", 100, 100);
var splitter = Graph.createNode("SPLIT", 250, 100);
var sink1 = Graph.createNode("SINK", 400, 50);
var sink2 = Graph.createNode("SINK", 400, 150);

Graph.connect(source, 0, splitter, 0);     // SOURCE → SPLIT input
Graph.connect(splitter, 1, sink1, 0);       // SPLIT output 1 → SINK 1
Graph.connect(splitter, 2, sink2, 0);       // SPLIT output 2 → SINK 2

console.log("Split workflow created!");
```

---

### Example 3: Merge Workflow
```javascript
// Create multiple sources that merge to one sink
Graph.clear();

var source1 = Graph.createNode("SOURCE", 100, 50);
var source2 = Graph.createNode("SOURCE", 100, 150);
var merger = Graph.createNode("MERGE", 250, 100);
var sink = Graph.createNode("SINK", 400, 100);

Graph.connect(source1, 0, merger, 0);      // SOURCE 1 → MERGE input 1
Graph.connect(source2, 0, merger, 1);      // SOURCE 2 → MERGE input 2
Graph.connect(merger, 2, sink, 0);          // MERGE output → SINK

console.log("Merge workflow created!");
```

---

### Example 4: Dynamic Graph Creation
```javascript
// Create a grid of nodes programmatically
Graph.clear();

var gridSize = 3;
var spacing = 150;
var nodes = [];

// Create grid
for (var row = 0; row < gridSize; row++) {
    nodes[row] = [];
    for (var col = 0; col < gridSize; col++) {
        var x = 100 + col * spacing;
        var y = 100 + row * spacing;
        var type = (col === 0) ? "SOURCE" :
                   (col === gridSize - 1) ? "SINK" : "TRANSFORM";

        nodes[row][col] = Graph.createNode(type, x, y);
    }
}

// Connect grid horizontally
for (var row = 0; row < gridSize; row++) {
    for (var col = 0; col < gridSize - 1; col++) {
        var fromIdx = (col === 0) ? 0 : 1;  // SOURCE uses 0, TRANSFORM uses 1
        Graph.connect(nodes[row][col], fromIdx, nodes[row][col + 1], 0);
    }
}

console.log(`Created ${gridSize}x${gridSize} grid with ${gridSize * (gridSize-1)} connections`);
```

---

### Example 5: Graph Analysis
```javascript
// Analyze graph structure
function analyzeGraph() {
    var stats = Graph.getStats();
    console.log("=== Graph Analysis ===");
    console.log(`Nodes: ${stats.nodes}`);
    console.log(`Edges: ${stats.edges}`);

    // Count node types
    var nodes = Graph.getNodes();
    var typeCounts = {};

    for (var i = 0; i < nodes.length; i++) {
        var type = nodes[i].type;
        typeCounts[type] = (typeCounts[type] || 0) + 1;
    }

    console.log("Node type distribution:");
    for (var type in typeCounts) {
        console.log(`  ${type}: ${typeCounts[type]}`);
    }

    // Analyze connectivity
    var edges = Graph.getEdges();
    var avgConnections = edges.length > 0 ? (edges.length / nodes.length).toFixed(2) : 0;
    console.log(`Average connections per node: ${avgConnections}`);
}

analyzeGraph();
```

---

## Error Handling

All operations that can fail return `false` or empty string. Always check return values:

```javascript
var nodeId = Graph.createNode("INVALID_TYPE", 100, 100);
if (nodeId === "") {
    console.error("Failed to create node - invalid type");
}

var success = Graph.deleteNode("invalid-uuid");
if (!success) {
    console.error("Failed to delete node - not found");
}

var edgeId = Graph.connect(sourceId, 99, sinkId, 0);  // Invalid socket index
if (edgeId === "") {
    console.error("Failed to create edge - invalid socket");
}
```

---

## Best Practices

1. **Check node types before creating:**
   ```javascript
   if (Graph.isValidNodeType(nodeType)) {
       Graph.createNode(nodeType, x, y);
   }
   ```

2. **Verify connections succeeded:**
   ```javascript
   var edgeId = Graph.connect(from, 0, to, 0);
   if (edgeId !== "") {
       console.log("Connection created:", edgeId);
   }
   ```

3. **Clear graph before creating new:**
   ```javascript
   Graph.clear();
   // Now create fresh graph
   ```

4. **Save your work:**
   ```javascript
   Graph.saveXml("autosave.xml");
   ```

5. **Use console.log for debugging:**
   ```javascript
   console.log("Created node:", nodeId);
   console.log("Stats:", JSON.stringify(Graph.getStats()));
   ```
