#include "duktape_script_backend.h"

#ifdef NODEGRAPH_HAS_DUKTAPE

#include "duktape.h"

#include <QDebug>
#include <QMetaMethod>
#include <QMetaObject>
#include <QSet>
#include <QVariantList>
#include <QVariantMap>

// ============================================================================
// QVariant <-> Duktape stack marshalling
// ============================================================================

namespace {

void pushVariant(duk_context* ctx, const QVariant& value);

void pushVariantMap(duk_context* ctx, const QVariantMap& map)
{
    duk_push_object(ctx);
    for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
        pushVariant(ctx, it.value());
        duk_put_prop_string(ctx, -2, it.key().toUtf8().constData());
    }
}

void pushVariantList(duk_context* ctx, const QVariantList& list)
{
    const duk_idx_t arr = duk_push_array(ctx);
    for (int i = 0; i < list.size(); ++i) {
        pushVariant(ctx, list.at(i));
        duk_put_prop_index(ctx, arr, static_cast<duk_uarridx_t>(i));
    }
}

void pushVariant(duk_context* ctx, const QVariant& value)
{
    switch (value.userType()) {
    case QMetaType::UnknownType:
        duk_push_undefined(ctx);
        break;
    case QMetaType::Nullptr:
        duk_push_null(ctx);
        break;
    case QMetaType::Bool:
        duk_push_boolean(ctx, value.toBool());
        break;
    case QMetaType::Int:
    case QMetaType::UInt:
    case QMetaType::Long:
    case QMetaType::ULong:
    case QMetaType::LongLong:
    case QMetaType::ULongLong:
    case QMetaType::Float:
    case QMetaType::Double:
        duk_push_number(ctx, value.toDouble());
        break;
    case QMetaType::QVariantList:
        pushVariantList(ctx, value.toList());
        break;
    case QMetaType::QStringList:
        pushVariantList(ctx, value.toList());
        break;
    case QMetaType::QVariantMap:
        pushVariantMap(ctx, value.toMap());
        break;
    default: {
        // QString and everything else that stringifies
        const QByteArray utf8 = value.toString().toUtf8();
        duk_push_lstring(ctx, utf8.constData(), static_cast<duk_size_t>(utf8.size()));
        break;
    }
    }
}

QVariant getVariant(duk_context* ctx, duk_idx_t idx, int depth = 0)
{
    // Depth guard: cyclic JS objects (var o = {}; o.me = o;) would otherwise
    // recurse until the native stack overflows. Beyond the limit we emit a
    // marker string instead of recursing.
    if (depth > 32) {
        return QStringLiteral("[max conversion depth]");
    }
    idx = duk_normalize_index(ctx, idx);
    switch (duk_get_type(ctx, idx)) {
    case DUK_TYPE_BOOLEAN:
        return bool(duk_get_boolean(ctx, idx));
    case DUK_TYPE_NUMBER:
        return duk_get_number(ctx, idx);
    case DUK_TYPE_STRING: {
        duk_size_t len = 0;
        const char* str = duk_get_lstring(ctx, idx, &len);
        return QString::fromUtf8(str, static_cast<int>(len));
    }
    case DUK_TYPE_OBJECT:
        if (duk_is_array(ctx, idx)) {
            QVariantList list;
            const duk_size_t n = duk_get_length(ctx, idx);
            list.reserve(static_cast<int>(n));
            for (duk_size_t i = 0; i < n; ++i) {
                duk_get_prop_index(ctx, idx, static_cast<duk_uarridx_t>(i));
                list.append(getVariant(ctx, -1, depth + 1));
                duk_pop(ctx);
            }
            return list;
        }
        if (duk_is_function(ctx, idx)) {
            return {};
        }
        {
            QVariantMap map;
            duk_enum(ctx, idx, DUK_ENUM_OWN_PROPERTIES_ONLY);
            while (duk_next(ctx, -1, 1 /* get value too */)) {
                const QString key = QString::fromUtf8(duk_get_string(ctx, -2));
                map.insert(key, getVariant(ctx, -1, depth + 1));
                duk_pop_2(ctx);
            }
            duk_pop(ctx); // enum
            return map;
        }
    case DUK_TYPE_UNDEFINED:
    case DUK_TYPE_NULL:
    default:
        return {};
    }
}

// ============================================================================
// QMetaObject bridge: expose public slots / Q_INVOKABLEs as script functions
// ============================================================================

/// Dynamic invoke by name; overloads resolved by argument count + convertibility.
QVariant invokeBridged(QObject* object, const QByteArray& methodName,
                       const QVariantList& args, QString* error)
{
    const QMetaObject* mo = object->metaObject();
    bool sawName = false;

    for (int i = 0; i < mo->methodCount(); ++i) {
        const QMetaMethod method = mo->method(i);
        if (method.name() != methodName) {
            continue;
        }
        sawName = true;
        if (method.parameterCount() != args.size() || args.size() > 10) {
            continue;
        }
        if (method.access() != QMetaMethod::Public ||
            method.methodType() == QMetaMethod::Signal ||
            method.methodType() == QMetaMethod::Constructor) {
            continue;
        }

        // Convert arguments to the declared parameter types (stable storage)
        QVector<QVariant> converted(args.size());
        bool convertible = true;
        for (int p = 0; p < args.size(); ++p) {
            converted[p] = args.at(p);
            const int targetType = method.parameterType(p);
            if (targetType == QMetaType::QVariant) {
                continue; // parameter takes QVariant as-is
            }
            if (converted[p].userType() != targetType && !converted[p].convert(targetType)) {
                convertible = false;
                break;
            }
        }
        if (!convertible) {
            continue; // try next overload
        }

        QGenericArgument ga[10];
        for (int p = 0; p < args.size(); ++p) {
            if (method.parameterType(p) == QMetaType::QVariant) {
                ga[p] = QGenericArgument("QVariant", &converted[p]);
            } else {
                ga[p] = QGenericArgument(converted[p].typeName(), converted[p].constData());
            }
        }

        const int returnType = method.returnType();
        bool ok = false;
        QVariant result;
        if (returnType == QMetaType::Void) {
            ok = method.invoke(object, Qt::DirectConnection,
                               ga[0], ga[1], ga[2], ga[3], ga[4],
                               ga[5], ga[6], ga[7], ga[8], ga[9]);
        } else if (returnType == QMetaType::QVariant) {
            ok = method.invoke(object, Qt::DirectConnection,
                               Q_RETURN_ARG(QVariant, result),
                               ga[0], ga[1], ga[2], ga[3], ga[4],
                               ga[5], ga[6], ga[7], ga[8], ga[9]);
        } else {
            result = QVariant(returnType, nullptr);
            QGenericReturnArgument ret(method.typeName(), result.data());
            ok = method.invoke(object, Qt::DirectConnection, ret,
                               ga[0], ga[1], ga[2], ga[3], ga[4],
                               ga[5], ga[6], ga[7], ga[8], ga[9]);
        }

        if (!ok && error) {
            *error = QStringLiteral("invoke failed for %1").arg(QString::fromUtf8(methodName));
        }
        return result;
    }

    if (error) {
        *error = sawName
            ? QStringLiteral("no %1 overload takes %2 argument(s)")
                  .arg(QString::fromUtf8(methodName)).arg(args.size())
            : QStringLiteral("no such method: %1").arg(QString::fromUtf8(methodName));
    }
    return {};
}

/// Duktape C dispatcher; target QObject and method name ride on hidden props.
duk_ret_t methodDispatcher(duk_context* ctx)
{
    const duk_idx_t nargs = duk_get_top(ctx);
    QVariantList args;
    args.reserve(nargs);
    for (duk_idx_t i = 0; i < nargs; ++i) {
        args.append(getVariant(ctx, i));
    }

    duk_push_current_function(ctx);
    duk_get_prop_string(ctx, -1, DUK_HIDDEN_SYMBOL("qobject"));
    QObject* object = static_cast<QObject*>(duk_get_pointer(ctx, -1));
    duk_get_prop_string(ctx, -2, DUK_HIDDEN_SYMBOL("method"));
    const QByteArray methodName(duk_get_string(ctx, -1));
    duk_pop_3(ctx);

    if (!object) {
        return DUK_RET_REFERENCE_ERROR;
    }

    QString error;
    const QVariant result = invokeBridged(object, methodName, args, &error);
    if (!error.isEmpty()) {
        // Details to the log; a plain TypeError to the script (avoids
        // longjmp skipping C++ destructors of locals holding resources)
        qWarning() << "DuktapeBackend: bridged call failed:" << error;
        return DUK_RET_TYPE_ERROR;
    }

    pushVariant(ctx, result);
    return 1;
}

/// Push a plain JS object whose properties dispatch into the QObject.
void pushQObjectBridge(duk_context* ctx, QObject* object)
{
    duk_push_object(ctx);
    const QMetaObject* mo = object->metaObject();
    QSet<QByteArray> seen;

    // Skip QObject's own methods (deleteLater etc.) - start past the base
    for (int i = QObject::staticMetaObject.methodCount(); i < mo->methodCount(); ++i) {
        const QMetaMethod method = mo->method(i);
        if (method.access() != QMetaMethod::Public) {
            continue;
        }
        if (method.methodType() != QMetaMethod::Slot &&
            method.methodType() != QMetaMethod::Method) {
            continue;
        }
        const QByteArray name = method.name();
        if (seen.contains(name)) {
            continue; // one dispatcher per name; overloads resolved at call time
        }
        seen.insert(name);

        duk_push_c_function(ctx, methodDispatcher, DUK_VARARGS);
        duk_push_pointer(ctx, object);
        duk_put_prop_string(ctx, -2, DUK_HIDDEN_SYMBOL("qobject"));
        duk_push_string(ctx, name.constData());
        duk_put_prop_string(ctx, -2, DUK_HIDDEN_SYMBOL("method"));
        duk_put_prop_string(ctx, -2, name.constData());
    }
}

ScriptResult errorFromStackTop(duk_context* ctx)
{
    ScriptResult result;
    result.error = QString::fromUtf8(duk_safe_to_string(ctx, -1));
    duk_pop(ctx);
    return result;
}

} // namespace

// ============================================================================
// DuktapeBackend::Compiled::StashSlot
// ============================================================================

struct DuktapeBackend::Compiled::StashSlot
{
    StashSlot(std::shared_ptr<duk_hthread> c, quint32 k)
        : ctx(std::move(c)), key(k) {}
    ~StashSlot()
    {
        // Delete the function's global-stash entry when the last Compiled
        // copy dies (previously every compile leaked one stash entry for the
        // life of the heap). Runs wherever the last reference drops - in
        // practice the UI thread, where all ScriptedNodes live and die.
        if (ctx) {
            duk_context* raw = ctx.get();
            duk_push_global_stash(raw);
            duk_del_prop_index(raw, -1, static_cast<duk_uarridx_t>(key));
            duk_pop(raw);
        }
    }

    std::shared_ptr<duk_hthread> ctx;
    quint32 key;
};

// ============================================================================
// DuktapeBackend
// ============================================================================

DuktapeBackend::DuktapeBackend()
    : m_ctx(duk_create_heap_default(),
            [](duk_hthread* ctx) { if (ctx) duk_destroy_heap(ctx); })
{
}

ScriptResult DuktapeBackend::evaluate(const QString& code, const QString& fileName)
{
    Q_UNUSED(fileName)
    duk_context* ctx = m_ctx.get();

    const QByteArray utf8 = code.toUtf8();
    if (duk_peval_lstring(ctx, utf8.constData(),
                          static_cast<duk_size_t>(utf8.size())) != 0) {
        return errorFromStackTop(ctx);
    }

    ScriptResult result{getVariant(ctx, -1), QString()};
    duk_pop(ctx);
    return result;
}

DuktapeBackend::Compiled::Compiled(std::shared_ptr<duk_hthread> ctx,
                                   quint32 stashKey, bool valid)
    : m_ctx(std::move(ctx))
    , m_valid(valid)
{
    if (valid) {
        m_slot = std::make_shared<StashSlot>(m_ctx, stashKey);
    }
}

DuktapeBackend::Compiled::~Compiled() = default;

ScriptResult DuktapeBackend::Compiled::call(QObject* api, const QVariantMap& context)
{
    if (!m_valid) {
        return {QVariant(), QStringLiteral("compiled function is not callable")};
    }

    duk_context* ctx = m_ctx.get();

    duk_push_global_stash(ctx);
    duk_get_prop_index(ctx, -1, m_slot->key); // [stash fn]
    duk_remove(ctx, -2);                      // [fn]

    pushQObjectBridge(ctx, api);             // [fn node]
    pushVariantMap(ctx, context);            // [fn node ctx]

    if (duk_pcall(ctx, 2) != 0) {
        return errorFromStackTop(ctx);
    }

    ScriptResult result{getVariant(ctx, -1), QString()};
    duk_pop(ctx);
    return result;
}

DuktapeBackend::Compiled DuktapeBackend::compile(const QString& functionBody, QString* error)
{
    duk_context* ctx = m_ctx.get();

    const QString wrapped =
        QStringLiteral("(function(node, context) {\n%1\n})").arg(functionBody);
    const QByteArray utf8 = wrapped.toUtf8();

    if (duk_peval_lstring(ctx, utf8.constData(),
                          static_cast<duk_size_t>(utf8.size())) != 0) {
        if (error) {
            *error = QString::fromUtf8(duk_safe_to_string(ctx, -1));
        }
        duk_pop(ctx);
        return Compiled(m_ctx, 0, false);
    }

    // Park the function in the global stash so it survives (and is reachable
    // by) every copy of this Compiled handle. The StashSlot in the returned
    // Compiled deletes the entry again when the last copy dies.
    const quint32 key = m_nextStashKey++;
    duk_push_global_stash(ctx);  // [fn stash]
    duk_dup(ctx, -2);            // [fn stash fn]
    duk_put_prop_index(ctx, -2, key);
    duk_pop_2(ctx);

    if (error) {
        error->clear();
    }
    return Compiled(m_ctx, key, true);
}

void DuktapeBackend::registerObject(const QString& globalName, QObject* object)
{
    duk_context* ctx = m_ctx.get();
    pushQObjectBridge(ctx, object);
    duk_put_global_string(ctx, globalName.toUtf8().constData());
}

#endif // NODEGRAPH_HAS_DUKTAPE
