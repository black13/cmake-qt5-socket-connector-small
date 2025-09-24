#pragma once

// Define ENABLE_JS if not already defined
#ifndef ENABLE_JS
#define ENABLE_JS 0
#endif

#if ENABLE_JS

#include <QObject>
#include <QUuid>
#include <QJSValue>

class Scene;
class GraphFactory;

/**
 * GraphScriptApi - Safe JavaScript façade for graph operations
 * 
 * This class provides a controlled API surface for JavaScript automation.
 * It calls existing C++ methods and returns only IDs/simple values.
 * No raw Node/Edge QObjects are exposed to JavaScript.
 * 
 * Only compiled when ENABLE_JS=ON.
 */
class GraphScriptApi : public QObject
{
    Q_OBJECT
    
public:
    explicit GraphScriptApi(Scene* scene, GraphFactory* factory, QObject* parent = nullptr);
    
    // Public JS API - exposed via Q_INVOKABLE
public slots:
    // Node creation - returns UUID string or empty on failure
    Q_INVOKABLE QString createNode(const QString& type, double x, double y);
    
    // Edge creation - returns true on success
    Q_INVOKABLE bool connect(const QString& fromNodeId, int fromSocketIndex,
                            const QString& toNodeId, int toSocketIndex);
    
    // Deletion operations
    Q_INVOKABLE bool deleteNode(const QString& nodeId);
    Q_INVOKABLE bool deleteEdge(const QString& edgeId);
    
    // Batch operations for performance
    Q_INVOKABLE void beginBatch();
    Q_INVOKABLE void endBatch();
    
    // Query operations - return arrays of ID strings
    Q_INVOKABLE QJSValue getSelectedNodes();
    Q_INVOKABLE QJSValue getAllNodes();
    Q_INVOKABLE QJSValue getAllEdges();
    
    // Graph state operations
    Q_INVOKABLE bool saveNow();
    Q_INVOKABLE void clearGraph();
    
    // Diagnostics
    Q_INVOKABLE QJSValue getGraphStats();
    
private:
    Scene* m_scene;
    GraphFactory* m_factory;
    bool m_batchMode;
    
    // Helper methods
    QUuid parseUuid(const QString& uuidStr) const;
    QString formatUuid(const QUuid& uuid) const;
};

#endif // ENABLE_JS
