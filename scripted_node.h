#pragma once

#include "node.h"
#include <QJSEngine>
#include <QJSValue>
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
 *  - Scripts are compiled lazily with a shared QJSEngine owned by Graph
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

    static void setSharedEngine(QJSEngine* engine);
    static QJSEngine* sharedEngine() { return s_engine; }

    xmlNodePtr write(xmlDocPtr doc, xmlNodePtr repr = nullptr) const override;
    void read(xmlNodePtr node) override;

protected:
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

private:
    void compileIfNeeded();
    QJSValue callScript(const QVariantMap& context);

    QString m_script;
    QVariantMap m_payload;
    QString m_displayLabel;
    QJSValue m_compiledFunction;
    QVariant m_lastResult;

    static QJSEngine* s_engine;
};
