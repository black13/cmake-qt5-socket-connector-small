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
#include <QDrag>
#include <QMimeData>
#include <QApplication>

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
    m_gridLayout->setContentsMargins(8, 8, 8, 8);
    m_gridLayout->setSpacing(10); // Increased spacing between buttons
    m_gridLayout->setAlignment(Qt::AlignTop | Qt::AlignHCenter); // Center buttons and align to top
    
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
    qDebug() << "NodePalette: Starting population of 5 node templates";
    
    // 1. One Source (0 inputs, 1 output)
    NodeTemplate sourceNode;
    sourceNode.type = "SOURCE";
    sourceNode.name = "Source";
    sourceNode.description = "Source node with one output";
    sourceNode.iconPath = "";
    sourceNode.inputSockets = 0;
    sourceNode.outputSockets = 1;
    qDebug() << "NodePalette: Adding SOURCE node template - inputs:" << sourceNode.inputSockets << "outputs:" << sourceNode.outputSockets;
    addNodeTemplate(sourceNode);
    
    // 2. One Sink (1 input, 0 outputs)
    NodeTemplate sinkNode;
    sinkNode.type = "SINK";
    sinkNode.name = "Sink";
    sinkNode.description = "Sink node with one input";
    sinkNode.iconPath = "";
    sinkNode.inputSockets = 1;
    sinkNode.outputSockets = 0;
    qDebug() << "NodePalette: Adding SINK node template - inputs:" << sinkNode.inputSockets << "outputs:" << sinkNode.outputSockets;
    addNodeTemplate(sinkNode);
    
    // 3. One Sink + One Source (1 input, 1 output)
    NodeTemplate transformNode;
    transformNode.type = "TRANSFORM";
    transformNode.name = "Transform";
    transformNode.description = "Transform node with one input and one output";
    transformNode.iconPath = "";
    transformNode.inputSockets = 1;
    transformNode.outputSockets = 1;
    qDebug() << "NodePalette: Adding TRANSFORM node template - inputs:" << transformNode.inputSockets << "outputs:" << transformNode.outputSockets;
    addNodeTemplate(transformNode);
    
    // 4. Two Sinks + One Source (2 inputs, 1 output)
    NodeTemplate mergeNode;
    mergeNode.type = "MERGE";
    mergeNode.name = "Merge";
    mergeNode.description = "Merge node with two inputs and one output";
    mergeNode.iconPath = "";
    mergeNode.inputSockets = 2;
    mergeNode.outputSockets = 1;
    qDebug() << "NodePalette: Adding MERGE node template - inputs:" << mergeNode.inputSockets << "outputs:" << mergeNode.outputSockets;
    addNodeTemplate(mergeNode);
    
    // 5. One Sink + Two Sources (1 input, 2 outputs)
    NodeTemplate splitNode;
    splitNode.type = "SPLIT";
    splitNode.name = "Split";
    splitNode.description = "Split node with one input and two outputs";
    splitNode.iconPath = "";
    splitNode.inputSockets = 1;
    splitNode.outputSockets = 2;
    qDebug() << "NodePalette: Adding SPLIT node template - inputs:" << splitNode.inputSockets << "outputs:" << splitNode.outputSockets;
    addNodeTemplate(splitNode);
    
    qDebug() << "✓ NodePalette: Populated with" << m_nodeTemplates.size() << "socket configuration templates";
}

void NodePaletteWidget::addNodeTemplate(const NodeTemplate& nodeTemplate)
{
    qDebug() << "NodePalette: Adding template to internal list -" << nodeTemplate.name << "(" << nodeTemplate.type << ")";
    m_nodeTemplates.append(nodeTemplate);
    
    // Create icon button for this node type
    qDebug() << "NodePalette: Creating NodeButton for" << nodeTemplate.name;
    NodeButton* button = new NodeButton(nodeTemplate, m_scrollContent);
    m_nodeButtons.append(button);
    
    // Connect button to our slot
    qDebug() << "NodePalette: Connecting button signals for" << nodeTemplate.name;
    connect(button, &QToolButton::clicked, this, &NodePaletteWidget::onNodeButtonClicked);
    
    // Add to grid layout (2 columns) - proper grid arrangement
    int buttonIndex = m_nodeButtons.size() - 1; // Current button index (0-based)
    int row = buttonIndex / 2; // Integer division for row
    int col = buttonIndex % 2; // Remainder for column (0 or 1)
    qDebug() << "NodePalette: Adding button" << (buttonIndex + 1) << "to grid layout at row" << row << "col" << col;
    m_gridLayout->addWidget(button, row, col);
    qDebug() << "✓ NodePalette: Successfully added" << nodeTemplate.name << "button to palette";
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
    : QToolButton(parent)
    , m_nodeTemplate(nodeTemplate)
{
    qDebug() << "NodeButton: Creating button for" << nodeTemplate.name << "type:" << nodeTemplate.type;
    qDebug() << "NodeButton: Socket configuration - inputs:" << nodeTemplate.inputSockets << "outputs:" << nodeTemplate.outputSockets;
    
    setFixedSize(80, 80);
    setToolTip(QString("%1\n%2\nInputs: %3, Outputs: %4\n\nDrag to create or double-click")
               .arg(nodeTemplate.name)
               .arg(nodeTemplate.description)
               .arg(nodeTemplate.inputSockets)
               .arg(nodeTemplate.outputSockets));
    
    // Create custom icon based on node type
    qDebug() << "NodeButton: Creating custom icon for" << nodeTemplate.name;
    QIcon icon = NodeButton::createNodeIcon(nodeTemplate);
    setIcon(icon);
    setIconSize(QSize(48, 48));
    
    // Set text below icon
    setText(nodeTemplate.name);
    setToolButtonStyle(Qt::ToolButtonTextUnderIcon); // Position text under the icon
    
    // Apply object name for external styling
    setObjectName("nodeButton");
    
    // Enable drag support
    setAcceptDrops(false); // This is a drag source, not a drop target
    qDebug() << "✓ NodeButton: Button created successfully for" << nodeTemplate.name;
}

bool NodePaletteWidget::NodeButton::matchesFilter(const QString& filter) const
{
    return m_nodeTemplate.name.contains(filter, Qt::CaseInsensitive) ||
           m_nodeTemplate.description.contains(filter, Qt::CaseInsensitive) ||
           m_nodeTemplate.type.contains(filter, Qt::CaseInsensitive);
}

QIcon NodePaletteWidget::NodeButton::createNodeIcon(const NodeTemplate& nodeTemplate)
{
    // Create a custom icon representing the node function
    QPixmap pixmap(48, 48);
    pixmap.fill(Qt::transparent);
    
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Define colors and symbols based on node type
    QColor nodeColor;
    QString symbol;
    
    if (nodeTemplate.type == "SOURCE") {
        nodeColor = QColor(46, 204, 113); // Green for source
        symbol = "SRC";
    } else if (nodeTemplate.type == "SINK") {
        nodeColor = QColor(231, 76, 60); // Red for sink
        symbol = "SNK";
    } else if (nodeTemplate.type == "TRANSFORM") {
        nodeColor = QColor(52, 152, 219); // Blue for transform
        symbol = "TRN";
    } else if (nodeTemplate.type == "MERGE") {
        nodeColor = QColor(155, 89, 182); // Purple for merge
        symbol = "MRG";
    } else if (nodeTemplate.type == "SPLIT") {
        nodeColor = QColor(243, 156, 18); // Orange for split
        symbol = "SPL";
    } else {
        nodeColor = QColor(149, 165, 166); // Gray for unknown
        symbol = "?";
    }
    
    // Draw main node body
    painter.setBrush(QBrush(nodeColor));
    painter.setPen(QPen(nodeColor.darker(120), 2));
    painter.drawRoundedRect(6, 6, 36, 36, 4, 4);
    
    // Draw function symbol
    painter.setPen(Qt::white);
    painter.setFont(QFont("Arial", 7, QFont::Bold));
    painter.drawText(QRect(6, 6, 36, 20), Qt::AlignCenter, symbol);
    
    // Draw socket representations with proper alignment
    painter.setBrush(QBrush(Qt::white));
    painter.setPen(QPen(Qt::darkGray, 1.5));
    
    // Constants for consistent positioning
    const qreal nodeTop = 6;
    const qreal nodeHeight = 36;
    const qreal socketSize = 4;
    const qreal socketSpacing = 8; // Consistent spacing between sockets
    
    // Input sockets (left side) - properly centered vertically
    if (nodeTemplate.inputSockets > 0) {
        qreal totalHeight = (nodeTemplate.inputSockets - 1) * socketSpacing;
        qreal startY = nodeTop + (nodeHeight - totalHeight) / 2;
        
        for (int i = 0; i < nodeTemplate.inputSockets; ++i) {
            qreal y = startY + (i * socketSpacing) - socketSize/2;
            painter.drawEllipse(QRectF(1, y, socketSize, socketSize));
        }
    }
    
    // Output sockets (right side) - properly centered vertically  
    if (nodeTemplate.outputSockets > 0) {
        qreal totalHeight = (nodeTemplate.outputSockets - 1) * socketSpacing;
        qreal startY = nodeTop + (nodeHeight - totalHeight) / 2;
        
        for (int i = 0; i < nodeTemplate.outputSockets; ++i) {
            qreal y = startY + (i * socketSpacing) - socketSize/2;
            painter.drawEllipse(QRectF(43, y, socketSize, socketSize));
        }
    }
    
    // Add visual flow indicators for function type - aligned with node center
    const qreal centerY = nodeTop + nodeHeight / 2;
    painter.setPen(QPen(Qt::white, 1.5, Qt::SolidLine));
    
    if (nodeTemplate.type == "TRANSFORM") {
        // Horizontal arrow through center
        painter.drawLine(8, centerY, 40, centerY);
        // Arrow head
        painter.drawLine(36, centerY - 3, 40, centerY);
        painter.drawLine(36, centerY + 3, 40, centerY);
    } else if (nodeTemplate.type == "MERGE") {
        // Converging lines to center
        painter.drawLine(8, centerY - 6, 24, centerY);
        painter.drawLine(8, centerY + 6, 24, centerY);
        painter.drawLine(24, centerY, 40, centerY);
        // Arrow head
        painter.drawLine(36, centerY - 2, 40, centerY);
        painter.drawLine(36, centerY + 2, 40, centerY);
    } else if (nodeTemplate.type == "SPLIT") {
        // Diverging lines from center
        painter.drawLine(8, centerY, 24, centerY);
        painter.drawLine(24, centerY, 40, centerY - 6);
        painter.drawLine(24, centerY, 40, centerY + 6);
        // Arrow heads
        painter.drawLine(36, centerY - 8, 40, centerY - 6);
        painter.drawLine(36, centerY - 4, 40, centerY - 6);
        painter.drawLine(36, centerY + 4, 40, centerY + 6);
        painter.drawLine(36, centerY + 8, 40, centerY + 6);
    }
    
    return QIcon(pixmap);
}

void NodePaletteWidget::NodeButton::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        qDebug() << "NodeButton: Mouse press detected on" << m_nodeTemplate.name << "at position:" << event->pos();
        m_dragStartPosition = event->pos();
    }
    QToolButton::mousePressEvent(event);
}

void NodePaletteWidget::NodeButton::mouseMoveEvent(QMouseEvent* event)
{
    if (!(event->buttons() & Qt::LeftButton)) {
        QToolButton::mouseMoveEvent(event);
        return;
    }
    
    qreal distance = (event->pos() - m_dragStartPosition).manhattanLength();
    if (distance < QApplication::startDragDistance()) {
        qDebug() << "NodeButton: Mouse moved but distance" << distance << "< drag threshold" << QApplication::startDragDistance();
        QToolButton::mouseMoveEvent(event);
        return;
    }
    
    qDebug() << "NodeButton: Starting drag operation for" << m_nodeTemplate.name;
    qDebug() << "NodeButton: Template data - type:" << m_nodeTemplate.type << "inputs:" << m_nodeTemplate.inputSockets << "outputs:" << m_nodeTemplate.outputSockets;
    
    // Start drag operation
    QDrag* drag = new QDrag(this);
    QMimeData* mimeData = new QMimeData;
    
    // Store node template data in mime data
    QString mimeString = QString("%1|%2|%3|%4|%5")
                        .arg(m_nodeTemplate.type)
                        .arg(m_nodeTemplate.name)
                        .arg(m_nodeTemplate.description)
                        .arg(m_nodeTemplate.inputSockets)
                        .arg(m_nodeTemplate.outputSockets);
    
    qDebug() << "NodeButton: Encoding mime data:" << mimeString;
    mimeData->setData("application/x-node-template", mimeString.toUtf8());
    
    // Create drag pixmap from the button's icon
    QPixmap dragPixmap = icon().pixmap(48, 48);
    if (dragPixmap.isNull()) {
        qDebug() << "NodeButton: Warning - icon pixmap is null, creating fallback";
        dragPixmap = QPixmap(48, 48);
        dragPixmap.fill(Qt::gray);
    }
    
    // Make it semi-transparent for visual feedback
    QPixmap transparentPixmap(dragPixmap.size());
    transparentPixmap.fill(Qt::transparent);
    QPainter painter(&transparentPixmap);
    painter.setOpacity(0.7);
    painter.drawPixmap(0, 0, dragPixmap);
    painter.end();
    
    drag->setMimeData(mimeData);
    drag->setPixmap(transparentPixmap);
    drag->setHotSpot(QPoint(24, 24)); // Center of the icon
    
    qDebug() << "NodeButton: Executing drag operation for" << m_nodeTemplate.name;
    
    // Execute the drag
    Qt::DropAction dropAction = drag->exec(Qt::CopyAction);
    
    if (dropAction == Qt::CopyAction) {
        qDebug() << "✓ NodeButton: Drag completed successfully for" << m_nodeTemplate.name;
    } else {
        qDebug() << "✗ NodeButton: Drag was cancelled or failed for" << m_nodeTemplate.name;
    }
}

