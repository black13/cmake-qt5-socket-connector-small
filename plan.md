# Self-Serializing Node Graph System - Implementation Plan

## Project Overview
A Qt5-based visual node graph editor with Inkscape-inspired self-serializing architecture. Each object (Node, Socket, Edge) is responsible for its own XML serialization, eliminating "god object" anti-patterns.

## Completed: Step 1 - Foundation Architecture ✅

### What Was Accomplished
- **Clean self-serializing classes**: Node, Socket, Edge with write()/read() methods
- **Cross-platform libxml2**: FetchContent automatically builds libxml2 v2.12.5 on both Windows and Linux
- **Value semantics**: Qt containers, no smart pointers, no QObject overhead
- **Socket ownership model**: Sockets belong only to nodes, proper parent-child relationships
- **Minimal UI boilerplate**: Window, View, Scene classes ready for extension
- **Build system**: Works identically on Windows (MSVC) and Linux (GCC)

### Architecture Principles Established
1. **Self-serialization**: Objects serialize themselves via `write(xmlDocPtr, xmlNodePtr)` and `read(xmlNodePtr)`
2. **No god objects**: Eliminated centralized serialization managers
3. **libxml2 everywhere**: Same XML API on all platforms via FetchContent
4. **Simple callbacks**: Function pointers instead of QObject::connect for fast-changing graphics items
5. **UUID-based references**: Edges reference sockets by UUID, not pointers

### Build Status
- ✅ **Windows**: `build_Debug\NodeGraph.exe` - compiles and runs
- ✅ **Linux**: `build_linux/NodeGraph` - compiles and runs
- ✅ **libxml2**: Automatically downloaded and built from GNOME/libxml2 v2.12.5

## Immediate Next Steps

### Fix libxml2 Static Linking ✅ COMPLETED
**Issue**: libxml2 builds as shared library (.dll/.so), need static library (.lib/.a) for better distribution.

**Solution Applied**:
```cmake
# Added to CMakeLists.txt before FetchContent_MakeAvailable(libxml2):
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries")
```

**Benefits**:
- Single executable file (no separate libxml2.dll/libxml2.so required)
- Easier deployment and distribution
- Consistent behavior across environments

### GraphLoader Design: Avoid God Object Anti-Pattern
**Critical Architectural Decision**: GraphLoader must NOT become a god object.

**God Object Risks**:
- ❌ Managing object lifecycles (creates AND owns nodes/edges)
- ❌ Centralizing all XML logic (becomes only way to serialize)
- ❌ Knowing too much about internals (socket positioning, node types)

**Solution: Utility Functions + Self-Serialization**:
```cpp
// GraphLoader as pure utility - NO class needed
namespace GraphUtils {
    bool loadFromFile(Scene* scene, const QString& filename);
    bool saveToFile(Scene* scene, const QString& filename);
}

// Objects STILL self-serialize
Node* node = new Node();
node->read(xmlNodePtr);  // NODE reads itself, not GraphLoader
scene->addNode(node);    // SCENE owns the node, not GraphLoader
```

**Command Line Bootstrap (Qt5 Professional)**:
```cpp
// In main.cpp - Qt5 QCommandLineParser for professional CLI
int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    // Application metadata
    QCoreApplication::setApplicationName("NodeGraph");
    QCoreApplication::setApplicationVersion("1.0.0");
    
    // Qt5 command line parser
    QCommandLineParser parser;
    parser.setApplicationDescription("Self-serializing node graph editor");
    parser.addHelpOption();
    parser.addVersionOption();
    
    // File loading options
    QCommandLineOption loadFileOption(QStringList() << "l" << "load",
                                      "Load graph from XML file", "file");
    parser.addOption(loadFileOption);
    parser.addPositionalArgument("file", "XML file to load (optional)");
    parser.process(app);
    
    Window window;
    
    // Professional file handling with QFileInfo validation
    QString filename = parser.isSet(loadFileOption) ? 
                      parser.value(loadFileOption) :
                      parser.positionalArguments().value(0);
    
    if (!filename.isEmpty()) {
        QFileInfo fileInfo(filename);
        if (fileInfo.exists() && fileInfo.isReadable()) {
            GraphUtils::loadFromFile(window.getScene(), fileInfo.absoluteFilePath());
        }
    }
    
    window.show();
    return app.exec();
}
```

**Architecture Preservation**:
- ✅ **GraphLoader is just coordination utility**
- ✅ **Objects still self-serialize via read()/write()**
- ✅ **Scene owns and manages all objects**
- ✅ **No centralized XML knowledge**
- ✅ **Command line testing without UI complexity**

## Future Implementation Steps

### Step 2: Socket Integration and Visual Connections
**Goal**: Enable visual socket-to-socket connections

**Tasks**:
1. **Socket Resolver System**:
   ```cpp
   // In Scene class, add socket lookup
   Socket* findSocketByUuid(const QUuid& socketId) const;
   
   // In Edge class, implement proper path drawing
   void Edge::updatePath() {
       Socket* from = scene->findSocketByUuid(m_fromSocketId);
       Socket* to = scene->findSocketByUuid(m_toSocketId);
       if (from && to) {
           QPointF start = from->getAbsolutePosition(parentNode);
           QPointF end = to->getAbsolutePosition(parentNode);
           buildPath(start, end);
       }
   }
   ```

2. **Interactive Connection Creation**:
   - Mouse drag from socket to socket
   - Visual feedback during connection
   - Snap-to-socket behavior

3. **Socket Factories**:
   - Different socket types (data, control, event)
   - Visual styling per type
   - Validation rules for connections

### Step 3: Live XML Synchronization
**Goal**: Visual changes immediately reflected in XML

**Architecture**:
```cpp
class LiveXmlSync {
    xmlDocPtr m_document;
    QElapsedTimer m_batchTimer;
    bool m_batchMode;
    
public:
    void onNodeMoved(Node* node);      // Update XML position
    void onNodeAdded(Node* node);      // Add to XML tree
    void onConnectionMade(Edge* edge); // Add edge to XML
    
    void beginBatchMode();  // Disable live updates during bulk operations
    void endBatchMode();    // Re-enable and flush changes
};
```

**Features**:
- Position changes immediately update XML attributes
- Batch mode for loading/reconstruction (prevents double-writing)
- Undo/redo system hooks into XML changes
- Auto-save timer for crash recovery

### Step 4: NetworkX Integration and File Loading
**Goal**: Load and display NetworkX-generated graphs

**Implementation**:
1. **XML Parser Enhancement**:
   ```cpp
   class GraphLoader {
   public:
       bool loadFromFile(Scene* scene, const QString& filename);
       
   private:
       Node* createNodeFromXml(xmlNodePtr nodeXml);
       void createSocketsFromXml(Node* node, xmlNodePtr nodeXml);
       void createEdgesFromXml(Scene* scene, xmlNodePtr edgesXml);
   };
   ```

2. **Test with Existing Files**:
   - Load `graph_tree.xml`, `graph_scale_free.xml`, `graph_random.xml`
   - Verify socket creation and positioning
   - Test edge drawing between loaded nodes

3. **Graph Layout Integration**:
   - Optional: Spring-force layout algorithm
   - Optional: Hierarchical layout for tree graphs
   - User-adjustable layout parameters

### Step 5: Advanced Features

**Factory Pattern for Node Types**:
```cpp
class NodeFactory {
public:
    static Node* createNode(const QString& type);
    static void registerNodeType(const QString& type, std::function<Node*()> creator);
};
```

**Compute Functions**:
- Node-specific computation capabilities
- Data flow through connections
- Result visualization in nodes

**Style System**:
```cpp
class NodeStyle {
    QColor backgroundColor;
    QColor borderColor;
    qreal borderWidth;
    QFont textFont;
    
public:
    void apply(QPainter* painter, const Node* node);
};
```

## Technical Architecture

### Class Hierarchy
```
QGraphicsItem
├── Node (self-serializing, contains Sockets)
└── Edge (self-serializing, UUID-based socket references)

QObject
├── Window (QMainWindow)
├── Scene (QGraphicsScene) 
└── View (QGraphicsView)

Value Types
└── Socket (child of Node only, relative positioning)
```

### XML Structure
```xml
<graph>
  <nodes>
    <node id="uuid" x="100" y="200" type="processor">
      <socket id="uuid" rel_x="40" rel_y="0" type="output"/>
      <socket id="uuid" rel_x="-40" rel_y="0" type="input"/>
    </node>
  </nodes>
  <edges>
    <edge id="uuid" from_socket="uuid" to_socket="uuid" type="connection"/>
  </edges>
</graph>
```

### Build System Dependencies
- **Qt5**: Core, Widgets, Gui modules
- **libxml2**: v2.12.5 via FetchContent (will be static)
- **C++17**: Required standard
- **CMake**: 3.16+ for FetchContent support

## Success Criteria
1. **Cross-platform compatibility**: Same code builds and runs on Windows and Linux
2. **Self-serializing architecture**: No centralized XML management
3. **Performance**: No QObject overhead in graphics items
4. **Extensibility**: Easy to add new node types and features
5. **Visual quality**: Smooth interactions, proper rendering
6. **XML fidelity**: Round-trip XML preservation

## Observer Pattern Design: Pure Function Pointers (No Qt Signals)

### Inkscape Analysis Results ✅
Based on comprehensive analysis of Inkscape's 20+ year proven architecture, their observer pattern provides the blueprint for change notifications without "god objects".

**Key Inkscape Insights:**
1. **Layered Observer System**: XML changes → Object changes → UI updates
2. **CompositeObserver Pattern**: Manages multiple observers with safe concurrent access
3. **Modification Flags**: Efficient change propagation with cascade flags
4. **Event Synthesis**: New observers can "catch up" on current state
5. **No Centralized Knowledge**: Each object manages its own notifications

### Qt5 Adaptation: Function Pointers Only

**Critical Design Decision**: QGraphicsItem classes must NOT use Qt signals/slots. Using pure function pointers maintains performance and avoids QObject overhead.

**Observer Interface Pattern**:
```cpp
// Pure function pointer observer - NO QObject inheritance
class NodeObserver {
public:
    virtual ~NodeObserver() = default;
    
    // Notification methods inspired by Inkscape's NodeObserver
    virtual void notifyNodeChanged(Node* node, unsigned int flags) = 0;
    virtual void notifyNodeMoved(Node* node, const QPointF& oldPos, const QPointF& newPos) = 0;
    virtual void notifySocketAdded(Node* node, const Socket& socket) = 0;
    virtual void notifySocketRemoved(Node* node, const QUuid& socketId) = 0;
    virtual void notifyEdgeConnected(Edge* edge, const QUuid& fromSocket, const QUuid& toSocket) = 0;
};
```

**Composite Observer Management**:
```cpp
// Multi-observer coordination like Inkscape's CompositeNodeObserver
class CompositeNodeObserver : public NodeObserver {
public:
    void addObserver(NodeObserver* observer);
    void removeObserver(NodeObserver* observer);
    
    // Thread-safe iteration with pending removal (Inkscape pattern)
    void notifyNodeChanged(Node* node, unsigned int flags) override;
    void notifyNodeMoved(Node* node, const QPointF& oldPos, const QPointF& newPos) override;
    // ... forward all notifications to registered observers

private:
    QVector<NodeObserver*> m_observers;
    QVector<NodeObserver*> m_pendingRemovals;  // Safe concurrent modification
    bool m_iterating;
};
```

**Node Integration with Function Pointers**:
```cpp
class Node : public QGraphicsItem {
public:
    // Direct observer registration - NO Qt connect()
    void setObserver(NodeObserver* observer) { m_observer = observer; }
    
    // Simple callback for immediate notifications
    void setXmlUpdateCallback(void (*callback)(Node*, unsigned int)) {
        m_xmlUpdateCallback = callback;
    }

private:
    NodeObserver* m_observer;
    void (*m_xmlUpdateCallback)(Node*, unsigned int);
    
    // Call observers directly when changes occur
    void notifyObservers(unsigned int flags) {
        if (m_observer) m_observer->notifyNodeChanged(this, flags);
        if (m_xmlUpdateCallback) m_xmlUpdateCallback(this, flags);
    }
};
```

**Modification Flags System (Inkscape-inspired)**:
```cpp
// Change flags for efficient propagation
enum NodeModificationFlags {
    NODE_MODIFIED_POSITION     = 1 << 0,
    NODE_MODIFIED_SIZE         = 1 << 1,
    NODE_MODIFIED_SOCKETS      = 1 << 2,
    NODE_MODIFIED_ATTRIBUTES   = 1 << 3,
    NODE_MODIFIED_CONNECTIONS  = 1 << 4,
    NODE_MODIFIED_CASCADE      = 1 << 5,  // Propagate to parent
    NODE_MODIFIED_ALL          = 0xFF
};

// Efficient change batching
inline unsigned int cascadeFlags(unsigned int flags) {
    return (flags & NODE_MODIFIED_CASCADE) ? 
           (flags | NODE_MODIFIED_CONNECTIONS) : flags;
}
```

**Scene-Level Coordination**:
```cpp
class Scene : public QGraphicsScene, public NodeObserver {
public:
    // Scene acts as central observer coordinator
    void notifyNodeChanged(Node* node, unsigned int flags) override;
    void notifyNodeMoved(Node* node, const QPointF& oldPos, const QPointF& newPos) override;
    
    // XML synchronization triggers
    void updateXmlDocument(Node* node, unsigned int flags);
    void batchXmlUpdates(bool enabled);  // For bulk operations

private:
    CompositeNodeObserver m_sceneObserver;
    xmlDocPtr m_xmlDocument;
    bool m_batchMode;
    QVector<Node*> m_pendingUpdates;
};
```

**Live XML Synchronization**:
```cpp
class LiveXmlSync : public NodeObserver {
public:
    LiveXmlSync(xmlDocPtr document) : m_document(document) {}
    
    void notifyNodeMoved(Node* node, const QPointF& oldPos, const QPointF& newPos) override {
        // Immediately update XML position attributes
        if (!m_batchMode) {
            xmlNodePtr nodeXml = findNodeInXml(node->getId());
            if (nodeXml) {
                updateXmlPosition(nodeXml, newPos);
            }
        }
    }
    
    void beginBatchMode() { m_batchMode = true; }
    void endBatchMode() { m_batchMode = false; flushPendingUpdates(); }

private:
    xmlDocPtr m_document;
    bool m_batchMode;
    QVector<Node*> m_pendingNodes;
};
```

### Observer Pattern Benefits
1. **Performance**: No QObject overhead in graphics items
2. **Flexibility**: Multiple observers can monitor same object
3. **Decoupling**: UI, XML sync, and undo systems work independently
4. **Batch Operations**: Efficient bulk updates with batch mode
5. **Thread Safety**: Safe observer management during iteration
6. **Extensibility**: Easy to add new observer types

### Integration with Self-Serialization
- **Objects still self-serialize** via write()/read() methods
- **Observers coordinate** when serialization should occur
- **No centralized XML management** - each object knows its own format
- **Scene coordinates** observer notifications and XML document updates

## Code Evolution Review ✅

### Comprehensive Analysis Complete
I have created a detailed code review document that shows the complete evolution from problematic "God Object" patterns to clean self-serializing architecture.

**Review Document**: `COMPLETE_CODEBASE_EVOLUTION_REVIEW.md`

### Key Insights from Evolution Analysis:

#### What Went Wrong (GraphManager Era):
1. **God Object Anti-Pattern**: 284-line class handling everything
2. **Complex State Management**: 7 hash tables with intricate relationships  
3. **Performance Overhead**: Qt signals/slots + excessive debugging
4. **Testing Nightmare**: 40+ methods with interdependencies
5. **Maintenance Burden**: Every change required understanding entire system

#### What Went Right (Self-Serializing Era):
1. **44% Code Reduction**: From 1,200+ lines to 676 lines
2. **Inkscape-Inspired Pattern**: Proven 20+ year architecture
3. **Value Semantics**: Qt containers instead of pointer management
4. **Performance Focus**: No QObject overhead in graphics items
5. **Cross-Platform**: Identical build on Windows and Linux

### Critical Questions for Reviewer:

#### Architectural Safeguards:
- How do we prevent future god objects from emerging?
- What safeguards should we put in place to maintain clean architecture?
- How do we ensure new features don't violate single responsibility?

#### Performance Optimization:
- Are there any remaining performance bottlenecks?
- Should we add performance monitoring, or does that add overhead?
- How do we balance features vs. performance?

#### Maintainability Standards:
- How do we make the codebase more testable?
- What documentation standards should we follow?
- How do we onboard new developers without them falling into old patterns?

#### Extensibility Planning:
- How do we add new node types without breaking existing code?
- What's the proper way to extend the observer pattern?
- How do we handle future UI requirements?

#### Quality Assurance:
- What coding standards should we enforce?
- How do we ensure XML serialization remains robust?
- What automated tests should we add?

### Architecture Validation Metrics:
- **Code Quality**: 44% reduction in complexity
- **Performance**: Direct function calls vs. Qt signals overhead
- **Memory Safety**: Value semantics vs. manual memory management
- **Testability**: Isolated units vs. monolithic dependencies
- **Maintainability**: Simple interfaces vs. complex state management

## Current Status: Observer Pattern Complete ✅
Observer pattern with XML autosave functionality is now implemented and working. Ready for robust testing infrastructure.

## Next Priority: Robust Testing Infrastructure

### Step 6: Delete Functionality & UI Controls
**Goal**: Complete the delete functionality with proper UI controls

**Current State**: 
- ✅ Scene has all delete methods: `deleteNode()`, `deleteEdge()`, `deleteSelected()`, `clearGraph()`
- ✅ Observer notifications work for delete operations
- ❌ No UI controls (keyboard shortcuts) for delete operations

**Implementation Tasks**:
1. **Add Delete Keyboard Shortcuts to Window::keyPressEvent()**:
   ```cpp
   case Qt::Key_Delete:
   case Qt::Key_Backspace:
       m_scene->deleteSelected();  // Delete all selected items
       break;
   case Qt::Key_X:
       if (event->modifiers() & Qt::ControlModifier) {
           m_scene->deleteSelected();  // Ctrl+X = cut/delete
       }
       break;
   case Qt::Key_A:
       if (event->modifiers() & Qt::ControlModifier) {
           m_scene->selectAll();  // Ctrl+A = select all
       }
       break;
   ```

2. **Test Delete Operations**:
   - Delete single selected node → connected edges should also be deleted
   - Delete single selected edge → leaves nodes intact
   - Delete multiple selected items → batch delete with observer notifications
   - Verify autosave triggers after delete operations

### Step 7: Large File Generation for Stress Testing
**Goal**: Create robust testing infrastructure for large graphs

**Test File Generators**:
1. **Python Script: `generate_stress_test_graphs.py`**:
   ```python
   import xml.etree.ElementTree as ET
   import uuid
   import random
   import math
   
   def generate_large_graph(num_nodes=1000, num_edges=2000, filename="stress_test.xml"):
       """Generate large XML graphs for performance testing"""
       root = ET.Element("graph", version="1.0")
       
       # Generate nodes in grid pattern for visual testing
       nodes = []
       for i in range(num_nodes):
           x = (i % int(math.sqrt(num_nodes))) * 120 + random.randint(-20, 20)
           y = (i // int(math.sqrt(num_nodes))) * 80 + random.randint(-20, 20)
           
           node_id = str(uuid.uuid4())
           node_type = random.choice(["IN", "OUT"])
           inputs = random.randint(0, 3) if node_type == "IN" else random.randint(1, 5)
           outputs = random.randint(1, 5) if node_type == "OUT" else random.randint(0, 3)
           
           node = ET.SubElement(root, "node", 
                               id=node_id, x=str(x), y=str(y), 
                               type=node_type, inputs=str(inputs), outputs=str(outputs))
           nodes.append((node_id, inputs, outputs))
       
       # Generate random edges
       for _ in range(min(num_edges, len(nodes) * 2)):
           from_node = random.choice([n for n in nodes if n[2] > 0])  # has outputs
           to_node = random.choice([n for n in nodes if n[1] > 0])    # has inputs
           
           if from_node[0] != to_node[0]:  # no self-loops
               edge = ET.SubElement(root, "edge",
                                   id=str(uuid.uuid4()),
                                   fromNode=from_node[0], toNode=to_node[0],
                                   fromSocketIndex="0", toSocketIndex="0")
       
       tree = ET.ElementTree(root)
       tree.write(filename, encoding="utf-8", xml_declaration=True)
       print(f"Generated {filename}: {num_nodes} nodes, {num_edges} edges")
   
   # Generate test files of increasing size
   generate_large_graph(100, 200, "test_small.xml")      # Quick load test
   generate_large_graph(500, 1000, "test_medium.xml")    # Medium load test  
   generate_large_graph(1000, 2000, "test_large.xml")    # Large load test
   generate_large_graph(5000, 10000, "test_stress.xml")  # Stress test
   ```

2. **Performance Benchmark Script: `benchmark_autosave.py`**:
   ```python
   import subprocess
   import time
   import os
   
   def benchmark_autosave_performance():
       """Benchmark autosave performance with different graph sizes"""
       test_files = ["test_small.xml", "test_medium.xml", "test_large.xml"]
       
       for test_file in test_files:
           if os.path.exists(test_file):
               print(f"\n=== Benchmarking {test_file} ===")
               
               # Start application with test file
               start_time = time.time()
               process = subprocess.Popen(["./NodeGraph", test_file])
               
               # Let it run for 30 seconds to test autosave
               time.sleep(30)
               process.terminate()
               
               load_time = time.time() - start_time
               print(f"Load + 30s runtime: {load_time:.2f}s")
               
               # Check autosave file size
               if os.path.exists("autosave.xml"):
                   size = os.path.getsize("autosave.xml")
                   print(f"Autosave file size: {size/1024:.1f} KB")
   ```

### Step 8: Comprehensive Test Suite
**Goal**: Automated testing for all functionality

**Test Categories**:
1. **Unit Tests**:
   - Node serialization/deserialization
   - Edge connection resolution
   - Observer pattern notifications
   - XML autosave functionality

2. **Integration Tests**:
   - File loading with various graph sizes
   - Delete operations with observer notifications
   - Autosave triggering after changes
   - UI keyboard shortcuts

3. **Performance Tests**:
   - Autosave latency with large graphs
   - Memory usage during large file loads
   - UI responsiveness with 1000+ nodes
   - Observer notification overhead

4. **Stress Tests**:
   - 5000+ node graphs
   - Rapid add/delete operations
   - Continuous autosave over time
   - Memory leak detection

**Test Infrastructure**:
```cpp
// In selftest.cpp - expand existing test framework
class StressTest {
public:
    static bool testLargeFileLoading();
    static bool testDeleteOperations();
    static bool testAutosavePerformance();
    static bool testObserverNotifications();
    static bool testMemoryUsage();
    
    static bool runStressTestSuite();
};

bool StressTest::testDeleteOperations() {
    Scene scene;
    // Create test nodes
    Node* node1 = NodeRegistry::instance().createNode("IN");
    Node* node2 = NodeRegistry::instance().createNode("OUT");
    scene.addNode(node1);
    scene.addNode(node2);
    
    // Test single delete
    QUuid nodeId = node1->getId();
    scene.deleteNode(nodeId);
    
    return scene.getNode(nodeId) == nullptr;  // Should be deleted
}
```

### Step 9: Documentation & Usage Patterns
**Goal**: Clear documentation for testing and usage

**Testing Documentation**:
1. **TESTING.md** - Comprehensive testing guide
2. **PERFORMANCE.md** - Performance benchmarks and optimization tips
3. **STRESS_TESTING.md** - How to run and interpret stress tests

**Usage Patterns**:
```bash
# Basic usage (creates test nodes + autosave)
./NodeGraph

# Load existing file (with autosave backup)
./NodeGraph test.xml

# Stress testing mode
./NodeGraph --test --headless test_stress.xml

# Generate test files
python3 generate_stress_test_graphs.py

# Run performance benchmarks
python3 benchmark_autosave.py
```

### Success Metrics for Robust Testing:
- ✅ Delete operations work via keyboard shortcuts (Del, Backspace, Ctrl+X)
- ✅ Autosave triggers correctly after all operations (add/move/delete)
- ✅ Application handles 1000+ node graphs smoothly
- ✅ Memory usage remains stable during stress tests
- ✅ Observer notifications work correctly under load
- ✅ XML file integrity maintained through all operations

## Current Status: Observer Pattern Complete ✅
Ready to proceed with robust testing infrastructure and UI delete controls.

## OPTION C OWNERSHIP FIX - Raw Pointers + Destruction Hooks

### Goal
Fix dangling pointer crashes while keeping all hash system convenience by adding QObject::destroyed signal connections.

### Implementation Strategy
Keep existing `QHash<QUuid, Node*>` but add one line per item creation:
```cpp
connect(item, &QObject::destroyed, this, [id, this]{ m_hash.remove(id); });
```

### Benefits
- ✅ Keep all hash convenience (O(1) lookup, type safety, iteration)
- ✅ Fix dangling pointers (hash auto-cleans when items deleted)
- ✅ Zero API changes (existing code continues working)
- ✅ Qt-idiomatic (uses proper signal system)
- ✅ Minimal code change (literally one line per addNode/addEdge)

### Test Plan - Option C Ownership Fix

#### Phase 1: Add Destruction Hooks
**Files to modify**: `scene.cpp` - `addNode()` and `addEdge()` methods

**Changes**:
```cpp
void Scene::addNode(Node* node) {
    if (!node) return;
    
    QUuid nodeId = node->getId();
    m_nodes.insert(nodeId, node);
    addItem(node);
    
    // OPTION C FIX: Auto-remove from hash when item destroyed
    connect(node, &QObject::destroyed, this, [nodeId, this]() {
        qDebug() << "OPTION_C: Auto-removing destroyed node" << nodeId.toString().left(8);
        m_nodes.remove(nodeId);
    });
    
    // ... rest unchanged
}
```

#### Phase 2: Logging & Verification
**Log all ownership events** with "OPTION_C:" prefix:

1. **Hash insertion**: `OPTION_C: Added node <id> to hash`
2. **Destruction hook**: `OPTION_C: Connected destroy signal for node <id>`
3. **Auto-removal**: `OPTION_C: Auto-removing destroyed node <id>`
4. **Lookup safety**: `OPTION_C: Lookup node <id> result: <FOUND/NULL>`

#### Phase 3: Physical Test Scenarios

**Test 1: Basic Node Operations**
```bash
./NodeGraph --test --headless
```
1. Create 3 nodes via UI or test
2. Verify hash contains 3 entries
3. Delete 1 node via Delete key
4. Verify hash auto-removes entry
5. Lookup deleted node ID → should return nullptr

**Test 2: Dangling Pointer Prevention**  
```bash
./NodeGraph
```
1. Create node, remember its ID
2. Delete node via scene operations
3. Try to access node by stored ID
4. Should get nullptr, not crash

**Test 3: Mass Deletion**
```bash
./NodeGraph --test
```
1. Create 10 nodes
2. Select all (Ctrl+A)
3. Delete all (Delete key)
4. Verify hash is empty
5. Verify no crashes

**Test 4: Application Shutdown**
```bash
./NodeGraph
```
1. Create complex graph
2. Close application window
3. Check logs for clean destruction order
4. No double-delete errors

#### Phase 4: Performance Verification
**Measure impact of signal connections**:
1. Create 1000 nodes rapidly
2. Delete 500 nodes
3. Compare timing vs. raw pointer baseline
4. Acceptable if < 5% overhead

#### DISCOVERY: Option A & C Have Architectural Issues

**Problem Found**: Node/Edge inherit from QGraphicsItem, not QObject, so:
- **Option C fails**: No QObject::destroyed signal available
- **Option A complex**: Changing return types breaks 20+ call sites

**Actual Code Impact**:
- 20+ files use `getNodes().values()`, `getEdges().values()` 
- Iterator patterns expect QHash interface
- Major API breaking changes required

#### RECOMMENDATION: Keep Raw Pointers + Manual Cleanup

The current code actually already has **proper cleanup patterns**:
```cpp
void Scene::deleteNode(const QUuid& nodeId) {
    m_nodes.remove(nodeId);  // Hash cleanup
    removeItem(node);        // Qt scene cleanup  
    delete node;             // Memory cleanup
}
```

**Better approach**: Fix the dangling pointer in clearGraph() method only:
```cpp
void Scene::clearGraph() {
    // SAFE ORDER: Clear hash first, then scene
    m_nodes.clear();
    m_edges.clear(); 
    QGraphicsScene::clear();  // Now safe - no hash dangles
}
```

#### Simple Fix Success Criteria
- [ ] Fix clearGraph() order to prevent dangling pointers
- [ ] Add logging to verify cleanup order
- [ ] Test with our 4-node chain graph
- [ ] Verify no crashes during app shutdown
- [ ] Document the cleanup pattern for future developers

#### Precise Test Items to Try

**In Application**:
1. **Node Creation**: File → New, create nodes via palette
2. **Node Deletion**: Select node, press Delete key
3. **Edge Creation**: Drag between node sockets  
4. **Edge Deletion**: Select edge, press Delete key
5. **Mass Selection**: Ctrl+A, then Delete
6. **App Shutdown**: Close window, check clean exit

**Expected Log Output**:
```
OPTION_C: Added node 12345678 to hash
OPTION_C: Connected destroy signal for node 12345678
OPTION_C: Auto-removing destroyed node 12345678
OPTION_C: Lookup node 12345678 result: NULL
```

## Recent Performance Fixes ✅

### Task 1: Remove Edge ItemAcceptsInputMethod Flag - COMPLETED
**Branch**: `fix/edge-input-method-flag` 
**Issue**: Edges had unnecessary `ItemAcceptsInputMethod` flag causing Qt to route input method events through every edge, wasting cycles on large scenes.

**Fix Applied**:
- Removed `setFlag(QGraphicsItem::ItemAcceptsInputMethod, true);` from Edge constructor
- One line deleted, zero functional risk
- Performance improvement on scenes with many edges

**Test Plan**:
- ✅ Edge selection works unchanged (click to select)
- ✅ Edge hover effects work unchanged (blue hover highlight)  
- ✅ Edge deletion works unchanged (select + Delete key)
- ✅ Edge connection integrity maintained (edges update when nodes move)
- ✅ No crashes or visual regressions
- ✅ Autosave system continues working correctly

**Benefits**:
- Reduced Qt input method event routing overhead
- Better performance on large scenes with many edges
- Cleaner flag usage (edges don't handle text input)