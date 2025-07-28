#pragma once

#include <QObject>
#include <QtTest>
#include <QApplication>
#include <QElapsedTimer>
#include <libxml/tree.h>
#include "scene.h"
#include "graph_factory.h"
#include "node.h"
#include "edge.h"
#include "node_registry.h"

/**
 * tst_Main - Main Qt Test suite that grows with the application
 * 
 * Single comprehensive test suite for all application functionality.
 * Will expand as we add more features to the NodeGraph application.
 */
class tst_Main : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Core functionality tests
    void testCreateNode();
    void testFactoryNodeCreation();
    void testXmlLoadSave();
    void testCompleteWorkflow();
    
    // XML Performance test - data-driven
    void testXmlPerformance();
    
    // XML Dynamic updates
    void testNodePositionToXml();
    void testEdgeModificationToXml();

private:
    // Helper methods
    Node* createNode(const QString& type = "OUT");
    bool setupEnvironment();
    void cleanupEnvironment();
    bool validateSceneSetup();
    
    // Performance test helpers
    void performXmlLoadTest(const QString& filename, const QString& testName);
    qint64 measureXmlLoadTime(const QString& filename);
    void validateLoadedGraph(int expectedNodes, int expectedEdges);
    
    // Test infrastructure
    QApplication* m_app;
    Scene* m_testScene;
    GraphFactory* m_factory;
    xmlDocPtr m_xmlDoc;
};