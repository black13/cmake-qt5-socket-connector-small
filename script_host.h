#pragma once

#if ENABLE_JS

#include <QObject>
#include <QJSEngine>
#include <QJSValue>

class Scene;
class GraphFactory;
class GraphScriptApi;

/**
 * ScriptHost - JavaScript engine wrapper for graph automation
 * 
 * This class owns the QJSEngine and provides safe script evaluation.
 * It registers the GraphScriptApi and handles error reporting.
 * 
 * Only compiled when ENABLE_JS=ON.
 */
class ScriptHost : public QObject
{
    Q_OBJECT
    
public:
    explicit ScriptHost(Scene* scene, GraphFactory* factory, QObject* parent = nullptr);
    ~ScriptHost();
    
    // Script evaluation
    Q_INVOKABLE QJSValue eval(const QString& code);
    Q_INVOKABLE QJSValue evalFile(const QString& filename);
    
    // Engine access for advanced use
    QJSEngine* getEngine() const { return m_engine; }
    
    // Batch wrapper for scripts
    Q_INVOKABLE QJSValue batch(const QJSValue& function);
    
signals:
    // Emitted when script execution completes (success or error)
    void scriptExecuted(const QString& code, bool success, const QString& result);
    void scriptError(const QString& error, int lineNumber);
    
private slots:
    void handleConsoleMessage(const QString& message);
    
private:
    QJSEngine* m_engine;
    GraphScriptApi* m_api;
    
    // Error handling
    bool checkForErrors(const QJSValue& result);
    QString formatError(const QJSValue& error) const;
};

#else

#include <QObject>
#include <QString>

class Scene;
class GraphFactory;

// Stub implementation when JavaScript is disabled
class ScriptHost : public QObject
{
    Q_OBJECT
    
public:
    explicit ScriptHost(Scene*, GraphFactory*, QObject* parent = nullptr)
        : QObject(parent) {}
    
    // Empty methods - no JavaScript functionality (return QString instead of QJSValue)
    Q_INVOKABLE QString eval(const QString&) { return QString("JavaScript disabled"); }
    Q_INVOKABLE QString evalFile(const QString&) { return QString("JavaScript disabled"); }
    Q_INVOKABLE bool batch(const QString&) { return false; }
};

#endif // ENABLE_JS