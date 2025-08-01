#include <iostream>
#include <vector>
#include "node.h"
#include "edge.h"
#include "graph_facades.h"
#include <libxml/tree.h>
#include <libxml/parser.h>

// Helper to print facade info
void printNodeFacade(const NodeFacade& facade, const std::string& label) {
    std::cout << label << ":\n";
    std::cout << "  ID: " << facade.id().toString().toStdString() << "\n";
    std::cout << "  Type: " << facade.nodeType().toStdString() << "\n";
    std::cout << "  Position: (" << facade.position().x() << ", " << facade.position().y() << ")\n";
}

void printEdgeFacade(const EdgeFacade& facade, const std::string& label) {
    std::cout << label << ":\n";
    std::cout << "  ID: " << facade.id().toString().toStdString() << "\n";
    std::cout << "  From: " << facade.sourceNodeId().toString().toStdString() << "[" << facade.sourceSocketIndex() << "]\n";
    std::cout << "  To: " << facade.targetNodeId().toString().toStdString() << "[" << facade.targetSocketIndex() << "]\n";
}

int main() {
    std::cout << "=== Testing Facades with Real XML Files ===\n\n";
    
    // Test 1: Load existing autosave.xml through facades
    std::cout << "=== Test 1: Loading autosave.xml ===\n";
    
    xmlDocPtr doc = xmlParseFile("autosave.xml");
    if (!doc) {
        std::cout << "Failed to parse autosave.xml\n";
        return 1;
    }
    
    xmlNodePtr root = xmlDocGetRootElement(doc);
    if (root && !xmlStrcmp(root->name, BAD_CAST "node")) {
        // Create Node and wrap in facade
        Node* node = new Node();
        NodeFacade facade(node);
        
        // Load through facade
        facade.read(root);
        printNodeFacade(facade, "Loaded Node from autosave.xml");
        
        delete node;
    }
    xmlFreeDoc(doc);
    
    std::cout << "\n=== Test 2: Round-trip Test with Facades ===\n";
    
    // Create test nodes and edges
    Node* node1 = new Node();
    node1->setNodeType("SOURCE");
    node1->setPos(QPointF(100.0, 200.0));
    
    Node* node2 = new Node();
    node2->setNodeType("TRANSFORM");
    node2->setPos(QPointF(300.0, 400.0));
    
    Edge* edge = new Edge();
    edge->setFromNode(node1->getId(), 0);
    edge->setToNode(node2->getId(), 1);
    
    // Wrap in facades
    NodeFacade facade1(node1);
    NodeFacade facade2(node2);
    EdgeFacade edgeFacade(edge);
    
    std::cout << "Original objects:\n";
    printNodeFacade(facade1, "Node1");
    printNodeFacade(facade2, "Node2"); 
    printEdgeFacade(edgeFacade, "Edge");
    
    // Serialize to XML through facades
    xmlDocPtr testDoc = xmlNewDoc(BAD_CAST "1.0");
    xmlNodePtr testRoot = xmlNewNode(nullptr, BAD_CAST "graph");
    xmlDocSetRootElement(testDoc, testRoot);
    
    xmlNodePtr node1Xml = facade1.write(testDoc, testRoot);
    xmlNodePtr node2Xml = facade2.write(testDoc, testRoot);
    xmlNodePtr edgeXml = edgeFacade.write(testDoc, testRoot);
    
    // Save to file
    xmlSaveFile("test_facades_output.xml", testDoc);
    std::cout << "\nSaved complete graph to test_facades_output.xml\n";
    
    // Create new objects and load back through facades
    Node* newNode1 = new Node();
    Node* newNode2 = new Node();
    Edge* newEdge = new Edge();
    
    NodeFacade newFacade1(newNode1);
    NodeFacade newFacade2(newNode2);
    EdgeFacade newEdgeFacade(newEdge);
    
    newFacade1.read(node1Xml);
    newFacade2.read(node2Xml);
    newEdgeFacade.read(edgeXml);
    
    std::cout << "\nLoaded back objects:\n";
    printNodeFacade(newFacade1, "New Node1");
    printNodeFacade(newFacade2, "New Node2");
    printEdgeFacade(newEdgeFacade, "New Edge");
    
    // Verify round-trip
    bool node1Match = (newNode1->getNodeType() == node1->getNodeType() && 
                      newNode1->pos() == node1->pos());
    bool node2Match = (newNode2->getNodeType() == node2->getNodeType() && 
                      newNode2->pos() == node2->pos());
    bool edgeMatch = (newEdge->fromNodeId() == edge->fromNodeId() &&
                     newEdge->toNodeId() == edge->toNodeId() &&
                     newEdge->fromSocketIndex() == edge->fromSocketIndex() &&
                     newEdge->toSocketIndex() == edge->toSocketIndex());
    
    std::cout << "\n=== Round-trip Verification ===\n";
    std::cout << "Node1 match: " << (node1Match ? "✓" : "✗") << "\n";
    std::cout << "Node2 match: " << (node2Match ? "✓" : "✗") << "\n";
    std::cout << "Edge match: " << (edgeMatch ? "✓" : "✗") << "\n";
    
    // Test 3: Polymorphic containers
    std::cout << "\n=== Test 3: Polymorphic Storage ===\n";
    
    std::vector<NodeFacade> nodes;
    nodes.emplace_back(node1);
    nodes.emplace_back(node2);
    
    std::cout << "Stored " << nodes.size() << " nodes polymorphically:\n";
    for (size_t i = 0; i < nodes.size(); ++i) {
        std::cout << "  [" << i << "] " << nodes[i].nodeType().toStdString() 
                  << " at (" << nodes[i].position().x() << ", " << nodes[i].position().y() << ")\n";
    }
    
    // Cleanup
    delete node1;
    delete node2;
    delete edge;
    delete newNode1;
    delete newNode2;
    delete newEdge;
    xmlFreeDoc(testDoc);
    
    bool allSuccess = node1Match && node2Match && edgeMatch;
    std::cout << "\n=== FINAL RESULT ===\n";
    std::cout << "All facade tests with XML: " << (allSuccess ? "SUCCESS ✓" : "FAILED ✗") << "\n";
    
    return allSuccess ? 0 : 1;
}