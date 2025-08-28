# 🏗️ NodeGraph Program Structure Analysis

## 🎯 **Core Architecture Overview**

The NodeGraph application follows a **Non-QObject**, **XML-First**, **Observer Pattern** architecture designed to avoid Qt's parent-child ownership issues while maintaining clean separation of concerns.

## 📊 **Architecture Layers**

```
┌─────────────────────────────────────────────────────────┐
│                    PRESENTATION LAYER                   │
├─────────────────────────────────────────────────────────┤
│  Window (QMainWindow) ← Scene (QGraphicsScene) ← View  │
│       ↓                                                 │
│  NodePaletteWidget ← NodeButton × N                    │
└─────────────────────────────────────────────────────────┘
┌─────────────────────────────────────────────────────────┐
│                    BUSINESS LOGIC LAYER                 │
├─────────────────────────────────────────────────────────┤
│  GraphFactory ← XML Document (libxml2)                 │
│       ↓                                                 │
│  NodeRegistry ← Node Types (9 registered)              │
│       ↓                                                 │
│  Node ← Socket ← Edge (Non-QObject entities)           │
└─────────────────────────────────────────────────────────┘
┌─────────────────────────────────────────────────────────┐
│                    PERSISTENCE LAYER                    │
├─────────────────────────────────────────────────────────┤
│  XML Autosave Observer ← Graph Observer (Manual)       │
│       ↓                                                 │
│  libxml2 Documents ← File I/O                          │
└─────────────────────────────────────────────────────────┘
```

## 🧩 **Core Components**

### **1. Main Application Components**

| Component | Type | Purpose | Key Features |
|-----------|------|---------|--------------|
| **Window** | `QMainWindow` | Main application window | Menu bar, toolbars, central widget |
| **Scene** | `QGraphicsScene` | Canvas for nodes/edges | Qt Graphics framework integration |
| **View** | `QGraphicsView` | Viewport for scene | Zoom, pan, selection handling |
| **NodePaletteWidget** | `QWidget` | Node creation interface | 5 node type buttons, drag creation |

### **2. Core Graph Entities** (Non-QObject)

| Entity | Inheritance | Purpose | Socket Config |
|--------|-------------|---------|---------------|
| **Node** | `QGraphicsItem` | Graph node representation | Variable inputs/outputs |
| **Edge** | `QGraphicsItem` | Connection between nodes | Bezier curve rendering |
| **Socket** | `QGraphicsItem` | Connection points | Input (left) / Output (right) |
| **GhostEdge** | `QGraphicsItem` | Temporary connection visual | Mouse drag feedback |

### **3. Registry & Factory Pattern**

| Component | Pattern | Purpose | Capacity |
|-----------|---------|---------|----------|
| **NodeRegistry** | Singleton | Node type factory registry | 9 registered types |
| **GraphFactory** | Factory | XML ↔ Graph conversion | Single XML authority |
| **NodeButton** | Factory Trigger | UI → Node creation | Palette integration |

### **4. Observer System** (Manual, Non-Qt)

| Component | Role | Purpose | Lifecycle |
|-----------|------|---------|-----------|
| **GraphObserver** | Base Observer | Change notification interface | Manual attach/detach |
| **XMLAutosaveObserver** | Concrete Observer | Auto-save on changes | 750ms intervals |
| **GraphSubject** | Observable | Change event dispatcher | 1 observer tracked |

## 🔧 **Node Type System**

### **Registered Node Types** (9 total)
```cpp
// Core Types (3)
"IN"        - Input node
"OUT"       - Output node  
"PROC"      - Processing node

// Palette Types (5)
"SOURCE"    - Data source (0 inputs, 1 output)
"SINK"      - Data sink (1 input, 0 outputs)
"TRANSFORM" - Data transformer (1 input, 1 output)
"MERGE"     - Data merger (2 inputs, 1 output)
"SPLIT"     - Data splitter (1 input, 2 outputs)

// Legacy Type (1)
"PROCESSOR" - Legacy processing node
```

### **Socket Indexing System**
```
Node Socket Layout:
┌─────────────────┐
│  [0] ← INPUT    │  Input sockets: 0, 1, 2, ...
│  [1] ← INPUT    │  
│      NODE       │  
│      OUTPUT → [N]   │  Output sockets: inputCount, inputCount+1, ...
│      OUTPUT → [N+1] │  
└─────────────────┘
```

## 🏛️ **Architectural Patterns**

### **1. Non-QObject Pattern**
**Purpose**: Avoid Qt parent-child ownership issues
```cpp
// Traditional Qt (AVOIDED)
class Node : public QObject { /* zombie reference risk */ }

// NodeGraph approach (USED)
class Node : public QGraphicsItem { /* value semantics, manual cleanup */ }
```

**Benefits**:
- ✅ No zombie references
- ✅ Predictable memory management
- ✅ No signal/slot connection issues
- ✅ Clean shutdown (0 objects remaining)

### **2. XML-First Architecture**
**Purpose**: Single source of truth for graph state
```cpp
// GraphFactory is the ONLY component that touches XML
GraphFactory factory(scene, xmlDoc);
factory.loadFromXmlFile(filename);  // Single authority
```

**Benefits**:
- ✅ Consistent state management
- ✅ Reliable save/load operations
- ✅ No state synchronization issues

### **3. Observer Pattern (Manual)**
**Purpose**: Change tracking without Qt signals/slots
```cpp
// Manual registration (not Qt connect())
GraphSubject::instance().attach(observer);
GraphSubject::instance().detach(observer);  // Explicit cleanup
```

**Benefits**:
- ✅ No connect/disconnect issues
- ✅ Explicit observer lifecycle
- ✅ Clean shutdown tracking

## 📁 **File Organization**

### **Core Files**
- `main.cpp` - Application entry, node registration, logging
- `window.h/cpp` - Main window, menus, central layout
- `scene.h/cpp` - Graphics scene, node/edge management  
- `view.h/cpp` - Graphics view, zoom/pan/selection

### **Graph Entities**
- `node.h/cpp` - Core node implementation
- `edge.h/cpp` - Edge connections, Bezier curves
- `socket.h/cpp` - Connection points, visual feedback
- `ghost_edge.h/cpp` - Temporary connection visual

### **System Components**
- `graph_factory.h/cpp` - XML ↔ Graph conversion
- `node_registry.h/cpp` - Node type factory registry
- `node_palette_widget.h/cpp` - UI palette, node buttons
- `graph_observer.h/cpp` - Observer pattern implementation

### **Persistence**
- `xml_autosave_observer.h/cpp` - Auto-save functionality
- `graph_item_types.h` - Type enums and interfaces

## 🎯 **Key Design Decisions**

### **1. Why Non-QObject?**
**Problem**: Qt parent-child ownership creates zombie references
**Solution**: Manual memory management with value semantics
**Result**: Clean shutdown, no memory leaks

### **2. Why XML-First?**
**Problem**: Multiple components modifying state = inconsistency  
**Solution**: GraphFactory as single XML authority
**Result**: Reliable persistence, no state conflicts

### **3. Why Manual Observer Pattern?**
**Problem**: Qt signals/slots can create connection issues
**Solution**: Explicit attach/detach observer management
**Result**: Predictable change tracking, clean lifecycle

## 📈 **Performance Characteristics**

- **Node Lookup**: O(1) via UUID-based hash maps
- **Edge Rendering**: Optimized Bezier curves
- **Memory Management**: Manual cleanup, no Qt overhead  
- **Startup Time**: ~430ms (5 node types registered)
- **Shutdown Time**: Clean (0 objects remaining)

## 🔮 **Extension Points**

### **Adding Node Types**
```cpp
// 1. Register in NodeRegistry
NodeRegistry::instance().registerNode("CUSTOM", []() {
    Node* node = new Node();
    node->setNodeType("CUSTOM");
    return node;
});

// 2. Add to palette templates (optional)
// 3. Implement custom behavior (optional)
```

### **Adding Observers**
```cpp
// 1. Inherit from GraphObserver
class CustomObserver : public GraphObserver {
    void notify() override { /* custom behavior */ }
};

// 2. Manual registration
GraphSubject::instance().attach(observer);
```

This architecture provides a **rock-solid foundation** for node graph editing with clean separation of concerns and predictable behavior.