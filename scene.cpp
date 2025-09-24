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
#include <QMetaObject>
#include <cmath>

// Static flag for clearing state
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
    , m_shutdownInProgress(false)
    , m_graphFactory(nullptr)
    // JavaScript engine initialization removed
{
    setSceneRect(-1000, -1000, 2000, 2000);
    
    // JavaScript engine initialization removed - focusing on core C++ functionality
}

Scene::~Scene()
{
    // Ensure scene is empty before base dtor invokes QGraphicsScene::clear()
    clearGraphControlled();
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
    
    // Emit signal for UI updates (unless clearing)
    if (!m_isClearing) {
        emit sceneChanged();
    }
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
    
    // Emit signal for UI updates (unless clearing)
    if (!m_isClearing) {
        emit sceneChanged();
    }
    
    // Clean design: edges manage their own socket connections via resolveConnections()
}

void Scene::addSocket(Socket* socket)
{
    if (!socket) {
        return;
    }
    
    // Clean design: sockets are managed by their parent nodes, not scene
    // Socket is automatically added to scene as child of parent node
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
    
    // Emit signal for UI updates (unless clearing)
    if (!m_isClearing) {
        emit sceneChanged();
    }
    
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
    
    // Emit signal for UI updates (unless clearing)
    if (!m_isClearing) {
        emit sceneChanged();
    }
    
    qDebug() << "Edge deleted - Observer notified";
}

// DELETED: Scene::deleteSelected() method removed
// Reason: Violates Qt architecture - items should handle their own delete events
// New approach: Each QGraphicsItem handles keyPressEvent directly

void Scene::clearGraph()
{
    // Keep symbol compatibility; forward to controlled clear
    clearGraphControlled();
}

void Scene::clearGraphControlled()
{
    if (m_isClearing) return;       // re-entrancy safe
    cancelGhostEdge();              // safe + idempotent
    m_isClearing = true;

    qDebug() << "Controlled clearing: removing" << m_edges.size() << "edges and" << m_nodes.size() << "nodes";
    
    this->beginBatch();

    // 1) Remove all edges FIRST (safe detach from sockets/nodes while all alive)
    const auto edgeIds = m_edges.keys();
    for (const auto& eid : edgeIds) {
        removeEdgeImmediate(eid);
        notifyEdgeRemoved(eid);
    }

    // 2) Remove all nodes AFTER edges
    const auto nodeIds = m_nodes.keys();
    for (const auto& nid : nodeIds) {
        removeNodeImmediate(nid);
        notifyNodeRemoved(nid);
    }

    // Clear deprecated socket registry
    m_sockets.clear();

    notifyGraphCleared();
    this->endBatch();
    
    qDebug() << "Controlled clearing complete";

    m_isClearing = false;
}

void Scene::removeEdgeImmediate(const QUuid& id)
{
    Edge* e = m_edges.take(id);
    if (!e) return;

    // Detach from sockets and update visuals (no cross-calls in destructors)
    if (auto s = e->getFromSocket()) { 
        s->setConnectedEdge(nullptr); 
        if (s->scene()) s->updateConnectionState(); 
    }
    if (auto s = e->getToSocket()) { 
        s->setConnectedEdge(nullptr); 
        if (s->scene()) s->updateConnectionState(); 
    }

    // Unregister from nodes
    if (auto n = e->getFromNode()) n->unregisterEdge(e);
    if (auto n = e->getToNode()) n->unregisterEdge(e);

    removeItem(e);
    delete e;
}

void Scene::removeNodeImmediate(const QUuid& id)
{
    // Delete incident edges first (defensive if any remain)
    QList<QUuid> incident;
    for (auto it = m_edges.constBegin(); it != m_edges.constEnd(); ++it) {
        Edge* e = it.value();
        if (!e) continue;
        if (e->isConnectedToNode(id)) incident.push_back(it.key());
    }
    for (const auto& eid : incident) removeEdgeImmediate(eid);

    Node* n = m_nodes.take(id);
    if (!n) return;
    removeItem(n);
    delete n;
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

void Scene::cleanupGhost()
{
    if (m_ghostEdge) {
        removeItem(m_ghostEdge);
        delete m_ghostEdge;
        m_ghostEdge = nullptr;
    }
    m_ghostFromSocket = nullptr;
    m_ghostEdgeActive = false;   // Ensure consistent state
}

void Scene::startGhostEdge(Socket* fromSocket, const QPointF& startPos)
{
    if (!m_ghostEdge) {
        m_ghostEdge = new GhostEdge();
        addItem(m_ghostEdge);
    }
    
    m_ghostEdge->setVisible(true);
    m_ghostEdge->setPen(ghostPen());
    m_ghostFromSocket = fromSocket;
    m_ghostEdgeActive = true;

    // Set source socket to connecting state
    resetAllSocketStates();
    if (fromSocket) {
        fromSocket->setConnectionState(Socket::Connecting);
    }

    updateGhostEdge(startPos);

    qDebug() << "GHOST FLOW: start"
             << "role" << (fromSocket ? Socket::roleToString(fromSocket->getRole()) : "<null>")
             << "index" << (fromSocket ? fromSocket->getIndex() : -1)
             << "scenePos" << startPos;
}

void Scene::updateGhostEdge(const QPointF& currentPos)
{
    if (!m_ghostEdge || !m_ghostFromSocket) {
        qDebug() << "GHOST FLOW: update skipped" << (m_ghostEdge ? "no source" : "no ghost edge");
        return;
    }

    const QPointF start = m_ghostFromSocket->scenePos();
    QPainterPath path;
    path.moveTo(start);

    // Find nearest valid socket for magnetic attraction
    QPointF endPos = currentPos;
    Socket* targetSocket = findNearestValidSocket(currentPos, m_ghostFromSocket, endPos);
    
    // Create curved ghost edge similar to real edges
    qreal dx = endPos.x() - start.x();
    qreal controlOffset = qMin(qAbs(dx) * 0.5, 100.0);

    QPointF control1 = start + QPointF(controlOffset, 0);
    QPointF control2 = endPos - QPointF(controlOffset, 0);
    path.cubicTo(control1, control2, endPos);
    m_ghostEdge->setPath(path);

    resetAllSocketStates();

    QPen currentPen = ghostPen();
    bool valid = false;
    if (targetSocket) {
        valid = (targetSocket->getRole() == Socket::Input &&
                 targetSocket != m_ghostFromSocket &&
                 targetSocket->getParentNode() != m_ghostFromSocket->getParentNode() &&
                 !m_ghostFromSocket->isConnected() &&
                 !targetSocket->isConnected());

        if (valid) {
            targetSocket->setConnectionState(Socket::Highlighted);
            // Enhanced magnetic connection visual feedback
            currentPen.setColor(QColor(40, 220, 60, 220));
            currentPen.setWidth(4);
            currentPen.setStyle(Qt::SolidLine); // Solid when magnetically connected
        } else {
            currentPen.setColor(QColor(200, 60, 60, 180));
        }
    } else {
        currentPen.setColor(QColor(0, 255, 0, 150));
        currentPen.setWidth(3);
        currentPen.setStyle(Qt::DashLine);
    }

    m_ghostEdge->setPen(currentPen);

    qDebug() << "GHOST FLOW: update" << "cursor" << currentPos
             << "snapped" << endPos
             << "target" << (targetSocket ? QStringLiteral("socket") : QStringLiteral("none"))
             << "valid" << valid
             << "distance" << (targetSocket ? QLineF(currentPos, targetSocket->scenePos()).length() : -1);
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

void Scene::finishGhostEdge(const QPointF& scenePos)
{
    if (!m_ghostEdge || !m_ghostFromSocket) {
        cleanupGhost();
        m_ghostEdgeActive = false;   // ensure ghost mode is off
        return;
    }

    // Find the target socket using magnetic attraction (same as in update)
    QPointF snappedPos;
    Socket* target = findNearestValidSocket(scenePos, m_ghostFromSocket, snappedPos);

    // Validate: source must be Output, target must be Input, no self-node, both free
    Socket* src = m_ghostFromSocket;  // Use the real source
    const bool ok = src && target &&
                    src != target &&
                    src->getRole() == Socket::Output &&
                    target->getRole() == Socket::Input &&
                    src->getParentNode() != target->getParentNode() &&
                    !src->isConnected() && !target->isConnected();

    if (ok && m_graphFactory) {
        m_graphFactory->connectSockets(src, target);
    }

    cleanupGhost();
    m_ghostEdgeActive = false;        // Make sure we leave ghost mode
}

void Scene::cancelGhostEdge()
{
    // Reset all socket visual states
    resetAllSocketStates();
    cleanupGhost();
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
    if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
        // Centralized deletion - handles multi-selection properly
        QList<QGraphicsItem*> selected = selectedItems();
        if (selected.isEmpty()) {
            QGraphicsScene::keyPressEvent(event);
            return;
        }
        
        qDebug() << "=== SCENE CENTRALIZED DELETION START ===";
        qDebug() << "Deleting" << selected.size() << "selected items";
        
        // Separate items by type for proper deletion order
        QList<Node*> nodesToDelete;
        QList<Edge*> edgesToDelete;
        QList<Socket*> socketsToDelete;
        
        for (QGraphicsItem* item : selected) {
            if (auto* node = qgraphicsitem_cast<Node*>(item)) {
                nodesToDelete.append(node);
            } else if (auto* edge = qgraphicsitem_cast<Edge*>(item)) {
                edgesToDelete.append(edge);
            } else if (auto* socket = qgraphicsitem_cast<Socket*>(item)) {
                socketsToDelete.append(socket);
            }
        }
        
        // Delete edges first (they reference nodes/sockets)
        // Use queued connections to avoid deletion during event processing
        for (Edge* edge : edgesToDelete) {
            qDebug() << "Scene queuing edge deletion" << edge->getId().toString(QUuid::WithoutBraces).left(8);
            QUuid edgeId = edge->getId();
            QMetaObject::invokeMethod(this, [this, edgeId]() {
                deleteEdge(edgeId);
            }, Qt::QueuedConnection);
        }
        
        // Delete nodes (this will handle their sockets automatically)
        for (Node* node : nodesToDelete) {
            qDebug() << "Scene queuing node deletion" << node->getId().toString(QUuid::WithoutBraces).left(8);
            QUuid nodeId = node->getId();
            QMetaObject::invokeMethod(this, [this, nodeId]() {
                deleteNode(nodeId);
            }, Qt::QueuedConnection);
        }
        
        // Note: Sockets are typically deleted with their parent nodes
        // Individual socket deletion is unusual but supported
        for (Socket* socket : socketsToDelete) {
            qDebug() << "Scene deleting individual socket" << socket->getRole() << socket->getIndex();
            // Disconnect any connected edges first
            if (socket->isConnected()) {
                deleteEdge(socket->getConnectedEdge()->getId());
            }
        }
        
        qDebug() << "=== SCENE CENTRALIZED DELETION COMPLETE ===";
        event->accept();
    } else {
        QGraphicsScene::keyPressEvent(event);
    }
}

Socket* Scene::findNearestValidSocket(const QPointF& scenePos, Socket* fromSocket, QPointF& snappedPos)
{
    if (!fromSocket) {
        snappedPos = scenePos;
        return nullptr;
    }
    
    Socket* nearestSocket = nullptr;
    qreal minDistance = getMagneticRadius();
    snappedPos = scenePos;
    
    // Check all sockets in the scene for magnetic attraction
    for (Node* node : m_nodes.values()) {
        for (QGraphicsItem* child : node->childItems()) {
            if (Socket* socket = qgraphicsitem_cast<Socket*>(child)) {
                // Skip if not a valid target
                if (socket->getRole() != Socket::Input ||
                    socket == fromSocket ||
                    socket->getParentNode() == fromSocket->getParentNode() ||
                    socket->isConnected() ||
                    fromSocket->isConnected()) {
                    continue;
                }
                
                // Calculate distance to socket center
                QPointF socketPos = socket->scenePos();
                qreal distance = QLineF(scenePos, socketPos).length();
                
                // If within magnetic radius and closer than previous best
                if (distance < minDistance) {
                    minDistance = distance;
                    nearestSocket = socket;
                    snappedPos = socketPos; // Snap to socket center
                }
            }
        }
    }
    
    return nearestSocket;
}

// GraphSubject implementation
int Scene::s_batchDepth = 0;

void Scene::attach(GraphObserver* observer)
{
    m_observers.insert(observer);
}

void Scene::detach(GraphObserver* observer)
{
    m_observers.remove(observer);
}

void Scene::beginBatch()
{
    s_batchDepth++;
}

void Scene::endBatch()
{
    if (s_batchDepth > 0) {
        s_batchDepth--;
    }
}

bool Scene::isInBatch() const
{
    return s_batchDepth > 0;
}

void Scene::notifyNodeAdded(const Node& node)
{
    if (isInBatch()) return;
    for (GraphObserver* observer : m_observers) {
        observer->onNodeAdded(node);
    }
}

void Scene::notifyNodeRemoved(const QUuid& nodeId)
{
    if (isInBatch()) return;
    for (GraphObserver* observer : m_observers) {
        observer->onNodeRemoved(nodeId);
    }
}

void Scene::notifyNodeMoved(const QUuid& nodeId, QPointF oldPos, QPointF newPos)
{
    if (isInBatch()) return;
    for (GraphObserver* observer : m_observers) {
        observer->onNodeMoved(nodeId, oldPos, newPos);
    }
}

void Scene::notifyEdgeAdded(const Edge& edge)
{
    if (isInBatch()) return;
    for (GraphObserver* observer : m_observers) {
        observer->onEdgeAdded(edge);
    }
}

void Scene::notifyEdgeRemoved(const QUuid& edgeId)
{
    if (isInBatch()) return;
    for (GraphObserver* observer : m_observers) {
        observer->onEdgeRemoved(edgeId);
    }
}

void Scene::notifyGraphCleared()
{
    if (isInBatch()) return;
    for (GraphObserver* observer : m_observers) {
        observer->onGraphCleared();
    }
}

void Scene::notifyGraphLoaded(const QString& filename)
{
    if (isInBatch()) return;
    for (GraphObserver* observer : m_observers) {
        observer->onGraphLoaded(filename);
    }
}

void Scene::notifyGraphSaved(const QString& filename)
{
    if (isInBatch()) return;
    for (GraphObserver* observer : m_observers) {
        observer->onGraphSaved(filename);
    }
}

// ============================================================================
// Auto Layout (Simulated Annealing) Implementation
// ============================================================================

namespace {
    inline qreal sqr(qreal v) { return v*v; }
    
    // Simple energy for "even spread":
    //  E = sum_{i<j} [ wRep/(d^2+eps) + wOverlap * max(0, minSpacing-d)^2 ]
    //    + wGrav * sum_i ||p_i - centroid||^2
    static double computeEnergy(const QVector<QPointF>& P,
                                double minSpacing,
                                double wRep, double wOverlap, double wGrav,
                                const QPointF& centroid)
    {
        const int n = P.size();
        if (n <= 1) {
            return 0.0;
        }
        const double eps = 1e-6;
        double E = 0.0;
        for (int i = 0; i < n; ++i) {
            // gravity (keeps system from drifting to infinity)
            const QPointF dC = P[i] - centroid;
            E += wGrav * (sqr(dC.x()) + sqr(dC.y()));
            for (int j = i+1; j < n; ++j) {
                const QPointF d = P[i] - P[j];
                const double d2 = d.x()*d.x() + d.y()*d.y() + eps;
                const double dlen = std::sqrt(d2);
                E += wRep / d2;
                if (dlen < minSpacing) {
                    const double pen = (minSpacing - dlen);
                    E += wOverlap * pen * pen;
                }
            }
        }
        return E;
    }
}

void Scene::autoLayoutAnneal(bool selectionOnly, int maxIters, double t0, double t1)
{
    // Collect nodes: selection, else all
    QList<Node*> nodesList;
    if (selectionOnly) {
        for (QGraphicsItem* gi : selectedItems())
            if (Node* n = qgraphicsitem_cast<Node*>(gi)) {
                nodesList.push_back(n);
            }
    }
    if (nodesList.isEmpty()) {
        for (auto it = m_nodes.begin(); it != m_nodes.end(); ++it) // uses your typed registry
            nodesList.push_back(it.value());
    }
    if (nodesList.size() < 2) {
        return;
    }

    // Initial positions and centroid
    QVector<QPointF> P;
    P.reserve(nodesList.size());
    QPointF centroid(0,0);
    for (Node* n : nodesList) {
        P.push_back(n->pos());
        centroid += n->pos();
    }
    centroid /= qreal(nodesList.size());

    // Parameters (tweakable): minSpacing uses grid size if snapping is on
    const double minSpacing = qMax(40, isSnapToGrid() ? gridSize()*2 : gridSize()*2);
    const double wRep = 2000.0;     // repulsion strength
    const double wOverlap = 4.0;    // overlap penalty when too close
    const double wGrav = 0.001;     // weak gravity toward initial centroid
    const double moveBase = qMax(20, gridSize()*2);

    auto energy = [&](const QVector<QPointF>& X){
        return computeEnergy(X, minSpacing, wRep, wOverlap, wGrav, centroid);
    };

    // Annealing loop
    QRandomGenerator* rng = QRandomGenerator::global();
    double E = energy(P);
    const int N = P.size();
    if (maxIters <= 0) {
        maxIters = 2000;
    }

    this->beginBatch();
    QElapsedTimer timer;
    timer.start();
    for (int k = 0; k < maxIters; ++k) {
        // geometric cooling
        const double alpha = (maxIters>1) ? double(k) / double(maxIters-1) : 1.0;
        const double T = t0 * std::pow(t1 / t0, alpha);
        const double step = moveBase * (0.25 + 0.75 * T); // smaller steps as we cool

        // pick a random node and propose a small move
        const int i = rng->bounded(N);
        const QPointF oldP = P[i];
        const double dx = (rng->generateDouble() - 0.5) * 2.0 * step;
        const double dy = (rng->generateDouble() - 0.5) * 2.0 * step;
        P[i] = oldP + QPointF(dx, dy);

        // Evaluate
        const double En = energy(P);
        const double dE = En - E;
        bool accept = false;
        if (dE <= 0.0) {
            accept = true;
        }
        else {
            const double u = rng->generateDouble(); // [0,1)
            const double prob = std::exp(-dE / qMax(1e-9, T));
            accept = (u < prob);
        }
        if (accept) {
            E = En; // keep P[i]
        } else {
            P[i] = oldP; // revert
        }

        // Time guard: ~50ms default budget to keep UI snappy
        if (timer.elapsed() > 50 && k > N*50) {
            break;
        }
    }

    // Commit final positions
    for (int i = 0; i < N; ++i) nodesList[i]->setPos(P[i]);

    // Snap at the end if enabled
    if (isSnapToGrid()) {
        for (Node* n : nodesList) {
            const QPointF snappedScene = snapPoint(n->scenePos());
            n->setPos(snappedScene); // top-level items: scenePos == pos
        }
    }
    this->endBatch();
    if (!m_isClearing) {
        emit sceneChanged();
    }
    
    qDebug() << "Auto-layout complete:" << nodesList.size() << "nodes arranged";
}

void Scene::autoLayoutForceDirected(bool selectionOnly, int maxIters, double cooling)
{
    // 1) Collect nodes
    QList<Node*> nodes;
    if (selectionOnly) {
        for (QGraphicsItem* gi : selectedItems()) {
            if (auto* n = qgraphicsitem_cast<Node*>(gi)) {
                nodes.push_back(n);
            }
        }
    }
    if (nodes.isEmpty()) {
        for (auto it = m_nodes.begin(); it != m_nodes.end(); ++it) {
            nodes.push_back(it.value());
        }
    }
    const int N = nodes.size();
    if (N < 2) {
        return;
    }

    // 2) Build sized geometry: center, size, radius, origin-offset
    struct Geom {
        Node* n;
        QPointF center;     // scene-space center
        QSizeF  size;       // sceneBoundingRect size
        double  radius;     // 0.5 * diagonal (circle approx)
        QPointF centerMinusScenePos; // center - scenePos() to convert center->pos
    };
    QVector<Geom> G;
    G.reserve(N);
    QRectF bbox;
    for (int i = 0; i < N; ++i) {
        Node* n = nodes[i];
        const QRectF r = n->sceneBoundingRect();   // true visual size in scene coords
        const QPointF c = r.center();
        const QSizeF  s = r.size();
        const double  rad = 0.5 * std::hypot(s.width(), s.height());
        const QPointF offset = c - n->scenePos();  // how far the node's center is from its origin
        G.push_back({n, c, s, rad, offset});
        bbox |= r;
    }
    if (bbox.width() < 1) {
        bbox.setWidth(200);
    }
    if (bbox.height() < 1) {
        bbox.setHeight(200);
    }

    // 3) Edge list (optional attraction). Adapt to your Edge API if needed.
    // If you have Scene::getEdges() and Edge::getFromNode()/getToNode(), keep this; otherwise leave 'edges' empty.
    QVector<QPair<int,int>> edges;
    {
        QHash<QUuid,int> idx;
        idx.reserve(N);
        for (int i = 0; i < N; ++i) {
            idx.insert(G[i].n->getId(), i);
        }
        for (Edge* e : getEdges()) {
            Node* a = e->getFromNode();
            Node* b = e->getToNode();
            if (!a || !b) {
                continue;
            }
            int ia = idx.value(a->getId(), -1);
            int ib = idx.value(b->getId(), -1);
            if (ia >= 0 && ib >= 0 && ia != ib) {
                edges.push_back({ia, ib});
            }
        }
    }

    // 4) Positions are node centers; displacements in scene space
    QVector<QPointF> pos(N), disp(N, QPointF(0,0));
    for (int i = 0; i < N; ++i) {
        pos[i] = G[i].center;
    }

    // 5) Constants: ideal edge length base, snap-aware margin
    const double area = std::max(1.0, bbox.width() * bbox.height());
    const double kBase = std::sqrt(area / double(N));           // classic FR scale
    const double margin = std::max<double>( (isSnapToGrid() ? gridSize() * 0.5 : 8.0), 6.0 ); // extra buffer
    double t = std::max(bbox.width(), bbox.height());           // temperature (step cap)
    if (maxIters <= 0) {
        maxIters = 300;
    }
    if (cooling <= 0.0 || cooling >= 1.0) {
        cooling = 0.92;
    }

    auto safeDistance = [&](int i, int j)->double {
        return G[i].radius + G[j].radius + margin;
    };

    this->beginBatch();

    for (int iter = 0; iter < maxIters; ++iter) {
        std::fill(disp.begin(), disp.end(), QPointF(0,0));

        // Repulsion (size-aware): use d_eff = |delta| - (r_i + r_j + margin), clamp to epsilon
        for (int i = 0; i < N; ++i) {
            for (int j = i + 1; j < N; ++j) {
                QPointF delta = pos[i] - pos[j];
                double d = std::hypot(delta.x(), delta.y());
                const double dSafe = safeDistance(i,j);
                double d_eff = d - dSafe;
                if (d_eff < 1e-6) {
                    d_eff = 1e-6; // overlapping/too close -> strong repel
                }
                // classic FR repulsion with size-aware distance
                const double Fr = (kBase * kBase) / d_eff;
                QPointF dir = (d > 1e-9) ? (delta / d) : QPointF( (i & 1) ? 1 : -1, (j & 1) ? 1 : -1 );
                QPointF force = dir * Fr;
                disp[i] += force;
                disp[j] -= force;
            }
        }

        // Attraction along edges: target length includes node sizes
        for (auto e : edges) {
            int i = e.first, j = e.second;
            QPointF delta = pos[i] - pos[j];
            double d = std::hypot(delta.x(), delta.y());
            const double target = kBase + safeDistance(i,j);  // longer springs if nodes are big
            // classic FR attraction with target length
            const double Fa = (d * d) / std::max(1e-9, target);
            QPointF dir = (d > 1e-9) ? (delta / d) : QPointF(0,0);
            QPointF force = dir * Fa;
            disp[i] -= force;
            disp[j] += force;
        }

        // Move centers, capping by temperature 't'
        for (int i = 0; i < N; ++i) {
            QPointF d = disp[i];
            double len = std::hypot(d.x(), d.y());
            if (len > 1e-9) {
                pos[i] += d * (std::min(t, len) / len);
            }
        }

        t *= cooling;
        if (t < 0.5) {
            break;
        }
    }

    // 6) Commit: convert center targets back to node->setPos(origin)
    for (int i = 0; i < N; ++i) {
        QPointF center = pos[i];
        if (isSnapToGrid()) {
            center = snapPoint(center);
        }
        // origin scene position so that center lands at 'center'
        QPointF newScenePos = center - G[i].centerMinusScenePos;
        G[i].n->setPos(newScenePos);
    }

    this->endBatch();
    if (!m_isClearing) {
        emit sceneChanged();
    }
    
    qDebug() << "Size-aware force layout complete:" << nodes.size() << "nodes arranged";
}

void Scene::debugForceLayout3Nodes()
{
    qDebug() << "\n=== DEBUG: 3-Node Animated Force Layout ===";
    
    // Clear scene and create exactly 3 nodes: source -> transfer -> sink
    clearGraph();
    
    if (!m_graphFactory) {
        qDebug() << "ERROR: No graph factory available";
        return;
    }
    
    qDebug() << "Step 1: Creating 3 test nodes...";
    
    // Create nodes with specific positions to see geometry mapping
    Node* source = m_graphFactory->createNode("Input", QPointF(-200, 0));
    Node* transfer = m_graphFactory->createNode("Process", QPointF(0, 0)); 
    Node* sink = m_graphFactory->createNode("Output", QPointF(200, 0));
    
    if (!source || !transfer || !sink) {
        qDebug() << "ERROR: Failed to create test nodes";
        return;
    }
    
    qDebug() << "Step 2: Connecting nodes with edges...";
    
    // Connect: source -> transfer -> sink
    Socket* sourceOut = nullptr;
    Socket* transferIn = nullptr;
    Socket* transferOut = nullptr;
    Socket* sinkIn = nullptr;
    
    // Find sockets (assuming first output/input sockets)
    for (QGraphicsItem* child : source->childItems()) {
        if (Socket* s = qgraphicsitem_cast<Socket*>(child)) {
            if (s->getRole() == Socket::Output && !sourceOut) {
                sourceOut = s;
                break;
            }
        }
    }
    
    for (QGraphicsItem* child : transfer->childItems()) {
        if (Socket* s = qgraphicsitem_cast<Socket*>(child)) {
            if (s->getRole() == Socket::Input && !transferIn) {
                transferIn = s;
            } else if (s->getRole() == Socket::Output && !transferOut) {
                transferOut = s;
            }
        }
    }
    
    for (QGraphicsItem* child : sink->childItems()) {
        if (Socket* s = qgraphicsitem_cast<Socket*>(child)) {
            if (s->getRole() == Socket::Input && !sinkIn) {
                sinkIn = s;
                break;
            }
        }
    }
    
    // Create connections
    if (sourceOut && transferIn) {
        m_graphFactory->connectSockets(sourceOut, transferIn);
        qDebug() << "Connected: source -> transfer";
    }
    if (transferOut && sinkIn) {
        m_graphFactory->connectSockets(transferOut, sinkIn);
        qDebug() << "Connected: transfer -> sink";
    }
    
    qDebug() << "Step 3: Analyzing initial geometry...";
    
    QList<Node*> nodes = {source, transfer, sink};
    for (int i = 0; i < nodes.size(); ++i) {
        Node* n = nodes[i];
        QRectF bounds = n->sceneBoundingRect();
        QPointF pos = n->scenePos();
        QPointF center = bounds.center();
        QSizeF size = bounds.size();
        
        qDebug() << QString("Node %1:").arg(i);
        qDebug() << QString("  scenePos: (%1, %2)").arg(pos.x()).arg(pos.y());
        qDebug() << QString("  sceneBounds: (%1, %2) %3x%4")
                    .arg(bounds.x()).arg(bounds.y()).arg(size.width()).arg(size.height());
        qDebug() << QString("  center: (%1, %2)").arg(center.x()).arg(center.y());
        qDebug() << QString("  center - pos: (%1, %2)")
                    .arg(center.x() - pos.x()).arg(center.y() - pos.y());
    }
    
    // Start with positions that definitely overlap to see force behavior
    qDebug() << "Step 4: Moving nodes to overlapping positions for force test...";
    source->setPos(QPointF(-50, 0));
    transfer->setPos(QPointF(0, 0));
    sink->setPos(QPointF(50, 0));
    
    qDebug() << "Step 5: Starting animated force simulation...";
    
    // Simple animated force test - just repulsion, no edges for now
    struct DebugGeom {
        Node* n;
        QPointF center;
        double radius;
        QPointF centerOffset;
    };
    
    QVector<DebugGeom> G;
    for (Node* n : nodes) {
        QRectF r = n->sceneBoundingRect();
        QPointF c = r.center();
        double rad = 0.5 * std::hypot(r.width(), r.height());
        QPointF offset = c - n->scenePos();
        G.push_back({n, c, rad, offset});
    }
    
    qDebug() << "Initial positions for force simulation:";
    for (int i = 0; i < 3; ++i) {
        qDebug() << QString("Node %1: center=(%2,%3) radius=%4 offset=(%5,%6)")
                    .arg(i).arg(G[i].center.x()).arg(G[i].center.y()).arg(G[i].radius)
                    .arg(G[i].centerOffset.x()).arg(G[i].centerOffset.y());
    }
    
    // Run a few iterations and show the process
    const double kBase = 100.0;
    const double margin = 20.0;
    
    for (int iter = 0; iter < 5; ++iter) {
        qDebug() << QString("\n--- Force Iteration %1 ---").arg(iter + 1);
        
        QVector<QPointF> forces(3, QPointF(0,0));
        
        // Calculate repulsion forces
        for (int i = 0; i < 3; ++i) {
            for (int j = i + 1; j < 3; ++j) {
                QPointF delta = G[i].center - G[j].center;
                double d = std::hypot(delta.x(), delta.y());
                double dSafe = G[i].radius + G[j].radius + margin;
                double d_eff = d - dSafe;
                
                if (d_eff < 1e-6) {
                    d_eff = 1e-6; // overlapping
                }
                
                double Fr = (kBase * kBase) / d_eff;
                QPointF dir = (d > 1e-9) ? (delta / d) : QPointF(1, 0);
                QPointF force = dir * Fr;
                
                qDebug() << QString("Force %1<->%2: d=%3 dSafe=%4 d_eff=%5 Fr=%6")
                            .arg(i).arg(j).arg(d).arg(dSafe).arg(d_eff).arg(Fr);
                qDebug() << QString("  delta=(%1,%2) force=(%3,%4)")
                            .arg(delta.x()).arg(delta.y()).arg(force.x()).arg(force.y());
                
                forces[i] += force;
                forces[j] -= force;
            }
        }
        
        // Apply forces (small step for debugging)
        double step = 2.0;
        for (int i = 0; i < 3; ++i) {
            QPointF newCenter = G[i].center + forces[i] * step;
            QPointF newScenePos = newCenter - G[i].centerOffset;
            
            qDebug() << QString("Node %1: force=(%2,%3) newCenter=(%4,%5) newPos=(%6,%7)")
                        .arg(i).arg(forces[i].x()).arg(forces[i].y())
                        .arg(newCenter.x()).arg(newCenter.y())
                        .arg(newScenePos.x()).arg(newScenePos.y());
            
            G[i].n->setPos(newScenePos);
            G[i].center = newCenter;
        }
        
        // Update the view (unless clearing)
        if (!m_isClearing) {
            emit sceneChanged();
        }
        
        // Brief pause so we can see the animation
        QApplication::processEvents();
        QThread::msleep(500);
    }
    
    qDebug() << "=== Debug session complete ===\n";
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
        finishGhostEdge(event->scenePos());
        event->accept();
        return;
    }
    QGraphicsScene::mouseReleaseEvent(event);
}

// JavaScript engine methods removed - focusing on core C++ functionality
