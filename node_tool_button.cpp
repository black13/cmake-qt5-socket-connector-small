#include "node_tool_button.h"
#include <QDebug>
#include <QDrag>
#include <QMimeData>
#include <QApplication>

NodeToolButton::NodeToolButton(const QString& type, const QIcon& icon, QWidget* parent)
    : QToolButton(parent)
    , m_nodeType(type)
{
    setIcon(icon);
    setIconSize(QSize(24, 24));         // Classic size for clear visibility
    setFixedSize(QSize(32, 32));        // Border padding around icon
    setToolTip(QString("Create %1 node").arg(type));
    setCheckable(false);
    
    updateButtonStyle();
    
    qDebug() << "✓ NodeToolButton created for type:" << type;
}

void NodeToolButton::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragStartPosition = event->pos();
    }
    QToolButton::mousePressEvent(event);
}

void NodeToolButton::mouseMoveEvent(QMouseEvent* event)
{
    if (!(event->buttons() & Qt::LeftButton))
        return;

    if ((event->pos() - m_dragStartPosition).manhattanLength()
         < QApplication::startDragDistance())
        return;

    startDrag();
}

void NodeToolButton::enterEvent(QEvent* event)
{
    QToolButton::enterEvent(event);
    setHoverStyle(true);
}

void NodeToolButton::leaveEvent(QEvent* event)
{
    QToolButton::leaveEvent(event);
    setHoverStyle(false);
}

void NodeToolButton::updateButtonStyle()
{
    setStyleSheet(
        "QToolButton {"
        "  background-color: #f5f5f5;"
        "  border: 1px solid #cccccc;"
        "  border-radius: 3px;"
        "  padding: 2px;"
        "}"
        "QToolButton:hover {"
        "  background-color: #e0e0e0;"
        "  border: 1px solid #999999;"
        "}"
        "QToolButton:pressed {"
        "  background-color: #d0d0d0;"
        "  border: 1px solid #666666;"
        "}"
    );
}

void NodeToolButton::setHoverStyle(bool hovered)
{
    // Additional hover feedback could be added here
    // For now, CSS handles the visual feedback
    Q_UNUSED(hovered);
}

void NodeToolButton::startDrag()
{
    qDebug() << "🔥 STARTING DRAG for node type:" << m_nodeType;
    
    QDrag* drag = new QDrag(this);
    QMimeData* mimeData = new QMimeData;
    
    // Set the node type as mime data
    mimeData->setText(m_nodeType);
    mimeData->setData("application/x-nodetype", m_nodeType.toUtf8());
    drag->setMimeData(mimeData);
    
    // Use the button's icon as drag pixmap
    QPixmap dragPixmap = icon().pixmap(32, 32);
    if (dragPixmap.isNull()) {
        // Create a simple drag pixmap if no icon
        dragPixmap = QPixmap(32, 32);
        dragPixmap.fill(Qt::lightGray);
    }
    
    drag->setPixmap(dragPixmap);
    drag->setHotSpot(QPoint(16, 16));  // Center of the pixmap
    
    // Execute the drag operation
    Qt::DropAction dropAction = drag->exec(Qt::CopyAction);
    
    if (dropAction == Qt::CopyAction) {
        qDebug() << "✓ Node drag completed successfully";
    } else {
        qDebug() << "✗ Node drag cancelled or failed";
    }
}