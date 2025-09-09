#include "xml_autosave_observer.h"
#include "scene.h"
#include "node.h"
#include "edge.h"
#include "graph_observer.h"
#include <QTimer>
#include <QDebug>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QFile>
#include <QTextStream>
#include <libxml/tree.h>
#include <libxml/parser.h>

XmlAutosaveObserver::XmlAutosaveObserver(Scene* scene, const QString& filename)
    : m_scene(scene)
    , m_filename(filename)
    , m_saveTimer(new QTimer())
    , m_enabled(true)
    , m_pendingChanges(false)
{
    // Configure timer for delayed saving
    m_saveTimer->setSingleShot(true);
    m_saveTimer->setInterval(2000); // 2 second delay by default
    
    // Connect timer to autosave
    QObject::connect(m_saveTimer, &QTimer::timeout, [this]() {
        performAutosave();
    });
    
}

XmlAutosaveObserver::~XmlAutosaveObserver()
{
    try {
        if (m_pendingChanges && m_enabled) {
            saveNow(); // Save any pending changes before destruction
        }
    } catch (const std::exception& e) {
        qWarning() << "XmlAutosaveObserver: Failed to save during cleanup:" << e.what();
        qWarning() << "Pending changes may be lost but application will continue safely";
    } catch (...) {
        qWarning() << "XmlAutosaveObserver: Unknown error during cleanup - data may be lost";
        qWarning() << "Application continues to prevent crash";
    }
    
    delete m_saveTimer;
}

void XmlAutosaveObserver::setFilename(const QString& filename)
{
    m_filename = filename;
    qDebug() << "XmlAutosaveObserver: Filename changed to" << m_filename;
}

void XmlAutosaveObserver::setDelay(int milliseconds)
{
    m_saveTimer->setInterval(milliseconds);
    logAutosaveState();
}

void XmlAutosaveObserver::setEnabled(bool enabled)
{
    m_enabled = enabled;
    if (!enabled) {
        m_saveTimer->stop();
    }
    logAutosaveState();
}

void XmlAutosaveObserver::saveNow()
{
    m_saveTimer->stop();
    performAutosave();
}

void XmlAutosaveObserver::onNodeAdded(const Node& node)
{
    // Node added - minimal logging
    scheduleAutosave();
}

void XmlAutosaveObserver::onNodeRemoved(const QUuid& nodeId)
{
    qDebug() << "OBSERVER: Node removed" << nodeId.toString(QUuid::WithoutBraces).left(8) << "- Triggering autosave";
    scheduleAutosave();
}

void XmlAutosaveObserver::onNodeMoved(const QUuid& nodeId, QPointF oldPos, QPointF newPos)
{
    Q_UNUSED(oldPos)
    Q_UNUSED(newPos)
    qDebug().noquote() << "[AUTOSAVE] Node moved:" << nodeId.toString(QUuid::WithoutBraces).left(8);
    scheduleAutosave();
}

void XmlAutosaveObserver::onEdgeAdded(const Edge& edge)
{
    // Edge added - minimal logging
    scheduleAutosave();
}

void XmlAutosaveObserver::onEdgeRemoved(const QUuid& edgeId)
{
    qDebug() << "OBSERVER: Edge removed" << edgeId.toString(QUuid::WithoutBraces).left(8) << "- Triggering autosave";
    scheduleAutosave();
}

void XmlAutosaveObserver::onGraphCleared()
{
    qDebug() << "OBSERVER: Graph cleared - Triggering autosave";
    scheduleAutosave();
}

void XmlAutosaveObserver::scheduleAutosave()
{
    if (!m_enabled) {
        return;
    }
    
    // OPTIMIZATION: Skip autosave scheduling during batch operations
    if (GraphSubject::isInBatch()) {
        qDebug().noquote() << "[AUTOSAVE] Skipping during batch mode";
        return;
    }
    
    m_pendingChanges = true;
    qDebug().noquote() << "[AUTOSAVE] markDirty() called. Timer started:" 
                       << m_saveTimer->interval() << "ms";
    m_saveTimer->start(); // Restart timer - delays save until activity stops
}

void XmlAutosaveObserver::performAutosave()
{
    qDebug().noquote() << "[AUTOSAVE] flushIfDirty() called. Enabled:" << m_enabled 
                       << "Pending:" << m_pendingChanges << "Scene:" << (m_scene ? "valid" : "NULL");
    
    if (!m_enabled || !m_pendingChanges || !m_scene) {
        return;
    }
    
    qDebug() << "XmlAutosaveObserver: Performing autosave to" << m_filename;
    
    QElapsedTimer timer;
    timer.start();
    
    // Create full XML serialization of the current graph
    QString xmlContent = generateFullXml();
    
    // Write to file (simplified)
    qDebug().noquote() << "[AUTOSAVE] writeAutosave() attempting to write to:" << m_filename;
    QFile file(m_filename);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << xmlContent;
        file.close();
        
        qint64 elapsed = timer.elapsed();
        QFileInfo fileInfo(m_filename);
        qint64 fileSize = fileInfo.size();
        
        // Get scene stats for logging
        int nodeCount = m_scene->getNodes().size();
        int edgeCount = m_scene->getEdges().size();
        
        // Unified autosave success log as suggested by ChatGPT analysis
        qDebug() << "Autosave: wrote" << fileInfo.fileName() 
                 << "(" << QString::number(fileSize / 1024.0, 'f', 1) << "KB)"
                 << "in" << elapsed << "ms"
                 << "(nodes=" << nodeCount << "edges=" << edgeCount << ")";
        
        m_pendingChanges = false;
    } else {
        qDebug().noquote() << "[AUTOSAVE] writeAutosave() FAILED! Cannot open file for writing.";
        qWarning() << "XmlAutosaveObserver: Failed to save" << m_filename;
    }
}

QString XmlAutosaveObserver::generateFullXml() const
{
    if (!m_scene) {
        return "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<graph version=\"1.0\"/>\n";
    }
    
    // Create XML document
    xmlDocPtr doc = xmlNewDoc(BAD_CAST "1.0");
    xmlNodePtr root = xmlNewNode(nullptr, BAD_CAST "graph");
    xmlDocSetRootElement(doc, root);
    xmlSetProp(root, BAD_CAST "version", BAD_CAST "1.0");
    
    // Add nodes section
    xmlNodePtr nodesNode = xmlNewChild(root, nullptr, BAD_CAST "nodes", nullptr);
    
    // CRASH VALIDATION: Report collection state before serialization
    qDebug() << __FUNCTION__ << "- AUTOSAVE VALIDATION: About to serialize" << m_scene->getNodes().size() << "nodes from hash map";
    qDebug() << __FUNCTION__ << "- AUTOSAVE VALIDATION: Qt scene currently has" << m_scene->items().size() << "total items";
    
    int validNodes = 0, invalidNodes = 0;
    for (auto it = m_scene->getNodes().begin(); it != m_scene->getNodes().end(); ++it) {
        Node* node = it.value();
        if (node) {
            // Validate node is still valid before serialization
            // SAFE VALIDATION: Only access node if it's still in the scene
            bool nodeValid = false;
            for (QGraphicsItem* item : m_scene->items()) {
                if (item == node) {
                    nodeValid = true;
                    break;
                }
            }
            
            if (nodeValid) {
                validNodes++;
                try {
                    xmlNodePtr nodeXml = node->write(doc, nullptr);
                    if (nodeXml) {
                        xmlAddChild(nodesNode, nodeXml);
                    }
                } catch (...) {
                    qDebug() << __FUNCTION__ << "- AUTOSAVE: Exception during node serialization - skipping";
                }
            } else {
                invalidNodes++;
                qWarning() << __FUNCTION__ << "- AUTOSAVE CRASH AVERTED: Skipping stale node pointer - object deleted but hash map not updated";
            }
        }
    }
    
    // CRASH VALIDATION: Report serialization results
    qDebug() << __FUNCTION__ << "- AUTOSAVE VALIDATION RESULTS: Valid nodes:" << validNodes << "Invalid nodes:" << invalidNodes;
    if (invalidNodes > 0) {
        qWarning() << __FUNCTION__ << "- CRASH CAUSE IDENTIFIED:" << invalidNodes << "stale pointers in Scene::m_nodes hash map";
        qWarning() << __FUNCTION__ << "- ROOT CAUSE: QGraphicsScene::clear() deletes objects but Scene doesn't clean hash maps";
        qWarning() << __FUNCTION__ << "- SOLUTION: Implement proper cleanup in Scene::clear() override or object destructors";
    }
    
    // Add edges section
    xmlNodePtr edgesNode = xmlNewChild(root, nullptr, BAD_CAST "connections", nullptr);
    for (auto it = m_scene->getEdges().begin(); it != m_scene->getEdges().end(); ++it) {
        Edge* edge = it.value();
        if (edge) {
            // Validate edge is still valid before serialization
            try {
                // Check if edge is still in the scene's items AND validate memory access
                bool edgeValid = false;
                for (QGraphicsItem* item : m_scene->items()) {
                    if (item == edge) {
                        // Additional validation - try to access edge properties safely
                        try {
                            // Test memory access with lightweight operation
                            QUuid testId = edge->getId();
                            if (!testId.isNull()) {
                                edgeValid = true;
                            }
                        } catch (...) {
                            qDebug() << __FUNCTION__ << "- AUTOSAVE: Edge pointer invalid - memory access failed";
                        }
                        break;
                    }
                }
                
                if (edgeValid) {
                    xmlNodePtr edgeXml = edge->write(doc, nullptr);
                    if (edgeXml) {
                        xmlAddChild(edgesNode, edgeXml);
                    }
                } else {
                    qDebug() << __FUNCTION__ << "- AUTOSAVE: Skipping invalid/stale edge pointer during serialization";
                }
            } catch (...) {
                qDebug() << __FUNCTION__ << "- AUTOSAVE: Exception caught during edge validation - skipping";
            }
        }
    }
    
    // Convert to string
    xmlChar* xmlBuffer;
    int bufferSize;
    xmlDocDumpFormatMemory(doc, &xmlBuffer, &bufferSize, 1);
    
    QString result = QString::fromUtf8(reinterpret_cast<const char*>(xmlBuffer));
    
    // Clean up
    xmlFree(xmlBuffer);
    xmlFreeDoc(doc);
    
    return result;
}

void XmlAutosaveObserver::logAutosaveState()
{
    if (m_enabled) {
        qDebug() << "Autosave: enabled (period=" << m_saveTimer->interval() << "ms)";
    } else {
        qDebug() << "Autosave: disabled (interval=" << m_saveTimer->interval() << "ms)";
    }
}
