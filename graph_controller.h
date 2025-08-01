#pragma once

#include <QObject>
#include <QString>
#include <QVariantMap>
#include <QUuid>

class Scene;
class GraphFactory;
class XmlLiveSync;
class Node;
class Edge;

/**
 * GraphController - JavaScript-accessible graph control interface
 * 
 * Provides runtime control over the node graph system via JavaScript:
 * - Create/delete nodes and edges
 * - Save/load XML 
 * - Query graph statistics
 * - Trigger XML synchronization
 * 
 * This class is exposed to QJSEngine as the "Graph" global object
 * 
 * ARCHITECTURAL RATIONALE: 
 * This class uses QObject inheritance specifically for JavaScript integration.
 * This does NOT contradict the "no QObject" rule for Node/Edge classes, which
 * must remain lightweight QGraphicsItems for performance. GraphController serves
 * as a bridge layer that provides Q_INVOKABLE methods to JavaScript while
 * keeping core graphics objects efficient.
 */
class GraphController : public QObject
{
    Q_OBJECT

public:
    explicit GraphController(Scene* scene, GraphFactory* factory, QObject* parent = nullptr);
    ~GraphController() = default;

public slots:
    // Node operations
    QString createNode(const QString& type, qreal x, qreal y);
    bool deleteNode(const QString& uuid);
    bool moveNode(const QString& uuid, qreal dx, qreal dy);
    QVariantMap getNode(const QString& uuid);
    QVariantList getNodes();
    
    // Edge operations  
    QString connect(const QString& fromNodeId, int fromIndex, 
                   const QString& toNodeId, int toIndex);
    bool deleteEdge(const QString& uuid);
    QVariantList getEdges();
    
    // Graph-wide operations
    void clear();
    void saveXml(const QString& path);
    void loadXml(const QString& path);
    void rebuildXml();
    QString getXmlString();
    QVariantMap getStats();
    
    // Utility functions
    bool isValidNodeType(const QString& type);
    QStringList getValidNodeTypes();
    
signals:
    void nodeCreated(const QString& uuid);
    void nodeDeleted(const QString& uuid);
    void edgeCreated(const QString& uuid);
    void edgeDeleted(const QString& uuid);
    void graphCleared();
    void xmlSaved(const QString& path);
    void xmlLoaded(const QString& path);
    void error(const QString& message);

private:
    Scene* m_scene;
    GraphFactory* m_factory;
    
    // Helper methods
    Node* findNode(const QString& uuid);
    Edge* findEdge(const QString& uuid);
    QVariantMap nodeToVariant(Node* node);
    QVariantMap edgeToVariant(Edge* edge);
};