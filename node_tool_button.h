#pragma once

#include <QToolButton>
#include <QMouseEvent>
#include <QString>
#include <QIcon>
#include <QSize>

/**
 * NodeToolButton - Custom tool button for node palette
 * 
 * Represents a specific node type that can be created in the scene.
 * Provides consistent styling and click handling for the node palette.
 */
class NodeToolButton : public QToolButton
{
    Q_OBJECT

public:
    explicit NodeToolButton(const QString& type, const QIcon& icon, QWidget* parent = nullptr);
    
    QString nodeType() const { return m_nodeType; }

signals:
    void nodeClicked(const QString& nodeType);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void enterEvent(QEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    QString m_nodeType;
    
    void updateButtonStyle();
    void setHoverStyle(bool hovered);
};