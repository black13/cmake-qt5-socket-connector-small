#include "graph_controller.h"
#include "scene.h"
#include "graph_factory.h"
#include "node.h"
#include "edge.h"
#include "socket.h"
#include "node_registry.h"
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <libxml/tree.h>
#include <libxml/parser.h>

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
    qDebug() << "GraphController: Validating node type" << type;
    qDebug() << "GraphController: Available types:" << validTypes;
    
    if (!isValidNodeType(type)) {
        qDebug() << "GraphController: INVALID node type:" << type;
        emit error(QString("GraphController: Invalid node type: %1").arg(type));
        return QString();
    } else {
        qDebug() << "GraphController: VALID node type:" << type;
    }
    
    qDebug() << "GraphController: Creating node" << type << "at" << x << "," << y;
    
    try {
        Node* node = m_factory->createNode(type, QPointF(x, y));
        if (node) {
            QString uuid = node->getId().toString();
            qDebug() << "GraphController: Node created successfully!";
            qDebug() << "GraphController: Node UUID:" << uuid;
            qDebug() << "GraphController: Node type:" << node->getNodeType();
            qDebug() << "GraphController: Node position:" << node->pos();
            emit nodeCreated(uuid);
            return uuid;
        } else {
            qDebug() << "GraphController: ❌ Factory returned null node";
            emit error("GraphController: Factory failed to create node");
        }
    } catch (const std::exception& e) {
        qDebug() << "GraphController: ❌ Exception during node creation:" << e.what();
        emit error(QString("GraphController: Error creating node: %1").arg(e.what()));
    }
    
    qDebug() << "GraphController: ❌ Node creation failed - returning empty string";
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
    QStringList registeredTypes = NodeRegistry::instance().getRegisteredTypes();
    bool isValid = registeredTypes.contains(type);
    qDebug() << "GraphController: Type validation:" << type << "→" << (isValid ? "VALID" : "INVALID");
    return isValid;
}

QStringList GraphController::getValidNodeTypes()
{
    QStringList types = NodeRegistry::instance().getRegisteredTypes();
    qDebug() << "GraphController: Available node types:" << types;
    return types;
}

Node* GraphController::findNode(const QString& uuid)
{
    if (!m_scene) {
        return nullptr;
    }
    
    QUuid nodeId = QUuid::fromString(uuid);
    return m_scene->getNode(nodeId);
}

Edge* GraphController::findEdge(const QString& uuid)
{
    if (!m_scene) {
        return nullptr;
    }
    
    QUuid edgeId = QUuid::fromString(uuid);
    return m_scene->getEdge(edgeId);
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
    nodeData["connectedEdges"] = node->getIncidentEdgeCount();
    
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
    Socket* fromSocket = edge->getFromSocket();
    Socket* toSocket = edge->getToSocket();
    
    if (fromSocket && toSocket) {
        edgeData["fromNode"] = fromSocket->getParentNode()->getId().toString();
        edgeData["fromIndex"] = fromSocket->getIndex();
        edgeData["toNode"] = toSocket->getParentNode()->getId().toString();
        edgeData["toIndex"] = toSocket->getIndex();
    }
    
    return edgeData;
}