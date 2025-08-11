#include "camera_component.h"
#include <QTimer>
#include <QDateTime>
#include <QPainter>
#include <QBuffer>
#include <QImageReader>
#include <QImageWriter>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>
// #include <QtConcurrent>  // Not used, removed to fix Qt6 compilation
#include <algorithm>
#include <chrono>
#include <deque>
#include <mutex>

namespace ComponentsForest {

// ========== Private Implementation Class ==========
class CameraComponent::Private {
public:
    // Timing for FPS calculation
    struct FpsCalculator {
        static constexpr int WINDOW_SIZE = 30;
        std::deque<qint64> frameTimes;
        mutable std::mutex mutex;
        
        void addFrame(qint64 timestamp) {
            std::lock_guard<std::mutex> lock(mutex);
            frameTimes.push_back(timestamp);
            while (frameTimes.size() > WINDOW_SIZE) {
                frameTimes.pop_front();
            }
        }
        
        double calculateFps() const {
            std::lock_guard<std::mutex> lock(mutex);
            if (frameTimes.size() < 2) return 0.0;
            
            qint64 timeSpan = frameTimes.back() - frameTimes.front();
            if (timeSpan <= 0) return 0.0;
            
            return (frameTimes.size() - 1) * 1000000.0 / timeSpan; // Convert from microseconds to seconds
        }
        
        void reset() {
            std::lock_guard<std::mutex> lock(mutex);
            frameTimes.clear();
        }
    };
    
    FpsCalculator fpsCalculator;
    
    // Default parameter values
    TriggerMode currentTriggerMode{TriggerMode::FreeRun};
    PixelFormat currentPixelFormat{PixelFormat::Mono8};
    double currentFrameRate{30.0};
    double currentGamma{1.0};
    QSize currentBinning{1, 1};
    
    // Auto features
    bool autoExposureEnabled{false};
    bool autoGainEnabled{false};
    bool autoWhiteBalanceEnabled{false};
    
    // Buffer management
    int bufferCount{10};
    std::atomic<int> droppedFrames{0};
    
    // User-defined properties
    QString userDefinedName;
    
    // Timestamp support
    bool timestampEnabled{true};
    std::chrono::steady_clock::time_point startTime{std::chrono::steady_clock::now()};
    
    // Feature map for custom features
    QMap<QString, QVariant> customFeatures;
    mutable QMutex featureMutex;
    
    // Frame processing queue
    std::atomic<bool> processingEnabled{true};
    
    // Performance tracking
    std::atomic<qint64> bytesTransferred{0};
    std::chrono::steady_clock::time_point bandwidthStartTime{std::chrono::steady_clock::now()};
};

// ========== Constructor/Destructor ==========
CameraComponent::CameraComponent(QObject* parent)
    : BaseComponent(parent)
    , d_ptr(std::make_unique<Private>())
{
    // Initialize timers
    m_statisticsTimer = new QTimer(this);
    m_statisticsTimer->setInterval(1000); // Update statistics every second
    connect(m_statisticsTimer, &QTimer::timeout, this, &CameraComponent::onStatisticsUpdate);
    
    m_frameTimeoutTimer = new QTimer(this);
    m_frameTimeoutTimer->setInterval(5000); // 5 second timeout for frame acquisition
    m_frameTimeoutTimer->setSingleShot(true);
    connect(m_frameTimeoutTimer, &QTimer::timeout, this, &CameraComponent::onFrameTimeout);
    
    m_reconnectTimer = new QTimer(this);
    m_reconnectTimer->setInterval(3000); // Try reconnection every 3 seconds
    connect(m_reconnectTimer, &QTimer::timeout, this, &CameraComponent::onReconnectTimer);
    
    // Initialize statistics
    m_statistics.acquisitionStartTime = QDateTime::currentMSecsSinceEpoch();
}

CameraComponent::~CameraComponent() {
    // Note: Derived classes must handle stopAcquisition() and disconnectCamera()
    // in their own destructors before reaching here, as we cannot call
    // pure virtual methods from the base destructor
    
    // Clean up slaves/master relationships
    QMutexLocker lock(&m_deviceMutex);
    for (auto* slave : m_slaves) {
        if (slave && slave->m_master == this) {
            slave->m_master = nullptr;
        }
    }
    if (m_master) {
        m_master->m_slaves.removeAll(this);
    }
}

// ========== Virtual Methods with Default Implementations ==========

bool CameraComponent::executeSoftwareTrigger() {
    // Default implementation - return false if not in software trigger mode
    if (triggerMode() != TriggerMode::Software) {
        setCameraError("Software trigger can only be executed in Software trigger mode");
        return false;
    }
    
    // Derived classes should override this to actually trigger the camera
    logWarning("executeSoftwareTrigger() not implemented for this camera type");
    return false;
}

bool CameraComponent::setTriggerMode(TriggerMode mode) {
    // Check if mode is supported
    auto capabilities = getCameraCapabilities();
    if (!capabilities.triggerModes.contains(mode)) {
        setCameraError(QString("Trigger mode %1 not supported").arg(triggerModeToString(mode)));
        return false;
    }
    
    TriggerMode oldMode = d_ptr->currentTriggerMode;
    d_ptr->currentTriggerMode = mode;
    
    if (oldMode != mode) {
        emit triggerModeChanged(mode);
    }
    
    return true;
}

TriggerMode CameraComponent::triggerMode() const {
    return d_ptr->currentTriggerMode;
}

bool CameraComponent::setFrameRate(double fps) {
    // Validate frame rate
    auto capabilities = getCameraCapabilities();
    if (fps < capabilities.minFrameRate || fps > capabilities.maxFrameRate) {
        setCameraError(QString("Frame rate %1 fps is out of range [%2, %3]")
                      .arg(fps)
                      .arg(capabilities.minFrameRate)
                      .arg(capabilities.maxFrameRate));
        return false;
    }
    
    double oldRate = d_ptr->currentFrameRate;
    d_ptr->currentFrameRate = fps;
    
    if (oldRate != fps) {
        emit frameRateChanged(fps);
    }
    
    return true;
}

bool CameraComponent::setPixelFormat(PixelFormat format) {
    // Check if format is supported
    auto capabilities = getCameraCapabilities();
    if (!capabilities.pixelFormats.contains(format)) {
        setCameraError(QString("Pixel format %1 not supported").arg(pixelFormatToString(format)));
        return false;
    }
    
    PixelFormat oldFormat = d_ptr->currentPixelFormat;
    d_ptr->currentPixelFormat = format;
    
    if (oldFormat != format) {
        emit pixelFormatChanged(format);
    }
    
    return true;
}

bool CameraComponent::setBinning(int horizontal, int vertical) {
    // Validate binning values
    if (horizontal < 1 || vertical < 1 || horizontal > 8 || vertical > 8) {
        setCameraError(QString("Invalid binning values: %1x%2").arg(horizontal).arg(vertical));
        return false;
    }
    
    auto capabilities = getCameraCapabilities();
    if (!capabilities.supportsBinning) {
        setCameraError("Binning not supported by this camera");
        return false;
    }
    
    QSize oldBinning = d_ptr->currentBinning;
    d_ptr->currentBinning = QSize(horizontal, vertical);
    
    if (oldBinning != d_ptr->currentBinning) {
        emit binningChanged(horizontal, vertical);
    }
    
    return true;
}

bool CameraComponent::setGamma(double gamma) {
    // Validate gamma value
    if (gamma <= 0.0 || gamma > 4.0) {
        setCameraError(QString("Invalid gamma value: %1 (must be between 0.0 and 4.0)").arg(gamma));
        return false;
    }
    
    d_ptr->currentGamma = gamma;
    return true;
}

bool CameraComponent::setWhiteBalance(double red, double green, double blue) {
    // Validate white balance values
    if (red < 0.0 || green < 0.0 || blue < 0.0 || 
        red > 10.0 || green > 10.0 || blue > 10.0) {
        setCameraError("Invalid white balance values (must be between 0.0 and 10.0)");
        return false;
    }
    
    auto capabilities = getCameraCapabilities();
    if (!capabilities.supportsWhiteBalance) {
        setCameraError("White balance not supported by this camera");
        return false;
    }
    
    // Store values in custom features
    QMutexLocker lock(&d_ptr->featureMutex);
    d_ptr->customFeatures["WhiteBalanceRed"] = red;
    d_ptr->customFeatures["WhiteBalanceGreen"] = green;
    d_ptr->customFeatures["WhiteBalanceBlue"] = blue;
    
    return true;
}

double CameraComponent::frameRate() const {
    return d_ptr->currentFrameRate;
}

PixelFormat CameraComponent::pixelFormat() const {
    return d_ptr->currentPixelFormat;
}

QSize CameraComponent::binning() const {
    return d_ptr->currentBinning;
}

double CameraComponent::gamma() const {
    return d_ptr->currentGamma;
}

bool CameraComponent::setAutoExposure(bool enable) {
    auto capabilities = getCameraCapabilities();
    if (enable && !capabilities.supportsAutoExposure) {
        setCameraError("Auto exposure not supported by this camera");
        return false;
    }
    
    d_ptr->autoExposureEnabled = enable;
    return true;
}

bool CameraComponent::setAutoGain(bool enable) {
    auto capabilities = getCameraCapabilities();
    if (enable && !capabilities.supportsAutoGain) {
        setCameraError("Auto gain not supported by this camera");
        return false;
    }
    
    d_ptr->autoGainEnabled = enable;
    return true;
}

bool CameraComponent::setAutoWhiteBalance(bool enable) {
    auto capabilities = getCameraCapabilities();
    if (enable && !capabilities.supportsWhiteBalance) {
        setCameraError("Auto white balance not supported by this camera");
        return false;
    }
    
    d_ptr->autoWhiteBalanceEnabled = enable;
    return true;
}

bool CameraComponent::isAutoExposureEnabled() const {
    return d_ptr->autoExposureEnabled;
}

bool CameraComponent::isAutoGainEnabled() const {
    return d_ptr->autoGainEnabled;
}

bool CameraComponent::setBufferCount(int count) {
    if (count < 1 || count > 1000) {
        setCameraError(QString("Invalid buffer count: %1 (must be between 1 and 1000)").arg(count));
        return false;
    }
    
    auto capabilities = getCameraCapabilities();
    if (count > capabilities.maxBuffers) {
        setCameraError(QString("Buffer count %1 exceeds maximum %2").arg(count).arg(capabilities.maxBuffers));
        return false;
    }
    
    d_ptr->bufferCount = count;
    return true;
}

int CameraComponent::bufferCount() const {
    return d_ptr->bufferCount;
}

bool CameraComponent::clearBuffers() {
    // Default implementation - derived classes should override
    logDebug("Clearing image buffers");
    return true;
}

int CameraComponent::droppedFrameCount() const {
    return d_ptr->droppedFrames.load();
}

double CameraComponent::getCurrentFps() const {
    return d_ptr->fpsCalculator.calculateFps();
}

double CameraComponent::getBandwidth() const {
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - d_ptr->bandwidthStartTime).count();
    
    if (duration <= 0) return 0.0;
    
    qint64 bytes = d_ptr->bytesTransferred.load();
    return (bytes / 1024.0 / 1024.0) / (duration / 1000.0); // MB/s
}

QJsonObject CameraComponent::getStatistics() const {
    QJsonObject stats;
    stats["totalFrames"] = static_cast<qint64>(m_statistics.totalFrames.load());
    stats["droppedFrames"] = static_cast<qint64>(m_statistics.droppedFrames.load());
    stats["currentFps"] = m_statistics.currentFps.load();
    stats["averageFps"] = m_statistics.averageFps.load();
    stats["peakFps"] = m_statistics.peakFps.load();
    stats["bandwidth"] = m_statistics.bandwidth.load();
    stats["acquisitionStartTime"] = m_statistics.acquisitionStartTime;
    stats["lastFrameTime"] = m_statistics.lastFrameTime;
    stats["uptime"] = QDateTime::currentMSecsSinceEpoch() - m_statistics.acquisitionStartTime;
    
    return stats;
}

QJsonObject CameraComponent::getPerformanceMetrics() const {
    QJsonObject metrics = BaseComponent::getPerformanceMetrics();
    
    // Add camera-specific metrics
    metrics["cameraMetrics"] = QJsonObject{
        {"fps", getCurrentFps()},
        {"bandwidth", getBandwidth()},
        {"droppedFrames", droppedFrameCount()},
        {"bufferCount", bufferCount()},
        {"totalFrames", static_cast<qint64>(m_statistics.totalFrames.load())}
    };
    
    return metrics;
}

bool CameraComponent::saveConfiguration(const QString& filePath) const {
    QJsonObject config;
    
    // Save current camera settings
    config["exposureTime"] = exposureTime();
    config["gain"] = gain();
    config["frameRate"] = frameRate();
    config["triggerMode"] = triggerModeToString(triggerMode());
    config["pixelFormat"] = pixelFormatToString(pixelFormat());
    config["binning"] = QJsonObject{
        {"horizontal", binning().width()},
        {"vertical", binning().height()}
    };
    config["roi"] = QJsonObject{
        {"x", roi().x()},
        {"y", roi().y()},
        {"width", roi().width()},
        {"height", roi().height()}
    };
    config["gamma"] = gamma();
    config["autoExposure"] = isAutoExposureEnabled();
    config["autoGain"] = isAutoGainEnabled();
    config["bufferCount"] = bufferCount();
    
    // Save custom features
    QJsonObject customFeatures;
    {
        QMutexLocker lock(&d_ptr->featureMutex);
        for (auto it = d_ptr->customFeatures.begin(); it != d_ptr->customFeatures.end(); ++it) {
            customFeatures[it.key()] = QJsonValue::fromVariant(it.value());
        }
    }
    config["customFeatures"] = customFeatures;
    
    // Write to file
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        logError(QString("Failed to open configuration file: %1").arg(filePath));
        return false;
    }
    
    QJsonDocument doc(config);
    file.write(doc.toJson());
    file.close();
    
    logInfo(QString("Configuration saved to: %1").arg(filePath));
    return true;
}

bool CameraComponent::loadConfiguration(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        setCameraError(QString("Failed to open configuration file: %1").arg(filePath));
        return false;
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    
    if (!doc.isObject()) {
        setCameraError("Invalid configuration file format");
        return false;
    }
    
    QJsonObject config = doc.object();
    
    // Apply settings
    bool success = true;
    
    if (config.contains("exposureTime")) {
        success &= setExposureTime(config["exposureTime"].toDouble());
    }
    
    if (config.contains("gain")) {
        success &= setGain(config["gain"].toDouble());
    }
    
    if (config.contains("frameRate")) {
        success &= setFrameRate(config["frameRate"].toDouble());
    }
    
    if (config.contains("triggerMode")) {
        success &= setTriggerMode(stringToTriggerMode(config["triggerMode"].toString()));
    }
    
    if (config.contains("pixelFormat")) {
        success &= setPixelFormat(stringToPixelFormat(config["pixelFormat"].toString()));
    }
    
    if (config.contains("binning")) {
        QJsonObject binning = config["binning"].toObject();
        success &= setBinning(binning["horizontal"].toInt(1), binning["vertical"].toInt(1));
    }
    
    if (config.contains("roi")) {
        QJsonObject roiObj = config["roi"].toObject();
        QRect roiRect(roiObj["x"].toInt(), roiObj["y"].toInt(),
                     roiObj["width"].toInt(), roiObj["height"].toInt());
        success &= setROI(roiRect);
    }
    
    if (config.contains("gamma")) {
        success &= setGamma(config["gamma"].toDouble());
    }
    
    if (config.contains("autoExposure")) {
        success &= setAutoExposure(config["autoExposure"].toBool());
    }
    
    if (config.contains("autoGain")) {
        success &= setAutoGain(config["autoGain"].toBool());
    }
    
    if (config.contains("bufferCount")) {
        success &= setBufferCount(config["bufferCount"].toInt());
    }
    
    // Load custom features
    if (config.contains("customFeatures")) {
        QJsonObject customFeatures = config["customFeatures"].toObject();
        QMutexLocker lock(&d_ptr->featureMutex);
        for (auto it = customFeatures.begin(); it != customFeatures.end(); ++it) {
            d_ptr->customFeatures[it.key()] = it.value().toVariant();
        }
    }
    
    if (success) {
        logInfo(QString("Configuration loaded from: %1").arg(filePath));
    } else {
        logWarning("Some configuration parameters could not be applied");
    }
    
    return success;
}

bool CameraComponent::setUserDefinedName(const QString& name) {
    d_ptr->userDefinedName = name;
    return true;
}

QString CameraComponent::userDefinedName() const {
    return d_ptr->userDefinedName.isEmpty() ? componentName() : d_ptr->userDefinedName;
}

bool CameraComponent::isFeatureSupported(const QString& feature) const {
    auto capabilities = getCameraCapabilities();
    
    // Check standard features
    if (feature == "HardwareTrigger") return capabilities.supportsHardwareTrigger;
    if (feature == "SoftwareTrigger") return capabilities.supportsSoftwareTrigger;
    if (feature == "ROI") return capabilities.supportsROI;
    if (feature == "Binning") return capabilities.supportsBinning;
    if (feature == "AutoExposure") return capabilities.supportsAutoExposure;
    if (feature == "AutoGain") return capabilities.supportsAutoGain;
    if (feature == "WhiteBalance") return capabilities.supportsWhiteBalance;
    if (feature == "GPU") return capabilities.supportsGPU;
    if (feature == "MultiROI") return capabilities.supportsMultiROI;
    if (feature == "Timestamp") return capabilities.supportsTimestamp;
    if (feature == "Temperature") return capabilities.supportsTemperature;
    
    // Check custom features
    return capabilities.customFeatures.contains(feature);
}

QVariant CameraComponent::getFeatureValue(const QString& feature) const {
    // Check standard features
    if (feature == "ExposureTime") return exposureTime();
    if (feature == "Gain") return gain();
    if (feature == "FrameRate") return frameRate();
    if (feature == "TriggerMode") return QVariant::fromValue(triggerMode());
    if (feature == "PixelFormat") return QVariant::fromValue(pixelFormat());
    if (feature == "Gamma") return gamma();
    if (feature == "AutoExposure") return isAutoExposureEnabled();
    if (feature == "AutoGain") return isAutoGainEnabled();
    if (feature == "BufferCount") return bufferCount();
    
    // Check custom features
    QMutexLocker lock(&d_ptr->featureMutex);
    return d_ptr->customFeatures.value(feature);
}

bool CameraComponent::setFeatureValue(const QString& feature, const QVariant& value) {
    // Set standard features
    if (feature == "ExposureTime") return setExposureTime(value.toDouble());
    if (feature == "Gain") return setGain(value.toDouble());
    if (feature == "FrameRate") return setFrameRate(value.toDouble());
    if (feature == "TriggerMode") return setTriggerMode(value.value<TriggerMode>());
    if (feature == "PixelFormat") return setPixelFormat(value.value<PixelFormat>());
    if (feature == "Gamma") return setGamma(value.toDouble());
    if (feature == "AutoExposure") return setAutoExposure(value.toBool());
    if (feature == "AutoGain") return setAutoGain(value.toBool());
    if (feature == "BufferCount") return setBufferCount(value.toInt());
    
    // Set custom feature
    QMutexLocker lock(&d_ptr->featureMutex);
    d_ptr->customFeatures[feature] = value;
    return true;
}

// ========== Common Methods Implementation ==========

QJsonObject CameraComponent::getCapabilities() const {
    // Convert CameraCapabilities to QJsonObject for BaseComponent interface
    CameraCapabilities caps = getCameraCapabilities();
    
    QJsonObject result;
    
    // Features
    QJsonObject features;
    features["supportsHardwareTrigger"] = caps.supportsHardwareTrigger;
    features["supportsSoftwareTrigger"] = caps.supportsSoftwareTrigger;
    features["supportsROI"] = caps.supportsROI;
    features["supportsBinning"] = caps.supportsBinning;
    features["supportsAutoExposure"] = caps.supportsAutoExposure;
    features["supportsAutoGain"] = caps.supportsAutoGain;
    features["supportsWhiteBalance"] = caps.supportsWhiteBalance;
    features["supportsGPU"] = caps.supportsGPU;
    features["supportsMultiROI"] = caps.supportsMultiROI;
    features["supportsTimestamp"] = caps.supportsTimestamp;
    features["supportsTemperature"] = caps.supportsTemperature;
    result["features"] = features;
    
    // Parameter ranges
    QJsonObject ranges;
    ranges["minExposure"] = caps.minExposure;
    ranges["maxExposure"] = caps.maxExposure;
    ranges["minGain"] = caps.minGain;
    ranges["maxGain"] = caps.maxGain;
    ranges["minFrameRate"] = caps.minFrameRate;
    ranges["maxFrameRate"] = caps.maxFrameRate;
    
    QJsonObject minRes;
    minRes["width"] = caps.minResolution.width();
    minRes["height"] = caps.minResolution.height();
    ranges["minResolution"] = minRes;
    
    QJsonObject maxRes;
    maxRes["width"] = caps.maxResolution.width();
    maxRes["height"] = caps.maxResolution.height();
    ranges["maxResolution"] = maxRes;
    result["ranges"] = ranges;
    
    // Supported formats
    QJsonArray pixelFormats;
    for (const auto& format : caps.pixelFormats) {
        pixelFormats.append(pixelFormatToString(format));
    }
    result["pixelFormats"] = pixelFormats;
    
    QJsonArray triggerModes;
    for (const auto& mode : caps.triggerModes) {
        triggerModes.append(triggerModeToString(mode));
    }
    result["triggerModes"] = triggerModes;
    
    // Custom features
    QJsonArray customFeatures;
    for (const auto& feature : caps.customFeatures) {
        customFeatures.append(feature);
    }
    result["customFeatures"] = customFeatures;
    
    // Performance characteristics
    QJsonObject performance;
    performance["maxBuffers"] = caps.maxBuffers;
    performance["maxBandwidth"] = caps.maxBandwidth;
    performance["zeroCopySupport"] = caps.zeroCopySupport;
    result["performance"] = performance;
    
    return result;
}

CameraState CameraComponent::cameraState() const {
    QMutexLocker lock(&m_cameraStateMutex);
    return m_cameraState;
}

bool CameraComponent::isConnected() const {
    QMutexLocker lock(&m_cameraStateMutex);
    return m_cameraState != CameraState::Disconnected && m_cameraState != CameraState::Error;
}

bool CameraComponent::isAcquiring() const {
    QMutexLocker lock(&m_cameraStateMutex);
    return m_cameraState == CameraState::Acquiring;
}

QString CameraComponent::cameraStateString() const {
    switch (cameraState()) {
        case CameraState::Disconnected: return "Disconnected";
        case CameraState::Connected: return "Connected";
        case CameraState::Acquiring: return "Acquiring";
        case CameraState::Error: return "Error";
        case CameraState::Recovering: return "Recovering";
        default: return "Unknown";
    }
}

bool CameraComponent::enableTimestamp(bool enable) {
    d_ptr->timestampEnabled = enable;
    return true;
}

bool CameraComponent::synchronizeWith(CameraComponent* other) {
    if (!other) {
        setCameraError("Cannot synchronize with null camera");
        return false;
    }
    
    if (other == this) {
        setCameraError("Cannot synchronize with self");
        return false;
    }
    
    // Add to slaves list if this is master
    if (m_isMaster && !m_slaves.contains(other)) {
        m_slaves.append(other);
        other->m_master = this;
        emit slaveConnected(other->componentId());
        other->emit masterStatusChanged(false);
    }
    
    return true;
}

void CameraComponent::setMaster(bool isMaster) {
    if (m_isMaster != isMaster) {
        m_isMaster = isMaster;
        
        if (!isMaster) {
            // Release all slaves
            for (auto* slave : m_slaves) {
                if (slave) {
                    slave->m_master = nullptr;
                    slave->emit synchronizationLost();
                }
            }
            m_slaves.clear();
        }
        
        emit masterStatusChanged(isMaster);
    }
}

bool CameraComponent::isMaster() const {
    return m_isMaster;
}

QString CameraComponent::getLastCameraError() const {
    return m_lastCameraError;
}

bool CameraComponent::recoverFromError() {
    if (cameraState() != CameraState::Error) {
        return true; // Not in error state
    }
    
    setCameraState(CameraState::Recovering);
    
    // Try to reconnect
    if (m_autoReconnect) {
        m_reconnectTimer->start();
        return true;
    }
    
    // Default recovery: reset to disconnected state
    setCameraState(CameraState::Disconnected);
    clearCameraError();
    
    return true;
}

void CameraComponent::setAutoReconnect(bool enable) {
    m_autoReconnect = enable;
    
    if (!enable && m_reconnectTimer->isActive()) {
        m_reconnectTimer->stop();
    }
}

bool CameraComponent::isAutoReconnectEnabled() const {
    return m_autoReconnect;
}

QString CameraComponent::pixelFormatToString(PixelFormat format) const {
    switch (format) {
        case PixelFormat::Mono8: return "Mono8";
        case PixelFormat::Mono10: return "Mono10";
        case PixelFormat::Mono12: return "Mono12";
        case PixelFormat::Mono16: return "Mono16";
        case PixelFormat::RGB24: return "RGB24";
        case PixelFormat::BGR24: return "BGR24";
        case PixelFormat::RGBA32: return "RGBA32";
        case PixelFormat::YUV422: return "YUV422";
        case PixelFormat::BayerRG8: return "BayerRG8";
        case PixelFormat::BayerGB8: return "BayerGB8";
        case PixelFormat::BayerGR8: return "BayerGR8";
        case PixelFormat::BayerBG8: return "BayerBG8";
        case PixelFormat::Custom: return "Custom";
        default: return "Unknown";
    }
}

PixelFormat CameraComponent::stringToPixelFormat(const QString& str) const {
    if (str == "Mono8") return PixelFormat::Mono8;
    if (str == "Mono10") return PixelFormat::Mono10;
    if (str == "Mono12") return PixelFormat::Mono12;
    if (str == "Mono16") return PixelFormat::Mono16;
    if (str == "RGB24") return PixelFormat::RGB24;
    if (str == "BGR24") return PixelFormat::BGR24;
    if (str == "RGBA32") return PixelFormat::RGBA32;
    if (str == "YUV422") return PixelFormat::YUV422;
    if (str == "BayerRG8") return PixelFormat::BayerRG8;
    if (str == "BayerGB8") return PixelFormat::BayerGB8;
    if (str == "BayerGR8") return PixelFormat::BayerGR8;
    if (str == "BayerBG8") return PixelFormat::BayerBG8;
    if (str == "Custom") return PixelFormat::Custom;
    
    return PixelFormat::Mono8; // Default
}

QString CameraComponent::triggerModeToString(TriggerMode mode) const {
    switch (mode) {
        case TriggerMode::FreeRun: return "FreeRun";
        case TriggerMode::Software: return "Software";
        case TriggerMode::Hardware: return "Hardware";
        case TriggerMode::FixedRate: return "FixedRate";
        case TriggerMode::Burst: return "Burst";
        default: return "Unknown";
    }
}

TriggerMode CameraComponent::stringToTriggerMode(const QString& str) const {
    if (str == "FreeRun") return TriggerMode::FreeRun;
    if (str == "Software") return TriggerMode::Software;
    if (str == "Hardware") return TriggerMode::Hardware;
    if (str == "FixedRate") return TriggerMode::FixedRate;
    if (str == "Burst") return TriggerMode::Burst;
    
    return TriggerMode::FreeRun; // Default
}

// ========== Protected Methods ==========

void CameraComponent::setCameraState(CameraState state) {
    QMutexLocker lock(&m_cameraStateMutex);
    
    if (m_cameraState == state) {
        return; // No change
    }
    
    if (!canTransitionTo(state)) {
        logWarning(QString("Invalid state transition from %1 to %2")
                  .arg(cameraStateString())
                  .arg(static_cast<int>(state)));
        return;
    }
    
    CameraState oldState = m_cameraState;
    m_cameraState = state;
    lock.unlock();
    
    // Emit signals
    emit cameraStateChanged(state, oldState);
    
    switch (state) {
        case CameraState::Connected:
            emit connectionStateChanged(true);
            m_statisticsTimer->start();
            break;
            
        case CameraState::Disconnected:
            emit connectionStateChanged(false);
            m_statisticsTimer->stop();
            m_frameTimeoutTimer->stop();
            d_ptr->fpsCalculator.reset();
            break;
            
        case CameraState::Acquiring:
            emit acquisitionStateChanged(true);
            m_statistics.acquisitionStartTime = QDateTime::currentMSecsSinceEpoch();
            m_frameTimeoutTimer->start();
            break;
            
        case CameraState::Error:
            emit cameraError(m_lastCameraError);
            if (m_autoReconnect) {
                m_reconnectTimer->start();
            }
            break;
            
        case CameraState::Recovering:
            logInfo("Attempting to recover from error state");
            break;
            
        default:
            break;
    }
    
    // Stop acquisition if transitioning away from Acquiring
    if (oldState == CameraState::Acquiring && state != CameraState::Acquiring) {
        emit acquisitionStateChanged(false);
        m_frameTimeoutTimer->stop();
    }
}

bool CameraComponent::canTransitionTo(CameraState newState) const {
    // Define valid state transitions
    switch (m_cameraState) {
        case CameraState::Disconnected:
            return newState == CameraState::Connected || 
                   newState == CameraState::Error;
            
        case CameraState::Connected:
            return newState == CameraState::Acquiring || 
                   newState == CameraState::Disconnected ||
                   newState == CameraState::Error;
            
        case CameraState::Acquiring:
            return newState == CameraState::Connected || 
                   newState == CameraState::Error ||
                   newState == CameraState::Disconnected;
            
        case CameraState::Error:
            return newState == CameraState::Recovering || 
                   newState == CameraState::Disconnected ||
                   newState == CameraState::Connected;
            
        case CameraState::Recovering:
            return newState == CameraState::Connected || 
                   newState == CameraState::Disconnected ||
                   newState == CameraState::Error;
            
        default:
            return false;
    }
}

void CameraComponent::setCameraError(const QString& error) {
    m_lastCameraError = error;
    
    if (!error.isEmpty()) {
        logError(error);
        emit cameraError(error);
        
        // Transition to error state if not already there
        if (cameraState() != CameraState::Error && cameraState() != CameraState::Recovering) {
            setCameraState(CameraState::Error);
        }
    }
}

void CameraComponent::clearCameraError() {
    m_lastCameraError.clear();
}

void CameraComponent::processFrame(const ImageBuffer& buffer) {
    if (!d_ptr->processingEnabled) {
        return;
    }
    
    // Update statistics
    updateFrameStatistics(buffer.metadata());
    
    // Update FPS calculator
    d_ptr->fpsCalculator.addFrame(buffer.metadata().timestamp);
    
    // Update bandwidth
    d_ptr->bytesTransferred += buffer.size();
    
    // Reset frame timeout
    if (m_frameTimeoutTimer->isActive()) {
        m_frameTimeoutTimer->stop();
        m_frameTimeoutTimer->start();
    }
    
    // Handle synchronization
    if (d_ptr->timestampEnabled) {
        handleSynchronization(buffer.metadata().timestamp);
    }
    
    // Emit raw frame signal
    emit rawFrameReady(buffer);
    
    // Convert to QImage and emit if needed
    QImage image = convertToQImage(buffer);
    if (!image.isNull()) {
        emit frameReady(image, buffer.metadata());
    }
}

QImage CameraComponent::convertToQImage(const ImageBuffer& buffer) {
    return buffer.toQImage();
}

void CameraComponent::updateFrameStatistics(const FrameMetadata& metadata) {
    m_statistics.totalFrames++;
    m_statistics.lastFrameTime = metadata.timestamp;
    
    // Calculate current FPS
    double fps = d_ptr->fpsCalculator.calculateFps();
    m_statistics.currentFps = fps;
    
    // Update average FPS
    if (m_statistics.totalFrames > 1) {
        qint64 timeDiff = QDateTime::currentMSecsSinceEpoch() - m_statistics.acquisitionStartTime;
        if (timeDiff > 0) {
            m_statistics.averageFps = (m_statistics.totalFrames * 1000.0) / timeDiff;
        }
    }
    
    // Update peak FPS
    if (fps > m_statistics.peakFps) {
        m_statistics.peakFps = fps;
    }
    
    // Update bandwidth
    m_statistics.bandwidth = getBandwidth();
}

void CameraComponent::updatePerformanceMetrics() {
    BaseComponent::updatePerformanceMetrics();
    
    // Emit FPS update
    emit fpsUpdated(getCurrentFps());
    
    // Check bandwidth limits
    auto capabilities = getCameraCapabilities();
    double currentBandwidth = getBandwidth();
    if (currentBandwidth > capabilities.maxBandwidth) {
        emit bandwidthExceeded(currentBandwidth, capabilities.maxBandwidth);
        emit performanceWarning(QString("Bandwidth exceeded: %1 MB/s (max: %2 MB/s)")
                               .arg(currentBandwidth)
                               .arg(capabilities.maxBandwidth));
    }
}

void CameraComponent::handleSynchronization(qint64 timestamp) {
    if (!m_isMaster) {
        return;
    }
    
    // Broadcast timestamp to slaves
    for (auto* slave : m_slaves) {
        if (slave) {
            // Slaves should use this timestamp for synchronization
            QMetaObject::invokeMethod(slave, [slave, timestamp]() {
                slave->handleSynchronization(timestamp);
            }, Qt::QueuedConnection);
        }
    }
}

// ========== Protected Slots ==========

void CameraComponent::onFrameTimeout() {
    logWarning("Frame acquisition timeout - no frames received for 5 seconds");
    emit performanceWarning("Frame acquisition timeout");
    
    // Increment dropped frame counter
    m_statistics.droppedFrames++;
    d_ptr->droppedFrames++;
    
    // Try to recover if in acquisition mode
    if (isAcquiring()) {
        // Restart timeout timer
        m_frameTimeoutTimer->start();
    }
}

void CameraComponent::onReconnectTimer() {
    if (cameraState() != CameraState::Error && cameraState() != CameraState::Recovering) {
        m_reconnectTimer->stop();
        return;
    }
    
    m_reconnectAttempts++;
    
    if (m_reconnectAttempts > 10) {
        logError("Maximum reconnection attempts reached");
        m_reconnectTimer->stop();
        m_reconnectAttempts = 0;
        return;
    }
    
    logInfo(QString("Reconnection attempt %1/10").arg(m_reconnectAttempts));
    
    // Try to reconnect using the last known device
    if (!m_currentDevice.serialNumber.isEmpty()) {
        if (connectCamera(m_currentDevice.serialNumber)) {
            logInfo("Reconnection successful");
            m_reconnectTimer->stop();
            m_reconnectAttempts = 0;
            emit recovered();
        }
    }
}

void CameraComponent::onStatisticsUpdate() {
    // Update performance metrics
    updatePerformanceMetrics();
    
    // Emit statistics signal
    emit performanceMetricsUpdated(getPerformanceMetrics());
    
    // Check for dropped frames
    int dropped = droppedFrameCount();
    if (dropped > 0) {
        static int lastDropped = 0;
        if (dropped > lastDropped) {
            emit frameDropped(m_statistics.totalFrames.load(), 
                            QString("Total dropped: %1").arg(dropped));
            lastDropped = dropped;
        }
    }
    
    // Monitor temperature if supported
    auto capabilities = getCameraCapabilities();
    if (capabilities.supportsTemperature) {
        // This would typically query the camera for temperature
        // For now, we'll use a placeholder
        double temperature = 25.0; // Placeholder
        if (temperature > 70.0) {
            emit temperatureWarning(temperature);
        }
    }
}

// ========== ImageBuffer Implementation ==========

QImage ImageBuffer::toQImage() const {
    if (!m_data || m_size == 0) {
        return QImage();
    }
    
    QImage::Format format = QImage::Format_Invalid;
    int width = m_metadata.imageSize.width();
    int height = m_metadata.imageSize.height();
    
    // Determine QImage format based on pixel format
    switch (m_metadata.pixelFormat) {
        case PixelFormat::Mono8:
            format = QImage::Format_Grayscale8;
            break;
            
        case PixelFormat::Mono16:
            format = QImage::Format_Grayscale16;
            break;
            
        case PixelFormat::RGB24:
            format = QImage::Format_RGB888;
            break;
            
        case PixelFormat::BGR24:
            format = QImage::Format_BGR888;
            break;
            
        case PixelFormat::RGBA32:
            format = QImage::Format_RGBA8888;
            break;
            
        default:
            // For unsupported formats, try to convert to Mono8
            format = QImage::Format_Grayscale8;
            break;
    }
    
    if (format == QImage::Format_Invalid) {
        return QImage();
    }
    
    // Create QImage from data
    // Note: QImage takes ownership of the data if we use the constructor with cleanupFunction
    // For safety, we'll make a copy
    QImage image(width, height, format);
    
    if (image.sizeInBytes() == static_cast<qsizetype>(m_size)) {
        memcpy(image.bits(), m_data, m_size);
    } else {
        // Size mismatch - try to copy what we can
        qsizetype bytesToCopy = qMin(image.sizeInBytes(), static_cast<qsizetype>(m_size));
        memcpy(image.bits(), m_data, bytesToCopy);
    }
    
    return image;
}

QByteArray ImageBuffer::toByteArray() const {
    if (!m_data || m_size == 0) {
        return QByteArray();
    }
    
    return QByteArray(static_cast<const char*>(m_data), static_cast<int>(m_size));
}

} // namespace ComponentsForest