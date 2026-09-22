#include "qjs_script_backend.h"

#include <QDebug>
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

/// JS factory that wraps a host QObject: structure-taking methods get an
/// iterative (non-recursive), depth- and node-capped argument check. Without
/// this, QJSEngine's own QJSValue -> QVariantMap conversion recurses on the
/// native stack and a ~1000-deep object crashes the process
/// (STATUS_STACK_OVERFLOW) before any C++ validation can run.
const char* const kDepthGuardSource = R"JS(
(function(api) {
    var MAX_DEPTH = 512;
    var MAX_NODES = 100000;
    function tooDeep(value) {
        if (value === null || (typeof value !== 'object' && typeof value !== 'function')) {
            return false;
        }
        var stack = [value];
        var depths = [0];
        var nodes = 0;
        while (stack.length > 0) {
            var v = stack.pop();
            var d = depths.pop();
            if (++nodes > MAX_NODES || d > MAX_DEPTH) {
                return true; // also stops cyclic structures at the caps
            }
            if (v === null || (typeof v !== 'object' && typeof v !== 'function')) {
                continue;
            }
            var keys = Object.keys(v);
            for (var i = 0; i < keys.length; ++i) {
                stack.push(v[keys[i]]);
                depths.push(d + 1);
            }
        }
        return false;
    }
    var guardedNames = ['setPayload', 'setPayloadValue', 'runWork',
                        'setNodePayload', 'executeNodeScript', 'runSyntheticWork'];
    var wrapper = Object.create(api);
    for (var i = 0; i < guardedNames.length; ++i) {
        var name = guardedNames[i];
        if (typeof api[name] !== 'function') {
            continue;
        }
        // defineProperty, not assignment: the inherited QObject method is
        // read-only, so `wrapper[name] = fn` throws after Object.create(api).
        Object.defineProperty(wrapper, name, {
            value: (function(methodName, method) {
                return function() {
                    for (var a = 0; a < arguments.length; ++a) {
                        if (tooDeep(arguments[a])) {
                            throw new Error(methodName +
                                ': argument is too large or too deeply nested (max depth ' +
                                MAX_DEPTH + ')');
                        }
                    }
                    return method.apply(api, arguments);
                };
            })(name, api[name]),
            writable: true,
            enumerable: false,
            configurable: true
        });
    }
    return wrapper;
})
)JS";

QJSValue wrapApi(QJSEngine* engine, QJSValue& guard, QObject* object)
{
    if (guard.isCallable()) {
        QJSValueList args;
        args << engine->newQObject(object);
        const QJSValue wrapped = guard.call(args); // QJSValue::call is non-const
        if (wrapped.isObject() && !wrapped.isError()) {
            return wrapped;
        }
        qWarning() << "wrapApi: guard fallback, isObject=" << wrapped.isObject()
                   << "isError=" << wrapped.isError() << wrapped.toString();
    } else {
        qWarning() << "wrapApi: guard missing, isError=" << guard.isError()
                   << "isCallable=" << guard.isCallable() << guard.toString();
    }
    return engine->newQObject(object);
}

} // namespace

QJsBackend::QJsBackend()
    : m_engine(std::make_shared<QJSEngine>())
{
    m_guard = m_engine->evaluate(QString::fromLatin1(kDepthGuardSource),
                                 QStringLiteral("nodegraph_guard.js"));
    if (m_guard.isError()) {
        qWarning() << "QJsBackend: depth guard failed to install:"
                   << m_guard.toString();
    }
}

ScriptResult QJsBackend::evaluate(const QString& code, const QString& fileName)
{
    m_engine->setInterrupted(false); // start every evaluation clean
    return resultFromJsValue(m_engine->evaluate(code, fileName));
}

QJsBackend::Compiled::Compiled(std::shared_ptr<QJSEngine> engine, QJSValue guard, QJSValue function)
    : m_engine(std::move(engine))
    , m_guard(std::move(guard))
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
    args << wrapApi(m_engine.get(), m_guard, api)
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

    return Compiled(m_engine, m_guard, function);
}

void QJsBackend::registerObject(const QString& globalName, QObject* object)
{
    QQmlEngine::setObjectOwnership(object, QQmlEngine::CppOwnership);
    m_engine->globalObject().setProperty(globalName,
                                         wrapApi(m_engine.get(), m_guard, object));
}

void QJsBackend::interrupt()
{
    // QJSEngine::setInterrupted is documented as callable from any thread;
    // the engine aborts the running evaluation at the next opportunity.
    m_engine->setInterrupted(true);
}
