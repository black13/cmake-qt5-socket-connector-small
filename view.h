#pragma once

#include <QGraphicsView>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>

class Scene;

/**
 * View - Custom graphics view for node editor
 * 
 * Simple boilerplate extending QGraphicsView
 * Handles basic mouse and keyboard events
 */
class View : public QGraphicsView
{
    Q_OBJECT

public:
    explicit View(Scene* scene, QWidget* parent = nullptr);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    
    // Drag and drop support
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;

signals:
    // Signal emitted when a node is dropped
    void nodeDropped(const QString& nodeType, const QString& name, 
                    int inputSockets, int outputSockets, const QPointF& scenePos);

private:
    Scene* m_scene;
};