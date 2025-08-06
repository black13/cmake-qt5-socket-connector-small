# Plan 11 - Runtime Extensibility via Rubber Types and Scripting

## 🎯 Goal

Add a minimal and robust runtime behavior system that:

- Does not require subclassing Node, Edge, or Socket
- Uses type-erased actions (rubber types) 
- Allows engineers to define runtime logic in JavaScript via QJSEngine
- Does not require modifying existing serialization, UI, or layout systems

## 📁 Files Overview

| File | Purpose | Status |
|------|---------|--------|
| `rubber_action.h/.cpp` | Type-erased action system | ✅ Implemented |
| `action_registry.h/.cpp` | Central storage for runtime actions | ✅ Implemented |
| `scripts/plan11_proc.js` | Example JavaScript actions | ✅ Implemented |
| `node.h/.cpp` | Add action storage (minimal changes) | 🟡 Integration needed |
| `javascript_engine.cpp` | Add registerNodeAction(...) | 🟡 Integration needed |

## 🏗️ Architecture Overview

### Core Concepts

1. **RubberAction**: Type-erased interface for node behaviors
2. **ActionRegistry**: Thread-safe singleton for action storage and retrieval  
3. **Context**: Runtime execution environment providing input/output access
4. **LambdaRubberAction**: Wraps C++ callables as actions
5. **JavaScriptRubberAction**: Wraps JavaScript functions as actions

### Design Principles

- **No Class Explosion**: Single rubber interface, not inheritance hierarchy
- **Non-Invasive**: Existing Node/Edge/Socket classes unchanged (only minimal additions)
- **Runtime Extensible**: Actions registered dynamically via scripts
- **Type Safe**: Strong typing through templates and interfaces
- **Thread Safe**: Registry supports concurrent access
- **Testable**: Actions can be tested in isolation

## 🔧 Core Implementation

### 1. RubberAction Interface

```cpp
class RubberAction {
public:
    virtual ~RubberAction() = default;
    virtual void run(Node& node, Context& ctx) = 0;
    virtual QString getDescription() const { return "RubberAction"; }
    virtual bool isApplicableTo(const QString& nodeType) const { return true; }
};
```

**Key Features:**
- Pure virtual interface for maximum flexibility
- Context-based execution (no direct node modification)
- Type applicability checking
- Self-describing for debugging/UI

### 2. Lambda Integration

```cpp
template <typename Callable>
class LambdaRubberAction : public RubberAction {
    Callable m_function;
public:
    explicit LambdaRubberAction(Callable callable) : m_function(std::move(callable)) {}
    void run(Node& node, Context& ctx) override { m_function(node, ctx); }
};
```

**Usage Example:**
```cpp
auto action = makeAction([](Node& node, Context& ctx) {
    QString input = ctx.getInput("text").toString();
    ctx.setOutput("result", input.toUpper());
}, "Uppercase converter", "PROC");

ActionRegistry::instance().registerAction("PROC", "uppercase", action);
```

### 3. ActionRegistry (Thread-Safe Singleton)

```cpp
class ActionRegistry {
public:
    static ActionRegistry& instance();
    bool registerAction(const QString& nodeType, const QString& actionName, ActionPtr action);
    ActionPtr getAction(const QString& nodeType, const QString& actionName) const;
    QHash<QString, ActionPtr> getActionsForType(const QString& nodeType) const;
    // ... more methods
};
```

**Features:**
- Thread-safe singleton pattern
- Type-specific and global ("*") action storage
- Action discovery and enumeration
- RAII helper for temporary registrations
- Comprehensive statistics and debugging

### 4. Context Interface

```cpp
class Context {
public:
    virtual QVariant getInput(const QString& name) const = 0;
    virtual void setOutput(const QString& name, const QVariant& value) = 0;
    virtual void setError(const QString& message) = 0;
    virtual QString getNodeId() const = 0;
    // ... more methods
};
```

**Purpose:**
- Provides controlled access to node inputs/outputs
- Maintains error state for execution feedback
- Abstracts node implementation details
- Enables testing without real nodes

## 📜 JavaScript Integration

### Action Registration API

```javascript
// Register a JavaScript function as a node action
registerNodeAction("PROC", "uppercase_payload", function(node, ctx) {
    let input = ctx.getInput("text");
    ctx.setOutput("result", input.toUpperCase());
});
```

### Example Actions (plan11_proc.js)

1. **uppercase_payload**: Converts input text to uppercase
2. **concat_inputs**: Concatenates multiple inputs with separator
3. **hash_input**: Generates hash of input data for integrity
4. **json_transform**: Transforms JSON with various operations

### JavaScript Integration Points

```cpp
// In JavaScriptEngine.cpp - expose registration function
engine.globalObject().setProperty("registerNodeAction", 
    engine.newFunction([](QString nodeType, QString actionName, QJSValue jsFunction) {
        auto action = makeJavaScriptAction(jsEngine, jsFunction, actionName, nodeType);
        return ActionRegistry::instance().registerAction(nodeType, actionName, action);
    })
);
```

## 🔌 Node Integration (Minimal Changes)

### Add to Node Class

```cpp
// In node.h
#include "rubber_action.h"
class Node {
    // ... existing members
    std::map<QString, ActionPtr> m_actions;  // NEW: Action storage
    
public:
    // NEW: Action management
    void addAction(const QString& name, ActionPtr action);
    void executeAction(const QString& name, Context& ctx);
    bool hasAction(const QString& name) const;
    QStringList getActionNames() const;
};
```

### Node Implementation

```cpp
// In node.cpp
void Node::addAction(const QString& name, ActionPtr action) {
    if (action && action->isApplicableTo(getNodeType())) {
        m_actions[name] = std::move(action);
    }
}

void Node::executeAction(const QString& name, Context& ctx) {
    auto it = m_actions.find(name);
    if (it != m_actions.end()) {
        it->second->run(*this, ctx);
    }
}
```

## 🧪 Usage Examples

### 1. C++ Lambda Action

```cpp
// Register a C++ action
REGISTER_LAMBDA_ACTION("PROC", "multiply_by_two", [](Node& node, Context& ctx) {
    if (ctx.hasInput("value")) {
        double input = ctx.getInput("value").toDouble();
        ctx.setOutput("result", input * 2.0);
    } else {
        ctx.setError("multiply_by_two requires 'value' input");
    }
});
```

### 2. JavaScript Action (from script file)

```javascript
// In scripts/my_actions.js
registerNodeAction("SINK", "log_input", function(node, ctx) {
    let value = ctx.getInput("input_0");
    console.log("SINK node " + ctx.getNodeId() + " received: " + value);
    ctx.setOutput("logged", true);
});
```

### 3. Orchestrator Integration

```cpp
// In ExecutionOrchestrator
void executeNodeWithActions(const QUuid& nodeId, const QVariantMap& inputs) {
    Node* node = m_scene->getNode(nodeId);
    if (!node) return;
    
    // Create execution context
    NodeExecutionContext ctx(node, inputs);
    
    // Execute registered actions
    QStringList actionNames = ActionRegistry::instance().getActionNames(node->getNodeType());
    for (const QString& actionName : actionNames) {
        ActionPtr action = ActionRegistry::instance().getAction(node->getNodeType(), actionName);
        if (action) {
            node->addAction(actionName, action);
            node->executeAction(actionName, ctx);
            
            if (ctx.hasError()) {
                qWarning() << "Action" << actionName << "failed:" << ctx.getError();
                break;
            }
        }
    }
}
```

### 4. Testing Actions

```cpp
// Unit test for actions
TEST_F(ActionTest, testUppercaseAction) {
    auto action = makeAction([](Node& node, Context& ctx) {
        QString input = ctx.getInput("text").toString();
        ctx.setOutput("result", input.toUpper());
    });
    
    MockContext ctx;
    ctx.setInput("text", "hello world");
    
    MockNode node;
    action->run(node, ctx);
    
    EXPECT_EQ(ctx.getOutput("result").toString(), "HELLO WORLD");
}
```

## 🎛️ Runtime Control

### Action Discovery

```cpp
// Get all actions for a node type
QHash<QString, ActionPtr> actions = ActionRegistry::instance().getActionsForType("PROC");

// Check if specific action exists
bool hasAction = ActionRegistry::instance().hasAction("PROC", "uppercase_payload");

// Get action metadata
ActionPtr action = ActionRegistry::instance().getAction("PROC", "hash_input");
if (action) {
    qDebug() << "Action description:" << action->getDescription();
}
```

### Dynamic Registration

```cpp
// Register action at runtime
ActionRegistry::instance().registerAction("CUSTOM", "my_action", 
    makeAction([](Node& node, Context& ctx) {
        // Custom logic here
    }, "My custom action", "CUSTOM")
);

// Temporary registration with RAII cleanup
{
    ActionRegistrationHelper helper("TEST", "temp_action", tempAction);
    // Action is available within this scope
    // Automatically unregistered when helper destructs
}
```

## 📊 Debugging and Monitoring

### Registry Statistics

```cpp
auto stats = ActionRegistry::instance().getStats();
qDebug() << "Total actions:" << stats.totalActions;
qDebug() << "Node types:" << stats.nodeTypes;
qDebug() << "Global actions:" << stats.globalActions;
```

### Registry Dump

```cpp
QString dump = ActionRegistry::instance().dumpRegistry();
qDebug() << dump;
// Output:
// === Action Registry Dump ===
// Total actions: 8, Node types: 3, Global actions: 2
// 
// Node Type: * (2 actions)
//   - debug_log: Global debug logging action
//   - validate_inputs: Global input validation
//
// Node Type: PROC (4 actions)  
//   - uppercase_payload: Converts input text to uppercase
//   - concat_inputs: Concatenates multiple text inputs
//   ...
```

## 🔄 Integration with Existing Systems

### ExecutionOrchestrator Integration

```cpp
// Modified ExecutionOrchestrator to support actions
class ExecutionOrchestrator : public QObject, public GraphObserver {
    // ... existing code ...
    
    QVariantMap executeNodeWithActions(const QUuid& nodeId, const QVariantMap& inputs) {
        // 1. Execute normal node logic (existing)
        QVariantMap outputs = executeNodeInternal(nodeId, inputs);
        
        // 2. Execute registered actions (NEW)
        Node* node = m_scene->getNode(nodeId);
        if (node) {
            NodeExecutionContext ctx(node, inputs, outputs);
            
            QStringList actions = ActionRegistry::instance().getActionNames(node->getNodeType());
            for (const QString& actionName : actions) {
                ActionPtr action = ActionRegistry::instance().getAction(node->getNodeType(), actionName);
                if (action) {
                    action->run(*node, ctx);
                    if (ctx.hasError()) {
                        qWarning() << "Action" << actionName << "failed:" << ctx.getError();
                        break;
                    }
                }
            }
            
            // Merge action outputs
            outputs = ctx.getAllOutputs();
        }
        
        return outputs;
    }
};
```

### JavaScript Engine Integration

```cpp
// Enhanced JavaScriptEngine with action support
class JavaScriptEngine : public QObject {
    // ... existing code ...
    
    void setupActionAPI() {
        // Expose action registration to JavaScript
        m_engine->globalObject().setProperty("registerNodeAction",
            m_engine->newFunction([this](QString nodeType, QString actionName, QJSValue jsFunc) {
                auto action = std::make_shared<JavaScriptRubberAction>(this, jsFunc, actionName, nodeType);
                return ActionRegistry::instance().registerAction(nodeType, actionName, action);
            })
        );
        
        // Expose action query functions
        m_engine->globalObject().setProperty("getAvailableActions", 
            m_engine->newFunction([](QString nodeType) {
                QStringList actions = ActionRegistry::instance().getActionNames(nodeType);
                return actions;
            })
        );
    }
    
    void loadActionScripts(const QString& scriptDir) {
        QDir dir(scriptDir);
        QStringList scripts = dir.entryList(QStringList() << "*.js", QDir::Files);
        
        for (const QString& script : scripts) {
            QString fullPath = dir.absoluteFilePath(script);
            qDebug() << "Loading action script:" << fullPath;
            evaluateFile(fullPath);
        }
    }
};
```

## 🚀 Future Enhancements

### Priority Enhancements

| Feature | Priority | Easy to Add? | Description |
|---------|----------|-------------|-------------|
| Context with data flow | 🟢 High | ✅ Yes | Enhanced context with flow control |
| JS validation on connect | 🟢 High | ✅ Yes | Pre-connection validation actions |
| Auto-bind node properties | 🟡 Med | ✅ Yes | Automatic property exposure to JS |
| Action dependency system | 🟡 Med | 🟡 Workable | Actions can depend on other actions |
| Plugin system for actions | 🔴 Later | ❌ Complex | External plugin loading |

### Context Enhancements

```cpp
class EnhancedContext : public Context {
public:
    // Data flow control
    virtual void skipDownstream() = 0;
    virtual void triggerRecompute(const QString& nodeId) = 0;
    virtual void setFlowPriority(int priority) = 0;
    
    // Action chaining
    virtual void scheduleAction(const QString& actionName) = 0;
    virtual QStringList getScheduledActions() const = 0;
    
    // Metadata access
    virtual QVariantMap getNodeMetadata() const = 0;
    virtual QStringList getConnectedNodeIds() const = 0;
};
```

### Configuration-Based Registration

```json
{
  "actions": [
    {
      "nodeType": "PROC",
      "name": "validate_email",
      "script": "scripts/validators.js",
      "function": "validateEmailInput",
      "description": "Validates email address format",
      "enabled": true
    }
  ]
}
```

## 📋 Implementation Checklist

### Phase 1: Core System ✅
- [x] RubberAction interface and implementations
- [x] ActionRegistry singleton with thread safety  
- [x] Context interface for execution environment
- [x] LambdaRubberAction for C++ callables
- [x] JavaScriptRubberAction for JS functions
- [x] Helper functions and macros
- [x] Comprehensive documentation

### Phase 2: Integration 🟡
- [ ] Add action storage to Node class
- [ ] Implement Node::addAction/executeAction methods
- [ ] Create NodeExecutionContext implementation
- [ ] Integrate with JavaScriptEngine
- [ ] Add action loading to startup sequence
- [ ] Update ExecutionOrchestrator for actions

### Phase 3: Testing ⏳
- [ ] Unit tests for ActionRegistry
- [ ] Unit tests for RubberAction implementations  
- [ ] Integration tests with Node class
- [ ] JavaScript action execution tests
- [ ] Performance benchmarks
- [ ] Thread safety tests

### Phase 4: Documentation ⏳
- [ ] API documentation for engineers
- [ ] JavaScript action writing guide
- [ ] Integration examples and best practices
- [ ] Migration guide for existing code

## 🧩 Summary

Plan 11 provides a **minimal, non-invasive runtime extensibility system** that:

✅ **Preserves Architecture**: No changes to existing serialization, UI, or layout  
✅ **Avoids Class Explosion**: Single rubber interface, not inheritance hierarchy  
✅ **Enables Runtime Logic**: JavaScript-driven behaviors without recompilation  
✅ **Maintains Performance**: Type-erased design with minimal overhead  
✅ **Supports Testing**: Actions testable in isolation with mock contexts  
✅ **Thread Safe**: Registry supports concurrent access from multiple threads  

The system is **engineer-friendly** with clear APIs, comprehensive documentation, and practical examples that demonstrate real-world usage patterns.

**Next Steps:**
1. Integrate Node class changes (minimal additions)
2. Connect JavaScriptEngine with action registration API  
3. Load action scripts during application startup
4. Add action execution to ExecutionOrchestrator workflow

The rubber types pattern ensures **maximum flexibility** while maintaining **strong typing** and **excellent performance** - exactly what's needed for a professional node graph system.