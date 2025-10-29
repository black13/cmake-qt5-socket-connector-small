#pragma once

#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QWidget>
#include <QUuid>
#include <QPointF>
#include <QString>
#include <QVector>
#include <QRectF>
#include <QSizeF>
#include <QColor>
#include <QSet>
#include <QKeyEvent>

// Forward declarations to avoid circular includes
class Socket;
class Edge;

// Forward declarations for libxml types (reduces header pollution)
typedef struct _xmlNode xmlNode;
typedef xmlNode* xmlNodePtr;
typedef struct _xmlDoc xmlDoc;
typedef xmlDoc* xmlDocPtr;

/**
 * Node - A self-serializing visual node
 * 
 * Core principles:
 * - Self-serialization via write()/read() methods
 * - Value semantics, no smart pointers
 * - No QObject inheritance or connect usage
 * - Sockets are children of nodes only
 * - Uses Qt containers instead of std library
 */
class Node : public QGraphicsItem
{
public:
    Node(const QUuid& id = QUuid::createUuid(), 
         const QPointF& position = QPointF(100, 100));
    ~Node(); // Destructor for safe edge invalidation
    
    // Core identity
    [[nodiscard]] const QUuid& getId() const { return m_id; }
    
    // Self-serialization interface
    xmlNodePtr write(xmlDocPtr doc, xmlNodePtr repr = nullptr) const;
    virtual void read(xmlNodePtr node);
    
    // QGraphicsItem interface
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    
    // Event handling - proper Qt architecture  
    // Note: Delete key handling centralized in Scene::keyPressEvent()
    
    // Movement tracking for live XML updates
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
    
    // Node properties
    void setNodeSize(qreal width, qreal height);
    [[nodiscard]] QSizeF getNodeSize() const { return QSizeF(m_width, m_height); }
    
    // Visual state - using Qt's selection system
    // Use QGraphicsItem::isSelected() and setSelected()
    
    // Socket management - O(1) performance
    [[nodiscard]] Socket* getSocketByIndex(int index) const;
    [[nodiscard]] int getSocketCount() const;
    void setNodeType(const QString& type);
    [[nodiscard]] QString getNodeType() const { return m_nodeType; }
    [[nodiscard]] const QVector<Socket*>& getInputSockets() const { return m_inputSockets; }
    [[nodiscard]] const QVector<Socket*>& getOutputSockets() const { return m_outputSockets; }
    [[nodiscard]] const QVector<Socket*>& getAllSockets() const { return m_sockets; }
    
    // Socket registration for performance cache
    void registerSocket(Socket* socket, int index);
    
    // XML-driven socket creation
    void createSocketsFromXml(int inputCount, int outputCount);
    
    // Two-phase positioning: Position all sockets with complete information
    void positionAllSockets(int totalInputs, int totalOutputs);
    
    // Change notification - simple callback, no connect
    void setChangeCallback(void (*callback)(Node*));
    
    // Observer interface for GraphFactory - contract enforcement
    void setObserver(void* observer) { m_observer = observer; }
    [[nodiscard]] bool hasObserver() const { return m_observer != nullptr; }
    [[nodiscard]] void* getObserver() const { return m_observer; }
    
    // Edge connection management - O(degree) performance optimization
    void registerEdge(Edge* edge);
    void unregisterEdge(Edge* edge);
    void updateConnectedEdges();
    
    // Debug/testing helper
    [[nodiscard]] int getIncidentEdgeCount() const { return m_incidentEdges.size(); }

    // Typed socket access (eliminates qgraphicsitem_cast)
    [[nodiscard]] const QVector<Socket*>& inputSockets() const { return m_inputSockets; }
    [[nodiscard]] const QVector<Socket*>& outputSockets() const { return m_outputSockets; }
    [[nodiscard]] const QVector<Socket*>& allSockets() const { return m_sockets; }

private:
    QUuid m_id;
    QString m_nodeType;
    qreal m_width;
    qreal m_height;
    
    // Performance optimization: cache display string (created once, not every frame)
    mutable QString m_cachedDisplayId;
    
    // Socket cache for O(1) lookups - critical performance fix
    QVector<Socket*> m_sockets;  // Indexed by socket index for O(1) access
    QVector<Socket*> m_inputSockets;
    QVector<Socket*> m_outputSockets;
    
    // Edge adjacency set for O(degree) edge updates - performance optimization
    QSet<Edge*> m_incidentEdges;  // Edges touching this node (cleaned up by Edge destructor)
    
    // Simple callback - no QObject connect
    void (*m_changeCallback)(Node*);
    
    // Observer for contract enforcement
    void* m_observer;
    
    // Per-node position tracking (fixes global static bug)
    QPointF m_lastPos;
    
    // Dynamic node sizing based on socket count
    void calculateNodeSize(int inputCount, int outputCount);
    
    // Socket creation
    // createStaticSockets() ELIMINATED - unified XML-first creation only
    
    // Visual styling helpers
    void paintSockets(QPainter* painter) const;

protected:
    void keyPressEvent(QKeyEvent* event) override;
};
