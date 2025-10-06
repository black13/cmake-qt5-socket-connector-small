#include "qgraph.h"
#include "scene.h"
#include "node.h"
#include "edge.h"
#include "socket.h"
#include "node_templates.h"
#include "graph_factory.h"
#include <QDebug>
#include <QUuid>
#include <libxml/tree.h>
#include <libxml/parser.h>

QGraph::QGraph(Scene* scene, QObject* parent)
    : QObject(parent)
    , scene_(scene)
    , m_isLoadingXml(false)
    , m_unresolvedEdges(0)
{
    Q_ASSERT(scene_ && "QGraph requires a typed Scene*");
}

QString QGraph::createNode(const QString& type, qreal x, qreal y)
{
    if (!scene_) {
        emit error("QGraph: Scene not initialized");
        return QString();
    }

    if (!NodeTypeTemplates::hasNodeType(type)) {
        emit error(QString("QGraph: Unknown node type: %1").arg(type));
        return QString();
    }

    try {
        // Generate UUID for new node
        QUuid nodeId = QUuid::createUuid();

        // Use template system to generate XML with correct socket counts
        QString xmlString = NodeTypeTemplates::generateNodeXml(type, QPointF(x, y), QVariantMap(), nodeId);
        if (xmlString.isEmpty()) {
            emit error(QString("QGraph: Failed to generate XML for node type: %1").arg(type));
            return QString();
        }

        qDebug() << "QGraph: Generated XML for" << type << ":" << xmlString;

        // Parse XML string into DOM
        xmlDocPtr doc = xmlParseMemory(xmlString.toUtf8().constData(), xmlString.toUtf8().size());
        if (!doc) {
            emit error(QString("QGraph: Failed to parse generated XML for type: %1").arg(type));
            return QString();
        }

        xmlNodePtr root = xmlDocGetRootElement(doc);
        if (!root) {
            xmlFreeDoc(doc);
            emit error(QString("QGraph: No root element in generated XML for type: %1").arg(type));
            return QString();
        }

        // Create node via GraphFactory (handles socket creation from XML attributes)
        // GraphFactory adds node to scene automatically
        GraphFactory factory(scene_, doc);
        Node* node = factory.createNodeFromXml(root);

        xmlFreeDoc(doc);

        if (!node) {
            emit error(QString("QGraph: GraphFactory failed to create node of type: %1").arg(type));
            return QString();
        }

        // Note: Node already added to scene by GraphFactory

        QString nodeIdStr = node->getId().toString();
        emit nodeCreated(nodeIdStr);

        qDebug() << "QGraph: Created node" << type << "at" << x << "," << y << "id:" << nodeIdStr.left(8);
        return nodeIdStr;

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
        scene_->removeNodeInternal(QUuid(nodeId));
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

        // CRITICAL: Resolve connections to set socket->edge references
        if (!edge->resolveConnections(scene_)) {
            qWarning() << "QGraph: Failed to resolve edge connections";
            scene_->removeEdgeInternal(edge->getId());  // This deletes the edge
            emit error("QGraph: Failed to resolve edge connections");
            return QString();
        }

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
        scene_->removeEdgeInternal(QUuid(edgeId));
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

    scene_->clearGraphInternal();
    emit graphCleared();

    qDebug() << "QGraph: Graph cleared";
}

bool QGraph::deleteSelected()
{
    if (!scene_) {
        emit error("QGraph: Scene not initialized");
        return false;
    }

    scene_->removeSelectedInternal();
    // Note: Individual node/edge deleted signals will be emitted by Scene
    qDebug() << "QGraph: Deleted selected items";
    return true;
}

void QGraph::saveXml(const QString& path)
{
    if (!scene_) {
        emit error("QGraph: Scene not initialized");
        return;
    }

    qDebug() << "QGraph: Saving XML to" << path;

    try {
        // Create XML document
        xmlDocPtr doc = xmlNewDoc(BAD_CAST "1.0");
        xmlNodePtr root = xmlNewNode(nullptr, BAD_CAST "graph");
        xmlDocSetRootElement(doc, root);
        xmlNewProp(root, BAD_CAST "version", BAD_CAST "1.0");

        // Serialize all nodes
        const QHash<QUuid, Node*>& nodes = scene_->getNodes();
        for (Node* node : nodes.values()) {
            node->write(doc, root);
        }

        // Serialize all edges
        const QHash<QUuid, Edge*>& edges = scene_->getEdges();
        for (Edge* edge : edges.values()) {
            edge->write(doc, root);
        }

        // Save to file
        int result = xmlSaveFileEnc(path.toUtf8().constData(), doc, "UTF-8");
        xmlFreeDoc(doc);

        if (result != -1) {
            qDebug() << "QGraph: XML saved successfully to" << path;
            emit xmlSaved(path);
        } else {
            emit error(QString("QGraph: Failed to save XML to %1").arg(path));
        }
    } catch (const std::exception& e) {
        emit error(QString("QGraph: Exception saving XML: %1").arg(e.what()));
    }
}


bool QGraph::loadXml(const QString& path)
{
    if (!scene_) {
        emit error("QGraph: Scene not initialized");
        return false;
    }

    qDebug() << "QGraph: Loading XML from" << path;
    emit xmlLoadStarted(path);
    m_isLoadingXml = true;

    // Parse file
    xmlDocPtr doc = xmlParseFile(path.toUtf8().constData());
    if (!doc) {
        m_isLoadingXml = false;
        emit error(QString("QGraph: Failed to parse XML file: %1").arg(path));
        emit xmlLoadComplete(path, false);
        return false;
    }

    // Get <graph> root
    xmlNodePtr root = xmlDocGetRootElement(doc);
    if (!root) {
        xmlFreeDoc(doc);
        m_isLoadingXml = false;
        emit error(QString("QGraph: XML has no root element: %1").arg(path));
        emit xmlLoadComplete(path, false);
        return false;
    }

    // Bulk update: suppress observer storms
    GraphSubject::beginBatch();

    bool ok = true;
    try {
        // Start clean
        scene_->clearGraphInternal();

        // Factory uses doc+scene to create nodes from XML
        GraphFactory factory(scene_, doc);

        // 1) Load NODES (support both <graph><nodes><node/></nodes> and flat <graph><node/>)
        auto loadNodeEl = [&](xmlNodePtr n) {
            if (n && xmlStrcmp(n->name, BAD_CAST "node") == 0) {
                if (!factory.createNodeFromXml(n)) {
                    qWarning() << "QGraph: Failed to create node from XML";
                    ok = false;
                }
            }
            };
        for (xmlNodePtr c = root->children; c; c = c->next) {
            if (xmlStrcmp(c->name, BAD_CAST "nodes") == 0) {
                for (xmlNodePtr n = c->children; n; n = n->next) loadNodeEl(n);
            }
            else {
                loadNodeEl(c);
            }
        }

        // 2) Load EDGES after nodes exist (support <connections><edge/> and flat <edge/>)
        QVector<Edge*> edgesToResolve;

        auto loadEdgeEl = [&](xmlNodePtr e) {
            if (e && xmlStrcmp(e->name, BAD_CAST "edge") == 0) {
                Edge* edge = new Edge(QUuid::createUuid(), QUuid(), QUuid());
                edge->read(e);               // stores node UUIDs + socket indices
                scene_->addEdge(edge);
                edgesToResolve.push_back(edge);
            }
            };
        for (xmlNodePtr c = root->children; c; c = c->next) {
            if (xmlStrcmp(c->name, BAD_CAST "connections") == 0) {
                for (xmlNodePtr e = c->children; e; e = e->next) loadEdgeEl(e);
            }
            else {
                loadEdgeEl(c);
            }
        }

        // 3) Resolve edge endpoints now that all nodes exist
        for (Edge* e : edgesToResolve) {
            if (!e->resolveConnections(scene_)) {
                qWarning() << "QGraph: Failed to resolve edge" << e->getId().toString(QUuid::WithoutBraces);
                ok = false;
            }
        }

    }
    catch (const std::exception& ex) {
        qCritical() << "QGraph: Exception during load:" << ex.what();
        ok = false;
    }
    catch (...) {
        qCritical() << "QGraph: Unknown exception during load";
        ok = false;
    }

    GraphSubject::endBatch();
    xmlFreeDoc(doc);

    m_isLoadingXml = false;

    if (ok) {
        updateUnresolvedEdgeCount();
        qDebug() << "QGraph: XML loaded successfully from" << path;
        emit xmlLoadComplete(path, true);
        if (isStable()) emit graphStabilized();
        return true;
    }
    else {
        emit error(QString("QGraph: Failed to fully load/resolve XML from %1").arg(path));
        emit xmlLoadComplete(path, false);
        return false;
    }
}

QString QGraph::getXmlString()
{
    if (!scene_) {
        return QString("<graph></graph>");
    }

    try {
        // Create XML document
        xmlDocPtr doc = xmlNewDoc(BAD_CAST "1.0");
        xmlNodePtr root = xmlNewNode(nullptr, BAD_CAST "graph");
        xmlDocSetRootElement(doc, root);
        xmlNewProp(root, BAD_CAST "version", BAD_CAST "1.0");

        // Serialize all nodes
        const QHash<QUuid, Node*>& nodes = scene_->getNodes();
        for (Node* node : nodes.values()) {
            node->write(doc, root);
        }

        // Serialize all edges
        const QHash<QUuid, Edge*>& edges = scene_->getEdges();
        for (Edge* edge : edges.values()) {
            edge->write(doc, root);
        }

        // Convert to string
        xmlChar* xmlBuff;
        int bufferSize;
        xmlDocDumpFormatMemory(doc, &xmlBuff, &bufferSize, 1);
        QString result = QString::fromUtf8((const char*)xmlBuff);
        xmlFree(xmlBuff);
        xmlFreeDoc(doc);

        return result;
    } catch (const std::exception& e) {
        emit error(QString("QGraph: Exception generating XML string: %1").arg(e.what()));
        return QString("<graph></graph>");
    }
}

QVariantMap QGraph::getStats()
{
    QVariantMap stats;
    if (!scene_) {
        stats["nodes"] = 0;
        stats["edges"] = 0;
        return stats;
    }

    stats["nodes"] = scene_->getNodes().size();
    stats["edges"] = scene_->getEdges().size();

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
    return NodeTypeTemplates::hasNodeType(type);
}

QStringList QGraph::getValidNodeTypes()
{
    return NodeTypeTemplates::getAvailableTypes();
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

    // Add connection information using Edge's public accessors
    Node* fromNode = edge->getFromNode();
    Node* toNode = edge->getToNode();
    Socket* fromSocket = edge->getFromSocket();
    Socket* toSocket = edge->getToSocket();

    if (fromNode) {
        map["fromNode"] = fromNode->getId().toString();
    }
    if (toNode) {
        map["toNode"] = toNode->getId().toString();
    }
    if (fromSocket) {
        map["fromSocket"] = fromSocket->getIndex();
    }
    if (toSocket) {
        map["toSocket"] = toSocket->getIndex();
    }

    return map;
}

// Load state tracking implementation

bool QGraph::isLoadingXml() const
{
    return m_isLoadingXml;
}

bool QGraph::isStable() const
{
    return !m_isLoadingXml && m_unresolvedEdges == 0;
}

int QGraph::getUnresolvedEdgeCount() const
{
    return m_unresolvedEdges;
}

void QGraph::updateUnresolvedEdgeCount()
{
    if (!scene_) {
        m_unresolvedEdges = 0;
        return;
    }

    // Count edges without valid socket connections
    int unresolved = 0;
    const QHash<QUuid, Edge*>& edges = scene_->getEdges();
    for (Edge* edge : edges.values()) {
        if (!edge) continue;

        Socket* fromSocket = edge->getFromSocket();
        Socket* toSocket = edge->getToSocket();

        // Edge is unresolved if either socket is null
        if (!fromSocket || !toSocket) {
            unresolved++;
        }
    }

    m_unresolvedEdges = unresolved;
    qDebug() << "QGraph: Unresolved edges:" << m_unresolvedEdges;
}