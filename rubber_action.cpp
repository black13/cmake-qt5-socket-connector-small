#include "rubber_action.h"
#include "javascript_engine.h"
#include "node.h"
#include <QDebug>
#include <QJSValue>

JavaScriptRubberAction::JavaScriptRubberAction(JavaScriptEngine* jsEngine,
                                              const QString& functionName,
                                              const QString& description,
                                              const QString& nodeType)
    : m_jsEngine(jsEngine)
    , m_functionName(functionName)
    , m_description(description)
    , m_nodeType(nodeType)
{
}

void JavaScriptRubberAction::run(Node& node, Context& ctx)
{
    if (!m_jsEngine) {
        ctx.setError("No JavaScript engine available");
        return;
    }
    
    // Create JavaScript objects for node and context
    // This is a simplified implementation - full implementation would
    // need proper JS object wrappers
    QString script = QString("if (typeof %1 === 'function') { %1(node, ctx); }")
                        .arg(m_functionName);
    
    // Execute the JavaScript function
    QJSValue result = m_jsEngine->evaluate(script);
    
    if (result.isError()) {
        ctx.setError(QString("JavaScript action failed: %1").arg(result.toString()));
    }
}

bool JavaScriptRubberAction::isApplicableTo(const QString& nodeType) const
{
    return m_nodeType == "*" || m_nodeType == nodeType;
}

ActionPtr makeJavaScriptAction(JavaScriptEngine* jsEngine,
                              const QString& functionName,
                              const QString& description,
                              const QString& nodeType)
{
    return std::make_shared<JavaScriptRubberAction>(jsEngine, functionName, description, nodeType);
}