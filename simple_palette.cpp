#include "simple_palette.h"
#include <QApplication>
#include <QDebug>

SimplePalette::SimplePalette(QWidget* parent)
    : QWidget(parent)
    , m_layout(nullptr)
    , m_sourceButton(nullptr)
    , m_sinkButton(nullptr)
    , m_oneToTwoButton(nullptr)
    , m_twoToOneButton(nullptr)
{
    setupUI();
    qDebug() << "SimplePalette: Created with 4 node type buttons";
}

void SimplePalette::setupUI()
{
    setFixedWidth(120);
    setMinimumHeight(200);
    
    m_layout = new QVBoxLayout(this);
    m_layout->setSpacing(8);
    m_layout->setContentsMargins(8, 8, 8, 8);
    
    // Create 4 node type buttons
    m_sourceButton = createNodeButton("Source", "Source (0→1)");
    m_sinkButton = createNodeButton("Sink", "Sink (1→0)");
    m_oneToTwoButton = createNodeButton("1-to-2", "1-to-2 (1→2)");
    m_twoToOneButton = createNodeButton("2-to-1", "2-to-1 (2→1)");
    
    // Add buttons to layout
    m_layout->addWidget(m_sourceButton);
    m_layout->addWidget(m_sinkButton);
    m_layout->addWidget(m_oneToTwoButton);
    m_layout->addWidget(m_twoToOneButton);
    m_layout->addStretch(); // Push buttons to top
    
    setLayout(m_layout);
}

QPushButton* SimplePalette::createNodeButton(const QString& nodeType, const QString& description)
{
    QPushButton* button = new QPushButton(description, this);
    button->setMinimumHeight(40);
    button->setToolTip(QString("Drag to create %1 node").arg(nodeType));
    
    // Style the button
    button->setStyleSheet(
        "QPushButton {"
        "  background-color: #f0f0f0;"
        "  border: 2px solid #cccccc;"
        "  border-radius: 4px;"
        "  padding: 8px;"
        "  text-align: center;"
        "}"
        "QPushButton:hover {"
        "  background-color: #e0e0e0;"
        "  border: 2px solid #999999;"
        "}"
        "QPushButton:pressed {"
        "  background-color: #d0d0d0;"
        "  border: 2px solid #666666;"
        "}"
    );
    
    // Override mouse events for drag-and-drop
    button->installEventFilter(this);
    
    return button;
}

bool SimplePalette::eventFilter(QObject* obj, QEvent* event)
{
    QPushButton* button = qobject_cast<QPushButton*>(obj);
    if (!button) {
        return QWidget::eventFilter(obj, event);
    }
    
    if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            m_dragStartPosition = mouseEvent->pos();
        }
    }
    else if (event->type() == QEvent::MouseMove) {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
        if (!(mouseEvent->buttons() & Qt::LeftButton)) {
            return QWidget::eventFilter(obj, event);
        }
        
        if ((mouseEvent->pos() - m_dragStartPosition).manhattanLength() 
            < QApplication::startDragDistance()) {
            return QWidget::eventFilter(obj, event);
        }
        
        // Determine node type from button
        QString nodeType;
        if (button == m_sourceButton) nodeType = "Source";
        else if (button == m_sinkButton) nodeType = "Sink";
        else if (button == m_oneToTwoButton) nodeType = "1-to-2";
        else if (button == m_twoToOneButton) nodeType = "2-to-1";
        
        if (!nodeType.isEmpty()) {
            startDrag(nodeType, button);
        }
        
        return true; // Event handled
    }
    
    return QWidget::eventFilter(obj, event);
}

void SimplePalette::startDrag(const QString& nodeType, QPushButton* button)
{
    qDebug() << "SimplePalette: Starting drag for node type:" << nodeType;
    
    QDrag* drag = new QDrag(this);
    QMimeData* mimeData = new QMimeData;
    
    // Set the node type as mime data (same format as existing system)
    mimeData->setText(nodeType);
    mimeData->setData("application/x-nodetype", nodeType.toUtf8());
    drag->setMimeData(mimeData);
    
    // Create a simple drag pixmap from button text
    QPixmap dragPixmap(button->size());
    dragPixmap.fill(Qt::lightGray);
    button->render(&dragPixmap);
    drag->setPixmap(dragPixmap);
    drag->setHotSpot(QPoint(dragPixmap.width() / 2, dragPixmap.height() / 2));
    
    // Execute the drag operation
    Qt::DropAction dropAction = drag->exec(Qt::CopyAction);
    
    if (dropAction == Qt::CopyAction) {
        qDebug() << "SimplePalette: Drag completed successfully for" << nodeType;
    } else {
        qDebug() << "SimplePalette: Drag cancelled for" << nodeType;
    }
}