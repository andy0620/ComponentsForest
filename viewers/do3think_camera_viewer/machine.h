#ifndef DO3THINK_CAMERA_VIEWER_MACHINE_H
#define DO3THINK_CAMERA_VIEWER_MACHINE_H

#include <QObject>
#include <QThread>
#include <QMap>
#include <QVariantMap>
#include <memory>

namespace ComponentsForest {

namespace OpenCV {
class ThresholdPreProcessor;
class ContourAreaAlgorithm;
}

class Do3ThinkCameraComponent;
class BaseComponent;

class Do3ThinkCameraMachine : public QObject
{
    Q_OBJECT

public:
    explicit Do3ThinkCameraMachine(QObject *parent = nullptr);
    ~Do3ThinkCameraMachine();

    // Camera management
    bool addCamera(const QString &cameraId, const QVariantMap &config);
    bool removeCamera(const QString &cameraId);
    bool hasCamera(const QString &cameraId) const;
    QStringList getCameraIds() const;
    
    // Component access (for control panel connection)
    Do3ThinkCameraComponent* getCameraComponent(const QString &cameraId) const;
    BaseComponent* getComponent(const QString &cameraId) const;
    
    // Machine control
    bool start();
    bool stop();
    bool isRunning() const;
    
    // Camera control
    bool startCamera(const QString &cameraId);
    bool stopCamera(const QString &cameraId);
    bool startAllCameras();
    bool stopAllCameras();
    
    // Configuration
    void setDefaultCameraConfig(const QVariantMap &config);
    QVariantMap getDefaultCameraConfig() const;
    
    // Device discovery
    QStringList discoverAvailableDevices();
    bool refreshDeviceList();

public slots:
    void runContourAnalysis(const QString& cameraId);

signals:
    // Machine state signals
    void machineStarted();
    void machineStopped();
    void machineError(const QString &error);
    
    // Camera management signals
    void cameraAdded(const QString &cameraId);
    void cameraRemoved(const QString &cameraId);
    void cameraStarted(const QString &cameraId);
    void cameraStopped(const QString &cameraId);
    void cameraError(const QString &cameraId, const QString &error);
    
    // Device discovery signals
    void devicesDiscovered(const QStringList &devices);
    void deviceConnected(const QString &deviceName);
    void deviceDisconnected(const QString &deviceName);
    
    // Performance monitoring
    void performanceUpdate(const QString &cameraId, const QVariantMap &metrics);

    // Analysis results
    void contourAnalysisResult(const QString &cameraId, const QVariantList &areas);

private slots:
    void onComponentStateChanged();
    void onComponentError(const QString &error);
    void onComponentPerformanceUpdate(const QVariantMap &metrics);

private:
    class Private;
    std::unique_ptr<Private> d;
    
    // Helper methods
    bool initializeComponent(const QString &cameraId, const QVariantMap &config);
    void cleanupComponent(const QString &cameraId);
    void connectComponentSignals(Do3ThinkCameraComponent *component);
    void disconnectComponentSignals(Do3ThinkCameraComponent *component);
};

} // namespace ComponentsForest

#endif // DO3THINK_CAMERA_VIEWER_MACHINE_H