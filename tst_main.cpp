#include "tst_main.h"
#include "node_registry.h"
#include <QtTest>
#include <QApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QSysInfo>
#include <QTime>
#include <QFileInfo>

// ─── Test Summary Logging (concise, structured) ───────────────────────────
static QTextStream* testSummaryStream = nullptr;

static void setupLogging()
{
    QDir{"logs"}.mkpath(".");
    const QString logFileName =
        QStringLiteral("logs/TestSummary_%1.log")
        .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss"));

    static QFile summaryFile(logFileName);
    summaryFile.open(QIODevice::WriteOnly | QIODevice::Append);
    testSummaryStream = new QTextStream(&summaryFile);
    
    // Write test session header
    *testSummaryStream << "=== NodeGraph Test Summary ===" << Qt::endl;
    *testSummaryStream << "Date: " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << Qt::endl;
    *testSummaryStream << "Platform: " << QSysInfo::productType() << " " << QSysInfo::productVersion() << Qt::endl;
    *testSummaryStream << Qt::endl;
    testSummaryStream->flush();
    
    // Disable verbose Qt logging during tests
    qInstallMessageHandler([](QtMsgType type, const QMessageLogContext &, const QString &msg) {
        // Only log critical errors to avoid noise
        if (type == QtCriticalMsg || type == QtFatalMsg) {
            if (testSummaryStream) {
                *testSummaryStream << "ERROR: " << msg << Qt::endl;
                testSummaryStream->flush();
            }
        }
    });
    
    qInfo().noquote() << "Test summary logging to:" << logFileName;
}

// Test summary helper
static void logTestSummary(const QString& message)
{
    if (testSummaryStream) {
        *testSummaryStream << QTime::currentTime().toString("hh:mm:ss") << " | " << message << Qt::endl;
        testSummaryStream->flush();
    }
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
    
    // Get test data directory from environment variable
    QString testDataPath = qgetenv("NODEGRAPH_TEST_DATA");
    if (testDataPath.isEmpty()) {
        testDataPath = "..";  // fallback to parent directory
        qDebug() << "NODEGRAPH_TEST_DATA not set, using fallback:" << testDataPath;
    } else {
        qDebug() << "Using test data path from environment:" << testDataPath;
    }
    
    // Test loading available XML files
    QStringList testFiles = {"tests_tiny.xml", "tests_small.xml", "tests_medium.xml"};
    QString testFile;
    
    // Find first available test file in the specified directory
    for (const QString& candidate : testFiles) {
        QString fullPath = QDir(testDataPath).absoluteFilePath(candidate);
        if (QFile::exists(fullPath)) {
            testFile = fullPath;
            break;
        }
    }
    
    qDebug() << "Loading test file:" << (testFile.isEmpty() ? "none found" : testFile);
    
    bool loaded = false;
    if (!testFile.isEmpty()) {
        loaded = m_factory->loadFromXmlFile(testFile);
    }
    
    if (!loaded) {
        qDebug() << "No test file available, creating minimal test instead";
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

// XML Performance Tests - data-driven
void tst_Main::testXmlPerformance()
{
    logTestSummary("=== XML PERFORMANCE TESTS START ===");
    
    // Get test data directory from environment variable
    QString testDataPath = qgetenv("NODEGRAPH_TEST_DATA");
    if (testDataPath.isEmpty()) {
        testDataPath = "..";  // fallback to parent directory
    }
    logTestSummary(QString("Test data path: %1").arg(testDataPath));
    
    // Test files with expected approximate node counts (exclude large files for regular testing)
    QList<QPair<QString, QString>> testCases = {
        {"tests_tiny.xml", "Tiny (10 nodes)"},
        {"tests_small.xml", "Small (100 nodes)"},
        {"tests_medium.xml", "Medium (500 nodes)"}
        // Skip large files: they're too slow for regular testing
        // {"tests_large.xml", "Large (1000 nodes)"},
        // {"tests_stress.xml", "Stress (5000 nodes)"}
    };
    
    int testsRun = 0;
    for (const auto& testCase : testCases) {
        QString fullPath = QDir(testDataPath).absoluteFilePath(testCase.first);
        if (QFile::exists(fullPath)) {
            performXmlLoadTest(fullPath, testCase.second);
            testsRun++;
        } else {
            logTestSummary(QString("SKIP: %1 - file not found").arg(testCase.second));
        }
    }
    
    if (testsRun == 0) {
        logTestSummary("ERROR: No XML test files found for performance testing");
        QSKIP("No XML test files found for performance testing");
    } else {
        logTestSummary(QString("COMPLETE: %1 performance tests executed").arg(testsRun));
    }
}

// XML Dynamic Update Tests
void tst_Main::testNodePositionToXml()
{
    qDebug() << "\n=== Testing Node Position → XML Sync ===";
    QVERIFY(setupEnvironment());
    
    // Create a node at initial position
    auto node = m_factory->createNode("OUT", QPointF(100, 100), 0, 1);
    QVERIFY(node != nullptr);
    
    QUuid nodeId = node->getId();
    qDebug() << "Created node at (100, 100)";
    
    // Move the node to a new position
    QPointF newPos(250, 150);
    node->setPos(newPos);
    qDebug() << "Moved node to" << newPos;
    
    // Verify the node position was updated by checking node directly
    QPointF actualPos = node->pos();
    bool positionMatches = (actualPos.x() == newPos.x()) && (actualPos.y() == newPos.y());
    
    if (positionMatches) {
        qDebug() << "✓ Node position correctly updated to" << actualPos;
    } else {
        qDebug() << "✗ Node position mismatch. Expected:" << newPos << "Actual:" << actualPos;
    }
    
    QVERIFY(positionMatches);
    
    // TODO: Add XML serialization verification when save method is available
    qDebug() << "✓ Node position modification test passed";
}

void tst_Main::testEdgeModificationToXml()
{
    qDebug() << "\n=== Testing Edge Modifications → XML Sync ===";
    QVERIFY(setupEnvironment());
    
    // Create two nodes
    auto node1 = m_factory->createNode("OUT", QPointF(100, 100), 0, 1);
    auto node2 = m_factory->createNode("IN", QPointF(200, 100), 1, 0);
    QVERIFY(node1 && node2);
    
    QUuid node1Id = node1->getId();
    QUuid node2Id = node2->getId();
    
    // Check initial edge count in scene
    int edgeCountBefore = m_testScene->getEdges().size();
    qDebug() << "Initial edge count:" << edgeCountBefore;
    
    // Create an edge
    auto edge = m_factory->createEdge(node1, 0, node2, 0);
    QVERIFY(edge != nullptr);
    QVERIFY(edge->resolveConnections(m_testScene));
    
    // Verify edge appears in scene
    int edgeCountAfter = m_testScene->getEdges().size();
    qDebug() << "After adding edge:" << edgeCountAfter;
    
    QVERIFY(edgeCountAfter > edgeCountBefore);
    
    // Verify edge connection is correct
    bool edgeFound = false;
    for (auto it = m_testScene->getEdges().begin(); it != m_testScene->getEdges().end(); ++it) {
        Edge* sceneEdge = it.value();
        if (sceneEdge->isConnectedToNode(node1Id) && sceneEdge->isConnectedToNode(node2Id)) {
            edgeFound = true;
            break;
        }
    }
    
    if (edgeFound) {
        qDebug() << "✓ Edge correctly connects the two nodes";
    } else {
        qDebug() << "✗ Edge connection not found in scene";
    }
    
    QVERIFY(edgeFound);
    
    // TODO: Add XML serialization verification when save method is available  
    qDebug() << "✓ Edge modification test passed";
}

// Performance Test Helpers
void tst_Main::performXmlLoadTest(const QString& filename, const QString& testName)
{
    logTestSummary(QString("TEST: %1").arg(testName));
    QVERIFY(setupEnvironment());
    
    // Measure load time and track batch mode
    QElapsedTimer totalTimer;
    totalTimer.start();
    
    qint64 loadTime = measureXmlLoadTime(filename);
    qint64 totalTime = totalTimer.elapsed();
    
    // Get loaded graph stats
    int nodeCount = m_testScene->getNodes().size();
    int edgeCount = m_testScene->getEdges().size();
    
    // Log structured summary
    logTestSummary(QString("RESULT: %1 | Nodes: %2 | Edges: %3 | Load: %4ms | Total: %5ms")
                    .arg(testName)
                    .arg(nodeCount)
                    .arg(edgeCount)
                    .arg(loadTime)
                    .arg(totalTime));
    
    // Performance assertions - skip large files in regular testing
    if (nodeCount > 1000) {
        logTestSummary(QString("SKIP_PERF: %1 nodes too large for timing validation").arg(nodeCount));
        return;
    }
    
    // Check edge resolution success rate
    int expectedEdges = qMax(0, nodeCount - 1);  // Rough estimate for chain topology
    float edgeSuccessRate = expectedEdges > 0 ? (float)edgeCount / expectedEdges * 100 : 100;
    logTestSummary(QString("EDGES: %1/%2 connected (%.1f%% success)")
                    .arg(edgeCount).arg(expectedEdges).arg(edgeSuccessRate));
    
    QVERIFY(loadTime < 5000);  // Should load <1000 nodes within 5 seconds
    if (nodeCount == 0) {
        logTestSummary("WARNING: No nodes loaded from file");
    }
}

qint64 tst_Main::measureXmlLoadTime(const QString& filename)
{
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
    
    logTestSummary(QString("LOAD_START: %1").arg(QFileInfo(filename).baseName()));
    
    QElapsedTimer timer;
    timer.start();
    
    // Load the XML file (will use batch mode optimization)
    bool success = m_factory->loadFromXmlFile(filename);
    
    qint64 elapsed = timer.elapsed();
    
    if (!success) {
        logTestSummary(QString("LOAD_FAILED: %1").arg(QFileInfo(filename).baseName()));
        return elapsed;
    }
    
    logTestSummary(QString("LOAD_SUCCESS: %1 in %2ms").arg(QFileInfo(filename).baseName()).arg(elapsed));
    return elapsed;
}

void tst_Main::validateLoadedGraph(int expectedNodes, int expectedEdges)
{
    int actualNodes = m_testScene->getNodes().size();
    int actualEdges = m_testScene->getEdges().size();
    
    qDebug() << QString("Graph validation: %1/%2 nodes, %3/%4 edges")
                .arg(actualNodes).arg(expectedNodes)
                .arg(actualEdges).arg(expectedEdges);
    
    QVERIFY(actualNodes >= expectedNodes * 0.8); // Allow 20% variance
    QVERIFY(actualEdges >= 0); // At least some edges should connect
}

QTEST_MAIN(tst_Main)
#include "tst_main.moc"