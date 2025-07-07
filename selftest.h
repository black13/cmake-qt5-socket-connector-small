#pragma once

#include <QApplication>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QString>
#include <QDebug>
#include <memory>

class Node;
class Edge;
class Socket;
class Scene;
class GraphFactory;

/**
 * SelfTest - Integrated headless testing framework for Phase 4
 * 
 * Core principles:
 * - Integrated testing system within main application
 * - Headless testing without separate Qt Test framework
 * - Ownership validation and performance testing
 * - Command-line driven testing with --test and --headless flags
 */
class SelfTest
{
public:
    static int runAll(QApplication* app = nullptr);
    static bool isHeadless() { return s_headless; }

private:
    static bool s_headless;
    static QApplication* s_app;
    static Scene* s_testScene;
    static GraphFactory* s_factory;
    static int s_testsPassed;
    static int s_testsFailed;
    static int s_testsTotal;
    
    // Test execution framework
    static void runTest(const QString& testName, bool (*testFunc)());
    static void assertTrue(bool condition, const QString& message);
    static void assertEqual(const QString& expected, const QString& actual, const QString& message);
    static void assertNotNull(void* ptr, const QString& message);
    
    // Core component tests
    static bool testNodeFactory();
    static bool testSocketFactory();
    static bool testEdgeFactory();
    static bool testXmlSerialization();
    static bool testSceneIntegration();
    
    // UI component tests
    static bool testNodeCreation();
    static bool testSocketConnections();
    static bool testEdgeRendering();
    static bool testSelectionHandling();
    static bool testDragAndDrop();
    
    // Performance tests
    static bool testLargeGraphPerformance();
    static bool testRapidCreateDelete();
    static bool testMemoryUsage();
    
    // Ownership tests
    static bool testFactoryOwnership();
    static bool testRegistryCleanup();
    static bool testSceneOwnership();
    static bool testEdgeOwnership();
    
    // Test helpers
    static void setupTestEnvironment();
    static void cleanupTestEnvironment();
    static Node* createTestNode(const QString& type = "SOURCE");
    static Edge* createTestEdge(Node* from, Node* to);
    static bool validateOwnership(Node* node);
    static bool validateSceneIntegrity();
    
    // Performance measurement
    static qint64 measureGraphCreationTime(int nodeCount);
    static qint64 measureEdgeCreationTime(int edgeCount);
    static void logPerformanceResults(const QString& testName, qint64 timeMs);
};