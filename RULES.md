# Development Rules for Qt NodeGraph System

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