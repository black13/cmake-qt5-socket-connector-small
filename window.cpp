#include "window.h"
#include "view.h"
#include "scene.h"
#include "node.h"
#include "edge.h"
#include "graph_factory.h"
// GraphController removed - unnecessary complexity
#include "xml_autosave_observer.h"
#include "script_host.h"
// JavaScript engine include removed
#include "node_palette_widget.h"
// #include "javascript_console.h"  // Disabled for now
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
// QJSValue include removed - focusing on core C++ functionality
#include <libxml/tree.h>
#include <libxml/xmlsave.h>

Window::Window(QWidget* parent)
    : QMainWindow(parent)
    , m_scene(new Scene(this))
    , m_view(new View(m_scene, this))
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
    
    // DO NOT delete m_factory (non-owning)
}

void Window::adoptFactory(GraphFactory* factory)
{
    Q_ASSERT(factory); // Null factory is a programming error
    m_factory = factory; // non-owning: main owns factory lifetime
    
    // ISSUE 4: Inject factory into scene for consistent edge creation
    m_scene->setGraphFactory(factory);
    
    // Now that a factory exists, wire things that depend on it:
    // ISSUE 2: Centralize autosave timing - using 1200ms as compromise
    m_autosaveObserver = new XmlAutosaveObserver(m_scene, "autosave.xml");
    m_autosaveObserver->setDelay(1200); // Centralized autosave policy
    
    // CRITICAL: Attach observer to scene to receive notifications
    m_scene->attach(m_autosaveObserver);
    
    // Initialize optional JavaScript host
    m_scriptHost = new ScriptHost(m_scene, m_factory, this);
    
    initializeWithFactory(); // actions that depend on m_factory
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
    
    // Initial status update
    updateStatusBar();
    
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
        int nodeCount = m_scene->getNodes().size();
        int edgeCount = m_scene->getEdges().size();
        
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
    qDebug() << "=== LOADING GRAPH ===" << filename;
    
    QElapsedTimer timer;
    timer.start();
    
    // Clear current scene AND registries to prevent dangling pointers
    qDebug() << "Clearing current graph...";
    m_scene->clearGraph();
    
    // Use GraphFactory to load from XML file
    qDebug() << "Starting GraphFactory XML load...";
    if (m_factory->loadFromXmlFile(filename)) {
        qint64 elapsed = timer.elapsed();
        
        // Set current file for Ctrl+S functionality
        m_currentFile = filename;
        setWindowTitle(QString("Node Editor - %1").arg(QFileInfo(filename).fileName()));
        
        qDebug() << "Graph loaded successfully in" << elapsed << "ms";
        
        // DEBUGGING: Detailed count verification
        int nodeCount = m_scene->getNodes().size();
        int edgeCount = m_scene->getEdges().size();
        qDebug() << "DEBUG: Hash container sizes:";
        qDebug() << "  m_scene->getNodes().size() =" << nodeCount;
        qDebug() << "  m_scene->getEdges().size() =" << edgeCount;
        qDebug() << "  Qt scene items count:" << m_scene->items().size();
        qDebug() << "  Current file set to:" << m_currentFile;
        
        QMessageBox::information(this, "Load Complete", 
            QString("Graph loaded successfully!\n\nFile: %1\nNodes: %2\nEdges: %3\nTime: %4ms\n\nCtrl+S will now save to this file.")
            .arg(QFileInfo(filename).fileName())
            .arg(nodeCount)
            .arg(edgeCount)
            .arg(elapsed));
        return true;
    } else {
        qDebug() << "Failed to load graph";
        QMessageBox::critical(this, "Load Error", 
            QString("Failed to load graph from file.\n\nFile: %1")
            .arg(QFileInfo(filename).fileName()));
        return false;
    }
}

void Window::createInputNode()
{
    // Find a nice position in the view center
    QPointF viewCenter = m_view->mapToScene(m_view->viewport()->rect().center());
    
    // Add some randomization so multiple nodes don't overlap
    qreal randomX = QRandomGenerator::global()->bounded(-50, 50);
    qreal randomY = QRandomGenerator::global()->bounded(-50, 50);
    QPointF position = viewCenter + QPointF(randomX, randomY);
    
    // Create input node using factory (XML-first approach)
    Node* node = m_factory->createNode("SOURCE", position);
    
    if (node) {
        qDebug() << "Created input node at" << position;
    } else {
        qDebug() << "Failed to create input node";
    }
}

void Window::createOutputNode()
{
    // Find a nice position in the view center
    QPointF viewCenter = m_view->mapToScene(m_view->viewport()->rect().center());
    
    // Add some randomization so multiple nodes don't overlap
    qreal randomX = QRandomGenerator::global()->bounded(-50, 50);
    qreal randomY = QRandomGenerator::global()->bounded(-50, 50);
    QPointF position = viewCenter + QPointF(randomX, randomY);
    
    // Create output node using factory (XML-first approach)
    Node* node = m_factory->createNode("SINK", position);
    
    if (node) {
        qDebug() << "Created output node at" << position;
    } else {
        qDebug() << "Failed to create output node";
    }
}

void Window::createProcessorNode()
{
    // Find a nice position in the view center
    QPointF viewCenter = m_view->mapToScene(m_view->viewport()->rect().center());
    
    // Add some randomization so multiple nodes don't overlap
    qreal randomX = QRandomGenerator::global()->bounded(-50, 50);
    qreal randomY = QRandomGenerator::global()->bounded(-50, 50);
    QPointF position = viewCenter + QPointF(randomX, randomY);
    
    // Create processor node using factory (XML-first approach)
    Node* node = m_factory->createNode("TRANSFORM", position);
    
    if (node) {
        qDebug() << "Created processor node at" << position;
    } else {
        qDebug() << "Failed to create processor node";
    }
}

void Window::createNodeFromPalette(const QPointF& scenePos, const QString& nodeType, 
                                  const QString& name, int inputSockets, int outputSockets)
{
    qDebug() << "========================================";
    qDebug() << "Window: RECEIVED nodeDropped signal";
    qDebug() << "Window: Creating node from palette:";
    qDebug() << "  - Name:" << name;
    qDebug() << "  - Type:" << nodeType;
    qDebug() << "  - Position:" << scenePos;
    qDebug() << "  - Input sockets:" << inputSockets;
    qDebug() << "  - Output sockets:" << outputSockets;
    qDebug() << "Window: Calling factory->createNode()";
    
    // Create node using factory with the exact specifications from the palette
    Node* node = m_factory->createNode(nodeType, scenePos, inputSockets, outputSockets);
    
    if (node) {
        qDebug() << "Window: Factory successfully created" << name << "node";
        qDebug() << "Window: Node created at scene position:" << scenePos;
        qDebug() << "Window: Updating status bar";
        
        // Update status bar to reflect the new node
        updateStatusBar();
        statusBar()->showMessage(QString("Created %1 node").arg(name), 2000);
        
        qDebug() << "Window: Node creation process completed successfully";
    } else {
        qDebug() << "Window: Factory FAILED to create" << name << "node";
        qDebug() << "Window: This may indicate factory or scene issues";
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
    
    // JavaScript test menu items removed - focusing on core C++ functionality
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
    if (!m_scene) return;
    
    // Update graph statistics
    int nodeCount = m_scene->getNodes().size();
    int edgeCount = m_scene->getEdges().size();
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
    if (!m_scene) return;
    
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
    if (!m_scene) return;
    
    // Clear current scene
    m_scene->clearGraph();
    
    // Reset file state
    setCurrentFile(QString());
    
    // Update UI
    updateStatusBar();
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
// PHASE 3: Safe Shutdown Coordination
// ============================================================================

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
    qDebug() << __FUNCTION__ << "- COLLECTION STATE BEFORE: Scene has" << m_scene->getNodes().size() << "nodes," << m_scene->getEdges().size() << "edges";
    qDebug() << __FUNCTION__ << "- COLLECTION STATE BEFORE: Qt scene has" << m_scene->items().size() << "total items";
    
    // Clear existing graph for clean test - ISSUE 3: Use safe clear ordering
    m_scene->clearGraph();
    qDebug() << __FUNCTION__ << "- TEMPLATE SYSTEM: Cleared scene safely for testing";
    
    // Show collection state AFTER clearing  
    qDebug() << __FUNCTION__ << "- COLLECTION STATE AFTER: Scene has" << m_scene->getNodes().size() << "nodes," << m_scene->getEdges().size() << "edges";
    qDebug() << __FUNCTION__ << "- COLLECTION STATE AFTER: Qt scene has" << m_scene->items().size() << "total items";
    
    // CRASH VALIDATION: Check for stale pointer problem
    if (m_scene->getNodes().size() > 0 && m_scene->items().size() == 0) {
        qWarning() << __FUNCTION__ << "- CRASH RISK DETECTED: Scene hash maps contain" << m_scene->getNodes().size() << "node pointers but Qt scene is empty!";
        qWarning() << __FUNCTION__ << "- CRASH CAUSE: Autosave will try to access deleted Node objects";
        qWarning() << __FUNCTION__ << "- SOLUTION NEEDED: Scene::clear() should also clear hash maps, not just Qt items";
    }
    if (m_scene->getEdges().size() > 0 && m_scene->items().size() == 0) {
        qWarning() << __FUNCTION__ << "- CRASH RISK DETECTED: Scene hash maps contain" << m_scene->getEdges().size() << "edge pointers but Qt scene is empty!";
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
    int finalNodeCount = m_scene->getNodes().size();
    
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
    qDebug() << __FUNCTION__ << "- COLLECTION STATE BEFORE: Scene has" << m_scene->getNodes().size() << "nodes," << m_scene->getEdges().size() << "edges";
    qDebug() << __FUNCTION__ << "- COLLECTION STATE BEFORE: Qt scene has" << m_scene->items().size() << "total items";
    
    // Clear existing graph for clean test - ISSUE 3: Use safe clear ordering  
    m_scene->clearGraph();
    qDebug() << __FUNCTION__ << "- TEMPLATE SYSTEM: Cleared scene safely for edge testing";
    
    // Show collection state AFTER clearing  
    qDebug() << __FUNCTION__ << "- COLLECTION STATE AFTER: Scene has" << m_scene->getNodes().size() << "nodes," << m_scene->getEdges().size() << "edges";
    qDebug() << __FUNCTION__ << "- COLLECTION STATE AFTER: Qt scene has" << m_scene->items().size() << "total items";
    
    // CRASH VALIDATION: Check for stale pointer problem  
    if (m_scene->getNodes().size() > 0 && m_scene->items().size() == 0) {
        qWarning() << __FUNCTION__ << "- CRASH RISK DETECTED: Scene hash maps contain" << m_scene->getNodes().size() << "node pointers but Qt scene is empty!";
        qWarning() << __FUNCTION__ << "- CRASH CAUSE: Autosave will try to access deleted Node objects";
        qWarning() << __FUNCTION__ << "- SOLUTION NEEDED: Scene::clear() should also clear hash maps, not just Qt items";
    }
    if (m_scene->getEdges().size() > 0 && m_scene->items().size() == 0) {
        qWarning() << __FUNCTION__ << "- CRASH RISK DETECTED: Scene hash maps contain" << m_scene->getEdges().size() << "edge pointers but Qt scene is empty!";
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
    Edge* edge1 = m_factory->createEdge(sourceNode, 0, transformNode, 0);
    if (edge1) {
        edgeResults << "SOURCE->TRANSFORM: SUCCESS";
        edgeSuccessCount++;
        qDebug() << __FUNCTION__ << "- TEMPLATE SYSTEM: Edge 1 created with ID" << edge1->getId().toString(QUuid::WithoutBraces);
    } else {
        edgeResults << "SOURCE->TRANSFORM: FAILED";
        qDebug() << __FUNCTION__ << "- TEMPLATE SYSTEM: FAILED to create SOURCE->TRANSFORM edge";
    }
    
    // Edge 2: TRANSFORM (output 1) -> MERGE (input 0) 
    qDebug() << __FUNCTION__ << "- TEMPLATE SYSTEM: Connecting TRANSFORM->MERGE";
    Edge* edge2 = m_factory->createEdge(transformNode, 1, mergeNode, 0);
    if (edge2) {
        edgeResults << "TRANSFORM->MERGE: SUCCESS";
        edgeSuccessCount++;
        qDebug() << __FUNCTION__ << "- TEMPLATE SYSTEM: Edge 2 created with ID" << edge2->getId().toString(QUuid::WithoutBraces);
    } else {
        edgeResults << "TRANSFORM->MERGE: FAILED";
        qDebug() << __FUNCTION__ << "- TEMPLATE SYSTEM: FAILED to create TRANSFORM->MERGE edge";
    }
    
    // Edge 3: MERGE (output 2) -> SINK (input 0)
    qDebug() << __FUNCTION__ << "- TEMPLATE SYSTEM: Connecting MERGE->SINK";
    Edge* edge3 = m_factory->createEdge(mergeNode, 2, sinkNode, 0);
    if (edge3) {
        edgeResults << "MERGE->SINK: SUCCESS";
        edgeSuccessCount++;
        qDebug() << __FUNCTION__ << "- TEMPLATE SYSTEM: Edge 3 created with ID" << edge3->getId().toString(QUuid::WithoutBraces);
    } else {
        edgeResults << "MERGE->SINK: FAILED";
        qDebug() << __FUNCTION__ << "- TEMPLATE SYSTEM: FAILED to create MERGE->SINK edge";
    }
    
    // Test invalid connection (wrong socket indices)
    qDebug() << __FUNCTION__ << "- TEMPLATE SYSTEM: Testing invalid connection rejection...";
    Edge* invalidEdge = m_factory->createEdge(sourceNode, 999, sinkNode, 0); // Invalid socket index
    bool invalidRejected = (invalidEdge == nullptr);
    
    // Get final scene stats
    int finalNodeCount = m_scene->getNodes().size();
    int finalEdgeCount = m_scene->getEdges().size();
    
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

