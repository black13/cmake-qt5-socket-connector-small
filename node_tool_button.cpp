#include "node_tool_button.h"
#include <QDebug>

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
    QToolButton::mousePressEvent(event);
    
    qDebug() << "NodeToolButton clicked:" << m_nodeType;
    emit nodeClicked(m_nodeType);
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