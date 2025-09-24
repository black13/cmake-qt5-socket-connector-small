#include "socket.h"
#include "node.h"
#include "edge.h"
#include "scene.h"
#include "constants.h"
#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include <QDebug>
#include <QDateTime>
#include <QtMath>
#include <libxml/tree.h>

Socket::Socket(Role role, Node* parentNode, int index)
    : QGraphicsItem(parentNode)
    , m_role(role)
    , m_index(index)
    , m_connectedEdge(nullptr)
    , m_connectionState(Disconnected)
    , m_radius(8.0)
    , m_hovered(false)
    , m_hoverOpacity(0.0)
    , m_pressed(false)
{
    setAcceptHoverEvents(true);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemIsFocusable, true); // Enable keyboard events
    
    // Set socket z-order: above nodes (0) but below edges (2)
    setZValue(1);
    
    // NO positioning in constructor - will be positioned later with complete information
    
    // Register with parent node for O(1) lookups
    if (parentNode) {
        parentNode->registerSocket(this, m_index);
    }
    
    qDebug() << "+Socket" << m_index << (m_role == Input ? "IN" : "OUT");
}

Node* Socket::getParentNode() const
{
    return qgraphicsitem_cast<Node*>(parentItem());
}

QRectF Socket::boundingRect() const
{
    // Smaller rounded square sockets with better proportions
    return QRectF(-7, -7, 14, 14);
}

void Socket::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)
    
    painter->setRenderHint(QPainter::Antialiasing);
    
    // Color-coded sockets like upper level system
    QColor socketColor;
    QColor borderColor;
    
    if (m_role == Input) {
        socketColor = QColor(100, 149, 237); // Cornflower blue
        borderColor = QColor(70, 130, 180);  // Steel blue
    } else {
        socketColor = QColor(220, 20, 60);    // Crimson red
        borderColor = QColor(178, 34, 34);    // Fire brick
    }
    
    // Add smooth hover effect with opacity
    if (m_hovered || m_hoverOpacity > 0.0) {
        qreal hoverAmount = m_hovered ? 1.0 : m_hoverOpacity;
        socketColor = QColor::fromRgb(
            socketColor.red() + (50 * hoverAmount),
            socketColor.green() + (50 * hoverAmount),
            socketColor.blue() + (50 * hoverAmount)
        );
        borderColor = QColor::fromRgb(
            borderColor.red() + (30 * hoverAmount),
            borderColor.green() + (30 * hoverAmount),
            borderColor.blue() + (30 * hoverAmount)
        );
    }
    
    // Draw socket as circular with enhanced styling
    QRectF rect = boundingRect();
    
    // Apply pressed state effect
    if (m_pressed) {
        socketColor = socketColor.darker(120);
        borderColor = borderColor.darker(120);
        rect = rect.adjusted(1, 1, -1, -1); // Slight inset when pressed
    }
    
    // Add connection state visual feedback
    switch (m_connectionState) {
        case Connected:
            {
                // ENHANCED: Clear visual feedback for connected sockets
                
                // Draw socket body (slightly dimmed to show "occupied" state)
                painter->setBrush(socketColor.darker(110));
                painter->setPen(QPen(borderColor, 2));
                painter->drawRoundedRect(rect, 3.0, 3.0);
                
                // Draw prominent BLACK connection dot in center
                QRectF dotRect = rect.adjusted(3, 3, -3, -3); // Larger dot for visibility
                painter->setBrush(QBrush(Qt::black));
                painter->setPen(QPen(Qt::black, 1));
                painter->drawEllipse(dotRect); // Black circular dot
                
                // Optional: Add subtle glow around socket
                if (m_hovered) {
                    painter->setBrush(Qt::NoBrush);
                    painter->setPen(QPen(borderColor.lighter(150), 1));
                    painter->drawRoundedRect(rect.adjusted(-1, -1, 1, 1), 4.0, 4.0);
                }
            }
            break;
            
        case Highlighted:
            {
                // Enhanced magnetic highlight with stronger visual feedback
                // Use faster pulsing for magnetic attraction
                qreal pulse = 0.7 + 0.3 * qSin(QDateTime::currentMSecsSinceEpoch() * 0.02);
                QColor highlightColor = QColor(0, 255, 100, 100 + 55 * pulse);
                QColor glowColor = QColor(0, 255, 100, 100 + 100 * pulse);
                
                // Draw outer glow for magnetic attraction
                painter->setBrush(QBrush(highlightColor));
                painter->setPen(QPen(glowColor, 3));
                painter->drawRoundedRect(rect.adjusted(-3, -3, 3, 3), 5.0, 5.0);
                
                // Draw inner socket with enhanced colors
                painter->setBrush(socketColor.lighter(150));
                painter->setPen(QPen(borderColor.lighter(120), 2));
                painter->drawRoundedRect(rect, 3.0, 3.0);
                
                // Draw magnetic attraction indicator
                QRectF magnetRect = rect.adjusted(2, 2, -2, -2);
                painter->setBrush(QBrush(QColor(255, 255, 255, 200)));
                painter->setPen(QPen(Qt::white, 1));
                painter->drawEllipse(magnetRect);
            }
            break;
            
        case Connecting:
            // Draw connecting state (for source socket during drag)
            painter->setBrush(QBrush(socketColor.lighter(110)));
            painter->setPen(QPen(borderColor.darker(120), 3));
            painter->drawRoundedRect(rect, 3.0, 3.0);
            break;
            
        case Disconnected:
        default:
            // Normal socket appearance - rounded square
            painter->setBrush(socketColor);
            painter->setPen(QPen(borderColor, 2));
            painter->drawRoundedRect(rect, 3.0, 3.0);
            break;
    }
    
    // Draw socket index number with better contrast
    if (rect.width() > 8) { // Only draw index if socket is large enough
        painter->setPen(Qt::white);
        
        // Performance optimization: static font (created once, not every frame)
        static const QFont socketFont("Arial", 7, QFont::Bold);
        painter->setFont(socketFont);
        
        // Performance optimization: cache index string (created once, not every frame)
        if (m_cachedIndexString.isEmpty()) {
            m_cachedIndexString = QString::number(m_index);
        }
        painter->drawText(rect, Qt::AlignCenter, m_cachedIndexString);
    }
}

void Socket::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    m_pressed = true;
    update(); // Show pressed state immediately
    
    // DISABLE dragging from connected sockets
    if (isConnected()) {
        qDebug() << "Socket" << m_index << "is connected - dragging disabled";
        event->ignore(); // Don't start drag operations on connected sockets
        return;
    }
    
    if (event->button() == Qt::LeftButton) {
        qDebug() << "Socket clicked: index:" << m_index << "role:" << (m_role == Input ? "Input" : "Output");
        // TODO: Start edge creation drag
        event->accept();
    } else if (event->button() == Qt::RightButton && m_role == Output) {
        qDebug() << "Socket right-clicked: index:" << m_index << "role:" << (m_role == Input ? "Input" : "Output");
        // Start ghost edge from output socket
        Scene* scene = qobject_cast<Scene*>(this->scene());
        if (scene) {
            scene->startGhostEdge(this, event->scenePos());
        }
        event->accept();
    }
    QGraphicsItem::mousePressEvent(event);
}

void Socket::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    m_pressed = false;
    update(); // Remove pressed state
    
    if (event->button() == Qt::LeftButton) {
        qDebug() << "Socket released: index:" << m_index;
        // TODO: Complete edge connection
        event->accept();
    }
    QGraphicsItem::mouseReleaseEvent(event);
}

void Socket::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    Q_UNUSED(event)
    m_hovered = true;
    m_hoverOpacity = 1.0; // Animate to full opacity
    update();
}

void Socket::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    Q_UNUSED(event)
    m_hovered = false;
    m_hoverOpacity = 0.0; // Animate to no opacity
    update();
}


xmlNodePtr Socket::write(xmlDocPtr doc, xmlNodePtr repr) const
{
    Q_UNUSED(doc)
    Q_UNUSED(repr)
    // Sockets are written as part of their parent node
    return nullptr;
}

void Socket::read(xmlNodePtr node)
{
    Q_UNUSED(node)
    // Socket properties read from parent node's socket definitions
    // Position is set by parent node's positionAllSockets() method
}

QVariant Socket::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == ItemSelectedHasChanged) {
        bool isNowSelected = value.toBool();
        qDebug() << "Socket" << roleToString(m_role) << "index" << m_index 
                 << (isNowSelected ? "SELECTED" : "DESELECTED");
        
        // CRITICAL: When selected, take keyboard focus for delete key events
        if (isNowSelected) {
            setFocus(Qt::MouseFocusReason);
            qDebug() << "Socket: Taking keyboard focus for delete key handling";
        }
        
        // Trigger visual update when selection changes
        update();
    }
    
    return QGraphicsItem::itemChange(change, value);
}

// Note: Delete key handling moved to Scene::keyPressEvent() for centralized multi-selection support


QPainterPath Socket::shape() const {
    QPainterPath p;
    p.addEllipse(boundingRect());
    QPainterPathStroker s;
    s.setWidth(GraphConstants::PICK_WIDTH);
    return s.createStroke(p);
}
