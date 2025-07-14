#pragma once

#include "graph_observer.h"
#include <libxml/tree.h>
#include <QHash>
#include <QUuid>
#include <QString>
#include <QPointF>

class Scene;
class Node;
class Edge;

/**
 * XmlLiveSync - Real-time XML Document Synchronization
 * 
 * Maintains perfect synchronization between Scene state and XML document.
 * Unlike XmlAutosaveObserver (which saves to files), this keeps the 
 * in-memory XML document live and current for:
 * 
 * - Fast file saves (no need to rebuild XML)
 * - Undo/Redo system foundation  
 * - State consistency across sessions
 * - Real-time collaboration (future)
 * 
 * Inkscape-style architecture: XML is the source of truth.
 */
class XmlLiveSync : public GraphObserver
{
public:
    explicit XmlLiveSync(Scene* scene, xmlDocPtr xmlDocument);
    ~XmlLiveSync();
    
    // Enable/disable live sync
    void setEnabled(bool enabled);
    bool isEnabled() const { return m_enabled; }
    
    // Force full XML rebuild (for initialization or recovery)
    void rebuildXmlFromScene();
    
    // Get current XML document (always up-to-date)
    xmlDocPtr getXmlDocument() const { return m_xmlDocument; }
    
    // Fast save to file (XML already current)
    bool saveToFile(const QString& filename);
    
    // GraphObserver interface - real-time XML updates
    void onNodeAdded(const Node& node) override;
    void onNodeRemoved(const QUuid& nodeId) override;
    void onNodeMoved(const QUuid& nodeId, QPointF oldPos, QPointF newPos) override;
    void onEdgeAdded(const Edge& edge) override;
    void onEdgeRemoved(const QUuid& edgeId) override;
    void onGraphCleared() override;

private:
    Scene* m_scene;
    xmlDocPtr m_xmlDocument;
    bool m_enabled;
    
    // Cache XML nodes for fast updates
    QHash<QUuid, xmlNodePtr> m_nodeXmlCache;
    QHash<QUuid, xmlNodePtr> m_edgeXmlCache;
    
    // XML structure helpers
    xmlNodePtr getOrCreateNodesElement();
    xmlNodePtr getOrCreateEdgesElement();
    xmlNodePtr findNodeXml(const QUuid& nodeId);
    xmlNodePtr findEdgeXml(const QUuid& edgeId);
    
    // Incremental XML updates
    void updateNodePositionInXml(const QUuid& nodeId, const QPointF& newPos);
    void addNodeToXml(const Node& node);
    void removeNodeFromXml(const QUuid& nodeId);
    void addEdgeToXml(const Edge& edge);
    void removeEdgeFromXml(const QUuid& edgeId);
    
    // XML utilities
    void setXmlProperty(xmlNodePtr node, const QString& name, const QString& value);
    QString getXmlProperty(xmlNodePtr node, const QString& name);
};