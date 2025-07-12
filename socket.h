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
#include <QSet>
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
    
    enum VisualState {
        Normal,         // Default state
        Hovered,        // Mouse hovering over socket
        ValidTarget,    // Valid target for ghost edge connection
        InvalidTarget,  // Invalid target for ghost edge connection
        Connected       // Socket has active connections
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
    
    // Connection state - supports multiple edges per socket
    bool isConnected() const { return !m_connectedEdges.isEmpty(); }
    void addConnectedEdge(Edge* edge);
    void removeConnectedEdge(Edge* edge);
    const QSet<Edge*>& getConnectedEdges() const { return m_connectedEdges; }
    
    // Legacy single-edge interface for compatibility
    void setConnectedEdge(Edge* edge); // Deprecated - use addConnectedEdge
    Edge* getConnectedEdge() const;    // Returns first connected edge or nullptr
    
    // Visual feedback state for ghost edge interactions
    void setVisualState(VisualState state);
    VisualState getVisualState() const { return m_visualState; }
    
    // Socket positioning (automatic based on index and role)
    void updatePosition();

private:
    Role m_role;
    int m_index;                 // Socket index within parent node (0, 1, 2...)
    QSet<Edge*> m_connectedEdges; // All connected edges - supports multiple connections
    qreal m_radius;
    bool m_hovered;
    VisualState m_visualState;   // Current visual feedback state
    
    // Performance optimization: cache index string (created once, not every frame)
    mutable QString m_cachedIndexString;
    
    // Positioning helpers
    QPointF calculatePosition() const;
};