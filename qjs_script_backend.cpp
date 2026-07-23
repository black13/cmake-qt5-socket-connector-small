#include "qjs_script_backend.h"

#include <QJSEngine>
#include <QJSValueList>
#include <QQmlEngine>

namespace {

ScriptResult resultFromJsValue(const QJSValue& value)
{
    if (value.isError()) {
        return {QVariant(),
                QStringLiteral("line %1: %2")
                    .arg(value.property(QStringLiteral("lineNumber")).toInt())
                    .arg(value.toString())};
    }
    return {value.toVariant(), QString()};
}

} // namespace

QJsBackend::QJsBackend()
    : m_engine(std::make_shared<QJSEngine>())
{
}

ScriptResult QJsBackend::evaluate(const QString& code, const QString& fileName)
{
    m_engine->setInterrupted(false); // start every evaluation clean
    return resultFromJsValue(m_engine->evaluate(code, fileName));
}

QJsBackend::Compiled::Compiled(std::shared_ptr<QJSEngine> engine, QJSValue function)
    : m_engine(std::move(engine))
    , m_function(std::move(function))
{
}

ScriptResult QJsBackend::Compiled::call(QObject* api, const QVariantMap& context)
{
    if (!m_function.isCallable()) {
        return {QVariant(), QStringLiteral("compiled function is not callable")};
    }

    m_engine->setInterrupted(false); // start every call clean

    // The api object is owned by the caller (often stack-allocated); make that
    // explicit so the JS garbage collector never tries to delete it.
    QQmlEngine::setObjectOwnership(api, QQmlEngine::CppOwnership);

    QJSValueList args;
    args << m_engine->newQObject(api)
         << m_engine->toScriptValue(context);

    return resultFromJsValue(m_function.call(args));
}

QJsBackend::Compiled QJsBackend::compile(const QString& functionBody, QString* error)
{
    const QString wrapped =
        QStringLiteral("(function(node, context) {\n%1\n})").arg(functionBody);

    // Clear any stale watchdog interrupt: this path evaluates directly via
    // m_engine (bypassing the reset in evaluate()/Compiled::call), and without
    // a reset the first compile after an interrupted script fails spuriously.
    m_engine->setInterrupted(false);

    QJSValue function = m_engine->evaluate(wrapped, QStringLiteral("scripted_node.js"));

    if (error) {
        if (function.isError()) {
            *error = QStringLiteral("line %1: %2")
                         .arg(function.property(QStringLiteral("lineNumber")).toInt())
                         .arg(function.toString());
        } else if (!function.isCallable()) {
            *error = QStringLiteral("script did not compile to a callable function");
        } else {
            error->clear();
        }
    }

    return Compiled(m_engine, function);
}

void QJsBackend::registerObject(const QString& globalName, QObject* object)
{
    QQmlEngine::setObjectOwnership(object, QQmlEngine::CppOwnership);
    m_engine->globalObject().setProperty(globalName, m_engine->newQObject(object));
}

void QJsBackend::interrupt()
{
    // QJSEngine::setInterrupted is documented as callable from any thread;
    // the engine aborts the running evaluation at the next opportunity.
    m_engine->setInterrupted(true);
}
