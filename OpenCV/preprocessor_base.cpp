#include "preprocessor_base.h"
#include <QThread>
#include <QMutexLocker>
#include <QDebug>
#include <QDateTime>
#include <QtConcurrent>
#include <QJsonObject>
#include <QJsonValue>
#include <chrono>
#include <deque>

namespace ComponentsForest {
namespace OpenCV {

// Private implementation class for preprocessor-specific data
class PreProcessorBase::PreprocessorPrivate {
public:
    // Processing-specific configuration (BaseComponent handles base config)
    QVariantMap processingConfig;
    QVariantMap parameterDescriptions;
    ProcessingMode processingMode = ProcessingMode::Balanced;
    bool gpuEnabled = false;
    bool debugMode = false;
    
    // Processing state (uses BaseComponent's state machine)
    std::atomic<bool> isProcessing{false};
    std::atomic<bool> shouldStop{false};
    std::atomic<bool> isPaused{false};
    
    // Processing mode flags
    std::atomic<bool> processingStarted{false};
    
    // Input/output queues
    struct ProcessingUnit {
        QImage image;
        FrameMetadata metadata;
    };
    
    QQueue<ProcessingUnit> inputQueue;
    QMutex queueMutex;
    QWaitCondition queueCondition;
    int maxQueueSize = 100;
    
    // Statistics
    std::atomic<qint64> totalFramesProcessed{0};
    std::atomic<qint64> framesDropped{0};
    std::deque<qint64> processingTimes;  // microseconds
    QMutex statsMutex;
    QElapsedTimer fpsTimer;
    qint64 fpsFrameCount = 0;
    double currentFps = 0.0;
    
    // Processing-specific performance monitoring
    QTimer* processingStatsTimer = nullptr;
    QTimer* bufferCheckTimer = nullptr;
    std::chrono::steady_clock::time_point lastProcessTime;
    
    // Processing error handling (in addition to BaseComponent error handling)
    QString lastProcessingError;
    QMutex processingErrorMutex;
    
    // GPU resources
    bool gpuInitialized = false;
    int gpuDeviceId = 0;
    
    // Memory pool for efficiency
    struct MatBuffer {
        cv::Mat mat;
        bool inUse = false;
    };
    std::vector<MatBuffer> matPool;
    QMutex poolMutex;
};

// Constructor
PreProcessorBase::PreProcessorBase(const QString& name, QObject* parent)
    : BaseComponent(parent)
    , d_preprocessor(std::make_unique<PreprocessorPrivate>()) {
    
    // Set component name using BaseComponent method
    setComponentName(name);
    
    // Initialize processing timers
    d_preprocessor->fpsTimer.start();
    
    // Setup processing statistics timer
    d_preprocessor->processingStatsTimer = new QTimer(this);
    d_preprocessor->processingStatsTimer->setInterval(1000);  // Update every second
    connect(d_preprocessor->processingStatsTimer, &QTimer::timeout, 
            this, &PreProcessorBase::updateProcessingStatistics);
    
    // Setup buffer check timer
    d_preprocessor->bufferCheckTimer = new QTimer(this);
    d_preprocessor->bufferCheckTimer->setInterval(100);  // Check every 100ms
    connect(d_preprocessor->bufferCheckTimer, &QTimer::timeout, 
            this, &PreProcessorBase::checkBufferStatus);
    
    // Initialize processing resources
    initializeProcessingResources();
}

// Destructor
PreProcessorBase::~PreProcessorBase() {
    // Stop preprocessing
    stopPreprocessing();
    
    // Cleanup preprocessing resources first
    cleanupPreprocessorImplementation();
    
    // Shutdown processing resources
    shutdownProcessingResources();
    
    // Cleanup timers
    if (d_preprocessor->processingStatsTimer) {
        d_preprocessor->processingStatsTimer->stop();
        delete d_preprocessor->processingStatsTimer;
    }
    
    if (d_preprocessor->bufferCheckTimer) {
        d_preprocessor->bufferCheckTimer->stop();
        delete d_preprocessor->bufferCheckTimer;
    }
}

// Initialize processing resources (uses BaseComponent's thread management)
void PreProcessorBase::initializeProcessingResources() {
    // BaseComponent handles thread management, we just initialize processing resources
    // The processing loop will be started when the component starts
    
    // Initialize memory pools
    d_preprocessor->matPool.resize(10);  // Pre-allocate 10 Mat buffers
    for (auto& buffer : d_preprocessor->matPool) {
        buffer.inUse = false;
    }
}

// Shutdown processing resources
void PreProcessorBase::shutdownProcessingResources() {
    d_preprocessor->shouldStop = true;
    d_preprocessor->queueCondition.wakeAll();
    
    // Clear all processing queues
    QMutexLocker locker(&d_preprocessor->queueMutex);
    d_preprocessor->inputQueue.clear();
    
    // Release memory pools
    d_preprocessor->matPool.clear();
}

// Main processing loop (integrated with BaseComponent state machine)
void PreProcessorBase::processLoop() {
    while (!d_preprocessor->shouldStop && state() == ComponentState::Running) {
        PreprocessorPrivate::ProcessingUnit unit;
        
        // Get frame from queue
        {
            QMutexLocker locker(&d_preprocessor->queueMutex);
            
            // Wait for frame or stop signal
            while (d_preprocessor->inputQueue.isEmpty() && !d_preprocessor->shouldStop && 
                   state() == ComponentState::Running) {
                d_preprocessor->queueCondition.wait(&d_preprocessor->queueMutex, 100);
                
                if (d_preprocessor->isPaused) {
                    continue;
                }
            }
            
            if (d_preprocessor->shouldStop || state() != ComponentState::Running) {
                break;
            }
            
            if (d_preprocessor->isPaused || d_preprocessor->inputQueue.isEmpty()) {
                continue;
            }
            
            unit = d_preprocessor->inputQueue.dequeue();
        }
        
        // Process frame
        processFrame(unit.image, unit.metadata);
    }
    
    d_preprocessor->isProcessing = false;
    d_preprocessor->processingStarted = false;
}

// BaseComponent lifecycle implementation

bool PreProcessorBase::onInitialize() {
    logInfo("Initializing preprocessor: " + componentName());
    
    // Initialize preprocessing-specific resources
    if (!initializePreprocessorImplementation()) {
        setError("Failed to initialize preprocessor implementation");
        return false;
    }
    
    logInfo("Preprocessor initialized successfully");
    return true;
}

bool PreProcessorBase::onStart() {
    logInfo("Starting preprocessor: " + componentName());
    
    // Start preprocessing
    if (!startPreprocessing()) {
        setError("Failed to start preprocessing");
        return false;
    }
    
    logInfo("Preprocessor started successfully");
    return true;
}

bool PreProcessorBase::onStop() {
    logInfo("Stopping preprocessor: " + componentName());
    
    // Stop preprocessing
    if (!stopPreprocessing()) {
        setError("Failed to stop preprocessing cleanly");
        return false;
    }
    
    logInfo("Preprocessor stopped successfully");
    return true;
}

bool PreProcessorBase::onReset() {
    logInfo("Resetting preprocessor: " + componentName());
    
    // Stop processing
    stopPreprocessing();
    
    // Clear queues and reset statistics
    flushBuffers();
    
    // Reset statistics
    {
        QMutexLocker locker(&d_preprocessor->statsMutex);
        d_preprocessor->totalFramesProcessed = 0;
        d_preprocessor->framesDropped = 0;
        d_preprocessor->processingTimes.clear();
        d_preprocessor->currentFps = 0.0;
        d_preprocessor->fpsFrameCount = 0;
        d_preprocessor->fpsTimer.restart();
    }
    
    clearError();
    logInfo("Preprocessor reset successfully");
    return true;
}

void PreProcessorBase::onDestroy() {
    logInfo("Destroying preprocessor: " + componentName());
    cleanupPreprocessorImplementation();
}

void PreProcessorBase::onConfigurationChanged(const QJsonObject& config) {
    logInfo("Configuration changed for preprocessor: " + componentName());
    
    // Convert JSON config to QVariantMap for processing configuration
    QVariantMap processingConfig = jsonToVariantMap(config);
    configurePreprocessor(processingConfig);
}

// Process a single frame
void PreProcessorBase::processFrame(const QImage& frame, const FrameMetadata& metadata) {
    auto startTime = std::chrono::high_resolution_clock::now();
    
    try {
        // Convert QImage to cv::Mat
        cv::Mat input = qImageToMat(frame);
        
        // Validate input
        if (!validateInput(input)) {
            QString error = d_preprocessor->lastProcessingError.isEmpty() ? 
                          "Invalid input" : d_preprocessor->lastProcessingError;
            emit processingError(ProcessingError::InvalidInput, error);
            setError(error);  // Also set BaseComponent error
            return;
        }
        
        // Process using child implementation
        cv::Mat result = processImplementation(input);
        
        // Handle result
        handleProcessingResult(result, metadata);
        
        // Record processing time
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>
                       (endTime - startTime).count();
        recordProcessingTime(duration);
        
    } catch (const cv::Exception& e) {
        QString error = QString("OpenCV error: %1").arg(e.what());
        emit processingError(ProcessingError::AlgorithmError, error);
        setError(error);  // Also set BaseComponent error
    } catch (const std::exception& e) {
        QString error = QString("Processing error: %1").arg(e.what());
        emit processingError(ProcessingError::UnknownError, error);
        setError(error);  // Also set BaseComponent error
    }
}

// Handle processing result
void PreProcessorBase::handleProcessingResult(const cv::Mat& result, 
                                             const FrameMetadata& metadata) {
    if (result.empty()) {
        QString error = "Empty processing result";
        emit processingError(ProcessingError::AlgorithmError, error);
        setError(error);
        return;
    }
    
    // Convert back to QImage
    QImage processedImage = matToQImage(result);
    
    // Update statistics
    d_preprocessor->totalFramesProcessed++;
    
    // Emit result
    emit frameProcessed(processedImage, metadata);
}

// Public slots implementation

void PreProcessorBase::onFrameReceived(const QImage& frame, const FrameMetadata& metadata) {
    // Only process if component is running and preprocessing is active
    if (!d_preprocessor->isProcessing || d_preprocessor->isPaused || 
        state() != ComponentState::Running) {
        return;
    }
    
    // Check queue size
    {
        QMutexLocker locker(&d_preprocessor->queueMutex);
        
        if (d_preprocessor->inputQueue.size() >= d_preprocessor->maxQueueSize) {
            // Handle based on processing mode
            switch (d_preprocessor->processingMode) {
            case ProcessingMode::RealTime:
                // Drop oldest frame
                if (!d_preprocessor->inputQueue.isEmpty()) {
                    d_preprocessor->inputQueue.dequeue();
                    d_preprocessor->framesDropped++;
                }
                break;
                
            case ProcessingMode::HighQuality:
                // Block until space available
                emit bufferFull();
                return;
                
            case ProcessingMode::Balanced:
                // Drop if queue is very full
                if (d_preprocessor->inputQueue.size() > d_preprocessor->maxQueueSize * 0.9) {
                    d_preprocessor->inputQueue.dequeue();
                    d_preprocessor->framesDropped++;
                }
                break;
            }
        }
        
        // Add to queue
        d_preprocessor->inputQueue.enqueue({frame, metadata});
        d_preprocessor->queueCondition.wakeOne();
    }
}

void PreProcessorBase::onBatchReceived(const QList<QImage>& frames) {
    for (int i = 0; i < frames.size(); ++i) {
        FrameMetadata metadata;
        metadata.frameNumber = i;
        metadata.timestamp = QDateTime::currentMSecsSinceEpoch();
        onFrameReceived(frames[i], metadata);
    }
}

// Processing control methods

bool PreProcessorBase::startPreprocessing() {
    if (!d_preprocessor->processingStarted) {
        d_preprocessor->isProcessing = true;
        d_preprocessor->isPaused = false;
        d_preprocessor->shouldStop = false;
        d_preprocessor->processingStarted = true;
        
        // Start timers
        d_preprocessor->processingStatsTimer->start();
        d_preprocessor->bufferCheckTimer->start();
        
        // Start processing loop in component's thread
        QMetaObject::invokeMethod(this, "processLoop", Qt::QueuedConnection);
        
        emit preprocessingStarted();
        logInfo("Preprocessing started");
        return true;
    }
    return false;
}

bool PreProcessorBase::stopPreprocessing() {
    if (d_preprocessor->processingStarted) {
        d_preprocessor->isProcessing = false;
        d_preprocessor->shouldStop = true;
        d_preprocessor->processingStarted = false;
        d_preprocessor->queueCondition.wakeAll();
        
        // Stop timers
        d_preprocessor->processingStatsTimer->stop();
        d_preprocessor->bufferCheckTimer->stop();
        
        // Clear queues
        flushBuffers();
        
        emit preprocessingStopped();
        logInfo("Preprocessing stopped");
        return true;
    }
    return false;
}

bool PreProcessorBase::pausePreprocessing() {
    if (d_preprocessor->isProcessing && !d_preprocessor->isPaused) {
        d_preprocessor->isPaused = true;
        emit preprocessingPaused();
        logInfo("Preprocessing paused");
        return true;
    }
    return false;
}

bool PreProcessorBase::resumePreprocessing() {
    if (d_preprocessor->isProcessing && d_preprocessor->isPaused) {
        d_preprocessor->isPaused = false;
        d_preprocessor->queueCondition.wakeAll();
        logInfo("Preprocessing resumed");
        return true;
    }
    return false;
}

// These methods are replaced by the new preprocessing control methods above

void PreProcessorBase::flushBuffers() {
    QMutexLocker locker(&d_preprocessor->queueMutex);
    d_preprocessor->inputQueue.clear();
    emit bufferAvailable();
}

// Configuration methods

bool PreProcessorBase::configurePreprocessor(const QVariantMap& config) {
    d_preprocessor->processingConfig = config;
    
    // Apply processing configuration
    if (config.contains("processingMode")) {
        QString mode = config["processingMode"].toString();
        if (mode == "realtime") {
            d_preprocessor->processingMode = ProcessingMode::RealTime;
        } else if (mode == "quality") {
            d_preprocessor->processingMode = ProcessingMode::HighQuality;
        } else {
            d_preprocessor->processingMode = ProcessingMode::Balanced;
        }
    }
    
    if (config.contains("gpuEnabled")) {
        d_preprocessor->gpuEnabled = config["gpuEnabled"].toBool();
    }
    
    if (config.contains("maxQueueSize")) {
        d_preprocessor->maxQueueSize = config["maxQueueSize"].toInt();
    }
    
    if (config.contains("debugMode")) {
        d_preprocessor->debugMode = config["debugMode"].toBool();
    }
    
    // Call child implementation
    return configurePreprocessorImplementation(config);
}

QVariantMap PreProcessorBase::getPreProcessorConfiguration() const {
    return d_preprocessor->processingConfig;
}

void PreProcessorBase::setParameter(const QString& key, const QVariant& value) {
    d_preprocessor->processingConfig[key] = value;
    
    // Reapply configuration
    configurePreprocessorImplementation(d_preprocessor->processingConfig);
}

QVariant PreProcessorBase::getParameter(const QString& key) const {
    return d_preprocessor->processingConfig.value(key);
}

// Statistics and monitoring (enhanced from BaseComponent)

ProcessingStats PreProcessorBase::getProcessingStatistics() const {
    ProcessingStats stats;
    
    QMutexLocker locker(&d_preprocessor->statsMutex);
    
    stats.fps = d_preprocessor->currentFps;
    stats.totalFramesProcessed = d_preprocessor->totalFramesProcessed;
    stats.framesDropped = d_preprocessor->framesDropped;
    
    // Calculate average processing time
    if (!d_preprocessor->processingTimes.empty()) {
        qint64 sum = 0;
        for (qint64 time : d_preprocessor->processingTimes) {
            sum += time;
        }
        stats.averageProcessingTime = sum / d_preprocessor->processingTimes.size() / 1000.0;  // Convert to ms
    } else {
        stats.averageProcessingTime = 0.0;
    }
    
    // Use BaseComponent's performance monitoring for CPU/GPU usage
    stats.cpuUsage = getCpuUsage();
    stats.memoryUsage = static_cast<size_t>(getMemoryUsage() * 1024 * 1024);  // Convert MB to bytes
    stats.gpuUsage = 0.0;  // TODO: Implement GPU usage if needed
    
    return stats;
}

void PreProcessorBase::updateProcessingStatistics() {
    QMutexLocker locker(&d_preprocessor->statsMutex);
    
    // Calculate FPS
    qint64 elapsed = d_preprocessor->fpsTimer.elapsed();
    if (elapsed > 0) {
        d_preprocessor->currentFps = (d_preprocessor->totalFramesProcessed - d_preprocessor->fpsFrameCount) * 1000.0 / elapsed;
        d_preprocessor->fpsFrameCount = d_preprocessor->totalFramesProcessed;
        d_preprocessor->fpsTimer.restart();
    }
    
    // Emit processing-specific statistics
    emit processingStatistics(getProcessingStatistics());
}

void PreProcessorBase::checkBufferStatus() {
    QMutexLocker locker(&d_preprocessor->queueMutex);
    
    int used = d_preprocessor->inputQueue.size();
    int total = d_preprocessor->maxQueueSize;
    
    emit bufferStatus(used, total);
    
    // Check for buffer availability after being full
    if (used < d_preprocessor->maxQueueSize * 0.8) {
        emit bufferAvailable();
    }
}

void PreProcessorBase::recordProcessingTime(qint64 microseconds) {
    QMutexLocker locker(&d_preprocessor->statsMutex);
    
    d_preprocessor->processingTimes.push_back(microseconds);
    
    // Keep only last 100 measurements
    if (d_preprocessor->processingTimes.size() > 100) {
        d_preprocessor->processingTimes.pop_front();
    }
}

// Memory pool management for high-speed processing

cv::Mat PreProcessorBase::getMatFromPool(const cv::Size& size, int type) {
    QMutexLocker locker(&d_preprocessor->poolMutex);
    
    // Look for available buffer with matching size and type
    for (auto& buffer : d_preprocessor->matPool) {
        if (!buffer.inUse && 
            buffer.mat.size() == size && 
            buffer.mat.type() == type) {
            buffer.inUse = true;
            return buffer.mat;
        }
    }
    
    // No suitable buffer found, create new one
    for (auto& buffer : d_preprocessor->matPool) {
        if (!buffer.inUse) {
            buffer.mat = cv::Mat::zeros(size, type);
            buffer.inUse = true;
            return buffer.mat;
        }
    }
    
    // Pool is full, expand it or return a new Mat
    if (d_preprocessor->matPool.size() < 50) {  // Limit pool growth
        d_preprocessor->matPool.resize(d_preprocessor->matPool.size() + 5);
        auto& buffer = d_preprocessor->matPool.back();
        buffer.mat = cv::Mat::zeros(size, type);
        buffer.inUse = true;
        return buffer.mat;
    }
    
    // Pool at max size, return temporary Mat
    return cv::Mat::zeros(size, type);
}

void PreProcessorBase::returnMatToPool(cv::Mat& mat) {
    if (mat.empty()) return;
    
    QMutexLocker locker(&d_preprocessor->poolMutex);
    
    // Find the corresponding buffer and mark as available
    for (auto& buffer : d_preprocessor->matPool) {
        if (buffer.mat.data == mat.data) {
            buffer.inUse = false;
            return;
        }
    }
    
    // Mat not from pool, just release it
    mat.release();
}

void PreProcessorBase::resizeMatPool(int newSize) {
    QMutexLocker locker(&d_preprocessor->poolMutex);
    d_preprocessor->matPool.resize(newSize);
    for (auto& buffer : d_preprocessor->matPool) {
        buffer.inUse = false;
    }
}

// Utility methods with zero-copy optimization

cv::Mat PreProcessorBase::qImageToMat(const QImage& image, bool zeroCopy) {
    switch (image.format()) {
    case QImage::Format_RGB888: {
        if (zeroCopy) {
            // Zero-copy wrap (data is shared, be careful with lifetime)
            cv::Mat mat(image.height(), image.width(), CV_8UC3, 
                       const_cast<uchar*>(image.bits()), image.bytesPerLine());
            cv::Mat result;
            cv::cvtColor(mat, result, cv::COLOR_RGB2BGR);
            return result;
        } else {
            // Safe copy for long-term use
            cv::Mat mat(image.height(), image.width(), CV_8UC3);
            cv::Mat temp(image.height(), image.width(), CV_8UC3,
                        const_cast<uchar*>(image.bits()), image.bytesPerLine());
            cv::cvtColor(temp, mat, cv::COLOR_RGB2BGR);
            return mat;
        }
    }
    case QImage::Format_RGBA8888: {
        cv::Mat mat(image.height(), image.width(), CV_8UC4,
                   const_cast<uchar*>(image.bits()), image.bytesPerLine());
        return zeroCopy ? mat : mat.clone();
    }
    case QImage::Format_Grayscale8: {
        cv::Mat mat(image.height(), image.width(), CV_8UC1,
                   const_cast<uchar*>(image.bits()), image.bytesPerLine());
        return zeroCopy ? mat : mat.clone();
    }
    default: {
        // Convert to RGB888 first
        QImage rgb = image.convertToFormat(QImage::Format_RGB888);
        cv::Mat mat(rgb.height(), rgb.width(), CV_8UC3,
                   const_cast<uchar*>(rgb.bits()), rgb.bytesPerLine());
        cv::Mat result;
        cv::cvtColor(mat, result, cv::COLOR_RGB2BGR);
        return result;  // Always copy for converted formats
    }
    }
}

QImage PreProcessorBase::matToQImage(const cv::Mat& mat, bool zeroCopy) {
    switch (mat.type()) {
    case CV_8UC1: {
        QImage image(mat.data, mat.cols, mat.rows, mat.step, QImage::Format_Grayscale8);
        return zeroCopy ? image : image.copy();
    }
    case CV_8UC3: {
        cv::Mat rgb;
        cv::cvtColor(mat, rgb, cv::COLOR_BGR2RGB);
        QImage image(rgb.data, rgb.cols, rgb.rows, rgb.step, QImage::Format_RGB888);
        return image.copy();  // Always copy due to temporary rgb Mat
    }
    case CV_8UC4: {
        QImage image(mat.data, mat.cols, mat.rows, mat.step, QImage::Format_RGBA8888);
        return zeroCopy ? image : image.copy();
    }
    default:
        // Use BaseComponent logging instead of qWarning
        return QImage();
    }
}

// Protected virtual methods (default implementations)

bool PreProcessorBase::validateInput(const cv::Mat& input) {
    return !input.empty();
}

bool PreProcessorBase::configurePreprocessorImplementation(const QVariantMap& config) {
    Q_UNUSED(config);
    return true;
}

bool PreProcessorBase::initializePreprocessorImplementation() {
    // Default: nothing to initialize
    return true;
}

void PreProcessorBase::cleanupPreprocessorImplementation() {
    // Default: nothing to cleanup
}

// Protected helper methods

void PreProcessorBase::setProcessingError(const QString& error) {
    QMutexLocker locker(&d_preprocessor->processingErrorMutex);
    d_preprocessor->lastProcessingError = error;
}

void PreProcessorBase::registerParameter(const QString& name, const QVariant& defaultValue,
                                        const QString& description) {
    d_preprocessor->processingConfig[name] = defaultValue;
    d_preprocessor->parameterDescriptions[name] = description;
}

void PreProcessorBase::setDefaultConfiguration(const QVariantMap& config) {
    for (auto it = config.begin(); it != config.end(); ++it) {
        if (!d_preprocessor->processingConfig.contains(it.key())) {
            d_preprocessor->processingConfig[it.key()] = it.value();
        }
    }
}

// Public getter methods

bool PreProcessorBase::isProcessing() const {
    return d_preprocessor->isProcessing;
}

void PreProcessorBase::setProcessingMode(ProcessingMode mode) {
    d_preprocessor->processingMode = mode;
}

ProcessingMode PreProcessorBase::getProcessingMode() const {
    return d_preprocessor->processingMode;
}

void PreProcessorBase::setGPUEnabled(bool enable) {
    d_preprocessor->gpuEnabled = enable;
}

bool PreProcessorBase::isGPUEnabled() const {
    return d_preprocessor->gpuEnabled;
}

void PreProcessorBase::setMaxQueueSize(int size) {
    d_preprocessor->maxQueueSize = size;
}

int PreProcessorBase::getMaxQueueSize() const {
    return d_preprocessor->maxQueueSize;
}

void PreProcessorBase::setDebugMode(bool enable) {
    d_preprocessor->debugMode = enable;
}

bool PreProcessorBase::isDebugMode() const {
    return d_preprocessor->debugMode;
}

PerformanceMetrics PreProcessorBase::getProcessingPerformanceMetrics() const {
    PerformanceMetrics metrics;
    
    {
        QMutexLocker locker(&d_preprocessor->queueMutex);
        metrics.inputQueueSize = d_preprocessor->inputQueue.size();
    }
    
    metrics.outputQueueSize = 0;  // TODO: Implement if output queue is added
    metrics.throughput = d_preprocessor->currentFps;
    metrics.gpuActive = d_preprocessor->gpuEnabled && d_preprocessor->gpuInitialized;
    
    // Calculate latency
    if (!d_preprocessor->processingTimes.empty()) {
        qint64 sum = 0;
        for (qint64 time : d_preprocessor->processingTimes) {
            sum += time;
        }
        metrics.processingLatency = sum / d_preprocessor->processingTimes.size() / 1000.0;  // Convert to ms
    } else {
        metrics.processingLatency = 0.0;
    }
    
    return metrics;
}

void PreProcessorBase::updateConfiguration(const QVariantMap& config) {
    configurePreprocessor(config);
}

void PreProcessorBase::enableGPU(bool enable) {
    setGPUEnabled(enable);
}

void PreProcessorBase::handleUpstreamError(const QString& error) {
    qWarning() << "Upstream error:" << error;
    // Optionally pause processing or handle error
}

void PreProcessorBase::handleChainError(ProcessingError error, const QString& details) {
    qWarning() << "Chain error:" << static_cast<int>(error) << details;
    // Propagate or handle error
}

void PreProcessorBase::updateMetadata(const QVariantMap& metadata) {
    // Update any relevant metadata
    Q_UNUSED(metadata);
}

// BaseComponent health monitoring integration
void PreProcessorBase::onHealthCheck() {
    // Call base implementation first
    BaseComponent::onHealthCheck();
    
    // Add preprocessing-specific health checks
    if (d_preprocessor->isProcessing) {
        // Check if processing queue is reasonable
        QMutexLocker locker(&d_preprocessor->queueMutex);
        if (d_preprocessor->inputQueue.size() > d_preprocessor->maxQueueSize * 0.9) {
            logWarning("Processing queue nearly full - potential backpressure");
        }
        
        // Check processing rate
        if (d_preprocessor->currentFps < 1.0 && d_preprocessor->totalFramesProcessed > 10) {
            logWarning("Low processing rate detected");
        }
    }
}

void PreProcessorBase::onPerformanceUpdate() {
    // Call base implementation first
    BaseComponent::onPerformanceUpdate();
    
    // Emit processing-specific performance metrics
    emit processingPerformanceUpdate(getProcessingPerformanceMetrics());
}

// Configuration conversion helpers
QVariantMap PreProcessorBase::jsonToVariantMap(const QJsonObject& json) const {
    QVariantMap map;
    for (auto it = json.begin(); it != json.end(); ++it) {
        map[it.key()] = it.value().toVariant();
    }
    return map;
}

QJsonObject PreProcessorBase::variantMapToJson(const QVariantMap& map) const {
    QJsonObject json;
    for (auto it = map.begin(); it != map.end(); ++it) {
        json[it.key()] = QJsonValue::fromVariant(it.value());
    }
    return json;
}

// Processing control public slots (called by external signals)

void PreProcessorBase::startProcessing() {
    startPreprocessing();
}

void PreProcessorBase::pauseProcessing() {
    pausePreprocessing();
}

void PreProcessorBase::stopProcessing() {
    stopPreprocessing();
}

// BaseComponent configuration methods implementation

bool PreProcessorBase::validateConfiguration(const QJsonObject& config) const {
    // First validate with base component
    if (!BaseComponent::validateConfiguration(config)) {
        return false;
    }
    
    // Validate preprocessing-specific configuration
    if (config.contains("processingMode")) {
        QString mode = config["processingMode"].toString();
        if (mode != "realtime" && mode != "quality" && mode != "balanced") {
            return false;
        }
    }
    
    if (config.contains("maxQueueSize")) {
        int size = config["maxQueueSize"].toInt();
        if (size <= 0 || size > 1000) {
            return false;
        }
    }
    
    if (config.contains("gpuEnabled")) {
        if (!config["gpuEnabled"].isBool()) {
            return false;
        }
    }
    
    // Basic validation is sufficient for now - detailed validation happens during configuration
    return true;
}

QJsonObject PreProcessorBase::getConfigurationSchema() const {
    // Start with base component schema
    QJsonObject schema = BaseComponent::getConfigurationSchema();
    
    // Add preprocessing-specific schema
    QJsonObject properties = schema["properties"].toObject();
    
    // Processing mode
    QJsonObject processingMode;
    processingMode["type"] = "string";
    processingMode["enum"] = QJsonArray{"realtime", "quality", "balanced"};
    processingMode["default"] = "balanced";
    processingMode["description"] = "Processing mode for frame handling";
    properties["processingMode"] = processingMode;
    
    // GPU enabled
    QJsonObject gpuEnabled;
    gpuEnabled["type"] = "boolean";
    gpuEnabled["default"] = false;
    gpuEnabled["description"] = "Enable GPU acceleration if available";
    properties["gpuEnabled"] = gpuEnabled;
    
    // Max queue size
    QJsonObject maxQueueSize;
    maxQueueSize["type"] = "integer";
    maxQueueSize["minimum"] = 1;
    maxQueueSize["maximum"] = 1000;
    maxQueueSize["default"] = 100;
    maxQueueSize["description"] = "Maximum input queue size";
    properties["maxQueueSize"] = maxQueueSize;
    
    // Debug mode
    QJsonObject debugMode;
    debugMode["type"] = "boolean";
    debugMode["default"] = false;
    debugMode["description"] = "Enable debug logging";
    properties["debugMode"] = debugMode;
    
    schema["properties"] = properties;
    return schema;
}

// SignalRateLimiter implementation

SignalRateLimiter::SignalRateLimiter(int maxFps, QObject* parent)
    : QObject(parent)
    , m_maxFps(maxFps)
    , m_lastEmitTime(0) {
    
    m_minInterval = 1000000 / m_maxFps;  // microseconds
}

void SignalRateLimiter::handleFrame(const QImage& frame, const FrameMetadata& metadata) {
    qint64 now = QDateTime::currentMSecsSinceEpoch() * 1000;  // Convert to microseconds
    
    if (now - m_lastEmitTime >= m_minInterval) {
        m_lastEmitTime = now;
        emit frameReady(frame, metadata);
    }
}

} // namespace OpenCV
} // namespace ComponentsForest