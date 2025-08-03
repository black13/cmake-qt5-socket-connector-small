#pragma once

#include <QGraphicsItem>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QWidget>
#include <QGraphicsSceneMouseEvent>
#include <QUuid>
#include <QPointF>
#include <QString>
#include <QColor>
#include <QRectF>
#include <libxml/tree.h>

class Node;
class Edge;

/**
 * Socket - QGraphicsItem connection point on a node
 * 
 * Core principles:
 * - QGraphicsItem child of Node QGraphicsItem
 * - Self-serializing like its parent node
 * - Handles mouse events for connection creation
 * - Position managed by Qt parent-child system
 * - Index-based identification within parent node
 */
class Socket : public QGraphicsItem
{
public:
    enum Role {
        Input,
        Output
    };
    
    enum ConnectionState {
        Disconnected,
        Connecting,    // During ghost edge drag
        Connected,     // Has connected edge
        Highlighted    // Target of ghost edge
    };
    
    // Helper for debugging
    static const char* roleToString(Role role) {
        switch (role) {
            case Input: return "INPUT";
            case Output: return "OUTPUT";
            default: return "UNKNOWN";
        }
    }
    
    Socket(Role role, Node* parentNode, int index);
    
    // Core identity - NO UUID, just index within parent node
    int getIndex() const { return m_index; }
    Role getRole() const { return m_role; }
    // Access parent node via Qt's system  
    Node* getParentNode() const;
    
    // Self-serialization interface
    xmlNodePtr write(xmlDocPtr doc, xmlNodePtr repr = nullptr) const;
    void read(xmlNodePtr node);
    
    // QGraphicsItem interface
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    
    // Mouse events for connection creation
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;
    
    // Connection state
    bool isConnected() const { return m_connectedEdge != nullptr; }
    void setConnectedEdge(Edge* edge) { m_connectedEdge = edge; updateConnectionState(); }
    Edge* getConnectedEdge() const { return m_connectedEdge; }
    
    // Visual connection state
    ConnectionState getConnectionState() const { return m_connectionState; }
    void setConnectionState(ConnectionState state) { m_connectionState = state; update(); }
    void updateConnectionState() { 
        setConnectionState(m_connectedEdge ? Connected : Disconnected); 
    }
    
    // ✅ Direct position assignment - no calculations, just assignment
    void setDirectPosition(qreal x, qreal y) { setPos(x, y); }
    
    // Size properties for edge connection calculations
    qreal getRadius() const { return m_radius; }
    QSizeF getSocketSize() const { return boundingRect().size(); }
    
    // Visual state for drag-and-drop feedback (disabled)
    // VisualState getVisualState() const { return m_visualState; }
    // void setVisualState(VisualState state) { m_visualState = state; update(); }

private:
    Role m_role;
    int m_index;                 // Socket index within parent node (0, 1, 2...)
    Edge* m_connectedEdge;       // Connected edge (if any)  
    ConnectionState m_connectionState; // Visual connection state
    qreal m_radius;
    bool m_hovered;
    qreal m_hoverOpacity; // Smooth hover opacity transition
    bool m_pressed; // Click feedback state
    
    // Performance optimization: cache index string (created once, not every frame)
    mutable QString m_cachedIndexString;
    
};