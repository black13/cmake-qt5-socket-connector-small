#include "graph_script_api.h"

#if ENABLE_JS

#include "scene.h"
#include "graph_factory.h"
#include "node.h"
#include "edge.h"
#include "graph_observer.h"
#include "xml_autosave_observer.h"
#include <QJSEngine>
#include <QDebug>

GraphScriptApi::GraphScriptApi(Scene* scene, GraphFactory* factory, QObject* parent)
    : QObject(parent)
    , m_scene(scene)
    , m_factory(factory)
    , m_batchMode(false)
{
    Q_ASSERT(m_scene);
    Q_ASSERT(m_factory);
}

QString GraphScriptApi::createNode(const QString& type, double x, double y)
{
    if (!m_factory) {
        qWarning() << "GraphScriptApi::createNode: No factory available";
        return QString();
    }
    
    Node* node = m_factory->createNode(type, QPointF(x, y));
    if (!node) {
        qWarning() << "GraphScriptApi::createNode: Factory failed to create node of type" << type;
        return QString();
    }
    
    return formatUuid(node->getId());
}

bool GraphScriptApi::connect(const QString& fromNodeId, int fromSocketIndex,
                            const QString& toNodeId, int toSocketIndex)
{
    if (!m_factory) {
        qWarning() << "GraphScriptApi::connect: No factory available";
        return false;
    }
    
    QUuid fromId = parseUuid(fromNodeId);
    QUuid toId = parseUuid(toNodeId);
    
    if (fromId.isNull() || toId.isNull()) {
        qWarning() << "GraphScriptApi::connect: Invalid UUID format";
        return false;
    }
    
    Edge* edge = m_factory->connectByIds(fromId, fromSocketIndex, toId, toSocketIndex);
    return (edge != nullptr);
}

bool GraphScriptApi::deleteNode(const QString& nodeId)
{
    if (!m_scene) {
        qWarning() << "GraphScriptApi::deleteNode: No scene available";
        return false;
    }
    
    QUuid uuid = parseUuid(nodeId);
    if (uuid.isNull()) {
        qWarning() << "GraphScriptApi::deleteNode: Invalid UUID format";
        return false;
    }
    
    Node* node = m_scene->getNode(uuid);
    if (!node) {
        qWarning() << "GraphScriptApi::deleteNode: Node not found" << nodeId;
        return false;
    }
    
    m_scene->deleteNode(uuid);
    return true;
}

bool GraphScriptApi::deleteEdge(const QString& edgeId)
{
    if (!m_scene) {
        qWarning() << "GraphScriptApi::deleteEdge: No scene available";
        return false;
    }
    
    QUuid uuid = parseUuid(edgeId);
    if (uuid.isNull()) {
        qWarning() << "GraphScriptApi::deleteEdge: Invalid UUID format";
        return false;
    }
    
    Edge* edge = m_scene->getEdge(uuid);
    if (!edge) {
        qWarning() << "GraphScriptApi::deleteEdge: Edge not found" << edgeId;
        return false;
    }
    
    m_scene->deleteEdge(uuid);
    return true;
}

void GraphScriptApi::beginBatch()
{
    if (!m_batchMode) {
        GraphSubject::beginBatch();
        m_batchMode = true;
        qDebug() << "GraphScriptApi: Batch mode started";
    }
}

void GraphScriptApi::endBatch()
{
    if (m_batchMode) {
        GraphSubject::endBatch();
        m_batchMode = false;
        qDebug() << "GraphScriptApi: Batch mode ended";
    }
}

QJSValue GraphScriptApi::getSelectedNodes()
{
    // TODO: Implement when selection system is available
    // For now, return empty array
    QJSEngine* engine = qjsEngine(this);
    return engine ? engine->newArray(0) : QJSValue();
}

QJSValue GraphScriptApi::getAllNodes()
{
    if (!m_scene) {
        QJSEngine* engine = qjsEngine(this);
        return engine ? engine->newArray(0) : QJSValue();
    }
    
    QJSEngine* engine = qjsEngine(this);
    if (!engine) return QJSValue();
    
    const auto& nodes = m_scene->getNodes();
    QJSValue result = engine->newArray(nodes.size());
    
    int index = 0;
    for (auto it = nodes.begin(); it != nodes.end(); ++it, ++index) {
        result.setProperty(index, formatUuid(it.key()));
    }
    
    return result;
}

QJSValue GraphScriptApi::getAllEdges()
{
    if (!m_scene) {
        QJSEngine* engine = qjsEngine(this);
        return engine ? engine->newArray(0) : QJSValue();
    }
    
    QJSEngine* engine = qjsEngine(this);
    if (!engine) return QJSValue();
    
    const auto& edges = m_scene->getEdges();
    QJSValue result = engine->newArray(edges.size());
    
    int index = 0;
    for (auto it = edges.begin(); it != edges.end(); ++it, ++index) {
        result.setProperty(index, formatUuid(it.key()));
    }
    
    return result;
}

bool GraphScriptApi::saveNow()
{
    // Trigger autosave if available
    // Note: This would need access to the autosave observer
    // For now, just return true as a placeholder
    qDebug() << "GraphScriptApi::saveNow: Manual save triggered from JS";
    return true;
}

void GraphScriptApi::clearGraph()
{
    if (m_scene) {
        m_scene->clearGraph();
    }
}

QJSValue GraphScriptApi::getGraphStats()
{
    QJSEngine* engine = qjsEngine(this);
    if (!engine || !m_scene) return QJSValue();
    
    QJSValue stats = engine->newObject();
    stats.setProperty("nodeCount", m_scene->getNodes().size());
    stats.setProperty("edgeCount", m_scene->getEdges().size());
    
    return stats;
}

QUuid GraphScriptApi::parseUuid(const QString& uuidStr) const
{
    return QUuid::fromString(uuidStr);
}

QString GraphScriptApi::formatUuid(const QUuid& uuid) const
{
    return uuid.toString(QUuid::WithoutBraces);
}

#endif // ENABLE_JS