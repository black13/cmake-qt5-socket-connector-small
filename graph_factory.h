#pragma once

#include <QGraphicsScene>
#include <QUuid>
#include <QString>
#include <QPointF>
#include <QDebug>
#include <QVariantMap>
#include "socket.h"
#include "node_templates.h"

class Node;
class Edge;
class Scene;

// Forward declarations for libxml types (reduces header pollution)
typedef struct _xmlNode xmlNode;
typedef xmlNode* xmlNodePtr;
typedef struct _xmlDoc xmlDoc;
typedef xmlDoc* xmlDocPtr;

/**
 * GraphFactory - XML-First Object Creation with NodeRegistry
 * 
 * Simplified factory that uses NodeRegistry for type-based creation.
 * Enforces XML-first discipline while allowing extensible node types.
 */
class GraphFactory
{
public:
    // Initialize factory with scene and XML document
    GraphFactory(QGraphicsScene* scene, xmlDocPtr xmlDoc);
    
    // XML-first creation methods
    Node* createNodeFromXml(xmlNodePtr xmlNode);
    Edge* createEdgeFromXml(xmlNodePtr xmlEdge);
    
    // UNIFIED CREATION - Template-driven XML-first approach (inputs/outputs ignored - template defines sockets)
    Node* createNode(const QString& nodeType, const QPointF& position, int inputs = -1, int outputs = -1);
    
    Edge* createEdge(Node* fromNode, int fromSocketIndex, Node* toNode, int toSocketIndex);
    
    // Atomic edge connection - enforces proper edge creation
    Edge* connectSockets(Socket* fromSocket, Socket* toSocket);
    
    // Post-load validation
    bool validateGraphIntegrity() const;
    
    // XML file loading
    bool loadFromXmlFile(const QString& filePath);
    
    // Clean design: socket resolution handled by edges internally
    
    // Socket factory method - prevents manual socket creation
    Socket* createSocket(Socket::Role role, Node* parentNode, int index);
    
    // Access to managed scene
    Scene* getScene() const { return qobject_cast<Scene*>(m_scene); }
    
    // Utility to get XML property
    static QString getXmlProperty(xmlNodePtr node, const QString& name);

    // Unified in-memory edge creation by node UUIDs and socket indices
    Edge* connectByIds(const QUuid& fromNodeId, int fromSocketIndex,
                       const QUuid& toNodeId,   int toSocketIndex);

private:
    QGraphicsScene* m_scene;
    xmlDocPtr m_xmlDocument;
    
    // Helper methods
    xmlNodePtr createXmlNode(const QString& nodeType, const QPointF& position, int inputs = 1, int outputs = 1);
    xmlNodePtr createXmlEdgeNodeIndex(const QUuid& fromNodeId, int fromSocketIndex, const QUuid& toNodeId, int toSocketIndex);
    
    // Get nodes/edges parent elements in XML
    xmlNodePtr getNodesElement();
    xmlNodePtr getEdgesElement();
};