#pragma once

#include <QMainWindow>
#include <QString>
#include <QToolBar>
#include <QAction>
#include <QDockWidget>
#include <QLabel>
#include <QStatusBar>
#include <QMenuBar>
#include <QProgressBar>
#include <QSpinBox>
#include <QComboBox>
#include <libxml/tree.h>

class View;
class Scene;
class QGraph;
class GraphFactory;
class JavaScriptEngine;
class XmlAutosaveObserver;
class NodePaletteWidget;
// class JavaScriptConsole;  // Disabled for now

/**
 * Window - Enhanced main window for self-serializing node editor
 * 
 * Features:
 * - Professional UI with dock widgets and status bar
 * - Node palette with drag-and-drop functionality
 * - Multi-section status bar with graph statistics
 * - Menu system with proper actions
 * - Integration with self-serializing architecture
 */
class Window : public QMainWindow
{
public:
    explicit Window(QWidget* parent = nullptr);
    ~Window();
    
    // Access to scene for testing
    Scene* getScene() const { return m_scene; }

    // Update status bar with current graph information
    void updateStatusBar();

    // Create node at specific position (disabled for now)
    // void createNodeAtPosition(const QString& nodeType, const QPointF& scenePos);

    // Simple script execution
    void loadAndExecuteScript();
    void executeScriptFile(const QString& filePath);

    // Command-line driven initialization
    void setAutoTestScript(const QString& scriptPath);

protected:
    // PHASE 3: Safe shutdown coordination
    void closeEvent(QCloseEvent* event) override;
    void showEvent(QShowEvent* event) override;
public slots:
    // Scene event handlers
    void onSceneChanged();
    void onSelectionChanged();
    
    // Basic XML saving functionality
    bool saveGraph(const QString& filename);
    bool loadGraph(const QString& filename);
    
    // File management
    void setCurrentFile(const QString& filename);
    QString getCurrentFile() const { return m_currentFile; }

    // Node creation from palette (template system only)
    void createNodeFromPalette(const QPointF& scenePos, const QString& nodeType, 
                              const QString& name, int inputSockets, int outputSockets);
    
private slots:
    // Menu actions
    void newFile();
    void openFile();
    void saveFile();
    void saveAsFile();
    void exportGraph();
    void showAbout();
    
    // View actions
    void zoomIn();
    void zoomOut();
    void zoomFit();
    void zoomReset();
    
    // Selection info update
    void updateSelectionInfo();

    // Palette integration removed - nodes created via drag-and-drop only

protected:
    // Handle keyboard shortcuts
    void keyPressEvent(QKeyEvent* event) override;

private:
    Scene* m_scene;
    QGraph* m_graph;  // Graph orchestration layer
    View* m_view;
    GraphFactory* m_factory;
    JavaScriptEngine* m_jsEngine;  // Application logic layer
    xmlDocPtr m_xmlDocument;
    XmlAutosaveObserver* m_autosaveObserver;

    // UI elements - legacy actions removed (use NodePalette instead)

    // Professional node palette system
    QDockWidget* m_nodePaletteDock;
    NodePaletteWidget* m_nodePalette;
    
    // JavaScript console (disabled for now)
    // QDockWidget* m_javaScriptConsoleDock;
    // JavaScriptConsole* m_javaScriptConsole;
    
    // Status bar components
    QLabel* m_fileInfoLabel;      // Current file info
    QLabel* m_graphStatsLabel;    // Node/edge count
    QLabel* m_selectionLabel;     // Selection information
    QLabel* m_positionLabel;      // Mouse position
    QLabel* m_zoomLabel;          // Current zoom level
    QProgressBar* m_operationProgress; // For long operations
    
    // Menu system
    QMenu* m_fileMenu;
    QMenu* m_editMenu;
    QMenu* m_viewMenu;
    QMenu* m_toolsMenu;
    QMenu* m_helpMenu;
    
    // File management
    QString m_currentFile;

    // Command-line driven initialization
    QString m_autoTestScript;
    bool m_firstShow;

    // Setup methods
    void setupUI();
    void setupMenus();
    void setupActions();
    void setupStatusBar();
    void setupDockWidgets();
    
    // Menu creation helpers
    void createFileMenu();
    void createEditMenu();
    void createViewMenu();
    void createToolsMenu();
    void createHelpMenu();
    
    // Status bar helpers
    void createStatusBarWidgets();
    void connectStatusBarSignals();
};