#pragma once

#include <QObject>
#include <QPointF>
#include <QString>
#include <QVariantMap>
#include <QVariantList>

class QGraphicsScene;
class Socket;
class Scene;

/**
 * QGraph - Central graph data model and orchestration layer
 *
 * Separates graph semantics from visual rendering (Scene).
 * Handles all graph mutations, XML I/O, and business logic.
 *
 * Design principles:
 * - QGraph owns graph semantics (create, connect, delete)
 * - Scene handles only visual rendering and registries
 * - O(1) lookups via UUID-based maps
 * - O(degree) edge updates (only notify connected nodes)
 *
 * Note: Q_INVOKABLE methods use QString for UUIDs (not QUuid) because:
 * - JavaScript/QML doesn't have a native QUuid type
 * - QString converts seamlessly to/from JavaScript strings
 * - Internally we still use QUuid for type safety
 */
class QGraph : public QObject
{
    Q_OBJECT

public:
    explicit QGraph(QGraphicsScene* scene, QObject* parent = nullptr);
    ~QGraph() = default;

    // Node operations
    Q_INVOKABLE QString createNode(const QString& type, qreal x, qreal y);
    Q_INVOKABLE bool deleteNode(const QString& nodeId);
    Q_INVOKABLE bool moveNode(const QString& nodeId, qreal dx, qreal dy);
    Q_INVOKABLE QVariantMap getNode(const QString& nodeId);
    Q_INVOKABLE QVariantList getNodes();

    // Edge operations
    Q_INVOKABLE QString connect(const QString& fromNodeId, int fromIdx,
                                const QString& toNodeId, int toIdx);
    Q_INVOKABLE bool deleteEdge(const QString& edgeId);
    Q_INVOKABLE QVariantList getEdges();

    // Graph-wide operations
    Q_INVOKABLE void clear();
    Q_INVOKABLE void saveXml(const QString& path);
    Q_INVOKABLE void loadXml(const QString& path);
    Q_INVOKABLE QString getXmlString();
    Q_INVOKABLE QVariantMap getStats();

    // Ghost-edge preview orchestration (Scene renders, QGraph orchestrates)
    Q_INVOKABLE void beginPreview(Socket* from, const QPointF& start);
    Q_INVOKABLE void updatePreview(const QPointF& pos);
    Q_INVOKABLE void endPreview(Socket* to);

    // Utility functions
    Q_INVOKABLE bool isValidNodeType(const QString& type);
    Q_INVOKABLE QStringList getValidNodeTypes();

signals:
    void nodeCreated(QString id);
    void nodeDeleted(QString id);
    void edgeConnected(QString id);
    void edgeDeleted(QString id);
    void graphCleared();
    void xmlSaved(QString path);
    void xmlLoaded(QString path);
    void error(QString message);

private:
    Scene* scene_;  // Access to visual layer and registries

    // Helper methods
    class Node* findNode(const QString& uuid);
    class Edge* findEdge(const QString& uuid);
    QVariantMap nodeToVariant(Node* node);
    QVariantMap edgeToVariant(Edge* edge);
};