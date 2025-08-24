#ifndef EXECUTION_ORCHESTRATOR_H
#define EXECUTION_ORCHESTRATOR_H

#include "graph_observer.h"
#include "executable_spec.h"
#include <QObject>
#include <QUuid>
#include <QVariantMap>
#include <QHash>
#include <QSet>
#include <QTimer>
#include <QMutex>
#include <memory>

// Forward declarations
class Scene;
class Node;
class Edge;

/**
 * ExecutionResult - Contains execution output and metadata
 */
struct ExecutionResult {
    QVariantMap outputs;        // Node execution outputs
    QString executionHash;      // Hash of inputs+script for caching
    qint64 executionTimeMs;     // Execution time in milliseconds
    bool success;               // Whether execution succeeded
    QString errorMessage;       // Error details if execution failed
    
    ExecutionResult() : executionTimeMs(0), success(false) {}
};

/**
 * ExecutionOrchestrator - Graph-level computation scheduling and memoization
 * 
 * Provides:
 * - Topological execution ordering for DAG nodes
 * - Lazy evaluation with memoization 
 * - Integration with existing observer pattern
 * - Batch execution optimization
 * - Error isolation and recovery
 * 
 * Design principles:
 * - Non-invasive: Existing JavaScript engine unchanged
 * - Performance: O(affected_nodes) invalidation, not O(total_nodes) 
 * - Thread-safe: Execution can be scheduled from UI or background threads
 * - Observable: Results available to UI components via Qt signals
 */
class ExecutionOrchestrator : public QObject, public GraphObserver
{
    Q_OBJECT

public:
    explicit ExecutionOrchestrator(Scene* scene, QObject* parent = nullptr);
    ~ExecutionOrchestrator();

    // Configuration
    void setExecutableSpec(std::unique_ptr<ExecutableSpec> spec);
    void setMemoizationEnabled(bool enabled) { m_memoizationEnabled = enabled; }
    void setExecutionTimeoutMs(int timeoutMs) { m_executionTimeoutMs = timeoutMs; }

    // Manual execution control
    QVariantMap executeNode(const QUuid& nodeId, const QVariantMap& inputs = QVariantMap());
    void executeSubgraph(const QSet<QUuid>& nodeIds);
    void executeAll();
    
    // Execution state queries
    bool hasExecutionResult(const QUuid& nodeId) const;
    ExecutionResult getExecutionResult(const QUuid& nodeId) const;
    void clearExecutionCache();
    void clearExecutionCache(const QUuid& nodeId);
    
    // Dependency analysis
    QList<QUuid> getTopologicalOrder() const;
    QSet<QUuid> getUpstreamNodes(const QUuid& nodeId) const;
    QSet<QUuid> getDownstreamNodes(const QUuid& nodeId) const;
    
    // Performance monitoring
    struct ExecutionStats {
        int totalExecutions = 0;
        int cacheHits = 0;
        qint64 totalExecutionTimeMs = 0;
        int failedExecutions = 0;
    };
    ExecutionStats getExecutionStats() const { return m_stats; }
    void resetExecutionStats() { m_stats = ExecutionStats(); }

signals:
    // Execution events (for UI updates)
    void nodeExecutionStarted(const QUuid& nodeId);
    void nodeExecutionCompleted(const QUuid& nodeId, const ExecutionResult& result);
    void nodeExecutionFailed(const QUuid& nodeId, const QString& error);
    void batchExecutionCompleted(const QSet<QUuid>& nodeIds);

protected:
    // GraphObserver interface - automatic execution scheduling
    void onNodeAdded(const Node& node) override;
    void onNodeRemoved(const QUuid& nodeId) override;
    void onEdgeAdded(const Edge& edge) override;
    void onEdgeRemoved(const QUuid& edgeId) override;
    void onGraphCleared() override;

private slots:
    void processScheduledExecutions();

private:
    // Core execution logic
    ExecutionResult executeNodeInternal(const QUuid& nodeId, const QVariantMap& inputs);
    QVariantMap gatherNodeInputs(const QUuid& nodeId) const;
    void invalidateDownstream(const QUuid& nodeId);
    void scheduleExecution(const QUuid& nodeId);
    void scheduleExecutionDelayed();
    
    // Topological sorting
    void rebuildDependencyGraph();
    void topologicalSortRecursive(const QUuid& nodeId, QSet<QUuid>& visited, 
                                  QSet<QUuid>& recursionStack, QList<QUuid>& result) const;
    
    // Caching
    QString computeCacheKey(const QUuid& nodeId, const QVariantMap& inputs) const;
    bool hasCachedResult(const QString& cacheKey) const;
    ExecutionResult getCachedResult(const QString& cacheKey) const;
    void setCachedResult(const QString& cacheKey, const ExecutionResult& result);
    
    // Data members
    Scene* m_scene;                                          // Graph data source
    std::unique_ptr<ExecutableSpec> m_executableSpec;       // Execution capability
    
    // Execution state
    mutable QMutex m_mutex;                                 // Thread safety
    QHash<QUuid, ExecutionResult> m_executionResults;      // Current node outputs
    QHash<QString, ExecutionResult> m_executionCache;      // Memoization cache
    
    // Dependency graph (for topological ordering)
    QHash<QUuid, QSet<QUuid>> m_dependencies;              // nodeId -> upstream node UUIDs
    QHash<QUuid, QSet<QUuid>> m_dependents;                // nodeId -> downstream node UUIDs
    mutable bool m_dependencyGraphDirty;                    // Needs rebuild
    
    // Scheduling
    QSet<QUuid> m_scheduledNodes;                           // Nodes pending execution
    QTimer* m_executionTimer;                               // Batched execution delay
    
    // Configuration
    bool m_memoizationEnabled;                              // Enable result caching
    int m_executionTimeoutMs;                               // Per-node timeout
    static const int EXECUTION_DELAY_MS = 50;              // Batch delay for UI responsiveness
    
    // Statistics
    ExecutionStats m_stats;
};

#endif // EXECUTION_ORCHESTRATOR_H