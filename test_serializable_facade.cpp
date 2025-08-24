#include <iostream>
#include <cassert>
#include <cstring>
#include "node.h"
#include "serializable_spec.h"
#include <libxml/tree.h>
#include <libxml/parser.h>

// Helper function to convert XML node to string for comparison
std::string xmlNodeToString(xmlNodePtr node) {
    xmlBufferPtr buffer = xmlBufferCreate();
    xmlNodeDump(buffer, node->doc, node, 0, 1);
    std::string result(reinterpret_cast<const char*>(xmlBufferContent(buffer)));
    xmlBufferFree(buffer);
    return result;
}

int main() {
    std::cout << "Testing SerializableFacade vs direct Node serialization...\n";
    
    // Create a test node
    Node* testNode = new Node();
    testNode->setNodeType("TestNode");
    testNode->setPos(QPointF(100.5, 200.7));
    
    // Create XML document
    xmlDocPtr doc = xmlNewDoc(BAD_CAST "1.0");
    xmlNodePtr root = xmlNewNode(nullptr, BAD_CAST "test");
    xmlDocSetRootElement(doc, root);
    
    // Test 1: Direct Node serialization
    xmlNodePtr directXml = testNode->write(doc, root);
    std::string directResult = xmlNodeToString(directXml);
    
    // Create new document for facade test
    xmlDocPtr doc2 = xmlNewDoc(BAD_CAST "1.0");
    xmlNodePtr root2 = xmlNewNode(nullptr, BAD_CAST "test");
    xmlDocSetRootElement(doc2, root2);
    
    // Test 2: Facade serialization
    SerializableFacade facade{*testNode};
    xmlNodePtr facadeXml = facade.write(doc2, root2);
    std::string facadeResult = xmlNodeToString(facadeXml);
    
    // Compare results
    std::cout << "Direct XML:  " << directResult << "\n";
    std::cout << "Facade XML:  " << facadeResult << "\n";
    
    bool identical = (directResult == facadeResult);
    std::cout << "Results identical: " << (identical ? "YES" : "NO") << "\n";
    
    // Test ID access through facade
    QUuid directId = testNode->getId();
    QUuid facadeId = facade.id();
    bool idsMatch = (directId == facadeId);
    std::cout << "IDs match: " << (idsMatch ? "YES" : "NO") << "\n";
    
    // Test round-trip: write then read back
    Node* roundTripNode = new Node();
    SerializableFacade roundTripFacade{*roundTripNode};
    roundTripFacade.read(facadeXml);
    
    bool positionMatch = (roundTripNode->pos() == testNode->pos());
    bool typeMatch = (roundTripNode->getNodeType() == testNode->getNodeType());
    
    std::cout << "Round-trip position match: " << (positionMatch ? "YES" : "NO") << "\n";
    std::cout << "Round-trip type match: " << (typeMatch ? "YES" : "NO") << "\n";
    
    // Cleanup
    delete testNode;
    delete roundTripNode;
    xmlFreeDoc(doc);
    xmlFreeDoc(doc2);
    
    // Summary
    bool allTestsPass = identical && idsMatch && positionMatch && typeMatch;
    std::cout << "\n=== SUMMARY ===\n";
    std::cout << "All tests pass: " << (allTestsPass ? "YES" : "NO") << "\n";
    
    return allTestsPass ? 0 : 1;
}