#include "edge.h"
#include "socket.h"
#include "node.h"
#include "scene.h"
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QWidget>
#include <QDebug>
#include <libxml/tree.h>
#include <cmath>

Edge::Edge(const QUuid& id, const QUuid& fromSocketId, const QUuid& toSocketId)
    : m_id(id)
    , m_fromNodeId()
    , m_toNodeId()
    , m_fromNodeUuid()
    , m_toNodeUuid()
    , m_fromSocketIndex(-1)
    , m_toSocketIndex(-1)
    , m_fromSocket(nullptr)
    , m_toSocket(nullptr)
    , m_fromNode(nullptr)
    , m_toNode(nullptr)
    , m_hovered(false)
    #ifdef QT_DEBUG
    , m_shapeCallCount(0)
    #endif
{
    Q_UNUSED(fromSocketId)  // Legacy parameter, not used in clean design
    Q_UNUSED(toSocketId)    // Legacy parameter, not used in clean design
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemIsFocusable, true); // Enable keyboard events
    setFlag(QGraphicsItem::ItemHasNoContents, false); // Ensure we control our own drawing
    setAcceptHoverEvents(true);  // Enable hover events for better interaction
    
    // Z-order hierarchy: Nodes(0) < Sockets(1) < Edges(2)
    // Edges appear on top of sockets for "plugged-in" visual effect
    setZValue(2);
    
    qDebug() << "+Edge" << m_id.toString(QUuid::WithoutBraces).left(8);
    // Don't call updatePath() here - sockets not resolved yet
}

Edge::~Edge()
{
    // SAFETY: Only touch nodes that are still valid (not nulled by invalidateNode)
    if (m_fromNode) {
        m_fromNode->unregisterEdge(this);
    }
    if (m_toNode) {
        m_toNode->unregisterEdge(this);
    }
    
    qDebug() << "~Edge" << m_id.toString(QUuid::WithoutBraces).left(8);
}

void Edge::invalidateNode(const Node* node)
{
    // Manual weak pointer nulling - called by Node::~Node()
    if (node == m_fromNode) {
        m_fromNode = nullptr;
    }
    if (node == m_toNode) {
        m_toNode = nullptr;
    }
}

QRectF Edge::boundingRect() const
{
    return m_boundingRect;
}

void Edge::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(widget)
    Q_UNUSED(option) // Don't use Qt's default drawing options
    
    painter->setRenderHint(QPainter::Antialiasing);
    
    // Save painter state to ensure no side effects
    painter->save();
    
    // Make sure no brush is set (no fill)
    painter->setBrush(Qt::NoBrush);
    
    // IMPROVED: Multi-layer cable-like rendering with depth
    if (isSelected()) {
        // Selection: bright orange with glow effect
        QPen glowPen(QColor(255, 69, 0, 100), 12);
        glowPen.setCapStyle(Qt::RoundCap);
        painter->setPen(glowPen);
        painter->drawPath(m_path);
        
        QPen selectionPen(QColor(255, 69, 0), 6);
        selectionPen.setCapStyle(Qt::RoundCap);
        painter->setPen(selectionPen);
        painter->drawPath(m_path);
    } else if (m_hovered) {
        // Hover: blue with subtle glow
        QPen hoverGlowPen(QColor(100, 150, 255, 80), 8);
        hoverGlowPen.setCapStyle(Qt::RoundCap);
        painter->setPen(hoverGlowPen);
        painter->drawPath(m_path);
        
        QPen hoverPen(QColor(100, 150, 255), 4);
        hoverPen.setCapStyle(Qt::RoundCap);
        painter->setPen(hoverPen);
        painter->drawPath(m_path);
    } else {
        // Normal: layered cable appearance with depth
        // Layer 1: Shadow for depth
        QPen shadowPen(QColor(0, 0, 0, 60), 5);
        shadowPen.setCapStyle(Qt::RoundCap);
        painter->setPen(shadowPen);
        QPainterPath shadowPath = m_path;
        shadowPath.translate(1.5, 1.5);
        painter->drawPath(shadowPath);
        
        // Layer 2: Dark outline for definition
        QPen outlinePen(QColor(40, 40, 40), 4);
        outlinePen.setCapStyle(Qt::RoundCap);
        painter->setPen(outlinePen);
        painter->drawPath(m_path);
        
        // Layer 3: Main cable body with subtle gradient effect
        QPen mainPen(QColor(85, 85, 85), 3);
        mainPen.setCapStyle(Qt::RoundCap);
        painter->setPen(mainPen);
        painter->drawPath(m_path);
        
        // Layer 4: Highlight for 3D cable effect
        QPen highlightPen(QColor(120, 120, 120), 1);
        highlightPen.setCapStyle(Qt::RoundCap);
        painter->setPen(highlightPen);
        painter->drawPath(m_path);
    }
    
    // Restore painter state
    painter->restore();
}

QPainterPath Edge::shape() const
{
    // Create a much wider path for easier selection - very generous selection area
    QPainterPathStroker stroker;
    stroker.setWidth(20);  // Very wide selection area for easy clicking
    stroker.setCapStyle(Qt::RoundCap);
    stroker.setJoinStyle(Qt::RoundJoin);
    QPainterPath selectionPath = stroker.createStroke(m_path);
    
    #ifdef QT_DEBUG
    // Debug: Log when shape is queried (indicates potential interaction)
    // Per-edge counter avoids thread safety issues with global static
    ++m_shapeCallCount;
    if (m_shapeCallCount % 100 == 0) {  // Throttled logging to avoid spam
        qDebug() << "Edge" << m_id.toString(QUuid::WithoutBraces).left(8) 
                 << "shape() called" << m_shapeCallCount << "times";
    }
    #endif
    
    return selectionPath;
}

// Removed manual setSelected - using Qt's selection system

QVariant Edge::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == ItemSelectedHasChanged) {
        // Selection tracking logging - what has been selected
        bool wasSelected = isSelected();
        bool willBeSelected = value.toBool();
        qDebug() << "=== EDGE SELECTION CHANGE ===";
        qDebug() << "Edge" << m_id.toString(QUuid::WithoutBraces).left(8) 
                 << "changing from" << (wasSelected ? "SELECTED" : "NOT_SELECTED")
                 << "to" << (willBeSelected ? "SELECTED" : "NOT_SELECTED");
        qDebug() << "Edge will" << (willBeSelected ? "turn ORANGE" : "turn NORMAL");
        
        // CRITICAL: When selected, take keyboard focus for delete key events
        if (willBeSelected) {
            setFocus(Qt::MouseFocusReason);
            qDebug() << "Edge: Taking keyboard focus for delete key handling";
        }
        
        // Trigger visual update when selection changes
        update();
        qDebug() << "=== EDGE SELECTION CHANGE COMPLETE ===";
    }
    return QGraphicsItem::itemChange(change, value);
}

void Edge::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    qDebug() << "=== EDGE MOUSE PRESS START ===";
    qDebug() << "Edge" << m_id.toString(QUuid::WithoutBraces).left(8) << "mousePressEvent at" << event->pos();
    qDebug() << "Edge was selected BEFORE mouse press:" << isSelected();
    qDebug() << "Mouse button:" << (event->button() == Qt::LeftButton ? "LEFT" : "OTHER");
    qDebug() << "Modifiers:" << event->modifiers();
    
    QGraphicsItem::mousePressEvent(event);
    
    qDebug() << "Edge is selected AFTER mouse press:" << isSelected();
    qDebug() << "=== EDGE MOUSE PRESS END ===";
}

void Edge::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    qDebug() << "=== EDGE MOUSE RELEASE START ===";
    qDebug() << "Edge" << m_id.toString(QUuid::WithoutBraces).left(8) << "mouseReleaseEvent at" << event->pos();
    qDebug() << "Edge is selected BEFORE mouse release:" << isSelected();
    
    QGraphicsItem::mouseReleaseEvent(event);
    
    qDebug() << "Edge is selected AFTER mouse release:" << isSelected();
    qDebug() << "Edge turns orange (selected):" << (isSelected() ? "YES" : "NO");
    qDebug() << "=== EDGE MOUSE RELEASE END ===";
}

void Edge::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    Q_UNUSED(event)
    m_hovered = true;
    qDebug() << "Edge" << m_id.toString(QUuid::WithoutBraces).left(8) << "HOVER ENTER";
    update();  // Redraw to show hover effect
    QGraphicsItem::hoverEnterEvent(event);
}

void Edge::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    Q_UNUSED(event)
    m_hovered = false;
    qDebug() << "Edge" << m_id.toString(QUuid::WithoutBraces).left(8) << "HOVER LEAVE";
    update();  // Redraw to remove hover effect
    QGraphicsItem::hoverLeaveEvent(event);
}

void Edge::updatePath()
{
    // Simple and clean: use direct socket pointers
    if (!m_fromSocket || !m_toSocket) {
        // Edge not resolved yet - notify BSP cache before clearing
        prepareGeometryChange();
        m_path = QPainterPath();
        m_boundingRect = QRectF();
        return;
    }
    
    // FIXED: Connect to exact socket centers in scene coordinates
    // Use mapToScene to get the socket's center point in scene coordinates
    QRectF fromRect = m_fromSocket->boundingRect();
    QRectF toRect = m_toSocket->boundingRect();
    
    // Get the center of each socket in its local coordinates, then map to scene
    QPointF start = m_fromSocket->mapToScene(fromRect.center());
    QPointF end = m_toSocket->mapToScene(toRect.center());
    
    buildPath(start, end);
}



void Edge::buildPath(const QPointF& start, const QPointF& end)
{
    // Validate input points
    if (start.isNull() || end.isNull() || !qIsFinite(start.x()) || !qIsFinite(start.y()) || 
        !qIsFinite(end.x()) || !qIsFinite(end.y())) {
        // Notify BSP cache before clearing
        prepareGeometryChange();
        m_path = QPainterPath();
        m_boundingRect = QRectF();
        return;
    }
    
    // Clear and rebuild path safely
    m_path.clear();
    
    // ENHANCED: Connect directly to socket centers (no adjustment needed)
    // Since updatePath() now provides socket centers, use them directly
    QPointF adjustedStart = start;
    QPointF adjustedEnd = end;
    
    m_path.moveTo(adjustedStart);
    
    // IMPROVED: Dynamic curve calculation based on distance and orientation
    qreal dx = adjustedEnd.x() - adjustedStart.x();
    qreal dy = adjustedEnd.y() - adjustedStart.y();
    qreal distance = std::sqrt(dx * dx + dy * dy);
    
    // Adaptive control point calculation for better curves
    qreal horizontalFactor = qAbs(dx) / qMax(distance, 1.0);
    qreal verticalFactor = qAbs(dy) / qMax(distance, 1.0);
    
    // Dynamic control offset based on distance and direction
    qreal controlOffset;
    if (horizontalFactor > 0.8) {
        // Mostly horizontal: use classic Bezier with distance-based offset
        controlOffset = qMax(qAbs(dx) * 0.4, qMin(distance * 0.3, 150.0));
    } else {
        // More vertical: tighter curves for better routing
        controlOffset = qMax(40.0, qMin(distance * 0.2, 80.0));
    }
    
    // Enhanced control point positioning for natural cable-like curves
    QPointF control1, control2;
    
    if (dx >= 0) {
        // Left-to-right: standard horizontal Bezier
        control1 = adjustedStart + QPointF(controlOffset, 0);
        control2 = adjustedEnd - QPointF(controlOffset, 0);
    } else {
        // Right-to-left: S-curve for better visual routing
        qreal verticalOffset = qAbs(dy) * 0.3;
        control1 = adjustedStart + QPointF(controlOffset * 0.6, dy > 0 ? verticalOffset : -verticalOffset);
        control2 = adjustedEnd - QPointF(controlOffset * 0.6, dy > 0 ? verticalOffset : -verticalOffset);
    }
    
    // Create smooth cubic Bezier curve
    m_path.cubicTo(control1, control2, adjustedEnd);
    
    // Notify Qt's BSP cache before changing bounding rectangle
    prepareGeometryChange();
    
    // Update bounding rectangle with validation
    QRectF pathBounds = m_path.boundingRect();
    if (pathBounds.isValid()) {
        // Inflate by strokeWidth/2 = 10 to match stroker.setWidth(20)
        m_boundingRect = pathBounds.adjusted(-10, -10, 10, 10);
    } else {
        // Inflate by strokeWidth/2 = 10 to match stroker.setWidth(20)
        m_boundingRect = QRectF(start, end).normalized().adjusted(-10, -10, 10, 10);
    }
}


xmlNodePtr Edge::write(xmlDocPtr doc, xmlNodePtr repr) const
{
    xmlNodePtr node = xmlNewNode(nullptr, BAD_CAST "edge");
    
    // Core attributes - clean design uses node+index format
    xmlSetProp(node, BAD_CAST "id", BAD_CAST m_id.toString(QUuid::WithoutBraces).toUtf8().constData());
    xmlSetProp(node, BAD_CAST "fromNode", BAD_CAST m_fromNodeId.toUtf8().constData());
    xmlSetProp(node, BAD_CAST "toNode", BAD_CAST m_toNodeId.toUtf8().constData());
    xmlSetProp(node, BAD_CAST "fromSocketIndex", BAD_CAST QString::number(m_fromSocketIndex).toUtf8().constData());
    xmlSetProp(node, BAD_CAST "toSocketIndex", BAD_CAST QString::number(m_toSocketIndex).toUtf8().constData());
    
    if (repr) {
        xmlAddChild(repr, node);
    } else {
        xmlDocSetRootElement(doc, node);
    }
    
    return node;
}

void Edge::read(xmlNodePtr node)
{
    if (!node) return;
    
    // Read UUID
    xmlChar* idStr = xmlGetProp(node, BAD_CAST "id");
    if (idStr) {
        m_id = QUuid(QString::fromUtf8((char*)idStr));
        xmlFree(idStr);
    }
    
    // Read node+index references - store for later resolution
    // Support both formats: new (fromNode/toNode) and legacy (from/to)
    xmlChar* fromNodeStr = xmlGetProp(node, BAD_CAST "fromNode");
    if (!fromNodeStr) fromNodeStr = xmlGetProp(node, BAD_CAST "from");
    
    xmlChar* toNodeStr = xmlGetProp(node, BAD_CAST "toNode");
    if (!toNodeStr) toNodeStr = xmlGetProp(node, BAD_CAST "to");
    
    xmlChar* fromIndexStr = xmlGetProp(node, BAD_CAST "fromSocketIndex");
    if (!fromIndexStr) fromIndexStr = xmlGetProp(node, BAD_CAST "from-socket");
    
    xmlChar* toIndexStr = xmlGetProp(node, BAD_CAST "toSocketIndex");
    if (!toIndexStr) toIndexStr = xmlGetProp(node, BAD_CAST "to-socket");
    
    if (!fromNodeStr || !toNodeStr || !fromIndexStr || !toIndexStr) {
        qCritical() << "Edge::read() - Missing required node+index attributes";
        if (fromNodeStr) xmlFree(fromNodeStr);
        if (toNodeStr) xmlFree(toNodeStr);
        if (fromIndexStr) xmlFree(fromIndexStr);
        if (toIndexStr) xmlFree(toIndexStr);
        return;
    }
    
    // Store data for later resolution - don't try to resolve now
    m_fromNodeId = QString::fromUtf8((char*)fromNodeStr);
    m_toNodeId = QString::fromUtf8((char*)toNodeStr);
    m_fromSocketIndex = QString::fromUtf8((char*)fromIndexStr).toInt();
    m_toSocketIndex = QString::fromUtf8((char*)toIndexStr).toInt();
    
    // Performance optimization: cache UUIDs for fast comparison
    m_fromNodeUuid = QUuid(m_fromNodeId);
    m_toNodeUuid = QUuid(m_toNodeId);
    
    qDebug() << "Edge: Stored connection data fromNode" << m_fromNodeId.left(8) 
             << "socket" << m_fromSocketIndex << "-> toNode" << m_toNodeId.left(8) 
             << "socket" << m_toSocketIndex;
    
    xmlFree(fromNodeStr);
    xmlFree(toNodeStr);
    xmlFree(fromIndexStr);
    xmlFree(toIndexStr);
    
    // DON'T call updatePath() here - scene may not be ready
    // Socket resolution will happen later via resolveConnections()
}

bool Edge::isConnectedToNode(const QString& nodeId) const
{
    return (m_fromNodeId == nodeId || m_toNodeId == nodeId);
}

bool Edge::isConnectedToNode(const QUuid& nodeId) const
{
    // Optimized: Fast UUID comparison (no string conversion)
    return (m_fromNodeUuid == nodeId || m_toNodeUuid == nodeId);
}

bool Edge::resolveConnections(Scene* scene)
{
    if (!scene) {
        qCritical() << "Edge::resolveConnections - null scene";
        return false;
    }
    
    if (m_fromNodeId.isEmpty() || m_toNodeId.isEmpty()) {
        qCritical() << "Edge::resolveConnections - empty node IDs";
        return false;
    }
    
    // Find nodes by UUID
    Node* fromNode = scene->getNode(QUuid(m_fromNodeId));
    Node* toNode = scene->getNode(QUuid(m_toNodeId));
    
    if (!fromNode) {
        qCritical() << "Edge::resolveConnections - fromNode not found:" << m_fromNodeId.left(8);
        return false;
    }
    if (!toNode) {
        qCritical() << "Edge::resolveConnections - toNode not found:" << m_toNodeId.left(8);
        return false;
    }
    
    // Find sockets by index
    Socket* fromSocket = fromNode->getSocketByIndex(m_fromSocketIndex);
    Socket* toSocket = toNode->getSocketByIndex(m_toSocketIndex);
    
    qDebug() << "Edge resolve: fromNode" << m_fromNodeId.left(8) << "type:" << fromNode->getNodeType()
             << "socket" << m_fromSocketIndex << "role:" << (fromSocket ? Socket::roleToString(fromSocket->getRole()) : "NULL");
    qDebug() << "Edge resolve: toNode" << m_toNodeId.left(8) << "type:" << toNode->getNodeType()
             << "socket" << m_toSocketIndex << "role:" << (toSocket ? Socket::roleToString(toSocket->getRole()) : "NULL");
    
    if (!fromSocket) {
        qCritical() << "Edge::resolveConnections - fromSocket index" << m_fromSocketIndex 
                   << "not found in node" << m_fromNodeId.left(8) 
                   << "with" << fromNode->getSocketCount() << "sockets";
        return false;
    }
    if (!toSocket) {
        qCritical() << "Edge::resolveConnections - toSocket index" << m_toSocketIndex 
                   << "not found in node" << m_toNodeId.left(8) 
                   << "with" << toNode->getSocketCount() << "sockets";
        return false;
    }
    
    // Validate connection rules
    if (fromSocket->getRole() != Socket::Output) {
        qCritical() << "ERROR: Edge::resolveConnections - fromSocket must be Output role"
                   << "- fromNode:" << m_fromNodeId.left(8) << "socket" << m_fromSocketIndex 
                   << "has role:" << Socket::roleToString(fromSocket->getRole());
        return false;
    }
    if (toSocket->getRole() != Socket::Input) {
        qCritical() << "ERROR: Edge::resolveConnections - toSocket must be Input role"
                   << "- toNode:" << m_toNodeId.left(8) << "socket" << m_toSocketIndex 
                   << "has role:" << Socket::roleToString(toSocket->getRole());
        return false;
    }
    
    // Store socket references directly - NO UUIDs
    m_fromSocket = fromSocket;
    m_toSocket = toSocket;
    
    // Cache node pointers for safe destruction
    m_fromNode = fromNode;
    m_toNode = toNode;
    
    // PERFORMANCE OPTIMIZATION: Register this edge with both connected nodes
    // This enables O(degree) edge updates instead of O(totalEdges)
    fromNode->registerEdge(this);
    toNode->registerEdge(this);
    
    qDebug() << "Edge" << m_id.toString(QUuid::WithoutBraces).left(8) << "resolved" 
             << m_fromSocketIndex << "->" << m_toSocketIndex;
    
    updatePath();
    return true;
}

void Edge::setConnectionData(const QString& fromNodeId, const QString& toNodeId, 
                            int fromSocketIndex, int toSocketIndex)
{
    m_fromNodeId = fromNodeId;
    m_toNodeId = toNodeId;
    m_fromSocketIndex = fromSocketIndex;
    m_toSocketIndex = toSocketIndex;
    
    // Performance optimization: cache UUIDs for fast comparison
    m_fromNodeUuid = QUuid(fromNodeId);
    m_toNodeUuid = QUuid(toNodeId);
    
    qDebug() << "Edge: Set connection data" << fromNodeId.left(8) 
             << "socket" << fromSocketIndex << "-> " << toNodeId.left(8) 
             << "socket" << toSocketIndex;
}

void Edge::setResolvedSockets(Socket* fromSocket, Socket* toSocket)
{
    if (!fromSocket || !toSocket) {
        qCritical() << "Edge::setResolvedSockets - null socket(s) provided";
        return;
    }
    
    // Validate socket roles
    if (fromSocket->getRole() != Socket::Output) {
        qCritical() << "Edge::setResolvedSockets - fromSocket must be Output role";
        return;
    }
    if (toSocket->getRole() != Socket::Input) {
        qCritical() << "Edge::setResolvedSockets - toSocket must be Input role";
        return;
    }
    
    m_fromSocket = fromSocket;
    m_toSocket = toSocket;
    
    // Cache node pointers for safe destruction
    Node* fromNode = fromSocket->getParentNode();
    Node* toNode = toSocket->getParentNode();
    m_fromNode = fromNode;
    m_toNode = toNode;
    
    // PERFORMANCE OPTIMIZATION: Register this edge with both connected nodes
    // This enables O(degree) edge updates instead of O(totalEdges)
    if (fromNode) fromNode->registerEdge(this);
    if (toNode) toNode->registerEdge(this);
    
    qDebug() << "Edge: Set resolved sockets directly (optimization)";
    updatePath();
}

void Edge::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
        qDebug() << "=== EDGE SELF-DELETION START ===";
        qDebug() << "Edge" << getId().toString(QUuid::WithoutBraces).left(8) << "handling its own delete key";
        
        // Proper Qt approach: Edge handles its own deletion
        // Get the scene to call proper deletion method
        Scene* scene = qobject_cast<Scene*>(this->scene());
        if (scene) {
            scene->deleteEdge(getId());
        } else {
            qWarning() << "Edge: No scene found for deletion";
        }
        return;
    }
    
    // Pass unhandled keys to parent
    QGraphicsItem::keyPressEvent(event);
}