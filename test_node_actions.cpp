#include "node.h"
#include "rubber_action.h"
#include "action_registry.h"
#include <QDebug>
#include <QApplication>
#include <QHash>
#include <QString>

/**
 * MockContext - Simple Context implementation for testing
 * 
 * Provides in-memory storage for inputs/outputs and error handling
 * without requiring complex node infrastructure.
 */
class MockContext : public Context {
private:
    QHash<QString, QVariant> m_inputs;
    QHash<QString, QVariant> m_outputs;
    QString m_error;
    QString m_nodeId;
    QString m_nodeType;

public:
    MockContext(const QString& nodeId = "test-node", const QString& nodeType = "TEST")
        : m_nodeId(nodeId), m_nodeType(nodeType) {}
    
    // Input/Output access
    QVariant getInput(const QString& name) const override {
        return m_inputs.value(name);
    }
    
    void setOutput(const QString& name, const QVariant& value) override {
        m_outputs[name] = value;
    }
    
    bool hasInput(const QString& name) const override {
        return m_inputs.contains(name);
    }
    
    bool hasOutput(const QString& name) const override {
        return m_outputs.contains(name);
    }
    
    // Execution state
    void setError(const QString& message) override {
        m_error = message;
    }
    
    QString getError() const override {
        return m_error;
    }
    
    bool hasError() const override {
        return !m_error.isEmpty();
    }
    
    void clearError() override {
        m_error.clear();
    }
    
    // Metadata access
    QString getNodeId() const override {
        return m_nodeId;
    }
    
    QString getNodeType() const override {
        return m_nodeType;
    }
    
    // Test helpers
    void setInput(const QString& name, const QVariant& value) {
        m_inputs[name] = value;
    }
    
    QVariant getOutput(const QString& name) const {
        return m_outputs.value(name);
    }
    
    QStringList getOutputNames() const {
        return m_outputs.keys();
    }
};

/**
 * Test 1: Basic Action Creation and Execution
 */
void testBasicActionExecution() {
    qDebug() << "\n=== Test 1: Basic Action Execution ===";
    
    // Create a simple uppercase action
    auto uppercaseAction = makeAction([](Node& node, Context& ctx) {
        if (ctx.hasInput("text")) {
            QString input = ctx.getInput("text").toString();
            ctx.setOutput("result", input.toUpper());
        } else {
            ctx.setError("Missing 'text' input");
        }
    }, "Uppercase converter", "PROC");
    
    // Create a node
    Node testNode;
    testNode.setNodeType("PROC");
    
    // Add action to node
    testNode.addAction("uppercase", uppercaseAction);
    
    // Create test context with input
    MockContext ctx("test-node-1", "PROC");
    ctx.setInput("text", "hello world");
    
    // Execute action
    testNode.executeAction("uppercase", ctx);
    
    // Check results
    if (ctx.hasError()) {
        qCritical() << "Test 1 FAILED:" << ctx.getError();
    } else {
        QString result = ctx.getOutput("result").toString();
        if (result == "HELLO WORLD") {
            qDebug() << "Test 1 PASSED: uppercase action worked correctly";
        } else {
            qCritical() << "Test 1 FAILED: expected 'HELLO WORLD', got" << result;
        }
    }
}

/**
 * Test 2: Action Registry Integration
 */
void testActionRegistry() {
    qDebug() << "\n=== Test 2: Action Registry Integration ===";
    
    // Register an action globally
    auto mathAction = makeAction([](Node& node, Context& ctx) {
        if (ctx.hasInput("a") && ctx.hasInput("b")) {
            double a = ctx.getInput("a").toDouble();
            double b = ctx.getInput("b").toDouble();
            ctx.setOutput("sum", a + b);
            ctx.setOutput("product", a * b);
        } else {
            ctx.setError("Missing inputs 'a' or 'b'");
        }
    }, "Basic math operations", "PROC");
    
    // Register with ActionRegistry
    bool registered = ActionRegistry::instance().registerAction("PROC", "math", mathAction);
    if (!registered) {
        qCritical() << "Test 2 FAILED: Could not register action";
        return;
    }
    
    // Create node and get action from registry
    Node testNode;
    testNode.setNodeType("PROC");
    
    ActionPtr retrievedAction = ActionRegistry::instance().getAction("PROC", "math");
    if (!retrievedAction) {
        qCritical() << "Test 2 FAILED: Could not retrieve action from registry";
        return;
    }
    
    testNode.addAction("math", retrievedAction);
    
    // Test execution
    MockContext ctx("test-node-2", "PROC");
    ctx.setInput("a", 15.0);
    ctx.setInput("b", 25.0);
    
    testNode.executeAction("math", ctx);
    
    if (ctx.hasError()) {
        qCritical() << "Test 2 FAILED:" << ctx.getError();
    } else {
        double sum = ctx.getOutput("sum").toDouble();
        double product = ctx.getOutput("product").toDouble();
        
        if (sum == 40.0 && product == 375.0) {
            qDebug() << "Test 2 PASSED: math action and registry integration work correctly";
        } else {
            qCritical() << "Test 2 FAILED: expected sum=40, product=375, got sum=" 
                       << sum << "product=" << product;
        }
    }
}

/**
 * Test 3: Action Type Safety
 */
void testActionTypeSafety() {
    qDebug() << "\n=== Test 3: Action Type Safety ===";
    
    // Create an action that only applies to SINK nodes
    auto sinkAction = makeAction([](Node& node, Context& ctx) {
        ctx.setOutput("logged", true);
    }, "Sink logging action", "SINK");
    
    // Try to add it to a PROC node (should fail)
    Node procNode;
    procNode.setNodeType("PROC");
    
    procNode.addAction("sink_log", sinkAction);
    
    // Check that action wasn't added
    if (procNode.hasAction("sink_log")) {
        qCritical() << "Test 3 FAILED: Action was added to incompatible node type";
    } else {
        qDebug() << "Test 3 PASSED: Type safety prevents incompatible action registration";
    }
    
    // Now try with correct node type
    Node sinkNode;
    sinkNode.setNodeType("SINK");
    
    sinkNode.addAction("sink_log", sinkAction);
    
    if (sinkNode.hasAction("sink_log")) {
        qDebug() << "Test 3 PASSED: Action correctly added to compatible node type";
    } else {
        qCritical() << "Test 3 FAILED: Action not added to compatible node type";
    }
}

/**
 * Test 4: Error Handling
 */
void testErrorHandling() {
    qDebug() << "\n=== Test 4: Error Handling ===";
    
    // Create an action that throws an error
    auto errorAction = makeAction([](Node& node, Context& ctx) {
        ctx.setError("Simulated action error for testing");
    }, "Error testing action", "TEST");
    
    Node testNode;
    testNode.setNodeType("TEST");
    testNode.addAction("error_test", errorAction);
    
    MockContext ctx("test-node-4", "TEST");
    testNode.executeAction("error_test", ctx);
    
    if (ctx.hasError() && ctx.getError() == "Simulated action error for testing") {
        qDebug() << "Test 4 PASSED: Error handling works correctly";
    } else {
        qCritical() << "Test 4 FAILED: Error not properly handled";
    }
    
    // Test executing non-existent action
    ctx.clearError();
    testNode.executeAction("non_existent", ctx);
    
    if (ctx.hasError() && ctx.getError().contains("not found")) {
        qDebug() << "Test 4 PASSED: Non-existent action error handling works";
    } else {
        qCritical() << "Test 4 FAILED: Non-existent action error not handled";
    }
}

/**
 * Main test runner
 */
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);  // Required for Qt initialization
    
    qDebug() << "Starting Node Action System Tests";
    qDebug() << "==================================";
    
    testBasicActionExecution();
    testActionRegistry();
    testActionTypeSafety();  
    testErrorHandling();
    
    // Display registry statistics
    qDebug() << "\n=== Registry Statistics ===";
    auto stats = ActionRegistry::instance().getStats();
    qDebug() << "Total actions:" << stats.totalActions;
    qDebug() << "Node types:" << stats.nodeTypes;
    qDebug() << "Global actions:" << stats.globalActions;
    
    qDebug() << "\n=== Registry Dump ===";
    qDebug() << ActionRegistry::instance().dumpRegistry();
    
    qDebug() << "\nNode Action System Tests Complete";
    
    return 0;  // Don't run Qt event loop for tests
}