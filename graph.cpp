#include "graph.h"
#include "scene.h"
#include "graph_factory.h"
#include "node.h"
#include "edge.h"
#include "socket.h"
#include "node_templates.h"
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QGraphicsItem>

Graph::Graph(Scene* scene, GraphFactory* factory, QObject* parent)
    : QObject(parent)
    , m_scene(scene)
    , m_factory(factory)
    , m_jsEngine(new QJSEngine(this))
    , m_batchMode(false)
{
    Q_ASSERT(m_scene);
    Q_ASSERT(m_factory);

    qDebug() << "Graph: Facade initialized with JavaScript engine";

    // Initialize JavaScript engine and expose this Graph object
    initializeJavaScript();
}

Graph::~Graph()
{
    qDebug() << "Graph: Facade destroyed";
    // m_jsEngine is QObject child, will be deleted automatically
}

void Graph::initializeJavaScript()
{
    // Expose this Graph object to JavaScript as global "graph"
    QJSValue graphObj = m_jsEngine->newQObject(this);
    m_jsEngine->globalObject().setProperty("graph", graphObj);

    // Add console.log support
    QJSValue consoleObj = m_jsEngine->newObject();
    consoleObj.setProperty("log", m_jsEngine->evaluate(
        "(function() { return function(msg) { print('[JS] ' + msg); }; })()"
    ));
    m_jsEngine->globalObject().setProperty("console", consoleObj);

    qDebug() << "Graph: JavaScript engine initialized - 'graph' object available";
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

    qDebug() << "Graph::createNode:" << type << "at" << x << "," << y;

    try {
        // Use factory to create node
        Node* node = m_factory->createNode(type, QPointF(x, y));
        if (node) {
            QString uuid = node->getId().toString();
            emit nodeCreated(uuid);
            qDebug() << "Graph::createNode: Created node" << uuid;
            return uuid;
        }
    } catch (const std::exception& e) {
        QString error = QString("Error creating node: %1").arg(e.what());
        qCritical() << "Graph::createNode:" << error;
        emit errorOccurred(error);
    }

    return QString();
}

bool Graph::deleteNode(const QString& nodeId)
{
    Node* node = findNode(nodeId);
    if (!node) {
        qWarning() << "Graph::deleteNode: Node not found:" << nodeId;
        return false;
    }

    qDebug() << "Graph::deleteNode:" << nodeId;

    try {
        QUuid uuid = parseUuid(nodeId);
        m_scene->deleteNode(uuid);
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

    node->setPos(newPos);
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

    node->setPos(QPointF(x, y));
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

    return data;
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

    qDebug() << "Graph::connectNodes:" << fromNodeId << "[" << fromSocketIndex << "] ->"
             << toNodeId << "[" << toSocketIndex << "]";

    try {
        // Get sockets from nodes
        const QVector<Socket*>& outputSockets = fromNode->getOutputSockets();
        const QVector<Socket*>& inputSockets = toNode->getInputSockets();

        if (fromSocketIndex < 0 || fromSocketIndex >= outputSockets.size()) {
            QString error = QString("Invalid output socket index: %1").arg(fromSocketIndex);
            qWarning() << "Graph::connectNodes:" << error;
            emit errorOccurred(error);
            return QString();
        }

        if (toSocketIndex < 0 || toSocketIndex >= inputSockets.size()) {
            QString error = QString("Invalid input socket index: %1").arg(toSocketIndex);
            qWarning() << "Graph::connectNodes:" << error;
            emit errorOccurred(error);
            return QString();
        }

        Socket* fromSocket = outputSockets[fromSocketIndex];
        Socket* toSocket = inputSockets[toSocketIndex];

        // Use factory to create edge (use connectSockets for socket-based connection)
        Edge* edge = m_factory->connectSockets(fromSocket, toSocket);
        if (edge) {
            QString edgeId = edge->getId().toString();
            emit edgeCreated(edgeId);
            qDebug() << "Graph::connectNodes: Created edge" << edgeId;
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

    qDebug() << "Graph::deleteEdge:" << edgeId;

    try {
        QUuid uuid = parseUuid(edgeId);
        m_scene->deleteEdge(uuid);
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
    m_batchMode = true;
    qDebug() << "Graph: Batch mode started";
}

void Graph::endBatch()
{
    m_batchMode = false;
    qDebug() << "Graph: Batch mode ended";
    // TODO: Emit accumulated notifications
}

// ========== Graph-wide Operations ==========

void Graph::clearGraph()
{
    qDebug() << "Graph::clearGraph";
    m_scene->clearGraph();
    emit graphCleared();
}

bool Graph::saveToFile(const QString& filePath)
{
    qDebug() << "Graph::saveToFile:" << filePath;

    // TODO: Implement proper save via GraphFactory
    // For now, delegate to scene's XML autosave mechanism
    emit graphSaved(filePath);
    return true;
}

bool Graph::loadFromFile(const QString& filePath)
{
    qDebug() << "Graph::loadFromFile:" << filePath;

    if (!m_factory) {
        qDebug() << "Graph::loadFromFile: No factory available";
        emit errorOccurred("Cannot load: No factory available");
        return false;
    }

    // Use batch mode for efficient loading
    beginBatch();
    bool ok = m_factory->loadFromXmlFile(filePath);
    endBatch();

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
    // TODO: Implement XML export
    return QString("<graph></graph>");
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

// ========== JavaScript Engine ==========

QJSValue Graph::evalScript(const QString& script)
{
    qDebug() << "Graph::evalScript:" << script.left(50) << "...";

    QJSValue result = m_jsEngine->evaluate(script);

    if (result.isError()) {
        QString error = QString("JavaScript error at line %1: %2")
            .arg(result.property("lineNumber").toInt())
            .arg(result.toString());
        qCritical() << "Graph::evalScript:" << error;
        emit errorOccurred(error);
    }

    return result;
}

QJSValue Graph::evalFile(const QString& filePath)
{
    qDebug() << "Graph::evalFile:" << filePath;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString error = QString("Cannot open file: %1").arg(filePath);
        qCritical() << "Graph::evalFile:" << error;
        emit errorOccurred(error);
        return QJSValue();
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
