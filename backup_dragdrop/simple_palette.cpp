#include "simple_palette.h"
#include <QLabel>
#include <QDebug>
#include <QApplication>

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
    setWindowTitle("Node Palette");
    setFixedWidth(150);
    
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(10, 10, 10, 10);
    m_layout->setSpacing(5);
    
    // Title
    QLabel* title = new QLabel("Node Types");
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet("font-weight: bold; padding: 5px;");
    m_layout->addWidget(title);
    
    // Create buttons for each node type
    m_sourceButton = createNodeButton("Source", "Source (0→1)");
    m_sinkButton = createNodeButton("Sink", "Sink (1→0)");
    m_oneToTwoButton = createNodeButton("1-to-2", "1-to-2 (1→2)");
    m_twoToOneButton = createNodeButton("2-to-1", "2-to-1 (2→1)");
    
    m_layout->addWidget(m_sourceButton);
    m_layout->addWidget(m_sinkButton);
    m_layout->addWidget(m_oneToTwoButton);
    m_layout->addWidget(m_twoToOneButton);
    
    // Add stretch to push buttons to top
    m_layout->addStretch();
    
    qDebug() << "SimplePalette: UI setup complete";
}

QPushButton* SimplePalette::createNodeButton(const QString& nodeType, const QString& description)
{
    QPushButton* button = new QPushButton(description);
    button->setProperty("nodeType", nodeType);
    button->setMinimumHeight(30);
    button->setStyleSheet(
        "QPushButton {"
        "    background-color: #f0f0f0;"
        "    border: 1px solid #ccc;"
        "    padding: 5px;"
        "    text-align: left;"
        "}"
        "QPushButton:hover {"
        "    background-color: #e0e0e0;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #d0d0d0;"
        "}"
    );
    
    // Install event filter for drag detection
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
        
        if ((mouseEvent->pos() - m_dragStartPosition).manhattanLength() < QApplication::startDragDistance()) {
            return QWidget::eventFilter(obj, event);
        }
        
        // Start drag operation
        QString nodeType = button->property("nodeType").toString();
        startDrag(nodeType, button);
        return true;
    }
    
    return QWidget::eventFilter(obj, event);
}

void SimplePalette::startDrag(const QString& nodeType, QPushButton* button)
{
    qDebug() << "SimplePalette: Starting drag for node type:" << nodeType;
    
    QDrag* drag = new QDrag(this);
    QMimeData* mimeData = new QMimeData;
    
    // Set the node type as both text and custom format
    mimeData->setText(nodeType);
    mimeData->setData("application/x-nodetype", nodeType.toUtf8());
    
    drag->setMimeData(mimeData);
    
    // Create a simple drag pixmap from the button
    QPixmap pixmap(button->size());
    button->render(&pixmap);
    drag->setPixmap(pixmap);
    drag->setHotSpot(QPoint(pixmap.width() / 2, pixmap.height() / 2));
    
    qDebug() << "SimplePalette: Executing drag operation...";
    
    // Execute the drag
    Qt::DropAction dropAction = drag->exec(Qt::CopyAction);
    
    if (dropAction == Qt::CopyAction) {
        qDebug() << "SimplePalette: Drag completed successfully for" << nodeType;
    } else {
        qDebug() << "SimplePalette: Drag was not completed for" << nodeType << "- result:" << dropAction;
    }
}