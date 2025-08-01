#include <iostream>
#include "node.h"
#include "graph_facades.h"
#include <libxml/tree.h>

int main() {
    std::cout << "Testing our custom NodeFacade...\n";
    
    // Create a real Node
    Node* realNode = new Node();
    realNode->setNodeType("TestNode");
    realNode->setPos(QPointF(42.0, 84.0));
    
    // Wrap it in our facade  
    NodeFacade facade(realNode);
    
    // Test: can we call methods through the facade?
    QUuid nodeId = facade.id();
    std::cout << "Node ID through facade: " << nodeId.toString().toStdString() << "\n";
    
    // Test: serialization through facade
    xmlDocPtr doc = xmlNewDoc(BAD_CAST "1.0");
    xmlNodePtr root = xmlNewNode(nullptr, BAD_CAST "test");
    xmlDocSetRootElement(doc, root);
    
    xmlNodePtr serialized = facade.write(doc, root);
    
    // Test: read back through facade
    Node* newNode = new Node();
    NodeFacade newFacade(newNode);
    newFacade.read(serialized);
    
    // Verify round-trip worked
    bool success = (newNode->getNodeType() == realNode->getNodeType() && 
                   newNode->pos() == realNode->pos());
    
    std::cout << "Round-trip success: " << (success ? "YES" : "NO") << "\n";
    std::cout << "Original pos: (" << realNode->pos().x() << ", " << realNode->pos().y() << ")\n";
    std::cout << "New pos: (" << newNode->pos().x() << ", " << newNode->pos().y() << ")\n";
    
    // Cleanup
    delete realNode;
    delete newNode;
    xmlFreeDoc(doc);
    
    std::cout << "\n=== Our custom type-erasure works! ===\n";
    return success ? 0 : 1;
}