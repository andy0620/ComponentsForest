#ifndef CAMERA_COMPONENT_H
#define CAMERA_COMPONENT_H

#ifdef COMPONENTSFORESTCORE_EXPORTS
#include "componentsforestcore_export.h"
#define COMPONENTSFORESTCORE_API COMPONENTSFORESTCORE_EXPORT
#else
#define COMPONENTSFORESTCORE_API
#endif
#include "base_component.h"
#include <QImage>
#include <QRect>
#include <QSize>
#include <memory>

namespace ComponentsForest {

/**
 * @brief Camera State Enumeration
 * Standard states for all camera components
 */
enum class CameraState {
    Disconnected,   // No camera connected
    Connected,      // Camera connected but not acquiring
    Acquiring,      // Actively acquiring images
    Error,          // Error state
    Recovering      // Attempting to recover from error
};

/**
 * @brief Standard Trigger Modes
 * Common trigger modes across all camera types
 */
enum class TriggerMode {
    FreeRun,        // Continuous acquisition
    Software,       // Software trigger
    Hardware,       // Hardware trigger
    FixedRate,      // Fixed frame rate
    Burst           // Burst mode
};

/**
 * @brief Standard Pixel Formats
 * Common pixel formats supported by most cameras
 */
enum class PixelFormat {
    Mono8,          // 8-bit monochrome
    Mono10,         // 10-bit monochrome
    Mono12,         // 12-bit monochrome
    Mono16,         // 16-bit monochrome
    RGB24,          // 24-bit RGB
    BGR24,          // 24-bit BGR
    RGBA32,         // 32-bit RGBA
    YUV422,         // YUV 4:2:2
    BayerRG8,       // Bayer pattern RG
    BayerGB8,       // Bayer pattern GB
    BayerGR8,       // Bayer pattern GR
    BayerBG8,       // Bayer pattern BG
    Custom          // Manufacturer specific
};

/**
 * @brief Camera Device Information
 * Standard information for any camera device
 */
struct CameraDeviceInfo {
    QString serialNumber;       // Unique identifier
    QString modelName;          // Camera model
    QString manufacturer;       // Manufacturer name
    QString friendlyName;       // User-friendly name
    QString interfaceType;      // USB3, GigE, CameraLink, etc.
    QString devicePath;         // System device path
    bool isAvailable{true};    // Device availability
    QJsonObject customInfo;     // Manufacturer-specific info
};

/**
 * @brief Frame Metadata
 * Standard metadata for acquired frames
 */
struct FrameMetadata {
    qint64 timestamp{0};        // Acquisition timestamp (microseconds)
    quint64 frameNumber{0};     // Frame sequence number
    double exposureTime{0.0};   // Actual exposure time (microseconds)
    double gain{0.0};           // Actual gain value
    double temperature{0.0};    // Sensor temperature
    QSize imageSize;            // Image dimensions
    PixelFormat pixelFormat{PixelFormat::Mono8};
    QRect roi;                  // Region of interest
    bool isValid{true};         // Frame validity
    QJsonObject customData;     // Manufacturer-specific metadata
};

/**
 * @brief Image Buffer
 * Efficient image data container for zero-copy operations
 */
class ImageBuffer {
public:
    ImageBuffer() = default;
    ImageBuffer(void* data, size_t size, const FrameMetadata& metadata)
        : m_data(data), m_size(size), m_metadata(metadata) {}
    
    void* data() const { return m_data; }
    size_t size() const { return m_size; }
    const FrameMetadata& metadata() const { return m_metadata; }
    
    QImage toQImage() const;
    QByteArray toByteArray() const;
    
private:
    void* m_data{nullptr};
    size_t m_size{0};
    FrameMetadata m_metadata;
};

/**
 * @brief Camera Capabilities
 * Describes what features a camera supports
 */
struct CameraCapabilities {
    // Supported features
    bool supportsHardwareTrigger{false};
    bool supportsSoftwareTrigger{true};
    bool supportsROI{true};
    bool supportsBinning{false};
    bool supportsAutoExposure{false};
    bool supportsAutoGain{false};
    bool supportsWhiteBalance{false};
    bool supportsGPU{false};
    bool supportsMultiROI{false};
    bool supportsTimestamp{true};
    bool supportsTemperature{false};
    
    // Parameter ranges
    double minExposure{10.0};      // microseconds
    double maxExposure{1000000.0}; // microseconds
    double minGain{1.0};
    double maxGain{16.0};
    double minFrameRate{1.0};
    double maxFrameRate{1000.0};
    QSize minResolution{64, 64};
    QSize maxResolution{4096, 4096};
    
    // Supported formats
    QList<PixelFormat> pixelFormats;
    QList<TriggerMode> triggerModes;
    QStringList customFeatures;    // Manufacturer-specific features
    
    // Performance characteristics
    int maxBuffers{100};
    double maxBandwidth{1000.0};   // MB/s
    bool zeroCopySupport{false};
};

/**
 * @brief Abstract Camera Component Base Class
 * Provides common interface for all camera components
 */
class COMPONENTSFORESTCORE_API CameraComponent : public BaseComponent {
    Q_OBJECT
    Q_PROPERTY(CameraState cameraState READ cameraState NOTIFY cameraStateChanged)
    Q_PROPERTY(bool isConnected READ isConnected NOTIFY connectionStateChanged)
    Q_PROPERTY(bool isAcquiring READ isAcquiring NOTIFY acquisitionStateChanged)
    Q_PROPERTY(double exposureTime READ exposureTime WRITE setExposureTime NOTIFY exposureTimeChanged)
    Q_PROPERTY(double gain READ gain WRITE setGain NOTIFY gainChanged)
    Q_PROPERTY(double frameRate READ frameRate WRITE setFrameRate NOTIFY frameRateChanged)
    Q_PROPERTY(TriggerMode triggerMode READ triggerMode WRITE setTriggerMode NOTIFY triggerModeChanged)
    Q_PROPERTY(PixelFormat pixelFormat READ pixelFormat WRITE setPixelFormat NOTIFY pixelFormatChanged)

public:
    explicit CameraComponent(QObject* parent = nullptr);
    ~CameraComponent() override;
    
    // Override from BaseComponent
    QString componentType() const override { return "Camera"; }
    
    // ========== Pure Virtual Methods (Must be implemented) ==========
    
    // Device Management
    virtual QList<CameraDeviceInfo> scanDevices() = 0;
    virtual bool connectCamera(const QString& identifier) = 0;
    virtual bool disconnectCamera() = 0;
    virtual CameraDeviceInfo getCurrentDevice() const = 0;
    
    // Basic Acquisition Control
    virtual bool startAcquisition() = 0;
    virtual bool stopAcquisition() = 0;
    virtual bool grabSingleFrame() = 0;
    
    // Core Parameters (must be supported by all cameras)
    virtual bool setExposureTime(double microseconds) = 0;
    virtual bool setGain(double gain) = 0;
    virtual bool setROI(const QRect& roi) = 0;
    virtual double exposureTime() const = 0;
    virtual double gain() const = 0;
    virtual QRect roi() const = 0;
    
    // Device Capabilities
    virtual CameraCapabilities getCameraCapabilities() const = 0;
    QJsonObject getCapabilities() const override;  // Override from BaseComponent
    
    // ========== Virtual Methods (Can be overridden) ==========
    
    // Advanced Acquisition Control
    virtual bool executeSoftwareTrigger();
    virtual bool setTriggerMode(TriggerMode mode);
    virtual TriggerMode triggerMode() const;
    
    // Extended Parameters
    virtual bool setFrameRate(double fps);
    virtual bool setPixelFormat(PixelFormat format);
    virtual bool setBinning(int horizontal, int vertical);
    virtual bool setGamma(double gamma);
    virtual bool setWhiteBalance(double red, double green, double blue);
    virtual double frameRate() const;
    virtual PixelFormat pixelFormat() const;
    virtual QSize binning() const;
    virtual double gamma() const;
    
    // Auto Features
    virtual bool setAutoExposure(bool enable);
    virtual bool setAutoGain(bool enable);
    virtual bool setAutoWhiteBalance(bool enable);
    virtual bool isAutoExposureEnabled() const;
    virtual bool isAutoGainEnabled() const;
    
    // Buffer Management
    virtual bool setBufferCount(int count);
    virtual int bufferCount() const;
    virtual bool clearBuffers();
    virtual int droppedFrameCount() const;
    
    // Performance & Statistics
    virtual double getCurrentFps() const;
    virtual double getBandwidth() const;
    virtual QJsonObject getStatistics() const;
    virtual QJsonObject getPerformanceMetrics() const;
    
    // Advanced Features
    virtual bool saveConfiguration(const QString& filePath) const;
    virtual bool loadConfiguration(const QString& filePath);
    virtual bool setUserDefinedName(const QString& name);
    virtual QString userDefinedName() const;
    
    // Feature Query
    virtual bool isFeatureSupported(const QString& feature) const;
    virtual QVariant getFeatureValue(const QString& feature) const;
    virtual bool setFeatureValue(const QString& feature, const QVariant& value);
    
    // ========== Common Methods (Implemented in base class) ==========
    
    // State Query
    Q_INVOKABLE CameraState cameraState() const;
    Q_INVOKABLE bool isConnected() const;
    Q_INVOKABLE bool isAcquiring() const;
    Q_INVOKABLE QString cameraStateString() const;
    
    // Synchronization Support
    Q_INVOKABLE bool enableTimestamp(bool enable);
    Q_INVOKABLE bool synchronizeWith(CameraComponent* other);
    Q_INVOKABLE void setMaster(bool isMaster);
    Q_INVOKABLE bool isMaster() const;
    
    // Error Handling
    Q_INVOKABLE QString getLastCameraError() const;
    Q_INVOKABLE bool recoverFromError();
    Q_INVOKABLE void setAutoReconnect(bool enable);
    Q_INVOKABLE bool isAutoReconnectEnabled() const;
    
    // Utility Methods
    Q_INVOKABLE QString pixelFormatToString(PixelFormat format) const;
    Q_INVOKABLE PixelFormat stringToPixelFormat(const QString& str) const;
    Q_INVOKABLE QString triggerModeToString(TriggerMode mode) const;
    Q_INVOKABLE TriggerMode stringToTriggerMode(const QString& str) const;

signals:
    // ========== Image Data Signals ==========
    void frameReady(const QImage& image, const FrameMetadata& metadata);
    void rawFrameReady(const ImageBuffer& buffer);
    void multiFrameReady(const QList<ImageBuffer>& buffers);
    
    // ========== State Change Signals ==========
    void cameraStateChanged(CameraState newState, CameraState oldState);
    void connectionStateChanged(bool connected);
    void acquisitionStateChanged(bool acquiring);
    void deviceChanged(const CameraDeviceInfo& device);
    void deviceLost(const QString& serialNumber);
    
    // ========== Parameter Change Signals ==========
    void exposureTimeChanged(double microseconds);
    void gainChanged(double gain);
    void frameRateChanged(double fps);
    void roiChanged(const QRect& roi);
    void triggerModeChanged(TriggerMode mode);
    void pixelFormatChanged(PixelFormat format);
    void binningChanged(int horizontal, int vertical);
    
    // ========== Performance Signals ==========
    void frameDropped(qint64 frameNumber, const QString& reason);
    void bufferOverflow();
    void performanceWarning(const QString& warning);
    void bandwidthExceeded(double current, double maximum);
    void fpsUpdated(double fps);
    
    // ========== Error Signals ==========
    void cameraError(const QString& error);
    void cameraWarning(const QString& warning);
    void temperatureWarning(double temperature);
    void hardwareError(const QString& error);
    
    // ========== Synchronization Signals ==========
    void synchronizationLost();
    void masterStatusChanged(bool isMaster);
    void slaveConnected(const QString& slaveId);

protected:
    // State Management
    void setCameraState(CameraState state);
    bool canTransitionTo(CameraState newState) const;
    
    // Error Management
    void setCameraError(const QString& error);
    void clearCameraError();
    
    // Frame Processing
    virtual void processFrame(const ImageBuffer& buffer);
    virtual QImage convertToQImage(const ImageBuffer& buffer);
    
    // Statistics Update
    void updateFrameStatistics(const FrameMetadata& metadata);
    void updatePerformanceMetrics();
    
    // Synchronization
    void handleSynchronization(qint64 timestamp);

protected slots:
    // Internal Slots
    virtual void onFrameTimeout();
    virtual void onReconnectTimer();
    virtual void onStatisticsUpdate();

protected:
    // Statistics - made protected for derived classes
    struct CameraStatistics {
        std::atomic<qint64> totalFrames{0};
        std::atomic<qint64> droppedFrames{0};
        std::atomic<double> currentFps{0.0};
        std::atomic<double> averageFps{0.0};
        std::atomic<double> peakFps{0.0};
        std::atomic<double> bandwidth{0.0};
        qint64 acquisitionStartTime{0};
        qint64 lastFrameTime{0};
    } m_statistics;
    
    // Current Device - made protected for derived classes
    CameraDeviceInfo m_currentDevice;
    mutable QMutex m_deviceMutex;

private:
    // Private Implementation
    class Private;
    std::unique_ptr<Private> d_ptr;
    
    // Camera State
    CameraState m_cameraState{CameraState::Disconnected};
    mutable QMutex m_cameraStateMutex;
    
    // Error Handling
    QString m_lastCameraError;
    bool m_autoReconnect{false};
    int m_reconnectAttempts{0};
    QTimer* m_reconnectTimer{nullptr};
    
    // Synchronization
    bool m_isMaster{false};
    QList<CameraComponent*> m_slaves;
    CameraComponent* m_master{nullptr};
    
    // Performance Monitoring
    QTimer* m_statisticsTimer{nullptr};
    QTimer* m_frameTimeoutTimer{nullptr};
    
    Q_DISABLE_COPY(CameraComponent)
};

/**
 * @brief Camera Factory Interface
 * Factory pattern for creating camera components
 */
class ICameraFactory {
public:
    virtual ~ICameraFactory() = default;
    virtual CameraComponent* createCamera(const QString& manufacturer, const QJsonObject& config) = 0;
    virtual QStringList supportedManufacturers() const = 0;
    virtual bool registerManufacturer(const QString& name, std::function<CameraComponent*()> creator) = 0;
};

} // namespace ComponentsForest

// Register types with Qt's meta system
Q_DECLARE_METATYPE(ComponentsForest::CameraState)
Q_DECLARE_METATYPE(ComponentsForest::TriggerMode)
Q_DECLARE_METATYPE(ComponentsForest::PixelFormat)
Q_DECLARE_METATYPE(ComponentsForest::CameraDeviceInfo)
Q_DECLARE_METATYPE(ComponentsForest::FrameMetadata)
Q_DECLARE_METATYPE(ComponentsForest::ImageBuffer)

#endif // CAMERA_COMPONENT_H