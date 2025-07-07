#pragma once

#include <QGraphicsView>
#include <QMouseEvent>
#include <QWheelEvent>

class Scene;

/**
 * View - Custom graphics view for node editor
 * 
 * Simple boilerplate extending QGraphicsView
 * Handles basic mouse and keyboard events
 */
class View : public QGraphicsView
{
public:
    explicit View(Scene* scene, QWidget* parent = nullptr);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    Scene* m_scene;
};