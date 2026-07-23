#pragma once

#include <QString>
#include <QVariant>
#include <QVariantMap>
#include <memory>
#include <type_traits>
#include <utility>

class QObject;

/**
 * script_engine.h - Runtime-polymorphic script engine as a VALUE TYPE
 *
 * Design follows Sean Parent's runtime-concept idiom ("Inheritance Is The
 * Base Class of Evil" / "Better Code: Runtime Polymorphism"):
 *
 * - ScriptEngine and ScriptFunction are copyable value types. Client code
 *   (Graph, ScriptedNode, Window) holds them by value and never sees a
 *   pointer, a virtual function, or a backend header.
 *
 * - Polymorphism is an IMPLEMENTATION DETAIL: the Concept/Model pair is
 *   private. Backends (QJsBackend, DuktapeBackend, a future Lua/Python
 *   backend, a mock for tests) are plain classes that satisfy the concept
 *   structurally - they inherit from nothing and include nothing of ours.
 *
 * - The interface speaks only vocabulary types (QString, QVariant,
 *   QVariantMap, QObject*), so no engine-specific type (QJSValue,
 *   duk_context) can leak into client code.
 *
 * Deviation from Parent's photoshop example: a script engine is inherently
 * stateful (globals, registered host objects), so copies of a ScriptEngine
 * are handles sharing one backend instance rather than independent values.
 * ScriptFunction shares ownership of its engine's heap, so a compiled
 * function keeps the interpreter alive - lifetime is structural, not managed
 * by the caller.
 *
 * A backend B must provide:
 *   ScriptResult evaluate(const QString& code, const QString& fileName);
 *   <callable>   compile(const QString& functionBody, QString* error);
 *                  where <callable> has: ScriptResult call(QObject* api,
 *                                                    const QVariantMap& ctx);
 *   void         registerObject(const QString& name, QObject* object);
 *   void         interrupt();   (may be a no-op - see interrupt() below)
 *   QString      name() const;
 */

/// Outcome of any script operation. error.isEmpty() means success.
struct ScriptResult
{
    QVariant value;
    QString error;

    [[nodiscard]] bool ok() const { return error.isEmpty(); }
};

/**
 * ScriptFunction - type-erased compiled script function.
 *
 * Scripts are compiled to the canonical signature function(node, context):
 * `node` is a host API object (bridged per backend), `context` a plain map.
 */
class ScriptFunction
{
public:
    ScriptFunction() = default;

    template <typename F,
              typename = std::enable_if_t<!std::is_same_v<std::decay_t<F>, ScriptFunction>>>
    ScriptFunction(F fn)
        : m_self(std::make_shared<Model<F>>(std::move(fn)))
    {
    }

    [[nodiscard]] bool isValid() const { return static_cast<bool>(m_self); }

    ScriptResult call(QObject* api, const QVariantMap& context) const
    {
        if (!m_self) {
            return {QVariant(), QStringLiteral("ScriptFunction: not compiled")};
        }
        return m_self->call_(api, context);
    }

private:
    struct Concept
    {
        virtual ~Concept() = default;
        virtual ScriptResult call_(QObject* api, const QVariantMap& context) = 0;
    };

    template <typename F>
    struct Model final : Concept
    {
        explicit Model(F fn) : m_fn(std::move(fn)) {}
        ScriptResult call_(QObject* api, const QVariantMap& context) override
        {
            return m_fn.call(api, context);
        }
        F m_fn;
    };

    std::shared_ptr<Concept> m_self;
};

/**
 * ScriptEngine - type-erased script engine handle.
 *
 * Construct from any backend value: ScriptEngine(QJsBackend()).
 * Default-constructed engine is "null"; all operations fail gracefully.
 */
class ScriptEngine
{
public:
    ScriptEngine() = default;

    template <typename B,
              typename = std::enable_if_t<!std::is_same_v<std::decay_t<B>, ScriptEngine>>>
    ScriptEngine(B backend)
        : m_self(std::make_shared<Model<B>>(std::move(backend)))
    {
    }

    explicit operator bool() const { return static_cast<bool>(m_self); }

    /// Evaluate a script in the global scope.
    ScriptResult evaluate(const QString& code, const QString& fileName = QString()) const
    {
        if (!m_self) {
            return {QVariant(), QStringLiteral("ScriptEngine: no backend")};
        }
        return m_self->evaluate_(code, fileName);
    }

    /// Compile a function body with signature function(node, context) { body }.
    /// On failure returns an invalid ScriptFunction and sets *error if given.
    ScriptFunction compile(const QString& functionBody, QString* error = nullptr) const
    {
        if (!m_self) {
            if (error) *error = QStringLiteral("ScriptEngine: no backend");
            return {};
        }
        return m_self->compile_(functionBody, error);
    }

    /// Expose a QObject's public slots/Q_INVOKABLEs as a script global.
    void registerObject(const QString& globalName, QObject* object) const
    {
        if (m_self && object) {
            m_self->registerObject_(globalName, object);
        }
    }

    /// Request interruption of any currently running evaluation (e.g. from a
    /// watchdog thread when a script spins in an infinite loop). May be called
    /// from any thread. Backends without interruption support implement this
    /// as a no-op. Backends reset the flag before each new evaluation, so the
    /// next script run starts clean.
    void interrupt() const
    {
        if (m_self) {
            m_self->interrupt_();
        }
    }

    [[nodiscard]] QString backendName() const
    {
        return m_self ? m_self->backendName_() : QStringLiteral("null");
    }

private:
    struct Concept
    {
        virtual ~Concept() = default;
        virtual ScriptResult evaluate_(const QString& code, const QString& fileName) = 0;
        virtual ScriptFunction compile_(const QString& functionBody, QString* error) = 0;
        virtual void registerObject_(const QString& globalName, QObject* object) = 0;
        virtual void interrupt_() = 0;
        virtual QString backendName_() const = 0;
    };

    template <typename B>
    struct Model final : Concept
    {
        explicit Model(B backend) : m_backend(std::move(backend)) {}

        ScriptResult evaluate_(const QString& code, const QString& fileName) override
        {
            return m_backend.evaluate(code, fileName);
        }

        ScriptFunction compile_(const QString& functionBody, QString* error) override
        {
            QString localError;
            auto compiled = m_backend.compile(functionBody, &localError);
            if (error) {
                *error = localError;
            }
            if (!localError.isEmpty()) {
                return {};
            }
            return ScriptFunction(std::move(compiled));
        }

        void registerObject_(const QString& globalName, QObject* object) override
        {
            m_backend.registerObject(globalName, object);
        }

        void interrupt_() override { m_backend.interrupt(); }

        QString backendName_() const override { return m_backend.name(); }

        B m_backend;
    };

    std::shared_ptr<Concept> m_self;
};
