#include "node_palette_bar.h"
#include <QLabel>
#include <QFrame>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QPixmap>
#include <QPainter>
#include <QFont>
#include <QDebug>

NodePaletteBar::NodePaletteBar(QWidget* parent)
    : QWidget(parent)
    , m_mainLayout(nullptr)
    , m_contentWidget(nullptr)
    , m_scrollArea(nullptr)
{
    setupUI();
    addBasicNodes();
    addMathNodes();
    addIONodes();
    
    qDebug() << "✓ NodePaletteBar initialized";
}

void NodePaletteBar::setupUI()
{
    setFixedWidth(120);  // Wider to accommodate category labels
    setMinimumHeight(300);
    
    // Create scroll area for node palette
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    
    m_contentWidget = new QWidget();
    m_mainLayout = new QVBoxLayout(m_contentWidget);
    m_mainLayout->setSpacing(8);
    m_mainLayout->setContentsMargins(4, 4, 4, 4);
    
    m_scrollArea->setWidget(m_contentWidget);
    
    // Main layout for the entire widget
    QVBoxLayout* outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->addWidget(m_scrollArea);
    
    setLayout(outerLayout);
}

void NodePaletteBar::addBasicNodes()
{
    // Not used - we only have the 4 specific node types
}

void NodePaletteBar::addMathNodes()
{
    // Not used - we only have the 4 specific node types  
}

void NodePaletteBar::addIONodes()
{
    QWidget* nodeSection = createCategorySection("Node Types");
    QGridLayout* nodeLayout = new QGridLayout(nodeSection);
    nodeLayout->setSpacing(2);
    nodeLayout->setContentsMargins(4, 4, 4, 4);
    
    // The 4 required node types
    addToolToLayout(nodeLayout, "Source", createSocketIcon(0, 1, QColor(100, 255, 100)), 0, 0);
    addToolToLayout(nodeLayout, "Sink", createSocketIcon(1, 0, QColor(255, 100, 100)), 0, 1);
    addToolToLayout(nodeLayout, "1-to-2", createSocketIcon(1, 2, QColor(100, 100, 255)), 1, 0);
    addToolToLayout(nodeLayout, "2-to-1", createSocketIcon(2, 1, QColor(255, 255, 100)), 1, 1);
    
    m_mainLayout->addWidget(nodeSection);
}

QWidget* NodePaletteBar::createCategorySection(const QString& title)
{
    QWidget* section = new QWidget();
    
    // Create title label
    QLabel* titleLabel = new QLabel(title);
    titleLabel->setStyleSheet(
        "QLabel {"
        "  font-weight: bold;"
        "  color: #333333;"
        "  background-color: #e8e8e8;"
        "  padding: 2px 4px;"
        "  border: 1px solid #cccccc;"
        "  border-radius: 2px;"
        "}"
    );
    titleLabel->setAlignment(Qt::AlignCenter);
    
    // Create container for the section content
    QVBoxLayout* sectionLayout = new QVBoxLayout(section);
    sectionLayout->setSpacing(2);
    sectionLayout->setContentsMargins(0, 0, 0, 0);
    sectionLayout->addWidget(titleLabel);
    
    return section;
}

void NodePaletteBar::addToolToLayout(QGridLayout* layout, const QString& name, const QIcon& icon, int row, int col)
{
    NodeToolButton* btn = new NodeToolButton(name, icon, this);
    connect(btn, &NodeToolButton::nodeClicked, this, &NodePaletteBar::onNodeClicked);
    layout->addWidget(btn, row, col);
}

void NodePaletteBar::onNodeClicked(const QString& nodeType)
{
    qDebug() << "NodePaletteBar: Node selected:" << nodeType;
    emit nodeSelected(nodeType);
}

void NodePaletteBar::addNodeType(const QString& category, const QString& nodeType, const QIcon& icon)
{
    // For now, add to the main layout - could be enhanced to find category section
    QWidget* customSection = createCategorySection(category);
    QGridLayout* customLayout = new QGridLayout(customSection);
    customLayout->setSpacing(2);
    
    addToolToLayout(customLayout, nodeType, icon, 0, 0);
    m_mainLayout->addWidget(customSection);
    
    qDebug() << "Added custom node type:" << nodeType << "to category:" << category;
}

void NodePaletteBar::addSeparator()
{
    QFrame* separator = new QFrame();
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Sunken);
    separator->setStyleSheet("QFrame { color: #cccccc; }");
    m_mainLayout->addWidget(separator);
}

QIcon NodePaletteBar::createNodeIcon(const QString& nodeType)
{
    // Create a simple node-like icon
    QPixmap pixmap(24, 24);
    pixmap.fill(Qt::transparent);
    
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Draw a rounded rectangle representing a node
    painter.setBrush(QColor(220, 220, 240));
    painter.setPen(QPen(QColor(100, 100, 120), 1));
    painter.drawRoundedRect(2, 2, 20, 20, 3, 3);
    
    // Draw connection points (sockets)
    painter.setBrush(QColor(100, 100, 120));
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(0, 10, 4, 4);  // Left socket
    painter.drawEllipse(20, 10, 4, 4); // Right socket
    
    return QIcon(pixmap);
}

QIcon NodePaletteBar::createTextIcon(const QString& text, const QColor& bgColor)
{
    QPixmap pixmap(24, 24);
    pixmap.fill(Qt::transparent);
    
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);
    
    // Draw background
    painter.setBrush(bgColor);
    painter.setPen(QPen(QColor(80, 80, 80), 1));
    painter.drawRoundedRect(1, 1, 22, 22, 3, 3);
    
    // Draw text
    painter.setPen(QColor(40, 40, 40));
    QFont font = painter.font();
    font.setPointSize(8);
    font.setBold(true);
    painter.setFont(font);
    
    painter.drawText(pixmap.rect(), Qt::AlignCenter, text);
    
    return QIcon(pixmap);
}

QIcon NodePaletteBar::createSocketIcon(int inputs, int outputs, const QColor& bgColor)
{
    QPixmap pixmap(32, 24);
    pixmap.fill(Qt::transparent);
    
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Draw main node body
    painter.setBrush(bgColor);
    painter.setPen(QPen(QColor(80, 80, 80), 1));
    painter.drawRoundedRect(6, 4, 20, 16, 2, 2);
    
    // Draw input sockets on the left
    painter.setBrush(QColor(60, 60, 60));
    painter.setPen(Qt::NoPen);
    for (int i = 0; i < inputs; ++i) {
        int y = 8 + (i * 8) - (inputs - 1) * 4;  // Center vertically
        painter.drawEllipse(2, y, 4, 4);
    }
    
    // Draw output sockets on the right
    for (int i = 0; i < outputs; ++i) {
        int y = 8 + (i * 8) - (outputs - 1) * 4;  // Center vertically
        painter.drawEllipse(26, y, 4, 4);
    }
    
    return QIcon(pixmap);
}