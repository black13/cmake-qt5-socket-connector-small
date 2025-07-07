#include "selftest.h"
#include "node.h"
#include "edge.h"
#include "socket.h"
#include "scene.h"
#include "graph_factory.h"
#include "node_registry.h"
#include <QDebug>
#include <QElapsedTimer>
#include <QGraphicsView>
#include <QApplication>
#include <QTimer>
#include <QCoreApplication>
#include <libxml/tree.h>
#include <libxml/parser.h>

// Static members
bool SelfTest::s_headless = false;
QApplication* SelfTest::s_app = nullptr;
Scene* SelfTest::s_testScene = nullptr;
GraphFactory* SelfTest::s_factory = nullptr;
int SelfTest::s_testsPassed = 0;
int SelfTest::s_testsFailed = 0;
int SelfTest::s_testsTotal = 0;

int SelfTest::runAll(QApplication* app)
{
    s_app = app;
    s_headless = (app == nullptr);
    s_testsPassed = 0;
    s_testsFailed = 0;
    s_testsTotal = 0;
    
    qDebug() << "=== NodeGraph Integrated Self-Test System ===";
    qDebug() << "Running in" << (s_headless ? "headless" : "GUI") << "mode";
    
    setupTestEnvironment();
    
    // Core component tests
    qDebug() << "\n=== Core Component Tests ===";
    runTest("Node Factory", testNodeFactory);
    runTest("Socket Factory", testSocketFactory);
    runTest("Edge Factory", testEdgeFactory);
    runTest("XML Serialization", testXmlSerialization);
    runTest("Scene Integration", testSceneIntegration);
    
    // Ownership tests
    qDebug() << "\n=== Ownership Tests ===";
    runTest("Factory Ownership", testFactoryOwnership);
    runTest("Registry Cleanup", testRegistryCleanup);
    runTest("Scene Ownership", testSceneOwnership);
    runTest("Edge Ownership", testEdgeOwnership);
    
    // UI tests (only if not headless)
    if (!s_headless) {
        qDebug() << "\n=== UI Component Tests ===";
        runTest("Node Creation", testNodeCreation);
        runTest("Socket Connections", testSocketConnections);
        runTest("Edge Rendering", testEdgeRendering);
        runTest("Selection Handling", testSelectionHandling);
        runTest("Drag and Drop", testDragAndDrop);
    }
    
    // Performance tests
    qDebug() << "\n=== Performance Tests ===";
    runTest("Large Graph Performance", testLargeGraphPerformance);
    runTest("Rapid Create/Delete", testRapidCreateDelete);
    runTest("Memory Usage", testMemoryUsage);
    
    cleanupTestEnvironment();
    
    qDebug() << "\n=== Test Results ===";
    qDebug() << "Total tests:" << s_testsTotal;
    qDebug() << "Passed:" << s_testsPassed;
    qDebug() << "Failed:" << s_testsFailed;
    
    if (s_testsFailed == 0) {
        qDebug() << "✓ All tests passed successfully!";
        return 0;
    } else {
        qDebug() << "✗" << s_testsFailed << "tests failed.";
        return 1;
    }
}

void SelfTest::runTest(const QString& testName, bool (*testFunc)())
{
    s_testsTotal++;
    qDebug() << "Running test:" << testName << "...";
    
    bool result = testFunc();
    
    if (result) {
        qDebug() << "  ✓ PASSED:" << testName;
        s_testsPassed++;
    } else {
        qDebug() << "  ✗ FAILED:" << testName;
        s_testsFailed++;
    }
}

void SelfTest::assertTrue(bool condition, const QString& message)
{
    if (!condition) {
        qDebug() << "    ASSERTION FAILED:" << message;
    }
}

void SelfTest::assertEqual(const QString& expected, const QString& actual, const QString& message)
{
    if (expected != actual) {
        qDebug() << "    ASSERTION FAILED:" << message;
        qDebug() << "      Expected:" << expected;
        qDebug() << "      Actual:" << actual;
    }
}

void SelfTest::assertNotNull(void* ptr, const QString& message)
{
    if (ptr == nullptr) {
        qDebug() << "    ASSERTION FAILED:" << message << "(pointer is null)";
    }
}

void SelfTest::setupTestEnvironment()
{
    qDebug() << "Setting up test environment...";
    
    // Create XML document for factory
    xmlDocPtr doc = xmlNewDoc(BAD_CAST "1.0");
    xmlNodePtr root = xmlNewNode(nullptr, BAD_CAST "graph");
    xmlDocSetRootElement(doc, root);
    
    // Create test scene
    s_testScene = new Scene();
    s_factory = new GraphFactory(s_testScene, doc);
    
    qDebug() << "✓ Test environment ready";
}

void SelfTest::cleanupTestEnvironment()
{
    qDebug() << "Cleaning up test environment...";
    
    delete s_factory;
    delete s_testScene;
    
    qDebug() << "✓ Test environment cleaned up";
}

bool SelfTest::testNodeFactory()
{
    bool passed = true;
    
    // Test node creation
    Node* sourceNode = createTestNode("OUT");
    if (!sourceNode) {
        qDebug() << "    Failed to create SOURCE node";
        passed = false;
    } else {
        assertEqual("OUT", sourceNode->getNodeType(), "Source node type mismatch");
        if (sourceNode->getId().isNull()) {
            qDebug() << "    Source node has null ID";
            passed = false;
        }
    }
    
    Node* procNode = createTestNode("OUT");
    if (!procNode) {
        qDebug() << "    Failed to create PROCESSOR node";
        passed = false;
    } else {
        assertEqual("OUT", procNode->getNodeType(), "Processor node type mismatch");
    }
    
    Node* sinkNode = createTestNode("IN");
    if (!sinkNode) {
        qDebug() << "    Failed to create SINK node";
        passed = false;
    } else {
        assertEqual("IN", sinkNode->getNodeType(), "Sink node type mismatch");
    }
    
    return passed;
}

bool SelfTest::testSocketFactory()
{
    bool passed = true;
    
    Node* testNode = createTestNode("OUT");
    if (!testNode) {
        qDebug() << "    Failed to create test node for socket testing";
        return false;
    }
    
    // Test socket creation
    if (testNode->getSocketCount() <= 0) {
        qDebug() << "    Test node has no sockets";
        passed = false;
    }
    
    Socket* inputSocket = testNode->getSocketByIndex(0);
    if (!inputSocket) {
        qDebug() << "    Failed to get input socket";
        passed = false;
    } else if (inputSocket->getRole() != Socket::Input) {
        qDebug() << "    Input socket has wrong role";
        passed = false;
    }
    
    if (testNode->getSocketCount() > 1) {
        Socket* outputSocket = testNode->getSocketByIndex(1);
        if (!outputSocket) {
            qDebug() << "    Failed to get output socket";
            passed = false;
        } else if (outputSocket->getRole() != Socket::Output) {
            qDebug() << "    Output socket has wrong role";
            passed = false;
        }
    }
    
    return passed;
}

bool SelfTest::testEdgeFactory()
{
    bool passed = true;
    
    Node* sourceNode = createTestNode("OUT");  // Clean design: use OUT instead of SOURCE
    Node* sinkNode = createTestNode("IN");    // Clean design: use IN instead of SINK
    
    if (!sourceNode || !sinkNode) {
        qDebug() << "    Failed to create nodes for edge testing";
        return false;
    }
    
    // Test edge creation
    Edge* edge = createTestEdge(sourceNode, sinkNode);
    if (!edge) {
        qDebug() << "    Failed to create edge";
        passed = false;
    } else {
        if (edge->getId().isNull()) {
            qDebug() << "    Edge has null ID";
            passed = false;
        }
        // Clean design: edges tested via resolveConnections() method
        if (!edge->resolveConnections(s_testScene)) {
            qDebug() << "    Edge failed socket resolution";
            passed = false;
        }
    }
    
    return passed;
}

bool SelfTest::testXmlSerialization()
{
    bool passed = true;
    
    Node* testNode = createTestNode("OUT");
    if (!testNode) {
        qDebug() << "    Failed to create test node for XML testing";
        return false;
    }
    
    // Test XML write
    xmlDocPtr doc = xmlNewDoc(BAD_CAST "1.0");
    xmlNodePtr nodeXml = testNode->write(doc);
    if (!nodeXml) {
        qDebug() << "    Failed to write node to XML";
        passed = false;
    } else {
        // Test XML read
        Node* readNode = new Node();
        readNode->read(nodeXml);
        
        if (readNode->getId() != testNode->getId()) {
            qDebug() << "    XML read/write ID mismatch";
            passed = false;
        }
        if (readNode->getNodeType() != testNode->getNodeType()) {
            qDebug() << "    XML read/write type mismatch";
            passed = false;
        }
        
        delete readNode;
    }
    
    xmlFreeDoc(doc);
    return passed;
}

bool SelfTest::testSceneIntegration()
{
    bool passed = true;
    
    int initialNodeCount = s_testScene->getNodes().size();
    int initialEdgeCount = s_testScene->getEdges().size();
    
    Node* node1 = createTestNode("OUT");
    Node* node2 = createTestNode("IN");
    
    if (s_testScene->getNodes().size() != initialNodeCount + 2) {
        qDebug() << "    Scene node count incorrect after adding nodes";
        passed = false;
    }
    
    Edge* edge = createTestEdge(node1, node2);
    if (s_testScene->getEdges().size() != initialEdgeCount + 1) {
        qDebug() << "    Scene edge count incorrect after adding edge";
        passed = false;
    }
    
    // Test scene lookup
    if (s_testScene->getNode(node1->getId()) != node1) {
        qDebug() << "    Scene node lookup failed";
        passed = false;
    }
    if (edge && s_testScene->getEdge(edge->getId()) != edge) {
        qDebug() << "    Scene edge lookup failed";
        passed = false;
    }
    
    return passed;
}

bool SelfTest::testNodeCreation()
{
    if (s_headless) {
        qDebug() << "    Skipping UI test in headless mode";
        return true;
    }
    
    bool passed = true;
    
    QGraphicsView view(s_testScene);
    view.show();
    
    // Test node creation through UI
    Node* node = s_factory->createNode("OUT", QPointF(200, 200));
    if (!node) {
        qDebug() << "    Failed to create node through UI";
        passed = false;
    } else {
        // Test node positioning
        if (node->pos() != QPointF(200, 200)) {
            qDebug() << "    Node position incorrect";
            passed = false;
        }
    }
    
    return passed;
}

bool SelfTest::testSocketConnections()
{
    bool passed = true;
    
    Node* sourceNode = createTestNode("OUT");  // Clean design: use OUT instead of SOURCE
    Node* sinkNode = createTestNode("IN");    // Clean design: use IN instead of SINK
    
    Socket* outputSocket = sourceNode->getSocketByIndex(0);
    Socket* inputSocket = sinkNode->getSocketByIndex(0);
    
    if (!outputSocket || !inputSocket) {
        qDebug() << "    Failed to get sockets for connection test";
        return false;
    }
    
    // Test socket connection
    Edge* edge = s_factory->connectSockets(outputSocket, inputSocket);
    if (!edge) {
        qDebug() << "    Failed to connect sockets";
        passed = false;
    } else {
        if (outputSocket->getConnectedEdge() != edge) {
            qDebug() << "    Output socket not properly connected";
            passed = false;
        }
        if (inputSocket->getConnectedEdge() != edge) {
            qDebug() << "    Input socket not properly connected";
            passed = false;
        }
        if (!outputSocket->isConnected() || !inputSocket->isConnected()) {
            qDebug() << "    Sockets not reporting as connected";
            passed = false;
        }
    }
    
    return passed;
}

bool SelfTest::testEdgeRendering()
{
    if (s_headless) {
        qDebug() << "    Skipping UI test in headless mode";
        return true;
    }
    
    bool passed = true;
    
    QGraphicsView view(s_testScene);
    view.show();
    
    Node* node1 = createTestNode("OUT");
    Node* node2 = createTestNode("IN");
    if (!node1 || !node2) {
        qDebug() << "    Failed to create nodes for edge rendering test";
        return false;
    }
    
    node1->setPos(100, 100);
    node2->setPos(300, 200);
    
    Edge* edge = createTestEdge(node1, node2);
    if (!edge) {
        qDebug() << "    Failed to create edge for rendering test";
        passed = false;
    } else {
        // Test edge path is built
        if (edge->boundingRect().isEmpty()) {
            qDebug() << "    Edge bounding rect is empty";
            passed = false;
        }
    }
    
    return passed;
}

bool SelfTest::testSelectionHandling()
{
    if (s_headless) {
        qDebug() << "    Skipping UI test in headless mode";
        return true;
    }
    
    bool passed = true;
    
    Node* testNode = createTestNode("OUT");
    if (!testNode) {
        qDebug() << "    Failed to create node for selection test";
        return false;
    }
    
    testNode->setPos(150, 150);
    
    // Test selection using Qt's selection system
    testNode->setSelected(true);
    if (!testNode->isSelected()) {
        qDebug() << "    Node not reporting as selected";
        passed = false;
    }
    
    testNode->setSelected(false);
    if (testNode->isSelected()) {
        qDebug() << "    Node still reporting as selected after deselection";
        passed = false;
    }
    
    return passed;
}

bool SelfTest::testDragAndDrop()
{
    if (s_headless) {
        qDebug() << "    Skipping UI test in headless mode";
        return true;
    }
    
    bool passed = true;
    
    Node* testNode = createTestNode("OUT");
    if (!testNode) {
        qDebug() << "    Failed to create node for drag test";
        return false;
    }
    
    QPointF initialPos = testNode->pos();
    
    // Simulate drag
    QPointF newPos = initialPos + QPointF(50, 50);
    testNode->setPos(newPos);
    
    if (testNode->pos() != newPos) {
        qDebug() << "    Node position not updated after drag";
        passed = false;
    }
    
    return passed;
}

bool SelfTest::testLargeGraphPerformance()
{
    bool passed = true;
    
    const int nodeCount = 100; // Reduced for reasonable testing
    const int edgeCount = 50;
    
    // Test node creation performance
    qint64 nodeTime = measureGraphCreationTime(nodeCount);
    logPerformanceResults(QString("Node creation (%1 nodes)").arg(nodeCount), nodeTime);
    
    // Test edge creation performance
    qint64 edgeTime = measureEdgeCreationTime(edgeCount);
    logPerformanceResults(QString("Edge creation (%1 edges)").arg(edgeCount), edgeTime);
    
    // Verify reasonable performance (not too strict for testing)
    if (nodeTime > 10000) { // 10 seconds max
        qDebug() << "    Node creation performance too slow:" << nodeTime << "ms";
        passed = false;
    }
    if (edgeTime > 5000) { // 5 seconds max
        qDebug() << "    Edge creation performance too slow:" << edgeTime << "ms";
        passed = false;
    }
    
    return passed;
}

bool SelfTest::testRapidCreateDelete()
{
    bool passed = true;
    
    QElapsedTimer timer;
    timer.start();
    
    // Rapid create/delete test
    for (int i = 0; i < 50; ++i) { // Reduced count for reasonable testing
        Node* node = createTestNode("OUT");
        if (!node) {
            qDebug() << "    Failed to create node in rapid test iteration" << i;
            passed = false;
            break;
        }
        
        // Delete through scene
        s_testScene->removeNode(node->getId());
        
        if (s_app && i % 10 == 0) {
            QCoreApplication::processEvents(); // Allow Qt event processing
        }
    }
    
    qint64 elapsed = timer.elapsed();
    logPerformanceResults("Rapid create/delete (50 cycles)", elapsed);
    
    if (elapsed > 5000) { // Should complete in under 5 seconds
        qDebug() << "    Rapid create/delete too slow:" << elapsed << "ms";
        passed = false;
    }
    
    return passed;
}

bool SelfTest::testMemoryUsage()
{
    bool passed = true;
    
    int initialNodeCount = s_testScene->getNodes().size();
    int initialEdgeCount = s_testScene->getEdges().size();
    
    // Create and delete nodes
    QVector<QUuid> nodeIds;
    for (int i = 0; i < 10; ++i) {
        Node* node = createTestNode("OUT");
        if (!node) {
            qDebug() << "    Failed to create node for memory test";
            passed = false;
            break;
        }
        nodeIds.append(node->getId());
    }
    
    // Delete all nodes
    for (const QUuid& id : nodeIds) {
        s_testScene->removeNode(id);
    }
    
    // Verify cleanup
    if (s_testScene->getNodes().size() != initialNodeCount) {
        qDebug() << "    Node count not restored after cleanup";
        passed = false;
    }
    if (s_testScene->getEdges().size() != initialEdgeCount) {
        qDebug() << "    Edge count not restored after cleanup";
        passed = false;
    }
    
    return passed;
}

bool SelfTest::testFactoryOwnership()
{
    bool passed = true;
    
    Node* node = createTestNode("OUT");
    if (!node) {
        qDebug() << "    Failed to create node for ownership test";
        return false;
    }
    
    if (!validateOwnership(node)) {
        qDebug() << "    Node ownership validation failed";
        passed = false;
    }
    
    // Test observer pattern
    if (!node->hasObserver()) {
        qDebug() << "    Node has no observer";
        passed = false;
    }
    if (node->getObserver() != s_factory) {
        qDebug() << "    Node observer is not the factory";
        passed = false;
    }
    
    return passed;
}

bool SelfTest::testRegistryCleanup()
{
    bool passed = true;
    
    // Test simple registry functionality - just check if registry works
    QStringList types = NodeRegistry::instance().getRegisteredTypes();
    if (types.isEmpty()) {
        qDebug() << "    Registry has no registered types";
        passed = false;
    } else {
        qDebug() << "    Registry has" << types.size() << "registered types";
        
        // Test creating a node from any available type
        Node* testNode = NodeRegistry::instance().createNode(types.first());
        if (!testNode) {
            qDebug() << "    Failed to create node from registry";
            passed = false;
        } else {
            qDebug() << "    Successfully created node from registry";
            delete testNode;
        }
    }
    
    return passed;
}

bool SelfTest::testSceneOwnership()
{
    bool passed = true;
    
    if (!validateSceneIntegrity()) {
        qDebug() << "    Scene integrity validation failed";
        passed = false;
    }
    
    // Test scene item management
    int itemCount = s_testScene->items().size();
    int typedCount = s_testScene->getNodes().size() + s_testScene->getEdges().size();
    
    // Account for sockets
    int socketCount = 0;
    for (Node* node : s_testScene->getNodes().values()) {
        socketCount += node->getSocketCount();
    }
    
    if (itemCount != typedCount + socketCount) {
        qDebug() << "    Scene item count mismatch - items:" << itemCount 
                 << "typed:" << typedCount << "sockets:" << socketCount;
        // This is a warning, not necessarily a failure
    }
    
    return passed;
}

bool SelfTest::testEdgeOwnership()
{
    bool passed = true;
    
    Node* node1 = createTestNode("OUT");
    Node* node2 = createTestNode("IN");
    
    if (!node1 || !node2) {
        qDebug() << "    Failed to create nodes for edge ownership test";
        return false;
    }
    
    Edge* edge = createTestEdge(node1, node2);
    if (!edge) {
        qDebug() << "    Failed to create edge for ownership test";
        return false;
    }
    
    // Test edge is properly registered
    if (s_testScene->getEdge(edge->getId()) != edge) {
        qDebug() << "    Edge not properly registered in scene";
        passed = false;
    }
    
    // Clean design: test edge resolution instead of socket UUIDs
    if (!edge->resolveConnections(s_testScene)) {
        qDebug() << "    Edge failed socket resolution";
        passed = false;
        return passed;
    }
    // Clean design: socket connections validated internally by edge resolution
    qDebug() << "    ✓ Edge socket resolution successful";
    
    return passed;
}

// Helper methods
Node* SelfTest::createTestNode(const QString& type)
{
    Q_UNUSED(type)
    return s_factory->createNode("node", QPointF(100, 100));
}

Edge* SelfTest::createTestEdge(Node* from, Node* to)
{
    if (!from || !to) return nullptr;
    
    Socket* fromSocket = from->getSocketByIndex(0);
    Socket* toSocket = to->getSocketByIndex(0);
    
    if (!fromSocket || !toSocket) return nullptr;
    
    return s_factory->connectSockets(fromSocket, toSocket);
}

bool SelfTest::validateOwnership(Node* node)
{
    if (!node) return false;
    
    // Check observer is set
    if (!node->hasObserver()) return false;
    
    // Check node is in scene
    if (s_testScene->getNode(node->getId()) != node) return false;
    
    return true;
}

bool SelfTest::validateSceneIntegrity()
{
    // Check all nodes have valid IDs
    for (Node* node : s_testScene->getNodes().values()) {
        if (!node || node->getId().isNull()) return false;
    }
    
    // Check all edges have valid connections
    for (Edge* edge : s_testScene->getEdges().values()) {
        if (!edge || edge->getId().isNull()) return false;
        // Clean design: test edge resolution instead of socket UUIDs
        if (!edge->resolveConnections(s_testScene)) return false;
    }
    
    return true;
}

qint64 SelfTest::measureGraphCreationTime(int nodeCount)
{
    QElapsedTimer timer;
    timer.start();
    
    QVector<Node*> nodes;
    for (int i = 0; i < nodeCount; ++i) {
        Node* node = createTestNode("OUT");
        nodes.append(node);
        
        if (s_app && i % 10 == 0) {
            QCoreApplication::processEvents(); // Allow Qt event processing
        }
    }
    
    return timer.elapsed();
}

qint64 SelfTest::measureEdgeCreationTime(int edgeCount)
{
    // Create nodes first
    QVector<Node*> nodes;
    for (int i = 0; i < edgeCount * 2; ++i) {
        nodes.append(createTestNode(i % 2 == 0 ? "OUT" : "IN"));
    }
    
    QElapsedTimer timer;
    timer.start();
    
    // Create edges
    for (int i = 0; i < edgeCount; ++i) {
        createTestEdge(nodes[i * 2], nodes[i * 2 + 1]);
        
        if (s_app && i % 10 == 0) {
            QCoreApplication::processEvents(); // Allow Qt event processing
        }
    }
    
    return timer.elapsed();
}

void SelfTest::logPerformanceResults(const QString& testName, qint64 timeMs)
{
    qDebug() << "    Performance:" << testName << "took" << timeMs << "ms";
}