#pragma once

#include <QUuid>
#include <QString>
#include <QPointF>
#include <QHash>

// Forward declaration to avoid circular includes
class Node;

/**
 * RubberNodeFacade - Minimal wrapper around existing Node*
 * 
 * Week 1 Goal: Prove wrapper concept without touching existing system
 * 
 * Design Principles:
 * - Zero impact on existing Node class
 * - Reference-based wrapper (doesn't own Node*)  
 * - Delegate all operations to existing Node methods
 * - Placeholder for future JavaScript action integration
 * 
 * Safety Features:
 * - No ownership of Node* (existing scene manages lifecycle)
 * - All methods delegate to proven existing code
 * - Optional feature - can be completely disabled
 */
class RubberNodeFacade
{
public:
    /**
     * Create facade wrapper around existing Node
     * @param node Reference to existing node (NOT owned by facade)
     */
    explicit RubberNodeFacade(Node& node);
    
    /**
     * Copy constructor for safe wrapper copying
     */
    RubberNodeFacade(const RubberNodeFacade& other) = default;
    
    /**
     * Assignment operator 
     */
    RubberNodeFacade& operator=(const RubberNodeFacade& other) = default;
    
    /**
     * Destructor - doesn't delete wrapped node (not owned)
     */
    ~RubberNodeFacade() = default;
    
    // === Core Node Properties (delegate to existing Node methods) ===
    
    /**
     * Get node unique identifier
     * @return UUID of wrapped node
     */
    QUuid getId() const;
    
    /**
     * Get node type string
     * @return Type identifier (e.g., "SOURCE", "TRANSFORM")
     */
    QString getType() const;
    
    /**
     * Get node position in scene
     * @return Scene coordinates of node
     */
    QPointF getPosition() const;
    
    /**
     * Set node position in scene
     * @param position New scene coordinates
     */
    void setPosition(const QPointF& position);
    
    // === Action System (Week 2-3 placeholders) ===
    
    /**
     * Register JavaScript action for this node instance
     * @param name Action identifier
     * @param script JavaScript code to execute
     */
    void registerAction(const QString& name, const QString& script);
    
    /**
     * Check if action is registered for this node instance
     * @param name Action identifier
     * @return True if action exists
     */
    bool hasAction(const QString& name) const;
    
    /**
     * Get all registered actions for this node instance
     * @return Map of action names to JavaScript code
     */
    QHash<QString, QString> getActions() const;
    
    /**
     * Remove registered action
     * @param name Action identifier to remove
     */
    void removeAction(const QString& name);
    
    // === Debugging and Validation ===
    
    /**
     * Check if wrapped node pointer is valid
     * @return True if node pointer is not null
     */
    bool isValid() const;
    
    /**
     * Get underlying Node* for integration with existing code
     * @return Pointer to wrapped node (use carefully)
     */
    Node* getNode() const;
    
    /**
     * Create debug string representation
     * @return Human-readable facade state
     */
    QString toString() const;
    
private:
    Node* m_node;  // Reference to existing node (NOT owned)
    
    // Week 2-3: JavaScript action storage (per-instance)
    QHash<QString, QString> m_actions;
    
    // Validate node pointer before operations
    void ensureValidNode() const;
};

/**
 * Equality comparison for facade objects
 * @param lhs Left-hand facade
 * @param rhs Right-hand facade  
 * @return True if facades wrap the same node
 */
bool operator==(const RubberNodeFacade& lhs, const RubberNodeFacade& rhs);

/**
 * Inequality comparison for facade objects
 */
bool operator!=(const RubberNodeFacade& lhs, const RubberNodeFacade& rhs);