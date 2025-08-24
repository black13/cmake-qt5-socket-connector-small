# NodeGraph Core Architecture - JavaScript-Free

## Project Vision

**A professional Qt5/C++ node graph editor focused on rubber types and extensible node processing**

### What This Project IS:
- ✅ **Self-Serializing Node Graph Editor**: Each node/edge serializes itself to XML
- ✅ **Qt5 Professional UI**: Drag-and-drop palette, scene management, proper event handling
- ✅ **XML-First Persistence**: Live XML synchronization, save/load workflows
- ✅ **Observer Pattern**: Automatic change notification and XML updates
- ✅ **Rubber Types System**: Flexible type checking and node compatibility
- ✅ **Extensible Architecture**: Template-driven node creation, registry system

### What This Project is NOT:
- ❌ **JavaScript-based**: No JavaScript engine, no scripting, no QJSEngine
- ❌ **Runtime Scripting Platform**: Focus on compiled C++ behavior
- ❌ **General Purpose IDE**: Specialized for node graph workflows

## Core Architecture Overview

```
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│     Window      │    │      View       │    │     Scene       │
│   (Main UI)     │◄──►│  (Qt Graphics)  │◄──►│ (Node Storage)  │
└─────────────────┘    └─────────────────┘    └─────────────────┘
         │                       │                       │
         ▼                       ▼                       ▼
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│  Node Palette   │    │   Graph Items   │    │  Graph Factory  │
│   (Drag-Drop)   │    │  (Nodes/Edges)  │    │  (XML + Types)  │
└─────────────────┘    └─────────────────┘    └─────────────────┘
                                │                       │
                                ▼                       ▼
                       ┌─────────────────┐    ┌─────────────────┐
                       │  Rubber Types   │    │   Observer      │
                       │   (Type Sys)    │    │  (Auto-sync)    │
                       └─────────────────┘    └─────────────────┘
```

## Key Components

### 1. Graphics Layer (Qt5)
- **Window**: Main application window, menus, toolbars
- **View**: QGraphicsView for pan/zoom/selection
- **Scene**: QGraphicsScene managing all graph items
- **Items**: Node, Edge, Socket (all QGraphicsItem subclasses)

### 2. Data Layer (XML-First)
- **GraphFactory**: Creates nodes/edges from XML templates
- **Self-Serialization**: Each object knows how to save/load itself
- **Live XML**: Real-time synchronization between graph and XML
- **NodeRegistry**: Type-safe node creation system

### 3. Type System (Rubber Types)
- **RubberType**: Flexible type checking for node compatibility
- **Type Checking**: Socket connection validation
- **Template System**: Define node types with input/output specifications

### 4. Extensibility (C++ Only)
- **Observer Pattern**: Auto-sync changes to XML
- **Template-Driven**: Create new node types via C++ templates
- **Registry System**: Dynamic node type registration

## Next Development Phase: Rubber Types Integration

### Priority 1: Core Rubber Types
1. **Integrate rubber_types/ directory** into main build
2. **Add rubber types to NodeRegistry** for type-safe creation
3. **Implement type checking** in socket connections
4. **Update Node Palette** with rubber type categories

### Priority 2: Type-Safe Operations  
1. **Socket compatibility checking** before connections
2. **Visual type indicators** on nodes and sockets
3. **Type error reporting** in UI
4. **Rubber type XML serialization**

### Priority 3: Extensible Type System
1. **Template-based type definitions** in C++
2. **Runtime type registration** via NodeRegistry
3. **Custom type categories** in palette
4. **Type inheritance hierarchies**

## Clean Codebase Goals

✅ **Zero JavaScript Dependencies**: No QJSEngine, QJSValue, or script files  
✅ **Consolidated Documentation**: Single architecture document  
✅ **Clear Build Process**: Standard CMake with Qt5 + libxml2  
✅ **Focused Scope**: Professional node editor with rubber types  
✅ **Maintainable Code**: Proper Qt patterns, RAII, observer pattern  

---

*This document replaces all scattered planning documents and defines the definitive project direction.*