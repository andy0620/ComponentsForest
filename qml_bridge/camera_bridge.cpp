#include "camera_bridge.h"
#include <QThread>
#include <QTimer>
#include <QMutexLocker>
#include <QMetaObject>
#include <QVariant>
#include <QDebug>
#include <QDateTime>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUuid>

// Static counter for unique bridge IDs
int CameraBridge::s_bridgeCounter = 0;

CameraBridge::CameraBridge(QObject *parent)
    : QObject(parent)
    , m_component(nullptr)
    , m_connected(false)
    , m_acquiring(false)
    , m_connectionStatus("Disconnected")
    , m_frameCount(0)
    , m_droppedFrames(0)
    , m_hasNewFrame(false)
{
    // Generate unique image provider ID for this bridge instance
    m_imageProviderId = QString("camera_bridge_%1").arg(++s_bridgeCounter);
    
    // Create statistics update timer
    m_statsTimer = new QTimer(this);
    m_statsTimer->setInterval(1000); // Update stats every second
    connect(m_statsTimer, &QTimer::timeout, this, &CameraBridge::updateStatistics);
    
    qDebug() << "CameraBridge created with provider ID:" << m_imageProviderId;
}

CameraBridge::~CameraBridge()
{
    disconnectFromComponent();
    
    if (m_statsTimer) {
        m_statsTimer->stop();
        delete m_statsTimer;
        m_statsTimer = nullptr;
    }
    
    qDebug() << "CameraBridge destroyed";
}

// Property implementations
QString CameraBridge::currentImageUrl() const
{
    QMutexLocker locker(&m_imageMutex);
    if (m_currentImageId.isEmpty()) {
        return QString();
    }
    // Return URL that QML can use with an image provider
    return QString("image://%1/%2").arg(m_imageProviderId, m_currentImageId);
}

void CameraBridge::setExposureTime(double value)
{
    if (qFuzzyCompare(m_exposureTime, value)) {
        return;
    }
    
    m_exposureTime = value;
    emit exposureTimeChanged();
    
    // Send request to component if connected
    if (m_component) {
        QMetaObject::invokeMethod(m_component, "setParameter",
                                  Qt::QueuedConnection,
                                  Q_ARG(QString, "exposureTime"),
                                  Q_ARG(QVariant, QVariant(value)));
    }
}

void CameraBridge::setGain(double value)
{
    if (qFuzzyCompare(m_gain, value)) {
        return;
    }
    
    m_gain = value;
    emit gainChanged();
    
    // Send request to component if connected
    if (m_component) {
        QMetaObject::invokeMethod(m_component, "setParameter",
                                  Qt::QueuedConnection,
                                  Q_ARG(QString, "gain"),
                                  Q_ARG(QVariant, QVariant(value)));
    }
}

void CameraBridge::setFrameRate(double value)
{
    if (qFuzzyCompare(m_frameRate, value)) {
        return;
    }
    
    m_frameRate = value;
    emit frameRateChanged();
    
    // Send request to component if connected
    if (m_component) {
        QMetaObject::invokeMethod(m_component, "setParameter",
                                  Qt::QueuedConnection,
                                  Q_ARG(QString, "frameRate"),
                                  Q_ARG(QVariant, QVariant(value)));
    }
}

void CameraBridge::setRoiX(int value)
{
    if (m_roiX == value) {
        return;
    }
    
    m_roiX = value;
    emit roiChanged();
    
    if (m_roiEnabled && m_component) {
        QVariantMap roi;
        roi["x"] = m_roiX;
        roi["y"] = m_roiY;
        roi["width"] = m_roiWidth;
        roi["height"] = m_roiHeight;
        setROI(roi);
    }
}

void CameraBridge::setRoiY(int value)
{
    if (m_roiY == value) {
        return;
    }
    
    m_roiY = value;
    emit roiChanged();
    
    if (m_roiEnabled && m_component) {
        QVariantMap roi;
        roi["x"] = m_roiX;
        roi["y"] = m_roiY;
        roi["width"] = m_roiWidth;
        roi["height"] = m_roiHeight;
        setROI(roi);
    }
}

void CameraBridge::setRoiWidth(int value)
{
    if (m_roiWidth == value) {
        return;
    }
    
    m_roiWidth = value;
    emit roiChanged();
    
    if (m_roiEnabled && m_component) {
        QVariantMap roi;
        roi["x"] = m_roiX;
        roi["y"] = m_roiY;
        roi["width"] = m_roiWidth;
        roi["height"] = m_roiHeight;
        setROI(roi);
    }
}

void CameraBridge::setRoiHeight(int value)
{
    if (m_roiHeight == value) {
        return;
    }
    
    m_roiHeight = value;
    emit roiChanged();
    
    if (m_roiEnabled && m_component) {
        QVariantMap roi;
        roi["x"] = m_roiX;
        roi["y"] = m_roiY;
        roi["width"] = m_roiWidth;
        roi["height"] = m_roiHeight;
        setROI(roi);
    }
}

void CameraBridge::setRoiEnabled(bool value)
{
    if (m_roiEnabled == value) {
        return;
    }
    
    m_roiEnabled = value;
    emit roiEnabledChanged();
    
    if (m_component) {
        if (value) {
            QVariantMap roi;
            roi["x"] = m_roiX;
            roi["y"] = m_roiY;
            roi["width"] = m_roiWidth;
            roi["height"] = m_roiHeight;
            setROI(roi);
        } else {
            clearROI();
        }
    }
}

// Component connection management
void CameraBridge::connectToComponent(QObject *cameraComponent)
{
    if (m_component == cameraComponent) {
        return;
    }
    
    // Disconnect from previous component
    if (m_component) {
        disconnectFromComponent();
    }
    
    if (!cameraComponent) {
        qWarning() << "CameraBridge: Cannot connect to null component";
        return;
    }
    
    m_component = cameraComponent;
    setupComponentConnections(cameraComponent);
    
    // Start statistics timer
    m_statsTimer->start();
    
    qDebug() << "CameraBridge connected to component:" << cameraComponent;
}

void CameraBridge::disconnectFromComponent()
{
    if (!m_component) {
        return;
    }
    
    // Stop statistics timer
    m_statsTimer->stop();
    
    teardownComponentConnections(m_component);
    m_component = nullptr;
    
    // Reset state
    m_connected = false;
    m_acquiring = false;
    m_connectionStatus = "Disconnected";
    emit connectedChanged();
    emit acquiringChanged();
    emit connectionStatusChanged();
    
    qDebug() << "CameraBridge disconnected from component";
}

void CameraBridge::setupComponentConnections(QObject *component)
{
    if (!component) {
        return;
    }
    
    // Use string-based connections to maintain decoupling
    // Component to Bridge connections
    connect(component, SIGNAL(frameReady(QImage,qint64)),
            this, SLOT(onComponentFrameReady(QImage,qint64)),
            Qt::QueuedConnection);
    
    connect(component, SIGNAL(stateChanged(int)),
            this, SLOT(onComponentStateChanged(int)),
            Qt::QueuedConnection);
    
    connect(component, SIGNAL(errorOccurred(QString)),
            this, SLOT(onComponentErrorOccurred(QString)),
            Qt::QueuedConnection);
    
    connect(component, SIGNAL(deviceListUpdated(QVariantList)),
            this, SLOT(onComponentDeviceListUpdated(QVariantList)),
            Qt::QueuedConnection);
    
    connect(component, SIGNAL(connected()),
            this, SLOT(onComponentConnected()),
            Qt::QueuedConnection);
    
    connect(component, SIGNAL(disconnected()),
            this, SLOT(onComponentDisconnected()),
            Qt::QueuedConnection);
    
    connect(component, SIGNAL(parameterChanged(QString,QVariant)),
            this, SLOT(onComponentParameterChanged(QString,QVariant)),
            Qt::QueuedConnection);
    
    connect(component, SIGNAL(statisticsUpdated(QVariantMap)),
            this, SLOT(onComponentStatisticsUpdated(QVariantMap)),
            Qt::QueuedConnection);
    
    // Bridge to Component connections
    connect(this, SIGNAL(requestScanDevices()),
            component, SLOT(scanDevices()),
            Qt::QueuedConnection);
    
    connect(this, SIGNAL(requestConnect(QString)),
            component, SLOT(connectDevice(QString)),
            Qt::QueuedConnection);
    
    connect(this, SIGNAL(requestDisconnect()),
            component, SLOT(disconnectDevice()),
            Qt::QueuedConnection);
    
    connect(this, SIGNAL(requestStartAcquisition()),
            component, SLOT(startAcquisition()),
            Qt::QueuedConnection);
    
    connect(this, SIGNAL(requestStopAcquisition()),
            component, SLOT(stopAcquisition()),
            Qt::QueuedConnection);
    
    connect(this, SIGNAL(requestCaptureFrame()),
            component, SLOT(captureFrame()),
            Qt::QueuedConnection);
}

void CameraBridge::teardownComponentConnections(QObject *component)
{
    if (!component) {
        return;
    }
    
    // Disconnect all signals
    disconnect(component, nullptr, this, nullptr);
    disconnect(this, nullptr, component, nullptr);
}

// QML-invokable methods
void CameraBridge::scanDevices()
{
    qDebug() << "CameraBridge: Scanning for devices";
    emit requestScanDevices();
}

void CameraBridge::connectCamera(const QString &deviceId)
{
    if (deviceId.isEmpty()) {
        qWarning() << "CameraBridge: Cannot connect to empty device ID";
        return;
    }
    
    m_deviceId = deviceId;
    emit deviceIdChanged();
    
    qDebug() << "CameraBridge: Connecting to camera:" << deviceId;
    emit requestConnect(deviceId);
}

void CameraBridge::disconnectCamera()
{
    qDebug() << "CameraBridge: Disconnecting camera";
    emit requestDisconnect();
}

void CameraBridge::startAcquisition()
{
    if (!m_connected) {
        qWarning() << "CameraBridge: Cannot start acquisition - not connected";
        emit errorOccurred("Camera not connected");
        return;
    }
    
    qDebug() << "CameraBridge: Starting acquisition";
    emit requestStartAcquisition();
}

void CameraBridge::stopAcquisition()
{
    qDebug() << "CameraBridge: Stopping acquisition";
    emit requestStopAcquisition();
}

void CameraBridge::captureFrame()
{
    if (!m_connected) {
        qWarning() << "CameraBridge: Cannot capture frame - not connected";
        emit errorOccurred("Camera not connected");
        return;
    }
    
    qDebug() << "CameraBridge: Capturing frame";
    emit requestCaptureFrame();
}

void CameraBridge::saveImage(const QString &filePath)
{
    QMutexLocker locker(&m_imageMutex);
    
    if (m_currentImage.isNull()) {
        qWarning() << "CameraBridge: No image to save";
        emit errorOccurred("No image available to save");
        return;
    }
    
    if (m_currentImage.save(filePath)) {
        qDebug() << "CameraBridge: Image saved to" << filePath;
    } else {
        qWarning() << "CameraBridge: Failed to save image to" << filePath;
        emit errorOccurred(QString("Failed to save image to %1").arg(filePath));
    }
}

void CameraBridge::setParameter(const QString &name, const QVariant &value)
{
    if (!m_component) {
        qWarning() << "CameraBridge: Cannot set parameter - no component connected";
        return;
    }
    
    qDebug() << "CameraBridge: Setting parameter" << name << "to" << value;
    
    QMetaObject::invokeMethod(m_component, "setParameter",
                              Qt::QueuedConnection,
                              Q_ARG(QString, name),
                              Q_ARG(QVariant, value));
}

QVariant CameraBridge::getParameter(const QString &name) const
{
    if (!m_component) {
        qWarning() << "CameraBridge: Cannot get parameter - no component connected";
        return QVariant();
    }
    
    QVariant result;
    QMetaObject::invokeMethod(m_component, "getParameter",
                              Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(QVariant, result),
                              Q_ARG(QString, name));
    
    return result;
}

void CameraBridge::resetToDefaults()
{
    if (!m_component) {
        qWarning() << "CameraBridge: Cannot reset - no component connected";
        return;
    }
    
    qDebug() << "CameraBridge: Resetting to defaults";
    
    QMetaObject::invokeMethod(m_component, "resetToDefaults",
                              Qt::QueuedConnection);
}

void CameraBridge::loadConfiguration(const QString &filePath)
{
    if (!m_component) {
        qWarning() << "CameraBridge: Cannot load configuration - no component connected";
        emit errorOccurred("No camera component connected");
        return;
    }
    
    // Read configuration file
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "CameraBridge: Failed to open configuration file:" << filePath;
        emit errorOccurred(QString("Failed to open configuration file: %1").arg(filePath));
        return;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    // Parse JSON configuration
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) {
        qWarning() << "CameraBridge: Invalid configuration file format";
        emit errorOccurred("Invalid configuration file format");
        return;
    }
    
    QJsonObject config = doc.object();
    QVariantMap configMap = config.toVariantMap();
    
    qDebug() << "CameraBridge: Loading configuration from" << filePath;
    
    QMetaObject::invokeMethod(m_component, "loadConfiguration",
                              Qt::QueuedConnection,
                              Q_ARG(QVariantMap, configMap));
    
    emit configurationLoaded();
}

void CameraBridge::saveConfiguration(const QString &filePath)
{
    if (!m_component) {
        qWarning() << "CameraBridge: Cannot save configuration - no component connected";
        emit errorOccurred("No camera component connected");
        return;
    }
    
    // Get current configuration from component
    QVariantMap config;
    QMetaObject::invokeMethod(m_component, "getConfiguration",
                              Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(QVariantMap, config));
    
    // Convert to JSON
    QJsonObject jsonConfig = QJsonObject::fromVariantMap(config);
    QJsonDocument doc(jsonConfig);
    
    // Write to file
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "CameraBridge: Failed to open file for writing:" << filePath;
        emit errorOccurred(QString("Failed to save configuration to: %1").arg(filePath));
        return;
    }
    
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    
    qDebug() << "CameraBridge: Configuration saved to" << filePath;
    emit configurationSaved();
}

void CameraBridge::setROI(const QVariantMap &roi)
{
    if (!m_component) {
        qWarning() << "CameraBridge: Cannot set ROI - no component connected";
        return;
    }
    
    // Update local ROI values
    if (roi.contains("x")) {
        m_roiX = roi["x"].toInt();
    }
    if (roi.contains("y")) {
        m_roiY = roi["y"].toInt();
    }
    if (roi.contains("width")) {
        m_roiWidth = roi["width"].toInt();
    }
    if (roi.contains("height")) {
        m_roiHeight = roi["height"].toInt();
    }
    
    m_roiEnabled = true;
    
    qDebug() << "CameraBridge: Setting ROI" << roi;
    
    QMetaObject::invokeMethod(m_component, "setROI",
                              Qt::QueuedConnection,
                              Q_ARG(QVariantMap, roi));
    
    emit roiChanged();
    emit roiEnabledChanged();
}

void CameraBridge::clearROI()
{
    if (!m_component) {
        qWarning() << "CameraBridge: Cannot clear ROI - no component connected";
        return;
    }
    
    m_roiEnabled = false;
    
    qDebug() << "CameraBridge: Clearing ROI";
    
    QMetaObject::invokeMethod(m_component, "clearROI",
                              Qt::QueuedConnection);
    
    emit roiEnabledChanged();
}

QVariantMap CameraBridge::getStatistics() const
{
    QVariantMap stats;
    stats["frameCount"] = QVariant::fromValue(m_frameCount.load());
    stats["droppedFrames"] = QVariant::fromValue(m_droppedFrames.load());
    stats["currentFps"] = m_currentFps;
    stats["connected"] = m_connected.load();
    stats["acquiring"] = m_acquiring.load();
    stats["imageWidth"] = m_width;
    stats["imageHeight"] = m_height;
    stats["exposureTime"] = m_exposureTime;
    stats["gain"] = m_gain;
    stats["frameRate"] = m_frameRate;
    
    return stats;
}

// Slots for component signals
void CameraBridge::onComponentFrameReady(const QImage &image, qint64 timestamp)
{
    // Update frame count
    m_frameCount++;
    
    // Update image with thread safety
    {
        QMutexLocker locker(&m_imageMutex);
        m_currentImage = image.copy(); // Make a copy for thread safety
        
        // Generate new unique image ID for this frame
        m_currentImageId = QString("%1_%2").arg(timestamp).arg(QUuid::createUuid().toString());
    }
    
    // Update image dimensions if changed
    if (image.width() != m_width || image.height() != m_height) {
        m_width = image.width();
        m_height = image.height();
        emit widthChanged();
        emit heightChanged();
    }
    
    // Update new frame flag
    m_hasNewFrame = true;
    emit hasNewFrameChanged();
    
    // Emit signals
    QString imageUrl = currentImageUrl();
    emit imageUrlChanged();
    emit frameReady(imageUrl, timestamp);
    emit frameCountChanged();
}

void CameraBridge::onComponentStateChanged(int state)
{
    // Map component state to bridge state
    // Assuming states: 0=Uninitialized, 1=Initialized, 2=Connected, 3=Acquiring, 4=Error
    switch (state) {
        case 0: // Uninitialized
            m_connectionStatus = "Uninitialized";
            m_connected = false;
            m_acquiring = false;
            break;
        case 1: // Initialized
            m_connectionStatus = "Initialized";
            m_connected = false;
            m_acquiring = false;
            break;
        case 2: // Connected/Ready
            m_connectionStatus = "Connected";
            m_connected = true;
            m_acquiring = false;
            break;
        case 3: // Acquiring/Running
            m_connectionStatus = "Acquiring";
            m_connected = true;
            m_acquiring = true;
            break;
        case 4: // Error
            m_connectionStatus = "Error";
            m_acquiring = false;
            break;
        default:
            m_connectionStatus = QString("Unknown (%1)").arg(state);
            break;
    }
    
    emit connectionStatusChanged();
    emit connectedChanged();
    emit acquiringChanged();
}

void CameraBridge::onComponentErrorOccurred(const QString &error)
{
    qWarning() << "CameraBridge: Component error:" << error;
    emit errorOccurred(error);
}

void CameraBridge::onComponentDeviceListUpdated(const QVariantList &devices)
{
    m_deviceList = devices;
    emit deviceListChanged();
    
    // Emit individual device discovered signals for QML
    for (const QVariant &device : devices) {
        QVariantMap deviceMap = device.toMap();
        emit deviceDiscovered(deviceMap);
    }
    
    qDebug() << "CameraBridge: Device list updated with" << devices.size() << "devices";
}

void CameraBridge::onComponentConnected()
{
    m_connected = true;
    m_connectionStatus = "Connected";
    emit connectedChanged();
    emit connectionStatusChanged();
    
    qDebug() << "CameraBridge: Camera connected";
}

void CameraBridge::onComponentDisconnected()
{
    m_connected = false;
    m_acquiring = false;
    m_connectionStatus = "Disconnected";
    
    // Reset device info
    m_deviceId.clear();
    m_deviceName.clear();
    
    emit connectedChanged();
    emit acquiringChanged();
    emit connectionStatusChanged();
    emit deviceIdChanged();
    emit deviceNameChanged();
    
    qDebug() << "CameraBridge: Camera disconnected";
}

void CameraBridge::onComponentParameterChanged(const QString &name, const QVariant &value)
{
    // Update cached parameters
    if (name == "exposureTime") {
        m_exposureTime = value.toDouble();
        emit exposureTimeChanged();
    } else if (name == "gain") {
        m_gain = value.toDouble();
        emit gainChanged();
    } else if (name == "frameRate") {
        m_frameRate = value.toDouble();
        emit frameRateChanged();
    } else if (name == "deviceName") {
        m_deviceName = value.toString();
        emit deviceNameChanged();
    }
    
    qDebug() << "CameraBridge: Parameter changed:" << name << "=" << value;
}

void CameraBridge::onComponentStatisticsUpdated(const QVariantMap &stats)
{
    // Update statistics from component
    if (stats.contains("fps")) {
        m_currentFps = stats["fps"].toDouble();
        emit fpsChanged();
    }
    
    if (stats.contains("frameCount")) {
        m_frameCount = stats["frameCount"].toLongLong();
        emit frameCountChanged();
    }
    
    if (stats.contains("droppedFrames")) {
        m_droppedFrames = stats["droppedFrames"].toLongLong();
        emit droppedFramesChanged();
    }
}

// Private helper methods
void CameraBridge::updateImageUrl(const QImage &image)
{
    QMutexLocker locker(&m_imageMutex);
    m_currentImage = image;
    
    // Generate new unique ID for this image
    m_currentImageId = QUuid::createUuid().toString();
    
    emit imageUrlChanged();
}

void CameraBridge::updateStatistics()
{
    // Request statistics update from component
    if (m_component) {
        QMetaObject::invokeMethod(m_component, "updateStatistics",
                                  Qt::QueuedConnection);
    }
}