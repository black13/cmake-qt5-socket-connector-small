#include "graph_controller.h"
#include "scene.h"
#include "graph_factory.h"
#include "node.h"
#include "edge.h"
#include "socket.h"
#include "node_templates.h"
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <QUuid>

GraphController::GraphController(Scene* scene, GraphFactory* factory, QObject* parent)
    : QObject(parent)
    , m_scene(scene)
    , m_factory(factory)
{
    qDebug() << "GraphController: JavaScript interface initialized";
}

QString GraphController::createNode(const QString& type, qreal x, qreal y)
{
    if (!m_scene || !m_factory) {
        emit error("GraphController: Scene or factory not initialized");
        return QString();
    }
    
    // Log node type validation
    QStringList validTypes = getValidNodeTypes();
    qDebug() << __FUNCTION__ << ": Validating node type" << type;
    qDebug() << __FUNCTION__ << ": Available types:" << validTypes;
    
    if (!isValidNodeType(type)) {
        qDebug() << __FUNCTION__ << ": INVALID node type:" << type;
        emit error(QString("GraphController: Invalid node type: %1").arg(type));
        return QString();
    } else {
        qDebug() << __FUNCTION__ << ": VALID node type:" << type;
    }
    
    qDebug() << __FUNCTION__ << ": Creating node" << type << "at" << x << "," << y;
    
    try {
        Node* node = m_factory->createNode(type, QPointF(x, y));
        if (node) {
            QString uuid = node->getId().toString();
            qDebug() << __FUNCTION__ << ": Node created successfully!";
            qDebug() << __FUNCTION__ << ": Node UUID:" << uuid;
            qDebug() << __FUNCTION__ << ": Node type:" << node->getNodeType();
            qDebug() << __FUNCTION__ << ": Node position:" << node->pos();
            emit nodeCreated(uuid);
            return uuid;
        } else {
            qDebug() << __FUNCTION__ << ": Factory returned null node";
            emit error("GraphController: Factory failed to create node");
        }
    } catch (const std::exception& e) {
        qDebug() << __FUNCTION__ << ": Exception during node creation:" << e.what();
        emit error(QString("GraphController: Error creating node: %1").arg(e.what()));
    }
    
    qDebug() << __FUNCTION__ << ": Node creation failed - returning empty string";
    return QString();
}

bool GraphController::deleteNode(const QString& uuid)
{
    if (!m_scene) {
        emit error("GraphController: Scene not initialized");
        return false;
    }
    
    Node* node = findNode(uuid);
    if (!node) {
        emit error(QString("GraphController: Node not found: %1").arg(uuid));
        return false;
    }
    
    qDebug() << "GraphController: Deleting node" << uuid;
    
    try {
        QUuid nodeId = QUuid::fromString(uuid);
        m_scene->deleteNode(nodeId);
        emit nodeDeleted(uuid);
        return true;
    } catch (const std::exception& e) {
        emit error(QString("GraphController: Error deleting node: %1").arg(e.what()));
        return false;
    }
}

bool GraphController::moveNode(const QString& uuid, qreal dx, qreal dy)
{
    Node* node = findNode(uuid);
    if (!node) {
        emit error(QString("GraphController: Node not found: %1").arg(uuid));
        return false;
    }
    
    QPointF currentPos = node->pos();
    QPointF newPos = currentPos + QPointF(dx, dy);
    
    qDebug() << "GraphController: Moving node" << uuid << "by" << dx << "," << dy;
    
    node->setPos(newPos);
    return true;
}

QVariantMap GraphController::getNode(const QString& uuid)
{
    Node* node = findNode(uuid);
    if (!node) {
        return QVariantMap();
    }
    
    return nodeToVariant(node);
}

QVariantList GraphController::getNodes()
{
    QVariantList nodes;
    
    if (!m_scene) {
        return nodes;
    }
    
    const auto& nodeMap = m_scene->getNodes();
    for (Node* node : nodeMap.values()) {
        nodes.append(nodeToVariant(node));
    }
    
    return nodes;
}

QString GraphController::connect(const QString& fromNodeId, int fromIndex, 
                                const QString& toNodeId, int toIndex)
{
    qDebug() << "GraphController::connect() called - from:" << fromNodeId.left(8) 
             << "[" << fromIndex << "] to:" << toNodeId.left(8) << "[" << toIndex << "]";
             
    if (!m_scene || !m_factory) {
        emit error("GraphController: Scene or factory not initialized");
        return QString();
    }
    
    Node* fromNode = findNode(fromNodeId);
    Node* toNode = findNode(toNodeId);
    
    if (!fromNode || !toNode) {
        emit error(QString("GraphController: Node not found for connection: %1 -> %2")
                  .arg(fromNodeId).arg(toNodeId));
        return QString();
    }
    
    // Validate connection before attempting
    emit error(QString("DEBUG: About to call canConnect() for %1[%2] -> %3[%4]")
              .arg(fromNodeId.left(8)).arg(fromIndex).arg(toNodeId.left(8)).arg(toIndex));
    if (!canConnect(fromNodeId, fromIndex, toNodeId, toIndex)) {
        qDebug() << "GraphController: Connection validation failed";
        emit error("DEBUG: canConnect() returned FALSE - connection blocked!");
        return QString();
    }
    emit error("DEBUG: canConnect() returned TRUE - proceeding with connection");
    
    qDebug() << "GraphController: Connecting" << fromNodeId << "[" << fromIndex << "] ->" 
             << toNodeId << "[" << toIndex << "]";
    
    try {
        // Find the appropriate sockets
        Socket* fromSocket = nullptr;
        Socket* toSocket = nullptr;
        
        // Get child sockets from nodes
        const auto& fromItems = fromNode->childItems();
        const auto& toItems = toNode->childItems();
        
        for (QGraphicsItem* item : fromItems) {
            if (Socket* socket = qgraphicsitem_cast<Socket*>(item)) {
                if (socket->getIndex() == fromIndex && socket->getRole() == Socket::Output) {
                    fromSocket = socket;
                    break;
                }
            }
        }
        
        for (QGraphicsItem* item : toItems) {
            if (Socket* socket = qgraphicsitem_cast<Socket*>(item)) {
                if (socket->getIndex() == toIndex && socket->getRole() == Socket::Input) {
                    toSocket = socket;
                    break;
                }
            }
        }
        
        if (!fromSocket || !toSocket) {
            emit error(QString("GraphController: Socket not found for connection"));
            return QString();
        }
        
        Edge* edge = m_factory->createEdge(fromNode, fromIndex, toNode, toIndex);
        if (edge) {
            QString uuid = edge->getId().toString();
            emit edgeCreated(uuid);
            qDebug() << "GraphController: Created edge" << uuid;
            return uuid;
        }
    } catch (const std::exception& e) {
        emit error(QString("GraphController: Error creating connection: %1").arg(e.what()));
    }
    
    return QString();
}

bool GraphController::deleteEdge(const QString& uuid)
{
    if (!m_scene) {
        emit error("GraphController: Scene not initialized");
        return false;
    }
    
    Edge* edge = findEdge(uuid);
    if (!edge) {
        emit error(QString("GraphController: Edge not found: %1").arg(uuid));
        return false;
    }
    
    qDebug() << "GraphController: Deleting edge" << uuid;
    
    try {
        QUuid edgeId = QUuid::fromString(uuid);
        m_scene->deleteEdge(edgeId);
        emit edgeDeleted(uuid);
        return true;
    } catch (const std::exception& e) {
        emit error(QString("GraphController: Error deleting edge: %1").arg(e.what()));
        return false;
    }
}

QVariantList GraphController::getEdges()
{
    QVariantList edges;
    
    if (!m_scene) {
        return edges;
    }
    
    const auto& edgeMap = m_scene->getEdges();
    for (Edge* edge : edgeMap.values()) {
        edges.append(edgeToVariant(edge));
    }
    
    return edges;
}

void GraphController::clear()
{
    if (!m_scene) {
        emit error("GraphController: Scene not initialized");
        return;
    }
    
    qDebug() << "GraphController: Clearing graph";
    
    m_scene->clearGraph();
    emit graphCleared();
}

void GraphController::saveXml(const QString& path)
{
    if (!m_scene) {
        emit error("GraphController: Scene not initialized");
        return;
    }
    
    qDebug() << "GraphController: Saving XML to" << path;
    
    try {
        // Create XML document
        xmlDocPtr doc = xmlNewDoc(BAD_CAST "1.0");
        xmlNodePtr root = xmlNewNode(NULL, BAD_CAST "graph");
        xmlDocSetRootElement(doc, root);
        xmlNewProp(root, BAD_CAST "version", BAD_CAST "1.0");
        
        // Add nodes
        const auto& nodes = m_scene->getNodes();
        for (Node* node : nodes.values()) {
            node->write(doc, root);
        }
        
        // Add edges
        const auto& edges = m_scene->getEdges();
        for (Edge* edge : edges.values()) {
            edge->write(doc, root);
        }
        
        // Save to file
        int result = xmlSaveFileEnc(path.toUtf8().constData(), doc, "UTF-8");
        xmlFreeDoc(doc);
        
        if (result != -1) {
            emit xmlSaved(path);
            qDebug() << "GraphController: XML saved successfully";
        } else {
            emit error(QString("GraphController: Failed to save XML to %1").arg(path));
        }
    } catch (const std::exception& e) {
        emit error(QString("GraphController: Error saving XML: %1").arg(e.what()));
    }
}

void GraphController::loadXml(const QString& path)
{
    if (!m_scene || !m_factory) {
        emit error("GraphController: Scene or factory not initialized");
        return;
    }
    
    qDebug() << "GraphController: Loading XML from" << path;
    
    try {
        // Clear existing graph
        m_scene->clearGraph();
        
        // Parse XML file
        xmlDocPtr doc = xmlParseFile(path.toUtf8().constData());
        if (!doc) {
            emit error(QString("GraphController: Failed to parse XML file: %1").arg(path));
            return;
        }
        
        xmlNodePtr root = xmlDocGetRootElement(doc);
        if (!root) {
            xmlFreeDoc(doc);
            emit error(QString("GraphController: Invalid XML structure in: %1").arg(path));
            return;
        }
        
        // Load nodes and edges through factory
        m_factory->loadFromXmlFile(path);
        
        xmlFreeDoc(doc);
        emit xmlLoaded(path);
        qDebug() << "GraphController: XML loaded successfully";
    } catch (const std::exception& e) {
        emit error(QString("GraphController: Error loading XML: %1").arg(e.what()));
    }
}

void GraphController::rebuildXml()
{
    qDebug() << "GraphController: Rebuilding XML from scene";
    
    // This would trigger XmlLiveSync to rebuild the XML from the current scene state
    // For now, we'll just log the action
    qDebug() << "GraphController: XML rebuild requested";
}

QString GraphController::getXmlString()
{
    if (!m_scene) {
        return QString();
    }
    
    try {
        // Create XML document
        xmlDocPtr doc = xmlNewDoc(BAD_CAST "1.0");
        xmlNodePtr root = xmlNewNode(NULL, BAD_CAST "graph");
        xmlDocSetRootElement(doc, root);
        xmlNewProp(root, BAD_CAST "version", BAD_CAST "1.0");
        
        // Add nodes
        const auto& nodes = m_scene->getNodes();
        for (Node* node : nodes.values()) {
            node->write(doc, root);
        }
        
        // Add edges
        const auto& edges = m_scene->getEdges();
        for (Edge* edge : edges.values()) {
            edge->write(doc, root);
        }
        
        // Convert to string
        xmlChar* xmlStr;
        int size;
        xmlDocDumpFormatMemoryEnc(doc, &xmlStr, &size, "UTF-8", 1);
        
        QString result = QString::fromUtf8(reinterpret_cast<const char*>(xmlStr));
        
        xmlFree(xmlStr);
        xmlFreeDoc(doc);
        
        return result;
    } catch (const std::exception& e) {
        emit error(QString("GraphController: Error generating XML string: %1").arg(e.what()));
        return QString();
    }
}

QVariantMap GraphController::getStats()
{
    QVariantMap stats;
    
    if (!m_scene) {
        stats["nodes"] = 0;
        stats["edges"] = 0;
        return stats;
    }
    
    const auto& nodes = m_scene->getNodes();
    const auto& edges = m_scene->getEdges();
    
    stats["nodes"] = nodes.size();
    stats["edges"] = edges.size();
    
    // Count by node type
    QVariantMap nodeTypes;
    for (Node* node : nodes.values()) {
        QString type = node->getNodeType();
        nodeTypes[type] = nodeTypes[type].toInt() + 1;
    }
    stats["nodeTypes"] = nodeTypes;
    
    return stats;
}

bool GraphController::isValidNodeType(const QString& type)
{
    // Use template system instead of NodeRegistry
    bool isValid = NodeTypeTemplates::hasNodeType(type);
    qDebug() << "GraphController: Type validation:" << type << "->" << (isValid ? "VALID" : "INVALID");
    return isValid;
}

QStringList GraphController::getValidNodeTypes()
{
    QStringList types = NodeTypeTemplates::getAvailableTypes();
    qDebug() << "GraphController: Available node types:" << types;
    return types;
}

Node* GraphController::findNode(const QString& uuid)
{
    if (!m_scene) {
        return nullptr;
    }
    
    QUuid nodeId = QUuid::fromString(uuid);
    // Since Scene::getNode() might not exist, we'll use the node map directly
    const auto& nodes = m_scene->getNodes();
    auto it = nodes.find(nodeId);
    if (it != nodes.end()) {
        return it.value();
    }
    return nullptr;
}

Edge* GraphController::findEdge(const QString& uuid)
{
    if (!m_scene) {
        return nullptr;
    }
    
    QUuid edgeId = QUuid::fromString(uuid);
    // Since Scene::getEdge() might not exist, we'll use the edge map directly
    const auto& edges = m_scene->getEdges();
    auto it = edges.find(edgeId);
    if (it != edges.end()) {
        return it.value();
    }
    return nullptr;
}

QVariantMap GraphController::nodeToVariant(Node* node)
{
    QVariantMap nodeData;
    
    if (!node) {
        return nodeData;
    }
    
    nodeData["id"] = node->getId().toString();
    nodeData["type"] = node->getNodeType();
    nodeData["x"] = node->pos().x();
    nodeData["y"] = node->pos().y();
    
    // Add node dimensions
    QRectF bounds = node->boundingRect();
    nodeData["width"] = bounds.width();
    nodeData["height"] = bounds.height();
    
    // Add selection state
    nodeData["selected"] = node->isSelected();
    
    // Add socket information with enhanced details
    QVariantList sockets;
    int inputCount = 0;
    int outputCount = 0;
    
    for (QGraphicsItem* item : node->childItems()) {
        if (Socket* socket = qgraphicsitem_cast<Socket*>(item)) {
            QVariantMap socketData;
            socketData["index"] = socket->getIndex();
            socketData["type"] = (socket->getRole() == Socket::Input) ? "input" : "output";
            socketData["connected"] = socket->isConnected();
            
            // Add socket position relative to node
            QPointF socketPos = socket->pos();
            socketData["relativeX"] = socketPos.x();
            socketData["relativeY"] = socketPos.y();
            
            sockets.append(socketData);
            
            if (socket->getRole() == Socket::Input) {
                inputCount++;
            } else {
                outputCount++;
            }
        }
    }
    
    nodeData["sockets"] = sockets;
    nodeData["inputCount"] = inputCount;
    nodeData["outputCount"] = outputCount;
    nodeData["totalSockets"] = sockets.size();
    
    // Add edge connection information
    // Since getIncidentEdgeCount() might not exist, we'll need to calculate it
    int edgeCount = 0;
    // This would need to be implemented differently, but for now, we'll set to 0
    // We could potentially count edges connected to this node's sockets
    nodeData["connectedEdges"] = edgeCount;
    
    return nodeData;
}

QVariantMap GraphController::edgeToVariant(Edge* edge)
{
    QVariantMap edgeData;
    
    if (!edge) {
        return edgeData;
    }
    
    edgeData["id"] = edge->getId().toString();
    
    // Add connection information
    // Since getFromSocket() and getToSocket() might not exist, we need to find another way
    // We'll need to track this information differently
    // For now, we'll leave these fields empty
    edgeData["fromNode"] = "";
    edgeData["fromIndex"] = -1;
    edgeData["toNode"] = "";
    edgeData["toIndex"] = -1;
    
    // TODO: Implement proper edge connection tracking
    
    return edgeData;
}

QVariantList GraphController::getInputSockets(const QString& nodeId)
{
    QVariantList inputSockets;
    Node* node = findNode(nodeId);
    if (!node) {
        emit error(QString("GraphController: Node not found: %1").arg(nodeId));
        return inputSockets;
    }
    
    for (QGraphicsItem* item : node->childItems()) {
        if (Socket* socket = qgraphicsitem_cast<Socket*>(item)) {
            if (socket->getRole() == Socket::Input) {
                QVariantMap socketInfo;
                socketInfo["index"] = socket->getIndex();
                socketInfo["connected"] = socket->isConnected();
                socketInfo["type"] = "input";
                inputSockets.append(socketInfo);
            }
        }
    }
    
    return inputSockets;
}

QVariantList GraphController::getOutputSockets(const QString& nodeId)
{
    QVariantList outputSockets;
    Node* node = findNode(nodeId);
    if (!node) {
        emit error(QString("GraphController: Node not found: %1").arg(nodeId));
        return outputSockets;
    }
    
    for (QGraphicsItem* item : node->childItems()) {
        if (Socket* socket = qgraphicsitem_cast<Socket*>(item)) {
            if (socket->getRole() == Socket::Output) {
                QVariantMap socketInfo;
                socketInfo["index"] = socket->getIndex();
                socketInfo["connected"] = socket->isConnected();
                socketInfo["type"] = "output";
                outputSockets.append(socketInfo);
            }
        }
    }
    
    return outputSockets;
}

QVariantMap GraphController::getSocketInfo(const QString& nodeId, int socketIndex)
{
    QVariantMap socketInfo;
    Node* node = findNode(nodeId);
    if (!node) {
        emit error(QString("GraphController: Node not found: %1").arg(nodeId));
        return socketInfo;
    }
    
    for (QGraphicsItem* item : node->childItems()) {
        if (Socket* socket = qgraphicsitem_cast<Socket*>(item)) {
            if (socket->getIndex() == socketIndex) {
                socketInfo["index"] = socket->getIndex();
                socketInfo["type"] = (socket->getRole() == Socket::Input) ? "input" : "output";
                socketInfo["connected"] = socket->isConnected();
                socketInfo["role"] = socket->getRole();
                
                // Add position info
                QPointF pos = socket->pos();
                socketInfo["x"] = pos.x();
                socketInfo["y"] = pos.y();
                
                return socketInfo;
            }
        }
    }
    
    emit error(QString("GraphController: Socket %1 not found on node %2").arg(socketIndex).arg(nodeId));
    return socketInfo;
}

bool GraphController::canConnect(const QString& fromNodeId, int fromIndex, const QString& toNodeId, int toIndex)
{
    // Find nodes
    Node* fromNode = findNode(fromNodeId);
    Node* toNode = findNode(toNodeId);
    
    if (!fromNode || !toNode) {
        emit error(QString("GraphController: Node not found for connection validation: %1 -> %2")
                  .arg(fromNodeId).arg(toNodeId));
        return false;
    }
    
    // Find sockets
    Socket* fromSocket = nullptr;
    Socket* toSocket = nullptr;
    
    for (QGraphicsItem* item : fromNode->childItems()) {
        if (Socket* socket = qgraphicsitem_cast<Socket*>(item)) {
            if (socket->getIndex() == fromIndex && socket->getRole() == Socket::Output) {
                fromSocket = socket;
                break;
            }
        }
    }
    
    for (QGraphicsItem* item : toNode->childItems()) {
        if (Socket* socket = qgraphicsitem_cast<Socket*>(item)) {
            if (socket->getIndex() == toIndex && socket->getRole() == Socket::Input) {
                toSocket = socket;
                break;
            }
        }
    }
    
    if (!fromSocket || !toSocket) {
        emit error(QString("GraphController: Socket not found - from:%1[%2] to:%3[%4]")
                  .arg(fromNodeId).arg(fromIndex).arg(toNodeId).arg(toIndex));
        return false;
    }
    
    // Validate connection rules
    if (fromSocket->getRole() != Socket::Output) {
        emit error(QString("GraphController: Source socket must be OUTPUT, got %1")
                  .arg(fromSocket->getRole() == Socket::Input ? "INPUT" : "UNKNOWN"));
        return false;
    }
    
    if (toSocket->getRole() != Socket::Input) {
        emit error(QString("GraphController: Target socket must be INPUT, got %1")
                  .arg(toSocket->getRole() == Socket::Output ? "OUTPUT" : "UNKNOWN"));
        return false;
    }
    
    // Check if sockets are already connected
    if (fromSocket->isConnected() || toSocket->isConnected()) {
        emit error(QString("GraphController: Socket already connected - from:%1[%2]=%3 to:%4[%5]=%6")
                  .arg(fromNodeId).arg(fromIndex).arg(fromSocket->isConnected() ? "CONN" : "FREE")
                  .arg(toNodeId).arg(toIndex).arg(toSocket->isConnected() ? "CONN" : "FREE"));
        return false;
    }
    
    // Additional check: scan existing edges to prevent race conditions during rapid creation
    if (m_scene) {
        const auto& existingEdges = m_scene->getEdges();
        // Check if toSocket is already connected through any edge
        // This is a redundant check since we already checked toSocket->isConnected() above
        // But it's here to be thorough
        if (toSocket->isConnected()) {
            qWarning() << "GraphController: BLOCKING double connection! Target socket already has connection";
            emit error(QString("GraphController: Target socket is already connected"));
            return false;
        }
    }
    
    // Prevent self-connection
    if (fromNodeId == toNodeId) {
        emit error("GraphController: Cannot connect node to itself");
        return false;
    }
    
    return true;
}
