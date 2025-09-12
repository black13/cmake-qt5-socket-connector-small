#include "view.h"
#include "scene.h"
#include <QMouseEvent>
#include <QWheelEvent>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QPainter>
#include <QDebug>
#include <cmath>

View::View(Scene* scene, QWidget* parent)
    : QGraphicsView(scene, parent)
    , m_scene(scene)
{
    setRenderHint(QPainter::Antialiasing);
    setRenderHint(QPainter::SmoothPixmapTransform);
    setDragMode(QGraphicsView::NoDrag);  // Temporarily disable rubber band
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    
    // Disable any debug drawing of item bounding rectangles
    setRenderHint(QPainter::Qt4CompatiblePainting, false);
    
    // Additional optimizations to prevent bounding box artifacts
    setOptimizationFlags(QGraphicsView::DontSavePainterState | 
                        QGraphicsView::DontAdjustForAntialiasing);
    
    // Enable drag and drop
    setAcceptDrops(true);
}

void View::mousePressEvent(QMouseEvent* event)
{
    QGraphicsView::mousePressEvent(event);
}

void View::mouseMoveEvent(QMouseEvent* event)
{
    QGraphicsView::mouseMoveEvent(event);
}

void View::mouseReleaseEvent(QMouseEvent* event)
{
    QGraphicsView::mouseReleaseEvent(event);
}

void View::wheelEvent(QWheelEvent* event)
{
    // Simple zoom
    const qreal scaleFactor = 1.15;
    if (event->angleDelta().y() > 0) {
        scale(scaleFactor, scaleFactor);
    } else {
        scale(1.0 / scaleFactor, 1.0 / scaleFactor);
    }
}

void View::dragEnterEvent(QDragEnterEvent* event)
{
    qDebug() << "View: Drag enter event received";
    qDebug() << "View: Available mime formats:" << event->mimeData()->formats();
    
    // Check if the drag contains node template data
    if (event->mimeData()->hasFormat("application/x-node-template")) {
        QByteArray nodeData = event->mimeData()->data("application/x-node-template");
        QString nodeString = QString::fromUtf8(nodeData);
        qDebug() << "View: Node template data detected:" << nodeString;
        
        event->acceptProposedAction();
        qDebug() << "View: Drag enter accepted - node template detected";
    } else {
        event->ignore();
        qDebug() << "View: Drag enter ignored - no node template data";
    }
}

void View::dragMoveEvent(QDragMoveEvent* event)
{
    // Allow drag movement if it contains node template data
    if (event->mimeData()->hasFormat("application/x-node-template")) {
        event->acceptProposedAction();
        // Only log every 10th move event to avoid spam
        static int moveCount = 0;
        if (++moveCount % 10 == 0) {
            QPointF scenePos = mapToScene(event->pos());
            qDebug() << "View: Drag move accepted at scene position:" << scenePos;
        }
    } else {
        event->ignore();
        qDebug() << "View: Drag move ignored - no node template data";
    }  
}

void View::dropEvent(QDropEvent* event)
{
    qDebug() << "View: Drop event received";
    
    // Handle node template drop
    if (event->mimeData()->hasFormat("application/x-node-template")) {
        QByteArray nodeData = event->mimeData()->data("application/x-node-template");
        QString nodeString = QString::fromUtf8(nodeData);
        qDebug() << "View: Decoding drop data:" << nodeString;
        
        QStringList parts = nodeString.split("|");
        qDebug() << "View: Split into" << parts.size() << "parts:" << parts;
        
        if (parts.size() >= 5) {
            QString nodeType = parts[0];
            QString name = parts[1];
            QString description = parts[2];
            int inputSockets = parts[3].toInt();
            int outputSockets = parts[4].toInt();
            
            // Convert drop position to scene coordinates
            QPointF scenePos = mapToScene(event->pos());
            
            qDebug() << "View: Parsed node data:";
            qDebug() << "  - Type:" << nodeType;
            qDebug() << "  - Name:" << name;
            qDebug() << "  - Description:" << description;
            qDebug() << "  - Input sockets:" << inputSockets;
            qDebug() << "  - Output sockets:" << outputSockets;
            qDebug() << "  - Scene position:" << scenePos;
            
            qDebug() << "View: Emitting nodeDropped signal to Window";
            
            // Emit signal to notify the window
            emit nodeDropped(scenePos, nodeType, name, inputSockets, outputSockets);
            
            event->acceptProposedAction();
            qDebug() << "View: Drop event accepted and processed";
        } else {
            qWarning() << "View: Invalid node template data format - expected 5 parts, got" << parts.size();
            event->ignore();
        }
    } else {
        qDebug() << "View: Drop event ignored - no node template data";
        event->ignore();
    }
}

void View::drawBackground(QPainter* painter, const QRectF& rect)
{
    // Draw the base background
    QGraphicsView::drawBackground(painter, rect);
    
    // Only draw grid if scene supports it and grid is enabled
    if (!m_scene) return;
    
    // Check if grid should be visible (we'll add a toggle later)
    static bool gridVisible = true;
    if (!gridVisible) return;
    
    const int gridSize = m_scene->gridSize();
    if (gridSize <= 1) return;
    
    // Configure grid appearance
    QPen gridPen(QColor(200, 200, 200, 100)); // Light gray, semi-transparent
    gridPen.setWidth(1);
    painter->setPen(gridPen);
    
    // Calculate grid bounds based on visible rect
    const int left = static_cast<int>(std::floor(rect.left() / gridSize)) * gridSize;
    const int right = static_cast<int>(std::ceil(rect.right() / gridSize)) * gridSize;
    const int top = static_cast<int>(std::floor(rect.top() / gridSize)) * gridSize;
    const int bottom = static_cast<int>(std::ceil(rect.bottom() / gridSize)) * gridSize;
    
    // Draw vertical lines
    for (int x = left; x <= right; x += gridSize) {
        painter->drawLine(x, rect.top(), x, rect.bottom());
    }
    
    // Draw horizontal lines  
    for (int y = top; y <= bottom; y += gridSize) {
        painter->drawLine(rect.left(), y, rect.right(), y);
    }
    
    // Draw origin indicator (darker lines at 0,0)
    if (rect.contains(QPointF(0, 0))) {
        QPen originPen(QColor(150, 150, 150, 150));
        originPen.setWidth(2);
        painter->setPen(originPen);
        
        // Draw origin cross
        painter->drawLine(-20, 0, 20, 0);   // Horizontal origin line
        painter->drawLine(0, -20, 0, 20);   // Vertical origin line
    }
}