#include "scene.h"
#include "node.h"
#include "edge.h"
#include "socket.h"
#include <QDebug>

Scene::Scene(QObject* parent)
    : QGraphicsScene(parent)
{
    setSceneRect(-1000, -1000, 2000, 2000);
}

// QElectroTech-style QHash implementation with SIMPLE_FIX logging
void Scene::addNode(Node* node)
{
    if (!node) return;
    
    QUuid nodeId = node->getId();
    m_nodes.insert(nodeId, node);
    addItem(node);
    
    qDebug() << "+" << nodeId.toString(QUuid::WithoutBraces).left(8);
    
    // Notify observers of node addition
    notifyNodeAdded(*node);
    
    // Emit signal for UI updates
    emit sceneChanged();
}

void Scene::addEdge(Edge* edge)
{
    if (!edge) return;
    
    QUuid edgeId = edge->getId();
    m_edges.insert(edgeId, edge);
    addItem(edge);
    
    qDebug() << "+" << edgeId.toString(QUuid::WithoutBraces).left(8);
    
    // Notify observers of edge addition
    notifyEdgeAdded(*edge);
    
    // Emit signal for UI updates
    emit sceneChanged();
    
    // Clean design: edges manage their own socket connections via resolveConnections()
}

void Scene::addSocket(Socket* socket)
{
    if (!socket) return;
    
    // Clean design: sockets are managed by their parent nodes, not scene
    // Socket is automatically added to scene as child of parent node
}

void Scene::removeNode(const QUuid& nodeId)
{
    Node* node = m_nodes.value(nodeId, nullptr);
    if (!node) return;
    
    // Clean design: sockets are children of nodes - no separate tracking needed
    
    // Remove connected edges that reference this node
    QList<QUuid> edgesToRemove;
    QString nodeIdStr = nodeId.toString();
    for (Edge* edge : m_edges.values()) {
        if (edge->isConnectedToNode(nodeIdStr)) {
            edgesToRemove.append(edge->getId());
        }
    }
    
    // Clean up edges first (they may reference sockets)
    for (const QUuid& edgeId : edgesToRemove) {
        removeEdge(edgeId);
    }
    
    // Clean design: sockets cleaned up automatically as node children
    
    // Finally remove the node
    removeItem(node);
    m_nodes.remove(nodeId);
    delete node;
}

void Scene::removeEdge(const QUuid& edgeId)
{
    Edge* edge = m_edges.value(edgeId, nullptr);
    if (!edge) return;
    
    // Clean design: edges manage their own socket disconnection via direct pointers
    // Socket cleanup handled automatically when edge is destroyed
    
    // Remove from scene and registry
    removeItem(edge);
    m_edges.remove(edgeId);
    delete edge;
}

// Clean design: socket management methods removed - sockets handled by parent nodes

// O(1) UUID lookups
Node* Scene::getNode(const QUuid& nodeId) const
{
    return m_nodes.value(nodeId, nullptr);
}

Edge* Scene::getEdge(const QUuid& edgeId) const
{
    return m_edges.value(edgeId, nullptr);
}

void Scene::deleteNode(const QUuid& nodeId)
{
    Node* node = getNode(nodeId);
    if (!node) {
        qWarning() << "Scene::deleteNode - node not found:" << nodeId.toString(QUuid::WithoutBraces).left(8);
        return;
    }
    
    qDebug() << "Deleting node:" << nodeId.toString(QUuid::WithoutBraces).left(8);
    
    // First, find and delete all edges connected to this node
    QList<QUuid> edgesToDelete;
    for (auto it = m_edges.begin(); it != m_edges.end(); ++it) {
        Edge* edge = it.value();
        if (edge->isConnectedToNode(nodeId)) {
            edgesToDelete.append(it.key());
        }
    }
    
    // Delete connected edges
    for (const QUuid& edgeId : edgesToDelete) {
        deleteEdge(edgeId);
    }
    
    // Remove node from collections and scene
    m_nodes.remove(nodeId);
    removeItem(node);
    
    // Notify observers BEFORE deleting the node
    notifyNodeRemoved(nodeId);
    
    delete node;
    
    // Emit signal for UI updates
    emit sceneChanged();
    
    qDebug() << "Node deleted with" << edgesToDelete.size() << "connected edges - Observer notified";
}

void Scene::deleteEdge(const QUuid& edgeId)
{
    Edge* edge = getEdge(edgeId);
    if (!edge) {
        qWarning() << "Scene::deleteEdge - edge not found:" << edgeId.toString(QUuid::WithoutBraces).left(8);
        return;
    }
    
    qDebug() << "Deleting edge:" << edgeId.toString(QUuid::WithoutBraces).left(8);
    
    // Remove from collection and scene
    m_edges.remove(edgeId);
    removeItem(edge);
    
    // Notify observers BEFORE deleting the edge
    notifyEdgeRemoved(edgeId);
    
    delete edge;
    
    // Emit signal for UI updates
    emit sceneChanged();
    
    qDebug() << "Edge deleted - Observer notified";
}

void Scene::deleteSelected()
{
    QList<QGraphicsItem*> selectedItems = this->selectedItems();
    if (selectedItems.isEmpty()) {
        qDebug() << "No items selected for deletion";
        return;
    }
    
    qDebug() << "DELETE KEY: Deleting" << selectedItems.size() << "selected items";
    
    // Separate nodes and edges for proper deletion order
    QList<Node*> selectedNodes;
    QList<Edge*> selectedEdges;
    
    for (QGraphicsItem* item : selectedItems) {
        if (Node* node = qgraphicsitem_cast<Node*>(item)) {
            selectedNodes.append(node);
        } else if (Edge* edge = qgraphicsitem_cast<Edge*>(item)) {
            selectedEdges.append(edge);
        }
    }
    
    // Delete selected edges first
    for (Edge* edge : selectedEdges) {
        deleteEdge(edge->getId());
    }
    
    // Then delete selected nodes (which will delete their remaining edges)
    for (Node* node : selectedNodes) {
        deleteNode(node->getId());
    }
    
    // Emit signal for UI updates
    emit sceneChanged();
    
    qDebug() << "DELETE COMPLETE: Deleted" << selectedEdges.size() << "edges and" << selectedNodes.size() << "nodes - Observers notified";
}

void Scene::clearGraph()
{
    qDebug() << "SIMPLE_FIX: Clearing graph - removing" << m_nodes.size() << "nodes and" << m_edges.size() << "edges";
    
    // SIMPLE FIX: Clear registries FIRST to prevent dangling pointers
    // This prevents hash lookups during Qt's destruction sequence
    qDebug() << "SIMPLE_FIX: Clearing hash registries first";
    m_nodes.clear();
    m_edges.clear();
    m_sockets.clear();  // Clear deprecated socket registry too
    
    // Then clear Qt graphics scene (safe now - no hash references)
    qDebug() << "SIMPLE_FIX: Clearing Qt scene items";
    QGraphicsScene::clear();
    
    // Notify observers of graph clearing
    notifyGraphCleared();
    
    // Emit signal for UI updates
    emit sceneChanged();
    
    qDebug() << "SIMPLE_FIX: ✓ Graph cleared safely - hash cleared before Qt cleanup";
}