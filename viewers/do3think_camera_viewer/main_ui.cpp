#include "main_ui.h"
#include "machine.h"
#include "material_theme.h"  // Modern Material Design theme
#include "futuristic_widgets.h"  // Custom futuristic widgets
#include "../../Do3ThinkCamera/dothink_camera_control_panel.h"
#include "../../components/base_component.h"
#include "../../OpenCV/preprocessor_control_panel.h"
#include "../../OpenCV/preprocessor_base.h"

#include <iostream>
#include <cstdio>
#include <string>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#endif

#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QDockWidget>
#include <QTabWidget>
#include <QListWidget>
#include <QTextEdit>
#include <QSplitter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QAction>
#include <QSettings>
#include <QMessageBox>
#include <QCloseEvent>
#include <QDebug>
#include <QLabel>

// Simple debug output that works independently
#define DEBUG_LOG(msg) do { \
    std::cerr << "[MainUI] " << msg << std::endl; \
    std::cerr.flush(); \
} while(0)

namespace ComponentsForest {

// Private implementation
class Do3ThinkCameraViewerMainUI::Private
{
public:
    Private(Do3ThinkCameraViewerMainUI* q) : q(q) {
        std::cerr << "[DEBUG] Do3ThinkCameraViewerMainUI::Private constructor" << std::endl;
    }
    
    Do3ThinkCameraViewerMainUI* q;
    
    // Helper methods
    void arrangeInGrid(const QList<QWidget*>& widgets);
    void arrangeInTabs(const QList<QWidget*>& widgets);
    void arrangeInSplit(const QList<QWidget*>& widgets);
};

Do3ThinkCameraViewerMainUI::Do3ThinkCameraViewerMainUI(QWidget *parent)
    : QMainWindow(parent)
    , d(std::make_unique<Private>(this))
    , m_machine(nullptr)
    , m_layoutMode(TabView)
    , m_isRecording(false)
    , m_imageProcessingDock(nullptr)
    , m_imageProcessingTabWidget(nullptr)
    , m_processingMenu(nullptr)
    , m_processingToolBar(nullptr)
    , m_preprocessAction(nullptr)
{
    DEBUG_LOG("Do3ThinkCameraViewerMainUI constructor entered");
    std::cerr << "[DEBUG] Creating main UI window" << std::endl;
    
    try {
        DEBUG_LOG("Setting up UI...");
        setupUI();
        DEBUG_LOG("UI setup complete");
        
        DEBUG_LOG("Creating status bar...");
        createStatusBar();
        DEBUG_LOG("Status bar created");
        
        DEBUG_LOG("Creating dock widgets...");
        createDockWidgets();
        DEBUG_LOG("Dock widgets created");
        
        // CRITICAL: Ensure image processing dock is hidden immediately after creation
        // It should ONLY be shown when user explicitly clicks "Preprocess"
        if (m_imageProcessingDock) {
            m_imageProcessingDock->hide();
            DEBUG_LOG("Image processing dock hidden by default");
        }
        
        DEBUG_LOG("Creating central widget...");
        createCentralWidget();
        DEBUG_LOG("Central widget created");
        
        DEBUG_LOG("Creating menus...");
        createMenus();
        DEBUG_LOG("Menus created");
        
        DEBUG_LOG("Creating toolbars...");
        createToolBars();
        DEBUG_LOG("Toolbars created");
        
        DEBUG_LOG("Connecting signals...");
        connectSignals();
        DEBUG_LOG("Signals connected");
        
        DEBUG_LOG("Updating actions...");
        updateActions();
        DEBUG_LOG("Actions updated");
        
        DEBUG_LOG("Do3ThinkCameraViewerMainUI constructor completed");
        
    } catch (const std::exception& e) {
        std::stringstream ss;
        ss << "EXCEPTION in MainUI constructor: " << e.what();
        DEBUG_LOG(ss.str());
    } catch (...) {
        DEBUG_LOG("UNKNOWN EXCEPTION in MainUI constructor");
    }
}

Do3ThinkCameraViewerMainUI::~Do3ThinkCameraViewerMainUI()
{
    DEBUG_LOG("Do3ThinkCameraViewerMainUI destructor entered");
    // Clear all panels before destruction
    clearAllPanels();
    DEBUG_LOG("Do3ThinkCameraViewerMainUI destructor completed");
}

void Do3ThinkCameraViewerMainUI::connectToMachine(Do3ThinkCameraMachine *machine)
{
    m_machine = machine;
}

void Do3ThinkCameraViewerMainUI::disconnectFromMachine()
{
    m_machine = nullptr;
}

bool Do3ThinkCameraViewerMainUI::addCameraPanel(const QString &cameraId, BaseComponent *component)
{
    // Camera panels go in the CENTER widget for image display
    // This is separate from preprocessor panels which go in the dock
    if (m_panels.contains(cameraId)) {
        qWarning() << "Panel already exists for camera:" << cameraId;
        return false;
    }
    
    // Create control panel
    auto* panel = new Do3ThinkCameraControlPanel(this);
    panel->setObjectName(QString("Panel_%1").arg(cameraId));
    
    // Connect to component
    panel->connectToComponent(component);
    
    // Connect panel signals for lifecycle management
    connect(panel, &Do3ThinkCameraControlPanel::panelClosing,
            [this, cameraId]() {
                // Stop acquisition before panel closes
                if (auto* p = m_panels.value(cameraId)) {
                    p->stopAcquisition();
                }
                emit removeCameraRequested(cameraId);
            });
    
    // Store panel
    m_panels[cameraId] = panel;
    
    // Add to UI based on layout mode
    if (m_layoutMode == TabView) {
        m_cameraTabWidget->addTab(panel, cameraId);
    } else if (m_layoutMode == SplitView) {
        m_cameraSplitter->addWidget(panel);
    } else {
        // Grid or single view - need container management
        QWidget* container = new QWidget();
        QVBoxLayout* layout = new QVBoxLayout(container);
        layout->addWidget(panel);
        layout->setContentsMargins(0, 0, 0, 0);
        m_panelContainers[cameraId] = container;
        applyLayoutMode();
    }
    
    updateActions();
    updateStatusBar();
    return true;
}

void Do3ThinkCameraViewerMainUI::updateStatusBar()
{
    // Update camera count in status bar
    if (QLabel* label = statusBar()->findChild<QLabel*>("CameraCountLabel")) {
        label->setText(tr("Cameras: %1").arg(m_panels.count()));
    }
    
    // Update preprocessor count if we have the label
    if (QLabel* label = statusBar()->findChild<QLabel*>("ProcessorCountLabel")) {
        label->setText(tr("Processors: %1").arg(m_preprocessorPanels.count()));
    }
}

bool Do3ThinkCameraViewerMainUI::removeCameraPanel(const QString &cameraId)
{
    auto* panel = m_panels.take(cameraId);
    if (!panel) {
        return false;
    }
    
    // Stop acquisition and disconnect
    panel->stopAcquisition();
    panel->disconnectFromComponent();
    
    // Remove from UI
    if (m_layoutMode == TabView) {
        for (int i = 0; i < m_cameraTabWidget->count(); ++i) {
            if (m_cameraTabWidget->widget(i) == panel) {
                m_cameraTabWidget->removeTab(i);
                break;
            }
        }
    }
    
    // Remove container if exists
    if (m_panelContainers.contains(cameraId)) {
        delete m_panelContainers.take(cameraId);
    }
    
    // Delete panel
    panel->deleteLater();
    
    updateActions();
    return true;
}

void Do3ThinkCameraViewerMainUI::clearAllPanels()
{
    // Stop all acquisitions first
    for (auto* panel : m_panels) {
        if (panel->isAcquiring()) {
            panel->stopAcquisition();
        }
    }
    
    // Remove all panels
    QStringList cameraIds = m_panels.keys();
    for (const QString& cameraId : cameraIds) {
        removeCameraPanel(cameraId);
    }
    
    m_panels.clear();
    m_panelContainers.clear();
}

QList<Do3ThinkCameraControlPanel*> Do3ThinkCameraViewerMainUI::getAllPanels() const
{
    return m_panels.values();
}

Do3ThinkCameraControlPanel* Do3ThinkCameraViewerMainUI::getActivePanel() const
{
    if (m_layoutMode == TabView && m_cameraTabWidget) {
        QWidget* current = m_cameraTabWidget->currentWidget();
        return qobject_cast<Do3ThinkCameraControlPanel*>(current);
    }
    
    // Return first panel if any
    if (!m_panels.isEmpty()) {
        return m_panels.first();
    }
    
    return nullptr;
}

// Preprocessor panel management
bool Do3ThinkCameraViewerMainUI::addPreprocessorPanel(const QString &processorId, 
                                                       OpenCV::PreProcessorBase *preprocessor)
{
    // Preprocessor panels go in the RIGHT DOCK widget for controls
    // This keeps them separate from camera image displays in the center
    if (m_preprocessorPanels.contains(processorId)) {
        qWarning() << "Preprocessor panel already exists for:" << processorId;
        return false;
    }
    
    // Create control panel
    auto* panel = new OpenCV::PreProcessorControlPanel(this);
    panel->setPanelId(processorId);
    panel->setObjectName(QString("PreprocessorPanel_%1").arg(processorId));
    
    // Connect to preprocessor
    panel->connectToPreprocessor(preprocessor);
    
    // Connect panel signals
    connect(panel, &OpenCV::PreProcessorControlPanel::panelError,
            [this](const QString& error) {
                appendLog(QString("Preprocessor error: %1").arg(error), "ERROR");
                showStatusMessage(error, 5000);
            });
    
    // Store panel
    m_preprocessorPanels[processorId] = panel;
    
    // Add to the dedicated Image Processing dock widget (RIGHT side)
    // This keeps preprocessor controls separate from camera displays (CENTER)
    if (m_imageProcessingTabWidget) {
        m_imageProcessingTabWidget->addTab(panel, processorId);
        
        // Don't automatically show the dock when adding preprocessor panels
        // User must explicitly click "Preprocess" action to show it
        // This keeps the UI clean and uncluttered
        
        // Just raise it if it's already visible
        if (m_imageProcessingDock->isVisible()) {
            m_imageProcessingDock->raise();
        }
    }
    
    updateActions();
    updateStatusBar();
    
    appendLog(QString("Preprocessor panel added: %1").arg(processorId), "INFO");
    return true;
}

bool Do3ThinkCameraViewerMainUI::removePreprocessorPanel(const QString &processorId)
{
    auto* panel = m_preprocessorPanels.take(processorId);
    if (!panel) {
        return false;
    }
    
    // Disconnect from preprocessor
    panel->disconnectFromPreprocessor();
    
    // Remove from the Image Processing dock widget
    if (m_imageProcessingTabWidget) {
        for (int i = 0; i < m_imageProcessingTabWidget->count(); ++i) {
            if (m_imageProcessingTabWidget->widget(i) == panel) {
                m_imageProcessingTabWidget->removeTab(i);
                break;
            }
        }
        
        // Hide the dock if no more preprocessor panels
        if (m_imageProcessingTabWidget->count() == 0 && m_imageProcessingDock) {
            m_imageProcessingDock->hide();
        }
    }
    
    // Remove container if exists (for backward compatibility)
    if (m_preprocessorContainers.contains(processorId)) {
        delete m_preprocessorContainers.take(processorId);
    }
    
    // Delete panel
    delete panel;
    
    updateActions();
    updateStatusBar();
    
    appendLog(QString("Preprocessor panel removed: %1").arg(processorId), "INFO");
    return true;
}

QList<OpenCV::PreProcessorControlPanel*> Do3ThinkCameraViewerMainUI::getAllPreprocessorPanels() const
{
    return m_preprocessorPanels.values();
}

OpenCV::PreProcessorControlPanel* Do3ThinkCameraViewerMainUI::getActivePreprocessorPanel() const
{
    // Get the active preprocessor panel from the Image Processing dock
    if (m_imageProcessingTabWidget) {
        QWidget* current = m_imageProcessingTabWidget->currentWidget();
        return qobject_cast<OpenCV::PreProcessorControlPanel*>(current);
    }
    
    // Return first preprocessor panel if any
    if (!m_preprocessorPanels.isEmpty()) {
        return m_preprocessorPanels.first();
    }
    
    return nullptr;
}

void Do3ThinkCameraViewerMainUI::setFullScreen(bool fullScreen)
{
    if (fullScreen) {
        showFullScreen();
    } else {
        showNormal();
    }
    m_fullScreenAction->setChecked(fullScreen);
}

void Do3ThinkCameraViewerMainUI::showStatusMessage(const QString &message, int timeout)
{
    statusBar()->showMessage(message, timeout);
}

void Do3ThinkCameraViewerMainUI::setLayoutMode(LayoutMode mode)
{
    m_layoutMode = mode;
    applyLayoutMode();
}

Do3ThinkCameraViewerMainUI::LayoutMode Do3ThinkCameraViewerMainUI::getLayoutMode() const
{
    return m_layoutMode;
}

// Batch operations through control panels
void Do3ThinkCameraViewerMainUI::onStartAllPanels()
{
    int started = 0;
    int failed = 0;
    
    // Iterate through all panels and start acquisition
    for (auto* panel : m_panels) {
        if (!panel->isAcquiring()) {
            if (panel->startAcquisition()) {
                started++;
            } else {
                failed++;
            }
        }
    }
    
    QString message = QString("Started %1 cameras").arg(started);
    if (failed > 0) {
        message += QString(", %1 failed").arg(failed);
    }
    showStatusMessage(message);
}

void Do3ThinkCameraViewerMainUI::onStopAllPanels()
{
    int stopped = 0;
    
    // Iterate through all panels and stop acquisition
    for (auto* panel : m_panels) {
        if (panel->isAcquiring()) {
            if (panel->stopAcquisition()) {
                stopped++;
            }
        }
    }
    
    showStatusMessage(QString("Stopped %1 cameras").arg(stopped));
}

// Machine event handlers
void Do3ThinkCameraViewerMainUI::onMachineStarted()
{
    showStatusMessage("Machine started successfully");
    updateActions();
}

void Do3ThinkCameraViewerMainUI::onMachineStopped()
{
    showStatusMessage("Machine stopped");
    updateActions();
}

void Do3ThinkCameraViewerMainUI::onMachineError(const QString &error)
{
    QMessageBox::critical(this, "Machine Error", error);
    appendLog(QString("ERROR: %1").arg(error), "ERROR");
}

void Do3ThinkCameraViewerMainUI::onCameraAdded(const QString &cameraId)
{
    appendLog(QString("Camera added: %1").arg(cameraId), "INFO");
    
    // Update device list
    if (m_deviceListWidget) {
        m_deviceListWidget->addItem(cameraId);
    }
    
    updateActions();
}

void Do3ThinkCameraViewerMainUI::onCameraRemoved(const QString &cameraId)
{
    appendLog(QString("Camera removed: %1").arg(cameraId), "INFO");
    
    // Update device list
    if (m_deviceListWidget) {
        for (int i = 0; i < m_deviceListWidget->count(); ++i) {
            if (m_deviceListWidget->item(i)->text() == cameraId) {
                delete m_deviceListWidget->takeItem(i);
                break;
            }
        }
    }
    
    updateActions();
}

void Do3ThinkCameraViewerMainUI::onCameraStarted(const QString &cameraId)
{
    showStatusMessage(QString("Camera %1 started").arg(cameraId));
    appendLog(QString("Camera started: %1").arg(cameraId), "INFO");
}

void Do3ThinkCameraViewerMainUI::onCameraStopped(const QString &cameraId)
{
    showStatusMessage(QString("Camera %1 stopped").arg(cameraId));
    appendLog(QString("Camera stopped: %1").arg(cameraId), "INFO");
}

void Do3ThinkCameraViewerMainUI::onCameraError(const QString &cameraId, const QString &error)
{
    QString message = QString("Camera %1 error: %2").arg(cameraId, error);
    showStatusMessage(message);
    appendLog(message, "ERROR");
}

void Do3ThinkCameraViewerMainUI::onDevicesDiscovered(const QStringList &devices)
{
    appendLog(QString("Discovered %1 devices").arg(devices.count()), "INFO");
    
    // Update device list widget
    if (m_deviceListWidget) {
        m_deviceListWidget->clear();
        m_deviceListWidget->addItems(devices);
    }
    
    if (devices.isEmpty()) {
        showStatusMessage("No cameras found");
    } else {
        showStatusMessage(QString("Found %1 camera(s)").arg(devices.count()));
    }
}

// Menu action handlers
void Do3ThinkCameraViewerMainUI::onAddCamera()
{
    emit addCameraRequested();
}

void Do3ThinkCameraViewerMainUI::onRemoveCamera()
{
    QString currentId = getCurrentCameraId();
    if (!currentId.isEmpty()) {
        emit removeCameraRequested(currentId);
    }
}

void Do3ThinkCameraViewerMainUI::onRefreshDevices()
{
    showStatusMessage("Refreshing device list...");
    emit refreshDevicesRequested();
}

void Do3ThinkCameraViewerMainUI::onShowSettings()
{
    emit settingsRequested();
}

void Do3ThinkCameraViewerMainUI::onShowAbout()
{
    QMessageBox::about(this, "About Do3Think Camera Viewer",
                       "Do3Think Camera Viewer v1.0.0\n\n"
                       "Part of ComponentsForest ecosystem\n"
                       "Industrial-grade camera control application");
}

void Do3ThinkCameraViewerMainUI::onToggleFullScreen()
{
    bool fullScreen = windowState() & Qt::WindowFullScreen;
    setFullScreen(!fullScreen);
    emit fullScreenToggled(!fullScreen);
}

void Do3ThinkCameraViewerMainUI::onChangeLayout()
{
    // Cycle through layout modes
    LayoutMode newMode = static_cast<LayoutMode>((m_layoutMode + 1) % 4);
    setLayoutMode(newMode);
    emit layoutModeChangeRequested(newMode);
}

void Do3ThinkCameraViewerMainUI::onPanelClosed(const QString &cameraId)
{
    removeCameraPanel(cameraId);
}

void Do3ThinkCameraViewerMainUI::onCurrentCameraChanged(int index)
{
    if (index >= 0 && m_cameraTabWidget) {
        QWidget* widget = m_cameraTabWidget->widget(index);
        // Find camera ID for this widget
        for (auto it = m_panels.begin(); it != m_panels.end(); ++it) {
            if (it.value() == widget) {
                m_currentCameraId = it.key();
                emit panelActivated(m_currentCameraId);
                break;
            }
        }
    }
    updateActions();
}

void Do3ThinkCameraViewerMainUI::appendLog(const QString &message, const QString &level)
{
    if (m_logTextEdit) {
        QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
        QString formattedMessage = QString("[%1] [%2] %3")
            .arg(timestamp)
            .arg(level)
            .arg(message);
        
        // Color code by level
        if (level == "ERROR") {
            m_logTextEdit->setTextColor(Qt::red);
        } else if (level == "WARNING") {
            m_logTextEdit->setTextColor(QColor(255, 165, 0)); // Orange
        } else {
            m_logTextEdit->setTextColor(palette().color(QPalette::Text));
        }
        
        m_logTextEdit->append(formattedMessage);
    }
}

// UI Setup methods
void Do3ThinkCameraViewerMainUI::setupUI()
{
    DEBUG_LOG("setupUI: Starting window setup");
    
    // Set a reasonable size first
    resize(1280, 800);
    setMinimumSize(800, 600);  // Ensure minimum size
    
    DEBUG_LOG("setupUI: Window resized to 1280x800");
    
    // Set window icon (may fail silently if icon doesn't exist)
    setWindowIcon(QIcon(":/icons/camera.png"));
    
    // Apply futuristic cyberpunk theme
    ComponentsForest::MaterialTheme::applyMaterialTheme(this);
    
    // Add Material Design elevation to main window
    ComponentsForest::MaterialTheme::addElevation(this, 0);  // Main window has no elevation
    
    // Material Design doesn't use pulse animations
    
    // Set window properties for visibility
    setWindowTitle(tr("Do3Think Camera Viewer - Material Design"));
    
    // Enable styled background
    setAttribute(Qt::WA_StyledBackground, true);
    
    // Force window to be visible
    setAttribute(Qt::WA_ShowWithoutActivating, false);
    setWindowState(Qt::WindowActive);
    
    DEBUG_LOG("setupUI: Window setup complete");
}

void Do3ThinkCameraViewerMainUI::createMenus()
{
    // File menu
    m_fileMenu = menuBar()->addMenu(tr("&File"));
    
    m_settingsAction = m_fileMenu->addAction(tr("&Settings..."));
    m_settingsAction->setShortcut(QKeySequence::Preferences);
    connect(m_settingsAction, &QAction::triggered, this, &Do3ThinkCameraViewerMainUI::onShowSettings);
    
    m_fileMenu->addSeparator();
    
    m_exitAction = m_fileMenu->addAction(tr("E&xit"));
    m_exitAction->setShortcut(QKeySequence::Quit);
    connect(m_exitAction, &QAction::triggered, this, &QWidget::close);
    
    // Camera menu
    m_cameraMenu = menuBar()->addMenu(tr("&Camera"));
    
    m_addCameraAction = m_cameraMenu->addAction(tr("&Add Camera"));
    m_addCameraAction->setShortcut(QKeySequence("Ctrl+N"));
    connect(m_addCameraAction, &QAction::triggered, this, &Do3ThinkCameraViewerMainUI::onAddCamera);
    
    m_removeCameraAction = m_cameraMenu->addAction(tr("&Remove Camera"));
    m_removeCameraAction->setShortcut(QKeySequence::Delete);
    connect(m_removeCameraAction, &QAction::triggered, this, &Do3ThinkCameraViewerMainUI::onRemoveCamera);
    
    m_cameraMenu->addSeparator();
    
    m_startAllPanelsAction = m_cameraMenu->addAction(tr("Start &All"));
    m_startAllPanelsAction->setShortcut(QKeySequence("Ctrl+Shift+S"));
    connect(m_startAllPanelsAction, &QAction::triggered, this, &Do3ThinkCameraViewerMainUI::onStartAllPanels);
    
    m_stopAllPanelsAction = m_cameraMenu->addAction(tr("S&top All"));
    m_stopAllPanelsAction->setShortcut(QKeySequence("Ctrl+Shift+T"));
    connect(m_stopAllPanelsAction, &QAction::triggered, this, &Do3ThinkCameraViewerMainUI::onStopAllPanels);
    
    m_cameraMenu->addSeparator();
    
    m_refreshDevicesAction = m_cameraMenu->addAction(tr("&Refresh Devices"));
    m_refreshDevicesAction->setShortcut(QKeySequence::Refresh);
    connect(m_refreshDevicesAction, &QAction::triggered, this, &Do3ThinkCameraViewerMainUI::onRefreshDevices);
    
    // Processing menu
    m_processingMenu = menuBar()->addMenu(tr("&Processing"));
    
    // Add Preprocess action to toggle preprocessor dock
    m_preprocessAction = m_processingMenu->addAction(tr("&Preprocess"));
    m_preprocessAction->setShortcut(QKeySequence("Ctrl+P"));
    m_preprocessAction->setCheckable(true);
    m_preprocessAction->setChecked(false);  // Start unchecked (hidden)
    m_preprocessAction->setToolTip(tr("Show/Hide preprocessing controls"));
    connect(m_preprocessAction, &QAction::triggered, [this](bool checked) {
        if (m_imageProcessingDock) {
            if (checked) {
                // First time showing - make it floating for optional feel
                if (!m_imageProcessingDock->isVisible()) {
                    m_imageProcessingDock->setFloating(true);
                    // Position it nicely
                    QRect mainGeometry = geometry();
                    m_imageProcessingDock->move(mainGeometry.right() - 400, mainGeometry.top() + 100);
                    m_imageProcessingDock->resize(380, 600);
                }
                m_imageProcessingDock->show();
                m_imageProcessingDock->raise();
            } else {
                m_imageProcessingDock->hide();
            }
        }
    });
    
    // Connect dock visibility to action state
    if (m_imageProcessingDock) {
        connect(m_imageProcessingDock, &QDockWidget::visibilityChanged,
                [this](bool visible) {
                    if (m_preprocessAction) {
                        m_preprocessAction->setChecked(visible);
                    }
                });
    }
    
    // View menu
    m_viewMenu = menuBar()->addMenu(tr("&View"));
    
    m_fullScreenAction = m_viewMenu->addAction(tr("&Full Screen"));
    m_fullScreenAction->setShortcut(QKeySequence::FullScreen);
    m_fullScreenAction->setCheckable(true);
    connect(m_fullScreenAction, &QAction::triggered, this, &Do3ThinkCameraViewerMainUI::onToggleFullScreen);
    
    m_viewMenu->addSeparator();
    
    QAction* changeLayoutAction = m_viewMenu->addAction(tr("Change &Layout"));
    changeLayoutAction->setShortcut(QKeySequence("Ctrl+L"));
    connect(changeLayoutAction, &QAction::triggered, this, &Do3ThinkCameraViewerMainUI::onChangeLayout);
    
    m_viewMenu->addSeparator();
    
    // Add dock widget visibility toggles
    if (m_deviceListDock) {
        QAction* devicesAction = m_deviceListDock->toggleViewAction();
        devicesAction->setText(tr("&Devices Panel"));
        m_viewMenu->addAction(devicesAction);
    }
    if (m_logDock) {
        QAction* logAction = m_logDock->toggleViewAction();
        logAction->setText(tr("&Log Panel"));
        m_viewMenu->addAction(logAction);
    }
    if (m_propertiesDock) {
        QAction* propAction = m_propertiesDock->toggleViewAction();
        propAction->setText(tr("&Properties Panel"));
        m_viewMenu->addAction(propAction);
    }
    // Removed - we're using our custom m_preprocessAction instead
    // The custom action provides better control over dock behavior
    
    // Help menu
    m_helpMenu = menuBar()->addMenu(tr("&Help"));
    
    m_aboutAction = m_helpMenu->addAction(tr("&About"));
    connect(m_aboutAction, &QAction::triggered, this, &Do3ThinkCameraViewerMainUI::onShowAbout);
}

void Do3ThinkCameraViewerMainUI::createToolBars()
{
    // Main toolbar
    m_mainToolBar = addToolBar(tr("Main"));
    m_mainToolBar->addAction(m_addCameraAction);
    m_mainToolBar->addAction(m_removeCameraAction);
    m_mainToolBar->addSeparator();
    m_mainToolBar->addAction(m_refreshDevicesAction);
    
    // Camera toolbar
    m_cameraToolBar = addToolBar(tr("Camera"));
    m_cameraToolBar->addAction(m_startAllPanelsAction);
    m_cameraToolBar->addAction(m_stopAllPanelsAction);
    
    // Processing toolbar
    m_processingToolBar = addToolBar(tr("Processing"));
    m_processingToolBar->addAction(m_preprocessAction);
    
    // Add visual separator between camera and processing tools
    m_cameraToolBar->addSeparator();
    m_cameraToolBar->addAction(m_preprocessAction);  // Also add to main toolbar for convenience
}

void Do3ThinkCameraViewerMainUI::createStatusBar()
{
    // Configure status bar - temporarily disable custom styling for debugging
    QStatusBar* sb = statusBar();
    // Commented out for debugging - may be causing visibility issues
    /*sb->setStyleSheet(
        "QStatusBar {"
        "    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,"
        "                                stop: 0 #161b22, stop: 1 #0d1117);"
        "    border-top: 1px solid #30363d;"
        "    color: #8b949e;"
        "    padding: 4px;"
        "}"
        "QStatusBar::item {"
        "    border: none;"
        "}"
        "QLabel {"
        "    color: #f0f6fc;"
        "    padding: 0 8px;"
        "}"
    );*/
    
    // Add modern status indicators with icons
    QLabel* statusIcon = new QLabel();
    statusIcon->setPixmap(QPixmap(":/icons/status-ready.png").scaled(16, 16, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    sb->addWidget(statusIcon);
    
    QLabel* statusText = new QLabel(tr("Ready"));
    // statusText->setStyleSheet("color: #3fb950; font-weight: 600;");
    sb->addWidget(statusText);
    
    // Add separator
    QLabel* separator1 = new QLabel("|");
    // separator1->setStyleSheet("color: #30363d; padding: 0 4px;");
    sb->addPermanentWidget(separator1);
    
    // Add camera count indicator
    QLabel* cameraCountLabel = new QLabel(tr("Cameras: 0"));
    cameraCountLabel->setObjectName("CameraCountLabel");
    sb->addPermanentWidget(cameraCountLabel);
    
    // Add separator
    QLabel* separator2 = new QLabel("|");
    sb->addPermanentWidget(separator2);
    
    // Add processor count indicator
    QLabel* processorCountLabel = new QLabel(tr("Processors: 0"));
    processorCountLabel->setObjectName("ProcessorCountLabel");
    sb->addPermanentWidget(processorCountLabel);
    
    // Add separator
    QLabel* separator3 = new QLabel("|");
    sb->addPermanentWidget(separator3);
    
    // Add FPS indicator (will be updated by camera panels)
    QLabel* fpsLabel = new QLabel(tr("FPS: --"));
    fpsLabel->setObjectName("FPSLabel");
    // fpsLabel->setStyleSheet("color: #58a6ff;");
    sb->addPermanentWidget(fpsLabel);
    
    // Add separator
    QLabel* separator4 = new QLabel("|");
    // separator4->setStyleSheet("color: #30363d; padding: 0 4px;");
    sb->addPermanentWidget(separator4);
    
    // Add memory usage indicator
    QLabel* memLabel = new QLabel(tr("Memory: --"));
    memLabel->setObjectName("MemoryLabel");
    // memLabel->setStyleSheet("color: #d29922;");
    sb->addPermanentWidget(memLabel);
    
    sb->showMessage(tr("Application started successfully"), 3000);
}

void Do3ThinkCameraViewerMainUI::createDockWidgets()
{
    // Device list dock
    m_deviceListDock = new QDockWidget(tr("Devices"), this);
    m_deviceListDock->setObjectName("devicesDock");  // Set object name for styling
    m_deviceListWidget = new QListWidget();
    m_deviceListDock->setWidget(m_deviceListWidget);
    addDockWidget(Qt::LeftDockWidgetArea, m_deviceListDock);
    
    // Log dock
    m_logDock = new QDockWidget(tr("Log"), this);
    m_logDock->setObjectName("logDock");  // Set object name for styling
    m_logTextEdit = new QTextEdit();
    m_logTextEdit->setReadOnly(true);
    m_logTextEdit->document()->setMaximumBlockCount(1000);
    m_logDock->setWidget(m_logTextEdit);
    addDockWidget(Qt::BottomDockWidgetArea, m_logDock);
    
    // Properties dock
    m_propertiesDock = new QDockWidget(tr("Properties"), this);
    m_propertiesDock->setObjectName("propertiesDock");  // Set object name for styling
    m_propertiesTabWidget = new QTabWidget();
    m_propertiesDock->setWidget(m_propertiesTabWidget);
    addDockWidget(Qt::RightDockWidgetArea, m_propertiesDock);
    
    // Image Processing dock - NEW: Dedicated dock for preprocessor controls
    m_imageProcessingDock = new QDockWidget(tr("Image Processing"), this);
    m_imageProcessingDock->setObjectName("imageProcessingDock");
    m_imageProcessingDock->setFeatures(QDockWidget::DockWidgetMovable | 
                                       QDockWidget::DockWidgetFloatable | 
                                       QDockWidget::DockWidgetClosable);
    
    // Create tab widget for preprocessor panels
    m_imageProcessingTabWidget = new QTabWidget();
    m_imageProcessingTabWidget->setTabsClosable(true);
    m_imageProcessingTabWidget->setDocumentMode(true);
    m_imageProcessingTabWidget->setElideMode(Qt::ElideRight);
    
    // Connect tab close signal
    connect(m_imageProcessingTabWidget, &QTabWidget::tabCloseRequested,
            [this](int index) {
                QWidget* widget = m_imageProcessingTabWidget->widget(index);
                for (auto it = m_preprocessorPanels.begin(); it != m_preprocessorPanels.end(); ++it) {
                    if (it.value() == widget) {
                        removePreprocessorPanel(it.key());
                        break;
                    }
                }
            });
    
    m_imageProcessingDock->setWidget(m_imageProcessingTabWidget);
    addDockWidget(Qt::RightDockWidgetArea, m_imageProcessingDock);
    
    // Tabify the Properties and Image Processing docks to save space
    tabifyDockWidget(m_propertiesDock, m_imageProcessingDock);
    m_propertiesDock->raise();  // Make Properties dock the default active tab
    
    // Start with Image Processing dock hidden by default
    // It will only appear when user clicks the "Preprocess" action
    m_imageProcessingDock->hide();
    m_imageProcessingDock->setFloating(false);  // Start docked, but hidden
}

void Do3ThinkCameraViewerMainUI::createCentralWidget()
{
    m_centralContainer = new QWidget();
    
    // Apply background styling to central widget - commented out for debugging
    m_centralContainer->setObjectName("CentralContainer");
    /* m_centralContainer->setStyleSheet(
        "#CentralContainer {"
        "    background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 1,"
        "                                stop: 0 #0d1117, stop: 0.5 #161b22, stop: 1 #0d1117);"
        "    border-radius: 8px;"
        "}"
    ); */
    
    // Create tab widget for tab view mode with modern styling
    m_cameraTabWidget = new QTabWidget();
    m_cameraTabWidget->setTabsClosable(true);
    m_cameraTabWidget->setDocumentMode(true);  // Modern flat tabs
    m_cameraTabWidget->setElideMode(Qt::ElideRight);
    m_cameraTabWidget->setUsesScrollButtons(true);
    
    // Apply custom tab styling - commented out for debugging
    /* m_cameraTabWidget->setStyleSheet(
        "QTabWidget::pane {"
        "    border: 1px solid #30363d;"
        "    background: #161b22;"
        "    border-radius: 4px;"
        "}"
        "QTabBar::tab {"
        "    padding: 8px 16px;"
        "    margin: 2px;"
        "    border-radius: 4px;"
        "    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,"
        "                                stop: 0 #21262d, stop: 1 #161b22);"
        "}"
        "QTabBar::tab:selected {"
        "    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,"
        "                                stop: 0 #58a6ff, stop: 1 #4a90e2);"
        "    color: white;"
        "}"
        "QTabBar::tab:hover:!selected {"
        "    background: rgba(88, 166, 255, 0.2);"
        "}"
    ); */
    
    connect(m_cameraTabWidget, &QTabWidget::currentChanged,
            this, &Do3ThinkCameraViewerMainUI::onCurrentCameraChanged);
    connect(m_cameraTabWidget, &QTabWidget::tabCloseRequested,
            [this](int index) {
                QWidget* widget = m_cameraTabWidget->widget(index);
                for (auto it = m_panels.begin(); it != m_panels.end(); ++it) {
                    if (it.value() == widget) {
                        emit removeCameraRequested(it.key());
                        break;
                    }
                }
            });
    
    // Create splitter for split view mode with modern styling
    m_cameraSplitter = new QSplitter();
    m_cameraSplitter->setOrientation(Qt::Horizontal);
    m_cameraSplitter->setHandleWidth(8);
    // Splitter styling - commented out for debugging
    /* m_cameraSplitter->setStyleSheet(
        "QSplitter::handle {"
        "    background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0,"
        "                                stop: 0 transparent, stop: 0.5 #30363d, stop: 1 transparent);"
        "}"
        "QSplitter::handle:hover {"
        "    background: #58a6ff;"
        "}"
    ); */
    
    // Default to tab view with modern layout
    QVBoxLayout* layout = new QVBoxLayout(m_centralContainer);
    layout->addWidget(m_cameraTabWidget);
    layout->setContentsMargins(8, 8, 8, 8);  // Add padding for modern look
    layout->setSpacing(8);
    m_cameraSplitter->hide();
    
    setCentralWidget(m_centralContainer);
}

void Do3ThinkCameraViewerMainUI::connectSignals()
{
    // Internal connections are handled in create methods
}

void Do3ThinkCameraViewerMainUI::updateActions()
{
    bool hasPanels = !m_panels.isEmpty();
    bool hasCurrentPanel = !getCurrentCameraId().isEmpty();
    
    m_removeCameraAction->setEnabled(hasCurrentPanel);
    m_startAllPanelsAction->setEnabled(hasPanels);
    m_stopAllPanelsAction->setEnabled(hasPanels);
}

void Do3ThinkCameraViewerMainUI::applyLayoutMode()
{
    QLayout* layout = m_centralContainer->layout();
    
    // Hide all containers first
    m_cameraTabWidget->hide();
    m_cameraSplitter->hide();
    
    switch (m_layoutMode) {
    case TabView:
        m_cameraTabWidget->show();
        break;
        
    case SplitView:
        m_cameraSplitter->show();
        break;
        
    case GridView:
        d->arrangeInGrid(m_panelContainers.values());
        break;
        
    case SingleView:
        // Show only active panel
        if (auto* panel = getActivePanel()) {
            panel->show();
        }
        break;
    }
}

void Do3ThinkCameraViewerMainUI::arrangePanelsInGrid()
{
    d->arrangeInGrid(m_panelContainers.values());
}

void Do3ThinkCameraViewerMainUI::arrangePanelsInTabs()
{
    d->arrangeInTabs(m_panelContainers.values());
}

void Do3ThinkCameraViewerMainUI::arrangePanelsInSplit()
{
    d->arrangeInSplit(m_panelContainers.values());
}

QString Do3ThinkCameraViewerMainUI::getCurrentCameraId() const
{
    if (m_layoutMode == TabView && m_cameraTabWidget) {
        QWidget* current = m_cameraTabWidget->currentWidget();
        for (auto it = m_panels.begin(); it != m_panels.end(); ++it) {
            if (it.value() == current) {
                return it.key();
            }
        }
    }
    
    return m_currentCameraId;
}

Do3ThinkCameraControlPanel* Do3ThinkCameraViewerMainUI::getPanel(const QString &cameraId) const
{
    return m_panels.value(cameraId, nullptr);
}

void Do3ThinkCameraViewerMainUI::saveSettings()
{
    QSettings settings("ComponentsForest", "Do3ThinkCameraViewer");
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
    settings.setValue("layoutMode", static_cast<int>(m_layoutMode));
}

void Do3ThinkCameraViewerMainUI::loadSettings()
{
    QSettings settings("ComponentsForest", "Do3ThinkCameraViewer");
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());
    m_layoutMode = static_cast<LayoutMode>(settings.value("layoutMode", TabView).toInt());
    applyLayoutMode();
    
    // CRITICAL: Always hide the image processing dock on startup
    // It should ONLY appear when user explicitly clicks "Preprocess"
    // This overrides any saved state that might have it visible
    if (m_imageProcessingDock) {
        m_imageProcessingDock->hide();
        if (m_preprocessAction) {
            m_preprocessAction->setChecked(false);
        }
    }
}

// Private implementation methods
void Do3ThinkCameraViewerMainUI::Private::arrangeInGrid(const QList<QWidget*>& widgets)
{
    if (widgets.isEmpty()) return;
    
    // Calculate grid dimensions
    int count = widgets.size();
    int cols = qCeil(qSqrt(count));
    int rows = qCeil(count / double(cols));
    
    // Create grid layout
    QGridLayout* gridLayout = new QGridLayout();
    gridLayout->setSpacing(2);
    gridLayout->setContentsMargins(0, 0, 0, 0);
    
    int index = 0;
    for (int row = 0; row < rows && index < count; ++row) {
        for (int col = 0; col < cols && index < count; ++col) {
            gridLayout->addWidget(widgets[index], row, col);
            widgets[index]->show();
            index++;
        }
    }
    
    // Replace central widget layout
    delete q->m_centralContainer->layout();
    q->m_centralContainer->setLayout(gridLayout);
}

void Do3ThinkCameraViewerMainUI::Private::arrangeInTabs(const QList<QWidget*>& widgets)
{
    q->m_cameraTabWidget->clear();
    for (QWidget* widget : widgets) {
        q->m_cameraTabWidget->addTab(widget, widget->objectName());
    }
}

void Do3ThinkCameraViewerMainUI::Private::arrangeInSplit(const QList<QWidget*>& widgets)
{
    // Clear splitter
    while (q->m_cameraSplitter->count() > 0) {
        q->m_cameraSplitter->widget(0)->setParent(nullptr);
    }
    
    // Add widgets to splitter
    for (QWidget* widget : widgets) {
        q->m_cameraSplitter->addWidget(widget);
    }
}

} // namespace ComponentsForest