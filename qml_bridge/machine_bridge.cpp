#include "machine_bridge.h"
#include "camera_bridge.h"
#include <QThread>
#include <QTimer>
#include <QMetaObject>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDebug>
#include <QCoreApplication>

// Private implementation class
class MachineBridge::Private
{
public:
    QObject* machine = nullptr;              // Machine instance (no compile-time dependency)
    QThread* machineThread = nullptr;        // Thread for Machine
    QList<CameraBridge*> cameras;            // Managed camera bridges
    QTimer* statisticsTimer = nullptr;       // Update timer
    QVariantMap statistics;                  // Current statistics
    QString currentState = "Uninitialized";  // Machine state
    bool running = false;                    // Running flag
    int nextCameraId = 1;                    // For generating unique IDs
    
    // Configuration
    QVariantMap configuration;
    
    // State mapping (must match ComponentState enum in base_component.h)
    QMap<int, QString> stateMap = {
        {0, "Uninitialized"},
        {1, "Initialized"},
        {2, "Starting"},
        {3, "Running"},
        {4, "Stopping"},
        {5, "Stopped"},
        {6, "Error"},
        {7, "Recovering"},
        {8, "Paused"}
    };
};

MachineBridge::MachineBridge(QObject* parent)
    : QObject(parent)
    , d(std::make_unique<Private>())
{
    // Initialize statistics timer
    d->statisticsTimer = new QTimer(this);
    d->statisticsTimer->setInterval(1000); // Update every second
    connect(d->statisticsTimer, &QTimer::timeout, 
            this, &MachineBridge::updateStatistics);
    
    // Set default configuration
    d->configuration["autoStart"] = false;
    d->configuration["maxCameras"] = 10;
    d->configuration["enableLogging"] = true;
    d->configuration["logLevel"] = "INFO";
}

MachineBridge::~MachineBridge()
{
    shutdown();
}

// Property getters
bool MachineBridge::isRunning() const
{
    return d->running;
}

QString MachineBridge::state() const
{
    return d->currentState;
}

QList<CameraBridge*> MachineBridge::cameras() const
{
    return d->cameras;
}

int MachineBridge::cameraCount() const
{
    return d->cameras.size();
}

QVariantMap MachineBridge::statistics() const
{
    return d->statistics;
}

// Machine lifecycle
bool MachineBridge::initialize()
{
    if (d->machine) {
        qDebug() << "MachineBridge: Already initialized";
        return true;
    }
    
    if (!createMachine()) {
        qWarning() << "MachineBridge: Failed to create Machine";
        return false;
    }
    
    // Initialize the machine via signal
    bool result = false;
    QMetaObject::invokeMethod(d->machine, "initialize", 
                              Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(bool, result));
    
    if (result) {
        d->statisticsTimer->start();
        emit machineInitialized();
        emit statusMessage("Machine initialized successfully");
    } else {
        destroyMachine();
        emit machineError("Failed to initialize machine");
    }
    
    return result;
}

void MachineBridge::shutdown()
{
    qDebug() << "MachineBridge: Shutting down";
    
    // Stop statistics timer
    if (d->statisticsTimer) {
        d->statisticsTimer->stop();
    }
    
    // Remove all cameras
    removeAllCameras();
    
    // Shutdown machine
    if (d->machine) {
        emit requestMachineShutdown();
        
        // Give it time to shutdown gracefully
        QThread::msleep(100);
        
        destroyMachine();
    }
    
    d->currentState = "Uninitialized";
    d->running = false;
    emit stateChanged(d->currentState);
    emit runningChanged(false);
}

// Machine control
bool MachineBridge::startMachine()
{
    if (!d->machine) {
        qWarning() << "MachineBridge: Machine not initialized";
        return false;
    }
    
    emit requestMachineStart();
    emit statusMessage("Starting machine...");
    return true;
}

bool MachineBridge::stopMachine()
{
    if (!d->machine) {
        return false;
    }
    
    emit requestMachineStop();
    emit statusMessage("Stopping machine...");
    return true;
}

bool MachineBridge::pauseMachine()
{
    if (!d->machine || !d->running) {
        return false;
    }
    
    emit requestMachinePause();
    emit statusMessage("Pausing machine...");
    return true;
}

bool MachineBridge::resumeMachine()
{
    if (!d->machine || d->currentState != "Paused") {
        return false;
    }
    
    emit requestMachineResume();
    emit statusMessage("Resuming machine...");
    return true;
}

// Camera management
CameraBridge* MachineBridge::addCamera(const QString& type, const QVariantMap& config)
{
    if (!d->machine) {
        qWarning() << "MachineBridge: Machine not initialized";
        return nullptr;
    }
    
    if (d->cameras.size() >= d->configuration["maxCameras"].toInt()) {
        qWarning() << "MachineBridge: Maximum camera limit reached";
        emit machineError("Maximum camera limit reached");
        return nullptr;
    }
    
    if (!validateCameraType(type)) {
        qWarning() << "MachineBridge: Invalid camera type:" << type;
        return nullptr;
    }
    
    // Create camera bridge
    QVariantMap cameraConfig = config;
    if (cameraConfig.isEmpty()) {
        cameraConfig = getDefaultCameraConfig(type);
    }
    
    // Add unique ID if not present
    if (!cameraConfig.contains("id")) {
        cameraConfig["id"] = QString("camera_%1").arg(d->nextCameraId++);
    }
    
    CameraBridge* bridge = createCameraBridge(type, cameraConfig);
    if (!bridge) {
        qWarning() << "MachineBridge: Failed to create camera bridge";
        return nullptr;
    }
    
    // Initialize the camera
    if (!bridge->initialize()) {
        qWarning() << "MachineBridge: Failed to initialize camera";
        delete bridge;
        return nullptr;
    }
    
    // Add to list
    d->cameras.append(bridge);
    connectCameraBridge(bridge);
    
    emit cameraAdded(bridge);
    emit camerasChanged();
    emit statusMessage(QString("Camera added: %1").arg(bridge->serialNumber()));
    
    return bridge;
}

bool MachineBridge::removeCamera(CameraBridge* camera)
{
    if (!camera) {
        return false;
    }
    
    int index = d->cameras.indexOf(camera);
    if (index < 0) {
        return false;
    }
    
    return removeCamera(index);
}

bool MachineBridge::removeCamera(int index)
{
    if (index < 0 || index >= d->cameras.size()) {
        return false;
    }
    
    CameraBridge* camera = d->cameras.at(index);
    
    // Disconnect and stop camera
    camera->disconnect();
    disconnectCameraBridge(camera);
    
    // Remove from list
    d->cameras.removeAt(index);
    
    // Schedule deletion
    camera->deleteLater();
    
    emit cameraRemoved(index);
    emit camerasChanged();
    emit statusMessage(QString("Camera removed at index %1").arg(index));
    
    return true;
}

void MachineBridge::removeAllCameras()
{
    if (d->cameras.isEmpty()) {
        return;
    }
    
    // Stop all cameras first
    stopAllCameras();
    
    // Disconnect all cameras
    for (CameraBridge* camera : d->cameras) {
        camera->disconnect();
        disconnectCameraBridge(camera);
        camera->deleteLater();
    }
    
    d->cameras.clear();
    
    emit allCamerasRemoved();
    emit camerasChanged();
    emit statusMessage("All cameras removed");
}

CameraBridge* MachineBridge::cameraAt(int index) const
{
    if (index < 0 || index >= d->cameras.size()) {
        return nullptr;
    }
    return d->cameras.at(index);
}

CameraBridge* MachineBridge::findCamera(const QString& serialNumber) const
{
    for (CameraBridge* camera : d->cameras) {
        if (camera->serialNumber() == serialNumber) {
            return camera;
        }
    }
    return nullptr;
}

// Batch operations
bool MachineBridge::startAllCameras()
{
    if (d->cameras.isEmpty()) {
        return false;
    }
    
    int started = 0;
    int total = d->cameras.size();
    
    for (CameraBridge* camera : d->cameras) {
        emit progressUpdate(started, total);
        if (camera->startAcquisition()) {
            started++;
        }
    }
    
    emit progressUpdate(started, total);
    emit statusMessage(QString("Started %1/%2 cameras").arg(started).arg(total));
    
    return started > 0;
}

bool MachineBridge::stopAllCameras()
{
    if (d->cameras.isEmpty()) {
        return false;
    }
    
    int stopped = 0;
    int total = d->cameras.size();
    
    for (CameraBridge* camera : d->cameras) {
        emit progressUpdate(stopped, total);
        if (camera->stopAcquisition()) {
            stopped++;
        }
    }
    
    emit progressUpdate(stopped, total);
    emit statusMessage(QString("Stopped %1/%2 cameras").arg(stopped).arg(total));
    
    return stopped > 0;
}

bool MachineBridge::connectAllCameras()
{
    if (d->cameras.isEmpty()) {
        return false;
    }
    
    int connected = 0;
    int total = d->cameras.size();
    
    for (CameraBridge* camera : d->cameras) {
        emit progressUpdate(connected, total);
        if (camera->connect()) {
            connected++;
        }
    }
    
    emit progressUpdate(connected, total);
    emit statusMessage(QString("Connected %1/%2 cameras").arg(connected).arg(total));
    
    return connected > 0;
}

bool MachineBridge::disconnectAllCameras()
{
    if (d->cameras.isEmpty()) {
        return false;
    }
    
    int disconnected = 0;
    int total = d->cameras.size();
    
    for (CameraBridge* camera : d->cameras) {
        emit progressUpdate(disconnected, total);
        if (camera->disconnect()) {
            disconnected++;
        }
    }
    
    emit progressUpdate(disconnected, total);
    emit statusMessage(QString("Disconnected %1/%2 cameras").arg(disconnected).arg(total));
    
    return disconnected > 0;
}

// Configuration
bool MachineBridge::loadConfiguration(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "MachineBridge: Cannot open configuration file:" << filePath;
        return false;
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject()) {
        qWarning() << "MachineBridge: Invalid configuration format";
        return false;
    }
    
    QVariantMap config = doc.object().toVariantMap();
    setConfiguration(config);
    
    emit requestLoadConfiguration(filePath);
    emit statusMessage(QString("Configuration loaded from %1").arg(filePath));
    
    return true;
}

bool MachineBridge::saveConfiguration(const QString& filePath)
{
    QJsonDocument doc(QJsonObject::fromVariantMap(d->configuration));
    
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "MachineBridge: Cannot write configuration file:" << filePath;
        return false;
    }
    
    file.write(doc.toJson());
    
    emit requestSaveConfiguration(filePath);
    emit statusMessage(QString("Configuration saved to %1").arg(filePath));
    
    return true;
}

void MachineBridge::setConfiguration(const QVariantMap& config)
{
    d->configuration = config;
    
    // Apply configuration to existing components
    if (d->machine) {
        QMetaObject::invokeMethod(d->machine, "setConfiguration",
                                  Qt::QueuedConnection,
                                  Q_ARG(QVariantMap, config));
    }
}

QVariantMap MachineBridge::configuration() const
{
    return d->configuration;
}

// Diagnostics
QVariantList MachineBridge::getCameraStatuses() const
{
    QVariantList statuses;
    
    for (const CameraBridge* camera : d->cameras) {
        QVariantMap status;
        status["serialNumber"] = camera->serialNumber();
        status["modelName"] = camera->modelName();
        status["connected"] = camera->isConnected();
        status["acquiring"] = camera->isAcquiring();
        status["state"] = camera->state();
        status["fps"] = camera->currentFps();
        status["frameCount"] = camera->frameCount();
        status["errorCount"] = camera->errorCount();
        
        statuses.append(status);
    }
    
    return statuses;
}

QString MachineBridge::getMachineInfo() const
{
    QString info;
    info += QString("Machine State: %1\n").arg(d->currentState);
    info += QString("Running: %1\n").arg(d->running ? "Yes" : "No");
    info += QString("Camera Count: %1/%2\n").arg(d->cameras.size())
                                             .arg(d->configuration["maxCameras"].toInt());
    info += QString("Total Frames: %1\n").arg(d->statistics["totalFrames"].toLongLong());
    info += QString("Average FPS: %1\n").arg(d->statistics["averageFps"].toDouble());
    
    return info;
}

void MachineBridge::refreshDeviceList()
{
    if (!d->machine) {
        return;
    }
    
    // Trigger device refresh via Machine
    QMetaObject::invokeMethod(d->machine, "refreshDevices",
                              Qt::QueuedConnection);
    
    emit statusMessage("Refreshing device list...");
}

// Private slots
void MachineBridge::onMachineStateChanged(int state)
{
    QString newState = stateToString(state);
    if (newState != d->currentState) {
        d->currentState = newState;
        d->running = (newState == "Running");
        
        emit stateChanged(d->currentState);
        emit runningChanged(d->running);
        
        // Emit specific state signals
        if (newState == "Running") {
            emit machineStarted();
        } else if (newState == "Stopped") {
            emit machineStopped();
        } else if (newState == "Paused") {
            emit machinePaused();
        }
    }
}

void MachineBridge::onMachineError(const QString& error)
{
    emit machineError(error);
    emit statusMessage(QString("Machine error: %1").arg(error));
}

void MachineBridge::onMachineStatisticsUpdated(const QVariantMap& stats)
{
    d->statistics = stats;
    emit statisticsChanged();
}

void MachineBridge::onComponentAdded(QObject* component)
{
    // Check if it's a camera component
    QString className = component->metaObject()->className();
    if (!className.contains("Camera")) {
        return;
    }
    
    // Find corresponding bridge or create new one
    QString serialNumber;
    QMetaObject::invokeMethod(component, "serialNumber",
                              Qt::DirectConnection,
                              Q_RETURN_ARG(QString, serialNumber));
    
    if (!findCamera(serialNumber)) {
        // Component added externally, create bridge for it
        QVariantMap config;
        config["serialNumber"] = serialNumber;
        
        QString type = "Generic";
        if (className.contains("Do3Think")) {
            type = "Do3Think";
        }
        
        addCamera(type, config);
    }
}

void MachineBridge::onComponentRemoved(const QString& id)
{
    // Find and remove corresponding camera bridge
    for (int i = 0; i < d->cameras.size(); ++i) {
        if (d->cameras[i]->serialNumber() == id) {
            removeCamera(i);
            break;
        }
    }
}

void MachineBridge::onCameraStateChanged()
{
    updateStatistics();
}

void MachineBridge::onCameraError(const QString& error)
{
    CameraBridge* camera = qobject_cast<CameraBridge*>(sender());
    if (camera) {
        QString message = QString("Camera %1 error: %2")
                         .arg(camera->serialNumber())
                         .arg(error);
        emit statusMessage(message);
    }
}

void MachineBridge::handleCameraDestroyed(QObject* obj)
{
    // Remove destroyed camera from list
    CameraBridge* camera = static_cast<CameraBridge*>(obj);
    d->cameras.removeAll(camera);
    emit camerasChanged();
}

// Internal updates
void MachineBridge::updateStatistics()
{
    QVariantMap stats;
    
    // Aggregate camera statistics
    qint64 totalFrames = 0;
    double totalFps = 0.0;
    int activeCount = 0;
    int errorCount = 0;
    
    for (const CameraBridge* camera : d->cameras) {
        if (camera->isAcquiring()) {
            activeCount++;
            totalFps += camera->currentFps();
        }
        totalFrames += camera->frameCount();
        errorCount += camera->errorCount();
    }
    
    stats["totalFrames"] = totalFrames;
    stats["averageFps"] = activeCount > 0 ? totalFps / activeCount : 0.0;
    stats["activeCameras"] = activeCount;
    stats["totalCameras"] = d->cameras.size();
    stats["totalErrors"] = errorCount;
    stats["uptime"] = QCoreApplication::applicationPid(); // Placeholder for actual uptime
    
    if (stats != d->statistics) {
        d->statistics = stats;
        emit statisticsChanged();
    }
}

void MachineBridge::updateMachineState()
{
    if (!d->machine) {
        return;
    }
    
    // Query machine state
    int state = 0;
    QMetaObject::invokeMethod(d->machine, "state",
                              Qt::DirectConnection,
                              Q_RETURN_ARG(int, state));
    
    onMachineStateChanged(state);
}

// Helper methods
bool MachineBridge::createMachine()
{
    if (d->machine) {
        return true;
    }
    
    // Create Machine instance dynamically to avoid compile-time dependency
    const QMetaObject* metaObj = QMetaType::fromName("Machine").metaObject();
    if (!metaObj) {
        qWarning() << "MachineBridge: Machine class not registered";
        return false;
    }
    
    d->machine = metaObj->newInstance(Q_ARG(QObject*, nullptr));
    if (!d->machine) {
        qWarning() << "MachineBridge: Failed to create Machine instance";
        return false;
    }
    
    // Create thread for Machine
    d->machineThread = new QThread(this);
    d->machineThread->setObjectName("MachineThread");
    
    // Move Machine to its thread
    d->machine->moveToThread(d->machineThread);
    
    // Connect thread lifecycle
    connect(d->machineThread, &QThread::started,
            d->machine, [this]() {
                qDebug() << "Machine thread started";
            });
    
    connect(d->machineThread, &QThread::finished,
            d->machine, &QObject::deleteLater);
    
    // Start thread
    d->machineThread->start();
    
    // Connect signals
    connectMachineSignals();
    
    return true;
}

void MachineBridge::destroyMachine()
{
    if (!d->machine) {
        return;
    }
    
    disconnectMachineSignals();
    
    // Stop thread
    if (d->machineThread) {
        d->machineThread->quit();
        d->machineThread->wait(5000);
        delete d->machineThread;
        d->machineThread = nullptr;
    }
    
    d->machine = nullptr;
}

void MachineBridge::connectMachineSignals()
{
    if (!d->machine) {
        return;
    }
    
    // Connect Machine signals to bridge slots (string-based for decoupling)
    connect(d->machine, SIGNAL(stateChanged(int)),
            this, SLOT(onMachineStateChanged(int)),
            Qt::QueuedConnection);
    
    connect(d->machine, SIGNAL(errorOccurred(QString)),
            this, SLOT(onMachineError(QString)),
            Qt::QueuedConnection);
    
    connect(d->machine, SIGNAL(statisticsUpdated(QVariantMap)),
            this, SLOT(onMachineStatisticsUpdated(QVariantMap)),
            Qt::QueuedConnection);
    
    connect(d->machine, SIGNAL(componentAdded(QObject*)),
            this, SLOT(onComponentAdded(QObject*)),
            Qt::QueuedConnection);
    
    connect(d->machine, SIGNAL(componentRemoved(QString)),
            this, SLOT(onComponentRemoved(QString)),
            Qt::QueuedConnection);
    
    // Connect bridge signals to Machine slots
    connect(this, SIGNAL(requestMachineStart()),
            d->machine, SLOT(start()),
            Qt::QueuedConnection);
    
    connect(this, SIGNAL(requestMachineStop()),
            d->machine, SLOT(stop()),
            Qt::QueuedConnection);
    
    connect(this, SIGNAL(requestMachinePause()),
            d->machine, SLOT(pause()),
            Qt::QueuedConnection);
    
    connect(this, SIGNAL(requestMachineResume()),
            d->machine, SLOT(resume()),
            Qt::QueuedConnection);
    
    connect(this, SIGNAL(requestMachineShutdown()),
            d->machine, SLOT(shutdown()),
            Qt::QueuedConnection);
}

void MachineBridge::disconnectMachineSignals()
{
    if (!d->machine) {
        return;
    }
    
    // Disconnect all signals
    disconnect(d->machine, nullptr, this, nullptr);
    disconnect(this, nullptr, d->machine, nullptr);
}

CameraBridge* MachineBridge::createCameraBridge(const QString& type, const QVariantMap& config)
{
    CameraBridge* bridge = new CameraBridge(this);
    
    // Set camera type-specific configuration
    QVariantMap fullConfig = config;
    fullConfig["type"] = type;
    
    if (type == "Do3Think") {
        fullConfig["componentClass"] = "Do3ThinkCameraComponent";
    } else {
        fullConfig["componentClass"] = "CameraComponent";
    }
    
    bridge->setConfiguration(fullConfig);
    
    return bridge;
}

void MachineBridge::connectCameraBridge(CameraBridge* bridge)
{
    if (!bridge) {
        return;
    }
    
    // Connect camera state changes
    connect(bridge, &CameraBridge::stateChanged,
            this, &MachineBridge::onCameraStateChanged);
    
    connect(bridge, &CameraBridge::errorOccurred,
            this, &MachineBridge::onCameraError);
    
    // Monitor destruction
    connect(bridge, &QObject::destroyed,
            this, &MachineBridge::handleCameraDestroyed);
}

void MachineBridge::disconnectCameraBridge(CameraBridge* bridge)
{
    if (!bridge) {
        return;
    }
    
    disconnect(bridge, nullptr, this, nullptr);
}

QString MachineBridge::stateToString(int state) const
{
    return d->stateMap.value(state, "Unknown");
}

int MachineBridge::stringToState(const QString& state) const
{
    return d->stateMap.key(state, 0);
}

void MachineBridge::cleanupCameras()
{
    for (CameraBridge* camera : d->cameras) {
        disconnectCameraBridge(camera);
        camera->deleteLater();
    }
    d->cameras.clear();
}

bool MachineBridge::validateCameraType(const QString& type) const
{
    // List of supported camera types
    static const QStringList supportedTypes = {
        "Do3Think",
        "Generic",
        "Basler",
        "FLIR",
        "HIKVision"
    };
    
    return supportedTypes.contains(type);
}

QVariantMap MachineBridge::getDefaultCameraConfig(const QString& type) const
{
    QVariantMap config;
    
    // Common defaults
    config["autoConnect"] = true;
    config["autoReconnect"] = true;
    config["reconnectInterval"] = 5000;
    config["frameBufferSize"] = 10;
    config["enableStatistics"] = true;
    
    // Type-specific defaults
    if (type == "Do3Think") {
        config["triggerMode"] = "Continuous";
        config["exposureTime"] = 10000;
        config["gain"] = 1.0;
        config["targetFps"] = 30.0;
        config["pixelFormat"] = "Mono8";
    } else if (type == "Basler") {
        config["packetSize"] = 1500;
        config["interPacketDelay"] = 0;
    } else if (type == "FLIR") {
        config["bufferMode"] = "NewestOnly";
        config["bufferCount"] = 10;
    }
    
    return config;
}