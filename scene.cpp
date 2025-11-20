#include "scene.h"
#include "node.h"
#include "edge.h"
#include "socket.h"
#include "graph_factory.h"
#include "ghost_edge.h"
#include <QRandomGenerator>
#include <QElapsedTimer>
#include <QtMath>
#include <QKeyEvent>
#include <QLineF>
#include <cmath>

// Static flag for clearing state
bool Scene::s_clearingGraph = false;
#include <QDebug>
#include <QTimer>
#include <QGraphicsPathItem>
#include <QApplication>
#include <QThread>

Scene::Scene(QObject* parent)
    : QGraphicsScene(parent)
    , m_ghostEdge(nullptr)
    , m_ghostFromSocket(nullptr)
    , m_ghostEdgeActive(false)
    , m_ghostCurrentPos()
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
    if (!node) {
        return;
    }
    
    QUuid nodeId = node->getId();
    m_nodes.insert(nodeId, node);
    addItem(node);
    
    // Scene stats logging as suggested by ChatGPT analysis
    qDebug() << "Scene: nodes=" << m_nodes.size() << "edges=" << m_edges.size() 
             << "(added node" << node->getNodeType() << ")";
    
    // Notify observers of node addition
    notifyNodeAdded(*node);
    
    // Emit signal for UI updates
    emit sceneChanged();
}

void Scene::addEdge(Edge* edge)
{
    if (!edge) {
        return;
    }
    
    QUuid edgeId = edge->getId();
    m_edges.insert(edgeId, edge);
    addItem(edge);
    
    // Scene stats logging as suggested by ChatGPT analysis  
    qDebug() << "Scene: nodes=" << m_nodes.size() << "edges=" << m_edges.size()
             << "(added edge" << edgeId.toString(QUuid::WithoutBraces).left(8) << ")";
    
    // Notify observers of edge addition
    notifyEdgeAdded(*edge);
    
    // Emit signal for UI updates
    emit sceneChanged();
    
    // Clean design: edges manage their own socket connections via resolveConnections()
}

void Scene::removeNode(const QUuid& nodeId)
{
    Node* node = m_nodes.value(nodeId, nullptr);
    if (!node) {
        return;
    }
    
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
    if (!edge) {
        return;
    }
    
    edge->detachSockets();
    
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

QList<Node*> Scene::selectedNodes() const
{
    QList<Node*> result;
    for (Node* node : m_nodes) {
        if (node && node->isSelected()) {
            result.append(node);
        }
    }
    return result;
}

QList<Edge*> Scene::selectedEdges() const
{
    QList<Edge*> result;
    for (Edge* edge : m_edges) {
        if (edge && edge->isSelected()) {
            result.append(edge);
        }
    }
    return result;
}

Socket* Scene::socketAt(const QPointF& scenePos) const
{
    Socket* best = nullptr;
    qreal bestDist2 = std::numeric_limits<qreal>::max();

    // Iterate typed collections; no QGraphicsScene::itemAt or casts
    for (Node* node : m_nodes.values()) {
        if (!node) continue;
        const auto& sockets = node->getAllSockets();
        for (Socket* socket : sockets) {
            if (!socket) continue;
            // Hit test in socket's local coordinates
            QPointF local = socket->mapFromScene(scenePos);
            if (socket->contains(local)) {
                QPointF c = socket->scenePos();
                qreal dx = c.x() - scenePos.x();
                qreal dy = c.y() - scenePos.y();
                qreal d2 = dx*dx + dy*dy;
                if (d2 < bestDist2) {
                    bestDist2 = d2;
                    best = socket;
                }
            }
        }
    }
    return best;
}

void Scene::logSceneState(const QString& context) const
{
    qDebug() << "\n=== Scene State:" << context << "===";
    const QList<QGraphicsItem*> currentItems = items();
    qDebug() << "QGraphicsScene items:" << currentItems.size();
    qDebug() << "m_nodes hash size:" << m_nodes.size();
    qDebug() << "m_edges hash size:" << m_edges.size();

    int validNodes = 0;
    int invalidNodes = 0;
    for (auto it = m_nodes.constBegin(); it != m_nodes.constEnd(); ++it) {
        bool found = false;
        for (QGraphicsItem* item : currentItems) {
            if (item == it.value()) {
                found = true;
                break;
            }
        }
        if (found) {
            ++validNodes;
        } else {
            ++invalidNodes;
        }
    }
    qDebug() << "Valid node pointers:" << validNodes;
    qDebug() << "INVALID node pointers:" << invalidNodes;
    qDebug() << "========================\n";
}

int Scene::validatePointers() const
{
    const QList<QGraphicsItem*> currentItems = items();
    int invalidNodes = 0;
    for (auto it = m_nodes.constBegin(); it != m_nodes.constEnd(); ++it) {
        bool found = false;
        for (QGraphicsItem* item : currentItems) {
            if (item == it.value()) {
                found = true;
                break;
            }
        }
        if (!found) {
            ++invalidNodes;
        }
    }
    return invalidNodes;
}

void Scene::notifyNodeDestroyed(Node* node)
{
    if (!node) {
        return;
    }

    const QUuid nodeId = node->getId();
    if (m_nodes.remove(nodeId) > 0) {
        qDebug() << "Scene::notifyNodeDestroyed removed node"
                 << nodeId.toString(QUuid::WithoutBraces).left(8)
                 << "- remaining nodes:" << m_nodes.size();
    } else {
        qDebug() << "Scene::notifyNodeDestroyed - node"
                 << nodeId.toString(QUuid::WithoutBraces).left(8)
                 << "already absent from registry";
    }
}

void Scene::notifyEdgeDestroyed(Edge* edge)
{
    if (!edge) {
        return;
    }

    const QUuid edgeId = edge->getId();
    if (m_edges.remove(edgeId) > 0) {
        qDebug() << "Scene::notifyEdgeDestroyed removed edge"
                 << edgeId.toString(QUuid::WithoutBraces).left(8)
                 << "- remaining edges:" << m_edges.size();
    } else {
        qDebug() << "Scene::notifyEdgeDestroyed - edge"
                 << edgeId.toString(QUuid::WithoutBraces).left(8)
                 << "already absent from registry";
    }
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

    edge->detachSockets();
    
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

void Scene::clear()
{
    ScopedClearing guard(s_clearingGraph);

    qDebug() << "Scene::clear override - items:" << items().size()
             << "nodes:" << m_nodes.size()
             << "edges:" << m_edges.size();
    logSceneState("Scene::clear (before QGraphicsScene::clear)");

    QGraphicsScene::clear();

    if (!m_nodes.isEmpty() || !m_edges.isEmpty()) {
        qWarning() << "Scene::clear: registries not empty after clear!"
                   << "nodes:" << m_nodes.size()
                   << "edges:" << m_edges.size();
        m_nodes.clear();
        m_edges.clear();
    }

    notifyGraphCleared();
    emit sceneChanged();

    logSceneState("Scene::clear (after QGraphicsScene::clear)");
    qDebug() << "Scene::clear complete - registries synced";
}

void Scene::clearGraph()
{
    qDebug() << "Scene::clearGraph requested";
    clear();
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
    clearGraph();
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
    m_ghostCurrentPos = startPos;
    
    // Set source socket to connecting state
    fromSocket->setConnectionState(Socket::Connecting);
    
    updateGhostEdge(startPos);
    
    qDebug() << "GHOST: Started from socket" << fromSocket->getIndex() 
             << "(" << (fromSocket->getRole() == Socket::Input ? "Input" : "Output") << ")";
}

void Scene::updateGhostEdge(const QPointF& currentPos)
{
    if (!m_ghostEdge || !m_ghostFromSocket) {
        return;
    }

    m_ghostCurrentPos = currentPos;
    
    QPointF start = m_ghostFromSocket->scenePos();
    QPainterPath path;
    path.moveTo(start);
    
    // Create curved ghost edge similar to real edges
    qreal dx = currentPos.x() - start.x();
    qreal controlOffset = qMin(qAbs(dx) * 0.5, 100.0);
    
    QPointF control1 = start + QPointF(controlOffset, 0);
    QPointF control2 = currentPos - QPointF(controlOffset, 0);
    path.cubicTo(control1, control2, currentPos);
    
    // Update ghost edge visual based on target validity (typed lookup)
    QPen ghostPenCurrent = ghostPen();
    Socket* targetSocket = socketAt(currentPos);
    
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
        for (Socket* socket : node->getAllSockets()) {
            if (!socket || socket == m_ghostFromSocket) {
                continue;
            }
            socket->updateConnectionState(); // Reset to connected/disconnected
        }
    }
}

void Scene::finishGhostEdge(Socket* toSocket)
{
    if (!toSocket) {
        cancelGhostEdge();
        return;
    }

    Socket* resolvedTarget = toSocket;

    if (!resolvedTarget || resolvedTarget->getRole() != Socket::Input) {
        // Magnet to nearest input socket within 24px
        constexpr qreal magnetRadius = 24.0;
        qreal bestDistance = magnetRadius;
        Socket* bestSocket = nullptr;

        for (Node* node : m_nodes.values()) {
            for (Socket* socket : node->getInputSockets()) {
                if (!socket || socket == m_ghostFromSocket) {
                    continue;
                }
                qreal distance = QLineF(socket->scenePos(), m_ghostCurrentPos).length();
                if (distance < bestDistance) {
                    bestDistance = distance;
                    bestSocket = socket;
                }
            }
        }

        if (bestSocket) {
            qDebug() << "GHOST: Magnet snapped to input socket"
                     << bestSocket->getParentNode()->getId().toString(QUuid::WithoutBraces).left(8)
                     << "index" << bestSocket->getIndex();
            resolvedTarget = bestSocket;
        }
    }

    if (m_ghostFromSocket && resolvedTarget) {
        qDebug() << "GHOST: Attempting connection from role"
                 << (m_ghostFromSocket->getRole() == Socket::Output ? "Output" : "Input")
                 << "to role"
                 << (resolvedTarget->getRole() == Socket::Output ? "Output" : "Input");
        // Validate connection roles
        if (m_ghostFromSocket->getRole() == Socket::Output && 
            resolvedTarget->getRole() == Socket::Input) {

            // Mirror the one-edge-per-socket rule for immediate feedback
            if (m_ghostFromSocket->isConnected() || resolvedTarget->isConnected()) {
                qWarning() << "Scene::finishGhostEdge: socket already connected; rejecting";
                cancelGhostEdge();
                return;
            }
            
            if (m_graphFactory) {
                Q_ASSERT(m_ghostFromSocket && resolvedTarget); // Ghost edge requires both sockets
                // Use factory for consistent edge creation
                Edge* newEdge = m_graphFactory->connectSockets(m_ghostFromSocket, resolvedTarget);
                if (newEdge) {
                    qDebug() << "GHOST: Created edge via factory"
                             << m_ghostFromSocket->getParentNode()->getId().toString(QUuid::WithoutBraces).left(8)
                             << ":" << m_ghostFromSocket->getIndex()
                             << "->"
                             << resolvedTarget->getParentNode()->getId().toString(QUuid::WithoutBraces).left(8)
                             << ":" << resolvedTarget->getIndex()
                             << "edge" << newEdge->getId().toString(QUuid::WithoutBraces).left(8);
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

void Scene::keyPressEvent(QKeyEvent* event)
{
    QGraphicsScene::keyPressEvent(event);
}

QPointF Scene::snapPoint(const QPointF& scenePos) const
{
    // Simple grid snapping (when snap-to-grid system is implemented)
    int grid = gridSize();
    if (grid <= 1) {
        return scenePos;
    }
    
    qreal x = qRound(scenePos.x() / grid) * grid;
    qreal y = qRound(scenePos.y() / grid) * grid;
    return QPointF(x, y);
}

void Scene::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    if (m_ghostEdgeActive && event->button() == Qt::RightButton) {
        // Find socket under mouse (typed lookup)
        Socket* targetSocket = socketAt(event->scenePos());
        finishGhostEdge(targetSocket);
        event->accept();
        return;
    }
    QGraphicsScene::mouseReleaseEvent(event);
}

// JavaScript engine methods removed - focusing on core C++ functionality
