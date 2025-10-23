# Development Rules for Qt NodeGraph System

## 🔧 **MANDATORY: Windows Build System Requirements**

### Visual Studio Debugger PATH Configuration

**ALL Windows development MUST ensure CMake configures the Qt PATH for Visual Studio debugging.**

#### Required CMakeLists.txt Configuration (Lines 321-324)

```cmake
set_target_properties(NodeGraph PROPERTIES
    VS_DEBUGGER_ENVIRONMENT_DEBUG
        "PATH=D:/Qt-5.15.17-msvc142-x64-Debug/msvc2019_64/bin;%PATH%"
    VS_DEBUGGER_ENVIRONMENT_RELEASE
        "PATH=D:/Qt-5.15.17-msvc142-x64-Release/msvc2019_64/bin;%PATH%"
)
```

#### Why This Matters

- ✅ Ensures Qt DLLs are found when debugging in Visual Studio
- ✅ Eliminates "Qt5Core.dll not found" runtime errors
- ✅ Consistent behavior across all developer machines
- ✅ No manual PATH configuration required

#### Verification Steps

1. After any CMakeLists.txt changes, run `build.bat debug`
2. Verify `build_Debug\NodeGraph.vcxproj.user` exists
3. Check that it contains `<LocalDebuggerEnvironment>PATH=...Qt.../bin;%PATH%</LocalDebuggerEnvironment>`

#### When Adapting to New Machines

1. Update Qt paths in CMakeLists.txt (lines 36-38 for CMAKE_PREFIX_PATH AND lines 321-324 for VS_DEBUGGER_ENVIRONMENT)
2. Run `build.bat debug` to regenerate project files
3. Never rely on system PATH or manual environment variables

**This configuration is NON-NEGOTIABLE for all Windows development work.**

---

## Branch: fix/proper-delete-key-event-routing

### Purpose
Fix the fundamental architectural flaw where delete key events are handled by external objects (Window/Scene) with loops and type checking, instead of letting each QGraphicsItem handle its own deletion.

### Core Rules for This Branch

#### 1. Event Routing Rules
- ✅ **Each QGraphicsItem handles its own keyPressEvent**
- ❌ **NO external objects deciding what to delete**
- ❌ **NO loops over selectedItems() for deletion**
- ✅ **Delete key goes directly to the selected item**

#### 2. Object Lifecycle Rules  
- ✅ **Objects manage their own deletion**
- ❌ **NO external deletion management**
- ✅ **Self-contained cleanup in destructors**

#### 3. Type System Rules
- ❌ **NO qgraphicsitem_cast anywhere**
- ❌ **NO type checking loops**
- ❌ **NO "figure out what type this is" logic**
- ✅ **Objects know what they are intrinsically**

#### 4. Qt Architecture Rules
- ✅ **Use proper Qt event propagation**
- ✅ **QGraphicsItem::ItemIsFocusable for keyboard events**
- ✅ **Accept focus when selected**
- ✅ **Call parent keyPressEvent for unhandled keys**

### Implementation Plan

1. **Edge Class**: Add keyPressEvent override to handle Delete key
2. **Node Class**: Add keyPressEvent override to handle Delete key  
3. **Remove Scene::deleteSelected()**: Delete the problematic method entirely
4. **Remove Window delete handling**: Stop intercepting delete keys at window level
5. **Focus Management**: Ensure selected items receive keyboard focus

### Success Criteria

- Edge selected + Delete key = Edge deletes itself
- Node selected + Delete key = Node deletes itself  
- No loops, no type checking, no external deletion logic
- Clean Qt event flow: Selected item receives and handles its own events

### Code Review Standards

Any code that:
- Loops over selectedItems() for deletion → **REJECT**
- Uses qgraphicsitem_cast for deletion → **REJECT**
- Has external objects managing other objects' deletion → **REJECT**
- Violates "objects handle their own lifecycle" → **REJECT**

### Files to Modify

- `edge.h` - Add keyPressEvent override
- `edge.cpp` - Implement delete key handling
- `node.h` - Add keyPressEvent override  
- `node.cpp` - Implement delete key handling
- `scene.cpp` - Remove deleteSelected() method
- `window.cpp` - Remove delete key interception