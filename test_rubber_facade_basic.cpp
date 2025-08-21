/**
 * Basic Test for RubberNodeFacade - Week 1 Validation
 * 
 * Goal: Prove wrapper concept works without touching existing system
 * 
 * Test Plan:
 * 1. Create Node instance (existing proven system)
 * 2. Wrap with RubberNodeFacade
 * 3. Verify all delegation methods work
 * 4. Test action storage (no execution yet)
 * 5. Validate safety features (null checks)
 */

#include "rubber_node_facade.h"
#include "node.h"
#include <QCoreApplication>
#include <QDebug>
#include <QUuid>
#include <cassert>

void testBasicDelegation()
{
    qDebug() << "\n=== Testing Basic Delegation ===";
    
    // Create node using existing system
    QUuid testId = QUuid::createUuid();
    QPointF testPos(100.0, 200.0);
    Node* node = new Node(testId, testPos);
    node->setNodeType("TRANSFORM");
    
    // Wrap with facade
    RubberNodeFacade facade(*node);
    
    // Test delegation methods
    assert(facade.getId() == testId);
    assert(facade.getType() == "TRANSFORM");
    assert(facade.getPosition() == testPos);
    assert(facade.getNode() == node);
    assert(facade.isValid() == true);
    
    qDebug() << "Basic delegation: All assertions passed";
    qDebug() << "   Facade toString():" << facade.toString();
    
    delete node;
}

void testPositionModification()
{
    qDebug() << "\n=== Testing Position Modification ===";
    
    Node* node = new Node();
    node->setNodeType("SOURCE");
    
    RubberNodeFacade facade(*node);
    
    // Test position changes
    QPointF newPos(300.0, 400.0);
    facade.setPosition(newPos);
    
    assert(facade.getPosition() == newPos);
    assert(node->pos() == newPos);  // Verify delegation worked
    
    qDebug() << "Position modification: Delegation working correctly";
    
    delete node;
}

void testActionStorage()
{
    qDebug() << "\n=== Testing Action Storage (Week 1) ===";
    
    Node* node = new Node();
    node->setNodeType("MERGE");
    
    RubberNodeFacade facade(*node);
    
    // Test action registration
    assert(!facade.hasAction("testAction"));
    assert(facade.getActions().size() == 0);
    
    facade.registerAction("testAction", "console.log('Hello from merge node');");
    
    assert(facade.hasAction("testAction"));
    assert(facade.getActions().size() == 1);
    assert(facade.getActions()["testAction"] == "console.log('Hello from merge node');");
    
    // Test multiple actions
    facade.registerAction("secondAction", "return 42;");
    assert(facade.getActions().size() == 2);
    
    // Test action removal
    facade.removeAction("testAction");
    assert(!facade.hasAction("testAction"));
    assert(facade.hasAction("secondAction"));
    assert(facade.getActions().size() == 1);
    
    qDebug() << "Action storage: Working correctly (Week 2 will add execution)";
    
    delete node;
}

void testFacadeEquality()
{
    qDebug() << "\n=== Testing Facade Equality ===";
    
    Node* node1 = new Node();
    Node* node2 = new Node();
    
    RubberNodeFacade facade1(*node1);
    RubberNodeFacade facade2(*node1);  // Same node
    RubberNodeFacade facade3(*node2);  // Different node
    
    assert(facade1 == facade2);  // Same wrapped node
    assert(facade1 != facade3);  // Different wrapped nodes
    assert(facade2 != facade3);  // Different wrapped nodes
    
    qDebug() << "Facade equality: Working correctly";
    
    delete node1;
    delete node2;
}

void testSafetyFeatures()
{
    qDebug() << "\n=== Testing Safety Features ===";
    
    Node* node = new Node();
    RubberNodeFacade facade(*node);
    
    // Test valid facade
    assert(facade.isValid());
    
    // Test copy constructor safety
    RubberNodeFacade copy(facade);
    assert(copy.isValid());
    assert(copy == facade);
    
    qDebug() << "Safety features: Copy constructor and validation working";
    
    delete node;
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    
    qDebug() << "RubberNodeFacade Basic Test - Week 1 Validation";
    qDebug() << "==============================================";
    
    try {
        testBasicDelegation();
        testPositionModification();
        testActionStorage();
        testFacadeEquality();
        testSafetyFeatures();
        
        qDebug() << "\nALL TESTS PASSED - Week 1 Success Criteria Met!";
        qDebug() << "\nWeek 1 Achievements:";
        qDebug() << "Minimal wrapper around existing Node*";
        qDebug() << "Zero impact on existing Node class";
        qDebug() << "All methods delegate to proven existing code";
        qDebug() << "Placeholder action system (storage only)";
        qDebug() << "Safety features (validation, copy constructor)";
        qDebug() << "Reference-based wrapper (doesn't own Node*)";
        
        qDebug() << "\n➡️  Ready for Week 2: JavaScript Bridge Integration";
        
    } catch (const std::exception& e) {
        qDebug() << "TEST FAILED:" << e.what();
        return 1;
    }
    
    return 0;
}