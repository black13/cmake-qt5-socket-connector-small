#include "window.h"
#include "view.h"
#include "scene.h"
#include "node.h"
#include "edge.h"
#include "graph_factory.h"
#include "xml_autosave_observer.h"
#include "javascript_engine.h"
// #include "javascript_console.h"  // Disabled for now
// #include "node_palette_bar.h" // Disabled for now
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
    , m_view(new View(m_scene, this))
{
    setWindowTitle("NodeGraph - Self-Serializing Node Editor");
    resize(1400, 900);
    
    // Initialize UI components to nullptr
    // m_nodePaletteDock = nullptr;
    // m_nodePalette = nullptr;
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
    
    // Connect view signals for drag-and-drop (disabled)
    // connect(m_view, &View::nodeDropped, this, &Window::createNodeAtPosition);
    
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
        // Delete selected items
        qDebug() << "🗑️ Delete key pressed - deleting selected items";
        m_scene->deleteSelected();
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
    m_scene->clearGraph();
    
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

void Window::createInputNode()
{
    // Find a nice position in the view center
    QPointF viewCenter = m_view->mapToScene(m_view->viewport()->rect().center());
    
    // Add some randomization so multiple nodes don't overlap
    qreal randomX = QRandomGenerator::global()->bounded(-50, 50);
    qreal randomY = QRandomGenerator::global()->bounded(-50, 50);
    QPointF position = viewCenter + QPointF(randomX, randomY);
    
    // Create input node using factory (XML-first approach)
    Node* node = m_factory->createNode("IN", position, 0, 2);  // 0 inputs, 2 outputs
    
    if (node) {
        qDebug() << "✓ Created input node at" << position;
    } else {
        qDebug() << "✗ Failed to create input node";
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
    Node* node = m_factory->createNode("OUT", position, 2, 0);  // 2 inputs, 0 outputs
    
    if (node) {
        qDebug() << "✓ Created output node at" << position;
    } else {
        qDebug() << "✗ Failed to create output node";
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
    Node* node = m_factory->createNode("PROC", position, 2, 2);  // 2 inputs, 2 outputs
    
    if (node) {
        qDebug() << "✓ Created processor node at" << position;
    } else {
        qDebug() << "✗ Failed to create processor node";
    }
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
    
    // JavaScript test runner
    m_toolsMenu->addSeparator();
    QAction* jsTestAction = new QAction("🧪 Run &JavaScript Tests", this);
    jsTestAction->setStatusTip("Run embedded JavaScript test suite");
    jsTestAction->setShortcut(QKeySequence("Ctrl+J"));
    connect(jsTestAction, &QAction::triggered, this, &Window::runJavaScriptTests);
    m_toolsMenu->addAction(jsTestAction);
    
    // Simple script execution
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
    // Dock widgets disabled for now - using simple script loading instead
    qDebug() << "Dock widgets disabled - using simple script loading";
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

void Window::runJavaScriptTests()
{
    qDebug() << "Window: Running JavaScript test suite";
    
    // Initialize JavaScript engine with GraphController
    auto* jsEngine = m_scene->getJavaScriptEngine();
    if (!jsEngine) {
        QMessageBox::warning(this, "JavaScript Error", "JavaScript engine not initialized");
        return;
    }
    
    // Register GraphController if not already done
    jsEngine->registerGraphController(m_scene, m_factory);
    
    // Show status message
    statusBar()->showMessage("Running JavaScript tests...", 2000);
    
    // Run the basic test suite
    QString testScript = R"(
        console.log("=== Starting JavaScript Test Suite ===");
        
        // Test 1: Basic graph creation
        try {
            console.log("Test 1: Basic graph creation");
            Graph.clear();
            let node1 = Graph.createNode("Source", 100, 100);
            let node2 = Graph.createNode("Sink", 300, 100);
            let edge = Graph.connect(node1, 0, node2, 0);
            
            let stats = Graph.getStats();
            console.log("Created graph with " + stats.nodes + " nodes and " + stats.edges + " edges");
            
            if (stats.nodes === 2 && stats.edges === 1) {
                console.log("✅ Test 1 PASSED");
            } else {
                console.log("❌ Test 1 FAILED");
            }
        } catch (e) {
            console.log("❌ Test 1 ERROR: " + e.toString());
        }
        
        // Test 2: Node deletion
        try {
            console.log("Test 2: Node deletion");
            let beforeStats = Graph.getStats();
            Graph.deleteNode(node1);
            let afterStats = Graph.getStats();
            
            if (afterStats.nodes === 1 && afterStats.edges === 0) {
                console.log("✅ Test 2 PASSED");
            } else {
                console.log("❌ Test 2 FAILED");
            }
        } catch (e) {
            console.log("❌ Test 2 ERROR: " + e.toString());
        }
        
        // Test 3: XML operations
        try {
            console.log("Test 3: XML operations");
            Graph.clear();
            let testNode = Graph.createNode("Source", 150, 150);
            
            Graph.saveXml("test_output.xml");
            let xmlString = Graph.getXmlString();
            
            if (xmlString.length > 0 && xmlString.includes('<graph')) {
                console.log("✅ Test 3 PASSED");
            } else {
                console.log("❌ Test 3 FAILED");
            }
        } catch (e) {
            console.log("❌ Test 3 ERROR: " + e.toString());
        }
        
        // Test 4: Complex graph
        try {
            console.log("Test 4: Complex graph creation");
            Graph.clear();
            
            let source = Graph.createNode("Source", 50, 100);
            let processor = Graph.createNode("1-to-2", 200, 100);
            let sink1 = Graph.createNode("Sink", 350, 50);
            let sink2 = Graph.createNode("Sink", 350, 150);
            
            Graph.connect(source, 0, processor, 0);
            Graph.connect(processor, 0, sink1, 0);
            Graph.connect(processor, 1, sink2, 0);
            
            let complexStats = Graph.getStats();
            
            if (complexStats.nodes === 4 && complexStats.edges === 3) {
                console.log("✅ Test 4 PASSED");
            } else {
                console.log("❌ Test 4 FAILED - Expected 4 nodes, 3 edges, got " + 
                           complexStats.nodes + " nodes, " + complexStats.edges + " edges");
            }
        } catch (e) {
            console.log("❌ Test 4 ERROR: " + e.toString());
        }
        
        console.log("=== JavaScript Test Suite Complete ===");
    )";
    
    // Execute the test script
    QJSValue result = jsEngine->evaluate(testScript);
    
    if (result.isError()) {
        QMessageBox::critical(this, "JavaScript Test Error", 
                             QString("Test execution failed: %1").arg(result.toString()));
    } else {
        QMessageBox::information(this, "JavaScript Tests", 
                                "Test suite completed. Check debug output for results.");
    }
    
    // Update status bar
    updateStatusBar();
}

void Window::loadAndExecuteScript()
{
    qDebug() << "JS_EXECUTION: User requested script loading";
    
    QString fileName = QFileDialog::getOpenFileName(
        this, 
        "Load JavaScript File", 
        "./scripts/", 
        "JavaScript Files (*.js);;All Files (*)"
    );
    
    if (!fileName.isEmpty()) {
        qDebug() << "JS_EXECUTION: User selected file:" << fileName;
        
        qDebug() << "JS_EXECUTION: Getting JavaScript engine from scene";
        qDebug() << "JS_EXECUTION: Scene pointer:" << m_scene;
        
        auto* jsEngine = m_scene->getJavaScriptEngine();
        qDebug() << "JS_EXECUTION: JavaScript engine pointer:" << jsEngine;
        
        if (!jsEngine) {
            qDebug() << "JS_ERROR: JavaScript engine not initialized";
            QMessageBox::warning(this, "JavaScript Error", "JavaScript engine not initialized");
            return;
        }
        
        // Register GraphController if not already done
        jsEngine->registerGraphController(m_scene, m_factory);
        qDebug() << "JS_EXECUTION: GraphController registered";
        
        // Log current graph state before script execution
        qDebug() << "JS_EXECUTION: Current graph state - nodes:" << m_scene->getNodes().size() << "edges:" << m_scene->getEdges().size();
        
        QJSValue result = jsEngine->evaluateFile(fileName);
        
        if (result.isError()) {
            qDebug() << "JS_ERROR: Script execution failed, showing error dialog";
            QMessageBox::critical(this, "Script Error", 
                                 QString("Script execution failed: %1").arg(result.toString()));
        } else {
            QString resultText = result.isUndefined() ? "Script executed successfully" : result.toString();
            qDebug() << "JS_EXECUTION: Script completed, showing success dialog";
            QMessageBox::information(this, "Script Executed", 
                                   QString("Script completed: %1").arg(resultText));
        }
        
        // Log graph state after script execution
        qDebug() << "JS_EXECUTION: Post-execution graph state - nodes:" << m_scene->getNodes().size() << "edges:" << m_scene->getEdges().size();
        
        // Update status bar
        updateStatusBar();
    } else {
        qDebug() << "JS_EXECUTION: User cancelled script loading";
    }
}