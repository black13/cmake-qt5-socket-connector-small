#pragma once

#include <memory>
#include <functional>
#include <QString>
#include <QVariant>

// Forward declarations
class Node;
class Context;

/**
 * Context - Runtime execution context for rubber actions
 * 
 * Provides access to node inputs/outputs and execution state
 * without tightly coupling to specific node implementations.
 */
class Context {
public:
    Context() = default;
    virtual ~Context() = default;
    
    // Input/Output access
    virtual QVariant getInput(const QString& name) const = 0;
    virtual void setOutput(const QString& name, const QVariant& value) = 0;
    virtual bool hasInput(const QString& name) const = 0;
    virtual bool hasOutput(const QString& name) const = 0;
    
    // Execution state
    virtual void setError(const QString& message) = 0;
    virtual QString getError() const = 0;
    virtual bool hasError() const = 0;
    virtual void clearError() = 0;
    
    // Metadata access
    virtual QString getNodeId() const = 0;
    virtual QString getNodeType() const = 0;
};

/**
 * RubberAction - Type-erased behavior interface
 * 
 * Enables runtime-extensible node behaviors without class inheritance.
 * Actions can be implemented in C++ lambdas or JavaScript functions.
 * 
 * Design principles:
 * - No virtual inheritance required for Node classes
 * - Actions are composable and testable in isolation
 * - Runtime registration allows script-driven behavior
 */
class RubberAction {
public:
    virtual ~RubberAction() = default;
    
    /**
     * Execute the action on a node with given context
     * @param node The node to operate on
     * @param ctx Execution context with input/output access
     */
    virtual void run(Node& node, Context& ctx) = 0;
    
    /**
     * Get a description of what this action does
     * @return Human-readable action description
     */
    virtual QString getDescription() const { return "RubberAction"; }
    
    /**
     * Check if this action is applicable to the given node type
     * @param nodeType The node type to check
     * @return true if action can run on this node type
     */
    virtual bool isApplicableTo(const QString& nodeType) const { return true; }
};

/**
 * LambdaRubberAction - Concrete implementation for C++ callables
 * 
 * Wraps any callable (lambda, function pointer, std::function) as a RubberAction.
 * Enables easy registration of C++ logic without boilerplate classes.
 * 
 * Example:
 *   auto action = std::make_shared<LambdaRubberAction>([](Node& node, Context& ctx) {
 *       QString input = ctx.getInput("text").toString();
 *       ctx.setOutput("result", input.toUpper());
 *   });
 */
template <typename Callable>
class LambdaRubberAction : public RubberAction {
private:
    Callable m_function;
    QString m_description;
    QString m_nodeType;

public:
    explicit LambdaRubberAction(Callable callable, 
                               const QString& description = "Lambda Action",
                               const QString& nodeType = "*")
        : m_function(std::move(callable))
        , m_description(description)
        , m_nodeType(nodeType)
    {}
    
    void run(Node& node, Context& ctx) override {
        m_function(node, ctx);
    }
    
    QString getDescription() const override {
        return m_description;
    }
    
    bool isApplicableTo(const QString& nodeType) const override {
        return m_nodeType == "*" || m_nodeType == nodeType;
    }
};

/**
 * JavaScriptRubberAction - JavaScript function wrapper
 * 
 * Wraps a JavaScript function as a RubberAction for script-driven behaviors.
 * Enables runtime extensibility through .js files without recompilation.
 */
class JavaScriptRubberAction : public RubberAction {
private:
    class JavaScriptEngine* m_jsEngine;
    QString m_functionName;
    QString m_description;
    QString m_nodeType;

public:
    JavaScriptRubberAction(class JavaScriptEngine* jsEngine,
                          const QString& functionName,
                          const QString& description = "JavaScript Action",
                          const QString& nodeType = "*");
    
    void run(Node& node, Context& ctx) override;
    QString getDescription() const override { return m_description; }
    bool isApplicableTo(const QString& nodeType) const override;
};

// Type aliases for convenience
using ActionPtr = std::shared_ptr<RubberAction>;
using ActionFactory = std::function<ActionPtr()>;

/**
 * Helper functions for creating actions
 */
template <typename Callable>
ActionPtr makeAction(Callable&& callable, 
                    const QString& description = "Lambda Action",
                    const QString& nodeType = "*") {
    return std::make_shared<LambdaRubberAction<Callable>>(
        std::forward<Callable>(callable), description, nodeType);
}

ActionPtr makeJavaScriptAction(class JavaScriptEngine* jsEngine,
                              const QString& functionName,
                              const QString& description = "JavaScript Action",
                              const QString& nodeType = "*");