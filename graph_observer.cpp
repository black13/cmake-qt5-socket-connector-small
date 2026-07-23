#include "graph_observer.h"
#include "node.h"
#include "edge.h"
#include <QDebug>

// ============================================================================
// GraphObserver Implementation
// ============================================================================

GraphObserver::~GraphObserver()
{
    // Self-detach from every subject so no subject is left holding a dangling
    // pointer to this observer (subjects iterate raw GraphObserver* on notify).
    const auto subjects = m_subjects; // copy: detach() mutates m_subjects
    for (GraphSubject* subject : subjects) {
        if (subject) {
            subject->detach(this);
        }
    }
}

void GraphObserver::addSubject(GraphSubject* subject)
{
    if (subject) {
        m_subjects.insert(subject);
    }
}

void GraphObserver::removeSubject(GraphSubject* subject)
{
    m_subjects.remove(subject);
}

// ============================================================================
// GraphSubject Implementation
// ============================================================================

// Initialize static batch depth
int GraphSubject::s_batchDepth = 0;

// Registry of all live subjects (for the end-of-batch observer flush)
QSet<GraphSubject*> GraphSubject::s_subjects;

GraphSubject::GraphSubject()
{
    s_subjects.insert(this);
}

GraphSubject::~GraphSubject()
{
    s_subjects.remove(this);
    
    // Unregister ourselves from every observer's subject list, so their
    // self-detach (GraphObserver::~GraphObserver) never calls back into this
    // destroyed subject. (The old code only nulled a local pointer - fixed.)
    qDebug() << "GraphSubject: Destroying subject with" << m_observers.size() << "observers";
    
    for (GraphObserver* observer : m_observers) {
        if (observer) {
            observer->removeSubject(this);
        }
    }
    m_observers.clear();
    
    qDebug() << "GraphSubject: Observer container cleared safely";
}

void GraphSubject::attach(GraphObserver* observer)
{
    if (observer) {
        m_observers.insert(observer);
        observer->addSubject(this);
        qDebug() << "GraphSubject: Observer attached, total observers:" << m_observers.size();
    }
}

void GraphSubject::detach(GraphObserver* observer)
{
    if (observer && m_observers.remove(observer)) {
        observer->removeSubject(this);
        qDebug() << "GraphSubject: Observer detached, remaining observers:" << m_observers.size();
    }
}

void GraphSubject::notifyNodeAdded(const Node& node)
{
    // OPTIMIZATION: Skip notifications during batch operations
    if (isInBatch()) return;
    
    qDebug() << "GraphSubject: Notifying" << m_observers.size() << "observers of node added:" 
             << node.getId().toString(QUuid::WithoutBraces).left(8);
    
    // Iterate a copy: observers may detach or be destroyed inside callbacks
    const auto observers = m_observers;
    for (GraphObserver* observer : observers) {
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
    
    const auto observers = m_observers;
    for (GraphObserver* observer : observers) {
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
    
    const auto observers = m_observers;
    for (GraphObserver* observer : observers) {
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
    
    const auto observers = m_observers;
    for (GraphObserver* observer : observers) {
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
    
    const auto observers = m_observers;
    for (GraphObserver* observer : observers) {
        if (observer) {
            observer->onEdgeRemoved(edgeId);
        }
    }
}

void GraphSubject::notifyGraphCleared()
{
    qDebug() << "GraphSubject: Notifying" << m_observers.size() << "observers of graph cleared";
    
    const auto observers = m_observers;
    for (GraphObserver* observer : observers) {
        if (observer) {
            observer->onGraphCleared();
        }
    }
}

void GraphSubject::notifyGraphLoaded(const QString& filename)
{
    qDebug() << "GraphSubject: Notifying" << m_observers.size() << "observers of graph loaded:" << filename;
    
    const auto observers = m_observers;
    for (GraphObserver* observer : observers) {
        if (observer) {
            observer->onGraphLoaded(filename);
        }
    }
}

void GraphSubject::notifyGraphSaved(const QString& filename)
{
    qDebug() << "GraphSubject: Notifying" << m_observers.size() << "observers of graph saved:" << filename;
    
    const auto observers = m_observers;
    for (GraphObserver* observer : observers) {
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
            // End-of-batch flush: mutations inside the batch were muted, so
            // give every subject's observers a single catch-up notification.
            qDebug() << "GraphSubject: Batch complete - flushing observers";
            const auto subjects = s_subjects; // copy: callbacks may reenter
            for (GraphSubject* subject : subjects) {
                if (subject) {
                    subject->flushBatchObservers();
                }
            }
        }
    }
}

void GraphSubject::flushBatchObservers()
{
    const auto observers = m_observers; // copy: callbacks may detach
    for (GraphObserver* observer : observers) {
        if (observer) {
            observer->onBatchEnded();
        }
    }
}
