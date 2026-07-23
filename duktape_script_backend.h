#pragma once

#ifdef NODEGRAPH_HAS_DUKTAPE

#include "script_engine.h"
#include <memory>

// Forward declaration of duktape's context (duk_context is a typedef of this)
struct duk_hthread;

/**
 * DuktapeBackend - Duktape (embedded ECMAScript) backend for ScriptEngine
 *
 * Like QJsBackend this is a plain regular type: no base class, satisfies the
 * ScriptEngine concept structurally. Copies share one Duktape heap; compiled
 * functions co-own the heap (stored in the global stash) so they stay valid
 * for as long as any handle exists.
 *
 * Host objects are exposed through a QMetaObject bridge: every public slot /
 * Q_INVOKABLE of a registered QObject becomes a callable property, arguments
 * and return values are marshalled through QVariant. This means the same
 * `graph` / `node` script API works here as under QJSEngine - no script
 * changes when switching engines.
 *
 * Enable with -DENABLE_DUKTAPE=ON at configure time, select at runtime with
 * NODEGRAPH_SCRIPT_ENGINE=duktape.
 */
class DuktapeBackend
{
public:
    DuktapeBackend();

    ScriptResult evaluate(const QString& code, const QString& fileName);

    /// Compiled callable for ScriptFunction (structural: has .call(api, ctx)).
    class Compiled
    {
    public:
        Compiled(std::shared_ptr<duk_hthread> ctx, quint32 stashKey, bool valid);
        ~Compiled(); // in .cpp: StashSlot is incomplete here
        ScriptResult call(QObject* api, const QVariantMap& context);

    private:
        // Shared ownership of the function's global-stash slot: when the last
        // Compiled copy dies, the stash entry is deleted (previously every
        // compile leaked one stash entry for the life of the heap).
        struct StashSlot;
        std::shared_ptr<duk_hthread> m_ctx; // keeps the heap alive
        std::shared_ptr<StashSlot> m_slot;  // null when !m_valid
        bool m_valid;
    };

    Compiled compile(const QString& functionBody, QString* error);

    void registerObject(const QString& globalName, QObject* object);

    // No-op: duk_interrupt is not wired up; the QJS backend (the default
    // engine) implements real interruption. Required by the ScriptEngine
    // concept - leaving it a no-op is deliberate, not an omission.
    void interrupt() {}

    [[nodiscard]] QString name() const { return QStringLiteral("duktape"); }

private:
    std::shared_ptr<duk_hthread> m_ctx;
    quint32 m_nextStashKey = 0;
};

#endif // NODEGRAPH_HAS_DUKTAPE
