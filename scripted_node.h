#pragma once

#include "node.h"
#include <QJSEngine>
#include <QJSValue>
#include <QVariantMap>

/**
 * ScriptedNode - Node subclass that executes embedded JavaScript.
 *
 * Stores a JavaScript snippet, payload key/value data, and exposes a thin API
 * to scripts via ScriptNodeApi (payload helpers + SyntheticWork hook).
 * Scripts are compiled once per node using a shared QJSEngine supplied by the
 * Graph facade.
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
