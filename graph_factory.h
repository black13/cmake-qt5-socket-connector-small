#pragma once

#include <QGraphicsScene>
#include <QUuid>
#include <QString>
#include <QPointF>
#include <QDebug>
#include <libxml/tree.h>
#include "socket.h"

class Node;
class Edge;
class Scene;  // Forward declaration for custom scene type

/**
 * GraphFactory - XML-First Object Creation with Templates
 *
 * Simplified factory that uses NodeTypeTemplates for type-based creation.
 * Enforces XML-first discipline while allowing extensible node types.
 */
class GraphFactory
{
public:
    // Initialize factory with scene and XML document
    GraphFactory(Scene* scene, xmlDocPtr xmlDoc);

    // XML-first creation methods
    Node* createNodeFromXml(xmlNodePtr xmlNode);

    // Runtime creation (creates XML first, then objects)
    Node* createNode(const QString& nodeType, const QPointF& position, int inputs = 1, int outputs = 1);

    // Utility to get XML property
    static QString getXmlProperty(xmlNodePtr node, const QString& name);

private:
    Scene* m_scene;  // ✅ Type-safe: GraphFactory always requires custom Scene
    xmlDocPtr m_xmlDocument;

    // Helper methods
    xmlNodePtr createXmlNode(const QString& nodeType, const QPointF& position, int inputs = 1, int outputs = 1);
    xmlNodePtr getNodesElement();
};