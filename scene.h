#pragma once

#include <QGraphicsScene>
#include <QHash>
#include <QUuid>
#include "graph_observer.h"

class Node;
class Edge;
class Socket;
class JavaScriptEngine;
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
    
    // Deletion methods - maintain integrity
    void deleteNode(const QUuid& nodeId);
    void deleteEdge(const QUuid& edgeId);
    void deleteSelected();  // Delete all selected items
    
    // Clear both graphics items AND registries - prevents dangling pointers  
    void clearGraph();
    
    // Ghost edge for visual connection feedback
    void startGhostEdge(Socket* fromSocket, const QPointF& startPos);
    void updateGhostEdge(const QPointF& currentPos);
    void finishGhostEdge(Socket* toSocket = nullptr);
    void cancelGhostEdge();
    bool ghostEdgeActive() const { return m_ghostEdgeActive; }
    
    // JavaScript engine access
    JavaScriptEngine* getJavaScriptEngine() const { return m_jsEngine; }
    
    // Observer notification helpers (public for Node to call)
    void nodeMovedNotification(const QUuid& nodeId, QPointF oldPos, QPointF newPos) {
        notifyNodeMoved(nodeId, oldPos, newPos);
    }
    
    // Shutdown preparation (public for Window to call)
    void prepareForShutdown();

private:
    // DEPRECATED: Use deleteEdge() instead
    void removeEdge_INTERNAL(const QUuid& edgeId);
    
    // PHASE 1.2: Safe shutdown preparation  
    bool isShutdownInProgress() const { return m_shutdownInProgress; }
    
    QString executeJavaScript(const QString& script);
    void loadJavaScriptFile(const QString& filePath);
    
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
    
    // Shutdown coordination flag
    bool m_shutdownInProgress;
    
    // JavaScript engine integration
    JavaScriptEngine* m_jsEngine;
};