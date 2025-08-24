#ifndef EXECUTABLE_SPEC_H
#define EXECUTABLE_SPEC_H

#include <memory>
#include <QVariantMap>
#include <QUuid>
#include <QString>

/**
 * ExecutableSpec - Rubber types capability for node execution
 * 
 * Provides a unified interface for executing nodes with different capabilities:
 * - JavaScript nodes via existing JavaScriptEngine
 * - Future: Python nodes, C++ compiled nodes, etc.
 * 
 * Uses type-erasure pattern to avoid inheritance explosion.
 * Delegates to existing execution systems while adding orchestration layer.
 */
class ExecutableSpec {
public:
    struct Concept {
        virtual ~Concept() = default;
        
        /**
         * Execute the node with given inputs
         * @param nodeId UUID of the node being executed
         * @param inputs Input data as Qt-native QVariantMap
         * @return Output data as Qt-native QVariantMap
         */
        virtual QVariantMap execute(const QUuid& nodeId, const QVariantMap& inputs) = 0;
        
        /**
         * Check if this node can be executed (has script/implementation)
         * @param nodeId UUID of the node to check
         * @return true if node is executable
         */
        virtual bool canExecute(const QUuid& nodeId) const = 0;
        
        /**
         * Get hash of execution context for memoization
         * Includes script content, node configuration, etc.
         * @param nodeId UUID of the node
         * @return Hash string for cache keying
         */
        virtual QString getExecutionHash(const QUuid& nodeId) const = 0;
        
        /**
         * Get execution dependencies (input node IDs)
         * Used for topological ordering and invalidation
         * @param nodeId UUID of the node
         * @return List of upstream node UUIDs this node depends on
         */
        virtual QList<QUuid> getDependencies(const QUuid& nodeId) const = 0;
    };
    
    template<typename T>
    struct Model : public Concept {
        T* obj;
        explicit Model(T* o) : obj(o) {}
        
        QVariantMap execute(const QUuid& nodeId, const QVariantMap& inputs) override {
            return obj->execute(nodeId, inputs);
        }
        
        bool canExecute(const QUuid& nodeId) const override {
            return obj->canExecute(nodeId);
        }
        
        QString getExecutionHash(const QUuid& nodeId) const override {
            return obj->getExecutionHash(nodeId);
        }
        
        QList<QUuid> getDependencies(const QUuid& nodeId) const override {
            return obj->getDependencies(nodeId);
        }
    };
    
private:
    std::unique_ptr<Concept> m_impl;
    
public:
    template<typename T>
    explicit ExecutableSpec(T* obj) : m_impl(std::make_unique<Model<T>>(obj)) {}
    
    // Move-only semantics like other facades
    ExecutableSpec(const ExecutableSpec&) = delete;
    ExecutableSpec& operator=(const ExecutableSpec&) = delete;
    ExecutableSpec(ExecutableSpec&&) = default;
    ExecutableSpec& operator=(ExecutableSpec&&) = default;
    
    // Public interface delegates to implementation
    QVariantMap execute(const QUuid& nodeId, const QVariantMap& inputs) {
        return m_impl->execute(nodeId, inputs);
    }
    
    bool canExecute(const QUuid& nodeId) const {
        return m_impl->canExecute(nodeId);
    }
    
    QString getExecutionHash(const QUuid& nodeId) const {
        return m_impl->getExecutionHash(nodeId);
    }
    
    QList<QUuid> getDependencies(const QUuid& nodeId) const {
        return m_impl->getDependencies(nodeId);
    }
};

#endif // EXECUTABLE_SPEC_H