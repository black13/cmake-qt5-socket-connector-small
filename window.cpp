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
    
    // Connect scene signals for status updates
    connect(m_scene, &Scene::sceneChanged, this, &Window::onSceneChanged);
    
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
}


void Window::keyPressEvent(QKeyEvent* event)
{
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
        // Delete key handled by individual QGraphicsItems (proper Qt architecture)
        qDebug() << "Delete key pressed - items will handle their own deletion";
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
        
        // Optional smoke test immediately after a successful load
        if (m_runForceLayoutTestOnLoad) {
            // modest size and a chain so attraction works
            runForceLayoutSmokeInternal(/*nodeCount*/ 30, /*connectSequential*/ true);
            // Restore the just-loaded file so the smoke test has no lasting effect
            restoreJustLoadedFile();
        }
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
    
    // Template System Tests - testing XML+template integration
    m_toolsMenu->addSeparator();
    QMenu* templateTestMenu = m_toolsMenu->addMenu("Template System Tests");
    
    QAction* testTemplateCreationAction = new QAction("Test Template Node Creation", this);
    testTemplateCreationAction->setStatusTip("Test NodeTypeTemplates system via GraphFactory");
    connect(testTemplateCreationAction, &QAction::triggered, this, &Window::testTemplateNodeCreation);
    templateTestMenu->addAction(testTemplateCreationAction);
    
    QAction* testTemplateConnectionsAction = new QAction("Test Template Edge Connections", this);
    testTemplateConnectionsAction->setStatusTip("Test edge creation between template-generated nodes");
    connect(testTemplateConnectionsAction, &QAction::triggered, this, &Window::testTemplateConnections);
    templateTestMenu->addAction(testTemplateConnectionsAction);
    
    // Automatic but visual tests
    m_toolsMenu->addSeparator();
    QMenu* testsMenu = m_toolsMenu->addMenu("&Tests");
    QAction* smokeAction = new QAction("Visual Smoke Test (build → snapshot → restore)", this);
    connect(smokeAction, &QAction::triggered, this, &Window::runVisualSmokeTest);
    testsMenu->addAction(smokeAction);
    QAction* snapAction = new QAction("Save Scene Snapshot...", this);
    connect(snapAction, &QAction::triggered, this, &Window::saveSceneSnapshot);
    testsMenu->addAction(snapAction);
    
    // Diagnostics / Smoke Tests (non-QtTest, in-app)
    m_toolsMenu->addSeparator();
    QMenu* smokeMenu = m_toolsMenu->addMenu("Diagnostics / Smoke Tests");
    auto* smokeConnect = new QAction("Smoke: Connect SOURCE → SINK", this);
    connect(smokeConnect, &QAction::triggered, this, &Window::testConnectSourceToSink);
    smokeMenu->addAction(smokeConnect);

    auto* smokeClear = new QAction("Smoke: ClearGraph removes all", this);
    connect(smokeClear, &QAction::triggered, this, &Window::testClearGraphRemovesEverything);
    smokeMenu->addAction(smokeClear);

    auto* smokeNewFile = new QAction("Smoke: New File resets", this);
    connect(smokeNewFile, &QAction::triggered, this, &Window::testNewFileResetsState);
    smokeMenu->addAction(smokeNewFile);

    auto* smokeAll = new QAction("Run All Smoke Tests", this);
    connect(smokeAll, &QAction::triggered, this, &Window::runAllSmokes);
    smokeMenu->addAction(smokeAll);
    
    // Template system smoke test runner
    QAction* runSmoke = new QAction("Run Smoke Tests", this);
    connect(runSmoke, &QAction::triggered, this, &Window::runSmokeTests);
    m_toolsMenu->addSeparator();
    m_toolsMenu->addAction(runSmoke);
    
    // Arrange submenu with auto-layout
    m_toolsMenu->addSeparator();
    QMenu* arrangeMenu = m_toolsMenu->addMenu("&Arrange");
    
    QAction* autoAnnealSel = new QAction("Auto Layout (Annealing) — &Selection", this);
    autoAnnealSel->setStatusTip("Spread out selected nodes using simulated annealing");
    connect(autoAnnealSel, &QAction::triggered, this, &Window::arrangeAutoAnnealSelection);
    arrangeMenu->addAction(autoAnnealSel);
    
    QAction* autoAnnealAll = new QAction("Auto Layout (Annealing) — &All Nodes", this);
    autoAnnealAll->setStatusTip("Spread out all nodes using simulated annealing");
    connect(autoAnnealAll, &QAction::triggered, this, &Window::arrangeAutoAnnealAll);
    arrangeMenu->addAction(autoAnnealAll);
    
    // Load-time Force-Directed Layout Smoke Test
    arrangeMenu->addSeparator();
    QAction* forceOnLoad = new QAction("Enable Force-Layout Smoke Test on &Load", this);
    forceOnLoad->setCheckable(true);
    forceOnLoad->setChecked(false);
    connect(forceOnLoad, &QAction::toggled, this, &Window::toggleForceLayoutTestOnLoad);
    arrangeMenu->addAction(forceOnLoad);

    QAction* forceRunNow = new QAction("Run Force-Layout Smoke Test &Now...", this);
    connect(forceRunNow, &QAction::triggered, this, &Window::runForceLayoutSmokeNow);
    arrangeMenu->addAction(forceRunNow);
    
    // Debug Force Layout with 3 Nodes
    arrangeMenu->addSeparator();
    QAction* debugForce3Action = new QAction("Debug Force Layout (3 Nodes)...", this);
    debugForce3Action->setStatusTip("Animated debug of force layout with 3 test nodes");
    connect(debugForce3Action, &QAction::triggered, this, [this]() {
        if (m_scene) {
            m_scene->debugForceLayout3Nodes();
        }
    });
    arrangeMenu->addAction(debugForce3Action);
    
#if ENABLE_JS
    // JavaScript console and scripting (only when enabled)
    m_toolsMenu->addSeparator();
    
    QAction* jsConsoleAction = new QAction("JavaScript &Console...", this);
    jsConsoleAction->setStatusTip("Open JavaScript console for graph automation");
    // TODO: Connect to JavaScript console dialog when implemented
    m_toolsMenu->addAction(jsConsoleAction);
    
    QAction* runScriptAction = new QAction("&Run Script File...", this);
    runScriptAction->setStatusTip("Execute JavaScript file for graph automation");
    // TODO: Connect to script file runner when implemented  
    m_toolsMenu->addAction(runScriptAction);
#endif
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
    
    // Connect palette signals
    connect(m_nodePalette, &NodePaletteWidget::nodeCreationRequested, 
            this, &Window::onNodeCreationRequested);
    
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
    
    QList<QGraphicsItem*> selectedItems = m_scene->selectedItems();
    if (selectedItems.isEmpty()) {
        m_selectionLabel->setText("No selection");
    } else {
        int nodeCount = 0;
        int edgeCount = 0;
        
        for (QGraphicsItem* item : selectedItems) {
            if (qgraphicsitem_cast<Node*>(item)) {
                nodeCount++;
            } else if (qgraphicsitem_cast<Edge*>(item)) {
                edgeCount++;
            }
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
// Template System Tests - Validate NodeTypeTemplates + GraphFactory integration
// ============================================================================

void Window::testTemplateNodeCreation()
{
    qDebug() << "\n" << __FUNCTION__ << "- TEMPLATE SYSTEM: Starting template system validation test";

    // Show collection state BEFORE clearing
    QVariantMap statsBefore = m_graph->getGraphStats();
    qDebug() << "[FACADE-TEST]" << __FUNCTION__ << "- getGraphStats() BEFORE clear: nodes=" << statsBefore["nodeCount"].toInt() << ", edges=" << statsBefore["edgeCount"].toInt();
    qDebug() << __FUNCTION__ << "- COLLECTION STATE BEFORE: Scene has" << statsBefore["nodeCount"].toInt() << "nodes," << statsBefore["edgeCount"].toInt() << "edges";
    qDebug() << __FUNCTION__ << "- COLLECTION STATE BEFORE: Qt scene has" << m_scene->items().size() << "total items";

    // Clear existing graph for clean test - ISSUE 3: Use safe clear ordering
    m_graph->clearGraph();
    qDebug() << __FUNCTION__ << "- TEMPLATE SYSTEM: Cleared scene safely for testing";

    // Show collection state AFTER clearing
    QVariantMap statsAfter = m_graph->getGraphStats();
    qDebug() << "[FACADE-TEST]" << __FUNCTION__ << "- getGraphStats() AFTER clear: nodes=" << statsAfter["nodeCount"].toInt() << ", edges=" << statsAfter["edgeCount"].toInt() << "(expect 0,0)";
    qDebug() << __FUNCTION__ << "- COLLECTION STATE AFTER: Scene has" << statsAfter["nodeCount"].toInt() << "nodes," << statsAfter["edgeCount"].toInt() << "edges";
    qDebug() << __FUNCTION__ << "- COLLECTION STATE AFTER: Qt scene has" << m_scene->items().size() << "total items";

    // CRASH VALIDATION: Check for stale pointer problem
    int nodeCount = statsAfter["nodeCount"].toInt();
    int edgeCount = statsAfter["edgeCount"].toInt();
    if (nodeCount > 0 && m_scene->items().size() == 0) {
        qWarning() << __FUNCTION__ << "- CRASH RISK DETECTED: Scene hash maps contain" << nodeCount << "node pointers but Qt scene is empty!";
        qWarning() << __FUNCTION__ << "- CRASH CAUSE: Autosave will try to access deleted Node objects";
        qWarning() << __FUNCTION__ << "- SOLUTION NEEDED: Scene::clear() should also clear hash maps, not just Qt items";
    }
    if (edgeCount > 0 && m_scene->items().size() == 0) {
        qWarning() << __FUNCTION__ << "- CRASH RISK DETECTED: Scene hash maps contain" << edgeCount << "edge pointers but Qt scene is empty!";
        qWarning() << __FUNCTION__ << "- CRASH CAUSE: Autosave will try to access deleted Edge objects";
    }
    
    // Test 1: Create different node types using template system
    qDebug() << __FUNCTION__ << "- TEMPLATE SYSTEM: Testing all built-in node types...";
    
    QStringList testTypes = {"SOURCE", "SINK", "TRANSFORM", "MERGE", "SPLIT"};
    QStringList createdNodes;
    int successCount = 0;
    
    for (int i = 0; i < testTypes.size(); ++i) {
        const QString& nodeType = testTypes[i];
        
        // Use template system via GraphFactory unified creation method
        Node* node = m_factory->createNode(nodeType, QPointF(100 + i * 150, 100));
        
        if (node) {
            createdNodes << node->getId().toString(QUuid::WithoutBraces);
            successCount++;
            qDebug() << __FUNCTION__ << "- TEMPLATE SYSTEM: Successfully created" << nodeType << "node with ID" << node->getId().toString(QUuid::WithoutBraces);
        } else {
            qDebug() << __FUNCTION__ << "- TEMPLATE SYSTEM: FAILED to create" << nodeType << "node";
        }
    }
    
    // Test 2: Test invalid node type
    qDebug() << __FUNCTION__ << "- TEMPLATE SYSTEM: Testing invalid node type rejection...";
    Node* invalidNode = m_factory->createNode("INVALID_TYPE", QPointF(600, 100));
    bool invalidRejected = (invalidNode == nullptr);
    
    // Get final scene stats
    int finalNodeCount = m_graph->getGraphStats()["nodeCount"].toInt();
    
    // Update status bar
    updateStatusBar();
    
    // Show results
    QMessageBox::information(this, "Template System Validation Test", 
        QString("Template System Test Completed!\n\n"
                "TEMPLATE SYSTEM RESULTS:\n"
                "[OK] Built-in types tested: %1\n"
                "[OK] Successful creations: %2\n"
                "[OK] Invalid type rejected: %3\n"
                "[OK] Final scene count: %4 nodes\n\n"
                "Created node types:\n"
                "- SOURCE: %5\n"
                "- SINK: %6\n" 
                "- TRANSFORM: %7\n"
                "- MERGE: %8\n"
                "- SPLIT: %9\n\n"
                "Check console for detailed TEMPLATE SYSTEM logs!")
        .arg(testTypes.size())
        .arg(successCount)
        .arg(invalidRejected ? "YES" : "NO - ERROR")
        .arg(finalNodeCount)
        .arg(createdNodes.size() > 0 ? "SUCCESS" : "FAILED")
        .arg(createdNodes.size() > 1 ? "SUCCESS" : "FAILED")
        .arg(createdNodes.size() > 2 ? "SUCCESS" : "FAILED")
        .arg(createdNodes.size() > 3 ? "SUCCESS" : "FAILED")
        .arg(createdNodes.size() > 4 ? "SUCCESS" : "FAILED"));
        
    qDebug() << __FUNCTION__ << "- TEMPLATE SYSTEM: Test completed. Created" << successCount << "of" << testTypes.size() << "node types";
}

void Window::testTemplateConnections()
{
    qDebug() << "\n" << __FUNCTION__ << "- TEMPLATE SYSTEM: Starting edge connection test";

    // Show collection state BEFORE clearing
    QVariantMap statsBefore = m_graph->getGraphStats();
    qDebug() << __FUNCTION__ << "- COLLECTION STATE BEFORE: Scene has" << statsBefore["nodeCount"].toInt() << "nodes," << statsBefore["edgeCount"].toInt() << "edges";
    qDebug() << __FUNCTION__ << "- COLLECTION STATE BEFORE: Qt scene has" << m_scene->items().size() << "total items";

    // Clear existing graph for clean test - ISSUE 3: Use safe clear ordering
    m_graph->clearGraph();
    qDebug() << __FUNCTION__ << "- TEMPLATE SYSTEM: Cleared scene safely for edge testing";

    // Show collection state AFTER clearing
    QVariantMap statsAfter = m_graph->getGraphStats();
    qDebug() << __FUNCTION__ << "- COLLECTION STATE AFTER: Scene has" << statsAfter["nodeCount"].toInt() << "nodes," << statsAfter["edgeCount"].toInt() << "edges";
    qDebug() << __FUNCTION__ << "- COLLECTION STATE AFTER: Qt scene has" << m_scene->items().size() << "total items";

    // CRASH VALIDATION: Check for stale pointer problem
    int nodeCount = statsAfter["nodeCount"].toInt();
    int edgeCount = statsAfter["edgeCount"].toInt();
    if (nodeCount > 0 && m_scene->items().size() == 0) {
        qWarning() << __FUNCTION__ << "- CRASH RISK DETECTED: Scene hash maps contain" << nodeCount << "node pointers but Qt scene is empty!";
        qWarning() << __FUNCTION__ << "- CRASH CAUSE: Autosave will try to access deleted Node objects";
        qWarning() << __FUNCTION__ << "- SOLUTION NEEDED: Scene::clear() should also clear hash maps, not just Qt items";
    }
    if (edgeCount > 0 && m_scene->items().size() == 0) {
        qWarning() << __FUNCTION__ << "- CRASH RISK DETECTED: Scene hash maps contain" << edgeCount << "edge pointers but Qt scene is empty!";
        qWarning() << __FUNCTION__ << "- CRASH CAUSE: Autosave will try to access deleted Edge objects";
    }
    
    // Create nodes for connection testing using template system
    qDebug() << __FUNCTION__ << "- TEMPLATE SYSTEM: Creating nodes for connection chain...";
    
    Node* sourceNode = m_factory->createNode("SOURCE", QPointF(100, 100));      // 0 inputs, 1 output
    Node* transformNode = m_factory->createNode("TRANSFORM", QPointF(300, 100)); // 1 input, 1 output  
    Node* mergeNode = m_factory->createNode("MERGE", QPointF(500, 100));        // 2 inputs, 1 output
    Node* sinkNode = m_factory->createNode("SINK", QPointF(700, 100));          // 1 input, 0 outputs
    
    if (!sourceNode || !transformNode || !mergeNode || !sinkNode) {
        qDebug() << __FUNCTION__ << "- TEMPLATE SYSTEM: FAILED to create required nodes for edge testing";
        QMessageBox::warning(this, "Template Connection Test Failed", "Could not create required nodes for testing");
        return;
    }
    
    qDebug() << __FUNCTION__ << "- TEMPLATE SYSTEM: Created 4 nodes successfully";
    qDebug() << __FUNCTION__ << "- SOURCE node:" << sourceNode->getId().toString(QUuid::WithoutBraces) << "sockets:" << sourceNode->getSocketCount();
    qDebug() << __FUNCTION__ << "- TRANSFORM node:" << transformNode->getId().toString(QUuid::WithoutBraces) << "sockets:" << transformNode->getSocketCount();
    qDebug() << __FUNCTION__ << "- MERGE node:" << mergeNode->getId().toString(QUuid::WithoutBraces) << "sockets:" << mergeNode->getSocketCount();
    qDebug() << __FUNCTION__ << "- SINK node:" << sinkNode->getId().toString(QUuid::WithoutBraces) << "sockets:" << sinkNode->getSocketCount();
    
    // Test edge creation using GraphFactory
    qDebug() << __FUNCTION__ << "- TEMPLATE SYSTEM: Creating edge connections...";
    
    QStringList edgeResults;
    int edgeSuccessCount = 0;
    
    // Edge 1: SOURCE (output 0) -> TRANSFORM (input 0)
    qDebug() << __FUNCTION__ << "- TEMPLATE SYSTEM: Connecting SOURCE->TRANSFORM";
    QString edgeId1 = m_graph->connectNodes(
        sourceNode->getId().toString(QUuid::WithoutBraces), 0,
        transformNode->getId().toString(QUuid::WithoutBraces), 0
    );
    if (!edgeId1.isEmpty()) {
        edgeResults << "SOURCE->TRANSFORM: SUCCESS";
        edgeSuccessCount++;
        qDebug() << __FUNCTION__ << "- TEMPLATE SYSTEM: Edge 1 created with ID" << edgeId1;
    } else {
        edgeResults << "SOURCE->TRANSFORM: FAILED";
        qDebug() << __FUNCTION__ << "- TEMPLATE SYSTEM: FAILED to create SOURCE->TRANSFORM edge";
    }
    
    // Edge 2: TRANSFORM (output 1) -> MERGE (input 0)
    qDebug() << __FUNCTION__ << "- TEMPLATE SYSTEM: Connecting TRANSFORM->MERGE";
    QString edgeId2 = m_graph->connectNodes(
        transformNode->getId().toString(QUuid::WithoutBraces), 1,
        mergeNode->getId().toString(QUuid::WithoutBraces), 0
    );
    if (!edgeId2.isEmpty()) {
        edgeResults << "TRANSFORM->MERGE: SUCCESS";
        edgeSuccessCount++;
        qDebug() << __FUNCTION__ << "- TEMPLATE SYSTEM: Edge 2 created with ID" << edgeId2;
    } else {
        edgeResults << "TRANSFORM->MERGE: FAILED";
        qDebug() << __FUNCTION__ << "- TEMPLATE SYSTEM: FAILED to create TRANSFORM->MERGE edge";
    }
    
    // Edge 3: MERGE (output 2) -> SINK (input 0)
    qDebug() << __FUNCTION__ << "- TEMPLATE SYSTEM: Connecting MERGE->SINK";
    QString edgeId3 = m_graph->connectNodes(
        mergeNode->getId().toString(QUuid::WithoutBraces), 2,
        sinkNode->getId().toString(QUuid::WithoutBraces), 0
    );
    if (!edgeId3.isEmpty()) {
        edgeResults << "MERGE->SINK: SUCCESS";
        edgeSuccessCount++;
        qDebug() << __FUNCTION__ << "- TEMPLATE SYSTEM: Edge 3 created with ID" << edgeId3;
    } else {
        edgeResults << "MERGE->SINK: FAILED";
        qDebug() << __FUNCTION__ << "- TEMPLATE SYSTEM: FAILED to create MERGE->SINK edge";
    }

    // Test invalid connection (wrong socket indices)
    qDebug() << __FUNCTION__ << "- TEMPLATE SYSTEM: Testing invalid connection rejection...";
    QString invalidEdgeId = m_graph->connectNodes(
        sourceNode->getId().toString(QUuid::WithoutBraces), 999,
        sinkNode->getId().toString(QUuid::WithoutBraces), 0
    ); // Invalid socket index
    bool invalidRejected = invalidEdgeId.isEmpty();
    
    // Get final scene stats
    QVariantMap finalStats = m_graph->getGraphStats();
    int finalNodeCount = finalStats["nodeCount"].toInt();
    int finalEdgeCount = finalStats["edgeCount"].toInt();
    
    // Update status bar
    updateStatusBar();
    
    // Show results
    QString edgeResultsText = edgeResults.join("\n");
    QMessageBox::information(this, "Template System Edge Connection Test", 
        QString("Template System Edge Test Completed!\n\n"
                "EDGE CONNECTION RESULTS:\n"
                "[OK] Nodes created: 4 (SOURCE, TRANSFORM, MERGE, SINK)\n"
                "[OK] Successful edge connections: %1/3\n"
                "[OK] Invalid connection rejected: %2\n"
                "[OK] Final scene: %3 nodes, %4 edges\n\n"
                "Connection Details:\n%5\n\n"
                "This demonstrates the API patterns needed for:\n"
                "- JavaScript: graph.createNode(type, x, y)\n"
                "- JavaScript: graph.connect(fromNode, fromSocket, toNode, toSocket)\n\n"
                "Check console for detailed TEMPLATE SYSTEM logs!")
        .arg(edgeSuccessCount)
        .arg(invalidRejected ? "YES" : "NO - ERROR")
        .arg(finalNodeCount)
        .arg(finalEdgeCount)
        .arg(edgeResultsText));
        
    qDebug() << __FUNCTION__ << "- TEMPLATE SYSTEM: Edge test completed." << edgeSuccessCount << "edges created," << finalEdgeCount << "total in scene";
    qDebug() << __FUNCTION__ << "- API PATTERNS IDENTIFIED: createNode(type,x,y) + createEdge(fromNode,fromSocket,toNode,toSocket)";
}

// ============================================================================
// Diagnostics / Smoke Tests
// ============================================================================

void Window::testConnectSourceToSink()
{
    if (!m_factory) {
        QMessageBox::warning(this, "Smoke: Connect",
                             "No GraphFactory available. Did Window::adoptFactory() run?");
        return;
    }
    // Start from a clean scene
    m_graph->clearGraph(); // safe, typed clear
    // Create nodes
    Node* src = m_factory->createNode("SOURCE", QPointF(-150, 0));
    Node* snk = m_factory->createNode("SINK",   QPointF( 150, 0));
    bool okCreated = (src && snk);
    // Connect 0→0 via unified path
    bool okEdge = false;
    if (okCreated) {
        Socket* out0 = src->getSocketByIndex(0);
        Socket* in0  = snk->getSocketByIndex(0);
        if (out0 && in0) {
            auto* e = m_factory->connectSockets(out0, in0); // same in-memory path used by UI/ghost edge
            okEdge = (e != nullptr);
        }
    }
    updateStatusBar();
    const QString verdict = (okCreated && okEdge) ? "PASS" : "FAIL";
    QMessageBox::information(this, "Smoke: Connect SOURCE → SINK",
        QString("Nodes created: %1\nEdge created: %2\n\nResult: %3")
            .arg(okCreated ? "YES" : "NO")
            .arg(okEdge ? "YES" : "NO")
            .arg(verdict));
}

void Window::testClearGraphRemovesEverything()
{
    if (!m_factory) {
        QMessageBox::warning(this, "Smoke: ClearGraph",
                             "No GraphFactory available. Did Window::adoptFactory() run?");
        return;
    }
    // Seed a few items
    for (int i=0;i<5;++i) {
        Node* a = m_factory->createNode("SOURCE", QPointF(-200, i*40));
        Node* b = m_factory->createNode("SINK",   QPointF( 200, i*40));
        if (!a || !b) {
            continue;
        }
        Socket* out0 = a->getSocketByIndex(0);
        Socket* in0  = b->getSocketByIndex(0);
        if (out0 && in0) {
            (void)m_factory->connectSockets(out0, in0);
        }
    }
    QVariantMap beforeStats = m_graph->getGraphStats();
    int beforeNodes = beforeStats["nodeCount"].toInt();
    int beforeEdges = beforeStats["edgeCount"].toInt();
    int beforeItems = m_scene->items().size();

    // Action
    m_graph->clearGraph();  // your safe, typed clear

    // Assert
    QVariantMap afterStats = m_graph->getGraphStats();
    int afterNodes = afterStats["nodeCount"].toInt();
    int afterEdges = afterStats["edgeCount"].toInt();
    bool itemsEmpty = m_scene->items().isEmpty();
    updateStatusBar();
    const bool pass = (afterNodes==0 && afterEdges==0 && itemsEmpty);
    QMessageBox::information(this, "Smoke: ClearGraph removes all",
        QString("Before: %1 nodes / %2 edges / %3 items\n"
                "After:  %4 nodes / %5 edges / items empty=%6\n\nResult: %7")
            .arg(beforeNodes).arg(beforeEdges).arg(beforeItems)
            .arg(afterNodes).arg(afterEdges).arg(itemsEmpty ? "YES":"NO")
            .arg(pass ? "PASS" : "FAIL"));
}

void Window::testNewFileResetsState()
{
    if (!m_factory) {
        QMessageBox::warning(this, "Smoke: New File",
                             "No GraphFactory available. Did Window::adoptFactory() run?");
        return;
    }
    // Seed graph + set fake filename
    Node* a = m_factory->createNode("SOURCE", QPointF(-60,0));
    Node* b = m_factory->createNode("SINK",   QPointF( 60,0));
    if (a && b) {
        auto* e = m_factory->connectSockets(a->getSocketByIndex(0), b->getSocketByIndex(0));
        Q_UNUSED(e);
    }
    setCurrentFile(QStringLiteral("temp_for_newfile_test.xml"));
    updateStatusBar();

    // Call the action under test
    this->newFile(); // should clear scene, file, and update UI

    // Expectations: scene empty + currentFile empty
    QVariantMap stats = m_graph->getGraphStats();
    const int nodes = stats["nodeCount"].toInt();
    const int edges = stats["edgeCount"].toInt();
    const bool fileCleared = getCurrentFile().isEmpty();
    const bool pass = (nodes==0 && edges==0 && fileCleared);
    QMessageBox::information(this, "Smoke: New File resets",
        QString("After New:\nNodes=%1  Edges=%2  CurrentFile empty=%3\n\nResult: %4\n\n%5")
            .arg(nodes).arg(edges).arg(fileCleared ? "YES":"NO")
            .arg(pass ? "PASS" : "FAIL")
            .arg(pass ? "" : "Hint: implement Window::newFile() to clear scene, clear current file, and update status bar."));
}

void Window::runAllSmokes()
{
    // Simple sequencer: run individual tests; each shows its own dialog.
    testConnectSourceToSink();
    testClearGraphRemovesEverything();
    testNewFileResetsState();
}

void Window::runSmokeTests()
{
    // Defensive: ensure factory present
    if (!m_factory) {
        QMessageBox::warning(this, "Factory Missing", "Adopt a GraphFactory before running tests.");
        return;
    }

    // Batch tests to keep observers quiet; autosave suspended briefly
    if (m_autosaveObserver) {
        m_autosaveObserver->setEnabled(false);
    }
    GraphSubject::beginBatch();

    testTemplateNodeCreation();
    testTemplateConnections();

    GraphSubject::endBatch();
    if (m_autosaveObserver) {
        m_autosaveObserver->saveNow();
        m_autosaveObserver->setEnabled(true);
    }
}

void Window::arrangeAutoAnnealSelection()
{
    if (!m_scene) {
        return;
    }
    
    if (m_autosaveObserver) {
        m_autosaveObserver->setEnabled(false);
    }
    m_scene->autoLayoutAnneal(/*selectionOnly=*/true, /*iters=*/2000, /*t0=*/1.0, /*t1=*/0.01);
    if (m_autosaveObserver) { 
        m_autosaveObserver->saveNow(); 
        m_autosaveObserver->setEnabled(true); 
    }
    updateStatusBar();
}

void Window::arrangeAutoAnnealAll()
{
    if (!m_scene) {
        return;
    }
    
    if (m_autosaveObserver) {
        m_autosaveObserver->setEnabled(false);
    }
    m_scene->autoLayoutAnneal(/*selectionOnly=*/false, /*iters=*/2500, /*t0=*/1.2, /*t1=*/0.02);
    if (m_autosaveObserver) { 
        m_autosaveObserver->saveNow(); 
        m_autosaveObserver->setEnabled(true); 
    }
    updateStatusBar();
}

void Window::toggleForceLayoutTestOnLoad(bool on)
{
    m_runForceLayoutTestOnLoad = on;
    statusBar()->showMessage(QString("Force-layout smoke test on load: %1")
                             .arg(on ? "ENABLED" : "DISABLED"), 1500);
}

void Window::runForceLayoutSmokeNow()
{
    // Pick a modest default (change to a dialog later if you want)
    const int N = 40;               // number of random nodes
    const bool connectChain = true; // connect as a simple chain so edges exist
    runForceLayoutSmokeInternal(N, connectChain);
}

void Window::runForceLayoutSmokeInternal(int nodeCount, bool connectSequential)
{
    if (!m_factory || !m_scene) {
        QMessageBox::warning(this, "Smoke Test", "Factory/Scene not ready.");
        return;
    }

    // Keep autosave quiet; batch edits
    if (m_autosaveObserver) {
        m_autosaveObserver->setEnabled(false);
    }
    GraphSubject::beginBatch();

    // Start from a clean slate (we'll restore the file if we were called from load)
    m_graph->clearGraph();

    // Random drop: mix of SOURCE / TRANSFORM / SINK (adjust to your template names)
    static const QStringList types = { "SOURCE", "TRANSFORM", "SINK" };
    QRandomGenerator* rng = QRandomGenerator::global();

    const int N = qMax(2, nodeCount);
    const QRectF box(-400, -300, 800, 600); // initial scatter area
    QVector<Node*> created; created.reserve(N);

    for (int i=0; i<N; ++i) {
        const QString& t = types.at(rng->bounded(types.size()));
        const qreal x = box.left() + rng->generateDouble() * box.width();
        const qreal y = box.top()  + rng->generateDouble() * box.height();
        if (Node* n = m_factory->createNode(t, QPointF(x, y))) {
            created.push_back(n);
        }
    }

    // Optionally connect nodes as a simple chain so force-directed has attractions
    int edgesMade = 0;
    if (connectSequential && created.size() > 1) {
        for (int i=0; i<created.size()-1; ++i) {
            Node* a = created[i];
            Node* b = created[i+1];
            if (!a || !b) {
            continue;
        }
            Socket* out0 = a->getSocketByIndex(0);
            Socket* in0  = b->getSocketByIndex(0);
            if (out0 && in0) {
                if (m_factory->connectSockets(out0, in0)) {
                    ++edgesMade;
                }
            }
        }
    }

    GraphSubject::endBatch();

    // Run the force-directed layout over ALL nodes
    // (uses your Scene::autoLayoutForceDirected; snaps at end if snap-to-grid is enabled)
    m_scene->autoLayoutForceDirected(/*selectionOnly*/ false, /*iters*/ 350, /*cooling*/ 0.92);

    // Summarize
    QVariantMap stats = m_graph->getGraphStats();
    const int nodes = stats["nodeCount"].toInt();
    const int edges = stats["edgeCount"].toInt();
    QMessageBox::information(this, "Force-Layout Smoke Test",
        QString("Created %1 nodes and %2 edges.\n"
                "Applied force-directed layout.\n"
                "This scene will %3 be persisted.")
            .arg(nodes)
            .arg(edges)
            .arg(m_runForceLayoutTestOnLoad ? "NOT" : "only if you save"));

    // If this was a manual run, do nothing further.
    // If called from loadGraph with 'on load' toggle, restoreJustLoadedFile() will be called by the caller.
    if (m_autosaveObserver) { m_autosaveObserver->saveNow(); m_autosaveObserver->setEnabled(true); }
}

void Window::restoreJustLoadedFile()
{
    // If there's a "current file", reload it to leave no side effects from the smoke test
    const QString path = getCurrentFile();
    if (path.isEmpty() || !QFileInfo::exists(path)) {
        // No file to restore — just clear to a blank scene
        GraphSubject::beginBatch();
        m_graph->clearGraph();
        setCurrentFile(QString());
        GraphSubject::endBatch();
        updateStatusBar();
        return;
    }

    // Reload the file silently (autosave gated)
    if (m_autosaveObserver) {
        m_autosaveObserver->setEnabled(false);
    }
    // Graph facade handles clearing, batch mode internally
    m_graph->loadFromFile(path);
    if (m_autosaveObserver) { m_autosaveObserver->saveNow(); m_autosaveObserver->setEnabled(true); }
    updateStatusBar();
}

// Renders the scene deterministically (antialiasing off) to an image
QImage Window::renderSceneImage(const QRectF& viewRect, const QSize& size) const
{
    QImage img(size, QImage::Format_ARGB32_Premultiplied);
    img.fill(Qt::white);
    QPainter p(&img);
    p.setRenderHint(QPainter::Antialiasing, false);
    m_scene->render(&p, QRectF(QPointF(0,0), QSizeF(size)), viewRect);
    p.end();
    return img;
}

// Reload current file (if any) so tests leave no trace; else clear
void Window::restoreCurrentFile()
{
    if (m_currentFile.isEmpty()) {
        m_graph->clearGraph();                    // safe clear (typed registries first)
        updateStatusBar();
        return;
    }
    // Silently reload
    m_graph->clearGraph();
    (void)loadGraph(m_currentFile);               // already sets title & status inside
}

// Visual Smoke Test: build a small deterministic graph, snapshot it, then restore
void Window::runVisualSmokeTest()
{
    if (!m_factory || !m_scene) {
        QMessageBox::warning(this, "Smoke Test", "Factory/Scene not ready.");
        return;
    }

    // 1) remember what's open
    const QString fileBefore = m_currentFile;

    // 2) start clean
    m_graph->clearGraph();

    // 3) build a deterministic sample (no random needed)
    QList<Node*> nodes;
    const QRectF box(-400, -300, 800, 600);
    const QStringList types = {"SOURCE","TRANSFORM","SINK","MERGE","SPLIT"};
    for (int i=0; i<30; ++i) {
        const QString t = types.at(i % types.size());
        // deterministic "random-ish" scatter using a tiny LCG
        quint32 r = 1664525u * (1234u + i*977u) + 1013904223u;
        qreal rx = (r % 10000) / 10000.0; r = 1664525u * r + 1013904223u;
        qreal ry = (r % 10000) / 10000.0;
        QPointF pos(box.left() + rx*box.width(), box.top() + ry*box.height());
        if (Node* n = m_factory->createNode(t, pos)) {
            nodes.push_back(n);
        }
    }
    // simple chain so edges exist (if sockets permit index 0)
    int edgesMade = 0;
    for (int i=0; i+1<nodes.size(); ++i) {
        QString edgeId = m_graph->connectNodes(
            nodes[i]->getId().toString(QUuid::WithoutBraces), 0,
            nodes[i+1]->getId().toString(QUuid::WithoutBraces), 0
        );
        if (!edgeId.isEmpty()) {
            ++edgesMade;
        }
    }

    // 4) render snapshot
    const QRectF viewRect(-500, -400, 1000, 800);
    QImage img = renderSceneImage(viewRect, QSize(1000, 800));

    // 5) save to a predictable place
    QDir out(QStandardPaths::writableLocation(QStandardPaths::PicturesLocation));
    if (!out.exists("NodeGraphTests")) {
        out.mkdir("NodeGraphTests");
    }
    const QString ts = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
    const QString path = out.filePath(QString("NodeGraphTests/smoke_%1_nodes%2_edges%3.png")
                                      .arg(ts).arg(nodes.size()).arg(edgesMade));
    img.save(path);

    // 6) show a quick summary
    QMessageBox::information(this, "Visual Smoke Test",
        QString("Built %1 nodes, %2 edges.\nSaved snapshot:\n%3\n\nScene will be restored now.")
            .arg(nodes.size()).arg(edgesMade).arg(path));

    // 7) restore previous state (no dirt)
    restoreCurrentFile();
}

// Snapshot current scene as-is to a user-chosen PNG
void Window::saveSceneSnapshot()
{
    const QRectF rect = m_scene->itemsBoundingRect().marginsAdded(QMarginsF(50,50,50,50));
    const QSize   size( std::max<int>(800, int(rect.width())),
                        std::max<int>(600, int(rect.height())) );

    QImage img = renderSceneImage(rect, size);
    const QString fn = QFileDialog::getSaveFileName(this, "Save Scene Snapshot",
                        "scene.png", "PNG Image (*.png)");
    if (fn.isEmpty()) {
        return;
    }
    if (img.save(fn)) {
        QMessageBox::information(this, "Snapshot Saved", QString("Saved: %1").arg(fn));
    } else {
        QMessageBox::warning(this, "Snapshot Failed", "Could not save PNG.");
    }
}

// JavaScript test methods removed - focusing on core C++ functionality




void Window::onNodeCreationRequested()
{
    // The signal includes the nodeTemplate, but we need to handle it via sender() for now
    // due to header file forward declaration limitations
    
    // For double-click from palette, create node at center of view
    QPointF centerPoint = m_view->mapToScene(m_view->rect().center());
    
    // We'll implement drag-and-drop separately - for now just handle the signal connection
    
    // Create a default input node for testing
    createNodeFromPalette(centerPoint, "IN", "Input", 0, 2);
}

