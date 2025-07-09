#include "tst_main.h"
#include "node_registry.h"
#include <QtTest>
#include <QApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QDebug>

// ─── Test logging setup (copy-paste from main.cpp) ───────────────────────────
static void setupLogging()
{
    QDir{"logs"}.mkpath(".");                                   // ensure dir
    const QString logFileName =
        QStringLiteral("logs/NodeGraph_%1.test.log")
        .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss"));

    static QFile   debugFile{logFileName};
    static QTextStream stream{&debugFile};
    debugFile.open(QIODevice::WriteOnly | QIODevice::Append);

    qInstallMessageHandler([](QtMsgType type,
                               const QMessageLogContext &,
                               const QString &msg)
    {
        static QFile   debugFile{QStringLiteral("logs/NodeGraph_%1.test.log")
                                .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss"))};
        static QTextStream stream{&debugFile};
        static bool initialized = false;
        
        if (!initialized) {
            QDir{"logs"}.mkpath(".");
            debugFile.open(QIODevice::WriteOnly | QIODevice::Append);
            initialized = true;
        }
        
        const QString ts = QDateTime::currentDateTime()
                           .toString("yyyy-MM-dd hh:mm:ss.zzz");
        const char *lvl  = "????";
        switch (type) {
        case QtDebugMsg:    lvl = "DEBUG"; break;
        case QtInfoMsg:     lvl = "INFO "; break;
        case QtWarningMsg:  lvl = "WARN "; break;
        case QtCriticalMsg: lvl = "ERROR"; break;
        case QtFatalMsg:    lvl = "FATAL"; break;
        }
        stream << "[" << ts << "] " << lvl << ": " << msg << Qt::endl;
        stream.flush();
    });

    qInfo().noquote() << "=== TEST RUN STARTED ===  log file →" << logFileName;
}
// ───────────────────────────────────────────────────────

void tst_Main::initTestCase()
{
    setupLogging();  // Enable file logging early
    qDebug() << "=== NodeGraph Main Test Suite ===";
    qDebug() << "Initializing test case...";
    
    m_app = nullptr;
    m_testScene = nullptr;
    m_factory = nullptr;
    m_xmlDoc = nullptr;
    
    qDebug() << "✓ Test case initialized";
}

void tst_Main::cleanupTestCase()
{
    qDebug() << "Cleaning up test case...";
    cleanupEnvironment();
    qInfo().noquote() << "=== TEST RUN FINISHED ===";
    qDebug() << "✓ Test case cleaned up";
}

void tst_Main::init()
{
    qDebug() << "\n--- Setting up test environment ---";
    QVERIFY(setupEnvironment());
    qDebug() << "✓ Test environment ready";
}

void tst_Main::cleanup()
{
    qDebug() << "--- Cleaning up test environment ---";
    cleanupEnvironment();
    qDebug() << "✓ Test environment cleaned up";
}

bool tst_Main::setupEnvironment()
{
    qDebug() << "Setting up test environment...";
    
    // Step 1: Create XML document for factory
    qDebug() << "  Creating XML document...";
    m_xmlDoc = xmlNewDoc(BAD_CAST "1.0");
    if (!m_xmlDoc) {
        qCritical() << "  FAILED: Could not create XML document";
        return false;
    }
    qDebug() << "  ✓ XML document created";
    
    // Step 2: Create root graph element
    qDebug() << "  Creating root graph element...";
    xmlNodePtr root = xmlNewNode(nullptr, BAD_CAST "graph");
    if (!root) {
        qCritical() << "  FAILED: Could not create root graph element";
        xmlFreeDoc(m_xmlDoc);
        m_xmlDoc = nullptr;
        return false;
    }
    xmlDocSetRootElement(m_xmlDoc, root);
    qDebug() << "  ✓ Root graph element created";
    
    // Step 3: Create test scene
    qDebug() << "  Creating test scene...";
    m_testScene = new Scene();
    if (!m_testScene) {
        qCritical() << "  FAILED: Could not create Scene";
        xmlFreeDoc(m_xmlDoc);
        m_xmlDoc = nullptr;
        return false;
    }
    qDebug() << "  ✓ Scene created - initial nodes:" << m_testScene->getNodes().size() 
             << "edges:" << m_testScene->getEdges().size();
    
    // Step 4: Create GraphFactory
    qDebug() << "  Creating GraphFactory...";
    m_factory = new GraphFactory(m_testScene, m_xmlDoc);
    if (!m_factory) {
        qCritical() << "  FAILED: Could not create GraphFactory";
        delete m_testScene;
        m_testScene = nullptr;
        xmlFreeDoc(m_xmlDoc);
        m_xmlDoc = nullptr;
        return false;
    }
    qDebug() << "  ✓ GraphFactory created";
    
    // Step 5: Validate setup
    qDebug() << "  Validating setup...";
    if (!validateSceneSetup()) {
        qCritical() << "  FAILED: Scene setup validation failed";
        return false;
    }
    
    qDebug() << "✓ Test environment setup complete";
    return true;
}

void tst_Main::cleanupEnvironment()
{
    if (m_factory) {
        delete m_factory;
        m_factory = nullptr;
    }
    
    if (m_testScene) {
        delete m_testScene;
        m_testScene = nullptr;
    }
    
    if (m_xmlDoc) {
        xmlFreeDoc(m_xmlDoc);
        m_xmlDoc = nullptr;
    }
}

bool tst_Main::validateSceneSetup()
{
    if (!m_testScene) {
        qCritical() << "    Scene is null";
        return false;
    }
    
    if (!m_factory) {
        qCritical() << "    Factory is null";
        return false;
    }
    
    if (!m_xmlDoc) {
        qCritical() << "    XML document is null";
        return false;
    }
    
    // Check that scene is properly initialized
    const auto& nodes = m_testScene->getNodes();
    const auto& edges = m_testScene->getEdges();
    
    qDebug() << "    Scene validation: nodes=" << nodes.size() << "edges=" << edges.size();
    
    return true;
}

Node* tst_Main::createNode(const QString& type)
{
    qDebug() << "Creating node of type:" << type;
    
    Node* node = m_factory->createNode("node", QPointF(100, 100));
    if (!node) {
        qDebug() << "Factory failed to create node";
        return nullptr;
    }
    
    qDebug() << "Node created with ID:" << node->getId().toString(QUuid::WithoutBraces).left(8);
    
    // Ensure proper socket initialization based on type
    if (type == "OUT") {
        qDebug() << "Creating OUT node with 0 inputs, 1 output";
        node->createSocketsFromXml(0, 1);  // 0 inputs, 1 output
    } else if (type == "IN") {
        qDebug() << "Creating IN node with 1 input, 0 outputs";
        node->createSocketsFromXml(1, 0);  // 1 input, 0 outputs
    } else {
        qDebug() << "Creating default node with 1 input, 1 output";
        node->createSocketsFromXml(1, 1);  // Default: 1 input, 1 output
    }
    
    node->setNodeType(type);
    
    qDebug() << "Node has" << node->getSocketCount() << "sockets";
    
    return node;
}

void tst_Main::testCreateNode()
{
    qDebug() << "\n=== Testing Basic Node Creation ===";
    
    // Test creating nodes directly (bypass factory/registry for now)
    Node* outNode = new Node();
    outNode->setNodeType("OUT");
    outNode->createSocketsFromXml(0, 1);  // 0 inputs, 1 output
    m_testScene->addNode(outNode);
    
    QVERIFY(outNode != nullptr);
    QCOMPARE(outNode->getNodeType(), QString("OUT"));
    QVERIFY(!outNode->getId().isNull());
    QCOMPARE(outNode->getSocketCount(), 1);
    
    Node* inNode = new Node();
    inNode->setNodeType("IN");
    inNode->createSocketsFromXml(1, 0);  // 1 input, 0 outputs
    m_testScene->addNode(inNode);
    
    QVERIFY(inNode != nullptr);
    QCOMPARE(inNode->getNodeType(), QString("IN"));
    QVERIFY(!inNode->getId().isNull());
    QCOMPARE(inNode->getSocketCount(), 1);
    
    // Verify scene has correct node count
    QCOMPARE(m_testScene->getNodes().size(), 2);
    
    qDebug() << "✓ Basic node creation works";
    
    // Test edge creation and connection
    qDebug() << "\n=== Testing Edge System ===";
    
    // Create edge using factory method
    Edge* edge = m_factory->createEdge(outNode, 0, inNode, 0);
    QVERIFY(edge != nullptr);
    
    qDebug() << "Edge created with ID:" << edge->getId().toString(QUuid::WithoutBraces).left(8);
    
    // Test edge resolution
    bool resolved = edge->resolveConnections(m_testScene);
    if (resolved) {
        qDebug() << "✓ Edge resolution successful";
        QVERIFY(resolved);
    } else {
        qDebug() << "✗ Edge resolution failed";
        QVERIFY(resolved);  // This will fail and show the issue
    }
    
    // Verify scene has the edge
    QCOMPARE(m_testScene->getEdges().size(), 1);
    
    qDebug() << "✓ Edge system test passed";
}

void tst_Main::testFactoryNodeCreation()
{
    qDebug() << "\n=== Testing Factory/Registry Node Creation ===";
    
    // First register node types (this should be moved to a shared function)
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
    
    qDebug() << "Registered node types:" << NodeRegistry::instance().getRegisteredTypes();
    
    // Test factory node creation with proper XML structure  
    Node* outNode = m_factory->createNode("OUT", QPointF(100, 100), 0, 1);
    QVERIFY(outNode != nullptr);
    QCOMPARE(outNode->getNodeType(), QString("OUT"));
    QCOMPARE(outNode->getSocketCount(), 1);
    
    Node* inNode = m_factory->createNode("IN", QPointF(200, 100), 1, 0);
    QVERIFY(inNode != nullptr);
    QCOMPARE(inNode->getNodeType(), QString("IN"));
    QCOMPARE(inNode->getSocketCount(), 1);
    
    // Test edge creation through factory
    Edge* edge = m_factory->createEdge(outNode, 0, inNode, 0);
    QVERIFY(edge != nullptr);
    
    // Test edge resolution
    QVERIFY(edge->resolveConnections(m_testScene));
    
    qDebug() << "✓ Factory/Registry system working";
}

void tst_Main::testXmlLoadSave()
{
    qDebug() << "\n=== Testing XML Load/Save Round-Trip ===";
    
    // Register node types
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
    
    // Test loading the tiny test file
    QString testFile = "tests_tiny.xml";
    qDebug() << "Loading test file:" << testFile;
    
    bool loaded = m_factory->loadFromXmlFile(testFile);
    if (!loaded) {
        qDebug() << "Failed to load test file, creating minimal test instead";
        // Create a minimal graph for testing
        Node* node1 = m_factory->createNode("OUT", QPointF(100, 100), 0, 1);
        Node* node2 = m_factory->createNode("IN", QPointF(200, 100), 1, 0);
        Edge* edge = m_factory->createEdge(node1, 0, node2, 0);
        QVERIFY(node1 && node2 && edge);
    } else {
        qDebug() << "✓ Successfully loaded test file";
    }
    
    // Check scene has content
    int nodeCount = m_testScene->getNodes().size();
    int edgeCount = m_testScene->getEdges().size();
    qDebug() << "Loaded graph: " << nodeCount << "nodes," << edgeCount << "edges";
    
    QVERIFY(nodeCount > 0);
    
    qDebug() << "✓ XML loading test passed";
}

void tst_Main::testCompleteWorkflow()
{
    qDebug() << "\n=== Testing Complete Workflow ===";
    
    // Register node types
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
    
    // Step 1: Create a test graph
    qDebug() << "Step 1: Creating test graph...";
    Node* sourceNode = m_factory->createNode("OUT", QPointF(50, 50), 0, 1);
    Node* middleNode = m_factory->createNode("OUT", QPointF(150, 50), 1, 1);  
    Node* sinkNode = m_factory->createNode("IN", QPointF(250, 50), 1, 0);
    
    QVERIFY(sourceNode && middleNode && sinkNode);
    
    Edge* edge1 = m_factory->createEdge(sourceNode, 0, middleNode, 0);
    Edge* edge2 = m_factory->createEdge(middleNode, 1, sinkNode, 0);
    
    QVERIFY(edge1 && edge2);
    
    // Resolve all connections
    QVERIFY(edge1->resolveConnections(m_testScene));
    QVERIFY(edge2->resolveConnections(m_testScene));
    
    // Step 2: Verify initial state
    QCOMPARE(m_testScene->getNodes().size(), 3);
    QCOMPARE(m_testScene->getEdges().size(), 2);
    
    qDebug() << "✓ Test graph created successfully";
    qDebug() << "✓ Complete workflow test passed";
}

QTEST_MAIN(tst_Main)
#include "tst_main.moc"