#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstring>  // For strcmp
#include <chrono>
#include <iomanip>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#include <winuser.h>  // For SetForegroundWindow, BringWindowToTop, etc.
#endif

#include <QApplication>
#include <QThread>
#include <QDockWidget>
#include <QLoggingCategory>
#include <QSettings>
#include <QDir>
#include <QStandardPaths>
#include <QMessageBox>
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QTimer>
#include <QScreen>
#include <QFileInfo>
#include <QWidget>
#include <QLayout>
#include <QJsonObject>
#include <QJsonValue>

#include "main_ui.h"
#include "machine.h"
#include "Do3ThinkCamera/dothink_camera.h"
#include "../../OpenCV/simple_edge_preprocessor.h"
#include "../../OpenCV/preprocessor_control_panel.h"

// Global debug log file
static std::ofstream g_debugLog;
static int g_debugCounter = 0;

// Debug output helper
void debugOutput(const char* file, int line, const std::string& message) {
    g_debugCounter++;
    
    // Get timestamp
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << "[" << std::setfill('0') << std::setw(4) << g_debugCounter << "] ";
    ss << std::put_time(std::localtime(&time_t), "%H:%M:%S");
    ss << "." << std::setfill('0') << std::setw(3) << ms.count() << " ";
    ss << file << ":" << line << " - " << message;
    
    std::string fullMsg = ss.str();
    
    // Output to multiple destinations
    std::cout << fullMsg << std::endl;
    std::cerr << fullMsg << std::endl;
    fprintf(stderr, "%s\n", fullMsg.c_str());
    fflush(stderr);
    
    // Write to log file
    if (g_debugLog.is_open()) {
        g_debugLog << fullMsg << std::endl;
        g_debugLog.flush();
    }
    
#ifdef _WIN32
    // Windows debug output
    OutputDebugStringA((fullMsg + "\n").c_str());
#endif
}

#define DEBUG_LOG(msg) debugOutput(__FILE__, __LINE__, msg)

using namespace ComponentsForest;

// Logging categories
Q_LOGGING_CATEGORY(lcMain, "Do3ThinkViewer.Main")
Q_LOGGING_CATEGORY(lcMachine, "Do3ThinkViewer.Machine")
Q_LOGGING_CATEGORY(lcUI, "Do3ThinkViewer.UI")

// Application configuration
namespace {
    const QString APP_NAME = "Do3Think Camera Viewer";
    const QString APP_VERSION = "1.0.0";
    const QString ORG_NAME = "ComponentsForest";
    const QString ORG_DOMAIN = "componentsforest.com";
}

// Initialize console for Windows GUI apps - DISABLED to fix GUI visibility issue
void initializeConsole() {
    // Console allocation disabled - this is a GUI application
    // The console allocation was interfering with the main window visibility
    // Debug output is still written to debug.log file
    DEBUG_LOG("Console allocation disabled for GUI application");
    
#ifdef _WIN32
    // Ensure standard streams still work for debugging (without allocating a console)
    // This allows cout/cerr to work even without a visible console
    FILE* pCout = nullptr;
    FILE* pCerr = nullptr;
    FILE* pCin = nullptr;
    
    // Try to connect to existing console if launched from command line
    if (GetConsoleWindow() != nullptr) {
        freopen_s(&pCout, "CONOUT$", "w", stdout);
        freopen_s(&pCerr, "CONOUT$", "w", stderr);
        freopen_s(&pCin, "CONIN$", "r", stdin);
        std::ios::sync_with_stdio();
        DEBUG_LOG("Connected to existing console (launched from command line)");
    } else {
        // No console - redirect to NUL to prevent crashes
        freopen_s(&pCout, "NUL", "w", stdout);
        freopen_s(&pCerr, "NUL", "w", stderr);
        freopen_s(&pCin, "NUL", "r", stdin);
        DEBUG_LOG("No console available - streams redirected to NUL");
    }
#else
    DEBUG_LOG("Linux system - console already available");
#endif
}

class ApplicationManager : public QObject
{
    Q_OBJECT

public:
    ApplicationManager(QObject *parent = nullptr)
        : QObject(parent)
        , m_machine(nullptr)
        , m_mainUI(nullptr)
        , m_machineThread(nullptr)
    {
        DEBUG_LOG("ApplicationManager constructor entered");
        DEBUG_LOG("ApplicationManager constructor completed");
    }
    
    ~ApplicationManager()
    {
        DEBUG_LOG("ApplicationManager destructor entered");
        cleanup();
        DEBUG_LOG("ApplicationManager destructor completed");
    }
    
    bool initialize()
    {
        DEBUG_LOG("ApplicationManager::initialize() entered");
        
        try {
            qCInfo(lcMain) << "Initializing" << APP_NAME << "version" << APP_VERSION;
            DEBUG_LOG("Qt logging initialized");
            
            // Create machine thread
            DEBUG_LOG("Creating machine thread...");
            m_machineThread = new QThread(this);
            if (!m_machineThread) {
                DEBUG_LOG("ERROR: Failed to create machine thread");
                return false;
            }
            m_machineThread->setObjectName("MachineThread");
            DEBUG_LOG("Machine thread created successfully");
            
            // Create machine and move to thread
            DEBUG_LOG("Creating Do3ThinkCameraMachine...");
            m_machine = new Do3ThinkCameraMachine();
            if (!m_machine) {
                DEBUG_LOG("ERROR: Failed to create machine");
                return false;
            }
            DEBUG_LOG("Machine created, moving to thread...");
            m_machine->moveToThread(m_machineThread);
            DEBUG_LOG("Machine moved to thread");
            
            // Create main UI
            DEBUG_LOG("Creating main UI window...");
            m_mainUI = new Do3ThinkCameraViewerMainUI();
            if (!m_mainUI) {
                DEBUG_LOG("ERROR: Failed to create main UI");
                return false;
            }
            DEBUG_LOG("Main UI created, setting window title...");
            m_mainUI->setWindowTitle(QString("%1 v%2").arg(APP_NAME, APP_VERSION));
            DEBUG_LOG("Window title set");
            
            // Apply enhanced modern theme with distinct dock widget titles
            DEBUG_LOG("Applying enhanced modern theme...");
            applyEnhancedModernTheme();
            DEBUG_LOG("Theme applied");
            
            // Connect machine lifecycle
            DEBUG_LOG("Connecting machine lifecycle signals...");
            connect(m_machineThread, &QThread::started,
                    m_machine, &Do3ThinkCameraMachine::start);
            connect(m_machineThread, &QThread::finished,
                    m_machine, &Do3ThinkCameraMachine::deleteLater);
            DEBUG_LOG("Machine lifecycle connected");
            
            // Connect UI to Machine signals
            DEBUG_LOG("Connecting UI to Machine signals...");
            connectUIToMachine();
            DEBUG_LOG("UI to Machine connected");
            
            // Connect Machine to UI signals  
            DEBUG_LOG("Connecting Machine to UI signals...");
            connectMachineToUI();
            DEBUG_LOG("Machine to UI connected");
            
            // Start machine thread
            DEBUG_LOG("Starting machine thread...");
            m_machineThread->start();
            DEBUG_LOG("Machine thread started");
            
            // Connect UI to machine for component access
            DEBUG_LOG("Connecting UI to machine for component access...");
            m_mainUI->connectToMachine(m_machine);
            DEBUG_LOG("UI connected to machine");
            
            // Load settings and restore state
            DEBUG_LOG("Loading application settings...");
            loadApplicationSettings();
            DEBUG_LOG("Settings loaded");
            
            // Auto-discover devices on startup
            DEBUG_LOG("Scheduling device discovery...");
            QMetaObject::invokeMethod(m_machine, "refreshDeviceList", 
                                      Qt::QueuedConnection);
            DEBUG_LOG("Device discovery scheduled");
            
            DEBUG_LOG("ApplicationManager::initialize() completed successfully");
            return true;
            
        } catch (const std::exception& e) {
            DEBUG_LOG(std::string("EXCEPTION in initialize: ") + e.what());
            return false;
        } catch (...) {
            DEBUG_LOG("UNKNOWN EXCEPTION in initialize");
            return false;
        }
    }
    
    Do3ThinkCameraViewerMainUI* getMainUI() { return m_mainUI; }
    
    void show()
    {
        DEBUG_LOG("ApplicationManager::show() entered");
        
        if (m_mainUI) {
            DEBUG_LOG("m_mainUI exists, attempting to show window");
            
            // Get screen information
            QScreen* screen = QGuiApplication::primaryScreen();
            if (screen) {
                QRect screenGeometry = screen->availableGeometry();
                DEBUG_LOG(QString("Primary screen: %1").arg(screen->name()).toStdString());
                DEBUG_LOG(QString("Screen geometry: %1x%2 at (%3,%4)")
                    .arg(screenGeometry.width())
                    .arg(screenGeometry.height())
                    .arg(screenGeometry.x())
                    .arg(screenGeometry.y()).toStdString());
            } else {
                DEBUG_LOG("WARNING: No primary screen detected!");
            }
            
            // Log window state BEFORE showing
            DEBUG_LOG(QString("Before show - isVisible: %1").arg(m_mainUI->isVisible()).toStdString());
            DEBUG_LOG(QString("Before show - isHidden: %1").arg(m_mainUI->isHidden()).toStdString());
            DEBUG_LOG(QString("Before show - windowState: %1").arg(m_mainUI->windowState()).toStdString());
            DEBUG_LOG(QString("Before show - geometry: %1x%2 at (%3,%4)")
                .arg(m_mainUI->width())
                .arg(m_mainUI->height())
                .arg(m_mainUI->x())
                .arg(m_mainUI->y()).toStdString());
            
            // Clear any problematic window states
            m_mainUI->setWindowState(Qt::WindowNoState);
            DEBUG_LOG("Cleared window states");
            
            // Show the window
            m_mainUI->show();
            DEBUG_LOG("Called m_mainUI->show()");
            
            // Force window to be visible
            m_mainUI->raise();
            DEBUG_LOG("Called m_mainUI->raise()");
            
            m_mainUI->activateWindow();
            DEBUG_LOG("Called m_mainUI->activateWindow()");
            
            m_mainUI->setFocus();
            DEBUG_LOG("Called m_mainUI->setFocus()");
            
            // Process events to ensure window is shown
            QApplication::processEvents();
            DEBUG_LOG("Processed pending events");
            
            // Force a repaint
            m_mainUI->repaint();
            DEBUG_LOG("Forced window repaint");
            
#ifdef Q_OS_WIN
            // Windows-specific code to force window to foreground
            DEBUG_LOG("Applying Windows-specific window activation");
            HWND hwnd = (HWND)m_mainUI->winId();
            if (hwnd) {
                DEBUG_LOG(QString("Got HWND: %1").arg((quintptr)hwnd).toStdString());
                
                // Make sure window is not minimized
                if (IsIconic(hwnd)) {
                    ShowWindow(hwnd, SW_RESTORE);
                    DEBUG_LOG("Restored minimized window");
                }
                
                // Force window to foreground
                SetForegroundWindow(hwnd);
                DEBUG_LOG("Called SetForegroundWindow");
                
                SetActiveWindow(hwnd);
                DEBUG_LOG("Called SetActiveWindow");
                
                ShowWindow(hwnd, SW_SHOW);
                DEBUG_LOG("Called ShowWindow(SW_SHOW)");
                
                BringWindowToTop(hwnd);
                DEBUG_LOG("Called BringWindowToTop");
                
                // Force focus
                SetFocus(hwnd);
                DEBUG_LOG("Called SetFocus");
                
                // Update window
                UpdateWindow(hwnd);
                DEBUG_LOG("Called UpdateWindow");
            } else {
                DEBUG_LOG("ERROR: Failed to get HWND for main window");
            }
#endif
            
            // Center window on screen
            if (screen) {
                QRect screenGeometry = screen->availableGeometry();
                int x = (screenGeometry.width() - m_mainUI->width()) / 2 + screenGeometry.x();
                int y = (screenGeometry.height() - m_mainUI->height()) / 2 + screenGeometry.y();
                m_mainUI->move(x, y);
                DEBUG_LOG(QString("Centered window at (%1,%2)").arg(x).arg(y).toStdString());
            }
            
            // Log window state AFTER showing
            DEBUG_LOG(QString("After show - isVisible: %1").arg(m_mainUI->isVisible()).toStdString());
            DEBUG_LOG(QString("After show - isHidden: %1").arg(m_mainUI->isHidden()).toStdString());
            DEBUG_LOG(QString("After show - isMinimized: %1").arg(m_mainUI->isMinimized()).toStdString());
            DEBUG_LOG(QString("After show - isMaximized: %1").arg(m_mainUI->isMaximized()).toStdString());
            DEBUG_LOG(QString("After show - isFullScreen: %1").arg(m_mainUI->isFullScreen()).toStdString());
            DEBUG_LOG(QString("After show - isActiveWindow: %1").arg(m_mainUI->isActiveWindow()).toStdString());
            DEBUG_LOG(QString("After show - windowState: %1").arg(m_mainUI->windowState()).toStdString());
            DEBUG_LOG(QString("After show - geometry: %1x%2 at (%3,%4)")
                .arg(m_mainUI->width())
                .arg(m_mainUI->height())
                .arg(m_mainUI->x())
                .arg(m_mainUI->y()).toStdString());
            
            // Check if window is actually visible to the user
            if (!m_mainUI->isVisible()) {
                DEBUG_LOG("ERROR: Window is not visible after show()!");
                
                // Try alternative show methods
                m_mainUI->setVisible(true);
                DEBUG_LOG("Called setVisible(true)");
                
                m_mainUI->showNormal();
                DEBUG_LOG("Called showNormal()");
                
                // Force a repaint
                m_mainUI->update();
                QApplication::processEvents();
                DEBUG_LOG("Forced repaint and processed events");
            }
            
            // List all top-level widgets
            QWidgetList topLevel = QApplication::topLevelWidgets();
            DEBUG_LOG(QString("Number of top-level widgets: %1").arg(topLevel.count()).toStdString());
            for (int i = 0; i < topLevel.count(); ++i) {
                QWidget* w = topLevel[i];
                DEBUG_LOG(QString("  Widget %1: %2, visible=%3, class=%4")
                    .arg(i)
                    .arg(w->objectName())
                    .arg(w->isVisible())
                    .arg(w->metaObject()->className()).toStdString());
            }
            
            // Check active window
            QWidget* activeWindow = QApplication::activeWindow();
            if (activeWindow) {
                DEBUG_LOG(QString("Active window: %1").arg(activeWindow->objectName()).toStdString());
            } else {
                DEBUG_LOG("No active window!");
            }
            
            qCInfo(lcMain) << "Application UI shown";
            
            // Show a test message box to verify Qt windows can appear
            // DEBUG_LOG("Showing test message box to verify Qt GUI functionality");
            // QMessageBox::information(nullptr, "Do3Think Camera Viewer",
            //     "Application started successfully!\n\n"
            //     "The main window should now be visible.\n"
            //     "If you cannot see the main window after closing this dialog,\n"
            //     "please check the debug.log file for troubleshooting information.");
            // DEBUG_LOG("Test message box closed");
            
            // After message box, ensure main window is still on top
            m_mainUI->raise();
            m_mainUI->activateWindow();
            
            // Set up a timer to check window visibility after event loop starts
            QTimer::singleShot(1000, [this]() {
                DEBUG_LOG("=== Window visibility check after 1 second ===");
                if (m_mainUI) {
                    DEBUG_LOG(QString("Window isVisible: %1").arg(m_mainUI->isVisible()).toStdString());
                    DEBUG_LOG(QString("Window geometry: %1x%2 at (%3,%4)")
                        .arg(m_mainUI->width())
                        .arg(m_mainUI->height())
                        .arg(m_mainUI->x())
                        .arg(m_mainUI->y()).toStdString());
                    
                    if (!m_mainUI->isVisible()) {
                        DEBUG_LOG("Window still not visible! Attempting emergency show...");
                        
#ifdef Q_OS_WIN
                        // Try Windows-specific recovery
                        HWND hwnd = (HWND)m_mainUI->winId();
                        if (hwnd) {
                            ShowWindow(hwnd, SW_SHOWNORMAL);
                            SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, 
                                       SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
                            SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, 
                                       SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
                            DEBUG_LOG("Applied emergency Windows show commands");
                        }
#endif
                        
                        // Try Qt recovery
                        m_mainUI->setWindowFlags(Qt::Window | Qt::WindowStaysOnTopHint);
                        m_mainUI->show();
                        QTimer::singleShot(100, [this]() {
                            m_mainUI->setWindowFlags(Qt::Window);
                            m_mainUI->show();
                        });
                        DEBUG_LOG("Applied emergency Qt show commands");
                    }
                }
            });
            
        } else {
            DEBUG_LOG("ERROR: m_mainUI is null!");
        }
        
        DEBUG_LOG("ApplicationManager::show() completed");
    }
    
    void cleanup()
    {
        qCInfo(lcMain) << "Cleaning up application";
        
        // Save settings
        saveApplicationSettings();
        
        // Clear all panels (panels will handle stopping their cameras)
        if (m_mainUI) {
            m_mainUI->clearAllPanels();
        }
        
        // Stop machine thread
        if (m_machineThread) {
            m_machineThread->quit();
            if (!m_machineThread->wait(5000)) {
                qCWarning(lcMain) << "Machine thread failed to stop gracefully";
                m_machineThread->terminate();
                m_machineThread->wait();
            }
        }
        
        // Delete UI
        delete m_mainUI;
        m_mainUI = nullptr;
    }

private:
    void applyEnhancedModernTheme()
    {
        // Enhanced modern theme with SQUARE dock widgets and proper margins
        QString enhancedTheme = R"(
            /* Base application styling */
            QMainWindow {
                background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 1,
                                          stop: 0 #0d1117, stop: 0.5 #161b22, stop: 1 #0d1117);
            }
            
            /* Separator styling */
            QMainWindow::separator {
                width: 1px;
                height: 1px;
                margin: 0px;
                padding: 0px;
                background: #30363d;
            }
            
            /* Square Dock Widget Styling */
            QDockWidget {
                background-color: #161b22;
                border: none;
                border-radius: 0px;  /* Square shape */
                font-family: 'Segoe UI', Arial, sans-serif;
                margin: 0px;
                padding: 0px;
                titlebar-close-icon: none;
                titlebar-normal-icon: none;
            }
            
            /* Square dock title with proper margins for text visibility */
            QDockWidget::title {
                text-align: left;
                padding: 12px 20px;  /* Normal padding for text visibility */
                font-size: 13px;
                font-weight: 600;
                color: #e0e6ed;
                text-shadow: 0 1px 2px rgba(0, 0, 0, 0.5);
                text-transform: uppercase;
                letter-spacing: 1.5px;
                
                /* Slight extension without extreme margins */
                margin: 0px -2px;  /* Just slight extension */
                
                /* Standard positioning */
                subcontrol-origin: margin;
                subcontrol-position: top left;
                position: relative;
                left: -2px;  /* Slight offset */
                
                /* Width with slight extension */
                width: calc(100% + 4px);  /* Slight width extension */
                box-sizing: border-box;
                
                /* Square visual styling */
                border: none;
                border-radius: 0px;  /* Square edges */
                border-bottom: 1px solid rgba(255, 255, 255, 0.05);
                box-shadow: inset 0 -1px 0 rgba(0, 0, 0, 0.3);
            }
            
            /* Devices Dock - Square with subtle gradient */
            QDockWidget#devicesDock::title,
            QDockWidget[windowTitle="Devices"]::title {
                background: linear-gradient(135deg, #1b2025 0%, #232830 100%);
                margin: 0px -1px;
                padding: 10px 16px;
                padding-left: 20px;  /* Adequate left padding */
                border-radius: 0px;  /* Square */
                width: calc(100% + 2px);
                left: -1px;
                position: relative;
            }
            
            /* Properties Dock - Square with subtle gradient */
            QDockWidget#propertiesDock::title,
            QDockWidget[windowTitle="Properties"]::title {
                background: linear-gradient(135deg, #1e2024 0%, #26282d 100%);
                margin: 0px -1px;
                padding: 10px 16px;
                padding-left: 20px;  /* Adequate left padding */
                border-radius: 0px;  /* Square */
                width: calc(100% + 2px);
                left: -1px;
                position: relative;
            }
            
            /* Log Dock - Square with subtle gradient */
            QDockWidget#logDock::title,
            QDockWidget[windowTitle="Log"]::title {
                background: linear-gradient(135deg, #201e22 0%, #28262b 100%);
                margin: 0px -1px;
                padding: 10px 16px;
                padding-left: 20px;  /* Adequate left padding */
                border-radius: 0px;  /* Square */
                width: calc(100% + 2px);
                left: -1px;
                position: relative;
            }
            
            /* Hover effects with square design */
            QDockWidget::title:hover {
                background: linear-gradient(135deg, #252a30 0%, #2d323a 100%);
                box-shadow: inset 0 -1px 0 rgba(0, 0, 0, 0.3),
                           0 0 8px rgba(255, 255, 255, 0.02);
                /* Maintain proper margins on hover */
                margin: 0px -2px;
                width: calc(100% + 4px);
                left: -2px;
                border-radius: 0px;  /* Keep square on hover */
            }
            
            /* Square dock buttons */
            QDockWidget::close-button, QDockWidget::float-button {
                background: transparent;
                border: none;
                padding: 4px;
                border-radius: 0px;  /* Square buttons */
                margin: 2px 6px 2px 2px;
                subcontrol-origin: margin;
                subcontrol-position: top right;
            }
            
            QDockWidget::close-button:hover {
                background-color: rgba(255, 86, 86, 0.2);
                border-radius: 0px;  /* Square on hover */
            }
            
            QDockWidget::float-button:hover {
                background-color: rgba(88, 166, 255, 0.2);
                border-radius: 0px;  /* Square on hover */
            }
            
            /* Square widget content areas */
            QDockWidget QWidget {
                border-radius: 0px;  /* Square content areas */
            }
            
            /* Square list widgets */
            QListWidget {
                background-color: #0d1117;
                border: 1px solid #30363d;
                border-radius: 0px;  /* Square */
                padding: 4px;
                color: #c9d1d9;
            }
            
            /* Square text edit */
            QTextEdit {
                background-color: #0d1117;
                border: 1px solid #30363d;
                border-radius: 0px;  /* Square */
                padding: 8px;
                color: #c9d1d9;
                font-family: 'Cascadia Code', 'Consolas', monospace;
            }
            
            /* Square tab widget pane */
            QTabWidget::pane {
                background-color: #161b22;
                border: 1px solid #30363d;
                border-radius: 0px;  /* Square */
            }
            
            /* Square tabs */
            QTabBar::tab {
                background: #21262d;
                color: #8b949e;
                padding: 8px 16px;
                margin-right: 2px;
                border-radius: 0px;  /* Square tabs */
                border-top: 2px solid transparent;
            }
            
            QTabBar::tab:selected {
                background: #161b22;
                color: #f0f6fc;
                border-top: 2px solid #58a6ff;
            }
            
            /* Square Scrollbars */
            QScrollBar:vertical {
                background-color: #161b22;
                width: 12px;
                border-radius: 0px;  /* Square */
            }
            
            QScrollBar::handle:vertical {
                background-color: #30363d;
                border-radius: 0px;  /* Square */
                min-height: 20px;
            }
            
            QScrollBar::handle:vertical:hover {
                background-color: #484f58;
                border-radius: 0px;  /* Square */
            }
            
            QScrollBar:horizontal {
                background-color: #161b22;
                height: 12px;
                border-radius: 0px;  /* Square */
            }
            
            QScrollBar::handle:horizontal {
                background-color: #30363d;
                border-radius: 0px;  /* Square */
                min-width: 20px;
            }
            
            QScrollBar::handle:horizontal:hover {
                background-color: #484f58;
                border-radius: 0px;  /* Square */
            }
            
            /* Square Menu bar */
            QMenuBar {
                background: #161b22;
                border-bottom: 1px solid #30363d;
                padding: 4px;
                border-radius: 0px;  /* Square */
            }
            
            QMenuBar::item {
                background: transparent;
                padding: 6px 12px;
                border-radius: 0px;  /* Square */
            }
            
            QMenuBar::item:selected {
                background: rgba(88, 166, 255, 0.15);
                border-radius: 0px;  /* Square */
            }
            
            /* Square Status bar */
            QStatusBar {
                background: #161b22;
                border-top: 1px solid #30363d;
                color: #8b949e;
                border-radius: 0px;  /* Square */
            }
            
            /* Square Buttons */
            QPushButton {
                background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
                                          stop: 0 #21262d, stop: 1 #161b22);
                border: 1px solid #30363d;
                border-radius: 0px;  /* Square */
                padding: 6px 16px;
                color: #c9d1d9;
                font-weight: 500;
            }
            
            QPushButton:hover {
                background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
                                          stop: 0 #30363d, stop: 1 #21262d);
                border: 1px solid #58a6ff;
                border-radius: 0px;  /* Square */
            }
            
            QPushButton:pressed {
                background: #0d1117;
                border-radius: 0px;  /* Square */
            }
        )";
        
        // Apply the theme to the application
        qApp->setStyleSheet(enhancedTheme);
        
        // Clean programmatic adjustments for square design
        if (m_mainUI) {
            // Set proper margins on the main window
            m_mainUI->setContentsMargins(0, 0, 0, 0);
            
            // Get all dock widgets and apply clean square styling
            QList<QDockWidget*> dockWidgets = m_mainUI->findChildren<QDockWidget*>();
            for (QDockWidget* dock : dockWidgets) {
                // Set zero margins on dock widgets for clean edges
                dock->setContentsMargins(0, 0, 0, 0);
                
                // Force update
                dock->updateGeometry();
                dock->update();
            }
            
            // Set proper spacing for central widget
            if (m_mainUI->centralWidget()) {
                m_mainUI->centralWidget()->setContentsMargins(2, 2, 2, 2);
                if (m_mainUI->centralWidget()->layout()) {
                    m_mainUI->centralWidget()->layout()->setContentsMargins(2, 2, 2, 2);
                    m_mainUI->centralWidget()->layout()->setSpacing(2);
                }
            }
            
            // Force a complete repaint
            m_mainUI->update();
            
            DEBUG_LOG("Enhanced modern theme applied with clean square dock design");
        }
    }
    
    void connectUIToMachine()
    {
        // Panel management signals from UI to Manager
        connect(m_mainUI, &Do3ThinkCameraViewerMainUI::addCameraRequested,
                this, &ApplicationManager::onAddCameraRequested,
                Qt::DirectConnection);
                
        connect(m_mainUI, &Do3ThinkCameraViewerMainUI::removeCameraRequested,
                this, &ApplicationManager::onRemoveCameraRequested,
                Qt::QueuedConnection);
                
        // Device discovery
        connect(m_mainUI, &Do3ThinkCameraViewerMainUI::refreshDevicesRequested,
                m_machine, &Do3ThinkCameraMachine::refreshDeviceList,
                Qt::QueuedConnection);
                
        // Note: Camera control is now handled through control panels directly
        // No direct camera control signals from MainUI to Machine
    }
    
    void connectMachineToUI()
    {
        // Machine state signals to UI
        connect(m_machine, &Do3ThinkCameraMachine::machineStarted,
                m_mainUI, &Do3ThinkCameraViewerMainUI::onMachineStarted,
                Qt::QueuedConnection);
                
        connect(m_machine, &Do3ThinkCameraMachine::machineStopped,
                m_mainUI, &Do3ThinkCameraViewerMainUI::onMachineStopped,
                Qt::QueuedConnection);
                
        connect(m_machine, &Do3ThinkCameraMachine::machineError,
                m_mainUI, &Do3ThinkCameraViewerMainUI::onMachineError,
                Qt::QueuedConnection);
                
        // Camera events
        connect(m_machine, &Do3ThinkCameraMachine::cameraAdded,
                this, &ApplicationManager::onCameraAdded,
                Qt::QueuedConnection);
                
        connect(m_machine, &Do3ThinkCameraMachine::cameraRemoved,
                m_mainUI, &Do3ThinkCameraViewerMainUI::onCameraRemoved,
                Qt::QueuedConnection);
                
        connect(m_machine, &Do3ThinkCameraMachine::cameraStarted,
                m_mainUI, &Do3ThinkCameraViewerMainUI::onCameraStarted,
                Qt::QueuedConnection);
                
        connect(m_machine, &Do3ThinkCameraMachine::cameraStopped,
                m_mainUI, &Do3ThinkCameraViewerMainUI::onCameraStopped,
                Qt::QueuedConnection);
                
        connect(m_machine, &Do3ThinkCameraMachine::cameraError,
                m_mainUI, &Do3ThinkCameraViewerMainUI::onCameraError,
                Qt::QueuedConnection);
                
        // Device discovery
        connect(m_machine, &Do3ThinkCameraMachine::devicesDiscovered,
                m_mainUI, &Do3ThinkCameraViewerMainUI::onDevicesDiscovered,
                Qt::QueuedConnection);
    }
    
    void loadApplicationSettings()
    {
        QSettings settings(ORG_NAME, APP_NAME);
        
        // Restore window geometry
        settings.beginGroup("MainWindow");
        if (settings.contains("geometry")) {
            m_mainUI->restoreGeometry(settings.value("geometry").toByteArray());
        }
        if (settings.contains("state")) {
            m_mainUI->restoreState(settings.value("state").toByteArray());
        }
        settings.endGroup();
        
        // Load default camera configuration
        settings.beginGroup("DefaultCamera");
        QVariantMap defaultConfig;
        defaultConfig["resolution"] = settings.value("resolution", "1920x1080").toString();
        defaultConfig["fps"] = settings.value("fps", 30).toInt();
        defaultConfig["exposure"] = settings.value("exposure", 10000).toInt();
        defaultConfig["gain"] = settings.value("gain", 1.0).toDouble();
        m_machine->setDefaultCameraConfig(defaultConfig);
        settings.endGroup();
        
        qCInfo(lcMain) << "Application settings loaded";
    }
    
    void saveApplicationSettings()
    {
        QSettings settings(ORG_NAME, APP_NAME);
        
        // Save window geometry
        settings.beginGroup("MainWindow");
        settings.setValue("geometry", m_mainUI->saveGeometry());
        settings.setValue("state", m_mainUI->saveState());
        settings.endGroup();
        
        // Save default camera configuration
        QVariantMap defaultConfig = m_machine->getDefaultCameraConfig();
        settings.beginGroup("DefaultCamera");
        settings.setValue("resolution", defaultConfig["resolution"]);
        settings.setValue("fps", defaultConfig["fps"]);
        settings.setValue("exposure", defaultConfig["exposure"]);
        settings.setValue("gain", defaultConfig["gain"]);
        settings.endGroup();
        
        settings.sync();
        qCInfo(lcMain) << "Application settings saved";
    }

private slots:
    void onAddCameraRequested()
    {
        // Get available devices
        QStringList devices = m_machine->discoverAvailableDevices();
        
        if (devices.isEmpty()) {
            QMessageBox::warning(m_mainUI, "No Cameras Found",
                                "No Do3Think cameras were detected. Please check connections.");
            return;
        }
        
        // For demo, add first available device
        // In production, show a dialog to select device
        QString deviceName = devices.first();
        QString cameraId = QString("Camera_%1").arg(QDateTime::currentMSecsSinceEpoch());
        
        QVariantMap config = m_machine->getDefaultCameraConfig();
        config["deviceName"] = deviceName;
        config["bufferCount"] = 5;
        config["useCallback"] = true;
        
        // Add camera to machine
        bool success = false;
        QMetaObject::invokeMethod(m_machine, "addCamera",
                                  Qt::BlockingQueuedConnection,
                                  Q_RETURN_ARG(bool, success),
                                  Q_ARG(QString, cameraId),
                                  Q_ARG(QVariantMap, config));
        
        if (!success) {
            QMessageBox::critical(m_mainUI, "Failed to Add Camera",
                                 QString("Failed to add camera: %1").arg(deviceName));
        }
    }
    
    void onRemoveCameraRequested(const QString &cameraId)
    {
        // The control panel should handle stopping its camera before removal
        // Remove panel from UI first
        m_mainUI->removeCameraPanel(cameraId);
        
        // Then remove from machine
        bool success = false;
        QMetaObject::invokeMethod(m_machine, "removeCamera",
                                  Qt::BlockingQueuedConnection,
                                  Q_RETURN_ARG(bool, success),
                                  Q_ARG(QString, cameraId));
        
        if (!success) {
            qCWarning(lcMain) << "Failed to remove camera:" << cameraId;
        }
    }
    
    void onCameraAdded(const QString &cameraId)
    {
        // Get component from machine and add panel to UI
        BaseComponent *component = m_machine->getComponent(cameraId);
        if (component) {
            m_mainUI->addCameraPanel(cameraId, component);
            m_mainUI->onCameraAdded(cameraId);
            qCInfo(lcMain) << "Camera added to UI:" << cameraId;
        } else {
            qCWarning(lcMain) << "Failed to get component for camera:" << cameraId;
        }
    }

private:
    Do3ThinkCameraMachine *m_machine;
    Do3ThinkCameraViewerMainUI *m_mainUI;
    QThread *m_machineThread;
};

#include "main.moc"

int main(int argc, char *argv[])
{
    // Initialize debug log file FIRST
    g_debugLog.open("debug.log", std::ios::out | std::ios::trunc);
    
    std::cout << "[STARTUP] main() entered - argc=" << argc << std::endl;
    std::cerr << "[STARTUP] main() entered - argc=" << argc << std::endl;
    fprintf(stderr, "[STARTUP] main() entered - argc=%d\n", argc);
    fflush(stderr);
    
    DEBUG_LOG("=== DO3THINK CAMERA VIEWER STARTING ===");
    DEBUG_LOG("main() function entered");
    DEBUG_LOG(std::string("Arguments: argc=") + std::to_string(argc));
    
    // Check for special flags
    bool minimalMode = false;
    bool noTheme = false;
    
    for (int i = 0; i < argc; ++i) {
        DEBUG_LOG(std::string("argv[") + std::to_string(i) + "] = " + argv[i]);
        if (strcmp(argv[i], "--minimal") == 0) {
            minimalMode = true;
            DEBUG_LOG("MINIMAL MODE ENABLED - Will skip complex UI");
        }
        if (strcmp(argv[i], "--no-theme") == 0) {
            noTheme = true;
            DEBUG_LOG("THEME DISABLED - Will use default Qt theme");
        }
    }
    
    // If minimal mode, just try to create a basic Qt app and show a message
    if (minimalMode) {
        DEBUG_LOG("=== MINIMAL MODE TEST ===");
        try {
            DEBUG_LOG("Creating minimal QApplication...");
            QApplication minApp(argc, argv);
            DEBUG_LOG("Minimal QApplication created");
            
            DEBUG_LOG("Creating QMessageBox...");
            QMessageBox::information(nullptr, "Minimal Test", 
                "Qt is working!\n\nThis is a minimal test.\n\nClick OK to exit.");
            DEBUG_LOG("Message box closed, exiting minimal mode");
            return 0;
        } catch (...) {
            DEBUG_LOG("FAILED to create minimal Qt application!");
            return -1;
        }
    }
    
    // Initialize console for Windows
    initializeConsole();
    
    try {
        DEBUG_LOG("Setting Qt application attributes...");
        
        // Set application attributes
        QApplication::setApplicationName(APP_NAME);
        DEBUG_LOG("Application name set");
        
        QApplication::setApplicationVersion(APP_VERSION);
        DEBUG_LOG("Application version set");
        
        QApplication::setOrganizationName(ORG_NAME);
        DEBUG_LOG("Organization name set");
        
        QApplication::setOrganizationDomain(ORG_DOMAIN);
        DEBUG_LOG("Organization domain set");
        
        // Enable high DPI support
        DEBUG_LOG("Setting high DPI attributes...");
        QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
        DEBUG_LOG("High DPI scaling enabled");
        
        QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
        DEBUG_LOG("High DPI pixmaps enabled");
        
        // Create application
        DEBUG_LOG("Creating QApplication object...");
        
        // Check environment
        const char* display = std::getenv("DISPLAY");
        if (display) {
            DEBUG_LOG(std::string("DISPLAY environment variable: ") + display);
        } else {
            DEBUG_LOG("DISPLAY environment variable not set");
        }
        
        // Check for platform
        const char* qtPlatform = std::getenv("QT_QPA_PLATFORM");
        if (qtPlatform) {
            DEBUG_LOG(std::string("QT_QPA_PLATFORM: ") + qtPlatform);
        } else {
            DEBUG_LOG("QT_QPA_PLATFORM not set, using default");
        }
        
        // Check Qt plugin paths before QApplication
        DEBUG_LOG("=== Qt Plugin Path Diagnostics ===");
        
        // Check QT_PLUGIN_PATH environment variable
        const char* qtPluginPath = std::getenv("QT_PLUGIN_PATH");
        if (qtPluginPath) {
            DEBUG_LOG(std::string("QT_PLUGIN_PATH: ") + qtPluginPath);
        } else {
            DEBUG_LOG("QT_PLUGIN_PATH not set");
        }
        
        // Check application directory
        QString appDir = QCoreApplication::applicationDirPath();
        DEBUG_LOG(std::string("Application directory: ") + appDir.toStdString());
        
        // Check for platforms directory
        QString platformsDir = appDir + "/platforms";
        QDir pDir(platformsDir);
        if (pDir.exists()) {
            DEBUG_LOG(std::string("Platforms directory exists: ") + platformsDir.toStdString());
            QStringList platformFiles = pDir.entryList(QStringList() << "*", QDir::Files);
            for (const QString& file : platformFiles) {
                DEBUG_LOG(std::string("  Found platform plugin: ") + file.toStdString());
            }
        } else {
            DEBUG_LOG(std::string("WARNING: Platforms directory NOT found: ") + platformsDir.toStdString());
            
            // Try to find Qt installation
            QString qtInstallPath;
#ifdef Q_OS_WIN
            // Common Qt installation paths on Windows
            QStringList possiblePaths = {
                "C:/Qt",
                "D:/Qt",
                QDir::homePath() + "/Qt",
                appDir + "/../..", // Development build location
                appDir + "/../../.."
            };
            
            for (const QString& basePath : possiblePaths) {
                QDir baseDir(basePath);
                if (baseDir.exists()) {
                    DEBUG_LOG(std::string("Checking Qt path: ") + basePath.toStdString());
                    // Look for Qt version directories
                    QStringList versionDirs = baseDir.entryList(QStringList() << "6.*", QDir::Dirs);
                    for (const QString& version : versionDirs) {
                        QString versionPath = basePath + "/" + version;
                        QDir vDir(versionPath);
                        if (vDir.exists()) {
                            // Look for compiler directories
                            QStringList compilerDirs = vDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
                            for (const QString& compiler : compilerDirs) {
                                QString fullPath = versionPath + "/" + compiler + "/plugins/platforms";
                                QDir platformDir(fullPath);
                                if (platformDir.exists()) {
                                    DEBUG_LOG(std::string("Found Qt platforms at: ") + fullPath.toStdString());
                                    qtInstallPath = fullPath;
                                    break;
                                }
                            }
                        }
                        if (!qtInstallPath.isEmpty()) break;
                    }
                }
                if (!qtInstallPath.isEmpty()) break;
            }
            
            if (!qtInstallPath.isEmpty()) {
                DEBUG_LOG("Setting QT_PLUGIN_PATH to found Qt installation");
                QCoreApplication::addLibraryPath(QFileInfo(qtInstallPath).absolutePath());
            }
#endif
        }
        
        // Try to set library paths before QApplication
        QStringList libraryPaths = QCoreApplication::libraryPaths();
        DEBUG_LOG("Library paths before QApplication:");
        for (const QString& path : libraryPaths) {
            DEBUG_LOG(std::string("  ") + path.toStdString());
        }
        
#ifdef Q_OS_WIN
        // Windows-specific error checking
        DEBUG_LOG("=== Windows Error State Before QApplication ===");
        DWORD lastError = GetLastError();
        if (lastError != 0) {
            char* messageBuffer = nullptr;
            FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                          NULL, lastError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                          (LPSTR)&messageBuffer, 0, NULL);
            if (messageBuffer) {
                DEBUG_LOG(std::string("Windows last error: ") + messageBuffer);
                LocalFree(messageBuffer);
            }
            SetLastError(0); // Clear error
        } else {
            DEBUG_LOG("No Windows errors detected");
        }
        
        // Set error mode to get more information
        SetErrorMode(0); // Enable all error dialogs
        DEBUG_LOG("Windows error mode set to show all dialogs");
#endif
        
        QApplication* app = nullptr;
        
        try {
            DEBUG_LOG("=== ATTEMPTING QAPPLICATION CREATION ===");
            DEBUG_LOG("Calling QApplication constructor...");
            
            // Add more granular debugging
            fprintf(stderr, "[PRE-QAPP] About to call QApplication constructor\n");
            fflush(stderr);
            
#ifdef Q_OS_WIN
            // Try to pre-load Qt5Core.dll or Qt6Core.dll to check for missing dependencies
            HMODULE qtCore = LoadLibraryA("Qt6Core.dll");
            if (qtCore) {
                DEBUG_LOG("Qt6Core.dll loaded successfully");
                FreeLibrary(qtCore);
            } else {
                DWORD error = GetLastError();
                DEBUG_LOG(std::string("WARNING: Could not load Qt6Core.dll, error: ") + std::to_string(error));
            }
            
            HMODULE qtGui = LoadLibraryA("Qt6Gui.dll");
            if (qtGui) {
                DEBUG_LOG("Qt6Gui.dll loaded successfully");
                FreeLibrary(qtGui);
            } else {
                DWORD error = GetLastError();
                DEBUG_LOG(std::string("WARNING: Could not load Qt6Gui.dll, error: ") + std::to_string(error));
            }
            
            HMODULE qtWidgets = LoadLibraryA("Qt6Widgets.dll");
            if (qtWidgets) {
                DEBUG_LOG("Qt6Widgets.dll loaded successfully");
                FreeLibrary(qtWidgets);
            } else {
                DWORD error = GetLastError();
                DEBUG_LOG(std::string("WARNING: Could not load Qt6Widgets.dll, error: ") + std::to_string(error));
            }
#endif
            
            app = new QApplication(argc, argv);
            
            fprintf(stderr, "[POST-QAPP] QApplication constructor returned\n");
            fflush(stderr);
            DEBUG_LOG("QApplication constructor returned");
            
            if (!app) {
                DEBUG_LOG("ERROR: QApplication is null after construction");
                return -1;
            }
            
            DEBUG_LOG("QApplication created successfully");
            
            // Get detailed information about the Qt installation
            DEBUG_LOG("=== Qt Application Information ===");
            DEBUG_LOG(std::string("Platform: ") + app->platformName().toStdString());
            DEBUG_LOG(std::string("Application name: ") + app->applicationName().toStdString());
            DEBUG_LOG(std::string("Application path: ") + app->applicationDirPath().toStdString());
            DEBUG_LOG(std::string("Application file path: ") + app->applicationFilePath().toStdString());
            
            // List all available platform plugins
            QStringList paths = app->libraryPaths();
            DEBUG_LOG("Library paths after QApplication:");
            for (const QString& path : paths) {
                DEBUG_LOG(std::string("  ") + path.toStdString());
                
                // Check what's in the platforms subdirectory
                QDir platformDir(path + "/platforms");
                if (platformDir.exists()) {
                    QStringList plugins = platformDir.entryList(QStringList() << "*.dll" << "*.so", QDir::Files);
                    for (const QString& plugin : plugins) {
                        QFileInfo fileInfo(platformDir.absoluteFilePath(plugin));
                        DEBUG_LOG(std::string("    Platform plugin: ") + plugin.toStdString() + 
                                 " (" + std::to_string(fileInfo.size()) + " bytes)");
                    }
                }
            }
            
            // Check if we can actually create widgets
            DEBUG_LOG("Testing Qt widget creation...");
            try {
                QWidget* testWidget = new QWidget();
                if (testWidget) {
                    DEBUG_LOG("Test widget created successfully");
                    delete testWidget;
                } else {
                    DEBUG_LOG("WARNING: Test widget is null");
                }
            } catch (const std::exception& e) {
                DEBUG_LOG(std::string("ERROR: Failed to create test widget: ") + e.what());
            } catch (...) {
                DEBUG_LOG("ERROR: Failed to create test widget (unknown exception)");
            }
            
#ifdef Q_OS_WIN
            // Windows-specific checks after QApplication
            DWORD postError = GetLastError();
            if (postError != 0) {
                char* messageBuffer = nullptr;
                FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                              NULL, postError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                              (LPSTR)&messageBuffer, 0, NULL);
                if (messageBuffer) {
                    DEBUG_LOG(std::string("Windows error after QApplication: ") + messageBuffer);
                    LocalFree(messageBuffer);
                }
            }
#endif
            
        } catch (const std::exception& e) {
            std::stringstream ss;
            ss << "EXCEPTION creating QApplication: " << e.what();
            DEBUG_LOG(ss.str());
            
#ifdef Q_OS_WIN
            // Get Windows error details
            DWORD error = GetLastError();
            if (error != 0) {
                char* messageBuffer = nullptr;
                FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                              NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                              (LPSTR)&messageBuffer, 0, NULL);
                if (messageBuffer) {
                    DEBUG_LOG(std::string("Windows error: ") + messageBuffer);
                    LocalFree(messageBuffer);
                }
            }
#endif
            return -1;
        } catch (...) {
            DEBUG_LOG("UNKNOWN EXCEPTION creating QApplication");
            
#ifdef Q_OS_WIN
            // Get Windows error details
            DWORD error = GetLastError();
            if (error != 0) {
                char* messageBuffer = nullptr;
                FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                              NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                              (LPSTR)&messageBuffer, 0, NULL);
                if (messageBuffer) {
                    DEBUG_LOG(std::string("Windows error: ") + messageBuffer);
                    LocalFree(messageBuffer);
                }
            }
#endif
            return -1;
        }
        
        if (!app) {
            DEBUG_LOG("ERROR: QApplication is null after creation");
            return -1;
        }
        
        // Set up logging
        DEBUG_LOG("Setting up Qt logging categories...");
        QLoggingCategory::setFilterRules(
            "Do3ThinkViewer.*=true\n"
            "ComponentsForest.*=true"
        );
        DEBUG_LOG("Qt logging configured");
        
        // Also setup Qt message handler to redirect to our debug log
        qInstallMessageHandler([](QtMsgType type, const QMessageLogContext &context, const QString &msg) {
            std::string typeStr;
            switch (type) {
                case QtDebugMsg: typeStr = "Qt-Debug"; break;
                case QtInfoMsg: typeStr = "Qt-Info"; break;
                case QtWarningMsg: typeStr = "Qt-Warning"; break;
                case QtCriticalMsg: typeStr = "Qt-Critical"; break;
                case QtFatalMsg: typeStr = "Qt-Fatal"; break;
            }
            debugOutput(context.file ? context.file : "unknown", context.line, 
                       typeStr + ": " + msg.toStdString());
        });
        DEBUG_LOG("Qt message handler installed");
        
        // Create and initialize application manager
        DEBUG_LOG("Creating ApplicationManager...");
        ApplicationManager* manager = nullptr;
        
        try {
            manager = new ApplicationManager();
            DEBUG_LOG("ApplicationManager created successfully");
        } catch (const std::exception& e) {
            DEBUG_LOG(std::string("EXCEPTION creating ApplicationManager: ") + e.what());
            return -1;
        } catch (...) {
            DEBUG_LOG("UNKNOWN EXCEPTION creating ApplicationManager");
            return -1;
        }
        
        if (!manager) {
            DEBUG_LOG("ERROR: ApplicationManager is null");
            return -1;
        }
        
        DEBUG_LOG("Initializing ApplicationManager...");
        if (!manager->initialize()) {
            DEBUG_LOG("ERROR: ApplicationManager initialization failed");
            QMessageBox::critical(nullptr, "Initialization Failed",
                                 "Failed to initialize application components.");
            return -1;
        }
        DEBUG_LOG("ApplicationManager initialized successfully");
        
        // Show main window
        DEBUG_LOG("Showing main window...");
        manager->show();
        DEBUG_LOG("Main window shown");
        
        // DISABLED: Preprocessor panel creation commented out to keep UI clean
        // The preprocessor panel should only be added when explicitly requested by user
        // It crowds the interface and should appear only via the "Preprocess" menu action
        /*
        // Create and add a preprocessor panel for demonstration
        DEBUG_LOG("Adding preprocessor panel for demonstration...");
        try {
            // Create an edge preprocessor
            auto* edgeProcessor = new ComponentsForest::OpenCV::SimpleEdgePreProcessor("DemoEdgeProcessor");
            
            // Create thread for processor
            QThread* processorThread = new QThread(app);
            edgeProcessor->moveToThread(processorThread);
            
            // Initialize processor when thread starts
            QObject::connect(processorThread, &QThread::started, [edgeProcessor]() {
                QVariantMap config;
                config["lowThreshold"] = 50.0;
                config["highThreshold"] = 150.0;
                config["kernelSize"] = 3;
                config["debugMode"] = true;
                
                // Convert to QJsonObject for BaseComponent::initialize
                QJsonObject jsonConfig;
                for (auto it = config.begin(); it != config.end(); ++it) {
                    jsonConfig[it.key()] = QJsonValue::fromVariant(it.value());
                }
                
                edgeProcessor->configurePreprocessor(config);
                edgeProcessor->initialize(jsonConfig);
            });
            
            // Start processor thread
            processorThread->start();
            
            // Add preprocessor panel to UI
            if (manager->getMainUI()) {
                manager->getMainUI()->addPreprocessorPanel("EdgeDetection", edgeProcessor);
                manager->getMainUI()->showStatusMessage("Preprocessor Panel Added: Edge Detection", 5000);
                DEBUG_LOG("Preprocessor panel added to UI");
            } else {
                DEBUG_LOG("ERROR: Could not get main UI to add preprocessor panel");
            }
            
        } catch (const std::exception& e) {
            DEBUG_LOG(std::string("Failed to create preprocessor panel: ") + e.what());
        }
        DEBUG_LOG("Preprocessor panel setup completed");
        */
        
        // Connect application quit
        DEBUG_LOG("Connecting application quit signal...");
        QObject::connect(app, &QApplication::aboutToQuit,
                         manager, &ApplicationManager::cleanup);
        DEBUG_LOG("Quit signal connected");
        
        // Run event loop
        DEBUG_LOG("Starting Qt event loop...");
        int result = app->exec();
        DEBUG_LOG(std::string("Qt event loop exited with code: ") + std::to_string(result));
        
        DEBUG_LOG("Cleaning up...");
        delete manager;
        delete app;
        
        DEBUG_LOG("=== APPLICATION EXITING NORMALLY ===");
        g_debugLog.close();
        
        return result;
        
    } catch (const std::exception& e) {
        DEBUG_LOG(std::string("UNHANDLED EXCEPTION in main: ") + e.what());
        g_debugLog.close();
        return -1;
    } catch (...) {
        DEBUG_LOG("UNKNOWN UNHANDLED EXCEPTION in main");
        g_debugLog.close();
        return -1;
    }
}