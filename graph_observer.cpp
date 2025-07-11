#include "graph_observer.h"
#include "node.h"
#include "edge.h"
#include <QDebug>

// ============================================================================
// GraphSubject Implementation
// ============================================================================

// Initialize static batch depth
int GraphSubject::s_batchDepth = 0;

GraphSubject::~GraphSubject()
{
    // CRITICAL FIX: Clear observer container properly to prevent dangling pointers
    // The old code was setting local pointer to nullptr, not the container elements!
    qDebug() << "GraphSubject: Destroying subject with" << m_observers.size() << "observers";
    
    // Simply clear the container - observers manage their own lifecycle
    m_observers.clear();
    
    qDebug() << "GraphSubject: ✓ Observer container cleared safely";
}

void GraphSubject::attach(GraphObserver* observer)
{
    if (observer) {
        m_observers.insert(observer);
        qDebug() << "GraphSubject: Observer attached, total observers:" << m_observers.size();
    }
}

void GraphSubject::detach(GraphObserver* observer)
{
    if (observer && m_observers.remove(observer)) {
        qDebug() << "GraphSubject: Observer detached, remaining observers:" << m_observers.size();
    }
}

void GraphSubject::notifyNodeAdded(const Node& node)
{
    // OPTIMIZATION: Skip notifications during batch operations
    if (isInBatch()) return;
    
    qDebug() << "GraphSubject: Notifying" << m_observers.size() << "observers of node added:" 
             << node.getId().toString(QUuid::WithoutBraces).left(8);
    
    for (GraphObserver* observer : m_observers) {
        if (observer) {
            observer->onNodeAdded(node);
        }
    }
}

void GraphSubject::notifyNodeRemoved(const QUuid& nodeId)
{
    // OPTIMIZATION: Skip notifications during batch operations
    if (isInBatch()) return;
    
    qDebug() << "GraphSubject: Notifying" << m_observers.size() << "observers of node removed:" 
             << nodeId.toString(QUuid::WithoutBraces).left(8);
    
    for (GraphObserver* observer : m_observers) {
        if (observer) {
            observer->onNodeRemoved(nodeId);
        }
    }
}

void GraphSubject::notifyNodeMoved(const QUuid& nodeId, QPointF oldPos, QPointF newPos)
{
    // OPTIMIZATION: Skip notifications during batch operations
    if (isInBatch()) return;
    
    qDebug() << "GraphSubject: Notifying" << m_observers.size() << "observers of node moved:" 
             << nodeId.toString(QUuid::WithoutBraces).left(8) << "from" << oldPos << "to" << newPos;
    
    for (GraphObserver* observer : m_observers) {
        if (observer) {
            observer->onNodeMoved(nodeId, oldPos, newPos);
        }
    }
}

void GraphSubject::notifyEdgeAdded(const Edge& edge)
{
    // OPTIMIZATION: Skip notifications during batch operations
    if (isInBatch()) return;
    
    qDebug() << "GraphSubject: Notifying" << m_observers.size() << "observers of edge added:" 
             << edge.getId().toString(QUuid::WithoutBraces).left(8);
    
    for (GraphObserver* observer : m_observers) {
        if (observer) {
            observer->onEdgeAdded(edge);
        }
    }
}

void GraphSubject::notifyEdgeRemoved(const QUuid& edgeId)
{
    // OPTIMIZATION: Skip notifications during batch operations
    if (isInBatch()) return;
    
    qDebug() << "GraphSubject: Notifying" << m_observers.size() << "observers of edge removed:" 
             << edgeId.toString(QUuid::WithoutBraces).left(8);
    
    for (GraphObserver* observer : m_observers) {
        if (observer) {
            observer->onEdgeRemoved(edgeId);
        }
    }
}

void GraphSubject::notifyGraphCleared()
{
    qDebug() << "GraphSubject: Notifying" << m_observers.size() << "observers of graph cleared";
    
    for (GraphObserver* observer : m_observers) {
        if (observer) {
            observer->onGraphCleared();
        }
    }
}

void GraphSubject::notifyGraphLoaded(const QString& filename)
{
    qDebug() << "GraphSubject: Notifying" << m_observers.size() << "observers of graph loaded:" << filename;
    
    for (GraphObserver* observer : m_observers) {
        if (observer) {
            observer->onGraphLoaded(filename);
        }
    }
}

void GraphSubject::notifyGraphSaved(const QString& filename)
{
    qDebug() << "GraphSubject: Notifying" << m_observers.size() << "observers of graph saved:" << filename;
    
    for (GraphObserver* observer : m_observers) {
        if (observer) {
            observer->onGraphSaved(filename);
        }
    }
}

// ============================================================================
// Batch Mode Implementation
// ============================================================================

void GraphSubject::beginBatch()
{
    ++s_batchDepth;
    qDebug() << "GraphSubject: Begin batch mode (depth:" << s_batchDepth << ")";
}

void GraphSubject::endBatch()
{
    if (s_batchDepth > 0) {
        --s_batchDepth;
        qDebug() << "GraphSubject: End batch mode (depth:" << s_batchDepth << ")";
        
        if (s_batchDepth == 0) {
            qDebug() << "GraphSubject: Batch complete - observers can resume";
        }
    }
}