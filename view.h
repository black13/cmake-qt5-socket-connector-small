#pragma once

#include <QGraphicsView>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QScrollBar>

class Scene;

/**
 * View - Enhanced graphics view for node editor
 * 
 * Features:
 * - Smooth zoom with limits
 * - Middle-click pan
 * - Keyboard navigation
 * - View reset and fit controls
 * - Grid background
 */
class View : public QGraphicsView
{
    Q_OBJECT

public:
    explicit View(Scene* scene, QWidget* parent = nullptr);
    
    // View control methods
    void zoomIn();
    void zoomOut();
    void zoomReset();
    void zoomToFit();
    void centerView();
    
    // Zoom level management
    qreal getZoomLevel() const { return m_zoomLevel; }
    void setZoomLevel(qreal zoom);

signals:
    void zoomChanged(qreal zoomLevel);
    void viewChanged(const QRectF& viewRect);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void drawBackground(QPainter* painter, const QRectF& rect) override;
    void scrollContentsBy(int dx, int dy) override;

private:
    Scene* m_scene;
    
    // Zoom management
    qreal m_zoomLevel;
    static constexpr qreal MIN_ZOOM = 0.1;
    static constexpr qreal MAX_ZOOM = 5.0;
    static constexpr qreal ZOOM_FACTOR = 1.2;
    
    // Pan management
    bool m_middleClickPanning;
    QPoint m_lastPanPoint;
    
    // Grid settings
    bool m_showGrid;
    qreal m_gridSize;
    
    void updateZoom(qreal factor, const QPointF& center = QPointF());
    void drawGrid(QPainter* painter, const QRectF& rect);
};