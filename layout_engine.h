#pragma once

#include <QPointF>
#include <QUuid>
#include <QString>
#include <QHash>
#include <QList>
#include <QPair>
#include <QDebug>
#include <QVector>

class Node;
class Edge;
class Scene;

/**
 * Layout Engine - Lightweight graph layout algorithms
 * 
 * Provides multiple layout algorithms for node graph positioning
 * without external dependencies beyond Qt5.
 */
class LayoutEngine
{
public:
    enum LayoutType {
        Grid,
        ForceDirected,
        Hierarchical,
        Circular,
        Random
    };
    
    struct LayoutNode {
        QUuid id;
        QPointF position;
        QPointF velocity;  // For force-directed algorithms
        bool fixed;        // Pin node position
        
        LayoutNode() : fixed(false) {}
        LayoutNode(const QUuid& nodeId, const QPointF& pos) 
            : id(nodeId), position(pos), fixed(false) {}
    };
    
    struct LayoutEdge {
        QUuid fromNodeId;
        QUuid toNodeId;
        qreal weight;      // Edge weight for algorithms
        
        LayoutEdge() : weight(1.0) {}
        LayoutEdge(const QUuid& from, const QUuid& to, qreal w = 1.0)
            : fromNodeId(from), toNodeId(to), weight(w) {}
    };
    
    LayoutEngine();
    
    // Graph construction from Scene
    void buildGraphFromScene(Scene* scene);
    void addNode(const QUuid& nodeId, const QPointF& position);
    void addEdge(const QUuid& fromNodeId, const QUuid& toNodeId, qreal weight = 1.0);
    void clear();
    
    // Layout algorithms
    void applyGridLayout(qreal spacing = 100.0);
    void applyCircularLayout(qreal radius = 200.0);
    void applyForceDirectedLayout(int iterations = 100, qreal k = 50.0);
    void applyRandomLayout(qreal width = 800.0, qreal height = 600.0);
    
    // Results
    QHash<QUuid, QPointF> getNodePositions() const;
    void applyToScene(Scene* scene);
    
    // Configuration
    void setCanvasSize(qreal width, qreal height) { m_canvasWidth = width; m_canvasHeight = height; }
    void setNodeFixed(const QUuid& nodeId, bool fixed);
    
    // Debug
    void printGraphStats() const;
    
private:
    QHash<QUuid, LayoutNode> m_nodes;  // Simple node storage
    QVector<LayoutEdge> m_edges;       // Simple edge storage
    
    qreal m_canvasWidth;
    qreal m_canvasHeight;
    
    // Force-directed algorithm helpers
    QPointF calculateRepulsiveForce(const LayoutNode& node1, const LayoutNode& node2, qreal k) const;
    QPointF calculateAttractiveForce(const LayoutNode& node1, const LayoutNode& node2, qreal k) const;
    void updateNodePosition(const QUuid& nodeId, const QPointF& force, qreal timeStep);
    
    // Utility functions
    QPointF constrainToCanvas(const QPointF& position) const;
};