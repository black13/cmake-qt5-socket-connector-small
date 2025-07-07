# NodeGraph Specification v1.0

## Graph Model Definition

### Core Elements

| Element | Mandatory Attributes | Optional Attributes | Invariants |
|---------|---------------------|-------------------|------------|
| **Graph** | `version="1.0"` | `label`, metadata | Only one per XML file |
| **Node** | `id` (UUID, unique)<br>`type` (string)<br>`inputs` (int ≥ 0)<br>`outputs` (int ≥ 0)<br>`x,y` (coordinates) | Arbitrary key-value pairs | `inputs ≥ 1 ∨ outputs ≥ 1` |
| **Socket** | Implicit (index 0…inputs-1 / outputs-1) | Metadata in future | Index stable for node lifetime |
| **Edge** | `id` (UUID)<br>`fromNode` (UUID)<br>`fromSocketIndex` (int)<br>`toNode` (UUID)<br>`toSocketIndex` (int) | `label`, `weight`, etc. | Connects output → input<br>Self-loops allowed<br>Multi-edges allowed |

### Canonical Operations & Invariants

| Operation | Post-conditions |
|-----------|----------------|
| `addNode(n)` | `graph.nodes` gains `n`; observers get `onNodeAdded(n)` |
| `removeNode(id)` | All incident edges removed first; observers receive `onEdgeRemoved`, then `onNodeRemoved` |
| `addEdge(e)` | Both nodes exist; `fromSocketIndex < outputs`, `toSocketIndex < inputs`; observers get `onEdgeAdded(e)` |
| `moveNode(id, x, y)` | Geometry changes but topology intact; observers get `onNodeMoved(id, oldPos, newPos)` |

## Observer Pattern Architecture

### Core Interfaces

```cpp
// Subject interface
class GraphSubject {
public:
    void attach(GraphObserver* observer);
    void detach(GraphObserver* observer);
    
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

// Observer interface
class GraphObserver {
public:
    virtual ~GraphObserver() = default;
    virtual void onNodeAdded(const Node& node) {}
    virtual void onNodeRemoved(const QUuid& nodeId) {}
    virtual void onNodeMoved(const QUuid& nodeId, QPointF oldPos, QPointF newPos) {}
    virtual void onEdgeAdded(const Edge& edge) {}
    virtual void onEdgeRemoved(const QUuid& edgeId) {}
    virtual void onGraphCleared() {}
    virtual void onGraphLoaded(const QString& filename) {}
    virtual void onGraphSaved(const QString& filename) {}
};
```

### Observer Types & Purposes

| Observer | Purpose |
|----------|---------|
| **XmlAutosaveObserver** | Listens to every mutation, writes incremental updates or full export |
| **ValidationObserver** | Updates "errors/warnings" overlay when graph becomes cyclic, sockets mismatch, etc. |
| **CommandHistoryObserver** | Records operations for undo/redo stack |
| **RuntimeExecutionObserver** | Maps node IDs to live runtime objects, triggers re-compute when inputs change |

## XML Format Standard

### Basic Structure
```xml
<?xml version="1.0" encoding="UTF-8"?>
<graph version="1.0">
  <metadata>
    <generator>NodeGraph v1.0</generator>
    <timestamp>1625097600</timestamp>
    <node_count>3</node_count>
    <edge_count>2</edge_count>
  </metadata>
  
  <nodes>
    <node id="{uuid1}" type="IN" x="100" y="100" inputs="0" outputs="2"/>
    <node id="{uuid2}" type="PROC" x="300" y="100" inputs="2" outputs="2"/>
    <node id="{uuid3}" type="OUT" x="500" y="100" inputs="2" outputs="0"/>
  </nodes>
  
  <connections>
    <connection id="{edge-uuid1}" 
                from="{uuid1}" from-socket="0"
                to="{uuid2}" to-socket="0"/>
    <connection id="{edge-uuid2}"
                from="{uuid2}" from-socket="1" 
                to="{uuid3}" to-socket="0"/>
  </connections>
</graph>
```

### Node Types

| Type | Description | Typical Socket Config |
|------|-------------|---------------------|
| `IN` | Input/Source node | `inputs="0" outputs="1+"` |
| `OUT` | Output/Sink node | `inputs="1+" outputs="0"` |
| `PROC` | Processor node | `inputs="1+" outputs="1+"` |
| `HUB` | Hub/Fanout node | `inputs="1" outputs="3+"` |
| `LEAF` | Leaf node | `inputs="1" outputs="1"` |

## Implementation Guidelines

### Socket Index Management
- Sockets are identified by `(nodeId, socketIndex)` pairs
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

### Data Integrity Guarantees
1. **XML-First**: All objects created through XML parsing
2. **Observer Consistency**: All mutations trigger appropriate notifications
3. **Reference Integrity**: Edges automatically deleted when nodes removed
4. **UUID Uniqueness**: All IDs globally unique within graph
5. **Socket Stability**: Socket indices never change during node lifetime

## Test Generator Usage

Generate test graphs using the unified generator:

```bash
# Basic graphs
python nodegraph_gen.py basic simple --output test_simple.xml
python nodegraph_gen.py basic grid --rows 3 --cols 4
python nodegraph_gen.py basic circle --nodes 6

# Topology graphs  
python nodegraph_gen.py topology chain --nodes 5
python nodegraph_gen.py topology star --nodes 7

# NetworkX graphs (if available)
python nodegraph_gen.py networkx erdos-renyi --nodes 10 --probability 0.3
python nodegraph_gen.py networkx barabasi-albert --nodes 15 --m 3

# Help and options
python nodegraph_gen.py --help
```

## Files in Source Root

All components live in single directory:
- **C++ Sources**: `*.h`, `*.cpp` 
- **XML Fixtures**: `*.xml` (generated test graphs)
- **Generator**: `nodegraph_gen.py` (unified graph generator)
- **Documentation**: `GRAPH_SPECIFICATION.md` (this file)

## German Summary (Kurz & Knapp)

**Graph-Definition**: Ein Node hat ID, Typ, mindestens einen Ein- oder Ausgang, Koordinaten. Ein Edge verbindet genau einen Ausgang (Index) mit einem Eingang (Index). Mehrfachkanten und Selbstschleifen erlaubt.

**Observer-Schicht**: GraphSubject verwaltet Observer-Liste und ruft bei jeder Änderung entsprechende `notify*()` Methoden. Beispiele: XmlAutosaveObserver, ValidationObserver, UndoStackObserver.

**Datenintegrität**: XML-First, Observer-Konsistenz, referentielle Integrität, UUID-Eindeutigkeit, Socket-Stabilität.