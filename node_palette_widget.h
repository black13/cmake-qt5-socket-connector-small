#pragma once

#include <QWidget>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QToolButton>
#include <QFrame>

/**
 * PaletteButton - Consistent button for palette operations
 * 
 * Features:
 * - Uniform 24x24 icon size
 * - Consistent styling and hover states
 * - Tooltip support
 * - Icon resource alias support
 */
class PaletteButton : public QToolButton {
    Q_OBJECT
public:
    explicit PaletteButton(const QString& iconAlias, 
                          const QString& tooltip, 
                          QWidget* parent = nullptr);
    
    // Set checkable state for toggle tools
    void setCheckable(bool checkable);
    
    // Apply consistent styling
    void applyDefaultStyle();
};

/**
 * NodePaletteWidget - Icon-based node palette with grid layout
 * 
 * Features:
 * - Search filtering
 * - Double-click node creation
 * - Icon-based grid layout
 * - Professional visual node type recognition
 * - Integration with self-serializing nodes
 */
class NodePaletteWidget : public QWidget
{
    Q_OBJECT

public:
    explicit NodePaletteWidget(QWidget* parent = nullptr);
    
    // Node template structure
    struct NodeTemplate {
        QString type;       // Node type (IN, OUT, PROCESSOR)
        QString name;       // Display name
        QString description; // Tooltip description
        QString iconPath;   // Icon file path
        int inputSockets;   // Number of input sockets
        int outputSockets;  // Number of output sockets
        
        // Default constructor for QVariant
        NodeTemplate() : inputSockets(0), outputSockets(0) {}
        
        // Copy constructor
        NodeTemplate(const NodeTemplate& other) = default;
        NodeTemplate& operator=(const NodeTemplate& other) = default;
    };

signals:
    // Emitted when user wants to create a node
    void nodeCreationRequested(const NodeTemplate& nodeTemplate);

private slots:
    void filterChanged(const QString& text);
    void onNodeButtonClicked();

private:
    void setupUI();
    void populateNodeTemplates();
    void addNodeTemplate(const NodeTemplate& nodeTemplate);
    void updateVisibility();
    
    // Custom node button class with drag and drop support
    class NodeButton : public QPushButton {
    public:
        NodeButton(const NodeTemplate& nodeTemplate, QWidget* parent = nullptr);
        NodeTemplate getNodeTemplate() const { return m_nodeTemplate; }
        bool matchesFilter(const QString& filter) const;
        
    protected:
        // Drag and drop support
        void mousePressEvent(QMouseEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;
        
    private:
        NodeTemplate m_nodeTemplate;
        QPoint m_dragStartPosition;
        static QIcon createNodeIcon(const NodeTemplate& nodeTemplate);
    };
    
    // UI components
    QVBoxLayout* m_mainLayout;
    QLineEdit* m_searchEdit;
    QScrollArea* m_scrollArea;
    QWidget* m_scrollContent;
    QGridLayout* m_gridLayout;
    QLabel* m_titleLabel;
    
    // Node templates and buttons
    QList<NodeTemplate> m_nodeTemplates;
    QList<NodeButton*> m_nodeButtons;
    QString m_currentFilter;
};

// Declare NodeTemplate as a Qt metatype for QVariant storage
Q_DECLARE_METATYPE(NodePaletteWidget::NodeTemplate)