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
    , m_fromNodeValid(false)
    , m_toNodeValid(false)
    , m_hovered(false)
    #ifdef QT_DEBUG
    , m_shapeCallCount(0)
    #endif
{
    Q_UNUSED(fromSocketId)  // Legacy parameter, not used in clean design
    Q_UNUSED(toSocketId)    // Legacy parameter, not used in clean design
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setAcceptHoverEvents(true);  // Enable hover events for better interaction
    
    // Ensure edges are above nodes in z-order for easier selection
    setZValue(1);  // Nodes default to z=0, edges at z=1
    
    // Edge creation - minimal logging
}

Edge::~Edge()
{
    // PHASE 1.1: Remove socket cleanup entirely from destructor
    // Socket connections will be cleaned up by prepareForShutdown() before destruction
    
    // SAFETY: Use validity flags to prevent use-after-free
    if (m_fromNodeValid && m_fromNode) {
        m_fromNode->unregisterEdge(this);
    }
    if (m_toNodeValid && m_toNode) {
        m_toNode->unregisterEdge(this);
    }
    
    // PHASE 1: No socket cleanup in destructor - prevents use-after-free
}

void Edge::onNodeDestroying(const Node* node)
{
    // Built-in destruction safety - mark which node is being destroyed
    if (node == m_fromNode) {
        m_fromNodeValid = false;
        qDebug() << "SAFETY: Edge" << m_id.toString(QUuid::WithoutBraces).left(8) << "- fromNode destroying";
    }
    if (node == m_toNode) {
        m_toNodeValid = false;
        qDebug() << "SAFETY: Edge" << m_id.toString(QUuid::WithoutBraces).left(8) << "- toNode destroying";
    }
}

QRectF Edge::boundingRect() const
{
    return m_boundingRect;
}

void Edge::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)
    
    // Clean edge painting - strokes only, no fills
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setBrush(Qt::NoBrush);  // Explicitly no fill
    
    // Simple edge styling
    QPen edgePen;
    if (isSelected()) {
        edgePen = QPen(QColor(255, 100, 0), 3);  // Orange selection
    } else if (m_hovered) {
        edgePen = QPen(QColor(100, 150, 255), 2);  // Blue hover
    } else {
        edgePen = QPen(QColor(80, 80, 80), 1);  // Gray normal
    }
    
    // Draw edge path - stroke only
    painter->setPen(edgePen);
    painter->drawPath(m_path);
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
        bool isNowSelected = value.toBool();
        qDebug() << "Edge" << m_id.toString(QUuid::WithoutBraces).left(8) 
                 << (isNowSelected ? "SELECT" : "DESELECT");
        
        // Trigger visual update when selection changes
        update();
    }
    return QGraphicsItem::itemChange(change, value);
}

void Edge::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    QGraphicsItem::mousePressEvent(event);
}

void Edge::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    QGraphicsItem::mouseReleaseEvent(event);
}

void Edge::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    Q_UNUSED(event)
    m_hovered = true;
    update();  // Redraw to show hover effect
    QGraphicsItem::hoverEnterEvent(event);
}

void Edge::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    Q_UNUSED(event)
    m_hovered = false;
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
    
    QPointF start = m_fromSocket->scenePos();
    QPointF end = m_toSocket->scenePos();
    
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
    m_path.moveTo(start);
    
    // Create a curved connection with bounds checking
    qreal dx = end.x() - start.x();
    qreal controlOffset = qMin(qAbs(dx) * 0.5, 100.0); // Limit control point distance
    
    QPointF control1 = start + QPointF(controlOffset, 0);
    QPointF control2 = end - QPointF(controlOffset, 0);
    m_path.cubicTo(control1, control2, end);
    
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
    
    // Cache node pointers for safe destruction with validity flags
    Q_ASSERT(fromNode && toNode); // Future-risk check: ensure nodes are valid
    m_fromNode = fromNode;
    m_toNode = toNode;
    m_fromNodeValid = true;
    m_toNodeValid = true;
    
    // PATCH: Scene consistency check - prevent cross-scene connections
    if (fromNode->scene() != toNode->scene()) {
        qCritical() << "ERROR: Edge::resolveConnections - cross-scene edge detected"
                   << "fromNode scene:" << fromNode->scene() << "toNode scene:" << toNode->scene();
        return false;
    }
    
    // PERFORMANCE OPTIMIZATION: Register this edge with both connected nodes
    // This enables O(degree) edge updates instead of O(totalEdges)
    fromNode->registerEdge(this);
    toNode->registerEdge(this);
    
    // PATCH: Use new multi-edge socket system
    fromSocket->addConnectedEdge(this);
    toSocket->addConnectedEdge(this);
    
    qDebug() << "EDGE" << m_id.toString(QUuid::WithoutBraces).left(8) << ": Resolved" 
             << m_fromSocketIndex << "→" << m_toSocketIndex
             << "(from:" << fromSocket->getConnectedEdges().size() 
             << "to:" << toSocket->getConnectedEdges().size() << "connections)";
    
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
    
    // Cache node pointers for safe destruction with validity flags
    Node* fromNode = fromSocket->getParentNode();
    Node* toNode = toSocket->getParentNode();
    Q_ASSERT(fromNode && toNode); // Future-risk check: ensure nodes are valid
    m_fromNode = fromNode;
    m_toNode = toNode;
    m_fromNodeValid = true;
    m_toNodeValid = true;
    
    // PERFORMANCE OPTIMIZATION: Register this edge with both connected nodes
    // This enables O(degree) edge updates instead of O(totalEdges)
    if (fromNode) fromNode->registerEdge(this);
    if (toNode) toNode->registerEdge(this);
    
    qDebug() << "✓ Edge: Set resolved sockets directly (optimization)";
    updatePath();
}