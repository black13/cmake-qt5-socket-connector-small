#include "xml_live_sync.h"
#include "scene.h"
#include "node.h"
#include "edge.h"
#include "socket.h"
#include <QDebug>
#include <QFile>
#include <libxml/xmlsave.h>

XmlLiveSync::XmlLiveSync(Scene* scene, xmlDocPtr xmlDocument)
    : m_scene(scene)
    , m_xmlDocument(xmlDocument)
    , m_enabled(true)
{
    if (!m_scene) {
        qCritical() << "XmlLiveSync: null scene provided";
        return;
    }
    
    if (!m_xmlDocument) {
        qCritical() << "XmlLiveSync: null XML document provided";
        return;
    }
    
    // Register as observer with the scene
    m_scene->attach(this);
    
    qDebug() << "✓ XmlLiveSync: Real-time XML synchronization enabled";
}

XmlLiveSync::~XmlLiveSync()
{
    if (m_scene) {
        m_scene->detach(this);
    }
    qDebug() << "✓ XmlLiveSync: Destroyed";
}

void XmlLiveSync::setEnabled(bool enabled)
{
    m_enabled = enabled;
    qDebug() << "XmlLiveSync:" << (enabled ? "ENABLED" : "DISABLED");
}

bool XmlLiveSync::saveToFile(const QString& filename)
{
    if (!m_xmlDocument) {
        qCritical() << "XmlLiveSync::saveToFile: No XML document";
        return false;
    }
    
    // Fast save - XML is already current
    xmlSaveCtxtPtr saveCtxt = xmlSaveToFilename(filename.toUtf8().constData(), "UTF-8", XML_SAVE_FORMAT);
    if (!saveCtxt) {
        qCritical() << "XmlLiveSync::saveToFile: Failed to create save context for" << filename;
        return false;
    }
    
    long bytesWritten = xmlSaveDoc(saveCtxt, m_xmlDocument);
    xmlSaveClose(saveCtxt);
    
    if (bytesWritten > 0) {
        qDebug() << "✓ XmlLiveSync: Fast save to" << filename << "(" << bytesWritten << "bytes)";
        return true;
    } else {
        qCritical() << "XmlLiveSync::saveToFile: Save failed for" << filename;
        return false;
    }
}

void XmlLiveSync::rebuildXmlFromScene()
{
    if (!m_enabled || !m_xmlDocument || !m_scene) return;
    
    qDebug() << "XmlLiveSync: Rebuilding XML from current scene state";
    
    // Clear existing XML content
    xmlNodePtr root = xmlDocGetRootElement(m_xmlDocument);
    if (root) {
        xmlUnlinkNode(root);
        xmlFreeNode(root);
    }
    
    // Create new root element
    root = xmlNewNode(nullptr, BAD_CAST "graph");
    xmlDocSetRootElement(m_xmlDocument, root);
    
    // Clear caches
    m_nodeXmlCache.clear();
    m_edgeXmlCache.clear();
    
    // Rebuild from scene
    const auto& nodes = m_scene->getNodes();
    const auto& edges = m_scene->getEdges();
    
    // Add all nodes
    for (auto it = nodes.constBegin(); it != nodes.constEnd(); ++it) {
        addNodeToXml(*it.value());
    }
    
    // Add all edges
    for (auto it = edges.constBegin(); it != edges.constEnd(); ++it) {
        addEdgeToXml(*it.value());
    }
    
    qDebug() << "✓ XmlLiveSync: Rebuilt XML with" << nodes.size() << "nodes," << edges.size() << "edges";
}

// GraphObserver interface implementations
void XmlLiveSync::onNodeAdded(const Node& node)
{
    if (!m_enabled) return;
    addNodeToXml(node);
    qDebug() << "XmlLiveSync: Added node" << node.getId().toString(QUuid::WithoutBraces).left(8) << "to XML";
}

void XmlLiveSync::onNodeRemoved(const QUuid& nodeId)
{
    if (!m_enabled) return;
    removeNodeFromXml(nodeId);
    qDebug() << "XmlLiveSync: Removed node" << nodeId.toString(QUuid::WithoutBraces).left(8) << "from XML";
}

void XmlLiveSync::onNodeMoved(const QUuid& nodeId, QPointF oldPos, QPointF newPos)
{
    if (!m_enabled) return;
    updateNodePositionInXml(nodeId, newPos);
    qDebug() << "XmlLiveSync: Updated node" << nodeId.toString(QUuid::WithoutBraces).left(8) 
             << "position" << oldPos << "→" << newPos;
}

void XmlLiveSync::onEdgeAdded(const Edge& edge)
{
    if (!m_enabled) return;
    addEdgeToXml(edge);
    qDebug() << "XmlLiveSync: Added edge" << edge.getId().toString(QUuid::WithoutBraces).left(8) << "to XML";
}

void XmlLiveSync::onEdgeRemoved(const QUuid& edgeId)
{
    if (!m_enabled) return;
    removeEdgeFromXml(edgeId);
    qDebug() << "XmlLiveSync: Removed edge" << edgeId.toString(QUuid::WithoutBraces).left(8) << "from XML";
}

void XmlLiveSync::onGraphCleared()
{
    if (!m_enabled) return;
    qDebug() << "XmlLiveSync: Graph cleared, rebuilding XML";
    rebuildXmlFromScene();
}

// Private implementation methods
xmlNodePtr XmlLiveSync::getOrCreateNodesElement()
{
    xmlNodePtr root = xmlDocGetRootElement(m_xmlDocument);
    if (!root) return nullptr;
    
    // Look for existing nodes element
    for (xmlNodePtr child = root->children; child; child = child->next) {
        if (child->type == XML_ELEMENT_NODE && 
            xmlStrcmp(child->name, BAD_CAST "nodes") == 0) {
            return child;
        }
    }
    
    // Create new nodes element
    xmlNodePtr nodesElement = xmlNewChild(root, nullptr, BAD_CAST "nodes", nullptr);
    return nodesElement;
}

xmlNodePtr XmlLiveSync::getOrCreateEdgesElement()
{
    xmlNodePtr root = xmlDocGetRootElement(m_xmlDocument);
    if (!root) return nullptr;
    
    // Look for existing edges element
    for (xmlNodePtr child = root->children; child; child = child->next) {
        if (child->type == XML_ELEMENT_NODE && 
            xmlStrcmp(child->name, BAD_CAST "edges") == 0) {
            return child;
        }
    }
    
    // Create new edges element
    xmlNodePtr edgesElement = xmlNewChild(root, nullptr, BAD_CAST "edges", nullptr);
    return edgesElement;
}

void XmlLiveSync::updateNodePositionInXml(const QUuid& nodeId, const QPointF& newPos)
{
    xmlNodePtr nodeXml = m_nodeXmlCache.value(nodeId, nullptr);
    if (!nodeXml) {
        nodeXml = findNodeXml(nodeId);
        if (nodeXml) {
            m_nodeXmlCache[nodeId] = nodeXml;
        }
    }
    
    if (nodeXml) {
        setXmlProperty(nodeXml, "x", QString::number(newPos.x()));
        setXmlProperty(nodeXml, "y", QString::number(newPos.y()));
    }
}

void XmlLiveSync::addNodeToXml(const Node& node)
{
    xmlNodePtr nodesElement = getOrCreateNodesElement();
    if (!nodesElement) return;
    
    xmlNodePtr nodeXml = xmlNewChild(nodesElement, nullptr, BAD_CAST "node", nullptr);
    if (!nodeXml) return;
    
    // Set node properties
    setXmlProperty(nodeXml, "id", node.getId().toString(QUuid::WithoutBraces));
    setXmlProperty(nodeXml, "type", node.getNodeType());
    setXmlProperty(nodeXml, "x", QString::number(node.pos().x()));
    setXmlProperty(nodeXml, "y", QString::number(node.pos().y()));
    // Count input/output sockets by iterating through all sockets
    int inputCount = 0, outputCount = 0;
    for (int i = 0; i < node.getSocketCount(); ++i) {
        Socket* socket = node.getSocketByIndex(i);
        if (socket) {
            if (socket->getRole() == Socket::Input) inputCount++;
            else if (socket->getRole() == Socket::Output) outputCount++;
        }
    }
    
    setXmlProperty(nodeXml, "inputs", QString::number(inputCount));
    setXmlProperty(nodeXml, "outputs", QString::number(outputCount));
    
    // Cache for fast updates
    m_nodeXmlCache[node.getId()] = nodeXml;
}

void XmlLiveSync::removeNodeFromXml(const QUuid& nodeId)
{
    xmlNodePtr nodeXml = m_nodeXmlCache.value(nodeId, nullptr);
    if (!nodeXml) {
        nodeXml = findNodeXml(nodeId);
    }
    
    if (nodeXml) {
        xmlUnlinkNode(nodeXml);
        xmlFreeNode(nodeXml);
        m_nodeXmlCache.remove(nodeId);
    }
}

void XmlLiveSync::addEdgeToXml(const Edge& edge)
{
    xmlNodePtr edgesElement = getOrCreateEdgesElement();
    if (!edgesElement) return;
    
    xmlNodePtr edgeXml = xmlNewChild(edgesElement, nullptr, BAD_CAST "edge", nullptr);
    if (!edgeXml) return;
    
    // Get edge connection data
    Node* fromNode = edge.getFromNode();
    Node* toNode = edge.getToNode();
    
    if (fromNode && toNode) {
        setXmlProperty(edgeXml, "id", edge.getId().toString(QUuid::WithoutBraces));
        setXmlProperty(edgeXml, "fromNode", fromNode->getId().toString(QUuid::WithoutBraces));
        setXmlProperty(edgeXml, "toNode", toNode->getId().toString(QUuid::WithoutBraces));
        
        // Get actual socket indices from edge
        setXmlProperty(edgeXml, "fromSocketIndex", QString::number(edge.getFromSocketIndex()));
        setXmlProperty(edgeXml, "toSocketIndex", QString::number(edge.getToSocketIndex()));
        
        // Cache for fast updates
        m_edgeXmlCache[edge.getId()] = edgeXml;
    }
}

void XmlLiveSync::removeEdgeFromXml(const QUuid& edgeId)
{
    xmlNodePtr edgeXml = m_edgeXmlCache.value(edgeId, nullptr);
    if (!edgeXml) {
        edgeXml = findEdgeXml(edgeId);
    }
    
    if (edgeXml) {
        xmlUnlinkNode(edgeXml);
        xmlFreeNode(edgeXml);
        m_edgeXmlCache.remove(edgeId);
    }
}

xmlNodePtr XmlLiveSync::findNodeXml(const QUuid& nodeId)
{
    xmlNodePtr nodesElement = getOrCreateNodesElement();
    if (!nodesElement) return nullptr;
    
    QString nodeIdStr = nodeId.toString(QUuid::WithoutBraces);
    
    for (xmlNodePtr child = nodesElement->children; child; child = child->next) {
        if (child->type == XML_ELEMENT_NODE && 
            xmlStrcmp(child->name, BAD_CAST "node") == 0) {
            
            QString id = getXmlProperty(child, "id");
            if (id == nodeIdStr) {
                return child;
            }
        }
    }
    return nullptr;
}

xmlNodePtr XmlLiveSync::findEdgeXml(const QUuid& edgeId)
{
    xmlNodePtr edgesElement = getOrCreateEdgesElement();
    if (!edgesElement) return nullptr;
    
    QString edgeIdStr = edgeId.toString(QUuid::WithoutBraces);
    
    for (xmlNodePtr child = edgesElement->children; child; child = child->next) {
        if (child->type == XML_ELEMENT_NODE && 
            xmlStrcmp(child->name, BAD_CAST "edge") == 0) {
            
            QString id = getXmlProperty(child, "id");
            if (id == edgeIdStr) {
                return child;
            }
        }
    }
    return nullptr;
}

void XmlLiveSync::setXmlProperty(xmlNodePtr node, const QString& name, const QString& value)
{
    if (node && !name.isEmpty()) {
        xmlSetProp(node, BAD_CAST name.toUtf8().constData(), BAD_CAST value.toUtf8().constData());
    }
}

QString XmlLiveSync::getXmlProperty(xmlNodePtr node, const QString& name)
{
    if (!node || name.isEmpty()) return QString();
    
    xmlChar* prop = xmlGetProp(node, BAD_CAST name.toUtf8().constData());
    if (prop) {
        QString result = QString::fromUtf8((char*)prop);
        xmlFree(prop);
        return result;
    }
    return QString();
}