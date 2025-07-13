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
    , m_connectedEdges()  // Initialize empty QSet
    , m_radius(12.0)
    , m_hovered(false)
    , m_visualState(Normal)
{
    setAcceptHoverEvents(true);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    updatePosition();
    
    // Register with parent node for O(1) lookups
    if (parentNode) {
        parentNode->registerSocket(this, m_index);
    }
    
    // Socket creation - keep minimal logging
}

Node* Socket::getParentNode() const
{
    return qgraphicsitem_cast<Node*>(parentItem());
}

QRectF Socket::boundingRect() const
{
    // Larger sockets for better usability and ghost edge interaction
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
    
    // Apply visual state effects
    switch (m_visualState) {
        case Hovered:
            socketColor = socketColor.lighter(150);
            borderColor = borderColor.lighter(130);
            break;
        case ValidTarget:
            // Green glow effect for valid ghost edge target
            socketColor = QColor(100, 255, 100);  // Bright green
            borderColor = QColor(0, 200, 0);      // Green border
            break;
        case InvalidTarget:
            // Red glow effect for invalid ghost edge target
            socketColor = QColor(255, 100, 100);  // Bright red
            borderColor = QColor(200, 0, 0);      // Red border
            break;
        case Connected:
            // Subtle highlight for connected sockets
            borderColor = borderColor.lighter(120);
            break;
        case Normal:
        default:
            // Keep original colors
            break;
    }
    
    // Draw socket as rounded rectangle with better styling
    QRectF rect = boundingRect();
    
    // Add glow effect for target states
    if (m_visualState == ValidTarget || m_visualState == InvalidTarget) {
        QColor glowColor = (m_visualState == ValidTarget) ? QColor(0, 255, 0, 100) : QColor(255, 0, 0, 100);
        
        // Draw outer glow
        for (int i = 1; i <= 3; ++i) {
            QColor fadeColor = glowColor;
            fadeColor.setAlpha(glowColor.alpha() / (i * 2));
            painter->setPen(QPen(fadeColor, 2 + i));
            painter->setBrush(Qt::NoBrush);
            painter->drawRoundedRect(rect.adjusted(-i, -i, i, i), 3.0 + i, 3.0 + i);
        }
    }
    
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
        // TODO: Start edge creation drag
        event->accept();
    }
    QGraphicsItem::mousePressEvent(event);
}

void Socket::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        // TODO: Complete edge connection
        event->accept();
    }
    QGraphicsItem::mouseReleaseEvent(event);
}

void Socket::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    Q_UNUSED(event)
    m_hovered = true;
    if (m_visualState == Normal) {
        m_visualState = Hovered;
    }
    update();
}

void Socket::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    Q_UNUSED(event)
    m_hovered = false;
    if (m_visualState == Hovered) {
        m_visualState = Normal;
    }
    update();
}

void Socket::setVisualState(VisualState state)
{
    if (m_visualState != state) {
        m_visualState = state;
        update();  // Trigger repaint with new visual state
    }
}

void Socket::updatePosition()
{
    Node* parent = getParentNode();
    if (!parent) return;
    
    QRectF nodeRect = parent->boundingRect();
    const qreal socketSpacing = 30.0;  // Increased spacing for better usability
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
    const qreal socketSpacing = 30.0;  // Increased spacing for better usability
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

// ============================================================================
// Multi-Edge Connection Support (Patch Implementation)
// ============================================================================

void Socket::addConnectedEdge(Edge* edge)
{
    if (edge && !m_connectedEdges.contains(edge)) {
        m_connectedEdges.insert(edge);
        qDebug() << "SOCKET" << m_index << ": +Edge (" << m_connectedEdges.size() << "total)";
    }
}

void Socket::removeConnectedEdge(Edge* edge)
{
    if (!edge) return;
    
    if (m_connectedEdges.contains(edge)) {
        m_connectedEdges.remove(edge);
        qDebug() << "SOCKET" << m_index << ": -Edge (" << m_connectedEdges.size() << "remaining)";
    }
}

void Socket::clearAllConnections()
{
    if (!m_connectedEdges.isEmpty()) {
        qDebug() << "PHASE1: Socket emergency clear (" << m_connectedEdges.size() << "connections)";
        m_connectedEdges.clear();
    }
}

// Legacy compatibility methods - support existing code
void Socket::setConnectedEdge(Edge* edge)
{
    qWarning() << "setConnectedEdge() is deprecated - use addConnectedEdge()";
    if (edge) {
        addConnectedEdge(edge);
    }
}

Edge* Socket::getConnectedEdge() const
{
    if (m_connectedEdges.isEmpty()) {
        return nullptr;
    }
    // Return first edge for legacy compatibility
    return *m_connectedEdges.begin();
}