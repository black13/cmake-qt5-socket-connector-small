#pragma once

#include <QObject>
#include <QtTest>
#include <QTemporaryDir>
#include <QFileInfo>
#include "scene.h"
#include "graph_factory.h"
#include "node_registry.h"

/**
 * tst_RoundTrip - Save → Load → Save comparison tests
 * 
 * Validates that save/load cycles preserve graph integrity.
 * Critical for verifying our Simple Fix ownership solution.
 */
class tst_RoundTrip : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Core round-trip tests
    void saveLoadSave_identical();
    void saveLoadSave_emptyGraph();
    void saveLoadSave_singleNode();
    void saveLoadSave_simpleConnection();
    void saveLoadSave_complexGraph();
    
    // Edge case tests
    void saveLoadSave_disconnectedNodes();
    void saveLoadSave_multipleEdges();
    void saveLoadSave_variableSocketCounts();

private:
    // Helper methods
    void makeSampleGraph(Scene* scene, GraphFactory* factory);
    bool sameXml(const QString& file1, const QString& file2);
    bool saveGraph(Scene* scene, const QString& filename);
    bool loadGraph(Scene* scene, GraphFactory* factory, const QString& filename);
    
    // Test infrastructure
    QTemporaryDir* m_tempDir;
    xmlDocPtr m_xmlDoc;
};