#pragma once

#include <QGraphicsItem>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QWidget>
#include <QUuid>
#include <QString>
#include <QColor>
#include <QPainterPath>
#include <QPainterPathStroker>
#include <QPointF>
#include <functional>
#include <libxml/tree.h>

class Socket;
class Node;

/**
 * Edge - Connection between two sockets
 * 
 * Core principles:
 * - Self-serializing connection between socket UUIDs
 * - No QObject inheritance or connect usage
 * - Draws path from socket to socket
 * - References sockets by UUID, not pointers
 */
class Edge : public QGraphicsItem
{
public:
    // QGraphicsItem type system (required by Qt, DO NOT USE in logic)
    // Policy: Use Gik::KindKey metadata instead of type() for type checking
    enum { Type = UserType + 2 };
    int type() const override { return Type; }

    Edge(const QUuid& id = QUuid::createUuid(),
         const QUuid& fromSocketId = QUuid(),
         const QUuid& toSocketId = QUuid());
    ~Edge(); // Destructor for node unregistration
    
    // Core identity
    const QUuid& getId() const { return m_id; }
    
    // Self-serialization interface
    xmlNodePtr write(xmlDocPtr doc, xmlNodePtr repr = nullptr) const;
    void read(xmlNodePtr node);
    
    // QGraphicsItem interface
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    QPainterPath shape() const override;
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
    
    // Connection management - clean design uses node+index only
    // No socket UUIDs - edges resolved via resolveConnections() method
    
    // Visual state uses Qt's selection system
    // Use QGraphicsItem::isSelected() and setSelected()
    
    // Path update - call when socket positions change
    void updatePath();
    
    // Mouse event debugging and interaction
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;
    
    // Connection checking
    bool isConnectedToNode(const QString& nodeId) const;
    bool isConnectedToNode(const QUuid& nodeId) const;  // Optimized UUID version
    
    // Socket resolution after all nodes are loaded
    bool resolveConnections(class Scene* scene);
    
    // Direct connection methods (optimization for GraphFactory)
    void setConnectionData(const QString& fromNodeId, const QString& toNodeId, 
                          int fromSocketIndex, int toSocketIndex);
    void setResolvedSockets(Socket* fromSocket, Socket* toSocket);
    
    // Manual weak pointer system for safe destruction
    void invalidateNode(const Node* node);  // Legacy method
    void onNodeDestroying(const Node* node);  // New: Safe destruction callback
    
    // Public accessors for layout engine
    Node* getFromNode() const { return m_fromNode; }
    Node* getToNode() const { return m_toNode; }
    Socket* getFromSocket() const { return m_fromSocket; }
    Socket* getToSocket() const { return m_toSocket; }

private:
    QUuid m_id;
    QString m_fromNodeId;     // Store node IDs from XML (for serialization)
    QString m_toNodeId;
    QUuid m_fromNodeUuid;     // Cached UUIDs for fast comparison
    QUuid m_toNodeUuid;
    int m_fromSocketIndex;    // Store socket indices from XML
    int m_toSocketIndex;
    Socket* m_fromSocket;     // Resolved socket pointers
    Socket* m_toSocket;
    
    // Manual weak pointers for safe destruction (nulled by Node::~Node)
    Node* m_fromNode;         // Source node (may be nullptr during destruction)
    Node* m_toNode;           // Destination node (may be nullptr during destruction)
    bool m_fromNodeValid;     // Safety flag: true if m_fromNode is safe to access
    bool m_toNodeValid;       // Safety flag: true if m_toNode is safe to access
    
    // Cached path for rendering
    QPainterPath m_path;
    QRectF m_boundingRect;
    
    // Interaction state
    bool m_hovered;
    
    #ifdef QT_DEBUG
    // Per-edge debug counter (thread-safe, per-instance)
    mutable int m_shapeCallCount;
    #endif
    
    // Visual styling
    void buildPath(const QPointF& start, const QPointF& end);
};