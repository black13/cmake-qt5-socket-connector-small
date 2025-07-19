#include "javascript_engine.h"
#include "node.h"
#include "edge.h"
#include "scene.h"
#include "graph_controller.h"
#include "graph_factory.h"
#include <QFile>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonObject>

JavaScriptEngine::JavaScriptEngine(QObject* parent)
    : QObject(parent)
    , m_engine(new QJSEngine(this))
    , m_scene(nullptr)
    , m_graphController(nullptr)
{
    setupGlobalAPI();
    registerConsoleAPI();
    registerUtilityAPI();
    
    // Auto-load enhanced APIs disabled for now - will load scripts manually
    // loadEnhancedAPIs();
    
    qDebug() << "JavaScriptEngine: Simple JavaScript engine initialized";
}

JavaScriptEngine::~JavaScriptEngine()
{
    qDebug() << "JavaScriptEngine: Shutting down";
}

QJSValue JavaScriptEngine::evaluate(const QString& script)
{
    clearErrors();
    
    qDebug() << "JS_EXECUTION: Starting script execution";
    qDebug() << "JS_EXECUTION: Script length:" << script.length() << "characters";
    
    QJSValue result = m_engine->evaluate(script);
    
    if (result.isError()) {
        m_lastError = QString("JavaScript Error: %1").arg(result.toString());
        emit scriptError(m_lastError);
        qDebug() << "JS_ERROR: Script execution failed:" << m_lastError;
        qDebug() << "JS_ERROR: Script content:" << script.left(500); // Log first 500 chars
    } else {
        emit scriptExecuted(script, result);
        QString resultStr = result.isUndefined() ? "undefined" : result.toString();
        qDebug() << "JS_EXECUTION: Script completed successfully";
        qDebug() << "JS_EXECUTION: Result:" << resultStr;
    }
    
    return result;
}

QJSValue JavaScriptEngine::evaluateFile(const QString& filePath)
{
    qDebug() << "JS_EXECUTION: Loading script file:" << filePath;
    
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_lastError = QString("Cannot open script file: %1").arg(filePath);
        emit scriptError(m_lastError);
        qDebug() << "JS_ERROR: Failed to open script file:" << filePath;
        return QJSValue();
    }
    
    QTextStream in(&file);
    QString script = in.readAll();
    
    qDebug() << "JS_EXECUTION: Loaded script file:" << filePath;
    qDebug() << "JS_EXECUTION: Script size:" << script.length() << "characters";
    
    return evaluate(script);
}

void JavaScriptEngine::registerNodeAPI(Scene* scene)
{
    m_scene = scene;
    
    // Register Node API
    QJSValue nodeAPI = m_engine->newObject();
    
    // Node creation functions - Qt5 compatible
    QJSValue createFunc = m_engine->evaluate(R"(
        (function(type, x, y) {
            if (arguments.length < 3) {
                throw new Error("Node.create() requires type, x, y parameters");
            }
            console.log("JavaScript: Creating node " + type + " at " + x + "," + y);
            return {}; // Placeholder
        })
    )");
    nodeAPI.setProperty("create", createFunc);
    
    // Node query functions - Qt5 compatible
    QJSValue findByIdFunc = m_engine->evaluate(R"(
        (function(id) {
            if (arguments.length < 1) {
                throw new Error("Node.findById() requires id parameter");
            }
            console.log("JavaScript: Finding node by ID: " + id);
            return null; // Placeholder
        })
    )");
    nodeAPI.setProperty("findById", findByIdFunc);
    
    // Enhanced node manipulation functions
    QJSValue moveNodeFunc = m_engine->evaluate(R"(
        (function(nodeId, x, y) {
            if (arguments.length < 3) {
                throw new Error("Node.move() requires nodeId, x, y parameters");
            }
            console.log("JavaScript: Moving node " + nodeId + " to " + x + "," + y);
            return true; // Placeholder
        })
    )");
    nodeAPI.setProperty("move", moveNodeFunc);
    
    QJSValue getPropertiesFunc = m_engine->evaluate(R"(
        (function(nodeId) {
            if (arguments.length < 1) {
                throw new Error("Node.getProperties() requires nodeId parameter");
            }
            console.log("JavaScript: Getting properties for node " + nodeId);
            return {}; // Placeholder
        })
    )");
    nodeAPI.setProperty("getProperties", getPropertiesFunc);
    
    m_engine->globalObject().setProperty("Node", nodeAPI);
    
    // Register Graph API
    QJSValue graphAPI = m_engine->newObject();
    
    // Graph API functions - Qt5 compatible
    QJSValue getNodesFunc = m_engine->evaluate(R"(
        (function() {
            console.log("JavaScript: Getting all nodes");
            return []; // Placeholder
        })
    )");
    graphAPI.setProperty("getNodes", getNodesFunc);
    
    QJSValue getEdgesFunc = m_engine->evaluate(R"(
        (function() {
            console.log("JavaScript: Getting all edges");
            return []; // Placeholder
        })
    )");
    graphAPI.setProperty("getEdges", getEdgesFunc);
    
    // Enhanced graph operations
    QJSValue clearGraphFunc = m_engine->evaluate(R"(
        (function() {
            console.log("JavaScript: Clearing graph");
            return true; // Placeholder
        })
    )");
    graphAPI.setProperty("clear", clearGraphFunc);
    
    QJSValue connectNodesFunc = m_engine->evaluate(R"(
        (function(fromNodeId, fromSocket, toNodeId, toSocket) {
            if (arguments.length < 4) {
                throw new Error("Graph.connect() requires fromNodeId, fromSocket, toNodeId, toSocket parameters");
            }
            console.log("JavaScript: Connecting " + fromNodeId + "[" + fromSocket + "] to " + toNodeId + "[" + toSocket + "]");
            return {}; // Placeholder edge ID
        })
    )");
    graphAPI.setProperty("connect", connectNodesFunc);
    
    QJSValue getStatsFunc = m_engine->evaluate(R"(
        (function() {
            console.log("JavaScript: Getting graph statistics");
            return {nodes: 0, edges: 0}; // Placeholder
        })
    )");
    graphAPI.setProperty("getStats", getStatsFunc);
    
    QJSValue graphMoveNodeFunc = m_engine->evaluate(R"(
        (function(nodeId, x, y) {
            if (arguments.length < 3) {
                throw new Error("Graph.moveNode() requires nodeId, x, y parameters");
            }
            console.log("JavaScript: Moving node " + nodeId + " to " + x + "," + y);
            return true; // Placeholder
        })
    )");
    graphAPI.setProperty("moveNode", graphMoveNodeFunc);
    
    QJSValue saveXmlFunc = m_engine->evaluate(R"(
        (function(filename) {
            if (arguments.length < 1) {
                throw new Error("Graph.saveXml() requires filename parameter");
            }
            console.log("JavaScript: Saving graph to " + filename);
            return true; // Placeholder
        })
    )");
    graphAPI.setProperty("saveXml", saveXmlFunc);
    
    QJSValue loadXmlFunc = m_engine->evaluate(R"(
        (function(filename) {
            if (arguments.length < 1) {
                throw new Error("Graph.loadXml() requires filename parameter");
            }
            console.log("JavaScript: Loading graph from " + filename);
            return true; // Placeholder
        })
    )");
    graphAPI.setProperty("loadXml", loadXmlFunc);
    
    m_engine->globalObject().setProperty("Graph", graphAPI);
    
    qDebug() << "JavaScriptEngine: Node and Graph APIs registered";
}

void JavaScriptEngine::registerGraphAPI()
{
    QJSValue algorithms = m_engine->newObject();
    
    // Layout algorithms - Qt5 compatible
    QJSValue forceDirectedFunc = m_engine->evaluate(R"(
        (function() {
            console.log("JavaScript: Running force-directed layout");
            return {}; // Placeholder
        })
    )");
    algorithms.setProperty("forceDirected", forceDirectedFunc);
    
    QJSValue hierarchicalFunc = m_engine->evaluate(R"(
        (function() {
            console.log("JavaScript: Running hierarchical layout");
            return {}; // Placeholder
        })
    )");
    algorithms.setProperty("hierarchical", hierarchicalFunc);
    
    m_engine->globalObject().setProperty("Algorithms", algorithms);
    
    qDebug() << "JavaScriptEngine: Graph algorithms registered";
}

void JavaScriptEngine::registerGraphController(Scene* scene, GraphFactory* factory)
{
    m_scene = scene;
    
    // Create GraphController instance
    m_graphController = new GraphController(scene, factory, this);
    
    // Register as global Graph object
    QJSValue controllerValue = m_engine->newQObject(m_graphController);
    m_engine->globalObject().setProperty("Graph", controllerValue);
    
    // Connect signals for debugging
    connect(m_graphController, &GraphController::nodeCreated, [](const QString& uuid) {
        qDebug() << "JavaScript: Node created:" << uuid;
    });
    
    connect(m_graphController, &GraphController::nodeDeleted, [](const QString& uuid) {
        qDebug() << "JavaScript: Node deleted:" << uuid;
    });
    
    connect(m_graphController, &GraphController::edgeCreated, [](const QString& uuid) {
        qDebug() << "JavaScript: Edge created:" << uuid;
    });
    
    connect(m_graphController, &GraphController::edgeDeleted, [](const QString& uuid) {
        qDebug() << "JavaScript: Edge deleted:" << uuid;
    });
    
    connect(m_graphController, &GraphController::error, [](const QString& message) {
        qDebug() << "JavaScript Graph Error:" << message;
    });
    
    qDebug() << "JavaScriptEngine: GraphController registered as 'Graph' global object";
}

QJSValue JavaScriptEngine::createNodeScript(const QString& nodeType, const QString& script)
{
    QString wrappedScript = QString(R"(
        (function(nodeType, inputs, outputs) {
            %1
        })
    )").arg(script);
    
    QJSValue nodeFunction = evaluate(wrappedScript);
    
    if (!nodeFunction.isError()) {
        m_scriptModules[nodeType] = nodeFunction;
        qDebug() << "JavaScriptEngine: Created node script for type:" << nodeType;
    }
    
    return nodeFunction;
}

bool JavaScriptEngine::executeNodeScript(Node* node, const QString& script, const QVariantMap& inputs)
{
    if (!node) {
        m_lastError = "Cannot execute script on null node";
        return false;
    }
    
    // Convert inputs to JavaScript object
    QJSValue jsInputs = m_engine->newObject();
    for (auto it = inputs.begin(); it != inputs.end(); ++it) {
        jsInputs.setProperty(it.key(), m_engine->toScriptValue(it.value()));
    }
    
    // Set up node context
    QJSValue nodeObj = nodeToJSValue(node);
    m_engine->globalObject().setProperty("currentNode", nodeObj);
    m_engine->globalObject().setProperty("inputs", jsInputs);
    
    QJSValue result = evaluate(script);
    
    return !result.isError();
}

QJSValue JavaScriptEngine::processGraph(const QString& algorithm, const QVariantMap& parameters)
{
    QJSValue params = m_engine->newObject();
    for (auto it = parameters.begin(); it != parameters.end(); ++it) {
        params.setProperty(it.key(), m_engine->toScriptValue(it.value()));
    }
    
    QString script = QString("Algorithms.%1(arguments[0])").arg(algorithm);
    QJSValue algorithmFunc = evaluate(script);
    
    if (algorithmFunc.isCallable()) {
        return algorithmFunc.call(QJSValueList() << params);
    }
    
    return QJSValue();
}

bool JavaScriptEngine::hasErrors() const
{
    return !m_lastError.isEmpty();
}

QString JavaScriptEngine::getLastError() const
{
    return m_lastError;
}

void JavaScriptEngine::clearErrors()
{
    m_lastError.clear();
}

QString JavaScriptEngine::getEngineInfo() const
{
    QJSValue info = m_engine->evaluate(R"(
        JSON.stringify({
            engine: 'QJSEngine',
            qtVersion: ')" + QString(QT_VERSION_STR) + R"(',
            ecmaScript: 'ES5+',
            timestamp: new Date().toISOString(),
            features: {
                objects: typeof Object !== 'undefined',
                arrays: typeof Array !== 'undefined',
                functions: typeof Function !== 'undefined',
                json: typeof JSON !== 'undefined',
                console: typeof console !== 'undefined',
                math: typeof Math !== 'undefined',
                date: typeof Date !== 'undefined'
            }
        }, null, 2)
    )");
    
    return info.isError() ? "Error getting engine info" : info.toString();
}

void JavaScriptEngine::logEngineCapabilities() const
{
    qDebug() << "=== JavaScript Engine Information ===";
    qDebug() << "Engine Type: QJSEngine (Qt JavaScript Engine)";
    qDebug() << "Qt Version:" << QT_VERSION_STR;
    qDebug() << "ECMAScript Level: ES5+ (limited ES6 support)";
    
    QString info = getEngineInfo();
    qDebug() << "Detailed Capabilities:" << info;
    
    // Test specific features
    QJSValue testModernJS = m_engine->evaluate("const test = {a: 1, b: 2}; test.a + test.b");
    qDebug() << "Modern JS (const) support:" << (testModernJS.isError() ? "NO" : "YES");
    
    QJSValue testArrowFunction = m_engine->evaluate("((x) => x * 2)(5)");
    qDebug() << "Arrow function support:" << (testArrowFunction.isError() ? "NO" : "YES");
    
    qDebug() << "======================================";
}

void JavaScriptEngine::loadScriptModule(const QString& moduleName, const QString& scriptContent)
{
    QString moduleScript = QString(R"(
        (function() {
            var module = { exports: {} };
            var exports = module.exports;
            
            %1
            
            return module.exports;
        })()
    )").arg(scriptContent);
    
    QJSValue moduleResult = evaluate(moduleScript);
    
    if (!moduleResult.isError()) {
        m_scriptModules[moduleName] = moduleResult;
        qDebug() << "JavaScriptEngine: Loaded module:" << moduleName;
    }
}

QJSValue JavaScriptEngine::getModule(const QString& moduleName)
{
    return m_scriptModules.value(moduleName, QJSValue());
}

void JavaScriptEngine::handleJavaScriptException(const QJSValue& exception)
{
    m_lastError = QString("JavaScript Exception: %1").arg(exception.toString());
    emit scriptError(m_lastError);
    qDebug() << m_lastError;
}

void JavaScriptEngine::setupGlobalAPI()
{
    // Set up global JavaScript environment
    QJSValue globalObject = m_engine->globalObject();
    
    // Add setTimeout/setInterval placeholders - Qt5 compatible
    QJSValue setTimeoutFunc = m_engine->evaluate(
        "(function(func, delay) {"
        "    console.log('JavaScript: setTimeout called (not implemented)');"
        "    return 0;"
        "})"
    );
    globalObject.setProperty("setTimeout", setTimeoutFunc);
    
    QJSValue setIntervalFunc = m_engine->evaluate(
        "(function(func, delay) {"
        "    console.log('JavaScript: setInterval called (not implemented)');"
        "    return 0;"
        "})"
    );
    globalObject.setProperty("setInterval", setIntervalFunc);
}

void JavaScriptEngine::registerConsoleAPI()
{
    QJSValue console = m_engine->newObject();
    
    // Console API - Qt5 compatible with C++ callback
    QJSValue consoleLog = m_engine->evaluate(R"(
        (function() {
            var args = Array.prototype.slice.call(arguments);
            qt_console_log(args.join(" "));
        })
    )");
    console.setProperty("log", consoleLog);
    
    QJSValue consoleError = m_engine->evaluate(R"(
        (function() {
            var args = Array.prototype.slice.call(arguments);
            qt_console_error(args.join(" "));
        })
    )");
    console.setProperty("error", consoleError);
    
    // Register the entire JavaScriptEngine object so its public slots are accessible
    QJSValue engineObject = m_engine->newQObject(this);
    m_engine->globalObject().setProperty("qt_console_log", engineObject.property("qt_console_log"));
    m_engine->globalObject().setProperty("qt_console_error", engineObject.property("qt_console_error"));
    
    m_engine->globalObject().setProperty("console", console);
}

void JavaScriptEngine::qt_console_log(const QString& message)
{
    qDebug() << "JavaScript Console:" << message;
}

void JavaScriptEngine::qt_console_error(const QString& message)
{
    qDebug() << "JavaScript Error:" << message;
}

void JavaScriptEngine::registerUtilityAPI()
{
    QJSValue utils = m_engine->newObject();
    
    // JSON utilities - Qt5 compatible using built-in JSON
    QJSValue parseJSONFunc = m_engine->evaluate(R"(
        (function(jsonString) {
            if (arguments.length < 1) {
                throw new Error("parseJSON() requires a string parameter");
            }
            try {
                return JSON.parse(jsonString);
            } catch (e) {
                throw new Error("Invalid JSON string");
            }
        })
    )");
    utils.setProperty("parseJSON", parseJSONFunc);
    
    QJSValue stringifyJSONFunc = m_engine->evaluate(R"(
        (function(obj) {
            if (arguments.length < 1) {
                throw new Error("stringifyJSON() requires an object parameter");
            }
            try {
                return JSON.stringify(obj);
            } catch (e) {
                throw new Error("Cannot stringify object");
            }
        })
    )");
    utils.setProperty("stringifyJSON", stringifyJSONFunc);
    
    m_engine->globalObject().setProperty("Utils", utils);
}

QJSValue JavaScriptEngine::nodeToJSValue(Node* node)
{
    if (!node) {
        return QJSValue();
    }
    
    QJSValue nodeObj = m_engine->newObject();
    
    // Basic node properties
    nodeObj.setProperty("id", node->getId().toString());
    nodeObj.setProperty("type", node->getNodeType());
    nodeObj.setProperty("x", node->pos().x());
    nodeObj.setProperty("y", node->pos().y());
    
    // Socket information
    QJSValue sockets = m_engine->newArray();
    // TODO: Populate with actual socket data
    nodeObj.setProperty("sockets", sockets);
    
    return nodeObj;
}

QJSValue JavaScriptEngine::edgeToJSValue(Edge* edge)
{
    if (!edge) {
        return QJSValue();
    }
    
    QJSValue edgeObj = m_engine->newObject();
    
    // Basic edge properties
    edgeObj.setProperty("id", edge->getId().toString());
    
    // TODO: Add from/to node information
    
    return edgeObj;
}

void JavaScriptEngine::loadEnhancedAPIs()
{
    // List of enhanced API scripts to auto-load
    QStringList apiScripts = {
        "scripts/enhanced_graph_api.js",
        "scripts/custom_nodes.js",
        "scripts/node_algorithms.js",
        "scripts/node_execution_engine.js",
        "scripts/demo_interactive.js"
    };
    
    for (const QString& scriptPath : apiScripts) {
        QFile file(scriptPath);
        if (file.exists() && file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            QString script = in.readAll();
            
            QJSValue result = evaluate(script);
            if (!result.isError()) {
                qDebug() << "JavaScriptEngine: Loaded enhanced API:" << scriptPath;
            } else {
                qDebug() << "JavaScriptEngine: Failed to load API:" << scriptPath << "-" << result.toString();
            }
        } else {
            qDebug() << "JavaScriptEngine: API script not found:" << scriptPath;
        }
    }
}