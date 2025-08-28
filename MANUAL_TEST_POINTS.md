# Manual Test Points for NodeGraph Application

## Build Verification ✅

### Debug Build
- **Status**: ✅ PASS
- **Executable**: `build_linux/NodeGraph` (6.7MB)
- **Libraries**: Successfully linked with Qt5 Debug and libxml2

### Release Build  
- **Status**: ✅ PASS
- **Executable**: `build_linux/NodeGraph` (541KB)
- **Libraries**: Successfully linked with Qt5 Release and libxml2

## Core Application Test Points

### 1. Application Startup
**Test**: Basic application launch and initialization
```bash
cd build_linux
export DISPLAY=:0  # if using X11 server
./NodeGraph
```

**Expected Results**:
- [ ] Application window opens without crashes
- [ ] Main window title shows "Node Editor"
- [ ] Menu bar displays: File, Edit, View, Tools, Help
- [ ] Node palette widget visible on left side
- [ ] Empty canvas in center with grid/background
- [ ] Status bar shows basic application info

### 2. Node Creation and Manipulation

#### 2.1 Node Palette Functionality
**Test**: Node creation from palette
- [ ] Node palette displays available node types (IN, OUT, PROC, SOURCE, SINK, TRANSFORM, MERGE, SPLIT, PROCESSOR)
- [ ] Double-click on node type creates node at center of view
- [ ] Each node type creates visually distinct nodes

#### 2.2 Node Selection and Movement
**Test**: Basic node interaction
- [ ] Click on node selects it (visual selection indicator)
- [ ] Drag node to move it around canvas
- [ ] Multiple nodes can be selected (Ctrl+click)
- [ ] Node positions persist during drag operations

#### 2.3 Node Properties
**Test**: Node identification and properties
- [ ] Each created node has unique UUID (check debug logs)
- [ ] Nodes display their type visually
- [ ] Node sockets are visible and properly positioned

### 3. Socket System

#### 3.1 Socket Visibility
**Test**: Socket rendering and identification
- [ ] Input sockets visible on left side of nodes
- [ ] Output sockets visible on right side of nodes
- [ ] Sockets have visual indicators (different from node body)
- [ ] Socket count varies by node type if applicable

#### 3.2 Socket Interaction
**Test**: Socket connection preparation
- [ ] Right-click on output socket starts ghost edge
- [ ] Ghost edge follows mouse cursor
- [ ] Ghost edge changes color when hovering over valid input socket
- [ ] Ghost edge shows red when hovering over invalid targets

### 4. Edge Connection System

#### 4.1 Basic Edge Creation
**Test**: Socket-to-socket connections
- [ ] Right-click on output socket, drag to input socket creates edge
- [ ] Edges are curved (Bezier curves) between sockets
- [ ] Edges have proper visual styling
- [ ] Multiple edges can connect different socket pairs

#### 4.2 Edge Validation
**Test**: Connection rules enforcement
- [ ] Output socket can only connect to input socket
- [ ] Cannot connect socket to itself
- [ ] Cannot connect sockets on same node
- [ ] Invalid connection attempts are rejected visually

#### 4.3 Edge Management
**Test**: Edge lifecycle management
- [ ] Connected edges move with their parent nodes
- [ ] Edges update visual path when nodes are moved
- [ ] Edges can be selected and deleted
- [ ] Edge deletion doesn't crash application

### 5. XML Persistence System

#### 5.1 File Save Operations
**Test**: Graph serialization to XML
- [ ] Create a simple graph (2-3 nodes, 1-2 edges)
- [ ] Ctrl+S opens save dialog
- [ ] Save to XML file completes without errors
- [ ] Generated XML file is valid and readable

#### 5.2 File Load Operations  
**Test**: Graph deserialization from XML
- [ ] File → Open loads existing XML files
- [ ] Loaded graph recreates nodes at correct positions
- [ ] Loaded graph recreates edges with correct connections
- [ ] Node properties (types, UUIDs) are preserved

#### 5.3 Command Line Loading
**Test**: XML file loading via command line
```bash
./NodeGraph test.xml
# or
./NodeGraph --load test.xml
```
- [ ] Application loads with specified XML file
- [ ] Graph displays correctly on startup
- [ ] Command line errors handled gracefully

### 6. Memory Management and Stability

#### 6.1 Node/Edge Deletion
**Test**: Resource cleanup verification
- [ ] Select node and press Delete key
- [ ] Node disappears from canvas
- [ ] Connected edges are automatically removed
- [ ] No memory leaks or dangling references (check debug logs)

#### 6.2 Graph Clearing
**Test**: Complete graph reset
- [ ] Create complex graph with multiple nodes/edges
- [ ] Use clear/new operation
- [ ] All nodes and edges removed
- [ ] Canvas returns to clean state
- [ ] Memory usage returns to baseline

#### 6.3 Application Shutdown
**Test**: Clean application termination
- [ ] Close application via X button
- [ ] Close application via File → Exit
- [ ] Application terminates without crashes
- [ ] No error messages in console/logs

### 7. Error Handling and Recovery

#### 7.1 File Operations
**Test**: Invalid file handling
- [ ] Try loading non-existent file
- [ ] Try loading invalid XML file
- [ ] Try loading XML with corrupted node data
- [ ] Application shows appropriate error messages
- [ ] Application remains stable after errors

#### 7.2 User Interface Stress
**Test**: UI robustness
- [ ] Create many nodes (20+) without performance degradation
- [ ] Rapidly create and delete nodes
- [ ] Test edge creation/deletion under stress
- [ ] UI remains responsive during operations

## Log File Verification

### 8.1 Debug Logging
**Test**: Application logging system
- [ ] Check `logs/` directory for timestamped log files
- [ ] Verify node creation events are logged
- [ ] Verify edge connection events are logged
- [ ] Verify XML save/load operations are logged
- [ ] No critical errors or warnings in logs

## Architecture Verification

### 9.1 Non-QObject Design
**Test**: Value semantics verification
- [ ] Application runs without QObject connection warnings
- [ ] Node deletion doesn't produce zombie reference errors
- [ ] Edge connections remain stable during node operations

### 9.2 Performance Characteristics
**Test**: O(1) lookup verification
- [ ] Node selection remains fast with many nodes
- [ ] Edge lookup operations are responsive
- [ ] UUID-based operations perform consistently

## Test XML Files ✅ **GENERATED**

### 📊 **Available Test Files** (Generated by `generate_test_files.py`)

| File | Nodes | Edges | Size | Purpose |
|------|-------|-------|------|---------|
| `simple_test.xml` | 2 | 1 | 350B | Basic manual test |
| `complex_test.xml` | 5 | 5 | 801B | Multi-node manual test |
| `tests_tiny.xml` | 10 | 9 | 2.8KB | Quick automated test |
| `tests_small.xml` | 100 | 99 | 28.8KB | Small load test |
| `tests_medium.xml` | 500 | 499 | 144.8KB | Medium performance test |
| `tests_large.xml` | 1000 | 999 | 289.9KB | Large graph test |
| `tests_stress.xml` | 5000 | 4999 | 1.4MB | Stress/performance test |

### 🔧 **Test File Features**
- **Proper XML Schema**: Follows NodeGraph XML format exactly
- **Valid Node Types**: SOURCE, SINK, TRANSFORM, MERGE, SPLIT, IN, OUT, PROC
- **Correct Socket Indexing**: Input sockets (0, 1, ...), Output sockets (inputCount, inputCount+1, ...)
- **Grid Layout**: Organized positioning for visual clarity
- **UUID Format**: Proper braced UUID format `{uuid}`
- **Valid Connections**: Only connects compatible sockets (output → input)

### 📋 **Usage Examples**
```bash
# Windows
NodeGraph.exe simple_test.xml
NodeGraph.exe tests_tiny.xml
NodeGraph.exe tests_stress.xml

# WSL/Linux
./NodeGraph simple_test.xml
./NodeGraph tests_medium.xml
export DISPLAY=:0 && ./NodeGraph tests_large.xml
```

## Success Criteria

- [ ] All startup tests pass
- [ ] Node creation/manipulation works correctly
- [ ] Edge connection system functions properly
- [ ] XML save/load preserves graph integrity
- [ ] No crashes during normal operation
- [ ] Memory management is stable
- [ ] Error handling is graceful
- [ ] Performance remains acceptable

## Known Issues to Monitor

1. **JavaScript Test Failures**: Tests still reference JavaScript engine - these are expected to fail
2. **Compiler Warnings**: Some null pointer warnings in graph_factory.cpp - monitor for runtime issues
3. **AutoMoc Warnings**: MOC warnings about missing Q_OBJECT macros - cosmetic only

## Test Environment

- **Platform**: Linux/WSL2
- **Qt Version**: 5.15.17
- **Build Type**: Both Debug and Release verified
- **Dependencies**: libxml2, Qt5 Core/Widgets/Gui