#include "window.h"
#include "view.h"
#include "scene.h"
#include "qgraph.h"
#include "node.h"
#include "edge.h"
#include "graph_factory.h"
#include "xml_autosave_observer.h"
#include "javascript_engine.h"
#include "node_palette_widget.h"
#include "graphics_item_keys.h"
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
#include <QJSValue>
#include <libxml/tree.h>
#include <libxml/xmlsave.h>

Window::Window(QWidget* parent)
    : QMainWindow(parent)
    , m_scene(new Scene(this))
    , m_graph(new QGraph(m_scene, this))  // QGraph wraps Scene
    , m_view(new View(m_scene, this))
    , m_jsEngine(new JavaScriptEngine(this))  // Application logic layer
    , m_firstShow(true)
{
    setWindowTitle("NodeGraph - Self-Serializing Node Editor");
    resize(1400, 900);
    
    // Initialize UI components to nullptr
    m_nodePaletteDock = nullptr;
    m_nodePalette = nullptr;
    // m_javaScriptConsoleDock = nullptr;
    // m_javaScriptConsole = nullptr;
    m_fileInfoLabel = nullptr;
    m_graphStatsLabel = nullptr;
    m_selectionLabel = nullptr;
    m_positionLabel = nullptr;
    m_zoomLabel = nullptr;
    m_operationProgress = nullptr;
    
    // Create XML document for factory
    m_xmlDocument = xmlNewDoc(BAD_CAST "1.0");
    xmlNodePtr root = xmlNewNode(nullptr, BAD_CAST "graph");
    xmlDocSetRootElement(m_xmlDocument, root);
    xmlSetProp(root, BAD_CAST "version", BAD_CAST "1.0");
    
    // Initialize factory for interactive node creation
    m_factory = new GraphFactory(m_scene, m_xmlDocument);

    // Initialize JavaScript engine with API bindings
    if (m_jsEngine) {
        m_jsEngine->registerNodeAPI(m_scene);
        m_jsEngine->registerGraphAPI();
        m_jsEngine->registerQGraph(m_graph);
        qDebug() << "Window: JavaScript engine initialized and APIs registered";
    }

    // Initialize autosave observer for automatic XML saving
    m_autosaveObserver = new XmlAutosaveObserver(m_scene, "autosave.xml");
    m_autosaveObserver->setDelay(750); // 750ms delay after changes
    
    // CRITICAL: Attach observer to scene to receive notifications
    m_scene->attach(m_autosaveObserver);
    
    // Setup enhanced UI
    setupUI();
    setupActions();
    setupMenus();
    setupStatusBar();
    setupDockWidgets(); // JavaScript console disabled for now
    
    // Connect scene signals for status updates
    connect(m_scene, &Scene::sceneChanged, this, &Window::onSceneChanged);
    
    // Connect view signals for drag-and-drop
    connect(m_view, &View::nodeDropped, this, &Window::createNodeFromPalette);
    
    // Initial status update
    updateStatusBar();
    
    // Enable keyboard shortcuts
    setFocusPolicy(Qt::StrongFocus);
}

Window::~Window()
{
    // Clean up autosave observer
    if (m_autosaveObserver) {
        m_scene->detach(m_autosaveObserver);
        delete m_autosaveObserver;
    }
    
    // Clean up XML document
    if (m_xmlDocument) {
        xmlFreeDoc(m_xmlDocument);
        m_xmlDocument = nullptr;
    }
}

void Window::setupActions()
{
    // Legacy node creation actions removed - use NodePalette instead
    // All node types are created through the template system (SOURCE, SINK, TRANSFORM, MERGE, SPLIT)
}


void Window::keyPressEvent(QKeyEvent* event)
{
    if (event->modifiers() & Qt::ControlModifier) {
        switch (event->key()) {
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
        // Delete selected items
        qDebug() << "🗑️ Delete key pressed - deleting selected items";
        m_graph->deleteSelected();  // Use QGraph instead of Scene
    }
    QMainWindow::keyPressEvent(event);
}

void Window::setCurrentFile(const QString& filename)
{
    m_currentFile = filename;
    if (!filename.isEmpty()) {
        setWindowTitle(QString("Node Editor - %1").arg(QFileInfo(filename).fileName()));
        qDebug() << "📁 Current file set to:" << filename;
    } else {
        setWindowTitle("Node Editor");
        qDebug() << "📁 Current file cleared";
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
        
        qDebug() << "✅ MANUAL SAVE COMPLETE:";
        qDebug() << "   📁 File:" << fileInfo.fileName();
        qDebug() << "   ⏱️  Time:" << elapsed << "ms";
        qDebug() << "   📊 Size:" << (fileSize / 1024.0) << "KB";
        qDebug() << "   🔵 Nodes:" << nodeCount;
        qDebug() << "   🔗 Edges:" << edgeCount;
        
        QMessageBox::information(this, "Save Complete", 
            QString("Graph saved successfully!\n\n📁 File: %1\n🔵 Nodes: %2\n🔗 Edges: %3\n⏱️ Time: %4ms\n📊 Size: %5 KB")
            .arg(fileInfo.fileName())
            .arg(nodeCount)
            .arg(edgeCount)
            .arg(elapsed)
            .arg(fileSize / 1024.0, 0, 'f', 1));
        return true;
    } else {
        qDebug() << "✗ Failed to save graph";
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
    m_graph->clear();
    
    // Use GraphFactory to load from XML file
    qDebug() << "Starting GraphFactory XML load...";
    if (m_factory->loadFromXmlFile(filename)) {
        qint64 elapsed = timer.elapsed();
        
        // Set current file for Ctrl+S functionality
        m_currentFile = filename;
        setWindowTitle(QString("Node Editor - %1").arg(QFileInfo(filename).fileName()));
        
        qDebug() << "✓ Graph loaded successfully in" << elapsed << "ms";
        
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
        qDebug() << "✗ Failed to load graph";
        QMessageBox::critical(this, "Load Error", 
            QString("Failed to load graph from file.\n\nFile: %1")
            .arg(QFileInfo(filename).fileName()));
        return false;
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
        qDebug() << "✓ Window: Factory successfully created" << name << "node";
        qDebug() << "Window: Node created at scene position:" << scenePos;
        qDebug() << "Window: Updating status bar";
        
        // Update status bar to reflect the new node
        updateStatusBar();
        statusBar()->showMessage(QString("Created %1 node").arg(name), 2000);
        
        qDebug() << "✓ Window: Node creation process completed successfully";
    } else {
        qDebug() << "✗ Window: Factory FAILED to create" << name << "node";
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

    // Legacy "Create Node" submenu removed - use NodePalette instead
    // All node creation now goes through template system (drag from palette)
    
    QAction* validateAction = new QAction("&Validate Graph", this);
    validateAction->setStatusTip("Check graph for errors and inconsistencies");
    m_toolsMenu->addAction(validateAction);
    
    QAction* statisticsAction = new QAction("Graph &Statistics", this);
    statisticsAction->setStatusTip("Show detailed graph statistics");
    m_toolsMenu->addAction(statisticsAction);

    // Simple script execution
    m_toolsMenu->addSeparator();
    QAction* jsScriptAction = new QAction("📝 Load &Script", this);
    jsScriptAction->setStatusTip("Load and execute JavaScript script");
    jsScriptAction->setShortcut(QKeySequence("Ctrl+Shift+L"));
    connect(jsScriptAction, &QAction::triggered, this, &Window::loadAndExecuteScript);
    m_toolsMenu->addAction(jsScriptAction);
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
    
    // Note: Node creation handled via drag-and-drop (View::nodeDropped signal)
    // NodeCreationRequested signal not used - nodes created only through template system
    
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
    
    // ✅ CAST-FREE IMPLEMENTATION using metadata keys
    QList<QGraphicsItem*> selectedItems = m_scene->selectedItems();
    if (selectedItems.isEmpty()) {
        m_selectionLabel->setText("No selection");
    } else {
        int nodeCount = 0;
        int edgeCount = 0;

        for (QGraphicsItem* item : selectedItems) {
            const auto k = item->data(Gik::KindKey);
            if (!k.isValid()) continue;

            if (k.toInt() == Gik::Kind_Node) {
                nodeCount++;
            } else if (k.toInt() == Gik::Kind_Edge) {
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
    // TODO: Clear current scene and reset
    qDebug() << "🆕 New file requested";
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
            qDebug() << "✓ Save successful";
            updateStatusBar();
        } else {
            qDebug() << "✗ Save FAILED";
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
    
    qDebug() << "PHASE1: ✓ Window shutdown complete";
}

void Window::loadAndExecuteScript()
{
    QString fileName = QFileDialog::getOpenFileName(
        this,
        "Load JavaScript File",
        "./scripts/",
        "JavaScript Files (*.js);;All Files (*)"
    );

    if (!fileName.isEmpty()) {
        if (!m_jsEngine) {
            QMessageBox::warning(this, "JavaScript Error", "JavaScript engine not initialized");
            return;
        }

        // Test engine functionality
        QJSValue quickTest = m_jsEngine->evaluate("1 + 1");
        if (quickTest.isError()) {
            QMessageBox::warning(this, "JavaScript Error", "JavaScript engine is not functional");
            return;
        }

        QJSValue result = m_jsEngine->evaluateFile(fileName);

        if (result.isError()) {
            QMessageBox::critical(this, "Script Error",
                                 QString("Script execution failed: %1").arg(result.toString()));
        } else {
            QString resultText = result.isUndefined() ? "Script executed successfully" : result.toString();
            QMessageBox::information(this, "Script Executed",
                                   QString("Script completed: %1").arg(resultText));
        }

        updateStatusBar();
    }
}

void Window::executeScriptFile(const QString& filePath)
{
    qDebug() << "Window: Executing script file:" << filePath;

    if (!m_jsEngine) {
        qCritical() << "JavaScript engine not initialized";
        return;
    }

    // Test engine functionality
    QJSValue quickTest = m_jsEngine->evaluate("1 + 1");
    if (quickTest.isError()) {
        qCritical() << "JavaScript engine is not functional";
        return;
    }

    qDebug() << "Window: Evaluating script file...";
    QJSValue result = m_jsEngine->evaluateFile(filePath);

    if (result.isError()) {
        qCritical() << "Script error:" << result.toString();
    } else {
        QString resultText = result.isUndefined() ? "Script executed successfully" : result.toString();
        qDebug() << "Script result:" << resultText;
    }

    updateStatusBar();
}

void Window::setAutoTestScript(const QString& scriptPath)
{
    m_autoTestScript = scriptPath;
    qDebug() << "Window: Auto-test script set to:" << scriptPath;
}

void Window::showEvent(QShowEvent* event)
{
    QMainWindow::showEvent(event);

    // Run auto-test on first show (if requested via command line)
    if (m_firstShow && !m_autoTestScript.isEmpty()) {
        m_firstShow = false;

        qDebug() << "Window: Running auto-test script on first show...";
        QTimer::singleShot(100, this, [this]() {
            executeScriptFile(m_autoTestScript);
        });
    }
}

// onNodeCreationRequested() removed - nodes created only via drag-and-drop from palette

