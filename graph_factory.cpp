#include "graph_factory.h"
#include "node.h"
#include "socket.h"
#include "edge.h"
#include "scene.h"
#include "node_templates.h"  // Template system - NO NodeRegistry needed
#include "graph_observer.h"
#include <QDateTime>
#include <QDebug>

GraphFactory::GraphFactory(Scene* scene, xmlDocPtr xmlDoc)
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
    
    // Get node type from XML (for logging only)
    QString nodeType = getXmlProperty(xmlNode, "type");
    if (nodeType.isEmpty()) {
        qWarning() << "GraphFactory::createNodeFromXml - missing type attribute";
        return nullptr;
    }

    // Validate node type against template system (no NodeRegistry needed)
    if (!NodeTypeTemplates::hasNodeType(nodeType)) {
        qCritical() << "GraphFactory::createNodeFromXml - Invalid node type:" << nodeType;
        qCritical() << "Available types:" << NodeTypeTemplates::getAvailableTypes();
        return nullptr;
    }

    // Create node directly - template system is the authority
    Node* node = new Node();
    if (!node) {
        qCritical() << "GraphFactory::createNodeFromXml - failed to create node";
        return nullptr;
    }

    // Set node type BEFORE reading XML
    node->setNodeType(nodeType);

    // Attach factory pointer before reading XML - contract requirement
    node->setFactory(this);

    // Let the node read its XML and configure itself
    node->read(xmlNode);

    // Verify factory pointer is still attached
    if (!node->hasFactory()) {
        qCritical() << "GraphFactory::createNodeFromXml - factory detached during read";
        delete node;
        return nullptr;
    }
    
    // Add to scene's typed collection
    m_scene->addNode(node);
    
    qDebug() << "GraphFactory: Created node from XML, type:" << nodeType 
             << "id:" << node->getId().toString(QUuid::WithoutBraces).left(8);
    
    return node;
}


Node* GraphFactory::createNode(const QString& nodeType, const QPointF& position, int inputs, int outputs)
{
    if (!m_xmlDocument) {
        qCritical() << "GraphFactory::createNode - no XML document";
        return nullptr;
    }
    
    // Create XML node first with socket configuration
    xmlNodePtr xmlNode = createXmlNode(nodeType, position, inputs, outputs);
    if (!xmlNode) {
        qCritical() << "GraphFactory::createNode - failed to create XML node";
        return nullptr;
    }
    
    // Create object from XML
    return createNodeFromXml(xmlNode);
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

