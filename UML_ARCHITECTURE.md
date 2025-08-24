# NodeGraph UML Architecture

## Core Class Diagram

```plantuml
@startuml NodeGraphCore

!define PRIMARY_COLOR #E3F2FD
!define SECONDARY_COLOR #FFF3E0
!define ACCENT_COLOR #E8F5E8

' =============================================================================
' Qt5 Framework Layer (Blue)
' =============================================================================
package "Qt5 Framework" PRIMARY_COLOR {
    abstract class QMainWindow
    abstract class QGraphicsView  
    abstract class QGraphicsScene
    abstract class QGraphicsItem
    abstract class QObject
    abstract class QWidget
}

' =============================================================================
' Application UI Layer (Orange) 
' =============================================================================
package "UI Layer" SECONDARY_COLOR {
    class Window {
        -Scene* m_scene
        -View* m_view
        -NodePaletteWidget* m_palette
        -QMenuBar* m_menuBar
        +setupUI()
        +loadFile(QString filename)
        +saveFile(QString filename)
    }
    
    class View {
        -Scene* m_scene
        -qreal m_scaleFactor
        +wheelEvent(QWheelEvent*)
        +mousePressEvent(QMouseEvent*)
        +keyPressEvent(QKeyEvent*)
    }
    
    class NodePaletteWidget {
        -QList<NodeTemplate> m_templates
        -QGridLayout* m_layout
        +addNodeType(QString type)
        +createNodeButton(NodeTemplate)
        +nodeButtonClicked(QString type)
    }
}

' =============================================================================
' Scene Management Layer (Light Green)
' =============================================================================
package "Scene Layer" ACCENT_COLOR {
    class Scene {
        -QHash<QUuid, Node*> m_nodes
        -QHash<QUuid, Edge*> m_edges
        -GraphFactory* m_factory
        -QList<GraphObserver*> m_observers
        +addNode(Node*)
        +addEdge(Edge*)
        +deleteNode(QUuid)
        +deleteEdge(QUuid)
        +notifyObservers()
    }
    
    class Node {
        -QUuid m_id
        -QString m_nodeType
        -RubberType m_type
        -QList<Socket*> m_inputSockets
        -QList<Socket*> m_outputSockets
        -QPointF m_position
        +serialize() : QString
        +deserialize(QDomElement)
        +canConnectTo(Node*) : bool
        +keyPressEvent(QKeyEvent*)
    }
    
    class Edge {
        -QUuid m_id
        -Socket* m_fromSocket
        -Socket* m_toSocket
        -QPainterPath m_path
        +serialize() : QString
        +deserialize(QDomElement)
        +updatePath()
        +keyPressEvent(QKeyEvent*)
    }
    
    class Socket {
        -QUuid m_id
        -Node* m_parentNode
        -RubberType m_socketType
        -bool m_isInput
        -QList<Edge*> m_connections
        +canAccept(RubberType) : bool
        +connectTo(Socket*) : Edge*
        +keyPressEvent(QKeyEvent*)
    }
}

' =============================================================================
' Data Management Layer (Purple)
' =============================================================================
package "Data Layer" {
    class GraphFactory {
        -Scene* m_scene
        -xmlDocPtr m_xmlDoc
        -NodeRegistry* m_registry
        +createNode(QString type, QPointF pos) : Node*
        +createEdge(Socket*, Socket*) : Edge*
        +loadFromXml(QString filename) : bool
        +saveToXml(QString filename) : bool
    }
    
    class NodeRegistry {
        -QHash<QString, NodeFactory> m_factories
        -static NodeRegistry* s_instance
        +registerNode(QString, std::function<Node*()>)
        +createNode(QString type) : Node*
        +getRegisteredTypes() : QStringList
        +instance() : NodeRegistry&
    }
    
    class GraphObserver {
        <<abstract>>
        +onNodeAdded(Node*)
        +onNodeDeleted(QUuid)
        +onEdgeAdded(Edge*)
        +onEdgeDeleted(QUuid)
    }
    
    class XmlAutosaveObserver {
        -QString m_filename
        -QTimer* m_timer
        +onNodeAdded(Node*)
        +onEdgeAdded(Edge*)
        +saveToFile()
    }
}

' =============================================================================
' Rubber Types System (Pink)
' =============================================================================
package "Type System" {
    class RubberType {
        -QString m_typeName
        -QList<QString> m_compatibleTypes
        -RubberTypeCategory m_category
        +isCompatibleWith(RubberType) : bool
        +getDisplayName() : QString
        +getCategory() : RubberTypeCategory
    }
    
    enum RubberTypeCategory {
        NUMERIC
        STRING  
        BOOLEAN
        OBJECT
        ARRAY
        ANY
    }
    
    class RubberTypeRegistry {
        -QHash<QString, RubberType> m_types
        -static RubberTypeRegistry* s_instance
        +registerType(RubberType)
        +getType(QString name) : RubberType
        +getCompatibleTypes(QString) : QList<RubberType>
        +instance() : RubberTypeRegistry&
    }
}

' =============================================================================
' Relationships
' =============================================================================

' Inheritance relationships
QMainWindow <|-- Window
QGraphicsView <|-- View
QGraphicsScene <|-- Scene  
QGraphicsItem <|-- Node
QGraphicsItem <|-- Edge
QGraphicsItem <|-- Socket
QWidget <|-- NodePaletteWidget
QObject <|-- GraphFactory
QObject <|-- NodeRegistry
QObject <|-- GraphObserver
GraphObserver <|-- XmlAutosaveObserver

' Composition relationships
Window *-- View : contains
Window *-- NodePaletteWidget : contains
Window *-- Scene : manages
Scene *-- Node : stores
Scene *-- Edge : stores
Node *-- Socket : contains
Edge -- Socket : connects
Scene *-- GraphFactory : uses
GraphFactory -- NodeRegistry : queries
Scene *-- GraphObserver : notifies

' Type system relationships  
Socket -- RubberType : "has type"
Node -- RubberType : "validates with"
RubberTypeRegistry -- RubberType : manages

' Key dependencies
View --> Scene : displays
NodePaletteWidget --> GraphFactory : "triggers node creation"
GraphFactory --> NodeRegistry : "creates nodes via"
XmlAutosaveObserver --> GraphFactory : "saves via"

@enduml
```

## Component Interaction Flow

### 1. Node Creation Flow
```
User clicks palette → NodePaletteWidget → GraphFactory → NodeRegistry → Node created → Scene adds → Observers notified → XML updated
```

### 2. Connection Creation Flow  
```
User drags socket → View captures → Scene validates → Socket checks RubberType → Edge created → Observers notified → XML updated
```

### 3. File Load Flow
```
User opens file → Window → GraphFactory → XML parsed → Nodes/Edges created → Scene populated → View refreshed
```

### 4. Type Checking Flow
```
Socket connection attempt → RubberType compatibility check → Connection allowed/denied → UI feedback
```

## Architecture Benefits

1. **Separation of Concerns**: UI, Scene, Data, and Types are cleanly separated
2. **Observer Pattern**: Automatic synchronization without tight coupling  
3. **Factory Pattern**: Extensible node creation system
4. **Registry Pattern**: Dynamic type system management
5. **Self-Serialization**: Each object manages its own persistence
6. **Type Safety**: Rubber types prevent invalid connections

---

*This UML represents the target architecture after JavaScript removal and rubber types integration.*