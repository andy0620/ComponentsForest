#ifndef DOTHINK_CAMERA_H
#define DOTHINK_CAMERA_H

#include <QObject>
#include <QThread>
#include <QImage>
#include <QJsonObject>
#include <QJsonArray>
#include <QVariantMap>
#include <QTimer>
#include <QMutex>
#include <QQueue>
#include <QMatrix3x3>
#include <memory>
#include <atomic>
#include "../components/camera_component.h"
#include "do3thinkcameracomponent_export.h"
#include "dvp_sdk_wrapper_interface.h"

// Forward declarations for preprocessors
namespace ComponentsForest {
namespace OpenCV {
class PreProcessorBase;
class PreProcessingPipeline;
class BlurPreProcessor;
class EdgePreProcessor;
class DenoisePreProcessor;
}
}

namespace ComponentsForest {

namespace Do3ThinkCameraConstants {
    constexpr dvpUint32 kUserDataNameOffset = 0;
    constexpr dvpUint32 kUserDataGeneralOffset = 256;
    constexpr dvpUint32 kUserDataNameMaxSize = 256;
    constexpr dvpUint32 kDefaultFrameTimeoutMs = 5000;
}

/**
 * @brief Do3Think Specific Device Information
 * Extended information specific to Do3Think cameras
 */
struct Do3ThinkDeviceInfo {
    dvpHandle handle{0};
    dvpCameraInfo info;
    QString connectionString;
    bool supportsColorCorrection{false};
    bool supportsHDR{false};
};

/**
 * @brief Do3Think Camera Component
 * Industrial-grade camera component for AOI equipment
 * Implements CameraComponent abstract interface for Do3Think cameras
 */
class DO3THINKCAMERACOMPONENT_EXPORT Do3ThinkCameraComponent : public CameraComponent {
    Q_OBJECT
    
    // Do3Think specific properties
    Q_PROPERTY(bool supportsColorCorrection READ supportsColorCorrection NOTIFY capabilitiesChanged)
    Q_PROPERTY(bool supportsHDR READ supportsHDR NOTIFY capabilitiesChanged)
    Q_PROPERTY(double sensorTemperature READ sensorTemperature NOTIFY temperatureChanged)

public:
    // Production constructor
    explicit Do3ThinkCameraComponent(QObject* parent = nullptr);
    // Testing constructor with dependency injection
    explicit Do3ThinkCameraComponent(std::shared_ptr<IDvpSdkWrapper> sdkWrapper, QObject* parent = nullptr);
    ~Do3ThinkCameraComponent() override;
    
    // Override component version
    QString componentVersion() const override { return "1.0.0"; }
    
    // ========== Pure Virtual Methods Implementation from CameraComponent ==========
    
    // Device Management
    QList<CameraDeviceInfo> scanDevices() override;
    [[nodiscard]] bool connectCamera(const QString& identifier) override;
    [[nodiscard]] bool disconnectCamera() override;
    CameraDeviceInfo getCurrentDevice() const override;
    
    // Basic Acquisition Control
    [[nodiscard]] bool startAcquisition() override;
    [[nodiscard]] bool stopAcquisition() override;
    [[nodiscard]] bool grabSingleFrame() override;
    
    // Core Parameters (must be supported)
    [[nodiscard]] bool setExposureTime(double microseconds) override;
    [[nodiscard]] bool setGain(double gain) override;
    [[nodiscard]] bool setROI(const QRect& roi) override;
    double exposureTime() const override;
    double gain() const override;
    QRect roi() const override;
    
    // Device Capabilities
    CameraCapabilities getCameraCapabilities() const override;
    
    // ========== Virtual Methods Override from CameraComponent ==========
    
    // Advanced Acquisition Control
    [[nodiscard]] bool executeSoftwareTrigger() override;
    [[nodiscard]] bool setTriggerMode(TriggerMode mode) override;
    TriggerMode triggerMode() const override;
    
    // Extended Parameters
    [[nodiscard]] bool setFrameRate(double fps) override;
    [[nodiscard]] bool setPixelFormat(PixelFormat format) override;
    [[nodiscard]] bool setBinning(int horizontal, int vertical) override;
    [[nodiscard]] bool setGamma(double gamma) override;
    [[nodiscard]] bool setWhiteBalance(double red, double green, double blue) override;
    double frameRate() const override;
    PixelFormat pixelFormat() const override;
    QSize binning() const override;
    double gamma() const override;
    
    // Auto Features
    [[nodiscard]] bool setAutoExposure(bool enable) override;
    [[nodiscard]] bool setAutoGain(bool enable) override;
    [[nodiscard]] bool setAutoWhiteBalance(bool enable) override;
    [[nodiscard]] bool isAutoExposureEnabled() const override;
    [[nodiscard]] bool isAutoGainEnabled() const override;
    
    // Buffer Management
    [[nodiscard]] bool setBufferCount(int count) override;
    int bufferCount() const override;
    [[nodiscard]] bool clearBuffers() override;
    int droppedFrameCount() const override;
    
    // Performance & Statistics
    double getCurrentFps() const override;
    double getBandwidth() const override;
    QJsonObject getStatistics() const override;
    
    // Advanced Features
    [[nodiscard]] bool saveConfiguration(const QString& filePath) const override;
    [[nodiscard]] bool loadConfiguration(const QString& filePath) override;
    [[nodiscard]] bool setUserDefinedName(const QString& name) override;
    QString userDefinedName() const override;
    
    // Feature Query
    [[nodiscard]] bool isFeatureSupported(const QString& feature) const override;
    QVariant getFeatureValue(const QString& feature) const override;
    [[nodiscard]] bool setFeatureValue(const QString& feature, const QVariant& value) override;
    
    // ========== Do3Think Specific Methods ==========
    
    // Color Correction (Do3Think specific)
    [[nodiscard]] Q_INVOKABLE bool setColorCorrectionMatrix(const QMatrix3x3& matrix);
    Q_INVOKABLE QMatrix3x3 colorCorrectionMatrix() const;
    [[nodiscard]] Q_INVOKABLE bool enableColorCorrection(bool enable);
    [[nodiscard]] Q_INVOKABLE bool supportsColorCorrection() const;
    
    // HDR Features (Do3Think specific)
    [[nodiscard]] Q_INVOKABLE bool setHDRMode(bool enable);
    [[nodiscard]] Q_INVOKABLE bool isHDREnabled() const;
    [[nodiscard]] Q_INVOKABLE bool setHDRLevels(int levels);
    Q_INVOKABLE int hdrLevels() const;
    [[nodiscard]] Q_INVOKABLE bool supportsHDR() const;
    
    // Sensor Information (Do3Think specific)
    Q_INVOKABLE double sensorTemperature() const;
    Q_INVOKABLE QString sensorModel() const;
    Q_INVOKABLE QSize sensorResolution() const;
    
    // Advanced Trigger (Do3Think specific)
    [[nodiscard]] Q_INVOKABLE bool setTriggerDelay(double microseconds);
    Q_INVOKABLE double triggerDelay() const;
    [[nodiscard]] Q_INVOKABLE bool setTriggerDivider(int divider);
    Q_INVOKABLE int triggerDivider() const;
    
    // Stream Control (Do3Think specific)
    [[nodiscard]] Q_INVOKABLE bool setStreamMode(int mode);
    Q_INVOKABLE int streamMode() const;
    [[nodiscard]] Q_INVOKABLE bool setPacketSize(int size);
    Q_INVOKABLE int packetSize() const;
    
    // Lookup Table (Do3Think specific)
    [[nodiscard]] Q_INVOKABLE bool setLUT(const QVector<quint16>& lut);
    Q_INVOKABLE QVector<quint16> getLUT() const;
    [[nodiscard]] Q_INVOKABLE bool enableLUT(bool enable);
    [[nodiscard]] Q_INVOKABLE bool isLUTEnabled() const;
    
    // GPIO Control (Do3Think specific)
    [[nodiscard]] Q_INVOKABLE bool setGPIODirection(int pin, bool output);
    [[nodiscard]] Q_INVOKABLE bool setGPIOValue(int pin, bool value);
    [[nodiscard]] Q_INVOKABLE bool getGPIOValue(int pin) const;
    
    // User Data (Do3Think specific)
    [[nodiscard]] Q_INVOKABLE bool writeUserData(const QByteArray& data);
    Q_INVOKABLE QByteArray readUserData() const;
    
    // Image Processing Pipeline (Do3Think specific)
    // This is now delegated to ProcessingPipelineManager

signals:
    // Do3Think specific signals
    void capabilitiesChanged();
    void temperatureChanged(double temperature);
    void hdrModeChanged(bool enabled);
    void colorCorrectionChanged(bool enabled);
    void gpioStateChanged(int pin, bool value);
    void streamModeChanged(int mode);
    void userDataWritten();

protected:
    // BaseComponent Virtual Methods Override
    bool onInitialize() override;
    bool onStart() override;
    bool onStop() override;
    bool onReset() override;
    void onDestroy() override;
    void onStateChanged(ComponentState newState, ComponentState oldState) override;
    void onConfigurationChanged(const QJsonObject& config) override;
    void onEventReceived(const ComponentEvent& event) override;
    
    // CameraComponent Virtual Methods Override
    void processFrame(const ImageBuffer& buffer) override;
    QImage convertToQImage(const ImageBuffer& buffer) override;
    
    // Do3Think specific processing
    void processDo3ThinkFrame(const dvpFrame& frame, void* pBuffer);
    void updateDo3ThinkStatistics();
    void handleDo3ThinkError(dvpStatus status, const QString& context);
    bool applyDo3ThinkParameters();
    void cleanupDo3ThinkResources();

protected slots:
    void onHealthCheck() override;
    void onFrameTimeout() override;
    void onReconnectTimer() override;
    void onStatisticsUpdate() override;
    
    // Do3Think specific slots
    void onDo3ThinkCallback();
    void onTemperatureMonitor();
    
    // Image processing slots (from control panel)
    void onImageProcessingEnabled(bool enabled);
    void onPreprocessorEnabled(const QString& type, bool enabled);
    void onPreprocessorParameterChanged(const QString& type, const QString& parameter, const QVariant& value);
    void onProcessingOrderChanged(const QStringList& order);
    void onProcessingConfigurationRequested();

private:
    // Do3Think SDK Handle
    dvpHandle m_cameraHandle{0};
    Do3ThinkDeviceInfo m_do3thinkInfo;
    
    // Do3Think specific parameters
    struct Do3ThinkParameters {
        double colorCorrectionMatrix[9]{1,0,0,0,1,0,0,0,1};
        bool colorCorrectionEnabled{false};
        bool hdrEnabled{false};
        int hdrLevels{3};
        double triggerDelay{0.0};
        int triggerDivider{1};
        int streamMode{0};
        int packetSize{1500};
        bool lutEnabled{false};
        QVector<quint16> lutTable;
        QString userDefinedName;
    } m_do3thinkParams;
    
    // Do3Think specific state
    struct Do3ThinkState {
        double currentTemperature{0.0};
        QString sensorModel;
        QSize sensorResolution;
        bool isColorCamera{false};
        dvpStreamState streamState{STATE_STOPED};
        bool triggerEnabled{false};
    } m_do3thinkState;
    
    // Do3Think callback data
    struct CallbackData {
        Do3ThinkCameraComponent* component{nullptr};
        QMutex* mutex{nullptr};
        std::atomic<bool> active{false};
    } m_callbackData;
    
    // Image Processing Pipeline
    std::unique_ptr<ProcessingPipelineManager> m_pipelineManager;
    
    // Helper Methods
    bool initializeDo3ThinkDevice(dvpHandle handle);
    bool configureDo3ThinkDefaults();
    QString dvpStatusToString(dvpStatus status) const;
    PixelFormat dvpFormatToPixelFormat(dvpStreamFormat format) const;
    dvpStreamFormat pixelFormatToDvpFormat(PixelFormat format) const;
    TriggerMode dvpTriggerToTriggerMode(dvpTriggerSource source) const;
    dvpTriggerSource triggerModeToDvpTrigger(TriggerMode mode) const;
    
    // Image processing helper methods
    bool initializeProcessingPipeline();
    void cleanupProcessingPipeline();
    bool updateProcessingPipeline();
    QImage processImage(const QImage& inputImage);
    void setupPreprocessorConnections();
    
    // Static callback for Do3Think SDK
    // Static callback wrapper - matches SDK signature dvpStreamCallback
    static dvpInt32 frameCallback(dvpHandle handle, dvpStreamEvent event, void* pContext, dvpFrame* pFrame, void* pBuffer);
    
    std::shared_ptr<IDvpSdkWrapper> m_sdkWrapper;

    Q_DISABLE_COPY(Do3ThinkCameraComponent)
};

} // namespace ComponentsForest

#endif // DOTHINK_CAMERA_H