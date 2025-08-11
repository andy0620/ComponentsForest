#pragma once

#include <QWidget>
#include <QVariantMap>
#include <QJsonObject>
#include <memory>

QT_BEGIN_NAMESPACE
class QComboBox;
class QSpinBox;
class QDoubleSpinBox;
class QCheckBox;
class QPushButton;
class QLabel;
class QGroupBox;
class QTextEdit;
class QSlider;
class QTabWidget;
QT_END_NAMESPACE

namespace ComponentsForest {
namespace OpenCV {

// Forward declarations
class PreProcessorBase;
struct ProcessingStats;
struct PerformanceMetrics;
enum class ProcessingError;
enum class ProcessingMode;

/**
 * @brief Control panel for preprocessor components
 * 
 * This control panel provides UI controls for preprocessor components,
 * following the same Signal/Slot decoupling architecture as CameraControlPanel.
 * It communicates with preprocessor components only through signals and slots.
 */
class PreProcessorControlPanel : public QWidget {
    Q_OBJECT
    
public:
    explicit PreProcessorControlPanel(QWidget* parent = nullptr);
    virtual ~PreProcessorControlPanel();
    
    // Component connection (uses string-based connections to maintain decoupling)
    void connectToPreprocessor(PreProcessorBase* preprocessor);
    void disconnectFromPreprocessor();
    
    // Panel identification
    QString getPanelId() const;
    void setPanelId(const QString& id);
    
    // Panel state
    bool isConnected() const;
    bool isProcessing() const;
    
    // Configuration
    void setConfiguration(const QVariantMap& config);
    QVariantMap getConfiguration() const;
    
    // UI customization
    void setCompactMode(bool compact);
    bool isCompactMode() const;
    void setShowAdvancedControls(bool show);
    
signals:
    // Control signals to preprocessor
    void requestStartProcessing();
    void requestStopProcessing();
    void requestPauseProcessing();
    void requestResumeProcessing();
    void requestFlushBuffers();
    
    // Configuration signals
    void requestConfigurationUpdate(const QVariantMap& config);
    void requestParameterChange(const QString& key, const QVariant& value);
    void requestGPUEnable(bool enable);
    void requestProcessingModeChange(int mode);
    void requestMaxQueueSizeChange(int size);
    void requestDebugModeChange(bool enable);
    
    // Panel status signals
    void panelConnected(const QString& panelId);
    void panelDisconnected(const QString& panelId);
    void panelError(const QString& error);
    
public slots:
    // Control slots
    void startProcessing();
    void stopProcessing();
    void pauseProcessing();
    void resumeProcessing();
    void clearBuffers();
    
    // Configuration slots
    void updateConfiguration();
    void resetToDefaults();
    
    // Slots for receiving preprocessor signals
    void onFrameProcessed(const QImage& frame, const QVariantMap& metadata);
    void onProcessingError(int error, const QString& details);
    void onProcessingStatistics(const QVariantMap& stats);
    void onProcessingPerformanceUpdate(const QVariantMap& metrics);
    void onBufferStatus(int used, int total);
    void onProcessingStarted();
    void onProcessingStopped();
    void onProcessingPaused();
    void onStateChanged(int state);
    
protected:
    // UI creation methods
    virtual void setupUI();
    virtual QWidget* createControlSection();
    virtual QWidget* createConfigurationSection();
    virtual QWidget* createMonitoringSection();
    virtual QWidget* createAdvancedSection();
    
    // UI update methods
    virtual void updateControlsState();
    virtual void updateStatisticsDisplay();
    virtual void updatePerformanceDisplay();
    virtual void updateBufferDisplay(int used, int total);
    
    // Event handlers
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;
    
private slots:
    // Internal UI slots
    void onStartButtonClicked();
    void onStopButtonClicked();
    void onPauseButtonClicked();
    void onGPUCheckboxToggled(bool checked);
    void onProcessingModeChanged(int index);
    void onMaxQueueSizeChanged(int value);
    void onDebugModeToggled(bool checked);
    void onApplyConfigurationClicked();
    void onResetConfigurationClicked();
    
    // Timer slots
    void updateDisplayRefresh();
    
private:
    class Private;
    std::unique_ptr<Private> d;
    
    // Helper methods
    void connectInternalSignals();
    void updateButtonStates(bool isProcessing);
    void logMessage(const QString& message, const QString& level = "INFO");
    QString formatStatistics(const QVariantMap& stats);
    QString formatPerformance(const QVariantMap& metrics);
};

/**
 * @brief Specialized control panel for specific preprocessor implementations
 * 
 * Child classes can extend this for specific preprocessor types
 * (e.g., BlurPreProcessorControlPanel, EdgePreProcessorControlPanel)
 */
class SpecializedPreProcessorControlPanel : public PreProcessorControlPanel {
    Q_OBJECT
    
public:
    explicit SpecializedPreProcessorControlPanel(QWidget* parent = nullptr);
    
protected:
    // Override to add specialized controls
    virtual QWidget* createSpecializedControls();
    
    // Override to handle specialized parameters
    virtual void applySpecializedConfiguration(const QVariantMap& config);
    virtual QVariantMap getSpecializedConfiguration() const;
};

} // namespace OpenCV
} // namespace ComponentsForest

// Register metatypes for signal/slot connections
// ProcessingError is already declared in preprocessor_base.h
// ProcessingMode is already declared in preprocessor_base.h