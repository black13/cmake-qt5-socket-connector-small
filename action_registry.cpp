#include "action_registry.h"
#include <QDebug>
#include <QMutexLocker>
#include <QThread>

ActionRegistry& ActionRegistry::instance()
{
    // Thread-safe singleton using C++11 magic statics
    static ActionRegistry instance;
    return instance;
}

bool ActionRegistry::registerAction(const QString& nodeType, 
                                   const QString& actionName, 
                                   ActionPtr action,
                                   bool overwrite)
{
    if (!action) {
        qWarning() << "ActionRegistry: Cannot register null action";
        return false;
    }
    
    if (nodeType.isEmpty() || actionName.isEmpty()) {
        qWarning() << "ActionRegistry: Node type and action name cannot be empty";
        return false;
    }
    
    QMutexLocker locker(&m_mutex);
    
    // Check for existing action
    if (!overwrite && m_registry[nodeType].contains(actionName)) {
        qWarning() << "ActionRegistry: Action" << actionName << "already registered for" << nodeType;
        return false;
    }
    
    // Register the action
    m_registry[nodeType][actionName] = action;
    
    qDebug() << "ActionRegistry: Registered action" << actionName 
             << "for node type" << nodeType
             << "- Description:" << action->getDescription();
    
    return true;
}

ActionPtr ActionRegistry::getAction(const QString& nodeType, const QString& actionName) const
{
    QMutexLocker locker(&m_mutex);
    
    // First try the specific node type
    if (m_registry.contains(nodeType)) {
        const auto& typeActions = m_registry[nodeType];
        if (typeActions.contains(actionName)) {
            return typeActions[actionName];
        }
    }
    
    // Then try global actions ("*")
    if (nodeType != "*" && m_registry.contains("*")) {
        const auto& globalActions = m_registry["*"];
        if (globalActions.contains(actionName)) {
            // Check if the global action is applicable to this node type
            ActionPtr action = globalActions[actionName];
            if (action && action->isApplicableTo(nodeType)) {
                return action;
            }
        }
    }
    
    return nullptr;
}

QHash<QString, ActionPtr> ActionRegistry::getActionsForType(const QString& nodeType) const
{
    QMutexLocker locker(&m_mutex);
    
    QHash<QString, ActionPtr> result;
    
    // Add global actions that are applicable
    if (m_registry.contains("*")) {
        const auto& globalActions = m_registry["*"];
        for (auto it = globalActions.begin(); it != globalActions.end(); ++it) {
            if (it.value() && it.value()->isApplicableTo(nodeType)) {
                result[it.key()] = it.value();
            }
        }
    }
    
    // Add type-specific actions (override globals if same name)
    if (nodeType != "*" && m_registry.contains(nodeType)) {
        const auto& typeActions = m_registry[nodeType];
        for (auto it = typeActions.begin(); it != typeActions.end(); ++it) {
            result[it.key()] = it.value();
        }
    }
    
    return result;
}

QStringList ActionRegistry::getRegisteredNodeTypes() const
{
    QMutexLocker locker(&m_mutex);
    return m_registry.keys();
}

QStringList ActionRegistry::getActionNames(const QString& nodeType) const
{
    QHash<QString, ActionPtr> actions = getActionsForType(nodeType);
    return actions.keys();
}

bool ActionRegistry::hasAction(const QString& nodeType, const QString& actionName) const
{
    return getAction(nodeType, actionName) != nullptr;
}

bool ActionRegistry::unregisterAction(const QString& nodeType, const QString& actionName)
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_registry.contains(nodeType)) {
        return false;
    }
    
    auto& typeActions = m_registry[nodeType];
    bool removed = typeActions.remove(actionName) > 0;
    
    // Clean up empty node type entries
    if (typeActions.isEmpty()) {
        m_registry.remove(nodeType);
    }
    
    if (removed) {
        qDebug() << "ActionRegistry: Unregistered action" << actionName << "from" << nodeType;
    }
    
    return removed;
}

int ActionRegistry::clearActionsForType(const QString& nodeType)
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_registry.contains(nodeType)) {
        return 0;
    }
    
    int count = m_registry[nodeType].size();
    m_registry.remove(nodeType);
    
    qDebug() << "ActionRegistry: Cleared" << count << "actions for node type" << nodeType;
    return count;
}

void ActionRegistry::clearAll()
{
    QMutexLocker locker(&m_mutex);
    int totalActions = 0;
    
    for (const auto& typeActions : m_registry) {
        totalActions += typeActions.size();
    }
    
    m_registry.clear();
    qDebug() << "ActionRegistry: Cleared all" << totalActions << "actions";
}

ActionRegistry::RegistryStats ActionRegistry::getStats() const
{
    QMutexLocker locker(&m_mutex);
    
    RegistryStats stats;
    stats.nodeTypes = m_registry.size();
    
    for (const auto& typeActions : m_registry) {
        stats.totalActions += typeActions.size();
        if (m_registry.key(typeActions) == "*") {
            stats.globalActions = typeActions.size();
        }
    }
    
    return stats;
}

int ActionRegistry::registerFromConfig(const QString& config)
{
    // TODO: Implement configuration-based registration
    // Could parse JSON, XML, or custom format
    Q_UNUSED(config)
    qDebug() << "ActionRegistry: Configuration-based registration not implemented yet";
    return 0;
}

QString ActionRegistry::dumpRegistry() const
{
    QMutexLocker locker(&m_mutex);
    
    QString dump;
    dump += "=== Action Registry Dump ===\n";
    
    // Calculate stats under existing lock to avoid deadlock
    RegistryStats stats;
    stats.nodeTypes = m_registry.size();
    for (const auto& typeActions : m_registry) {
        stats.totalActions += typeActions.size();
        if (m_registry.key(typeActions) == "*") {
            stats.globalActions = typeActions.size();
        }
    }
    
    dump += QString("Total actions: %1, Node types: %2, Global actions: %3\n\n")
                .arg(stats.totalActions)
                .arg(stats.nodeTypes)
                .arg(stats.globalActions);
    
    for (auto typeIt = m_registry.begin(); typeIt != m_registry.end(); ++typeIt) {
        const QString& nodeType = typeIt.key();
        const auto& actions = typeIt.value();
        
        dump += QString("Node Type: %1 (%2 actions)\n").arg(nodeType).arg(actions.size());
        
        for (auto actionIt = actions.begin(); actionIt != actions.end(); ++actionIt) {
            const QString& actionName = actionIt.key();
            const ActionPtr& action = actionIt.value();
            
            dump += QString("  - %1: %2\n")
                        .arg(actionName)
                        .arg(action ? action->getDescription() : "NULL ACTION");
        }
        
        dump += "\n";
    }
    
    return dump;
}

// ActionRegistrationHelper implementation

ActionRegistrationHelper::ActionRegistrationHelper(const QString& nodeType, 
                                                 const QString& actionName, 
                                                 ActionPtr action)
    : m_nodeType(nodeType)
    , m_actionName(actionName)
{
    if (!ActionRegistry::instance().registerAction(nodeType, actionName, action)) {
        m_nodeType.clear(); // Mark as invalid
        m_actionName.clear();
    }
}

ActionRegistrationHelper::~ActionRegistrationHelper()
{
    if (isValid()) {
        ActionRegistry::instance().unregisterAction(m_nodeType, m_actionName);
    }
}

ActionRegistrationHelper::ActionRegistrationHelper(ActionRegistrationHelper&& other) noexcept
    : m_nodeType(std::move(other.m_nodeType))
    , m_actionName(std::move(other.m_actionName))
{
    other.m_nodeType.clear();
    other.m_actionName.clear();
}

ActionRegistrationHelper& ActionRegistrationHelper::operator=(ActionRegistrationHelper&& other) noexcept
{
    if (this != &other) {
        // Clean up current registration
        if (isValid()) {
            ActionRegistry::instance().unregisterAction(m_nodeType, m_actionName);
        }
        
        // Move from other
        m_nodeType = std::move(other.m_nodeType);
        m_actionName = std::move(other.m_actionName);
        
        // Clear other
        other.m_nodeType.clear();
        other.m_actionName.clear();
    }
    
    return *this;
}