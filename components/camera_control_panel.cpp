#include "camera_control_panel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QGroupBox>
#include <QTabWidget>
#include <QProgressBar>
#include <QTimer>
#include <QMessageBox>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonArray>
#include <QFile>
#include <QTextStream>
#include <QMutex>
#include <QMutexLocker>
#include <QMetaObject>
#include <QMetaMethod>
#include <QPixmap>
#include <QPainter>
// #include <QtConcurrent>  // Not used, removed to fix Qt6 compilation
#include <algorithm>
#include <cmath>
#include <functional>

namespace ComponentsForest {

// Thread-safe UI update mutex
static QMutex g_uiUpdateMutex;

CameraControlPanel::CameraControlPanel(QWidget* parent)
    : QWidget(parent)
    , m_connectedComponent(nullptr)
    , m_updateTimer(nullptr)
    , m_statisticsTimer(nullptr)
    , m_mainLayout(nullptr)
    , m_topLayout(nullptr)
    , m_parameterLayout(nullptr)
{
    // Initialize state
    m_state.currentLayout = PanelLayout::Standard;
    m_state.displayMode = ImageDisplayMode::Embedded;
    
    // Create the UI
    createUI();
    
    // Setup timers for periodic updates
    m_updateTimer = new QTimer(this);
    m_updateTimer->setInterval(33); // ~30 FPS for image display
    connect(m_updateTimer, &QTimer::timeout, this, &CameraControlPanel::updateImageDisplay);
    
    m_statisticsTimer = new QTimer(this);
    m_statisticsTimer->setInterval(1000); // Update statistics every second
    connect(m_statisticsTimer, &QTimer::timeout, this, &CameraControlPanel::updateStatistics);
    
    // Connect internal signals
    connectInternalSignals();
    
    // Apply default theme
    applyTheme("default");
}

CameraControlPanel::~CameraControlPanel()
{
    // Safely disconnect from component
    if (m_connectedComponent) {
        disconnectFromComponent();
    }
    
    // Stop timers
    if (m_updateTimer) {
        m_updateTimer->stop();
    }
    if (m_statisticsTimer) {
        m_statisticsTimer->stop();
    }
}

// ========== Component Connection ==========

void CameraControlPanel::connectToComponent(QObject* cameraComponent)
{
    if (!cameraComponent) {
        return;
    }
    
    // Disconnect from previous component if any
    if (m_connectedComponent) {
        disconnectFromComponent();
    }
    
    m_connectedComponent = cameraComponent;
    m_state.connectedToComponent = true;
    
    // Connect to component signals
    connectComponentSignals(cameraComponent);
    
    // Request initial state
    QMetaObject::invokeMethod(cameraComponent, "scanDevices", Qt::QueuedConnection);
    
    // Start update timers
    m_statisticsTimer->start();
    
    emit connectionStatusChanged(true);
}

void CameraControlPanel::disconnectFromComponent()
{
    if (!m_connectedComponent) {
        return;
    }
    
    // Stop timers
    m_updateTimer->stop();
    m_statisticsTimer->stop();
    
    // Disconnect signals
    disconnectComponentSignals(m_connectedComponent);
    
    m_connectedComponent = nullptr;
    m_state.connectedToComponent = false;
    m_state.cameraConnected = false;
    m_state.acquiring = false;
    
    // Update UI state
    updateUIState(false, false);
    
    emit connectionStatusChanged(false);
}

bool CameraControlPanel::isConnectedToComponent() const
{
    return m_state.connectedToComponent;
}

QObject* CameraControlPanel::connectedComponent() const
{
    return m_connectedComponent;
}

// ========== Panel Configuration ==========

void CameraControlPanel::setPanelLayout(PanelLayout layout)
{
    if (m_state.currentLayout == layout) {
        return;
    }
    
    m_state.currentLayout = layout;
    
    // Recreate UI with new layout
    switch (layout) {
        case PanelLayout::Compact:
            createCompactLayout();
            break;
        case PanelLayout::Advanced:
            createAdvancedLayout();
            break;
        case PanelLayout::Standard:
        default:
            createStandardLayout();
            break;
    }
    
    emit layoutChanged(layout);
}

PanelLayout CameraControlPanel::panelLayout() const
{
    return m_state.currentLayout;
}

void CameraControlPanel::setAutoConnect(bool enable)
{
    m_state.autoConnect = enable;
    emit autoConnectChanged(enable);
    
    if (enable && !m_state.cameraConnected && !m_state.availableDevices.isEmpty()) {
        // Auto-connect to first available device
        QString firstDevice = m_state.availableDevices[0].toObject()["serialNumber"].toString();
        emit requestConnect(firstDevice);
    }
}

bool CameraControlPanel::isAutoConnectEnabled() const
{
    return m_state.autoConnect;
}

void CameraControlPanel::setImageDisplayMode(ImageDisplayMode mode)
{
    if (m_state.displayMode == mode) {
        return;
    }
    
    m_state.displayMode = mode;
    
    // Recreate image display widget
    if (m_controls.imageDisplay && m_controls.imageDisplay->parent()) {
        delete m_controls.imageDisplay;
        m_controls.imageDisplay = nullptr;
    }
    
    createImageDisplay();
    
    emit displayModeChanged(mode);
}

ImageDisplayMode CameraControlPanel::imageDisplayMode() const
{
    return m_state.displayMode;
}

void CameraControlPanel::setControlGroupEnabled(const QString& groupName, bool enabled)
{
    QWidget* group = getControlGroup(groupName);
    if (group) {
        group->setEnabled(enabled);
        group->setVisible(enabled);
    }
}

bool CameraControlPanel::isControlGroupEnabled(const QString& groupName) const
{
    QWidget* group = getControlGroup(groupName);
    return group ? group->isEnabled() : false;
}

bool CameraControlPanel::savePanelConfiguration(const QString& filePath) const
{
    QJsonObject config;
    
    // Save panel state
    config["layout"] = static_cast<int>(m_state.currentLayout);
    config["displayMode"] = static_cast<int>(m_state.displayMode);
    config["autoConnect"] = m_state.autoConnect;
    config["showAdvanced"] = m_state.showAdvanced;
    
    // Save display settings
    QJsonObject displaySettings;
    displaySettings["autoScale"] = m_display.autoScale;
    displaySettings["zoomLevel"] = m_display.zoomLevel;
    displaySettings["showOverlay"] = m_display.showOverlay;
    displaySettings["showCrosshair"] = m_display.showCrosshair;
    displaySettings["showHistogram"] = m_display.showHistogram;
    config["displaySettings"] = displaySettings;
    
    // Save control states
    QJsonObject controlStates;
    if (m_controls.autoExposureCheck) {
        controlStates["autoExposure"] = m_controls.autoExposureCheck->isChecked();
    }
    if (m_controls.autoGainCheck) {
        controlStates["autoGain"] = m_controls.autoGainCheck->isChecked();
    }
    config["controlStates"] = controlStates;
    
    // Write to file
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        showError(tr("Failed to save configuration: %1").arg(file.errorString()));
        return false;
    }
    
    QJsonDocument doc(config);
    file.write(doc.toJson());
    file.close();
    
    showInfo(tr("Configuration saved successfully"));
    return true;
}

bool CameraControlPanel::loadPanelConfiguration(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        showError(tr("Failed to load configuration: %1").arg(file.errorString()));
        return false;
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    
    if (!doc.isObject()) {
        showError(tr("Invalid configuration file format"));
        return false;
    }
    
    QJsonObject config = doc.object();
    
    // Load panel state
    if (config.contains("layout")) {
        setPanelLayout(static_cast<PanelLayout>(config["layout"].toInt()));
    }
    if (config.contains("displayMode")) {
        setImageDisplayMode(static_cast<ImageDisplayMode>(config["displayMode"].toInt()));
    }
    if (config.contains("autoConnect")) {
        setAutoConnect(config["autoConnect"].toBool());
    }
    if (config.contains("showAdvanced")) {
        setShowAdvancedControls(config["showAdvanced"].toBool());
    }
    
    // Load display settings
    if (config.contains("displaySettings")) {
        QJsonObject displaySettings = config["displaySettings"].toObject();
        m_display.autoScale = displaySettings["autoScale"].toBool();
        m_display.zoomLevel = displaySettings["zoomLevel"].toDouble();
        m_display.showOverlay = displaySettings["showOverlay"].toBool();
        m_display.showCrosshair = displaySettings["showCrosshair"].toBool();
        m_display.showHistogram = displaySettings["showHistogram"].toBool();
    }
    
    // Load control states
    if (config.contains("controlStates")) {
        QJsonObject controlStates = config["controlStates"].toObject();
        if (m_controls.autoExposureCheck && controlStates.contains("autoExposure")) {
            m_controls.autoExposureCheck->setChecked(controlStates["autoExposure"].toBool());
        }
        if (m_controls.autoGainCheck && controlStates.contains("autoGain")) {
            m_controls.autoGainCheck->setChecked(controlStates["autoGain"].toBool());
        }
    }
    
    showInfo(tr("Configuration loaded successfully"));
    return true;
}

// ========== UI Access Methods ==========

QWidget* CameraControlPanel::getControlGroup(const QString& groupName) const
{
    if (groupName == "connection") return m_controls.connectionGroup;
    if (groupName == "acquisition") return m_controls.acquisitionGroup;
    if (groupName == "parameters") return m_controls.parametersGroup;
    if (groupName == "statistics") return m_controls.statisticsGroup;
    return nullptr;
}

QPushButton* CameraControlPanel::getButton(const QString& buttonName) const
{
    if (buttonName == "scan") return m_controls.scanButton;
    if (buttonName == "connect") return m_controls.connectButton;
    if (buttonName == "disconnect") return m_controls.disconnectButton;
    if (buttonName == "start") return m_controls.startButton;
    if (buttonName == "stop") return m_controls.stopButton;
    if (buttonName == "singleFrame") return m_controls.singleFrameButton;
    if (buttonName == "trigger") return m_controls.triggerButton;
    return nullptr;
}

QSlider* CameraControlPanel::getSlider(const QString& sliderName) const
{
    if (sliderName == "exposure") return m_controls.exposureSlider;
    if (sliderName == "gain") return m_controls.gainSlider;
    return nullptr;
}

// ========== State Query ==========

bool CameraControlPanel::isConnected() const
{
    return m_state.cameraConnected;
}

bool CameraControlPanel::isAcquiring() const
{
    return m_state.acquiring;
}

QString CameraControlPanel::currentDeviceInfo() const
{
    return m_state.currentDevice;
}

// ========== Utility Methods ==========

void CameraControlPanel::applyTheme(const QString& themeName)
{
    QString styleSheet;
    
    if (themeName == "dark") {
        styleSheet = R"(
            QWidget {
                background-color: #2b2b2b;
                color: #ffffff;
            }
            QGroupBox {
                border: 1px solid #555555;
                border-radius: 5px;
                margin-top: 10px;
                padding-top: 10px;
            }
            QGroupBox::title {
                subcontrol-origin: margin;
                left: 10px;
                padding: 0 5px 0 5px;
            }
            QPushButton {
                background-color: #3c3c3c;
                border: 1px solid #555555;
                border-radius: 3px;
                padding: 5px;
                min-width: 80px;
            }
            QPushButton:hover {
                background-color: #4a4a4a;
            }
            QPushButton:pressed {
                background-color: #2a2a2a;
            }
            QPushButton:disabled {
                background-color: #1a1a1a;
                color: #666666;
            }
            QSlider::groove:horizontal {
                height: 6px;
                background: #3c3c3c;
                border-radius: 3px;
            }
            QSlider::handle:horizontal {
                width: 18px;
                height: 18px;
                background: #5a5a5a;
                border-radius: 9px;
            }
            QProgressBar {
                text-align: center;
                border: 1px solid #555555;
                border-radius: 3px;
            }
            QProgressBar::chunk {
                background-color: #4a90e2;
                border-radius: 3px;
            }
        )";
    } else {
        // Default light theme
        styleSheet = R"(
            QGroupBox {
                border: 1px solid #cccccc;
                border-radius: 5px;
                margin-top: 10px;
                padding-top: 10px;
                font-weight: bold;
            }
            QGroupBox::title {
                subcontrol-origin: margin;
                left: 10px;
                padding: 0 5px 0 5px;
            }
            QPushButton {
                padding: 5px;
                min-width: 80px;
            }
            QProgressBar {
                text-align: center;
            }
        )";
    }
    
    setStyleSheet(styleSheet);
}

void CameraControlPanel::setShowAdvancedControls(bool show)
{
    m_state.showAdvanced = show;
    
    // Show/hide advanced control groups
    if (m_controls.roiX) {
        m_controls.roiX->setVisible(show);
        m_controls.roiY->setVisible(show);
        m_controls.roiWidth->setVisible(show);
        m_controls.roiHeight->setVisible(show);
        m_controls.roiSetButton->setVisible(show);
        m_controls.roiResetButton->setVisible(show);
    }
    
    emit advancedControlsVisibilityChanged(show);
}

bool CameraControlPanel::showAdvancedControls() const
{
    return m_state.showAdvanced;
}

void CameraControlPanel::setTooltipsEnabled(bool enabled)
{
    // Enable/disable tooltips for all controls
    std::function<void(QWidget*)> setTooltipsRecursive = [&](QWidget* widget) {
        if (!widget) return;
        
        if (enabled) {
            // Set helpful tooltips
            if (auto btn = qobject_cast<QPushButton*>(widget)) {
                if (btn->objectName() == "scanButton") {
                    btn->setToolTip(tr("Scan for available cameras"));
                } else if (btn->objectName() == "connectButton") {
                    btn->setToolTip(tr("Connect to selected camera"));
                }
            }
        } else {
            widget->setToolTip("");
        }
        
        // Recursively process children
        for (auto child : widget->children()) {
            if (auto childWidget = qobject_cast<QWidget*>(child)) {
                setTooltipsRecursive(childWidget);
            }
        }
    };
    
    setTooltipsRecursive(this);
}

// ========== Receive Component Updates (Slots) ==========

void CameraControlPanel::onDeviceListUpdated(const QJsonArray& devices)
{
    QMutexLocker locker(&g_uiUpdateMutex);
    
    m_state.availableDevices = devices;
    updateDeviceList(devices);
    
    // Auto-connect if enabled and not connected
    if (m_state.autoConnect && !m_state.cameraConnected && !devices.isEmpty()) {
        QString firstDevice = devices[0].toObject()["serialNumber"].toString();
        emit requestConnect(firstDevice);
    }
}

void CameraControlPanel::onDeviceConnected(const QString& deviceId)
{
    QMutexLocker locker(&g_uiUpdateMutex);
    
    m_state.cameraConnected = true;
    m_state.currentDevice = deviceId;
    
    updateUIState(true, false);
    
    if (m_controls.connectionStatus) {
        m_controls.connectionStatus->setText(tr("Connected: %1").arg(deviceId));
        m_controls.connectionStatus->setStyleSheet("QLabel { color: green; }");
    }
    
    showInfo(tr("Connected to camera: %1").arg(deviceId));
}

void CameraControlPanel::onDeviceDisconnected()
{
    QMutexLocker locker(&g_uiUpdateMutex);
    
    m_state.cameraConnected = false;
    m_state.acquiring = false;
    m_state.currentDevice.clear();
    
    updateUIState(false, false);
    
    if (m_controls.connectionStatus) {
        m_controls.connectionStatus->setText(tr("Disconnected"));
        m_controls.connectionStatus->setStyleSheet("QLabel { color: gray; }");
    }
    
    // Stop image update timer
    m_updateTimer->stop();
    
    showInfo(tr("Camera disconnected"));
}

void CameraControlPanel::onCameraStateChanged(int state)
{
    QMutexLocker locker(&g_uiUpdateMutex);
    
    CameraState cameraState = static_cast<CameraState>(state);
    
    switch (cameraState) {
        case CameraState::Disconnected:
            onDeviceDisconnected();
            break;
        case CameraState::Connected:
            m_state.cameraConnected = true;
            m_state.acquiring = false;
            updateUIState(true, false);
            break;
        case CameraState::Acquiring:
            m_state.acquiring = true;
            updateUIState(true, true);
            m_updateTimer->start();
            break;
        case CameraState::Error:
            showError(tr("Camera entered error state"));
            break;
        case CameraState::Recovering:
            showInfo(tr("Camera is recovering..."));
            break;
    }
}

void CameraControlPanel::onAcquisitionStateChanged(bool acquiring)
{
    QMutexLocker locker(&g_uiUpdateMutex);
    
    m_state.acquiring = acquiring;
    updateUIState(m_state.cameraConnected, acquiring);
    
    if (acquiring) {
        m_updateTimer->start();
        showInfo(tr("Acquisition started"));
    } else {
        m_updateTimer->stop();
        showInfo(tr("Acquisition stopped"));
    }
}

void CameraControlPanel::onExposureChanged(double microseconds)
{
    QMutexLocker locker(&g_uiUpdateMutex);
    
    if (m_controls.exposureSpinBox && !m_controls.exposureSpinBox->hasFocus()) {
        m_controls.exposureSpinBox->setValue(microseconds);
    }
    if (m_controls.exposureSlider && !m_controls.exposureSlider->isSliderDown()) {
        m_controls.exposureSlider->setValue(static_cast<int>(microseconds));
    }
}

void CameraControlPanel::onGainChanged(double gain)
{
    QMutexLocker locker(&g_uiUpdateMutex);
    
    if (m_controls.gainSpinBox && !m_controls.gainSpinBox->hasFocus()) {
        m_controls.gainSpinBox->setValue(gain);
    }
    if (m_controls.gainSlider && !m_controls.gainSlider->isSliderDown()) {
        m_controls.gainSlider->setValue(static_cast<int>(gain * 10));
    }
}

void CameraControlPanel::onFrameRateChanged(double fps)
{
    QMutexLocker locker(&g_uiUpdateMutex);
    
    if (m_controls.frameRateSpinBox && !m_controls.frameRateSpinBox->hasFocus()) {
        m_controls.frameRateSpinBox->setValue(fps);
    }
}

void CameraControlPanel::onROIChanged(const QRect& roi)
{
    QMutexLocker locker(&g_uiUpdateMutex);
    
    if (m_controls.roiX && !m_controls.roiX->hasFocus()) {
        m_controls.roiX->setValue(roi.x());
    }
    if (m_controls.roiY && !m_controls.roiY->hasFocus()) {
        m_controls.roiY->setValue(roi.y());
    }
    if (m_controls.roiWidth && !m_controls.roiWidth->hasFocus()) {
        m_controls.roiWidth->setValue(roi.width());
    }
    if (m_controls.roiHeight && !m_controls.roiHeight->hasFocus()) {
        m_controls.roiHeight->setValue(roi.height());
    }
}

void CameraControlPanel::onTriggerModeChanged(int mode)
{
    QMutexLocker locker(&g_uiUpdateMutex);
    
    if (m_controls.triggerModeCombo) {
        m_controls.triggerModeCombo->setCurrentIndex(mode);
    }
}

void CameraControlPanel::onPixelFormatChanged(int format)
{
    QMutexLocker locker(&g_uiUpdateMutex);
    
    if (m_controls.pixelFormatCombo) {
        m_controls.pixelFormatCombo->setCurrentIndex(format);
    }
}

void CameraControlPanel::onImageReceived(const QImage& image, qint64 timestamp)
{
    QMutexLocker locker(&g_uiUpdateMutex);
    
    m_display.currentImage = image;
    m_statistics.frameCount++;
    m_statistics.lastFrameTime = timestamp;
    
    // Update frame count display
    if (m_controls.frameCountLabel) {
        m_controls.frameCountLabel->setText(tr("Frames: %1").arg(m_statistics.frameCount));
    }
}

void CameraControlPanel::onFrameDropped(qint64 frameNumber)
{
    QMutexLocker locker(&g_uiUpdateMutex);
    
    m_statistics.droppedFrames++;
    
    // Update dropped frame counter
    if (m_controls.frameCountLabel) {
        m_controls.frameCountLabel->setText(
            tr("Frames: %1 (Dropped: %2)")
            .arg(m_statistics.frameCount)
            .arg(m_statistics.droppedFrames)
        );
    }
}

void CameraControlPanel::onFpsUpdated(double fps)
{
    QMutexLocker locker(&g_uiUpdateMutex);
    
    m_statistics.currentFps = fps;
    
    if (m_controls.fpsLabel) {
        m_controls.fpsLabel->setText(formatFrameRate(fps));
    }
}

void CameraControlPanel::onStatisticsUpdated(const QJsonObject& stats)
{
    QMutexLocker locker(&g_uiUpdateMutex);
    
    // Update buffer usage if available
    if (stats.contains("bufferUsage") && m_controls.bufferUsage) {
        double usage = stats["bufferUsage"].toDouble();
        m_controls.bufferUsage->setValue(static_cast<int>(usage * 100));
    }
    
    // Update other statistics as needed
    if (stats.contains("bandwidth")) {
        double bandwidth = stats["bandwidth"].toDouble();
        // Update bandwidth display if available
    }
}

void CameraControlPanel::onErrorOccurred(const QString& error)
{
    showError(error);
}

void CameraControlPanel::onWarningOccurred(const QString& warning)
{
    showInfo(warning);
}

// ========== Protected Virtual UI Creation Methods ==========

void CameraControlPanel::createUI()
{
    setupDefaultUI();
    
    switch (m_state.currentLayout) {
        case PanelLayout::Compact:
            createCompactLayout();
            break;
        case PanelLayout::Advanced:
            createAdvancedLayout();
            break;
        case PanelLayout::Standard:
        default:
            createStandardLayout();
            break;
    }
}

QWidget* CameraControlPanel::createConnectionControls()
{
    QGroupBox* group = new QGroupBox(tr("Connection"));
    m_controls.connectionGroup = group;
    
    QVBoxLayout* layout = new QVBoxLayout(group);
    
    // Device selection row
    QHBoxLayout* deviceLayout = new QHBoxLayout;
    
    m_controls.scanButton = new QPushButton(tr("Scan"));
    m_controls.scanButton->setObjectName("scanButton");
    
    m_controls.deviceCombo = new QComboBox;
    m_controls.deviceCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    
    deviceLayout->addWidget(m_controls.scanButton);
    deviceLayout->addWidget(m_controls.deviceCombo);
    
    // Connection buttons row
    QHBoxLayout* buttonLayout = new QHBoxLayout;
    
    m_controls.connectButton = new QPushButton(tr("Connect"));
    m_controls.connectButton->setObjectName("connectButton");
    
    m_controls.disconnectButton = new QPushButton(tr("Disconnect"));
    m_controls.disconnectButton->setObjectName("disconnectButton");
    m_controls.disconnectButton->setEnabled(false);
    
    buttonLayout->addWidget(m_controls.connectButton);
    buttonLayout->addWidget(m_controls.disconnectButton);
    
    // Status label
    m_controls.connectionStatus = new QLabel(tr("Disconnected"));
    m_controls.connectionStatus->setAlignment(Qt::AlignCenter);
    m_controls.connectionStatus->setStyleSheet("QLabel { color: gray; }");
    
    layout->addLayout(deviceLayout);
    layout->addLayout(buttonLayout);
    layout->addWidget(m_controls.connectionStatus);
    
    return group;
}

QWidget* CameraControlPanel::createAcquisitionControls()
{
    QGroupBox* group = new QGroupBox(tr("Acquisition"));
    m_controls.acquisitionGroup = group;
    
    QGridLayout* layout = new QGridLayout(group);
    
    m_controls.startButton = new QPushButton(tr("Start"));
    m_controls.startButton->setEnabled(false);
    
    m_controls.stopButton = new QPushButton(tr("Stop"));
    m_controls.stopButton->setEnabled(false);
    
    m_controls.singleFrameButton = new QPushButton(tr("Single Frame"));
    m_controls.singleFrameButton->setEnabled(false);
    
    m_controls.triggerButton = new QPushButton(tr("Trigger"));
    m_controls.triggerButton->setEnabled(false);
    
    layout->addWidget(m_controls.startButton, 0, 0);
    layout->addWidget(m_controls.stopButton, 0, 1);
    layout->addWidget(m_controls.singleFrameButton, 1, 0);
    layout->addWidget(m_controls.triggerButton, 1, 1);
    
    return group;
}

QWidget* CameraControlPanel::createBasicParameterControls()
{
    QGroupBox* group = new QGroupBox(tr("Parameters"));
    m_controls.parametersGroup = group;
    
    QGridLayout* layout = new QGridLayout(group);
    m_parameterLayout = layout;
    
    int row = 0;
    
    // Exposure control
    layout->addWidget(new QLabel(tr("Exposure (μs):")), row, 0);
    
    m_controls.exposureSlider = new QSlider(Qt::Horizontal);
    m_controls.exposureSlider->setRange(10, 1000000);
    m_controls.exposureSlider->setValue(1000);
    
    m_controls.exposureSpinBox = new QDoubleSpinBox;
    m_controls.exposureSpinBox->setRange(10.0, 1000000.0);
    m_controls.exposureSpinBox->setValue(1000.0);
    m_controls.exposureSpinBox->setSuffix(" μs");
    
    layout->addWidget(m_controls.exposureSlider, row, 1);
    layout->addWidget(m_controls.exposureSpinBox, row, 2);
    row++;
    
    // Gain control
    layout->addWidget(new QLabel(tr("Gain:")), row, 0);
    
    m_controls.gainSlider = new QSlider(Qt::Horizontal);
    m_controls.gainSlider->setRange(10, 160); // 1.0 to 16.0 (x10)
    m_controls.gainSlider->setValue(10);
    
    m_controls.gainSpinBox = new QDoubleSpinBox;
    m_controls.gainSpinBox->setRange(1.0, 16.0);
    m_controls.gainSpinBox->setValue(1.0);
    m_controls.gainSpinBox->setSingleStep(0.1);
    
    layout->addWidget(m_controls.gainSlider, row, 1);
    layout->addWidget(m_controls.gainSpinBox, row, 2);
    row++;
    
    // Frame rate control
    layout->addWidget(new QLabel(tr("Frame Rate:")), row, 0);
    
    m_controls.frameRateSpinBox = new QDoubleSpinBox;
    m_controls.frameRateSpinBox->setRange(1.0, 1000.0);
    m_controls.frameRateSpinBox->setValue(30.0);
    m_controls.frameRateSpinBox->setSuffix(" fps");
    
    layout->addWidget(m_controls.frameRateSpinBox, row, 1, 1, 2);
    row++;
    
    // Trigger mode
    layout->addWidget(new QLabel(tr("Trigger Mode:")), row, 0);
    
    m_controls.triggerModeCombo = new QComboBox;
    m_controls.triggerModeCombo->addItems({
        tr("Free Run"),
        tr("Software"),
        tr("Hardware"),
        tr("Fixed Rate"),
        tr("Burst")
    });
    
    layout->addWidget(m_controls.triggerModeCombo, row, 1, 1, 2);
    row++;
    
    // Pixel format
    layout->addWidget(new QLabel(tr("Pixel Format:")), row, 0);
    
    m_controls.pixelFormatCombo = new QComboBox;
    m_controls.pixelFormatCombo->addItems({
        "Mono8", "Mono10", "Mono12", "Mono16",
        "RGB24", "BGR24", "RGBA32"
    });
    
    layout->addWidget(m_controls.pixelFormatCombo, row, 1, 1, 2);
    row++;
    
    // Auto features
    m_controls.autoExposureCheck = new QCheckBox(tr("Auto Exposure"));
    m_controls.autoGainCheck = new QCheckBox(tr("Auto Gain"));
    
    layout->addWidget(m_controls.autoExposureCheck, row, 0, 1, 2);
    layout->addWidget(m_controls.autoGainCheck, row, 2);
    
    return group;
}

QWidget* CameraControlPanel::createAdvancedParameterControls()
{
    QGroupBox* group = new QGroupBox(tr("ROI Settings"));
    
    QGridLayout* layout = new QGridLayout(group);
    
    // ROI controls
    layout->addWidget(new QLabel(tr("X:")), 0, 0);
    m_controls.roiX = new QSpinBox;
    m_controls.roiX->setRange(0, 4096);
    layout->addWidget(m_controls.roiX, 0, 1);
    
    layout->addWidget(new QLabel(tr("Y:")), 0, 2);
    m_controls.roiY = new QSpinBox;
    m_controls.roiY->setRange(0, 4096);
    layout->addWidget(m_controls.roiY, 0, 3);
    
    layout->addWidget(new QLabel(tr("Width:")), 1, 0);
    m_controls.roiWidth = new QSpinBox;
    m_controls.roiWidth->setRange(1, 4096);
    m_controls.roiWidth->setValue(640);
    layout->addWidget(m_controls.roiWidth, 1, 1);
    
    layout->addWidget(new QLabel(tr("Height:")), 1, 2);
    m_controls.roiHeight = new QSpinBox;
    m_controls.roiHeight->setRange(1, 4096);
    m_controls.roiHeight->setValue(480);
    layout->addWidget(m_controls.roiHeight, 1, 3);
    
    m_controls.roiSetButton = new QPushButton(tr("Set ROI"));
    m_controls.roiResetButton = new QPushButton(tr("Reset ROI"));
    
    layout->addWidget(m_controls.roiSetButton, 2, 0, 1, 2);
    layout->addWidget(m_controls.roiResetButton, 2, 2, 1, 2);
    
    return group;
}

QWidget* CameraControlPanel::createImageDisplay()
{
    if (m_state.displayMode == ImageDisplayMode::None) {
        return nullptr;
    }
    
    QWidget* container = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout(container);
    
    // Create image label
    m_controls.imageDisplay = new QLabel;
    m_controls.imageDisplay->setMinimumSize(320, 240);
    m_controls.imageDisplay->setAlignment(Qt::AlignCenter);
    m_controls.imageDisplay->setScaledContents(m_display.autoScale);
    m_controls.imageDisplay->setStyleSheet("QLabel { border: 1px solid #cccccc; background: black; }");
    m_controls.imageDisplay->setText(tr("No Image"));
    
    layout->addWidget(m_controls.imageDisplay);
    
    // Add zoom controls if in advanced mode
    if (m_state.showAdvanced) {
        QHBoxLayout* zoomLayout = new QHBoxLayout;
        
        QPushButton* zoomInBtn = new QPushButton(tr("Zoom In"));
        QPushButton* zoomOutBtn = new QPushButton(tr("Zoom Out"));
        QPushButton* fitBtn = new QPushButton(tr("Fit"));
        
        connect(zoomInBtn, &QPushButton::clicked, [this]() {
            m_display.zoomLevel *= 1.25;
            updateImageDisplay();
        });
        
        connect(zoomOutBtn, &QPushButton::clicked, [this]() {
            m_display.zoomLevel *= 0.8;
            updateImageDisplay();
        });
        
        connect(fitBtn, &QPushButton::clicked, [this]() {
            m_display.zoomLevel = 1.0;
            m_display.autoScale = true;
            updateImageDisplay();
        });
        
        zoomLayout->addWidget(zoomInBtn);
        zoomLayout->addWidget(zoomOutBtn);
        zoomLayout->addWidget(fitBtn);
        zoomLayout->addStretch();
        
        layout->addLayout(zoomLayout);
    }
    
    return container;
}

QWidget* CameraControlPanel::createStatisticsDisplay()
{
    QGroupBox* group = new QGroupBox(tr("Statistics"));
    m_controls.statisticsGroup = group;
    
    QGridLayout* layout = new QGridLayout(group);
    
    // FPS display
    m_controls.fpsLabel = new QLabel(tr("FPS: 0.0"));
    layout->addWidget(new QLabel(tr("Frame Rate:")), 0, 0);
    layout->addWidget(m_controls.fpsLabel, 0, 1);
    
    // Frame count
    m_controls.frameCountLabel = new QLabel(tr("Frames: 0"));
    layout->addWidget(new QLabel(tr("Frame Count:")), 1, 0);
    layout->addWidget(m_controls.frameCountLabel, 1, 1);
    
    // Buffer usage
    m_controls.bufferUsage = new QProgressBar;
    m_controls.bufferUsage->setRange(0, 100);
    m_controls.bufferUsage->setValue(0);
    m_controls.bufferUsage->setFormat(tr("Buffer: %p%"));
    layout->addWidget(new QLabel(tr("Buffer Usage:")), 2, 0);
    layout->addWidget(m_controls.bufferUsage, 2, 1);
    
    return group;
}

// ========== Internal Signal/Slot Connections ==========

void CameraControlPanel::connectInternalSignals()
{
    // Connection controls
    if (m_controls.scanButton) {
        connect(m_controls.scanButton, &QPushButton::clicked, 
                this, &CameraControlPanel::onScanButtonClicked);
    }
    if (m_controls.connectButton) {
        connect(m_controls.connectButton, &QPushButton::clicked,
                this, &CameraControlPanel::onConnectButtonClicked);
    }
    if (m_controls.disconnectButton) {
        connect(m_controls.disconnectButton, &QPushButton::clicked,
                this, &CameraControlPanel::onDisconnectButtonClicked);
    }
    
    // Acquisition controls
    if (m_controls.startButton) {
        connect(m_controls.startButton, &QPushButton::clicked,
                this, &CameraControlPanel::onStartButtonClicked);
    }
    if (m_controls.stopButton) {
        connect(m_controls.stopButton, &QPushButton::clicked,
                this, &CameraControlPanel::onStopButtonClicked);
    }
    if (m_controls.singleFrameButton) {
        connect(m_controls.singleFrameButton, &QPushButton::clicked,
                this, &CameraControlPanel::onSingleFrameButtonClicked);
    }
    if (m_controls.triggerButton) {
        connect(m_controls.triggerButton, &QPushButton::clicked,
                this, [this]() { emit requestSoftwareTrigger(); });
    }
    
    // Parameter controls - synchronized slider and spinbox
    if (m_controls.exposureSlider && m_controls.exposureSpinBox) {
        connect(m_controls.exposureSlider, &QSlider::valueChanged,
                [this](int value) {
                    if (!m_controls.exposureSpinBox->hasFocus()) {
                        m_controls.exposureSpinBox->setValue(static_cast<double>(value));
                    }
                });
        connect(m_controls.exposureSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                this, &CameraControlPanel::onExposureValueChanged);
    }
    
    if (m_controls.gainSlider && m_controls.gainSpinBox) {
        connect(m_controls.gainSlider, &QSlider::valueChanged,
                [this](int value) {
                    if (!m_controls.gainSpinBox->hasFocus()) {
                        m_controls.gainSpinBox->setValue(value / 10.0);
                    }
                });
        connect(m_controls.gainSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                this, &CameraControlPanel::onGainValueChanged);
    }
    
    if (m_controls.frameRateSpinBox) {
        connect(m_controls.frameRateSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                this, &CameraControlPanel::onFrameRateValueChanged);
    }
    
    if (m_controls.triggerModeCombo) {
        connect(m_controls.triggerModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &CameraControlPanel::onTriggerModeSelected);
    }
    
    if (m_controls.pixelFormatCombo) {
        connect(m_controls.pixelFormatCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &CameraControlPanel::onPixelFormatSelected);
    }
    
    // Auto features
    if (m_controls.autoExposureCheck) {
        connect(m_controls.autoExposureCheck, &QCheckBox::toggled,
                this, &CameraControlPanel::onAutoExposureToggled);
    }
    if (m_controls.autoGainCheck) {
        connect(m_controls.autoGainCheck, &QCheckBox::toggled,
                this, &CameraControlPanel::onAutoGainToggled);
    }
    
    // ROI controls
    if (m_controls.roiSetButton) {
        connect(m_controls.roiSetButton, &QPushButton::clicked,
                [this]() {
                    QRect roi(
                        m_controls.roiX->value(),
                        m_controls.roiY->value(),
                        m_controls.roiWidth->value(),
                        m_controls.roiHeight->value()
                    );
                    emit requestSetROI(roi);
                });
    }
    if (m_controls.roiResetButton) {
        connect(m_controls.roiResetButton, &QPushButton::clicked,
                [this]() {
                    emit requestSetROI(QRect()); // Empty rect = full sensor
                });
    }
}

void CameraControlPanel::connectComponentSignals(QObject* component)
{
    if (!component) return;
    
    // Connect to camera component signals using string-based connections
    // This maintains complete decoupling - we only know signal names
    
    // Device updates
    connect(component, SIGNAL(deviceListUpdated(QJsonArray)),
            this, SLOT(onDeviceListUpdated(QJsonArray)));
    connect(component, SIGNAL(deviceConnected(QString)),
            this, SLOT(onDeviceConnected(QString)));
    connect(component, SIGNAL(deviceDisconnected()),
            this, SLOT(onDeviceDisconnected()));
    
    // State updates
    connect(component, SIGNAL(cameraStateChanged(int,int)),
            this, SLOT(onCameraStateChanged(int)));
    connect(component, SIGNAL(acquisitionStateChanged(bool)),
            this, SLOT(onAcquisitionStateChanged(bool)));
    
    // Parameter updates
    connect(component, SIGNAL(exposureTimeChanged(double)),
            this, SLOT(onExposureChanged(double)));
    connect(component, SIGNAL(gainChanged(double)),
            this, SLOT(onGainChanged(double)));
    connect(component, SIGNAL(frameRateChanged(double)),
            this, SLOT(onFrameRateChanged(double)));
    connect(component, SIGNAL(roiChanged(QRect)),
            this, SLOT(onROIChanged(QRect)));
    connect(component, SIGNAL(triggerModeChanged(int)),
            this, SLOT(onTriggerModeChanged(int)));
    connect(component, SIGNAL(pixelFormatChanged(int)),
            this, SLOT(onPixelFormatChanged(int)));
    
    // Image updates
    connect(component, SIGNAL(frameReady(QImage,qint64)),
            this, SLOT(onImageReceived(QImage,qint64)));
    connect(component, SIGNAL(frameDropped(qint64)),
            this, SLOT(onFrameDropped(qint64)));
    
    // Performance updates
    connect(component, SIGNAL(fpsUpdated(double)),
            this, SLOT(onFpsUpdated(double)));
    connect(component, SIGNAL(statisticsUpdated(QJsonObject)),
            this, SLOT(onStatisticsUpdated(QJsonObject)));
    
    // Error handling
    connect(component, SIGNAL(cameraError(QString)),
            this, SLOT(onErrorOccurred(QString)));
    connect(component, SIGNAL(cameraWarning(QString)),
            this, SLOT(onWarningOccurred(QString)));
}

void CameraControlPanel::disconnectComponentSignals(QObject* component)
{
    if (!component) return;
    
    // Disconnect all signals from component to this panel
    disconnect(component, nullptr, this, nullptr);
}

// ========== UI Update Methods ==========

void CameraControlPanel::updateUIState(bool connected, bool acquiring)
{
    // Connection controls
    if (m_controls.connectButton) {
        m_controls.connectButton->setEnabled(!connected);
    }
    if (m_controls.disconnectButton) {
        m_controls.disconnectButton->setEnabled(connected);
    }
    if (m_controls.deviceCombo) {
        m_controls.deviceCombo->setEnabled(!connected);
    }
    
    // Acquisition controls
    if (m_controls.startButton) {
        m_controls.startButton->setEnabled(connected && !acquiring);
    }
    if (m_controls.stopButton) {
        m_controls.stopButton->setEnabled(connected && acquiring);
    }
    if (m_controls.singleFrameButton) {
        m_controls.singleFrameButton->setEnabled(connected && !acquiring);
    }
    if (m_controls.triggerButton) {
        m_controls.triggerButton->setEnabled(connected && acquiring);
    }
    
    // Parameter controls - enabled when connected
    bool paramEnabled = connected && !acquiring;
    if (m_controls.exposureSlider) {
        m_controls.exposureSlider->setEnabled(paramEnabled);
    }
    if (m_controls.exposureSpinBox) {
        m_controls.exposureSpinBox->setEnabled(paramEnabled);
    }
    if (m_controls.gainSlider) {
        m_controls.gainSlider->setEnabled(paramEnabled);
    }
    if (m_controls.gainSpinBox) {
        m_controls.gainSpinBox->setEnabled(paramEnabled);
    }
    if (m_controls.frameRateSpinBox) {
        m_controls.frameRateSpinBox->setEnabled(paramEnabled);
    }
    if (m_controls.triggerModeCombo) {
        m_controls.triggerModeCombo->setEnabled(paramEnabled);
    }
    if (m_controls.pixelFormatCombo) {
        m_controls.pixelFormatCombo->setEnabled(paramEnabled);
    }
}

void CameraControlPanel::updateDeviceList(const QJsonArray& devices)
{
    if (!m_controls.deviceCombo) return;
    
    m_controls.deviceCombo->clear();
    
    for (const auto& value : devices) {
        QJsonObject device = value.toObject();
        QString displayText = QString("%1 - %2")
            .arg(device["modelName"].toString())
            .arg(device["serialNumber"].toString());
        QString serialNumber = device["serialNumber"].toString();
        
        m_controls.deviceCombo->addItem(displayText, serialNumber);
    }
}

void CameraControlPanel::updateParameterDisplays()
{
    // Update all parameter displays with current values
    // This is called when switching between cameras or loading settings
}

void CameraControlPanel::showError(const QString& message) const
{
    QMessageBox::critical(const_cast<CameraControlPanel*>(this), tr("Camera Error"), message);
    
    const_cast<CameraControlPanel*>(this)->emit userMessage(message, 2); // Severity 2 = Error
}

void CameraControlPanel::showInfo(const QString& message) const
{
    // Show in status bar or info area instead of dialog
    if (m_controls.connectionStatus) {
        m_controls.connectionStatus->setText(message);
        
        // Reset status text after 3 seconds
        QTimer::singleShot(3000, [this]() {
            if (m_state.cameraConnected) {
                m_controls.connectionStatus->setText(tr("Connected: %1").arg(m_state.currentDevice));
            } else {
                m_controls.connectionStatus->setText(tr("Disconnected"));
            }
        });
    }
    
    const_cast<CameraControlPanel*>(this)->emit userMessage(message, 0); // Severity 0 = Info
}

// ========== Internal UI Event Handlers ==========

void CameraControlPanel::onScanButtonClicked()
{
    emit requestScanDevices();
    showInfo(tr("Scanning for devices..."));
}

void CameraControlPanel::onConnectButtonClicked()
{
    if (!m_controls.deviceCombo || m_controls.deviceCombo->count() == 0) {
        showError(tr("No devices available. Please scan first."));
        return;
    }
    
    QString deviceId = m_controls.deviceCombo->currentData().toString();
    emit requestConnect(deviceId);
}

void CameraControlPanel::onDisconnectButtonClicked()
{
    emit requestDisconnect();
}

void CameraControlPanel::onStartButtonClicked()
{
    emit requestStartAcquisition();
}

void CameraControlPanel::onStopButtonClicked()
{
    emit requestStopAcquisition();
}

void CameraControlPanel::onSingleFrameButtonClicked()
{
    emit requestSingleFrame();
}

void CameraControlPanel::onExposureValueChanged(double value)
{
    if (m_controls.exposureSlider && !m_controls.exposureSlider->isSliderDown()) {
        m_controls.exposureSlider->setValue(static_cast<int>(value));
    }
    emit requestSetExposure(value);
}

void CameraControlPanel::onGainValueChanged(double value)
{
    if (m_controls.gainSlider && !m_controls.gainSlider->isSliderDown()) {
        m_controls.gainSlider->setValue(static_cast<int>(value * 10));
    }
    emit requestSetGain(value);
}

void CameraControlPanel::onFrameRateValueChanged(double value)
{
    emit requestSetFrameRate(value);
}

void CameraControlPanel::onTriggerModeSelected(int index)
{
    emit requestSetTriggerMode(index);
}

void CameraControlPanel::onPixelFormatSelected(int index)
{
    emit requestSetPixelFormat(index);
}

void CameraControlPanel::onAutoExposureToggled(bool checked)
{
    emit requestAutoExposure(checked);
    
    // Disable manual exposure controls when auto is enabled
    if (m_controls.exposureSlider) {
        m_controls.exposureSlider->setEnabled(!checked);
    }
    if (m_controls.exposureSpinBox) {
        m_controls.exposureSpinBox->setEnabled(!checked);
    }
}

void CameraControlPanel::onAutoGainToggled(bool checked)
{
    emit requestAutoGain(checked);
    
    // Disable manual gain controls when auto is enabled
    if (m_controls.gainSlider) {
        m_controls.gainSlider->setEnabled(!checked);
    }
    if (m_controls.gainSpinBox) {
        m_controls.gainSpinBox->setEnabled(!checked);
    }
}

void CameraControlPanel::onSaveImageClicked()
{
    if (m_display.currentImage.isNull()) {
        showError(tr("No image to save"));
        return;
    }
    
    QString fileName = QFileDialog::getSaveFileName(
        this,
        tr("Save Image"),
        QString(),
        tr("Images (*.png *.jpg *.bmp)")
    );
    
    if (!fileName.isEmpty()) {
        emit requestSaveImage(fileName);
    }
}

void CameraControlPanel::onLoadSettingsClicked()
{
    QString fileName = QFileDialog::getOpenFileName(
        this,
        tr("Load Settings"),
        QString(),
        tr("JSON Files (*.json)")
    );
    
    if (!fileName.isEmpty()) {
        emit requestLoadConfiguration(fileName);
    }
}

void CameraControlPanel::onSaveSettingsClicked()
{
    QString fileName = QFileDialog::getSaveFileName(
        this,
        tr("Save Settings"),
        QString(),
        tr("JSON Files (*.json)")
    );
    
    if (!fileName.isEmpty()) {
        emit requestSaveConfiguration(fileName);
    }
}

void CameraControlPanel::updateStatistics()
{
    // Calculate average FPS
    if (m_statistics.frameCount > 0) {
        qint64 elapsedTime = QDateTime::currentMSecsSinceEpoch() - m_statistics.lastFrameTime;
        if (elapsedTime > 0) {
            m_statistics.averageFps = m_statistics.frameCount * 1000.0 / elapsedTime;
        }
    }
    
    // Update displays
    if (m_controls.fpsLabel) {
        m_controls.fpsLabel->setText(formatFrameRate(m_statistics.currentFps));
    }
}

void CameraControlPanel::updateImageDisplay()
{
    if (!m_controls.imageDisplay || m_display.currentImage.isNull()) {
        return;
    }
    
    QImage displayImage = m_display.currentImage;
    
    // Apply zoom if not auto-scaling
    if (!m_display.autoScale && m_display.zoomLevel != 1.0) {
        int newWidth = static_cast<int>(displayImage.width() * m_display.zoomLevel);
        int newHeight = static_cast<int>(displayImage.height() * m_display.zoomLevel);
        displayImage = displayImage.scaled(newWidth, newHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    
    // Draw overlays if enabled
    if (m_display.showOverlay || m_display.showCrosshair) {
        displayImage = displayImage.copy(); // Make a copy to draw on
        QPainter painter(&displayImage);
        
        if (m_display.showCrosshair) {
            // Draw crosshair
            painter.setPen(QPen(Qt::red, 1, Qt::DashLine));
            int centerX = displayImage.width() / 2;
            int centerY = displayImage.height() / 2;
            painter.drawLine(centerX, 0, centerX, displayImage.height());
            painter.drawLine(0, centerY, displayImage.width(), centerY);
        }
        
        if (m_display.showOverlay) {
            // Draw frame info overlay
            painter.setPen(Qt::green);
            painter.setFont(QFont("Arial", 10));
            QString info = QString("Frame: %1 | FPS: %2")
                .arg(m_statistics.frameCount)
                .arg(m_statistics.currentFps, 0, 'f', 1);
            painter.drawText(10, 20, info);
        }
    }
    
    // Update the display
    m_controls.imageDisplay->setPixmap(QPixmap::fromImage(displayImage));
}

// ========== Private Helper Methods ==========

void CameraControlPanel::setupDefaultUI()
{
    // Clear any existing layout
    if (m_mainLayout) {
        QLayoutItem* item;
        while ((item = m_mainLayout->takeAt(0)) != nullptr) {
            delete item->widget();
            delete item;
        }
        delete m_mainLayout;
    }
    
    // Create main layout
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(5, 5, 5, 5);
}

void CameraControlPanel::createStandardLayout()
{
    setupDefaultUI();
    
    // Top section: Connection and Acquisition side by side
    QHBoxLayout* topSection = new QHBoxLayout;
    topSection->addWidget(createConnectionControls());
    topSection->addWidget(createAcquisitionControls());
    m_mainLayout->addLayout(topSection);
    
    // Middle section: Parameters and Statistics
    QHBoxLayout* middleSection = new QHBoxLayout;
    middleSection->addWidget(createBasicParameterControls(), 2);
    middleSection->addWidget(createStatisticsDisplay(), 1);
    m_mainLayout->addLayout(middleSection);
    
    // Advanced controls (initially hidden)
    QWidget* advancedWidget = createAdvancedParameterControls();
    advancedWidget->setVisible(m_state.showAdvanced);
    m_mainLayout->addWidget(advancedWidget);
    
    // Bottom section: Image display
    if (m_state.displayMode != ImageDisplayMode::None) {
        m_mainLayout->addWidget(createImageDisplay(), 1);
    }
    
    // Re-connect internal signals after UI recreation
    connectInternalSignals();
}

void CameraControlPanel::createCompactLayout()
{
    setupDefaultUI();
    
    // Compact layout uses tabs to save space
    QTabWidget* tabs = new QTabWidget;
    
    // Connection tab
    tabs->addTab(createConnectionControls(), tr("Connection"));
    
    // Control tab (combines acquisition and basic parameters)
    QWidget* controlTab = new QWidget;
    QVBoxLayout* controlLayout = new QVBoxLayout(controlTab);
    controlLayout->addWidget(createAcquisitionControls());
    controlLayout->addWidget(createBasicParameterControls());
    controlLayout->addStretch();
    tabs->addTab(controlTab, tr("Controls"));
    
    // Statistics tab
    tabs->addTab(createStatisticsDisplay(), tr("Statistics"));
    
    m_mainLayout->addWidget(tabs);
    
    // Small image display at bottom
    if (m_state.displayMode == ImageDisplayMode::Embedded) {
        QWidget* imageWidget = createImageDisplay();
        if (imageWidget && m_controls.imageDisplay) {
            m_controls.imageDisplay->setMaximumHeight(200);
        }
        m_mainLayout->addWidget(imageWidget);
    }
    
    // Re-connect internal signals after UI recreation
    connectInternalSignals();
}

void CameraControlPanel::createAdvancedLayout()
{
    setupDefaultUI();
    
    // Advanced layout shows everything
    QHBoxLayout* mainHLayout = new QHBoxLayout;
    
    // Left side: Controls
    QVBoxLayout* leftLayout = new QVBoxLayout;
    leftLayout->addWidget(createConnectionControls());
    leftLayout->addWidget(createAcquisitionControls());
    leftLayout->addWidget(createBasicParameterControls());
    leftLayout->addWidget(createAdvancedParameterControls());
    leftLayout->addWidget(createStatisticsDisplay());
    leftLayout->addStretch();
    
    // Right side: Image display (larger)
    QWidget* imageWidget = createImageDisplay();
    
    mainHLayout->addLayout(leftLayout, 1);
    if (imageWidget) {
        mainHLayout->addWidget(imageWidget, 2);
    }
    
    m_mainLayout->addLayout(mainHLayout);
    
    // Re-connect internal signals after UI recreation
    connectInternalSignals();
}

QString CameraControlPanel::formatExposureTime(double microseconds) const
{
    if (microseconds < 1000) {
        return QString("%1 μs").arg(microseconds, 0, 'f', 1);
    } else if (microseconds < 1000000) {
        return QString("%1 ms").arg(microseconds / 1000.0, 0, 'f', 2);
    } else {
        return QString("%1 s").arg(microseconds / 1000000.0, 0, 'f', 3);
    }
}

QString CameraControlPanel::formatFrameRate(double fps) const
{
    return QString("FPS: %1").arg(fps, 0, 'f', 1);
}

} // namespace ComponentsForest