#pragma once

#include "graph_observer.h"
#include <QString>
#include <QTimer>

class Scene;

/**
 * XmlAutosaveObserver - Automatically saves graph changes to XML
 * 
 * Listens to all graph mutations and maintains an up-to-date XML file.
 * Uses delayed saving to avoid excessive I/O during rapid changes.
 */
class XmlAutosaveObserver : public GraphObserver
{
public:
    explicit XmlAutosaveObserver(Scene* scene, const QString& filename = "autosave.xml");
    ~XmlAutosaveObserver();
    
    // Configure autosave behavior
    void setFilename(const QString& filename);
    void setDelay(int milliseconds);  // Delay before saving after last change
    void setEnabled(bool enabled);
    
    // Force immediate save
    void saveNow();
    
    // GraphObserver interface
    void onNodeAdded(const Node& node) override;
    void onNodeRemoved(const QUuid& nodeId) override;
    void onNodeMoved(const QUuid& nodeId, QPointF oldPos, QPointF newPos) override;
    void onEdgeAdded(const Edge& edge) override;
    void onEdgeRemoved(const QUuid& edgeId) override;
    void onGraphCleared() override;
    
private:
    Scene* m_scene;
    QString m_filename;
    QTimer* m_saveTimer;
    bool m_enabled;
    bool m_pendingChanges;
    
    void scheduleAutosave();
    void performAutosave();
    void logAutosaveState();
    QString generateFullXml() const;
};