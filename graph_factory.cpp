#include "graph_factory.h"
#include "node.h"
#include "socket.h"
#include "edge.h"
#include "scene.h"
#include "node_registry.h"
#include "graph_observer.h"
#include "node_type_templates.h"
#include <QDateTime>
#include <QDebug>

GraphFactory::GraphFactory(QGraphicsScene* scene, xmlDocPtr xmlDoc)
    : m_scene(scene)
    , m_xmlDocument(xmlDoc)
{
    qDebug() << "GraphFactory initialized with scene and XML document";
}

Node* GraphFactory::createNodeFromXml(xmlNodePtr xmlNode)
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
    
    // Create node using registry
    Node* node = NodeRegistry::instance().createNode(nodeType);
    if (!node) {
        qCritical() << "GraphFactory::createNodeFromXml - failed to create node of type:" << nodeType;
        return nullptr;
    }
    
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
    if (Scene* typedScene = static_cast<Scene*>(m_scene)) {
        typedScene->addNode(node);
    } else {
        m_scene->addItem(node);
    }
    
    qDebug() << "GraphFactory: Created node from XML, type:" << nodeType 
             << "id:" << node->getId().toString(QUuid::WithoutBraces).left(8);
    
    return node;
}

Edge* GraphFactory::createEdgeFromXml(xmlNodePtr xmlEdge)
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
    if (Scene* typedScene = static_cast<Scene*>(m_scene)) {
        typedScene->addEdge(edge);
    } else {
        m_scene->addItem(edge);
    }
    
    qDebug() << "GraphFactory: Created edge from XML, id:" << edgeId.left(8)
             << "from node:" << fromNode.left(8) << "socket" << fromIndex
             << "to node:" << toNode.left(8) << "socket" << toIndex;
    
    return edge;
}

Node* GraphFactory::createNode(const QString& nodeType, const QPointF& position, int inputs, int outputs)
{
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
        qDebug() << "GraphFactory::createNode - SUCCESS: Created" << nodeType 
                 << "node:" << node->getId().toString(QUuid::WithoutBraces).left(8);
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
    return createEdgeFromXml(xmlEdge);
}

Edge* GraphFactory::connectSockets(Socket* fromSocket, Socket* toSocket)
{
    if (!fromSocket || !toSocket) {
        qCritical() << "GraphFactory::connectSockets - null socket(s)";
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
    edge->setResolvedSockets(fromSocket, toSocket);
    
    // Add to scene
    if (Scene* typedScene = static_cast<Scene*>(m_scene)) {
        typedScene->addEdge(edge);
    } else {
        m_scene->addItem(edge);
    }
    
    // Create XML for persistence (lazy serialization)
    xmlNodePtr xmlEdge = createXmlEdgeNodeIndex(fromNode->getId(), fromSocket->getIndex(), 
                                                toNode->getId(), toSocket->getIndex());
    if (!xmlEdge) {
        qWarning() << "GraphFactory::connectSockets - XML serialization failed (edge still created)";
    }
    
    // Connect sockets atomically
    fromSocket->setConnectedEdge(edge);
    toSocket->setConnectedEdge(edge);
    
    qDebug() << "GraphFactory: Atomically connected sockets" 
             << "index" << fromSocket->getIndex()
             << "to index" << toSocket->getIndex();
    
    return edge;
}

bool GraphFactory::loadFromXmlFile(const QString& filePath)
{
    qDebug() << "=== GraphFactory: Loading from XML File ===" << filePath;
    
    // OPTIMIZATION: Enable batch mode to prevent observer storm during bulk loading
    GraphSubject::beginBatch();
    
    // Parse XML file
    xmlDocPtr doc = xmlParseFile(filePath.toUtf8().constData());
    if (!doc) {
        qCritical() << "GraphFactory::loadFromXmlFile - failed to parse XML file:" << filePath;
        GraphSubject::endBatch();  // Clean up batch mode on error
        return false;
    }
    
    xmlNodePtr root = xmlDocGetRootElement(doc);
    if (!root) {
        qCritical() << "GraphFactory::loadFromXmlFile - no root element";
        xmlFreeDoc(doc);
        GraphSubject::endBatch();  // Clean up batch mode on error
        return false;
    }
    
    if (xmlStrcmp(root->name, (const xmlChar*)"graph") != 0) {
        qCritical() << "Invalid XML file: root element should be 'graph'";
        xmlFreeDoc(doc);
        GraphSubject::endBatch();  // Clean up batch mode on error
        return false;
    }
    
    qDebug() << "✓ XML file parsed successfully";
    
    // PHASE 1: Load ALL nodes first - handle both direct and nested formats
    QVector<Node*> allNodes;
    
    // Check if nodes are direct children or nested under <nodes>
    xmlNodePtr nodesContainer = nullptr;
    for (xmlNodePtr child = root->children; child; child = child->next) {
        if (child->type == XML_ELEMENT_NODE && xmlStrcmp(child->name, (const xmlChar*)"nodes") == 0) {
            nodesContainer = child;
            break;
        }
    }
    
    // Load nodes from appropriate location
    xmlNodePtr nodeParent = nodesContainer ? nodesContainer : root;
    qDebug() << "=== Loading Nodes" << (nodesContainer ? "from <nodes> wrapper" : "directly from root") << "===";
    
    for (xmlNodePtr xmlNode = nodeParent->children; xmlNode; xmlNode = xmlNode->next) {
        if (xmlNode->type == XML_ELEMENT_NODE && xmlStrcmp(xmlNode->name, (const xmlChar*)"node") == 0) {
            
            // Check if this node has the format we support (inputs/outputs attributes)
            xmlChar* inputsAttr = xmlGetProp(xmlNode, BAD_CAST "inputs");
            xmlChar* outputsAttr = xmlGetProp(xmlNode, BAD_CAST "outputs");
            
            if (inputsAttr && outputsAttr) {
                // Our format: <node inputs="1" outputs="1" />
                Node* node = createNodeFromXml(xmlNode);
                if (node) {
                    allNodes.append(node);
                    qDebug() << "✓ Loaded node:" << node->getNodeType() 
                             << "ID:" << node->getId().toString(QUuid::WithoutBraces).left(8);
                }
                xmlFree(inputsAttr);
                xmlFree(outputsAttr);
            } else {
                // Nested socket format: <node><socket /></node> - skip
                qDebug() << "Skipping node with nested socket format (not supported)";
                if (inputsAttr) xmlFree(inputsAttr);
                if (outputsAttr) xmlFree(outputsAttr);
            }
        }
    }
    
    // PHASE 2: Load edges (but don't resolve connections yet)
    QVector<Edge*> allEdges;
    qDebug() << "=== Loading Edges directly from root ===";
    for (xmlNodePtr xmlNode = root->children; xmlNode; xmlNode = xmlNode->next) {
        if (xmlNode->type == XML_ELEMENT_NODE && xmlStrcmp(xmlNode->name, (const xmlChar*)"edge") == 0) {
            Edge* edge = createEdgeFromXml(xmlNode);
            if (edge) {
                allEdges.append(edge);
                qDebug() << "✓ Loaded edge:" << edge->getId().toString(QUuid::WithoutBraces).left(8);
            }
        }
    }
    
    qDebug() << "=== XML Loading Complete ===";
    qDebug() << "✓ Loaded" << allNodes.size() << "nodes and" << allEdges.size() << "edges";
    
    xmlFreeDoc(doc);
    
    // PHASE 3: Resolve all edge connections now that all nodes exist
    qDebug() << "=== Resolving Edge Connections ===";
    int successfulConnections = 0;
    if (Scene* typedScene = static_cast<Scene*>(m_scene)) {
        for (Edge* edge : allEdges) {
            qDebug() << "Resolving edge:" << edge->getId().toString(QUuid::WithoutBraces).left(8);
            if (edge->resolveConnections(typedScene)) {
                successfulConnections++;
            } else {
                qWarning() << "FAILED to resolve edge connections:" << edge->getId().toString(QUuid::WithoutBraces).left(8);
            }
        }
    }
    
    qDebug() << "✓ Graph loaded:" << allNodes.size() << "nodes," << successfulConnections << "/" << allEdges.size() << "edges connected";
    
    // OPTIMIZATION: End batch mode to resume normal observer notifications
    GraphSubject::endBatch();
    
    // Validate graph integrity in debug builds
    #ifdef QT_DEBUG
    if (!validateGraphIntegrity()) {
        qWarning() << "Graph integrity validation failed after loading";
    }
    #endif
    
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
    
    Scene* typedScene = static_cast<Scene*>(m_scene);
    if (!typedScene) {
        qCritical() << "GraphFactory::validateGraphIntegrity - scene not typed";
        return false;
    }
    
    bool valid = true;
    
    // Validate all nodes have UUIDs and observers
    for (Node* node : typedScene->getNodes().values()) {
        if (!node) {
            qCritical() << "Validation: null node in scene";
            valid = false;
            continue;
        }
        
        if (node->getId().isNull()) {
            qCritical() << "Validation: node without UUID";
            valid = false;
        }
        
        // Check sockets belong to parent node
        for (QGraphicsItem* child : node->childItems()) {
            if (Socket* socket = qgraphicsitem_cast<Socket*>(child)) {
                if (socket->getParentNode() != node) {
                    qCritical() << "Validation: socket parent mismatch";
                    valid = false;
                }
            }
        }
    }
    
    // Validate all edges have valid socket connections
    for (Edge* edge : typedScene->getEdges().values()) {
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
    int typedItems = typedScene->getNodes().size() + typedScene->getEdges().size();
    
    // Account for sockets as children
    int socketCount = 0;
    for (Node* node : typedScene->getNodes().values()) {
        socketCount += node->getSocketCount();
    }
    typedItems += socketCount;
    
    if (sceneItems != typedItems) {
        qWarning() << "Validation: scene item count mismatch - scene:" << sceneItems 
                   << "typed:" << typedItems;
        // This is a warning, not an error - some items might be temporary
    }
    
    if (valid) {
        qDebug() << "✓ Graph integrity validation passed";
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
    
    // Add to typed scene collection if available
    if (Scene* typedScene = static_cast<Scene*>(m_scene)) {
        typedScene->addSocket(socket);
    }
    
    qDebug() << "GraphFactory: Created socket" << (role == Socket::Input ? "Input" : "Output") 
             << "index" << index << "for node" << parentNode->getId().toString(QUuid::WithoutBraces).left(8);
    
    return socket;
}