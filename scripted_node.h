#pragma once

#include "node.h"
#include "script_engine.h"
#include <QVariantMap>

/**
 * ============================================================================
 * ScriptedNode – Lightweight scripting surface for each visual node
 * ----------------------------------------------------------------------------
 *  - Every node instance is created as a ScriptedNode; no special “script” type
 *  - Stores a per-node JavaScript snippet plus payload key/value map
 *  - Exposes a guarded API (ScriptNodeApi) to scripts: payload helpers, runWork,
 *    metadata (id/type/socket counts/edge counts) so scripts can inspect their
 *    C++ host safely
 *  - Scripts are compiled lazily with a shared type-erased ScriptEngine owned
 *    by Graph (backend-agnostic: QJSEngine today, Duktape or others tomorrow)
 *  - Serialized via `<script>` / `<payload>` children so behavior survives save/load.
 *    (Autosave runs immediately after palette drops, so starter scripts are persisted
 *     as soon as a node lands on the scene.)
 * ============================================================================
 */
class ScriptedNode : public Node
{
public:
    ScriptedNode(const QUuid& id = QUuid::createUuid(),
                 const QPointF& position = QPointF(100, 100));
    ~ScriptedNode() override = default;

    void setScript(const QString& code);
    [[nodiscard]] QString script() const { return m_script; }

    void setPayload(const QVariantMap& payload);
    [[nodiscard]] QVariantMap payload() const { return m_payload; }

    QVariant evaluate(const QVariantMap& context = QVariantMap());

    void setDisplayLabel(const QString& text);
    [[nodiscard]] QString displayLabel() const { return m_displayLabel; }

    static void setSharedEngine(ScriptEngine engine);
    static const ScriptEngine& sharedEngine() { return s_engine; }

    // Reentrancy guard: >0 while any node script is executing. Graph uses this
    // to refuse destructive operations (delete/clear/load) mid-evaluation, and
    // evaluate() uses it to cap native recursion depth.
    static bool isExecuting() { return s_executionDepth > 0; }

    xmlNodePtr write(xmlDocPtr doc, xmlNodePtr repr = nullptr) const override;
    void read(xmlNodePtr node) override;

protected:
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

private:
    void compileIfNeeded();

    QString m_script;
    QVariantMap m_payload;
    QString m_displayLabel;
    ScriptFunction m_compiledFunction;
    QVariant m_lastResult;

    static ScriptEngine s_engine;
    static int s_executionDepth;
};
