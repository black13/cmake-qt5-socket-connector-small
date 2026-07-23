#include "undo_commands.h"

#include "scene.h"
#include "node.h"
#include "edge.h"
#include "graph_factory.h"

#include <QDebug>
#include <QSet>
#include <libxml/tree.h>
#include <libxml/parser.h>

// ============================================================================
// XML snapshot helpers - reuse the self-serialization pathway
// ============================================================================

namespace {

QString dumpElementToXml(xmlNodePtr element)
{
    if (!element) {
        return {};
    }

    xmlDocPtr doc = xmlNewDoc(BAD_CAST "1.0");
    xmlDocSetRootElement(doc, element); // takes ownership of element

    xmlChar* buffer = nullptr;
    int size = 0;
    xmlDocDumpMemory(doc, &buffer, &size);

    QString xml;
    if (buffer) {
        xml = QString::fromUtf8(reinterpret_cast<const char*>(buffer), size);
        xmlFree(buffer);
    }
    xmlFreeDoc(doc);
    return xml;
}

QString serializeNode(Scene* scene, const QUuid& nodeId)
{
    Node* node = scene->getNode(nodeId);
    if (!node) {
        return {};
    }
    xmlDocPtr doc = xmlNewDoc(BAD_CAST "1.0");
    xmlNodePtr element = node->write(doc); // virtual: ScriptedNode adds script/payload
    QString xml = dumpElementToXml(element);
    // dumpElementToXml owns element through its own doc; free only our temp doc
    xmlFreeDoc(doc);
    return xml;
}

QString serializeEdge(Scene* scene, const QUuid& edgeId)
{
    Edge* edge = scene->getEdge(edgeId);
    if (!edge) {
        return {};
    }
    xmlDocPtr doc = xmlNewDoc(BAD_CAST "1.0");
    xmlNodePtr element = edge->write(doc);
    QString xml = dumpElementToXml(element);
    xmlFreeDoc(doc);
    return xml;
}

bool restoreNode(GraphFactory* factory, const QString& xml)
{
    if (xml.isEmpty()) {
        return false;
    }
    xmlDocPtr doc = xmlParseDoc(BAD_CAST xml.toUtf8().constData());
    if (!doc) {
        qWarning() << "undo: failed to parse node snapshot";
        return false;
    }
    Node* node = factory->createNodeFromXml(xmlDocGetRootElement(doc));
    xmlFreeDoc(doc);
    return node != nullptr;
}

bool restoreEdge(Scene* scene, GraphFactory* factory, const QString& xml)
{
    if (xml.isEmpty()) {
        return false;
    }
    xmlDocPtr doc = xmlParseDoc(BAD_CAST xml.toUtf8().constData());
    if (!doc) {
        qWarning() << "undo: failed to parse edge snapshot";
        return false;
    }
    Edge* edge = factory->createEdgeFromXml(xmlDocGetRootElement(doc));
    xmlFreeDoc(doc);
    if (!edge) {
        return false;
    }
    if (!edge->resolveConnections(scene)) {
        qWarning() << "undo: restored edge failed to resolve connections";
        scene->deleteEdge(edge->getId());
        return false;
    }
    return true;
}

} // namespace

// ============================================================================
// CreateNodeCommand
// ============================================================================

CreateNodeCommand::CreateNodeCommand(Scene* scene, GraphFactory* factory,
                                     const QString& nodeType, const QPointF& position,
                                     QUndoCommand* parent)
    : QUndoCommand(QObject::tr("Create %1 node").arg(nodeType), parent)
    , m_scene(scene)
    , m_factory(factory)
    , m_nodeType(nodeType)
    , m_position(position)
{
}

void CreateNodeCommand::redo()
{
    if (m_nodeId.isNull()) {
        // First execution: create fresh through the template pathway
        Node* node = m_factory->createNode(m_nodeType, m_position);
        if (!node) {
            qWarning() << "CreateNodeCommand: creation failed for type" << m_nodeType;
            setObsolete(true);
            return;
        }
        m_nodeId = node->getId();
    } else {
        // Re-do after undo: replay the snapshot so the UUID (and any script
        // edits made before the undo) survive
        if (!restoreNode(m_factory, m_xml)) {
            qWarning() << "CreateNodeCommand: redo failed to restore node";
        }
    }
}

void CreateNodeCommand::undo()
{
    if (m_nodeId.isNull()) {
        return;
    }
    // Snapshot latest state (script/payload may have changed since creation)
    m_xml = serializeNode(m_scene, m_nodeId);
    m_scene->deleteNode(m_nodeId);
}

// ============================================================================
// ConnectEdgeCommand
// ============================================================================

ConnectEdgeCommand::ConnectEdgeCommand(Scene* scene, GraphFactory* factory,
                                       const QUuid& fromNodeId, int fromSocketIndex,
                                       const QUuid& toNodeId, int toSocketIndex,
                                       QUndoCommand* parent)
    : QUndoCommand(QObject::tr("Connect nodes"), parent)
    , m_scene(scene)
    , m_factory(factory)
    , m_fromNodeId(fromNodeId)
    , m_toNodeId(toNodeId)
    , m_fromSocketIndex(fromSocketIndex)
    , m_toSocketIndex(toSocketIndex)
{
}

void ConnectEdgeCommand::redo()
{
    if (m_edgeId.isNull()) {
        Edge* edge = m_factory->connectByIds(m_fromNodeId, m_fromSocketIndex,
                                             m_toNodeId, m_toSocketIndex);
        if (!edge) {
            qWarning() << "ConnectEdgeCommand: connection failed";
            setObsolete(true);
            return;
        }
        m_edgeId = edge->getId();
    } else {
        if (!restoreEdge(m_scene, m_factory, m_xml)) {
            qWarning() << "ConnectEdgeCommand: redo failed to restore edge";
        }
    }
}

void ConnectEdgeCommand::undo()
{
    if (m_edgeId.isNull()) {
        return;
    }
    m_xml = serializeEdge(m_scene, m_edgeId);
    m_scene->deleteEdge(m_edgeId);
}

// ============================================================================
// DeleteSelectionCommand
// ============================================================================

DeleteSelectionCommand::DeleteSelectionCommand(Scene* scene, GraphFactory* factory,
                                               QUndoCommand* parent)
    : QUndoCommand(QObject::tr("Delete selection"), parent)
    , m_scene(scene)
    , m_factory(factory)
{
    // Capture selected nodes
    for (Node* node : m_scene->selectedNodes()) {
        m_nodeIds.append(node->getId());
    }

    // Capture selected edges PLUS edges incident to selected nodes (dedup)
    QSet<QUuid> edgeIds;
    for (Edge* edge : m_scene->selectedEdges()) {
        edgeIds.insert(edge->getId());
    }
    for (const QUuid& nodeId : m_nodeIds) {
        for (auto it = m_scene->getEdges().constBegin(); it != m_scene->getEdges().constEnd(); ++it) {
            if (it.value()->isConnectedToNode(nodeId)) {
                edgeIds.insert(it.key());
            }
        }
    }
    m_edgeIds = edgeIds.values();

    // Snapshot everything now, while it is alive
    for (const QUuid& edgeId : m_edgeIds) {
        m_edgeXml.append(serializeEdge(m_scene, edgeId));
    }
    for (const QUuid& nodeId : m_nodeIds) {
        m_nodeXml.append(serializeNode(m_scene, nodeId));
    }

    setText(QObject::tr("Delete %1 node(s), %2 edge(s)")
                .arg(m_nodeIds.size())
                .arg(m_edgeIds.size()));
}

void DeleteSelectionCommand::redo()
{
    // Edges first (they reference sockets), then nodes
    for (const QUuid& edgeId : m_edgeIds) {
        m_scene->deleteEdge(edgeId);
    }
    for (const QUuid& nodeId : m_nodeIds) {
        m_scene->deleteNode(nodeId);
    }
}

void DeleteSelectionCommand::undo()
{
    // Nodes first, then the edges that connect them
    for (const QString& xml : m_nodeXml) {
        restoreNode(m_factory, xml);
    }
    for (const QString& xml : m_edgeXml) {
        restoreEdge(m_scene, m_factory, xml);
    }
}

// ============================================================================
// MoveNodesCommand
// ============================================================================

MoveNodesCommand::MoveNodesCommand(Scene* scene, const QVector<NodeMove>& moves,
                                   QUndoCommand* parent)
    : QUndoCommand(moves.size() == 1 ? QObject::tr("Move node")
                                     : QObject::tr("Move %1 nodes").arg(moves.size()),
                   parent)
    , m_scene(scene)
    , m_moves(moves)
    , m_firstRedo(true)
{
}

void MoveNodesCommand::applyPositions(bool useNewPos)
{
    for (const NodeMove& move : m_moves) {
        if (Node* node = m_scene->getNode(move.nodeId)) {
            node->setPos(useNewPos ? move.newPos : move.oldPos);
            node->updateConnectedEdges();
        }
    }
}

void MoveNodesCommand::redo()
{
    if (m_firstRedo) {
        // The interactive drag already moved the items; don't move them twice
        m_firstRedo = false;
        return;
    }
    applyPositions(true);
}

void MoveNodesCommand::undo()
{
    applyPositions(false);
}
