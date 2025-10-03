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

    // Add test option (runs default enumeration test)
    QCommandLineOption testOption(QStringList() << "t" << "test",
                                  "Run XML enumeration test on startup");
    parser.addOption(testOption);

    // Add script option (runs any JavaScript file)
    QCommandLineOption scriptOption(QStringList() << "s" << "script",
                                    "Run JavaScript test script on startup",
                                    "script");
    parser.addOption(scriptOption);

    // Add positional argument for file
    parser.addPositionalArgument("file", "XML file to load (optional)");
    
    // Process command line arguments
    parser.process(app);
    
    // Command line parsing

    // Create main window
    Window window;

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
    
    xmlDocPtr xmlDoc = nullptr;
    if (!filename.isEmpty()) {
        
        QFileInfo fileInfo(filename);
        qDebug() << "File path analysis:";
        qDebug() << "  - Absolute path:" << fileInfo.absoluteFilePath();
        qDebug() << "  - File exists:" << fileInfo.exists();
        qDebug() << "  - File readable:" << fileInfo.isReadable();
        qDebug() << "  - File size:" << fileInfo.size() << "bytes";
        qDebug() << "  - Directory:" << fileInfo.absoluteDir().absolutePath();
        
        if (fileInfo.exists() && fileInfo.isReadable()) {
            qDebug() << "✓ File found and accessible, loading XML content...";
            
            // Load XML file using libxml2
            xmlDoc = xmlParseFile(fileInfo.absoluteFilePath().toUtf8().constData());
            if (!xmlDoc) {
                qCritical() << "✗ Failed to parse XML file:" << filename;
                qCritical() << "Check XML syntax and format";
                qDebug() << "Continuing with empty graph...";
                filename.clear(); // Clear filename so we create a default document
            } else {
                qDebug() << "✓ XML file parsed successfully";
            }
        } else {
            qDebug() << "✗ File not found or not readable:" << filename;
            qDebug() << "Searched in:" << QDir::currentPath();
            qDebug() << "Full path attempted:" << fileInfo.absoluteFilePath();
            qDebug() << "Continuing with empty graph...";
            filename.clear(); // Clear filename so we create a default document
        }
    }
    
    // Create default XML document if no file was loaded or file was missing/invalid
    if (!xmlDoc) {
        // Create default XML document structure for XML-first architecture
        qDebug() << "=== Creating XML Document Structure ===";
        xmlDoc = xmlNewDoc(BAD_CAST "1.0");
        xmlNodePtr root = xmlNewNode(nullptr, BAD_CAST "graph");
        xmlDocSetRootElement(xmlDoc, root);
        
        xmlSetProp(root, BAD_CAST "version", BAD_CAST "1.0");
        xmlSetProp(root, BAD_CAST "xmlns", BAD_CAST "http://nodegraph.org/schema");
        
        qDebug() << "✓ XML document created with root element";
    }
    
    // Register all supported node types
    qDebug() << "=== Registering Node Types ===";
    
    // Core node types
    NodeRegistry::instance().registerNode("IN", []() { 
        Node* node = new Node(); 
        node->setNodeType("IN"); 
        return node; 
    });
    NodeRegistry::instance().registerNode("OUT", []() { 
        Node* node = new Node(); 
        node->setNodeType("OUT"); 
        return node; 
    });
    NodeRegistry::instance().registerNode("PROC", []() { 
        Node* node = new Node(); 
        node->setNodeType("PROC"); 
        return node; 
    });
    
    // Palette node types - each with proper type designation
    NodeRegistry::instance().registerNode("SOURCE", []() { 
        Node* node = new Node(); 
        node->setNodeType("SOURCE"); 
        return node; 
    });
    NodeRegistry::instance().registerNode("SINK", []() { 
        Node* node = new Node(); 
        node->setNodeType("SINK"); 
        return node; 
    });
    NodeRegistry::instance().registerNode("TRANSFORM", []() { 
        Node* node = new Node(); 
        node->setNodeType("TRANSFORM"); 
        return node; 
    });
    NodeRegistry::instance().registerNode("MERGE", []() { 
        Node* node = new Node(); 
        node->setNodeType("MERGE"); 
        return node; 
    });
    NodeRegistry::instance().registerNode("SPLIT", []() { 
        Node* node = new Node(); 
        node->setNodeType("SPLIT"); 
        return node; 
    });
    
    // Legacy compatibility for older tests
    NodeRegistry::instance().registerNode("PROCESSOR", []() { 
        Node* node = new Node(); 
        node->setNodeType("PROCESSOR"); 
        return node; 
    });
    
    // Test the NodeRegistry to verify nodes are registered
    qDebug() << "=== NodeGraph Application Starting ===";
    qDebug() << "Registered node types:" << NodeRegistry::instance().getRegisteredTypes();

    // Initialize GraphFactory with scene and XML document
    Scene* scene = window.getScene();
    if (!scene) {
        qCritical() << "✗ Failed to get scene from window";
        return -1;
    }
    
    GraphFactory factory(scene, xmlDoc);
    qDebug() << "GraphFactory initialized";
    
    if (!filename.isEmpty()) {
        // Use GraphFactory's XML loading - single source of truth
        if (!factory.loadFromXmlFile(filename)) {
            qCritical() << "Failed to load XML file:" << filename;
            return -1;
        }
        
        qDebug() << "✓ Graph loaded successfully from file:" << filename;
        
    } else {
        // Start with empty graph - no default test nodes
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

    // Set up auto-test script if requested (Window will run it on first show)
    if (parser.isSet(scriptOption)) {
        // User specified custom script
        window.setAutoTestScript(parser.value(scriptOption));
    } else if (parser.isSet(testOption)) {
        // Default test script
        window.setAutoTestScript("scripts/test_xml_load_enumeration.js");
    }

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
        qDebug() << "✓ XML document cleaned up";
    }
    
    return result;
}