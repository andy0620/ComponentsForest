#ifndef DOTHINK_CAMERA_CONTROL_PANEL_H
#define DOTHINK_CAMERA_CONTROL_PANEL_H

#include "../components/camera_control_panel.h"
#include "do3thinkcameracomponent_export.h"
#include <QChart>
#include <QChartView>
#include <QLineSeries>
#include <QTextEdit>
#include <QTableWidget>
#include <QMatrix3x3>
#include <QCheckBox>
#include <QListWidget>
#include <QListWidgetItem>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QSlider>
#include <QComboBox>
#include <QGroupBox>
#include <QLabel>

namespace ComponentsForest {

// Forward declaration
class Do3ThinkCameraComponent;

/**
 * @brief Do3Think Camera Control Panel
 * Specialized control panel for Do3ThinkCameraComponent
 * Extends CameraControlPanel with Do3Think specific features
 */
class DO3THINKCAMERACOMPONENT_EXPORT Do3ThinkCameraControlPanel : public CameraControlPanel {
    Q_OBJECT
    Q_PROPERTY(bool showDo3ThinkFeatures READ showDo3ThinkFeatures WRITE setShowDo3ThinkFeatures)
    Q_PROPERTY(bool enableAdvancedDiagnostics READ isAdvancedDiagnosticsEnabled WRITE setAdvancedDiagnostics)

public:
    explicit Do3ThinkCameraControlPanel(QWidget* parent = nullptr);
    ~Do3ThinkCameraControlPanel() override;
    
    // ========== Override Virtual Methods from CameraControlPanel ==========
    
    // Component Connection
    void connectToComponent(QObject* cameraComponent) override;
    void disconnectFromComponent() override;
    
    // Panel Configuration
    void setPanelLayout(PanelLayout layout) override;
    bool savePanelConfiguration(const QString& filePath) const override;
    bool loadPanelConfiguration(const QString& filePath) override;
    
    // Theme
    void applyTheme(const QString& themeName) override;
    
    // ========== Do3Think Specific Methods ==========
    
    // Primary Camera Control Interface
    Q_INVOKABLE bool startAcquisition();
    Q_INVOKABLE bool stopAcquisition();
    Q_INVOKABLE bool isAcquiring() const;
    Q_INVOKABLE bool connectCamera();
    Q_INVOKABLE bool disconnectCamera();
    Q_INVOKABLE bool isConnected() const;
    
    // Do3Think Features Control
    Q_INVOKABLE void setShowDo3ThinkFeatures(bool show);
    Q_INVOKABLE bool showDo3ThinkFeatures() const;
    
    // Advanced Diagnostics
    Q_INVOKABLE void setAdvancedDiagnostics(bool enable);
    Q_INVOKABLE bool isAdvancedDiagnosticsEnabled() const;
    
    // Color Correction UI
    Q_INVOKABLE void showColorCorrectionDialog();
    Q_INVOKABLE void applyColorCorrectionPreset(const QString& presetName);
    Q_INVOKABLE QStringList colorCorrectionPresets() const;
    
    // HDR Settings UI
    Q_INVOKABLE void showHDRSettingsDialog();
    Q_INVOKABLE void setHDRVisualization(bool enable);
    
    // GPIO Control UI
    Q_INVOKABLE void showGPIOControlPanel();
    Q_INVOKABLE void setGPIOMonitoring(bool enable);
    
    // Performance Profiling
    Q_INVOKABLE void startPerformanceProfiling();
    Q_INVOKABLE void stopPerformanceProfiling();
    Q_INVOKABLE void exportProfilingResults(const QString& filePath);
    
    // Calibration
    Q_INVOKABLE void startCalibrationWizard();
    Q_INVOKABLE void loadCalibrationFile(const QString& filePath);
    Q_INVOKABLE void saveCalibrationFile(const QString& filePath);
    
    // Image Analysis Tools
    Q_INVOKABLE void enableHistogram(bool enable);
    Q_INVOKABLE void enableWaveform(bool enable);
    Q_INVOKABLE void enableVectorscope(bool enable);
    Q_INVOKABLE void enableFocusPeaking(bool enable);
    Q_INVOKABLE void enableZebraPattern(bool enable);
    
    // Image Processing Controls
    Q_INVOKABLE void enableImageProcessing(bool enable);
    Q_INVOKABLE bool isImageProcessingEnabled() const;
    Q_INVOKABLE void enableBlurPreprocessor(bool enable);
    Q_INVOKABLE void enableEdgePreprocessor(bool enable);
    Q_INVOKABLE void enableDenoisePreprocessor(bool enable);
    Q_INVOKABLE void setBlurKernelSize(int size);
    Q_INVOKABLE void setEdgeThreshold(double threshold);
    Q_INVOKABLE void setDenoiseStrength(double strength);
    Q_INVOKABLE void setProcessingOrder(const QStringList& order);
    Q_INVOKABLE void showProcessingConfiguration();

signals:
    // Primary control signals
    void acquisitionStartRequested();
    void acquisitionStopRequested();
    void connectionRequested();
    void disconnectionRequested();
    void panelClosing();  // Emitted before panel is destroyed
    
    // Do3Think specific signals
    void colorCorrectionRequested(const QMatrix3x3& matrix);
    void hdrSettingsChanged(bool enabled, int levels);
    void gpioControlRequested(int pin, bool value);
    void calibrationCompleted(const QJsonObject& calibrationData);
    void profilingDataReady(const QJsonObject& data);
    void do3ThinkFeatureToggled(const QString& feature, bool enabled);
    
    // Image processing signals
    void imageProcessingEnabled(bool enabled);
    void preprocessorEnabled(const QString& type, bool enabled);
    void preprocessorParameterChanged(const QString& type, const QString& parameter, const QVariant& value);
    void processingOrderChanged(const QStringList& order);
    void processingConfigurationRequested();

public slots:
    // Override slots from CameraControlPanel for Do3Think specific handling
    void onImageReceived(const QImage& image, qint64 timestamp) override;
    void onStatisticsUpdated(const QJsonObject& stats) override;
    void onErrorOccurred(const QString& error) override;
    
    // Do3Think specific slots
    void onTemperatureUpdated(double temperature);
    void onHDRModeChanged(bool enabled);
    void onColorCorrectionChanged(bool enabled);
    void onGPIOStateChanged(int pin, bool value);
    void onStreamModeChanged(int mode);
    void onDo3ThinkCapabilitiesUpdated(const QJsonObject& capabilities);
    
    // Image processing slots
    void onProcessingEnabledChanged(bool enabled);
    void onPreprocessorEnabledChanged(const QString& type, bool enabled);
    void onProcessingOrderChanged(const QStringList& order);

protected:
    // Override event handlers
    void closeEvent(QCloseEvent* event) override;
    
    // Override UI creation methods
    void createUI() override;
    QWidget* createConnectionControls() override;
    QWidget* createBasicParameterControls() override;
    QWidget* createAdvancedParameterControls() override;
    QWidget* createImageDisplay() override;
    QWidget* createStatisticsDisplay() override;
    
    // Do3Think specific UI creation
    virtual QWidget* createDo3ThinkControls();
    virtual QWidget* createColorCorrectionPanel();
    virtual QWidget* createHDRPanel();
    virtual QWidget* createGPIOPanel();
    virtual QWidget* createDiagnosticsPanel();
    virtual QWidget* createProfilingPanel();
    virtual QWidget* createCalibrationPanel();
    virtual QWidget* createImageAnalysisPanel();
    virtual QWidget* createImageProcessingPanel();
    
    // Internal connections
    void connectInternalSignals() override;
    void connectComponentSignals(QObject* component) override;
    void disconnectComponentSignals(QObject* component) override;
    
    // UI Updates
    void updateUIState(bool connected, bool acquiring) override;
    void updateParameterDisplays() override;
    
    // Do3Think specific updates
    virtual void updateDo3ThinkControls();
    virtual void updateTemperatureDisplay(double temperature);
    virtual void updateHDRDisplay(bool enabled, int levels);
    virtual void updateGPIODisplay();
    virtual void updateProfilingDisplay();
    virtual void updateProcessingDisplay();

protected slots:
    // Internal Do3Think UI handlers
    void onColorCorrectionButtonClicked();
    void onHDRButtonClicked();
    void onGPIOButtonClicked();
    void onCalibrationButtonClicked();
    void onProfilingButtonClicked();
    
    void onColorMatrixChanged();
    void onHDRLevelChanged(int level);
    void onGPIOPinToggled(int pin);
    void onTriggerDelayChanged(double delay);
    void onPacketSizeChanged(int size);
    
    // Analysis tools handlers
    void onHistogramToggled(bool checked);
    void onWaveformToggled(bool checked);
    void onVectorscopeToggled(bool checked);
    void onFocusPeakingToggled(bool checked);
    void onZebraPatternToggled(bool checked);
    
    // Update timers
    void updateDiagnostics();
    void updateProfiling();
    
    // Image processing handlers
    void onProcessingEnableToggled(bool enabled);
    void onBlurEnableToggled(bool enabled);
    void onEdgeEnableToggled(bool enabled);
    void onDenoiseEnableToggled(bool enabled);
    void onBlurKernelChanged(int value);
    void onEdgeThresholdChanged(double value);
    void onDenoiseStrengthChanged(double value);
    void onProcessingOrderChanged();
    void onProcessingConfigurationClicked();

private:
    // Do3Think specific UI components
    struct Do3ThinkControls {
        // Color Correction
        QPushButton* colorCorrectionButton{nullptr};
        QWidget* colorMatrixWidget{nullptr};
        QComboBox* colorPresetCombo{nullptr};
        
        // HDR Controls
        QCheckBox* hdrEnableCheck{nullptr};
        QSpinBox* hdrLevelSpinBox{nullptr};
        QPushButton* hdrSettingsButton{nullptr};
        
        // Temperature Display
        QLabel* temperatureLabel{nullptr};
        QProgressBar* temperatureBar{nullptr};
        
        // GPIO Panel
        QWidget* gpioPanel{nullptr};
        QList<QCheckBox*> gpioPins;
        
        // Trigger Advanced
        QDoubleSpinBox* triggerDelaySpinBox{nullptr};
        QSpinBox* triggerDividerSpinBox{nullptr};
        
        // Stream Control
        QComboBox* streamModeCombo{nullptr};
        QSpinBox* packetSizeSpinBox{nullptr};
        
        // LUT Control
        QCheckBox* lutEnableCheck{nullptr};
        QPushButton* lutLoadButton{nullptr};
        
        // Diagnostics
        QTextEdit* diagnosticsLog{nullptr};
        QTableWidget* diagnosticsTable{nullptr};
        
        // Performance Profiling
        QChartView* performanceChart{nullptr};
        QLineSeries* fpsSeries{nullptr};
        QLineSeries* cpuSeries{nullptr};
        QLineSeries* memorySeries{nullptr};
        QPushButton* profilingStartButton{nullptr};
        QPushButton* profilingStopButton{nullptr};
        
        // Calibration
        QPushButton* calibrationWizardButton{nullptr};
        QLabel* calibrationStatusLabel{nullptr};
        
        // Image Analysis
        QWidget* histogramWidget{nullptr};
        QWidget* waveformWidget{nullptr};
        QWidget* vectorscopeWidget{nullptr};
        QCheckBox* focusPeakingCheck{nullptr};
        QCheckBox* zebraPatternCheck{nullptr};
        
        // Tab Widget for organization
        QTabWidget* do3ThinkTabs{nullptr};
        
        // Image Processing Panel
        QWidget* imageProcessingPanel{nullptr};
        QCheckBox* processingEnableCheck{nullptr};
        QGroupBox* preprocessorGroup{nullptr};
        
        // Preprocessor Controls
        QCheckBox* blurEnableCheck{nullptr};
        QCheckBox* edgeEnableCheck{nullptr};
        QCheckBox* denoiseEnableCheck{nullptr};
        
        // Parameter Controls
        QSpinBox* blurKernelSpinBox{nullptr};
        QDoubleSpinBox* edgeThresholdSpinBox{nullptr};
        QDoubleSpinBox* denoiseStrengthSpinBox{nullptr};
        
        // Processing Order Control
        QListWidget* processingOrderList{nullptr};
        QPushButton* moveUpButton{nullptr};
        QPushButton* moveDownButton{nullptr};
        QPushButton* configureButton{nullptr};
        
        // Processing Status
        QLabel* processingStatusLabel{nullptr};
        QProgressBar* processingLoadBar{nullptr};
    } m_do3thinkControls;
    
    // Do3Think specific state
    struct Do3ThinkState {
        bool showDo3ThinkFeatures{true};
        bool advancedDiagnosticsEnabled{false};
        bool profilingActive{false};
        bool calibrationInProgress{false};
        double lastTemperature{0.0};
        QJsonObject lastCapabilities;
        QJsonObject calibrationData;
        QJsonObject profilingData;
        
        // Image processing state
        bool processingEnabled{false};
        bool blurEnabled{false};
        bool edgeEnabled{false};
        bool denoiseEnabled{false};
        QStringList processingOrder;
    } m_do3thinkState;
    
    // Analysis state
    struct AnalysisState {
        bool histogramEnabled{false};
        bool waveformEnabled{false};
        bool vectorscopeEnabled{false};
        bool focusPeakingEnabled{false};
        bool zebraPatternEnabled{false};
        int zebraThreshold{235};
        QImage lastAnalyzedImage;
    } m_analysisState;
    
    // Profiling data
    struct ProfilingData {
        QList<double> fpsHistory;
        QList<double> cpuHistory;
        QList<double> memoryHistory;
        QList<qint64> timestamps;
        qint64 profilingStartTime{0};
        int maxHistorySize{1000};
    } m_profilingData;
    
    // Timers
    QTimer* m_diagnosticsTimer{nullptr};
    QTimer* m_profilingTimer{nullptr};
    
    // Helper methods
    void setupDo3ThinkUI();
    void updateCharts();
    void processImageAnalysis(const QImage& image);
    QImage generateHistogram(const QImage& image);
    QImage generateWaveform(const QImage& image);
    QImage generateVectorscope(const QImage& image);
    QImage applyFocusPeaking(const QImage& image);
    QImage applyZebraPattern(const QImage& image);
    QString formatTemperature(double temp) const;
    void applyDo3ThinkTheme();
    void updateProcessingOrderList();
    
    Q_DISABLE_COPY(Do3ThinkCameraControlPanel)
};

} // namespace ComponentsForest

#endif // DOTHINK_CAMERA_CONTROL_PANEL_H