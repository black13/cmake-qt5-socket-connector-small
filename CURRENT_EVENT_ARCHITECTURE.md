# Current Event Handling Architecture
*Comprehensive UML Documentation of Proven Foundation*

## Overview

This document maps the **actual implemented event handling system** as of August 2025, based on months of proven development. This is the foundation that JavaScript integration should respect as a **client**, not replace.

## Event Flow Architecture

```plantuml
@startuml EventFlow
title "Qt Event Flow - Your Proven Architecture"

actor User as U
participant QApplication as App
participant Window as W
participant View as V
participant Scene as S
participant Node as N
participant Socket as Sock
participant Edge as E
participant Observer as O

U -> App: Mouse Click
App -> W: Route Event
W -> V: mousePressEvent()
note right: Pass-through to Qt
V -> V: QGraphicsView::mousePressEvent()
V -> S: Forward to Scene
S -> S: Determine Hit Item

alt Hit Socket
    S -> Sock: mousePressEvent()
    Sock -> Sock: Handle Connection Logic
    Sock -> S: Start Ghost Edge
    S -> O: Notify Connection Started
else Hit Edge
    S -> E: mousePressEvent()
    E -> E: Handle Selection
    E -> E: Log Debug Info
else Hit Node
    S -> N: Qt Default Selection
    N -> N: itemChange(ItemSelectedHasChanged)
    N -> O: Notify Selection Change
    N -> N: update() - Visual refresh
end

@enduml
```

## Component Event Responsibilities

### **1. View (Pass-Through Layer)**
```plantuml
@startuml ViewEventHandling
class View {
    +mousePressEvent(QMouseEvent*)
    +mouseMoveEvent(QMouseEvent*) 
    +mouseReleaseEvent(QMouseEvent*)
    +wheelEvent(QWheelEvent*)
    +dragEnterEvent(QDragEnterEvent*)
    +dropEvent(QDropEvent*)
    --
    **Pattern: Pass-Through**
    All methods call Qt defaults
}

note right: View delegates all event\nhandling to Qt's proven system.\nNo custom logic interferes\nwith Qt's event routing.

View --> QGraphicsView : Delegates to
@enduml
```

**Implementation Pattern:**
```cpp
void View::mousePressEvent(QMouseEvent* event) {
    QGraphicsView::mousePressEvent(event);  // Pure delegation
}
```

**Why Pass-Through Works:**
- Qt's event system handles view-to-scene forwarding
- Scene determines which item was clicked
- Items handle their own specific interactions
- No interference with proven Qt patterns

### **2. Scene (Coordination Layer)**
```plantuml
@startuml SceneEventHandling
class Scene {
    +mouseMoveEvent(QGraphicsSceneMouseEvent*)
    +mouseReleaseEvent(QGraphicsSceneMouseEvent*)
    +addNode(Node*)
    +addEdge(Edge*)
    +getNode(QUuid): Node*
    +getEdges(): QHash<QUuid, Edge*>
    --
    **Ghost Edge Coordination**
    +startGhostEdge(Socket*, QPointF)
    +updateGhostEdge(QPointF)
    +finishGhostEdge(Socket*)
    --
    **Observer Notifications**
    +notifyNodeMoved(QUuid, QPointF, QPointF)
    +notifyEdgeAdded(Edge)
}

Scene --> GhostEdge : Manages
Scene --> GraphObserver : Notifies
Scene --> "QHash<QUuid,Node*>" : Maintains
Scene --> "QHash<QUuid,Edge*>" : Maintains

note bottom: Scene handles:\n- Ghost edge visual feedback\n- Observer coordination\n- Typed collections (no casting)\n- UUID-based O(1) lookups
@enduml
```

**Scene Event Responsibilities:**
- **Ghost Edge Management:** Visual connection feedback
- **Observer Coordination:** Notify registered observers
- **Collection Management:** Maintain typed Node/Edge collections
- **NOT Item-Level Events:** Those handled by items themselves

### **3. Node (Position + Selection Tracking)**
```plantuml
@startuml NodeEventHandling
class Node {
    +itemChange(GraphicsItemChange, QVariant): QVariant
    +updateConnectedEdges()
    +registerEdge(Edge*)
    +unregisterEdge(Edge*)
    --
    **No Direct Mouse Events**
    -mousePressEvent() ❌
    -mouseMoveEvent() ❌  
    -hoverEnterEvent() ❌
    --
    **Change Tracking Only**
    +m_lastPos: QPointF
    +m_incidentEdges: QSet<Edge*>
    +m_changeCallback: void(*)(Node*)
}

Node --> Edge : Updates via\nupdateConnectedEdges()
Node --> GraphObserver : Notifies via Scene
Node --> QGraphicsItem : Uses Qt's default\nmouse handling

note right: Node uses Qt's built-in\nselection and dragging.\nOnly tracks changes via\nitemChange() for:\n- Position updates\n- Selection logging\n- Edge coordination
@enduml
```

**Node itemChange() Logic:**
```cpp
if (change == ItemSelectedHasChanged) {
    // Log selection, trigger visual update
    update();
} else if (change == ItemPositionHasChanged) {
    // Update connected edges, notify observers
    updateConnectedEdges();
    scene->notifyNodeMoved(m_id, oldPos, newPos);
}
```

**Why No Direct Mouse Events:**
- Qt handles node selection automatically
- Qt handles node dragging automatically  
- Node only needs to react to **completed changes**
- Simpler, more reliable than custom mouse handling

### **4. Socket (Connection Interface)**
```plantuml
@startuml SocketEventHandling
class Socket {
    +mousePressEvent(QGraphicsSceneMouseEvent*)
    +mouseReleaseEvent(QGraphicsSceneMouseEvent*)
    +hoverEnterEvent(QGraphicsSceneHoverEvent*)
    +hoverLeaveEvent(QGraphicsSceneHoverEvent*)
    --
    **Connection Logic**
    +canConnectTo(Socket*): bool
    +startConnection(QPointF)
    +completeConnection(Socket*)
    --
    **Visual State**
    +m_highlighted: bool
    +m_connectionInProgress: bool
}

Socket --> Scene : Requests ghost edge
Socket --> Node : Gets parent node
Socket --> Edge : Creates connections

note right: Sockets are the primary\ninteraction points for users.\nThey handle:\n- Click-to-connect\n- Visual hover feedback\n- Connection validation\n- Ghost edge initiation
@enduml
```

**Socket Event Responsibilities:**
- **Primary User Interface:** Sockets are where users interact to create connections
- **Connection Initiation:** Start ghost edge visual feedback
- **Hover Feedback:** Visual highlight during interaction
- **Validation Logic:** Determine valid connections

### **5. Edge (Selection + Visual Feedback)**
```plantuml
@startuml EdgeEventHandling
class Edge {
    +mousePressEvent(QGraphicsSceneMouseEvent*)
    +mouseReleaseEvent(QGraphicsSceneMouseEvent*)
    +hoverEnterEvent(QGraphicsSceneHoverEvent*)
    +hoverLeaveEvent(QGraphicsSceneHoverEvent*)
    +itemChange(GraphicsItemChange, QVariant): QVariant
    --
    **Visual State Management**
    +m_hovered: bool
    +updatePath()
    +buildPath(QPointF, QPointF)
    --
    **Connection Data**
    +m_fromSocket: Socket*
    +m_toSocket: Socket*
    +resolveConnections(Scene*): bool
}

Edge --> Socket : Connects between
Edge --> Node : References via sockets
Edge --> QPainterPath : Renders as

note right: Edges provide:\n- Selection feedback\n- Hover highlighting\n- Debug interaction logging\n- Visual path updates
@enduml
```

**Edge Event Pattern:**
```cpp
void Edge::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    qDebug() << "Edge clicked:" << m_id;  // Debug logging
    QGraphicsItem::mousePressEvent(event);  // Delegate to Qt
}
```

## Observer Pattern Integration

```plantuml
@startuml ObserverIntegration
interface GraphObserver {
    +onNodeAdded(Node&)
    +onNodeRemoved(QUuid)
    +onNodeMoved(QUuid, QPointF, QPointF)
    +onEdgeAdded(Edge&)
    +onEdgeRemoved(QUuid)
    +onGraphCleared()
}

class GraphSubject {
    +attach(GraphObserver*)
    +detach(GraphObserver*)
    +beginBatch()
    +endBatch()
    --
    #notifyNodeAdded(Node&)
    #notifyNodeMoved(QUuid, QPointF, QPointF)
    #notifyEdgeAdded(Edge&)
    -m_observers: QSet<GraphObserver*>
    -s_batchDepth: int
}

class Scene {
    +notifyNodeMoved(QUuid, QPointF, QPointF)
}

class XmlAutosaveObserver {
    +onNodeMoved(QUuid, QPointF, QPointF)
    +onEdgeAdded(Edge&)
}

GraphSubject <|-- Scene
GraphObserver <|.. XmlAutosaveObserver
Scene --> GraphObserver : Notifies via\ninherited methods
Node --> Scene : Triggers notifications\nvia itemChange()

note bottom: Observer pattern provides:\n- Performance batching\n- Decoupled change notifications\n- XML autosave integration\n- Future JavaScript hooks
@enduml
```

## Event Flow Sequences

### **Node Creation Sequence**
```plantuml
@startuml NodeCreationSequence
actor User as U
participant View as V
participant Scene as S
participant GraphFactory as GF  
participant Node as N
participant Observer as O

U -> V: Drag from palette
V -> V: dropEvent()
V -> S: nodeDropped signal
S -> GF: createNode()
GF -> N: new Node()
GF -> S: addNode(node)
S -> O: notifyNodeAdded()
N -> N: itemChange(ItemPositionChange)
N -> S: notifyNodeMoved()
S -> O: notifyNodeMoved()
@enduml
```

### **Edge Connection Sequence**  
```plantuml
@startuml EdgeConnectionSequence
actor User as U
participant Socket1 as S1
participant Scene as Sc
participant GhostEdge as GE
participant Socket2 as S2
participant Edge as E

U -> S1: Click socket
S1 -> S1: mousePressEvent()
S1 -> Sc: startGhostEdge()
Sc -> GE: Create ghost edge
U -> Sc: Drag mouse
Sc -> Sc: mouseMoveEvent()
Sc -> GE: updateGhostEdge()
U -> S2: Release on socket
S2 -> S2: mouseReleaseEvent()
Sc -> Sc: finishGhostEdge()
Sc -> E: Create real edge
Sc -> GE: Remove ghost
@enduml
```

## Performance Characteristics

### **O(1) Operations (Proven Fast)**
- `Scene::getNode(QUuid)` → QHash lookup
- `Scene::getEdge(QUuid)` → QHash lookup  
- `Node::registerEdge()` → QSet insertion
- `Socket::getParentNode()` → Direct pointer

### **O(degree) Operations (Acceptable)**
- `Node::updateConnectedEdges()` → Only edges touching node
- `Socket::canConnectTo()` → Validation logic

### **O(n) Operations (Avoided via Architecture)**
- ❌ **Never iterate scene->items() and cast**
- ❌ **Never search for items by properties**
- ✅ **Always use typed collections**
- ✅ **Always use UUID-based lookups**

## JavaScript Client Integration Points

**Based on this proven architecture, JavaScript should be a CLIENT that:**

### **Respects Observer Pattern**
```cpp
// JavaScript calls should trigger observer notifications
jsEngine->evaluateScript("Graph.createNode('SOURCE', 100, 100)");
// → GraphFactory::createNode() → Scene::addNode() → Observer::onNodeAdded()
```

### **Uses Typed Collections**
```cpp  
// JavaScript should access via Scene's typed methods
const QHash<QUuid, Node*>& nodes = scene->getNodes();
// NOT: iterate scene->items() and cast
```

### **Leverages Event System**
```cpp
// JavaScript can observe via GraphObserver interface  
class JSObserver : public GraphObserver {
    void onNodeMoved(QUuid id, QPointF old, QPointF new) override {
        jsEngine->notifyNodeMoved(id, old, new);
    }
};
```

## Conclusion

**Your proven architecture has:**
- ✅ **Layered event handling** with clear responsibilities
- ✅ **Performance-optimized collections** with O(1) lookups  
- ✅ **Observer pattern** for decoupled notifications
- ✅ **Qt integration** that works with, not against, Qt's event system
- ✅ **Visual feedback systems** (ghost edges, hover effects)

**JavaScript integration should:**
- **Respect this architecture** as the foundation
- **Act as a client** of these proven systems
- **Use the observer pattern** for notifications
- **Call into C++ methods** rather than replacing them

This is your **battle-tested foundation** from months of development - it should be preserved and leveraged, not replaced.