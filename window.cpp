#include "window.h"
#include "view.h"
#include "scene.h"
#include "graph.h"
#include "node.h"
#include "edge.h"
#include "graph_factory.h"
#include "xml_autosave_observer.h"
#include "node_palette_widget.h"
#include <QKeyEvent>
#include <QGraphicsScene>
#include <QFileDialog>
#include <QMessageBox>
#include <QDebug>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QAction>
#include <QIcon>
#include <QRandomGenerator>
#include <QDockWidget>
#include <QLabel>
#include <QStatusBar>
#include <QMenuBar>
#include <QProgressBar>
#include <QApplication>
#include <QDesktopServices>
#include <QUrl>
#include <QRandomGenerator>
#include <QFileInfo>
#include <QStatusBar>
#include <QImage>
#include <QVariantList>
#include <QPainter>
#include <QStandardPaths>
#include <QDir>
#include <QDateTime>
#include <QFileDialog>
#include <libxml/tree.h>
#include <libxml/xmlsave.h>

Window::Window(QWidget* parent)
    : QMainWindow(parent)
    , m_scene(new Scene(this))
    , m_view(new View(m_scene, this))
    , m_graph(nullptr)
    , m_factory(nullptr)
{
    initializeUi(); // Setup UI that doesn't depend on factory
}
    

Window::~Window()
{
    // Clean up autosave observer
    if (m_autosaveObserver) {
        m_scene->detach(m_autosaveObserver);
        delete m_autosaveObserver;
    }

    // Clean up Graph facade (owns QJSEngine)
    delete m_graph;

    // DO NOT delete m_factory (non-owning)
}

void Window::adoptFactory(GraphFactory* factory)
{
    Q_ASSERT(factory); // Null factory is a programming error
    m_factory = factory; // non-owning: main owns factory lifetime

    // ISSUE 4: Inject factory into scene for consistent edge creation
    m_scene->setGraphFactory(factory);

    // Create Graph facade - the new public API with JavaScript integration
    m_graph = new Graph(m_scene, m_factory, this);
    qDebug() << "Graph facade created with JavaScript engine enabled";

    // Now that a factory exists, wire things that depend on it:
    // ISSUE 2: Centralize autosave timing - using 1200ms as compromise
    m_autosaveObserver = new XmlAutosaveObserver(m_scene, "autosave.xml");
    m_autosaveObserver->setDelay(1200); // Centralized autosave policy

    // CRITICAL: Attach observer to scene to receive notifications
    m_scene->attach(m_autosaveObserver);

    initializeWithFactory(); // actions that depend on m_factory

    // Initial status bar update (m_graph exists now, safe to call)
    updateStatusBar();
}

void Window::initializeUi()
{
    setWindowTitle("NodeGraph - Self-Serializing Node Editor");
    resize(1400, 900);
    
    // Initialize UI components to nullptr
    m_nodePaletteDock = nullptr;
    m_nodePalette = nullptr;
    m_fileInfoLabel = nullptr;
    m_graphStatsLabel = nullptr;
    m_selectionLabel = nullptr;
    m_positionLabel = nullptr;
    m_zoomLabel = nullptr;
    m_operationProgress = nullptr;
    
    // Autosave will be initialized when factory is adopted
    
    // Setup enhanced UI
    setupUI();
    setupActions();
    setupMenus();
    setupStatusBar();
    setupDockWidgets();
    
    // Connect scene signals for status/selection updates
    connect(m_scene, &Scene::sceneChanged, this, &Window::onSceneChanged);
    connect(m_scene, &QGraphicsScene::selectionChanged, this, &Window::onSelectionChanged);
    
    // Connect view signals for drag-and-drop
    connect(m_view, &View::nodeDropped, this, &Window::createNodeFromPalette);

    // Initial status update moved to adoptFactory() after m_graph is created
    // (updateStatusBar() now requires m_graph to exist)

    // Enable keyboard shortcuts
    setFocusPolicy(Qt::StrongFocus);
}

void Window::initializeWithFactory()
{
    // Actions or connections that require m_factory can go here if needed
    // Currently most UI elements work without factory, so this might be minimal
}

void Window::setupActions()
{
    // Create actions for node creation
    m_addInputAction = new QAction("Add Input", this);
    m_addInputAction->setToolTip("Add Input Node (Ctrl+1)");
    m_addInputAction->setShortcut(QKeySequence("Ctrl+1"));
    connect(m_addInputAction, &QAction::triggered, this, &Window::createInputNode);
    
    m_addOutputAction = new QAction("Add Output", this);
    m_addOutputAction->setToolTip("Add Output Node (Ctrl+2)");
    m_addOutputAction->setShortcut(QKeySequence("Ctrl+2"));
    connect(m_addOutputAction, &QAction::triggered, this, &Window::createOutputNode);
    
    m_addProcessorAction = new QAction("Add Processor", this);
    m_addProcessorAction->setToolTip("Add Processor Node (Ctrl+3)");
    m_addProcessorAction->setShortcut(QKeySequence("Ctrl+3"));
    connect(m_addProcessorAction, &QAction::triggered, this, &Window::createProcessorNode);

    QShortcut* deleteShortcut = new QShortcut(QKeySequence::Delete, this);
    deleteShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    connect(deleteShortcut, &QShortcut::activated, this, [this]() {
        deleteSelection();
    });

    QShortcut* backspaceShortcut = new QShortcut(QKeySequence::Backspace, this);
    backspaceShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    connect(backspaceShortcut, &QShortcut::activated, this, [this]() {
        deleteSelection();
    });
}


void Window::keyPressEvent(QKeyEvent* event)
{
    qDebug() << "Window::keyPressEvent key" << event->key()
             << "modifiers" << event->modifiers();
    if (event->modifiers() & Qt::ControlModifier) {
        switch (event->key()) {
            case Qt::Key_1:
                createInputNode();
                break;
            case Qt::Key_2:
                createOutputNode();
                break;
            case Qt::Key_3:
                createProcessorNode();
                break;
            case Qt::Key_S:
                if (event->modifiers() & Qt::ShiftModifier) {
                    // Ctrl+Shift+S = Save As
                    QString filename = QFileDialog::getSaveFileName(
                        this, 
                        "Save Graph As", 
                        "graph.xml", 
                        "XML Files (*.xml)");
                    if (!filename.isEmpty()) {
                        if (saveGraph(filename)) {
                            m_currentFile = filename;
                            setWindowTitle(QString("Node Editor - %1").arg(QFileInfo(filename).fileName()));
                        }
                    }
                } else {
                    // Ctrl+S = Save
                    if (m_currentFile.isEmpty()) {
                        // No current file, show Save As dialog
                        QString filename = QFileDialog::getSaveFileName(
                            this, 
                            "Save Graph", 
                            "graph.xml", 
                            "XML Files (*.xml)");
                        if (!filename.isEmpty()) {
                            if (saveGraph(filename)) {
                                m_currentFile = filename;
                                setWindowTitle(QString("Node Editor - %1").arg(QFileInfo(filename).fileName()));
                            }
                        }
                    } else {
                        // Save to current file
                        saveGraph(m_currentFile);
                    }
                }
                break;
            case Qt::Key_O:
                // Ctrl+O = Open
                {
                    QString filename = QFileDialog::getOpenFileName(
                        this, 
                        "Open Graph", 
                        "", 
                        "XML Files (*.xml)");
                    if (!filename.isEmpty()) {
                        if (loadGraph(filename)) {
                            m_currentFile = filename;
                            setWindowTitle(QString("Node Editor - %1").arg(QFileInfo(filename).fileName()));
                        }
                    }
                }
                break;
        }
    } else if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
        if (deleteSelection()) {
            event->accept();
            return;
        }
    }
    QMainWindow::keyPressEvent(event);
}

void Window::setCurrentFile(const QString& filename)
{
    m_currentFile = filename;
    if (!filename.isEmpty()) {
        setWindowTitle(QString("Node Editor - %1").arg(QFileInfo(filename).fileName()));
        qDebug() << "Current file set to:" << filename;
    } else {
        setWindowTitle("Node Editor");
        qDebug() << "Current file cleared";
    }
}

void Window::setStartupScript(const QString& scriptPath)
{
    m_startupScript = scriptPath;
    qDebug() << "Window: Startup script set:" << scriptPath;
    // TODO: Execute script in showEvent() after event loop is running
}

bool Window::saveGraph(const QString& filename)
{
    qDebug() << "Saving graph to:" << filename;
    
    QElapsedTimer timer;
    timer.start();
    
    // Create XML document
    xmlDocPtr doc = xmlNewDoc(BAD_CAST "1.0");
    xmlNodePtr root = xmlNewNode(nullptr, BAD_CAST "graph");
    xmlDocSetRootElement(doc, root);
    xmlSetProp(root, BAD_CAST "version", BAD_CAST "1.0");
    
    // Step 1: Save all nodes
    for (Node* node : m_scene->getNodes().values()) {
        xmlNodePtr nodeXml = node->write(doc, root);
        Q_UNUSED(nodeXml)
    }
    
    // Step 2: Save all edges
    for (Edge* edge : m_scene->getEdges().values()) {
        xmlNodePtr edgeXml = edge->write(doc, root);
        Q_UNUSED(edgeXml)
    }
    
    // Step 3: Save to file
    int result = xmlSaveFormatFileEnc(filename.toUtf8().constData(), doc, "UTF-8", 1);
    xmlFreeDoc(doc);
    
    qint64 elapsed = timer.elapsed();
    
    if (result != -1) {
        QFileInfo fileInfo(filename);
        qint64 fileSize = fileInfo.size();
        QVariantMap stats = m_graph->getGraphStats();
        int nodeCount = stats["nodeCount"].toInt();
        int edgeCount = stats["edgeCount"].toInt();
        
        qDebug() << "Manual save complete:";
        qDebug() << "   File:" << fileInfo.fileName();
        qDebug() << "   Time:" << elapsed << "ms";
        qDebug() << "   Size:" << (fileSize / 1024.0) << "KB";
        qDebug() << "   Nodes:" << nodeCount;
        qDebug() << "   Edges:" << edgeCount;
        
        QMessageBox::information(this, "Save Complete", 
            QString("Graph saved successfully!\n\nFile: %1\nNodes: %2\nEdges: %3\nTime: %4ms\nSize: %5 KB")
            .arg(fileInfo.fileName())
            .arg(nodeCount)
            .arg(edgeCount)
            .arg(elapsed)
            .arg(fileSize / 1024.0, 0, 'f', 1));
        return true;
    } else {
        qDebug() << "Failed to save graph";
        QMessageBox::critical(this, "Save Error", "Failed to save graph to file.");
        return false;
    }
}

bool Window::loadGraph(const QString& filename)
{
    if (!m_factory) {
        QMessageBox::warning(this, "No Factory", "Cannot load without a GraphFactory.");
        return false;
    }

    if (m_autosaveObserver) {
        m_autosaveObserver->setEnabled(false);
    }

    // Graph facade handles batch mode and clearing internally
    m_graph->clearGraph();

    const bool ok = m_graph->loadFromFile(filename);
    if (ok) {
        setCurrentFile(filename);
        updateStatusBar();
    }
    // Graph facade handles batch mode internally (removed endBatch)

    if (m_autosaveObserver) {
        m_autosaveObserver->saveNow();
        m_autosaveObserver->setEnabled(true);
    }

    return ok;
}

void Window::createInputNode()
{
    if (!m_graph) {
        QMessageBox::warning(this,"No Graph","Graph facade not initialized.");
        return;
    }

    // Find a nice position in the view center
    QPointF viewCenter = m_view->mapToScene(m_view->viewport()->rect().center());

    // Add some randomization so multiple nodes don't overlap
    qreal randomX = QRandomGenerator::global()->bounded(-50, 50);
    qreal randomY = QRandomGenerator::global()->bounded(-50, 50);
    QPointF position = viewCenter + QPointF(randomX, randomY);

    // Create input node using Graph facade API
    QString nodeId = m_graph->createNode("SOURCE", position.x(), position.y());

    if (!nodeId.isEmpty()) {
        qDebug() << "Created input node" << nodeId << "at" << position;
        statusBar()->showMessage(QString("Created SOURCE node: %1").arg(nodeId), 2000);
    } else {
        qDebug() << "Failed to create input node";
        statusBar()->showMessage("Failed to create SOURCE node", 3000);
    }
}

void Window::createOutputNode()
{
    if (!m_graph) {
        QMessageBox::warning(this,"No Graph","Graph facade not initialized.");
        return;
    }

    // Find a nice position in the view center
    QPointF viewCenter = m_view->mapToScene(m_view->viewport()->rect().center());

    // Add some randomization so multiple nodes don't overlap
    qreal randomX = QRandomGenerator::global()->bounded(-50, 50);
    qreal randomY = QRandomGenerator::global()->bounded(-50, 50);
    QPointF position = viewCenter + QPointF(randomX, randomY);

    // Create output node using Graph facade API
    QString nodeId = m_graph->createNode("SINK", position.x(), position.y());

    if (!nodeId.isEmpty()) {
        qDebug() << "Created output node" << nodeId << "at" << position;
        statusBar()->showMessage(QString("Created SINK node: %1").arg(nodeId), 2000);
    } else {
        qDebug() << "Failed to create output node";
        statusBar()->showMessage("Failed to create SINK node", 3000);
    }
}

void Window::createProcessorNode()
{
    if (!m_graph) {
        QMessageBox::warning(this,"No Graph","Graph facade not initialized.");
        return;
    }

    // Find a nice position in the view center
    QPointF viewCenter = m_view->mapToScene(m_view->viewport()->rect().center());

    // Add some randomization so multiple nodes don't overlap
    qreal randomX = QRandomGenerator::global()->bounded(-50, 50);
    qreal randomY = QRandomGenerator::global()->bounded(-50, 50);
    QPointF position = viewCenter + QPointF(randomX, randomY);

    // Create processor node using Graph facade API
    QString nodeId = m_graph->createNode("TRANSFORM", position.x(), position.y());

    if (!nodeId.isEmpty()) {
        qDebug() << "Created processor node" << nodeId << "at" << position;
        statusBar()->showMessage(QString("Created TRANSFORM node: %1").arg(nodeId), 2000);
    } else {
        qDebug() << "Failed to create processor node";
        statusBar()->showMessage("Failed to create TRANSFORM node", 3000);
    }
}

void Window::createNodeFromPalette(const QPointF& scenePos, const QString& nodeType,
                                  const QString& name, int inputSockets, int outputSockets)
{
    Q_UNUSED(inputSockets)  // Template system handles socket counts
    Q_UNUSED(outputSockets) // Template system handles socket counts

    qDebug() << "========================================";
    qDebug() << "Window: RECEIVED nodeDropped signal";
    qDebug() << "Window: Creating node from palette:";
    qDebug() << "  - Name:" << name;
    qDebug() << "  - Type:" << nodeType;
    qDebug() << "  - Position:" << scenePos;
    qDebug() << "Window: Calling Graph facade API";

    if (!m_graph) {
        qDebug() << "Window: Graph facade not initialized!";
        statusBar()->showMessage("Failed to create node - Graph not initialized", 3000);
        qDebug() << "========================================";
        return;
    }

    // Create node using Graph facade API (unified interface)
    QString nodeId = m_graph->createNode(nodeType, scenePos.x(), scenePos.y());

    if (!nodeId.isEmpty()) {
        qDebug() << "Window: Graph facade successfully created" << name << "node:" << nodeId;
        qDebug() << "Window: Node created at scene position:" << scenePos;
        qDebug() << "Window: Updating status bar";

        // Update status bar to reflect the new node
        updateStatusBar();
        statusBar()->showMessage(QString("Created %1 node: %2").arg(name).arg(nodeId), 2000);

        qDebug() << "Window: Node creation process completed successfully";
    } else {
        qDebug() << "Window: Graph facade FAILED to create" << name << "node";
        qDebug() << "Window: This may indicate invalid node type or scene issues";
        statusBar()->showMessage(QString("Failed to create %1 node").arg(name), 3000);
    }
    qDebug() << "========================================";
}

// ============================================================================
// Enhanced UI Implementation - Lookatme + Inkscape Status Bar Patterns
// ============================================================================

void Window::setupUI()
{
    setCentralWidget(m_view);
    
    // Set application icon and improve window appearance
    setWindowIcon(QIcon(":/icons/app-icon.png")); // Optional - if you have icons
    
    // Enable dock widget features
    setDockOptions(QMainWindow::AllowNestedDocks | 
                   QMainWindow::AllowTabbedDocks | 
                   QMainWindow::AnimatedDocks);
}

void Window::setupMenus()
{
    createFileMenu();
    createEditMenu();
    createViewMenu();
    createToolsMenu();
    createHelpMenu();
}

void Window::createFileMenu()
{
    m_fileMenu = menuBar()->addMenu("&File");
    
    // New file
    QAction* newAction = new QAction("&New", this);
    newAction->setShortcut(QKeySequence::New);
    newAction->setStatusTip("Create a new graph");
    connect(newAction, &QAction::triggered, this, &Window::newFile);
    m_fileMenu->addAction(newAction);
    
    // Open file
    QAction* openAction = new QAction("&Open...", this);
    openAction->setShortcut(QKeySequence::Open);
    openAction->setStatusTip("Open an existing graph");
    connect(openAction, &QAction::triggered, this, &Window::openFile);
    m_fileMenu->addAction(openAction);
    
    m_fileMenu->addSeparator();
    
    // Save file
    QAction* saveAction = new QAction("&Save", this);
    saveAction->setShortcut(QKeySequence::Save);
    saveAction->setStatusTip("Save the current graph");
    connect(saveAction, &QAction::triggered, this, &Window::saveFile);
    m_fileMenu->addAction(saveAction);
    
    // Save As
    QAction* saveAsAction = new QAction("Save &As...", this);
    saveAsAction->setShortcut(QKeySequence::SaveAs);
    saveAsAction->setStatusTip("Save the graph with a new name");
    connect(saveAsAction, &QAction::triggered, this, &Window::saveAsFile);
    m_fileMenu->addAction(saveAsAction);
    
    m_fileMenu->addSeparator();
    
    // Export
    QAction* exportAction = new QAction("&Export...", this);
    exportAction->setStatusTip("Export graph to various formats");
    connect(exportAction, &QAction::triggered, this, &Window::exportGraph);
    m_fileMenu->addAction(exportAction);
    
    m_fileMenu->addSeparator();
    
    // Exit
    QAction* exitAction = new QAction("E&xit", this);
    exitAction->setShortcut(QKeySequence::Quit);
    exitAction->setStatusTip("Exit the application");
    connect(exitAction, &QAction::triggered, this, &QWidget::close);
    m_fileMenu->addAction(exitAction);
}

void Window::createEditMenu()
{
    m_editMenu = menuBar()->addMenu("&Edit");
    
    // Undo/Redo placeholders for future implementation
    QAction* undoAction = new QAction("&Undo", this);
    undoAction->setShortcut(QKeySequence::Undo);
    undoAction->setEnabled(false); // TODO: Implement undo system
    m_editMenu->addAction(undoAction);
    
    QAction* redoAction = new QAction("&Redo", this);
    redoAction->setShortcut(QKeySequence::Redo);
    redoAction->setEnabled(false); // TODO: Implement redo system
    m_editMenu->addAction(redoAction);
    
    m_editMenu->addSeparator();
    
    // Selection operations
    QAction* selectAllAction = new QAction("Select &All", this);
    selectAllAction->setShortcut(QKeySequence::SelectAll);
    selectAllAction->setStatusTip("Select all nodes and edges");
    m_editMenu->addAction(selectAllAction);
    
    QAction* deselectAction = new QAction("&Deselect All", this);
    deselectAction->setShortcut(QKeySequence("Ctrl+D"));
    deselectAction->setStatusTip("Clear selection");
    m_editMenu->addAction(deselectAction);
    
    m_editMenu->addSeparator();
    
    // Delete
    QAction* deleteAction = new QAction("&Delete Selected", this);
    deleteAction->setShortcut(QKeySequence::Delete);
    deleteAction->setStatusTip("Delete selected nodes and edges");
    m_editMenu->addAction(deleteAction);
}

void Window::createViewMenu()
{
    m_viewMenu = menuBar()->addMenu("&View");
    
    // Zoom operations
    QAction* zoomInAction = new QAction("Zoom &In", this);
    zoomInAction->setShortcut(QKeySequence::ZoomIn);
    zoomInAction->setStatusTip("Zoom in to the graph");
    connect(zoomInAction, &QAction::triggered, this, &Window::zoomIn);
    m_viewMenu->addAction(zoomInAction);
    
    QAction* zoomOutAction = new QAction("Zoom &Out", this);
    zoomOutAction->setShortcut(QKeySequence::ZoomOut);
    zoomOutAction->setStatusTip("Zoom out from the graph");
    connect(zoomOutAction, &QAction::triggered, this, &Window::zoomOut);
    m_viewMenu->addAction(zoomOutAction);
    
    QAction* zoomFitAction = new QAction("Zoom to &Fit", this);
    zoomFitAction->setShortcut(QKeySequence("Ctrl+0"));
    zoomFitAction->setStatusTip("Fit entire graph in view");
    connect(zoomFitAction, &QAction::triggered, this, &Window::zoomFit);
    m_viewMenu->addAction(zoomFitAction);
    
    QAction* zoomResetAction = new QAction("&Reset Zoom", this);
    zoomResetAction->setShortcut(QKeySequence("Ctrl+1"));
    zoomResetAction->setStatusTip("Reset zoom to 100%");
    connect(zoomResetAction, &QAction::triggered, this, &Window::zoomReset);
    m_viewMenu->addAction(zoomResetAction);
    
    m_viewMenu->addSeparator();
    
    // Snap to Grid toggle
    QAction* snapToGridAction = new QAction("&Snap to Grid", this);
    snapToGridAction->setCheckable(true);
    snapToGridAction->setChecked(false);
    snapToGridAction->setStatusTip("Enable snap to grid for node positioning and layout");
    connect(snapToGridAction, &QAction::toggled, this, [this](bool checked) {
        if (m_scene) {
            m_scene->setSnapToGrid(checked);
            qDebug() << "Snap to grid:" << (checked ? "enabled" : "disabled");
        }
    });
    m_viewMenu->addAction(snapToGridAction);
    
    m_viewMenu->addSeparator();
    
    // Dock widget toggles will be added after dock widgets are created
}

void Window::createToolsMenu()
{
    m_toolsMenu = menuBar()->addMenu("&Tools");
    
    // Node creation submenu
    QMenu* createNodeMenu = m_toolsMenu->addMenu("&Create Node");
    createNodeMenu->addAction(m_addInputAction);
    createNodeMenu->addAction(m_addOutputAction);
    createNodeMenu->addAction(m_addProcessorAction);
    
    m_toolsMenu->addSeparator();
    
    QAction* validateAction = new QAction("&Validate Graph", this);
    validateAction->setStatusTip("Check graph for errors and inconsistencies");
    m_toolsMenu->addAction(validateAction);
    
    QAction* statisticsAction = new QAction("Graph &Statistics", this);
    statisticsAction->setStatusTip("Show detailed graph statistics");
    m_toolsMenu->addAction(statisticsAction);
    
    // All testing/automation goes through JavaScript + Graph facade
}

void Window::createHelpMenu()
{
    m_helpMenu = menuBar()->addMenu("&Help");
    
    QAction* aboutAction = new QAction("&About", this);
    aboutAction->setStatusTip("About this application");
    connect(aboutAction, &QAction::triggered, this, &Window::showAbout);
    m_helpMenu->addAction(aboutAction);
    
    QAction* aboutQtAction = new QAction("About &Qt", this);
    aboutQtAction->setStatusTip("About Qt Framework");
    connect(aboutQtAction, &QAction::triggered, qApp, &QApplication::aboutQt);
    m_helpMenu->addAction(aboutQtAction);
}

void Window::setupStatusBar()
{
    createStatusBarWidgets();
    connectStatusBarSignals();
}

void Window::createStatusBarWidgets()
{
    QStatusBar* statusBar = this->statusBar();
    statusBar->setStyleSheet(
        "QStatusBar {"
        "  border-top: 1px solid #bdc3c7;"
        "  background: #ecf0f1;"
        "}"
        "QStatusBar::item {"
        "  border: none;"
        "}"
    );
    
    // File info (leftmost)
    m_fileInfoLabel = new QLabel("No file loaded");
    m_fileInfoLabel->setStyleSheet("QLabel { color: #2c3e50; margin-right: 15px; }");
    statusBar->addWidget(m_fileInfoLabel);
    
    // Graph statistics
    m_graphStatsLabel = new QLabel("Nodes: 0 | Edges: 0");
    m_graphStatsLabel->setStyleSheet("QLabel { color: #27ae60; font-weight: bold; margin-right: 15px; }");
    statusBar->addWidget(m_graphStatsLabel);
    
    // Selection info
    m_selectionLabel = new QLabel("No selection");
    m_selectionLabel->setStyleSheet("QLabel { color: #8e44ad; margin-right: 15px; }");
    statusBar->addWidget(m_selectionLabel);
    
    // Add stretch to push remaining widgets to the right
    statusBar->addPermanentWidget(new QWidget(), 1);
    
    // Mouse position (right side)
    m_positionLabel = new QLabel("Position: (0, 0)");
    m_positionLabel->setStyleSheet("QLabel { color: #34495e; margin-right: 10px; }");
    statusBar->addPermanentWidget(m_positionLabel);
    
    // Zoom level (rightmost)
    m_zoomLabel = new QLabel("Zoom: 100%");
    m_zoomLabel->setStyleSheet("QLabel { color: #e74c3c; font-weight: bold; }");
    statusBar->addPermanentWidget(m_zoomLabel);
    
    // Operation progress (hidden by default)
    m_operationProgress = new QProgressBar();
    m_operationProgress->setVisible(false);
    m_operationProgress->setMaximumWidth(200);
    statusBar->addPermanentWidget(m_operationProgress);
}

void Window::connectStatusBarSignals()
{
    // Update status bar when scene changes
    connect(m_scene, &Scene::sceneChanged, this, &Window::updateStatusBar);
    
    // TODO: Connect view signals for mouse position and zoom updates
    // This would require extending the View class to emit these signals
}

void Window::setupDockWidgets()
{
    // Create node palette dock widget
    m_nodePaletteDock = new QDockWidget("Node Palette", this);
    m_nodePaletteDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    m_nodePaletteDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    
    // Create palette widget
    m_nodePalette = new NodePaletteWidget();
    m_nodePaletteDock->setWidget(m_nodePalette);
    
    // Add dock widget to left side
    addDockWidget(Qt::LeftDockWidgetArea, m_nodePaletteDock);
    
    // Node creation works via drag-and-drop: View::nodeDropped → Window::createNodeFromPalette()
    // Palette double-click signal currently unused (drag-and-drop is the primary workflow)
    
}

void Window::updateStatusBar()
{
    if (!m_scene) {
        return;
    }

    // Update graph statistics
    QVariantMap stats = m_graph->getGraphStats();
    int nodeCount = stats["nodeCount"].toInt();
    int edgeCount = stats["edgeCount"].toInt();
    qDebug() << "[FACADE-TEST] updateStatusBar: getGraphStats() returned nodes=" << nodeCount << ", edges=" << edgeCount;
    m_graphStatsLabel->setText(QString("Nodes: %1 | Edges: %2").arg(nodeCount).arg(edgeCount));
    
    // Update file info
    if (m_currentFile.isEmpty()) {
        m_fileInfoLabel->setText("No file loaded");
    } else {
        QFileInfo fileInfo(m_currentFile);
        m_fileInfoLabel->setText(QString("File: %1").arg(fileInfo.fileName()));
    }
    
    // Update selection info (placeholder for now)
    QList<QGraphicsItem*> selectedItems = m_scene->selectedItems();
    if (selectedItems.isEmpty()) {
        m_selectionLabel->setText("No selection");
    } else {
        m_selectionLabel->setText(QString("Selected: %1 items").arg(selectedItems.size()));
    }
}

// ============================================================================
// Slot Implementations
// ============================================================================

void Window::onSceneChanged()
{
    updateStatusBar();
}

void Window::onSelectionChanged()
{
    updateSelectionInfo();
}

void Window::updateSelectionInfo()
{
    if (!m_scene) {
        return;
    }

    const QList<Node*> nodes = m_scene->selectedNodes();
    const QList<Edge*> edges = m_scene->selectedEdges();
    const int nodeCount = nodes.size();
    const int edgeCount = edges.size();

    if (nodeCount == 0 && edgeCount == 0) {
        m_selectionLabel->setText("No selection");
        return;
    }

    QString selectionText;
    if (nodeCount > 0 && edgeCount > 0) {
        selectionText = QString("Selected: %1 nodes, %2 edges").arg(nodeCount).arg(edgeCount);
    } else if (nodeCount > 0) {
        selectionText = QString("Selected: %1 nodes").arg(nodeCount);
    } else if (edgeCount > 0) {
        selectionText = QString("Selected: %1 edges").arg(edgeCount);
    }

    m_selectionLabel->setText(selectionText);
}

bool Window::deleteSelection()
{
    if (!m_graph) {
        qWarning() << "Delete key pressed but graph not available";
        return false;
    }

    const QVariantList nodes = m_graph->getSelectedNodes();
    const QVariantList edges = m_graph->getSelectedEdges();

    if (edges.isEmpty() && nodes.isEmpty()) {
        qDebug() << "Delete key pressed - nothing selected";
        return false;
    }

    qDebug() << "Delete key pressed - deleting" << nodes.size() << "nodes and" << edges.size() << "edges";

    const bool deleted = m_graph->deleteSelection();
    if (!deleted) {
        qWarning() << "Delete key pressed - graph.deleteSelection() reported failure";
    }
    updateSelectionInfo();
    updateStatusBar();
    return deleted;
}

// ============================================================================
// Menu Action Implementations (Placeholders)
// ============================================================================

void Window::newFile()
{
    qDebug() << "New file requested";
    if (!m_scene) {
        return;
    }
    
    // Suspend autosave during destructive ops
    if (m_autosaveObserver) {
        m_autosaveObserver->setEnabled(false);
    }

    GraphSubject::beginBatch();
    m_graph->clearGraph();                        // hard clear
    setCurrentFile(QString());                    // forget current file
    updateStatusBar();
    GraphSubject::endBatch();

    // Commit clean state and resume autosave
    if (m_autosaveObserver) {
        m_autosaveObserver->saveNow();
        m_autosaveObserver->setEnabled(true);
    }
}

void Window::openFile()
{
    qDebug() << "=== FILE OPEN DIALOG ===";
    QString fileName = QFileDialog::getOpenFileName(this, "Open Graph", "", "XML Files (*.xml)");
    if (!fileName.isEmpty()) {
        qDebug() << "File selected:" << fileName;
        if (loadGraph(fileName)) {
            setCurrentFile(fileName);
            updateStatusBar();
            qDebug() << "File loaded successfully";
        } else {
            qDebug() << "File load FAILED";
        }
    } else {
        qDebug() << "File dialog cancelled";
    }
}

void Window::saveFile()
{
    qDebug() << "=== CTRL+S SAVE TRIGGERED ===";
    qDebug() << "Current file:" << (m_currentFile.isEmpty() ? "NONE (will show Save As dialog)" : m_currentFile);
    
    if (m_currentFile.isEmpty()) {
        qDebug() << "No current file - opening Save As dialog...";
        saveAsFile();
    } else {
        qDebug() << "Saving to current file:" << m_currentFile;
        if (saveGraph(m_currentFile)) {
            qDebug() << "Save successful";
            updateStatusBar();
        } else {
            qDebug() << "Save FAILED";
        }
    }
}

void Window::saveAsFile()
{
    QString fileName = QFileDialog::getSaveFileName(this, "Save Graph", "", "XML Files (*.xml)");
    if (!fileName.isEmpty()) {
        if (saveGraph(fileName)) {
            setCurrentFile(fileName);
            updateStatusBar();
        }
    }
}

void Window::exportGraph()
{
    // TODO: Implement export functionality
    QMessageBox::information(this, "Export", "Export functionality will be implemented in a future update.");
}

void Window::showAbout()
{
    QMessageBox::about(this, "About NodeGraph",
        "<h3>NodeGraph - Self-Serializing Node Editor</h3>"
        "<p>A professional node-based graph editor with self-serializing architecture.</p>"
        "<p><b>Features:</b></p>"
        "<ul>"
        "<li>Self-serializing nodes with libxml2 backend</li>"
        "<li>Observer pattern with automatic XML persistence</li>"
        "<li>Professional UI with docking panels</li>"
        "<li>Enhanced visual selection highlighting</li>"
        "<li>Drag-and-drop node creation</li>"
        "</ul>"
        "<p>Built with Qt5 and modern C++ patterns.</p>");
}

void Window::zoomIn()
{
    m_view->scale(1.2, 1.2);
    // TODO: Update zoom label
}

void Window::zoomOut()
{
    m_view->scale(0.8, 0.8);
    // TODO: Update zoom label
}

void Window::zoomFit()
{
    m_view->fitInView(m_scene->itemsBoundingRect(), Qt::KeepAspectRatio);
    // TODO: Update zoom label
}

void Window::zoomReset()
{
    m_view->resetTransform();
    // TODO: Update zoom label
}

/*
void Window::createNodeAtPosition(const QString& nodeType, const QPointF& scenePos)
{
    // Drag-and-drop node creation disabled for now
    // Focus on JavaScript integration
}
*/

// ============================================================================
// Event Handlers
// ============================================================================

void Window::showEvent(QShowEvent* event)
{
    QMainWindow::showEvent(event);

    if (!m_startupScript.isEmpty() && !m_startupScriptExecuted) {
        m_startupScriptExecuted = true;

        QTimer::singleShot(50, this, [this]() {
            qDebug() << "=== Executing startup script:" << m_startupScript;

            if (m_graph) {
                QJSValue result = m_graph->evalFile(m_startupScript);

                if (result.isError()) {
                    qCritical() << "Script error:" << result.toString();
                } else {
                    qDebug() << "Script executed successfully";
                }
            } else {
                qCritical() << "Graph facade not available";
            }

            qDebug() << "=== Script execution complete";
        });
    }
}

void Window::closeEvent(QCloseEvent* event)
{
    qDebug() << "PHASE1: Window shutdown initiated";
    
    // PHASE 1.2: Prepare scene for safe shutdown
    if (m_scene) {
        m_scene->prepareForShutdown();
    }
    
    // Accept the close event (no dirty state tracking yet)
    QMainWindow::closeEvent(event);
    
    qDebug() << "PHASE1: Window shutdown complete";
}

// ============================================================================
// Utility Functions
// ============================================================================

// Snapshot current scene as-is to a user-chosen PNG
// All testing/automation goes through JavaScript + Graph facade
// Drag-and-drop node creation works via View::nodeDropped signal → Window::createNodeFromPalette()
