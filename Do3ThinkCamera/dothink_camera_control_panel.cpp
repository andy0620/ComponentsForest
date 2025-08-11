/**
 * @file dothink_camera_control_panel.cpp
 * @brief Do3Think Camera Control Panel Implementation
 * 
 * Professional industrial UI controls for Do3Think AOI camera equipment
 * Features HDR, color correction, GPIO control, and advanced diagnostics
 */

#include "dothink_camera_control_panel.h"
#include "../viewers/do3think_camera_viewer/material_theme.h"
#include "../viewers/do3think_camera_viewer/futuristic_theme.h"
#include "../viewers/do3think_camera_viewer/futuristic_widgets.h"
#include <QtWidgets>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QPolarChart>
#include <QtCharts/QScatterSeries>
#include <QJsonDocument>
#include <QJsonArray>
#include <QFileDialog>
#include <QMessageBox>
#include <QMatrix3x3>
#include <QMatrix4x4>
#include <QRandomGenerator>
#include <cmath>
#include <algorithm>
#include <numeric>

// Qt Charts namespace - not using namespace directive to avoid conflicts

namespace ComponentsForest {

// Color themes for industrial UI - Now using MaterialTheme
namespace Themes {
    // Using the modern Material Design theme
    inline QString DARK_INDUSTRIAL() { return MaterialTheme::getCompleteStylesheet(); }
    
    // Light theme placeholder - can be expanded later
    inline QString LIGHT_PROFESSIONAL() { return MaterialTheme::getCompleteStylesheet(); }
}

// ============================================================================
// Constructor and Destructor
// ============================================================================

Do3ThinkCameraControlPanel::Do3ThinkCameraControlPanel(QWidget* parent)
    : CameraControlPanel(parent)
{
    setWindowTitle("Do3Think Camera Control Panel");
    
    // Apply futuristic cyberpunk theme
    setStyleSheet(ComponentsForest::FuturisticTheme::getFuturisticStylesheet());
    
    setupDo3ThinkUI();
    connectInternalSignals();
    
    // Initialize timers
    m_diagnosticsTimer = new QTimer(this);
    m_diagnosticsTimer->setInterval(1000); // 1 second update
    connect(m_diagnosticsTimer, &QTimer::timeout,
            this, &Do3ThinkCameraControlPanel::updateDiagnostics);
    
    m_profilingTimer = new QTimer(this);
    m_profilingTimer->setInterval(100); // 100ms for smooth profiling
    connect(m_profilingTimer, &QTimer::timeout,
            this, &Do3ThinkCameraControlPanel::updateProfiling);
    
    // Apply additional Do3Think specific styling
    applyDo3ThinkTheme();
    
    // Add futuristic entrance animation with glow
    ComponentsForest::FuturisticTheme::ThemeManager::instance()->pulseWidget(this, QColor(0, 212, 255));
    ComponentsForest::FuturisticTheme::addPulseAnimation(this, 500);
}

Do3ThinkCameraControlPanel::~Do3ThinkCameraControlPanel()
{
    if (m_profilingTimer) {
        m_profilingTimer->stop();
    }
    if (m_diagnosticsTimer) {
        m_diagnosticsTimer->stop();
    }
}

// ============================================================================
// Override Virtual Methods from CameraControlPanel
// ============================================================================

void Do3ThinkCameraControlPanel::connectToComponent(QObject* cameraComponent)
{
    CameraControlPanel::connectToComponent(cameraComponent);
    
    if (cameraComponent) {
        connectComponentSignals(cameraComponent);
        
        // Request Do3Think specific capabilities
        QMetaObject::invokeMethod(cameraComponent, "getDo3ThinkCapabilities",
                                 Qt::QueuedConnection);
        
        // Start diagnostics if enabled
        if (m_do3thinkState.advancedDiagnosticsEnabled) {
            m_diagnosticsTimer->start();
        }
    }
}

void Do3ThinkCameraControlPanel::disconnectFromComponent()
{
    if (m_connectedComponent) {
        disconnectComponentSignals(m_connectedComponent);
    }
    
    m_diagnosticsTimer->stop();
    m_profilingTimer->stop();
    
    CameraControlPanel::disconnectFromComponent();
}

void Do3ThinkCameraControlPanel::setPanelLayout(PanelLayout layout)
{
    CameraControlPanel::setPanelLayout(layout);
    
    // Adjust Do3Think specific controls based on layout
    if (m_do3thinkControls.do3ThinkTabs) {
        switch (layout) {
        case PanelLayout::Compact:
            // Hide advanced tabs in compact mode
            for (int i = 3; i < m_do3thinkControls.do3ThinkTabs->count(); ++i) {
                m_do3thinkControls.do3ThinkTabs->setTabVisible(i, false);
            }
            break;
        case PanelLayout::Advanced:
            // Show all tabs in advanced mode
            for (int i = 0; i < m_do3thinkControls.do3ThinkTabs->count(); ++i) {
                m_do3thinkControls.do3ThinkTabs->setTabVisible(i, true);
            }
            break;
        default:
            break;
        }
    }
}

bool Do3ThinkCameraControlPanel::savePanelConfiguration(const QString& filePath) const
{
    QJsonObject config;
    
    // Save base configuration
    config["baseConfig"] = QJsonObject(); // Would call base implementation
    
    // Save Do3Think specific configuration
    QJsonObject do3thinkConfig;
    do3thinkConfig["showDo3ThinkFeatures"] = m_do3thinkState.showDo3ThinkFeatures;
    do3thinkConfig["advancedDiagnostics"] = m_do3thinkState.advancedDiagnosticsEnabled;
    do3thinkConfig["calibrationData"] = m_do3thinkState.calibrationData;
    
    // Save analysis settings
    QJsonObject analysisConfig;
    analysisConfig["histogram"] = m_analysisState.histogramEnabled;
    analysisConfig["waveform"] = m_analysisState.waveformEnabled;
    analysisConfig["vectorscope"] = m_analysisState.vectorscopeEnabled;
    analysisConfig["focusPeaking"] = m_analysisState.focusPeakingEnabled;
    analysisConfig["zebraPattern"] = m_analysisState.zebraPatternEnabled;
    analysisConfig["zebraThreshold"] = m_analysisState.zebraThreshold;
    
    config["do3think"] = do3thinkConfig;
    config["analysis"] = analysisConfig;
    
    // Save to file
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    
    QJsonDocument doc(config);
    file.write(doc.toJson());
    file.close();
    
    return true;
}

bool Do3ThinkCameraControlPanel::loadPanelConfiguration(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    
    if (!doc.isObject()) {
        return false;
    }
    
    QJsonObject config = doc.object();
    
    // Load Do3Think specific configuration
    if (config.contains("do3think")) {
        QJsonObject do3thinkConfig = config["do3think"].toObject();
        setShowDo3ThinkFeatures(do3thinkConfig["showDo3ThinkFeatures"].toBool());
        setAdvancedDiagnostics(do3thinkConfig["advancedDiagnostics"].toBool());
        m_do3thinkState.calibrationData = do3thinkConfig["calibrationData"].toObject();
    }
    
    // Load analysis settings
    if (config.contains("analysis")) {
        QJsonObject analysisConfig = config["analysis"].toObject();
        enableHistogram(analysisConfig["histogram"].toBool());
        enableWaveform(analysisConfig["waveform"].toBool());
        enableVectorscope(analysisConfig["vectorscope"].toBool());
        enableFocusPeaking(analysisConfig["focusPeaking"].toBool());
        enableZebraPattern(analysisConfig["zebraPattern"].toBool());
        m_analysisState.zebraThreshold = analysisConfig["zebraThreshold"].toInt(235);
    }
    
    return true;
}

void Do3ThinkCameraControlPanel::applyTheme(const QString& themeName)
{
    if (themeName == "dark" || themeName == "industrial") {
        setStyleSheet(Themes::DARK_INDUSTRIAL());
    } else if (themeName == "light" || themeName == "professional") {
        setStyleSheet(Themes::LIGHT_PROFESSIONAL());
    } else {
        CameraControlPanel::applyTheme(themeName);
    }
}

// ============================================================================
// Do3Think Specific Methods
// ============================================================================

void Do3ThinkCameraControlPanel::setShowDo3ThinkFeatures(bool show)
{
    if (m_do3thinkState.showDo3ThinkFeatures != show) {
        m_do3thinkState.showDo3ThinkFeatures = show;
        
        if (m_do3thinkControls.do3ThinkTabs) {
            m_do3thinkControls.do3ThinkTabs->setVisible(show);
        }
    }
}

bool Do3ThinkCameraControlPanel::showDo3ThinkFeatures() const
{
    return m_do3thinkState.showDo3ThinkFeatures;
}

void Do3ThinkCameraControlPanel::setAdvancedDiagnostics(bool enable)
{
    if (m_do3thinkState.advancedDiagnosticsEnabled != enable) {
        m_do3thinkState.advancedDiagnosticsEnabled = enable;
        
        if (enable && m_connectedComponent) {
            m_diagnosticsTimer->start();
        } else {
            m_diagnosticsTimer->stop();
        }
    }
}

bool Do3ThinkCameraControlPanel::isAdvancedDiagnosticsEnabled() const
{
    return m_do3thinkState.advancedDiagnosticsEnabled;
}

void Do3ThinkCameraControlPanel::showColorCorrectionDialog()
{
    QDialog dialog(this);
    dialog.setWindowTitle("Color Correction Matrix");
    dialog.setMinimumSize(400, 300);
    
    QVBoxLayout* layout = new QVBoxLayout(&dialog);
    
    // Create 3x3 matrix input
    QGridLayout* matrixLayout = new QGridLayout();
    QList<QDoubleSpinBox*> matrixSpinBoxes;
    
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col) {
            auto* spinBox = new QDoubleSpinBox();
            spinBox->setRange(-2.0, 2.0);
            spinBox->setSingleStep(0.01);
            spinBox->setDecimals(3);
            
            // Set identity matrix as default
            spinBox->setValue((row == col) ? 1.0 : 0.0);
            
            matrixLayout->addWidget(spinBox, row, col);
            matrixSpinBoxes.append(spinBox);
        }
    }
    
    layout->addLayout(matrixLayout);
    
    // Add preset buttons
    QHBoxLayout* presetLayout = new QHBoxLayout();
    
    QPushButton* identityBtn = new QPushButton("Identity");
    connect(identityBtn, &QPushButton::clicked, [&matrixSpinBoxes]() {
        for (int i = 0; i < 9; ++i) {
            matrixSpinBoxes[i]->setValue((i % 4 == 0) ? 1.0 : 0.0);
        }
    });
    presetLayout->addWidget(identityBtn);
    
    QPushButton* warmBtn = new QPushButton("Warm");
    connect(warmBtn, &QPushButton::clicked, [&matrixSpinBoxes]() {
        const double warm[9] = {1.2, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.8};
        for (int i = 0; i < 9; ++i) {
            matrixSpinBoxes[i]->setValue(warm[i]);
        }
    });
    presetLayout->addWidget(warmBtn);
    
    QPushButton* coolBtn = new QPushButton("Cool");
    connect(coolBtn, &QPushButton::clicked, [&matrixSpinBoxes]() {
        const double cool[9] = {0.8, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.2};
        for (int i = 0; i < 9; ++i) {
            matrixSpinBoxes[i]->setValue(cool[i]);
        }
    });
    presetLayout->addWidget(coolBtn);
    
    layout->addLayout(presetLayout);
    
    // Dialog buttons
    QDialogButtonBox* buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttonBox);
    
    if (dialog.exec() == QDialog::Accepted) {
        // Create matrix and emit signal
        QMatrix3x3 matrix;
        float* data = matrix.data();
        for (int i = 0; i < 9; ++i) {
            data[i] = static_cast<float>(matrixSpinBoxes[i]->value());
        }
        
        emit colorCorrectionRequested(matrix);
    }
}

void Do3ThinkCameraControlPanel::applyColorCorrectionPreset(const QString& presetName)
{
    QMatrix3x3 matrix;
    float* data = matrix.data();
    
    if (presetName == "Identity") {
        for (int i = 0; i < 9; ++i) {
            data[i] = (i % 4 == 0) ? 1.0f : 0.0f;
        }
    } else if (presetName == "Warm") {
        const float warm[9] = {1.2f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.8f};
        std::copy(warm, warm + 9, data);
    } else if (presetName == "Cool") {
        const float cool[9] = {0.8f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.2f};
        std::copy(cool, cool + 9, data);
    } else if (presetName == "Sepia") {
        const float sepia[9] = {0.393f, 0.349f, 0.272f, 0.769f, 0.686f, 0.534f, 0.189f, 0.168f, 0.131f};
        std::copy(sepia, sepia + 9, data);
    }
    
    emit colorCorrectionRequested(matrix);
}

QStringList Do3ThinkCameraControlPanel::colorCorrectionPresets() const
{
    return QStringList() << "Identity" << "Warm" << "Cool" << "Sepia" 
                        << "High Contrast" << "Low Contrast";
}

void Do3ThinkCameraControlPanel::showHDRSettingsDialog()
{
    QDialog dialog(this);
    dialog.setWindowTitle("HDR Settings");
    dialog.setMinimumSize(350, 250);
    
    QVBoxLayout* layout = new QVBoxLayout(&dialog);
    
    // HDR enable checkbox
    QCheckBox* enableCheck = new QCheckBox("Enable HDR");
    enableCheck->setChecked(m_do3thinkControls.hdrEnableCheck ? 
                           m_do3thinkControls.hdrEnableCheck->isChecked() : false);
    layout->addWidget(enableCheck);
    
    // HDR levels
    QHBoxLayout* levelLayout = new QHBoxLayout();
    levelLayout->addWidget(new QLabel("HDR Levels:"));
    QSpinBox* levelSpinBox = new QSpinBox();
    levelSpinBox->setRange(2, 8);
    levelSpinBox->setValue(3);
    levelLayout->addWidget(levelSpinBox);
    layout->addLayout(levelLayout);
    
    // Exposure bracket settings
    QGroupBox* bracketGroup = new QGroupBox("Exposure Bracketing");
    QGridLayout* bracketLayout = new QGridLayout(bracketGroup);
    
    bracketLayout->addWidget(new QLabel("EV Step:"), 0, 0);
    QDoubleSpinBox* evStepSpinBox = new QDoubleSpinBox();
    evStepSpinBox->setRange(0.5, 3.0);
    evStepSpinBox->setSingleStep(0.5);
    evStepSpinBox->setValue(1.0);
    evStepSpinBox->setSuffix(" EV");
    bracketLayout->addWidget(evStepSpinBox, 0, 1);
    
    bracketLayout->addWidget(new QLabel("Base Exposure:"), 1, 0);
    QDoubleSpinBox* baseExpSpinBox = new QDoubleSpinBox();
    baseExpSpinBox->setRange(10, 100000);
    baseExpSpinBox->setValue(1000);
    baseExpSpinBox->setSuffix(" µs");
    bracketLayout->addWidget(baseExpSpinBox, 1, 1);
    
    layout->addWidget(bracketGroup);
    
    // Tone mapping options
    QGroupBox* toneMapGroup = new QGroupBox("Tone Mapping");
    QVBoxLayout* toneMapLayout = new QVBoxLayout(toneMapGroup);
    
    QComboBox* algorithmCombo = new QComboBox();
    algorithmCombo->addItems(QStringList() << "Reinhard" << "Drago" << "Mantiuk" << "Fattal");
    toneMapLayout->addWidget(algorithmCombo);
    
    layout->addWidget(toneMapGroup);
    
    // Dialog buttons
    QDialogButtonBox* buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttonBox);
    
    if (dialog.exec() == QDialog::Accepted) {
        emit hdrSettingsChanged(enableCheck->isChecked(), levelSpinBox->value());
    }
}

void Do3ThinkCameraControlPanel::setHDRVisualization(bool enable)
{
    // Would update HDR visualization in image display
    if (m_do3thinkControls.hdrEnableCheck) {
        m_do3thinkControls.hdrEnableCheck->setChecked(enable);
    }
}

void Do3ThinkCameraControlPanel::showGPIOControlPanel()
{
    QDialog dialog(this);
    dialog.setWindowTitle("GPIO Control");
    dialog.setMinimumSize(400, 300);
    
    QVBoxLayout* layout = new QVBoxLayout(&dialog);
    
    // Create GPIO pin controls
    QGridLayout* pinLayout = new QGridLayout();
    
    for (int i = 0; i < 8; ++i) {
        QLabel* label = new QLabel(QString("GPIO %1:").arg(i));
        pinLayout->addWidget(label, i, 0);
        
        QCheckBox* outputCheck = new QCheckBox("Output");
        pinLayout->addWidget(outputCheck, i, 1);
        
        QPushButton* toggleBtn = new QPushButton("Low");
        toggleBtn->setCheckable(true);
        toggleBtn->setMaximumWidth(60);
        connect(toggleBtn, &QPushButton::toggled, [toggleBtn](bool checked) {
            toggleBtn->setText(checked ? "High" : "Low");
            toggleBtn->setStyleSheet(checked ? 
                "background-color: #4CAF50;" : "background-color: #f44336;");
        });
        pinLayout->addWidget(toggleBtn, i, 2);
        
        QLabel* statusLabel = new QLabel("Input: Low");
        pinLayout->addWidget(statusLabel, i, 3);
    }
    
    layout->addLayout(pinLayout);
    
    // Add pulse generator section
    QGroupBox* pulseGroup = new QGroupBox("Pulse Generator");
    QGridLayout* pulseLayout = new QGridLayout(pulseGroup);
    
    pulseLayout->addWidget(new QLabel("Pin:"), 0, 0);
    QSpinBox* pinSpinBox = new QSpinBox();
    pinSpinBox->setRange(0, 7);
    pulseLayout->addWidget(pinSpinBox, 0, 1);
    
    pulseLayout->addWidget(new QLabel("Frequency:"), 1, 0);
    QDoubleSpinBox* freqSpinBox = new QDoubleSpinBox();
    freqSpinBox->setRange(0.1, 1000.0);
    freqSpinBox->setSuffix(" Hz");
    pulseLayout->addWidget(freqSpinBox, 1, 1);
    
    pulseLayout->addWidget(new QLabel("Duty Cycle:"), 2, 0);
    QSpinBox* dutySpinBox = new QSpinBox();
    dutySpinBox->setRange(1, 99);
    dutySpinBox->setSuffix(" %");
    dutySpinBox->setValue(50);
    pulseLayout->addWidget(dutySpinBox, 2, 1);
    
    QPushButton* startPulseBtn = new QPushButton("Start Pulse");
    pulseLayout->addWidget(startPulseBtn, 3, 0, 1, 2);
    
    layout->addWidget(pulseGroup);
    
    // Dialog buttons
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttonBox);
    
    dialog.exec();
}

void Do3ThinkCameraControlPanel::setGPIOMonitoring(bool enable)
{
    // Would enable/disable GPIO state monitoring
}

void Do3ThinkCameraControlPanel::startPerformanceProfiling()
{
    if (!m_do3thinkState.profilingActive) {
        m_do3thinkState.profilingActive = true;
        m_profilingData.profilingStartTime = QDateTime::currentMSecsSinceEpoch();
        
        // Clear previous data
        m_profilingData.fpsHistory.clear();
        m_profilingData.cpuHistory.clear();
        m_profilingData.memoryHistory.clear();
        m_profilingData.timestamps.clear();
        
        // Update UI
        if (m_do3thinkControls.profilingStartButton) {
            m_do3thinkControls.profilingStartButton->setEnabled(false);
        }
        if (m_do3thinkControls.profilingStopButton) {
            m_do3thinkControls.profilingStopButton->setEnabled(true);
        }
        
        // Start profiling timer
        m_profilingTimer->start();
    }
}

void Do3ThinkCameraControlPanel::stopPerformanceProfiling()
{
    if (m_do3thinkState.profilingActive) {
        m_do3thinkState.profilingActive = false;
        m_profilingTimer->stop();
        
        // Update UI
        if (m_do3thinkControls.profilingStartButton) {
            m_do3thinkControls.profilingStartButton->setEnabled(true);
        }
        if (m_do3thinkControls.profilingStopButton) {
            m_do3thinkControls.profilingStopButton->setEnabled(false);
        }
        
        // Prepare profiling data
        QJsonObject data;
        data["duration"] = QDateTime::currentMSecsSinceEpoch() - m_profilingData.profilingStartTime;
        data["samples"] = static_cast<int>(m_profilingData.timestamps.size());
        
        if (!m_profilingData.fpsHistory.isEmpty()) {
            data["avgFps"] = std::accumulate(m_profilingData.fpsHistory.begin(), 
                                            m_profilingData.fpsHistory.end(), 0.0) / 
                             m_profilingData.fpsHistory.size();
            data["maxFps"] = *std::max_element(m_profilingData.fpsHistory.begin(), 
                                               m_profilingData.fpsHistory.end());
            data["minFps"] = *std::min_element(m_profilingData.fpsHistory.begin(), 
                                               m_profilingData.fpsHistory.end());
        }
        
        emit profilingDataReady(data);
    }
}

void Do3ThinkCameraControlPanel::exportProfilingResults(const QString& filePath)
{
    QJsonObject results;
    results["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    results["duration"] = QDateTime::currentMSecsSinceEpoch() - m_profilingData.profilingStartTime;
    
    // Add FPS data
    QJsonArray fpsArray;
    for (double fps : m_profilingData.fpsHistory) {
        fpsArray.append(fps);
    }
    results["fps"] = fpsArray;
    
    // Add CPU data
    QJsonArray cpuArray;
    for (double cpu : m_profilingData.cpuHistory) {
        cpuArray.append(cpu);
    }
    results["cpu"] = cpuArray;
    
    // Add memory data
    QJsonArray memArray;
    for (double mem : m_profilingData.memoryHistory) {
        memArray.append(mem);
    }
    results["memory"] = memArray;
    
    // Save to file
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        QJsonDocument doc(results);
        file.write(doc.toJson());
        file.close();
    }
}

void Do3ThinkCameraControlPanel::startCalibrationWizard()
{
    QWizard wizard(this);
    wizard.setWindowTitle("Camera Calibration Wizard");
    wizard.setMinimumSize(600, 400);
    
    // Page 1: Introduction
    QWizardPage* introPage = new QWizardPage();
    introPage->setTitle("Camera Calibration");
    QVBoxLayout* introLayout = new QVBoxLayout(introPage);
    QLabel* introLabel = new QLabel(
        "This wizard will guide you through the camera calibration process.\n\n"
        "You will need:\n"
        "• A calibration target (checkerboard or circle grid)\n"
        "• Good lighting conditions\n"
        "• Stable camera mount");
    introLayout->addWidget(introLabel);
    wizard.addPage(introPage);
    
    // Page 2: Dark Frame Calibration
    QWizardPage* darkPage = new QWizardPage();
    darkPage->setTitle("Dark Frame Calibration");
    QVBoxLayout* darkLayout = new QVBoxLayout(darkPage);
    darkLayout->addWidget(new QLabel("Cover the lens and click 'Capture' to acquire dark frames."));
    QPushButton* captureDarkBtn = new QPushButton("Capture Dark Frame");
    darkLayout->addWidget(captureDarkBtn);
    QProgressBar* darkProgress = new QProgressBar();
    darkLayout->addWidget(darkProgress);
    wizard.addPage(darkPage);
    
    // Page 3: Flat Field Calibration
    QWizardPage* flatPage = new QWizardPage();
    flatPage->setTitle("Flat Field Calibration");
    QVBoxLayout* flatLayout = new QVBoxLayout(flatPage);
    flatLayout->addWidget(new QLabel("Place a uniform white target and click 'Capture'."));
    QPushButton* captureFlatBtn = new QPushButton("Capture Flat Field");
    flatLayout->addWidget(captureFlatBtn);
    QProgressBar* flatProgress = new QProgressBar();
    flatLayout->addWidget(flatProgress);
    wizard.addPage(flatPage);
    
    // Page 4: Geometric Calibration
    QWizardPage* geomPage = new QWizardPage();
    geomPage->setTitle("Geometric Calibration");
    QVBoxLayout* geomLayout = new QVBoxLayout(geomPage);
    QComboBox* patternCombo = new QComboBox();
    patternCombo->addItems(QStringList() << "Checkerboard" << "Circle Grid" << "ChArUco");
    geomLayout->addWidget(new QLabel("Select calibration pattern:"));
    geomLayout->addWidget(patternCombo);
    
    QSpinBox* rowsSpinBox = new QSpinBox();
    rowsSpinBox->setRange(3, 20);
    rowsSpinBox->setValue(9);
    QSpinBox* colsSpinBox = new QSpinBox();
    colsSpinBox->setRange(3, 20);
    colsSpinBox->setValue(6);
    
    QHBoxLayout* sizeLayout = new QHBoxLayout();
    sizeLayout->addWidget(new QLabel("Pattern size:"));
    sizeLayout->addWidget(rowsSpinBox);
    sizeLayout->addWidget(new QLabel("x"));
    sizeLayout->addWidget(colsSpinBox);
    geomLayout->addLayout(sizeLayout);
    
    QPushButton* captureGeomBtn = new QPushButton("Capture Pattern");
    geomLayout->addWidget(captureGeomBtn);
    wizard.addPage(geomPage);
    
    // Page 5: Results
    QWizardPage* resultsPage = new QWizardPage();
    resultsPage->setTitle("Calibration Results");
    QVBoxLayout* resultsLayout = new QVBoxLayout(resultsPage);
    QTextEdit* resultsText = new QTextEdit();
    resultsText->setReadOnly(true);
    resultsLayout->addWidget(resultsText);
    wizard.addPage(resultsPage);
    
    if (wizard.exec() == QDialog::Accepted) {
        // Save calibration data
        m_do3thinkState.calibrationData["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
        m_do3thinkState.calibrationData["completed"] = true;
        
        emit calibrationCompleted(m_do3thinkState.calibrationData);
        
        if (m_do3thinkControls.calibrationStatusLabel) {
            m_do3thinkControls.calibrationStatusLabel->setText("Calibration: Complete");
            m_do3thinkControls.calibrationStatusLabel->setStyleSheet("color: green;");
        }
    }
}

void Do3ThinkCameraControlPanel::loadCalibrationFile(const QString& filePath)
{
    QFile file(filePath);
    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        file.close();
        
        if (doc.isObject()) {
            m_do3thinkState.calibrationData = doc.object();
            
            if (m_do3thinkControls.calibrationStatusLabel) {
                m_do3thinkControls.calibrationStatusLabel->setText("Calibration: Loaded");
                m_do3thinkControls.calibrationStatusLabel->setStyleSheet("color: green;");
            }
        }
    }
}

void Do3ThinkCameraControlPanel::saveCalibrationFile(const QString& filePath)
{
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        QJsonDocument doc(m_do3thinkState.calibrationData);
        file.write(doc.toJson());
        file.close();
    }
}

void Do3ThinkCameraControlPanel::enableHistogram(bool enable)
{
    m_analysisState.histogramEnabled = enable;
    if (m_do3thinkControls.histogramWidget) {
        m_do3thinkControls.histogramWidget->setVisible(enable);
    }
}

void Do3ThinkCameraControlPanel::enableWaveform(bool enable)
{
    m_analysisState.waveformEnabled = enable;
    if (m_do3thinkControls.waveformWidget) {
        m_do3thinkControls.waveformWidget->setVisible(enable);
    }
}

void Do3ThinkCameraControlPanel::enableVectorscope(bool enable)
{
    m_analysisState.vectorscopeEnabled = enable;
    if (m_do3thinkControls.vectorscopeWidget) {
        m_do3thinkControls.vectorscopeWidget->setVisible(enable);
    }
}

void Do3ThinkCameraControlPanel::enableFocusPeaking(bool enable)
{
    m_analysisState.focusPeakingEnabled = enable;
    if (m_do3thinkControls.focusPeakingCheck) {
        m_do3thinkControls.focusPeakingCheck->setChecked(enable);
    }
}

void Do3ThinkCameraControlPanel::enableZebraPattern(bool enable)
{
    m_analysisState.zebraPatternEnabled = enable;
    if (m_do3thinkControls.zebraPatternCheck) {
        m_do3thinkControls.zebraPatternCheck->setChecked(enable);
    }
}

// ============================================================================
// Public Slots
// ============================================================================

void Do3ThinkCameraControlPanel::onImageReceived(const QImage& image, qint64 timestamp)
{
    // Call base implementation
    CameraControlPanel::onImageReceived(image, timestamp);
    
    // Process image analysis if needed
    if (m_analysisState.histogramEnabled || 
        m_analysisState.waveformEnabled || 
        m_analysisState.vectorscopeEnabled ||
        m_analysisState.focusPeakingEnabled ||
        m_analysisState.zebraPatternEnabled) {
        
        processImageAnalysis(image);
    }
}

void Do3ThinkCameraControlPanel::onStatisticsUpdated(const QJsonObject& stats)
{
    // Call base implementation
    CameraControlPanel::onStatisticsUpdated(stats);
    
    // Update Do3Think specific statistics
    if (stats.contains("temperature")) {
        onTemperatureUpdated(stats["temperature"].toDouble());
    }
    
    // Update profiling data if active
    if (m_do3thinkState.profilingActive && stats.contains("fps")) {
        double fps = stats["fps"].toDouble();
        m_profilingData.fpsHistory.append(fps);
        
        // Limit history size
        if (m_profilingData.fpsHistory.size() > m_profilingData.maxHistorySize) {
            m_profilingData.fpsHistory.removeFirst();
        }
    }
}

void Do3ThinkCameraControlPanel::onErrorOccurred(const QString& error)
{
    // Call base implementation
    CameraControlPanel::onErrorOccurred(error);
    
    // Log to diagnostics if enabled
    if (m_do3thinkState.advancedDiagnosticsEnabled && m_do3thinkControls.diagnosticsLog) {
        QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
        m_do3thinkControls.diagnosticsLog->append(
            QString("[%1] ERROR: %2").arg(timestamp, error));
    }
}

void Do3ThinkCameraControlPanel::onTemperatureUpdated(double temperature)
{
    m_do3thinkState.lastTemperature = temperature;
    updateTemperatureDisplay(temperature);
}

void Do3ThinkCameraControlPanel::onHDRModeChanged(bool enabled)
{
    if (m_do3thinkControls.hdrEnableCheck) {
        m_do3thinkControls.hdrEnableCheck->setChecked(enabled);
    }
}

void Do3ThinkCameraControlPanel::onColorCorrectionChanged(bool enabled)
{
    // Update UI to reflect color correction state
}

void Do3ThinkCameraControlPanel::onGPIOStateChanged(int pin, bool value)
{
    if (pin >= 0 && pin < m_do3thinkControls.gpioPins.size()) {
        if (m_do3thinkControls.gpioPins[pin]) {
            m_do3thinkControls.gpioPins[pin]->setChecked(value);
        }
    }
}

void Do3ThinkCameraControlPanel::onStreamModeChanged(int mode)
{
    if (m_do3thinkControls.streamModeCombo) {
        m_do3thinkControls.streamModeCombo->setCurrentIndex(mode);
    }
}

void Do3ThinkCameraControlPanel::onDo3ThinkCapabilitiesUpdated(const QJsonObject& capabilities)
{
    m_do3thinkState.lastCapabilities = capabilities;
    
    // Update UI based on capabilities
    if (capabilities.contains("hasHDR")) {
        bool hasHDR = capabilities["hasHDR"].toBool();
        if (m_do3thinkControls.hdrEnableCheck) {
            m_do3thinkControls.hdrEnableCheck->setEnabled(hasHDR);
        }
    }
    
    if (capabilities.contains("hasColorCorrection")) {
        bool hasCC = capabilities["hasColorCorrection"].toBool();
        if (m_do3thinkControls.colorCorrectionButton) {
            m_do3thinkControls.colorCorrectionButton->setEnabled(hasCC);
        }
    }
    
    if (capabilities.contains("gpioCount")) {
        int gpioCount = capabilities["gpioCount"].toInt();
        // Update GPIO panel based on available pins
    }
}

// ============================================================================
// Protected Methods - UI Creation
// ============================================================================

void Do3ThinkCameraControlPanel::createUI()
{
    // Create base UI first
    CameraControlPanel::createUI();
    
    // Add Do3Think specific controls
    if (m_do3thinkState.showDo3ThinkFeatures) {
        setupDo3ThinkUI();
    }
}

QWidget* Do3ThinkCameraControlPanel::createConnectionControls()
{
    // Create base connection controls
    QWidget* baseControls = CameraControlPanel::createConnectionControls();
    
    // Add Do3Think specific connection info
    QGroupBox* do3thinkInfo = new QGroupBox("Do3Think Status");
    QGridLayout* infoLayout = new QGridLayout(do3thinkInfo);
    
    m_do3thinkControls.temperatureLabel = new QLabel("Temperature: --");
    infoLayout->addWidget(m_do3thinkControls.temperatureLabel, 0, 0);
    
    m_do3thinkControls.temperatureBar = new QProgressBar();
    m_do3thinkControls.temperatureBar->setRange(0, 80);
    m_do3thinkControls.temperatureBar->setFormat("%v°C");
    infoLayout->addWidget(m_do3thinkControls.temperatureBar, 0, 1);
    
    // Combine widgets
    QWidget* combined = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(combined);
    layout->addWidget(baseControls);
    layout->addWidget(do3thinkInfo);
    
    return combined;
}

QWidget* Do3ThinkCameraControlPanel::createBasicParameterControls()
{
    // Create base parameter controls
    QWidget* baseControls = CameraControlPanel::createBasicParameterControls();
    
    // Add Do3Think specific basic controls
    QGroupBox* do3thinkBasic = new QGroupBox("Do3Think Features");
    QGridLayout* layout = new QGridLayout(do3thinkBasic);
    
    // HDR quick controls
    m_do3thinkControls.hdrEnableCheck = new QCheckBox("HDR Mode");
    connect(m_do3thinkControls.hdrEnableCheck, &QCheckBox::toggled,
            [this](bool checked) { emit hdrSettingsChanged(checked, 3); });
    layout->addWidget(m_do3thinkControls.hdrEnableCheck, 0, 0);
    
    m_do3thinkControls.hdrSettingsButton = new QPushButton("HDR Settings...");
    connect(m_do3thinkControls.hdrSettingsButton, &QPushButton::clicked,
            this, &Do3ThinkCameraControlPanel::showHDRSettingsDialog);
    layout->addWidget(m_do3thinkControls.hdrSettingsButton, 0, 1);
    
    // Color correction quick access
    m_do3thinkControls.colorCorrectionButton = new QPushButton("Color Correction...");
    connect(m_do3thinkControls.colorCorrectionButton, &QPushButton::clicked,
            this, &Do3ThinkCameraControlPanel::showColorCorrectionDialog);
    layout->addWidget(m_do3thinkControls.colorCorrectionButton, 1, 0, 1, 2);
    
    // Combine widgets
    QWidget* combined = new QWidget();
    QVBoxLayout* mainLayout = new QVBoxLayout(combined);
    mainLayout->addWidget(baseControls);
    mainLayout->addWidget(do3thinkBasic);
    
    return combined;
}

QWidget* Do3ThinkCameraControlPanel::createAdvancedParameterControls()
{
    // Create base advanced controls
    QWidget* baseControls = CameraControlPanel::createAdvancedParameterControls();
    
    // Add Do3Think specific advanced controls
    QTabWidget* advancedTabs = new QTabWidget();
    
    // Trigger advanced tab
    QWidget* triggerTab = new QWidget();
    QGridLayout* triggerLayout = new QGridLayout(triggerTab);
    
    triggerLayout->addWidget(new QLabel("Trigger Delay:"), 0, 0);
    m_do3thinkControls.triggerDelaySpinBox = new QDoubleSpinBox();
    m_do3thinkControls.triggerDelaySpinBox->setRange(0, 1000);
    m_do3thinkControls.triggerDelaySpinBox->setSuffix(" ms");
    triggerLayout->addWidget(m_do3thinkControls.triggerDelaySpinBox, 0, 1);
    
    triggerLayout->addWidget(new QLabel("Trigger Divider:"), 1, 0);
    m_do3thinkControls.triggerDividerSpinBox = new QSpinBox();
    m_do3thinkControls.triggerDividerSpinBox->setRange(1, 256);
    triggerLayout->addWidget(m_do3thinkControls.triggerDividerSpinBox, 1, 1);
    
    advancedTabs->addTab(triggerTab, "Trigger");
    
    // Stream control tab
    QWidget* streamTab = new QWidget();
    QGridLayout* streamLayout = new QGridLayout(streamTab);
    
    streamLayout->addWidget(new QLabel("Stream Mode:"), 0, 0);
    m_do3thinkControls.streamModeCombo = new QComboBox();
    m_do3thinkControls.streamModeCombo->addItems(QStringList() 
        << "Continuous" << "MultiFrame" << "SingleFrame");
    streamLayout->addWidget(m_do3thinkControls.streamModeCombo, 0, 1);
    
    streamLayout->addWidget(new QLabel("Packet Size:"), 1, 0);
    m_do3thinkControls.packetSizeSpinBox = new QSpinBox();
    m_do3thinkControls.packetSizeSpinBox->setRange(1400, 9000);
    m_do3thinkControls.packetSizeSpinBox->setSuffix(" bytes");
    streamLayout->addWidget(m_do3thinkControls.packetSizeSpinBox, 1, 1);
    
    advancedTabs->addTab(streamTab, "Stream");
    
    // LUT tab
    QWidget* lutTab = new QWidget();
    QVBoxLayout* lutLayout = new QVBoxLayout(lutTab);
    
    m_do3thinkControls.lutEnableCheck = new QCheckBox("Enable LUT");
    lutLayout->addWidget(m_do3thinkControls.lutEnableCheck);
    
    m_do3thinkControls.lutLoadButton = new QPushButton("Load LUT File...");
    lutLayout->addWidget(m_do3thinkControls.lutLoadButton);
    
    advancedTabs->addTab(lutTab, "LUT");
    
    // Combine widgets
    QWidget* combined = new QWidget();
    QVBoxLayout* mainLayout = new QVBoxLayout(combined);
    mainLayout->addWidget(baseControls);
    mainLayout->addWidget(advancedTabs);
    
    return combined;
}

QWidget* Do3ThinkCameraControlPanel::createImageDisplay()
{
    // Create base image display
    QWidget* baseDisplay = CameraControlPanel::createImageDisplay();
    
    // Add analysis overlay options
    QGroupBox* overlayGroup = new QGroupBox("Image Analysis");
    QHBoxLayout* overlayLayout = new QHBoxLayout(overlayGroup);
    
    m_do3thinkControls.focusPeakingCheck = new QCheckBox("Focus Peaking");
    connect(m_do3thinkControls.focusPeakingCheck, &QCheckBox::toggled,
            this, &Do3ThinkCameraControlPanel::onFocusPeakingToggled);
    overlayLayout->addWidget(m_do3thinkControls.focusPeakingCheck);
    
    m_do3thinkControls.zebraPatternCheck = new QCheckBox("Zebra Pattern");
    connect(m_do3thinkControls.zebraPatternCheck, &QCheckBox::toggled,
            this, &Do3ThinkCameraControlPanel::onZebraPatternToggled);
    overlayLayout->addWidget(m_do3thinkControls.zebraPatternCheck);
    
    // Combine widgets
    QWidget* combined = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(combined);
    layout->addWidget(baseDisplay);
    layout->addWidget(overlayGroup);
    
    return combined;
}

QWidget* Do3ThinkCameraControlPanel::createStatisticsDisplay()
{
    // Create base statistics display
    QWidget* baseStats = CameraControlPanel::createStatisticsDisplay();
    
    // Add Do3Think specific statistics
    QGroupBox* do3thinkStats = new QGroupBox("Do3Think Statistics");
    QGridLayout* layout = new QGridLayout(do3thinkStats);
    
    layout->addWidget(new QLabel("Sensor Temp:"), 0, 0);
    QLabel* tempValue = new QLabel("--°C");
    layout->addWidget(tempValue, 0, 1);
    
    layout->addWidget(new QLabel("Data Rate:"), 1, 0);
    QLabel* dataRateValue = new QLabel("-- MB/s");
    layout->addWidget(dataRateValue, 1, 1);
    
    layout->addWidget(new QLabel("Buffer Fill:"), 2, 0);
    QProgressBar* bufferBar = new QProgressBar();
    bufferBar->setMaximum(100);
    layout->addWidget(bufferBar, 2, 1);
    
    // Combine widgets
    QWidget* combined = new QWidget();
    QVBoxLayout* mainLayout = new QVBoxLayout(combined);
    mainLayout->addWidget(baseStats);
    mainLayout->addWidget(do3thinkStats);
    
    return combined;
}

QWidget* Do3ThinkCameraControlPanel::createDo3ThinkControls()
{
    m_do3thinkControls.do3ThinkTabs = new QTabWidget();
    
    m_do3thinkControls.do3ThinkTabs->addTab(createColorCorrectionPanel(), "Color");
    m_do3thinkControls.do3ThinkTabs->addTab(createHDRPanel(), "HDR");
    m_do3thinkControls.do3ThinkTabs->addTab(createImageProcessingPanel(), "Processing");
    m_do3thinkControls.do3ThinkTabs->addTab(createGPIOPanel(), "GPIO");
    m_do3thinkControls.do3ThinkTabs->addTab(createDiagnosticsPanel(), "Diagnostics");
    m_do3thinkControls.do3ThinkTabs->addTab(createProfilingPanel(), "Profiling");
    m_do3thinkControls.do3ThinkTabs->addTab(createCalibrationPanel(), "Calibration");
    m_do3thinkControls.do3ThinkTabs->addTab(createImageAnalysisPanel(), "Analysis");
    
    return m_do3thinkControls.do3ThinkTabs;
}

QWidget* Do3ThinkCameraControlPanel::createColorCorrectionPanel()
{
    QWidget* panel = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(panel);
    
    // Preset selection
    QHBoxLayout* presetLayout = new QHBoxLayout();
    presetLayout->addWidget(new QLabel("Preset:"));
    m_do3thinkControls.colorPresetCombo = new QComboBox();
    m_do3thinkControls.colorPresetCombo->addItems(colorCorrectionPresets());
    connect(m_do3thinkControls.colorPresetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            [this](int index) {
                if (index >= 0) {
                    applyColorCorrectionPreset(m_do3thinkControls.colorPresetCombo->currentText());
                }
            });
    presetLayout->addWidget(m_do3thinkControls.colorPresetCombo);
    presetLayout->addStretch();
    layout->addLayout(presetLayout);
    
    // Color matrix display
    QGroupBox* matrixGroup = new QGroupBox("Color Correction Matrix");
    QGridLayout* matrixLayout = new QGridLayout(matrixGroup);
    
    m_do3thinkControls.colorMatrixWidget = new QWidget();
    QGridLayout* matrixGrid = new QGridLayout(m_do3thinkControls.colorMatrixWidget);
    
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col) {
            QDoubleSpinBox* spinBox = new QDoubleSpinBox();
            spinBox->setRange(-2.0, 2.0);
            spinBox->setSingleStep(0.01);
            spinBox->setDecimals(3);
            spinBox->setValue((row == col) ? 1.0 : 0.0);
            connect(spinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                    this, &Do3ThinkCameraControlPanel::onColorMatrixChanged);
            matrixGrid->addWidget(spinBox, row, col);
        }
    }
    
    matrixLayout->addWidget(m_do3thinkControls.colorMatrixWidget);
    layout->addWidget(matrixGroup);
    
    // Control buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    QPushButton* resetBtn = new QPushButton("Reset to Identity");
    connect(resetBtn, &QPushButton::clicked, [this]() {
        applyColorCorrectionPreset("Identity");
    });
    buttonLayout->addWidget(resetBtn);
    
    QPushButton* applyBtn = new QPushButton("Apply Matrix");
    connect(applyBtn, &QPushButton::clicked,
            this, &Do3ThinkCameraControlPanel::onColorMatrixChanged);
    buttonLayout->addWidget(applyBtn);
    
    buttonLayout->addStretch();
    layout->addLayout(buttonLayout);
    
    layout->addStretch();
    
    return panel;
}

QWidget* Do3ThinkCameraControlPanel::createHDRPanel()
{
    QWidget* panel = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(panel);
    
    // HDR enable
    m_do3thinkControls.hdrEnableCheck = new QCheckBox("Enable HDR Mode");
    connect(m_do3thinkControls.hdrEnableCheck, &QCheckBox::toggled,
            this, &Do3ThinkCameraControlPanel::onHDRModeChanged);
    layout->addWidget(m_do3thinkControls.hdrEnableCheck);
    
    // HDR settings group
    QGroupBox* settingsGroup = new QGroupBox("HDR Settings");
    QGridLayout* settingsLayout = new QGridLayout(settingsGroup);
    
    settingsLayout->addWidget(new QLabel("HDR Levels:"), 0, 0);
    m_do3thinkControls.hdrLevelSpinBox = new QSpinBox();
    m_do3thinkControls.hdrLevelSpinBox->setRange(2, 8);
    m_do3thinkControls.hdrLevelSpinBox->setValue(3);
    connect(m_do3thinkControls.hdrLevelSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &Do3ThinkCameraControlPanel::onHDRLevelChanged);
    settingsLayout->addWidget(m_do3thinkControls.hdrLevelSpinBox, 0, 1);
    
    settingsLayout->addWidget(new QLabel("EV Step:"), 1, 0);
    QDoubleSpinBox* evStepSpinBox = new QDoubleSpinBox();
    evStepSpinBox->setRange(0.5, 3.0);
    evStepSpinBox->setSingleStep(0.5);
    evStepSpinBox->setValue(1.0);
    evStepSpinBox->setSuffix(" EV");
    settingsLayout->addWidget(evStepSpinBox, 1, 1);
    
    settingsLayout->addWidget(new QLabel("Tone Mapping:"), 2, 0);
    QComboBox* toneMappingCombo = new QComboBox();
    toneMappingCombo->addItems(QStringList() << "Reinhard" << "Drago" << "Mantiuk" << "Fattal");
    settingsLayout->addWidget(toneMappingCombo, 2, 1);
    
    layout->addWidget(settingsGroup);
    
    // HDR preview
    QGroupBox* previewGroup = new QGroupBox("HDR Preview");
    QVBoxLayout* previewLayout = new QVBoxLayout(previewGroup);
    
    QLabel* previewLabel = new QLabel();
    previewLabel->setMinimumSize(320, 240);
    previewLabel->setStyleSheet("border: 1px solid #ccc;");
    previewLabel->setAlignment(Qt::AlignCenter);
    previewLabel->setText("HDR Preview");
    previewLayout->addWidget(previewLabel);
    
    layout->addWidget(previewGroup);
    
    layout->addStretch();
    
    return panel;
}

QWidget* Do3ThinkCameraControlPanel::createGPIOPanel()
{
    m_do3thinkControls.gpioPanel = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(m_do3thinkControls.gpioPanel);
    
    // GPIO pins control
    QGroupBox* pinsGroup = new QGroupBox("GPIO Pins");
    QGridLayout* pinsLayout = new QGridLayout(pinsGroup);
    
    for (int i = 0; i < 8; ++i) {
        QLabel* label = new QLabel(QString("GPIO %1:").arg(i));
        pinsLayout->addWidget(label, i, 0);
        
        QComboBox* dirCombo = new QComboBox();
        dirCombo->addItems(QStringList() << "Input" << "Output");
        pinsLayout->addWidget(dirCombo, i, 1);
        
        QCheckBox* valueCheck = new QCheckBox("High");
        connect(valueCheck, &QCheckBox::toggled,
                [this, i](bool checked) { onGPIOPinToggled(i); });
        pinsLayout->addWidget(valueCheck, i, 2);
        m_do3thinkControls.gpioPins.append(valueCheck);
        
        QLabel* statusLabel = new QLabel("Low");
        statusLabel->setStyleSheet("QLabel { background-color: #f44336; padding: 2px; }");
        pinsLayout->addWidget(statusLabel, i, 3);
    }
    
    layout->addWidget(pinsGroup);
    
    // Pulse generator
    QGroupBox* pulseGroup = new QGroupBox("Pulse Generator");
    QGridLayout* pulseLayout = new QGridLayout(pulseGroup);
    
    pulseLayout->addWidget(new QLabel("Pin:"), 0, 0);
    QSpinBox* pinSpinBox = new QSpinBox();
    pinSpinBox->setRange(0, 7);
    pulseLayout->addWidget(pinSpinBox, 0, 1);
    
    pulseLayout->addWidget(new QLabel("Frequency:"), 1, 0);
    QDoubleSpinBox* freqSpinBox = new QDoubleSpinBox();
    freqSpinBox->setRange(0.1, 1000.0);
    freqSpinBox->setSuffix(" Hz");
    pulseLayout->addWidget(freqSpinBox, 1, 1);
    
    pulseLayout->addWidget(new QLabel("Duty:"), 2, 0);
    QSpinBox* dutySpinBox = new QSpinBox();
    dutySpinBox->setRange(1, 99);
    dutySpinBox->setSuffix(" %");
    dutySpinBox->setValue(50);
    pulseLayout->addWidget(dutySpinBox, 2, 1);
    
    QPushButton* startPulseBtn = new QPushButton("Start");
    QPushButton* stopPulseBtn = new QPushButton("Stop");
    QHBoxLayout* pulseButtonLayout = new QHBoxLayout();
    pulseButtonLayout->addWidget(startPulseBtn);
    pulseButtonLayout->addWidget(stopPulseBtn);
    pulseLayout->addLayout(pulseButtonLayout, 3, 0, 1, 2);
    
    layout->addWidget(pulseGroup);
    
    layout->addStretch();
    
    return m_do3thinkControls.gpioPanel;
}

QWidget* Do3ThinkCameraControlPanel::createDiagnosticsPanel()
{
    QWidget* panel = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(panel);
    
    // Diagnostics log
    QGroupBox* logGroup = new QGroupBox("Diagnostics Log");
    QVBoxLayout* logLayout = new QVBoxLayout(logGroup);
    
    m_do3thinkControls.diagnosticsLog = new QTextEdit();
    m_do3thinkControls.diagnosticsLog->setReadOnly(true);
    m_do3thinkControls.diagnosticsLog->setMaximumHeight(200);
    logLayout->addWidget(m_do3thinkControls.diagnosticsLog);
    
    QHBoxLayout* logButtonLayout = new QHBoxLayout();
    QPushButton* clearLogBtn = new QPushButton("Clear Log");
    connect(clearLogBtn, &QPushButton::clicked, [this]() {
        if (m_do3thinkControls.diagnosticsLog) {
            m_do3thinkControls.diagnosticsLog->clear();
        }
    });
    logButtonLayout->addWidget(clearLogBtn);
    
    QPushButton* saveLogBtn = new QPushButton("Save Log...");
    connect(saveLogBtn, &QPushButton::clicked, [this]() {
        QString fileName = QFileDialog::getSaveFileName(this, "Save Diagnostics Log", 
                                                        "", "Text Files (*.txt)");
        if (!fileName.isEmpty() && m_do3thinkControls.diagnosticsLog) {
            QFile file(fileName);
            if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream stream(&file);
                stream << m_do3thinkControls.diagnosticsLog->toPlainText();
                file.close();
            }
        }
    });
    logButtonLayout->addWidget(saveLogBtn);
    logButtonLayout->addStretch();
    logLayout->addLayout(logButtonLayout);
    
    layout->addWidget(logGroup);
    
    // Diagnostics table
    QGroupBox* tableGroup = new QGroupBox("System Diagnostics");
    QVBoxLayout* tableLayout = new QVBoxLayout(tableGroup);
    
    m_do3thinkControls.diagnosticsTable = new QTableWidget(8, 2);
    m_do3thinkControls.diagnosticsTable->setHorizontalHeaderLabels(QStringList() << "Parameter" << "Value");
    m_do3thinkControls.diagnosticsTable->verticalHeader()->setVisible(false);
    
    // Initialize table rows
    QStringList parameters = QStringList() << "Camera State" << "Temperature" << "FPS" 
                                          << "Data Rate" << "Buffer Usage" << "Dropped Frames"
                                          << "Error Count" << "Uptime";
    for (int i = 0; i < parameters.size(); ++i) {
        m_do3thinkControls.diagnosticsTable->setItem(i, 0, new QTableWidgetItem(parameters[i]));
        m_do3thinkControls.diagnosticsTable->setItem(i, 1, new QTableWidgetItem("--"));
    }
    
    m_do3thinkControls.diagnosticsTable->resizeColumnsToContents();
    tableLayout->addWidget(m_do3thinkControls.diagnosticsTable);
    
    layout->addWidget(tableGroup);
    
    return panel;
}

QWidget* Do3ThinkCameraControlPanel::createProfilingPanel()
{
    QWidget* panel = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(panel);
    
    // Profiling controls
    QGroupBox* controlGroup = new QGroupBox("Profiling Control");
    QHBoxLayout* controlLayout = new QHBoxLayout(controlGroup);
    
    m_do3thinkControls.profilingStartButton = new QPushButton("Start Profiling");
    connect(m_do3thinkControls.profilingStartButton, &QPushButton::clicked,
            this, &Do3ThinkCameraControlPanel::startPerformanceProfiling);
    controlLayout->addWidget(m_do3thinkControls.profilingStartButton);
    
    m_do3thinkControls.profilingStopButton = new QPushButton("Stop Profiling");
    m_do3thinkControls.profilingStopButton->setEnabled(false);
    connect(m_do3thinkControls.profilingStopButton, &QPushButton::clicked,
            this, &Do3ThinkCameraControlPanel::stopPerformanceProfiling);
    controlLayout->addWidget(m_do3thinkControls.profilingStopButton);
    
    QPushButton* exportBtn = new QPushButton("Export Results...");
    connect(exportBtn, &QPushButton::clicked, [this]() {
        QString fileName = QFileDialog::getSaveFileName(this, "Export Profiling Results",
                                                        "", "JSON Files (*.json)");
        if (!fileName.isEmpty()) {
            exportProfilingResults(fileName);
        }
    });
    controlLayout->addWidget(exportBtn);
    
    controlLayout->addStretch();
    layout->addWidget(controlGroup);
    
    // Performance chart
    QGroupBox* chartGroup = new QGroupBox("Performance Metrics");
    QVBoxLayout* chartLayout = new QVBoxLayout(chartGroup);
    
    QChart* chart = new QChart();
    chart->setTitle("Real-time Performance");
    chart->setAnimationOptions(QChart::NoAnimation);
    
    m_do3thinkControls.fpsSeries = new QLineSeries();
    m_do3thinkControls.fpsSeries->setName("FPS");
    chart->addSeries(m_do3thinkControls.fpsSeries);
    
    m_do3thinkControls.cpuSeries = new QLineSeries();
    m_do3thinkControls.cpuSeries->setName("CPU %");
    chart->addSeries(m_do3thinkControls.cpuSeries);
    
    m_do3thinkControls.memorySeries = new QLineSeries();
    m_do3thinkControls.memorySeries->setName("Memory MB");
    chart->addSeries(m_do3thinkControls.memorySeries);
    
    chart->createDefaultAxes();
    chart->legend()->setAlignment(Qt::AlignBottom);
    
    m_do3thinkControls.performanceChart = new QChartView(chart);
    m_do3thinkControls.performanceChart->setRenderHint(QPainter::Antialiasing);
    m_do3thinkControls.performanceChart->setMinimumHeight(300);
    
    chartLayout->addWidget(m_do3thinkControls.performanceChart);
    layout->addWidget(chartGroup);
    
    return panel;
}

QWidget* Do3ThinkCameraControlPanel::createCalibrationPanel()
{
    QWidget* panel = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(panel);
    
    // Calibration status
    QGroupBox* statusGroup = new QGroupBox("Calibration Status");
    QGridLayout* statusLayout = new QGridLayout(statusGroup);
    
    m_do3thinkControls.calibrationStatusLabel = new QLabel("Calibration: Not Performed");
    m_do3thinkControls.calibrationStatusLabel->setStyleSheet("color: orange;");
    statusLayout->addWidget(m_do3thinkControls.calibrationStatusLabel, 0, 0, 1, 2);
    
    statusLayout->addWidget(new QLabel("Last Calibration:"), 1, 0);
    QLabel* lastCalibLabel = new QLabel("--");
    statusLayout->addWidget(lastCalibLabel, 1, 1);
    
    layout->addWidget(statusGroup);
    
    // Calibration actions
    QGroupBox* actionsGroup = new QGroupBox("Calibration Actions");
    QVBoxLayout* actionsLayout = new QVBoxLayout(actionsGroup);
    
    m_do3thinkControls.calibrationWizardButton = new QPushButton("Start Calibration Wizard");
    connect(m_do3thinkControls.calibrationWizardButton, &QPushButton::clicked,
            this, &Do3ThinkCameraControlPanel::startCalibrationWizard);
    actionsLayout->addWidget(m_do3thinkControls.calibrationWizardButton);
    
    QPushButton* loadCalibBtn = new QPushButton("Load Calibration File...");
    connect(loadCalibBtn, &QPushButton::clicked, [this]() {
        QString fileName = QFileDialog::getOpenFileName(this, "Load Calibration File",
                                                        "", "JSON Files (*.json)");
        if (!fileName.isEmpty()) {
            loadCalibrationFile(fileName);
        }
    });
    actionsLayout->addWidget(loadCalibBtn);
    
    QPushButton* saveCalibBtn = new QPushButton("Save Calibration File...");
    connect(saveCalibBtn, &QPushButton::clicked, [this]() {
        QString fileName = QFileDialog::getSaveFileName(this, "Save Calibration File",
                                                        "", "JSON Files (*.json)");
        if (!fileName.isEmpty()) {
            saveCalibrationFile(fileName);
        }
    });
    actionsLayout->addWidget(saveCalibBtn);
    
    layout->addWidget(actionsGroup);
    
    // Calibration types
    QGroupBox* typesGroup = new QGroupBox("Calibration Types");
    QVBoxLayout* typesLayout = new QVBoxLayout(typesGroup);
    
    QCheckBox* darkFrameCheck = new QCheckBox("Dark Frame Calibration");
    typesLayout->addWidget(darkFrameCheck);
    
    QCheckBox* flatFieldCheck = new QCheckBox("Flat Field Calibration");
    typesLayout->addWidget(flatFieldCheck);
    
    QCheckBox* geometricCheck = new QCheckBox("Geometric Calibration");
    typesLayout->addWidget(geometricCheck);
    
    QCheckBox* colorCalibCheck = new QCheckBox("Color Calibration");
    typesLayout->addWidget(colorCalibCheck);
    
    layout->addWidget(typesGroup);
    
    layout->addStretch();
    
    return panel;
}

QWidget* Do3ThinkCameraControlPanel::createImageAnalysisPanel()
{
    QWidget* panel = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(panel);
    
    // Analysis tools selection
    QGroupBox* toolsGroup = new QGroupBox("Analysis Tools");
    QGridLayout* toolsLayout = new QGridLayout(toolsGroup);
    
    QCheckBox* histogramCheck = new QCheckBox("Histogram");
    connect(histogramCheck, &QCheckBox::toggled,
            this, &Do3ThinkCameraControlPanel::onHistogramToggled);
    toolsLayout->addWidget(histogramCheck, 0, 0);
    
    QCheckBox* waveformCheck = new QCheckBox("Waveform");
    connect(waveformCheck, &QCheckBox::toggled,
            this, &Do3ThinkCameraControlPanel::onWaveformToggled);
    toolsLayout->addWidget(waveformCheck, 0, 1);
    
    QCheckBox* vectorscopeCheck = new QCheckBox("Vectorscope");
    connect(vectorscopeCheck, &QCheckBox::toggled,
            this, &Do3ThinkCameraControlPanel::onVectorscopeToggled);
    toolsLayout->addWidget(vectorscopeCheck, 1, 0);
    
    QCheckBox* rgbParadeCheck = new QCheckBox("RGB Parade");
    toolsLayout->addWidget(rgbParadeCheck, 1, 1);
    
    layout->addWidget(toolsGroup);
    
    // Analysis displays
    QTabWidget* analysisTabs = new QTabWidget();
    
    // Histogram tab
    m_do3thinkControls.histogramWidget = new QWidget();
    QVBoxLayout* histLayout = new QVBoxLayout(m_do3thinkControls.histogramWidget);
    QLabel* histLabel = new QLabel();
    histLabel->setMinimumSize(256, 200);
    histLabel->setStyleSheet("border: 1px solid #ccc;");
    histLabel->setAlignment(Qt::AlignCenter);
    histLayout->addWidget(histLabel);
    analysisTabs->addTab(m_do3thinkControls.histogramWidget, "Histogram");
    
    // Waveform tab
    m_do3thinkControls.waveformWidget = new QWidget();
    QVBoxLayout* waveLayout = new QVBoxLayout(m_do3thinkControls.waveformWidget);
    QLabel* waveLabel = new QLabel();
    waveLabel->setMinimumSize(256, 200);
    waveLabel->setStyleSheet("border: 1px solid #ccc;");
    waveLabel->setAlignment(Qt::AlignCenter);
    waveLayout->addWidget(waveLabel);
    analysisTabs->addTab(m_do3thinkControls.waveformWidget, "Waveform");
    
    // Vectorscope tab
    m_do3thinkControls.vectorscopeWidget = new QWidget();
    QVBoxLayout* vecLayout = new QVBoxLayout(m_do3thinkControls.vectorscopeWidget);
    QLabel* vecLabel = new QLabel();
    vecLabel->setMinimumSize(256, 256);
    vecLabel->setStyleSheet("border: 1px solid #ccc;");
    vecLabel->setAlignment(Qt::AlignCenter);
    vecLayout->addWidget(vecLabel);
    analysisTabs->addTab(m_do3thinkControls.vectorscopeWidget, "Vectorscope");
    
    layout->addWidget(analysisTabs);
    
    return panel;
}

QWidget* Do3ThinkCameraControlPanel::createImageProcessingPanel()
{
    m_do3thinkControls.imageProcessingPanel = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(m_do3thinkControls.imageProcessingPanel);
    
    // Main enable/disable processing
    m_do3thinkControls.processingEnableCheck = new QCheckBox("Enable Image Processing");
    m_do3thinkControls.processingEnableCheck->setStyleSheet("QCheckBox { font-weight: bold; color: #2E86AB; }");
    connect(m_do3thinkControls.processingEnableCheck, &QCheckBox::toggled,
            this, &Do3ThinkCameraControlPanel::onProcessingEnableToggled);
    layout->addWidget(m_do3thinkControls.processingEnableCheck);
    
    // Preprocessor selection group
    m_do3thinkControls.preprocessorGroup = new QGroupBox("Preprocessor Selection");
    m_do3thinkControls.preprocessorGroup->setEnabled(false); // Initially disabled
    QGridLayout* preprocessorLayout = new QGridLayout(m_do3thinkControls.preprocessorGroup);
    
    // Blur preprocessor
    m_do3thinkControls.blurEnableCheck = new QCheckBox("Blur Filter");
    connect(m_do3thinkControls.blurEnableCheck, &QCheckBox::toggled,
            this, &Do3ThinkCameraControlPanel::onBlurEnableToggled);
    preprocessorLayout->addWidget(m_do3thinkControls.blurEnableCheck, 0, 0);
    
    QLabel* blurLabel = new QLabel("Kernel Size:");
    preprocessorLayout->addWidget(blurLabel, 0, 1);
    m_do3thinkControls.blurKernelSpinBox = new QSpinBox();
    m_do3thinkControls.blurKernelSpinBox->setRange(3, 31);
    m_do3thinkControls.blurKernelSpinBox->setSingleStep(2);
    m_do3thinkControls.blurKernelSpinBox->setValue(5);
    m_do3thinkControls.blurKernelSpinBox->setEnabled(false);
    connect(m_do3thinkControls.blurKernelSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &Do3ThinkCameraControlPanel::onBlurKernelChanged);
    preprocessorLayout->addWidget(m_do3thinkControls.blurKernelSpinBox, 0, 2);
    
    // Edge detection preprocessor
    m_do3thinkControls.edgeEnableCheck = new QCheckBox("Edge Detection");
    connect(m_do3thinkControls.edgeEnableCheck, &QCheckBox::toggled,
            this, &Do3ThinkCameraControlPanel::onEdgeEnableToggled);
    preprocessorLayout->addWidget(m_do3thinkControls.edgeEnableCheck, 1, 0);
    
    QLabel* edgeLabel = new QLabel("Threshold:");
    preprocessorLayout->addWidget(edgeLabel, 1, 1);
    m_do3thinkControls.edgeThresholdSpinBox = new QDoubleSpinBox();
    m_do3thinkControls.edgeThresholdSpinBox->setRange(0.0, 255.0);
    m_do3thinkControls.edgeThresholdSpinBox->setSingleStep(5.0);
    m_do3thinkControls.edgeThresholdSpinBox->setValue(100.0);
    m_do3thinkControls.edgeThresholdSpinBox->setEnabled(false);
    connect(m_do3thinkControls.edgeThresholdSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &Do3ThinkCameraControlPanel::onEdgeThresholdChanged);
    preprocessorLayout->addWidget(m_do3thinkControls.edgeThresholdSpinBox, 1, 2);
    
    // Denoise preprocessor
    m_do3thinkControls.denoiseEnableCheck = new QCheckBox("Noise Reduction");
    connect(m_do3thinkControls.denoiseEnableCheck, &QCheckBox::toggled,
            this, &Do3ThinkCameraControlPanel::onDenoiseEnableToggled);
    preprocessorLayout->addWidget(m_do3thinkControls.denoiseEnableCheck, 2, 0);
    
    QLabel* denoiseLabel = new QLabel("Strength:");
    preprocessorLayout->addWidget(denoiseLabel, 2, 1);
    m_do3thinkControls.denoiseStrengthSpinBox = new QDoubleSpinBox();
    m_do3thinkControls.denoiseStrengthSpinBox->setRange(0.1, 10.0);
    m_do3thinkControls.denoiseStrengthSpinBox->setSingleStep(0.1);
    m_do3thinkControls.denoiseStrengthSpinBox->setValue(1.0);
    m_do3thinkControls.denoiseStrengthSpinBox->setEnabled(false);
    connect(m_do3thinkControls.denoiseStrengthSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &Do3ThinkCameraControlPanel::onDenoiseStrengthChanged);
    preprocessorLayout->addWidget(m_do3thinkControls.denoiseStrengthSpinBox, 2, 2);
    
    layout->addWidget(m_do3thinkControls.preprocessorGroup);
    
    // Processing order configuration
    QGroupBox* orderGroup = new QGroupBox("Processing Order");
    QHBoxLayout* orderLayout = new QHBoxLayout(orderGroup);
    
    // Order list
    m_do3thinkControls.processingOrderList = new QListWidget();
    m_do3thinkControls.processingOrderList->setMaximumHeight(120);
    m_do3thinkControls.processingOrderList->setDragDropMode(QAbstractItemView::InternalMove);
    connect(m_do3thinkControls.processingOrderList, SIGNAL(itemChanged(QListWidgetItem*)),
            this, SLOT(onProcessingOrderChanged()));
    orderLayout->addWidget(m_do3thinkControls.processingOrderList, 2);
    
    // Order control buttons
    QVBoxLayout* orderButtonsLayout = new QVBoxLayout();
    m_do3thinkControls.moveUpButton = new QPushButton("Move Up");
    m_do3thinkControls.moveUpButton->setMaximumWidth(80);
    connect(m_do3thinkControls.moveUpButton, &QPushButton::clicked, [this]() {
        int currentRow = m_do3thinkControls.processingOrderList->currentRow();
        if (currentRow > 0) {
            QListWidgetItem* item = m_do3thinkControls.processingOrderList->takeItem(currentRow);
            m_do3thinkControls.processingOrderList->insertItem(currentRow - 1, item);
            m_do3thinkControls.processingOrderList->setCurrentRow(currentRow - 1);
            onProcessingOrderChanged();
        }
    });
    orderButtonsLayout->addWidget(m_do3thinkControls.moveUpButton);
    
    m_do3thinkControls.moveDownButton = new QPushButton("Move Down");
    m_do3thinkControls.moveDownButton->setMaximumWidth(80);
    connect(m_do3thinkControls.moveDownButton, &QPushButton::clicked, [this]() {
        int currentRow = m_do3thinkControls.processingOrderList->currentRow();
        if (currentRow >= 0 && currentRow < m_do3thinkControls.processingOrderList->count() - 1) {
            QListWidgetItem* item = m_do3thinkControls.processingOrderList->takeItem(currentRow);
            m_do3thinkControls.processingOrderList->insertItem(currentRow + 1, item);
            m_do3thinkControls.processingOrderList->setCurrentRow(currentRow + 1);
            onProcessingOrderChanged();
        }
    });
    orderButtonsLayout->addWidget(m_do3thinkControls.moveDownButton);
    
    m_do3thinkControls.configureButton = new QPushButton("Configure");
    m_do3thinkControls.configureButton->setMaximumWidth(80);
    connect(m_do3thinkControls.configureButton, &QPushButton::clicked,
            this, &Do3ThinkCameraControlPanel::onProcessingConfigurationClicked);
    orderButtonsLayout->addWidget(m_do3thinkControls.configureButton);
    
    orderButtonsLayout->addStretch();
    orderLayout->addLayout(orderButtonsLayout);
    
    layout->addWidget(orderGroup);
    
    // Processing status
    QGroupBox* statusGroup = new QGroupBox("Processing Status");
    QHBoxLayout* statusLayout = new QHBoxLayout(statusGroup);
    
    m_do3thinkControls.processingStatusLabel = new QLabel("Processing: Disabled");
    statusLayout->addWidget(m_do3thinkControls.processingStatusLabel);
    
    m_do3thinkControls.processingLoadBar = new QProgressBar();
    m_do3thinkControls.processingLoadBar->setRange(0, 100);
    m_do3thinkControls.processingLoadBar->setValue(0);
    m_do3thinkControls.processingLoadBar->setTextVisible(true);
    m_do3thinkControls.processingLoadBar->setFormat("Load: %p%");
    statusLayout->addWidget(m_do3thinkControls.processingLoadBar);
    
    layout->addWidget(statusGroup);
    
    layout->addStretch();
    
    return m_do3thinkControls.imageProcessingPanel;
}

// ============================================================================
// Protected Methods - Signal/Slot Connections
// ============================================================================

void Do3ThinkCameraControlPanel::connectInternalSignals()
{
    // Connect base signals
    CameraControlPanel::connectInternalSignals();
    
    // Connect Do3Think specific internal signals
    if (m_do3thinkControls.colorCorrectionButton) {
        connect(m_do3thinkControls.colorCorrectionButton, &QPushButton::clicked,
                this, &Do3ThinkCameraControlPanel::onColorCorrectionButtonClicked);
    }
    
    if (m_do3thinkControls.hdrSettingsButton) {
        connect(m_do3thinkControls.hdrSettingsButton, &QPushButton::clicked,
                this, &Do3ThinkCameraControlPanel::onHDRButtonClicked);
    }
    
    if (m_do3thinkControls.calibrationWizardButton) {
        connect(m_do3thinkControls.calibrationWizardButton, &QPushButton::clicked,
                this, &Do3ThinkCameraControlPanel::onCalibrationButtonClicked);
    }
}

void Do3ThinkCameraControlPanel::connectComponentSignals(QObject* component)
{
    if (!component) return;
    
    // Connect base signals
    CameraControlPanel::connectComponentSignals(component);
    
    // Connect Do3Think specific signals from component to panel
    connect(component, SIGNAL(temperatureChanged(double)),
            this, SLOT(onTemperatureUpdated(double)));
    
    connect(component, SIGNAL(hdrModeChanged(bool)),
            this, SLOT(onHDRModeChanged(bool)));
    
    connect(component, SIGNAL(colorCorrectionChanged(bool)),
            this, SLOT(onColorCorrectionChanged(bool)));
    
    connect(component, SIGNAL(gpioStateChanged(int, bool)),
            this, SLOT(onGPIOStateChanged(int, bool)));
    
    connect(component, SIGNAL(streamModeChanged(int)),
            this, SLOT(onStreamModeChanged(int)));
    
    connect(component, SIGNAL(do3ThinkCapabilitiesChanged(QJsonObject)),
            this, SLOT(onDo3ThinkCapabilitiesUpdated(QJsonObject)));
    
    // Connect image processing signals from component to panel
    connect(component, SIGNAL(processingEnabledChanged(bool)),
            this, SLOT(onProcessingEnabledChanged(bool)));
            
    connect(component, SIGNAL(preprocessorEnabledChanged(QString, bool)),
            this, SLOT(onPreprocessorEnabledChanged(QString, bool)));
            
    connect(component, SIGNAL(processingOrderChanged(QStringList)),
            this, SLOT(onProcessingOrderChanged(QStringList)));
            
    // TODO: Re-enable when processing pipeline is fixed
    /*
    connect(component, SIGNAL(processingPerformanceUpdate(double, double)),
            this, [this](double fps, double latency) {
                if (m_do3thinkControls.processingLoadBar) {
                    // Update load bar based on latency (simple approximation)
                    int load = qMin(100, static_cast<int>(latency * 2)); // 50ms = 100% load
                    m_do3thinkControls.processingLoadBar->setValue(load);
                }
            });
    */
    
    // Connect panel signals to component
    connect(this, SIGNAL(imageProcessingEnabled(bool)),
            component, SLOT(onImageProcessingEnabled(bool)));
            
    connect(this, SIGNAL(preprocessorEnabled(QString, bool)),
            component, SLOT(onPreprocessorEnabled(QString, bool)));
            
    connect(this, SIGNAL(preprocessorParameterChanged(QString, QString, QVariant)),
            component, SLOT(onPreprocessorParameterChanged(QString, QString, QVariant)));
            
    connect(this, SIGNAL(processingOrderChanged(QStringList)),
            component, SLOT(onProcessingOrderChanged(QStringList)));
            
    connect(this, SIGNAL(processingConfigurationRequested()),
            component, SLOT(onProcessingConfigurationRequested()));
    
    connect(this, SIGNAL(colorCorrectionRequested(QMatrix3x3)),
            component, SLOT(setColorCorrectionMatrix(QMatrix3x3)));
    
    connect(this, SIGNAL(hdrSettingsChanged(bool, int)),
            component, SLOT(setHDRMode(bool, int)));
    
    connect(this, SIGNAL(gpioControlRequested(int, bool)),
            component, SLOT(setGPIOPin(int, bool)));
}

void Do3ThinkCameraControlPanel::disconnectComponentSignals(QObject* component)
{
    if (!component) return;
    
    // Disconnect all Do3Think specific signals
    disconnect(component, nullptr, this, nullptr);
    disconnect(this, nullptr, component, nullptr);
    
    // Call base implementation
    CameraControlPanel::disconnectComponentSignals(component);
}

// ============================================================================
// Protected Methods - UI Updates
// ============================================================================

void Do3ThinkCameraControlPanel::updateUIState(bool connected, bool acquiring)
{
    // Update base UI state
    CameraControlPanel::updateUIState(connected, acquiring);
    
    // Update Do3Think specific controls
    if (m_do3thinkControls.colorCorrectionButton) {
        m_do3thinkControls.colorCorrectionButton->setEnabled(connected);
    }
    
    if (m_do3thinkControls.hdrEnableCheck) {
        m_do3thinkControls.hdrEnableCheck->setEnabled(connected);
    }
    
    if (m_do3thinkControls.calibrationWizardButton) {
        m_do3thinkControls.calibrationWizardButton->setEnabled(connected && !acquiring);
    }
    
    // Update GPIO controls
    for (QCheckBox* gpioPin : m_do3thinkControls.gpioPins) {
        if (gpioPin) {
            gpioPin->setEnabled(connected);
        }
    }
}

void Do3ThinkCameraControlPanel::updateParameterDisplays()
{
    // Update base parameter displays
    CameraControlPanel::updateParameterDisplays();
    
    // Update Do3Think specific parameters
    updateDo3ThinkControls();
}

void Do3ThinkCameraControlPanel::updateDo3ThinkControls()
{
    // Update temperature display
    updateTemperatureDisplay(m_do3thinkState.lastTemperature);
    
    // Update HDR display
    if (m_do3thinkControls.hdrEnableCheck && m_do3thinkControls.hdrLevelSpinBox) {
        updateHDRDisplay(m_do3thinkControls.hdrEnableCheck->isChecked(),
                        m_do3thinkControls.hdrLevelSpinBox->value());
    }
    
    // Update GPIO display
    updateGPIODisplay();
    
    // Update profiling display if active
    if (m_do3thinkState.profilingActive) {
        updateProfilingDisplay();
    }
}

void Do3ThinkCameraControlPanel::updateTemperatureDisplay(double temperature)
{
    if (m_do3thinkControls.temperatureLabel) {
        m_do3thinkControls.temperatureLabel->setText(formatTemperature(temperature));
    }
    
    if (m_do3thinkControls.temperatureBar) {
        m_do3thinkControls.temperatureBar->setValue(static_cast<int>(temperature));
        
        // Color code based on temperature
        QString style;
        if (temperature < 40) {
            style = "QProgressBar::chunk { background-color: #4CAF50; }"; // Green
        } else if (temperature < 60) {
            style = "QProgressBar::chunk { background-color: #FFC107; }"; // Yellow
        } else {
            style = "QProgressBar::chunk { background-color: #f44336; }"; // Red
        }
        m_do3thinkControls.temperatureBar->setStyleSheet(style);
    }
}

void Do3ThinkCameraControlPanel::updateHDRDisplay(bool enabled, int levels)
{
    // Update HDR status display
    QString status = enabled ? 
        QString("HDR: ON (%1 levels)").arg(levels) : 
        "HDR: OFF";
    
    // Would update HDR status label if it existed
}

void Do3ThinkCameraControlPanel::updateGPIODisplay()
{
    // Update GPIO pin states
    for (int i = 0; i < m_do3thinkControls.gpioPins.size(); ++i) {
        if (m_do3thinkControls.gpioPins[i]) {
            // Would update based on actual GPIO state
        }
    }
}

void Do3ThinkCameraControlPanel::updateProfilingDisplay()
{
    if (!m_do3thinkControls.performanceChart) return;
    
    // Update chart with latest profiling data
    updateCharts();
}

void Do3ThinkCameraControlPanel::updateProcessingDisplay()
{
    if (!m_do3thinkControls.processingStatusLabel) return;
    
    QString status = m_do3thinkState.processingEnabled ? "Processing: Enabled" : "Processing: Disabled";
    
    // Add active preprocessors to status
    QStringList activeProcessors;
    if (m_do3thinkState.processingEnabled) {
        if (m_do3thinkState.blurEnabled) activeProcessors << "Blur";
        if (m_do3thinkState.edgeEnabled) activeProcessors << "Edge";
        if (m_do3thinkState.denoiseEnabled) activeProcessors << "Denoise";
        
        if (!activeProcessors.isEmpty()) {
            status += QString(" (%1)").arg(activeProcessors.join(", "));
        }
    }
    
    m_do3thinkControls.processingStatusLabel->setText(status);
    
    // Update load bar based on number of active processors
    if (m_do3thinkControls.processingLoadBar) {
        int load = 0;
        if (m_do3thinkState.processingEnabled) {
            load = activeProcessors.count() * 25; // Rough estimate
        }
        m_do3thinkControls.processingLoadBar->setValue(qMin(load, 100));
    }
}

void Do3ThinkCameraControlPanel::updateProcessingOrderList()
{
    if (!m_do3thinkControls.processingOrderList) return;
    
    m_do3thinkControls.processingOrderList->clear();
    
    // Add enabled preprocessors to the list
    if (m_do3thinkState.blurEnabled) {
        QListWidgetItem* item = new QListWidgetItem("Blur Filter");
        item->setFlags(item->flags() | Qt::ItemIsEditable);
        m_do3thinkControls.processingOrderList->addItem(item);
    }
    
    if (m_do3thinkState.edgeEnabled) {
        QListWidgetItem* item = new QListWidgetItem("Edge Detection");
        item->setFlags(item->flags() | Qt::ItemIsEditable);
        m_do3thinkControls.processingOrderList->addItem(item);
    }
    
    if (m_do3thinkState.denoiseEnabled) {
        QListWidgetItem* item = new QListWidgetItem("Noise Reduction");
        item->setFlags(item->flags() | Qt::ItemIsEditable);
        m_do3thinkControls.processingOrderList->addItem(item);
    }
    
    // Update the processing order
    onProcessingOrderChanged();
}

// ============================================================================
// Protected Slots - Internal UI Handlers
// ============================================================================

void Do3ThinkCameraControlPanel::onColorCorrectionButtonClicked()
{
    showColorCorrectionDialog();
}

void Do3ThinkCameraControlPanel::onHDRButtonClicked()
{
    showHDRSettingsDialog();
}

void Do3ThinkCameraControlPanel::onGPIOButtonClicked()
{
    showGPIOControlPanel();
}

void Do3ThinkCameraControlPanel::onCalibrationButtonClicked()
{
    startCalibrationWizard();
}

void Do3ThinkCameraControlPanel::onProfilingButtonClicked()
{
    if (m_do3thinkState.profilingActive) {
        stopPerformanceProfiling();
    } else {
        startPerformanceProfiling();
    }
}

void Do3ThinkCameraControlPanel::onColorMatrixChanged()
{
    if (!m_do3thinkControls.colorMatrixWidget) return;
    
    // Extract matrix values from spin boxes
    QMatrix3x3 matrix;
    float* data = matrix.data();
    
    QList<QDoubleSpinBox*> spinBoxes = m_do3thinkControls.colorMatrixWidget->findChildren<QDoubleSpinBox*>();
    for (int i = 0; i < qMin(9, spinBoxes.size()); ++i) {
        data[i] = static_cast<float>(spinBoxes[i]->value());
    }
    
    emit colorCorrectionRequested(matrix);
}

void Do3ThinkCameraControlPanel::onHDRLevelChanged(int level)
{
    bool enabled = m_do3thinkControls.hdrEnableCheck ? 
                  m_do3thinkControls.hdrEnableCheck->isChecked() : false;
    emit hdrSettingsChanged(enabled, level);
}

void Do3ThinkCameraControlPanel::onGPIOPinToggled(int pin)
{
    if (pin >= 0 && pin < m_do3thinkControls.gpioPins.size()) {
        bool value = m_do3thinkControls.gpioPins[pin] ? 
                    m_do3thinkControls.gpioPins[pin]->isChecked() : false;
        emit gpioControlRequested(pin, value);
    }
}

void Do3ThinkCameraControlPanel::onTriggerDelayChanged(double delay)
{
    // Would emit signal to set trigger delay
}

void Do3ThinkCameraControlPanel::onPacketSizeChanged(int size)
{
    // Would emit signal to set packet size
}

void Do3ThinkCameraControlPanel::onHistogramToggled(bool checked)
{
    enableHistogram(checked);
}

void Do3ThinkCameraControlPanel::onWaveformToggled(bool checked)
{
    enableWaveform(checked);
}

void Do3ThinkCameraControlPanel::onVectorscopeToggled(bool checked)
{
    enableVectorscope(checked);
}

void Do3ThinkCameraControlPanel::onFocusPeakingToggled(bool checked)
{
    enableFocusPeaking(checked);
}

void Do3ThinkCameraControlPanel::onZebraPatternToggled(bool checked)
{
    enableZebraPattern(checked);
}

// ============================================================================
// Image Processing Slot Implementations
// ============================================================================

void Do3ThinkCameraControlPanel::onProcessingEnableToggled(bool enabled)
{
    m_do3thinkState.processingEnabled = enabled;
    
    // Enable/disable the preprocessor group
    if (m_do3thinkControls.preprocessorGroup) {
        m_do3thinkControls.preprocessorGroup->setEnabled(enabled);
    }
    
    // Update status display
    updateProcessingDisplay();
    
    // Emit signal for component to handle
    emit imageProcessingEnabled(enabled);
    
    qDebug() << "Image processing" << (enabled ? "enabled" : "disabled");
}

void Do3ThinkCameraControlPanel::onBlurEnableToggled(bool enabled)
{
    m_do3thinkState.blurEnabled = enabled;
    
    // Enable/disable blur parameter controls
    if (m_do3thinkControls.blurKernelSpinBox) {
        m_do3thinkControls.blurKernelSpinBox->setEnabled(enabled);
    }
    
    // Update processing order list
    updateProcessingOrderList();
    
    // Emit signal for component
    emit preprocessorEnabled("blur", enabled);
    
    qDebug() << "Blur preprocessor" << (enabled ? "enabled" : "disabled");
}

void Do3ThinkCameraControlPanel::onEdgeEnableToggled(bool enabled)
{
    m_do3thinkState.edgeEnabled = enabled;
    
    // Enable/disable edge parameter controls
    if (m_do3thinkControls.edgeThresholdSpinBox) {
        m_do3thinkControls.edgeThresholdSpinBox->setEnabled(enabled);
    }
    
    // Update processing order list
    updateProcessingOrderList();
    
    // Emit signal for component
    emit preprocessorEnabled("edge", enabled);
    
    qDebug() << "Edge preprocessor" << (enabled ? "enabled" : "disabled");
}

void Do3ThinkCameraControlPanel::onDenoiseEnableToggled(bool enabled)
{
    m_do3thinkState.denoiseEnabled = enabled;
    
    // Enable/disable denoise parameter controls
    if (m_do3thinkControls.denoiseStrengthSpinBox) {
        m_do3thinkControls.denoiseStrengthSpinBox->setEnabled(enabled);
    }
    
    // Update processing order list
    updateProcessingOrderList();
    
    // Emit signal for component
    emit preprocessorEnabled("denoise", enabled);
    
    qDebug() << "Denoise preprocessor" << (enabled ? "enabled" : "disabled");
}

void Do3ThinkCameraControlPanel::onBlurKernelChanged(int value)
{
    // Ensure odd kernel size
    if (value % 2 == 0) {
        value = value + 1;
        m_do3thinkControls.blurKernelSpinBox->setValue(value);
    }
    
    emit preprocessorParameterChanged("blur", "kernelSize", value);
    qDebug() << "Blur kernel size changed to" << value;
}

void Do3ThinkCameraControlPanel::onEdgeThresholdChanged(double value)
{
    emit preprocessorParameterChanged("edge", "threshold", value);
    qDebug() << "Edge threshold changed to" << value;
}

void Do3ThinkCameraControlPanel::onDenoiseStrengthChanged(double value)
{
    emit preprocessorParameterChanged("denoise", "strength", value);
    qDebug() << "Denoise strength changed to" << value;
}

void Do3ThinkCameraControlPanel::onProcessingOrderChanged()
{
    if (!m_do3thinkControls.processingOrderList) return;
    
    QStringList newOrder;
    for (int i = 0; i < m_do3thinkControls.processingOrderList->count(); ++i) {
        QListWidgetItem* item = m_do3thinkControls.processingOrderList->item(i);
        if (item) {
            newOrder << item->text();
        }
    }
    
    m_do3thinkState.processingOrder = newOrder;
    emit processingOrderChanged(newOrder);
    
    qDebug() << "Processing order changed to:" << newOrder;
}

void Do3ThinkCameraControlPanel::onProcessingConfigurationClicked()
{
    emit processingConfigurationRequested();
    
    // Could also open a detailed configuration dialog here
    // showProcessingConfiguration();
}

void Do3ThinkCameraControlPanel::updateDiagnostics()
{
    if (!m_do3thinkState.advancedDiagnosticsEnabled) return;
    
    // Update diagnostics table
    if (m_do3thinkControls.diagnosticsTable) {
        // Update values in the table
        m_do3thinkControls.diagnosticsTable->item(0, 1)->setText(
            m_state.cameraConnected ? "Connected" : "Disconnected");
        m_do3thinkControls.diagnosticsTable->item(1, 1)->setText(
            formatTemperature(m_do3thinkState.lastTemperature));
        m_do3thinkControls.diagnosticsTable->item(2, 1)->setText(
            QString::number(m_statistics.currentFps, 'f', 1) + " fps");
        // ... update other rows
    }
    
    // Log diagnostic info
    if (m_do3thinkControls.diagnosticsLog) {
        QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
        QString logEntry = QString("[%1] FPS: %2, Temp: %3°C")
            .arg(timestamp)
            .arg(m_statistics.currentFps, 0, 'f', 1)
            .arg(m_do3thinkState.lastTemperature, 0, 'f', 1);
        
        // Limit log size
        if (m_do3thinkControls.diagnosticsLog->document()->lineCount() > 1000) {
            m_do3thinkControls.diagnosticsLog->clear();
        }
        
        m_do3thinkControls.diagnosticsLog->append(logEntry);
    }
}

void Do3ThinkCameraControlPanel::updateProfiling()
{
    if (!m_do3thinkState.profilingActive) return;
    
    // Collect profiling data
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    double elapsed = (currentTime - m_profilingData.profilingStartTime) / 1000.0;
    
    m_profilingData.timestamps.append(currentTime);
    m_profilingData.fpsHistory.append(m_statistics.currentFps);
    
    // Simulate CPU and memory data (would get from actual system)
    double cpuUsage = 20.0 + (QRandomGenerator::global()->bounded(30));
    double memoryUsage = 100.0 + (QRandomGenerator::global()->bounded(50));
    
    m_profilingData.cpuHistory.append(cpuUsage);
    m_profilingData.memoryHistory.append(memoryUsage);
    
    // Limit data size
    while (m_profilingData.timestamps.size() > m_profilingData.maxHistorySize) {
        m_profilingData.timestamps.removeFirst();
        m_profilingData.fpsHistory.removeFirst();
        m_profilingData.cpuHistory.removeFirst();
        m_profilingData.memoryHistory.removeFirst();
    }
    
    // Update charts
    updateCharts();
}

// ============================================================================
// Private Helper Methods
// ============================================================================

void Do3ThinkCameraControlPanel::setupDo3ThinkUI()
{
    // Create Do3Think specific UI components
    QWidget* do3thinkControls = createDo3ThinkControls();
    
    // Add to main layout
    if (m_mainLayout && do3thinkControls) {
        m_mainLayout->addWidget(do3thinkControls);
    }
}

void Do3ThinkCameraControlPanel::updateCharts()
{
    if (!m_do3thinkControls.performanceChart) return;
    
    QChart* chart = m_do3thinkControls.performanceChart->chart();
    if (!chart) return;
    
    // Update FPS series
    if (m_do3thinkControls.fpsSeries) {
        m_do3thinkControls.fpsSeries->clear();
        for (int i = 0; i < m_profilingData.fpsHistory.size(); ++i) {
            m_do3thinkControls.fpsSeries->append(i, m_profilingData.fpsHistory[i]);
        }
    }
    
    // Update CPU series
    if (m_do3thinkControls.cpuSeries) {
        m_do3thinkControls.cpuSeries->clear();
        for (int i = 0; i < m_profilingData.cpuHistory.size(); ++i) {
            m_do3thinkControls.cpuSeries->append(i, m_profilingData.cpuHistory[i]);
        }
    }
    
    // Update memory series
    if (m_do3thinkControls.memorySeries) {
        m_do3thinkControls.memorySeries->clear();
        for (int i = 0; i < m_profilingData.memoryHistory.size(); ++i) {
            m_do3thinkControls.memorySeries->append(i, m_profilingData.memoryHistory[i]);
        }
    }
    
    // Update axes
    chart->createDefaultAxes();
}

void Do3ThinkCameraControlPanel::processImageAnalysis(const QImage& image)
{
    m_analysisState.lastAnalyzedImage = image;
    
    if (m_analysisState.histogramEnabled) {
        QImage histogram = generateHistogram(image);
        // Display histogram
    }
    
    if (m_analysisState.waveformEnabled) {
        QImage waveform = generateWaveform(image);
        // Display waveform
    }
    
    if (m_analysisState.vectorscopeEnabled) {
        QImage vectorscope = generateVectorscope(image);
        // Display vectorscope
    }
    
    if (m_analysisState.focusPeakingEnabled) {
        QImage peaking = applyFocusPeaking(image);
        // Display with focus peaking
    }
    
    if (m_analysisState.zebraPatternEnabled) {
        QImage zebra = applyZebraPattern(image);
        // Display with zebra pattern
    }
}

QImage Do3ThinkCameraControlPanel::generateHistogram(const QImage& image)
{
    // Create histogram image
    QImage histogram(256, 200, QImage::Format_RGB32);
    histogram.fill(Qt::black);
    
    // Calculate histogram data
    std::vector<int> redHist(256, 0);
    std::vector<int> greenHist(256, 0);
    std::vector<int> blueHist(256, 0);
    
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            QRgb pixel = image.pixel(x, y);
            redHist[qRed(pixel)]++;
            greenHist[qGreen(pixel)]++;
            blueHist[qBlue(pixel)]++;
        }
    }
    
    // Find max value for normalization
    int maxVal = 0;
    for (int i = 0; i < 256; ++i) {
        maxVal = std::max(maxVal, redHist[i]);
        maxVal = std::max(maxVal, greenHist[i]);
        maxVal = std::max(maxVal, blueHist[i]);
    }
    
    // Draw histogram
    QPainter painter(&histogram);
    painter.setRenderHint(QPainter::Antialiasing);
    
    for (int i = 0; i < 256; ++i) {
        int redHeight = (redHist[i] * 200) / maxVal;
        int greenHeight = (greenHist[i] * 200) / maxVal;
        int blueHeight = (blueHist[i] * 200) / maxVal;
        
        painter.setPen(QColor(255, 0, 0, 128));
        painter.drawLine(i, 200, i, 200 - redHeight);
        
        painter.setPen(QColor(0, 255, 0, 128));
        painter.drawLine(i, 200, i, 200 - greenHeight);
        
        painter.setPen(QColor(0, 0, 255, 128));
        painter.drawLine(i, 200, i, 200 - blueHeight);
    }
    
    return histogram;
}

QImage Do3ThinkCameraControlPanel::generateWaveform(const QImage& image)
{
    // Create waveform image
    QImage waveform(image.width(), 256, QImage::Format_RGB32);
    waveform.fill(Qt::black);
    
    QPainter painter(&waveform);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Draw waveform for each column
    for (int x = 0; x < image.width(); ++x) {
        for (int y = 0; y < image.height(); ++y) {
            QRgb pixel = image.pixel(x, y);
            int luma = qGray(pixel);
            
            painter.setPen(QColor(255, 255, 255, 64));
            painter.drawPoint(x, 255 - luma);
        }
    }
    
    return waveform;
}

QImage Do3ThinkCameraControlPanel::generateVectorscope(const QImage& image)
{
    // Create vectorscope image
    QImage vectorscope(256, 256, QImage::Format_RGB32);
    vectorscope.fill(Qt::black);
    
    QPainter painter(&vectorscope);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Draw vectorscope grid
    painter.setPen(QColor(64, 64, 64));
    painter.drawEllipse(QPoint(128, 128), 120, 120);
    painter.drawLine(128, 8, 128, 248);
    painter.drawLine(8, 128, 248, 128);
    
    // Plot color vectors
    for (int y = 0; y < image.height(); y += 4) {
        for (int x = 0; x < image.width(); x += 4) {
            QRgb pixel = image.pixel(x, y);
            
            // Convert to YUV
            int r = qRed(pixel);
            int g = qGreen(pixel);
            int b = qBlue(pixel);
            
            double u = -0.147 * r - 0.289 * g + 0.436 * b;
            double v = 0.615 * r - 0.515 * g - 0.100 * b;
            
            // Map to vectorscope coordinates
            int scopeX = 128 + static_cast<int>(u);
            int scopeY = 128 - static_cast<int>(v);
            
            painter.setPen(QColor(r, g, b, 128));
            painter.drawPoint(scopeX, scopeY);
        }
    }
    
    return vectorscope;
}

QImage Do3ThinkCameraControlPanel::applyFocusPeaking(const QImage& image)
{
    QImage result = image.copy();
    
    // Simple edge detection for focus peaking
    for (int y = 1; y < image.height() - 1; ++y) {
        for (int x = 1; x < image.width() - 1; ++x) {
            // Calculate gradient
            QRgb center = image.pixel(x, y);
            QRgb left = image.pixel(x - 1, y);
            QRgb right = image.pixel(x + 1, y);
            QRgb top = image.pixel(x, y - 1);
            QRgb bottom = image.pixel(x, y + 1);
            
            int dx = qGray(right) - qGray(left);
            int dy = qGray(bottom) - qGray(top);
            int gradient = std::sqrt(dx * dx + dy * dy);
            
            // Highlight edges in red
            if (gradient > 30) {
                result.setPixel(x, y, qRgb(255, 0, 0));
            }
        }
    }
    
    return result;
}

QImage Do3ThinkCameraControlPanel::applyZebraPattern(const QImage& image)
{
    QImage result = image.copy();
    
    // Apply zebra stripes to overexposed areas
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            QRgb pixel = image.pixel(x, y);
            int luma = qGray(pixel);
            
            if (luma > m_analysisState.zebraThreshold) {
                // Create diagonal stripe pattern
                if ((x + y) % 20 < 10) {
                    result.setPixel(x, y, qRgb(255, 0, 255)); // Magenta stripes
                }
            }
        }
    }
    
    return result;
}

QString Do3ThinkCameraControlPanel::formatTemperature(double temp) const
{
    return QString("Temperature: %1°C").arg(temp, 0, 'f', 1);
}

void Do3ThinkCameraControlPanel::applyDo3ThinkTheme()
{
    // Apply modern dark industrial theme with Do3Think specific enhancements
    QString do3thinkEnhancedStyle = ComponentsForest::FuturisticTheme::getFuturisticStylesheet() + R"(
        /* ==================== Do3Think Specific Enhancements ==================== */
        
        /* Camera control buttons with gradient effects */
        QPushButton#CameraControlButton {
            background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
                                        stop: 0 #2ea043, stop: 1 #238636);
            border: 1px solid #3fb950;
            border-radius: 6px;
            padding: 8px 16px;
            font-weight: 600;
            color: white;
            min-height: 32px;
        }
        
        QPushButton#CameraControlButton:hover {
            background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
                                        stop: 0 #3fb950, stop: 1 #2ea043);
            border: 2px solid #3fb950;
        }
        
        QPushButton#CameraControlButton:pressed {
            background: #238636;
        }
        
        /* Stop button with red theme */
        QPushButton#StopButton {
            background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
                                        stop: 0 #da3633, stop: 1 #b62324);
            border: 1px solid #f85149;
        }
        
        QPushButton#StopButton:hover {
            background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
                                        stop: 0 #f85149, stop: 1 #da3633);
        }
        
        /* HDR toggle with special styling */
        QCheckBox#HDRToggle {
            spacing: 8px;
            color: #58a6ff;
            font-weight: 600;
        }
        
        QCheckBox#HDRToggle::indicator {
            width: 20px;
            height: 20px;
            border-radius: 4px;
            border: 2px solid #58a6ff;
            background: #161b22;
        }
        
        QCheckBox#HDRToggle::indicator:checked {
            background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 1,
                                        stop: 0 #58a6ff, stop: 1 #8b5cf6);
            image: url(:/icons/check.png);
        }
        
        /* Temperature gauge styling */
        QProgressBar#TemperatureGauge {
            border: 2px solid #30363d;
            border-radius: 5px;
            text-align: center;
            background: #0d1117;
        }
        
        QProgressBar#TemperatureGauge::chunk {
            background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0,
                                        stop: 0 #3fb950, stop: 0.5 #d29922, stop: 1 #f85149);
            border-radius: 3px;
        }
        
        /* GPIO indicator LEDs */
        QLabel#GPIOIndicator {
            width: 16px;
            height: 16px;
            border-radius: 8px;
            border: 1px solid #30363d;
        }
        
        QLabel#GPIOIndicatorOn {
            background: radial-gradient(circle, #3fb950 0%, #238636 100%);
            border: 1px solid #3fb950;
        }
        
        QLabel#GPIOIndicatorOff {
            background: #161b22;
            border: 1px solid #30363d;
        }
        
        /* Advanced diagnostic charts */
        QChartView {
            background: transparent;
            border: 1px solid #30363d;
            border-radius: 4px;
        }
        
        /* Tab widget for Do3Think controls */
        QTabWidget#Do3ThinkTabs::pane {
            border: 1px solid #30363d;
            background: rgba(22, 27, 34, 0.95);
            border-radius: 0 4px 4px 4px;
        }
        
        QTabWidget#Do3ThinkTabs QTabBar::tab {
            background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
                                        stop: 0 #21262d, stop: 1 #161b22);
            border: 1px solid #30363d;
            padding: 8px 16px;
            margin-right: 2px;
            border-radius: 4px 4px 0 0;
        }
        
        QTabWidget#Do3ThinkTabs QTabBar::tab:selected {
            background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
                                        stop: 0 #58a6ff, stop: 1 #4a90e2);
            color: white;
            border-bottom: 2px solid #58a6ff;
        }
    )";
    
    setStyleSheet(do3thinkEnhancedStyle);
    
    // Apply object names to ensure styling works
    if (m_do3thinkControls.hdrEnableCheck) {
        m_do3thinkControls.hdrEnableCheck->setObjectName("HDRToggle");
    }
    
    if (m_do3thinkControls.temperatureBar) {
        m_do3thinkControls.temperatureBar->setObjectName("TemperatureGauge");
    }
    
    if (m_do3thinkControls.do3ThinkTabs) {
        m_do3thinkControls.do3ThinkTabs->setObjectName("Do3ThinkTabs");
    }
    
    // Set camera control button object names
    auto buttons = findChildren<QPushButton*>();
    for (auto* button : buttons) {
        QString text = button->text().toLower();
        if (text.contains("start") || text.contains("capture")) {
            button->setObjectName("CameraControlButton");
        } else if (text.contains("stop")) {
            button->setObjectName("StopButton");
        }
    }
}

} // namespace ComponentsForest