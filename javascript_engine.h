#pragma once

#include <QJSEngine>
#include <QJSValue>
#include <QObject>
#include <QString>
#include <QVariant>
#include <QDebug>

class Node;
class Edge;
class Scene;
class GraphController;
class GraphFactory;

/**
 * JavaScriptEngine - Modern JavaScript integration for NodeGraph
 * 
 * Provides QJSEngine integration for:
 * - Node scripting and behavior logic
 * - Graph processing algorithms
 * - Custom node types in JavaScript
 * - Real-time graph operations
 */
class JavaScriptEngine : public QObject
{
    Q_OBJECT

public:
    explicit JavaScriptEngine(QObject* parent = nullptr);
    ~JavaScriptEngine();

    // JavaScript execution
    QJSValue evaluate(const QString& script);
    QJSValue evaluateFile(const QString& filePath);
    
    // API registration
    void registerNodeAPI(Scene* scene);
    void registerGraphAPI();
    void registerGraphController(Scene* scene, GraphFactory* factory);
    
    // Node scripting support
    QJSValue createNodeScript(const QString& nodeType, const QString& script);
    bool executeNodeScript(Node* node, const QString& script, const QVariantMap& inputs = QVariantMap());
    
    // Graph processing
    QJSValue processGraph(const QString& algorithm, const QVariantMap& parameters = QVariantMap());
    
    // Utility methods
    bool hasErrors() const;
    QString getLastError() const;
    void clearErrors();
    
    // Script management
    void loadScriptModule(const QString& moduleName, const QString& scriptContent);
    QJSValue getModule(const QString& moduleName);

signals:
    void scriptExecuted(const QString& script, const QJSValue& result);
    void scriptError(const QString& error);
    void nodeScriptChanged(const QString& nodeId, const QString& script);

public slots:
    // Console API callbacks
    void qt_console_log(const QString& message);
    void qt_console_error(const QString& message);

private slots:
    void handleJavaScriptException(const QJSValue& exception);

private:
    void setupGlobalAPI();
    void registerConsoleAPI();
    void registerUtilityAPI();
    void loadEnhancedAPIs();
    
    QJSValue nodeToJSValue(Node* node);
    QJSValue edgeToJSValue(Edge* edge);
    
    QJSEngine* m_engine;
    Scene* m_scene;
    GraphController* m_graphController;
    QString m_lastError;
    QMap<QString, QJSValue> m_scriptModules;
};