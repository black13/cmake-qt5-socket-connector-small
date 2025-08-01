#include <iostream>
#include "node.h"
#include "edge.h"
#include "graph_facades.h"
#include <libxml/tree.h>
#include <libxml/parser.h>

int main() {
    std::cout << "=== Simple Facade Test ===\n\n";
    
    // Test 1: NodeFacade basic functionality
    std::cout << "=== Testing NodeFacade ===\n";
    
    Node* node = new Node();
    node->setNodeType("TestNode");
    node->setPos(QPointF(123.45, 678.90));
    
    NodeFacade nodeFacade(node);
    
    std::cout << "Original Node:\n";
    std::cout << "  ID: " << node->getId().toString().toStdString() << "\n";
    std::cout << "  Type: " << node->getNodeType().toStdString() << "\n";
    std::cout << "  Position: (" << node->pos().x() << ", " << node->pos().y() << ")\n";
    
    std::cout << "\nThrough NodeFacade:\n";
    std::cout << "  ID: " << nodeFacade.id().toString().toStdString() << "\n";
    std::cout << "  Type: " << nodeFacade.nodeType().toStdString() << "\n";
    std::cout << "  Position: (" << nodeFacade.position().x() << ", " << nodeFacade.position().y() << ")\n";
    
    // Test facade modification
    nodeFacade.setPosition(QPointF(999.0, 888.0));
    std::cout << "\nAfter setPosition through facade:\n";
    std::cout << "  Real Node Position: (" << node->pos().x() << ", " << node->pos().y() << ")\n";
    
    // Test 2: NodeFacade serialization
    std::cout << "\n=== Testing NodeFacade Serialization ===\n";
    
    xmlDocPtr doc = xmlNewDoc(BAD_CAST "1.0");
    xmlNodePtr root = xmlNewNode(nullptr, BAD_CAST "test");
    xmlDocSetRootElement(doc, root);
    
    // Serialize through facade
    xmlNodePtr nodeXml = nodeFacade.write(doc, root);
    
    // Create new node and facade for round-trip
    Node* newNode = new Node();
    NodeFacade newFacade(newNode);
    
    // Read back through facade
    newFacade.read(nodeXml);
    
    std::cout << "Round-trip test:\n";
    std::cout << "  Original: " << node->getNodeType().toStdString() << " at (" 
              << node->pos().x() << ", " << node->pos().y() << ")\n";
    std::cout << "  Loaded:   " << newNode->getNodeType().toStdString() << " at (" 
              << newNode->pos().x() << ", " << newNode->pos().y() << ")\n";
    
    bool nodeSuccess = (newNode->getNodeType() == node->getNodeType() && 
                       newNode->pos() == node->pos());
    std::cout << "  Success: " << (nodeSuccess ? "✓" : "✗") << "\n";
    
    // Test 3: EdgeFacade basic functionality
    std::cout << "\n=== Testing EdgeFacade ===\n";
    
    Edge* edge = new Edge();
    EdgeFacade edgeFacade(edge);
    
    std::cout << "Edge ID through facade: " << edgeFacade.id().toString().toStdString() << "\n";
    
    // Test edge serialization
    xmlNodePtr edgeXml = edgeFacade.write(doc, root);
    
    Edge* newEdge = new Edge();
    EdgeFacade newEdgeFacade(newEdge);
    newEdgeFacade.read(edgeXml);
    
    bool edgeSuccess = (newEdge->getId() == edge->getId());
    std::cout << "Edge round-trip success: " << (edgeSuccess ? "✓" : "✗") << "\n";
    
    // Test 4: Load real XML file
    std::cout << "\n=== Testing with autosave.xml ===\n";
    
    xmlDocPtr autoDoc = xmlParseFile("autosave.xml");
    if (autoDoc) {
        xmlNodePtr autoRoot = xmlDocGetRootElement(autoDoc);
        if (autoRoot && !xmlStrcmp(autoRoot->name, BAD_CAST "node")) {
            Node* loadedNode = new Node();
            NodeFacade loadedFacade(loadedNode);
            
            loadedFacade.read(autoRoot);
            
            std::cout << "Loaded from autosave.xml:\n";
            std::cout << "  ID: " << loadedFacade.id().toString().toStdString() << "\n";
            std::cout << "  Type: " << loadedFacade.nodeType().toStdString() << "\n";
            std::cout << "  Position: (" << loadedFacade.position().x() << ", " << loadedFacade.position().y() << ")\n";
            
            delete loadedNode;
        }
        xmlFreeDoc(autoDoc);
    } else {
        std::cout << "Could not load autosave.xml (this is OK if file doesn't exist)\n";
    }
    
    // Cleanup
    delete node;
    delete newNode;
    delete edge;
    delete newEdge;
    xmlFreeDoc(doc);
    
    bool overallSuccess = nodeSuccess && edgeSuccess;
    std::cout << "\n=== FINAL RESULT ===\n";
    std::cout << "All facade tests: " << (overallSuccess ? "SUCCESS ✓" : "FAILED ✗") << "\n";
    
    return overallSuccess ? 0 : 1;
}