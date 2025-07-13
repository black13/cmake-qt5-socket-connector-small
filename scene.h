#pragma once

#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QHash>
#include <QUuid>
#include "graph_observer.h"

class Node;
class Edge;
class Socket;
class QGraphicsPathItem;

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
    void removeEdge(const QUuid& edgeId);
    
    // Fast UUID-based lookups O(1) - no searching, no casting
    Node* getNode(const QUuid& nodeId) const;
    Edge* getEdge(const QUuid& edgeId) const;
    
    // Type-safe iteration - never need qgraphicsitem_cast
    const QHash<QUuid, Node*>& getNodes() const { return m_nodes; }
    const QHash<QUuid, Edge*>& getEdges() const { return m_edges; }
    // Clean design: sockets accessed via nodes, not scene
    
    // Deletion methods - maintain integrity
    void deleteNode(const QUuid& nodeId);
    void deleteEdge(const QUuid& edgeId);
    void deleteSelected();  // Delete all selected items
    
    // Clear both graphics items AND registries - prevents dangling pointers
    void clearGraph();
    
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
    QGraphicsPathItem* m_ghostEdge;
    Socket* m_ghostFromSocket;
    bool m_ghostEdgeActive;
    
    // Helper method for ghost edge styling
    QPen ghostPen() const;
    void resetAllSocketStates();
    
    // Static flag to prevent socket cleanup during clearGraph
    static bool s_clearingGraph;
    
    // Shutdown coordination flag
    bool m_shutdownInProgress;
};