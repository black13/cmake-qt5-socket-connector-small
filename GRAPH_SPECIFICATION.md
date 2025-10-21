# NodeGraph Specification v1.0

**Last Updated:** October 2025

## Graph Model Definition

### Core Elements

| Element | Mandatory Attributes | Optional Attributes | Invariants |
|---------|---------------------|-------------------|------------|
| **Graph** | `version="1.0"` | none | Only one per XML file |
| **Node** | `id` (UUID, unique)<br>`type` (string)<br>`inputs` (int ≥ 0)<br>`outputs` (int ≥ 0)<br>`x,y` (coordinates) | none | `inputs ≥ 1 ∨ outputs ≥ 1` |
| **Socket** | Implicit (index 0…inputs-1 / outputs-1) | none | Index stable for node lifetime |
| **Edge** | `id` (UUID)<br>`fromNode` (UUID)<br>`fromSocketIndex` (int)<br>`toNode` (UUID)<br>`toSocketIndex` (int) | none | Connects output → input<br>Self-loops allowed<br>Multi-edges allowed |

### Graph Management

The **Scene** class serves as the graph manager and implements the `GraphSubject` interface:
- Maintains `QHash<QUuid, Node*> m_nodes` for O(1) node lookup
- Maintains `QHash<QUuid, Edge*> m_edges` for O(1) edge lookup
- Notifies observers of all graph mutations
- Manages QGraphicsScene for visual representation

### Canonical Operations & Invariants

| Operation | Post-conditions |
|-----------|----------------|
| `addNode(n)` | Scene gains node; observers get `onNodeAdded(n)` |
| `removeNode(id)` | All incident edges removed first; observers receive `onEdgeRemoved`, then `onNodeRemoved` |
| `addEdge(e)` | Both nodes exist; socket indices valid; observers get `onEdgeAdded(e)` |
| Node movement | Geometry changes via QGraphicsItem; observers get `onNodeMoved(id, oldPos, newPos)` |
| `clearGraph()` | All edges removed, then all nodes; observers get `onGraphCleared()` |

## Observer Pattern Architecture

### Core Interfaces

```cpp
// Observer interface
class GraphObserver {
public:
    virtual ~GraphObserver() = default;

    // Node lifecycle events
    virtual void onNodeAdded(const Node& node) {}
    virtual void onNodeRemoved(const QUuid& nodeId) {}
    virtual void onNodeMoved(const QUuid& nodeId, QPointF oldPos, QPointF newPos) {}

    // Edge lifecycle events
    virtual void onEdgeAdded(const Edge& edge) {}
    virtual void onEdgeRemoved(const QUuid& edgeId) {}

    // Graph-level events
    virtual void onGraphCleared() {}
    virtual void onGraphLoaded(const QString& filename) {}
    virtual void onGraphSaved(const QString& filename) {}
};

// Subject interface
class GraphSubject {
public:
    void attach(GraphObserver* observer);
    void detach(GraphObserver* observer);

    // Batch mode for bulk operations (prevents observer storm)
    static void beginBatch();
    static void endBatch();
    static bool isInBatch();

protected:
    void notifyNodeAdded(const Node& node);
    void notifyNodeRemoved(const QUuid& nodeId);
    void notifyNodeMoved(const QUuid& nodeId, QPointF oldPos, QPointF newPos);
    void notifyEdgeAdded(const Edge& edge);
    void notifyEdgeRemoved(const QUuid& edgeId);
    void notifyGraphCleared();
    void notifyGraphLoaded(const QString& filename);
    void notifyGraphSaved(const QString& filename);
};
```

### Observer Types & Purposes

| Observer | Purpose | Status |
|----------|---------|--------|
| **XmlAutosaveObserver** | Listens to every mutation, writes autosave.xml | ✅ Implemented |
| **ValidationObserver** | Updates errors/warnings overlay | ❌ Not implemented |
| **CommandHistoryObserver** | Records operations for undo/redo | ❌ Not implemented |
| **RuntimeExecutionObserver** | Maps node IDs to runtime objects | ❌ Not implemented |

## XML Format Standard

### Actual Format (Current Implementation)

```xml
<?xml version="1.0" encoding="UTF-8"?>
<graph version="1.0">
  <node id="{uuid1}" type="SOURCE" x="100" y="100" inputs="1" outputs="1"/>
  <node id="{uuid2}" type="TRANSFORM" x="300" y="100" inputs="1" outputs="1"/>
  <node id="{uuid3}" type="SINK" x="500" y="100" inputs="1" outputs="1"/>

  <edge id="{edge-uuid1}"
        fromNode="{uuid1}" fromSocketIndex="0"
        toNode="{uuid2}" toSocketIndex="0"/>
  <edge id="{edge-uuid2}"
        fromNode="{uuid2}" fromSocketIndex="0"
        toNode="{uuid3}" toSocketIndex="0"/>
</graph>
```

**Key Points:**
- Flat structure: `<node>` and `<edge>` elements directly under `<graph>`
- No `<nodes>` or `<connections>` wrapper elements
- No metadata section (can be added later if needed)
- All attributes inline, no nested elements

### Node Types (From Templates)

Node types are defined in `node_type_templates.xml`. Common types include:

| Type | Description | Typical Use |
|------|-------------|-------------|
| `SOURCE` | Input/Source node | Data generators, file readers |
| `SINK` | Output/Sink node | Data consumers, file writers |
| `TRANSFORM` | Processor node | Data transformation |
| `SPLIT` | Fanout node | Duplicate data to multiple outputs |
| `MERGE` | Join node | Combine multiple inputs |

**Note:** Socket counts are per-instance configuration, not type-specific.

## Implementation Guidelines

### Socket Index Management
- Sockets are identified by `(nodeId, socketIndex, direction)` tuples
- Input sockets: indices `0` to `inputs-1`
- Output sockets: indices `0` to `outputs-1`
- Indices must be stable for node lifetime
- Socket recreation requires edge cleanup first

### Edge Validation Rules
1. `fromNode` and `toNode` must exist in graph
2. `fromSocketIndex` must be `< fromNode.outputs`
3. `toSocketIndex` must be `< toNode.inputs`
4. Self-loops are allowed: `fromNode == toNode`
5. Multi-edges are allowed: multiple edges between same socket pair
6. Edges connect output sockets to input sockets (direction enforced)

### Data Integrity Guarantees
1. **XML-First**: All objects created through `GraphFactory::createFromXml()`
2. **Observer Consistency**: All mutations trigger appropriate notifications
3. **Reference Integrity**: Edges automatically deleted when nodes removed
4. **UUID Uniqueness**: All IDs globally unique within graph
5. **Socket Stability**: Socket indices never change during node lifetime
6. **Typed Collections**: `QHash<QUuid, Node*>` and `QHash<QUuid, Edge*>` - no casting needed

### Self-Serialization Pattern

All graph objects implement XML self-serialization:

```cpp
class Node {
    void writeToXml(xmlTextWriterPtr writer) const;
    static Node* createFromXml(xmlNodePtr xmlNode, Scene* scene);
};

class Edge {
    void writeToXml(xmlTextWriterPtr writer) const;
    static Edge* createFromXml(xmlNodePtr xmlNode, Scene* scene);
};
```

This ensures XML format consistency and simplifies persistence.

## Architecture Principles

### No Type Casting
- Scene maintains typed collections (`QHash<QUuid, Node*>`, not `QGraphicsItem*`)
- No `qgraphicsitem_cast<>` or `dynamic_cast<>` needed
- Type safety enforced at collection level

### Object Lifecycle Management
- Nodes and edges are **NOT** QObject-derived (avoid zombie reference issues)
- QGraphicsScene owns items via Qt's parent-child system
- UUID-based lookup for graph topology queries
- Qt scene graph handles rendering and interaction

### Observer Pattern Benefits
- Decouples graph logic from persistence (autosave)
- Enables future undo/redo without core changes
- Allows runtime execution layer without graph pollution
- Prevents callback spaghetti through well-defined events

## Files in Source Root

Current implementation files:
- **Core**: `node.{h,cpp}`, `socket.{h,cpp}`, `edge.{h,cpp}`
- **Graph Management**: `scene.{h,cpp}`, `graph_factory.{h,cpp}`
- **Observers**: `graph_observer.{h,cpp}`, `xml_autosave_observer.{h,cpp}`
- **UI**: `window.{h,cpp}`, `view.{h,cpp}`, `node_palette_widget.{h,cpp}`
- **Templates**: `node_templates.{h,cpp}`
- **Build**: `CMakelists.txt`, `build.sh`, `build.bat`
- **Spec**: `GRAPH_SPECIFICATION.md` (this file)

## Known Limitations

1. **No Undo/Redo**: Observer framework exists but CommandHistoryObserver not implemented
2. **No Validation Layer**: No cycle detection or type checking observers
3. **No Runtime Execution**: No node execution/computation framework yet
4. **No Metadata**: XML format doesn't preserve creation time, version info, etc.
5. **No Socket Metadata**: Sockets are just indices, no names/types/descriptions

## Future Considerations

### Graph Interface Design (In Flux)
The public API for graph manipulation is currently being redesigned to support:
- Potential JavaScript scripting integration
- Node/edge behavior customization via scripts
- External graph algorithms and analysis tools

**Status:** Interface design in active development. Current `Scene` methods may change.

### Scripting Layer
JavaScript integration files exist (`graph_script_api.*`, `script_host.*`) but are:
- Not compiled into current build (JS engine disabled)
- Kept as reference implementations
- May be reactivated when graph interface stabilizes

---

*This specification reflects the current implementation as of October 2025. The graph interface design is actively evolving.*
