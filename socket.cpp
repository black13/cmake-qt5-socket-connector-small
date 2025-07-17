#include "socket.h"
#include "node.h"
#include "edge.h"
#include "scene.h"
#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include <QDebug>

Socket::Socket(Role role, Node* parentNode, int index)
    : QGraphicsItem(parentNode)
    , m_role(role)
    , m_index(index)
    , m_connectedEdge(nullptr)
    , m_connectionState(Disconnected)
    , m_radius(8.0)
    , m_hovered(false)
{
    setAcceptHoverEvents(true);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    updatePosition();
    
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
    // Larger, more visible sockets for better user interaction
    return QRectF(-10, -10, 20, 20);
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
    
    // Add hover effect
    if (m_hovered) {
        socketColor = socketColor.lighter(150);
        borderColor = borderColor.lighter(130);
    }
    
    // Draw socket as circular with enhanced styling
    QRectF rect = boundingRect();
    
    // Add connection state visual feedback
    switch (m_connectionState) {
        case Connected:
            // Draw outer glow for connected sockets
            painter->setBrush(QBrush(socketColor.lighter(120)));
            painter->setPen(QPen(borderColor.lighter(150), 1));
            painter->drawEllipse(rect.adjusted(-3, -3, 3, 3));
            
            // Draw inner highlight
            painter->setBrush(socketColor.lighter(140));
            painter->setPen(QPen(borderColor, 4));
            painter->drawEllipse(rect);
            
            // Draw connection indicator (small center dot)
            painter->setBrush(QBrush(Qt::white));
            painter->setPen(Qt::NoPen);
            painter->drawEllipse(rect.adjusted(6, 6, -6, -6));
            break;
            
        case Highlighted:
            // Draw pulsing highlight for valid connection targets
            painter->setBrush(QBrush(QColor(0, 255, 0, 100))); // Green highlight
            painter->setPen(QPen(QColor(0, 255, 0), 2));
            painter->drawEllipse(rect.adjusted(-2, -2, 2, 2));
            
            painter->setBrush(socketColor.lighter(130));
            painter->setPen(QPen(borderColor, 3));
            painter->drawEllipse(rect);
            break;
            
        case Connecting:
            // Draw connecting state (for source socket during drag)
            painter->setBrush(QBrush(socketColor.lighter(110)));
            painter->setPen(QPen(borderColor.darker(120), 3));
            painter->drawEllipse(rect);
            break;
            
        case Disconnected:
        default:
            // Normal socket appearance
            painter->setBrush(socketColor);
            painter->setPen(QPen(borderColor, 3));
            painter->drawEllipse(rect);
            break;
    }
    
    // Draw socket index number with better contrast
    if (rect.width() > 8) { // Only draw index if socket is large enough
        painter->setPen(Qt::white);
        
        // Performance optimization: static font (created once, not every frame)
        static const QFont socketFont("Arial", 8, QFont::Bold);
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
    update();
}

void Socket::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    Q_UNUSED(event)
    m_hovered = false;
    update();
}

void Socket::updatePosition()
{
    Node* parent = getParentNode();
    if (!parent) return;
    
    QRectF nodeRect = parent->boundingRect();
    const qreal socketSpacing = 28.0;  // Increased spacing for larger sockets
    const qreal socketOffset = 5.0;    // Proper offset for larger sockets
    
    // Count input and output sockets to calculate proper vertical positioning
    int inputCount = 0;
    int outputCount = 0;
    int myInputIndex = -1;
    int myOutputIndex = -1;
    
    // Find socket counts and my position within role
    for (QGraphicsItem* child : parent->childItems()) {
        if (Socket* socket = qgraphicsitem_cast<Socket*>(child)) {
            if (socket->getRole() == Socket::Input) {
                if (socket == this) myInputIndex = inputCount;
                inputCount++;
            } else {
                if (socket == this) myOutputIndex = outputCount;
                outputCount++;
            }
        }
    }
    
    if (m_role == Input) {
        // Input sockets on left side, centered vertically
        qreal totalInputHeight = (inputCount - 1) * socketSpacing;
        qreal startY = nodeRect.center().y() - totalInputHeight / 2.0;
        setPos(-socketOffset, startY + (myInputIndex * socketSpacing));
    } else {
        // Output sockets on right side, centered vertically  
        qreal totalOutputHeight = (outputCount - 1) * socketSpacing;
        qreal startY = nodeRect.center().y() - totalOutputHeight / 2.0;
        setPos(nodeRect.width() + socketOffset, startY + (myOutputIndex * socketSpacing));
    }
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
    updatePosition();
}


QPointF Socket::calculatePosition() const
{
    Node* parent = getParentNode();
    if (!parent) return QPointF(0, 0);
    
    QRectF nodeRect = parent->boundingRect();
    const qreal socketSpacing = 28.0;  // Increased spacing for larger sockets
    const qreal socketOffset = 5.0;    // Proper offset for larger sockets
    
    // Count input and output sockets for proper centering
    int inputCount = 0;
    int outputCount = 0;
    int myInputIndex = -1;
    int myOutputIndex = -1;
    
    for (QGraphicsItem* child : parent->childItems()) {
        if (Socket* socket = qgraphicsitem_cast<Socket*>(child)) {
            if (socket->getRole() == Socket::Input) {
                if (socket == this) myInputIndex = inputCount;
                inputCount++;
            } else {
                if (socket == this) myOutputIndex = outputCount;
                outputCount++;
            }
        }
    }
    
    if (m_role == Input) {
        qreal totalInputHeight = (inputCount - 1) * socketSpacing;
        qreal startY = nodeRect.center().y() - totalInputHeight / 2.0;
        return QPointF(-socketOffset, startY + (myInputIndex * socketSpacing));
    } else {
        qreal totalOutputHeight = (outputCount - 1) * socketSpacing;
        qreal startY = nodeRect.center().y() - totalOutputHeight / 2.0;
        return QPointF(nodeRect.width() + socketOffset, startY + (myOutputIndex * socketSpacing));
    }
}