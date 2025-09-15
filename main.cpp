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
#include <QUuid>
#include <QMessageBox>
#include <iostream>
#include "window.h"
#include "scene.h"
#include "node.h"
#include "edge.h"
#include "graph_factory.h"
#include "node_templates.h"
#include <libxml/tree.h>

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
        case QtDebugMsg:
            typeStr = "DEBUG";
            break;
        case QtInfoMsg:
            typeStr = "INFO ";
            break;
        case QtWarningMsg:
            typeStr = "WARN ";
            break;
        case QtCriticalMsg:
            typeStr = "ERROR";
            break;
        case QtFatalMsg:
            typeStr = "FATAL";
            break;
        }
        
        QString logEntry = QString("[%1] %2: %3").arg(timestamp, typeStr, msg);
        
        // Write to main log
        stream << logEntry << Qt::endl;
        stream.flush();
        
        // JavaScript logging removed - focusing on core C++ functionality
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
    
    // Session identification for log correlation (ChatGPT suggestion)
    QString sessionId = QUuid::createUuid().toString(QUuid::WithoutBraces).left(8);
    qDebug() << "Session:" << sessionId << "Graph: empty (0 nodes, 0 edges)";
    
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
    
    // JavaScript verification option removed - focusing on core C++ functionality
    
    
    // Add positional argument for file
    parser.addPositionalArgument("file", "XML file to load (optional)");
    
    // Log command line arguments before processing
    qDebug() << "Command line arguments:";
    for (int i = 0; i < argc; i++) {
        qDebug() << QString("  [%1]: %2").arg(i).arg(argv[i]);
    }
    
    // Check for help/version before processing to log them
    QStringList args = QCoreApplication::arguments();
    if (args.contains("--help") || args.contains("-h")) {
        qDebug() << "=== HELP REQUESTED ===";
        qDebug() << "Application Name:" << QCoreApplication::applicationName();
        qDebug() << "Version:" << QCoreApplication::applicationVersion();
        qDebug() << "Description:" << parser.applicationDescription();
        qDebug() << "Usage: NodeGraph [options] file";
        qDebug() << "Options:";
        qDebug() << "  -h, --help         Displays help on commandline options";
        qDebug() << "  --help-all         Displays help including Qt specific options";
        qDebug() << "  -v, --version      Displays version information";
        qDebug() << "  -l, --load <file>  Load graph from XML file";
        qDebug() << "Arguments:";
        qDebug() << "  file               XML file to load (optional)";
        qDebug() << "=== END HELP ===";
    }
    
    if (args.contains("--version") || args.contains("-v")) {
        qDebug() << "=== VERSION REQUESTED ===";
        qDebug() << "Application:" << QCoreApplication::applicationName();
        qDebug() << "Version:" << QCoreApplication::applicationVersion();
        qDebug() << "Organization:" << QCoreApplication::organizationName();
        qDebug() << "Build Date:" << __DATE__ << __TIME__;
        qDebug() << "Qt Version:" << QT_VERSION_STR;
        qDebug() << "=== END VERSION ===";
    }
    
    // Process command line arguments
    parser.process(app);
    
    // Command line parsing
    
    // Create main window first (so Scene exists)
    Window window;
    Scene* scene = window.getScene();
    
    // Create XML document for GraphFactory
    qDebug() << "Creating unified XML document for GraphFactory";
    xmlDocPtr xmlDoc = xmlNewDoc(BAD_CAST "1.0");
    xmlNodePtr root = xmlNewNode(nullptr, BAD_CAST "graph");
    xmlDocSetRootElement(xmlDoc, root);
    xmlSetProp(root, BAD_CAST "version", BAD_CAST "1.0");
    xmlSetProp(root, BAD_CAST "xmlns", BAD_CAST "http://nodegraph.org/schema");
    
    // Create single GraphFactory instance
    GraphFactory factory(scene, xmlDoc);
    qDebug() << "GraphFactory created with unified XML document";
    
    // Hand factory to window (non-owning)
    window.adoptFactory(&factory);
    qDebug() << "Window adopted factory - single source of truth established";
    
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
    
    qDebug() << "=== Using Template-Driven Node Creation (Single Source of Truth) ===";
    qDebug() << "Available node types from templates:" << NodeTypeTemplates::getAvailableTypes();
    qDebug() << "Single GraphFactory will handle all XML operations";
    
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
    
    // JavaScript verification mode removed - focusing on core C++ functionality
    
    // Show user-friendly message about file loading status
    if (fileLoadAttempted && originalFilename != filename) {
        // File was attempted but failed to load (filename was cleared)
        QTimer::singleShot(500, [&window, originalFilename]() {
            QMessageBox::information(&window, "File Not Found", 
                QString("The specified file could not be found or loaded:\n\n%1\n\nStarting with an empty graph instead.\n\nYou can create a new graph or open an existing file using File -> Open.")
                .arg(originalFilename));
        });
    }
    
    int result = app.exec();
    
    // Final status before exit
    qDebug() << "=== NodeGraph Application Ending ===";
    
    // Cleanup XML document (main is the owner)
    xmlFreeDoc(xmlDoc);
    qDebug() << "XML document cleaned up";
    
    return result;
}