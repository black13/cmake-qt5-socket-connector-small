#pragma once

#include "script_engine.h"
#include <QJSValue>
#include <memory>

class QJSEngine;

/**
 * QJsBackend - Qt QJSEngine backend for ScriptEngine
 *
 * A regular (copyable) type with no base class: it satisfies the ScriptEngine
 * concept structurally. Copies share one QJSEngine heap via shared_ptr, and
 * compiled functions co-own the heap so they can outlive the backend handle.
 *
 * QObject registration uses QJSEngine::newQObject with explicit C++ ownership,
 * so the meta-object system exposes the full slot/Q_INVOKABLE surface for free.
 */
class QJsBackend
{
public:
    QJsBackend();

    ScriptResult evaluate(const QString& code, const QString& fileName);

    /// Compiled callable for ScriptFunction (structural: has .call(api, ctx)).
    class Compiled
    {
    public:
        Compiled(std::shared_ptr<QJSEngine> engine, QJSValue function);
        ScriptResult call(QObject* api, const QVariantMap& context);

    private:
        std::shared_ptr<QJSEngine> m_engine; // keeps the heap alive
        QJSValue m_function;
    };

    Compiled compile(const QString& functionBody, QString* error);

    void registerObject(const QString& globalName, QObject* object);

    /// Thread-safe interrupt request (QJSEngine::setInterrupted). The flag is
    /// cleared at the start of every evaluate()/Compiled::call.
    void interrupt();

    [[nodiscard]] QString name() const { return QStringLiteral("qjs"); }

private:
    std::shared_ptr<QJSEngine> m_engine;
};
