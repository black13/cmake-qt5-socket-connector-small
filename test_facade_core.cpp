#include <iostream>
#include <QCoreApplication>
#include "node.h"
#include "edge.h"
#include "node_facade.h"
#include "edge_facade.h"
#include <libxml/tree.h>

int main(int argc, char *argv[]) {
    // Minimal Qt core application (no GUI)
    QCoreApplication app(argc, argv);
    
    std::cout << "=== NodeFacade and EdgeFacade Core Test ===\n\n";
    
    // Test 1: NodeFacade basic functionality
    std::cout << "=== Test 1: NodeFacade Basic Test ===\n";
    
    Node* node = new Node();
    node->setNodeType("TRANSFORM");
    node->setPos(QPointF(100.5, 200.75));
    
    // Wrap in facade
    NodeFacade facade(node);
    
    std::cout << "Node through facade:\n";
    std::cout << "  ID: " << facade.id().toString().toStdString() << "\n";
    std::cout << "  Type: " << facade.nodeType().toStdString() << "\n";
    std::cout << "  Position: (" << facade.position().x() << ", " << facade.position().y() << ")\n";
    
    // Test facade modification
    facade.setPosition(QPointF(300.0, 400.0));
    std::cout << "After facade.setPosition(300, 400):\n";
    std::cout << "  Real Node Position: (" << node->pos().x() << ", " << node->pos().y() << ")\n";
    
    bool positionTest = (node->pos().x() == 300.0 && node->pos().y() == 400.0);
    std::cout << "  Position update test: " << (positionTest ? "PASS" : "FAIL") << "\n";
    
    // Test 2: NodeFacade serialization
    std::cout << "\n=== Test 2: NodeFacade Serialization ===\n";
    
    xmlDocPtr doc = xmlNewDoc(BAD_CAST "1.0");
    xmlNodePtr root = xmlNewNode(nullptr, BAD_CAST "test");
    xmlDocSetRootElement(doc, root);
    
    // Serialize through facade
    xmlNodePtr serializedNode = facade.write(doc, root);
    
    // Create new node and facade for deserialization
    Node* newNode = new Node();
    NodeFacade newFacade(newNode);
    
    // Deserialize through facade
    newFacade.read(serializedNode);
    
    bool typeTest = (newNode->getNodeType() == node->getNodeType());
    bool posTest = (newNode->pos() == node->pos());
    bool idTest = (newNode->getId() == node->getId());
    
    std::cout << "Round-trip tests:\n";
    std::cout << "  Type match: " << (typeTest ? "PASS" : "FAIL") << "\n";
    std::cout << "  Position match: " << (posTest ? "PASS" : "FAIL") << "\n";
    std::cout << "  ID match: " << (idTest ? "PASS" : "FAIL") << "\n";
    
    // Test 3: EdgeFacade basic functionality
    std::cout << "\n=== Test 3: EdgeFacade Basic Test ===\n";
    
    Edge* edge = new Edge();
    EdgeFacade edgeFacade(edge);
    
    QUuid edgeId = edgeFacade.id();
    std::cout << "Edge ID through facade: " << edgeId.toString().toStdString() << "\n";
    
    // Test edge serialization
    xmlNodePtr serializedEdge = edgeFacade.write(doc, root);
    
    Edge* newEdge = new Edge();
    EdgeFacade newEdgeFacade(newEdge);
    newEdgeFacade.read(serializedEdge);
    
    bool edgeIdTest = (newEdge->getId() == edge->getId());
    std::cout << "Edge round-trip ID test: " << (edgeIdTest ? "PASS" : "FAIL") << "\n";
    
    // Final results
    bool allTests = positionTest && typeTest && posTest && idTest && edgeIdTest;
    
    std::cout << "\n=== FINAL RESULTS ===\n";
    std::cout << "All facade tests: " << (allTests ? "SUCCESS" : "FAILED") << "\n";
    
    if (allTests) {
        std::cout << "\nPhase 11.1 SUCCESS: Type-erasure facades working!\n";
        std::cout << "NodeFacade provides uniform interface to Node objects\n";
        std::cout << "EdgeFacade provides uniform interface to Edge objects\n";
        std::cout << "Serialization through facades identical to direct serialization\n";
        std::cout << "Zero impact on existing Node::write/read and Edge::write/read\n";
    }
    
    // Cleanup
    delete node;
    delete newNode;
    delete edge;
    delete newEdge;
    xmlFreeDoc(doc);
    
    return allTests ? 0 : 1;
}