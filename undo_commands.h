#pragma once

#include <QUndoCommand>
#include <QPointF>
#include <QString>
#include <QStringList>
#include <QUuid>
#include <QVector>
#include "scene.h" // NodeMove

class GraphFactory;

/**
 * undo_commands.h - QUndoStack commands for graph mutations
 *
 * Design: commands snapshot state as XML strings using the project's
 * self-serialization (Node::write / Edge::write). Restoring goes through
 * GraphFactory::createNodeFromXml / createEdgeFromXml, the same single
 * pathway used by file loading - so undo/redo preserves UUIDs, node
 * scripts, and payloads, and observers (autosave, status bar) fire
 * exactly as they do for any other mutation.
 *
 * Stack discipline: commands above a node's CreateNodeCommand are undone
 * first, so by the time a creation is undone the node has no edges left.
 * DeleteSelectionCommand handles the node+incident-edge case atomically.
 */

/// Create a node of a template type at a position.
class CreateNodeCommand : public QUndoCommand
{
public:
    CreateNodeCommand(Scene* scene, GraphFactory* factory,
                      const QString& nodeType, const QPointF& position,
                      QUndoCommand* parent = nullptr);

    void redo() override;
    void undo() override;

    [[nodiscard]] QUuid nodeId() const { return m_nodeId; }

private:
    Scene* m_scene;
    GraphFactory* m_factory;
    QString m_nodeType;
    QPointF m_position;
    QUuid m_nodeId;   // set on first redo
    QString m_xml;    // snapshot taken on undo, replayed on redo
};

/// Connect an output socket to an input socket (node UUID + socket index).
class ConnectEdgeCommand : public QUndoCommand
{
public:
    ConnectEdgeCommand(Scene* scene, GraphFactory* factory,
                       const QUuid& fromNodeId, int fromSocketIndex,
                       const QUuid& toNodeId, int toSocketIndex,
                       QUndoCommand* parent = nullptr);

    void redo() override;
    void undo() override;

private:
    Scene* m_scene;
    GraphFactory* m_factory;
    QUuid m_fromNodeId;
    QUuid m_toNodeId;
    int m_fromSocketIndex;
    int m_toSocketIndex;
    QUuid m_edgeId;   // set on first redo
    QString m_xml;    // snapshot taken on undo, replayed on redo
};

/// Delete the current selection: selected edges, selected nodes, and all
/// edges incident to those nodes. Snapshots everything at construction.
class DeleteSelectionCommand : public QUndoCommand
{
public:
    DeleteSelectionCommand(Scene* scene, GraphFactory* factory,
                           QUndoCommand* parent = nullptr);

    void redo() override;
    void undo() override;

    [[nodiscard]] bool isEmpty() const { return m_nodeIds.isEmpty() && m_edgeIds.isEmpty(); }

private:
    Scene* m_scene;
    GraphFactory* m_factory;
    QList<QUuid> m_nodeIds;
    QList<QUuid> m_edgeIds;
    QStringList m_nodeXml;
    QStringList m_edgeXml;
};

/// Move one or more nodes (one drag gesture = one command).
class MoveNodesCommand : public QUndoCommand
{
public:
    MoveNodesCommand(Scene* scene, const QVector<NodeMove>& moves,
                     QUndoCommand* parent = nullptr);

    void redo() override;
    void undo() override;

private:
    void applyPositions(bool useNewPos);

    Scene* m_scene;
    QVector<NodeMove> m_moves;
    bool m_firstRedo; // the drag itself already applied the new positions
};
