#pragma once

#include <QSet>
#include <QUuid>
#include <QPointF>

// Forward declarations
class Node;
class Edge;
class GraphSubject;

/**
 * GraphObserver - Interface for observing graph changes
 * 
 * Pure virtual interface that observers implement to receive
 * notifications about graph mutations.
 *
 * Lifetime contract: the destructor self-detaches from every subject it is
 * attached to (tracked via addSubject/removeSubject), so a dying observer can
 * never leave a dangling pointer in a subject's container. GraphSubject's
 * destructor symmetrically unregisters itself from its observers.
 */
class GraphObserver
{
public:
    virtual ~GraphObserver();

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

    /// End-of-batch flush: called once per subject when the outermost batch
    /// ends, so batched mutations are not silently dropped by observers that
    /// skip notifications during batch mode (e.g. autosave catch-up).
    virtual void onBatchEnded() {}

    // Self-detach bookkeeping - called by GraphSubject::attach/detach only.
    void addSubject(GraphSubject* subject);
    void removeSubject(GraphSubject* subject);

private:
    QSet<GraphSubject*> m_subjects;
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
    GraphSubject();
    virtual ~GraphSubject();
    
    // Observer management
    void attach(GraphObserver* observer);
    void detach(GraphObserver* observer);
    
    // Batch mode for bulk operations (prevents observer storm)
    static void beginBatch();
    static void endBatch();
    static bool isInBatch() { return s_batchDepth > 0; }
    
    /// RAII batch guard: begins a batch on construction and ends it on
    /// destruction, so every return/throw path is covered. Prefer this over
    /// manual beginBatch/endBatch pairs - one missed endBatch permanently
    /// mutes all notifications process-wide.
    class BatchGuard
    {
    public:
        BatchGuard() { GraphSubject::beginBatch(); }
        ~BatchGuard() { GraphSubject::endBatch(); }
        BatchGuard(const BatchGuard&) = delete;
        BatchGuard& operator=(const BatchGuard&) = delete;
    };
    
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
    // Deliver onBatchEnded() to all observers of this subject (end-of-batch flush)
    void flushBatchObservers();
    
    QSet<GraphObserver*> m_observers;
    
    // Static batch control
    static int s_batchDepth;
    
    // All live subjects - needed so endBatch() can flush every subject's
    // observers when the outermost batch ends
    static QSet<GraphSubject*> s_subjects;
};
