#include "graph_factory.h"
#include "node.h"
#include "scripted_node.h"
#include "socket.h"
#include "edge.h"
#include "scene.h"
// NodeRegistry removed - using template system directly
#include "graph_observer.h"
#include "node_templates.h"
#include <QDateTime>
#include <QElapsedTimer>
#include <QDebug>
#include <QFile>
#include <QSignalBlocker>
#include <cmath>
#include <memory>
#include <vector>
#include <libxml/tree.h>

GraphFactory::GraphFactory(Scene* scene, xmlDocPtr xmlDoc)
    : m_scene(scene)
    , m_xmlDocument(xmlDoc)
{
    qDebug() << "GraphFactory initialized with scene and XML document";
}

Node* GraphFactory::createNodeFromXml(xmlNodePtr xmlNode, bool addToScene)
{
    if (!xmlNode) {
        qWarning() << "GraphFactory::createNodeFromXml - null XML node";
        return nullptr;
    }
    
    // Get node type from XML
    QString nodeType = getXmlProperty(xmlNode, "type");
    if (nodeType.isEmpty()) {
        qWarning() << "GraphFactory::createNodeFromXml - missing type attribute";
        return nullptr;
    }
    
    // Validate node type against template system
    if (!NodeTypeTemplates::hasNodeType(nodeType)) {
        qCritical() << "Invalid node type:" << nodeType;
        qCritical() << "Available types:" << NodeTypeTemplates::getAvailableTypes();
        return nullptr;
    }

    Node* node = new ScriptedNode();
    node->setNodeType(nodeType);
    
    // Attach observer before reading XML - contract requirement
    node->setObserver(this);
    
    // Let the node read its XML and configure itself
    node->read(xmlNode);
    
    // Verify observer is still attached
    if (!node->hasObserver()) {
        qCritical() << "GraphFactory::createNodeFromXml - observer detached during read";
        delete node;
        return nullptr;
    }
    
    // Add to typed scene collection
    if (addToScene) {
        m_scene->addNode(node);
    }
    
    qDebug() << "GraphFactory: Created node from XML, type:" << nodeType 
             << "id:" << node->getId().toString(QUuid::WithoutBraces).left(8);
    
    return node;
}

Edge* GraphFactory::createEdgeFromXml(xmlNodePtr xmlEdge, bool addToScene)
{
    if (!xmlEdge) {
        qWarning() << "GraphFactory::createEdgeFromXml - null XML edge";
        return nullptr;
    }
    
    // Get edge properties from XML (new node+index format)
    QString edgeId = getXmlProperty(xmlEdge, "id");
    QString fromNode = getXmlProperty(xmlEdge, "fromNode");
    QString toNode = getXmlProperty(xmlEdge, "toNode");
    QString fromIndex = getXmlProperty(xmlEdge, "fromSocketIndex");
    QString toIndex = getXmlProperty(xmlEdge, "toSocketIndex");
    
    if (edgeId.isEmpty() || fromNode.isEmpty() || toNode.isEmpty() || 
        fromIndex.isEmpty() || toIndex.isEmpty()) {
        qWarning() << "GraphFactory::createEdgeFromXml - missing required node+index attributes";
        qWarning() << "Required: id, fromNode, toNode, fromSocketIndex, toSocketIndex";
        return nullptr;
    }
    
    // Create edge object with temporary UUIDs (will be resolved in read())
    Edge* edge = new Edge(QUuid::fromString(edgeId), QUuid(), QUuid());
    
    // Let the edge read its XML and configure itself
    edge->read(xmlEdge);
    
    // Add to typed scene collection
    if (addToScene) {
        m_scene->addEdge(edge);
    }
    
    qDebug() << "GraphFactory: Created edge from XML, id:" << edgeId.left(8)
             << "from node:" << fromNode.left(8) << "socket" << fromIndex
             << "to node:" << toNode.left(8) << "socket" << toIndex;
    
    return edge;
}

Node* GraphFactory::createNode(const QString& nodeType, const QPointF& position, int inputs, int outputs)
{
    #ifdef QT_DEBUG
    QElapsedTimer timer;
    timer.start();
    #endif
    
    qDebug() << "GraphFactory::createNode - UNIFIED XML-FIRST CREATION for type:" << nodeType;
    
    // Generate XML specification from template system (ignores inputs/outputs params - template has correct config)
    QString xmlSpecification = NodeTypeTemplates::generateNodeXml(nodeType, position);
    
    if (xmlSpecification.isEmpty()) {
        qCritical() << "GraphFactory::createNode - Failed to generate XML for node type:" << nodeType;
        return nullptr;
    }
    
    // Parse the XML specification
    xmlDocPtr tempDoc = xmlParseDoc(BAD_CAST xmlSpecification.toUtf8().constData());
    if (!tempDoc) {
        qCritical() << "GraphFactory::createNode - Failed to parse generated XML:" << xmlSpecification;
        return nullptr;
    }
    
    xmlNodePtr rootNode = xmlDocGetRootElement(tempDoc);
    if (!rootNode) {
        qCritical() << "GraphFactory::createNode - No root element in generated XML";
        xmlFreeDoc(tempDoc);
        return nullptr;
    }
    
    // Use existing working XML pipeline - the ONLY pathway now
    Node* node = createNodeFromXml(rootNode);
    
    // Cleanup temporary document
    xmlFreeDoc(tempDoc);
    
    if (node) {
        #ifdef QT_DEBUG
        qint64 elapsed = timer.elapsed();
        int sockets = node->getSocketCount();
        qDebug() << "createNode(type=" << nodeType << "):" << elapsed << "ms"
                 << "(uuid=" << node->getId().toString(QUuid::WithoutBraces).left(8) 
                 << "sockets=" << sockets << ")";
        #endif
    } else {
        qCritical() << "GraphFactory::createNode - FAILED to create node from XML";
    }
             
    return node;
}

Edge* GraphFactory::createEdge(Node* fromNode, int fromSocketIndex, Node* toNode, int toSocketIndex)
{
    if (!m_xmlDocument) {
        qCritical() << "GraphFactory::createEdge - no XML document";
        return nullptr;
    }
    
    if (!fromNode || !toNode) {
        qCritical() << "GraphFactory::createEdge - null node(s)";
        return nullptr;
    }
    
    // Get actual socket UUIDs by finding sockets by index
    Socket* fromSocket = fromNode->getSocketByIndex(fromSocketIndex);
    Socket* toSocket = toNode->getSocketByIndex(toSocketIndex);
    
    if (!fromSocket || !toSocket) {
        qCritical() << "GraphFactory::createEdge - invalid socket index(es)";
        return nullptr;
    }
    
    // Create XML edge using clean node+index format
    xmlNodePtr xmlEdge = createXmlEdgeNodeIndex(fromNode->getId(), fromSocketIndex, toNode->getId(), toSocketIndex);
    if (!xmlEdge) {
        qCritical() << "GraphFactory::createEdge - failed to create XML edge";
        return nullptr;
    }
    
    qDebug() << "GraphFactory: Created XML edge from node" << fromNode->getId().toString(QUuid::WithoutBraces).left(8) 
             << "socket" << fromSocketIndex << "to node" << toNode->getId().toString(QUuid::WithoutBraces).left(8)
             << "socket" << toSocketIndex;
    
    // Create object from XML
    Edge* edge = createEdgeFromXml(xmlEdge);
    if (edge) {
        // Immediately resolve connections for JavaScript-created edges
        if (m_scene && edge->resolveConnections(m_scene)) {
            qDebug() << "GraphFactory: Edge connections resolved successfully";
        } else {
            qWarning() << "GraphFactory: Failed to resolve edge connections";
        }
    }
    return edge;
}

Edge* GraphFactory::connectSockets(Socket* fromSocket, Socket* toSocket)
{
    if (!fromSocket || !toSocket) {
        qWarning() << "GraphFactory::connectSockets: nullptr socket(s)";
        return nullptr;
    }

    // Direction check (Output -> Input only)
    if (fromSocket->getRole() != Socket::Output || toSocket->getRole() != Socket::Input) {
        qWarning() << "GraphFactory::connectSockets: invalid roles, expected Output->Input";
        return nullptr;
    }

    // Enforce one-edge-per-socket policy
    if (fromSocket->isConnected() || toSocket->isConnected()) {
        qWarning() << "GraphFactory::connectSockets: socket already connected"
                   << " from:" << fromSocket->getIndex()
                   << " to:"   << toSocket->getIndex();
        return nullptr;
    }

    // Block self-loops
    if (fromSocket->getParentNode() == toSocket->getParentNode()) {
        qWarning() << "GraphFactory::connectSockets: self-loop disallowed by policy";
        return nullptr;
    }
    
    if (!m_xmlDocument) {
        qCritical() << "GraphFactory::connectSockets - no XML document";
        return nullptr;
    }
    
    // Get parent nodes for clean node+index approach
    Node* fromNode = fromSocket->getParentNode();
    Node* toNode = toSocket->getParentNode();
    
    if (!fromNode || !toNode) {
        qCritical() << "GraphFactory::connectSockets - sockets have no parent nodes";
        return nullptr;
    }
    
    // Optimized: Create edge directly in memory, serialize later
    QUuid edgeId = QUuid::createUuid();
    Edge* edge = new Edge(edgeId, QUuid(), QUuid());
    
    // Set connection data directly without XML round-trip
    edge->setConnectionData(fromNode->getId().toString(QUuid::WithoutBraces), toNode->getId().toString(QUuid::WithoutBraces),
                           fromSocket->getIndex(), toSocket->getIndex());
    
    // Resolve connections immediately since we have the sockets
    if (!edge->setResolvedSockets(fromSocket, toSocket)) {
        qCritical() << "GraphFactory::connectSockets - edge rejected by socket validation, aborting";
        delete edge;
        return nullptr;
    }

    // Add to scene (connectSockets is for runtime creation, always adds)
    m_scene->addEdge(edge);
    
    // DESIGN DECISION: Treat m_xmlDocument as scratch pad, not live sync
    // Runtime edges are created in-memory only. Full serialization happens via:
    // 1. Manual save (Window::saveGraph() - creates new XML doc from scene)  
    // 2. Autosave (XmlAutosaveObserver - also serializes from scene)
    // This prevents XML/scene drift without expensive live sync overhead
    xmlNodePtr xmlEdge = createXmlEdgeNodeIndex(fromNode->getId(), fromSocket->getIndex(), 
                                                toNode->getId(), toSocket->getIndex());
    if (!xmlEdge) {
        qWarning() << "GraphFactory::connectSockets - XML serialization failed (edge still created)";
        qWarning() << "Note: Full graph serialization will occur during save/autosave";
    }
    
    // Connect sockets atomically
    fromSocket->setConnectedEdge(edge);
    toSocket->setConnectedEdge(edge);
    
    qDebug() << "GraphFactory: Atomically connected sockets" 
             << "index" << fromSocket->getIndex()
             << "to index" << toSocket->getIndex();
    
    return edge;
}

Edge* GraphFactory::connectByIds(const QUuid& fromNodeId, int fromSocketIndex,
                                 const QUuid& toNodeId,   int toSocketIndex)
{
    if (!m_scene) {
        qCritical() << "GraphFactory::connectByIds - scene is null";
        return nullptr;
    }
    Node* fromNode = m_scene->getNode(fromNodeId);
    Node* toNode   = m_scene->getNode(toNodeId);
    if (!fromNode || !toNode) {
        qCritical() << "GraphFactory::connectByIds - invalid node id(s)";
        return nullptr;
    }
    Socket* fromSocket = fromNode->getSocketByIndex(fromSocketIndex);
    Socket* toSocket   = toNode->getSocketByIndex(toSocketIndex);
    if (!fromSocket || !toSocket) {
        qCritical() << "GraphFactory::connectByIds - invalid socket index(es)";
        return nullptr;
    }
    return connectSockets(fromSocket, toSocket); // unify on in-memory creation
}

bool GraphFactory::loadFromXmlFile(const QString& filePath)
{
    if (!m_scene) {
        return false;
    }

    // QFile handles native Unicode paths on Windows; parse the bytes with
    // libxml2 rather than handing a UTF-8 filename to its narrow file API.
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Cannot open graph:" << filePath << file.errorString();
        return false;
    }
    const QByteArray data = file.readAll();
    if (file.error() != QFileDevice::NoError) {
        qWarning() << "Cannot read graph:" << filePath << file.errorString();
        return false;
    }
    using XmlDocument = std::unique_ptr<xmlDoc, decltype(&xmlFreeDoc)>;
    XmlDocument doc(xmlReadMemory(data.constData(), data.size(), nullptr, nullptr,
                                  XML_PARSE_NONET), &xmlFreeDoc);
    if (!doc) {
        qWarning() << "Cannot parse graph:" << filePath;
        return false;
    }
    xmlNodePtr root = xmlDocGetRootElement(doc.get());
    if (!root || xmlStrcmp(root->name, BAD_CAST "graph") != 0 ||
        doc->intSubset || doc->extSubset) {
        qWarning() << "Expected a graph document without a DTD:" << filePath;
        return false;
    }

    QVector<xmlNodePtr> nodeElements;
    QVector<xmlNodePtr> edgeElements;
    // Manual saves use direct children; autosaves use <nodes>/<connections>.
    for (xmlNodePtr child = root->children; child; child = child->next) {
        if (child->type != XML_ELEMENT_NODE) {
            continue;
        }
        if (xmlStrEqual(child->name, BAD_CAST "node")) {
            nodeElements.append(child);
        } else if (xmlStrEqual(child->name, BAD_CAST "edge")) {
            edgeElements.append(child);
        } else if (xmlStrEqual(child->name, BAD_CAST "nodes") ||
                   xmlStrEqual(child->name, BAD_CAST "connections")) {
            const bool nodes = xmlStrEqual(child->name, BAD_CAST "nodes");
            for (xmlNodePtr item = child->children; item; item = item->next) {
                if (item->type == XML_ELEMENT_NODE &&
                    xmlStrEqual(item->name, nodes ? BAD_CAST "node" : BAD_CAST "edge")) {
                    (nodes ? nodeElements : edgeElements).append(item);
                }
            }
        }
    }

    // Own the proposed graph independently until every connection is valid.
    // Declare edges last so rollback disconnects them before deleting nodes.
    std::vector<std::unique_ptr<Node>> nodes;
    std::vector<std::unique_ptr<Edge>> edges;
    QHash<QUuid, Node*> nodesById;
    QSet<QUuid> edgeIds;
    for (xmlNodePtr element : nodeElements) {
        const QUuid id(getXmlProperty(element, "id"));
        bool inputsOk = false;
        bool outputsOk = false;
        const int inputs = getXmlProperty(element, "inputs").toInt(&inputsOk);
        const int outputs = getXmlProperty(element, "outputs").toInt(&outputsOk);
        // Bound allocations and the arithmetic used to lay out sockets.
        constexpr int maxSockets = 4096;
        if (id.isNull() || nodesById.contains(id) ||
            !NodeTypeTemplates::hasNodeType(getXmlProperty(element, "type")) ||
            !inputsOk || !outputsOk || inputs < 0 || outputs < 0 ||
            inputs > maxSockets || outputs > maxSockets - inputs) {
            qWarning() << "Invalid or duplicate node in graph:" << filePath;
            return false;
        }
        for (const char* coordinate : {"x", "y"}) {
            if (xmlHasProp(element, BAD_CAST coordinate)) {
                bool ok = false;
                const double value = getXmlProperty(element, coordinate).toDouble(&ok);
                if (!ok || !std::isfinite(value)) {
                    qWarning() << "Invalid node coordinate in graph:" << filePath;
                    return false;
                }
            }
        }
        std::unique_ptr<Node> node(createNodeFromXml(element, false));
        if (!node) {
            return false;
        }
        nodesById.insert(id, node.get());
        nodes.push_back(std::move(node));
    }

    for (xmlNodePtr element : edgeElements) {
        const QUuid id(getXmlProperty(element, "id"));
        Node* from = nodesById.value(QUuid(getXmlProperty(element, "fromNode")), nullptr);
        Node* to = nodesById.value(QUuid(getXmlProperty(element, "toNode")), nullptr);
        bool fromOk = false;
        bool toOk = false;
        const int fromIndex = getXmlProperty(element, "fromSocketIndex").toInt(&fromOk);
        const int toIndex = getXmlProperty(element, "toSocketIndex").toInt(&toOk);
        if (id.isNull() || edgeIds.contains(id) || !from || !to || from == to ||
            !fromOk || !toOk || fromIndex < 0 || toIndex < 0 ||
            fromIndex >= from->getAllSockets().size() ||
            toIndex >= to->getAllSockets().size()) {
            qWarning() << "Invalid or duplicate edge in graph:" << filePath;
            return false;
        }
        std::unique_ptr<Edge> edge(createEdgeFromXml(element, false));
        if (!edge || !edge->setResolvedSockets(from->getAllSockets().at(fromIndex),
                                               to->getAllSockets().at(toIndex))) {
            qWarning() << "Invalid socket connection in graph:" << filePath;
            return false;
        }
        edgeIds.insert(id);
        edges.push_back(std::move(edge));
    }
    // Commit once. Failed loads never clear selection, ghost drags, undo
    // history, or autosave state, and never emit a batch-completion event.
    {
        GraphSubject::BatchGuard batchGuard;
        QSignalBlocker signalBlocker(m_scene);
        m_scene->clearGraph();
        for (auto& node : nodes) {
            m_scene->addNode(node.release());
        }
        for (auto& edge : edges) {
            m_scene->addEdge(edge.release());
        }
    }
    emit m_scene->sceneChanged();
    return true;
}

QString GraphFactory::getXmlProperty(xmlNodePtr node, const QString& name)
{
    if (!node) return QString();
    
    xmlChar* prop = xmlGetProp(node, BAD_CAST name.toUtf8().constData());
    if (!prop) return QString();
    
    QString result = QString::fromUtf8((char*)prop);
    xmlFree(prop);
    return result;
}

xmlNodePtr GraphFactory::createXmlNode(const QString& nodeType, const QPointF& position, int inputs, int outputs)
{
    xmlNodePtr nodesElement = getNodesElement();
    if (!nodesElement) {
        qCritical() << "GraphFactory::createXmlNode - no nodes element in XML";
        return nullptr;
    }
    
    // Create new node element
    xmlNodePtr nodeElement = xmlNewChild(nodesElement, nullptr, BAD_CAST "node", nullptr);
    
    // Set attributes including socket configuration
    QUuid nodeId = QUuid::createUuid();
    xmlSetProp(nodeElement, BAD_CAST "id", BAD_CAST nodeId.toString(QUuid::WithoutBraces).toUtf8().constData());
    xmlSetProp(nodeElement, BAD_CAST "type", BAD_CAST nodeType.toUtf8().constData());
    xmlSetProp(nodeElement, BAD_CAST "x", BAD_CAST QString::number(position.x()).toUtf8().constData());
    xmlSetProp(nodeElement, BAD_CAST "y", BAD_CAST QString::number(position.y()).toUtf8().constData());
    xmlSetProp(nodeElement, BAD_CAST "inputs", BAD_CAST QString::number(inputs).toUtf8().constData());
    xmlSetProp(nodeElement, BAD_CAST "outputs", BAD_CAST QString::number(outputs).toUtf8().constData());
    
    qDebug() << "GraphFactory: Created XML node, type:" << nodeType << "id:" << nodeId.toString(QUuid::WithoutBraces).left(8)
             << "inputs:" << inputs << "outputs:" << outputs;
    
    return nodeElement;
}

xmlNodePtr GraphFactory::createXmlEdgeNodeIndex(const QUuid& fromNodeId, int fromSocketIndex, const QUuid& toNodeId, int toSocketIndex)
{
    xmlNodePtr edgesElement = getEdgesElement();
    if (!edgesElement) {
        qCritical() << "GraphFactory::createXmlEdgeNodeIndex - no edges element in XML";
        return nullptr;
    }
    
    // Create new edge element
    xmlNodePtr edgeElement = xmlNewChild(edgesElement, nullptr, BAD_CAST "edge", nullptr);
    
    // Set attributes using clean node+index format
    QUuid edgeId = QUuid::createUuid();
    xmlSetProp(edgeElement, BAD_CAST "id", BAD_CAST edgeId.toString(QUuid::WithoutBraces).toUtf8().constData());
    xmlSetProp(edgeElement, BAD_CAST "fromNode", BAD_CAST fromNodeId.toString(QUuid::WithoutBraces).toUtf8().constData());
    xmlSetProp(edgeElement, BAD_CAST "toNode", BAD_CAST toNodeId.toString(QUuid::WithoutBraces).toUtf8().constData());
    xmlSetProp(edgeElement, BAD_CAST "fromSocketIndex", BAD_CAST QString::number(fromSocketIndex).toUtf8().constData());
    xmlSetProp(edgeElement, BAD_CAST "toSocketIndex", BAD_CAST QString::number(toSocketIndex).toUtf8().constData());
    
    qDebug() << "GraphFactory: Created XML edge, id:" << edgeId.toString(QUuid::WithoutBraces).left(8)
             << "from node:" << fromNodeId.toString(QUuid::WithoutBraces).left(8) << "socket" << fromSocketIndex
             << "to node:" << toNodeId.toString(QUuid::WithoutBraces).left(8) << "socket" << toSocketIndex;
    
    return edgeElement;
}

xmlNodePtr GraphFactory::getNodesElement()
{
    if (!m_xmlDocument) return nullptr;
    
    xmlNodePtr root = xmlDocGetRootElement(m_xmlDocument);
    if (!root) return nullptr;
    
    // Find or create <nodes> element
    for (xmlNodePtr child = root->children; child; child = child->next) {
        if (xmlStrcmp(child->name, BAD_CAST "nodes") == 0) {
            return child;
        }
    }
    
    // Create nodes element if it doesn't exist
    xmlNodePtr nodesElement = xmlNewChild(root, nullptr, BAD_CAST "nodes", nullptr);
    return nodesElement;
}

xmlNodePtr GraphFactory::getEdgesElement()
{
    if (!m_xmlDocument) return nullptr;
    
    xmlNodePtr root = xmlDocGetRootElement(m_xmlDocument);
    if (!root) return nullptr;
    
    // Find or create <edges> or <connections> element
    for (xmlNodePtr child = root->children; child; child = child->next) {
        if (xmlStrcmp(child->name, BAD_CAST "edges") == 0 ||
            xmlStrcmp(child->name, BAD_CAST "connections") == 0) {
            return child;
        }
    }
    
    // Create connections element if it doesn't exist
    xmlNodePtr connectionsElement = xmlNewChild(root, nullptr, BAD_CAST "connections", nullptr);
    return connectionsElement;
}

// Clean design: socket resolution handled by edges internally - method removed

// No socket resolver needed in clean design - edges use direct socket pointers

bool GraphFactory::validateGraphIntegrity() const
{
    if (!m_scene) {
        qCritical() << "GraphFactory::validateGraphIntegrity - no scene";
        return false;
    }

    bool valid = true;

    // Validate all nodes have UUIDs and observers
    for (Node* node : m_scene->getNodes().values()) {
        if (!node) {
            qCritical() << "Validation: null node in scene";
            valid = false;
            continue;
        }
        
        if (node->getId().isNull()) {
            qCritical() << "Validation: node without UUID";
            valid = false;
        }
        
        // Check sockets belong to parent node - use typed accessor instead of qgraphicsitem_cast
        for (Socket* socket : node->allSockets()) {
            if (socket->getParentNode() != node) {
                qCritical() << "Validation: socket parent mismatch";
                valid = false;
            }
        }
    }
    
    // Validate all edges have valid socket connections
    for (Edge* edge : m_scene->getEdges().values()) {
        if (!edge) {
            qCritical() << "Validation: null edge in scene";
            valid = false;
            continue;
        }
        
        if (edge->getId().isNull()) {
            qCritical() << "Validation: edge without UUID";
            valid = false;
        }
        
        // Clean design: edges should have been resolved during loading
        // Skip validation - edges were validated during resolveConnections() call
    }
    
    // Validate scene count matches typed collections
    int sceneItems = m_scene->items().size();
    int typedItems = m_scene->getNodes().size() + m_scene->getEdges().size();

    // Account for sockets as children
    int socketCount = 0;
    for (Node* node : m_scene->getNodes().values()) {
        socketCount += node->getSocketCount();
    }
    typedItems += socketCount;
    
    if (sceneItems != typedItems) {
        qWarning() << "Validation: scene item count mismatch - scene:" << sceneItems 
                   << "typed:" << typedItems;
        // This is a warning, not an error - some items might be temporary
    }
    
    if (valid) {
        // qDebug() << "Graph integrity validation passed";
    }
    
    return valid;
}

Socket* GraphFactory::createSocket(Socket::Role role, Node* parentNode, int index)
{
    if (!parentNode) {
        qCritical() << "GraphFactory::createSocket - null parent node";
        return nullptr;
    }
    
    // Create socket with factory access
    Socket* socket = new Socket(role, parentNode, index);
    if (!socket) {
        qCritical() << "GraphFactory::createSocket - failed to create socket";
        return nullptr;
    }
    
    qDebug() << "GraphFactory: Created socket" << (role == Socket::Input ? "Input" : "Output") 
             << "index" << index << "for node" << parentNode->getId().toString(QUuid::WithoutBraces).left(8);
    
    return socket;
}
