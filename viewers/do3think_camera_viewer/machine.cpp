#include "machine.h"
#include "../../Do3ThinkCamera/dothink_camera.h"
#include "../../components/base_component.h"
#include "../../OpenCV/threshold_preprocessor.h"
#include "../../OpenCV/contour_area_algorithm.h"

#include <iostream>
#include <fstream>
#include <cstdio>
#include <string>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#endif

#include <QThread>
#include <QMutex>
#include <QMutexLocker>
#include <QDebug>
#include <QTimer>
#include <QMetaObject>
#include <QJsonObject>
#include <QJsonDocument>

// Simple debug output that works independently
#define DEBUG_LOG(msg) do { \
    std::cerr << "[Machine] " << msg << std::endl; \
    std::cerr.flush(); \
} while(0)

namespace ComponentsForest {

// Private implementation class
class Do3ThinkCameraMachine::Private
{
public:
    Private(Do3ThinkCameraMachine* q) 
        : q(q)
        , isRunning(false)
    {
        std::cerr << "[DEBUG] Do3ThinkCameraMachine::Private constructor" << std::endl;
    }
    
    ~Private()
    {
        std::cerr << "[DEBUG] Do3ThinkCameraMachine::Private destructor" << std::endl;
        // Clean up all components
        cleanupAllComponents();
    }
    
    void cleanupAllComponents()
    {
        QMutexLocker locker(&componentsMutex);
        
        // Stop all components first
        for (auto* component : components.values()) {
            if (component) {
                component->stop();
                // cleanup() method doesn't exist, stop() handles cleanup
            }
        }
        
        // Delete component threads
        for (auto* thread : componentThreads.values()) {
            if (thread) {
                thread->quit();
                if (!thread->wait(5000)) {
                    qWarning() << "Component thread failed to stop, terminating";
                    thread->terminate();
                    thread->wait();
                }
                delete thread;
            }
        }
        componentThreads.clear();
        
        // Delete components
        qDeleteAll(components);
        components.clear();
    }
    
    Do3ThinkCameraMachine* q;
    
    // Component management
    QMap<QString, Do3ThinkCameraComponent*> components;
    QMap<QString, QThread*> componentThreads;
    QMutex componentsMutex;

    // Analysis components
    OpenCV::ThresholdPreProcessor* thresholdProcessor = nullptr;
    OpenCV::ContourAreaAlgorithm* contourAlgorithm = nullptr;
    
    // Default configuration
    QVariantMap defaultCameraConfig;
    
    // Machine state
    bool isRunning;
    QMutex stateMutex;
    
    // Device discovery
    QTimer* deviceDiscoveryTimer{nullptr};
    QStringList lastDiscoveredDevices;
};

void Do3ThinkCameraMachine::runContourAnalysis(const QString& cameraId)
{
    auto* camera = getCameraComponent(cameraId);
    if (!camera) {
        qWarning() << "Cannot run analysis, camera not found:" << cameraId;
        return;
    }

    if (!d->thresholdProcessor || !d->contourAlgorithm) {
        qWarning() << "Analysis components not initialized.";
        return;
    }

    qDebug() << "Connecting processing chain for analysis...";

    // Disconnect any previous connections to avoid multiple signals
    disconnect(camera, &Do3ThinkCameraComponent::frameReady, nullptr, nullptr);

    // 1. Connect Camera output to ThresholdPreprocessor input
    connect(camera, &Do3ThinkCameraComponent::frameReady,
            d->thresholdProcessor, &OpenCV::ThresholdPreProcessor::onFrameReceived);

    // 2. Connect ThresholdPreprocessor output to ContourAreaAlgorithm input
    connect(d->thresholdProcessor, &OpenCV::ThresholdPreProcessor::frameProcessed,
            d->contourAlgorithm, &OpenCV::ContourAreaAlgorithm::onFrameReceived);

    // 3. Connect ContourAreaAlgorithm output to this machine's signal
    connect(d->contourAlgorithm, &OpenCV::ContourAreaAlgorithm::resultReady,
            this, [this, cameraId](const QVariant& result){
                emit contourAnalysisResult(cameraId, result.toList());
            });

    qDebug() << "Analysis chain connected. Waiting for next frame from" << cameraId;
}

// Constructor
Do3ThinkCameraMachine::Do3ThinkCameraMachine(QObject *parent)
    : QObject(parent)
    , d(std::make_unique<Private>(this))
{
    DEBUG_LOG("Do3ThinkCameraMachine constructor entered");
    std::cerr << "[DEBUG] Do3ThinkCameraMachine constructor - creating machine" << std::endl;
    
    try {
        // Set default camera configuration
        DEBUG_LOG("Setting default camera configuration");
        QVariantMap defaultConfig;
        defaultConfig["resolution"] = "1920x1080";
        defaultConfig["fps"] = 30;
        defaultConfig["exposure"] = 10000; // 10ms
        defaultConfig["gain"] = 1.0;
        defaultConfig["useCallback"] = true;
        defaultConfig["bufferCount"] = 5;
        defaultConfig["debugMode"] = false;
        d->defaultCameraConfig = defaultConfig;
        DEBUG_LOG("Default configuration set");
        
        // Create device discovery timer
        DEBUG_LOG("Creating device discovery timer");
        d->deviceDiscoveryTimer = new QTimer(this);
        if (!d->deviceDiscoveryTimer) {
            DEBUG_LOG("ERROR: Failed to create device discovery timer");
        } else {
            d->deviceDiscoveryTimer->setInterval(5000); // Check every 5 seconds
            connect(d->deviceDiscoveryTimer, &QTimer::timeout,
                    this, &Do3ThinkCameraMachine::refreshDeviceList);
            DEBUG_LOG("Device discovery timer created and connected");
        }

        // Create and initialize analysis components
        DEBUG_LOG("Creating analysis components");
        d->thresholdProcessor = new OpenCV::ThresholdPreProcessor();
        d->contourAlgorithm = new OpenCV::ContourAreaAlgorithm();

        d->thresholdProcessor->initialize(QJsonObject());
        d->contourAlgorithm->initialize(QJsonObject());
        d->thresholdProcessor->start();
        d->contourAlgorithm->start();
        DEBUG_LOG("Analysis components created and started");
        
        DEBUG_LOG("Do3ThinkCameraMachine constructor completed");
        
    } catch (const std::exception& e) {
        std::stringstream ss;
        ss << "EXCEPTION in Do3ThinkCameraMachine constructor: " << e.what();
        DEBUG_LOG(ss.str());
    } catch (...) {
        DEBUG_LOG("UNKNOWN EXCEPTION in Do3ThinkCameraMachine constructor");
    }
}

// Destructor
Do3ThinkCameraMachine::~Do3ThinkCameraMachine()
{
    DEBUG_LOG("Do3ThinkCameraMachine destructor entered");
    stop();
    DEBUG_LOG("Do3ThinkCameraMachine destructor completed");
}

// Camera management
bool Do3ThinkCameraMachine::addCamera(const QString &cameraId, const QVariantMap &config)
{
    QMutexLocker locker(&d->componentsMutex);
    
    if (d->components.contains(cameraId)) {
        qWarning() << "Camera already exists:" << cameraId;
        return false;
    }
    
    // Create component
    auto* component = new Do3ThinkCameraComponent();
    component->setObjectName(cameraId);
    
    // Create thread for component
    auto* thread = new QThread(this);
    thread->setObjectName(QString("%1_Thread").arg(cameraId));
    
    // Move component to thread
    component->moveToThread(thread);
    
    // Connect component lifecycle
    connect(thread, &QThread::started,
            [component, config]() {
                // Convert QVariantMap to QJsonObject
                QJsonObject jsonConfig = QJsonObject::fromVariantMap(config);
                component->initialize(jsonConfig);
            });
    
    connect(thread, &QThread::finished,
            component, &QObject::deleteLater);
    
    // Connect component signals
    connectComponentSignals(component);
    
    // Store component and thread
    d->components[cameraId] = component;
    d->componentThreads[cameraId] = thread;
    
    // Start thread
    thread->start();
    
    // Emit signal
    emit cameraAdded(cameraId);
    
    qDebug() << "Camera added successfully:" << cameraId;
    return true;
}

bool Do3ThinkCameraMachine::removeCamera(const QString &cameraId)
{
    QMutexLocker locker(&d->componentsMutex);
    
    auto* component = d->components.take(cameraId);
    auto* thread = d->componentThreads.take(cameraId);
    
    if (!component || !thread) {
        qWarning() << "Camera not found:" << cameraId;
        return false;
    }
    
    // Disconnect signals
    disconnectComponentSignals(component);
    
    // Stop component
    QMetaObject::invokeMethod(component, "stop", Qt::BlockingQueuedConnection);
    QMetaObject::invokeMethod(component, "cleanup", Qt::BlockingQueuedConnection);
    
    // Stop thread
    thread->quit();
    if (!thread->wait(5000)) {
        qWarning() << "Component thread failed to stop, terminating";
        thread->terminate();
        thread->wait();
    }
    
    // Delete thread (component will be deleted by deleteLater)
    delete thread;
    
    // Emit signal
    emit cameraRemoved(cameraId);
    
    qDebug() << "Camera removed successfully:" << cameraId;
    return true;
}

bool Do3ThinkCameraMachine::hasCamera(const QString &cameraId) const
{
    QMutexLocker locker(&d->componentsMutex);
    return d->components.contains(cameraId);
}

QStringList Do3ThinkCameraMachine::getCameraIds() const
{
    QMutexLocker locker(&d->componentsMutex);
    return d->components.keys();
}

Do3ThinkCameraComponent* Do3ThinkCameraMachine::getCameraComponent(const QString &cameraId) const
{
    QMutexLocker locker(&d->componentsMutex);
    return d->components.value(cameraId, nullptr);
}

BaseComponent* Do3ThinkCameraMachine::getComponent(const QString &cameraId) const
{
    return getCameraComponent(cameraId);
}

// Machine control
bool Do3ThinkCameraMachine::start()
{
    DEBUG_LOG("Do3ThinkCameraMachine::start() entered");
    
    QMutexLocker locker(&d->stateMutex);
    
    if (d->isRunning) {
        qWarning() << "Machine already running";
        DEBUG_LOG("Machine already running");
        return false;
    }
    
    d->isRunning = true;
    DEBUG_LOG("Machine state set to running");
    
    // Start device discovery
    DEBUG_LOG("Starting device discovery timer");
    d->deviceDiscoveryTimer->start();
    
    // Initial device scan
    DEBUG_LOG("Performing initial device scan");
    refreshDeviceList();
    
    emit machineStarted();
    DEBUG_LOG("machineStarted signal emitted");
    
    qDebug() << "Machine started successfully";
    DEBUG_LOG("Do3ThinkCameraMachine::start() completed");
    return true;
}

bool Do3ThinkCameraMachine::stop()
{
    QMutexLocker locker(&d->stateMutex);
    
    if (!d->isRunning) {
        return false;
    }
    
    // Stop device discovery
    d->deviceDiscoveryTimer->stop();
    
    // Stop all cameras
    stopAllCameras();
    
    d->isRunning = false;
    
    emit machineStopped();
    
    qDebug() << "Machine stopped successfully";
    return true;
}

bool Do3ThinkCameraMachine::isRunning() const
{
    QMutexLocker locker(&d->stateMutex);
    return d->isRunning;
}

// Camera control
bool Do3ThinkCameraMachine::startCamera(const QString &cameraId)
{
    auto* component = getCameraComponent(cameraId);
    if (!component) {
        qWarning() << "Camera not found:" << cameraId;
        return false;
    }
    
    bool success = false;
    QMetaObject::invokeMethod(component, "start",
                              Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(bool, success));
    
    if (success) {
        emit cameraStarted(cameraId);
    } else {
        emit cameraError(cameraId, "Failed to start camera");
    }
    
    return success;
}

bool Do3ThinkCameraMachine::stopCamera(const QString &cameraId)
{
    auto* component = getCameraComponent(cameraId);
    if (!component) {
        qWarning() << "Camera not found:" << cameraId;
        return false;
    }
    
    bool success = false;
    QMetaObject::invokeMethod(component, "stop",
                              Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(bool, success));
    
    if (success) {
        emit cameraStopped(cameraId);
    } else {
        emit cameraError(cameraId, "Failed to stop camera");
    }
    
    return success;
}

bool Do3ThinkCameraMachine::startAllCameras()
{
    QMutexLocker locker(&d->componentsMutex);
    
    bool allSuccess = true;
    for (const QString& cameraId : d->components.keys()) {
        locker.unlock();
        bool success = startCamera(cameraId);
        locker.relock();
        allSuccess = allSuccess && success;
    }
    
    return allSuccess;
}

bool Do3ThinkCameraMachine::stopAllCameras()
{
    QMutexLocker locker(&d->componentsMutex);
    
    bool allSuccess = true;
    for (const QString& cameraId : d->components.keys()) {
        locker.unlock();
        bool success = stopCamera(cameraId);
        locker.relock();
        allSuccess = allSuccess && success;
    }
    
    return allSuccess;
}

// Configuration
void Do3ThinkCameraMachine::setDefaultCameraConfig(const QVariantMap &config)
{
    d->defaultCameraConfig = config;
}

QVariantMap Do3ThinkCameraMachine::getDefaultCameraConfig() const
{
    return d->defaultCameraConfig;
}

// Device discovery
QStringList Do3ThinkCameraMachine::discoverAvailableDevices()
{
    QStringList devices;
    
    // Create a temporary component to scan for devices
    Do3ThinkCameraComponent tempComponent;
    
    // Initialize the component with minimal configuration
    QJsonObject config;
    config["debugMode"] = false;
    tempComponent.initialize(config);
    
    // Scan for available devices using the SDK
    QList<CameraDeviceInfo> deviceList = tempComponent.scanDevices();
    
    // Convert device info to string list
    for (const CameraDeviceInfo& device : deviceList) {
        // Use serial number as the primary identifier
        // If serial number is empty, use friendly name
        QString deviceId = device.serialNumber.isEmpty() 
                          ? device.friendlyName 
                          : device.serialNumber;
        
        if (!deviceId.isEmpty()) {
            devices << deviceId;
        }
    }
    
    // If no real devices found, return empty list (no mock data)
    if (devices.isEmpty()) {
        qDebug() << "No Do3Think cameras detected";
    } else {
        qDebug() << "Found" << devices.size() << "Do3Think camera(s):" << devices;
    }
    
    return devices;
}

bool Do3ThinkCameraMachine::refreshDeviceList()
{
    QStringList devices = discoverAvailableDevices();
    
    // Check for changes
    bool changed = (devices != d->lastDiscoveredDevices);
    
    if (changed) {
        // Check for new devices
        for (const QString& device : devices) {
            if (!d->lastDiscoveredDevices.contains(device)) {
                emit deviceConnected(device);
            }
        }
        
        // Check for removed devices
        for (const QString& device : d->lastDiscoveredDevices) {
            if (!devices.contains(device)) {
                emit deviceDisconnected(device);
            }
        }
        
        d->lastDiscoveredDevices = devices;
    }
    
    emit devicesDiscovered(devices);
    
    return true;
}

// Private slots
void Do3ThinkCameraMachine::onComponentStateChanged()
{
    auto* component = qobject_cast<Do3ThinkCameraComponent*>(sender());
    if (!component) return;
    
    QString cameraId = component->objectName();
    
    // Get state from component
    QVariant stateVar;
    QMetaObject::invokeMethod(component, "currentState",
                              Qt::DirectConnection,
                              Q_RETURN_ARG(QVariant, stateVar));
    
    int state = stateVar.toInt();
    
    // Handle state changes
    switch (state) {
        case 3: // Running
            emit cameraStarted(cameraId);
            break;
        case 5: // Stopped
            emit cameraStopped(cameraId);
            break;
        case 6: // Error
            emit cameraError(cameraId, "Component entered error state");
            break;
    }
}

void Do3ThinkCameraMachine::onComponentError(const QString &error)
{
    auto* component = qobject_cast<Do3ThinkCameraComponent*>(sender());
    if (!component) return;
    
    QString cameraId = component->objectName();
    emit cameraError(cameraId, error);
    emit machineError(QString("Camera %1: %2").arg(cameraId, error));
}

void Do3ThinkCameraMachine::onComponentPerformanceUpdate(const QVariantMap &metrics)
{
    auto* component = qobject_cast<Do3ThinkCameraComponent*>(sender());
    if (!component) return;
    
    QString cameraId = component->objectName();
    emit performanceUpdate(cameraId, metrics);
}

// Helper methods
bool Do3ThinkCameraMachine::initializeComponent(const QString &cameraId, const QVariantMap &config)
{
    auto* component = getCameraComponent(cameraId);
    if (!component) {
        qWarning() << "Component not found:" << cameraId;
        return false;
    }
    
    bool success = false;
    QMetaObject::invokeMethod(component, "initialize",
                              Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(bool, success),
                              Q_ARG(QVariantMap, config));
    
    return success;
}

void Do3ThinkCameraMachine::cleanupComponent(const QString &cameraId)
{
    auto* component = getCameraComponent(cameraId);
    if (!component) return;
    
    QMetaObject::invokeMethod(component, "cleanup",
                              Qt::BlockingQueuedConnection);
}

void Do3ThinkCameraMachine::connectComponentSignals(Do3ThinkCameraComponent *component)
{
    if (!component) return;
    
    // Connect state change signals
    connect(component, SIGNAL(stateChanged(int)),
            this, SLOT(onComponentStateChanged()));
    
    // Connect error signals
    connect(component, SIGNAL(errorOccurred(QString)),
            this, SLOT(onComponentError(QString)));
    
    // Connect performance signals
    connect(component, SIGNAL(performanceMetricsUpdated(QVariantMap)),
            this, SLOT(onComponentPerformanceUpdate(QVariantMap)));
}

void Do3ThinkCameraMachine::disconnectComponentSignals(Do3ThinkCameraComponent *component)
{
    if (!component) return;
    
    // Disconnect all signals
    disconnect(component, nullptr, this, nullptr);
}

} // namespace ComponentsForest