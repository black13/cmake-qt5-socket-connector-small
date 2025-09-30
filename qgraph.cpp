#include "qgraph.h"
#include "scene.h"
#include "node.h"
#include "edge.h"
#include "socket.h"
#include "node_registry.h"
#include <QDebug>
#include <QUuid>

QGraph::QGraph(QGraphicsScene* scene, QObject* parent)
    : QObject(parent)
    , scene_(qobject_cast<Scene*>(scene))
{
    if (!scene_) {
        qCritical() << "QGraph: Scene pointer is null or not a Scene instance";
    }
}

QString QGraph::createNode(const QString& type, qreal x, qreal y)
{
    if (!scene_) {
        emit error("QGraph: Scene not initialized");
        return QString();
    }

    if (!isValidNodeType(type)) {
        emit error(QString("QGraph: Invalid node type: %1").arg(type));
        return QString();
    }

    try {
        // Create node through registry
        Node* node = NodeRegistry::instance().createNode(type);
        if (!node) {
            emit error(QString("QGraph: Failed to create node of type: %1").arg(type));
            return QString();
        }

        // Set position
        node->setPos(x, y);

        // Add to scene (scene handles visual registration)
        scene_->addNode(node);

        QString nodeId = node->getId().toString();
        emit nodeCreated(nodeId);

        qDebug() << "QGraph: Created node" << type << "at" << x << "," << y << "id:" << nodeId.left(8);
        return nodeId;

    } catch (const std::exception& e) {
        emit error(QString("QGraph: Exception creating node: %1").arg(e.what()));
        return QString();
    }
}

bool QGraph::deleteNode(const QString& nodeId)
{
    if (!scene_) {
        emit error("QGraph: Scene not initialized");
        return false;
    }

    Node* node = findNode(nodeId);
    if (!node) {
        emit error(QString("QGraph: Node not found: %1").arg(nodeId));
        return false;
    }

    try {
        // Scene handles deletion and registry cleanup
        scene_->deleteNode(QUuid(nodeId));
        emit nodeDeleted(nodeId);

        qDebug() << "QGraph: Deleted node" << nodeId.left(8);
        return true;

    } catch (const std::exception& e) {
        emit error(QString("QGraph: Exception deleting node: %1").arg(e.what()));
        return false;
    }
}

bool QGraph::moveNode(const QString& nodeId, qreal dx, qreal dy)
{
    Node* node = findNode(nodeId);
    if (!node) {
        emit error(QString("QGraph: Node not found: %1").arg(nodeId));
        return false;
    }

    QPointF currentPos = node->pos();
    node->setPos(currentPos.x() + dx, currentPos.y() + dy);

    return true;
}

QVariantMap QGraph::getNode(const QString& nodeId)
{
    Node* node = findNode(nodeId);
    if (!node) {
        return QVariantMap();
    }
    return nodeToVariant(node);
}

QVariantList QGraph::getNodes()
{
    QVariantList result;
    if (!scene_) return result;

    const QHash<QUuid, Node*>& nodes = scene_->getNodes();
    for (Node* node : nodes.values()) {
        result.append(nodeToVariant(node));
    }

    return result;
}

QString QGraph::connect(const QString& fromNodeId, int fromIdx,
                        const QString& toNodeId, int toIdx)
{
    if (!scene_) {
        emit error("QGraph: Scene not initialized");
        return QString();
    }

    Node* fromNode = findNode(fromNodeId);
    Node* toNode = findNode(toNodeId);

    if (!fromNode || !toNode) {
        emit error("QGraph: Source or target node not found");
        return QString();
    }

    Socket* fromSocket = fromNode->getSocketByIndex(fromIdx);
    Socket* toSocket = toNode->getSocketByIndex(toIdx);

    if (!fromSocket || !toSocket) {
        emit error("QGraph: Socket not found at specified index");
        return QString();
    }

    try {
        // Create edge with empty socket UUIDs (Edge uses node IDs + socket indices)
        Edge* edge = new Edge(QUuid::createUuid(), QUuid(), QUuid());

        // Set connection data (node IDs + socket indices)
        edge->setConnectionData(fromNodeId, toNodeId, fromIdx, toIdx);

        // Add to scene
        scene_->addEdge(edge);

        QString edgeId = edge->getId().toString();
        emit edgeConnected(edgeId);

        qDebug() << "QGraph: Connected" << fromNodeId.left(8) << "to" << toNodeId.left(8);
        return edgeId;

    } catch (const std::exception& e) {
        emit error(QString("QGraph: Exception connecting nodes: %1").arg(e.what()));
        return QString();
    }
}

bool QGraph::deleteEdge(const QString& edgeId)
{
    if (!scene_) {
        emit error("QGraph: Scene not initialized");
        return false;
    }

    Edge* edge = findEdge(edgeId);
    if (!edge) {
        emit error(QString("QGraph: Edge not found: %1").arg(edgeId));
        return false;
    }

    try {
        scene_->deleteEdge(QUuid(edgeId));
        emit edgeDeleted(edgeId);

        qDebug() << "QGraph: Deleted edge" << edgeId.left(8);
        return true;

    } catch (const std::exception& e) {
        emit error(QString("QGraph: Exception deleting edge: %1").arg(e.what()));
        return false;
    }
}

QVariantList QGraph::getEdges()
{
    QVariantList result;
    if (!scene_) return result;

    const QHash<QUuid, Edge*>& edges = scene_->getEdges();
    for (Edge* edge : edges.values()) {
        result.append(edgeToVariant(edge));
    }

    return result;
}

void QGraph::clear()
{
    if (!scene_) {
        emit error("QGraph: Scene not initialized");
        return;
    }

    scene_->clearGraph();
    emit graphCleared();

    qDebug() << "QGraph: Graph cleared";
}

void QGraph::saveXml(const QString& path)
{
    // TODO: Implement XML saving
    // This will coordinate with Scene to serialize the graph
    emit error("QGraph::saveXml not yet implemented");
}

void QGraph::loadXml(const QString& path)
{
    // TODO: Implement XML loading
    // This will coordinate with Scene to deserialize the graph
    emit error("QGraph::loadXml not yet implemented");
}

QString QGraph::getXmlString()
{
    // TODO: Implement XML string generation
    return QString("<graph></graph>");
}

QVariantMap QGraph::getStats()
{
    QVariantMap stats;
    if (!scene_) return stats;

    stats["nodeCount"] = scene_->getNodes().size();
    stats["edgeCount"] = scene_->getEdges().size();

    return stats;
}

void QGraph::beginPreview(Socket* from, const QPointF& start)
{
    if (scene_) {
        scene_->startGhostEdge(from, start);
    }
}

void QGraph::updatePreview(const QPointF& pos)
{
    if (scene_) {
        scene_->updateGhostEdge(pos);
    }
}

void QGraph::endPreview(Socket* to)
{
    if (scene_) {
        scene_->finishGhostEdge(to);
    }
}

bool QGraph::isValidNodeType(const QString& type)
{
    return NodeRegistry::instance().isRegistered(type);
}

QStringList QGraph::getValidNodeTypes()
{
    return NodeRegistry::instance().getRegisteredTypes();
}

// Helper methods

Node* QGraph::findNode(const QString& uuid)
{
    if (!scene_) return nullptr;
    return scene_->getNode(QUuid(uuid));
}

Edge* QGraph::findEdge(const QString& uuid)
{
    if (!scene_) return nullptr;
    return scene_->getEdge(QUuid(uuid));
}

QVariantMap QGraph::nodeToVariant(Node* node)
{
    QVariantMap map;
    if (!node) return map;

    map["id"] = node->getId().toString();
    map["type"] = node->getNodeType();
    map["x"] = node->pos().x();
    map["y"] = node->pos().y();
    map["socketCount"] = node->getSocketCount();

    return map;
}

QVariantMap QGraph::edgeToVariant(Edge* edge)
{
    QVariantMap map;
    if (!edge) return map;

    map["id"] = edge->getId().toString();
    // TODO: Add fromNode, toNode, fromSocket, toSocket when Edge provides access

    return map;
}