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
 * @brief Draw the scene background (currently delegates to base class).
 */
void View::drawBackground(QPainter* painter, const QRectF& rect)
{
    // Simple grid background
    QGraphicsView::drawBackground(painter, rect);
}

/**
 * @brief Suppress the native context menu; scripted menu is Shift+Left only.
 */
void View::contextMenuEvent(QContextMenuEvent* event)
{
    Q_UNUSED(event);
    qDebug() << "View: Native context menu suppressed. Use Shift+Left click.";
}
