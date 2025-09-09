#include "scene.h"
#include "node.h"
#include "edge.h"
#include "socket.h"
#include "graph_factory.h"
#include "ghost_edge.h"
#include <QRandomGenerator>
#include <QElapsedTimer>
#include <QtMath>
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
    if (!m_ghostEdge || !m_ghostFromSocket) {
        return;
    }
    
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

    GraphSubject::beginBatch();
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
    GraphSubject::endBatch();
    emit sceneChanged();
    
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

    GraphSubject::beginBatch();

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

    GraphSubject::endBatch();
    emit sceneChanged();
    
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
        
        // Update the view
        emit sceneChanged();
        
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
