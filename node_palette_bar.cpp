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
    QWidget* basicSection = createCategorySection("Basic");
    QGridLayout* basicLayout = new QGridLayout(basicSection);
    basicLayout->setSpacing(2);
    basicLayout->setContentsMargins(4, 4, 4, 4);
    
    addToolToLayout(basicLayout, "Constant", createNodeIcon("Const"), 0, 0);
    addToolToLayout(basicLayout, "Variable", createNodeIcon("Var"), 0, 1);
    
    m_mainLayout->addWidget(basicSection);
}

void NodePaletteBar::addMathNodes()
{
    QWidget* mathSection = createCategorySection("Math");
    QGridLayout* mathLayout = new QGridLayout(mathSection);
    mathLayout->setSpacing(2);
    mathLayout->setContentsMargins(4, 4, 4, 4);
    
    addToolToLayout(mathLayout, "Add", createTextIcon("+", QColor(200, 255, 200)), 0, 0);
    addToolToLayout(mathLayout, "Subtract", createTextIcon("-", QColor(255, 200, 200)), 0, 1);
    addToolToLayout(mathLayout, "Multiply", createTextIcon("×", QColor(200, 200, 255)), 1, 0);
    addToolToLayout(mathLayout, "Divide", createTextIcon("÷", QColor(255, 255, 200)), 1, 1);
    addToolToLayout(mathLayout, "Sin", createTextIcon("sin", QColor(255, 200, 255)), 2, 0);
    addToolToLayout(mathLayout, "Cos", createTextIcon("cos", QColor(200, 255, 255)), 2, 1);
    
    m_mainLayout->addWidget(mathSection);
}

void NodePaletteBar::addIONodes()
{
    QWidget* ioSection = createCategorySection("Input/Output");
    QGridLayout* ioLayout = new QGridLayout(ioSection);
    ioLayout->setSpacing(2);
    ioLayout->setContentsMargins(4, 4, 4, 4);
    
    addToolToLayout(ioLayout, "Input", createTextIcon("IN", QColor(150, 255, 150)), 0, 0);
    addToolToLayout(ioLayout, "Output", createTextIcon("OUT", QColor(255, 150, 150)), 0, 1);
    
    m_mainLayout->addWidget(ioSection);
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