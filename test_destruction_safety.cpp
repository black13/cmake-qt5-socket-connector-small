#include <QtTest>
#include <QGraphicsScene>
#include <QDebug>
#include "node.h"
#include "edge.h"
#include "socket.h"
#include "scene.h"

/**
 * Test cases for built-in destruction safety
 * 
 * This tests the critical use-after-free vulnerability fix where
 * Edge destruction could access dangling Node pointers.
 */
class TestDestructionSafety : public QObject
{
    Q_OBJECT

private slots:
    void test_nodeDestroyedBeforeEdge();
    void test_edgeDestroyedBeforeNode();
    void test_massiveNodeEdgeDestruction();
    void test_crossConnectedNodeDestruction();

private:
    Scene* createTestScene();
    void verifyNoMemoryLeaks();
};

void TestDestructionSafety::test_nodeDestroyedBeforeEdge()
{
    qDebug() << "\n=== TEST: Node destroyed before Edge ===";
    
    Scene* scene = createTestScene();
    
    // Create two nodes and connect them
    Node* node1 = new Node(QUuid::createUuid(), QPointF(0, 0));
    Node* node2 = new Node(QUuid::createUuid(), QPointF(100, 0));
    
    node1->setNodeType("OUT");
    node1->setSocketCount(0, 1);  // 1 output
    node2->setNodeType("IN"); 
    node2->setSocketCount(1, 0);  // 1 input
    
    scene->addItem(node1);
    scene->addItem(node2);
    
    // Create edge
    Edge* edge = new Edge();
    edge->setResolvedSockets(node1->getSocketByIndex(0), node2->getSocketByIndex(0));
    scene->addItem(edge);
    
    qDebug() << "Created edge connecting two nodes";
    
    // Get the edge's view of the nodes BEFORE destruction
    Node* edgeFromNode = edge->getFromNode();
    Node* edgeToNode = edge->getToNode();
    
    QVERIFY(edgeFromNode == node1);
    QVERIFY(edgeToNode == node2);
    
    // CRITICAL TEST: Destroy node1 first, then check edge safety
    qDebug() << "Destroying node1 while edge still exists...";
    delete node1;  // This should call edge->onNodeDestroying(node1)
    
    // Edge should now return nullptr for the destroyed node
    QVERIFY(edge->getFromNode() == nullptr);  // Should be safe null
    QVERIFY(edge->getToNode() == node2);      // Should still be valid
    
    qDebug() << "Edge safely handled node1 destruction";
    
    // Clean up remaining objects
    delete edge;  // This should NOT crash when accessing destroyed node1
    delete node2;
    delete scene;
    
    qDebug() << "✓ Test passed - no use-after-free crash";
}

void TestDestructionSafety::test_edgeDestroyedBeforeNode()
{
    qDebug() << "\n=== TEST: Edge destroyed before Node ===";
    
    Scene* scene = createTestScene();
    
    // Create two nodes and connect them
    Node* node1 = new Node(QUuid::createUuid(), QPointF(0, 0));
    Node* node2 = new Node(QUuid::createUuid(), QPointF(100, 0));
    
    node1->setNodeType("OUT");
    node1->setSocketCount(0, 1);
    node2->setNodeType("IN");
    node2->setSocketCount(1, 0);
    
    scene->addItem(node1);
    scene->addItem(node2);
    
    Edge* edge = new Edge();
    edge->setResolvedSockets(node1->getSocketByIndex(0), node2->getSocketByIndex(0));
    scene->addItem(edge);
    
    qDebug() << "Created edge connecting two nodes";
    
    // CRITICAL TEST: Destroy edge first, then nodes
    qDebug() << "Destroying edge while nodes still exist...";
    delete edge;  // This should safely unregister from both nodes
    
    // Nodes should still be valid and not crash
    QVERIFY(!node1->isBeingDestroyed());
    QVERIFY(!node2->isBeingDestroyed());
    
    qDebug() << "Edge safely destroyed without affecting nodes";
    
    // Clean up nodes
    delete node1;  // Should not crash trying to notify destroyed edge
    delete node2;
    delete scene;
    
    qDebug() << "✓ Test passed - normal destruction order works";
}

void TestDestructionSafety::test_massiveNodeEdgeDestruction()
{
    qDebug() << "\n=== TEST: Massive node-edge destruction stress test ===";
    
    Scene* scene = createTestScene();
    QVector<Node*> nodes;
    QVector<Edge*> edges;
    
    // Create a grid of interconnected nodes
    const int gridSize = 10;
    qDebug() << "Creating" << gridSize << "x" << gridSize << "grid of nodes...";
    
    // Create nodes
    for (int i = 0; i < gridSize * gridSize; ++i) {
        Node* node = new Node(QUuid::createUuid(), QPointF(i % gridSize * 50, i / gridSize * 50));
        node->setNodeType("OUT");
        node->setSocketCount(2, 2);  // 2 inputs, 2 outputs each
        scene->addItem(node);
        nodes.append(node);
    }
    
    // Create edges connecting adjacent nodes
    for (int i = 0; i < gridSize; ++i) {
        for (int j = 0; j < gridSize; ++j) {
            int nodeIndex = i * gridSize + j;
            Node* currentNode = nodes[nodeIndex];
            
            // Connect to right neighbor
            if (j < gridSize - 1) {
                Node* rightNode = nodes[nodeIndex + 1];
                Edge* edge = new Edge();
                edge->setResolvedSockets(currentNode->getSocketByIndex(1), rightNode->getSocketByIndex(0));
                scene->addItem(edge);
                edges.append(edge);
            }
            
            // Connect to bottom neighbor
            if (i < gridSize - 1) {
                Node* bottomNode = nodes[nodeIndex + gridSize];
                Edge* edge = new Edge();
                edge->setResolvedSockets(currentNode->getSocketByIndex(3), bottomNode->getSocketByIndex(2));
                scene->addItem(edge);
                edges.append(edge);
            }
        }
    }
    
    qDebug() << "Created" << nodes.size() << "nodes and" << edges.size() << "edges";
    
    // STRESS TEST: Delete nodes in random order while edges exist
    qDebug() << "Randomly destroying nodes while edges exist...";
    QVector<int> nodeIndices;
    for (int i = 0; i < nodes.size(); ++i) {
        nodeIndices.append(i);
    }
    
    // Shuffle destruction order
    for (int i = 0; i < nodeIndices.size(); ++i) {
        int j = QRandomGenerator::global()->bounded(nodeIndices.size());
        nodeIndices.swapItemsAt(i, j);
    }
    
    // Delete first half of nodes randomly
    for (int i = 0; i < nodes.size() / 2; ++i) {
        int index = nodeIndices[i];
        if (nodes[index]) {
            delete nodes[index];
            nodes[index] = nullptr;
        }
    }
    
    qDebug() << "Destroyed half the nodes, checking edge safety...";
    
    // Verify edges handle the destroyed nodes safely
    int nullFromNodes = 0, nullToNodes = 0;
    for (Edge* edge : edges) {
        if (edge->getFromNode() == nullptr) nullFromNodes++;
        if (edge->getToNode() == nullptr) nullToNodes++;
    }
    
    qDebug() << "Edges with null fromNode:" << nullFromNodes;
    qDebug() << "Edges with null toNode:" << nullToNodes;
    
    QVERIFY(nullFromNodes > 0 || nullToNodes > 0);  // Should have some nulled nodes
    
    // Clean up remaining objects
    for (Edge* edge : edges) {
        delete edge;
    }
    for (Node* node : nodes) {
        if (node) delete node;
    }
    delete scene;
    
    qDebug() << "✓ Stress test passed - handled massive destruction safely";
}

void TestDestructionSafety::test_crossConnectedNodeDestruction()
{
    qDebug() << "\n=== TEST: Cross-connected node destruction ===";
    
    Scene* scene = createTestScene();
    
    // Create 4 nodes in a cross pattern: A connects to B and C, B connects to D
    Node* nodeA = new Node(QUuid::createUuid(), QPointF(0, 0));
    Node* nodeB = new Node(QUuid::createUuid(), QPointF(100, 0));
    Node* nodeC = new Node(QUuid::createUuid(), QPointF(0, 100));
    Node* nodeD = new Node(QUuid::createUuid(), QPointF(100, 100));
    
    nodeA->setNodeType("OUT");
    nodeA->setSocketCount(0, 2);  // 2 outputs
    nodeB->setNodeType("OUT");
    nodeB->setSocketCount(1, 1);  // 1 input, 1 output
    nodeC->setNodeType("IN");
    nodeC->setSocketCount(1, 0);  // 1 input
    nodeD->setNodeType("IN");
    nodeD->setSocketCount(1, 0);  // 1 input
    
    scene->addItem(nodeA);
    scene->addItem(nodeB);
    scene->addItem(nodeC);
    scene->addItem(nodeD);
    
    // Create cross connections
    Edge* edgeAB = new Edge();
    edgeAB->setResolvedSockets(nodeA->getSocketByIndex(0), nodeB->getSocketByIndex(0));
    scene->addItem(edgeAB);
    
    Edge* edgeAC = new Edge();
    edgeAC->setResolvedSockets(nodeA->getSocketByIndex(1), nodeC->getSocketByIndex(0));
    scene->addItem(edgeAC);
    
    Edge* edgeBD = new Edge();
    edgeBD->setResolvedSockets(nodeB->getSocketByIndex(1), nodeD->getSocketByIndex(0));
    scene->addItem(edgeBD);
    
    qDebug() << "Created cross-connected graph: A->B->D, A->C";
    
    // CRITICAL TEST: Delete the central node A first
    qDebug() << "Destroying central node A...";
    delete nodeA;  // Should notify edgeAB and edgeAC
    
    // Verify edges handle the destruction
    QVERIFY(edgeAB->getFromNode() == nullptr);  // A is gone
    QVERIFY(edgeAB->getToNode() == nodeB);      // B still valid
    QVERIFY(edgeAC->getFromNode() == nullptr);  // A is gone  
    QVERIFY(edgeAC->getToNode() == nodeC);      // C still valid
    QVERIFY(edgeBD->getFromNode() == nodeB);    // Unaffected
    QVERIFY(edgeBD->getToNode() == nodeD);      // Unaffected
    
    qDebug() << "All edges handled node A destruction safely";
    
    // Clean up
    delete edgeAB;
    delete edgeAC;
    delete edgeBD;
    delete nodeB;
    delete nodeC;
    delete nodeD;
    delete scene;
    
    qDebug() << "✓ Cross-connection test passed";
}

Scene* TestDestructionSafety::createTestScene()
{
    Scene* scene = new Scene();
    scene->setSceneRect(-500, -500, 1000, 1000);
    return scene;
}

void TestDestructionSafety::verifyNoMemoryLeaks()
{
    // In a real implementation, this would use valgrind or similar
    // For now, just verify we can create and destroy without crashes
    QVERIFY(true);
}

// Include the MOC generated file
#include "test_destruction_safety.moc"

QTEST_MAIN(TestDestructionSafety)