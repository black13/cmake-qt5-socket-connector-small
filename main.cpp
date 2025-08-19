#include <QApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QFileInfo>
#include <QDebug>
#include <QLoggingCategory>
#include <QStandardPaths>
#include <QDir>
#include <QTextStream>
#include <QDateTime>
#include <QMutex>
#include <QTimer>
#include <QMessageBox>
#include <QJSValue>
#include <iostream>
#include "window.h"
#include "scene.h"
#include "node.h"
#include "edge.h"
#include "graph_factory.h"
#include "node_registry.h"
#include "javascript_engine.h"

void setupLogging()
{
    // Create logs directory if it doesn't exist
    QDir logsDir("logs");
    if (!logsDir.exists()) {
        logsDir.mkpath(".");
    }
    
    // Create timestamped log file
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss");
    QString logFileName = QString("logs/NodeGraph_%1.log").arg(timestamp);
    
    // Redirect Qt debug output to file
    static QFile debugFile(logFileName);
    debugFile.open(QIODevice::WriteOnly | QIODevice::Append);
    static QTextStream stream(&debugFile);
    
    qInstallMessageHandler([](QtMsgType type, const QMessageLogContext &context, const QString &msg) {
        QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
        QString typeStr;
        
        switch (type) {
        case QtDebugMsg:    typeStr = "DEBUG"; break;
        case QtInfoMsg:     typeStr = "INFO "; break;
        case QtWarningMsg:  typeStr = "WARN "; break;
        case QtCriticalMsg: typeStr = "ERROR"; break;
        case QtFatalMsg:    typeStr = "FATAL"; break;
        }
        
        QString logEntry = QString("[%1] %2: %3").arg(timestamp, typeStr, msg);
        
        // Write to main log
        stream << logEntry << Qt::endl;
        stream.flush();
        
        // Write JavaScript-related messages to separate JS log
        if (msg.contains("JavaScript", Qt::CaseInsensitive) || 
            msg.contains("Script", Qt::CaseInsensitive) ||
            msg.contains("QJSEngine", Qt::CaseInsensitive) ||
            msg.contains("JS_ERROR", Qt::CaseInsensitive) ||
            msg.contains("JS_EXECUTION", Qt::CaseInsensitive)) {
            
            static QFile jsLogFile(QString("logs/JavaScript_%1.log").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss")));
            if (!jsLogFile.isOpen()) {
                jsLogFile.open(QIODevice::WriteOnly | QIODevice::Append);
            }
            
            if (jsLogFile.isOpen()) {
                QTextStream jsStream(&jsLogFile);
                jsStream << logEntry << Qt::endl;
                jsStream.flush();
            }
        }
    });
    
    qDebug() << "=== NodeGraph Application Started ===";
    qDebug() << "Log file:" << logFileName;
    qDebug() << "Timestamp:" << QDateTime::currentDateTime().toString();
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // Setup file logging
    setupLogging();
    
    
    // Set application metadata for Qt command line tools
    QCoreApplication::setApplicationName("NodeGraph");
    QCoreApplication::setApplicationVersion("1.0.0");
    QCoreApplication::setOrganizationName("NodeGraph Project");
    // setApplicationDisplayName not available in Qt 5.x on all platforms
    
    // Setup Qt5 command line parser
    QCommandLineParser parser;
    parser.setApplicationDescription("A self-serializing node graph editor with libxml2 backend");
    parser.addHelpOption();
    parser.addVersionOption();
    
    // Add file loading option
    QCommandLineOption loadFileOption(QStringList() << "l" << "load",
                                      "Load graph from XML file",
                                      "file");
    parser.addOption(loadFileOption);
    
    // Add verification option
    QCommandLineOption verifyOption(QStringList() << "verify",
                                   "Run JavaScript integration verification and exit");
    parser.addOption(verifyOption);
    
    
    // Add positional argument for file
    parser.addPositionalArgument("file", "XML file to load (optional)");
    
    // Process command line arguments
    parser.process(app);
    
    // Command line parsing
    
    // Create main window
    qDebug() << "=== STEP 1: Creating Window (BEFORE node registration) ===";
    qDebug() << "NodeRegistry types at window creation:" << NodeRegistry::instance().getRegisteredTypes().size();
    Window window;
    qDebug() << "Window created successfully";
    
    // Initialize JavaScript engine
    qDebug() << "=== STEP 2: Initializing JavaScript Engine (BEFORE node registration) ===";
    qDebug() << "NodeRegistry types before JS engine:" << NodeRegistry::instance().getRegisteredTypes().size();
    Scene* scene = window.getScene();
    if (scene) {
        auto* jsEngine = scene->getJavaScriptEngine();
        if (!jsEngine) {
            qDebug() << "Warning: JavaScript engine initialization failed";
        } else {
            qDebug() << "JavaScript engine initialized successfully";
            qDebug() << "NodeRegistry types after JS engine init:" << NodeRegistry::instance().getRegisteredTypes().size();
        }
    }
    
    // Handle file loading
    QString filename;
    if (parser.isSet(loadFileOption)) {
        filename = parser.value(loadFileOption);
    } else {
        const QStringList positionalArgs = parser.positionalArguments();
        if (!positionalArgs.isEmpty()) {
            filename = positionalArgs.first();
        }
    }
    
    // Store information about file loading status for user notification
    bool fileLoadAttempted = !filename.isEmpty();
    QString originalFilename = filename; // Store original filename for user message
    
    // Create empty XML document for GraphFactory
    // GraphFactory will handle all file loading - single XML authority
    qDebug() << "=== Creating Empty XML Document ===";
    xmlDocPtr xmlDoc = xmlNewDoc(BAD_CAST "1.0");
    xmlNodePtr root = xmlNewNode(nullptr, BAD_CAST "graph");
    xmlDocSetRootElement(xmlDoc, root);
    
    xmlSetProp(root, BAD_CAST "version", BAD_CAST "1.0");
    xmlSetProp(root, BAD_CAST "xmlns", BAD_CAST "http://nodegraph.org/schema");
    
    qDebug() << "✓ Empty XML document created - GraphFactory will handle file loading";
    
    // Register all supported node types
    qDebug() << "=== STEP 3: Registering Node Types (AFTER window/JS engine creation) ===";
    qDebug() << "NodeRegistry types BEFORE registration:" << NodeRegistry::instance().getRegisteredTypes().size();
    qDebug() << "Starting node type registration process...";
    
    // Core node types
    NodeRegistry::instance().registerNode("IN", []() { 
        Node* node = new Node(); 
        node->setNodeType("IN"); 
        return node; 
    });
    qDebug() << "Registered: IN (count now:" << NodeRegistry::instance().getRegisteredTypes().size() << ")";
    
    NodeRegistry::instance().registerNode("OUT", []() { 
        Node* node = new Node(); 
        node->setNodeType("OUT"); 
        return node; 
    });
    qDebug() << "Registered: OUT (count now:" << NodeRegistry::instance().getRegisteredTypes().size() << ")";
    
    NodeRegistry::instance().registerNode("PROC", []() { 
        Node* node = new Node(); 
        node->setNodeType("PROC"); 
        return node; 
    });
    qDebug() << "Registered: PROC (count now:" << NodeRegistry::instance().getRegisteredTypes().size() << ")";
    
    // Palette node types - each with proper type designation
    NodeRegistry::instance().registerNode("SOURCE", []() { 
        Node* node = new Node(); 
        node->setNodeType("SOURCE"); 
        return node; 
    });
    qDebug() << "Registered: SOURCE (count now:" << NodeRegistry::instance().getRegisteredTypes().size() << ")";
    
    NodeRegistry::instance().registerNode("SINK", []() { 
        Node* node = new Node(); 
        node->setNodeType("SINK"); 
        return node; 
    });
    qDebug() << "Registered: SINK (count now:" << NodeRegistry::instance().getRegisteredTypes().size() << ")";
    
    NodeRegistry::instance().registerNode("TRANSFORM", []() { 
        Node* node = new Node(); 
        node->setNodeType("TRANSFORM"); 
        return node; 
    });
    qDebug() << "Registered: TRANSFORM (count now:" << NodeRegistry::instance().getRegisteredTypes().size() << ")";
    
    NodeRegistry::instance().registerNode("MERGE", []() { 
        Node* node = new Node(); 
        node->setNodeType("MERGE"); 
        return node; 
    });
    qDebug() << "Registered: MERGE (count now:" << NodeRegistry::instance().getRegisteredTypes().size() << ")";
    
    NodeRegistry::instance().registerNode("SPLIT", []() { 
        Node* node = new Node(); 
        node->setNodeType("SPLIT"); 
        return node; 
    });
    qDebug() << "Registered: SPLIT (count now:" << NodeRegistry::instance().getRegisteredTypes().size() << ")";
    
    // Legacy compatibility for older tests
    NodeRegistry::instance().registerNode("PROCESSOR", []() { 
        Node* node = new Node(); 
        node->setNodeType("PROCESSOR"); 
        return node; 
    });
    qDebug() << "Registered: PROCESSOR (count now:" << NodeRegistry::instance().getRegisteredTypes().size() << ")";
    
    QStringList registeredTypes = NodeRegistry::instance().getRegisteredTypes();

    // Test the NodeRegistry to verify nodes are registered
    qDebug() << "=== NodeGraph Application Starting ===";
    qDebug() << "Registered node types:" << NodeRegistry::instance().getRegisteredTypes();
    qDebug() << "Total registered types:" << registeredTypes.size();
    qDebug() << "=== Logging each registered type ===";
    for (int i = 0; i < registeredTypes.size(); ++i) {
        qDebug() << QString("  [%1] \"%2\"").arg(i + 1).arg(registeredTypes[i]);
    }
    qDebug() << "======================================";
    
    // Initialize GraphFactory with scene and XML document
    // Scene* scene = window.getScene(); // Already declared above
    if (!scene) {
        qCritical() << "✗ Failed to get scene from window";
        return -1;
    }
    
    GraphFactory factory(scene, xmlDoc);
    qDebug() << "GraphFactory initialized with empty XML document";
    
    if (!filename.isEmpty()) {
        // GraphFactory is now the single XML authority
        qDebug() << "Loading file via GraphFactory:" << filename;
        if (!factory.loadFromXmlFile(filename)) {
            qCritical() << "GraphFactory failed to load XML file:" << filename;
            if (fileLoadAttempted) {
                qDebug() << "Original filename was:" << originalFilename;
            }
            return -1;
        }
        
        qDebug() << "✓ Graph loaded successfully from file via GraphFactory:" << filename;
        
    } else {
        qDebug() << "Starting with empty graph - no file specified";
        qDebug() << "=== Starting with Empty Graph ===";
        qDebug() << "✓ No file specified - application will start with clean scene";
        qDebug() << "  Users can create nodes manually or load XML files via Ctrl+L";
    }
    
    qDebug() << "=== XML-First Architecture Test Complete ===";
    
    // Set current file if we loaded from command line
    if (!filename.isEmpty()) {
        window.setCurrentFile(filename);
        qDebug() << "📁 Command line file loaded - Ctrl+S will save to:" << filename;
    }
    
    // Cleanup XML document when done
    // Note: GraphFactory holds reference, so clean up after window closes
    
    window.show();
    
    // Check for verification mode - run after window is fully shown
    bool verifyMode = parser.isSet(verifyOption);
    
    if (verifyMode) {
        qDebug() << "=== VERIFICATION MODE ENABLED ===";
        qDebug() << "Running JavaScript integration verification...";
        
        // Give the application time to fully initialize after show()
        QTimer::singleShot(1000, [&]() {
            // Get JavaScript engine from scene
            Scene* scene = window.getScene();
            if (!scene) {
                qCritical() << "VERIFY_ERROR: Scene not available";
                app.exit(1);
                return;
            }
            
            auto* jsEngine = scene->getJavaScriptEngine();
            if (!jsEngine) {
                qCritical() << "VERIFY_ERROR: JavaScript engine not available";
                app.exit(1);
                return;
            }
            
            // CRITICAL: Setup GraphController for verification
            // The verification tests need real GraphController access
            qDebug() << "VERIFY_SETUP: Setting up GraphController for verification tests";
            jsEngine->registerGraphController(scene, &factory);
            qDebug() << "VERIFY_SETUP: GraphController registration completed";
            
            // Load and execute verification script
            QString scriptPath = "scripts/startup_verification.js";
            QJSValue result = jsEngine->evaluateFile(scriptPath);
            
            if (result.isError()) {
                qCritical() << "VERIFY_ERROR: Script execution failed:" << result.toString();
                app.exit(1);
                return;
            }
            
            // Extract test results
            QJSValue testResults = result;
            if (testResults.hasProperty("failed")) {
                int failed = testResults.property("failed").toInt();
                int passed = testResults.property("passed").toInt();
                int total = testResults.property("total").toInt();
                
                qDebug() << "VERIFY_SUMMARY:" << passed << "/" << total << "tests passed";
                
                if (failed == 0) {
                    qDebug() << "VERIFY_RESULT: SUCCESS - All JavaScript integration tests passed";
                    app.exit(0);
                } else {
                    qDebug() << "VERIFY_RESULT: FAILURE -" << failed << "tests failed";
                    app.exit(1);
                }
            } else {
                qDebug() << "VERIFY_ERROR: Could not parse test results";
                app.exit(1);
            }
        });
    }
    
    // Show user-friendly message about file loading status
    if (fileLoadAttempted && originalFilename != filename) {
        // File was attempted but failed to load (filename was cleared)
        QTimer::singleShot(500, [&window, originalFilename]() {
            QMessageBox::information(&window, "File Not Found", 
                QString("The specified file could not be found or loaded:\n\n%1\n\nStarting with an empty graph instead.\n\nYou can create a new graph or open an existing file using File → Open.")
                .arg(originalFilename));
        });
    }
    
    int result = app.exec();
    
    // Final status before exit
    qDebug() << "=== NodeGraph Application Ending ===";
    
    // Cleanup XML document
    if (xmlDoc) {
        xmlFreeDoc(xmlDoc);
        qDebug() << "✓ XML document cleaned up";
    }
    
    return result;
}