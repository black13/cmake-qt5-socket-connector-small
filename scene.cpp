#include "scene.h"
#include "node.h"
#include "edge.h"
#include "socket.h"
#include "graph_factory.h"
// JavaScript engine include removed
#include "ghost_edge.h"

// Static flag for clearing state
bool Scene::s_clearingGraph = false;
#include <QDebug>
#include <QTimer>
#include <QGraphicsPathItem>

Scene::Scene(QObject* parent)
    : QGraphicsScene(parent)
    , m_ghostEdge(nullptr)
    , m_ghostFromSocket(nullptr)
    , m_ghostEdgeActive(false)
    , m_shutdownInProgress(false)
    , m_graphFactory(nullptr)
    // JavaScript engine initialization removed
{
    setSceneRect(-1000, -1000, 2000, 2000);
    
    // JavaScript engine initialization removed - focusing on core C++ functionality
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

// DELETED: Scene::deleteSelected() method removed
// Reason: Violates Qt architecture - items should handle their own delete events
// New approach: Each QGraphicsItem handles keyPressEvent directly

void Scene::clearGraph()
{
    ScopedClearing _guard(s_clearingGraph);
    
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
    
    qDebug() << "SIMPLE_FIX: Graph cleared safely - hash cleared before Qt cleanup";
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
    
    qDebug() << "PHASE1: Socket connections cleared safely";
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
    
    m_ghostEdge = new GhostEdge();
    m_ghostEdge->setData(0, GHOST_EDGE_UUID); // IUnknown UUID marker
    
    addItem(m_ghostEdge);
    m_ghostEdgeActive = true;
    
    // Set source socket to connecting state
    fromSocket->setConnectionState(Socket::Connecting);
    
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
            targetSocket->setConnectionState(Socket::Highlighted);
            ghostPenCurrent.setColor(QColor(0, 255, 0, 180)); // Green ghost edge
        } else {
            ghostPenCurrent.setColor(QColor(255, 0, 0, 180)); // Red ghost edge
        }
    } else {
        // No socket under cursor - default ghost edge color
        ghostPenCurrent.setColor(QColor(0, 255, 0, 150)); // Default green
    }
    
    m_ghostEdge->setPath(path);
}

void Scene::resetAllSocketStates()
{
    // Reset all sockets to normal state when not being targeted
    for (Node* node : m_nodes.values()) {
        for (QGraphicsItem* child : node->childItems()) {
            if (Socket* socket = qgraphicsitem_cast<Socket*>(child)) {
                if (socket != m_ghostFromSocket) {
                    socket->updateConnectionState(); // Reset to connected/disconnected
                }
            }
        }
    }
}

void Scene::finishGhostEdge(Socket* toSocket)
{
    if (!toSocket) {
        cancelGhostEdge();
        return;
    }

    // NEW: mirror the one-edge-per-socket rule here as user feedback
    if (m_ghostFromSocket->isConnected() || toSocket->isConnected()) {
        qWarning() << "Scene::finishGhostEdge: socket already connected; rejecting";
        cancelGhostEdge();
        return;
    }

    if (m_ghostFromSocket && toSocket) {
        // Validate connection roles
        if (m_ghostFromSocket->getRole() == Socket::Output && 
            toSocket->getRole() == Socket::Input) {
            
            if (m_graphFactory) {
                Q_ASSERT(m_ghostFromSocket && toSocket); // Ghost edge requires both sockets
                // Use factory for consistent edge creation
                Edge* newEdge = m_graphFactory->connectSockets(m_ghostFromSocket, toSocket);
                if (newEdge) {
                    qDebug() << "GHOST: Created edge via factory" << m_ghostFromSocket->getIndex() << "->" << toSocket->getIndex();
                } else {
                    qWarning() << "GHOST: Factory failed to create edge";
                }
            } else {
                qWarning() << "GHOST: No factory available - cannot create edge";
            }
        } else {
            qDebug() << "GHOST: Invalid connection - wrong socket roles";
        }
    }
    
    // Reset all socket states before canceling
    resetAllSocketStates();
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
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
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

// JavaScript engine methods removed - focusing on core C++ functionality
