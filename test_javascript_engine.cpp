#include <QtTest/QtTest>
#include <QCoreApplication>
#include <QDebug>
#include <QJSValue>
#include <QVariantMap>

#include "javascript_engine.h"
#include "scene.h"
#include "node.h"
#include "edge.h"

class TestJavaScriptEngine : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Basic Engine Tests
    void testEngineCreation();
    void testBasicEvaluation();
    void testErrorHandling();
    void testScriptFromFile();
    
    // Console API Tests
    void testConsoleAPI();
    void testConsoleLogging();
    
    // Modern JavaScript Features
    void testES6Features();
    void testJSONSupport();
    void testMathOperations();
    void testArrayMethods();
    
    // Scene Integration Tests
    void testSceneRegistration();
    void testNodeAPIAvailability();
    void testGraphAPIAvailability();
    
    // Node Scripting Tests
    void testNodeScriptExecution();
    void testNodeScriptWithInputs();
    void testNodeScriptErrors();
    
    // Graph Processing Tests
    void testGraphProcessing();
    void testGraphAlgorithms();
    
    // Module System Tests
    void testScriptModules();
    void testModuleLoading();
    
    // Performance and Memory Tests
    void testLargeScriptExecution();
    void testMemoryUsage();
    
    // Error Recovery Tests
    void testErrorRecovery();
    void testExceptionHandling();

private:
    JavaScriptEngine* m_jsEngine;
    Scene* m_scene;
    
    // Helper methods
    void verifyJSValue(const QJSValue& value, const QString& testName);
    QString createTestScript(const QString& scriptContent);
    void logTestResults(const QString& testName, bool passed, const QString& details = "");
};

void TestJavaScriptEngine::initTestCase()
{
    qDebug() << "=== JavaScript Engine Test Suite Starting ===";
    
    // Initialize Qt application if needed
    if (!QCoreApplication::instance()) {
        static char arg0[] = "test_javascript_engine";
        static char* argv[] = { arg0, nullptr };
        static int argc = 1;
        new QCoreApplication(argc, argv);
    }
}

void TestJavaScriptEngine::cleanupTestCase()
{
    qDebug() << "=== JavaScript Engine Test Suite Complete ===";
}

void TestJavaScriptEngine::init()
{
    // Create fresh instances for each test
    m_scene = new Scene();
    m_jsEngine = m_scene->getJavaScriptEngine();
    
    QVERIFY(m_jsEngine != nullptr);
    qDebug() << "Test setup: Created Scene and JavaScriptEngine";
}

void TestJavaScriptEngine::cleanup()
{
    delete m_scene;
    m_scene = nullptr;
    m_jsEngine = nullptr;
    qDebug() << "Test cleanup: Destroyed Scene and JavaScriptEngine";
}

void TestJavaScriptEngine::testEngineCreation()
{
    qDebug() << "\n--- Testing Engine Creation ---";
    
    QVERIFY(m_jsEngine != nullptr);
    QVERIFY(!m_jsEngine->hasErrors());
    
    QString engineInfo = m_jsEngine->getEngineInfo();
    QVERIFY(!engineInfo.isEmpty());
    
    logTestResults("Engine Creation", true, QString("Engine info: %1").arg(engineInfo));
}

void TestJavaScriptEngine::testBasicEvaluation()
{
    qDebug() << "\n--- Testing Basic Script Evaluation ---";
    
    // Test simple arithmetic
    QJSValue result1 = m_jsEngine->evaluate("2 + 3");
    QVERIFY(!result1.isError());
    QCOMPARE(result1.toInt(), 5);
    
    // Test string operations
    QJSValue result2 = m_jsEngine->evaluate("'Hello' + ' ' + 'World'");
    QVERIFY(!result2.isError());
    QCOMPARE(result2.toString(), QString("Hello World"));
    
    // Test boolean operations
    QJSValue result3 = m_jsEngine->evaluate("true && false");
    QVERIFY(!result3.isError());
    QCOMPARE(result3.toBool(), false);
    
    logTestResults("Basic Evaluation", true, "Arithmetic, strings, and boolean operations working");
}

void TestJavaScriptEngine::testErrorHandling()
{
    qDebug() << "\n--- Testing Error Handling ---";
    
    // Test syntax error
    QJSValue result1 = m_jsEngine->evaluate("invalid syntax here");
    QVERIFY(result1.isError());
    QVERIFY(m_jsEngine->hasErrors());
    
    QString lastError = m_jsEngine->getLastError();
    QVERIFY(!lastError.isEmpty());
    
    // Clear errors and test recovery
    m_jsEngine->clearErrors();
    QVERIFY(!m_jsEngine->hasErrors());
    
    // Test that engine can still execute after error
    QJSValue result2 = m_jsEngine->evaluate("1 + 1");
    QVERIFY(!result2.isError());
    QCOMPARE(result2.toInt(), 2);
    
    logTestResults("Error Handling", true, QString("Error detected and recovered: %1").arg(lastError));
}

void TestJavaScriptEngine::testScriptFromFile()
{
    qDebug() << "\n--- Testing Script File Loading ---";
    
    // Create a temporary test script file
    QString testScript = R"(
        // Test script for file loading
        const result = {
            message: "Script loaded from file",
            timestamp: new Date().getTime(),
            calculation: Math.sqrt(16)
        };
        result;
    )";
    
    QString tempFile = "test_script_temp.js";
    QFile file(tempFile);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << testScript;
        file.close();
        
        QJSValue result = m_jsEngine->evaluateFile(tempFile);
        QVERIFY(!result.isError());
        
        QJSValue message = result.property("message");
        QCOMPARE(message.toString(), QString("Script loaded from file"));
        
        QJSValue calculation = result.property("calculation");
        QCOMPARE(calculation.toInt(), 4);
        
        // Clean up
        QFile::remove(tempFile);
        
        logTestResults("Script File Loading", true, "File loading and execution successful");
    } else {
        QFAIL("Could not create temporary test script file");
    }
}

void TestJavaScriptEngine::testConsoleAPI()
{
    qDebug() << "\n--- Testing Console API ---";
    
    // Test console.log availability
    QJSValue result = m_jsEngine->evaluate(R"(
        console.log("Test console.log message");
        console.error("Test console.error message");
        "Console API test complete";
    )");
    
    QVERIFY(!result.isError());
    QCOMPARE(result.toString(), QString("Console API test complete"));
    
    logTestResults("Console API", true, "console.log and console.error working");
}

void TestJavaScriptEngine::testES6Features()
{
    qDebug() << "\n--- Testing Modern JavaScript Features ---";
    
    QString es6Script = R"(
        // Arrow functions
        const add = (a, b) => a + b;
        
        // Template literals
        const name = "NodeGraph";
        const message = `Hello, ${name}!`;
        
        // Destructuring
        const obj = { x: 10, y: 20 };
        const { x, y } = obj;
        
        // Spread operator
        const arr1 = [1, 2, 3];
        const arr2 = [...arr1, 4, 5];
        
        // Return results
        {
            addition: add(5, 3),
            template: message,
            destructured: x + y,
            spread: arr2.length
        };
    )";
    
    QJSValue result = m_jsEngine->evaluate(es6Script);
    QVERIFY2(!result.isError(), qPrintable(result.toString()));
    
    if (!result.isError()) {
        QCOMPARE(result.property("addition").toInt(), 8);
        QCOMPARE(result.property("template").toString(), QString("Hello, NodeGraph!"));
        QCOMPARE(result.property("destructured").toInt(), 30);
        QCOMPARE(result.property("spread").toInt(), 5);
        
        logTestResults("ES6 Features", true, "Arrow functions, templates, destructuring, spread working");
    } else {
        logTestResults("ES6 Features", false, result.toString());
    }
}

void TestJavaScriptEngine::testJSONSupport()
{
    qDebug() << "\n--- Testing JSON Support ---";
    
    QString jsonScript = R"(
        const data = { name: "Test", value: 42, active: true };
        const jsonString = JSON.stringify(data);
        const parsed = JSON.parse(jsonString);
        
        {
            original: data.value,
            serialized: jsonString.length > 0,
            roundtrip: parsed.value
        };
    )";
    
    QJSValue result = m_jsEngine->evaluate(jsonScript);
    QVERIFY(!result.isError());
    
    QCOMPARE(result.property("original").toInt(), 42);
    QVERIFY(result.property("serialized").toBool());
    QCOMPARE(result.property("roundtrip").toInt(), 42);
    
    logTestResults("JSON Support", true, "JSON.stringify and JSON.parse working");
}

void TestJavaScriptEngine::testArrayMethods()
{
    qDebug() << "\n--- Testing Array Methods ---";
    
    QString arrayScript = R"(
        const numbers = [1, 2, 3, 4, 5];
        
        {
            original: numbers.length,
            mapped: numbers.map(x => x * 2),
            filtered: numbers.filter(x => x > 3),
            reduced: numbers.reduce((acc, x) => acc + x, 0)
        };
    )";
    
    QJSValue result = m_jsEngine->evaluate(arrayScript);
    QVERIFY(!result.isError());
    
    QCOMPARE(result.property("original").toInt(), 5);
    
    QJSValue mapped = result.property("mapped");
    QCOMPARE(mapped.property("length").toInt(), 5);
    QCOMPARE(mapped.property(1).toInt(), 4); // 2 * 2
    
    QJSValue filtered = result.property("filtered");
    QCOMPARE(filtered.property("length").toInt(), 2); // [4, 5]
    
    QCOMPARE(result.property("reduced").toInt(), 15); // 1+2+3+4+5
    
    logTestResults("Array Methods", true, "map, filter, reduce working correctly");
}

void TestJavaScriptEngine::testSceneRegistration()
{
    qDebug() << "\n--- Testing Scene Registration ---";
    
    // The Scene constructor should have already registered APIs
    QVERIFY(m_scene != nullptr);
    QVERIFY(m_jsEngine != nullptr);
    
    // Test that we can access engine capabilities
    QJSValue result = m_jsEngine->evaluate("'Scene registration test'");
    QVERIFY(!result.isError());
    
    logTestResults("Scene Registration", true, "Scene and engine properly connected");
}

void TestJavaScriptEngine::testNodeAPIAvailability() 
{
    qDebug() << "\n--- Testing Node API Availability ---";
    
    // Test if Node API has been registered
    QJSValue result = m_jsEngine->evaluate(R"(
        typeof Node !== 'undefined' ? 'available' : 'missing';
    )");
    
    QString availability = result.toString();
    bool nodeAPIAvailable = (availability == "available");
    
    logTestResults("Node API Availability", nodeAPIAvailable, 
                   QString("Node API status: %1").arg(availability));
    
    // This test might fail if Node API isn't fully implemented yet
    // That's expected and will help us identify what needs to be fixed
}

void TestJavaScriptEngine::testNodeScriptExecution()
{
    qDebug() << "\n--- Testing Node Script Execution ---";
    
    // Create a test node
    Node* testNode = new Node();
    m_scene->addNode(testNode);
    
    // Test basic node script execution
    QString script = "console.log('Node script executed'); 'success';";
    bool executed = m_jsEngine->executeNodeScript(testNode, script);
    
    // Clean up
    m_scene->deleteNode(testNode->getId());
    
    logTestResults("Node Script Execution", executed, "Basic node script execution attempted");
}

void TestJavaScriptEngine::testScriptModules()
{
    qDebug() << "\n--- Testing Script Module System ---";
    
    QString moduleScript = R"(
        const testModule = {
            version: '1.0.0',
            add: function(a, b) { return a + b; },
            multiply: function(a, b) { return a * b; }
        };
        testModule;
    )";
    
    // Load module
    m_jsEngine->loadScriptModule("testModule", moduleScript);
    
    // Try to get module
    QJSValue module = m_jsEngine->getModule("testModule");
    bool moduleLoaded = !module.isUndefined();
    
    logTestResults("Script Modules", moduleLoaded, 
                   moduleLoaded ? "Module loaded successfully" : "Module loading failed");
}

void TestJavaScriptEngine::testLargeScriptExecution()
{
    qDebug() << "\n--- Testing Large Script Performance ---";
    
    QString largeScript = R"(
        const startTime = new Date().getTime();
        
        // Create large array and process it
        const largeArray = [];
        for (let i = 0; i < 10000; i++) {
            largeArray.push(i);
        }
        
        const processed = largeArray
            .filter(x => x % 2 === 0)
            .map(x => x * 2)
            .slice(0, 100);
        
        const endTime = new Date().getTime();
        
        {
            arrayLength: largeArray.length,
            processedLength: processed.length,
            executionTime: endTime - startTime
        };
    )";
    
    QJSValue result = m_jsEngine->evaluate(largeScript);
    QVERIFY(!result.isError());
    
    int arrayLength = result.property("arrayLength").toInt();
    int processedLength = result.property("processedLength").toInt();
    int executionTime = result.property("executionTime").toInt();
    
    QCOMPARE(arrayLength, 10000);
    QCOMPARE(processedLength, 100);
    
    logTestResults("Large Script Performance", true, 
                   QString("Processed %1 items in %2ms").arg(arrayLength).arg(executionTime));
}

void TestJavaScriptEngine::testErrorRecovery()
{
    qDebug() << "\n--- Testing Error Recovery ---";
    
    // Execute script with error
    QJSValue errorResult = m_jsEngine->evaluate("throw new Error('Test error');");
    QVERIFY(errorResult.isError());
    QVERIFY(m_jsEngine->hasErrors());
    
    // Clear errors
    m_jsEngine->clearErrors();
    QVERIFY(!m_jsEngine->hasErrors());
    
    // Execute valid script after error
    QJSValue validResult = m_jsEngine->evaluate("'Recovery successful'");
    QVERIFY(!validResult.isError());
    QCOMPARE(validResult.toString(), QString("Recovery successful"));
    
    logTestResults("Error Recovery", true, "Engine recovered successfully after error");
}

// Helper methods
void TestJavaScriptEngine::verifyJSValue(const QJSValue& value, const QString& testName)
{
    if (value.isError()) {
        qDebug() << "JavaScript Error in" << testName << ":" << value.toString();
        QFAIL(qPrintable(QString("JavaScript error in %1: %2").arg(testName, value.toString())));
    }
}

QString TestJavaScriptEngine::createTestScript(const QString& scriptContent)
{
    return QString("(function() { %1 })()").arg(scriptContent);
}

void TestJavaScriptEngine::logTestResults(const QString& testName, bool passed, const QString& details)
{
    QString status = passed ? "✅ PASSED" : "❌ FAILED";
    qDebug() << QString("%1: %2").arg(status, testName);
    if (!details.isEmpty()) {
        qDebug() << "   Details:" << details;
    }
}

// Placeholder implementations for tests that need more complex setup
void TestJavaScriptEngine::testConsoleLogging() { /* TODO: Implement */ }
void TestJavaScriptEngine::testGraphAPIAvailability() { /* TODO: Implement */ }
void TestJavaScriptEngine::testNodeScriptWithInputs() { /* TODO: Implement */ }
void TestJavaScriptEngine::testNodeScriptErrors() { /* TODO: Implement */ }
void TestJavaScriptEngine::testGraphProcessing() { /* TODO: Implement */ }
void TestJavaScriptEngine::testGraphAlgorithms() { /* TODO: Implement */ }
void TestJavaScriptEngine::testModuleLoading() { /* TODO: Implement */ }
void TestJavaScriptEngine::testMemoryUsage() { /* TODO: Implement */ }
void TestJavaScriptEngine::testExceptionHandling() { /* TODO: Implement */ }

QTEST_MAIN(TestJavaScriptEngine)
#include "test_javascript_engine.moc"