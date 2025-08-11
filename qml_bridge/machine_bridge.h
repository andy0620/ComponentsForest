#ifndef MACHINE_BRIDGE_H
#define MACHINE_BRIDGE_H

#include <QObject>
#include <QQmlEngine>
#include <QList>
#include <QVariantMap>
#include <memory>

QT_BEGIN_NAMESPACE
class QThread;
QT_END_NAMESPACE

class CameraBridge;

/**
 * @brief QML bridge for managing multiple camera components through the Machine
 * 
 * This class provides a QML-friendly interface to the Machine class,
 * managing multiple CameraBridge instances and maintaining complete
 * decoupling through string-based signal/slot connections.
 * 
 * Architecture:
 * QML -> MachineBridge -> Machine (via signals)
 *     -> CameraBridge[] -> CameraComponents (via signals)
 */
class MachineBridge : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    
    // Machine state exposed to QML
    Q_PROPERTY(bool running READ isRunning NOTIFY runningChanged)
    Q_PROPERTY(QString state READ state NOTIFY stateChanged)
    Q_PROPERTY(QList<CameraBridge*> cameras READ cameras NOTIFY camerasChanged)
    Q_PROPERTY(int cameraCount READ cameraCount NOTIFY camerasChanged)
    Q_PROPERTY(QVariantMap statistics READ statistics NOTIFY statisticsChanged)
    
public:
    explicit MachineBridge(QObject* parent = nullptr);
    ~MachineBridge() override;
    
    // Property getters
    bool isRunning() const;
    QString state() const;
    QList<CameraBridge*> cameras() const;
    int cameraCount() const;
    QVariantMap statistics() const;
    
    // QML invokable methods
    Q_INVOKABLE bool initialize();
    Q_INVOKABLE void shutdown();
    
    // Machine control
    Q_INVOKABLE bool startMachine();
    Q_INVOKABLE bool stopMachine();
    Q_INVOKABLE bool pauseMachine();
    Q_INVOKABLE bool resumeMachine();
    
    // Camera management
    Q_INVOKABLE CameraBridge* addCamera(const QString& type = "Do3Think", 
                                        const QVariantMap& config = QVariantMap());
    Q_INVOKABLE bool removeCamera(CameraBridge* camera);
    Q_INVOKABLE bool removeCamera(int index);
    Q_INVOKABLE void removeAllCameras();
    
    Q_INVOKABLE CameraBridge* cameraAt(int index) const;
    Q_INVOKABLE CameraBridge* findCamera(const QString& serialNumber) const;
    
    // Batch operations
    Q_INVOKABLE bool startAllCameras();
    Q_INVOKABLE bool stopAllCameras();
    Q_INVOKABLE bool connectAllCameras();
    Q_INVOKABLE bool disconnectAllCameras();
    
    // Configuration
    Q_INVOKABLE bool loadConfiguration(const QString& filePath);
    Q_INVOKABLE bool saveConfiguration(const QString& filePath);
    Q_INVOKABLE void setConfiguration(const QVariantMap& config);
    Q_INVOKABLE QVariantMap configuration() const;
    
    // Diagnostics
    Q_INVOKABLE QVariantList getCameraStatuses() const;
    Q_INVOKABLE QString getMachineInfo() const;
    Q_INVOKABLE void refreshDeviceList();
    
signals:
    // Property change signals
    void runningChanged(bool running);
    void stateChanged(const QString& state);
    void camerasChanged();
    void statisticsChanged();
    
    // Machine events
    void machineInitialized();
    void machineStarted();
    void machineStopped();
    void machinePaused();
    void machineResumed();
    void machineError(const QString& error);
    
    // Camera events
    void cameraAdded(CameraBridge* camera);
    void cameraRemoved(int index);
    void allCamerasRemoved();
    
    // Status updates
    void statusMessage(const QString& message);
    void progressUpdate(int current, int total);
    
    // Signals to Machine (decoupled communication)
    void requestMachineStart();
    void requestMachineStop();
    void requestMachinePause();
    void requestMachineResume();
    void requestMachineShutdown();
    
    // Configuration signals
    void requestLoadConfiguration(const QString& filePath);
    void requestSaveConfiguration(const QString& filePath);
    
private slots:
    // Slots for Machine signals (connected via string-based connections)
    void onMachineStateChanged(int state);
    void onMachineError(const QString& error);
    void onMachineStatisticsUpdated(const QVariantMap& stats);
    void onComponentAdded(QObject* component);
    void onComponentRemoved(const QString& id);
    
    // Camera bridge management
    void onCameraStateChanged();
    void onCameraError(const QString& error);
    void handleCameraDestroyed(QObject* obj);
    
    // Internal updates
    void updateStatistics();
    void updateMachineState();
    
private:
    // Helper methods
    bool createMachine();
    void destroyMachine();
    void connectMachineSignals();
    void disconnectMachineSignals();
    
    CameraBridge* createCameraBridge(const QString& type, const QVariantMap& config);
    void connectCameraBridge(CameraBridge* bridge);
    void disconnectCameraBridge(CameraBridge* bridge);
    
    QString stateToString(int state) const;
    int stringToState(const QString& state) const;
    
    void cleanupCameras();
    bool validateCameraType(const QString& type) const;
    QVariantMap getDefaultCameraConfig(const QString& type) const;
    
private:
    class Private;
    std::unique_ptr<Private> d;
};

#endif // MACHINE_BRIDGE_H