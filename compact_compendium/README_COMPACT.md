# NodeGraph Compact Code Review

This is a focused code review package containing only the essential core files.

## Core Files Included

### Application Architecture
- `main.cpp` - Application entry point and initialization
- `window.h/cpp` - Main window and UI management

### Node Graph System  
- `scene.h/cpp` - Graphics scene managing nodes and edges
- `node.h/cpp` - Node implementation with socket management
- `socket.h/cpp` - Socket connection system
- `edge.h/cpp` - Edge connections between nodes

### JavaScript Integration
- `javascript_engine.h/cpp` - JavaScript engine for testing
- `graph_controller.h/cpp` - C++ to JavaScript bridge

### Build System
- `CMakelists.txt` - CMake build configuration

## Key Features Demonstrated

1. **Professional Qt5 Architecture** - Clean separation of concerns
2. **Socket Connection System** - Right-click drag functionality
3. **JavaScript Integration** - C++ to JS bridge for testing
4. **XML Serialization** - Proper socket count persistence
5. **Clean Code** - Professional logging without visual noise

## Total: ~8,000 lines of focused, production-ready code

This compact package demonstrates the core architecture and key innovations 
without overwhelming reviewers with auxiliary files.
