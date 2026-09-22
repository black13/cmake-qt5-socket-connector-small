#include "graph.h"
#include "scene.h"
#include "graph_factory.h"
#include "node.h"
#include "scripted_node.h"
#include "edge.h"
#include "socket.h"
#include "node_templates.h"
#include "graph_observer.h"
#include "synthetic_work.h"
#include "qjs_script_backend.h"
#include "duktape_script_backend.h"
#include "undo_commands.h"
#include <QDebug>
#include "nodegraph_logging.h"
#include <QFile>
#include <QGraphicsItem>
#include <QTextStream>
#include <QCoreApplication>
#include <QUndoStack>
#include <QVariantAnimation>
#include <QEasingCurve>
#include <QElapsedTimer>
#include <QThread>
#include <algorithm>
#include <cmath>
#include <QMap>
// XML save support
#include <libxml/tree.h>
#include <libxml/xmlsave.h>

namespace {

ScriptedNode* asScripted(Node* node)
{
    return dynamic_cast<ScriptedNode*>(node);
}

const ScriptedNode* asScripted(const Node* node)
{
    return dynamic_cast<const ScriptedNode*>(node);
}

/// Pick the script backend. Default is QJSEngine; NODEGRAPH_SCRIPT_ENGINE=duktape
/// selects Duktape when compiled in (-DENABLE_DUKTAPE=ON). Thanks to the
/// type-erased seam this is the only place that knows both backends exist.
ScriptEngine makeConfiguredEngine()
{
    const QByteArray choice = qgetenv("NODEGRAPH_SCRIPT_ENGINE").toLower();
#ifdef NODEGRAPH_HAS_DUKTAPE
    if (choice == "duktape") {
        return ScriptEngine(DuktapeBackend());
    }
#else
    if (choice == "duktape") {
        qWarning() << "NODEGRAPH_SCRIPT_ENGINE=duktape requested but the Duktape"
                      "backend is not compiled in (configure with -DENABLE_DUKTAPE=ON);"
                      "falling back to QJSEngine";
    }
#endif
    return ScriptEngine(QJsBackend());
}

} // namespace

Graph::Graph(Scene* scene, GraphFactory* factory, QObject* parent)
    : QObject(parent)
    , m_scene(scene)
    , m_factory(factory)
    , m_scriptEngine(makeConfiguredEngine())
{
    Q_ASSERT(m_scene);
    Q_ASSERT(m_factory);

    qDebug() << "Graph: Facade initialized with script engine:"
             << m_scriptEngine.backendName();

    // Expose the Graph API into the engine, then share it with all nodes
    initializeScripting();
    ScriptedNode::setSharedEngine(m_scriptEngine);
}

Graph::~Graph()
{
    qDebug() << "Graph: Facade destroyed";
    // Drop the nodes' shared handle so the engine heap is released deterministically
    ScriptedNode::setSharedEngine(ScriptEngine());
}

void Graph::jsLog(const QString& message)
{
    // Script output (console.log) is user-facing; keep it on the default
    // category instead of the opt-in verbose one.
    qDebug() << "[JS]" << message;
}

void Graph::quit()
{
    QCoreApplication::quit();
}

void Graph::quit(int exitCode)
{
    QCoreApplication::exit(exitCode);
}

void Graph::yield(int milliseconds)
{
    // Pump the event loop in slices so timers (autosave), animations and the
    // watchdog keep running while a script is mid-flight. Sleeps shorten the
    // slice; a pure processEvents loop would spin a core.
    QElapsedTimer timer;
    timer.start();
    do {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
        if (milliseconds > 0) {
            QThread::msleep(5);
        }
    } while (milliseconds > 0 && timer.elapsed() < milliseconds);
}

void Graph::initializeScripting()
{
    // Expose this Graph object to scripts as global "graph".
    // The bridge is backend-specific but the call is not.
    m_scriptEngine.registerObject(QStringLiteral("graph"), this);

    // console.log glue in plain JS - works on any ECMAScript backend
    const ScriptResult consoleGlue = m_scriptEngine.evaluate(QStringLiteral(
        "var console = { log: function(msg) { graph.jsLog(String(msg)); } };"));
    if (!consoleGlue.ok()) {
        qWarning() << "Graph: failed to install console.log glue:" << consoleGlue.error;
    }

    qDebug() << "Graph: script engine initialized - 'graph' object available"
             << "(backend:" << m_scriptEngine.backendName() << ")";
}

// ========== Node Operations ==========

QString Graph::createNode(const QString& type, qreal x, qreal y)
{
    if (!isValidNodeType(type)) {
        QString error = QString("Invalid node type: %1").arg(type);
        qCritical() << "Graph::createNode:" << error;
        emit errorOccurred(error);
        return QString();
    }

    // Non-finite coordinates would poison the scene, serialization writes them
    // as "nan"/"inf", and the loader rejects non-finite values - so a graph
    // saved with such a node could never be loaded again.
    if (!std::isfinite(x) || !std::isfinite(y)) {
        QString error = QString("Invalid node position: (%1, %2) - coordinates must be finite")
                            .arg(x).arg(y);
        qWarning() << "Graph::createNode:" << error;
        emit errorOccurred(error);
        return QString();
    }

    qCDebug(ngVerbose) << "Graph::createNode:" << type << "at" << x << "," << y;

    try {
        if (m_undoStack) {
            auto* command = new CreateNodeCommand(m_scene, m_factory, type, QPointF(x, y));
            pushCommand(command);
            const QUuid uuid = command->nodeId();
            if (uuid.isNull()) {
                const QString error = QString("Failed to create node of type %1").arg(type);
                qCritical() << "Graph::createNode:" << error;
                emit errorOccurred(error);
                return QString();
            }
            const QString id = uuid.toString();
            emit nodeCreated(id);
            qCDebug(ngVerbose) << "Graph::createNode: Created node" << id;
            return id;
        }

        // Use factory to create node
        Node* node = m_factory->createNode(type, QPointF(x, y));
        if (node) {
            QString uuid = node->getId().toString();
            emit nodeCreated(uuid);
            qCDebug(ngVerbose) << "Graph::createNode: Created node" << uuid;
            return uuid;
        }
    } catch (const std::exception& e) {
        QString error = QString("Error creating node: %1").arg(e.what());
        qCritical() << "Graph::createNode:" << error;
        emit errorOccurred(error);
    }

    return QString();
}

QString Graph::dropNode(const QString& type, qreal x, qreal y)
{
    return dropNode(type, x, y, 260);
}

QString Graph::dropNode(const QString& type, qreal x, qreal y, int durationMs)
{
    // Same creation path as createNode(), so template sockets/payload match.
    const QString nodeId = createNode(type, x, y);
    if (nodeId.isEmpty()) {
        return nodeId; // createNode() already emitted errorOccurred
    }

    Node* node = findNode(nodeId);
    if (!node || durationMs <= 0) {
        return nodeId;
    }

    // Animate a 0..1 progress value rather than the graphics item itself: the
    // node is looked up by UUID on every step, so deleting it mid-flight (a
    // script can call graph.deleteNode right after dropping) simply stops the
    // animation instead of touching freed memory, as QGraphicsItemAnimation's
    // raw item pointer would.
    const QPointF target(x, y);
    const QPointF start = target + QPointF(-90.0, -70.0); // "thrown" from up-left
    node->setPos(start);
    node->setOpacity(0.35);

    auto* animation = new QVariantAnimation(this);
    animation->setStartValue(0.0);
    animation->setEndValue(1.0);
    animation->setDuration(durationMs);
    animation->setEasingCurve(QEasingCurve::OutCubic);
    connect(animation, &QVariantAnimation::valueChanged, this,
            [this, uuid = parseUuid(nodeId), start, target](const QVariant& value) {
                Node* animated = m_scene ? m_scene->getNode(uuid) : nullptr;
                if (!animated) {
                    return;
                }
                const qreal t = value.toReal();
                animated->setPos(start + (target - start) * t);
                animated->setOpacity(0.35 + 0.65 * t);
            });
    animation->start(QAbstractAnimation::DeleteWhenStopped);

    qCDebug(ngVerbose) << "Graph::dropNode:" << nodeId << "animated in"
                       << durationMs << "ms to" << target;
    return nodeId;
}

bool Graph::deleteNode(const QString& nodeId)
{
    // Refuse while a node script runs: scripts reach this facade reentrantly,
    // and deleting nodes mid-evaluation leaves running JS on freed objects.
    if (ScriptedNode::isExecuting()) {
        const QString msg = QStringLiteral("Graph::deleteNode refused while a node script is executing");
        qWarning() << msg;
        emit errorOccurred(msg);
        return false;
    }

    Node* node = findNode(nodeId);
    if (!node) {
        qWarning() << "Graph::deleteNode: Node not found:" << nodeId;
        return false;
    }

    qCDebug(ngVerbose) << "Graph::deleteNode:" << nodeId;

    try {
        QUuid uuid = parseUuid(nodeId);
        if (m_undoStack) {
            pushCommand(new DeleteSelectionCommand(m_scene, m_factory,
                                                   QList<QUuid>{uuid}, QList<QUuid>{}));
        } else {
            m_scene->deleteNode(uuid);
        }
        emit nodeDeleted(nodeId);
        return true;
    } catch (const std::exception& e) {
        QString error = QString("Error deleting node: %1").arg(e.what());
        qCritical() << "Graph::deleteNode:" << error;
        emit errorOccurred(error);
        return false;
    }
}

bool Graph::moveNode(const QString& nodeId, qreal dx, qreal dy)
{
    Node* node = findNode(nodeId);
    if (!node) {
        qWarning() << "Graph::moveNode: Node not found:" << nodeId;
        return false;
    }

    QPointF currentPos = node->pos();
    QPointF newPos = currentPos + QPointF(dx, dy);

    // A finite delta can still overflow to infinity; reject both cases so a
    // non-finite position can never reach the scene/serializer.
    if (!std::isfinite(dx) || !std::isfinite(dy) ||
        !std::isfinite(newPos.x()) || !std::isfinite(newPos.y())) {
        const QString error = QString("Graph::moveNode: non-finite move for %1").arg(nodeId);
        qWarning() << error;
        emit errorOccurred(error);
        return false;
    }

    node->setPos(newPos);
    pushCommand(new MoveNodesCommand(m_scene,
        QVector<NodeMove>{NodeMove{parseUuid(nodeId), currentPos, newPos}}));
    emit nodeMoved(nodeId);

    return true;
}

bool Graph::setNodePosition(const QString& nodeId, qreal x, qreal y)
{
    Node* node = findNode(nodeId);
    if (!node) {
        qWarning() << "Graph::setNodePosition: Node not found:" << nodeId;
        return false;
    }

    if (!std::isfinite(x) || !std::isfinite(y)) {
        const QString error = QString("Graph::setNodePosition: non-finite position for %1").arg(nodeId);
        qWarning() << error;
        emit errorOccurred(error);
        return false;
    }

    const QPointF oldPos = node->pos();
    node->setPos(QPointF(x, y));
    pushCommand(new MoveNodesCommand(m_scene,
        QVector<NodeMove>{NodeMove{parseUuid(nodeId), oldPos, QPointF(x, y)}}));
    emit nodeMoved(nodeId);

    return true;
}

QVariantMap Graph::getNodeData(const QString& nodeId) const
{
    Node* node = findNode(nodeId);
    if (!node) {
        return QVariantMap();
    }

    QVariantMap data;
    data["id"] = nodeId;
    data["type"] = node->getNodeType();
    data["x"] = node->pos().x();
    data["y"] = node->pos().y();
    if (auto scripted = asScripted(node)) {
        data["script"] = scripted->script();
        data["payload"] = scripted->payload();
    }

    return data;
}

bool Graph::setNodeScript(const QString& nodeId, const QString& scriptCode)
{
    ScriptedNode* scripted = asScripted(findNode(nodeId));
    if (!scripted) {
        qWarning() << "Graph::setNodeScript: node is not SCRIPT type" << nodeId;
        return false;
    }

    scripted->setScript(scriptCode);
    return true;
}

QString Graph::getNodeScript(const QString& nodeId) const
{
    const ScriptedNode* scripted = asScripted(findNode(nodeId));
    return scripted ? scripted->script() : QString();
}

QVariant Graph::executeNodeScript(const QString& nodeId, const QVariantMap& context)
{
    ScriptedNode* scripted = asScripted(findNode(nodeId));
    if (!scripted) {
        const QString error = QString("executeNodeScript: node not found: %1").arg(nodeId);
        qWarning() << "Graph::executeNodeScript:" << error;
        emit errorOccurred(error);
        return QVariant();
    }

    const QVariant result = scripted->evaluate(context);

    // A failed run and a script that legitimately returned null both produce
    // an empty QVariant - the node's lastError() is the discriminator.
    if (!scripted->lastError().isEmpty()) {
        const QString error = QString("Script error in node %1: %2")
                                  .arg(nodeId.left(8), scripted->lastError());
        qWarning() << "Graph::executeNodeScript:" << error;
        emit errorOccurred(error);
    }

    return result;
}

QString Graph::getNodeScriptError(const QString& nodeId) const
{
    const ScriptedNode* scripted = asScripted(findNode(nodeId));
    return scripted ? scripted->lastError() : QString();
}

bool Graph::setNodePayload(const QString& nodeId, const QVariantMap& payload)
{
    ScriptedNode* scripted = asScripted(findNode(nodeId));
    if (!scripted) {
        qWarning() << "Graph::setNodePayload: node is not SCRIPT type" << nodeId;
        return false;
    }
    scripted->setPayload(payload);
    return true;
}

QVariantMap Graph::getNodePayload(const QString& nodeId) const
{
    const ScriptedNode* scripted = asScripted(findNode(nodeId));
    return scripted ? scripted->payload() : QVariantMap();
}

QVariantMap Graph::runSyntheticWork(const QVariantMap& request) const
{
    return SyntheticWork::run(request);
}

// ========== Grid & View Helpers ==========

void Graph::setSnapToGrid(bool on)
{
    m_scene->setSnapToGrid(on);
}

bool Graph::isSnapToGrid() const
{
    return m_scene->isSnapToGrid();
}

int Graph::gridSize() const
{
    return m_scene->gridSize();
}

QVariantMap Graph::snapPoint(qreal x, qreal y)
{
    if (!std::isfinite(x) || !std::isfinite(y)) {
        const QString error = QString("Graph::snapPoint: non-finite input (%1, %2)").arg(x).arg(y);
        qWarning() << error;
        emit errorOccurred(error);
        return QVariantMap();
    }

    const QPointF snapped = m_scene->snapPoint(QPointF(x, y));
    QVariantMap result;
    result["x"] = snapped.x();
    result["y"] = snapped.y();
    return result;
}

bool Graph::snapNode(const QString& nodeId)
{
    Node* node = findNode(nodeId);
    if (!node) {
        qWarning() << "Graph::snapNode: Node not found:" << nodeId;
        return false;
    }

    const QPointF currentPos = node->pos();
    const QPointF snapped = m_scene->snapPoint(currentPos);
    if (snapped == currentPos) {
        return true; // already on the grid
    }

    node->setPos(snapped);
    pushCommand(new MoveNodesCommand(m_scene,
        QVector<NodeMove>{NodeMove{parseUuid(nodeId), currentPos, snapped}}));
    emit nodeMoved(nodeId);
    return true;
}

int Graph::snapNodes()
{
    QVector<NodeMove> moves;
    for (auto it = m_scene->getNodes().constBegin(); it != m_scene->getNodes().constEnd(); ++it) {
        Node* node = it.value();
        if (!node) {
            continue;
        }
        const QPointF currentPos = node->pos();
        const QPointF snapped = m_scene->snapPoint(currentPos);
        if (snapped == currentPos) {
            continue;
        }
        node->setPos(snapped);
        moves.append(NodeMove{it.key(), currentPos, snapped});
        emit nodeMoved(it.key().toString());
    }

    if (!moves.isEmpty()) {
        pushCommand(new MoveNodesCommand(m_scene, moves));
    }
    return moves.size();
}

int Graph::alignGraph()
{
    const QHash<QUuid, Node*>& nodes = m_scene->getNodes();
    if (nodes.isEmpty()) {
        return 0;
    }

    // --- Layer assignment: longest path from source nodes (Kahn). ---
    QHash<QUuid, int> layer;
    QHash<QUuid, int> indegree;
    QHash<QUuid, QList<QUuid>> adjacency;
    for (auto it = nodes.constBegin(); it != nodes.constEnd(); ++it) {
        layer.insert(it.key(), 0);
        indegree.insert(it.key(), 0);
    }
    for (Edge* edge : m_scene->getEdges().values()) {
        if (!edge) {
            continue;
        }
        Node* from = edge->getFromNode();
        Node* to = edge->getToNode();
        if (!from || !to || from == to) {
            continue;
        }
        adjacency[from->getId()].append(to->getId());
        indegree[to->getId()] += 1;
    }

    QList<QUuid> queue;
    for (auto it = indegree.constBegin(); it != indegree.constEnd(); ++it) {
        if (it.value() == 0) {
            queue.append(it.key());
        }
    }
    while (!queue.isEmpty()) {
        const QUuid id = queue.takeFirst();
        for (const QUuid& next : adjacency.value(id)) {
            layer[next] = qMax(layer.value(next), layer.value(id) + 1);
            if (--indegree[next] == 0) {
                queue.append(next);
            }
        }
    }

    // Nodes still carrying indegree are in (or downstream of) a cycle: park
    // them in one extra column so the layout stays finite and deterministic.
    int maxProcessedLayer = 0;
    for (auto it = nodes.constBegin(); it != nodes.constEnd(); ++it) {
        if (indegree.value(it.key()) == 0) {
            maxProcessedLayer = qMax(maxProcessedLayer, layer.value(it.key()));
        }
    }

    QMap<int, QList<QUuid>> columns;
    for (auto it = nodes.constBegin(); it != nodes.constEnd(); ++it) {
        const QUuid id = it.key();
        const int column = (indegree.value(id) == 0)
                               ? layer.value(id)
                               : maxProcessedLayer + 1;
        columns[column].append(id);
    }

    // Deterministic row order within each column.
    int maxRows = 0;
    for (auto it = columns.begin(); it != columns.end(); ++it) {
        std::sort(it.value().begin(), it.value().end(),
                  [](const QUuid& a, const QUuid& b) {
                      return a.toString() < b.toString();
                  });
        maxRows = qMax(maxRows, it.value().size());
    }

    // Grid-aligned spacing/margins (must be multiples of Scene::gridSize()).
    constexpr qreal spacingX = 280.0;
    constexpr qreal spacingY = 160.0;
    constexpr qreal marginX = 120.0;
    constexpr qreal marginY = 80.0;

    QVector<NodeMove> moves;
    for (auto it = columns.constBegin(); it != columns.constEnd(); ++it) {
        const int column = it.key();
        const QList<QUuid>& ids = it.value();
        const qreal centeredOffset = (maxRows - ids.size()) * spacingY / 2.0;
        for (int row = 0; row < ids.size(); ++row) {
            Node* node = m_scene->getNode(ids.at(row));
            if (!node) {
                continue;
            }
            const QPointF target(marginX + column * spacingX,
                                 marginY + centeredOffset + row * spacingY);
            const QPointF oldPos = node->pos();
            if (target == oldPos) {
                continue;
            }
            node->setPos(target);
            node->updateConnectedEdges();
            moves.append(NodeMove{ids.at(row), oldPos, target});
            emit nodeMoved(ids.at(row).toString());
        }
    }

    if (!moves.isEmpty()) {
        pushCommand(new MoveNodesCommand(m_scene, moves));
    }

    qCDebug(ngVerbose) << "Graph::alignGraph: columns=" << columns.size()
                       << "moved=" << moves.size();
    return moves.size();
}

// ========== Edge Operations ==========

QString Graph::connectNodes(const QString& fromNodeId, int fromSocketIndex,
                             const QString& toNodeId, int toSocketIndex)
{
    Node* fromNode = findNode(fromNodeId);
    Node* toNode = findNode(toNodeId);

    if (!fromNode || !toNode) {
        QString error = "Cannot connect: node not found";
        qWarning() << "Graph::connectNodes:" << error;
        emit errorOccurred(error);
        return QString();
    }

    qCDebug(ngVerbose) << "Graph::connectNodes:" << fromNodeId << "[" << fromSocketIndex << "] ->"
             << toNodeId << "[" << toSocketIndex << "]";

    try {
        // Socket indices are per-node and GLOBAL: inputs occupy 0..n-1, then
        // outputs follow. This matches Socket::getIndex(), the index painted on
        // each socket, XML persistence, undo snapshots, and ghost-edge drags.
        // (TRANSFORM output = 1, MERGE output = 2, SPLIT outputs = 1,2.)
        Socket* fromSocket = fromNode->getSocketByIndex(fromSocketIndex);
        if (!fromSocket || fromSocket->getRole() != Socket::Output) {
            QString error = QString("Invalid output socket index: %1").arg(fromSocketIndex);
            qWarning() << "Graph::connectNodes:" << error;
            emit errorOccurred(error);
            return QString();
        }

        Socket* toSocket = toNode->getSocketByIndex(toSocketIndex);
        if (!toSocket || toSocket->getRole() != Socket::Input) {
            QString error = QString("Invalid input socket index: %1").arg(toSocketIndex);
            qWarning() << "Graph::connectNodes:" << error;
            emit errorOccurred(error);
            return QString();
        }

        // Mirror the factory's policy checks so the error surfaces here (and
        // so a pushed command cannot fail on its first redo).
        if (fromSocket->isConnected() || toSocket->isConnected()) {
            const QString error = "Cannot connect: socket already connected";
            qWarning() << "Graph::connectNodes:" << error;
            emit errorOccurred(error);
            return QString();
        }
        if (fromSocket->getParentNode() == toSocket->getParentNode()) {
            const QString error = "Cannot connect: self-loop not allowed";
            qWarning() << "Graph::connectNodes:" << error;
            emit errorOccurred(error);
            return QString();
        }

        if (m_undoStack) {
            auto* command = new ConnectEdgeCommand(m_scene, m_factory,
                                                   fromNode->getId(), fromSocketIndex,
                                                   toNode->getId(), toSocketIndex);
            pushCommand(command);
            const QUuid edgeUuid = command->edgeId();
            if (edgeUuid.isNull()) {
                const QString error = "Error connecting nodes: connection refused";
                qCritical() << "Graph::connectNodes:" << error;
                emit errorOccurred(error);
                return QString();
            }
            const QString edgeId = edgeUuid.toString();
            emit edgeCreated(edgeId);
            qCDebug(ngVerbose) << "Graph::connectNodes: Created edge" << edgeId;
            return edgeId;
        }

        // Use factory to create edge (use connectSockets for socket-based connection)
        Edge* edge = m_factory->connectSockets(fromSocket, toSocket);
        if (edge) {
            QString edgeId = edge->getId().toString();
            emit edgeCreated(edgeId);
            qCDebug(ngVerbose) << "Graph::connectNodes: Created edge" << edgeId;
            return edgeId;
        }
    } catch (const std::exception& e) {
        QString error = QString("Error connecting nodes: %1").arg(e.what());
        qCritical() << "Graph::connectNodes:" << error;
        emit errorOccurred(error);
    }

    return QString();
}

bool Graph::deleteEdge(const QString& edgeId)
{
    Edge* edge = findEdge(edgeId);
    if (!edge) {
        qWarning() << "Graph::deleteEdge: Edge not found:" << edgeId;
        return false;
    }

    qCDebug(ngVerbose) << "Graph::deleteEdge:" << edgeId;

    try {
        QUuid uuid = parseUuid(edgeId);
        if (m_undoStack) {
            pushCommand(new DeleteSelectionCommand(m_scene, m_factory,
                                                   QList<QUuid>{}, QList<QUuid>{uuid}));
        } else {
            m_scene->deleteEdge(uuid);
        }
        emit edgeDeleted(edgeId);
        return true;
    } catch (const std::exception& e) {
        QString error = QString("Error deleting edge: %1").arg(e.what());
        qCritical() << "Graph::deleteEdge:" << error;
        emit errorOccurred(error);
        return false;
    }
}

QVariantMap Graph::getEdgeData(const QString& edgeId) const
{
    Edge* edge = findEdge(edgeId);
    if (!edge) {
        return QVariantMap();
    }

    QVariantMap data;
    data["id"] = edgeId;

    // Sockets are identified by (node UUID + socket index)
    if (edge->getFromNode() && edge->getFromSocket()) {
        data["fromNode"] = edge->getFromNode()->getId().toString();
        data["fromSocketIndex"] = edge->getFromSocket()->getIndex();
    }
    if (edge->getToNode() && edge->getToSocket()) {
        data["toNode"] = edge->getToNode()->getId().toString();
        data["toSocketIndex"] = edge->getToSocket()->getIndex();
    }

    return data;
}

// ========== Graph Queries ==========

QVariantList Graph::getAllNodes() const
{
    QVariantList nodeList;
    const QHash<QUuid, Node*>& nodes = m_scene->getNodes();

    for (auto it = nodes.constBegin(); it != nodes.constEnd(); ++it) {
        nodeList.append(it.key().toString());
    }

    return nodeList;
}

QVariantList Graph::getAllEdges() const
{
    QVariantList edgeList;
    const QHash<QUuid, Edge*>& edges = m_scene->getEdges();

    for (auto it = edges.constBegin(); it != edges.constEnd(); ++it) {
        edgeList.append(it.key().toString());
    }

    return edgeList;
}

QVariantList Graph::getSelectedNodes() const
{
    QVariantList selectedList;
    const QHash<QUuid, Node*>& nodes = m_scene->getNodes();

    for (auto it = nodes.constBegin(); it != nodes.constEnd(); ++it) {
        if (it.value()->isSelected()) {
            selectedList.append(it.key().toString());
        }
    }

    return selectedList;
}

QVariantList Graph::getSelectedEdges() const
{
    QVariantList selectedList;
    const QHash<QUuid, Edge*>& edges = m_scene->getEdges();

    for (auto it = edges.constBegin(); it != edges.constEnd(); ++it) {
        if (it.value()->isSelected()) {
            selectedList.append(it.key().toString());
        }
    }

    return selectedList;
}

QVariantList Graph::getNodeEdges(const QString& nodeId) const
{
    Node* node = findNode(nodeId);
    if (!node) {
        return QVariantList();
    }

    // Since m_incidentEdges is private, we search through all edges
    QVariantList edgeList;
    const QHash<QUuid, Edge*>& allEdges = m_scene->getEdges();

    QUuid uuid = parseUuid(nodeId);
    for (auto it = allEdges.constBegin(); it != allEdges.constEnd(); ++it) {
        Edge* edge = it.value();
        if (edge->isConnectedToNode(uuid)) {
            edgeList.append(edge->getId().toString());
        }
    }

    return edgeList;
}

QVariantMap Graph::getGraphStats() const
{
    QVariantMap stats;
    stats["nodeCount"] = m_scene->getNodes().size();
    stats["edgeCount"] = m_scene->getEdges().size();

    // Count selected items
    int selectedNodes = 0;
    const QHash<QUuid, Node*>& nodes = m_scene->getNodes();
    for (auto it = nodes.constBegin(); it != nodes.constEnd(); ++it) {
        if (it.value()->isSelected()) {
            selectedNodes++;
        }
    }
    stats["selectedNodeCount"] = selectedNodes;

    return stats;
}

// ========== Batch Operations ==========

void Graph::beginBatch()
{
    // Forward to the REAL batch mechanism (GraphSubject): notifications are
    // muted during the batch and observers get a single onBatchEnded() flush
    // when the outermost batch ends. The undo macro starts lazily on the first
    // command push, so empty batches leave no history entry.
    GraphSubject::beginBatch();
    ++m_ownBatchDepth;
    qDebug() << "Graph: Batch started (GraphSubject)";
}

void Graph::endBatch()
{
    if (m_ownBatchDepth > 0) {
        --m_ownBatchDepth;
        if (m_ownBatchDepth == 0 && m_macroActive && m_undoStack) {
            m_undoStack->endMacro();
            m_macroActive = false;
        }
    }
    GraphSubject::endBatch();
    qDebug() << "Graph: Batch ended (GraphSubject)";
}

bool Graph::isBatchMode() const
{
    return GraphSubject::isInBatch();
}

// ========== Undo / Redo ==========

void Graph::setUndoStack(QUndoStack* stack)
{
    if (m_undoStack == stack) {
        return;
    }
    if (m_macroActive && m_undoStack) {
        m_undoStack->endMacro();
        m_macroActive = false;
    }
    m_undoStack = stack;
}

bool Graph::pushCommand(QUndoCommand* command)
{
    if (!m_undoStack || !command) {
        delete command;
        return false;
    }
    if (m_ownBatchDepth > 0 && !m_macroActive) {
        m_undoStack->beginMacro(QStringLiteral("Script batch"));
        m_macroActive = true;
    }
    m_undoStack->push(command);
    return true;
}

bool Graph::undo()
{
    if (!m_undoStack) {
        const QString msg = QStringLiteral("Undo unavailable: no undo stack attached");
        qWarning() << "Graph::undo:" << msg;
        emit errorOccurred(msg);
        return false;
    }
    if (ScriptedNode::isExecuting()) {
        const QString msg = QStringLiteral("Undo refused while a node script is executing");
        qWarning() << "Graph::undo:" << msg;
        emit errorOccurred(msg);
        return false;
    }
    if (m_ownBatchDepth > 0 || GraphSubject::isInBatch()) {
        const QString msg = QStringLiteral("Undo refused while a batch is open");
        qWarning() << "Graph::undo:" << msg;
        emit errorOccurred(msg);
        return false;
    }
    if (!m_undoStack->canUndo()) {
        return false;
    }
    m_undoStack->undo();
    return true;
}

bool Graph::redo()
{
    if (!m_undoStack) {
        const QString msg = QStringLiteral("Redo unavailable: no undo stack attached");
        qWarning() << "Graph::redo:" << msg;
        emit errorOccurred(msg);
        return false;
    }
    if (ScriptedNode::isExecuting()) {
        const QString msg = QStringLiteral("Redo refused while a node script is executing");
        qWarning() << "Graph::redo:" << msg;
        emit errorOccurred(msg);
        return false;
    }
    if (m_ownBatchDepth > 0 || GraphSubject::isInBatch()) {
        const QString msg = QStringLiteral("Redo refused while a batch is open");
        qWarning() << "Graph::redo:" << msg;
        emit errorOccurred(msg);
        return false;
    }
    if (!m_undoStack->canRedo()) {
        return false;
    }
    m_undoStack->redo();
    return true;
}

bool Graph::canUndo() const
{
    return m_undoStack && m_undoStack->canUndo();
}

bool Graph::canRedo() const
{
    return m_undoStack && m_undoStack->canRedo();
}

// ========== Graph-wide Operations ==========

void Graph::clearGraph()
{
    // Refuse while a node script runs (scripts reach this facade reentrantly,
    // and clearing mid-evaluation destroys the scene underneath the script).
    if (ScriptedNode::isExecuting()) {
        const QString msg = QStringLiteral("Graph::clearGraph refused while a node script is executing");
        qWarning() << msg;
        emit errorOccurred(msg);
        return;
    }

    qDebug() << "Graph::clearGraph";
    m_scene->clearGraph();
    emit graphCleared();
}

bool Graph::deleteSelection()
{
    // Refuse while a node script runs (scripts reach this facade reentrantly).
    if (ScriptedNode::isExecuting()) {
        const QString msg = QStringLiteral("Graph::deleteSelection refused while a node script is executing");
        qWarning() << msg;
        emit errorOccurred(msg);
        return false;
    }

    QVariantList selectedEdges = getSelectedEdges();
    QVariantList selectedNodes = getSelectedNodes();

    if (selectedEdges.isEmpty() && selectedNodes.isEmpty()) {
        qDebug() << "Graph::deleteSelection: nothing selected";
        return false;
    }

    qDebug() << "Graph::deleteSelection: deleting" << selectedNodes.size()
             << "nodes and" << selectedEdges.size() << "edges";

    beginBatch();

    bool deletedAnything = false;
    for (const QVariant& edgeVar : selectedEdges) {
        const QString edgeId = edgeVar.toString();
        if (!edgeId.isEmpty() && deleteEdge(edgeId)) {
            deletedAnything = true;
        }
    }

    for (const QVariant& nodeVar : selectedNodes) {
        const QString nodeId = nodeVar.toString();
        if (!nodeId.isEmpty() && deleteNode(nodeId)) {
            deletedAnything = true;
        }
    }

    endBatch();
    return deletedAnything;
}

bool Graph::saveToFile(const QString& filePath)
{
    qDebug() << "Graph::saveToFile:" << filePath;

    // Serialize through toXml() (the single serialization path), then write
    // the bytes with QFile: QFile handles native Unicode paths on Windows,
    // while libxml2's file API takes a narrow path and cannot open
    // non-ASCII filenames there.
    const QString xml = toXml();
    if (xml.isEmpty()) {
        qWarning() << "Graph::saveToFile: serialization failed for" << filePath;
        return false;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qWarning() << "Graph::saveToFile: cannot open" << filePath << file.errorString();
        return false;
    }

    const QByteArray bytes = xml.toUtf8();
    const qint64 written = file.write(bytes);
    const bool ok = (written == bytes.size()) && file.flush();
    file.close();

    if (!ok) {
        qWarning() << "Graph::saveToFile: write failed for" << filePath << file.errorString();
        return false;
    }

    emit graphSaved(filePath);
    return true;
}

bool Graph::loadFromFile(const QString& filePath)
{
    // Refuse while a node script runs: loading destroys the scene (and the
    // running node) underneath the script.
    if (ScriptedNode::isExecuting()) {
        const QString msg = QStringLiteral("Graph::loadFromFile refused while a node script is executing");
        qWarning() << msg;
        emit errorOccurred(msg);
        return false;
    }

    qDebug() << "Graph::loadFromFile:" << filePath;

    if (!m_factory) {
        qDebug() << "Graph::loadFromFile: No factory available";
        emit errorOccurred("Cannot load: No factory available");
        return false;
    }

    // The factory validates a complete replacement before committing it.
    // Leave the document and its observers untouched if loading fails.
    bool ok = m_factory->loadFromXmlFile(filePath);

    if (ok) {
        qDebug() << "Graph::loadFromFile: Successfully loaded" << filePath;
        emit graphLoaded();
    } else {
        qDebug() << "Graph::loadFromFile: Failed to load" << filePath;
        emit errorOccurred(QString("Failed to load file: %1").arg(filePath));
    }

    return ok;
}

QString Graph::toXml() const
{
    // Single serialization path: saveToFile() writes these bytes and
    // Window::saveGraph() delegates to the facade. (Previously saveGraph
    // duplicated this code and toXml returned a constant "<graph></graph>"
    // fake - the worst kind of API: silently wrong.)
    if (!m_scene) {
        return QString();
    }

    xmlDocPtr doc = xmlNewDoc(BAD_CAST "1.0");
    if (!doc) {
        return QString();
    }
    // Declare UTF-8 so dumps match the old xmlSaveFormatFileEnc output.
    doc->encoding = xmlStrdup(BAD_CAST "UTF-8");
    xmlNodePtr root = xmlNewNode(nullptr, BAD_CAST "graph");
    xmlDocSetRootElement(doc, root);
    xmlSetProp(root, BAD_CAST "version", BAD_CAST "1.0");

    for (Node* node : m_scene->getNodes().values()) {
        node->write(doc, root);
    }
    for (Edge* edge : m_scene->getEdges().values()) {
        edge->write(doc, root);
    }

    xmlChar* buffer = nullptr;
    int size = 0;
    xmlDocDumpFormatMemory(doc, &buffer, &size, 1);
    xmlFreeDoc(doc);

    if (!buffer) {
        return QString();
    }
    const QString xml = QString::fromUtf8(reinterpret_cast<const char*>(buffer), size);
    xmlFree(buffer);
    return xml;
}

// ========== Validation ==========

bool Graph::isValidNodeType(const QString& type) const
{
    return NodeTypeTemplates::hasNodeType(type);
}

QStringList Graph::getAvailableNodeTypes() const
{
    return NodeTypeTemplates::getAvailableTypes();
}

// ========== Script Engine ==========

QVariant Graph::evalScript(const QString& script)
{
    qDebug() << "Graph::evalScript:" << script.left(50) << "...";

    const ScriptResult result = m_scriptEngine.evaluate(script);

    if (!result.ok()) {
        QString error = QString("Script error: %1").arg(result.error);
        qCritical() << "Graph::evalScript:" << error;
        emit errorOccurred(error);
        return QVariant();
    }

    return result.value;
}

QVariant Graph::evalFile(const QString& filePath)
{
    qDebug() << "Graph::evalFile:" << filePath;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString error = QString("Cannot open file: %1").arg(filePath);
        qCritical() << "Graph::evalFile:" << error;
        emit errorOccurred(error);
        return QVariant();
    }

    QTextStream in(&file);
    QString script = in.readAll();
    file.close();

    return evalScript(script);
}

// ========== Internal Helpers ==========

Node* Graph::findNode(const QString& uuidStr) const
{
    QUuid uuid = parseUuid(uuidStr);
    if (uuid.isNull()) {
        return nullptr;
    }

    return m_scene->getNode(uuid);
}

Edge* Graph::findEdge(const QString& uuidStr) const
{
    QUuid uuid = parseUuid(uuidStr);
    if (uuid.isNull()) {
        return nullptr;
    }

    return m_scene->getEdge(uuid);
}

QUuid Graph::parseUuid(const QString& uuidStr) const
{
    QUuid uuid = QUuid::fromString(uuidStr);
    if (uuid.isNull()) {
        qWarning() << "Graph::parseUuid: Invalid UUID string:" << uuidStr;
    }
    return uuid;
}
