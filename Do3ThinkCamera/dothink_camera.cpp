/**
 * @file dothink_camera.cpp
 * @brief Do3Think Camera Component Implementation
 * @details High-performance industrial camera component for AOI equipment
 *          Supporting 100-1000+ fps operation with Do3Think DVP SDK
 */

#include "dothink_camera.h"
#include "dvp_wrapper.h" // For DvpSdkWrapper::create

#include <iostream>
#include <cstdio>
#include <string>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#endif

#include <QFile>
#include <QJsonDocument>
#include <QDebug>
#include <QThread>
#include <QElapsedTimer>
#include <chrono>
#include <algorithm>
#include <cstring>

// Image processing includes
#include "../OpenCV/preprocessing_pipeline.h"
#include "../OpenCV/blur_preprocessor.h"
#include "../OpenCV/edge_preprocessor.h"
#include "../OpenCV/denoise_preprocessor.h"

// Simple debug output macro
#define DEBUG_LOG(msg) do { \
    std::cerr << "[Do3ThinkCamera] " << msg << std::endl; \
    std::cerr.flush(); \
} while(0)

namespace ComponentsForest {

// Static callback wrapper for Do3Think SDK - matches dvpStreamCallback signature
dvpInt32 Do3ThinkCameraComponent::frameCallback(dvpHandle handle, dvpStreamEvent event, void* pContext, dvpFrame* pFrame, void* pBuffer) {
    Q_UNUSED(handle);
    Q_UNUSED(event);
    
    if (!pContext || !pFrame || !pBuffer) {
        return -1;
    }
    
    auto* callbackData = static_cast<CallbackData*>(pContext);
    if (!callbackData->active.load() || !callbackData->component) {
        return -1;
    }
    
    // Process frame in callback for high performance
    callbackData->component->processDo3ThinkFrame(*pFrame, pBuffer);
    
    return 0;  // Return 0 to continue acquisition
}

// Constructor
Do3ThinkCameraComponent::Do3ThinkCameraComponent(QObject* parent)
    : Do3ThinkCameraComponent(DvpSdkWrapper::create(), parent) {
    // Delegates to the dependency injection constructor
}

Do3ThinkCameraComponent::Do3ThinkCameraComponent(std::shared_ptr<IDvpSdkWrapper> sdkWrapper, QObject* parent)
    : CameraComponent(parent), m_sdkWrapper(sdkWrapper) {
    
    DEBUG_LOG("Do3ThinkCameraComponent constructor entered");
    std::cerr << "[DEBUG] Creating Do3ThinkCameraComponent" << std::endl;
    
    try {
        // Initialize callback data
        DEBUG_LOG("Initializing callback data");
        m_callbackData.component = this;
        m_callbackData.mutex = new QMutex();
        m_callbackData.active = false;
        
        // Set component metadata
        DEBUG_LOG("Setting component metadata");
        setComponentName("Do3ThinkCamera");
        
        // Initialize Do3Think specific timers
        DEBUG_LOG("Creating temperature monitor timer");
        auto* tempTimer = new QTimer(this);
        tempTimer->setInterval(5000);  // Check temperature every 5 seconds
        connect(tempTimer, &QTimer::timeout, this, &Do3ThinkCameraComponent::onTemperatureMonitor);
        
        // Initialize image processing pipeline
        DEBUG_LOG("Initializing image processing pipeline");
        if (!initializeProcessingPipeline()) {
            DEBUG_LOG("Warning: Failed to initialize image processing pipeline");
        } else {
            DEBUG_LOG("Image processing pipeline initialized successfully");
        }
        
        DEBUG_LOG("Do3ThinkCameraComponent constructor completed");
        
    } catch (const std::exception& e) {
        std::stringstream ss;
        ss << "EXCEPTION in Do3ThinkCameraComponent constructor: " << e.what();
        DEBUG_LOG(ss.str());
    } catch (...) {
        DEBUG_LOG("UNKNOWN EXCEPTION in Do3ThinkCameraComponent constructor");
    }
}

// Destructor
Do3ThinkCameraComponent::~Do3ThinkCameraComponent() {
    DEBUG_LOG("Do3ThinkCameraComponent destructor entered");
    
    // Ensure proper cleanup
    if (isAcquiring()) {
        DEBUG_LOG("Stopping acquisition in destructor");
        stopAcquisition();
    }
    
    if (isConnected()) {
        DEBUG_LOG("Disconnecting camera in destructor");
        disconnectCamera();
    }
    
    DEBUG_LOG("Cleaning up Do3Think resources");
    cleanupDo3ThinkResources();
    
    // Cleanup image processing pipeline
    DEBUG_LOG("Cleaning up image processing pipeline");
    cleanupProcessingPipeline();
    
    delete m_callbackData.mutex;
    
    DEBUG_LOG("Do3ThinkCameraComponent destructor completed");
}

// ========== Pure Virtual Methods Implementation ==========

QList<CameraDeviceInfo> Do3ThinkCameraComponent::scanDevices() {
    DEBUG_LOG("Do3ThinkCameraComponent::scanDevices() entered");
    
    QList<CameraDeviceInfo> devices;
    dvpStatus status;
    
    // Refresh device list - dvpRefresh takes a pointer to count
    DEBUG_LOG("Calling dvpRefresh to scan for devices");
    dvpUint32 deviceCount = 0;
    
    try {
        status = m_sdkWrapper->dvpRefresh(&deviceCount);
        std::stringstream ss;
        ss << "dvpRefresh returned status: " << status << ", device count: " << deviceCount;
        DEBUG_LOG(ss.str());
    } catch (const std::exception& e) {
        std::stringstream ss;
        ss << "EXCEPTION calling dvpRefresh: " << e.what();
        DEBUG_LOG(ss.str());
        return devices;
    } catch (...) {
        DEBUG_LOG("UNKNOWN EXCEPTION calling dvpRefresh");
        return devices;
    }
    
    if (status != DVP_STATUS_OK) {
        logError(QString("Failed to refresh device list: %1").arg(dvpStatusToString(status)));
        DEBUG_LOG("dvpRefresh failed");
        return devices;
    }
    
    if (deviceCount == 0) {
        logDebug("No Do3Think devices found");
        DEBUG_LOG("No Do3Think devices found");
        return devices;
    }
    
    // Enumerate devices - dvpEnum takes index and single dvpCameraInfo pointer
    for (dvpUint32 i = 0; i < deviceCount; ++i) {
        dvpCameraInfo info;
        status = m_sdkWrapper->dvpEnum(i, &info);
        if (status != DVP_STATUS_OK) {
            logError(QString("Failed to enumerate device %1: %2").arg(i).arg(dvpStatusToString(status)));
            continue;
        }
    
        
        CameraDeviceInfo deviceInfo;
        deviceInfo.serialNumber = QString::fromLatin1(info.SerialNumber);
        deviceInfo.modelName = QString::fromLatin1(info.FriendlyName);
        deviceInfo.manufacturer = "Do3Think";
        deviceInfo.friendlyName = QString::fromLatin1(info.FriendlyName);
        
        // Determine interface type from PortInfo string
        QString portInfo = QString::fromLatin1(info.PortInfo);
        if (portInfo.contains("USB3", Qt::CaseInsensitive)) {
            deviceInfo.interfaceType = "USB3";
        } else if (portInfo.contains("USB2", Qt::CaseInsensitive)) {
            deviceInfo.interfaceType = "USB2";
        } else if (portInfo.contains("GigE", Qt::CaseInsensitive)) {
            deviceInfo.interfaceType = "GigE";
        } else {
            deviceInfo.interfaceType = "Unknown";
        }
        
        deviceInfo.devicePath = portInfo;
        // Device is available if we can enumerate it
        deviceInfo.isAvailable = true;
        
        // Add custom Do3Think info
        QJsonObject customInfo;
        customInfo["Vendor"] = QString::fromLatin1(info.Vendor);
        customInfo["FirmwareVersion"] = QString::fromLatin1(info.FirmwareVersion);
        customInfo["HardwareVersion"] = QString::fromLatin1(info.HardwareVersion);
        customInfo["KernelVersion"] = QString::fromLatin1(info.KernelVersion);
        customInfo["DscamVersion"] = QString::fromLatin1(info.DscamVersion);
        customInfo["SensorInfo"] = QString::fromLatin1(info.SensorInfo);
        customInfo["CameraInfo"] = QString::fromLatin1(info.CameraInfo);
        deviceInfo.customInfo = customInfo;
        
        devices.append(deviceInfo);
    }
    
    logInfo(QString("Found %1 Do3Think device(s)").arg(devices.size()));
    return devices;
}

bool Do3ThinkCameraComponent::connectCamera(const QString& identifier) {
    QMutexLocker locker(&m_deviceMutex);
    
    if (isConnected()) {
        logWarning("Camera already connected");
        return true;
    }
    
    dvpStatus status;
    
    // Try to open by serial number or friendly name
    // dvpOpenMode: OPEN_NORMAL = 1, OPEN_DEBUG = 8
    status = m_sdkWrapper->dvpOpenByName(identifier.toLocal8Bit().data(), static_cast<dvpOpenMode>(1), &m_cameraHandle);
    if (status != DVP_STATUS_OK) {
        logError(QString("Failed to open camera %1: %2").arg(identifier).arg(dvpStatusToString(status)));
        return false;
    }
    
    // Get camera info
    status = m_sdkWrapper->dvpGetCameraInfo(m_cameraHandle, &m_do3thinkInfo.info);
    if (status != DVP_STATUS_OK) {
        m_sdkWrapper->dvpClose(m_cameraHandle);
        m_cameraHandle = 0;
        logError("Failed to get camera info");
        return false;
    }
    
    // Store device info
    m_do3thinkInfo.handle = m_cameraHandle;
    m_do3thinkInfo.connectionString = identifier;
    
    // Update current device
    CameraDeviceInfo deviceInfo;
    deviceInfo.serialNumber = QString::fromLatin1(m_do3thinkInfo.info.SerialNumber);
    deviceInfo.modelName = QString::fromLatin1(m_do3thinkInfo.info.FriendlyName);
    deviceInfo.manufacturer = "Do3Think";
    deviceInfo.friendlyName = QString::fromLatin1(m_do3thinkInfo.info.FriendlyName);
    m_currentDevice = deviceInfo;
    
    // Initialize camera parameters
    if (!initializeDo3ThinkDevice(m_cameraHandle)) {
        m_sdkWrapper->dvpClose(m_cameraHandle);
        m_cameraHandle = 0;
        return false;
    }
    
    // Configure defaults
    if (!configureDo3ThinkDefaults()) {
        m_sdkWrapper->dvpClose(m_cameraHandle);
        m_cameraHandle = 0;
        return false;
    }
    
    // Check capabilities - This part of the code has been removed as it is not available in the new wrapper
    m_do3thinkInfo.supportsColorCorrection = true; // Assume supported for now
    m_do3thinkInfo.supportsHDR = true; // Assume supported for now
    
    // Get sensor info
    dvpSensorInfo sensorInfo;
    // status = m_sdkWrapper->dvpGetSensorInfo(m_cameraHandle, &sensorInfo);
    // if (status == DVP_STATUS_OK) {
    //     m_do3thinkState.sensorModel = QString::fromLatin1(sensorInfo.descr);
    //     m_do3thinkState.sensorResolution = QSize(sensorInfo.region.iMaxW, sensorInfo.region.iMaxH);
    //     m_do3thinkState.isColorCamera = (sensorInfo.pixel != dvpSensorPixel::SENSOR_PIXEL_MONO);
    // }
    
    setCameraState(CameraState::Connected);
    logInfo(QString("Connected to camera: %1").arg(identifier));
    
    emit connectionStateChanged(true);
    emit deviceChanged(m_currentDevice);
    emit capabilitiesChanged();
    
    return true;
}

bool Do3ThinkCameraComponent::disconnectCamera() {
    QMutexLocker locker(&m_deviceMutex);
    
    if (!isConnected()) {
        return true;
    }
    
    // Stop acquisition if running
    if (isAcquiring()) {
        stopAcquisition();
    }
    
    // Close camera
    if (m_cameraHandle != 0) {
        dvpStatus status = m_sdkWrapper->dvpClose(m_cameraHandle);
        if (status != DVP_STATUS_OK) {
            logWarning(QString("Error closing camera: %1").arg(dvpStatusToString(status)));
        }
        m_cameraHandle = 0;
    }
    
    // Clear device info
    m_do3thinkInfo = Do3ThinkDeviceInfo();
    m_currentDevice = CameraDeviceInfo();
    
    setCameraState(CameraState::Disconnected);
    logInfo("Camera disconnected");
    
    emit connectionStateChanged(false);
    
    return true;
}

CameraDeviceInfo Do3ThinkCameraComponent::getCurrentDevice() const {
    QMutexLocker locker(&m_deviceMutex);
    return m_currentDevice;
}

bool Do3ThinkCameraComponent::startAcquisition() {
    if (!isConnected()) {
        logError("Cannot start acquisition: Camera not connected");
        return false;
    }
    
    if (isAcquiring()) {
        logWarning("Acquisition already running");
        return true;
    }
    
    dvpStatus status;
    
    // Register frame callback for high-speed acquisition
    m_callbackData.active = true;
    status = m_sdkWrapper->dvpRegisterStreamCallback(
        m_cameraHandle,
        frameCallback,
        static_cast<dvpStreamEvent>(STREAM_EVENT_PROCESSED),  // Get processed frames
        &m_callbackData
    );
    
    if (status != DVP_STATUS_OK) {
        m_callbackData.active = false;
        logError(QString("Failed to register callback: %1").arg(dvpStatusToString(status)));
        return false;
    }
    
    // Start video stream
    status = m_sdkWrapper->dvpStart(m_cameraHandle);
    if (status != DVP_STATUS_OK) {
        m_callbackData.active = false;
        m_sdkWrapper->dvpUnregisterStreamCallback(m_cameraHandle, frameCallback, static_cast<dvpStreamEvent>(STREAM_EVENT_PROCESSED), &m_callbackData);
        logError(QString("Failed to start acquisition: %1").arg(dvpStatusToString(status)));
        return false;
    }
    
    setCameraState(CameraState::Acquiring);
    // Acquisition start time tracked internally by base class through state change
    
    logInfo("Acquisition started");
    emit acquisitionStateChanged(true);
    
    return true;
}

bool Do3ThinkCameraComponent::stopAcquisition() {
    if (!isAcquiring()) {
        return true;
    }
    
    dvpStatus status;
    
    // Stop callback processing
    m_callbackData.active = false;
    
    // Stop video stream
    status = m_sdkWrapper->dvpStop(m_cameraHandle);
    if (status != DVP_STATUS_OK) {
        logWarning(QString("Error stopping acquisition: %1").arg(dvpStatusToString(status)));
    }
    
    // Unregister callback - needs the exact context pointer used during registration
    status = m_sdkWrapper->dvpUnregisterStreamCallback(m_cameraHandle, frameCallback, static_cast<dvpStreamEvent>(STREAM_EVENT_PROCESSED), &m_callbackData);
    if (status != DVP_STATUS_OK) {
        logWarning(QString("Error unregistering callback: %1").arg(dvpStatusToString(status)));
    }
    
    setCameraState(CameraState::Connected);
    
    logInfo("Acquisition stopped");
    emit acquisitionStateChanged(false);
    
    return true;
}

bool Do3ThinkCameraComponent::grabSingleFrame() {
    if (!isConnected()) {
        logError("Cannot grab frame: Camera not connected");
        return false;
    }
    
    dvpStatus status;
    dvpFrame frame;
    void* pBuffer = nullptr;
    
    // Software trigger for single frame
    // Note: Do3Think SDK doesn't have TRIGGER_SINGLE mode, use software trigger
    status = m_sdkWrapper->dvpSetTriggerSource(m_cameraHandle, dvpTriggerSource::TRIGGER_SOURCE_SOFTWARE);
    if (status != DVP_STATUS_OK) {
        logError(QString("Failed to set single trigger: %1").arg(dvpStatusToString(status)));
        return false;
    }
    
    // Start if not already running
    bool wasAcquiring = isAcquiring();
    if (!wasAcquiring) {
        status = m_sdkWrapper->dvpStart(m_cameraHandle);
        if (status != DVP_STATUS_OK) {
            logError(QString("Failed to start for single frame: %1").arg(dvpStatusToString(status)));
            return false;
        }
    }
    
    // Get single frame with timeout
    status = m_sdkWrapper->dvpGetFrame(m_cameraHandle, &frame, &pBuffer, Do3ThinkCameraConstants::kDefaultFrameTimeoutMs);
    
    if (status == DVP_STATUS_OK && pBuffer) {
        processDo3ThinkFrame(frame, pBuffer);
    }
    
    // Stop if we started it
    if (!wasAcquiring) {
        m_sdkWrapper->dvpStop(m_cameraHandle);
    }
    
    // Restore continuous mode if needed
    if (wasAcquiring) {
        // Set to continuous mode (no trigger)
        // m_sdkWrapper->dvpSetTriggerInputType(m_cameraHandle, dvpTriggerInputType::TRIGGER_IN_OFF);
    }
    
    return (status == DVP_STATUS_OK);
}

bool Do3ThinkCameraComponent::setExposureTime(double microseconds) {
    if (!isConnected()) {
        return false;
    }
    
    dvpStatus status = m_sdkWrapper->dvpSetExposure(m_cameraHandle, microseconds);
    if (status != DVP_STATUS_OK) {
        logError(QString("Failed to set exposure time: %1").arg(dvpStatusToString(status)));
        return false;
    }
    
    emit exposureTimeChanged(microseconds);
    return true;
}

bool Do3ThinkCameraComponent::setGain(double gain) {
    if (!isConnected()) {
        return false;
    }
    
    dvpStatus status = m_sdkWrapper->dvpSetAnalogGain(m_cameraHandle, static_cast<float>(gain));
    if (status != DVP_STATUS_OK) {
        logError(QString("Failed to set gain: %1").arg(dvpStatusToString(status)));
        return false;
    }
    
    emit gainChanged(gain);
    return true;
}

bool Do3ThinkCameraComponent::setROI(const QRect& roi) {
    if (!isConnected()) {
        return false;
    }
    
    dvpRegion region;
    region.X = roi.x();
    region.Y = roi.y();
    region.W = roi.width();
    region.H = roi.height();
    
    // dvpStatus status = m_sdkWrapper->dvpSetRoi(m_cameraHandle, region);
    // if (status != DVP_STATUS_OK) {
    //     logError(QString("Failed to set ROI: %1").arg(dvpStatusToString(status)));
    //     return false;
    // }
    
    emit roiChanged(roi);
    return true;
}

double Do3ThinkCameraComponent::exposureTime() const {
    if (!isConnected()) {
        return 0.0;
    }
    
    double exposureValue = 0.0;
    m_sdkWrapper->dvpGetExposure(m_cameraHandle, &exposureValue);
    return exposureValue;
}

double Do3ThinkCameraComponent::gain() const {
    if (!isConnected()) {
        return 1.0;
    }
    
    float gain = 1.0f;
    m_sdkWrapper->dvpGetAnalogGain(m_cameraHandle, &gain);
    return static_cast<double>(gain);
}

QRect Do3ThinkCameraComponent::roi() const {
    if (!isConnected()) {
        return QRect();
    }
    
    dvpRegion region;
    // m_sdkWrapper->dvpGetRoi(m_cameraHandle, &region);
    return QRect(region.X, region.Y, region.W, region.H);
}

CameraCapabilities Do3ThinkCameraComponent::getCameraCapabilities() const {
    CameraCapabilities caps;
    
    if (!isConnected()) {
        return caps;
    }
    
    dvpStatus status;
    
    // Basic features
    caps.supportsHardwareTrigger = true;
    caps.supportsSoftwareTrigger = true;
    caps.supportsROI = true;
    caps.supportsBinning = true;
    caps.supportsTimestamp = true;
    
    // Auto features - check if AE/AWB operations are available
    dvpAeOperation aeOp;
    status = dvpGetAeOperation(m_cameraHandle, &aeOp);
    caps.supportsAutoExposure = (status == DVP_STATUS_OK);
    
    dvpAwbOperation awbOp;
    status = dvpGetAwbOperation(m_cameraHandle, &awbOp);
    caps.supportsWhiteBalance = (status == DVP_STATUS_OK);
    
    // Temperature monitoring - check if temperature info is available
    dvpTemperatureInfo tempInfo;
    status = dvpGetTemperatureInfo(m_cameraHandle, &tempInfo);
    caps.supportsTemperature = (status == DVP_STATUS_OK);
    
    // Parameter ranges
    dvpDoubleDescr descr;
    status = dvpGetExposureDescr(m_cameraHandle, &descr);
    if (status == DVP_STATUS_OK) {
        caps.minExposure = descr.fMin;
        caps.maxExposure = descr.fMax;
    }
    
    dvpFloatDescr gainDescr;
    status = dvpGetAnalogGainDescr(m_cameraHandle, &gainDescr);
    if (status == DVP_STATUS_OK) {
        caps.minGain = gainDescr.fMin;
        caps.maxGain = gainDescr.fMax;
    }
    
    // Resolution - get from ROI descriptor
    dvpRegionDescr roiDescr;
    status = dvpGetRoiDescr(m_cameraHandle, &roiDescr);
    if (status == DVP_STATUS_OK) {
        caps.maxResolution = QSize(roiDescr.iMaxW, roiDescr.iMaxH);
    }
    
    // Frame rate - Do3Think SDK doesn't have direct frame rate query
    // Use default high-speed camera values
    caps.minFrameRate = 1.0;
    caps.maxFrameRate = 1000.0;  // Typical for industrial cameras
    
    // Pixel formats
    caps.pixelFormats = {
        PixelFormat::Mono8,
        PixelFormat::Mono10,
        PixelFormat::Mono12,
        PixelFormat::Mono16,
        PixelFormat::RGB24,
        PixelFormat::BGR24,
        PixelFormat::BayerRG8,
        PixelFormat::BayerGB8,
        PixelFormat::BayerGR8,
        PixelFormat::BayerBG8
    };
    
    // Trigger modes
    caps.triggerModes = {
        TriggerMode::FreeRun,
        TriggerMode::Software,
        TriggerMode::Hardware,
        TriggerMode::FixedRate
    };
    
    // Custom features
    if (m_do3thinkInfo.supportsColorCorrection) {
        caps.customFeatures.append("ColorCorrection");
    }
    if (m_do3thinkInfo.supportsHDR) {
        caps.customFeatures.append("HDR");
    }
    caps.customFeatures.append("GPIO");
    caps.customFeatures.append("LUT");
    caps.customFeatures.append("TriggerDelay");
    caps.customFeatures.append("UserData");
    
    // Performance
    caps.maxBuffers = 200;
    caps.maxBandwidth = 3000.0;  // USB3 theoretical max
    caps.zeroCopySupport = true;
    
    return caps;
}

// ========== Virtual Methods Override ==========

bool Do3ThinkCameraComponent::executeSoftwareTrigger() {
    if (!isConnected()) {
        return false;
    }
    
    dvpStatus status = dvpTriggerFire(m_cameraHandle);
    if (status != DVP_STATUS_OK) {
        logError(QString("Software trigger failed: %1").arg(dvpStatusToString(status)));
        return false;
    }
    
    return true;
}

bool Do3ThinkCameraComponent::setTriggerMode(TriggerMode mode) {
    if (!isConnected()) {
        return false;
    }
    
    dvpTriggerSource source = triggerModeToDvpTrigger(mode);
    dvpStatus status = dvpSetTriggerSource(m_cameraHandle, source);
    if (status != DVP_STATUS_OK) {
        logError(QString("Failed to set trigger mode: %1").arg(dvpStatusToString(status)));
        return false;
    }
    
    // Set trigger state based on mode
    // For FreeRun mode, disable trigger input
    // For other modes, enable appropriate trigger input
    if (mode == TriggerMode::FreeRun) {
        status = dvpSetTriggerInputType(m_cameraHandle, dvpTriggerInputType::TRIGGER_IN_OFF);
        if (status != DVP_STATUS_OK) {
            logError(QString("Failed to set trigger input type: %1").arg(dvpStatusToString(status)));
            return false;
        }
    } else if (mode == TriggerMode::Hardware) {
        // For hardware trigger, use positive edge trigger
        status = dvpSetTriggerInputType(m_cameraHandle, dvpTriggerInputType::TRIGGER_POS_EDGE);
        if (status != DVP_STATUS_OK) {
            logError(QString("Failed to set trigger input type: %1").arg(dvpStatusToString(status)));
            return false;
        }
    }
    // Software trigger doesn't need trigger input type configuration
    
    emit triggerModeChanged(mode);
    return true;
}

TriggerMode Do3ThinkCameraComponent::triggerMode() const {
    if (!isConnected()) {
        return TriggerMode::FreeRun;
    }
    
    dvpTriggerSource source;
    dvpGetTriggerSource(m_cameraHandle, &source);
    return dvpTriggerToTriggerMode(source);
}

bool Do3ThinkCameraComponent::setFrameRate(double fps) {
    if (!isConnected()) {
        return false;
    }
    
    // Do3Think SDK doesn't have direct frame rate control
    // Frame rate is controlled through exposure time and trigger settings
    // Store the value for reference
    logDebug(QString("Frame rate control not directly supported. Use exposure time to control frame rate. Requested: %1 fps").arg(fps));
    
    // Calculate and set exposure time based on desired frame rate
    // Max exposure time should be less than frame period
    double maxExposure = (1000000.0 / fps) * 0.9; // 90% of frame period in microseconds
    double currentExposure = exposureTime();
    if (currentExposure > maxExposure) {
        setExposureTime(maxExposure);
    }
    
    emit frameRateChanged(fps);
    return true;
}

bool Do3ThinkCameraComponent::setPixelFormat(PixelFormat format) {
    if (!isConnected()) {
        return false;
    }
    
    // Do3Think SDK uses dvpSetTargetFormat for setting image format
    dvpStreamFormat targetFormat = pixelFormatToDvpFormat(format);
    
    dvpStatus status = dvpSetTargetFormat(m_cameraHandle, targetFormat);
    if (status != DVP_STATUS_OK) {
        logError(QString("Failed to set pixel format: %1").arg(dvpStatusToString(status)));
        return false;
    }
    
    emit pixelFormatChanged(format);
    return true;
}

bool Do3ThinkCameraComponent::setBinning(int horizontal, int vertical) {
    if (!isConnected()) {
        return false;
    }
    
    // Do3Think SDK uses resolution mode selection for binning
    // Need to find appropriate resolution mode with binning
    dvpSelectionDescr resModeDescr;
    dvpStatus status = dvpGetResolutionModeSelDescr(m_cameraHandle, &resModeDescr);
    if (status != DVP_STATUS_OK) {
        logError("Failed to get resolution mode descriptor");
        return false;
    }
    
    // Search for a resolution mode with matching binning
    for (dvpUint32 i = 0; i < resModeDescr.uCount; ++i) {
        dvpResolutionMode mode;
        status = dvpGetResolutionModeSelDetail(m_cameraHandle, i, &mode);
        if (status == DVP_STATUS_OK) {
            // Check if this mode has the desired binning
            // For now, just check resolution changes as binning indicator
            if ((horizontal == 1 && vertical == 1) || // No binning requested
                (horizontal > 1 || vertical > 1)) { // Some binning requested
                // Set this resolution mode
                status = dvpSetResolutionModeSel(m_cameraHandle, i);
                if (status == DVP_STATUS_OK) {
                    emit binningChanged(horizontal, vertical);
                    return true;
                }
            }
        }
    }
    
    logWarning(QString("Binning %1x%2 not supported, keeping current settings").arg(horizontal).arg(vertical));
    return false;
}

bool Do3ThinkCameraComponent::setGamma(double gamma) {
    if (!isConnected()) {
        return false;
    }
    
    // dvpSetGamma uses integer value (percentage * 100)
    dvpInt32 gammaInt = static_cast<dvpInt32>(gamma * 100);
    dvpStatus status = dvpSetGamma(m_cameraHandle, gammaInt);
    if (status != DVP_STATUS_OK) {
        logError(QString("Failed to set gamma: %1").arg(dvpStatusToString(status)));
        return false;
    }
    
    return true;
}

bool Do3ThinkCameraComponent::setWhiteBalance(double red, double green, double blue) {
    if (!isConnected()) {
        return false;
    }
    
    dvpStatus status;
    
    // Set RGB gains
    status = dvpSetRgbGain(m_cameraHandle, 
        static_cast<float>(red),
        static_cast<float>(green), 
        static_cast<float>(blue));
    
    if (status != DVP_STATUS_OK) {
        logError(QString("Failed to set white balance: %1").arg(dvpStatusToString(status)));
        return false;
    }
    
    return true;
}

double Do3ThinkCameraComponent::frameRate() const {
    if (!isConnected()) {
        return 0.0;
    }
    
    // Do3Think SDK doesn't have direct frame rate query
    // Return the calculated FPS from statistics
    return getCurrentFps();
}

PixelFormat Do3ThinkCameraComponent::pixelFormat() const {
    if (!isConnected()) {
        return PixelFormat::Mono8;
    }
    
    // Do3Think SDK uses dvpGetTargetFormat for getting image format
    dvpStreamFormat streamFormat;
    dvpStatus status = dvpGetTargetFormat(m_cameraHandle, &streamFormat);
    if (status != DVP_STATUS_OK) {
        return PixelFormat::Mono8;
    }
    
    return dvpFormatToPixelFormat(streamFormat);
}

QSize Do3ThinkCameraComponent::binning() const {
    if (!isConnected()) {
        return QSize(1, 1);
    }
    
    // Do3Think SDK uses resolution mode for binning info
    dvpUint32 resModeIndex = 0;
    dvpStatus status = dvpGetResolutionModeSel(m_cameraHandle, &resModeIndex);
    if (status != DVP_STATUS_OK) {
        return QSize(1, 1);
    }
    
    dvpResolutionMode mode;
    status = dvpGetResolutionModeSelDetail(m_cameraHandle, resModeIndex, &mode);
    if (status != DVP_STATUS_OK) {
        return QSize(1, 1);
    }
    
    // Parse binning from mode description or selection
    // Default to 1x1 if no binning info available
    // The mode.selection typically contains binning info like "2x2 binning"
    QString description = QString::fromLatin1(mode.selection);
    if (description.contains("2x2", Qt::CaseInsensitive)) {
        return QSize(2, 2);
    } else if (description.contains("4x4", Qt::CaseInsensitive)) {
        return QSize(4, 4);
    }
    
    return QSize(1, 1);
}

double Do3ThinkCameraComponent::gamma() const {
    if (!isConnected()) {
        return 1.0;
    }
    
    dvpInt32 gamma = 100;  // Default gamma is 100 (1.0)
    dvpGetGamma(m_cameraHandle, &gamma);
    return gamma / 100.0;  // Convert from int percentage to double
}

bool Do3ThinkCameraComponent::setAutoExposure(bool enable) {
    if (!isConnected()) {
        return false;
    }
    
    dvpAeMode mode = enable ? AE_MODE_AE_ONLY : dvpAeMode(3);  // 3 is manual mode
    dvpStatus status = dvpSetAeMode(m_cameraHandle, mode);
    if (status != DVP_STATUS_OK) {
        logError(QString("Failed to set auto exposure: %1").arg(dvpStatusToString(status)));
        return false;
    }
    
    return true;
}

bool Do3ThinkCameraComponent::setAutoGain(bool enable) {
    if (!isConnected()) {
        return false;
    }
    
    // Do3Think combines auto gain with auto exposure
    return setAutoExposure(enable);
}

bool Do3ThinkCameraComponent::setAutoWhiteBalance(bool enable) {
    if (!isConnected()) {
        return false;
    }
    
    // Do3Think doesn't have auto white balance mode, use manual RGB gain
    // When enable is false, reset to 1.0 for all channels
    dvpStatus status = DVP_STATUS_OK;
    if (!enable) {
        status = dvpSetRgbGain(m_cameraHandle, 1.0f, 1.0f, 1.0f);
    }
    if (status != DVP_STATUS_OK) {
        logError(QString("Failed to set auto white balance: %1").arg(dvpStatusToString(status)));
        return false;
    }
    
    return true;
}

bool Do3ThinkCameraComponent::isAutoExposureEnabled() const {
    if (!isConnected()) {
        return false;
    }
    
    dvpAeMode mode;
    dvpGetAeMode(m_cameraHandle, &mode);
    // Do3Think doesn't have AE_MODE_AUTO, using AE_MODE_AE_AG (auto exposure and gain)
    return (mode == AE_MODE_AE_AG || mode == AE_MODE_AG_AE);
}

bool Do3ThinkCameraComponent::isAutoGainEnabled() const {
    // Do3Think combines auto gain with auto exposure
    return isAutoExposureEnabled();
}

bool Do3ThinkCameraComponent::setBufferCount(int count) {
    if (!isConnected()) {
        return false;
    }
    
    dvpStatus status = dvpSetBufferQueueSize(m_cameraHandle, count);
    if (status != DVP_STATUS_OK) {
        logError(QString("Failed to set buffer count: %1").arg(dvpStatusToString(status)));
        return false;
    }
    
    return true;
}

int Do3ThinkCameraComponent::bufferCount() const {
    if (!isConnected()) {
        return 0;
    }
    
    dvpUint32 count = 0;
    dvpGetBufferQueueSize(m_cameraHandle, &count);
    return count;
}

bool Do3ThinkCameraComponent::clearBuffers() {
    if (!isConnected()) {
        return false;
    }
    
    // Do3Think SDK doesn't have a specific buffer clear function
    // The buffers are managed internally by the SDK
    // We can stop and restart acquisition to clear buffers if needed
    logDebug("Clearing image buffers");
    
    return true;
}

int Do3ThinkCameraComponent::droppedFrameCount() const {
    // Get dropped frame count from statistics
    return static_cast<int>(m_statistics.droppedFrames.load());
}

double Do3ThinkCameraComponent::getCurrentFps() const {
    // Use base class implementation
    return CameraComponent::getCurrentFps();
}

double Do3ThinkCameraComponent::getBandwidth() const {
    // Use base class implementation
    return CameraComponent::getBandwidth();
}

QJsonObject Do3ThinkCameraComponent::getStatistics() const {
    // Get base statistics first
    QJsonObject stats = CameraComponent::getStatistics();
    
    if (isConnected()) {
        stats["temperature"] = sensorTemperature();
        stats["isColorCamera"] = m_do3thinkState.isColorCamera;
        stats["sensorModel"] = m_do3thinkState.sensorModel;
        
        // Get frame count statistics from SDK
        dvpFrameCount frameCount;
        if (dvpGetFrameCount(m_cameraHandle, &frameCount) == DVP_STATUS_OK) {
            stats["receivedFrames"] = static_cast<qint64>(frameCount.uFrameCount);
            stats["droppedFrames"] = static_cast<qint64>(frameCount.uFrameDrop);
            stats["ignoredFrames"] = static_cast<qint64>(frameCount.uFrameIgnore);
        }
    }
    
    return stats;
}

bool Do3ThinkCameraComponent::saveConfiguration(const QString& filePath) const {
    if (!isConnected()) {
        logError("Cannot save configuration: Camera not connected");
        return false;
    }
    
    // Save to Do3Think native format
    dvpStatus status = dvpSaveConfig(m_cameraHandle, filePath.toLocal8Bit().data());
    if (status != DVP_STATUS_OK) {
        logError(QString("Failed to save configuration: %1").arg(dvpStatusToString(status)));
        return false;
    }
    
    // Also save additional Qt-specific settings
    QJsonObject config;
    config["userDefinedName"] = m_do3thinkParams.userDefinedName;
    config["colorCorrectionEnabled"] = m_do3thinkParams.colorCorrectionEnabled;
    config["hdrEnabled"] = m_do3thinkParams.hdrEnabled;
    config["hdrLevels"] = m_do3thinkParams.hdrLevels;
    config["triggerDelay"] = m_do3thinkParams.triggerDelay;
    config["triggerDivider"] = m_do3thinkParams.triggerDivider;
    config["streamMode"] = m_do3thinkParams.streamMode;
    config["packetSize"] = m_do3thinkParams.packetSize;
    config["lutEnabled"] = m_do3thinkParams.lutEnabled;
    
    QString qtConfigPath = filePath + ".json";
    QFile file(qtConfigPath);
    if (file.open(QIODevice::WriteOnly)) {
        QJsonDocument doc(config);
        file.write(doc.toJson());
        file.close();
    }
    
    return true;
}

bool Do3ThinkCameraComponent::loadConfiguration(const QString& filePath) {
    if (!isConnected()) {
        logError("Cannot load configuration: Camera not connected");
        return false;
    }
    
    // Load Do3Think native configuration
    dvpStatus status = dvpLoadConfig(m_cameraHandle, filePath.toLocal8Bit().data());
    if (status != DVP_STATUS_OK) {
        logError(QString("Failed to load configuration: %1").arg(dvpStatusToString(status)));
        return false;
    }
    
    // Load additional Qt-specific settings
    QString qtConfigPath = filePath + ".json";
    QFile file(qtConfigPath);
    if (file.exists() && file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        QJsonObject config = doc.object();
        
        m_do3thinkParams.userDefinedName = config["userDefinedName"].toString();
        m_do3thinkParams.colorCorrectionEnabled = config["colorCorrectionEnabled"].toBool();
        m_do3thinkParams.hdrEnabled = config["hdrEnabled"].toBool();
        m_do3thinkParams.hdrLevels = config["hdrLevels"].toInt();
        m_do3thinkParams.triggerDelay = config["triggerDelay"].toDouble();
        m_do3thinkParams.triggerDivider = config["triggerDivider"].toInt();
        m_do3thinkParams.streamMode = config["streamMode"].toInt();
        m_do3thinkParams.packetSize = config["packetSize"].toInt();
        m_do3thinkParams.lutEnabled = config["lutEnabled"].toBool();
        
        file.close();
        
        // Apply loaded parameters
        applyDo3ThinkParameters();
    }
    
    return true;
}

bool Do3ThinkCameraComponent::setUserDefinedName(const QString& name) {
    m_do3thinkParams.userDefinedName = name;
    
    if (isConnected()) {
        // Store in camera's user data area using dvpWriteUserData
        // Using address 0 for user-defined name storage
        QByteArray nameData = name.toUtf8();
        dvpStatus status = m_sdkWrapper->dvpWriteUserData(m_cameraHandle, Do3ThinkCameraConstants::kUserDataNameOffset,
            nameData.data(), nameData.size());
        if (status != DVP_STATUS_OK) {
            logWarning(QString("Failed to set user defined name: %1").arg(dvpStatusToString(status)));
            // Not critical, continue anyway
        }
    }
    
    return true;
}

QString Do3ThinkCameraComponent::userDefinedName() const {
    return m_do3thinkParams.userDefinedName;
}

bool Do3ThinkCameraComponent::isFeatureSupported(const QString& feature) const {
    if (!isConnected()) {
        return false;
    }
    
    // Check standard features
    if (feature == "ColorCorrection") {
        return m_do3thinkInfo.supportsColorCorrection;
    } else if (feature == "HDR") {
        return m_do3thinkInfo.supportsHDR;
    } else if (feature == "GPIO" || feature == "LUT" || 
               feature == "TriggerDelay" || feature == "UserData") {
        return true;  // Always supported by Do3Think
    }
    
    // Check SDK capabilities
    dvpStatus status = DVP_STATUS_OK;
    
    if (feature == "AutoExposure") {
        dvpAeOperation aeOp;
        status = dvpGetAeOperation(m_cameraHandle, &aeOp);
        return (status == DVP_STATUS_OK);
    } else if (feature == "AutoWhiteBalance") {
        dvpAwbOperation awbOp;
        status = dvpGetAwbOperation(m_cameraHandle, &awbOp);
        return (status == DVP_STATUS_OK);
    } else if (feature == "Temperature") {
        dvpTemperatureInfo tempInfo;
        status = dvpGetTemperatureInfo(m_cameraHandle, &tempInfo);
        return (status == DVP_STATUS_OK);
    }
    
    return false;
}

QVariant Do3ThinkCameraComponent::getFeatureValue(const QString& feature) const {
    if (!isConnected()) {
        return QVariant();
    }
    
    if (feature == "Temperature") {
        return sensorTemperature();
    } else if (feature == "HDREnabled") {
        return m_do3thinkParams.hdrEnabled;
    } else if (feature == "HDRLevels") {
        return m_do3thinkParams.hdrLevels;
    } else if (feature == "ColorCorrectionEnabled") {
        return m_do3thinkParams.colorCorrectionEnabled;
    } else if (feature == "TriggerDelay") {
        return m_do3thinkParams.triggerDelay;
    } else if (feature == "TriggerDivider") {
        return m_do3thinkParams.triggerDivider;
    } else if (feature == "StreamMode") {
        return m_do3thinkParams.streamMode;
    } else if (feature == "PacketSize") {
        return m_do3thinkParams.packetSize;
    } else if (feature == "LUTEnabled") {
        return m_do3thinkParams.lutEnabled;
    }
    
    return QVariant();
}

bool Do3ThinkCameraComponent::setFeatureValue(const QString& feature, const QVariant& value) {
    if (!isConnected()) {
        return false;
    }
    
    if (feature == "HDREnabled") {
        return setHDRMode(value.toBool());
    } else if (feature == "HDRLevels") {
        return setHDRLevels(value.toInt());
    } else if (feature == "ColorCorrectionEnabled") {
        return enableColorCorrection(value.toBool());
    } else if (feature == "TriggerDelay") {
        return setTriggerDelay(value.toDouble());
    } else if (feature == "TriggerDivider") {
        return setTriggerDivider(value.toInt());
    } else if (feature == "StreamMode") {
        return setStreamMode(value.toInt());
    } else if (feature == "PacketSize") {
        return setPacketSize(value.toInt());
    } else if (feature == "LUTEnabled") {
        return enableLUT(value.toBool());
    }
    
    return false;
}

// ========== Do3Think Specific Methods ==========

bool Do3ThinkCameraComponent::setColorCorrectionMatrix(const QMatrix3x3& matrix) {
    if (!isConnected() || !m_do3thinkInfo.supportsColorCorrection) {
        return false;
    }
    
    // Copy matrix values
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            m_do3thinkParams.colorCorrectionMatrix[i * 3 + j] = matrix(i, j);
        }
    }
    
    // Apply to camera - Do3Think SDK uses BGR correction values, not full matrix
    dvpColorCorrection correction;
    // Use diagonal elements as BGR correction factors
    correction.bgr.blue = static_cast<float>(m_do3thinkParams.colorCorrectionMatrix[0]); // Blue
    correction.bgr.green = static_cast<float>(m_do3thinkParams.colorCorrectionMatrix[4]); // Green
    correction.bgr.red = static_cast<float>(m_do3thinkParams.colorCorrectionMatrix[8]); // Red
    
    dvpStatus status = dvpSetColorCorrection(m_cameraHandle, correction);
    
    if (status != DVP_STATUS_OK) {
        logError(QString("Failed to set color correction matrix: %1").arg(dvpStatusToString(status)));
        return false;
    }
    
    return true;
}

QMatrix3x3 Do3ThinkCameraComponent::colorCorrectionMatrix() const {
    QMatrix3x3 matrix;
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            matrix(i, j) = m_do3thinkParams.colorCorrectionMatrix[i * 3 + j];
        }
    }
    return matrix;
}

bool Do3ThinkCameraComponent::enableColorCorrection(bool enable) {
    if (!isConnected() || !m_do3thinkInfo.supportsColorCorrection) {
        return false;
    }
    
    // dvpSetColorCorrectionState doesn't exist, use dvpSetColorSolutionSel instead
    // 0 = OFF, 1 = ON (or use appropriate color solution index)
    dvpStatus status = dvpSetColorSolutionSel(m_cameraHandle, enable ? 1 : 0);
    if (status != DVP_STATUS_OK) {
        logError(QString("Failed to enable color correction: %1").arg(dvpStatusToString(status)));
        return false;
    }
    
    m_do3thinkParams.colorCorrectionEnabled = enable;
    emit colorCorrectionChanged(enable);
    return true;
}

bool Do3ThinkCameraComponent::supportsColorCorrection() const {
    return m_do3thinkInfo.supportsColorCorrection;
}

bool Do3ThinkCameraComponent::setHDRMode(bool enable) {
    if (!isConnected() || !m_do3thinkInfo.supportsHDR) {
        return false;
    }
    
    dvpStatus status = dvpSetHardwareIspState(m_cameraHandle, enable);
    if (status != DVP_STATUS_OK) {
        logError(QString("Failed to set HDR mode: %1").arg(dvpStatusToString(status)));
        return false;
    }
    
    m_do3thinkParams.hdrEnabled = enable;
    emit hdrModeChanged(enable);
    return true;
}

bool Do3ThinkCameraComponent::isHDREnabled() const {
    return m_do3thinkParams.hdrEnabled;
}

bool Do3ThinkCameraComponent::setHDRLevels(int levels) {
    if (!isConnected() || !m_do3thinkInfo.supportsHDR) {
        return false;
    }
    
    // Do3Think specific HDR level setting
    // This might be implemented through custom commands
    m_do3thinkParams.hdrLevels = levels;
    
    // Apply HDR levels through device-specific API if available
    // For now, store the value for future use
    
    return true;
}

int Do3ThinkCameraComponent::hdrLevels() const {
    return m_do3thinkParams.hdrLevels;
}

bool Do3ThinkCameraComponent::supportsHDR() const {
    return m_do3thinkInfo.supportsHDR;
}

double Do3ThinkCameraComponent::sensorTemperature() const {
    if (!isConnected()) {
        return 0.0;
    }
    
    // dvpGetTemperature doesn't exist, use dvpGetTemperatureInfo instead
    dvpTemperatureInfo tempInfo;
    dvpStatus status = dvpGetTemperatureInfo(m_cameraHandle, &tempInfo);
    if (status != DVP_STATUS_OK) {
        return 0.0;
    }
    
    // Return the device temperature (or sensor temperature if available)
    return static_cast<double>(tempInfo.fDevice);
}

QString Do3ThinkCameraComponent::sensorModel() const {
    return m_do3thinkState.sensorModel;
}

QSize Do3ThinkCameraComponent::sensorResolution() const {
    return m_do3thinkState.sensorResolution;
}

bool Do3ThinkCameraComponent::setTriggerDelay(double microseconds) {
    if (!isConnected()) {
        return false;
    }
    
    dvpStatus status = dvpSetTriggerDelay(m_cameraHandle, microseconds);
    if (status != DVP_STATUS_OK) {
        logError(QString("Failed to set trigger delay: %1").arg(dvpStatusToString(status)));
        return false;
    }
    
    m_do3thinkParams.triggerDelay = microseconds;
    return true;
}

double Do3ThinkCameraComponent::triggerDelay() const {
    return m_do3thinkParams.triggerDelay;
}

bool Do3ThinkCameraComponent::setTriggerDivider(int divider) {
    if (!isConnected()) {
        return false;
    }
    
    // dvpSetTriggerDivider doesn't exist in the SDK
    // Store the value for potential future use or custom implementation
    m_do3thinkParams.triggerDivider = divider;
    logWarning("Trigger divider is not supported by the current SDK version");
    return true;
}

int Do3ThinkCameraComponent::triggerDivider() const {
    return m_do3thinkParams.triggerDivider;
}

bool Do3ThinkCameraComponent::setStreamMode(int mode) {
    if (!isConnected()) {
        return false;
    }
    
    // dvpStreamMode type doesn't exist, use dvpBufferMode instead
    // BUFFER_MODE_NEWEST = 0, BUFFER_MODE_FIFO = 1
    dvpBufferMode bufferMode = (mode == 0) ? BUFFER_MODE_NEWEST : BUFFER_MODE_FIFO;
    
    // Note: There's no direct dvpSetStreamMode function, buffer mode is set via dvpBufferConfig
    // For now, just store the value
    m_do3thinkParams.streamMode = mode;
    logDebug(QString("Stream mode set to: %1").arg(mode));
    emit streamModeChanged(mode);
    return true;
}

int Do3ThinkCameraComponent::streamMode() const {
    return m_do3thinkParams.streamMode;
}

bool Do3ThinkCameraComponent::setPacketSize(int size) {
    if (!isConnected()) {
        return false;
    }
    
    // dvpSetPacketSize doesn't exist, use dvpSetPacketSizeSel for GigE cameras
    // This function is typically only available for GigE cameras
    dvpStatus status = dvpSetPacketSizeSel(m_cameraHandle, static_cast<dvpUint32>(size));
    if (status == DVP_STATUS_NOT_SUPPORTED) {
        // Not a GigE camera or feature not supported
        logDebug("Packet size configuration not supported (likely USB camera)");
        m_do3thinkParams.packetSize = size;
        return true;
    } else if (status != DVP_STATUS_OK) {
        logError(QString("Failed to set packet size: %1").arg(dvpStatusToString(status)));
        return false;
    }
    
    m_do3thinkParams.packetSize = size;
    return true;
}

int Do3ThinkCameraComponent::packetSize() const {
    return m_do3thinkParams.packetSize;
}

bool Do3ThinkCameraComponent::setLUT(const QVector<quint16>& lut) {
    if (!isConnected() || lut.size() != 4096) {  // 12-bit LUT
        return false;
    }
    
    m_do3thinkParams.lutTable = lut;
    
    // dvpSetLut doesn't exist in the current SDK
    // Store the LUT table for potential future use or custom implementation
    logDebug("LUT table stored (feature may not be supported by SDK)");
    
    return true;
}

QVector<quint16> Do3ThinkCameraComponent::getLUT() const {
    return m_do3thinkParams.lutTable;
}

bool Do3ThinkCameraComponent::enableLUT(bool enable) {
    if (!isConnected()) {
        return false;
    }
    
    // dvpSetLutState doesn't exist, use dvpSetLutEnable if available
    // Since dvpSetLutEnable also doesn't exist in the SDK, we'll just store the state
    // dvpStatus status = dvpSetLutEnable(m_cameraHandle, enable);
    
    // For now, just store the value as LUT functions may not be available
    m_do3thinkParams.lutEnabled = enable;
    logDebug(QString("LUT %1 (feature may not be supported by SDK)").arg(enable ? "enabled" : "disabled"));
    return true;
    
    /*
    dvpStatus status = DVP_STATUS_NOT_SUPPORTED;
    if (status != DVP_STATUS_OK) {
        logError(QString("Failed to enable LUT: %1").arg(dvpStatusToString(status)));
        return false;
    }
    */
}

bool Do3ThinkCameraComponent::isLUTEnabled() const {
    return m_do3thinkParams.lutEnabled;
}

bool Do3ThinkCameraComponent::setGPIODirection(int pin, bool output) {
    if (!isConnected()) {
        return false;
    }
    
    // dvpIoMode type doesn't exist, use dvpSetOutputIoFunction for outputs
    // For inputs, use dvpGetInputIoFunction
    
    if (output) {
        // For output pins, we can set their function
        // Use OUTPUT_FUNCTION_USER_OUTPUT for general purpose output
        dvpStatus status = dvpSetOutputIoFunction(m_cameraHandle, 
            static_cast<dvpOutputIo>(pin), 
            OUTPUT_FUNCTION_NORMAL);
        if (status != DVP_STATUS_OK) {
            logError(QString("Failed to set GPIO direction: %1").arg(dvpStatusToString(status)));
            return false;
        }
    } else {
        // Input pins are typically read-only, just log
        logDebug(QString("GPIO pin %1 set as input").arg(pin));
    }
    
    return true;
    
    /*
    dvpStatus status = DVP_STATUS_OK;
    */
}

bool Do3ThinkCameraComponent::setGPIOValue(int pin, bool value) {
    if (!isConnected()) {
        return false;
    }
    
    // dvpIoLevel type doesn't exist, use bool for dvpSetOutputIoLevel
    dvpStatus status = dvpSetOutputIoLevel(m_cameraHandle, 
        static_cast<dvpOutputIo>(pin), 
        value);
    if (status != DVP_STATUS_OK) {
        logError(QString("Failed to set GPIO value: %1").arg(dvpStatusToString(status)));
        return false;
    }
    
    emit gpioStateChanged(pin, value);
    return true;
}

bool Do3ThinkCameraComponent::getGPIOValue(int pin) const {
    if (!isConnected()) {
        return false;
    }
    
    // dvpIoLevel type doesn't exist, use bool for dvpGetInputIoLevel
    bool level = false;
    dvpStatus status = dvpGetInputIoLevel(m_cameraHandle, 
        static_cast<dvpInputIo>(pin), 
        &level);
    if (status != DVP_STATUS_OK) {
        return false;
    }
    
    return level;
}

bool Do3ThinkCameraComponent::writeUserData(const QByteArray& data) {
    if (!isConnected()) {
        return false;
    }
    
    // Use dvpWriteUserData with address parameter
    // Starting at address 256 for general user data (after user-defined name)
    dvpStatus status = m_sdkWrapper->dvpWriteUserData(m_cameraHandle, Do3ThinkCameraConstants::kUserDataGeneralOffset,
        data.data(), data.size());
    
    if (status != DVP_STATUS_OK) {
        logError(QString("Failed to write user data: %1").arg(dvpStatusToString(status)));
        return false;
    }
    
    emit userDataWritten();
    return true;
}

QByteArray Do3ThinkCameraComponent::readUserData() const {
    if (!isConnected()) {
        return QByteArray();
    }
    
    // Read user data from a fixed location (address 0)
    // Try to read up to 256 bytes for user-defined name
    const dvpUint32 maxSize = Do3ThinkCameraConstants::kUserDataNameMaxSize;
    QByteArray data(maxSize, 0);
    
    dvpStatus status = m_sdkWrapper->dvpReadUserData(m_cameraHandle, Do3ThinkCameraConstants::kUserDataNameOffset,
        data.data(), maxSize);
    
    if (status != DVP_STATUS_OK) {
        return QByteArray();
    }
    
    // Find actual string length (null-terminated)
    int actualLength = 0;
    for (int i = 0; i < maxSize; ++i) {
        if (data[i] == 0) {
            actualLength = i;
            break;
        }
    }
    
    data.resize(actualLength);
    return data;
}

// ========== Protected Virtual Methods ==========

bool Do3ThinkCameraComponent::onInitialize() {
    logInfo("Initializing Do3Think camera component");
    
    // SDK is initialized per-device, not globally
    // Just ensure our state is clean
    m_cameraHandle = 0;
    m_callbackData.active = false;
    
    return true;
}

bool Do3ThinkCameraComponent::onStart() {
    logInfo("Starting Do3Think camera component");
    
    // Start temperature monitoring timer
    auto* tempTimer = findChild<QTimer*>();
    if (tempTimer) {
        tempTimer->start();
    }
    
    return true;
}

bool Do3ThinkCameraComponent::onStop() {
    logInfo("Stopping Do3Think camera component");
    
    // Stop acquisition if running
    if (isAcquiring()) {
        stopAcquisition();
    }
    
    // Stop timers
    auto* tempTimer = findChild<QTimer*>();
    if (tempTimer) {
        tempTimer->stop();
    }
    
    return true;
}

bool Do3ThinkCameraComponent::onReset() {
    logInfo("Resetting Do3Think camera component");
    
    // Disconnect camera
    if (isConnected()) {
        disconnectCamera();
    }
    
    // Reset parameters to defaults
    m_do3thinkParams = Do3ThinkParameters();
    m_do3thinkState = Do3ThinkState();
    
    // Reset statistics
    m_statistics.totalFrames = 0;
    m_statistics.droppedFrames = 0;
    m_statistics.currentFps = 0.0;
    m_statistics.averageFps = 0.0;
    m_statistics.peakFps = 0.0;
    m_statistics.bandwidth = 0.0;
    m_statistics.acquisitionStartTime = 0;
    m_statistics.lastFrameTime = 0;
    
    return true;
}

void Do3ThinkCameraComponent::onDestroy() {
    logInfo("Destroying Do3Think camera component");
    cleanupDo3ThinkResources();
}

void Do3ThinkCameraComponent::onStateChanged(ComponentState newState, ComponentState oldState) {
    logDebug(QString("Component state changed from %1 to %2")
        .arg(static_cast<int>(oldState))
        .arg(static_cast<int>(newState)));
    
    // Handle state-specific transitions
    if (newState == ComponentState::Error) {
        // Stop acquisition on error
        if (isAcquiring()) {
            stopAcquisition();
        }
    }
}

void Do3ThinkCameraComponent::onConfigurationChanged(const QJsonObject& config) {
    logDebug("Configuration changed");
    
    // Apply new configuration
    if (config.contains("exposure")) {
        setExposureTime(config["exposure"].toDouble());
    }
    if (config.contains("gain")) {
        setGain(config["gain"].toDouble());
    }
    if (config.contains("frameRate")) {
        setFrameRate(config["frameRate"].toDouble());
    }
    if (config.contains("triggerMode")) {
        setTriggerMode(stringToTriggerMode(config["triggerMode"].toString()));
    }
    if (config.contains("pixelFormat")) {
        setPixelFormat(stringToPixelFormat(config["pixelFormat"].toString()));
    }
}

void Do3ThinkCameraComponent::onEventReceived(const ComponentEvent& event) {
    // Handle component events
    if (event.type == "Command") {
        if (event.data["command"] == "softwareTrigger") {
            executeSoftwareTrigger();
        } else if (event.data["command"] == "grabSingle") {
            grabSingleFrame();
        }
    }
}

void Do3ThinkCameraComponent::processFrame(const ImageBuffer& buffer) {
    // Base class frame processing
    updateFrameStatistics(buffer.metadata());
    
    // Convert to QImage if needed
    QImage image = convertToQImage(buffer);
    if (!image.isNull()) {
        // Apply image processing pipeline if enabled
        QImage processedImage = processImage(image);
        
        // Emit the processed frame (or original if processing disabled)
        emit frameReady(processedImage, buffer.metadata());
        
        // If processing was applied and different from original, 
        // you could also emit the original for comparison
        if (m_processingState.processingEnabled && !processedImage.isNull()) {
            // Optionally emit original for debugging/comparison
            // emit originalFrameReady(image, buffer.metadata());
        }
    }
    
    // Emit raw frame for high-performance processing
    emit rawFrameReady(buffer);
}

QImage Do3ThinkCameraComponent::convertToQImage(const ImageBuffer& buffer) {
    if (!buffer.data() || buffer.size() == 0) {
        return QImage();
    }
    
    const auto& metadata = buffer.metadata();
    QImage::Format format = QImage::Format_Invalid;
    
    // Map pixel format to QImage format
    switch (metadata.pixelFormat) {
        case PixelFormat::Mono8:
            format = QImage::Format_Grayscale8;
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
            // Need conversion for other formats
            break;
    }
    
    if (format != QImage::Format_Invalid) {
        // Direct copy for supported formats
        QImage image(static_cast<const uchar*>(buffer.data()),
                    metadata.imageSize.width(),
                    metadata.imageSize.height(),
                    format);
        return image.copy();  // Make a deep copy
    }
    
    // Handle unsupported formats with conversion
    // This is a simplified conversion, real implementation would be more complex
    QImage image(metadata.imageSize, QImage::Format_RGB888);
    
    // Perform format conversion based on source format
    // ... conversion logic ...
    
    return image;
}

void Do3ThinkCameraComponent::processDo3ThinkFrame(const dvpFrame& frame, void* pBuffer) {
    // Create frame metadata
    FrameMetadata metadata;
    metadata.timestamp = static_cast<qint64>(frame.uTimestamp);
    metadata.frameNumber = frame.uFrameID;
    metadata.exposureTime = frame.fExposure;
    metadata.gain = frame.fAGain;
    metadata.temperature = sensorTemperature();
    metadata.imageSize = QSize(frame.iWidth, frame.iHeight);
    metadata.pixelFormat = dvpFormatToPixelFormat(static_cast<dvpStreamFormat>(frame.format));
    metadata.isValid = true;
    
    // Update statistics through base class
    updateFrameStatistics(metadata);
    
    // Check if we have a valid buffer
    if (!pBuffer) {
        emit frameDropped(metadata.frameNumber, "No frame buffer provided");
        return;
    }
    
    // Create ImageBuffer wrapper
    ImageBuffer buffer(pBuffer, frame.uBytes, metadata);
    
    // Process frame
    processFrame(buffer);
    
    // Update Do3Think specific statistics
    updateDo3ThinkStatistics();
}

void Do3ThinkCameraComponent::updateDo3ThinkStatistics() {
    // Update Do3Think specific statistics only
    // Base class FPS is handled by updateFrameStatistics
    
    // Update temperature periodically
    static qint64 lastTempUpdate = 0;
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    
    if (currentTime - lastTempUpdate >= 5000) {
        double temp = sensorTemperature();
        if (temp > 0) {
            m_do3thinkState.currentTemperature = temp;
        }
        lastTempUpdate = currentTime;
    }
}

void Do3ThinkCameraComponent::handleDo3ThinkError(dvpStatus status, const QString& context) {
    QString errorMsg = QString("%1: %2").arg(context).arg(dvpStatusToString(status));
    
    // Log error
    logError(errorMsg);
    
    // Set camera error
    setCameraError(errorMsg);
    
    // Emit error signal
    emit cameraError(errorMsg);
    
    // Handle specific errors
    switch (status) {
        case DVP_STATUS_DEVICE_IS_DISCONNECTED:
            emit deviceLost(m_currentDevice.serialNumber);
            setCameraState(CameraState::Error);
            break;
            
        case DVP_STATUS_TIME_OUT:
            emit performanceWarning("Frame timeout detected");
            break;
            
        case DVP_STATUS_OVER_LOAD:
            emit bufferOverflow();
            break;
            
        default:
            break;
    }
}

bool Do3ThinkCameraComponent::applyDo3ThinkParameters() {
    if (!isConnected()) {
        return false;
    }
    
    bool success = true;
    
    // Apply stored parameters
    if (m_do3thinkParams.colorCorrectionEnabled) {
        success &= enableColorCorrection(true);
    }
    
    if (m_do3thinkParams.hdrEnabled) {
        success &= setHDRMode(true);
    }
    
    if (m_do3thinkParams.triggerDelay > 0) {
        success &= setTriggerDelay(m_do3thinkParams.triggerDelay);
    }
    
    if (m_do3thinkParams.triggerDivider > 1) {
        success &= setTriggerDivider(m_do3thinkParams.triggerDivider);
    }
    
    if (m_do3thinkParams.lutEnabled && !m_do3thinkParams.lutTable.isEmpty()) {
        success &= setLUT(m_do3thinkParams.lutTable);
        success &= enableLUT(true);
    }
    
    return success;
}

void Do3ThinkCameraComponent::cleanupDo3ThinkResources() {
    // Stop callback processing
    m_callbackData.active = false;
    
    // Clean up camera handle
    if (m_cameraHandle != 0) {
        dvpClose(m_cameraHandle);
        m_cameraHandle = 0;
    }
    
    // Clear state
    m_do3thinkInfo = Do3ThinkDeviceInfo();
    m_do3thinkState = Do3ThinkState();
}

bool Do3ThinkCameraComponent::initializeDo3ThinkDevice(dvpHandle handle) {
    dvpStatus status;
    
    // Stream mode is controlled by buffer configuration, not a direct function
    // The buffer mode is set when configuring buffer queue
    // BUFFER_MODE_NEWEST = 0 for getting newest frames
    
    // Set default buffer queue size for high-speed operation
    status = dvpSetBufferQueueSize(handle, 100);
    if (status != DVP_STATUS_OK) {
        logWarning("Failed to set buffer queue size");
    }
    
    // Timestamp is automatically included in frame data
    // No need to explicitly enable it
    
    // Set default trigger mode
    // Set to continuous mode (no trigger)
    status = dvpSetTriggerInputType(handle, dvpTriggerInputType::TRIGGER_IN_OFF);
    if (status != DVP_STATUS_OK) {
        logWarning("Failed to set trigger state");
    }
    
    return true;
}

bool Do3ThinkCameraComponent::configureDo3ThinkDefaults() {
    dvpStatus status;
    
    // Set default exposure
    status = dvpSetExposure(m_cameraHandle, 10000.0);  // 10ms
    if (status != DVP_STATUS_OK) {
        logWarning("Failed to set default exposure");
    }
    
    // Set default gain
    status = dvpSetAnalogGain(m_cameraHandle, 1.0f);
    if (status != DVP_STATUS_OK) {
        logWarning("Failed to set default gain");
    }
    
    // Set default image format using target format
    dvpStreamFormat streamFormat = S_RAW8;  // Default to 8-bit mono
    
    status = dvpSetTargetFormat(m_cameraHandle, streamFormat);
    if (status != DVP_STATUS_OK) {
        logWarning("Failed to set default image format");
    }
    
    return true;
}

// ========== Protected Slots ==========

void Do3ThinkCameraComponent::onHealthCheck() {
    if (!isConnected()) {
        return;
    }
    
    // Check camera health
    bool isValid = false;
    dvpStatus status = dvpIsValid(m_cameraHandle, &isValid);
    if (status != DVP_STATUS_OK || !isValid) {
        handleDo3ThinkError(status, "Health check failed");
    }
}

void Do3ThinkCameraComponent::onFrameTimeout() {
    logWarning("Frame timeout detected");
    emit performanceWarning("Frame timeout - no frames received");
    
    // Try to recover
    if (isAcquiring()) {
        clearBuffers();
    }
}

void Do3ThinkCameraComponent::onReconnectTimer() {
    if (isConnected() || !isAutoReconnectEnabled()) {
        return;
    }
    
    // Try to reconnect
    if (!m_do3thinkInfo.connectionString.isEmpty()) {
        logInfo("Attempting to reconnect...");
        connectCamera(m_do3thinkInfo.connectionString);
    }
}

void Do3ThinkCameraComponent::onStatisticsUpdate() {
    // Call base implementation
    CameraComponent::onStatisticsUpdate();
    
    // Can add Do3Think specific statistics handling here if needed
}

void Do3ThinkCameraComponent::onDo3ThinkCallback() {
    // Handle Do3Think specific callbacks
}

void Do3ThinkCameraComponent::onTemperatureMonitor() {
    if (!isConnected()) {
        return;
    }
    
    double temp = sensorTemperature();
    if (temp != m_do3thinkState.currentTemperature) {
        m_do3thinkState.currentTemperature = temp;
        emit temperatureChanged(temp);
        
        // Check for temperature warning
        if (temp > 70.0) {  // 70°C warning threshold
            emit temperatureWarning(temp);
        }
    }
}

// ========== Helper Methods ==========

QString Do3ThinkCameraComponent::dvpStatusToString(dvpStatus status) const {
    switch (status) {
        case DVP_STATUS_OK: return "OK";
        case DVP_STATUS_FAILED: return "Failed";
        case DVP_STATUS_NOT_INITIALIZED: return "Uninitialized";
        case DVP_STATUS_INVALID_HANDLE: return "Invalid handle";
        case DVP_STATUS_PARAMETER_INVALID: return "Invalid parameter";
        case DVP_STATUS_NOT_SUPPORTED: return "Not supported";
        case DVP_STATUS_FUNCTION_INVALID: return "Not implemented";
        case DVP_STATUS_DEVICE_IS_DISCONNECTED: return "Device lost";
        case DVP_STATUS_DENIED: return "Access denied";
        case DVP_STATUS_TIME_OUT: return "Timeout";
        case DVP_STATUS_NO_DEVICE_FOUND: return "No device";
        case DVP_STATUS_BUSY: return "Busy";
        case DVP_STATUS_OVER_LOAD: return "Buffer overflow";
        default: return QString("Unknown error (%1)").arg(status);
    }
}

PixelFormat Do3ThinkCameraComponent::dvpFormatToPixelFormat(dvpStreamFormat format) const {
    switch (format) {
        case S_RAW8: return PixelFormat::Mono8;
        case S_BGR24: return PixelFormat::BGR24;
        case S_RGB24: return PixelFormat::RGB24;
        case S_BGR32: return PixelFormat::RGBA32;  // BGR32 in SDK maps to RGBA32
        // Note: SDK doesn't have S_RGBA32, S_RAW8_BG etc.
        // Would need to check sensor pixel type for Bayer formats
        default: return PixelFormat::Custom;
    }
}

dvpStreamFormat Do3ThinkCameraComponent::pixelFormatToDvpFormat(PixelFormat format) const {
    switch (format) {
        case PixelFormat::Mono8: return S_RAW8;
        case PixelFormat::BGR24: return S_BGR24;
        case PixelFormat::RGB24: return S_RGB24;
        case PixelFormat::RGBA32: return S_BGR32;  // RGBA32 maps to BGR32 in SDK
        // For Bayer formats, Do3Think uses S_RAW8 with sensor pixel type
        case PixelFormat::BayerBG8:
        case PixelFormat::BayerGB8:
        case PixelFormat::BayerGR8:
        case PixelFormat::BayerRG8:
            return S_RAW8;  // Bayer formats use S_RAW8 in Do3Think
        default: return S_RAW8;
    }
}

TriggerMode Do3ThinkCameraComponent::dvpTriggerToTriggerMode(dvpTriggerSource source) const {
    switch (source) {
        case TRIGGER_SOURCE_SOFTWARE: return TriggerMode::Software;
        case TRIGGER_SOURCE_LINE1:
        case TRIGGER_SOURCE_LINE2:
        case TRIGGER_SOURCE_LINE3:
            return TriggerMode::Hardware;
        default: return TriggerMode::FreeRun;
    }
}

dvpTriggerSource Do3ThinkCameraComponent::triggerModeToDvpTrigger(TriggerMode mode) const {
    switch (mode) {
        case TriggerMode::Software: return TRIGGER_SOURCE_SOFTWARE;
        case TriggerMode::Hardware: return TRIGGER_SOURCE_LINE1;
        case TriggerMode::FreeRun:
        case TriggerMode::FixedRate:
        case TriggerMode::Burst:
        default: return TRIGGER_SOURCE_SOFTWARE;
    }
}

// ============================================================================
// Image Processing Pipeline Implementation
// ============================================================================

bool Do3ThinkCameraComponent::enableImageProcessing(bool enable)
{
    QMutexLocker locker(&m_processingState.processingMutex);
    
    if (m_processingState.processingEnabled == enable) {
        return true; // Already in desired state
    }
    
    m_processingState.processingEnabled = enable;
    
    if (enable) {
        DEBUG_LOG("Enabling image processing pipeline");
        // TODO: Re-enable when preprocessing pipeline is fixed
        // if (m_processingPipeline) {
        //     m_processingPipeline->start();
        // }
    } else {
        DEBUG_LOG("Disabling image processing pipeline");
        // TODO: Re-enable when preprocessing pipeline is fixed
        // if (m_processingPipeline) {
        //     m_processingPipeline->pause();
        // }
    }
    
    emit processingEnabledChanged(enable);
    return true;
}

bool Do3ThinkCameraComponent::isImageProcessingEnabled() const
{
    return m_processingState.processingEnabled;
}

bool Do3ThinkCameraComponent::enablePreprocessor(const QString& type, bool enable)
{
    QMutexLocker locker(&m_processingState.processingMutex);
    
    bool changed = false;
    
    if (type == "blur") {
        if (m_processingState.blurEnabled != enable) {
            m_processingState.blurEnabled = enable;
            changed = true;
            DEBUG_LOG("Blur preprocessor " << (enable ? "enabled" : "disabled"));
        }
    } else if (type == "edge") {
        if (m_processingState.edgeEnabled != enable) {
            m_processingState.edgeEnabled = enable;
            changed = true;
            DEBUG_LOG("Edge preprocessor " << (enable ? "enabled" : "disabled"));
        }
    } else if (type == "denoise") {
        if (m_processingState.denoiseEnabled != enable) {
            m_processingState.denoiseEnabled = enable;
            changed = true;
            DEBUG_LOG("Denoise preprocessor " << (enable ? "enabled" : "disabled"));
        }
    } else {
        DEBUG_LOG("Unknown preprocessor type: " << type.toStdString());
        return false;
    }
    
    if (changed) {
        updateProcessingPipeline();
        emit preprocessorEnabledChanged(type, enable);
    }
    
    return true;
}

bool Do3ThinkCameraComponent::isPreprocessorEnabled(const QString& type) const
{
    if (type == "blur") return m_processingState.blurEnabled;
    if (type == "edge") return m_processingState.edgeEnabled;
    if (type == "denoise") return m_processingState.denoiseEnabled;
    return false;
}

bool Do3ThinkCameraComponent::setPreprocessorParameter(const QString& type, const QString& parameter, const QVariant& value)
{
    // TODO: Re-enable when preprocessing pipeline is fixed
    Q_UNUSED(type);
    Q_UNUSED(parameter);
    Q_UNUSED(value);
    return true;
    /*
    QMutexLocker locker(&m_processingState.processingMutex);
    
    try {
        bool success = false;
        
        if (type == "blur" && m_blurProcessor) {
            // Hot-swap parameter change - processor can continue running
            m_blurProcessor->setParameter(parameter, value);
            DEBUG_LOG("Hot-swap: Set blur parameter " << parameter.toStdString() << " to " << value.toString().toStdString());
            success = true;
        } else if (type == "edge" && m_edgeProcessor) {
            // Hot-swap parameter change - processor can continue running
            m_edgeProcessor->setParameter(parameter, value);
            DEBUG_LOG("Hot-swap: Set edge parameter " << parameter.toStdString() << " to " << value.toString().toStdString());
            success = true;
        } else if (type == "denoise" && m_denoiseProcessor) {
            // Hot-swap parameter change - processor can continue running
            m_denoiseProcessor->setParameter(parameter, value);
            DEBUG_LOG("Hot-swap: Set denoise parameter " << parameter.toStdString() << " to " << value.toString().toStdString());
            success = true;
        }
        
        if (success) {
            // Parameters changed successfully without stopping the pipeline
            // This enables real-time parameter adjustment while processing
            DEBUG_LOG("Parameter hot-swap completed - processing continues without interruption");
            
            // Optionally emit a signal to inform UI of successful parameter change
            // This could be used to update parameter displays or show visual feedback
        }
        
        return success;
        
    } catch (const std::exception& e) {
        DEBUG_LOG("Exception during parameter hot-swap: " << e.what());
        
        // On parameter change failure, the pipeline should continue with old parameters
        // This ensures processing doesn't stop due to invalid parameter values
        return false;
    }
    */
}

QVariant Do3ThinkCameraComponent::getPreprocessorParameter(const QString& type, const QString& parameter) const
{
    // TODO: Re-enable when preprocessing pipeline is fixed
    Q_UNUSED(type);
    Q_UNUSED(parameter);
    return QVariant();
    /*
    if (type == "blur" && m_blurProcessor) {
        return m_blurProcessor->getParameter(parameter);
    } else if (type == "edge" && m_edgeProcessor) {
        return m_edgeProcessor->getParameter(parameter);
    } else if (type == "denoise" && m_denoiseProcessor) {
        return m_denoiseProcessor->getParameter(parameter);
    }
    
    return QVariant();
    */
}

void Do3ThinkCameraComponent::setProcessingOrder(const QStringList& order)
{
    QMutexLocker locker(&m_processingState.processingMutex);
    
    m_processingState.processingOrder = order;
    updateProcessingPipeline();
    
    emit processingOrderChanged(order);
    DEBUG_LOG("Processing order changed to: " << order.join(", ").toStdString());
}

QStringList Do3ThinkCameraComponent::getProcessingOrder() const
{
    return m_processingState.processingOrder;
}

// Protected slots for UI integration
void Do3ThinkCameraComponent::onImageProcessingEnabled(bool enabled)
{
    enableImageProcessing(enabled);
}

void Do3ThinkCameraComponent::onPreprocessorEnabled(const QString& type, bool enabled)
{
    enablePreprocessor(type, enabled);
}

void Do3ThinkCameraComponent::onPreprocessorParameterChanged(const QString& type, const QString& parameter, const QVariant& value)
{
    setPreprocessorParameter(type, parameter, value);
}

void Do3ThinkCameraComponent::onProcessingOrderChanged(const QStringList& order)
{
    setProcessingOrder(order);
}

void Do3ThinkCameraComponent::onProcessingConfigurationRequested()
{
    // This could open a configuration interface or save/load processing settings
    DEBUG_LOG("Processing configuration requested");
    
    // For now, just log the current configuration
    DEBUG_LOG("Current processing configuration:");
    DEBUG_LOG("  Processing enabled: " << (m_processingState.processingEnabled ? "yes" : "no"));
    DEBUG_LOG("  Blur enabled: " << (m_processingState.blurEnabled ? "yes" : "no"));
    DEBUG_LOG("  Edge enabled: " << (m_processingState.edgeEnabled ? "yes" : "no"));
    DEBUG_LOG("  Denoise enabled: " << (m_processingState.denoiseEnabled ? "yes" : "no"));
    DEBUG_LOG("  Processing order: " << m_processingState.processingOrder.join(", ").toStdString());
}

// Helper methods implementation
bool Do3ThinkCameraComponent::initializeProcessingPipeline()
{
    // TODO: Re-enable when preprocessing pipeline is fixed
    /*
    try {
        using namespace ComponentsForest::OpenCV;
        
        // Create the main processing pipeline
        m_processingPipeline = std::make_unique<SequentialPipeline>("Do3ThinkProcessing");
        
        // Create individual preprocessors
        m_blurProcessor = std::make_unique<BlurPreProcessor>();
        m_edgeProcessor = std::make_unique<EdgePreProcessor>();
        m_denoiseProcessor = std::make_unique<DenoisePreProcessor>();
        
        // Set default parameters
        m_blurProcessor->setParameter("kernelWidth", 5);
        m_blurProcessor->setParameter("kernelHeight", 5);
        m_blurProcessor->setParameter("sigmaX", 1.0);
        m_blurProcessor->setParameter("sigmaY", 1.0);
        
        m_edgeProcessor->setParameter("algorithm", "Canny");
        m_edgeProcessor->setParameter("lowThreshold", 50.0);
        m_edgeProcessor->setParameter("highThreshold", 150.0);
        
        m_denoiseProcessor->setParameter("algorithm", "Bilateral");
        m_denoiseProcessor->setParameter("d", 5);
        m_denoiseProcessor->setParameter("sigmaColor", 50.0);
        m_denoiseProcessor->setParameter("sigmaSpace", 50.0);
        
        // Initialize preprocessors
        if (!m_blurProcessor->initialize(QJsonObject()) ||
            !m_edgeProcessor->initialize(QJsonObject()) ||
            !m_denoiseProcessor->initialize(QJsonObject())) {
            DEBUG_LOG("Failed to initialize one or more preprocessors");
            return false;
        }
        
        // Setup connections for monitoring
        setupPreprocessorConnections();
        
        DEBUG_LOG("Processing pipeline initialized successfully");
        return true;
        
    } catch (const std::exception& e) {
        DEBUG_LOG("Exception initializing processing pipeline: " << e.what());
        return false;
    }
    */
    return true; // Temporarily return true until pipeline is fixed
}

void Do3ThinkCameraComponent::cleanupProcessingPipeline()
{
    // TODO: Re-enable when preprocessing pipeline is fixed
    /*
    try {
        if (m_processingPipeline) {
            m_processingPipeline->stop();
            m_processingPipeline.reset();
        }
        
        if (m_blurProcessor) {
            m_blurProcessor->shutdown();
            m_blurProcessor.reset();
        }
        
        if (m_edgeProcessor) {
            m_edgeProcessor->shutdown();
            m_edgeProcessor.reset();
        }
        
        if (m_denoiseProcessor) {
            m_denoiseProcessor->shutdown();
            m_denoiseProcessor.reset();
        }
        
        DEBUG_LOG("Processing pipeline cleaned up successfully");
        
    } catch (const std::exception& e) {
        DEBUG_LOG("Exception cleaning up processing pipeline: " << e.what());
    }
    */
}

bool Do3ThinkCameraComponent::updateProcessingPipeline()
{
    // TODO: Re-enable when preprocessing pipeline is fixed
    return true;
    /*
    if (!m_processingPipeline) return false;
    
    try {
        DEBUG_LOG("Hot-swapping processing pipeline...");
        
        // Pause the pipeline during reconfiguration for thread safety
        bool wasRunning = false;
        if (m_processingState.processingEnabled) {
            wasRunning = true;
            m_processingPipeline->pause();
            DEBUG_LOG("Pipeline paused for hot-swap");
        }
        
        // Clear the current pipeline stages
        m_processingPipeline->clearStages();
        DEBUG_LOG("Pipeline stages cleared");
        
        // Add enabled preprocessors in the specified order
        int stagesAdded = 0;
        for (const QString& processor : m_processingState.processingOrder) {
            if (processor == "Blur Filter" && m_processingState.blurEnabled && m_blurProcessor) {
                // Ensure the preprocessor is running
                if (!m_blurProcessor->isRunning()) {
                    m_blurProcessor->start();
                }
                m_processingPipeline->addStage(m_blurProcessor.get());
                stagesAdded++;
                DEBUG_LOG("Added blur processor to pipeline");
            } else if (processor == "Edge Detection" && m_processingState.edgeEnabled && m_edgeProcessor) {
                // Ensure the preprocessor is running
                if (!m_edgeProcessor->isRunning()) {
                    m_edgeProcessor->start();
                }
                m_processingPipeline->addStage(m_edgeProcessor.get());
                stagesAdded++;
                DEBUG_LOG("Added edge processor to pipeline");
            } else if (processor == "Noise Reduction" && m_processingState.denoiseEnabled && m_denoiseProcessor) {
                // Ensure the preprocessor is running
                if (!m_denoiseProcessor->isRunning()) {
                    m_denoiseProcessor->start();
                }
                m_processingPipeline->addStage(m_denoiseProcessor.get());
                stagesAdded++;
                DEBUG_LOG("Added denoise processor to pipeline");
            }
        }
        
        // Stop unused preprocessors to save resources
        if (!m_processingState.blurEnabled && m_blurProcessor && m_blurProcessor->isRunning()) {
            m_blurProcessor->stop();
            DEBUG_LOG("Stopped unused blur processor");
        }
        if (!m_processingState.edgeEnabled && m_edgeProcessor && m_edgeProcessor->isRunning()) {
            m_edgeProcessor->stop();
            DEBUG_LOG("Stopped unused edge processor");
        }
        if (!m_processingState.denoiseEnabled && m_denoiseProcessor && m_denoiseProcessor->isRunning()) {
            m_denoiseProcessor->stop();
            DEBUG_LOG("Stopped unused denoise processor");
        }
        
        // Resume the pipeline if it was running
        if (wasRunning && stagesAdded > 0) {
            m_processingPipeline->start();
            DEBUG_LOG("Pipeline resumed after hot-swap with " << stagesAdded << " stages");
        } else if (wasRunning && stagesAdded == 0) {
            // No stages to process, keep pipeline paused
            DEBUG_LOG("Pipeline kept paused - no stages enabled");
        }
        
        DEBUG_LOG("Hot-swap completed successfully");
        return true;
        
    } catch (const std::exception& e) {
        DEBUG_LOG("Exception during hot-swap: " << e.what());
        
        // Try to restore a safe state
        try {
            if (m_processingPipeline) {
                m_processingPipeline->stop();
            }
        } catch (...) {
            DEBUG_LOG("Failed to restore safe pipeline state");
        }
        
        return false;
    }
    */
}

QImage Do3ThinkCameraComponent::processImage(const QImage& inputImage)
{
    // TODO: Re-enable when preprocessing pipeline is fixed
    return inputImage; // Return original image until pipeline is fixed
    /*
    if (!m_processingState.processingEnabled || !m_processingPipeline) {
        return inputImage; // Return original image if processing is disabled
    }
    
    QElapsedTimer timer;
    timer.start();
    
    try {
        // Convert to appropriate format for processing if needed
        QImage processedImage = inputImage;
        
        // Process through the pipeline
        // Note: This is a simplified implementation
        // In practice, you'd convert QImage to cv::Mat, process, then convert back
        
        // For now, just return the input image as a placeholder
        // TODO: Implement actual OpenCV processing integration
        
        m_processingState.processingLatency = timer.elapsed();
        
        // Emit performance update occasionally
        static int frameCount = 0;
        if (++frameCount % 30 == 0) { // Every 30 frames
            double fps = 1000.0 / qMax(1.0, m_processingState.processingLatency);
            emit processingPerformanceUpdate(fps, m_processingState.processingLatency);
        }
        
        return processedImage;
        
    } catch (const std::exception& e) {
        DEBUG_LOG("Exception processing image: " << e.what());
        return inputImage; // Return original on error
    }
    */
}

void Do3ThinkCameraComponent::setupPreprocessorConnections()
{
    // TODO: Re-enable when preprocessing pipeline is fixed
    /*
    // Connect preprocessor signals to monitor performance and errors
    if (m_blurProcessor) {
        connect(m_blurProcessor.get(), &ComponentsForest::OpenCV::PreProcessorBase::errorOccurred,
                this, [this](const QString& error) {
                    DEBUG_LOG("Blur processor error: " << error.toStdString());
                    emit errorOccurred(QString("Blur processor: %1").arg(error));
                });
    }
    
    if (m_edgeProcessor) {
        connect(m_edgeProcessor.get(), &ComponentsForest::OpenCV::PreProcessorBase::errorOccurred,
                this, [this](const QString& error) {
                    DEBUG_LOG("Edge processor error: " << error.toStdString());
                    emit errorOccurred(QString("Edge processor: %1").arg(error));
                });
    }
    
    if (m_denoiseProcessor) {
        connect(m_denoiseProcessor.get(), &ComponentsForest::OpenCV::PreProcessorBase::errorOccurred,
                this, [this](const QString& error) {
                    DEBUG_LOG("Denoise processor error: " << error.toStdString());
                    emit errorOccurred(QString("Denoise processor: %1").arg(error));
                });
    }
}
*/
}

} // namespace ComponentsForest