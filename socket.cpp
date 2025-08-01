#include "socket.h"
#include "node.h"
#include "edge.h"
#include "scene.h"
#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include <QDebug>
#include <QDateTime>
#include <QtMath>

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
    // ✅ NO positioning in constructor - will be positioned later with complete information
    
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
            // Draw outer glow for connected sockets
            painter->setBrush(QBrush(socketColor.lighter(120)));
            painter->setPen(QPen(borderColor.lighter(150), 1));
            painter->drawRoundedRect(rect.adjusted(-2, -2, 2, 2), 4.0, 4.0);
            
            // Draw inner highlight
            painter->setBrush(socketColor.lighter(140));
            painter->setPen(QPen(borderColor, 3));
            painter->drawRoundedRect(rect, 3.0, 3.0);
            
            // Draw connection indicator (small center square)
            painter->setBrush(QBrush(Qt::white));
            painter->setPen(Qt::NoPen);
            painter->drawRoundedRect(rect.adjusted(4, 4, -4, -4), 1.0, 1.0);
            break;
            
        case Highlighted:
            {
                // Draw pulsing highlight for valid connection targets
                // Use time-based pulsing effect
                qreal pulse = 0.5 + 0.5 * qSin(QDateTime::currentMSecsSinceEpoch() * 0.01);
                QColor highlightColor = QColor(0, 255, 0, 80 + 40 * pulse);
                painter->setBrush(QBrush(highlightColor));
                painter->setPen(QPen(QColor(0, 255, 0, 150 + 50 * pulse), 2));
                painter->drawRoundedRect(rect.adjusted(-2, -2, 2, 2), 4.0, 4.0);
                
                painter->setBrush(socketColor.lighter(130));
                painter->setPen(QPen(borderColor, 2));
                painter->drawRoundedRect(rect, 3.0, 3.0);
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


