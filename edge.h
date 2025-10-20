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
#include <QKeyEvent>
#include <functional>

class Socket;
class Node;

// Forward declarations for libxml types (reduces header pollution)
typedef struct _xmlNode xmlNode;
typedef xmlNode* xmlNodePtr;
typedef struct _xmlDoc xmlDoc;
typedef xmlDoc* xmlDocPtr;

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
    Edge(const QUuid& id = QUuid::createUuid(),
         const QUuid& fromSocketId = QUuid(),
         const QUuid& toSocketId = QUuid());
    ~Edge(); // Destructor for node unregistration
    
    // Core identity
    [[nodiscard]] const QUuid& getId() const { return m_id; }
    
    // Self-serialization interface
    xmlNodePtr write(xmlDocPtr doc, xmlNodePtr repr = nullptr) const;
    void read(xmlNodePtr node);
    
    // QGraphicsItem interface
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    QPainterPath shape() const override;
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
    
    // Event handling - proper Qt architecture
    // Note: Delete key handling centralized in Scene::keyPressEvent()
    
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
    void invalidateNode(const Node* node);
    
    // Public accessors for layout engine
    [[nodiscard]] Node* getFromNode() const { return m_fromNode; }
    [[nodiscard]] Node* getToNode() const { return m_toNode; }
    [[nodiscard]] Socket* getFromSocket() const { return m_fromSocket; }
    [[nodiscard]] Socket* getToSocket() const { return m_toSocket; }
    
    // Connection data accessors (for validation)
    [[nodiscard]] QString getFromNodeId() const { return m_fromNodeId; }
    [[nodiscard]] QString getToNodeId() const { return m_toNodeId; }
    [[nodiscard]] int getFromSocketIndex() const { return m_fromSocketIndex; }
    [[nodiscard]] int getToSocketIndex() const { return m_toSocketIndex; }

    void detachSockets();

protected:
    void keyPressEvent(QKeyEvent* event) override;

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
