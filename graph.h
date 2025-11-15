#pragma once

#include <QObject>
#include <QString>
#include <QVariantMap>
#include <QVariantList>
#include <QJSEngine>
#include <QJSValue>
#include <QUuid>

class Scene;
class GraphFactory;
class Node;
class Edge;

/**
 * Graph - Public API facade for graph operations with JavaScript integration
 *
 * This class provides a clean public API for all graph operations and coordinates
 * between Scene (graphics management), GraphFactory (object creation), and
 * Observers (change notifications).
 *
 * Key responsibilities:
 * - Public API: createNode(), deleteNode(), connectNodes(), etc.
 * - Operation validation (type checking, UUID validation)
 * - Coordinate between Scene/Factory/Observers
 * - JavaScript engine integration (QJSEngine - always enabled)
 * - Emit signals for all state changes
 *
 * Key design principles:
 * - Scene does graphics (QGraphicsScene duties)
 * - Graph does coordination and provides public API
 * - Factory does object creation
 * - No conditional compilation - JavaScript is always available
 */
class Graph : public QObject
{
    Q_OBJECT

public:
    explicit Graph(Scene* scene, GraphFactory* factory, QObject* parent = nullptr);
    ~Graph();

    // ========== Node Operations ==========

    /**
     * Create node at position, returns UUID string
     * @param type Node type (SOURCE, SINK, TRANSFORM, etc.)
     * @param x X coordinate in scene
     * @param y Y coordinate in scene
     * @return UUID string of created node, or empty string on failure
     */
    Q_INVOKABLE QString createNode(const QString& type, qreal x, qreal y);

    /**
     * Delete node and its connected edges
     * @param nodeId UUID string of node to delete
     * @return true if node was deleted, false if not found
     */
    Q_INVOKABLE bool deleteNode(const QString& nodeId);

    /**
     * Move node by delta
     * @param nodeId UUID string of node
     * @param dx Delta X
     * @param dy Delta Y
     * @return true if node was moved, false if not found
     */
    Q_INVOKABLE bool moveNode(const QString& nodeId, qreal dx, qreal dy);

    /**
     * Set node position absolutely
     * @param nodeId UUID string of node
     * @param x New X coordinate
     * @param y New Y coordinate
     * @return true if position was set, false if not found
     */
    Q_INVOKABLE bool setNodePosition(const QString& nodeId, qreal x, qreal y);

    /**
     * Get node data as variant map (for JavaScript)
     * @param nodeId UUID string of node
     * @return Map with node properties (id, type, x, y, etc.)
     */
    Q_INVOKABLE QVariantMap getNodeData(const QString& nodeId) const;

    // ========== Edge Operations ==========

    /**
     * Connect two nodes by socket indices
     * @param fromNodeId UUID string of source node
     * @param fromSocketIndex Index of output socket on source node
     * @param toNodeId UUID string of destination node
     * @param toSocketIndex Index of input socket on destination node
     * @return UUID string of created edge, or empty string on failure
     */
    Q_INVOKABLE QString connectNodes(const QString& fromNodeId, int fromSocketIndex,
                                     const QString& toNodeId, int toSocketIndex);

    /**
     * Delete edge
     * @param edgeId UUID string of edge to delete
     * @return true if edge was deleted, false if not found
     */
    Q_INVOKABLE bool deleteEdge(const QString& edgeId);

    /**
     * Get edge data as variant map
     * @param edgeId UUID string of edge
     * @return Map with edge properties (id, from, to, etc.)
     */
    Q_INVOKABLE QVariantMap getEdgeData(const QString& edgeId) const;

    // ========== Graph Queries ==========

    /**
     * Get all nodes (returns array of UUID strings)
     * @return List of node UUID strings
     */
    Q_INVOKABLE QVariantList getAllNodes() const;

    /**
     * Get all edges (returns array of UUID strings)
     * @return List of edge UUID strings
     */
    Q_INVOKABLE QVariantList getAllEdges() const;

    /**
     * Get selected nodes
     * @return List of selected node UUID strings
     */
    Q_INVOKABLE QVariantList getSelectedNodes() const;

    /**
     * Get selected edges
     * @return List of selected edge UUID strings
     */
    Q_INVOKABLE QVariantList getSelectedEdges() const;

    /**
     * Get connected edges for a node
     * @param nodeId UUID string of node
     * @return List of edge UUID strings connected to this node
     */
    Q_INVOKABLE QVariantList getNodeEdges(const QString& nodeId) const;

    /**
     * Get graph statistics
     * @return Map with nodeCount, edgeCount, etc.
     */
    Q_INVOKABLE QVariantMap getGraphStats() const;

    // ========== Batch Operations ==========

    /**
     * Begin batch mode (defer observer notifications)
     * Useful for creating many nodes/edges at once
     */
    Q_INVOKABLE void beginBatch();

    /**
     * End batch mode (emit accumulated notifications)
     */
    Q_INVOKABLE void endBatch();

    /**
     * Check if batch mode is active
     * @return true if in batch mode
     */
    Q_INVOKABLE bool isBatchMode() const { return m_batchMode; }

    // ========== Graph-wide Operations ==========

    /**
     * Clear entire graph
     */
    Q_INVOKABLE void clearGraph();

    /**
     * Delete currently selected nodes and edges
     * @return true if any selection was deleted
     */
    Q_INVOKABLE bool deleteSelection();

    /**
     * Save graph to XML file
     * @param filePath Path to save file
     * @return true if save succeeded, false otherwise
     */
    Q_INVOKABLE bool saveToFile(const QString& filePath);

    /**
     * Load graph from XML file
     * @param filePath Path to load file
     * @return true if load succeeded, false otherwise
     */
    Q_INVOKABLE bool loadFromFile(const QString& filePath);

    /**
     * Get graph as XML string
     * @return XML representation of graph
     */
    Q_INVOKABLE QString toXml() const;

    // ========== Validation ==========

    /**
     * Check if node type is valid
     * @param type Node type string
     * @return true if type is valid
     */
    Q_INVOKABLE bool isValidNodeType(const QString& type) const;

    /**
     * Get available node types
     * @return List of valid node type strings
     */
    Q_INVOKABLE QStringList getAvailableNodeTypes() const;

    // ========== JavaScript Engine ==========

    /**
     * Evaluate JavaScript code
     * @param script JavaScript code to execute
     * @return Result of evaluation
     */
    Q_INVOKABLE QJSValue evalScript(const QString& script);

    /**
     * Evaluate JavaScript file
     * @param filePath Path to JavaScript file
     * @return Result of evaluation
     */
    Q_INVOKABLE QJSValue evalFile(const QString& filePath);

    /**
     * Get JavaScript engine (for advanced use)
     * @return Pointer to QJSEngine
     */
    QJSEngine* jsEngine() { return m_jsEngine; }

    /**
     * JavaScript console.log implementation
     * @param message Message to log
     */
    Q_INVOKABLE void jsLog(const QString& message);

signals:
    // Change notifications (for JavaScript listeners and UI)
    void nodeCreated(const QString& nodeId);
    void nodeDeleted(const QString& nodeId);
    void nodeMoved(const QString& nodeId);

    void edgeCreated(const QString& edgeId);
    void edgeDeleted(const QString& edgeId);

    void graphCleared();
    void graphLoaded();
    void graphSaved(const QString& filePath);

    void errorOccurred(const QString& message);

private:
    Scene* m_scene;              // Graphics management (non-owning)
    GraphFactory* m_factory;     // Object creation (non-owning)
    QJSEngine* m_jsEngine;       // JavaScript engine (owned)
    bool m_batchMode;

    // Internal helpers
    Node* findNode(const QString& uuidStr) const;
    Edge* findEdge(const QString& uuidStr) const;
    QUuid parseUuid(const QString& uuidStr) const;

    // Initialize JavaScript engine and expose Graph API
    void initializeJavaScript();
};
