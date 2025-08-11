#pragma once

#include "rubber_action.h"
#include <QString>
#include <QHash>
#include <QStringList>
#include <QMutex>
#include <memory>

/**
 * ActionRegistry - Central storage and management for runtime actions
 * 
 * Provides thread-safe registration and retrieval of RubberActions.
 * Supports both global actions (apply to all nodes) and type-specific actions.
 * 
 * Key features:
 * - Thread-safe singleton pattern
 * - Type-specific action storage
 * - Action discovery and enumeration
 * - Runtime registration from C++ and JavaScript
 * - Action lifecycle management
 * 
 * Usage patterns:
 * 1. Register actions during startup or script loading
 * 2. Query available actions for UI or orchestration
 * 3. Retrieve actions for execution on specific nodes
 */
class ActionRegistry {
public:
    /**
     * Get the singleton instance (thread-safe)
     * @return Reference to the global action registry
     */
    static ActionRegistry& instance();
    
    /**
     * Register an action for a specific node type
     * @param nodeType Node type this action applies to ("*" for all types)
     * @param actionName Unique name for this action
     * @param action The action implementation
     * @param overwrite Whether to overwrite existing actions with same name
     * @return true if registration succeeded
     */
    bool registerAction(const QString& nodeType, 
                       const QString& actionName, 
                       ActionPtr action,
                       bool overwrite = false);
    
    /**
     * Get a specific action by node type and name
     * @param nodeType Node type to look for
     * @param actionName Action name to retrieve
     * @return Action pointer, or nullptr if not found
     */
    ActionPtr getAction(const QString& nodeType, const QString& actionName) const;
    
    /**
     * Get all actions applicable to a node type
     * @param nodeType Node type to query (includes global "*" actions)
     * @return Hash of action name -> action pointer
     */
    QHash<QString, ActionPtr> getActionsForType(const QString& nodeType) const;
    
    /**
     * Get all registered node types that have actions
     * @return List of node types with registered actions
     */
    QStringList getRegisteredNodeTypes() const;
    
    /**
     * Get all action names for a specific node type
     * @param nodeType Node type to query
     * @return List of action names
     */
    QStringList getActionNames(const QString& nodeType) const;
    
    /**
     * Check if an action exists
     * @param nodeType Node type to check
     * @param actionName Action name to check
     * @return true if action is registered
     */
    bool hasAction(const QString& nodeType, const QString& actionName) const;
    
    /**
     * Unregister an action
     * @param nodeType Node type
     * @param actionName Action name to remove
     * @return true if action was removed
     */
    bool unregisterAction(const QString& nodeType, const QString& actionName);
    
    /**
     * Clear all actions for a node type
     * @param nodeType Node type to clear ("*" clears global actions)
     * @return Number of actions removed
     */
    int clearActionsForType(const QString& nodeType);
    
    /**
     * Clear all registered actions
     */
    void clearAll();
    
    /**
     * Get registry statistics
     */
    struct RegistryStats {
        int totalActions = 0;
        int nodeTypes = 0;
        int globalActions = 0;
    };
    RegistryStats getStats() const;
    
    /**
     * Bulk registration from a configuration
     * @param config Action configuration (format TBD)
     * @return Number of actions successfully registered
     */
    int registerFromConfig(const QString& config);
    
    /**
     * Export registry state for debugging
     * @return Human-readable registry dump
     */
    QString dumpRegistry() const;

private:
    // Private constructor for singleton
    ActionRegistry() = default;
    ~ActionRegistry() = default;
    
    // Prevent copying
    ActionRegistry(const ActionRegistry&) = delete;
    ActionRegistry& operator=(const ActionRegistry&) = delete;
    
    // Thread safety
    mutable QMutex m_mutex;
    
    // Storage: nodeType -> (actionName -> action)
    QHash<QString, QHash<QString, ActionPtr>> m_registry;
    
    // Helper methods
    QStringList getAllApplicableTypes(const QString& nodeType) const;
    void validateRegistration(const QString& nodeType, const QString& actionName) const;
};

/**
 * ActionRegistrationHelper - RAII helper for action registration
 * 
 * Automatically unregisters actions when the helper goes out of scope.
 * Useful for temporary or test actions.
 */
class ActionRegistrationHelper {
public:
    ActionRegistrationHelper(const QString& nodeType, const QString& actionName, ActionPtr action);
    ~ActionRegistrationHelper();
    
    // Non-copyable, movable
    ActionRegistrationHelper(const ActionRegistrationHelper&) = delete;
    ActionRegistrationHelper& operator=(const ActionRegistrationHelper&) = delete;
    ActionRegistrationHelper(ActionRegistrationHelper&& other) noexcept;
    ActionRegistrationHelper& operator=(ActionRegistrationHelper&& other) noexcept;
    
    bool isValid() const { return !m_nodeType.isEmpty(); }

private:
    QString m_nodeType;
    QString m_actionName;
};

/**
 * Convenience macros for action registration
 */
#define REGISTER_ACTION(nodeType, name, action) \
    ActionRegistry::instance().registerAction(nodeType, name, action)

#define REGISTER_LAMBDA_ACTION(nodeType, name, lambda) \
    REGISTER_ACTION(nodeType, name, makeAction(lambda, #name, nodeType))

/**
 * Action registration at static initialization time
 * Use in .cpp files to register actions when the library loads
 */
#define STATIC_ACTION_REGISTRATION(nodeType, name, action) \
    static bool _static_reg_##name = ActionRegistry::instance().registerAction(nodeType, #name, action);