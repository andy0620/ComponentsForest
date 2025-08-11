#ifndef CAMERA_CONTROL_PANEL_H
#define CAMERA_CONTROL_PANEL_H

#ifdef COMPONENTSFORESTCORE_EXPORTS
#include "componentsforestcore_export.h"
#define COMPONENTSFORESTCORE_API COMPONENTSFORESTCORE_EXPORT
#else
#define COMPONENTSFORESTCORE_API
#endif
#include <QWidget>
#include <QJsonObject>
#include <QImage>
#include "camera_component.h"

QT_BEGIN_NAMESPACE
class QLabel;
class QPushButton;
class QSlider;
class QSpinBox;
class QDoubleSpinBox;
class QComboBox;
class QCheckBox;
class QGroupBox;
class QTabWidget;
class QProgressBar;
class QTimer;
class QVBoxLayout;
class QHBoxLayout;
class QGridLayout;
QT_END_NAMESPACE

namespace ComponentsForest {

/**
 * @brief Panel Layout Mode
 * Different layout modes for various use cases
 */
enum class PanelLayout {
    Compact,        // Minimal controls, space-saving
    Standard,       // Normal layout with all basic controls
    Advanced,       // Extended controls and features
    Custom          // User-defined layout
};

/**
 * @brief Image Display Mode
 * How images are displayed in the panel
 */
enum class ImageDisplayMode {
    None,           // No image display
    Embedded,       // Image display within panel
    Floating,       // Separate floating window
    Docked          // Dockable window
};

/**
 * @brief Abstract Camera Control Panel Base Class
 * Provides common UI interface for all camera control panels
 */
class COMPONENTSFORESTCORE_API CameraControlPanel : public QWidget {
    Q_OBJECT
    Q_PROPERTY(PanelLayout layout READ panelLayout WRITE setPanelLayout NOTIFY layoutChanged)
    Q_PROPERTY(bool autoConnect READ isAutoConnectEnabled WRITE setAutoConnect NOTIFY autoConnectChanged)
    Q_PROPERTY(bool connected READ isConnected NOTIFY connectionStatusChanged)
    Q_PROPERTY(ImageDisplayMode displayMode READ imageDisplayMode WRITE setImageDisplayMode NOTIFY displayModeChanged)

public:
    explicit CameraControlPanel(QWidget* parent = nullptr);
    ~CameraControlPanel() override;
    
    // ========== Component Connection ==========
    
    /**
     * @brief Connect to a camera component via Signal/Slot only
     * This maintains complete decoupling - the panel only knows about signals/slots
     */
    virtual void connectToComponent(QObject* cameraComponent);
    
    /**
     * @brief Disconnect from the current camera component
     */
    virtual void disconnectFromComponent();
    
    /**
     * @brief Check if connected to a component
     */
    Q_INVOKABLE bool isConnectedToComponent() const;
    
    /**
     * @brief Get the connected component (as QObject to maintain decoupling)
     */
    Q_INVOKABLE QObject* connectedComponent() const;
    
    // ========== Panel Configuration ==========
    
    /**
     * @brief Set the panel layout mode
     */
    virtual void setPanelLayout(PanelLayout layout);
    Q_INVOKABLE PanelLayout panelLayout() const;
    
    /**
     * @brief Enable/disable auto-connect to first available camera
     */
    Q_INVOKABLE void setAutoConnect(bool enable);
    Q_INVOKABLE bool isAutoConnectEnabled() const;
    
    /**
     * @brief Set image display mode
     */
    virtual void setImageDisplayMode(ImageDisplayMode mode);
    Q_INVOKABLE ImageDisplayMode imageDisplayMode() const;
    
    /**
     * @brief Enable/disable specific control groups
     */
    Q_INVOKABLE void setControlGroupEnabled(const QString& groupName, bool enabled);
    Q_INVOKABLE bool isControlGroupEnabled(const QString& groupName) const;
    
    /**
     * @brief Save/Load panel configuration
     */
    Q_INVOKABLE virtual bool savePanelConfiguration(const QString& filePath) const;
    Q_INVOKABLE virtual bool loadPanelConfiguration(const QString& filePath);
    
    // ========== UI Access Methods ==========
    
    /**
     * @brief Get access to specific UI elements (for customization)
     */
    Q_INVOKABLE QWidget* getControlGroup(const QString& groupName) const;
    Q_INVOKABLE QPushButton* getButton(const QString& buttonName) const;
    Q_INVOKABLE QSlider* getSlider(const QString& sliderName) const;
    
    // ========== State Query ==========
    
    Q_INVOKABLE bool isConnected() const;
    Q_INVOKABLE bool isAcquiring() const;
    Q_INVOKABLE QString currentDeviceInfo() const;
    
    // ========== Utility Methods ==========
    
    /**
     * @brief Apply a theme/stylesheet
     */
    Q_INVOKABLE virtual void applyTheme(const QString& themeName);
    
    /**
     * @brief Show/hide advanced controls
     */
    Q_INVOKABLE void setShowAdvancedControls(bool show);
    Q_INVOKABLE bool showAdvancedControls() const;
    
    /**
     * @brief Enable/disable tooltips
     */
    Q_INVOKABLE void setTooltipsEnabled(bool enabled);
    
signals:
    // ========== Control Command Signals ==========
    // These signals are emitted to control the camera component
    
    void requestScanDevices();
    void requestConnect(const QString& deviceId);
    void requestDisconnect();
    void requestStartAcquisition();
    void requestStopAcquisition();
    void requestSingleFrame();
    void requestSoftwareTrigger();
    
    // ========== Parameter Change Signals ==========
    
    void requestSetExposure(double microseconds);
    void requestSetGain(double gain);
    void requestSetFrameRate(double fps);
    void requestSetROI(const QRect& roi);
    void requestSetTriggerMode(int mode);
    void requestSetPixelFormat(int format);
    void requestSetBinning(int horizontal, int vertical);
    
    // ========== Advanced Feature Signals ==========
    
    void requestAutoExposure(bool enable);
    void requestAutoGain(bool enable);
    void requestSaveImage(const QString& filePath);
    void requestLoadConfiguration(const QString& filePath);
    void requestSaveConfiguration(const QString& filePath);
    
    // ========== Panel State Signals ==========
    
    void layoutChanged(PanelLayout layout);
    void autoConnectChanged(bool enabled);
    void connectionStatusChanged(bool connected);
    void displayModeChanged(ImageDisplayMode mode);
    void advancedControlsVisibilityChanged(bool visible);
    
    // ========== User Interaction Signals ==========
    
    void userMessage(const QString& message, int severity);
    void settingsChangeRequested();

public slots:
    // ========== Receive Component Updates ==========
    // These slots receive updates from the camera component
    
    // Device Updates
    virtual void onDeviceListUpdated(const QJsonArray& devices);
    virtual void onDeviceConnected(const QString& deviceId);
    virtual void onDeviceDisconnected();
    
    // State Updates  
    virtual void onCameraStateChanged(int state);
    virtual void onAcquisitionStateChanged(bool acquiring);
    
    // Parameter Updates
    virtual void onExposureChanged(double microseconds);
    virtual void onGainChanged(double gain);
    virtual void onFrameRateChanged(double fps);
    virtual void onROIChanged(const QRect& roi);
    virtual void onTriggerModeChanged(int mode);
    virtual void onPixelFormatChanged(int format);
    
    // Image Updates
    virtual void onImageReceived(const QImage& image, qint64 timestamp);
    virtual void onFrameDropped(qint64 frameNumber);
    
    // Performance Updates
    virtual void onFpsUpdated(double fps);
    virtual void onStatisticsUpdated(const QJsonObject& stats);
    
    // Error Handling
    virtual void onErrorOccurred(const QString& error);
    virtual void onWarningOccurred(const QString& warning);

protected:
    // ========== Virtual UI Creation Methods ==========
    // Subclasses can override these to customize the UI
    
    /**
     * @brief Create the main UI layout
     */
    virtual void createUI();
    
    /**
     * @brief Create device connection controls
     */
    virtual QWidget* createConnectionControls();
    
    /**
     * @brief Create acquisition controls
     */
    virtual QWidget* createAcquisitionControls();
    
    /**
     * @brief Create basic parameter controls
     */
    virtual QWidget* createBasicParameterControls();
    
    /**
     * @brief Create advanced parameter controls
     */
    virtual QWidget* createAdvancedParameterControls();
    
    /**
     * @brief Create image display widget
     */
    virtual QWidget* createImageDisplay();
    
    /**
     * @brief Create statistics display
     */
    virtual QWidget* createStatisticsDisplay();
    
    // ========== Internal Signal/Slot Connections ==========
    
    /**
     * @brief Connect internal UI signals to panel slots
     */
    virtual void connectInternalSignals();
    
    /**
     * @brief Connect to camera component signals
     */
    virtual void connectComponentSignals(QObject* component);
    
    /**
     * @brief Disconnect from camera component signals  
     */
    virtual void disconnectComponentSignals(QObject* component);
    
    // ========== UI Update Methods ==========
    
    /**
     * @brief Update UI element states based on connection status
     */
    virtual void updateUIState(bool connected, bool acquiring);
    
    /**
     * @brief Update device list in UI
     */
    virtual void updateDeviceList(const QJsonArray& devices);
    
    /**
     * @brief Update parameter displays
     */
    virtual void updateParameterDisplays();
    
    /**
     * @brief Show error message to user
     */
    virtual void showError(const QString& message) const;
    
    /**
     * @brief Show info message to user
     */
    virtual void showInfo(const QString& message) const;

protected slots:
    // ========== Internal UI Event Handlers ==========
    
    virtual void onScanButtonClicked();
    virtual void onConnectButtonClicked();
    virtual void onDisconnectButtonClicked();
    virtual void onStartButtonClicked();
    virtual void onStopButtonClicked();
    virtual void onSingleFrameButtonClicked();
    
    virtual void onExposureValueChanged(double value);
    virtual void onGainValueChanged(double value);
    virtual void onFrameRateValueChanged(double value);
    virtual void onTriggerModeSelected(int index);
    virtual void onPixelFormatSelected(int index);
    
    virtual void onAutoExposureToggled(bool checked);
    virtual void onAutoGainToggled(bool checked);
    
    virtual void onSaveImageClicked();
    virtual void onLoadSettingsClicked();
    virtual void onSaveSettingsClicked();
    
    // Update timers
    virtual void updateStatistics();
    virtual void updateImageDisplay();

protected:
    // ========== Common UI Elements ==========
    // These are standard elements that most camera panels will need
    
    struct CommonControls {
        // Connection Controls
        QPushButton* scanButton{nullptr};
        QComboBox* deviceCombo{nullptr};
        QPushButton* connectButton{nullptr};
        QPushButton* disconnectButton{nullptr};
        QLabel* connectionStatus{nullptr};
        
        // Acquisition Controls
        QPushButton* startButton{nullptr};
        QPushButton* stopButton{nullptr};
        QPushButton* singleFrameButton{nullptr};
        QPushButton* triggerButton{nullptr};
        
        // Basic Parameters
        QSlider* exposureSlider{nullptr};
        QDoubleSpinBox* exposureSpinBox{nullptr};
        QSlider* gainSlider{nullptr};
        QDoubleSpinBox* gainSpinBox{nullptr};
        QDoubleSpinBox* frameRateSpinBox{nullptr};
        QComboBox* triggerModeCombo{nullptr};
        QComboBox* pixelFormatCombo{nullptr};
        
        // ROI Controls
        QSpinBox* roiX{nullptr};
        QSpinBox* roiY{nullptr};
        QSpinBox* roiWidth{nullptr};
        QSpinBox* roiHeight{nullptr};
        QPushButton* roiSetButton{nullptr};
        QPushButton* roiResetButton{nullptr};
        
        // Auto Features
        QCheckBox* autoExposureCheck{nullptr};
        QCheckBox* autoGainCheck{nullptr};
        
        // Image Display
        QLabel* imageDisplay{nullptr};
        QLabel* fpsLabel{nullptr};
        QLabel* frameCountLabel{nullptr};
        QProgressBar* bufferUsage{nullptr};
        
        // Layout Containers
        QGroupBox* connectionGroup{nullptr};
        QGroupBox* acquisitionGroup{nullptr};
        QGroupBox* parametersGroup{nullptr};
        QGroupBox* statisticsGroup{nullptr};
    } m_controls;
    
    // ========== State Management ==========
    
    struct PanelState {
        bool connectedToComponent{false};
        bool cameraConnected{false};
        bool acquiring{false};
        PanelLayout currentLayout{PanelLayout::Standard};
        ImageDisplayMode displayMode{ImageDisplayMode::Embedded};
        bool autoConnect{false};
        bool showAdvanced{false};
        QString currentDevice;
        QJsonArray availableDevices;
    } m_state;
    
    // ========== Statistics ==========
    
    struct Statistics {
        qint64 frameCount{0};
        qint64 droppedFrames{0};
        double currentFps{0.0};
        double averageFps{0.0};
        qint64 lastFrameTime{0};
    } m_statistics;
    
    // ========== Display Management ==========
    
    struct DisplaySettings {
        QImage currentImage;
        bool autoScale{true};
        double zoomLevel{1.0};
        bool showOverlay{false};
        bool showCrosshair{false};
        bool showHistogram{false};
    } m_display;
    
    // Component Connection
    QObject* m_connectedComponent{nullptr};
    
    // Timers
    QTimer* m_updateTimer{nullptr};
    QTimer* m_statisticsTimer{nullptr};
    
    // Layouts
    QVBoxLayout* m_mainLayout{nullptr};
    QHBoxLayout* m_topLayout{nullptr};
    QGridLayout* m_parameterLayout{nullptr};

private:
    // Helper methods
    void setupDefaultUI();
    void createStandardLayout();
    void createCompactLayout();
    void createAdvancedLayout();
    QString formatExposureTime(double microseconds) const;
    QString formatFrameRate(double fps) const;
    
    Q_DISABLE_COPY(CameraControlPanel)
};

} // namespace ComponentsForest

// Register types
Q_DECLARE_METATYPE(ComponentsForest::PanelLayout)
Q_DECLARE_METATYPE(ComponentsForest::ImageDisplayMode)

#endif // CAMERA_CONTROL_PANEL_H