#include "execution_orchestrator.h"
#include "scene.h"
#include "node.h"
#include "edge.h"
#include <QDebug>
#include <QElapsedTimer>
#include <QMutexLocker>
#include <algorithm>

ExecutionOrchestrator::ExecutionOrchestrator(Scene* scene, QObject* parent)
    : QObject(parent)
    , m_scene(scene)
    , m_dependencyGraphDirty(true)
    , m_memoizationEnabled(true)
    , m_executionTimeoutMs(5000)  // 5 second default timeout
{
    // Set up batched execution timer
    m_executionTimer = new QTimer(this);
    m_executionTimer->setSingleShot(true);
    m_executionTimer->setInterval(EXECUTION_DELAY_MS);
    connect(m_executionTimer, &QTimer::timeout, this, &ExecutionOrchestrator::processScheduledExecutions);
    
    // Attach to scene for graph change notifications
    if (m_scene) {
        m_scene->attach(this);
    }
    
    qDebug() << "ExecutionOrchestrator: Created with scene" << m_scene;
}

ExecutionOrchestrator::~ExecutionOrchestrator()
{
    if (m_scene) {
        m_scene->detach(this);
    }
}

void ExecutionOrchestrator::setExecutableSpec(std::unique_ptr<ExecutableSpec> spec)
{
    QMutexLocker locker(&m_mutex);
    m_executableSpec = std::move(spec);
    
    // Clear caches since execution behavior may have changed
    clearExecutionCache();
    m_dependencyGraphDirty = true;
    
    qDebug() << "ExecutionOrchestrator: ExecutableSpec updated";
}

QVariantMap ExecutionOrchestrator::executeNode(const QUuid& nodeId, const QVariantMap& inputs)
{
    if (!m_executableSpec) {
        qWarning() << "ExecutionOrchestrator: No ExecutableSpec configured";
        return QVariantMap();
    }
    
    emit nodeExecutionStarted(nodeId);
    
    ExecutionResult result = executeNodeInternal(nodeId, inputs);
    
    if (result.success) {
        emit nodeExecutionCompleted(nodeId, result);
        return result.outputs;
    } else {
        emit nodeExecutionFailed(nodeId, result.errorMessage);
        return QVariantMap();
    }
}

void ExecutionOrchestrator::executeSubgraph(const QSet<QUuid>& nodeIds)
{
    if (!m_executableSpec) {
        qWarning() << "ExecutionOrchestrator: No ExecutableSpec configured";
        return;
    }
    
    // Get topological order for all nodes
    QList<QUuid> allNodesOrdered = getTopologicalOrder();
    
    // Filter to only requested nodes, maintaining topological order
    QList<QUuid> orderedExecution;
    for (const QUuid& nodeId : allNodesOrdered) {
        if (nodeIds.contains(nodeId)) {
            orderedExecution.append(nodeId);
        }
    }
    
    qDebug() << "ExecutionOrchestrator: Executing subgraph of" << orderedExecution.size() << "nodes";
    
    // Execute in topological order
    for (const QUuid& nodeId : orderedExecution) {
        QVariantMap inputs = gatherNodeInputs(nodeId);
        executeNodeInternal(nodeId, inputs);
    }
    
    emit batchExecutionCompleted(nodeIds);
}

void ExecutionOrchestrator::executeAll()
{
    if (!m_scene) {
        qWarning() << "ExecutionOrchestrator: No scene configured";
        return;
    }
    
    QSet<QUuid> allNodeIds;
    for (Node* node : m_scene->getNodes().values()) {
        if (node) {
            allNodeIds.insert(node->getId());
        }
    }
    
    executeSubgraph(allNodeIds);
}

ExecutionResult ExecutionOrchestrator::executeNodeInternal(const QUuid& nodeId, const QVariantMap& inputs)
{
    QMutexLocker locker(&m_mutex);
    
    ExecutionResult result;
    result.success = false;
    
    if (!m_executableSpec) {
        result.errorMessage = "No ExecutableSpec configured";
        return result;
    }
    
    // Check if node can be executed
    if (!m_executableSpec->canExecute(nodeId)) {
        result.errorMessage = "Node is not executable (no script/implementation)";
        return result;
    }
    
    // Check cache if memoization is enabled
    QString cacheKey;
    if (m_memoizationEnabled) {
        cacheKey = computeCacheKey(nodeId, inputs);
        if (hasCachedResult(cacheKey)) {
            result = getCachedResult(cacheKey);
            m_stats.cacheHits++;
            qDebug() << "ExecutionOrchestrator: Cache hit for node" << nodeId.toString();
            return result;
        }
    }
    
    // Execute the node
    QElapsedTimer timer;
    timer.start();
    
    try {
        qDebug() << "ExecutionOrchestrator: Executing node" << nodeId.toString() << "with inputs:" << inputs;
        
        result.outputs = m_executableSpec->execute(nodeId, inputs);
        result.success = true;
        result.executionTimeMs = timer.elapsed();
        result.executionHash = m_executableSpec->getExecutionHash(nodeId);
        
        m_stats.totalExecutions++;
        m_stats.totalExecutionTimeMs += result.executionTimeMs;
        
        qDebug() << "ExecutionOrchestrator: Node execution completed in" << result.executionTimeMs << "ms";
        
    } catch (const std::exception& e) {
        result.errorMessage = QString("Execution failed: %1").arg(e.what());
        result.success = false;
        result.executionTimeMs = timer.elapsed();
        m_stats.failedExecutions++;
        
        qWarning() << "ExecutionOrchestrator: Node execution failed:" << result.errorMessage;
    }
    
    // Cache successful results
    if (result.success && m_memoizationEnabled && !cacheKey.isEmpty()) {
        setCachedResult(cacheKey, result);
    }
    
    // Store current result
    m_executionResults[nodeId] = result;
    
    return result;
}

QVariantMap ExecutionOrchestrator::gatherNodeInputs(const QUuid& nodeId) const
{
    if (!m_scene) {
        return QVariantMap();
    }
    
    Node* node = m_scene->getNode(nodeId);
    if (!node) {
        return QVariantMap();
    }
    
    QVariantMap inputs;
    
    // Gather inputs from connected upstream nodes
    // For now, use a simplified approach that collects all upstream outputs
    for (Edge* edge : m_scene->getEdges().values()) {
        if (edge && edge->getToNodeId() == nodeId) {
            QUuid fromNodeId = edge->getFromNodeId();
            int fromIndex = edge->getFromIndex();
            int toIndex = edge->getToIndex();
            
            // Get output from upstream node
            if (m_executionResults.contains(fromNodeId)) {
                const ExecutionResult& upstreamResult = m_executionResults[fromNodeId];
                if (upstreamResult.success) {
                    // Map from upstream output socket to our input socket
                    QString inputKey = QString("input_%1").arg(toIndex);
                    QString outputKey = QString("output_%1").arg(fromIndex);
                    
                    if (upstreamResult.outputs.contains(outputKey)) {
                        inputs[inputKey] = upstreamResult.outputs[outputKey];
                    }
                }
            }
        }
    }
    
    return inputs;
}

void ExecutionOrchestrator::invalidateDownstream(const QUuid& nodeId)
{
    QMutexLocker locker(&m_mutex);
    
    if (m_dependencyGraphDirty) {
        rebuildDependencyGraph();
    }
    
    // Get all downstream nodes
    QSet<QUuid> toInvalidate;
    QList<QUuid> queue = {nodeId};
    
    while (!queue.isEmpty()) {
        QUuid currentId = queue.takeFirst();
        
        if (m_dependents.contains(currentId)) {
            for (const QUuid& dependentId : m_dependents[currentId]) {
                if (!toInvalidate.contains(dependentId)) {
                    toInvalidate.insert(dependentId);
                    queue.append(dependentId);
                }
            }
        }
    }
    
    // Clear execution results for invalidated nodes
    for (const QUuid& invalidNodeId : toInvalidate) {
        m_executionResults.remove(invalidNodeId);
        qDebug() << "ExecutionOrchestrator: Invalidated node" << invalidNodeId.toString();
    }
    
    // Clear related cache entries
    if (m_memoizationEnabled) {
        // For now, clear entire cache on any invalidation
        // TODO: More granular cache invalidation
        m_executionCache.clear();
    }
}

void ExecutionOrchestrator::scheduleExecution(const QUuid& nodeId)
{
    QMutexLocker locker(&m_mutex);
    m_scheduledNodes.insert(nodeId);
    scheduleExecutionDelayed();
}

void ExecutionOrchestrator::scheduleExecutionDelayed()
{
    if (!m_executionTimer->isActive()) {
        m_executionTimer->start();
    }
}

void ExecutionOrchestrator::processScheduledExecutions()
{
    QSet<QUuid> nodesToExecute;
    {
        QMutexLocker locker(&m_mutex);
        nodesToExecute = m_scheduledNodes;
        m_scheduledNodes.clear();
    }
    
    if (!nodesToExecute.isEmpty()) {
        qDebug() << "ExecutionOrchestrator: Processing" << nodesToExecute.size() << "scheduled executions";
        executeSubgraph(nodesToExecute);
    }
}

QList<QUuid> ExecutionOrchestrator::getTopologicalOrder() const
{
    QMutexLocker locker(&m_mutex);
    
    if (m_dependencyGraphDirty) {
        const_cast<ExecutionOrchestrator*>(this)->rebuildDependencyGraph();
    }
    
    QList<QUuid> result;
    QSet<QUuid> visited;
    QSet<QUuid> recursionStack;
    
    // Start DFS from all nodes (handles disconnected components)  
    for (auto it = m_dependencies.begin(); it != m_dependencies.end(); ++it) {
        const QUuid& nodeId = it.key();
        if (!visited.contains(nodeId)) {
            topologicalSortRecursive(nodeId, visited, recursionStack, result);
        }
    }
    
    // Reverse to get correct topological order
    std::reverse(result.begin(), result.end());
    
    return result;
}

void ExecutionOrchestrator::topologicalSortRecursive(const QUuid& nodeId, QSet<QUuid>& visited,
                                                     QSet<QUuid>& recursionStack, QList<QUuid>& result) const
{
    if (recursionStack.contains(nodeId)) {
        qWarning() << "ExecutionOrchestrator: Cycle detected involving node" << nodeId.toString();
        return;
    }
    
    if (visited.contains(nodeId)) {
        return;
    }
    
    visited.insert(nodeId);
    recursionStack.insert(nodeId);
    
    // Visit all dependencies first
    if (m_dependencies.contains(nodeId)) {
        for (const QUuid& depId : m_dependencies[nodeId]) {
            topologicalSortRecursive(depId, visited, recursionStack, result);
        }
    }
    
    recursionStack.remove(nodeId);
    result.append(nodeId);
}

void ExecutionOrchestrator::rebuildDependencyGraph()
{
    m_dependencies.clear();
    m_dependents.clear();
    
    if (!m_scene) {
        m_dependencyGraphDirty = false;
        return;
    }
    
    // Initialize empty dependency sets for all nodes
    for (Node* node : m_scene->getNodes().values()) {
        if (node) {
            QUuid nodeId = node->getId();
            m_dependencies[nodeId] = QSet<QUuid>();
            m_dependents[nodeId] = QSet<QUuid>();
        }
    }
    
    // Build dependency graph from edges
    for (Edge* edge : m_scene->getEdges().values()) {
        if (edge) {
            QUuid fromNodeId = edge->getFromNodeId();
            QUuid toNodeId = edge->getToNodeId();
            
            // toNode depends on fromNode
            m_dependencies[toNodeId].insert(fromNodeId);
            m_dependents[fromNodeId].insert(toNodeId);
        }
    }
    
    m_dependencyGraphDirty = false;
    
    qDebug() << "ExecutionOrchestrator: Rebuilt dependency graph with" 
             << m_dependencies.size() << "nodes";
}

QString ExecutionOrchestrator::computeCacheKey(const QUuid& nodeId, const QVariantMap& inputs) const
{
    if (!m_executableSpec) {
        return QString();
    }
    
    QString executionHash = m_executableSpec->getExecutionHash(nodeId);
    
    // Create a hash of the inputs
    QStringList inputPairs;
    for (auto it = inputs.begin(); it != inputs.end(); ++it) {
        inputPairs.append(QString("%1:%2").arg(it.key(), it.value().toString()));
    }
    inputPairs.sort(); // Ensure consistent ordering
    
    QString inputsHash = QString::number(qHash(inputPairs.join("|")));
    
    return QString("%1:%2:%3").arg(nodeId.toString(), executionHash, inputsHash);
}

bool ExecutionOrchestrator::hasCachedResult(const QString& cacheKey) const
{
    return m_executionCache.contains(cacheKey);
}

ExecutionResult ExecutionOrchestrator::getCachedResult(const QString& cacheKey) const
{
    return m_executionCache.value(cacheKey, ExecutionResult());
}

void ExecutionOrchestrator::setCachedResult(const QString& cacheKey, const ExecutionResult& result)
{
    m_executionCache[cacheKey] = result;
}

bool ExecutionOrchestrator::hasExecutionResult(const QUuid& nodeId) const
{
    QMutexLocker locker(&m_mutex);
    return m_executionResults.contains(nodeId);
}

ExecutionResult ExecutionOrchestrator::getExecutionResult(const QUuid& nodeId) const
{
    QMutexLocker locker(&m_mutex);
    return m_executionResults.value(nodeId, ExecutionResult());
}

void ExecutionOrchestrator::clearExecutionCache()
{
    QMutexLocker locker(&m_mutex);
    m_executionResults.clear();
    m_executionCache.clear();
    qDebug() << "ExecutionOrchestrator: Execution cache cleared";
}

void ExecutionOrchestrator::clearExecutionCache(const QUuid& nodeId)
{
    QMutexLocker locker(&m_mutex);
    m_executionResults.remove(nodeId);
    
    // Remove cache entries related to this node
    // For now, clear entire cache (more efficient than scanning all keys)
    m_executionCache.clear();
    
    qDebug() << "ExecutionOrchestrator: Cache cleared for node" << nodeId.toString();
}

QSet<QUuid> ExecutionOrchestrator::getUpstreamNodes(const QUuid& nodeId) const
{
    QMutexLocker locker(&m_mutex);
    
    if (m_dependencyGraphDirty) {
        const_cast<ExecutionOrchestrator*>(this)->rebuildDependencyGraph();
    }
    
    return m_dependencies.value(nodeId, QSet<QUuid>());
}

QSet<QUuid> ExecutionOrchestrator::getDownstreamNodes(const QUuid& nodeId) const
{
    QMutexLocker locker(&m_mutex);
    
    if (m_dependencyGraphDirty) {
        const_cast<ExecutionOrchestrator*>(this)->rebuildDependencyGraph();
    }
    
    return m_dependents.value(nodeId, QSet<QUuid>());
}

// GraphObserver interface implementation
void ExecutionOrchestrator::onNodeAdded(const Node& node)
{
    QMutexLocker locker(&m_mutex);
    m_dependencyGraphDirty = true;
    qDebug() << "ExecutionOrchestrator: Node added" << node.getId().toString();
}

void ExecutionOrchestrator::onNodeRemoved(const QUuid& nodeId)
{
    QMutexLocker locker(&m_mutex);
    m_dependencyGraphDirty = true;
    m_executionResults.remove(nodeId);
    m_scheduledNodes.remove(nodeId);
    qDebug() << "ExecutionOrchestrator: Node removed" << nodeId.toString();
}

void ExecutionOrchestrator::onEdgeAdded(const Edge& edge)
{
    QMutexLocker locker(&m_mutex);
    m_dependencyGraphDirty = true;
    
    // Invalidate downstream nodes of the target node
    invalidateDownstream(edge.getToNodeId());
    
    // Schedule execution of affected nodes
    scheduleExecution(edge.getToNodeId());
    
    qDebug() << "ExecutionOrchestrator: Edge added" << edge.getId().toString();
}

void ExecutionOrchestrator::onEdgeRemoved(const QUuid& edgeId)
{
    QMutexLocker locker(&m_mutex);
    m_dependencyGraphDirty = true;
    
    // Note: We don't have edge details here, so invalidate more broadly
    // TODO: Store edge info before removal for more precise invalidation
    m_executionResults.clear();
    m_executionCache.clear();
    
    qDebug() << "ExecutionOrchestrator: Edge removed" << edgeId.toString();
}

void ExecutionOrchestrator::onGraphCleared()
{
    QMutexLocker locker(&m_mutex);
    m_dependencyGraphDirty = true;
    m_executionResults.clear();
    m_executionCache.clear();
    m_scheduledNodes.clear();
    m_dependencies.clear();
    m_dependents.clear();
    
    qDebug() << "ExecutionOrchestrator: Graph cleared";
}

