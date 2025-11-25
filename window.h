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
#include <QShortcut>

class View;
class Scene;
class Graph;
class Node;

// Forward declarations for libxml types (reduces header pollution)
typedef struct _xmlNode xmlNode;
typedef xmlNode* xmlNodePtr;
typedef struct _xmlDoc xmlDoc;
typedef xmlDoc* xmlDocPtr;
class GraphFactory;
class XmlAutosaveObserver;
class NodePaletteWidget;

// JavaScript integration will be added via Graph facade (no conditional compilation)

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
    Q_OBJECT
    
public:
    /**
     * @brief Construct the main applicaton window and attach default UI.
     */
    explicit Window(QWidget* parent = nullptr);
    ~Window();
    
    /**
     * @brief Provide an already-created GraphFactory (Window does not own it).
     */
    void adoptFactory(GraphFactory* factory);

    /// @return Underlying Scene (primarily for testing helpers).
    [[nodiscard]] Scene* getScene() const { return m_scene; }

    /// @return Graph facade powering UI + scripting.
    [[nodiscard]] Graph* getGraph() const { return m_graph; }

    /**
     * @brief Register a startup script path (CLI --script).
     *
     * Executed once during showEvent().
     */
    void setStartupScript(const QString& scriptPath);

    /**
     * @brief Refresh status-bar widgets with graph statistics.
     */
    void updateStatusBar();

protected:
    void showEvent(QShowEvent* event) override;
    void closeEvent(QCloseEvent* event) override;
public slots:
    /// Scene change callbacks wired to GraphSubject observers.
    void onSceneChanged();
    void onSelectionChanged();

    /**
     * @brief Open scripted-node context menu (invoked from View).
     */
    void showContextMenu(Node* node, const QPoint& screenPos, const QPointF& scenePos);
    
    /// Basic XML save/load entry points.
    bool saveGraph(const QString& filename);
    bool loadGraph(const QString& filename);
    
    /// File management helpers.
    void setCurrentFile(const QString& filename);
    [[nodiscard]] QString getCurrentFile() const { return m_currentFile; }
    
    /// Interactive node creation shortcuts.
    void createInputNode();
    void createOutputNode();
    void createProcessorNode();

    // Node creation from palette
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

    // Auto layout (annealing)
    // View actions
    void zoomIn();
    void zoomOut();
    void zoomFit();
    void zoomReset();
    
    // Selection info update
    void updateSelectionInfo();
    
protected:
    /// Handle Delete/shortcut dispatch that can't live in Scene.
    void keyPressEvent(QKeyEvent* event) override;
    
    /// Initialize UI components that do not require GraphFactory.
    void initializeUi();
    
    /// Perform setup that depends on m_factory (palette/templates).
    void initializeWithFactory();

    /// Delete selected nodes/edges via Graph facade.
    bool deleteSelection();

private:
    Scene* m_scene;
    View* m_view;
    Graph* m_graph;           // Graph facade (owns QJSEngine)
    GraphFactory* m_factory;  // non-owning
    // GraphController removed - using template system directly
    XmlAutosaveObserver* m_autosaveObserver;
    
    // UI elements
    QAction* m_addInputAction;
    QAction* m_addOutputAction;
    QAction* m_addProcessorAction;
    
    // Professional node palette system
    QDockWidget* m_nodePaletteDock;
    NodePaletteWidget* m_nodePalette;
    
    // JavaScript console members removed
    
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
    QString m_startupScript;
    bool m_startupScriptExecuted = false;

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
    
    // Helpers for visual tests
    void restoreCurrentFile();     // reload m_currentFile if set, else clear
    void restoreJustLoadedFile();

    /// Run a scripted node individually (context menu helper).
    bool runScriptForNode(Node* node);
    /// Execute scripts for a batch of nodes (selection/all).
    void runScriptsForNodes(const QList<Node*>& nodes, const QString& contextLabel);
    /// Query Graph facade to see if a node has script text.
    bool nodeHasScript(Node* node) const;

    // JavaScript integration will be added via Graph facade
};
