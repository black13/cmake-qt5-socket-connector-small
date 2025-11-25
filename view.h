#pragma once

#include <QGraphicsView>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>

class QEvent;

class Scene;
class Node;

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
    void contextMenuEvent(QContextMenuEvent* event) override;
    
    // Drag and drop support
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    
    // Grid visualization
    void drawBackground(QPainter* painter, const QRectF& rect) override;
    void drawForeground(QPainter* painter, const QRectF& rect) override;
    void leaveEvent(QEvent* event) override;

public:
    /**
     * @brief Enable/disable the background grid.
     */
    void setGridVisible(bool enabled);

    /**
     * @brief Enable/disable the snap preview crosshair.
     */
    void setSnapIndicatorVisible(bool enabled);

    /**
     * @brief Snap a scene position to the current grid spacing.
     */
    QPointF snapToGrid(const QPointF& scenePos) const;

signals:
    // Signal emitted when a node is dropped
    void nodeDropped(const QPointF& scenePos, const QString& nodeType, const QString& name, 
                    int inputSockets, int outputSockets);
    void contextMenuRequested(Node* node, const QPoint& screenPos, const QPointF& scenePos);

private:
    Scene* m_scene;
    bool m_rubberBandSelecting = false;
    bool m_rubberBandActive = false;
    int m_rubberBandMoveCounter = 0;
    QPoint m_rubberBandStartViewport;
    QPointF m_rubberBandStartScene;
    bool m_showGrid = true;
    qreal m_minorGridSpacing = 20.0;
    int m_majorLineInterval = 5;
    bool m_showSnapIndicator = true;
    QPointF m_lastMouseScenePos = QPointF();
    bool m_mouseInside = false;
};
