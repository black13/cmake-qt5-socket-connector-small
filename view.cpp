#include "view.h"
#include "scene.h"
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QPainter>
#include <QScrollBar>
#include <QDebug>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QDragMoveEvent>
#include <QMimeData>

View::View(Scene* scene, QWidget* parent)
    : QGraphicsView(scene, parent)
    , m_scene(scene)
    , m_zoomLevel(1.0)
    , m_middleClickPanning(false)
    , m_showGrid(true)
    , m_gridSize(50.0)
{
    setRenderHint(QPainter::Antialiasing);
    setRenderHint(QPainter::SmoothPixmapTransform);
    setDragMode(QGraphicsView::NoDrag);  // Allow custom drag handling
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    
    // Enable focus for keyboard events
    setFocusPolicy(Qt::StrongFocus);
    
    // Enable drag and drop
    setAcceptDrops(true);
    
    // Optimize for performance
    setOptimizationFlag(QGraphicsView::DontSavePainterState);
    setOptimizationFlag(QGraphicsView::DontAdjustForAntialiasing);
    
    qDebug() << "VIEW: Enhanced view initialized with grid and zoom controls";
}

void View::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::MiddleButton) {
        // Start middle-click panning
        m_middleClickPanning = true;
        m_lastPanPoint = event->pos();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }
    
    QGraphicsView::mousePressEvent(event);
}

void View::mouseMoveEvent(QMouseEvent* event)
{
    if (m_middleClickPanning) {
        // Handle middle-click panning
        QPoint delta = event->pos() - m_lastPanPoint;
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());
        m_lastPanPoint = event->pos();
        event->accept();
        return;
    }
    
    QGraphicsView::mouseMoveEvent(event);
}

void View::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::MiddleButton && m_middleClickPanning) {
        // End middle-click panning
        m_middleClickPanning = false;
        setCursor(Qt::ArrowCursor);
        event->accept();
        return;
    }
    
    QGraphicsView::mouseReleaseEvent(event);
}

void View::wheelEvent(QWheelEvent* event)
{
    // Enhanced zoom with center point
    qreal factor = (event->angleDelta().y() > 0) ? ZOOM_FACTOR : (1.0 / ZOOM_FACTOR);
    QPointF center = mapToScene(event->position().toPoint());
    updateZoom(factor, center);
    event->accept();
}

void View::keyPressEvent(QKeyEvent* event)
{
    switch (event->key()) {
        case Qt::Key_Plus:
        case Qt::Key_Equal:
            zoomIn();
            event->accept();
            return;
        case Qt::Key_Minus:
            zoomOut();
            event->accept();
            return;
        case Qt::Key_0:
            if (event->modifiers() & Qt::ControlModifier) {
                zoomReset();
                event->accept();
                return;
            }
            break;
        case Qt::Key_F:
            if (event->modifiers() & Qt::ControlModifier) {
                zoomToFit();
                event->accept();
                return;
            }
            break;
        case Qt::Key_Home:
            centerView();
            event->accept();
            return;
        case Qt::Key_G:
            m_showGrid = !m_showGrid;
            viewport()->update();
            qDebug() << "VIEW: Grid" << (m_showGrid ? "enabled" : "disabled");
            event->accept();
            return;
    }
    
    QGraphicsView::keyPressEvent(event);
}

// ============================================================================
// View Control Methods
// ============================================================================

void View::zoomIn()
{
    updateZoom(ZOOM_FACTOR);
}

void View::zoomOut()
{
    updateZoom(1.0 / ZOOM_FACTOR);
}

void View::zoomReset()
{
    setZoomLevel(1.0);
    qDebug() << "VIEW: Reset zoom to 100%";
}

void View::zoomToFit()
{
    if (m_scene) {
        QRectF itemsRect = m_scene->itemsBoundingRect();
        if (!itemsRect.isEmpty()) {
            fitInView(itemsRect, Qt::KeepAspectRatio);
            m_zoomLevel = transform().m11(); // Update zoom level
            emit zoomChanged(m_zoomLevel);
            qDebug() << "VIEW: Zoomed to fit items";
        }
    }
}

void View::centerView()
{
    if (m_scene) {
        QRectF itemsRect = m_scene->itemsBoundingRect();
        if (!itemsRect.isEmpty()) {
            centerOn(itemsRect.center());
            qDebug() << "VIEW: Centered on items";
        } else {
            centerOn(0, 0);
            qDebug() << "VIEW: Centered on origin";
        }
    }
}

void View::setZoomLevel(qreal zoom)
{
    zoom = qBound(MIN_ZOOM, zoom, MAX_ZOOM);
    if (qAbs(zoom - m_zoomLevel) > 0.01) {
        qreal factor = zoom / m_zoomLevel;
        m_zoomLevel = zoom;
        scale(factor, factor);
        emit zoomChanged(m_zoomLevel);
    }
}

void View::updateZoom(qreal factor, const QPointF& center)
{
    qreal newZoom = m_zoomLevel * factor;
    newZoom = qBound(MIN_ZOOM, newZoom, MAX_ZOOM);
    
    if (qAbs(newZoom - m_zoomLevel) > 0.01) {
        qreal actualFactor = newZoom / m_zoomLevel;
        m_zoomLevel = newZoom;
        
        if (!center.isNull()) {
            // Zoom towards specified center point
            QPointF viewCenter = mapFromScene(center);
            scale(actualFactor, actualFactor);
            QPointF newViewCenter = mapFromScene(center);
            QPointF delta = newViewCenter - viewCenter;
            horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
            verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());
        } else {
            scale(actualFactor, actualFactor);
        }
        
        emit zoomChanged(m_zoomLevel);
    }
}

void View::drawBackground(QPainter* painter, const QRectF& rect)
{
    // Draw default background
    QGraphicsView::drawBackground(painter, rect);
    
    // Draw grid if enabled
    if (m_showGrid) {
        drawGrid(painter, rect);
    }
}

void View::drawGrid(QPainter* painter, const QRectF& rect)
{
    painter->save();
    
    // Grid color based on zoom level
    QColor gridColor = QColor(128, 128, 128, 30 + (m_zoomLevel * 20));
    painter->setPen(QPen(gridColor, 1));
    
    // Calculate grid spacing based on zoom
    qreal gridSpacing = m_gridSize;
    while (gridSpacing * m_zoomLevel < 20) gridSpacing *= 2;
    while (gridSpacing * m_zoomLevel > 100) gridSpacing /= 2;
    
    // Draw vertical lines
    qreal left = int(rect.left() / gridSpacing) * gridSpacing;
    qreal right = rect.right();
    for (qreal x = left; x <= right; x += gridSpacing) {
        painter->drawLine(QLineF(x, rect.top(), x, rect.bottom()));
    }
    
    // Draw horizontal lines
    qreal top = int(rect.top() / gridSpacing) * gridSpacing;
    qreal bottom = rect.bottom();
    for (qreal y = top; y <= bottom; y += gridSpacing) {
        painter->drawLine(QLineF(rect.left(), y, rect.right(), y));
    }
    
    painter->restore();
}

void View::scrollContentsBy(int dx, int dy)
{
    QGraphicsView::scrollContentsBy(dx, dy);
    emit viewChanged(mapToScene(viewport()->rect()).boundingRect());
}

// ============================================================================
// Drag and Drop Support
// ============================================================================

void View::dragEnterEvent(QDragEnterEvent* event)
{
    // Check if the drag contains node type data
    if (event->mimeData()->hasFormat("application/x-nodetype") || 
        event->mimeData()->hasText()) {
        event->acceptProposedAction();
        qDebug() << "🎯 DRAG ENTER accepted - node type:" << event->mimeData()->text();
    } else {
        event->ignore();
    }
}

void View::dragMoveEvent(QDragMoveEvent* event)
{
    // Accept the drag move event if we can handle the data
    if (event->mimeData()->hasFormat("application/x-nodetype") || 
        event->mimeData()->hasText()) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void View::dropEvent(QDropEvent* event)
{
    // Get the node type from mime data
    QString nodeType;
    if (event->mimeData()->hasFormat("application/x-nodetype")) {
        nodeType = QString::fromUtf8(event->mimeData()->data("application/x-nodetype"));
    } else if (event->mimeData()->hasText()) {
        nodeType = event->mimeData()->text();
    }
    
    if (!nodeType.isEmpty()) {
        // Convert drop position to scene coordinates
        QPointF scenePos = mapToScene(event->pos());
        
        qDebug() << "🎯 ✅ NODE DROPPED:" << nodeType << "at scene position:" << scenePos;
        
        // Emit signal for the window to handle node creation
        emit nodeDropped(nodeType, scenePos);
        
        event->acceptProposedAction();
    } else {
        event->ignore();
        qDebug() << "✗ Drop ignored - no valid node type";
    }
}