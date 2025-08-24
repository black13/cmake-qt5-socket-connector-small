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
#include <iostream>
#include "window.h"
#include "scene.h"
#include "node.h"
#include "edge.h"
#include "graph_factory.h"
#include "node_registry.h"

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
    
    
    // Add positional argument for file
    parser.addPositionalArgument("file", "XML file to load (optional)");
    
    // Process command line arguments
    parser.process(app);
    
    // Command line parsing
    
    // Create main window
    // qDebug() << "=== STEP 1: Creating Window (BEFORE node registration) ===";
    qDebug() << "NodeRegistry types at window creation:" << NodeRegistry::instance().getRegisteredTypes().size();
    Window window;
    qDebug() << "Window created successfully";
    
    // Get scene reference for graph initialization
    Scene* scene = window.getScene();
    if (!scene) {
        qCritical() << "Failed to get scene from window";
        return -1;
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
    // qDebug() << "=== Creating Empty XML Document ===";
    xmlDocPtr xmlDoc = xmlNewDoc(BAD_CAST "1.0");
    xmlNodePtr root = xmlNewNode(nullptr, BAD_CAST "graph");
    xmlDocSetRootElement(xmlDoc, root);
    
    xmlSetProp(root, BAD_CAST "version", BAD_CAST "1.0");
    xmlSetProp(root, BAD_CAST "xmlns", BAD_CAST "http://nodegraph.org/schema");
    
    qDebug() << "Empty XML document created - GraphFactory will handle file loading";
    
    // Register all supported node types
    // qDebug() << "=== STEP 3: Registering Node Types (AFTER window/JS engine creation) ===";
    qDebug() << "NodeRegistry types BEFORE registration:" << NodeRegistry::instance().getRegisteredTypes().size();
    qDebug() << "Starting node type registration process...";
    
    // Core node types
    NodeRegistry::instance().registerNode("IN", []() { 
        Node* node = new Node(); 
        node->setNodeType("IN"); 
        return node; 
    });
    // qDebug() << "Registered: IN (count now:" << NodeRegistry::instance().getRegisteredTypes().size() << ")"
    
    NodeRegistry::instance().registerNode("OUT", []() { 
        Node* node = new Node(); 
        node->setNodeType("OUT"); 
        return node; 
    });
    // qDebug() << "Registered: OUT (count now:" << NodeRegistry::instance().getRegisteredTypes().size() << ")"
    
    NodeRegistry::instance().registerNode("PROC", []() { 
        Node* node = new Node(); 
        node->setNodeType("PROC"); 
        return node; 
    });
    // qDebug() << "Registered: PROC (count now:" << NodeRegistry::instance().getRegisteredTypes().size() << ")"
    
    // Palette node types - each with proper type designation
    NodeRegistry::instance().registerNode("SOURCE", []() { 
        Node* node = new Node(); 
        node->setNodeType("SOURCE"); 
        return node; 
    });
    // qDebug() << "Registered: SOURCE (count now:" << NodeRegistry::instance().getRegisteredTypes().size() << ")"
    
    NodeRegistry::instance().registerNode("SINK", []() { 
        Node* node = new Node(); 
        node->setNodeType("SINK"); 
        return node; 
    });
    // qDebug() << "Registered: SINK (count now:" << NodeRegistry::instance().getRegisteredTypes().size() << ")"
    
    NodeRegistry::instance().registerNode("TRANSFORM", []() { 
        Node* node = new Node(); 
        node->setNodeType("TRANSFORM"); 
        return node; 
    });
    // qDebug() << "Registered: TRANSFORM (count now:" << NodeRegistry::instance().getRegisteredTypes().size() << ")"
    
    NodeRegistry::instance().registerNode("MERGE", []() { 
        Node* node = new Node(); 
        node->setNodeType("MERGE"); 
        return node; 
    });
    // qDebug() << "Registered: MERGE (count now:" << NodeRegistry::instance().getRegisteredTypes().size() << ")"
    
    NodeRegistry::instance().registerNode("SPLIT", []() { 
        Node* node = new Node(); 
        node->setNodeType("SPLIT"); 
        return node; 
    });
    // qDebug() << "Registered: SPLIT (count now:" << NodeRegistry::instance().getRegisteredTypes().size() << ")"
    
    // Legacy compatibility for older tests
    NodeRegistry::instance().registerNode("PROCESSOR", []() { 
        Node* node = new Node(); 
        node->setNodeType("PROCESSOR"); 
        return node; 
    });
    // qDebug() << "Registered: PROCESSOR (count now:" << NodeRegistry::instance().getRegisteredTypes().size() << ")"
    
    QStringList registeredTypes = NodeRegistry::instance().getRegisteredTypes();

    // Test the NodeRegistry to verify nodes are registered
    qDebug() << "=== NodeGraph Application Starting ===";
    qDebug() << "Registered node types:" << NodeRegistry::instance().getRegisteredTypes();
    qDebug() << "Total registered types:" << registeredTypes.size();
    // qDebug() << "=== Logging each registered type ===";
    for (int i = 0; i < registeredTypes.size(); ++i) {
        qDebug() << QString("  [%1] \"%2\"").arg(i + 1).arg(registeredTypes[i]);
    }
    // qDebug() << "======================================";
    
    // Initialize GraphFactory with scene and XML document
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
        
        qDebug() << "Graph loaded successfully from file via GraphFactory:" << filename;
        
    } else {
        qDebug() << "Starting with empty graph - no file specified";
        // qDebug() << "=== Starting with Empty Graph ===";
        qDebug() << "No file specified - application will start with clean scene";
        qDebug() << "  Users can create nodes manually or load XML files via Ctrl+L";
    }
    
    // qDebug() << "=== XML-First Architecture Test Complete ===";
    
    // Set current file if we loaded from command line
    if (!filename.isEmpty()) {
        window.setCurrentFile(filename);
        qDebug() << "Command line file loaded - Ctrl+S will save to:" << filename;
    }
    
    // Cleanup XML document when done
    // Note: GraphFactory holds reference, so clean up after window closes
    
    window.show();
    
    
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
        qDebug() << "XML document cleaned up";
    }
    
    return result;
}