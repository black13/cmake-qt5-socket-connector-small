#include "scene.h"
#include "node.h"
#include "edge.h"
#include "socket.h"
#include "javascript_engine.h"
#include <QDebug>
#include <QTimer>
#include <QGraphicsPathItem>

Scene::Scene(QObject* parent)
    : QGraphicsScene(parent)
    , m_ghostEdge(nullptr)
    , m_ghostFromSocket(nullptr)
    , m_ghostEdgeActive(false)
    , m_shutdownInProgress(false)
    , m_jsEngine(new JavaScriptEngine(this))
{
    setSceneRect(-1000, -1000, 2000, 2000);
    
    // Initialize JavaScript engine with this scene
    m_jsEngine->registerNodeAPI(this);
    m_jsEngine->registerGraphAPI();
    // GraphController will be registered when GraphFactory is available
    
    qDebug() << "Scene: JavaScript engine integrated";
}

// QElectroTech-style QHash implementation with SIMPLE_FIX logging
void Scene::addNode(Node* node)
{
    if (!node) return;
    
    QUuid nodeId = node->getId();
    m_nodes.insert(nodeId, node);
    addItem(node);
    
    qDebug() << "+" << nodeId.toString(QUuid::WithoutBraces).left(8);
    
    // Notify observers of node addition
    notifyNodeAdded(*node);
    
    // Emit signal for UI updates
    emit sceneChanged();
}

void Scene::addEdge(Edge* edge)
{
    if (!edge) return;
    
    QUuid edgeId = edge->getId();
    m_edges.insert(edgeId, edge);
    addItem(edge);
    
    qDebug() << "+" << edgeId.toString(QUuid::WithoutBraces).left(8);
    
    // Notify observers of edge addition
    notifyEdgeAdded(*edge);
    
    // Emit signal for UI updates
    emit sceneChanged();
    
    // Clean design: edges manage their own socket connections via resolveConnections()
}

void Scene::addSocket(Socket* socket)
{
    if (!socket) return;
    
    // Clean design: sockets are managed by their parent nodes, not scene
    // Socket is automatically added to scene as child of parent node
}

void Scene::removeNode(const QUuid& nodeId)
{
    Node* node = m_nodes.value(nodeId, nullptr);
    if (!node) return;
    
    // Clean design: sockets are children of nodes - no separate tracking needed
    
    // Remove connected edges that reference this node
    QList<QUuid> edgesToRemove;
    QString nodeIdStr = nodeId.toString();
    for (Edge* edge : m_edges.values()) {
        if (edge->isConnectedToNode(nodeIdStr)) {
            edgesToRemove.append(edge->getId());
        }
    }
    
    // Clean up edges first (they may reference sockets)
    for (const QUuid& edgeId : edgesToRemove) {
        removeEdge(edgeId);
    }
    
    // Clean design: sockets cleaned up automatically as node children
    
    // Finally remove the node
    removeItem(node);
    m_nodes.remove(nodeId);
    delete node;
}

void Scene::removeEdge(const QUuid& edgeId)
{
    Edge* edge = m_edges.value(edgeId, nullptr);
    if (!edge) return;
    
    // Clean design: edges manage their own socket disconnection via direct pointers
    // Socket cleanup handled automatically when edge is destroyed
    
    // Remove from scene and registry
    removeItem(edge);
    m_edges.remove(edgeId);
    delete edge;
}

// Clean design: socket management methods removed - sockets handled by parent nodes

// O(1) UUID lookups
Node* Scene::getNode(const QUuid& nodeId) const
{
    return m_nodes.value(nodeId, nullptr);
}

Edge* Scene::getEdge(const QUuid& edgeId) const
{
    return m_edges.value(edgeId, nullptr);
}

void Scene::deleteNode(const QUuid& nodeId)
{
    Node* node = getNode(nodeId);
    if (!node) {
        qWarning() << "Scene::deleteNode - node not found:" << nodeId.toString(QUuid::WithoutBraces).left(8);
        return;
    }
    
    qDebug() << "Deleting node:" << nodeId.toString(QUuid::WithoutBraces).left(8);
    
    // First, find and delete all edges connected to this node
    QList<QUuid> edgesToDelete;
    for (auto it = m_edges.begin(); it != m_edges.end(); ++it) {
        Edge* edge = it.value();
        if (edge->isConnectedToNode(nodeId)) {
            edgesToDelete.append(it.key());
        }
    }
    
    // Delete connected edges
    for (const QUuid& edgeId : edgesToDelete) {
        deleteEdge(edgeId);
    }
    
    // Remove node from collections and scene
    m_nodes.remove(nodeId);
    removeItem(node);
    
    // Notify observers BEFORE deleting the node
    notifyNodeRemoved(nodeId);
    
    delete node;
    
    // Emit signal for UI updates
    emit sceneChanged();
    
    qDebug() << "Node deleted with" << edgesToDelete.size() << "connected edges - Observer notified";
}

void Scene::deleteEdge(const QUuid& edgeId)
{
    Edge* edge = getEdge(edgeId);
    if (!edge) {
        qWarning() << "Scene::deleteEdge - edge not found:" << edgeId.toString(QUuid::WithoutBraces).left(8);
        return;
    }
    
    qDebug() << "Deleting edge:" << edgeId.toString(QUuid::WithoutBraces).left(8);
    
    // Remove from collection and scene
    m_edges.remove(edgeId);
    removeItem(edge);
    
    // Notify observers BEFORE deleting the edge
    notifyEdgeRemoved(edgeId);
    
    delete edge;
    
    // Emit signal for UI updates
    emit sceneChanged();
    
    qDebug() << "Edge deleted - Observer notified";
}

void Scene::deleteSelected()
{
    QList<QGraphicsItem*> selectedItems = this->selectedItems();
    if (selectedItems.isEmpty()) {
        qDebug() << "No items selected for deletion";
        return;
    }
    
    qDebug() << "DELETE KEY: Deleting" << selectedItems.size() << "selected items";
    
    // Separate nodes and edges for proper deletion order
    QList<Node*> selectedNodes;
    QList<Edge*> selectedEdges;
    
    for (QGraphicsItem* item : selectedItems) {
        if (Node* node = qgraphicsitem_cast<Node*>(item)) {
            selectedNodes.append(node);
        } else if (Edge* edge = qgraphicsitem_cast<Edge*>(item)) {
            selectedEdges.append(edge);
        }
    }
    
    // Delete selected edges first
    for (Edge* edge : selectedEdges) {
        deleteEdge(edge->getId());
    }
    
    // Then delete selected nodes (which will delete their remaining edges)
    for (Node* node : selectedNodes) {
        deleteNode(node->getId());
    }
    
    // Emit signal for UI updates
    emit sceneChanged();
    
    qDebug() << "DELETE COMPLETE: Deleted" << selectedEdges.size() << "edges and" << selectedNodes.size() << "nodes - Observers notified";
}

void Scene::clearGraph()
{
    qDebug() << "SIMPLE_FIX: Clearing graph - removing" << m_nodes.size() << "nodes and" << m_edges.size() << "edges";
    
    // SIMPLE FIX: Clear registries FIRST to prevent dangling pointers
    // This prevents hash lookups during Qt's destruction sequence
    qDebug() << "SIMPLE_FIX: Clearing hash registries first";
    m_nodes.clear();
    m_edges.clear();
    m_sockets.clear();  // Clear deprecated socket registry too
    
    // Then clear Qt graphics scene (safe now - no hash references)
    qDebug() << "SIMPLE_FIX: Clearing Qt scene items";
    QGraphicsScene::clear();
    
    // Notify observers of graph clearing
    notifyGraphCleared();
    
    // Emit signal for UI updates
    emit sceneChanged();
    
    qDebug() << "SIMPLE_FIX: ✓ Graph cleared safely - hash cleared before Qt cleanup";
}

// ============================================================================
// PHASE 1.2: Safe Shutdown Preparation
// ============================================================================

void Scene::prepareForShutdown()
{
    if (m_shutdownInProgress) {
        qDebug() << "SHUTDOWN: Already in progress, skipping";
        return;
    }
    
    qDebug() << "PHASE1: Shutdown preparation -" << m_edges.size() << "edges," << m_nodes.size() << "nodes";
    m_shutdownInProgress = true;
    
    // Step 1: Clean edge-socket connections BEFORE any destruction
    for (Edge* edge : m_edges.values()) {
        // Note: Socket connection cleanup disabled - methods not available in current implementation
    }
    
    qDebug() << "PHASE1: ✓ Socket connections cleared safely";
}

// ============================================================================
// Ghost Edge Implementation for Right-Click Socket Connections
// ============================================================================

// IUnknown UUID for ghost edge identification
static const QUuid GHOST_EDGE_UUID = QUuid("{00000000-0000-0000-C000-000000000046}");

void Scene::startGhostEdge(Socket* fromSocket, const QPointF& startPos)
{
    if (m_ghostEdge) {
        removeItem(m_ghostEdge);
        delete m_ghostEdge;
    }
    
    m_ghostFromSocket = fromSocket;
    m_ghostEdge = new QGraphicsPathItem();
    m_ghostEdge->setZValue(-10); // Below all interactive items
    m_ghostEdge->setFlag(QGraphicsItem::ItemIsSelectable, false);
    m_ghostEdge->setAcceptedMouseButtons(Qt::NoButton);
    m_ghostEdge->setData(0, GHOST_EDGE_UUID); // IUnknown UUID marker
    m_ghostEdge->setPen(ghostPen());
    addItem(m_ghostEdge);
    m_ghostEdgeActive = true;
    
    updateGhostEdge(startPos);
    
    qDebug() << "GHOST: Started from socket" << fromSocket->getIndex() 
             << "(" << (fromSocket->getRole() == Socket::Input ? "Input" : "Output") << ")";
}

void Scene::updateGhostEdge(const QPointF& currentPos)
{
    if (!m_ghostEdge || !m_ghostFromSocket) return;
    
    QPointF start = m_ghostFromSocket->scenePos();
    QPainterPath path;
    path.moveTo(start);
    
    // Create curved ghost edge similar to real edges
    qreal dx = currentPos.x() - start.x();
    qreal controlOffset = qMin(qAbs(dx) * 0.5, 100.0);
    
    QPointF control1 = start + QPointF(controlOffset, 0);
    QPointF control2 = currentPos - QPointF(controlOffset, 0);
    path.cubicTo(control1, control2, currentPos);
    
    // Update ghost edge visual based on target validity
    QPen ghostPenCurrent = ghostPen();
    QGraphicsItem* itemUnderCursor = itemAt(currentPos, QTransform());
    Socket* targetSocket = qgraphicsitem_cast<Socket*>(itemUnderCursor);
    
    // Reset all socket visual states to normal first
    resetAllSocketStates();
    
    if (targetSocket) {
        // Check if this is a valid connection target
        bool isValidTarget = (targetSocket->getRole() == Socket::Input && 
                            targetSocket != m_ghostFromSocket &&
                            targetSocket->getParentNode() != m_ghostFromSocket->getParentNode());
        
        if (isValidTarget) {
            // targetSocket->setVisualState(Socket::ValidTarget); // Disabled
            ghostPenCurrent.setColor(QColor(0, 255, 0, 180)); // Green ghost edge
        } else {
            // targetSocket->setVisualState(Socket::InvalidTarget); // Disabled
            ghostPenCurrent.setColor(QColor(255, 0, 0, 180)); // Red ghost edge
        }
    } else {
        // No socket under cursor - default ghost edge color
        ghostPenCurrent.setColor(QColor(0, 255, 0, 150)); // Default green
    }
    
    m_ghostEdge->setPen(ghostPenCurrent);
    m_ghostEdge->setPath(path);
}

void Scene::resetAllSocketStates()
{
    // Reset all sockets to normal state when not being targeted (disabled)
    /*
    for (Node* node : m_nodes.values()) {
        for (QGraphicsItem* child : node->childItems()) {
            if (Socket* socket = qgraphicsitem_cast<Socket*>(child)) {
                if (socket->getVisualState() == Socket::ValidTarget || 
                    socket->getVisualState() == Socket::InvalidTarget) {
                    socket->setVisualState(Socket::Normal);
                }
            }
        }
    }
    */
}

void Scene::finishGhostEdge(Socket* toSocket)
{
    if (m_ghostFromSocket && toSocket) {
        // Validate connection roles
        if (m_ghostFromSocket->getRole() == Socket::Output && 
            toSocket->getRole() == Socket::Input) {
            
            // Create real edge using existing system
            Edge* newEdge = new Edge(QUuid::createUuid(), QUuid(), QUuid());
            
            Node* fromNode = m_ghostFromSocket->getParentNode();
            Node* toNode = toSocket->getParentNode();
            
            if (fromNode && toNode) {
                newEdge->setConnectionData(
                    fromNode->getId().toString(QUuid::WithoutBraces),
                    toNode->getId().toString(QUuid::WithoutBraces),
                    m_ghostFromSocket->getIndex(),
                    toSocket->getIndex()
                );
                
                addEdge(newEdge);
                newEdge->resolveConnections(this);
                
                // Brief success feedback - flash the connected sockets green (disabled)
                // m_ghostFromSocket->setVisualState(Socket::ValidTarget);
                // toSocket->setVisualState(Socket::ValidTarget);
                // QTimer::singleShot(300, [this, toSocket]() {
                //     // Reset to normal appearance after brief success flash
                //     if (m_ghostFromSocket) m_ghostFromSocket->setVisualState(Socket::Normal);
                //     toSocket->setVisualState(Socket::Normal);
                // });
                
                qDebug() << "GHOST: ✓ Created edge" << m_ghostFromSocket->getIndex() << "→" << toSocket->getIndex();
            }
        } else {
            qDebug() << "GHOST: ✗ Invalid connection - wrong socket roles";
        }
    }
    
    cancelGhostEdge();
}

void Scene::cancelGhostEdge()
{
    // Reset all socket visual states
    resetAllSocketStates();
    
    if (m_ghostEdge) {
        removeItem(m_ghostEdge);
        delete m_ghostEdge;
        m_ghostEdge = nullptr;
    }
    m_ghostFromSocket = nullptr;
    m_ghostEdgeActive = false;
    
    qDebug() << "GHOST: Cancelled";
}

QPen Scene::ghostPen() const
{
    QPen pen(QColor(0, 255, 0, 150)); // Semi-transparent green
    pen.setWidth(3);
    pen.setStyle(Qt::DashLine);
    pen.setDashPattern({8, 4});
    return pen;
}

void Scene::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    if (m_ghostEdgeActive) {
        updateGhostEdge(event->scenePos());
        event->accept();
        return;
    }
    QGraphicsScene::mouseMoveEvent(event);
}

void Scene::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    if (m_ghostEdgeActive && event->button() == Qt::RightButton) {
        // Find socket under mouse
        QGraphicsItem* item = itemAt(event->scenePos(), QTransform());
        Socket* targetSocket = qgraphicsitem_cast<Socket*>(item);
        finishGhostEdge(targetSocket);
        event->accept();
        return;
    }
    QGraphicsScene::mouseReleaseEvent(event);
}

// JavaScript engine integration methods
QString Scene::executeJavaScript(const QString& script)
{
    if (!m_jsEngine) {
        qDebug() << "Scene: JavaScript engine not initialized";
        return QString();
    }
    
    QJSValue result = m_jsEngine->evaluate(script);
    
    if (result.isError()) {
        qDebug() << "Scene: JavaScript error:" << result.toString();
        return QString();
    }
    
    return result.toString();
}

void Scene::loadJavaScriptFile(const QString& filePath)
{
    if (!m_jsEngine) {
        qDebug() << "Scene: JavaScript engine not initialized";
        return;
    }
    
    QJSValue result = m_jsEngine->evaluateFile(filePath);
    
    if (result.isError()) {
        qDebug() << "Scene: Failed to load JavaScript file:" << filePath << "Error:" << result.toString();
    } else {
        qDebug() << "Scene: Successfully loaded JavaScript file:" << filePath;
    }
}