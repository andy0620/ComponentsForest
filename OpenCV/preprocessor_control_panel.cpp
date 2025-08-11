#include "preprocessor_control_panel.h"
#include "preprocessor_base.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QTextEdit>
#include <QSlider>
#include <QTabWidget>
#include <QTimer>
#include <QDateTime>
#include <QMetaObject>
#include <QScrollArea>
#include <QSplitter>

namespace ComponentsForest {
namespace OpenCV {

// Private implementation class
class PreProcessorControlPanel::Private {
public:
    Private(PreProcessorControlPanel* q) 
        : q(q)
        , preprocessor(nullptr)
        , isConnected(false)
        , isProcessing(false)
        , compactMode(false)
        , showAdvanced(false)
    {
        // Initialize update timer
        updateTimer = new QTimer(q);
        updateTimer->setInterval(100); // 10Hz UI update rate
        
        // Initialize statistics
        lastFps = 0.0;
        framesProcessed = 0;
        framesDropped = 0;
    }
    
    ~Private() {
        if (updateTimer) {
            updateTimer->stop();
            delete updateTimer;
        }
    }
    
    PreProcessorControlPanel* q;
    
    // Connected preprocessor
    PreProcessorBase* preprocessor;
    bool isConnected;
    bool isProcessing;
    
    // Panel identification
    QString panelId;
    
    // UI mode
    bool compactMode;
    bool showAdvanced;
    
    // Control widgets
    QPushButton* btnStart = nullptr;
    QPushButton* btnStop = nullptr;
    QPushButton* btnPause = nullptr;
    QPushButton* btnResume = nullptr;
    QPushButton* btnClear = nullptr;
    
    // Configuration widgets
    QComboBox* cmbProcessingMode = nullptr;
    QCheckBox* chkGPU = nullptr;
    QCheckBox* chkDebugMode = nullptr;
    QSpinBox* spnMaxQueueSize = nullptr;
    QPushButton* btnApplyConfig = nullptr;
    QPushButton* btnResetConfig = nullptr;
    
    // Monitoring widgets
    QLabel* lblStatus = nullptr;
    QLabel* lblFPS = nullptr;
    QLabel* lblFramesProcessed = nullptr;
    QLabel* lblFramesDropped = nullptr;
    QLabel* lblBufferUsage = nullptr;
    QLabel* lblCPUUsage = nullptr;
    QLabel* lblGPUUsage = nullptr;
    QLabel* lblMemoryUsage = nullptr;
    QTextEdit* txtLog = nullptr;
    
    // Advanced controls
    QTabWidget* tabAdvanced = nullptr;
    QGroupBox* grpAdvanced = nullptr;
    
    // Update timer
    QTimer* updateTimer;
    
    // Current statistics
    double lastFps;
    qint64 framesProcessed;
    qint64 framesDropped;
    int bufferUsed;
    int bufferTotal;
    
    // Configuration
    QVariantMap currentConfig;
};

// Constructor
PreProcessorControlPanel::PreProcessorControlPanel(QWidget* parent)
    : QWidget(parent)
    , d(std::make_unique<Private>(this))
{
    setupUI();
    connectInternalSignals();
    
    // Set default panel ID
    d->panelId = QString("PreProcessorPanel_%1").arg(reinterpret_cast<quintptr>(this));
}

// Destructor
PreProcessorControlPanel::~PreProcessorControlPanel() {
    disconnectFromPreprocessor();
}

// Connect to preprocessor component
void PreProcessorControlPanel::connectToPreprocessor(PreProcessorBase* preprocessor) {
    if (!preprocessor) {
        return;
    }
    
    // Disconnect from previous preprocessor if any
    if (d->preprocessor) {
        disconnectFromPreprocessor();
    }
    
    d->preprocessor = preprocessor;
    
    // Connect signals using string-based connections for decoupling
    // Control signals
    connect(this, SIGNAL(requestStartProcessing()),
            preprocessor, SLOT(startProcessing()));
    connect(this, SIGNAL(requestStopProcessing()),
            preprocessor, SLOT(stopProcessing()));
    connect(this, SIGNAL(requestPauseProcessing()),
            preprocessor, SLOT(pauseProcessing()));
    connect(this, SIGNAL(requestResumeProcessing()),
            preprocessor, SLOT(stopProcessing())); // Resume uses stop/start pattern
    connect(this, SIGNAL(requestFlushBuffers()),
            preprocessor, SLOT(flushBuffers()));
    
    // Configuration signals
    connect(this, SIGNAL(requestConfigurationUpdate(QVariantMap)),
            preprocessor, SLOT(updateConfiguration(QVariantMap)));
    connect(this, SIGNAL(requestGPUEnable(bool)),
            preprocessor, SLOT(enableGPU(bool)));
    
    // Receive preprocessor signals
    connect(preprocessor, SIGNAL(preprocessingStarted()),
            this, SLOT(onProcessingStarted()));
    connect(preprocessor, SIGNAL(preprocessingStopped()),
            this, SLOT(onProcessingStopped()));
    connect(preprocessor, SIGNAL(preprocessingPaused()),
            this, SLOT(onProcessingPaused()));
    connect(preprocessor, SIGNAL(bufferStatus(int,int)),
            this, SLOT(onBufferStatus(int,int)));
    connect(preprocessor, SIGNAL(stateChanged(int)),
            this, SLOT(onStateChanged(int)));
    
    d->isConnected = true;
    updateControlsState();
    
    // Start update timer
    d->updateTimer->start();
    
    emit panelConnected(d->panelId);
}

// Disconnect from preprocessor
void PreProcessorControlPanel::disconnectFromPreprocessor() {
    if (!d->preprocessor) {
        return;
    }
    
    // Stop update timer
    d->updateTimer->stop();
    
    // Disconnect all signals
    disconnect(d->preprocessor, nullptr, this, nullptr);
    disconnect(this, nullptr, d->preprocessor, nullptr);
    
    d->preprocessor = nullptr;
    d->isConnected = false;
    d->isProcessing = false;
    
    updateControlsState();
    
    emit panelDisconnected(d->panelId);
}

// Panel identification
QString PreProcessorControlPanel::getPanelId() const {
    return d->panelId;
}

void PreProcessorControlPanel::setPanelId(const QString& id) {
    d->panelId = id;
}

// Panel state
bool PreProcessorControlPanel::isConnected() const {
    return d->isConnected;
}

bool PreProcessorControlPanel::isProcessing() const {
    return d->isProcessing;
}

// Configuration
void PreProcessorControlPanel::setConfiguration(const QVariantMap& config) {
    d->currentConfig = config;
    if (d->preprocessor) {
        emit requestConfigurationUpdate(config);
    }
}

QVariantMap PreProcessorControlPanel::getConfiguration() const {
    return d->currentConfig;
}

// UI customization
void PreProcessorControlPanel::setCompactMode(bool compact) {
    d->compactMode = compact;
    if (d->grpAdvanced) {
        d->grpAdvanced->setVisible(!compact && d->showAdvanced);
    }
}

bool PreProcessorControlPanel::isCompactMode() const {
    return d->compactMode;
}

void PreProcessorControlPanel::setShowAdvancedControls(bool show) {
    d->showAdvanced = show;
    if (d->grpAdvanced) {
        d->grpAdvanced->setVisible(show && !d->compactMode);
    }
}

// Control slots
void PreProcessorControlPanel::startProcessing() {
    emit requestStartProcessing();
}

void PreProcessorControlPanel::stopProcessing() {
    emit requestStopProcessing();
}

void PreProcessorControlPanel::pauseProcessing() {
    emit requestPauseProcessing();
}

void PreProcessorControlPanel::resumeProcessing() {
    emit requestResumeProcessing();
}

void PreProcessorControlPanel::clearBuffers() {
    emit requestFlushBuffers();
}

// Configuration slots
void PreProcessorControlPanel::updateConfiguration() {
    if (d->preprocessor) {
        emit requestConfigurationUpdate(d->currentConfig);
    }
}

void PreProcessorControlPanel::resetToDefaults() {
    // Reset to default configuration
    QVariantMap defaultConfig;
    defaultConfig["processingMode"] = 0; // RealTime
    defaultConfig["gpuEnabled"] = false;
    defaultConfig["maxQueueSize"] = 10;
    defaultConfig["debugMode"] = false;
    
    setConfiguration(defaultConfig);
    
    // Update UI
    if (d->cmbProcessingMode) d->cmbProcessingMode->setCurrentIndex(0);
    if (d->chkGPU) d->chkGPU->setChecked(false);
    if (d->spnMaxQueueSize) d->spnMaxQueueSize->setValue(10);
    if (d->chkDebugMode) d->chkDebugMode->setChecked(false);
}

// Slots for receiving preprocessor signals
void PreProcessorControlPanel::onFrameProcessed(const QImage& frame, const QVariantMap& metadata) {
    d->framesProcessed++;
    updateStatisticsDisplay();
}

void PreProcessorControlPanel::onProcessingError(int error, const QString& details) {
    QString errorMsg = QString("[ERROR] Processing error %1: %2").arg(error).arg(details);
    logMessage(errorMsg, "ERROR");
    emit panelError(errorMsg);
}

void PreProcessorControlPanel::onProcessingStatistics(const QVariantMap& stats) {
    // Update statistics display
    if (stats.contains("fps")) {
        d->lastFps = stats["fps"].toDouble();
    }
    if (stats.contains("totalFramesProcessed")) {
        d->framesProcessed = stats["totalFramesProcessed"].toLongLong();
    }
    if (stats.contains("framesDropped")) {
        d->framesDropped = stats["framesDropped"].toLongLong();
    }
    
    updateStatisticsDisplay();
}

void PreProcessorControlPanel::onProcessingPerformanceUpdate(const QVariantMap& metrics) {
    updatePerformanceDisplay();
}

void PreProcessorControlPanel::onBufferStatus(int used, int total) {
    d->bufferUsed = used;
    d->bufferTotal = total;
    updateBufferDisplay(used, total);
}

void PreProcessorControlPanel::onProcessingStarted() {
    d->isProcessing = true;
    updateControlsState();
    logMessage("Processing started", "INFO");
}

void PreProcessorControlPanel::onProcessingStopped() {
    d->isProcessing = false;
    updateControlsState();
    logMessage("Processing stopped", "INFO");
}

void PreProcessorControlPanel::onProcessingPaused() {
    d->isProcessing = false; // Treat pause as stopped for UI purposes
    updateControlsState();
    logMessage("Processing paused", "INFO");
}

void PreProcessorControlPanel::onStateChanged(int state) {
    QString stateStr = QString::number(state);
    if (d->lblStatus) {
        d->lblStatus->setText(QString("State: %1").arg(stateStr));
    }
}

// Setup UI
void PreProcessorControlPanel::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    
    // Create sections
    QWidget* controlSection = createControlSection();
    QWidget* configSection = createConfigurationSection();
    QWidget* monitorSection = createMonitoringSection();
    QWidget* advancedSection = createAdvancedSection();
    
    // Add sections to layout
    mainLayout->addWidget(controlSection);
    mainLayout->addWidget(configSection);
    mainLayout->addWidget(monitorSection);
    
    if (advancedSection) {
        d->grpAdvanced = qobject_cast<QGroupBox*>(advancedSection);
        mainLayout->addWidget(advancedSection);
        advancedSection->setVisible(d->showAdvanced && !d->compactMode);
    }
    
    // Add stretch at bottom
    mainLayout->addStretch();
    
    // Set initial state
    updateControlsState();
}

// Create control section
QWidget* PreProcessorControlPanel::createControlSection() {
    QGroupBox* group = new QGroupBox("Processing Control", this);
    QHBoxLayout* layout = new QHBoxLayout(group);
    
    d->btnStart = new QPushButton("Start", group);
    d->btnStop = new QPushButton("Stop", group);
    d->btnPause = new QPushButton("Pause", group);
    d->btnResume = new QPushButton("Resume", group);
    d->btnClear = new QPushButton("Clear Buffers", group);
    
    layout->addWidget(d->btnStart);
    layout->addWidget(d->btnStop);
    layout->addWidget(d->btnPause);
    layout->addWidget(d->btnResume);
    layout->addWidget(d->btnClear);
    layout->addStretch();
    
    // Connect buttons
    connect(d->btnStart, &QPushButton::clicked, this, &PreProcessorControlPanel::onStartButtonClicked);
    connect(d->btnStop, &QPushButton::clicked, this, &PreProcessorControlPanel::onStopButtonClicked);
    connect(d->btnPause, &QPushButton::clicked, this, &PreProcessorControlPanel::onPauseButtonClicked);
    connect(d->btnResume, &QPushButton::clicked, this, &PreProcessorControlPanel::resumeProcessing);
    connect(d->btnClear, &QPushButton::clicked, this, &PreProcessorControlPanel::clearBuffers);
    
    return group;
}

// Create configuration section
QWidget* PreProcessorControlPanel::createConfigurationSection() {
    QGroupBox* group = new QGroupBox("Configuration", this);
    QGridLayout* layout = new QGridLayout(group);
    
    // Processing mode
    layout->addWidget(new QLabel("Processing Mode:", group), 0, 0);
    d->cmbProcessingMode = new QComboBox(group);
    d->cmbProcessingMode->addItems(QStringList() << "Real-Time" << "High Quality" << "Balanced");
    layout->addWidget(d->cmbProcessingMode, 0, 1);
    
    // GPU enable
    d->chkGPU = new QCheckBox("Enable GPU", group);
    layout->addWidget(d->chkGPU, 1, 0, 1, 2);
    
    // Max queue size
    layout->addWidget(new QLabel("Max Queue Size:", group), 2, 0);
    d->spnMaxQueueSize = new QSpinBox(group);
    d->spnMaxQueueSize->setRange(1, 100);
    d->spnMaxQueueSize->setValue(10);
    layout->addWidget(d->spnMaxQueueSize, 2, 1);
    
    // Debug mode
    d->chkDebugMode = new QCheckBox("Debug Mode", group);
    layout->addWidget(d->chkDebugMode, 3, 0, 1, 2);
    
    // Apply/Reset buttons
    QHBoxLayout* btnLayout = new QHBoxLayout();
    d->btnApplyConfig = new QPushButton("Apply", group);
    d->btnResetConfig = new QPushButton("Reset", group);
    btnLayout->addWidget(d->btnApplyConfig);
    btnLayout->addWidget(d->btnResetConfig);
    btnLayout->addStretch();
    layout->addLayout(btnLayout, 4, 0, 1, 2);
    
    // Connect configuration widgets
    connect(d->cmbProcessingMode, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PreProcessorControlPanel::onProcessingModeChanged);
    connect(d->chkGPU, &QCheckBox::toggled, this, &PreProcessorControlPanel::onGPUCheckboxToggled);
    connect(d->spnMaxQueueSize, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &PreProcessorControlPanel::onMaxQueueSizeChanged);
    connect(d->chkDebugMode, &QCheckBox::toggled, this, &PreProcessorControlPanel::onDebugModeToggled);
    connect(d->btnApplyConfig, &QPushButton::clicked, this, &PreProcessorControlPanel::onApplyConfigurationClicked);
    connect(d->btnResetConfig, &QPushButton::clicked, this, &PreProcessorControlPanel::onResetConfigurationClicked);
    
    return group;
}

// Create monitoring section
QWidget* PreProcessorControlPanel::createMonitoringSection() {
    QGroupBox* group = new QGroupBox("Monitoring", this);
    QGridLayout* layout = new QGridLayout(group);
    
    // Status
    layout->addWidget(new QLabel("Status:", group), 0, 0);
    d->lblStatus = new QLabel("Disconnected", group);
    layout->addWidget(d->lblStatus, 0, 1);
    
    // FPS
    layout->addWidget(new QLabel("FPS:", group), 1, 0);
    d->lblFPS = new QLabel("0.0", group);
    layout->addWidget(d->lblFPS, 1, 1);
    
    // Frames processed
    layout->addWidget(new QLabel("Frames Processed:", group), 2, 0);
    d->lblFramesProcessed = new QLabel("0", group);
    layout->addWidget(d->lblFramesProcessed, 2, 1);
    
    // Frames dropped
    layout->addWidget(new QLabel("Frames Dropped:", group), 3, 0);
    d->lblFramesDropped = new QLabel("0", group);
    layout->addWidget(d->lblFramesDropped, 3, 1);
    
    // Buffer usage
    layout->addWidget(new QLabel("Buffer Usage:", group), 4, 0);
    d->lblBufferUsage = new QLabel("0/0", group);
    layout->addWidget(d->lblBufferUsage, 4, 1);
    
    // Performance metrics
    layout->addWidget(new QLabel("CPU Usage:", group), 5, 0);
    d->lblCPUUsage = new QLabel("0%", group);
    layout->addWidget(d->lblCPUUsage, 5, 1);
    
    layout->addWidget(new QLabel("GPU Usage:", group), 6, 0);
    d->lblGPUUsage = new QLabel("0%", group);
    layout->addWidget(d->lblGPUUsage, 6, 1);
    
    layout->addWidget(new QLabel("Memory:", group), 7, 0);
    d->lblMemoryUsage = new QLabel("0 MB", group);
    layout->addWidget(d->lblMemoryUsage, 7, 1);
    
    layout->setColumnStretch(1, 1);
    
    return group;
}

// Create advanced section
QWidget* PreProcessorControlPanel::createAdvancedSection() {
    QGroupBox* group = new QGroupBox("Advanced", this);
    QVBoxLayout* layout = new QVBoxLayout(group);
    
    // Create log text area
    d->txtLog = new QTextEdit(group);
    d->txtLog->setReadOnly(true);
    d->txtLog->setMaximumHeight(150);
    d->txtLog->setFont(QFont("Consolas", 9));
    
    layout->addWidget(new QLabel("Processing Log:", group));
    layout->addWidget(d->txtLog);
    
    return group;
}

// Update controls state
void PreProcessorControlPanel::updateControlsState() {
    bool connected = d->isConnected;
    bool processing = d->isProcessing;
    
    if (d->btnStart) d->btnStart->setEnabled(connected && !processing);
    if (d->btnStop) d->btnStop->setEnabled(connected && processing);
    if (d->btnPause) d->btnPause->setEnabled(connected && processing);
    if (d->btnResume) d->btnResume->setEnabled(connected && !processing);
    if (d->btnClear) d->btnClear->setEnabled(connected);
    
    if (d->btnApplyConfig) d->btnApplyConfig->setEnabled(connected);
    if (d->cmbProcessingMode) d->cmbProcessingMode->setEnabled(connected && !processing);
    if (d->chkGPU) d->chkGPU->setEnabled(connected && !processing);
    if (d->spnMaxQueueSize) d->spnMaxQueueSize->setEnabled(connected && !processing);
    
    if (d->lblStatus) {
        QString status = !connected ? "Disconnected" : 
                        (processing ? "Processing" : "Ready");
        d->lblStatus->setText(status);
    }
}

// Update statistics display
void PreProcessorControlPanel::updateStatisticsDisplay() {
    if (d->lblFPS) {
        d->lblFPS->setText(QString::number(d->lastFps, 'f', 1));
    }
    if (d->lblFramesProcessed) {
        d->lblFramesProcessed->setText(QString::number(d->framesProcessed));
    }
    if (d->lblFramesDropped) {
        d->lblFramesDropped->setText(QString::number(d->framesDropped));
    }
}

// Update performance display
void PreProcessorControlPanel::updatePerformanceDisplay() {
    // This would be updated based on actual performance metrics
    // For now, just placeholder
}

// Update buffer display
void PreProcessorControlPanel::updateBufferDisplay(int used, int total) {
    if (d->lblBufferUsage) {
        d->lblBufferUsage->setText(QString("%1/%2").arg(used).arg(total));
        
        // Change color based on usage
        if (total > 0) {
            double usage = (double)used / total;
            if (usage > 0.9) {
                d->lblBufferUsage->setStyleSheet("color: red;");
            } else if (usage > 0.7) {
                d->lblBufferUsage->setStyleSheet("color: orange;");
            } else {
                d->lblBufferUsage->setStyleSheet("color: green;");
            }
        }
    }
}

// Event handlers
void PreProcessorControlPanel::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
    if (d->updateTimer && d->isConnected) {
        d->updateTimer->start();
    }
}

void PreProcessorControlPanel::hideEvent(QHideEvent* event) {
    QWidget::hideEvent(event);
    if (d->updateTimer) {
        d->updateTimer->stop();
    }
}

// Internal UI slots
void PreProcessorControlPanel::onStartButtonClicked() {
    startProcessing();
}

void PreProcessorControlPanel::onStopButtonClicked() {
    stopProcessing();
}

void PreProcessorControlPanel::onPauseButtonClicked() {
    pauseProcessing();
}

void PreProcessorControlPanel::onGPUCheckboxToggled(bool checked) {
    emit requestGPUEnable(checked);
    d->currentConfig["gpuEnabled"] = checked;
}

void PreProcessorControlPanel::onProcessingModeChanged(int index) {
    emit requestProcessingModeChange(index);
    d->currentConfig["processingMode"] = index;
}

void PreProcessorControlPanel::onMaxQueueSizeChanged(int value) {
    emit requestMaxQueueSizeChange(value);
    d->currentConfig["maxQueueSize"] = value;
}

void PreProcessorControlPanel::onDebugModeToggled(bool checked) {
    emit requestDebugModeChange(checked);
    d->currentConfig["debugMode"] = checked;
}

void PreProcessorControlPanel::onApplyConfigurationClicked() {
    updateConfiguration();
    logMessage("Configuration applied", "INFO");
}

void PreProcessorControlPanel::onResetConfigurationClicked() {
    resetToDefaults();
    logMessage("Configuration reset to defaults", "INFO");
}

// Timer slots
void PreProcessorControlPanel::updateDisplayRefresh() {
    // Periodic UI updates
    updateStatisticsDisplay();
}

// Connect internal signals
void PreProcessorControlPanel::connectInternalSignals() {
    connect(d->updateTimer, &QTimer::timeout, 
            this, &PreProcessorControlPanel::updateDisplayRefresh);
}

// Helper methods
void PreProcessorControlPanel::updateButtonStates(bool isProcessing) {
    d->isProcessing = isProcessing;
    updateControlsState();
}

void PreProcessorControlPanel::logMessage(const QString& message, const QString& level) {
    if (!d->txtLog) {
        return;
    }
    
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
    QString formattedMsg = QString("[%1] [%2] %3").arg(timestamp).arg(level).arg(message);
    
    d->txtLog->append(formattedMsg);
    
    // Limit log size
    if (d->txtLog->document()->blockCount() > 100) {
        QTextCursor cursor = d->txtLog->textCursor();
        cursor.movePosition(QTextCursor::Start);
        cursor.select(QTextCursor::LineUnderCursor);
        cursor.removeSelectedText();
        cursor.deleteChar(); // Remove newline
    }
}

QString PreProcessorControlPanel::formatStatistics(const QVariantMap& stats) {
    QString result;
    for (auto it = stats.begin(); it != stats.end(); ++it) {
        result += QString("%1: %2\n").arg(it.key()).arg(it.value().toString());
    }
    return result;
}

QString PreProcessorControlPanel::formatPerformance(const QVariantMap& metrics) {
    QString result;
    for (auto it = metrics.begin(); it != metrics.end(); ++it) {
        result += QString("%1: %2\n").arg(it.key()).arg(it.value().toString());
    }
    return result;
}

// SpecializedPreProcessorControlPanel implementation
SpecializedPreProcessorControlPanel::SpecializedPreProcessorControlPanel(QWidget* parent)
    : PreProcessorControlPanel(parent)
{
}

QWidget* SpecializedPreProcessorControlPanel::createSpecializedControls() {
    // Default implementation - child classes override
    return nullptr;
}

void SpecializedPreProcessorControlPanel::applySpecializedConfiguration(const QVariantMap& config) {
    // Default implementation - child classes override
}

QVariantMap SpecializedPreProcessorControlPanel::getSpecializedConfiguration() const {
    // Default implementation - child classes override
    return QVariantMap();
}

} // namespace OpenCV
} // namespace ComponentsForest