#ifndef BASE_COMPONENT_H
#define BASE_COMPONENT_H

#ifdef COMPONENTSFORESTCORE_EXPORTS
#include "componentsforestcore_export.h"
#define COMPONENTSFORESTCORE_API COMPONENTSFORESTCORE_EXPORT
#else
#define COMPONENTSFORESTCORE_API
#endif
#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <QVariantMap>
#include <QMutex>
#include <QMutexLocker>
#include <QDateTime>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QThread>
#include <QMap>
#include <memory>
#include <atomic>
#include <functional>

namespace ComponentsForest {
Q_NAMESPACE

/**
 * @brief Component State Enumeration
 * Defines all possible states for a component lifecycle
 */
enum class ComponentState {
    Uninitialized,  // Component created but not initialized
    Initialized,    // Component initialized with configuration
    Starting,       // Component is starting up
    Running,        // Component is running normally
    Stopping,       // Component is shutting down
    Stopped,        // Component has stopped
    Error,          // Component encountered an error
    Recovering,     // Component is recovering from error
    Destroyed       // Component is destroyed
};
Q_ENUM_NS(ComponentState)

/**
 * @brief Component Type Enumeration
 * Defines standard component types in the ecosystem
 */
enum class ComponentType {
    Camera,         // Camera/Image acquisition component
    ImageProcessing,// Image processing component
    Algorithm,      // Algorithm/AI component
    MotionControl,  // Motion control component
    DataStorage,    // Data storage component
    Communication,  // Communication component
    Custom          // Custom/User-defined component
};
Q_ENUM_NS(ComponentType)

/**
 * @brief Health Status Structure
 * Contains component health information
 */
struct HealthStatus {
    bool isHealthy{true};
    QString status{"OK"};
    QJsonObject metrics;
    qint64 timestamp{0};
    qint64 uptime{0};
    double cpuUsage{0.0};
    double memoryUsage{0.0};
};

/**
 * @brief Component Event Structure
 * Represents events that can be sent between components
 */
struct ComponentEvent {
    QString type;
    QString source;
    QString target;
    QJsonObject data;
    qint64 timestamp;
};

/**
 * @brief Base Component Interface
 * Abstract interface that all components must implement
 */
class IComponent {
public:
    virtual ~IComponent() = default;
    
    // Basic Properties
    virtual QString componentId() const = 0;
    virtual QString componentName() const = 0;
    virtual QString componentType() const = 0;
    virtual QString componentVersion() const = 0;
    virtual ComponentState state() const = 0;
    
    // Lifecycle Management
    [[nodiscard]] virtual bool initialize(const QJsonObject& config) = 0;
    [[nodiscard]] virtual bool start() = 0;
    [[nodiscard]] virtual bool stop() = 0;
    [[nodiscard]] virtual bool reset() = 0;
    virtual void destroy() = 0;
    
    // Health & Status
    [[nodiscard]] virtual bool isHealthy() const = 0;
    virtual HealthStatus getHealthStatus() const = 0;
    virtual QJsonObject getStatus() const = 0;
    virtual QJsonObject getCapabilities() const = 0;
    
    // Configuration
    [[nodiscard]] virtual bool loadConfiguration(const QJsonObject& config) = 0;
    [[nodiscard]] virtual bool saveConfiguration(QJsonObject& config) const = 0;
    [[nodiscard]] virtual bool validateConfiguration(const QJsonObject& config) const = 0;
    virtual QJsonObject getConfigurationSchema() const = 0;
    
    // Event Handling
    virtual void handleEvent(const ComponentEvent& event) = 0;
    
    // Dependency Management
    virtual QStringList getDependencies() const = 0;
    virtual void injectDependency(const QString& name, IComponent* component) = 0;
};

/**
 * @brief Base Component Class
 * Provides standard implementation for all components in ComponentsForest
 * Inherits from QObject for Signal/Slot mechanism
 */
class COMPONENTSFORESTCORE_API BaseComponent : public QObject, public IComponent {
    Q_OBJECT
    Q_PROPERTY(QString componentId READ componentId CONSTANT)
    Q_PROPERTY(QString componentName READ componentName NOTIFY nameChanged)
    Q_PROPERTY(ComponentState state READ state NOTIFY stateChanged)
    Q_PROPERTY(bool isHealthy READ isHealthy NOTIFY healthStatusChanged)

public:
    explicit BaseComponent(QObject* parent = nullptr);
    virtual ~BaseComponent() override;
    
    // IComponent Interface Implementation
    QString componentId() const override;
    QString componentName() const override;
    QString componentType() const override { return "Base"; }
    QString componentVersion() const override { return "1.0.0"; }
    ComponentState state() const override;
    
    // Lifecycle Management
    [[nodiscard]] bool initialize(const QJsonObject& config) override;
    [[nodiscard]] bool start() override;
    [[nodiscard]] bool stop() override;
    [[nodiscard]] bool reset() override;
    void destroy() override;
    
    // Health & Status
    [[nodiscard]] bool isHealthy() const override;
    HealthStatus getHealthStatus() const override;
    QJsonObject getStatus() const override;
    QJsonObject getCapabilities() const override;
    
    // Configuration
    [[nodiscard]] bool loadConfiguration(const QJsonObject& config) override;
    [[nodiscard]] bool saveConfiguration(QJsonObject& config) const override;
    [[nodiscard]] bool validateConfiguration(const QJsonObject& config) const override;
    QJsonObject getConfigurationSchema() const override;
    
    // Event Handling
    void handleEvent(const ComponentEvent& event) override;
    
    // Dependency Management
    QStringList getDependencies() const override;
    void injectDependency(const QString& name, IComponent* component) override;
    
    // Component Management
    Q_INVOKABLE void setComponentName(const QString& name);
    [[nodiscard]] Q_INVOKABLE bool isRunning() const;
    [[nodiscard]] Q_INVOKABLE bool isInitialized() const;
    Q_INVOKABLE qint64 getUptime() const;
    Q_INVOKABLE QString getLastError() const;
    Q_INVOKABLE void clearError();
    
    // Performance Monitoring
    Q_INVOKABLE double getCpuUsage() const;
    Q_INVOKABLE double getMemoryUsage() const;
    Q_INVOKABLE QJsonObject getPerformanceMetrics() const;
    
    // Thread Management
    Q_INVOKABLE void moveToNewThread();
    Q_INVOKABLE QThread* componentThread() const;

signals:
    // State Signals
    void stateChanged(ComponentState newState, ComponentState oldState);
    void initialized();
    void started();
    void stopped();
    void errorOccurred(const QString& error);
    void recovered();
    
    // Health & Status Signals
    void healthStatusChanged(const HealthStatus& status);
    void statusUpdated(const QJsonObject& status);
    void performanceMetricsUpdated(const QJsonObject& metrics);
    
    // Configuration Signals
    void configurationChanged(const QJsonObject& newConfig);
    void configurationLoaded();
    void configurationSaved();
    
    // Event Signals
    void eventReceived(const ComponentEvent& event);
    void eventSent(const ComponentEvent& event);
    
    // Component Signals
    void nameChanged(const QString& name);
    void dependencyInjected(const QString& name);
    void componentReady();
    void componentShutdown();
    
    // Message Signals (for logging/debugging)
    void logMessage(const QString& message, const QString& level);
    void debugMessage(const QString& message);

protected:
    // Virtual methods for subclasses to override
    virtual bool onInitialize() { return true; }
    virtual bool onStart() { return true; }
    virtual bool onStop() { return true; }
    virtual bool onReset() { return true; }
    virtual void onDestroy() {}
    virtual void onStateChanged(ComponentState newState, ComponentState oldState) { Q_UNUSED(newState); Q_UNUSED(oldState); }
    virtual void onConfigurationChanged(const QJsonObject& config) { Q_UNUSED(config); }
    virtual void onEventReceived(const ComponentEvent& event) { Q_UNUSED(event); }
    virtual void onDependencyInjected(const QString& name, IComponent* component) { Q_UNUSED(name); Q_UNUSED(component); }
    
    // State Management
    [[nodiscard]] bool transitionTo(ComponentState newState);
    [[nodiscard]] bool canTransitionTo(ComponentState newState) const;
    void setError(const QString& error);
    
    // Logging Helpers
    void logInfo(const QString& message) const;
    void logWarning(const QString& message) const;
    void logError(const QString& message) const;
    void logDebug(const QString& message) const;
    
    // Event Helpers
    void sendEvent(const QString& type, const QJsonObject& data, const QString& target = QString());
    void broadcastEvent(const QString& type, const QJsonObject& data);
    
    // Configuration Helpers
    QVariant getConfigValue(const QString& key, const QVariant& defaultValue = QVariant()) const;
    void setConfigValue(const QString& key, const QVariant& value);
    
    // Performance Monitoring Helpers
    void updatePerformanceMetrics();
    void startPerformanceMonitoring(int intervalMs = 1000);
    void stopPerformanceMonitoring();

protected slots:
    // Internal slots
    virtual void onHealthCheck();
    virtual void onPerformanceUpdate();

private:
    // Private Implementation
    class Private;
    std::unique_ptr<Private> d_ptr;
    
    // State Management
    mutable QMutex m_stateMutex;
    ComponentState m_state{ComponentState::Uninitialized};
    ComponentState m_previousState{ComponentState::Uninitialized};
    
    // Component Properties
    QString m_componentId;
    QString m_componentName;
    qint64 m_startTime{0};
    qint64 m_initTime{0};
    
    // Configuration
    QJsonObject m_configuration;
    mutable QMutex m_configMutex;
    
    // Dependencies
    QMap<QString, IComponent*> m_dependencies;
    mutable QMutex m_dependencyMutex;
    
    // Health & Status
    mutable std::atomic<bool> m_isHealthy{true};
    QString m_lastError;
    mutable QMutex m_healthMutex;
    
    // Performance Metrics
    struct PerformanceData {
        double cpuUsage{0.0};
        double memoryUsage{0.0};
        qint64 processedItems{0};
        qint64 errorCount{0};
        QJsonObject customMetrics;
    } m_performance;
    mutable QMutex m_performanceMutex;
    
    // Timers
    QTimer* m_healthCheckTimer{nullptr};
    QTimer* m_performanceTimer{nullptr};
    
    // Helper Methods
    void generateComponentId();
    QString stateToString(ComponentState state) const;
    void setupTimers();
    void cleanupTimers();
    
    Q_DISABLE_COPY(BaseComponent)
};

/**
 * @brief Component Factory Interface
 * Factory pattern for creating components
 */
class IComponentFactory {
public:
    virtual ~IComponentFactory() = default;
    virtual BaseComponent* createComponent(const QString& type, const QJsonObject& config) = 0;
    virtual QStringList supportedTypes() const = 0;
    [[nodiscard]] virtual bool registerType(const QString& type, std::function<BaseComponent*()> creator) = 0;
};

/**
 * @brief Component Manager Interface
 * Manages lifecycle of multiple components
 */
class IComponentManager {
public:
    virtual ~IComponentManager() = default;
    [[nodiscard]] virtual bool registerComponent(BaseComponent* component) = 0;
    [[nodiscard]] virtual bool unregisterComponent(const QString& componentId) = 0;
    virtual BaseComponent* getComponent(const QString& componentId) const = 0;
    virtual QList<BaseComponent*> getAllComponents() const = 0;
    [[nodiscard]] virtual bool startAll() = 0;
    [[nodiscard]] virtual bool stopAll() = 0;
    virtual QJsonObject getSystemStatus() const = 0;
};

} // namespace ComponentsForest

// Register types with Qt's meta system
Q_DECLARE_METATYPE(ComponentsForest::ComponentState)
Q_DECLARE_METATYPE(ComponentsForest::ComponentType)
Q_DECLARE_METATYPE(ComponentsForest::HealthStatus)
Q_DECLARE_METATYPE(ComponentsForest::ComponentEvent)

#endif // BASE_COMPONENT_H