#include <iostream>
#include <vector>
#include "node.h"
#include "edge.h"
#include "graph_facades.h"
#include <libxml/tree.h>

// Helper to print XML node content
void printXmlNode(xmlNodePtr node, const std::string& label) {
    xmlBufferPtr buffer = xmlBufferCreate();
    xmlNodeDump(buffer, node->doc, node, 0, 1);
    std::cout << label << ": " << reinterpret_cast<const char*>(xmlBufferContent(buffer)) << "\n";
    xmlBufferFree(buffer);
}

int main() {
    std::cout << "=== Testing NodeFacade and EdgeFacade ===\n\n";
    
    // Create real objects
    Node* node1 = new Node();
    node1->setNodeType("SourceNode");
    node1->setPos(QPointF(100.0, 200.0));
    
    Node* node2 = new Node();
    node2->setNodeType("TargetNode"); 
    node2->setPos(QPointF(300.0, 400.0));
    
    Edge* edge = new Edge();
    edge->setFromNode(node1->getId(), 0);  // From node1, socket 0
    edge->setToNode(node2->getId(), 1);    // To node2, socket 1
    
    std::cout << "Created real objects:\n";
    std::cout << "- Node1 ID: " << node1->getId().toString().toStdString() << "\n";
    std::cout << "- Node2 ID: " << node2->getId().toString().toStdString() << "\n";
    std::cout << "- Edge ID: " << edge->getId().toString().toStdString() << "\n\n";
    
    // Wrap in facades
    NodeFacade facade1 = makeNodeFacade(node1);
    NodeFacade facade2 = makeNodeFacade(node2);
    EdgeFacade edgeFacade = makeEdgeFacade(edge);
    
    std::cout << "=== Testing NodeFacade Interface ===\n";
    std::cout << "Facade1 ID: " << facade1.id().toString().toStdString() << "\n";
    std::cout << "Facade1 Type: " << facade1.nodeType().toStdString() << "\n";
    std::cout << "Facade1 Position: (" << facade1.position().x() << ", " << facade1.position().y() << ")\n";
    
    // Test position change through facade
    facade1.setPosition(QPointF(150.0, 250.0));
    std::cout << "After setPosition through facade: (" << node1->pos().x() << ", " << node1->pos().y() << ")\n\n";
    
    std::cout << "=== Testing EdgeFacade Interface ===\n";
    std::cout << "Edge ID: " << edgeFacade.id().toString().toStdString() << "\n";
    std::cout << "Source Node: " << edgeFacade.sourceNodeId().toString().toStdString() << "\n";
    std::cout << "Target Node: " << edgeFacade.targetNodeId().toString().toStdString() << "\n";
    std::cout << "Source Socket: " << edgeFacade.sourceSocketIndex() << "\n";
    std::cout << "Target Socket: " << edgeFacade.targetSocketIndex() << "\n\n";
    
    std::cout << "=== Testing Serialization Through Facades ===\n";
    
    // Create XML document
    xmlDocPtr doc = xmlNewDoc(BAD_CAST "1.0");
    xmlNodePtr root = xmlNewNode(nullptr, BAD_CAST "graph");
    xmlDocSetRootElement(doc, root);
    
    // Serialize through facades (should be identical to direct serialization)
    xmlNodePtr node1Xml = facade1.write(doc, root);
    xmlNodePtr node2Xml = facade2.write(doc, root);
    xmlNodePtr edgeXml = edgeFacade.write(doc, root);
    
    printXmlNode(node1Xml, "Node1 XML");
    printXmlNode(node2Xml, "Node2 XML");
    printXmlNode(edgeXml, "Edge XML");
    
    std::cout << "\n=== Testing Round-trip Through Facades ===\n";
    
    // Create new objects and facades for round-trip test
    Node* newNode1 = new Node();
    Node* newNode2 = new Node();
    Edge* newEdge = new Edge();
    
    NodeFacade newFacade1 = makeNodeFacade(newNode1);
    NodeFacade newFacade2 = makeNodeFacade(newNode2);
    EdgeFacade newEdgeFacade = makeEdgeFacade(newEdge);
    
    // Read back through facades
    newFacade1.read(node1Xml);
    newFacade2.read(node2Xml);
    newEdgeFacade.read(edgeXml);
    
    // Verify round-trip success
    bool node1Success = (newNode1->getNodeType() == node1->getNodeType() && 
                        newNode1->pos() == node1->pos());
    bool node2Success = (newNode2->getNodeType() == node2->getNodeType() && 
                        newNode2->pos() == node2->pos());
    bool edgeSuccess = (newEdge->fromNodeId() == edge->fromNodeId() &&
                       newEdge->toNodeId() == edge->toNodeId() &&
                       newEdge->fromSocketIndex() == edge->fromSocketIndex() &&
                       newEdge->toSocketIndex() == edge->toSocketIndex());
    
    std::cout << "Node1 round-trip: " << (node1Success ? "SUCCESS" : "FAILED") << "\n";
    std::cout << "Node2 round-trip: " << (node2Success ? "SUCCESS" : "FAILED") << "\n";
    std::cout << "Edge round-trip: " << (edgeSuccess ? "SUCCESS" : "FAILED") << "\n";
    
    // Test polymorphic storage
    std::cout << "\n=== Testing Polymorphic Storage ===\n";
    std::vector<NodeFacade> nodes;
    nodes.push_back(makeNodeFacade(node1));
    nodes.push_back(makeNodeFacade(node2));
    
    std::cout << "Stored " << nodes.size() << " nodes in vector:\n";
    for (size_t i = 0; i < nodes.size(); ++i) {
        std::cout << "- Node " << i << ": " << nodes[i].nodeType().toStdString() 
                  << " at (" << nodes[i].position().x() << ", " << nodes[i].position().y() << ")\n";
    }
    
    // Cleanup
    delete node1;
    delete node2;
    delete edge;
    delete newNode1;
    delete newNode2;
    delete newEdge;
    xmlFreeDoc(doc);
    
    bool overallSuccess = node1Success && node2Success && edgeSuccess;
    std::cout << "\n=== FINAL RESULT ===\n";
    std::cout << "All facade tests: " << (overallSuccess ? "SUCCESS" : "FAILED") << "\n";
    
    return overallSuccess ? 0 : 1;
}