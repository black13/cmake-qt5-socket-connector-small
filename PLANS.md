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