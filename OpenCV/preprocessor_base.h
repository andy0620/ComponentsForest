#pragma once

#include "../components/base_component.h"
#include <QImage>
#include <QVariantMap>
#include <QMutex>
#include <QThread>
#include <QTimer>
#include <QQueue>
#include <QElapsedTimer>
#include <QJsonObject>
#include <QMetaType>
#include <opencv2/opencv.hpp>
#include <opencv2/core/cuda.hpp>
#include <atomic>
#include <memory>

namespace ComponentsForest {
namespace OpenCV {

// Processing error types
enum class ProcessingError {
    None,
    InvalidInput,
    ProcessingTimeout,
    GPUError,
    OutOfMemory,
    ConfigurationError,
    AlgorithmError,
    UnknownError
};

// Processing modes
enum class ProcessingMode {
    RealTime,       // Drop frames if necessary to maintain speed
    HighQuality,    // Process all frames, may have latency
    Balanced        // Adaptive mode
};

// Frame metadata
struct FrameMetadata {
    qint64 timestamp;
    int frameNumber;
    QSize originalSize;
    QString sourceId;
    QVariantMap customData;
    
    FrameMetadata() : timestamp(0), frameNumber(0) {}
};

// Processing statistics
struct ProcessingStats {
    double fps;
    double averageProcessingTime;  // ms
    qint64 totalFramesProcessed;
    qint64 framesDropped;
    double cpuUsage;
    double gpuUsage;
    size_t memoryUsage;
};

// Performance metrics
struct PerformanceMetrics {
    double inputQueueSize;
    double outputQueueSize;
    double processingLatency;  // ms
    double throughput;  // frames/sec
    bool gpuActive;
};

/**
 * @brief Base class for all image preprocessing components
 * 
 * This class provides the foundation for image preprocessing in the ComponentsForest
 * ecosystem. It inherits from BaseComponent to get thread management, state machine,
 * Signal/Slot integration, performance monitoring, and configuration management.
 * Child classes implement specific algorithms through pure virtual methods.
 */
class PreProcessorBase : public BaseComponent {
    Q_OBJECT
    
public:
    /**
     * @brief Constructor
     * @param name Component name for identification
     * @param parent Parent QObject
     */
    explicit PreProcessorBase(const QString& name, QObject* parent = nullptr);
    
    // BaseComponent interface overrides
    QString componentType() const override { return "ImageProcessing"; }
    QString componentVersion() const override { return "1.0.0"; }
    
    /**
     * @brief Destructor - ensures proper cleanup
     */
    virtual ~PreProcessorBase();
    
    // Configuration methods (enhanced from BaseComponent)
    QVariantMap getPreProcessorConfiguration() const;
    void setParameter(const QString& key, const QVariant& value);
    QVariant getParameter(const QString& key) const;
    
    // Processing-specific configuration
    bool configurePreprocessor(const QVariantMap& config);
    
    // Processing control
    void setProcessingMode(ProcessingMode mode);
    ProcessingMode getProcessingMode() const;
    void setGPUEnabled(bool enable);
    bool isGPUEnabled() const;
    void setMaxQueueSize(int size);
    int getMaxQueueSize() const;
    
    // Statistics and monitoring (enhanced from BaseComponent)
    ProcessingStats getProcessingStatistics() const;
    PerformanceMetrics getProcessingPerformanceMetrics() const;
    bool isProcessing() const;
    
    // Debug mode
    void setDebugMode(bool enable);
    bool isDebugMode() const;
    
    // Processing control
    bool startPreprocessing();
    bool stopPreprocessing();
    bool pausePreprocessing();
    bool resumePreprocessing();
    
signals:
    // Image processing output signals
    void frameProcessed(const QImage& processedFrame, const FrameMetadata& metadata);
    void batchProcessed(const QList<QImage>& frames, const QList<FrameMetadata>& metadata);
    void processingError(ProcessingError error, const QString& details);
    void processingStatistics(const ProcessingStats& stats);
    
    // Pipeline control signals (in addition to BaseComponent state signals)
    void preprocessingStarted();
    void preprocessingPaused();
    void preprocessingStopped();
    
    // Processing performance monitoring (in addition to BaseComponent metrics)
    void processingPerformanceUpdate(const PerformanceMetrics& metrics);
    void bufferStatus(int used, int total);
    
    // Backpressure signals
    void bufferFull();
    void bufferAvailable();
    
public slots:
    // Input slots
    void onFrameReceived(const QImage& frame, const FrameMetadata& metadata);
    void onBatchReceived(const QList<QImage>& frames);
    
    // Control slots
    void startProcessing();
    void pauseProcessing();
    void stopProcessing();
    void flushBuffers();
    
    // Configuration slots
    void updateConfiguration(const QVariantMap& config);
    void enableGPU(bool enable);
    
    // Error handling
    void handleUpstreamError(const QString& error);
    void handleChainError(ProcessingError error, const QString& details);
    
    // Metadata update
    void updateMetadata(const QVariantMap& metadata);
    
protected:
    // BaseComponent lifecycle overrides
    bool onInitialize() override;
    bool onStart() override;
    bool onStop() override;
    bool onReset() override;
    void onDestroy() override;
    void onConfigurationChanged(const QJsonObject& config) override;
    
    // BaseComponent configuration overrides
    bool validateConfiguration(const QJsonObject& config) const override;
    QJsonObject getConfigurationSchema() const override;
    
    // Pure virtual methods that child classes must implement
    
    /**
     * @brief Process implementation - child classes implement their algorithm here
     * @param input Input image as OpenCV Mat
     * @return Processed image as OpenCV Mat
     */
    virtual cv::Mat processImplementation(const cv::Mat& input) = 0;
    
    // Optional virtual methods for child classes
    
    /**
     * @brief Validate input before processing
     * @param input Input image to validate
     * @return true if input is valid, false otherwise
     */
    virtual bool validateInput(const cv::Mat& input);
    
    /**
     * @brief Configure implementation-specific parameters
     * @param config Configuration map
     * @return true if configuration successful
     */
    virtual bool configurePreprocessorImplementation(const QVariantMap& config);
    
    /**
     * @brief Initialize preprocessing-specific resources
     */
    virtual bool initializePreprocessorImplementation();
    
    /**
     * @brief Cleanup preprocessing-specific resources
     */
    virtual void cleanupPreprocessorImplementation();
    
    // Helper methods for child classes
    
    /**
     * @brief Set processing-specific error (in addition to BaseComponent error handling)
     * @param error Error message
     */
    void setProcessingError(const QString& error);
    
    /**
     * @brief Register a configurable parameter
     * @param name Parameter name
     * @param defaultValue Default value
     * @param description Parameter description
     */
    void registerParameter(const QString& name, const QVariant& defaultValue, 
                          const QString& description = QString());
    
    /**
     * @brief Set default configuration
     * @param config Default configuration map
     */
    void setDefaultConfiguration(const QVariantMap& config);
    
    // Utility methods with zero-copy optimization
    static cv::Mat qImageToMat(const QImage& image, bool zeroCopy = true);
    static QImage matToQImage(const cv::Mat& mat, bool zeroCopy = true);
    
    // Memory pool management for high-speed processing
    cv::Mat getMatFromPool(const cv::Size& size, int type);
    void returnMatToPool(cv::Mat& mat);
    void resizeMatPool(int newSize);
    
private slots:
    void processLoop();
    void updateProcessingStatistics();
    void checkBufferStatus();
    
    // BaseComponent integration slots
    void onHealthCheck() override;
    void onPerformanceUpdate() override;
    
private:
    class PreprocessorPrivate;
    std::unique_ptr<PreprocessorPrivate> d_preprocessor;
    
    // Processing thread management (uses BaseComponent's thread management)
    void initializeProcessingResources();
    void shutdownProcessingResources();
    
    // Convert between BaseComponent's QJsonObject config and processing QVariantMap
    QVariantMap jsonToVariantMap(const QJsonObject& json) const;
    QJsonObject variantMapToJson(const QVariantMap& map) const;
    
    // Internal processing methods
    void processFrame(const QImage& frame, const FrameMetadata& metadata);
    void handleProcessingResult(const cv::Mat& result, const FrameMetadata& metadata);
    
    // Queue management
    bool enqueueFrame(const QImage& frame, const FrameMetadata& metadata);
    QPair<QImage, FrameMetadata> dequeueFrame();
    
    // Statistics tracking
    void recordProcessingTime(qint64 microseconds);
    void recordFrameDropped();
    void updatePerformanceMetrics();
};

/**
 * @brief Helper class for rate limiting signals
 */
class SignalRateLimiter : public QObject {
    Q_OBJECT
    
public:
    explicit SignalRateLimiter(int maxFps, QObject* parent = nullptr);
    
public slots:
    void handleFrame(const QImage& frame, const FrameMetadata& metadata);
    
signals:
    void frameReady(const QImage& frame, const FrameMetadata& metadata);
    
private:
    int m_maxFps;
    qint64 m_minInterval;  // microseconds
    qint64 m_lastEmitTime;
};

} // namespace OpenCV
} // namespace ComponentsForest

// Register types with Qt's meta-object system
Q_DECLARE_METATYPE(ComponentsForest::OpenCV::FrameMetadata)
Q_DECLARE_METATYPE(ComponentsForest::OpenCV::ProcessingError)
Q_DECLARE_METATYPE(ComponentsForest::OpenCV::ProcessingMode)
Q_DECLARE_METATYPE(ComponentsForest::OpenCV::ProcessingStats)
Q_DECLARE_METATYPE(ComponentsForest::OpenCV::PerformanceMetrics)