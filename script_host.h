#pragma once
#include <QObject>
#include <QJSEngine>
#include <QJSValue>
#include <QUuid>
#include <QPointF>
#include "scene.h"
#include "graph_factory.h"
#include "node.h"
#include "edge.h"

class ScriptHost : public QObject {
    Q_OBJECT
public:
    explicit ScriptHost(Scene* scene, GraphFactory* factory, QObject* parent=nullptr)
      : QObject(parent), m_scene(scene), m_factory(factory) {
        m_engine.installExtensions(QJSEngine::ConsoleExtension);
        // expose this API as "nodegraph"
        m_engine.globalObject().setProperty("nodegraph", m_engine.newQObject(this));
    }

    Q_INVOKABLE QString createNode(const QString& type, double x, double y) {
        if (!m_factory) return {};
        Node* n = m_factory->createNode(type, QPointF(x,y));
        return n ? n->getId().toString(QUuid::WithoutBraces) : QString{};
    }

    Q_INVOKABLE bool connect(const QString& fromUuid, int outIdx,
                             const QString& toUuid,   int inIdx) {
        if (!m_factory) return false;
        QUuid fromId(QString("{%1}").arg(fromUuid));
        QUuid toId(QString("{%1}").arg(toUuid));
        Edge* e = m_factory->connectByIds(fromId, outIdx, toId, inIdx);
        return e != nullptr;
    }

    Q_INVOKABLE QJSValue eval(const QString& code) {
        return m_engine.evaluate(code, QStringLiteral("<console>"));
    }

private:
    QJSEngine m_engine;
    Scene* m_scene = nullptr;
    GraphFactory* m_factory = nullptr;
};