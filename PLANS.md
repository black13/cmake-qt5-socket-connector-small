# Development Plans: Inkscape-Style Live XML Synchronization

## Current Status
✅ **Completed**: Phase 3 XML serialization fixes - save/load working with actual graph data  
✅ **Completed**: Phase 3.5 JavaScript Integration - QJSEngine with comprehensive API  
✅ **Completed**: Socket balancing improvements - vertical centering within nodes  
🔄 **Next**: Implement Inkscape-style live XML synchronization system  
📍 **Branch**: Ready to merge `feature/inkscape-xml-system` to main, then start new work

## Phase 3.5: JavaScript Integration System ✅

### Overview & Achievement
Successfully integrated QJSEngine to provide comprehensive JavaScript scripting capabilities for the NodeGraph system, enabling scriptable node behaviors, graph algorithms, and automated testing.

### Implementation Components

#### 3.5.1 JavaScript Engine Core (`javascript_engine.h/cpp`)
**Files:** `javascript_engine.h`, `javascript_engine.cpp`

```cpp
class JavaScriptEngine : public QObject {
    // QJSEngine integration for node scripting
    QJSValue evaluate(const QString& script);
    QJSValue evaluateFile(const QString& filePath);
    
    // API registration with scene access
    void registerNodeAPI(Scene* scene);
    void registerGraphController(Scene* scene, GraphFactory* factory);
    
    // Node scripting and graph processing
    bool executeNodeScript(Node* node, const QString& script);
    QJSValue processGraph(const QString& algorithm);
    
    // Script module management
    void loadScriptModule(const QString& moduleName, const QString& scriptContent);
    QJSValue getModule(const QString& moduleName);
};
```

#### 3.5.2 Comprehensive JavaScript API
**Capabilities implemented:**
- **Node Operations**: Creation, deletion, property modification via JavaScript
- **Graph Processing**: Algorithm execution, traversal, analysis
- **Scene Integration**: Direct access to Qt5 graphics scene from scripts
- **Console API**: `console.log()`, `console.error()` with Qt integration
- **Error Handling**: Exception capture and JavaScript error reporting
- **Module System**: Script loading and reusable module management

#### 3.5.3 JavaScript Test Suite (`scripts/`)
**Test Files:**
- `test_javascript.js` - Comprehensive engine functionality testing
- `test_graph_creation.js` - Graph operations via JavaScript API
- `test_roundtrip.js` - Data serialization and consistency testing  
- `node_algorithms.js` - Graph processing algorithms in JavaScript
- `custom_nodes.js` - Custom node type definitions and behaviors
- `test_destructor_safety.js` - Safety testing for memory management

#### 3.5.4 Safety Integration
**Safety Features:**
- Destructor exception safety testing via JavaScript automation
- Memory leak detection through script-driven object lifecycle testing
- Crash prevention verification for edge cases
- JavaScript-driven stress testing of Qt5 graphics system

### Key Benefits Achieved

✅ **Scriptable Behaviors**: Nodes can now execute custom JavaScript logic  
✅ **Graph Algorithms**: Complex graph processing implemented in JavaScript  
✅ **Automated Testing**: Comprehensive test suite runs via JavaScript engine  
✅ **Rapid Development**: New node types and behaviors without C++ compilation  
✅ **Safety Validation**: JavaScript-driven safety and crash prevention testing  
✅ **API Extensibility**: Clean separation between core C++ and script logic  

### Success Criteria Met

- ✅ QJSEngine fully integrated with Qt5 graphics system
- ✅ JavaScript API provides full access to node and graph operations  
- ✅ Comprehensive test suite validates engine functionality
- ✅ Script module system enables code reuse and organization
- ✅ Error handling and debugging support implemented
- ✅ Safety testing automation via JavaScript scripts
- ✅ Performance acceptable for real-time graph operations

## Phase 4: Live XML Synchronization System

### Overview & Goal
Transform the system so GraphManager and XML document remain perfectly synchronized at all times, with XML as a live canonical representation that gets updated immediately on every graph operation.

### Current Architecture Issues
```
Current: GraphManager → (on save) → serialize to XML → write file
Problem: XML is only a snapshot, not live representation
```

### Target Architecture  
```
Target: GraphManager operations → immediately update XML → fast save writes XML
Result: XML is always current, save is instant write operation
```

## Implementation Plan

### Phase 4.1: Foundation Changes

#### 4.1.1 Establish File Context
**Files to modify:** `GraphXmlDocument.hpp/cpp`, `GraphView.cpp`

```cpp
class GraphXmlDocument {
private:
    QString m_currentFilename;           // Active file path (empty = new document)
    bool m_isDirty;                      // Has unsaved changes
    bool m_isLiveSyncEnabled;            // XML stays synchronized
    
public:
    // File management
    void setCurrentFile(const QString& filename);
    QString getCurrentFile() const { return m_currentFilename; }
    bool isDirty() const { return m_isDirty; }
    void markDirty(bool dirty = true);
    
    // Document lifecycle
    bool isNewDocument() const { return m_currentFilename.isEmpty(); }
    QString getDocumentTitle() const;    // "Untitled" or filename
};
```

#### 4.1.2 Distinguish Save vs Save As
**Files to modify:** `GraphView.cpp`, add new menu/shortcuts

```cpp
class GraphView {
private:
    void saveGraph();        // Save to current file (Ctrl+S)
    void saveGraphAs();      // Save with new filename (Ctrl+Shift+S)
    void newDocument();      // Clear and reset (Ctrl+N)
    void openDocument();     // Load existing file (Ctrl+O)
    
    void updateWindowTitle(); // Show filename and dirty state
};
```

### Phase 4.2: Live XML Synchronization Core

#### 4.2.1 Bidirectional Connection Setup
**Files to modify:** `GraphManager.cpp`, `GraphXmlDocument.cpp`

```cpp
class GraphManager {
private:
    GraphXmlDocument* m_xmlDoc;  // Live XML companion
    bool m_suppressXmlUpdates;   // For bulk operations
    
public:
    void setXmlDocument(GraphXmlDocument* xmlDoc);
    
    // All operations update both GraphManager AND XML immediately:
    QUuid createNode(const QString& type, const QPointF& position) override {
        QUuid id = createNodeInternal(type, position);
        if (m_xmlDoc && !m_suppressXmlUpdates) {
            m_xmlDoc->addNodeToXml(id, type, position);
            m_xmlDoc->markDirty();
        }
        return id;
    }
};
```

#### 4.2.2 Operation-by-Operation XML Updates
**Implementation for each GraphManager operation:**

```cpp
// Node operations
QUuid createNode() → addNodeToXml() + markDirty()
bool deleteNode() → removeNodeFromXml() + markDirty()
void moveNode() → updateNodePositionInXml() + markDirty()

// Socket operations  
void addSocket() → updateNodeSocketsInXml() + markDirty()
void removeSocket() → updateNodeSocketsInXml() + markDirty()

// Connection operations
QUuid createConnection() → addConnectionToXml() + markDirty()
bool deleteConnection() → removeConnectionFromXml() + markDirty()

// Bulk operations
void clearGraph() → clearXmlDocument() + markDirty()
void beginBatchOperation() → setSuppressXmlUpdates(true)
void endBatchOperation() → setSuppressXmlUpdates(false) + syncAllToXml()
```

### Phase 4.3: Fast Save System

#### 4.3.1 Eliminate Full Serialization
**Current slow path:**
```cpp
// OLD: Full reconstruction every time
bool saveGraph() {
    m_xmlDoc->createEmptyDocument();           // Wasteful
    m_xmlDoc->serializeFromGraph(m_manager);   // Slow full scan
    m_xmlDoc->saveToFile(filename);            // Write reconstructed XML
}
```

**New fast path:**
```cpp
// NEW: XML is always current
bool saveGraph() {
    if (m_xmlDoc->isNewDocument()) {
        return saveGraphAs();  // First save needs filename
    }
    
    // XML is already current, just write it
    bool success = m_xmlDoc->saveToFile(m_xmlDoc->getCurrentFile());
    if (success) {
        m_xmlDoc->markDirty(false);
        updateWindowTitle();
    }
    return success;
}
```

#### 4.3.2 Optimized Save Operations
```cpp
bool saveToFile(const QString& filename) {
    // XML is live and current - just write current state
    return writeCurrentXmlToFile(filename);  // Fast direct write
}

bool saveToCurrentFile() {
    if (m_currentFilename.isEmpty()) return false;
    bool success = writeCurrentXmlToFile(m_currentFilename);
    if (success) markDirty(false);
    return success;
}
```

### Phase 4.4: Consistency & Error Handling

#### 4.4.1 Synchronization Validation
```cpp
class GraphXmlDocument {
public:
    // Verify GraphManager and XML are in sync
    bool validateSynchronization(const GraphManager* manager);
    
    // Emergency resync if they get out of sync
    bool forceSyncFromGraphManager(const GraphManager* manager);
    bool forceSyncToGraphManager(GraphManager* manager);
    
    // Debug utilities
    void debugPrintSyncState(const GraphManager* manager);
};
```

#### 4.4.2 Bulk Operation Optimization
```cpp
// For performance during complex operations
void GraphManager::rebuildGraph() {
    beginBatchOperation();  // Suspend XML updates
    
    clearGraph();
    // ... create many nodes/connections ...
    
    endBatchOperation();    // Single XML sync at end
}
```

### Phase 4.5: User Experience Improvements

#### 4.5.1 Window Title & Status
```cpp
void updateWindowTitle() {
    QString title = m_xmlDoc->getDocumentTitle();
    if (m_xmlDoc->isDirty()) title += "*";
    title += " - NodeGraph Editor";
    setWindowTitle(title);
}

void showStatus() {
    QString status = QString("File: %1 | Nodes: %2 | Connections: %3 | %4")
                    .arg(m_xmlDoc->getDocumentTitle())
                    .arg(m_manager->nodeCount())
                    .arg(m_manager->connectionCount())
                    .arg(m_xmlDoc->isDirty() ? "Modified" : "Saved");
    printf("%s\n", status.toLocal8Bit().constData());
}
```

#### 4.5.2 Enhanced Keyboard Shortcuts
```cpp
// In GraphView::keyPressEvent()
case Qt::Key_S:
    if (event->modifiers() & Qt::ShiftModifier) {
        saveGraphAs();    // Ctrl+Shift+S
    } else {
        saveGraph();      // Ctrl+S  
    }
    break;
case Qt::Key_N:
    newDocument();        // Ctrl+N
    break;
case Qt::Key_O:
    openDocument();       // Ctrl+O
    break;
```

## Implementation Order

1. **Phase 4.1**: File context (current filename, dirty state)
2. **Phase 4.2**: Live XML sync (operations update XML immediately)  
3. **Phase 4.3**: Fast save (write current XML vs reconstruction)
4. **Phase 4.4**: Validation and error handling
5. **Phase 4.5**: Polish and user experience

## Key Benefits

✅ **Performance**: Save becomes instant XML write vs slow full serialization  
✅ **Consistency**: XML and GraphManager always in perfect sync  
✅ **User Experience**: Standard file operations (Save/Save As/New/Open)  
✅ **Reliability**: Always know current file state and if changes need saving  
✅ **Inkscape-style**: XML document is live canonical representation  

## Potential Challenges

⚠️ **Complexity**: Need to ensure every GraphManager operation updates XML  
⚠️ **Synchronization**: Risk of XML and GraphManager getting out of sync  
⚠️ **Performance**: XML updates on every operation (mitigated by batching)  
⚠️ **Testing**: Need to verify sync works correctly for all operations  

## Success Criteria

### Phase 4.1 Success
- ✅ File management working (current file, dirty state)
- ✅ Save vs Save As distinction working
- ✅ Window title shows file state

### Phase 4.2 Success  
- ✅ Every GraphManager operation updates XML immediately
- ✅ Bulk operations use batching for performance
- ✅ XML and GraphManager stay synchronized

### Phase 4.3 Success
- ✅ Save operations are instant (no serialization delay)
- ✅ XML document is always ready to save
- ✅ Fast file operations working

### Phase 4.4 Success
- ✅ Synchronization validation working
- ✅ Error recovery mechanisms in place
- ✅ Robust against desync scenarios

### Phase 4.5 Success
- ✅ Professional file management UI
- ✅ Standard keyboard shortcuts working
- ✅ Clear user feedback on file state

## Next Steps

1. **Merge current work**: `feature/inkscape-xml-system` → `main`
2. **Create new branch**: `feature/live-xml-sync` from updated `main`
3. **Start Phase 4.1**: Implement file context and basic file management
4. **Progressive implementation**: Each phase builds on previous success

---

## Future Phases (Post-Live Sync)

### Phase 5: Advanced File Features
- Auto-save functionality
- Recent files menu  
- Crash recovery
- File watching and external change detection

### Phase 6: Undo/Redo Integration
- XML-aware undo commands
- Live undo synchronization
- Command batching and optimization

### Phase 7: Performance Optimization
- XML writing optimization
- Memory usage optimization
- Large graph handling improvements

### Phase 8: Extension Points
- Plugin architecture for custom XML elements
- Export/import to other formats
- Template system for common graph patterns

---

## Phase 9: XML-Based Computation Engine

### Overview & Vision
Transform the NodeGraph system into an XML-driven computation engine that can either interpret computational graphs on-the-fly or generate and compile native code for high-performance execution.

### Core Concept
Create a visual programming environment where:
- Computational logic is stored as XML in the scene graph
- Nodes can contain embedded scripts or computational specifications  
- The system can switch between interpreted execution (fast iteration) and compiled execution (high performance)
- All computation remains serializable and version-controllable through XML

### Implementation Phases

#### Phase 9.1: Scriptable Compute Nodes (2-4 weeks)
**Goal**: Add nodes that can execute embedded JavaScript for computation

**XML Schema Extension:**
```xml
<node id="calc1" type="Compute" x="100" y="100">
    <script lang="javascript"><![CDATA[
        function compute(inputs) {
            return inputs.a + inputs.b * Math.sin(inputs.c);
        }
    ]]></script>
    <inputs>
        <input name="a" type="number" socket="0"/>
        <input name="b" type="number" socket="1"/>
        <input name="c" type="number" socket="2"/>
    </inputs>
    <output name="result" type="number" socket="0"/>
</node>
```

**ComputeNode Implementation:**
```cpp
class ComputeNode : public Node {
private:
    QString m_scriptSource;
    QJSValue m_compiledFunction;
    QVariantMap m_inputValues;
    QVariant m_outputValue;
    
public:
    void setScript(const QString& source);
    void execute();
    QVariant getOutput() const;
    
    // XML serialization integration
    xmlNodePtr write(xmlDocPtr doc, xmlNodePtr repr) const override;
    void read(xmlNodePtr node) override;
};
```

**Data Flow Integration:**
- Connect compute nodes via existing socket system
- When input sockets receive data → trigger `execute()`
- Cache results until inputs change
- Propagate outputs to connected nodes

#### Phase 9.2: Advanced Computation Features (Month 2)
**Goal**: Enhanced computational capabilities and debugging

**Multi-Language Support:**
- JavaScript (current QJSEngine)
- Lua integration (`lua.hpp`)
- Python embedding (`python.h`)

**Standard Library Functions:**
```javascript
// Math operations
Math.matrix([[1,2],[3,4]])
Math.fft([1,2,3,4])
Math.interpolate(points, method)

// Data processing  
Data.filter(array, predicate)
Data.reduce(array, operation)
Data.transform(data, mapping)

// Visualization helpers
Plot.line(data)
Plot.histogram(values)
Chart.bar(categories, values)
```

**Live Computation Console:**
- REPL environment integrated with existing JavaScript engine
- Real-time graph inspection
- Interactive debugging capabilities
- Script testing sandbox

#### Phase 9.3: Code Generation Engine (Month 3-4)
**Goal**: Generate and compile native code from XML computation graphs

**XML Computation Schema:**
```xml
<computation id="pipeline1">
    <const id="pi" type="double" value="3.14159"/>
    <var id="x" type="double" socket="input_0"/>
    <op id="mult1" fn="multiply" inputs="pi x"/>
    <op id="sin1" fn="sin" inputs="mult1"/>
    <output id="result" from="sin1" socket="output_0"/>
</computation>
```

**Code Generation Pipeline:**
```cpp
class ComputationIR {
    struct Operation {
        QString id;
        QString function;
        QStringList inputs;
        QString output;
        QVariantMap parameters;
    };
    
    QList<Operation> operations;
    QStringList dependencies;
    
public:
    static ComputationIR fromXml(xmlNodePtr computeGraph);
    QString generateCpp() const;
    QString generateLLVMIR() const;
};
```

**JIT Compilation System:**
```cpp
class JITCompiler {
private:
    QTemporaryDir m_tempDir;
    QHash<QString, QLibrary*> m_loadedLibraries;
    
public:
    QString compile(const ComputationIR& ir);
    void* loadFunction(const QString& libPath, const QString& funcName);
    void unloadLibrary(const QString& libPath);
};
```

**Generated Code Example:**
From XML computation → generates:
```cpp
extern "C" double pipeline1(double x) {
    const double pi = 3.14159;
    double mult1 = pi * x;
    return std::sin(mult1);
}
```

#### Phase 9.4: Advanced Features (Month 5-6)
**Goal**: Production-ready computational environment

**Hot-Reload Infrastructure:**
- Content-based caching (hash XML → compiled .so)
- Async compilation pipeline
- Fallback to interpreter during compilation
- Memory-mapped function loading

**Performance Optimizations:**
- Automatic vectorization detection
- Loop fusion and optimization
- Dead code elimination
- Common subexpression elimination

**Plugin Architecture:**
```cpp
class ComputationPlugin {
public:
    virtual QString name() const = 0;
    virtual QStringList supportedOperations() const = 0;
    virtual QJSValue execute(const QString& op, const QJSValueList& args) = 0;
    virtual QString generateCode(const QString& op, const QStringList& inputs) const = 0;
};
```

### Integration with Existing System

**Scene Integration:**
- ComputeNode inherits from existing Node class
- Uses current socket system for I/O connections
- Integrates with XML serialization via `write()/read()`
- Participates in observer pattern for change notifications

**JavaScript API Extension:**
```javascript  
// Enhanced Graph API building on existing javascript_engine.cpp
Graph.createComputeNode(x, y, script)
Graph.setNodeScript(nodeId, script)
Graph.executeNode(nodeId)
Graph.getNodeOutput(nodeId)

// New Compilation API
Compiler.generateCode(nodeId)
Compiler.compile(nodeId)
Compiler.switchToCompiled(nodeId)
Compiler.switchToInterpreted(nodeId)
```

**Memory Management Integration:**
- Smart pointer integration with existing Qt object system
- Reference counting for shared computational data
- Automatic cleanup of compiled libraries
- Memory pool for frequent allocations during computation

### Performance Targets

**Interpreter Path (Development):**
- QJSEngine execution: ~1-10ms per operation
- Suitable for prototyping and debugging
- Immediate feedback for script changes
- No compilation overhead

**Compiled Path (Production):**
- Native execution: ~0.01-0.1ms per operation  
- 10-100x performance improvement over interpreted
- Startup cost: 100-1000ms compilation time
- Suitable for intensive computational workloads

**Hybrid Approach:**
- Start with interpreter for immediate feedback
- Async compile in background while user continues editing
- Hot-swap to compiled version when ready
- Fallback to interpreter on compilation failure

### Security & Sandboxing

**Script Execution Limits:**
- Execution time limits to prevent infinite loops
- Memory usage caps for computational workloads
- Function whitelist/blacklist for allowed operations
- File system access restrictions

**Compilation Security:**
- Compiler process sandboxing
- Code injection prevention during IR generation
- Signed library verification for loaded .so files
- Resource usage monitoring during compilation

### Development Milestones

**Milestone 9.1: Basic Scriptable Nodes (Week 1-2)**
- [ ] ComputeNode class implementation
- [ ] XML schema for embedded scripts
- [ ] Basic JavaScript execution via existing engine
- [ ] Socket integration for data flow
- [ ] Simple UI for script editing

**Milestone 9.2: Enhanced Scripting (Week 3-4)**
- [ ] Error handling and reporting integration
- [ ] Standard library functions in JavaScript
- [ ] Live output display in nodes
- [ ] Script validation and syntax checking
- [ ] Performance monitoring dashboard

**Milestone 9.3: Code Generation Foundation (Week 5-8)**
- [ ] IR generation from XML computation graphs
- [ ] Basic C++ code generation templates
- [ ] Simple compilation pipeline using system compiler
- [ ] Function loading and execution testing
- [ ] Performance comparison tools (interpreted vs compiled)

**Milestone 9.4: Production Features (Week 9-12)**
- [ ] Hot-reload and content-based caching system
- [ ] Advanced optimizations (vectorization, fusion)
- [ ] GPU support investigation (CUDA/OpenCL)
- [ ] Plugin architecture implementation
- [ ] Comprehensive documentation and examples

### Integration Benefits

**Leverages Existing Infrastructure:**
- ✅ XML serialization system already handles complex node data
- ✅ JavaScript engine provides immediate execution environment
- ✅ Socket system handles data flow between computational nodes
- ✅ Observer pattern enables reactive computation updates
- ✅ Qt5 graphics system supports real-time visualization

**Extends Current Capabilities:**
- ✅ Visual programming becomes true computational platform
- ✅ Rapid prototyping with interpreted scripts
- ✅ Production deployment with compiled performance
- ✅ All computation remains version-controllable via XML
- ✅ Seamless integration with existing node-based workflow

### Success Criteria

**Technical Metrics:**
- **Performance**: 10-100x speedup for compiled vs interpreted execution
- **Memory Usage**: <10% overhead for computation framework
- **Compilation Time**: <1 second for typical computation graphs
- **Accuracy**: Identical results between interpreted and compiled modes

**User Experience Metrics:**
- **Learning Curve**: New users productive with compute nodes within 30 minutes
- **Debugging Experience**: Easy to locate and fix script errors
- **Visual Feedback**: Real-time computation results displayed in nodes
- **Workflow Integration**: Seamless with existing node editing and XML persistence

### Long-term Vision

**Year 1: Foundation**
- Complete XML computation engine with JavaScript and C++ paths
- Visual debugging environment integrated with existing tools
- Basic optimization pipeline for common computation patterns

**Year 2: Expansion**  
- Multi-language support (Python, Lua, Rust integration)
- GPU computation integration with automatic CPU/GPU dispatch
- Distributed execution capabilities for large computational graphs

**Year 3: Ecosystem**
- Plugin marketplace for computational modules
- Standard library ecosystem for domain-specific computation
- Cloud execution platform integration
- Industry-specific templates and computational building blocks

---

## Phase 10: Linux/WSL Cross-Platform Build System ✅

### Overview & Achievement
Successfully implemented comprehensive Linux/WSL build support with intelligent Qt5 auto-detection, eliminating manual configuration and supporting multiple Qt installations simultaneously.

### Implementation Components

#### 10.1 Qt5 Auto-Detection System (`CMakeLists.txt`)
**Files:** `CMakeLists.txt` lines 44-91

```cmake
# Linux/WSL: Auto-detect Qt installations in /usr/local/qt-*
file(GLOB QT_INSTALL_DIRS "/usr/local/qt*")

# Sort directories to prefer newer versions (reverse alphabetical)
list(SORT QT_INSTALL_DIRS)
list(REVERSE QT_INSTALL_DIRS)

# Add found Qt installations to search path
foreach(QT_DIR ${QT_INSTALL_DIRS})
    if(IS_DIRECTORY "${QT_DIR}")
        # Check for common Qt directory structures
        if(EXISTS "${QT_DIR}/lib/cmake/Qt5")
            list(APPEND CMAKE_PREFIX_PATH "${QT_DIR}")
            message(STATUS "Found Qt installation: ${QT_DIR}")
        endif()
        
        # Check for debug/release subdirectories
        if(EXISTS "${QT_DIR}/debug" AND EXISTS "${QT_DIR}/debug/lib/cmake/Qt5")
            list(APPEND CMAKE_PREFIX_PATH "${QT_DIR}/debug")
            message(STATUS "Found Qt Debug build: ${QT_DIR}/debug")
        endif()
        
        if(EXISTS "${QT_DIR}/release" AND EXISTS "${QT_DIR}/release/lib/cmake/Qt5")
            list(APPEND CMAKE_PREFIX_PATH "${QT_DIR}/release")
            message(STATUS "Found Qt Release build: ${QT_DIR}/release")
        endif()
    endif()
endforeach()

# Prefer debug builds for Debug configuration, release builds for Release
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Debug build: Preferring Qt debug libraries")
else()
    message(STATUS "Release build: Preferring Qt release libraries")
endif()
```

#### 10.2 Enhanced Build Script (`build.sh`)
**Files:** `build.sh` - Complete rewrite with intelligent Qt selection

**Key Features:**
- **Automatic Qt Detection**: Finds all `/usr/local/qt*` installations
- **Build Type Matching**: Prefers debug Qt for debug builds, release Qt for release builds
- **Incremental Build Support**: Smart cache preservation and cleanup options
- **WSL Detection**: Automatically detects WSL environment
- **X11 Integration**: Guides user through X11 server setup for GUI
- **Error Handling**: Comprehensive error checking and reporting
- **Performance**: Multi-core compilation using all available CPU cores

**Usage Examples:**
```bash
# Debug build with auto-detection
./build.sh debug

# Release build with auto-detection  
./build.sh release

# Clean debug build
./build.sh debug clean
```

#### 10.3 Comprehensive Documentation (`LINUX_BUILD.md`)
**Files:** `LINUX_BUILD.md` - Complete Linux/WSL build guide

**Documentation Covers:**
- Qt5 installation patterns and directory structures
- Build type detection and selection logic
- Manual CMake usage for advanced scenarios
- Troubleshooting guide with common error solutions
- Qt installation tips for different scenarios
- X11 server setup for Windows users

#### 10.4 Cross-Platform Consistency
**Achieved unified build experience:**
- Windows: Uses hardcoded Qt paths (existing system)
- Linux/WSL: Uses intelligent auto-detection (new system)
- Both: Share same CMake configuration and libxml2 handling
- Consistent: Build scripts provide same interface across platforms

### Key Benefits Achieved

✅ **Zero Configuration**: No manual Qt path specification required  
✅ **Multi-Version Support**: Automatically handles multiple Qt installations  
✅ **Build Type Intelligence**: Matches debug/release Qt with build configuration  
✅ **Performance Optimization**: Preserves build cache and uses all CPU cores  
✅ **Developer Experience**: Clear error messages and troubleshooting guidance  
✅ **WSL Integration**: Native Windows X11 server compatibility  

### Success Criteria Met

**Technical Success:**
- ✅ Automatically detects Qt 5.15.17 debug and release installations
- ✅ Selects correct Qt build type based on CMAKE_BUILD_TYPE
- ✅ Builds successfully with system libxml2 and FetchContent fallback
- ✅ Generates working executable compatible with X11 forwarding
- ✅ Preserves build performance with incremental compilation

**User Experience Success:**
- ✅ Single command builds (`./build.sh debug`) with no configuration
- ✅ Clear status messages show Qt detection and selection process
- ✅ Comprehensive documentation covers all installation scenarios
- ✅ Error messages provide actionable solutions
- ✅ Build script provides consistent interface across platforms

### Testing Results

**Test Environment:** WSL2 Ubuntu with Qt 5.15.17 debug/release installations  
**Build Output:**
```
[INFO] Found Qt installations:
  - /usr/local/qt5.15.17-release
  - /usr/local/qt5.15.17-debug
[SUCCESS] Selected Qt5 Debug build: /usr/local/qt5.15.17-debug
-- Found Qt installation: /usr/local/qt5.15.17-release
-- Found Qt installation: /usr/local/qt5.15.17-debug
-- Debug build: Preferring Qt debug libraries
-- Qt5 version      : 5.15.17
-- Qt5 location     : /usr/local/qt5.15.17-debug/lib/cmake/Qt5
[SUCCESS] Build completed successfully!
```

**Performance:** Complete build from scratch in ~30 seconds on 12-core system  
**Compatibility:** Executable runs successfully with VcXsrv X11 server on Windows  

### Future Enhancements

**Phase 10.2: Advanced Linux Features**
- Package manager integration (apt, pacman, dnf)
- Automatic Qt5 installation script
- Docker containerized build environment
- Continuous integration Linux build pipeline

**Phase 10.3: Platform Expansion**
- macOS support with Homebrew Qt detection
- FreeBSD compatibility testing
- ARM64 Linux support (Raspberry Pi, Apple Silicon)
- Android cross-compilation capability

---

## Phase 11: JavaScript Computation Engine with rubber_types ✨

### Overview & Vision
Transform the existing Node/Edge/Socket system into a scriptable computation platform using rubber_types for type erasure and QJSEngine for JavaScript orchestration, while maintaining the proven XML serialization system.

### Current Code Analysis

**Strong Foundation Available:**
- **Node class** (`node.h/cpp`): Self-serializing with `write(xmlDocPtr, xmlNodePtr)` and `read(xmlNodePtr)` 
- **Edge class** (`edge.h/cpp`): Complete source/target tracking with tomb-stone safe weak pointers
- **Socket class** (`socket.h/cpp`): Input/output port management with edge registration
- **XML Integration**: All classes serialize core attributes (id, x, y, type, fromNode, toNode) to XML
- **Observer Pattern**: Edge registration/unregistration maintains graph consistency
- **Memory Safety**: Weak pointer tomb-stoning prevents dangling references during undo

### Implementation Roadmap

#### Phase 11.1: Type Erasure Foundation (Week 1-2)
**Goal**: Wrap existing classes in rubber_types façades without changing serialization

**Core Specs to Implement:**
```cpp
// IdentifiableSpec - exposes ID management
struct IdentifiableSpec {
    struct Concept {
        virtual ~Concept() = default;
        virtual QUuid id() const = 0;
    };
    
    template<class Holder>
    struct Model : Holder, virtual Concept {
        using Holder::Holder;
        QUuid id() const override {
            return rubber_types::model_get(this).getId();
        }
    };
    
    template<class Container>
    struct ExternalInterface : Container {
        using Container::Container;
        QUuid id() const { 
            return rubber_types::interface_get(this).id(); 
        }
    };
};

// SpatialSpec - position and geometry
struct SpatialSpec {
    struct Concept {
        virtual ~Concept() = default;
        virtual double x() const = 0;
        virtual double y() const = 0;
        virtual void setPos(double x, double y) = 0;
    };
    // ... Model and ExternalInterface implementations
};

// SerializableSpec - XML persistence
struct SerializableSpec {
    struct Concept {
        virtual ~Concept() = default;
        virtual xmlNodePtr write(xmlDocPtr, xmlNodePtr) const = 0;
        virtual void read(xmlNodePtr) = 0;
    };
    // ... implementations delegate to Node::write/read
};
```

**Generated Façades:**
```cpp
using NodeFacade = rubber_types::MergeConcepts<
    rubber_types::TypeErasure<IdentifiableSpec>,
    rubber_types::TypeErasure<SpatialSpec>, 
    rubber_types::TypeErasure<SerializableSpec>
>;

using EdgeFacade = rubber_types::MergeConcepts<
    rubber_types::TypeErasure<IdentifiableSpec>,
    rubber_types::TypeErasure<SerializableSpec>
>;
```

**Integration Points:**
- Façades hold references to existing Node/Edge objects
- XML serialization unchanged - `write()/read()` methods delegate directly
- Tomb-stoning safety preserved through reference semantics
- Zero runtime overhead - virtual dispatch same as traditional inheritance

#### Phase 11.2: Relationship Graph Layer (Week 2-3)
**Goal**: Build live relationship tracking using façades

**RelationshipGraph Implementation:**
```cpp
class RelationshipGraph {
private:
    std::unordered_map<QUuid, NodeFacade> m_nodes;
    std::unordered_map<QUuid, EdgeFacade> m_edges;
    std::unordered_multimap<QUuid, QUuid> m_nodeEdges;  // node -> edges
    
public:
    void addNode(Node* raw) {
        m_nodes.emplace(raw->getId(), NodeFacade{*raw});
    }
    
    void addEdge(Edge* raw) {
        QUuid edgeId = raw->getId();
        m_edges.emplace(edgeId, EdgeFacade{*raw});
        m_nodeEdges.emplace(raw->fromNodeId(), edgeId);
        m_nodeEdges.emplace(raw->toNodeId(), edgeId);
    }
    
    NodeFacade* findNode(QUuid id) {
        auto it = m_nodes.find(id);
        return (it != m_nodes.end()) ? &it->second : nullptr;
    }
    
    std::vector<EdgeFacade*> getNodeEdges(QUuid nodeId);
};
```

**Observer Integration:**
- One observer per XML node updates the relationship graph
- `notifyAttributeChanged()` triggers façade updates
- `notifyChildAdded/Removed()` maintains graph structure
- Dirty-set with idle flush for derived computations

#### Phase 11.3: JavaScript Bridge (Week 3-4)
**Goal**: Expose façades to QJSEngine for scriptable algorithms

**QObject Proxy Layer:**
```cpp
class JsNode : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString id READ id CONSTANT)
    Q_PROPERTY(double x READ x WRITE setX)
    Q_PROPERTY(double y READ y WRITE setY)
    
public:
    explicit JsNode(NodeFacade facade, QObject* parent = nullptr)
        : QObject(parent), m_facade(std::move(facade)) {}
    
    QString id() const { return m_facade.id().toString(); }
    double x() const { return m_facade.x(); }
    double y() const { return m_facade.y(); }
    void setX(double x) { m_facade.setPos(x, m_facade.y()); }
    void setY(double y) { m_facade.setPos(m_facade.x(), y); }
    
    // Heavy computation methods callable from JavaScript
    Q_INVOKABLE double distanceTo(JsNode* other);
    Q_INVOKABLE QVariantList getConnectedNodes();
    Q_INVOKABLE void computeLayout();
    
private:
    NodeFacade m_facade;
};
```

**JavaScript Engine Setup:**
```cpp
void setupJavaScriptEngine() {
    QJSEngine* engine = new QJSEngine(this);
    
    // Expose graph as JavaScript array
    QJSValue jsNodes = engine->newArray(m_graph.nodeCount());
    int index = 0;
    for (auto& [id, facade] : m_graph.nodes()) {
        auto* proxy = new JsNode(facade, engine);
        jsNodes.setProperty(index++, engine->newQObject(proxy));
    }
    
    engine->globalObject().setProperty("nodes", jsNodes);
    engine->globalObject().setProperty("graph", 
                                     engine->newQObject(new JsGraph(&m_graph, engine)));
}
```

**JavaScript Algorithm Examples:**
```javascript
// Force-directed layout algorithm in JavaScript
function computeForceLayout(iterations) {
    for (let iter = 0; iter < iterations; iter++) {
        for (let i = 0; i < nodes.length; i++) {
            let fx = 0, fy = 0;
            
            // Repulsion forces (expensive calculation in C++)
            for (let j = 0; j < nodes.length; j++) {
                if (i !== j) {
                    let force = nodes[i].repulsionForce(nodes[j]);  // C++ method
                    fx += force.x;
                    fy += force.y;
                }
            }
            
            // Apply forces
            nodes[i].x += fx * 0.1;
            nodes[i].y += fy * 0.1;
        }
    }
}

// Path finding algorithm
function findShortestPath(startId, endId) {
    // JavaScript orchestrates, C++ does heavy lifting
    return graph.computeShortestPath(startId, endId);
}
```

#### Phase 11.4: Native Code Generation (Week 5-6)
**Goal**: Generate and compile native code from computation graphs

**Code Generation Pipeline:**
```cpp
class ComputationCodeGen {
public:
    struct ComputeNode {
        QString id;
        QString operation;  // "add", "multiply", "sin", etc.
        QStringList inputs;
        QString output;
    };
    
    QString generateCpp(const QList<ComputeNode>& nodes) {
        QString code = "extern \"C\" double compute(";
        // Generate parameter list
        // Generate computation body
        // Generate return statement
        return code;
    }
    
    QString compileToSharedLib(const QString& cppCode) {
        // Write to temporary file
        // Invoke clang -shared -fPIC -O3
        // Return path to .so file
    }
    
    void* loadFunction(const QString& libPath, const QString& funcName) {
        // dlopen() and dlsym()
        return functionPtr;
    }
};
```

**JavaScript Integration:**
```javascript
// Define computation in JavaScript
let computeGraph = [
    {id: "input", op: "input", type: "double"},
    {id: "mult1", op: "multiply", inputs: ["input", "3.14159"]},
    {id: "sin1", op: "sin", inputs: ["mult1"]},
    {id: "output", op: "output", inputs: ["sin1"]}
];

// Compile to native code
let libPath = Compiler.generateAndCompile(computeGraph);
let nativeFunc = Compiler.loadFunction(libPath, "compute");

// Use compiled function (10-100x faster than JS)
let result = nativeFunc(inputValue);
```

### Phase 11.5: Integration & Polish (Week 6-8)
**Goal**: Complete integration with existing Qt5 application

**Features to Implement:**
- Live JavaScript console with syntax highlighting
- Error reporting and debugging tools
- Performance profiling (interpreted vs compiled)
- Undo/redo integration with JavaScript changes
- Hot-reload of compiled modules
- Documentation and examples

**UI Integration:**
- QDockWidget with JavaScript editor
- Console output panel
- Graph visualization with computed results
- Performance metrics dashboard

### Technical Benefits

**Leverages Existing Strengths:**
- ✅ Proven XML serialization system (Node::write/read, Edge::write/read)
- ✅ Robust memory management with tomb-stoning
- ✅ Observer pattern for change notifications
- ✅ Qt5 graphics system for visualization
- ✅ Socket-based connection system

**Adds New Capabilities:**
- ✅ Type-safe polymorphism without inheritance hierarchy
- ✅ JavaScript scripting for rapid algorithm development
- ✅ Native code generation for performance-critical sections
- ✅ Hot-reload capability for iterative development
- ✅ Unified API through rubber_types façades

### Success Criteria

**Phase 11.1 Success:**
- ✅ NodeFacade/EdgeFacade wrap existing classes with zero overhead
- ✅ XML serialization completely unchanged and working
- ✅ All existing tests pass with façade layer
- ✅ Compile-time type safety maintained

**Phase 11.2 Success:**
- ✅ RelationshipGraph automatically updates on XML changes
- ✅ Observer pattern integration working smoothly
- ✅ Graph queries and traversals through façades
- ✅ Performance impact < 5% overhead

**Phase 11.3 Success:**
- ✅ JavaScript can manipulate nodes and edges
- ✅ Heavy computations execute in C++ methods
- ✅ QJSEngine integration stable and performant
- ✅ Algorithm examples working (layout, pathfinding)

**Phase 11.4 Success:**
- ✅ Code generation produces compilable C++
- ✅ Dynamic loading of compiled modules working
- ✅ 10-100x performance improvement over interpreted
- ✅ Content-based caching prevents redundant compilation

**Phase 11.5 Success:**
- ✅ Professional JavaScript development environment
- ✅ Complete integration with existing Qt5 application
- ✅ Documentation and examples available
- ✅ Ready for production use

### Future Extensions

**Phase 11.6: Advanced Features**
- Multi-threading support for parallel computation
- GPU computation via OpenCL/CUDA integration
- Distributed computation across network nodes
- Advanced optimization passes in code generation

**Phase 11.7: Domain-Specific Languages**
- Visual programming interface for computation graphs
- Domain-specific function libraries (image processing, signal processing)
- Template system for common computation patterns
- Export to other platforms (Python, R, MATLAB)

### Risk Mitigation

**Technical Risks:**
- **Type erasure overhead**: Mitigated by benchmarking and small-buffer optimization
- **JavaScript integration complexity**: Mitigated by incremental development and testing
- **Code generation security**: Mitigated by sandboxing and function whitelisting
- **Memory management**: Mitigated by leveraging existing tomb-stoning system

**Schedule Risks:**
- **rubber_types learning curve**: Mitigated by starting with simple specs
- **Qt5/JavaScript debugging**: Mitigated by comprehensive logging and error handling
- **Native compilation complexity**: Mitigated by starting with simple code generation

This plan builds directly on your existing Node/Edge/Socket serialization system while adding powerful computation capabilities through rubber_types and JavaScript integration.

---

## Phase 11 Branching Strategy: Risk Mitigation Through Isolation 🔒

### Critical Safety Principle
**Every Phase 11 experiment MUST be isolated in dedicated branches to protect the stable main codebase.**

### Branch Hierarchy & Safety Gates

#### Main Protection Branch
```
main (PROTECTED - never break this)
├── Phase 11 work branches below
```

#### Phase 11.1: Type Erasure Foundation
```
feature/rubber-types-foundation
├── feature/identifiable-spec       # Basic ID wrapper
├── feature/spatial-spec            # Position/geometry wrapper  
├── feature/serializable-spec       # XML delegation wrapper
└── feature/facade-integration      # Merge all specs into NodeFacade/EdgeFacade
```

**Safety Gates:**
- Each sub-branch must compile and pass ALL existing tests
- No changes to Node/Edge/Socket classes allowed
- Must demonstrate zero impact on XML serialization
- Performance regression tests required (< 1% overhead)

#### Phase 11.2: Relationship Graph
```
feature/relationship-graph
├── feature/graph-core             # Basic RelationshipGraph class
├── feature/observer-integration   # Hook into existing observers
└── feature/graph-queries          # Query and traversal methods
```

**Safety Gates:**
- Graph updates must not interfere with existing Qt scene management
- Observer integration must not break existing UI updates
- Memory usage monitoring (should not exceed 10% increase)
- All XML round-trip tests must still pass

#### Phase 11.3: JavaScript Bridge
```  
feature/javascript-bridge
├── feature/qobject-proxies       # JsNode, JsEdge proxy classes
├── feature/qjsengine-setup       # Basic QJSEngine integration
├── feature/heavy-lifting-cpp     # C++ methods for expensive operations
└── feature/js-algorithm-examples # Force layout, pathfinding examples
```

**Safety Gates:**
- JavaScript engine isolated in separate thread if possible
- Must not affect main application startup time
- Error handling prevents JS crashes from affecting main app
- Performance budget: JS operations < 16ms for UI responsiveness

#### Phase 11.4: Native Code Generation
```
feature/native-codegen
├── feature/code-generator        # C++ code emission from computation graphs
├── feature/compilation-pipeline  # clang integration and caching
├── feature/dynamic-loading       # dlopen/dlsym hot-loading
└── feature/js-compiler-api       # JavaScript API for compilation
```

**Safety Gates:**
- Code generation runs in completely sandboxed environment
- Compilation failures must not crash main application
- Generated .so files isolated in temporary directories
- Security: only whitelisted functions allowed in generated code

#### Phase 11.5: Integration & Polish
```
feature/phase11-integration
├── feature/js-console-ui         # QDockWidget JavaScript console
├── feature/performance-profiling # Benchmarking and metrics
├── feature/undo-integration      # JavaScript changes trigger undo system
└── feature/documentation         # Examples and user guides
```

### Branch Management Protocol

#### Development Workflow
```bash
# 1. Start each phase from clean main
git checkout main
git pull origin main
git checkout -b feature/rubber-types-foundation

# 2. Work in isolation
# ... implement and test ...

# 3. Validate before any merge
./build.sh debug       # Must build successfully
./build.sh release     # Must build successfully  
# Run full test suite
# Run performance benchmarks
# Verify XML serialization unchanged

# 4. Only merge when ALL safety gates pass
git checkout main
git merge feature/rubber-types-foundation
git push origin main

# 5. Delete merged branch
git branch -d feature/rubber-types-foundation
```

#### Emergency Rollback Plan
```bash
# If ANY phase breaks main, immediate rollback
git revert <commit-hash>
git push origin main

# Return to last known good state
git reset --hard <last-good-commit>
git push --force-with-lease origin main
```

### Validation Requirements Per Phase

#### Phase 11.1 Validation Checklist
- [ ] All existing unit tests pass
- [ ] XML serialization byte-for-byte identical
- [ ] Zero measurable performance regression
- [ ] Façades compile with -Werror -Wall
- [ ] Memory usage unchanged (Valgrind check)
- [ ] Works with existing Qt5 scene management

#### Phase 11.2 Validation Checklist  
- [ ] Observer pattern integration doesn't break UI
- [ ] Graph queries return consistent results
- [ ] No memory leaks in relationship tracking
- [ ] Performance impact < 5% for typical operations
- [ ] Undo/redo still works correctly
- [ ] Graph updates don't interfere with Qt rendering

#### Phase 11.3 Validation Checklist
- [ ] JavaScript errors don't crash main application
- [ ] QJSEngine properly sandboxed and isolated
- [ ] C++ heavy lifting methods thoroughly tested
- [ ] JS algorithm examples produce correct results
- [ ] Memory management between C++ and JS sound
- [ ] Error reporting and debugging functional

#### Phase 11.4 Validation Checklist
- [ ] Code generation produces compilable, secure C++
- [ ] Compilation pipeline properly sandboxed
- [ ] Dynamic loading doesn't introduce security vulnerabilities
- [ ] Generated code performance meets 10x improvement target
- [ ] Caching system prevents redundant compilation
- [ ] Failure modes handled gracefully

#### Phase 11.5 Validation Checklist
- [ ] UI integration doesn't affect main application performance
- [ ] JavaScript console stable and user-friendly
- [ ] Performance profiling accurate and helpful
- [ ] Undo integration maintains consistency
- [ ] Documentation complete and tested
- [ ] Ready for production deployment

### Risk Assessment Matrix

| Risk Level | Criteria | Action Required |
|------------|----------|-----------------|
| 🟢 **LOW** | No core changes, façade only | Standard branch + tests |
| 🟡 **MEDIUM** | Observer integration, new UI | Extended testing + performance |
| 🟠 **HIGH** | JavaScript integration, new dependencies | Comprehensive security review |
| 🔴 **CRITICAL** | Code generation, dynamic loading | Full security audit + sandboxing |

### Fallback Strategy

**If Phase 11 Proves Too Risky:**
- Each phase implemented as optional compile-time feature
- CMake flags to disable experimental features:
  ```cmake
  option(ENABLE_RUBBER_TYPES "Enable rubber_types façades" OFF)
  option(ENABLE_JAVASCRIPT "Enable JavaScript bridge" OFF) 
  option(ENABLE_CODEGEN "Enable native code generation" OFF)
  ```
- Main application remains fully functional without experimental features
- Allows gradual adoption based on stability and user feedback

### Success Metrics for Branch Strategy

**Technical Metrics:**
- Zero regressions introduced to main branch
- All experimental branches build and test successfully  
- Performance impact contained within defined budgets
- Security vulnerabilities identified and mitigated

**Process Metrics:**
- Average time from feature branch creation to safe merge
- Number of emergency rollbacks required (target: 0)
- Test coverage maintained above 80% throughout
- Documentation keeps pace with implementation

This branching strategy ensures that your stable, working Node/Edge/Socket system remains protected while allowing aggressive experimentation with rubber_types and JavaScript integration.

---

## Phase 11 Implementation Progress 🚀

### ✅ Phase 11.1: Type-Erasure Foundation (COMPLETED)
**Branch**: `feature/node-edge-facades` (pushed to remote)

**Achievements:**
- ✅ Created custom type-erasure system (NodeFacade, EdgeFacade)
- ✅ Zero external dependencies - no rubber_types library needed
- ✅ Separate clean header files: `node_facade.h`, `edge_facade.h`
- ✅ Comprehensive testing with real XML files
- ✅ Build integration with CMake optional flags
- ✅ Simplified Edge visualization (removed arrow heads)
- ✅ All safety gates passed - no impact on existing code

**Files Created:**
- `node_facade.h` - NodeFacade type-erasure (58 lines)
- `edge_facade.h` - EdgeFacade type-erasure (48 lines) 
- `test_facade_core.cpp` - Console test suite
- `test_simple_facades.cpp` - GUI test suite
- Updated CMakeLists.txt with BUILD_FACADE_TESTS option

**Technical Validation:**
- ✅ Type-erasure pattern working: Concept + Model + Container
- ✅ Uniform interfaces provide polymorphic storage
- ✅ XML serialization identical to direct Node/Edge serialization
- ✅ Zero runtime overhead - same performance as virtual inheritance
- ✅ Move semantics for safe memory management

**Next Steps:**
- Ready to proceed to Phase 11.2: Relationship Graph Layer
- Feature branch available on remote for multi-machine development
- Foundation established for JavaScript computation engine

**Impact:**
- Provides building blocks for computation engine
- Clean abstraction layer over existing Node/Edge classes
- Safe experimentation environment maintained
- Path forward to JavaScript integration clear

---

## Phase 12: Execution Orchestrator & Capability Architecture 🎯

### 📍 Current Status
**Branch**: `feature/execution-orchestrator` (created August 1, 2025)  
**Base**: `main` branch with latest visual socket improvements  
**Status**: 🔄 **In Progress** - Design and initial implementation

### 🎯 Architectural Vision

Transform the current per-node JavaScript execution into a graph-theoretic computation pipeline with capability-based architecture, lazy evaluation, and multi-language code generation.

#### Current State (Building Upon)
- ✅ **Solid UI Layer**: Ghost edge flow, Qt painting, observer pattern
- ✅ **Graph Representation**: Scene, Node, Edge with UUID-based lookups  
- ✅ **JavaScript Engine**: Per-node script execution via `JavaScriptEngine`
- ✅ **Unified Facade System**: `graph_facades.h` with type-erasure patterns
- ✅ **Edge Deletion Hardening**: Single deletion path with UUID fallback
- ✅ **Observer Pattern**: `GraphSubject` with change notifications

#### Target Architecture (What We're Building)
- 🎯 **ExecutionOrchestrator**: Graph-level computation scheduling with topological ordering
- 🎯 **Capability-Based Design**: ExecutableSpec + CodegenSpec as rubber types capabilities
- 🎯 **Lazy Evaluation**: Memoized computation with dependency tracking (`node_id, input_hash`)
- 🎯 **Code Generation Pipeline**: Graph → JSON IR → Python/C++/JS backends
- 🎯 **Legacy Integration**: Seamless adapters for existing JavaScript engine

### 🏗️ Implementation Phases

#### Phase 12.1: Core Execution Infrastructure (High Priority)
**Goal**: Graph-level execution orchestration with existing JavaScript integration

**Components:**
1. **ExecutableSpec Interface**
   ```cpp
   struct ExecutableSpec {
     virtual QVariantMap execute(const QVariantMap& inputs) = 0;
   };
   ```
   - Rubber types capability for node execution
   - Delegate to existing `JavaScriptEngine::executeNodeScript`
   - Qt-native I/O via `QVariantMap`

2. **ExecutionOrchestrator**
   ```cpp
   class ExecutionOrchestrator {
     void scheduleRecompute(const QSet<QUuid>& affectedNodes);
     void invalidateDownstream(const QUuid& changedNodeId);  
     QVariantMap executeNode(const QUuid& nodeId, const QVariantMap& inputs);
   };
   ```
   - Topological ordering for DAG execution
   - Lazy evaluation with memoization
   - Integration with existing observer pattern

3. **Observer Integration**
   ```cpp
   // Hook into existing pattern:
   GraphSubject::notifyEdgeCreated(edgeId) 
   → ExecutionOrchestrator::scheduleRecompute(affectedSubgraph)
   
   Scene::finishGhostEdge() 
   → ExecutionOrchestrator::invalidateDownstream(newEdge)
   ```

**Files:**
- `execution_orchestrator.h/cpp` - Core orchestration logic
- `executable_spec.h` - Rubber types capability interface
- `legacy_javascript_adapter.h/cpp` - Integration with existing engine
- `test_execution_orchestrator.cpp` - Comprehensive test suite

#### Phase 12.2: Code Generation Pipeline (Medium Priority)
**Goal**: Multi-language code emission from visual graphs

**Components:**
1. **CodegenSpec Interface**
   ```cpp
   struct CodegenSpec {
     virtual CodeChunk emit(CodeTarget target) const = 0;
   };
   ```
   - Per-node code chunk emission
   - JSON IR as first backend target
   - Foundation for language-specific backends

2. **Code Assembly Pipeline**
   ```cpp
   class CodegenPipeline {
     QString emitWholeGraph(CodeTarget target);
     void assembleCodeChunks(const QList<CodeChunk>& chunks);
   };
   ```
   - Topological traversal for code generation
   - Import/Definition/Body assembly policies
   - Multiple language target support

**Files:**
- `codegen_spec.h` - Code generation capability interface
- `codegen_pipeline.h/cpp` - Whole-graph code assembly
- `json_ir_backend.h/cpp` - JSON intermediate representation
- `test_codegen_pipeline.cpp` - Code generation testing

#### Phase 12.3: Advanced Features (Lower Priority)
**Goal**: Performance optimization and multi-language backends

**Components:**
1. **Memoization System**
   - Cache keyed by `(node_id, input_hash)`
   - Invalidation on script/connection changes
   - Performance optimization for UI interactions

2. **Multi-Language Backends**
   - Python code generation with dependency management
   - C++ code generation with compilation integration
   - Whole-graph assembly with proper imports/exports

**Files:**
- `memoization_cache.h/cpp` - Smart caching system
- `python_backend.h/cpp` - Python code generation
- `cpp_backend.h/cpp` - C++ code generation

### 🔧 Integration Strategy

#### Minimal Disruption Approach
- **UI Code**: Unchanged (ghost edges, Qt painting preserved)
- **Edge Validation**: Unchanged (`Edge::resolveConnections` preserved)
- **Existing JavaScript**: Wrapped, not replaced
- **Observer Pattern**: Extended, not modified

#### Legacy Adapters
```cpp
class LegacyNodeExecutableAdapter {
  // Wraps existing JavaScriptEngine::executeNodeScript
  QVariantMap execute(const QVariantMap& inputs) override;
};

class LegacyNodeCodegenAdapter {  
  // Basic IR generation until native backends exist
  CodeChunk emit(CodeTarget target) const override;
};
```

### 🎯 Success Criteria

#### Phase 12.1 Complete When:
- [ ] ExecutableSpec can execute JavaScript nodes via existing engine
- [ ] ExecutionOrchestrator schedules DAG execution topologically  
- [ ] Observer pattern triggers orchestrator recomputation correctly
- [ ] All existing functionality preserved (zero regressions)
- [ ] Comprehensive test coverage validates orchestration logic

#### Phase 12.2 Complete When:
- [ ] CodegenSpec emits JSON IR for simple nodes
- [ ] Legacy adapters provide seamless JavaScript integration
- [ ] Basic whole-graph code generation pipeline operational
- [ ] At least one language backend (Python/C++) generates working code

#### Full Phase 12 Complete When:
- [ ] Graph changes trigger appropriate execution updates automatically
- [ ] Memoization improves performance during UI interactions measurably
- [ ] Multiple language backends generate production-ready code
- [ ] Execution orchestrator handles complex DAGs and cyclic graphs
- [ ] Documentation and migration guides complete

### 🔍 Technical Design Principles

#### 1. Capability-Based Architecture
- Single rubber entity with optional ExecutableSpec/CodegenSpec
- No class proliferation or inheritance explosion
- Mix/match capabilities per node type

#### 2. Graph-Theoretic Foundation  
- Topological ordering for deterministic execution
- Lazy evaluation for UI responsiveness
- Explicit dependency tracking and invalidation

#### 3. Performance Optimization
- Memoization keyed by content hash
- O(affected_nodes) invalidation, not O(total_nodes)
- Background computation with UI thread safety

#### 4. Future-Proof Extension
- Clean separation: Graph logic ↔ Code generation
- JSON IR enables unlimited language targets
- Modular backend system for extensibility

### 📊 Risk Mitigation

#### Complexity Management
- **Phased Implementation**: Core → Generation → Advanced
- **Legacy Adapters**: Gradual migration, no breaking changes
- **Comprehensive Testing**: Each phase validated independently

#### Performance Safeguards
- **Lazy Evaluation**: Prevent unnecessary computation
- **Smart Invalidation**: Only recompute affected subgraphs
- **UI Thread Safety**: Orchestrator operations off main thread

#### Integration Safety
- **Observer Hooks**: Build on existing infrastructure
- **Qt-Native Types**: `QVariantMap` for seamless integration
- **Backward Compatibility**: All existing APIs preserved

### 📁 Implementation Tracking

**Todo System**: All work tracked via TodoWrite tool
**Branch Management**: `feature/execution-orchestrator` for all development
**Regular Checkpoints**: Progress documented in PLANS.md updates
**Testing Strategy**: Test-driven development with comprehensive coverage

This phase represents the natural evolution from static graph representation to dynamic computation pipeline, building upon all previous phases while maintaining architectural integrity and system stability.