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
    virtual ~GraphSubject() = default;
    
    // Observer management
    virtual void attach(GraphObserver* observer) = 0;
    virtual void detach(GraphObserver* observer) = 0;
    
    // Batch mode for bulk operations (prevents observer storm)
    virtual void beginBatch() = 0;
    virtual void endBatch() = 0;
    virtual bool isInBatch() const = 0;
    
    // Notification helpers for subclasses
    virtual void notifyNodeAdded(const Node& node) = 0;
    virtual void notifyNodeRemoved(const QUuid& nodeId) = 0;
    virtual void notifyNodeMoved(const QUuid& nodeId, QPointF oldPos, QPointF newPos) = 0;
    virtual void notifyEdgeAdded(const Edge& edge) = 0;
    virtual void notifyEdgeRemoved(const QUuid& edgeId) = 0;
    virtual void notifyGraphCleared() = 0;
    virtual void notifyGraphLoaded(const QString& filename) = 0;
    virtual void notifyGraphSaved(const QString& filename) = 0;
};
