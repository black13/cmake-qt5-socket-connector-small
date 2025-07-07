#include "node_palette_widget.h"
#include <QFont>
#include <QIcon>
#include <QDebug>
#include <QScrollArea>
#include <QGridLayout>
#include <QPainter>
#include <QMouseEvent>
#include <QToolButton>
#include <QSize>

// ============================================================================
// PaletteButton Implementation
// ============================================================================

PaletteButton::PaletteButton(const QString& iconAlias, 
                            const QString& tooltip, 
                            QWidget* parent)
    : QToolButton(parent)
{
    // Load icon from resource using alias
    QIcon icon(QStringLiteral(":/icons/%1").arg(iconAlias));
    setIcon(icon);
    setIconSize(QSize(24, 24));
    setToolTip(tooltip);
    
    // Apply consistent styling
    applyDefaultStyle();
}

void PaletteButton::setCheckable(bool checkable)
{
    QToolButton::setCheckable(checkable);
    if (checkable) {
        // Add checked state styling
        setStyleSheet(styleSheet() + 
                     "QToolButton:checked { "
                     "  background: #007acc; "
                     "  color: white; "
                     "}");
    }
}

void PaletteButton::applyDefaultStyle()
{
    setAutoRaise(true);
    setCursor(Qt::PointingHandCursor);
    setFixedSize(32, 32);
    
    // Apply CSS styling for hover states
    setStyleSheet(
        "QToolButton {"
        "  border: none;"
        "  border-radius: 4px;"
        "  padding: 4px;"
        "  background: transparent;"
        "}"
        "QToolButton:hover {"
        "  background: rgba(0, 0, 0, 0.08);"
        "}"
        "QToolButton:pressed {"
        "  background: rgba(0, 0, 0, 0.16);"
        "}"
    );
}

// ============================================================================
// NodePaletteWidget Implementation
// ============================================================================

NodePaletteWidget::NodePaletteWidget(QWidget* parent)
    : QWidget(parent)
    , m_mainLayout(nullptr)
    , m_searchEdit(nullptr)
    , m_scrollArea(nullptr)
    , m_scrollContent(nullptr)
    , m_gridLayout(nullptr)
    , m_titleLabel(nullptr)
{
    setupUI();
    populateNodeTemplates();
}

void NodePaletteWidget::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(8, 8, 8, 8);
    m_mainLayout->setSpacing(6);
    
    // Title
    m_titleLabel = new QLabel("Node Palette", this);
    QFont titleFont = m_titleLabel->font();
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 1);
    m_titleLabel->setFont(titleFont);
    m_titleLabel->setStyleSheet("QLabel { color: #2c3e50; margin-bottom: 4px; }");
    
    // Search box
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText("Search nodes...");
    m_searchEdit->setStyleSheet(
        "QLineEdit {"
        "  border: 1px solid #bdc3c7;"
        "  border-radius: 4px;"
        "  padding: 6px 10px;"
        "  background: white;"
        "  font-size: 12px;"
        "}"
        "QLineEdit:focus {"
        "  border-color: #3498db;"
        "  outline: none;"
        "}"
    );
    
    // Scroll area for node grid
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    
    // Content widget for the grid
    m_scrollContent = new QWidget();
    m_gridLayout = new QGridLayout(m_scrollContent);
    m_gridLayout->setContentsMargins(4, 4, 4, 4);
    m_gridLayout->setSpacing(6);
    
    m_scrollArea->setWidget(m_scrollContent);
    
    // Layout
    m_mainLayout->addWidget(m_titleLabel);
    m_mainLayout->addWidget(m_searchEdit);
    m_mainLayout->addWidget(m_scrollArea, 1);
    
    // Connections
    connect(m_searchEdit, &QLineEdit::textChanged, this, &NodePaletteWidget::filterChanged);
}

void NodePaletteWidget::populateNodeTemplates()
{
    // Add our self-serializing node types with visual icons
    
    NodeTemplate inputNode;
    inputNode.type = "IN";
    inputNode.name = "Input";
    inputNode.description = "Input node with configurable outputs";
    inputNode.iconPath = ""; // We'll create custom icons
    inputNode.inputSockets = 0;
    inputNode.outputSockets = 2;
    addNodeTemplate(inputNode);
    
    NodeTemplate outputNode;
    outputNode.type = "OUT";
    outputNode.name = "Output";
    outputNode.description = "Output node with configurable inputs";
    outputNode.iconPath = "";
    outputNode.inputSockets = 2;
    outputNode.outputSockets = 0;
    addNodeTemplate(outputNode);
    
    NodeTemplate processorNode;
    processorNode.type = "PROC";
    processorNode.name = "Processor";
    processorNode.description = "Processing node with inputs and outputs";
    processorNode.iconPath = "";
    processorNode.inputSockets = 2;
    processorNode.outputSockets = 2;
    addNodeTemplate(processorNode);
    
    qDebug() << "✓ Node palette populated with" << m_nodeTemplates.size() << "icon-based templates";
}

void NodePaletteWidget::addNodeTemplate(const NodeTemplate& nodeTemplate)
{
    m_nodeTemplates.append(nodeTemplate);
    
    // Create icon button for this node type
    NodeButton* button = new NodeButton(nodeTemplate, m_scrollContent);
    m_nodeButtons.append(button);
    
    // Connect button to our slot
    connect(button, &QPushButton::clicked, this, &NodePaletteWidget::onNodeButtonClicked);
    
    // Add to grid layout (2 columns)
    int row = m_nodeButtons.size() / 2;
    int col = (m_nodeButtons.size() - 1) % 2;
    m_gridLayout->addWidget(button, row, col);
}

void NodePaletteWidget::filterChanged(const QString& text)
{
    m_currentFilter = text;
    updateVisibility();
}

void NodePaletteWidget::onNodeButtonClicked()
{
    NodeButton* button = static_cast<NodeButton*>(sender());
    if (!button) return;
    
    NodeTemplate nodeTemplate = button->getNodeTemplate();
    qDebug() << "Node palette: Creating node" << nodeTemplate.name << "via button click";
    emit nodeCreationRequested(nodeTemplate);
}

void NodePaletteWidget::updateVisibility()
{
    for (NodeButton* button : m_nodeButtons) {
        bool visible = m_currentFilter.isEmpty() || button->matchesFilter(m_currentFilter);
        button->setVisible(visible);
    }
}

// ============================================================================
// NodeButton Implementation
// ============================================================================

NodePaletteWidget::NodeButton::NodeButton(const NodeTemplate& nodeTemplate, QWidget* parent)
    : QPushButton(parent)
    , m_nodeTemplate(nodeTemplate)
{
    setFixedSize(80, 80);
    setToolTip(QString("%1\n%2\nInputs: %3, Outputs: %4")
               .arg(nodeTemplate.name)
               .arg(nodeTemplate.description)
               .arg(nodeTemplate.inputSockets)
               .arg(nodeTemplate.outputSockets));
    
    // Create custom icon based on node type
    QIcon icon = NodeButton::createNodeIcon(nodeTemplate);
    setIcon(icon);
    setIconSize(QSize(48, 48));
    
    // Set text below icon
    setText(nodeTemplate.name);
    
    // Apply object name for external styling
    setObjectName("nodeButton");
}

bool NodePaletteWidget::NodeButton::matchesFilter(const QString& filter) const
{
    return m_nodeTemplate.name.contains(filter, Qt::CaseInsensitive) ||
           m_nodeTemplate.description.contains(filter, Qt::CaseInsensitive) ||
           m_nodeTemplate.type.contains(filter, Qt::CaseInsensitive);
}

QIcon NodePaletteWidget::NodeButton::createNodeIcon(const NodeTemplate& nodeTemplate)
{
    // Create a custom icon for each node type
    QPixmap pixmap(48, 48);
    pixmap.fill(Qt::transparent);
    
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Define colors for different node types
    QColor nodeColor;
    QString symbol;
    
    if (nodeTemplate.type == "IN") {
        nodeColor = QColor(46, 204, 113); // Green for input
        symbol = "IN";
    } else if (nodeTemplate.type == "OUT") {
        nodeColor = QColor(231, 76, 60); // Red for output
        symbol = "OUT";
    } else if (nodeTemplate.type == "PROC") {
        nodeColor = QColor(52, 152, 219); // Blue for processor
        symbol = "PROC";
    } else {
        nodeColor = QColor(149, 165, 166); // Gray for unknown
        symbol = "?";
    }
    
    // Draw node shape
    painter.setBrush(QBrush(nodeColor));
    painter.setPen(QPen(nodeColor.darker(120), 2));
    painter.drawRoundedRect(4, 4, 40, 40, 6, 6);
    
    // Draw text
    painter.setPen(Qt::white);
    painter.setFont(QFont("Arial", 8, QFont::Bold));
    painter.drawText(QRect(4, 4, 40, 40), Qt::AlignCenter, symbol);
    
    // Draw socket indicators
    painter.setBrush(QBrush(Qt::white));
    painter.setPen(QPen(Qt::darkGray, 1));
    
    // Input sockets (left side)
    int inputSpacing = nodeTemplate.inputSockets > 0 ? 32 / (nodeTemplate.inputSockets + 1) : 0;
    for (int i = 0; i < nodeTemplate.inputSockets; ++i) {
        int y = 8 + inputSpacing * (i + 1);
        painter.drawEllipse(0, y, 6, 6);
    }
    
    // Output sockets (right side)
    int outputSpacing = nodeTemplate.outputSockets > 0 ? 32 / (nodeTemplate.outputSockets + 1) : 0;
    for (int i = 0; i < nodeTemplate.outputSockets; ++i) {
        int y = 8 + outputSpacing * (i + 1);
        painter.drawEllipse(42, y, 6, 6);
    }
    
    return QIcon(pixmap);
}

