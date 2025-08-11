/**
 * @file futuristic_ui_showcase.cpp
 * @brief Showcase and example implementation of the Futuristic UI System
 * 
 * This file demonstrates how to integrate all futuristic UI components
 * into the Do3Think Camera Viewer application
 */

#include "futuristic_theme.h"
#include "futuristic_widgets.h"
#include "main_ui.h"
#include "../../Do3ThinkCamera/dothink_camera_control_panel.h"

#include <QApplication>
#include <QMainWindow>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QDockWidget>
#include <QToolBar>
#include <QMenuBar>
#include <QStatusBar>
#include <QTabWidget>
#include <QTimer>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QGraphicsDropShadowEffect>
#include <memory>

using namespace ComponentsForest::FuturisticTheme;
using namespace ComponentsForest::FuturisticWidgets;

namespace ComponentsForest {

/**
 * @class FuturisticCameraViewer
 * @brief Complete futuristic UI implementation for camera viewer
 */
class FuturisticCameraViewer : public QMainWindow {
    Q_OBJECT
    
public:
    FuturisticCameraViewer(QWidget* parent = nullptr) : QMainWindow(parent) {
        setupWindow();
        createFuturisticUI();
        applyFuturisticEffects();
        startAnimations();
    }
    
private:
    void setupWindow() {
        setWindowTitle("Do3Think Camera Viewer - Futuristic Edition");
        resize(1600, 900);
        setMinimumSize(1280, 720);
        
        // Apply main futuristic theme
        ThemeManager::instance()->applyTheme(this);
        ThemeManager::instance()->setThemeMode("futuristic");
        
        // Enable all special effects
        ThemeManager::instance()->enableAnimations(true);
        ThemeManager::instance()->setAnimationSpeed(7);
        ThemeManager::instance()->setGlowIntensity(8);
        
        // Set neon colors
        ThemeManager::instance()->setNeonColors(
            QColor(0, 212, 255),   // Cyan
            QColor(255, 0, 170)    // Magenta
        );
    }
    
    void createFuturisticUI() {
        // ==================== Central Widget ====================
        auto* centralWidget = new QWidget(this);
        centralWidget->setProperty("holographic", true);
        setCentralWidget(centralWidget);
        
        auto* mainLayout = new QVBoxLayout(centralWidget);
        mainLayout->setSpacing(15);
        mainLayout->setContentsMargins(20, 20, 20, 20);
        
        // ==================== Top Control Bar ====================
        auto* topBar = createTopControlBar();
        mainLayout->addWidget(topBar);
        
        // ==================== Main Content Area ====================
        auto* contentArea = new QHBoxLayout();
        contentArea->setSpacing(15);
        
        // Left Panel - Camera Controls
        auto* leftPanel = createCameraControlPanel();
        contentArea->addWidget(leftPanel, 1);
        
        // Center - Camera Display with Overlay
        auto* centerDisplay = createCameraDisplay();
        contentArea->addWidget(centerDisplay, 3);
        
        // Right Panel - Status and Diagnostics
        auto* rightPanel = createStatusPanel();
        contentArea->addWidget(rightPanel, 1);
        
        mainLayout->addLayout(contentArea, 1);
        
        // ==================== Bottom Status Bar ====================
        createFuturisticStatusBar();
        
        // ==================== Dock Widgets ====================
        createFuturisticDocks();
        
        // ==================== Toolbar ====================
        createFuturisticToolbar();
        
        // ==================== Menu Bar ====================
        createFuturisticMenuBar();
    }
    
    QWidget* createTopControlBar() {
        auto* container = new QWidget();
        container->setProperty("holographic", true);
        container->setMaximumHeight(100);
        
        auto* layout = new QHBoxLayout(container);
        layout->setSpacing(20);
        
        // Logo/Title Section
        auto* titleLabel = new QLabel("CAMERA CONTROL SYSTEM");
        titleLabel->setStyleSheet(R"(
            QLabel {
                color: #00D4FF;
                font-size: 24px;
                font-weight: 700;
                font-family: "Orbitron", monospace;
                letter-spacing: 3px;
                text-transform: uppercase;
            }
        )");
        addGlowEffect(titleLabel, QColor(0, 212, 255), 30);
        layout->addWidget(titleLabel);
        
        layout->addStretch();
        
        // Quick Action Buttons
        auto* btnStart = new NeonGlowButton("START", container);
        btnStart->setNeonColor(QColor(0, 255, 136));
        btnStart->setGlowIntensity(8);
        btnStart->enablePulse(true);
        layout->addWidget(btnStart);
        
        auto* btnStop = new NeonGlowButton("STOP", container);
        btnStop->setNeonColor(QColor(255, 0, 68));
        btnStop->setGlowIntensity(8);
        layout->addWidget(btnStop);
        
        auto* btnCapture = new NeonGlowButton("CAPTURE", container);
        btnCapture->setNeonColor(QColor(0, 212, 255));
        btnCapture->setGlowIntensity(10);
        btnCapture->enablePulse(true);
        layout->addWidget(btnCapture);
        
        // System Status Indicator
        auto* statusRing = new CyberpunkProgressRing(container);
        statusRing->setRange(0, 100);
        statusRing->setValue(75);
        statusRing->setLabel("SYSTEM");
        statusRing->setColors(QColor(0, 255, 136), QColor(0, 212, 255));
        layout->addWidget(statusRing);
        
        return container;
    }
    
    QWidget* createCameraControlPanel() {
        auto* panel = new QGroupBox("CAMERA CONTROLS");
        panel->setProperty("holographic", true);
        
        auto* layout = new QVBoxLayout(panel);
        layout->setSpacing(15);
        
        // Holographic Toggles
        auto* togglesLayout = new QGridLayout();
        
        auto* liveToggle = new HolographicToggle(panel);
        togglesLayout->addWidget(new QLabel("LIVE VIEW"), 0, 0);
        togglesLayout->addWidget(liveToggle, 0, 1);
        
        auto* hdrToggle = new HolographicToggle(panel);
        togglesLayout->addWidget(new QLabel("HDR MODE"), 1, 0);
        togglesLayout->addWidget(hdrToggle, 1, 1);
        
        auto* aiToggle = new HolographicToggle(panel);
        aiToggle->setChecked(true);
        togglesLayout->addWidget(new QLabel("AI ENHANCE"), 2, 0);
        togglesLayout->addWidget(aiToggle, 2, 1);
        
        layout->addLayout(togglesLayout);
        
        // Separator with glow
        auto* separator = new QFrame();
        separator->setFrameShape(QFrame::HLine);
        separator->setStyleSheet("QFrame { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, "
                                "stop:0 transparent, stop:0.5 #00D4FF, stop:1 transparent); "
                                "height: 2px; }");
        layout->addWidget(separator);
        
        // Holographic Sliders
        auto* exposureLabel = new QLabel("EXPOSURE");
        exposureLabel->setStyleSheet("color: #00D4FF; font-weight: 600;");
        layout->addWidget(exposureLabel);
        
        auto* exposureSlider = new HolographicSlider(Qt::Horizontal, panel);
        exposureSlider->setRange(0, 100);
        exposureSlider->setValue(50);
        exposureSlider->setNeonColors(QColor(0, 212, 255), QColor(255, 0, 170));
        exposureSlider->enableWaveAnimation(true);
        layout->addWidget(exposureSlider);
        
        auto* gainLabel = new QLabel("GAIN");
        gainLabel->setStyleSheet("color: #FF00AA; font-weight: 600;");
        layout->addWidget(gainLabel);
        
        auto* gainSlider = new HolographicSlider(Qt::Horizontal, panel);
        gainSlider->setRange(0, 100);
        gainSlider->setValue(30);
        gainSlider->setNeonColors(QColor(255, 0, 170), QColor(0, 153, 255));
        gainSlider->enableWaveAnimation(true);
        layout->addWidget(gainSlider);
        
        // Matrix Data Stream (small preview)
        auto* dataStream = new MatrixDataStream(panel);
        dataStream->setMinimumHeight(100);
        dataStream->setMaximumHeight(150);
        dataStream->setStreamSpeed(5);
        dataStream->setDataDensity(3);
        dataStream->startStream();
        layout->addWidget(dataStream);
        
        layout->addStretch();
        
        // Apply holographic effect to panel
        ThemeManager::instance()->holographicEffect(panel);
        
        return panel;
    }
    
    QWidget* createCameraDisplay() {
        auto* container = new QWidget();
        container->setProperty("holographic", true);
        
        auto* layout = new QVBoxLayout(container);
        layout->setContentsMargins(0, 0, 0, 0);
        
        // Camera view placeholder with particle overlay
        auto* cameraView = new QWidget(container);
        cameraView->setStyleSheet(R"(
            QWidget {
                background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                    stop:0 #0A0E1A, stop:0.5 #141824, stop:1 #0A0E1A);
                border: 2px solid qlineargradient(x1:0, y1:0, x2:1, y2:0,
                    stop:0 #00D4FF, stop:0.5 #FF00AA, stop:1 #00D4FF);
                border-radius: 0px;
            }
        )");
        cameraView->setMinimumSize(640, 480);
        
        // Add particle effect overlay
        auto* particleOverlay = new ParticleEffectOverlay(cameraView);
        particleOverlay->setParticleCount(30);
        particleOverlay->setParticleColor(QColor(0, 212, 255));
        particleOverlay->setEffectType("energy");
        particleOverlay->start();
        particleOverlay->resize(cameraView->size());
        
        // Camera info overlay
        auto* infoOverlay = new QLabel(cameraView);
        infoOverlay->setText("CAMERA 01\nRESOLUTION: 1920x1080\nFPS: 120\nSTATUS: ONLINE");
        infoOverlay->setStyleSheet(R"(
            QLabel {
                color: #00FF88;
                background: rgba(10, 14, 26, 200);
                border: 1px solid #00FF88;
                padding: 10px;
                font-family: "Share Tech Mono", monospace;
                font-size: 12px;
            }
        )");
        infoOverlay->move(10, 10);
        
        layout->addWidget(cameraView);
        
        // Bottom control bar
        auto* controlBar = new QWidget();
        controlBar->setMaximumHeight(60);
        auto* controlLayout = new QHBoxLayout(controlBar);
        
        // Playback controls with neon buttons
        auto* playBtn = new NeonGlowButton("▶", controlBar);
        playBtn->setNeonColor(QColor(0, 255, 136));
        playBtn->setMaximumWidth(50);
        controlLayout->addWidget(playBtn);
        
        auto* pauseBtn = new NeonGlowButton("❚❚", controlBar);
        pauseBtn->setNeonColor(QColor(255, 215, 0));
        pauseBtn->setMaximumWidth(50);
        controlLayout->addWidget(pauseBtn);
        
        auto* stopBtn = new NeonGlowButton("■", controlBar);
        stopBtn->setNeonColor(QColor(255, 0, 68));
        stopBtn->setMaximumWidth(50);
        controlLayout->addWidget(stopBtn);
        
        controlLayout->addStretch();
        
        // Timeline slider
        auto* timeline = new HolographicSlider(Qt::Horizontal, controlBar);
        timeline->setRange(0, 1000);
        timeline->setNeonColors(QColor(0, 212, 255), QColor(255, 0, 170));
        controlLayout->addWidget(timeline, 1);
        
        layout->addWidget(controlBar);
        
        // Add scanline effect to camera display
        ThemeManager::instance()->scanlineEffect(cameraView);
        
        return container;
    }
    
    QWidget* createStatusPanel() {
        auto* panel = new QGroupBox("SYSTEM STATUS");
        panel->setProperty("holographic", true);
        
        auto* layout = new QVBoxLayout(panel);
        layout->setSpacing(15);
        
        // Status Cards
        auto* fpsCard = new HolographicStatusCard("FPS", panel);
        fpsCard->setValue("120");
        fpsCard->setSubtext("FRAMES/SEC");
        fpsCard->setStatus("online");
        fpsCard->setTrend(15.5);
        layout->addWidget(fpsCard);
        
        auto* tempCard = new HolographicStatusCard("TEMP", panel);
        tempCard->setValue("42°C");
        tempCard->setSubtext("SENSOR");
        tempCard->setStatus("warning");
        tempCard->setTrend(-5.2);
        layout->addWidget(tempCard);
        
        auto* memCard = new HolographicStatusCard("MEMORY", panel);
        memCard->setValue("2.4GB");
        memCard->setSubtext("BUFFER USAGE");
        memCard->setStatus("online");
        memCard->setTrend(2.1);
        layout->addWidget(memCard);
        
        // Futuristic Gauge
        auto* cpuGauge = new FuturisticGauge(panel);
        cpuGauge->setRange(0, 100);
        cpuGauge->setValue(65);
        cpuGauge->setUnit("%");
        cpuGauge->setTitle("CPU LOAD");
        cpuGauge->setWarningThreshold(75);
        cpuGauge->setCriticalThreshold(90);
        cpuGauge->setColors(
            QColor(0, 255, 136),   // Normal
            QColor(255, 215, 0),   // Warning
            QColor(255, 0, 68)     // Critical
        );
        layout->addWidget(cpuGauge);
        
        // Cyber Terminal
        auto* terminal = new CyberTerminal(panel);
        terminal->setMaximumHeight(150);
        terminal->appendLine("SYSTEM INITIALIZED", QColor(0, 255, 136));
        terminal->appendLine("CAMERA CONNECTED", QColor(0, 212, 255));
        terminal->appendLine("AI MODULE LOADED", QColor(255, 0, 170));
        terminal->setPrompt("SYS> ");
        layout->addWidget(terminal);
        
        layout->addStretch();
        
        return panel;
    }
    
    void createFuturisticStatusBar() {
        auto* statusBar = this->statusBar();
        statusBar->setStyleSheet(R"(
            QStatusBar {
                background: rgba(20, 24, 36, 0.95);
                border-top: 2px solid qlineargradient(x1:0, y1:0, x2:1, y2:0,
                    stop:0 transparent, stop:0.5 #00D4FF, stop:1 transparent);
                color: #00D4FF;
                font-family: "Share Tech Mono", monospace;
                font-size: 12px;
            }
        )");
        
        // Add status widgets
        auto* statusLabel = new QLabel("SYSTEM: ONLINE");
        statusLabel->setStyleSheet("color: #00FF88; padding: 0 10px;");
        statusBar->addWidget(statusLabel);
        
        auto* fpsLabel = new QLabel("FPS: 120");
        fpsLabel->setStyleSheet("color: #00D4FF; padding: 0 10px;");
        statusBar->addWidget(fpsLabel);
        
        auto* memLabel = new QLabel("MEM: 2.4GB");
        memLabel->setStyleSheet("color: #FF00AA; padding: 0 10px;");
        statusBar->addWidget(memLabel);
        
        // Add animated status indicator
        auto* statusIndicator = new QLabel("●");
        statusIndicator->setStyleSheet("color: #00FF88; padding: 0 10px; font-size: 16px;");
        statusBar->addPermanentWidget(statusIndicator);
        
        // Pulse animation for status indicator
        auto* pulseTimer = new QTimer(this);
        connect(pulseTimer, &QTimer::timeout, [statusIndicator]() {
            static bool bright = false;
            bright = !bright;
            statusIndicator->setStyleSheet(bright ? 
                "color: #00FF88; padding: 0 10px; font-size: 16px;" :
                "color: #00AA44; padding: 0 10px; font-size: 16px;");
        });
        pulseTimer->start(500);
    }
    
    void createFuturisticDocks() {
        // Tools Dock
        auto* toolsDock = new QDockWidget("TOOLS", this);
        toolsDock->setProperty("holographic", true);
        
        auto* toolsWidget = new QWidget();
        auto* toolsLayout = new QVBoxLayout(toolsWidget);
        
        // Add tool buttons
        for (const QString& tool : {"MEASURE", "ANALYZE", "CALIBRATE", "EXPORT"}) {
            auto* btn = new NeonGlowButton(tool, toolsWidget);
            btn->setNeonColor(QColor(0, 212, 255));
            toolsLayout->addWidget(btn);
        }
        
        toolsDock->setWidget(toolsWidget);
        addDockWidget(Qt::LeftDockWidgetArea, toolsDock);
        
        // Info Dock
        auto* infoDock = new QDockWidget("INFORMATION", this);
        infoDock->setProperty("holographic", true);
        
        auto* infoWidget = new QWidget();
        auto* infoLayout = new QVBoxLayout(infoWidget);
        
        auto* infoText = new QTextEdit();
        infoText->setReadOnly(true);
        infoText->setProperty("terminal", true);
        infoText->setPlainText("SYSTEM INFORMATION\n"
                              "==================\n"
                              "MODEL: DO3THINK-X1\n"
                              "FIRMWARE: v2.4.1\n"
                              "RESOLUTION: 4K\n"
                              "INTERFACE: USB3.0\n"
                              "STATUS: OPERATIONAL");
        infoLayout->addWidget(infoText);
        
        infoDock->setWidget(infoWidget);
        addDockWidget(Qt::RightDockWidgetArea, infoDock);
    }
    
    void createFuturisticToolbar() {
        auto* toolbar = addToolBar("Main");
        toolbar->setMovable(false);
        toolbar->setStyleSheet(R"(
            QToolBar {
                background: rgba(20, 24, 36, 0.9);
                border-bottom: 1px solid rgba(0, 212, 255, 0.3);
                spacing: 10px;
                padding: 5px;
            }
        )");
        
        // Add futuristic tool buttons
        auto* connectAction = toolbar->addAction("CONNECT");
        auto* disconnectAction = toolbar->addAction("DISCONNECT");
        toolbar->addSeparator();
        auto* settingsAction = toolbar->addAction("SETTINGS");
        auto* diagnosticsAction = toolbar->addAction("DIAGNOSTICS");
        toolbar->addSeparator();
        auto* recordAction = toolbar->addAction("RECORD");
        auto* snapshotAction = toolbar->addAction("SNAPSHOT");
        
        // Style the actions
        for (auto* action : toolbar->actions()) {
            if (auto* widget = toolbar->widgetForAction(action)) {
                widget->setStyleSheet(R"(
                    QToolButton {
                        color: #00D4FF;
                        background: transparent;
                        border: 1px solid rgba(0, 212, 255, 0.3);
                        padding: 8px 15px;
                        font-weight: 600;
                        text-transform: uppercase;
                    }
                    QToolButton:hover {
                        background: rgba(0, 212, 255, 0.2);
                        border: 1px solid #00D4FF;
                    }
                )");
            }
        }
    }
    
    void createFuturisticMenuBar() {
        auto* menuBar = this->menuBar();
        
        // File Menu
        auto* fileMenu = menuBar->addMenu("FILE");
        fileMenu->addAction("New Session");
        fileMenu->addAction("Open Session");
        fileMenu->addAction("Save Session");
        fileMenu->addSeparator();
        fileMenu->addAction("Export Data");
        fileMenu->addAction("Exit");
        
        // Camera Menu
        auto* cameraMenu = menuBar->addMenu("CAMERA");
        cameraMenu->addAction("Connect");
        cameraMenu->addAction("Disconnect");
        cameraMenu->addSeparator();
        cameraMenu->addAction("Settings");
        cameraMenu->addAction("Calibration");
        
        // View Menu
        auto* viewMenu = menuBar->addMenu("VIEW");
        viewMenu->addAction("Full Screen");
        viewMenu->addAction("Split View");
        viewMenu->addAction("Grid View");
        viewMenu->addSeparator();
        viewMenu->addAction("Show Tools");
        viewMenu->addAction("Show Info");
        
        // Analysis Menu
        auto* analysisMenu = menuBar->addMenu("ANALYSIS");
        analysisMenu->addAction("Histogram");
        analysisMenu->addAction("Waveform");
        analysisMenu->addAction("Vector Scope");
        analysisMenu->addAction("False Color");
        
        // Help Menu
        auto* helpMenu = menuBar->addMenu("HELP");
        helpMenu->addAction("Documentation");
        helpMenu->addAction("About");
    }
    
    void applyFuturisticEffects() {
        // Apply various effects to different widgets
        
        // Main window glow
        ThemeManager::instance()->glowWidget(this, QColor(0, 212, 255));
        
        // Central widget holographic effect
        if (centralWidget()) {
            ThemeManager::instance()->holographicEffect(centralWidget());
        }
        
        // Add particle effects to specific areas
        // These would be applied to camera view areas
    }
    
    void startAnimations() {
        // Create ambient animations for the UI
        
        // Rotating animation for progress rings
        auto* rotationTimer = new QTimer(this);
        connect(rotationTimer, &QTimer::timeout, [this]() {
            // Update any rotating elements
        });
        rotationTimer->start(50);
        
        // Data update timer
        auto* dataTimer = new QTimer(this);
        connect(dataTimer, &QTimer::timeout, [this]() {
            // Update data displays with random values for demo
            static int counter = 0;
            counter++;
            
            // Update status bar
            if (auto* fpsLabel = statusBar()->findChild<QLabel*>()) {
                int fps = 115 + (counter % 10);
                fpsLabel->setText(QString("FPS: %1").arg(fps));
            }
        });
        dataTimer->start(100);
    }
};

} // namespace ComponentsForest

// ==================== Example Usage ====================
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // Set application style
    app.setStyle("Fusion");
    
    // Create and show the futuristic camera viewer
    ComponentsForest::FuturisticCameraViewer viewer;
    viewer.show();
    
    return app.exec();
}

#include "futuristic_ui_showcase.moc"