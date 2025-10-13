#pragma once

#include <QSet>
#include <QUuid>
#include <QPointF>

// Forward declarations
class Node;
class Edge;

/**
 * GraphObserver - Interface for observing graph changes
 * 
 * Pure virtual interface that observers implement to receive
 * notifications about graph mutations.
 */
class GraphObserver
{
public:
    virtual ~GraphObserver() = default;

    // Node lifecycle events
    virtual void onNodeAdded(const Node&) {}
    virtual void onNodeRemoved(const QUuid&) {}
    virtual void onNodeMoved(const QUuid&, QPointF, QPointF) {}

    // Edge lifecycle events
    virtual void onEdgeAdded(const Edge&) {}
    virtual void onEdgeRemoved(const QUuid&) {}

    // Graph-level events
    virtual void onGraphCleared() {}
    virtual void onGraphLoaded(const QString&) {}
    virtual void onGraphSaved(const QString&) {}
};

/**
 * GraphSubject - Base class for observable graph entities
 * 
 * Manages observer registration and provides notification helpers
 * for subclasses (Scene, GraphModel, etc.)
 */
class GraphSubject
{
public:
    virtual ~GraphSubject();
    
    // Observer management
    void attach(GraphObserver* observer);
    void detach(GraphObserver* observer);
    
    // Batch mode for bulk operations (prevents observer storm)
    static void beginBatch();
    static void endBatch();
    static bool isInBatch() { return s_batchDepth > 0; }
    
protected:
    // Notification helpers for subclasses
    void notifyNodeAdded(const Node& node);
    void notifyNodeRemoved(const QUuid& nodeId);
    void notifyNodeMoved(const QUuid& nodeId, QPointF oldPos, QPointF newPos);
    void notifyEdgeAdded(const Edge& edge);
    void notifyEdgeRemoved(const QUuid& edgeId);
    void notifyGraphCleared();
    void notifyGraphLoaded(const QString& filename);
    void notifyGraphSaved(const QString& filename);
    
private:
    QSet<GraphObserver*> m_observers;
    
    // Static batch control
    static int s_batchDepth;
};