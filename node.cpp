#include "node.h"
#include "socket.h"
#include "edge.h"
#include "scene.h"
#include "graphics_item_keys.h"
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QWidget>
#include <QDebug>
#include <QTimer>
#include <libxml/tree.h>

Node::Node(const QUuid& id, const QPointF& position)
    : m_id(id)
    , m_nodeType("DEFAULT")
    , m_width(80.0)
    , m_height(50.0)
    , m_changeCallback(nullptr)
    , m_observer(nullptr)
    , m_lastPos(position)
{
    setPos(position);
    setFlag(QGraphicsItem::ItemIsMovable, true);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);

    // Metadata annotations for cast-free identification
    setData(Gik::KindKey, Gik::Kind_Node);
    setData(Gik::UuidKey, m_id.toString(QUuid::WithoutBraces));

    // Node creation logging removed - working correctly
}

Node::~Node()
{
    // SAFETY: Invalidate all connected edges before destruction
    // Copy the set to avoid modification during iteration
    QSet<Edge*> edgesCopy = m_incidentEdges;
    for (Edge* edge : edgesCopy) {
        edge->invalidateNode(this);
    }
    
    // Node destruction logging removed - working correctly
}

QRectF Node::boundingRect() const
{
    return QRectF(0, 0, m_width, m_height);
}

void Node::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)
    
    QRectF rect = boundingRect();
    
    // Draw node body with rounded corners and gradient
    painter->setRenderHint(QPainter::Antialiasing);
    
    // Enhanced selection highlight using Qt's selection system
    if (isSelected()) {
        // Draw selection glow effect
        painter->setPen(QPen(QColor(255, 69, 0), 6)); // Thick orange border
        painter->setBrush(QColor(255, 245, 205)); // Light orange background
        
        // Add outer glow
        QRectF glowRect = rect.adjusted(-3, -3, 3, 3);
        painter->setPen(QPen(QColor(255, 69, 0, 100), 8));
        painter->drawRoundedRect(glowRect, 10.0, 10.0);
        
        // Restore main border
        painter->setPen(QPen(QColor(255, 69, 0), 4));
    } else {
        painter->setPen(QPen(Qt::darkGray, 2));
        painter->setBrush(QColor(240, 240, 240)); // Light gray background
    }
    
    painter->drawRoundedRect(rect, 8.0, 8.0);
    
    // Draw node type with improved typography
    painter->setPen(Qt::black);
    
    // Performance optimization: static font (created once, not every frame)
    static const QFont nodeFont("Arial", 8, QFont::Bold);
    painter->setFont(nodeFont);
    
    // Draw node type instead of UUID
    QString displayText = m_nodeType;
    painter->drawText(rect, Qt::AlignCenter, displayText);
    
    // Draw subtle node ID below type (smaller)
    if (rect.height() > 35) {
        // Performance optimization: cache display string (created once, not every frame)
        if (m_cachedDisplayId.isEmpty()) {
            m_cachedDisplayId = m_id.toString(QUuid::WithoutBraces).left(6);
        }
        
        static const QFont idFont("Arial", 6);
        painter->setFont(idFont);
        painter->setPen(QColor(120, 120, 120));
        
        QRectF idRect = rect.adjusted(0, rect.height() * 0.6, 0, 0);
        painter->drawText(idRect, Qt::AlignCenter, m_cachedDisplayId);
    }
}

QVariant Node::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == ItemSelectedHasChanged) {
        // Selection tracking logging - what has been selected
        bool isNowSelected = value.toBool();
        qDebug() << "Node" << m_id.toString(QUuid::WithoutBraces).left(8) 
                 << (isNowSelected ? "SELECT" : "DESELECT") << m_nodeType;
        
        // Trigger visual update when selection changes
        update();
    } else if (change == ItemPositionHasChanged) {
        // Only update edges when position actually changes significantly
        QPointF currentPos = value.toPointF();
        if ((currentPos - m_lastPos).manhattanLength() > 5.0) {
            QPointF oldPos = m_lastPos;
            m_lastPos = currentPos;
            
            // Re-enabled with safer edge updates
            updateConnectedEdges();
            
            // Notify observers of node movement via scene
            if (Scene* typedScene = qobject_cast<Scene*>(scene())) {
                typedScene->notifyNodeMoved(m_id, oldPos, currentPos);
            }
        }
        
        if (m_changeCallback) {
            m_changeCallback(this);
        }
    }
    return QGraphicsItem::itemChange(change, value);
}

void Node::setNodeSize(qreal width, qreal height)
{
    prepareGeometryChange();
    m_width = width;
    m_height = height;
    update();
}

// Removed manual setSelected - using Qt's selection system

void Node::setNodeType(const QString& type)
{
    m_nodeType = type;
    createStaticSockets();
    update();
    
    qDebug() << "Node" << m_id.toString(QUuid::WithoutBraces).left(8) << "type:" << type;
}

void Node::createStaticSockets()
{
    // CRITICAL: Delete edges connected to this node BEFORE deleting sockets
    Scene* typedScene = qobject_cast<Scene*>(scene());
    if (typedScene) {
        QList<QUuid> edgesToDelete;
        for (auto it = typedScene->getEdges().begin(); it != typedScene->getEdges().end(); ++it) {
            Edge* edge = it.value();
            if (edge->isConnectedToNode(m_id)) {
                edgesToDelete.append(it.key());
            }
        }
        // Delete edges that reference old sockets
        for (const QUuid& edgeId : edgesToDelete) {
            typedScene->removeEdgeInternal(edgeId);
        }
        qDebug() << "Node::createStaticSockets - removed" << edgesToDelete.size() 
                 << "edges before socket recreation for node" << m_id.toString(QUuid::WithoutBraces).left(8);
    }
    
    // Remove existing sockets (Qt will auto-delete child items)
    for (QGraphicsItem* child : childItems()) {
        if (qgraphicsitem_cast<Socket*>(child)) {
            delete child;  // Qt removes from parent automatically
        }
    }
    m_sockets.clear();  // Clear cache to prevent dangling pointers
    
    // This method now does nothing - sockets are created during XML read()
    // based on XML attributes like inputs="2" outputs="3"
}

void Node::createSocketsFromXml(int inputCount, int outputCount)
{
    // CRITICAL: Delete edges connected to this node BEFORE deleting sockets
    // Otherwise edges keep stale Socket* pointers and crash on updatePath()
    Scene* typedScene = qobject_cast<Scene*>(scene());
    if (typedScene) {
        QList<QUuid> edgesToDelete;
        for (auto it = typedScene->getEdges().begin(); it != typedScene->getEdges().end(); ++it) {
            Edge* edge = it.value();
            if (edge->isConnectedToNode(m_id)) {
                edgesToDelete.append(it.key());
            }
        }
        // Delete edges that reference old sockets
        for (const QUuid& edgeId : edgesToDelete) {
            typedScene->removeEdgeInternal(edgeId);
        }
        qDebug() << "Node::createSocketsFromXml - removed" << edgesToDelete.size() 
                 << "edges before socket recreation for node" << m_id.toString(QUuid::WithoutBraces).left(8);
    }
    
    // Clear existing sockets - both graphics items AND cache
    for (QGraphicsItem* child : childItems()) {
        if (qgraphicsitem_cast<Socket*>(child)) {
            delete child;  // Qt removes from parent automatically
        }
    }
    m_sockets.clear();  // Clear cache to prevent dangling pointers
    
    // Calculate dynamic node size based on socket count
    calculateNodeSize(inputCount, outputCount);
    
    // Phase 1: Create all socket objects (no positioning yet)
    int socketIndex = 0;
    
    // Create input sockets (indexes 0, 1, 2, ...)
    for (int i = 0; i < inputCount; ++i) {
        Socket* inputSocket = new Socket(Socket::Input, this, socketIndex++);
        // Register socket with scene if node has observer (GraphFactory)
        if (hasObserver()) {
            Scene* scene = qobject_cast<Scene*>(this->scene());
            if (scene) {
                scene->addSocket(inputSocket);
            }
        }
    }

    // Create output sockets (continuing index sequence)
    for (int i = 0; i < outputCount; ++i) {
        Socket* outputSocket = new Socket(Socket::Output, this, socketIndex++);
        // Register socket with scene if node has observer (GraphFactory)
        if (hasObserver()) {
            Scene* scene = qobject_cast<Scene*>(this->scene());
            if (scene) {
                scene->addSocket(outputSocket);
            }
        }
    }
    
    // Phase 2: Position all sockets with complete information
    positionAllSockets(inputCount, outputCount);
    
    qDebug() << "Node" << m_id.toString(QUuid::WithoutBraces).left(8) << inputCount << "IN" << outputCount << "OUT";
}

void Node::positionAllSockets(int totalInputs, int totalOutputs)
{
    // Parse-then-position architecture: Position all sockets with complete information
    // Uses K/O formula: max((2*K + 1), (2*O + 1)) × socketSize with BALANCED CENTERING
    
    // ✅ Get actual socket size from existing socket - no fallbacks or magic numbers
    if (m_sockets.isEmpty()) {
        qWarning() << "Node::positionAllSockets() called with no sockets available";
        return;
    }
    
    QSizeF actualSocketSize = m_sockets[0]->getSocketSize();
    qreal socketSize = qMax(actualSocketSize.width(), actualSocketSize.height());
    
    const qreal socketOffset = 4.0; // Distance from node edge
    const qreal socketSpacing = 32.0; // Match calculateNodeSize spacing
    
    QRectF nodeRect = boundingRect();
    int inputIndex = 0;
    int outputIndex = 0;
    
    // VIRTUAL BOUNDING BOX APPROACH: Create centered virtual boxes for socket placement
    qreal nodeHeight = nodeRect.height();
    qreal nodeCenterY = nodeHeight * 0.6; // Visual center, moved down from geometric center
    
    // Create virtual bounding boxes for each socket group using (2*n + 1) * socketSize formula
    qreal inputBoxHeight = (totalInputs > 0) ? (2 * totalInputs + 1) * socketSize : 0;
    qreal outputBoxHeight = (totalOutputs > 0) ? (2 * totalOutputs + 1) * socketSize : 0;
    
    // Align the horizontal center lines: socket box center = node center
    qreal inputBoxCenterY = nodeCenterY;
    qreal outputBoxCenterY = nodeCenterY;
    qreal inputBoxStartY = inputBoxCenterY - (inputBoxHeight / 2.0);
    qreal outputBoxStartY = outputBoxCenterY - (outputBoxHeight / 2.0);
    
    // Calculate socket positions using balanced centering
    for (Socket* socket : m_sockets) {
        if (!socket) continue;
        
        if (socket->getRole() == Socket::Input) {
            // Place socket within the centered virtual input bounding box
            qreal x = -socketOffset;  // Left side of node
            qreal y = inputBoxStartY + socketSize * (2 * inputIndex + 1); // Position at center of each slot
            
            socket->setDirectPosition(x, y);
            inputIndex++;
            
            qDebug() << "VIRTUAL BOX INPUT socket" << inputIndex-1 << "positioned at" << QPointF(x, y);
        } else {
            // Place socket within the centered virtual output bounding box  
            qreal x = nodeRect.width() + socketOffset;  // Right side of node
            qreal y = outputBoxStartY + socketSize * (2 * outputIndex + 1); // Position at center of each slot
            
            socket->setDirectPosition(x, y);
            outputIndex++;
            
            qDebug() << "VIRTUAL BOX OUTPUT socket" << outputIndex-1 << "positioned at" << QPointF(x, y);
        }
    }
    
    // Verify balanced positioning
    qreal requiredInputHeight = (totalInputs > 0) ? (2 * totalInputs + 1) * socketSize : 0;
    qreal requiredOutputHeight = (totalOutputs > 0) ? (2 * totalOutputs + 1) * socketSize : 0;
    qreal requiredHeight = qMax(requiredInputHeight, requiredOutputHeight);
    
    qDebug() << "VIRTUAL BOX POSITIONING: K=" << totalInputs << "inputs (box start:" << inputBoxStartY 
             << "), O=" << totalOutputs << "outputs (box start:" << outputBoxStartY << ")"
             << "| Node height:" << nodeHeight << "| Required:" << requiredHeight
             << "for node" << m_id.toString(QUuid::WithoutBraces).left(8);
}

Socket* Node::getSocketByIndex(int index) const
{
    // O(1) socket lookup using cache - with safety validation
    if (index >= 0 && index < m_sockets.size()) {
        Socket* socket = m_sockets[index];
        if (!socket) {
            qCritical() << "Node::getSocketByIndex() - null socket at index" << index 
                       << "in node" << m_id.toString(QUuid::WithoutBraces).left(8);
            return nullptr;
        }
        // Belt-and-suspenders: check if socket has been deleted
        if (socket->scene() == nullptr) {
            qWarning() << "Node::getSocketByIndex() - socket at index" << index 
                      << "has been deleted (not in scene) in node" << m_id.toString(QUuid::WithoutBraces).left(8);
            return nullptr;
        }
        return socket;
    }
    qWarning() << "Node::getSocketByIndex() - index" << index << "out of bounds [0," 
               << m_sockets.size() << ") in node" << m_id.toString(QUuid::WithoutBraces).left(8);
    return nullptr;
}

int Node::getSocketCount() const
{
    // O(1) socket count using cache
    return m_sockets.size();
}

void Node::registerSocket(Socket* socket, int index)
{
    if (!socket) {
        qCritical() << "Node::registerSocket() - null socket passed for index" << index;
        return;
    }
    
    // Enforce contiguous indices - prevent sparse arrays with nullptr gaps
    if (index != m_sockets.size()) {
        qCritical() << "Node::registerSocket() - index" << index 
                   << "is not contiguous. Expected index" << m_sockets.size() 
                   << "for node" << m_id.toString(QUuid::WithoutBraces).left(8);
        Q_ASSERT(index == m_sockets.size());
        return;
    }
    
    // Append socket to maintain contiguous array
    m_sockets.append(socket);
    
    qDebug() << "Node" << m_id.toString(QUuid::WithoutBraces).left(8) << "socket" << index 
             << (socket->getRole() == Socket::Input ? "IN" : "OUT");
}

void Node::setChangeCallback(void (*callback)(Node*))
{
    m_changeCallback = callback;
}

// ============================================================================
// Edge Management - O(degree) Performance Optimization
// ============================================================================

void Node::registerEdge(Edge* edge)
{
    if (!edge) {
        qWarning() << "Node::registerEdge() - null edge pointer for node" << m_id.toString(QUuid::WithoutBraces).left(8);
        return;
    }
    
    // Debug assertion in development builds
    #ifdef QT_DEBUG
    if (m_incidentEdges.contains(edge)) {
        qWarning() << "Node::registerEdge() - edge already registered with node" << m_id.toString(QUuid::WithoutBraces).left(8);
        return;
    }
    #endif
    
    m_incidentEdges.insert(edge);
    // Edge registration logging removed - working correctly
}

void Node::unregisterEdge(Edge* edge)
{
    if (!edge) {
        qWarning() << "Node::unregisterEdge() - null edge pointer for node" << m_id.toString(QUuid::WithoutBraces).left(8);
        return;
    }
    
    // Debug assertion in development builds
    #ifdef QT_DEBUG
    if (!m_incidentEdges.contains(edge)) {
        qWarning() << "Node::unregisterEdge() - edge not found in node" << m_id.toString(QUuid::WithoutBraces).left(8);
        return;
    }
    #endif
    
    m_incidentEdges.remove(edge);
    // Edge unregistration logging removed - working correctly
}

void Node::updateConnectedEdges()
{
    // NEW: O(degree) performance - only update edges actually connected to this node
    for (Edge* edge : m_incidentEdges) {
        edge->updatePath();
    }
    
    // Edge update logging removed - working correctly
}


// Sockets now paint themselves as QGraphicsItems - no need for this method

xmlNodePtr Node::write(xmlDocPtr doc, xmlNodePtr repr) const
{
    xmlNodePtr node = xmlNewNode(nullptr, BAD_CAST "node");
    
    // Core attributes
    xmlSetProp(node, BAD_CAST "id", BAD_CAST m_id.toString(QUuid::WithoutBraces).toUtf8().constData());
    xmlSetProp(node, BAD_CAST "x", BAD_CAST QString::number(pos().x()).toUtf8().constData());
    xmlSetProp(node, BAD_CAST "y", BAD_CAST QString::number(pos().y()).toUtf8().constData());
    xmlSetProp(node, BAD_CAST "type", BAD_CAST m_nodeType.toUtf8().constData());
    
    // Count sockets by role
    int inputCount = 0, outputCount = 0;
    for (QGraphicsItem* child : childItems()) {
        if (Socket* socket = qgraphicsitem_cast<Socket*>(child)) {
            if (socket->getRole() == Socket::Input) inputCount++;
            else if (socket->getRole() == Socket::Output) outputCount++;
        }
    }
    
    // Save socket configuration as XML attributes
    xmlSetProp(node, BAD_CAST "inputs", BAD_CAST QString::number(inputCount).toUtf8().constData());
    xmlSetProp(node, BAD_CAST "outputs", BAD_CAST QString::number(outputCount).toUtf8().constData());
    
    if (repr) {
        xmlAddChild(repr, node);
    } else {
        xmlDocSetRootElement(doc, node);
    }
    
    return node;
}

void Node::read(xmlNodePtr node)
{
    if (!node) return;
    
    // Read UUID
    xmlChar* idStr = xmlGetProp(node, BAD_CAST "id");
    if (idStr) {
        m_id = QUuid(QString::fromUtf8((char*)idStr));
        // ✅ CRITICAL: Update metadata when UUID changes from XML
        setData(Gik::UuidKey, m_id.toString(QUuid::WithoutBraces));
        xmlFree(idStr);
    }
    
    // Read node type 
    xmlChar* typeStr = xmlGetProp(node, BAD_CAST "type");
    if (typeStr) {
        m_nodeType = QString::fromUtf8((char*)typeStr);
        xmlFree(typeStr);
    }
    
    // Read socket configuration from XML attributes
    int inputCount = 1;  // Default
    int outputCount = 1; // Default
    
    xmlChar* inputsStr = xmlGetProp(node, BAD_CAST "inputs");
    if (inputsStr) {
        inputCount = QString::fromUtf8((char*)inputsStr).toInt();
        xmlFree(inputsStr);
    }
    
    xmlChar* outputsStr = xmlGetProp(node, BAD_CAST "outputs");
    if (outputsStr) {
        outputCount = QString::fromUtf8((char*)outputsStr).toInt();
        xmlFree(outputsStr);
    }
    
    // Create sockets based on XML configuration
    createSocketsFromXml(inputCount, outputCount);
    
    // Read position
    xmlChar* xStr = xmlGetProp(node, BAD_CAST "x");
    xmlChar* yStr = xmlGetProp(node, BAD_CAST "y");
    if (xStr && yStr) {
        qreal x = QString::fromUtf8((char*)xStr).toDouble();
        qreal y = QString::fromUtf8((char*)yStr).toDouble();
        setPos(x, y);
        xmlFree(xStr);
        xmlFree(yStr);
    }
    
    update();
}

void Node::calculateNodeSize(int inputCount, int outputCount)
{
    // Calculate required height based on socket count
    int maxSockets = qMax(inputCount, outputCount);
    
    // Constants matching socket configuration
    const qreal socketSpacing = 32.0;  // Must match socket.cpp
    const qreal minNodeHeight = 50.0;  // Minimum height for node text
    const qreal topPadding = 14.0;     // Top padding (socket width)
    const qreal bottomPadding = 14.0;  // Bottom padding (socket width)
    const qreal minNodeWidth = 100.0;  // Minimum width for node text
    
    // Calculate height based on socket count
    if (maxSockets > 0) {
        // Height = top padding + (sockets * spacing) + bottom padding
        qreal requiredHeight = topPadding + (maxSockets - 1) * socketSpacing + 14.0 + bottomPadding;
        m_height = qMax(minNodeHeight, requiredHeight);
    } else {
        m_height = minNodeHeight;
    }
    
    // Calculate width based on node type and content
    QString displayText = m_nodeType + " " + m_id.toString(QUuid::WithoutBraces).left(8);
    QFont font("Arial", 10);
    QFontMetrics metrics(font);
    qreal textWidth = metrics.horizontalAdvance(displayText) + 20; // Add padding
    
    m_width = qMax(minNodeWidth, textWidth);
    
    // Ensure node is wide enough to accommodate sockets with proper spacing
    const qreal socketOffset = 8.0; // Space for socket offset from edges
    m_width = qMax(m_width, socketOffset * 2 + 20); // Minimum width for sockets
    
    // Notify Qt graphics system of geometry change
    prepareGeometryChange();
    
    qDebug() << "Node" << m_id.toString(QUuid::WithoutBraces).left(8) 
             << "resized to" << m_width << "x" << m_height 
             << "for" << inputCount << "inputs," << outputCount << "outputs";
}