#include "script_host.h"

#if ENABLE_JS

#include "graph_script_api.h"
#include "scene.h"
#include "graph_factory.h"
#include "graph_observer.h"
#include "xml_autosave_observer.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>

ScriptHost::ScriptHost(Scene* scene, GraphFactory* factory, QObject* parent)
    : QObject(parent)
    , m_engine(new QJSEngine(this))
    , m_api(new GraphScriptApi(scene, factory, this))
{
    Q_ASSERT(scene);
    Q_ASSERT(factory);
    
    // Install console extension for console.log() support
    m_engine->installExtensions(QJSEngine::ConsoleExtension);
    
    // Register the GraphScriptApi as "graph" global object
    QJSValue apiObject = m_engine->newQObject(m_api);
    m_engine->globalObject().setProperty("graph", apiObject);
    
    // Add a batch helper function
    m_engine->evaluate(R"(
        function batch(fn) {
            graph.beginBatch();
            try {
                var result = fn();
                graph.endBatch();
                return result;
            } catch (e) {
                graph.endBatch();
                throw e;
            }
        }
    )");
    
    qDebug() << "ScriptHost: JavaScript engine initialized with graph API";
}

ScriptHost::~ScriptHost()
{
    qDebug() << "ScriptHost: JavaScript engine shutting down";
}

QJSValue ScriptHost::eval(const QString& code)
{
    if (code.trimmed().isEmpty()) {
        return QJSValue();
    }
    
    qDebug() << "ScriptHost: Evaluating JavaScript code";
    
    QJSValue result = m_engine->evaluate(code, QStringLiteral("<console>"));
    
    bool success = !checkForErrors(result);
    QString resultStr = success ? result.toString() : formatError(result);
    
    emit scriptExecuted(code, success, resultStr);
    
    return result;
}

QJSValue ScriptHost::evalFile(const QString& filename)
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString error = QString("Cannot open file: %1").arg(filename);
        qWarning() << "ScriptHost::evalFile:" << error;
        emit scriptError(error, 0);
        return m_engine->newErrorObject(QJSValue::GenericError, error);
    }
    
    QTextStream stream(&file);
    QString code = stream.readAll();
    file.close();
    
    qDebug() << "ScriptHost: Evaluating JavaScript file:" << filename;
    
    QJSValue result = m_engine->evaluate(code, filename);
    
    bool success = !checkForErrors(result);
    QString resultStr = success ? result.toString() : formatError(result);
    
    emit scriptExecuted(code, success, resultStr);
    
    return result;
}

QJSValue ScriptHost::batch(const QJSValue& function)
{
    if (!function.isCallable()) {
        QString error = "batch() requires a callable function";
        qWarning() << "ScriptHost::batch:" << error;
        return m_engine->newErrorObject(QJSValue::TypeError, error);
    }
    
    m_api->beginBatch();
    
    QJSValue result;
    try {
        QJSValue funcCopy = function; // Create non-const copy
        result = funcCopy.call();
        m_api->endBatch();
    } catch (...) {
        m_api->endBatch();
        throw;
    }
    
    return result;
}

void ScriptHost::handleConsoleMessage(const QString& message)
{
    qDebug() << "JS Console:" << message;
}

bool ScriptHost::checkForErrors(const QJSValue& result)
{
    if (result.isError()) {
        QString error = formatError(result);
        int lineNumber = result.property("lineNumber").toInt();
        
        qWarning() << "ScriptHost: JavaScript error at line" << lineNumber << ":" << error;
        emit scriptError(error, lineNumber);
        
        return true;
    }
    
    return false;
}

QString ScriptHost::formatError(const QJSValue& error) const
{
    if (!error.isError()) {
        return error.toString();
    }
    
    QString message = error.property("message").toString();
    QString name = error.property("name").toString();
    int lineNumber = error.property("lineNumber").toInt();
    QString stack = error.property("stack").toString();
    
    QString result = QString("%1: %2").arg(name, message);
    if (lineNumber > 0) {
        result += QString(" (line %1)").arg(lineNumber);
    }
    
    if (!stack.isEmpty()) {
        result += QString("\nStack: %1").arg(stack);
    }
    
    return result;
}

#endif // ENABLE_JS