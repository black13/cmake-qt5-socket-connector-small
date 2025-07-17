#pragma once

#include <QWidget>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include "node_tool_button.h"

/**
 * NodePaletteBar - Professional node palette for graph editing
 * 
 * Provides an organized collection of node types that users can add
 * to the scene. Supports categorization and clean visual layout.
 */
class NodePaletteBar : public QWidget
{
    Q_OBJECT

public:
    explicit NodePaletteBar(QWidget* parent = nullptr);
    
    // Add nodes dynamically
    void addNodeType(const QString& category, const QString& nodeType, const QIcon& icon);
    void addSeparator();

signals:
    void nodeSelected(const QString& nodeType);

private slots:
    void onNodeClicked(const QString& nodeType);

private:
    QVBoxLayout* m_mainLayout;
    QWidget* m_contentWidget;
    QScrollArea* m_scrollArea;
    
    void setupUI();
    void addBasicNodes();
    void addMathNodes();
    void addIONodes();
    
    QWidget* createCategorySection(const QString& title);
    void addToolToLayout(QGridLayout* layout, const QString& name, const QIcon& icon, int row, int col);
    
    // Create icons that show socket configuration
    QIcon createNodeIcon(const QString& nodeType);
    QIcon createTextIcon(const QString& text, const QColor& bgColor = QColor(200, 200, 255));
    QIcon createSocketIcon(int inputs, int outputs, const QColor& bgColor);
};