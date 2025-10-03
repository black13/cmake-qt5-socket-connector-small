#pragma once

#include <QGraphicsScene>
#include <QHash>
#include <QUuid>
#include "graph_observer.h"

class Node;
class Edge;
class Socket;
class GhostEdge;

/**
 * Scene - QElectroTech-style typed scene management
 * 
 * Uses QHash collections for O(1) UUID lookups
 * Never use generic items() - always use typed collections
 * Follows QElectroTech/Fritzing patterns for professional node editors
 */
class Scene : public QGraphicsScene, public GraphSubject
{
    Q_OBJECT

signals:
    void sceneChanged();

public:
    explicit Scene(QObject* parent = nullptr);
    
    // Typed item management - QElectroTech style
    void addNode(Node* node);
    void addEdge(Edge* edge);
    void addSocket(Socket* socket);  // Deprecated - sockets managed by nodes
    
    void removeNode(const QUuid& nodeId);

    // Fast UUID-based lookups O(1) - no searching, no casting
    Node* getNode(const QUuid& nodeId) const;
    Edge* getEdge(const QUuid& edgeId) const;
    
    // Type-safe iteration - never need qgraphicsitem_cast
    const QHash<QUuid, Node*>& getNodes() const { return m_nodes; }
    const QHash<QUuid, Edge*>& getEdges() const { return m_edges; }
    // Clean design: sockets accessed via nodes, not scene
    
    // Internal deletion methods (called by QGraph, not directly)
    void removeNodeInternal(const QUuid& nodeId);
    void removeEdgeInternal(const QUuid& edgeId);
    void removeSelectedInternal();  // Delete all selected items
    void clearGraphInternal();
    
    // PHASE 1.2: Safe shutdown preparation
    void prepareForShutdown();
    bool isShutdownInProgress() const { return m_shutdownInProgress; }
    
    // Public observer notifications (for Node movement)
    using GraphSubject::notifyNodeMoved;
    
    // Ghost edge for visual connection feedback (right-click and drag)
    void startGhostEdge(Socket* fromSocket, const QPointF& startPos);
    void updateGhostEdge(const QPointF& currentPos);
    void finishGhostEdge(Socket* toSocket = nullptr);
    void cancelGhostEdge();
    bool ghostEdgeActive() const { return m_ghostEdgeActive; }

    // Magnetic connection helpers
    Socket* findNearestValidSocket(const QPointF& scenePos, Socket* fromSocket, QPointF& snappedPos);
    qreal getMagneticRadius() const { return 40.0; } // Magnetic attraction radius

    // Grid and snap-to-grid
    QPointF snapPoint(const QPointF& scenePos) const;
    void setSnapToGrid(bool on) { m_snapToGrid = on; }
    bool isSnapToGrid() const { return m_snapToGrid; }
    int gridSize() const { return 40; }  // TODO: make configurable

    // Critical destruction safety flag
    static bool isClearing() { return s_clearingGraph; }

protected:
    // Mouse event handling for ghost edge interactions
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;

private:
    // QElectroTech-style typed collections with UUID keys
    QHash<QUuid, Node*> m_nodes;
    QHash<QUuid, Edge*> m_edges;
    QHash<QUuid, Socket*> m_sockets;  // Deprecated - kept for compatibility
    
    // Ghost edge for visual feedback during right-click connection creation
    GhostEdge* m_ghostEdge;
    Socket* m_ghostFromSocket;
    bool m_ghostEdgeActive;
    
    // Helper method for ghost edge styling
    QPen ghostPen() const;
    void resetAllSocketStates();

    // Static flag to prevent socket cleanup during clearGraph
    static bool s_clearingGraph;

    // Grid and snap state
    bool m_snapToGrid = false;

    // Shutdown coordination flag
    bool m_shutdownInProgress;
};