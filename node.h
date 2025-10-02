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
#include <libxml/tree.h>

// Forward declarations to avoid circular includes
class Socket;
class Edge;

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
    // QGraphicsItem type system (required for qgraphicsitem_cast)
    enum { Type = UserType + 1 };
    int type() const override { return Type; }

    Node(const QUuid& id = QUuid::createUuid(),
         const QPointF& position = QPointF(100, 100));
    ~Node(); // Destructor for safe edge invalidation

    // Core identity
    const QUuid& getId() const { return m_id; }
    
    // Self-serialization interface
    xmlNodePtr write(xmlDocPtr doc, xmlNodePtr repr = nullptr) const;
    virtual void read(xmlNodePtr node);
    
    // QGraphicsItem interface
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    
    // Movement tracking for live XML updates
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
    
    // Node properties
    void setNodeSize(qreal width, qreal height);
    QSizeF getNodeSize() const { return QSizeF(m_width, m_height); }
    
    // Visual state - using Qt's selection system
    // Use QGraphicsItem::isSelected() and setSelected()
    
    // Socket management - O(1) performance
    Socket* getSocketByIndex(int index) const;
    int getSocketCount() const;
    void setNodeType(const QString& type);
    QString getNodeType() const { return m_nodeType; }
    
    // Socket registration for performance cache
    void registerSocket(Socket* socket, int index);
    
    // XML-driven socket creation
    void createSocketsFromXml(int inputCount, int outputCount);
    
    // ✅ Two-phase positioning: Position all sockets with complete information
    void positionAllSockets(int totalInputs, int totalOutputs);
    
    // Change notification - simple callback, no connect
    void setChangeCallback(void (*callback)(Node*));
    
    // Observer interface for GraphFactory - contract enforcement
    void setObserver(void* observer) { m_observer = observer; }
    bool hasObserver() const { return m_observer != nullptr; }
    void* getObserver() const { return m_observer; }
    
    // Edge connection management - O(degree) performance optimization
    void registerEdge(Edge* edge);
    void unregisterEdge(Edge* edge);
    void updateConnectedEdges();
    
    // Debug/testing helper
    int getIncidentEdgeCount() const { return m_incidentEdges.size(); }

private:
    QUuid m_id;
    QString m_nodeType;
    qreal m_width;
    qreal m_height;
    
    // Performance optimization: cache display string (created once, not every frame)
    mutable QString m_cachedDisplayId;
    
    // Socket cache for O(1) lookups - critical performance fix
    QVector<Socket*> m_sockets;  // Indexed by socket index for O(1) access
    
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
    void createStaticSockets();
    
    // Visual styling helpers
    void paintSockets(QPainter* painter) const;
};