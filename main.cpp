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
        
        stream << QString("[%1] %2: %3").arg(timestamp, typeStr, msg) << Qt::endl;
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
    
    // Debug: Show what arguments were parsed
    qDebug() << "=== Command Line Parsing ===";
    qDebug() << "All arguments received:" << app.arguments();
    qDebug() << "Working directory:" << QDir::currentPath();
    qDebug() << "Load option (--load/-l) set:" << parser.isSet(loadFileOption);
    if (parser.isSet(loadFileOption)) {
        qDebug() << "Load option value:" << parser.value(loadFileOption);
    }
    qDebug() << "Positional arguments:" << parser.positionalArguments();
    
    // Create main window
    Window window;
    
    // Handle file loading - Qt5 professional way
    qDebug() << "\n=== File Loading Analysis ===";
    QString filename;
    if (parser.isSet(loadFileOption)) {
        filename = parser.value(loadFileOption);
        qDebug() << "File specified via --load/-l flag:" << filename;
    } else {
        const QStringList positionalArgs = parser.positionalArguments();
        if (!positionalArgs.isEmpty()) {
            filename = positionalArgs.first();
            qDebug() << "File specified as positional argument:" << filename;
        } else {
            qDebug() << "No file specified - will create default test nodes";
        }
    }
    
    // Store information about file loading status for user notification
    bool fileLoadAttempted = !filename.isEmpty();
    QString originalFilename = filename; // Store original filename for user message
    
    xmlDocPtr xmlDoc = nullptr;
    if (!filename.isEmpty()) {
        qDebug() << "Attempting to load file:" << filename;
        qDebug() << "Looking in working directory:" << QDir::currentPath();
        
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
    
    // Register clean design node types
    qDebug() << "=== Registering Node Types ===";
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
    
    // Legacy compatibility for tests (maps to clean types)
    NodeRegistry::instance().registerNode("PROCESSOR", []() { 
        Node* node = new Node(); 
        node->setNodeType("OUT");  // Map legacy PROCESSOR to OUT
        return node; 
    });
    NodeRegistry::instance().registerNode("SOURCE", []() { 
        Node* node = new Node(); 
        node->setNodeType("OUT");  // Map legacy SOURCE to OUT
        return node; 
    });
    NodeRegistry::instance().registerNode("SINK", []() { 
        Node* node = new Node(); 
        node->setNodeType("IN");   // Map legacy SINK to IN
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
    qDebug() << "✓ GraphFactory initialized with scene and XML document";
    
    if (!filename.isEmpty()) {
        // Use GraphFactory's XML loading - single source of truth
        if (!factory.loadFromXmlFile(filename)) {
            qCritical() << "Failed to load XML file:" << filename;
            return -1;
        }
        
        qDebug() << "✓ Graph loaded successfully from file:" << filename;
        
    } else {
        // Create test nodes with different socket configurations
        qDebug() << "=== XML-First Variable Socket Node Creation ===";
        
        Node* sourceNode = factory.createNode("OUT", QPointF(100, 100), 0, 1);    // 0 inputs, 1 output
        Node* processorNode = factory.createNode("OUT", QPointF(300, 100), 1, 1); // 1 input, 1 output  
        Node* sinkNode = factory.createNode("IN", QPointF(500, 100), 1, 0);       // 1 input, 0 outputs
        
        if (sourceNode && processorNode && sinkNode) {
            qDebug() << "✓ Successfully created variable socket nodes via XML-first approach";
            qDebug() << "  - Source node:" << sourceNode->getId().toString(QUuid::WithoutBraces).left(8) << "sockets:" << sourceNode->getSocketCount();
            qDebug() << "  - Processor node:" << processorNode->getId().toString(QUuid::WithoutBraces).left(8) << "sockets:" << processorNode->getSocketCount();
            qDebug() << "  - Sink node:" << sinkNode->getId().toString(QUuid::WithoutBraces).left(8) << "sockets:" << sinkNode->getSocketCount();
            
            // Test XML-first edge creation with variable sockets
            qDebug() << "=== XML-First Variable Socket Edge Creation ===";
            
            // Connect source output 0 to processor input 0
            Edge* edge1 = factory.createEdge(sourceNode, 0, processorNode, 0);
            // Connect processor output 1 to sink input 0  
            Edge* edge2 = factory.createEdge(processorNode, 1, sinkNode, 0);
            
            if (edge1 && edge2) {
                qDebug() << "✓ Created test edges via XML-first approach";
                qDebug() << "  - Edge 1 ID:" << edge1->getId().toString(QUuid::WithoutBraces).left(8);
                qDebug() << "  - Edge 2 ID:" << edge2->getId().toString(QUuid::WithoutBraces).left(8);
                
                // Resolve connections for test edges
                QVector<Edge*> testEdges = {edge1, edge2};
                int successfulConnections = 0;
                for (Edge* edge : testEdges) {
                    if (edge->resolveConnections(scene)) {
                        successfulConnections++;
                    }
                }
                qDebug() << "✓ Test edges resolved:" << successfulConnections << "/" << testEdges.size() << "connected";
            }
        }
        
        // Save XML to file to verify XML-first architecture
        QString xmlFileName = QString("logs/test_graph_%1.xml").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss"));
        xmlSaveFormatFileEnc(xmlFileName.toUtf8().constData(), xmlDoc, "UTF-8", 1);
        qDebug() << "✓ XML document saved to:" << xmlFileName;
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