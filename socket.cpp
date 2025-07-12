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
    // Smaller, more compact sockets like upper level system
    return QRectF(-6, -6, 12, 12);
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
    
    // Draw socket as rounded rectangle with better styling
    QRectF rect = boundingRect();
    painter->setBrush(socketColor);
    painter->setPen(QPen(borderColor, 2));
    painter->drawRoundedRect(rect, 3.0, 3.0);
    
    // Draw socket index number with better contrast
    if (rect.width() > 8) { // Only draw index if socket is large enough
        painter->setPen(Qt::white);
        
        // Performance optimization: static font (created once, not every frame)
        static const QFont socketFont("Arial", 6, QFont::Bold);
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
    if (event->button() == Qt::RightButton && m_role == Output) {
        // Start ghost edge from Output socket only
        Scene* scenePtr = qobject_cast<Scene*>(scene());
        if (scenePtr) {
            scenePtr->startGhostEdge(this, event->scenePos());
        }
        event->accept();
        return;
    }
    
    if (event->button() == Qt::LeftButton) {
        qDebug() << "Socket clicked: index:" << m_index << "role:" << (m_role == Input ? "Input" : "Output");
        // TODO: Start edge creation drag
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
    const qreal socketSpacing = 18.0;  // Reduced spacing for smaller nodes
    const qreal socketOffset = 3.0;    // Even closer to node edge
    
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
    const qreal socketSpacing = 18.0;  // Reduced spacing for smaller nodes
    const qreal socketOffset = 3.0;    // Even closer to node edge
    
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