#include "base_component.h"
#include <QUuid>
#include <QThread>
#include <QCoreApplication>
#include <QJsonDocument>
#include <QDebug>
#include <QElapsedTimer>
#include <chrono>
#include <thread>

// Platform-specific includes for performance monitoring
#ifdef Q_OS_WIN
    #include <windows.h>
    #include <psapi.h>
#elif defined(Q_OS_LINUX)
    #include <sys/types.h>
    #include <sys/stat.h>
    #include <unistd.h>
    #include <fstream>
    #include <sstream>
#endif

namespace ComponentsForest {

// Private implementation class using PIMPL idiom
class BaseComponent::Private {
public:
    Private(BaseComponent* q) : q_ptr(q) {}
    
    BaseComponent* q_ptr;
    QThread* workerThread{nullptr};
    QElapsedTimer uptimeTimer;
    
    // Performance monitoring data
    std::chrono::steady_clock::time_point lastCpuTime;
    double lastCpuUsage{0.0};
    
    // Helper to get current process memory usage
    double getMemoryUsageMB() const {
#ifdef Q_OS_WIN
        PROCESS_MEMORY_COUNTERS_EX pmc;
        if (GetProcessMemoryInfo(GetCurrentProcess(), 
                                  reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmc), 
                                  sizeof(pmc))) {
            return static_cast<double>(pmc.WorkingSetSize) / (1024.0 * 1024.0);
        }
#elif defined(Q_OS_LINUX)
        std::ifstream statusFile("/proc/self/status");
        std::string line;
        while (std::getline(statusFile, line)) {
            if (line.substr(0, 6) == "VmRSS:") {
                std::istringstream iss(line);
                std::string label;
                double value;
                std::string unit;
                iss >> label >> value >> unit;
                if (unit == "kB") {
                    return value / 1024.0;
                }
            }
        }
#endif
        return 0.0;
    }
    
    // Helper to get current process CPU usage
    double getCpuUsagePercent() {
#ifdef Q_OS_WIN
        static ULARGE_INTEGER lastCPU, lastSysCPU, lastUserCPU;
        static int firstRun = 1;
        static HANDLE self = GetCurrentProcess();
        
        FILETIME ftime, fsys, fuser;
        ULARGE_INTEGER now, sys, user;
        
        GetSystemTimeAsFileTime(&ftime);
        memcpy(&now, &ftime, sizeof(FILETIME));
        
        GetProcessTimes(self, &ftime, &ftime, &fsys, &fuser);
        memcpy(&sys, &fsys, sizeof(FILETIME));
        memcpy(&user, &fuser, sizeof(FILETIME));
        
        double percent = 0.0;
        if (!firstRun) {
            percent = static_cast<double>((sys.QuadPart - lastSysCPU.QuadPart) +
                     (user.QuadPart - lastUserCPU.QuadPart));
            percent /= (now.QuadPart - lastCPU.QuadPart);
            percent /= static_cast<double>(std::thread::hardware_concurrency());
            percent *= 100.0;
        }
        
        lastCPU = now;
        lastUserCPU = user;
        lastSysCPU = sys;
        firstRun = 0;
        
        return percent;
#elif defined(Q_OS_LINUX)
        static std::chrono::steady_clock::time_point lastTime;
        static clock_t lastClock = 0;
        
        auto now = std::chrono::steady_clock::now();
        clock_t currentClock = clock();
        
        if (lastClock == 0) {
            lastTime = now;
            lastClock = currentClock;
            return 0.0;
        }
        
        auto timeDiff = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastTime).count();
        if (timeDiff == 0) return lastCpuUsage;
        
        double cpuTime = static_cast<double>(currentClock - lastClock) / CLOCKS_PER_SEC * 1000.0;
        double usage = (cpuTime / timeDiff) * 100.0 / static_cast<double>(std::thread::hardware_concurrency());
        
        lastTime = now;
        lastClock = currentClock;
        lastCpuUsage = usage;
        
        return usage;
#else
        return 0.0;
#endif
    }
};

// Constructor
BaseComponent::BaseComponent(QObject* parent)
    : QObject(parent)
    , d_ptr(std::make_unique<Private>(this))
{
    generateComponentId();
    m_componentName = "BaseComponent";
    setupTimers();
    
    // Register meta types for signal/slot connections
    qRegisterMetaType<ComponentState>("ComponentState");
    qRegisterMetaType<HealthStatus>("HealthStatus");
    qRegisterMetaType<ComponentEvent>("ComponentEvent");
}

// Destructor
BaseComponent::~BaseComponent()
{
    // Ensure proper cleanup following RAII principles
    if (m_state == ComponentState::Running || m_state == ComponentState::Starting) {
        stop();
    }
    
    if (m_state != ComponentState::Uninitialized && m_state != ComponentState::Destroyed) {
        destroy();
    }
    
    cleanupTimers();
}

// Generate unique component ID
void BaseComponent::generateComponentId()
{
    m_componentId = QUuid::createUuid().toString(QUuid::WithoutBraces);
}

// Convert state to string for logging
QString BaseComponent::stateToString(ComponentState state) const
{
    switch (state) {
        case ComponentState::Uninitialized: return "Uninitialized";
        case ComponentState::Initialized: return "Initialized";
        case ComponentState::Starting: return "Starting";
        case ComponentState::Running: return "Running";
        case ComponentState::Stopping: return "Stopping";
        case ComponentState::Stopped: return "Stopped";
        case ComponentState::Error: return "Error";
        case ComponentState::Recovering: return "Recovering";
        case ComponentState::Destroyed: return "Destroyed";
        default: return "Unknown";
    }
}

// Setup internal timers
void BaseComponent::setupTimers()
{
    m_healthCheckTimer = new QTimer(this);
    m_healthCheckTimer->setInterval(5000); // 5 seconds default
    connect(m_healthCheckTimer, &QTimer::timeout, this, &BaseComponent::onHealthCheck);
    
    m_performanceTimer = new QTimer(this);
    m_performanceTimer->setInterval(1000); // 1 second default
    connect(m_performanceTimer, &QTimer::timeout, this, &BaseComponent::onPerformanceUpdate);
}

// Cleanup timers
void BaseComponent::cleanupTimers()
{
    if (m_healthCheckTimer) {
        m_healthCheckTimer->stop();
        m_healthCheckTimer->deleteLater();
        m_healthCheckTimer = nullptr;
    }
    
    if (m_performanceTimer) {
        m_performanceTimer->stop();
        m_performanceTimer->deleteLater();
        m_performanceTimer = nullptr;
    }
}

// Basic Properties Implementation
QString BaseComponent::componentId() const
{
    return m_componentId;
}

QString BaseComponent::componentName() const
{
    QMutexLocker locker(&m_configMutex);
    return m_componentName;
}

ComponentState BaseComponent::state() const
{
    QMutexLocker locker(&m_stateMutex);
    return m_state;
}

// Lifecycle Management Implementation
bool BaseComponent::initialize(const QJsonObject& config)
{
    QMutexLocker stateLocker(&m_stateMutex);
    
    if (m_state != ComponentState::Uninitialized && m_state != ComponentState::Stopped) {
        logWarning(QString("Cannot initialize component in state: %1").arg(stateToString(m_state)));
        return false;
    }
    
    ComponentState oldState = m_state;
    m_state = ComponentState::Initialized;
    stateLocker.unlock();
    
    // Load configuration
    if (!loadConfiguration(config)) {
        logError("Failed to load configuration during initialization");
        transitionTo(ComponentState::Error);
        return false;
    }
    
    // Call virtual initialization method
    bool success = onInitialize();
    
    if (success) {
        m_initTime = QDateTime::currentMSecsSinceEpoch();
        d_ptr->uptimeTimer.start();
        
        logInfo("Component initialized successfully");
        emit stateChanged(m_state, oldState);
        emit initialized();
        emit componentReady();
    } else {
        logError("Component initialization failed");
        transitionTo(ComponentState::Error);
    }
    
    return success;
}

bool BaseComponent::start()
{
    if (!canTransitionTo(ComponentState::Running)) {
        logWarning(QString("Cannot start component from state: %1").arg(stateToString(m_state)));
        return false;
    }
    
    transitionTo(ComponentState::Starting);
    
    // Call virtual start method
    bool success = onStart();
    
    if (success) {
        transitionTo(ComponentState::Running);
        m_startTime = QDateTime::currentMSecsSinceEpoch();
        
        // Start monitoring timers
        if (m_healthCheckTimer && !m_healthCheckTimer->isActive()) {
            m_healthCheckTimer->start();
        }
        if (m_performanceTimer && !m_performanceTimer->isActive()) {
            m_performanceTimer->start();
        }
        
        logInfo("Component started successfully");
        emit started();
    } else {
        logError("Component start failed");
        transitionTo(ComponentState::Error);
    }
    
    return success;
}

bool BaseComponent::stop()
{
    if (!canTransitionTo(ComponentState::Stopped)) {
        logWarning(QString("Cannot stop component from state: %1").arg(stateToString(m_state)));
        return false;
    }
    
    transitionTo(ComponentState::Stopping);
    
    // Stop monitoring timers
    if (m_healthCheckTimer && m_healthCheckTimer->isActive()) {
        m_healthCheckTimer->stop();
    }
    if (m_performanceTimer && m_performanceTimer->isActive()) {
        m_performanceTimer->stop();
    }
    
    // Call virtual stop method
    bool success = onStop();
    
    if (success) {
        transitionTo(ComponentState::Stopped);
        logInfo("Component stopped successfully");
        emit stopped();
        emit componentShutdown();
    } else {
        logError("Component stop failed");
        transitionTo(ComponentState::Error);
    }
    
    return success;
}

bool BaseComponent::reset()
{
    logInfo("Resetting component");
    
    // Stop if running
    if (m_state == ComponentState::Running || m_state == ComponentState::Starting) {
        if (!stop()) {
            logError("Failed to stop component during reset");
            return false;
        }
    }
    
    // Clear error state
    clearError();
    
    // Call virtual reset method
    bool success = onReset();
    
    if (success) {
        // Reset performance metrics
        {
            QMutexLocker locker(&m_performanceMutex);
            m_performance = PerformanceData();
        }
        
        logInfo("Component reset successfully");
        
        // Re-initialize if we have configuration
        if (!m_configuration.isEmpty()) {
            return initialize(m_configuration);
        }
    } else {
        logError("Component reset failed");
        transitionTo(ComponentState::Error);
    }
    
    return success;
}

void BaseComponent::destroy()
{
    logInfo("Destroying component");
    
    // Stop if still running
    if (m_state == ComponentState::Running || m_state == ComponentState::Starting) {
        stop();
    }
    
    // Cleanup timers
    cleanupTimers();
    
    // Clear dependencies
    {
        QMutexLocker locker(&m_dependencyMutex);
        m_dependencies.clear();
    }
    
    // Call virtual destroy method
    onDestroy();
    
    // Final state transition
    transitionTo(ComponentState::Destroyed);
    
    // Move back to main thread if needed
    if (d_ptr->workerThread) {
        moveToThread(QCoreApplication::instance()->thread());
        d_ptr->workerThread->quit();
        d_ptr->workerThread->wait(5000); // Wait up to 5 seconds
        d_ptr->workerThread->deleteLater();
        d_ptr->workerThread = nullptr;
    }
}

// Health & Status Implementation
bool BaseComponent::isHealthy() const
{
    return m_isHealthy.load(std::memory_order_acquire);
}

HealthStatus BaseComponent::getHealthStatus() const
{
    QMutexLocker healthLocker(&m_healthMutex);
    QMutexLocker perfLocker(&m_performanceMutex);
    
    HealthStatus status;
    status.isHealthy = m_isHealthy.load(std::memory_order_acquire);
    status.status = m_lastError.isEmpty() ? "OK" : m_lastError;
    status.timestamp = QDateTime::currentMSecsSinceEpoch();
    status.uptime = getUptime();
    status.cpuUsage = m_performance.cpuUsage;
    status.memoryUsage = m_performance.memoryUsage;
    
    // Add custom metrics
    status.metrics = m_performance.customMetrics;
    status.metrics["processedItems"] = static_cast<qint64>(m_performance.processedItems);
    status.metrics["errorCount"] = static_cast<qint64>(m_performance.errorCount);
    
    return status;
}

QJsonObject BaseComponent::getStatus() const
{
    QJsonObject status;
    
    status["componentId"] = m_componentId;
    status["componentName"] = componentName();
    status["componentType"] = componentType();
    status["componentVersion"] = componentVersion();
    status["state"] = stateToString(m_state);
    status["isHealthy"] = isHealthy();
    status["uptime"] = getUptime();
    
    // Add health status
    HealthStatus health = getHealthStatus();
    QJsonObject healthObj;
    healthObj["isHealthy"] = health.isHealthy;
    healthObj["status"] = health.status;
    healthObj["cpuUsage"] = health.cpuUsage;
    healthObj["memoryUsage"] = health.memoryUsage;
    healthObj["metrics"] = health.metrics;
    status["health"] = healthObj;
    
    // Add configuration info
    status["hasConfiguration"] = !m_configuration.isEmpty();
    
    // Add dependency info
    {
        QMutexLocker locker(&m_dependencyMutex);
        QJsonArray deps;
        for (auto it = m_dependencies.begin(); it != m_dependencies.end(); ++it) {
            deps.append(it.key());
        }
        status["dependencies"] = deps;
    }
    
    return status;
}

QJsonObject BaseComponent::getCapabilities() const
{
    QJsonObject capabilities;
    
    capabilities["supportsConfiguration"] = true;
    capabilities["supportsHealthCheck"] = true;
    capabilities["supportsPerformanceMonitoring"] = true;
    capabilities["supportsEvents"] = true;
    capabilities["supportsDependencyInjection"] = true;
    capabilities["supportsThreading"] = true;
    capabilities["supportsReset"] = true;
    
    // Add component-specific capabilities
    capabilities["componentType"] = componentType();
    capabilities["componentVersion"] = componentVersion();
    
    return capabilities;
}

// Configuration Implementation
bool BaseComponent::loadConfiguration(const QJsonObject& config)
{
    if (!validateConfiguration(config)) {
        logError("Configuration validation failed");
        return false;
    }
    
    QMutexLocker locker(&m_configMutex);
    m_configuration = config;
    locker.unlock();
    
    // Extract common configuration values
    if (config.contains("name")) {
        setComponentName(config["name"].toString());
    }
    
    if (config.contains("healthCheckInterval")) {
        int interval = config["healthCheckInterval"].toInt(5000);
        if (m_healthCheckTimer) {
            m_healthCheckTimer->setInterval(interval);
        }
    }
    
    if (config.contains("performanceMonitorInterval")) {
        int interval = config["performanceMonitorInterval"].toInt(1000);
        if (m_performanceTimer) {
            m_performanceTimer->setInterval(interval);
        }
    }
    
    // Notify about configuration change
    onConfigurationChanged(config);
    emit configurationChanged(config);
    emit configurationLoaded();
    
    logInfo("Configuration loaded successfully");
    return true;
}

bool BaseComponent::saveConfiguration(QJsonObject& config) const
{
    QMutexLocker locker(&m_configMutex);
    config = m_configuration;
    
    // Add runtime information
    config["componentId"] = m_componentId;
    config["componentName"] = m_componentName;
    config["componentType"] = componentType();
    config["componentVersion"] = componentVersion();
    config["savedAt"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    
    emit const_cast<BaseComponent*>(this)->configurationSaved();
    
    return true;
}

bool BaseComponent::validateConfiguration(const QJsonObject& config) const
{
    // Basic validation - can be overridden by subclasses
    if (config.isEmpty()) {
        return true; // Empty config is valid for base component
    }
    
    // Check against schema if provided
    QJsonObject schema = getConfigurationSchema();
    if (!schema.isEmpty()) {
        // Basic schema validation (can be enhanced with a proper JSON schema validator)
        QJsonObject required = schema["required"].toObject();
        for (auto it = required.begin(); it != required.end(); ++it) {
            if (!config.contains(it.key())) {
                logError(QString("Missing required configuration field: %1").arg(it.key()));
                return false;
            }
        }
    }
    
    return true;
}

QJsonObject BaseComponent::getConfigurationSchema() const
{
    QJsonObject schema;
    
    // Basic schema for base component
    QJsonObject properties;
    properties["name"] = QJsonObject{{"type", "string"}, {"description", "Component name"}};
    properties["healthCheckInterval"] = QJsonObject{{"type", "integer"}, {"description", "Health check interval in ms"}};
    properties["performanceMonitorInterval"] = QJsonObject{{"type", "integer"}, {"description", "Performance monitoring interval in ms"}};
    
    schema["properties"] = properties;
    schema["required"] = QJsonObject{};
    
    return schema;
}

// Event Handling Implementation
void BaseComponent::handleEvent(const ComponentEvent& event)
{
    logDebug(QString("Received event: %1 from %2").arg(event.type).arg(event.source));
    
    emit eventReceived(event);
    
    // Call virtual event handler
    onEventReceived(event);
    
    // Handle system events
    if (event.type == "shutdown") {
        stop();
    } else if (event.type == "reset") {
        reset();
    } else if (event.type == "healthCheck") {
        onHealthCheck();
    }
}

// Dependency Management Implementation
QStringList BaseComponent::getDependencies() const
{
    QMutexLocker locker(&m_dependencyMutex);
    return m_dependencies.keys();
}

void BaseComponent::injectDependency(const QString& name, IComponent* component)
{
    if (!component) {
        logWarning(QString("Attempted to inject null dependency: %1").arg(name));
        return;
    }
    
    {
        QMutexLocker locker(&m_dependencyMutex);
        m_dependencies[name] = component;
    }
    
    logInfo(QString("Dependency injected: %1").arg(name));
    
    // Call virtual handler
    onDependencyInjected(name, component);
    
    emit dependencyInjected(name);
}

// Component Management Implementation
void BaseComponent::setComponentName(const QString& name)
{
    if (name.isEmpty()) {
        return;
    }
    
    QMutexLocker locker(&m_configMutex);
    QString oldName = m_componentName;
    m_componentName = name;
    locker.unlock();
    
    if (oldName != name) {
        logInfo(QString("Component name changed from %1 to %2").arg(oldName).arg(name));
        emit nameChanged(name);
    }
}

bool BaseComponent::isRunning() const
{
    QMutexLocker locker(&m_stateMutex);
    return m_state == ComponentState::Running;
}

bool BaseComponent::isInitialized() const
{
    QMutexLocker locker(&m_stateMutex);
    return m_state != ComponentState::Uninitialized && m_state != ComponentState::Destroyed;
}

qint64 BaseComponent::getUptime() const
{
    if (d_ptr->uptimeTimer.isValid()) {
        return d_ptr->uptimeTimer.elapsed();
    }
    return 0;
}

QString BaseComponent::getLastError() const
{
    QMutexLocker locker(&m_healthMutex);
    return m_lastError;
}

void BaseComponent::clearError()
{
    QMutexLocker locker(&m_healthMutex);
    m_lastError.clear();
    m_isHealthy.store(true, std::memory_order_release);
    locker.unlock();
    
    if (m_state == ComponentState::Error) {
        transitionTo(ComponentState::Stopped);
    }
}

// Performance Monitoring Implementation
double BaseComponent::getCpuUsage() const
{
    QMutexLocker locker(&m_performanceMutex);
    return m_performance.cpuUsage;
}

double BaseComponent::getMemoryUsage() const
{
    QMutexLocker locker(&m_performanceMutex);
    return m_performance.memoryUsage;
}

QJsonObject BaseComponent::getPerformanceMetrics() const
{
    QMutexLocker locker(&m_performanceMutex);
    
    QJsonObject metrics;
    metrics["cpuUsage"] = m_performance.cpuUsage;
    metrics["memoryUsage"] = m_performance.memoryUsage;
    metrics["processedItems"] = static_cast<qint64>(m_performance.processedItems);
    metrics["errorCount"] = static_cast<qint64>(m_performance.errorCount);
    metrics["uptime"] = getUptime();
    
    // Add custom metrics
    for (auto it = m_performance.customMetrics.begin(); it != m_performance.customMetrics.end(); ++it) {
        metrics[it.key()] = it.value();
    }
    
    return metrics;
}

// Thread Management Implementation
void BaseComponent::moveToNewThread()
{
    if (d_ptr->workerThread) {
        logWarning("Component already has a worker thread");
        return;
    }
    
    d_ptr->workerThread = new QThread();
    d_ptr->workerThread->setObjectName(QString("%1_Thread").arg(m_componentName));
    
    moveToThread(d_ptr->workerThread);
    d_ptr->workerThread->start();
    
    logInfo("Component moved to new thread");
}

QThread* BaseComponent::componentThread() const
{
    return thread();
}

// State Management Protected Methods
bool BaseComponent::transitionTo(ComponentState newState)
{
    if (!canTransitionTo(newState)) {
        logWarning(QString("Invalid state transition from %1 to %2")
                  .arg(stateToString(m_state))
                  .arg(stateToString(newState)));
        return false;
    }
    
    QMutexLocker locker(&m_stateMutex);
    ComponentState oldState = m_state;
    m_previousState = oldState;
    m_state = newState;
    locker.unlock();
    
    logInfo(QString("State transition: %1 -> %2")
           .arg(stateToString(oldState))
           .arg(stateToString(newState)));
    
    // Call virtual handler
    onStateChanged(newState, oldState);
    
    // Emit signals
    emit stateChanged(newState, oldState);
    
    // Handle special state transitions
    if (newState == ComponentState::Error) {
        emit errorOccurred(m_lastError);
    } else if (newState == ComponentState::Running && oldState == ComponentState::Error) {
        emit recovered();
    }
    
    return true;
}

bool BaseComponent::canTransitionTo(ComponentState newState) const
{
    QMutexLocker locker(&m_stateMutex);
    
    // Define valid state transitions
    switch (m_state) {
        case ComponentState::Uninitialized:
            return newState == ComponentState::Initialized || 
                   newState == ComponentState::Destroyed;
            
        case ComponentState::Initialized:
            return newState == ComponentState::Starting || 
                   newState == ComponentState::Destroyed ||
                   newState == ComponentState::Error;
            
        case ComponentState::Starting:
            return newState == ComponentState::Running || 
                   newState == ComponentState::Error ||
                   newState == ComponentState::Stopping;
            
        case ComponentState::Running:
            return newState == ComponentState::Stopping || 
                   newState == ComponentState::Error ||
                   newState == ComponentState::Recovering;
            
        case ComponentState::Stopping:
            return newState == ComponentState::Stopped || 
                   newState == ComponentState::Error;
            
        case ComponentState::Stopped:
            return newState == ComponentState::Initialized || 
                   newState == ComponentState::Starting ||
                   newState == ComponentState::Destroyed;
            
        case ComponentState::Error:
            return newState == ComponentState::Recovering || 
                   newState == ComponentState::Stopped ||
                   newState == ComponentState::Destroyed;
            
        case ComponentState::Recovering:
            return newState == ComponentState::Running || 
                   newState == ComponentState::Error ||
                   newState == ComponentState::Stopped;
            
        case ComponentState::Destroyed:
            return false; // No transitions from destroyed state
            
        default:
            return false;
    }
}

void BaseComponent::setError(const QString& error)
{
    {
        QMutexLocker locker(&m_healthMutex);
        m_lastError = error;
        m_isHealthy.store(false, std::memory_order_release);
    }
    
    {
        QMutexLocker locker(&m_performanceMutex);
        m_performance.errorCount++;
    }
    
    logError(error);
    transitionTo(ComponentState::Error);
}

// Logging Helper Methods
void BaseComponent::logInfo(const QString& message) const
{
    QString fullMessage = QString("[%1] INFO: %2").arg(m_componentName).arg(message);
    qInfo() << fullMessage;
    const_cast<BaseComponent*>(this)->emit logMessage(message, "INFO");
}

void BaseComponent::logWarning(const QString& message) const
{
    QString fullMessage = QString("[%1] WARNING: %2").arg(m_componentName).arg(message);
    qWarning() << fullMessage;
    const_cast<BaseComponent*>(this)->emit logMessage(message, "WARNING");
}

void BaseComponent::logError(const QString& message) const
{
    QString fullMessage = QString("[%1] ERROR: %2").arg(m_componentName).arg(message);
    qCritical() << fullMessage;
    const_cast<BaseComponent*>(this)->emit logMessage(message, "ERROR");
}

void BaseComponent::logDebug(const QString& message) const
{
    QString fullMessage = QString("[%1] DEBUG: %2").arg(m_componentName).arg(message);
    qDebug() << fullMessage;
    const_cast<BaseComponent*>(this)->emit debugMessage(message);
    const_cast<BaseComponent*>(this)->emit logMessage(message, "DEBUG");
}

// Event Helper Methods
void BaseComponent::sendEvent(const QString& type, const QJsonObject& data, const QString& target)
{
    ComponentEvent event;
    event.type = type;
    event.source = m_componentId;
    event.target = target;
    event.data = data;
    event.timestamp = QDateTime::currentMSecsSinceEpoch();
    
    logDebug(QString("Sending event: %1 to %2").arg(type).arg(target.isEmpty() ? "all" : target));
    
    emit eventSent(event);
}

void BaseComponent::broadcastEvent(const QString& type, const QJsonObject& data)
{
    sendEvent(type, data, QString());
}

// Configuration Helper Methods
QVariant BaseComponent::getConfigValue(const QString& key, const QVariant& defaultValue) const
{
    QMutexLocker locker(&m_configMutex);
    
    if (m_configuration.contains(key)) {
        return m_configuration[key].toVariant();
    }
    
    return defaultValue;
}

void BaseComponent::setConfigValue(const QString& key, const QVariant& value)
{
    QMutexLocker locker(&m_configMutex);
    m_configuration[key] = QJsonValue::fromVariant(value);
    locker.unlock();
    
    emit configurationChanged(m_configuration);
}

// Performance Monitoring Helper Methods
void BaseComponent::updatePerformanceMetrics()
{
    QMutexLocker locker(&m_performanceMutex);
    
    m_performance.cpuUsage = d_ptr->getCpuUsagePercent();
    m_performance.memoryUsage = d_ptr->getMemoryUsageMB();
    
    QJsonObject metrics = getPerformanceMetrics();
    locker.unlock();
    
    emit performanceMetricsUpdated(metrics);
}

void BaseComponent::startPerformanceMonitoring(int intervalMs)
{
    if (m_performanceTimer) {
        m_performanceTimer->setInterval(intervalMs);
        if (!m_performanceTimer->isActive() && isRunning()) {
            m_performanceTimer->start();
        }
    }
}

void BaseComponent::stopPerformanceMonitoring()
{
    if (m_performanceTimer && m_performanceTimer->isActive()) {
        m_performanceTimer->stop();
    }
}

// Protected Slots Implementation
void BaseComponent::onHealthCheck()
{
    // Basic health check - can be overridden
    bool healthy = true;
    QString status = "OK";
    
    // Check state
    if (m_state == ComponentState::Error) {
        healthy = false;
        status = "Component in error state";
    } else if (m_state != ComponentState::Running && m_state != ComponentState::Initialized) {
        healthy = false;
        status = QString("Component not operational (state: %1)").arg(stateToString(m_state));
    }
    
    // Check dependencies
    {
        QMutexLocker locker(&m_dependencyMutex);
        for (auto it = m_dependencies.begin(); it != m_dependencies.end(); ++it) {
            if (it.value() && !it.value()->isHealthy()) {
                healthy = false;
                status = QString("Dependency %1 is unhealthy").arg(it.key());
                break;
            }
        }
    }
    
    // Update health status
    {
        QMutexLocker locker(&m_healthMutex);
        m_isHealthy.store(healthy, std::memory_order_release);
        if (!healthy && m_lastError.isEmpty()) {
            m_lastError = status;
        }
    }
    
    // Emit health status
    HealthStatus healthStatus = getHealthStatus();
    emit healthStatusChanged(healthStatus);
    
    // Log if health changed
    static bool lastHealthy = true;
    if (healthy != lastHealthy) {
        if (healthy) {
            logInfo("Component health restored");
        } else {
            logWarning(QString("Component health degraded: %1").arg(status));
        }
        lastHealthy = healthy;
    }
}

void BaseComponent::onPerformanceUpdate()
{
    updatePerformanceMetrics();
}

} // namespace ComponentsForest