#pragma once

#include <QGraphicsScene>
#include <QHash>
#include <QUuid>
#include "graph_observer.h"

class Node;
class Edge;
class Socket;
class GraphFactory;
// JavaScript engine forward declaration removed
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
    // RAII guard to mark clearing state
    class ScopedClearing {
    public:
        explicit ScopedClearing(bool& flag) : f(flag) { f = true; }
        ~ScopedClearing() { f = false; }
    private:
        bool& f;
    };

    bool isClearing() const noexcept { return m_isClearing; }
    // Instance-scoped guard for controlled teardown
    bool isClearing() const noexcept { return m_isClearing; }

    explicit Scene(QObject* parent = nullptr);
    ~Scene() override;
    
    // Typed item management - QElectroTech style
    void addNode(Node* node);
    void addEdge(Edge* edge);
    void addSocket(Socket* socket);  // Deprecated - sockets managed by nodes
    
    void removeNode(const QUuid& nodeId);
    void removeEdge(const QUuid& edgeId);
    
    // Fast UUID-based lookups O(1) - no searching, no casting
    [[nodiscard]] Node* getNode(const QUuid& nodeId) const;
    [[nodiscard]] Edge* getEdge(const QUuid& edgeId) const;
    
    // Type-safe iteration - never need qgraphicsitem_cast
    [[nodiscard]] const QHash<QUuid, Node*>& getNodes() const { return m_nodes; }
    [[nodiscard]] const QHash<QUuid, Edge*>& getEdges() const { return m_edges; }
    // Clean design: sockets accessed via nodes, not scene
    
    // Deletion methods - maintain integrity
    void deleteNode(const QUuid& nodeId);
    void deleteEdge(const QUuid& edgeId);
    
    // Clear both graphics items AND registries - prevents dangling pointers
    void clearGraph();
    
    // Controlled teardown - edges first, then nodes. Safe to call multiple times.
    void clearGraphControlled();
    
    // PHASE 1.2: Safe shutdown preparation
    void prepareForShutdown();
    [[nodiscard]] bool isShutdownInProgress() const { return m_shutdownInProgress; }
    
    // Public observer notifications (for Node movement)
    using GraphSubject::notifyNodeMoved;
    
    // Ghost edge for visual connection feedback (right-click and drag)
    void startGhostEdge(Socket* fromSocket, const QPointF& startPos);
    void updateGhostEdge(const QPointF& currentPos);
    void finishGhostEdge(Socket* toSocket = nullptr);
    void cancelGhostEdge();
    [[nodiscard]] bool ghostEdgeActive() const { return m_ghostEdgeActive; }
    
    // JavaScript engine methods removed - focusing on core C++ functionality
    
    
    // Factory integration for consistent edge creation
    void setGraphFactory(GraphFactory* factory) { m_graphFactory = factory; }

protected:
    // Mouse event handling for ghost edge interactions
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
    
    // Centralized key handling for consistent multi-selection deletion
    void keyPressEvent(QKeyEvent* event) override;

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
    
    // Internal immediate removal helpers for controlled teardown
    void removeEdgeImmediate(const QUuid& id);
    void removeNodeImmediate(const QUuid& id);
    
    // Grid and snap state
    bool m_snapToGrid = false;
    
    // Static flag to prevent socket cleanup during clearGraph
    
    // Instance-scoped guard replacing static teardown flag
    bool m_isClearing = false;
    
    // Shutdown coordination flag
    bool m_shutdownInProgress;
    
    // Factory for consistent edge creation (non-owning)
    GraphFactory* m_graphFactory;
    
public:
    // ===== Auto layout (Simulated Annealing) =====
    // If selectionOnly==true and no nodes are selected, falls back to all nodes.
    void autoLayoutAnneal(bool selectionOnly = true,
                          int maxIters = 2000,
                          double t0 = 1.0,
                          double t1 = 0.01);
    
    // Force-directed layout (using same annealing algorithm with different parameters)
    void autoLayoutForceDirected(bool selectionOnly = true,
                                 int maxIters = 350,
                                 double cooling = 0.92);
    
    // Grid and snap helpers (for auto-layout integration)
    QPointF snapPoint(const QPointF& scenePos) const;
    void setSnapToGrid(bool on) { m_snapToGrid = on; }
    bool isSnapToGrid() const { return m_snapToGrid; }
    int gridSize() const { return 40; }          // TODO: make configurable
    
    // Debug animated force layout for 3 nodes
    void debugForceLayout3Nodes();
    
    // JavaScript engine removed - focusing on core C++ functionality
};