#include "layout_engine.h"
#include "scene.h"
#include "node.h"
#include "edge.h"
#include <QRandomGenerator>
#include <QtMath>
#include <algorithm>

LayoutEngine::LayoutEngine()
    : m_canvasWidth(800.0)
    , m_canvasHeight(600.0)
{
    qDebug() << "LayoutEngine: Initialized with lightweight backend";
}

void LayoutEngine::buildGraphFromScene(Scene* scene)
{
    if (!scene) {
        qCritical() << "LayoutEngine::buildGraphFromScene - null scene";
        return;
    }
    
    clear();
    
    // Add all nodes
    auto nodes = scene->getNodes();
    for (Node* node : nodes) {
        QUuid nodeId = node->getId();
        QPointF position = node->pos();
        addNode(nodeId, position);
    }
    
    // Add all edges  
    auto edges = scene->getEdges();
    for (Edge* edge : edges) {
        Node* fromNode = edge->getFromNode();
        Node* toNode = edge->getToNode();
        
        if (!fromNode || !toNode) {
            continue; // Skip unresolved edges
        }
        
        QUuid fromId = fromNode->getId();
        QUuid toId = toNode->getId();
        addEdge(fromId, toId, 1.0);
    }
    
    qDebug() << "LayoutEngine: Built graph from scene with" 
             << m_nodes.size() << "nodes and" 
             << m_edges.size() << "edges";
}

void LayoutEngine::addNode(const QUuid& nodeId, const QPointF& position)
{
    LayoutNode layoutNode(nodeId, position);
    m_nodes[nodeId] = layoutNode;
    
    qDebug() << "LayoutEngine: Added node" << nodeId.toString(QUuid::WithoutBraces).left(8)
             << "at position" << position;
}

void LayoutEngine::addEdge(const QUuid& fromNodeId, const QUuid& toNodeId, qreal weight)
{
    if (!m_nodes.contains(fromNodeId) || !m_nodes.contains(toNodeId)) {
        qCritical() << "LayoutEngine::addEdge - one or both nodes not found";
        return;
    }
    
    LayoutEdge layoutEdge(fromNodeId, toNodeId, weight);
    m_edges.append(layoutEdge);
    
    qDebug() << "LayoutEngine: Added edge" 
             << fromNodeId.toString(QUuid::WithoutBraces).left(8) << "->"
             << toNodeId.toString(QUuid::WithoutBraces).left(8);
}

void LayoutEngine::clear()
{
    m_nodes.clear();
    m_edges.clear();
    qDebug() << "LayoutEngine: Cleared graph data";
}

void LayoutEngine::applyGridLayout(qreal spacing)
{
    if (m_nodes.isEmpty()) {
        qWarning() << "LayoutEngine::applyGridLayout - no nodes to layout";
        return;
    }
    
    int gridSize = qCeil(qSqrt(m_nodes.size()));
    int currentIndex = 0;
    
    for (auto it = m_nodes.begin(); it != m_nodes.end(); ++it) {
        LayoutNode& layoutNode = it.value();
        
        if (!layoutNode.fixed) {
            int row = currentIndex / gridSize;
            int col = currentIndex % gridSize;
            
            QPointF newPosition(col * spacing, row * spacing);
            layoutNode.position = newPosition;
        }
        
        currentIndex++;
    }
    
    qDebug() << "LayoutEngine: Applied grid layout with spacing" << spacing;
}

void LayoutEngine::applyCircularLayout(qreal radius)
{
    if (m_nodes.isEmpty()) {
        qWarning() << "LayoutEngine::applyCircularLayout - no nodes to layout";
        return;
    }
    
    QPointF center(m_canvasWidth / 2.0, m_canvasHeight / 2.0);
    auto nodeIds = m_nodes.keys();
    int nodeCount = nodeIds.size();
    
    for (int i = 0; i < nodeCount; ++i) {
        QUuid nodeId = nodeIds[i];
        LayoutNode& layoutNode = m_nodes[nodeId];
        
        if (!layoutNode.fixed) {
            qreal angle = (2.0 * M_PI * i) / nodeCount;
            QPointF newPosition(
                center.x() + radius * qCos(angle),
                center.y() + radius * qSin(angle)
            );
            layoutNode.position = constrainToCanvas(newPosition);
        }
    }
    
    qDebug() << "LayoutEngine: Applied circular layout with radius" << radius;
}

void LayoutEngine::applyForceDirectedLayout(int iterations, qreal k)
{
    if (m_nodes.isEmpty()) {
        qWarning() << "LayoutEngine::applyForceDirectedLayout - no nodes to layout";
        return;
    }
    
    qreal timeStep = 0.1;
    qreal damping = 0.9;
    
    for (int iter = 0; iter < iterations; ++iter) {
        // Calculate forces for each node
        for (auto nodeIt = m_nodes.begin(); nodeIt != m_nodes.end(); ++nodeIt) {
            QUuid nodeId = nodeIt.key();
            LayoutNode& currentNode = nodeIt.value();
            
            if (currentNode.fixed) continue;
            
            QPointF totalForce(0, 0);
            
            // Repulsive forces from all other nodes
            for (auto otherIt = m_nodes.begin(); otherIt != m_nodes.end(); ++otherIt) {
                if (nodeIt.key() == otherIt.key()) continue;
                
                const LayoutNode& otherNode = otherIt.value();
                QPointF repulsiveForce = calculateRepulsiveForce(currentNode, otherNode, k);
                totalForce += repulsiveForce;
            }
            
            // Attractive forces from connected nodes
            for (const LayoutEdge& edge : m_edges) {
                QUuid connectedNodeId;
                
                if (edge.fromNodeId == nodeId) {
                    connectedNodeId = edge.toNodeId;
                } else if (edge.toNodeId == nodeId) {
                    connectedNodeId = edge.fromNodeId;
                } else {
                    continue; // Edge doesn't involve current node
                }
                
                if (m_nodes.contains(connectedNodeId)) {
                    const LayoutNode& connectedNode = m_nodes[connectedNodeId];
                    QPointF attractiveForce = calculateAttractiveForce(currentNode, connectedNode, k);
                    totalForce += attractiveForce;
                }
            }
            
            // Update velocity and position with damping
            currentNode.velocity = (currentNode.velocity + totalForce * timeStep) * damping;
            updateNodePosition(nodeId, currentNode.velocity, timeStep);
        }
        
        // Cool down the system
        timeStep *= 0.99;
    }
    
    qDebug() << "LayoutEngine: Applied force-directed layout with" << iterations << "iterations";
}

void LayoutEngine::applyRandomLayout(qreal width, qreal height)
{
    if (m_nodes.isEmpty()) {
        qWarning() << "LayoutEngine::applyRandomLayout - no nodes to layout";
        return;
    }
    
    auto* generator = QRandomGenerator::global();
    
    for (auto it = m_nodes.begin(); it != m_nodes.end(); ++it) {
        LayoutNode& layoutNode = it.value();
        
        if (!layoutNode.fixed) {
            QPointF newPosition(
                generator->bounded(width),
                generator->bounded(height)
            );
            layoutNode.position = newPosition;
        }
    }
    
    qDebug() << "LayoutEngine: Applied random layout in" << width << "x" << height << "area";
}

QHash<QUuid, QPointF> LayoutEngine::getNodePositions() const
{
    QHash<QUuid, QPointF> positions;
    
    for (auto it = m_nodes.begin(); it != m_nodes.end(); ++it) {
        QUuid nodeId = it.key();
        const LayoutNode& layoutNode = it.value();
        positions[nodeId] = layoutNode.position;
    }
    
    return positions;
}

void LayoutEngine::applyToScene(Scene* scene)
{
    if (!scene) {
        qCritical() << "LayoutEngine::applyToScene - null scene";
        return;
    }
    
    auto positions = getNodePositions();
    int appliedCount = 0;
    
    for (auto it = positions.begin(); it != positions.end(); ++it) {
        QUuid nodeId = it.key();
        QPointF position = it.value();
        
        Node* node = scene->getNode(nodeId);
        if (node) {
            node->setPos(position);
            appliedCount++;
        }
    }
    
    qDebug() << "LayoutEngine: Applied positions to" << appliedCount << "nodes in scene";
}

void LayoutEngine::setNodeFixed(const QUuid& nodeId, bool fixed)
{
    if (m_nodes.contains(nodeId)) {
        m_nodes[nodeId].fixed = fixed;
        
        qDebug() << "LayoutEngine: Set node" << nodeId.toString(QUuid::WithoutBraces).left(8)
                 << "fixed =" << fixed;
    }
}

void LayoutEngine::printGraphStats() const
{
    qDebug() << "=== LayoutEngine Graph Statistics ===";
    qDebug() << "Nodes:" << m_nodes.size();
    qDebug() << "Edges:" << m_edges.size();
    qDebug() << "Canvas size:" << m_canvasWidth << "x" << m_canvasHeight;
    
    // Print node details
    for (auto it = m_nodes.begin(); it != m_nodes.end(); ++it) {
        const LayoutNode& layoutNode = it.value();
        qDebug() << "  Node" << layoutNode.id.toString(QUuid::WithoutBraces).left(8)
                 << "at" << layoutNode.position
                 << (layoutNode.fixed ? "(fixed)" : "(free)");
    }
}

// Private helper functions

QPointF LayoutEngine::calculateRepulsiveForce(const LayoutNode& node1, const LayoutNode& node2, qreal k) const
{
    QPointF delta = node1.position - node2.position;
    qreal distance = qSqrt(delta.x() * delta.x() + delta.y() * delta.y());
    
    if (distance < 1.0) distance = 1.0; // Avoid division by zero
    
    qreal force = (k * k) / distance;
    QPointF direction = delta / distance;
    
    return direction * force;
}

QPointF LayoutEngine::calculateAttractiveForce(const LayoutNode& node1, const LayoutNode& node2, qreal k) const
{
    QPointF delta = node2.position - node1.position;
    qreal distance = qSqrt(delta.x() * delta.x() + delta.y() * delta.y());
    
    if (distance < 1.0) distance = 1.0; // Avoid division by zero
    
    qreal force = (distance * distance) / k;
    QPointF direction = delta / distance;
    
    return direction * force;
}

void LayoutEngine::updateNodePosition(const QUuid& nodeId, const QPointF& force, qreal timeStep)
{
    if (m_nodes.contains(nodeId)) {
        LayoutNode& layoutNode = m_nodes[nodeId];
        
        if (!layoutNode.fixed) {
            QPointF newPosition = layoutNode.position + force * timeStep;
            layoutNode.position = constrainToCanvas(newPosition);
        }
    }
}

QPointF LayoutEngine::constrainToCanvas(const QPointF& position) const
{
    qreal margin = 50.0;
    return QPointF(
        qBound(margin, position.x(), m_canvasWidth - margin),
        qBound(margin, position.y(), m_canvasHeight - margin)
    );
}