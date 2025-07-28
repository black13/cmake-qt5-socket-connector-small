// Simple test to verify JavaScript integration
#include <iostream>
#include <QApplication>
#include <QDebug>
#include "scene.h"
#include "graph_factory.h"
#include "javascript_engine.h"
#include <libxml/tree.h>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    qDebug() << "Testing JavaScript integration...";
    
    // Create the core components
    Scene* scene = new Scene();
    
    // Create XML document for factory
    xmlDocPtr doc = xmlNewDoc(BAD_CAST "1.0");
    xmlNodePtr root = xmlNewNode(nullptr, BAD_CAST "graph");
    xmlDocSetRootElement(doc, root);
    
    GraphFactory* factory = new GraphFactory(scene, doc);
    
    // Get JavaScript engine from scene
    JavaScriptEngine* jsEngine = scene->getJavaScriptEngine();
    
    // Register GraphController
    jsEngine->registerGraphController(scene, factory);
    
    // Test basic JavaScript execution
    QString testScript = R"(
        console.log("=== JavaScript Integration Test ===");
        
        // Test Graph object is available
        console.log("Graph object available:", typeof Graph);
        
        // Test node creation
        console.log("Creating test node...");
        let nodeId = Graph.createNode("Source", 100, 100);
        console.log("Created node ID:", nodeId);
        
        // Test stats
        let stats = Graph.getStats();
        console.log("Graph stats:", JSON.stringify(stats));
        
        // Test success
        console.log("✅ JavaScript integration test PASSED");
        
        return "test_complete";
    )";
    
    QJSValue result = jsEngine->evaluate(testScript);
    
    if (result.isError()) {
        qDebug() << "❌ JavaScript test FAILED:" << result.toString();
        return 1;
    } else {
        qDebug() << "✅ JavaScript test result:" << result.toString();
    }
    
    // Cleanup
    delete factory;
    delete scene;
    xmlFreeDoc(doc);
    
    qDebug() << "JavaScript integration test completed successfully!";
    
    return 0;
}