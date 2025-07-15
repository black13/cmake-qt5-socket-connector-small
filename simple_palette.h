#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QMouseEvent>
#include <QDrag>
#include <QMimeData>

/**
 * SimplePalette - Basic 4-button node palette with drag-and-drop
 * 
 * Simple widget with 4 buttons for the node types:
 * - Source (0→1)
 * - Sink (1→0) 
 * - 1-to-2 (1→2)
 * - 2-to-1 (2→1)
 * 
 * Supports drag-and-drop to existing View drop handler.
 */
class SimplePalette : public QWidget
{
    Q_OBJECT

public:
    explicit SimplePalette(QWidget* parent = nullptr);

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    void setupUI();
    QPushButton* createNodeButton(const QString& nodeType, const QString& description);
    
    // Drag-and-drop support
    void startDrag(const QString& nodeType, QPushButton* button);

private:
    QVBoxLayout* m_layout;
    QPushButton* m_sourceButton;
    QPushButton* m_sinkButton; 
    QPushButton* m_oneToTwoButton;
    QPushButton* m_twoToOneButton;
    
    QPoint m_dragStartPosition;
};