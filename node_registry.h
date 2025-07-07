#pragma once

#include <QString>
#include <QMap>
#include <functional>

class Node;

/**
 * NodeRegistry - Factory registry for node types
 * 
 * Allows registering node types by string name and creating instances
 * without the factory knowing about concrete types.
 * 
 * Usage:
 *   NodeRegistry::instance().registerNode("SOURCE", []() { return new SourceNode(); });
 *   Node* node = NodeRegistry::instance().createNode("SOURCE");
 */

using NodeFactoryFunction = std::function<Node*()>;

class NodeRegistry
{
public:
    // Singleton instance
    static NodeRegistry& instance();
    
    // Register a node type with its factory function
    void registerNode(const QString& typeName, NodeFactoryFunction factory);
    
    // Create a node instance by type name
    Node* createNode(const QString& typeName) const;
    
    // Check if a type is registered
    bool isRegistered(const QString& typeName) const;
    
    // Get all registered type names
    QStringList getRegisteredTypes() const;
    
    // Clear all registrations (for testing)
    void clear();

private:
    NodeRegistry() = default;
    NodeRegistry(const NodeRegistry&) = delete;
    NodeRegistry& operator=(const NodeRegistry&) = delete;
    
    QMap<QString, NodeFactoryFunction> m_factories;
};

/**
 * Convenience macro for registering node types
 * Use this in the .cpp file of each node type:
 * 
 * REGISTER_NODE_TYPE("SOURCE", SourceNode)
 */
#define REGISTER_NODE_TYPE(typeName, className) \
    namespace { \
        struct Register##className { \
            Register##className() { \
                NodeRegistry::instance().registerNode(typeName, []() { \
                    return new className(); \
                }); \
            } \
        }; \
        static Register##className register##className##Instance; \
    }
