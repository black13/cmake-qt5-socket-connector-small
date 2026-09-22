#pragma once

#include <QObject>
#include <QString>
#include <QVariantMap>
#include <QVariantList>
#include <QUuid>
#include "script_engine.h"

class Scene;
class GraphFactory;
class Node;
class Edge;
class QUndoStack;
class QUndoCommand;

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
 * - Script engine integration via type-erased ScriptEngine (QJSEngine backend
 *   by default; other backends plug in without touching this class)
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
     * Create a node and animate it into place, so scripted creation looks
     * like a palette drop. Uses the same factory/template path as
     * createNode(); the node starts slightly offset and faded, then eases to
     * (x, y) with full opacity over durationMs (0 skips the animation).
     *
     * When an undo stack is attached the creation is undoable like any other
     * facade mutation (the animation itself is not recorded).
     *
     * Two overloads (no default argument) so QJSEngine can invoke either
     * arity predictably; scripts may call dropNode(type, x, y).
     *
     * @return UUID string of created node, or empty string on failure
     */
    Q_INVOKABLE QString dropNode(const QString& type, qreal x, qreal y);
    Q_INVOKABLE QString dropNode(const QString& type, qreal x, qreal y, int durationMs);

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

    // ========== Scripted Nodes ==========
    Q_INVOKABLE bool setNodeScript(const QString& nodeId, const QString& scriptCode);
    Q_INVOKABLE QString getNodeScript(const QString& nodeId) const;
    Q_INVOKABLE QVariant executeNodeScript(const QString& nodeId, const QVariantMap& context = QVariantMap());

    /**
     * Get the error from the node's most recent script evaluation
     * @param nodeId UUID string of node
     * @return Error text, or empty string if the last run succeeded
     */
    Q_INVOKABLE QString getNodeScriptError(const QString& nodeId) const;
    Q_INVOKABLE bool setNodePayload(const QString& nodeId, const QVariantMap& payload);
    Q_INVOKABLE QVariantMap getNodePayload(const QString& nodeId) const;
    Q_INVOKABLE QVariantMap runSyntheticWork(const QVariantMap& request) const;

    // ========== Grid & View Helpers ==========

    /// Toggle grid snapping (drag + drop use it; programmatic positions are
    /// left exact unless snapNode/snapNodes is called explicitly).
    Q_INVOKABLE void setSnapToGrid(bool on);
    Q_INVOKABLE bool isSnapToGrid() const;
    Q_INVOKABLE int gridSize() const;

    /// Snap a coordinate pair to the grid. Empty map + error on non-finite.
    Q_INVOKABLE QVariantMap snapPoint(qreal x, qreal y);

    /// Snap one node / every node to the grid (undoable; returns false / count).
    Q_INVOKABLE bool snapNode(const QString& nodeId);
    Q_INVOKABLE int snapNodes();

    /**
     * Lay the whole graph out by topology: sources to the left, each following
     * layer one column to the right (longest-path layering), rows centered
     * per column, everything aligned to the grid. One undoable move command.
     * Cyclic/remaining nodes are parked in a final column so the layout is
     * always finite.
     *
     * @return number of nodes whose position changed
     */
    Q_INVOKABLE int alignGraph();

    // ========== Edge Operations ==========

    /**
     * Connect two nodes by socket indices
     *
     * Socket indices are per-node and GLOBAL: inputs occupy 0..n-1, then
     * outputs follow. This matches Socket::getIndex(), the index painted on
     * each socket, XML persistence, and undo snapshots. Examples: SOURCE
     * output = 0, TRANSFORM output = 1, MERGE inputs = 0/1 and output = 2,
     * SPLIT input = 0 and outputs = 1/2.
     *
     * @param fromNodeId UUID string of source node
     * @param fromSocketIndex Global index of an output socket on the source node
     * @param toNodeId UUID string of destination node
     * @param toSocketIndex Global index of an input socket on the destination node
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
    Q_INVOKABLE bool isBatchMode() const;

    // ========== Undo / Redo ==========

    /**
     * Attach the window's undo stack so facade mutations (script or UI) become
     * undoable. A beginBatch()/endBatch() group collapses into one undo step.
     * Without a stack the facade mutates directly (headless tests).
     */
    void setUndoStack(QUndoStack* stack);

    /**
     * Undo/redo the last mutation; refused while a node script runs or a
     * batch is open (macro integrity).
     * @return true if an undo/redo step was performed
     */
    Q_INVOKABLE bool undo();
    Q_INVOKABLE bool redo();
    Q_INVOKABLE bool canUndo() const;
    Q_INVOKABLE bool canRedo() const;

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

    // ========== Script Engine ==========

    /**
     * Evaluate script code (errors are reported via errorOccurred signal)
     * @param script Script code to execute
     * @return Result of evaluation as QVariant (invalid on error)
     */
    Q_INVOKABLE QVariant evalScript(const QString& script);

    /**
     * Evaluate script file
     * @param filePath Path to script file
     * @return Result of evaluation as QVariant (invalid on error)
     */
    Q_INVOKABLE QVariant evalFile(const QString& filePath);

    /**
     * Get the script engine handle (value semantics - cheap to copy)
     * @return Type-erased ScriptEngine
     */
    [[nodiscard]] ScriptEngine scriptEngine() const { return m_scriptEngine; }

    /**
     * JavaScript console.log implementation
     * @param message Message to log
     */
    Q_INVOKABLE void jsLog(const QString& message);

    Q_INVOKABLE void quit();

    /// Quit with an explicit process exit code. JS test suites use this to
    /// fail the process (and therefore CTest) when checks fail.
    Q_INVOKABLE void quit(int exitCode);

    /**
     * Let the event loop run for the given duration, so long-running scripts
     * (soak/burn-in runs) stay responsive and timers like autosave, animations
     * and the watchdog can fire. Call between batches of work:
     *   while (...) { ...work...; graph.yield(100); }
     */
    Q_INVOKABLE void yield(int milliseconds);

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
    ScriptEngine m_scriptEngine; // Type-erased script engine (shared handle)
    QUndoStack* m_undoStack = nullptr; // Non-owning; nullptr = direct mutation

    // Batch bookkeeping: the outermost beginBatch() starts a QUndoStack macro
    // lazily (on the first command push) and endBatch() closes it, so a script
    // batch undoes as one step and empty batches leave no entry.
    int m_ownBatchDepth = 0;
    bool m_macroActive = false;

    /// Push a command when a stack is attached; false otherwise. Starts the
    /// batch macro lazily while a Graph batch is open.
    bool pushCommand(QUndoCommand* command);

    // Internal helpers
    Node* findNode(const QString& uuidStr) const;
    Edge* findEdge(const QString& uuidStr) const;
    QUuid parseUuid(const QString& uuidStr) const;

    // Initialize script engine and expose Graph API
    void initializeScripting();
};
