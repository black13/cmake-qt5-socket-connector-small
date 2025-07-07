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
    virtual void onNodeAdded(const Node& node) {}
    virtual void onNodeRemoved(const QUuid& nodeId) {}
    virtual void onNodeMoved(const QUuid& nodeId, QPointF oldPos, QPointF newPos) {}
    
    // Edge lifecycle events  
    virtual void onEdgeAdded(const Edge& edge) {}
    virtual void onEdgeRemoved(const QUuid& edgeId) {}
    
    // Graph-level events
    virtual void onGraphCleared() {}
    virtual void onGraphLoaded(const QString& filename) {}
    virtual void onGraphSaved(const QString& filename) {}
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
};