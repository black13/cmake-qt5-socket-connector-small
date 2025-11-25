#include "view.h"
#include "scene.h"
#include "node.h"
#include <QMouseEvent>
#include <QWheelEvent>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QPainter>
#include <QDebug>
#include <QApplication>
#include <QContextMenuEvent>
#include <QtMath>
#include <cmath>

/**
 * @brief Construct the custom graphics view used by the editor.
 *
 * Enables antialiasing, drag/drop, and rubber-band selection so the Scene can
 * focus strictly on node/edge logic. All per-view visual tweaks live here.
 */
View::View(Scene* scene, QWidget* parent)
    : QGraphicsView(scene, parent)
    , m_scene(scene)
{
    setRenderHint(QPainter::Antialiasing);
    setRenderHint(QPainter::SmoothPixmapTransform);
    setDragMode(QGraphicsView::RubberBandDrag);
    setRubberBandSelectionMode(Qt::IntersectsItemShape);
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    
    // Disable any debug drawing of item bounding rectangles
    setRenderHint(QPainter::Qt4CompatiblePainting, false);
    
    // Additional optimizations to prevent bounding box artifacts
    setOptimizationFlags(QGraphicsView::DontSavePainterState | 
                        QGraphicsView::DontAdjustForAntialiasing);
    
    // Enable drag and drop
    setAcceptDrops(true);
}

/**
 * @brief Handle mouse press. Shift+Left spawns scripted-node menu, otherwise pass to base view.
 */
void View::mousePressEvent(QMouseEvent* event)
{
    if ((event->modifiers() & Qt::ShiftModifier) && event->button() == Qt::LeftButton) {
        QPointF scenePos = mapToScene(event->pos());
        QGraphicsItem* graphicsItem = itemAt(event->pos());
        Node* node = dynamic_cast<Node*>(graphicsItem);
        qDebug() << "View: Shift+Left context trigger at" << event->pos()
                 << "scene" << scenePos
                 << "node" << (node ? node->getNodeType() : "<none>");
        emit contextMenuRequested(node, event->globalPos(), scenePos);
        event->accept();
        return;
    }

    if (event->button() == Qt::LeftButton && dragMode() == QGraphicsView::RubberBandDrag) {
        m_rubberBandSelecting = true;
        m_rubberBandActive = false;
        m_rubberBandMoveCounter = 0;
        m_rubberBandStartViewport = event->pos();
        m_rubberBandStartScene = mapToScene(event->pos());
    }
    QGraphicsView::mousePressEvent(event);
}

/**
 * @brief Track rubber-band selection progress for debug logging.
 */
void View::mouseMoveEvent(QMouseEvent* event)
{
    if (m_rubberBandSelecting && !m_rubberBandActive) {
        if ((event->pos() - m_rubberBandStartViewport).manhattanLength() >= QApplication::startDragDistance()) {
            m_rubberBandActive = true;
            qDebug() << "View: Rubber band selection started at" << m_rubberBandStartScene;
        }
    } else if (m_rubberBandActive) {
        if (++m_rubberBandMoveCounter % 15 == 0) {
            QPointF current = mapToScene(event->pos());
            qDebug() << "View: Rubber band update, current scene pos" << current;
        }
    }
    QGraphicsView::mouseMoveEvent(event);

    m_lastMouseScenePos = mapToScene(event->pos());
    m_mouseInside = true;
    viewport()->update();
}

/**
 * @brief Reset rubber-band flags when the mouse is released.
 */
void View::mouseReleaseEvent(QMouseEvent* event)
{
    QGraphicsView::mouseReleaseEvent(event);
    if (m_rubberBandSelecting) {
        m_rubberBandActive = false;
        m_rubberBandMoveCounter = 0;
        m_rubberBandSelecting = false;
    }
}

/**
 * @brief Simple zoom handler using the mouse wheel.
 */
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

/**
 * @brief Accept drag operations that carry node template data.
 */
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

/**
 * @brief Allow drag moves for node templates, logging every few events.
 */
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

/**
 * @brief Decode dropped node template data and emit nodeDropped().
 */
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

/**
 * @brief Draw the scene background with a minor/major grid.
 */
void View::drawBackground(QPainter* painter, const QRectF& rect)
{
    QGraphicsView::drawBackground(painter, rect);

    if (!m_showGrid || m_minorGridSpacing <= 0.0) {
        return;
    }

    const qreal spacing = m_minorGridSpacing;
    const int majorInterval = qMax(1, m_majorLineInterval);

    const int firstX = static_cast<int>(std::floor(rect.left() / spacing));
    const int lastX = static_cast<int>(std::ceil(rect.right() / spacing));
    const int firstY = static_cast<int>(std::floor(rect.top() / spacing));
    const int lastY = static_cast<int>(std::ceil(rect.bottom() / spacing));

    const QPen minorPen(QColor(255, 255, 255, 20));
    const QPen majorPen(QColor(255, 255, 255, 60));
    const QPen axisPen(QColor(82, 156, 255, 180));

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, false);

    auto penForCoordinate = [&](qreal coordinate, bool isMajor) -> QPen {
        if (qFuzzyIsNull(coordinate)) {
            return axisPen;
        }
        return isMajor ? majorPen : minorPen;
    };

    for (int i = firstX; i <= lastX; ++i) {
        const qreal x = static_cast<qreal>(i) * spacing;
        const bool major = (i % majorInterval) == 0;
        painter->setPen(penForCoordinate(x, major));
        painter->drawLine(QPointF(x, rect.top()), QPointF(x, rect.bottom()));
    }

    for (int j = firstY; j <= lastY; ++j) {
        const qreal y = static_cast<qreal>(j) * spacing;
        const bool major = (j % majorInterval) == 0;
        painter->setPen(penForCoordinate(y, major));
        painter->drawLine(QPointF(rect.left(), y), QPointF(rect.right(), y));
    }

    painter->restore();
}

/**
 * @brief Suppress the native context menu; scripted menu is Shift+Left only.
 */
void View::contextMenuEvent(QContextMenuEvent* event)
{
    Q_UNUSED(event);
    qDebug() << "View: Native context menu suppressed. Use Shift+Left click.";
}

/**
 * @brief Draw the snap preview crosshair.
 */
void View::drawForeground(QPainter* painter, const QRectF& rect)
{
    QGraphicsView::drawForeground(painter, rect);

    Q_UNUSED(rect);
    if (!m_showSnapIndicator || !m_mouseInside || m_minorGridSpacing <= 0.0) {
        return;
    }

    const QPointF snapPoint = snapToGrid(m_lastMouseScenePos);
    const qreal spacing = m_minorGridSpacing;
    const qreal arm = qMax<qreal>(6.0, spacing * 0.35);

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, false);
    painter->setPen(QPen(QColor(82, 156, 255, 200), 0));
    painter->drawLine(QPointF(snapPoint.x() - arm, snapPoint.y()),
                      QPointF(snapPoint.x() + arm, snapPoint.y()));
    painter->drawLine(QPointF(snapPoint.x(), snapPoint.y() - arm),
                      QPointF(snapPoint.x(), snapPoint.y() + arm));
    painter->restore();
}

/**
 * @brief Hide the snap indicator when the cursor leaves the view.
 */
void View::leaveEvent(QEvent* event)
{
    m_mouseInside = false;
    viewport()->update();
    QGraphicsView::leaveEvent(event);
}

void View::setGridVisible(bool enabled)
{
    if (m_showGrid == enabled) {
        return;
    }
    m_showGrid = enabled;
    viewport()->update();
}

void View::setSnapIndicatorVisible(bool enabled)
{
    if (m_showSnapIndicator == enabled) {
        return;
    }
    m_showSnapIndicator = enabled;
    viewport()->update();
}

QPointF View::snapToGrid(const QPointF& scenePos) const
{
    if (m_minorGridSpacing <= 0.0) {
        return scenePos;
    }

    const qreal snappedX = std::round(scenePos.x() / m_minorGridSpacing) * m_minorGridSpacing;
    const qreal snappedY = std::round(scenePos.y() / m_minorGridSpacing) * m_minorGridSpacing;
    return QPointF(snappedX, snappedY);
}
