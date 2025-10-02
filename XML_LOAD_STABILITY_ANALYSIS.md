# XML Load Stability Analysis - JavaScript Integration

## Critical Finding: QGraph.loadXml() NOT IMPLEMENTED

**Line 273-278 in qgraph.cpp:**
```cpp
void QGraph::loadXml(const QString& path)
{
    // TODO: Implement XML loading
    // This will coordinate with Scene to deserialize the graph
    emit error("QGraph::loadXml not yet implemented");
}
```

**Status**: JavaScript layer has NO WAY to load XML files through QGraph API.

---

## Current XML Loading Flow (GraphFactory only)

```
main.cpp → GraphFactory::loadFromXmlFile()
    ↓
GraphSubject::beginBatch()  ← CRITICAL: Suppresses observer notifications
    ↓
Parse XML file (libxml2)
    ↓
PHASE 1: Load ALL nodes first
    ↓
PHASE 2: Load ALL edges (unresolved)
    ↓
PHASE 3: Resolve edge connections
    ↓
GraphSubject::endBatch()  ← Resume notifications, emit one batch notification
```

---

## Instability Points During XML Load

### 1. **Observer Storm** (MITIGATED in GraphFactory)
**Problem**: Without batching, each node/edge creation triggers:
- `notifyNodeAdded()` → All observers fire
- `notifyEdgeAdded()` → All observers fire
- For 100 nodes + 200 edges = 300 individual notifications

**Current Mitigation**:
```cpp
GraphSubject::beginBatch();  // Suppress notifications
// ... load nodes and edges ...
GraphSubject::endBatch();    // One batch notification
```

**JavaScript Impact**:
- ✓ GraphFactory is safe (uses batching)
- ✗ QGraph.loadXml() doesn't exist, so NO BATCHING when implemented

### 2. **Partial Graph State**
**Problem**: JavaScript queries during load see incomplete graph:
```javascript
// JavaScript executing during XML load
let nodes = Graph.getNodes();  // Only 50% loaded
let edges = Graph.getEdges();  // No edges resolved yet
```

**Danger Zones**:
- After PHASE 1 (nodes loaded) but before PHASE 2 (edges loaded)
- After PHASE 2 (edges created) but before PHASE 3 (connections resolved)
- While scene is updating visual representation

### 3. **Dangling Edge References**
**Problem**: Edges reference nodes by UUID. If JavaScript tries to:
- Query edge connections before resolution → null pointers
- Create new edges during load → race condition with XML edges
- Delete nodes during load → corrupts edge resolution

**GraphFactory Protection** (graph_factory.cpp:380-392):
```cpp
// PHASE 3: Resolve all edge connections now that all nodes exist
for (Edge* edge : allEdges) {
    if (edge->resolveConnections(typedScene)) {
        successfulConnections++;
    }
}
```

### 4. **Graphics Rendering Race**
**Problem**: Scene adds items to QGraphicsView during load:
- Each `scene_->addNode()` triggers BSP tree update
- Each `scene_->addEdge()` triggers path calculation
- Each `edge->updatePath()` queries socket positions

**Cascade**:
```
addNode() → Qt BSP update → View redraw request
    → addEdge() → updatePath() → Socket lookup
        → Node lookup → BSP query → RACE CONDITION
```

---

## JavaScript "Pulse & Respirate" Monitoring Strategy

### What JavaScript Should Monitor

#### 1. **Graph State Flags**
```javascript
// JavaScript health check
function checkGraphHealth() {
    let stats = Graph.getStats();

    // Check for inconsistencies
    if (stats.edgesUnresolved > 0) {
        console.warn("Graph unstable: unresolved edges");
        return "UNSTABLE";
    }

    if (stats.danglingReferences > 0) {
        console.error("Graph corrupt: dangling references");
        return "CORRUPT";
    }

    return "STABLE";
}
```

#### 2. **Load State Tracking**
```cpp
// QGraph should expose:
Q_INVOKABLE bool isLoadingXml() const;  // During XML load
Q_INVOKABLE bool isStable() const;      // All edges resolved
Q_INVOKABLE int getUnresolvedEdgeCount() const;
```

```javascript
// JavaScript monitoring
setInterval(function() {
    if (Graph.isLoadingXml()) {
        console.log("⏳ XML load in progress - pausing operations");
        return;  // Don't execute during load
    }

    if (!Graph.isStable()) {
        console.warn("⚠ Graph unstable - waiting for resolution");
        return;
    }

    // Safe to execute now
    runGraphAnalytics();
}, 100);  // Check every 100ms
```

#### 3. **Signal-Based Coordination**
```cpp
// QGraph signals JavaScript should connect to:
signals:
    void xmlLoadStarted(QString path);
    void xmlLoadProgress(int nodesLoaded, int edgesLoaded);
    void xmlLoadComplete(QString path, bool success);
    void graphStabilized();  // All edges resolved
```

```javascript
// JavaScript coordination
Graph.xmlLoadStarted.connect(function(path) {
    console.log("🔄 Loading", path, "- suspending operations");
    suspendGraphOperations();
});

Graph.xmlLoadComplete.connect(function(path, success) {
    if (success) {
        console.log("✓ Load complete - validating graph");
        validateGraphIntegrity();
        resumeGraphOperations();
    }
});
```

---

## Critical Missing Pieces in QGraph

### 1. **QGraph::loadXml() Implementation**
Must coordinate with GraphFactory and implement batching:

```cpp
void QGraph::loadXml(const QString& path)
{
    if (!scene_) {
        emit error("QGraph: Scene not initialized");
        return;
    }

    emit xmlLoadStarted(path);

    // Clear existing graph first
    scene_->clearGraphInternal();

    // Use GraphFactory for proper phased loading + batching
    GraphFactory factory(scene_, /* need xmlDoc */);

    // CRITICAL: GraphFactory::loadFromXmlFile() handles batching
    bool success = factory.loadFromXmlFile(path);

    if (success) {
        emit xmlLoadComplete(path, true);
        emit graphStabilized();
    } else {
        emit xmlLoadComplete(path, false);
        emit error("Failed to load XML");
    }
}
```

### 2. **Load State Tracking**
```cpp
// Add to QGraph private members:
bool m_isLoadingXml = false;
int m_unresolvedEdges = 0;

// Public accessors:
Q_INVOKABLE bool isLoadingXml() const { return m_isLoadingXml; }
Q_INVOKABLE bool isStable() const { return m_unresolvedEdges == 0; }
```

### 3. **Batch Coordination**
GraphFactory uses `GraphSubject::beginBatch()` but QGraph doesn't expose it:

```cpp
// Option 1: QGraph forwards to Scene
void QGraph::beginBatchOperations() {
    if (scene_) scene_->beginBatch();
}

void QGraph::endBatchOperations() {
    if (scene_) scene_->endBatch();
}

// Option 2: QGraph wraps bulk operations automatically
QStringList QGraph::createMultipleNodes(QVariantList nodeSpecs) {
    scene_->beginBatch();
    QStringList ids;
    for (const QVariant& spec : nodeSpecs) {
        ids.append(createNode(...));
    }
    scene_->endBatch();
    return ids;
}
```

---

## JavaScript Best Practices for Stability

### DO:
✓ Check `Graph.isLoadingXml()` before operations
✓ Connect to `xmlLoadComplete` signal before loading
✓ Validate graph state with `Graph.getStats()`
✓ Use batch operations for bulk changes
✓ Wait for `graphStabilized()` signal after mutations

### DON'T:
✗ Query graph during `xmlLoadStarted` → `xmlLoadComplete` window
✗ Create nodes/edges while `isLoadingXml() == true`
✗ Assume edges are connected immediately after `Graph.connect()`
✗ Delete nodes without checking incident edges first
✗ Run analytics before `graphStabilized()` signal

---

## Proposed JavaScript "Health Monitor" API

```javascript
// Expose on Graph object
Graph.Health = {
    // State queries
    isLoading: function() { return Graph.isLoadingXml(); },
    isStable: function() { return Graph.isStable(); },
    getUnresolvedEdges: function() { return Graph.getUnresolvedEdgeCount(); },

    // Validation
    validateIntegrity: function() {
        let issues = [];
        let stats = Graph.getStats();

        // Check for orphaned edges
        let edges = Graph.getEdges();
        edges.forEach(function(edge) {
            if (!edge.fromNode || !edge.toNode) {
                issues.push("Orphaned edge: " + edge.id);
            }
        });

        // Check for dangling sockets
        let nodes = Graph.getNodes();
        nodes.forEach(function(node) {
            if (node.socketCount < 0) {
                issues.push("Invalid socket count on node: " + node.id);
            }
        });

        return {
            valid: issues.length === 0,
            issues: issues,
            stats: stats
        };
    },

    // Monitoring
    startMonitoring: function(intervalMs) {
        setInterval(function() {
            let health = Graph.Health.validateIntegrity();
            if (!health.valid) {
                console.error("🚨 Graph integrity issues:", health.issues);
            }
        }, intervalMs || 1000);
    }
};
```

---

## Recommended Implementation Order

1. **Implement QGraph::loadXml()** with proper batching
2. **Add load state tracking** (isLoadingXml, isStable flags)
3. **Add progress signals** (xmlLoadStarted, xmlLoadComplete, graphStabilized)
4. **Expose batch operations** to JavaScript
5. **Implement health monitoring** API in JavaScript
6. **Add validation methods** to QGraph (integrity checks)
7. **Document safe operation windows** for JavaScript developers

---

## Example: Safe JavaScript Graph Operations

```javascript
// SAFE: Wait for stable state
function safeGraphOperation() {
    if (Graph.isLoadingXml()) {
        console.log("Deferring operation until load complete");
        Graph.xmlLoadComplete.connect(function retry() {
            Graph.xmlLoadComplete.disconnect(retry);
            safeGraphOperation();  // Retry after load
        });
        return;
    }

    if (!Graph.isStable()) {
        console.log("Waiting for graph to stabilize");
        setTimeout(safeGraphOperation, 100);  // Retry in 100ms
        return;
    }

    // NOW SAFE TO OPERATE
    let nodes = Graph.getNodes();
    console.log("Operating on", nodes.length, "nodes");
}

// UNSAFE: No state checks
function unsafeGraphOperation() {
    let nodes = Graph.getNodes();  // May be partial during load!
    // ... operations may see inconsistent state
}
```

---

## Conclusion

**Current Status**:
- ✗ QGraph.loadXml() not implemented
- ✓ GraphFactory has proper batching
- ✗ No load state tracking exposed to JavaScript
- ✗ No signals for load coordination

**Risk Level**: **HIGH** - JavaScript has no way to know when graph is loading or stable

**Priority Actions**:
1. Implement QGraph::loadXml() with GraphFactory delegation
2. Add isLoadingXml() and isStable() state checks
3. Emit xmlLoadStarted/Complete/graphStabilized signals
4. Document safe operation patterns for JavaScript
5. Build health monitoring API

Without these, JavaScript operations during XML load will cause:
- Crashes from dangling pointers
- Incorrect analytics from partial graph state
- Race conditions between load and mutations
- Observer storms without batching
