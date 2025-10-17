# Response to plan.md - Architecture Decisions

Date: 2025-10-17

## Executive Summary

This document provides definitive answers to the architectural questions raised in `plan.md`. All recommendations align with the core principle: **Objects manage their own lifecycle and relationships through typed collections, never through type checking or casting.**

---

## 1. Delete-key handling: Move to Node/Edge keyPressEvent?

**Answer: YES - MANDATORY**

### Current Violation (scene.cpp:435-460)
```cpp
// ❌ ARCHITECTURAL MISTAKE
void Scene::keyPressEvent(QKeyEvent* event) {
    QList<QGraphicsItem*> selected = selectedItems();
    for (QGraphicsItem* item : selected) {
        if (auto* node = qgraphicsitem_cast<Node*>(item)) {
            nodesToDelete.append(node);
        } else if (auto* edge = qgraphicsitem_cast<Edge*>(item)) {
            edgesToDelete.append(edge);
        }
    }
}
```

**Violations:**
- ❌ External object (Scene) managing deletion
- ❌ Type checking loops with qgraphicsitem_cast
- ❌ Objects don't handle their own lifecycle

### Required Solution

**edge.h / edge.cpp:**
```cpp
protected:
    void keyPressEvent(QKeyEvent* event) override;

void Edge::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
        qDebug() << "Edge: Self-deleting via Delete key";
        deleteLater();
        event->accept();
        return;
    }
    QGraphicsItem::keyPressEvent(event);
}
```

**node.h / node.cpp:**
```cpp
protected:
    void keyPressEvent(QKeyEvent* event) override;

void Node::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
        qDebug() << "Node: Self-deleting via Delete key";
        deleteLater();
        event->accept();
        return;
    }
    QGraphicsItem::keyPressEvent(event);
}
```

**scene.cpp:**
```cpp
// REMOVE entire keyPressEvent delete handling block (lines 435-480)
// Let Qt event system route to selected items naturally
```

**Status:** ✅ **APPROVED - Implement immediately**

---

## 2. JavaScript engine (ENABLE_JS) - Restore headers or guard behind stub?

**Answer: KEEP STUB APPROACH - Remove obsolete console files**

### Current State Analysis
- `javascript_console.cpp/h` - **OBSOLETE** (not in CMakeLists.txt)
- `script_api_stub.h` - **ACTIVE** (stub for when ENABLE_JS=OFF)
- `graph_script_api.h/cpp` - **CONDITIONAL** (included when ENABLE_JS=ON)
- `script_host.h/cpp` - **ACTIVE** (JS integration layer)

### Actions Taken
✅ **COMPLETED:** Removed obsolete files:
- `javascript_console.cpp`
- `javascript_console.h`

### Current Architecture (CORRECT)
```cmake
# CMakeLists.txt:84
option(ENABLE_JS "Enable in-app JavaScript engine" OFF)

# Lines 248-252
if(ENABLE_JS)
    list(APPEND CORE_SOURCES
        graph_script_api.h
        graph_script_api.cpp
    )
endif()
```

**Status:** ✅ **NO CHANGES NEEDED** - Current stub approach is architecturally correct

---

## 3. graph_controller.cpp - Still part of active product?

**Answer: NO - Remove completely**

### Analysis
- `graph_controller.cpp` exists but **NOT in CMakeLists.txt**
- References deleted headers: `graph_controller.h`, `node_registry.h`
- Removed in commit 63755db: "Fix CMakeLists.txt - remove references to deleted graph_controller and node_registry files"
- Replaced by `GraphFactory` + `NodeTypeTemplates` architecture

### Action Taken
✅ **COMPLETED:** Removed obsolete file:
- `graph_controller.cpp`

### Current Architecture (CORRECT)
```cpp
// Modern template-first workflow:
GraphFactory    → XML serialization & graph construction
NodeTypeTemplates → Scriptable node type definitions
```

**Status:** ✅ **COMPLETED** - Obsolete code removed

---

## 4. Ghost-edge interaction - Implement drag flow or disable?

**Answer: FINISH LEFT-CLICK DRAG IMPLEMENTATION**

### Current State
✅ **Right-click drag: WORKING**
```cpp
// socket.cpp:185-192
if (event->button() == Qt::RightButton && m_role == Output) {
    Scene* scene = qobject_cast<Scene*>(this->scene());
    if (scene) {
        scene->startGhostEdge(this, event->scenePos());
    }
}
```

❌ **Left-click drag: INCOMPLETE**
```cpp
// socket.cpp:183
// TODO: Start edge creation drag

// socket.cpp:204
// TODO: Complete edge connection
```

### Required Implementation

**socket.cpp - mousePressEvent:**
```cpp
if (event->button() == Qt::LeftButton) {
    qDebug() << "Socket: Starting ghost edge on left-click";
    Scene* scene = qobject_cast<Scene*>(this->scene());
    if (scene) {
        scene->startGhostEdge(this, event->scenePos());
    }
    event->accept();
}
```

**socket.cpp - mouseReleaseEvent:**
```cpp
if (event->button() == Qt::LeftButton) {
    qDebug() << "Socket: Completing ghost edge connection";
    Scene* scene = qobject_cast<Scene*>(this->scene());
    if (scene && scene->hasActiveGhostEdge()) {
        scene->completeGhostEdge(this);
    }
    event->accept();
}
```

**Why finish it:**
- Infrastructure 80% complete (ghost_edge.h/cpp, Scene methods exist)
- Right-click proves system works
- Left-click is more intuitive UX
- Small change, big user experience improvement

**Status:** ✅ **APPROVED** - Implement after qgraphicsitem_cast elimination

---

## CRITICAL FINDING: qgraphicsitem_cast Violations

### Discovery
**17 violations** of qgraphicsitem_cast found across active codebase:

| File | Violations | Impact |
|------|------------|--------|
| scene.cpp | 11 | Delete handling, serialization, ghost edge |
| node.cpp | 2 | Socket child iteration |
| window.cpp | 2 | Selection counting |
| socket.cpp | 1 | Parent node access |
| graph_factory.cpp | 1 | Serialization |

### Root Cause
❌ **Objects use scene graph traversal + type checking instead of typed relationships**

### Architectural Fix: 7-Phase Plan

#### Phase 1: Socket → Node typed relationship
```cpp
// socket.h
private:
    Node* m_parentNode;  // Store typed pointer

Node* Socket::getParentNode() const {
    return m_parentNode;  // No cast needed
}
```

#### Phase 2: Node → Socket typed collections
```cpp
// node.h
private:
    QList<Socket*> m_inputSockets;
    QList<Socket*> m_outputSockets;

Socket* Node::getInputSocket(int index) const {
    return (index < m_inputSockets.size()) ? m_inputSockets[index] : nullptr;
}
```

#### Phase 3: Scene typed collections
```cpp
// scene.h
private:
    QList<Node*> m_nodes;
    QList<Edge*> m_edges;

public:
    void addNode(Node* node);
    QList<Node*> nodes() const { return m_nodes; }
```

#### Phase 4: Delete key - NO TYPE CHECKING
```cpp
// Move to Node::keyPressEvent and Edge::keyPressEvent
// Remove Scene::keyPressEvent deletion logic entirely
```

#### Phase 5: Ghost Edge typed accessors
```cpp
Socket* Node::socketAtPoint(const QPointF& scenePos) const {
    for (Socket* socket : m_inputSockets) {
        if (socket->sceneBoundingRect().contains(scenePos))
            return socket;
    }
    return nullptr;
}
```

#### Phase 6: Window typed queries
```cpp
// Use Scene::nodes() and Scene::edges() directly
int nodeCount = m_scene->nodes().size();
```

#### Phase 7: GraphFactory typed iteration
```cpp
// Use Node::inputSockets() and Node::outputSockets()
for (Socket* socket : node->inputSockets()) {
    writeSocketToXml(socket);
}
```

### Implementation Priority

| Phase | Description | Priority | Dependency |
|-------|-------------|----------|------------|
| 1 | Socket m_parentNode | HIGH | Foundation |
| 2 | Node typed socket lists | HIGH | Foundation |
| 3 | Scene typed collections | HIGH | Infrastructure |
| 4 | Delete key refactor | **CRITICAL** | Phases 1-3 |
| 5 | Ghost edge cleanup | MEDIUM | Phases 1-2 |
| 6 | Window cleanup | LOW | Phase 3 |
| 7 | Factory cleanup | LOW | Phase 2 |

**Goal:** ZERO qgraphicsitem_cast calls in entire codebase

---

## Summary of Actions

### Completed ✅
1. Removed `javascript_console.cpp` and `javascript_console.h`
2. Removed `graph_controller.cpp`

### Required Implementation 🔨
1. **CRITICAL:** Eliminate all 17 qgraphicsitem_cast violations (7-phase plan)
2. **HIGH:** Move delete-key handling to Node/Edge keyPressEvent
3. **MEDIUM:** Complete ghost-edge left-click drag flow

### No Action Needed ⏸️
1. JavaScript stub approach (working correctly)
2. Template-first architecture (already in place)

---

## Architectural Principles Reaffirmed

✅ **Objects manage their own lifecycle**
✅ **Typed collections, never type checking**
✅ **No qgraphicsitem_cast - it's always a mistake**
✅ **Qt event routing to selected items naturally**

**Next Step:** Begin Phase 1 implementation (Socket m_parentNode)
