#ifndef CAMERA_BRIDGE_H
#define CAMERA_BRIDGE_H

#include <QObject>
#include <QImage>
#include <QString>
#include <QVariantList>
#include <QVariantMap>
#include <QTimer>
#include <QMutex>
#include <memory>
#include <atomic>

class QThread;

/**
 * @brief CameraBridge provides QML access to camera components
 * 
 * This bridge class exposes camera functionality to QML while maintaining
 * complete Signal/Slot decoupling with the underlying camera components.
 * It uses string-based connections to avoid compile-time dependencies.
 */
class CameraBridge : public QObject
{
    Q_OBJECT
    
    // Device properties
    Q_PROPERTY(QString deviceId READ deviceId NOTIFY deviceIdChanged)
    Q_PROPERTY(QString deviceName READ deviceName NOTIFY deviceNameChanged)
    Q_PROPERTY(QVariantList deviceList READ deviceList NOTIFY deviceListChanged)
    
    // Connection state
    Q_PROPERTY(bool connected READ isConnected NOTIFY connectedChanged)
    Q_PROPERTY(bool acquiring READ isAcquiring NOTIFY acquiringChanged)
    Q_PROPERTY(QString connectionStatus READ connectionStatus NOTIFY connectionStatusChanged)
    
    // Camera parameters
    Q_PROPERTY(double exposureTime READ exposureTime WRITE setExposureTime NOTIFY exposureTimeChanged)
    Q_PROPERTY(double gain READ gain WRITE setGain NOTIFY gainChanged)
    Q_PROPERTY(double frameRate READ frameRate WRITE setFrameRate NOTIFY frameRateChanged)
    Q_PROPERTY(int width READ width NOTIFY widthChanged)
    Q_PROPERTY(int height READ height NOTIFY heightChanged)
    
    // Performance metrics
    Q_PROPERTY(double currentFps READ currentFps NOTIFY fpsChanged)
    Q_PROPERTY(qint64 frameCount READ frameCount NOTIFY frameCountChanged)
    Q_PROPERTY(qint64 droppedFrames READ droppedFrames NOTIFY droppedFramesChanged)
    
    // Image display
    Q_PROPERTY(QString currentImageUrl READ currentImageUrl NOTIFY imageUrlChanged)
    Q_PROPERTY(bool hasNewFrame READ hasNewFrame NOTIFY hasNewFrameChanged)
    
    // ROI properties
    Q_PROPERTY(int roiX READ roiX WRITE setRoiX NOTIFY roiChanged)
    Q_PROPERTY(int roiY READ roiY WRITE setRoiY NOTIFY roiChanged)
    Q_PROPERTY(int roiWidth READ roiWidth WRITE setRoiWidth NOTIFY roiChanged)
    Q_PROPERTY(int roiHeight READ roiHeight WRITE setRoiHeight NOTIFY roiChanged)
    Q_PROPERTY(bool roiEnabled READ roiEnabled WRITE setRoiEnabled NOTIFY roiEnabledChanged)

public:
    explicit CameraBridge(QObject *parent = nullptr);
    virtual ~CameraBridge();
    
    // Property getters
    QString deviceId() const { return m_deviceId; }
    QString deviceName() const { return m_deviceName; }
    QVariantList deviceList() const { return m_deviceList; }
    bool isConnected() const { return m_connected; }
    bool isAcquiring() const { return m_acquiring; }
    QString connectionStatus() const { return m_connectionStatus; }
    double exposureTime() const { return m_exposureTime; }
    double gain() const { return m_gain; }
    double frameRate() const { return m_frameRate; }
    int width() const { return m_width; }
    int height() const { return m_height; }
    double currentFps() const { return m_currentFps; }
    qint64 frameCount() const { return m_frameCount; }
    qint64 droppedFrames() const { return m_droppedFrames; }
    QString currentImageUrl() const;
    bool hasNewFrame() const { return m_hasNewFrame; }
    int roiX() const { return m_roiX; }
    int roiY() const { return m_roiY; }
    int roiWidth() const { return m_roiWidth; }
    int roiHeight() const { return m_roiHeight; }
    bool roiEnabled() const { return m_roiEnabled; }
    
    // Property setters
    void setExposureTime(double value);
    void setGain(double value);
    void setFrameRate(double value);
    void setRoiX(int value);
    void setRoiY(int value);
    void setRoiWidth(int value);
    void setRoiHeight(int value);
    void setRoiEnabled(bool value);
    
    // Connect to existing component
    void connectToComponent(QObject *cameraComponent);
    void disconnectFromComponent();
    
    // QML-invokable methods
    Q_INVOKABLE void scanDevices();
    Q_INVOKABLE void connectCamera(const QString &deviceId);
    Q_INVOKABLE void disconnectCamera();
    Q_INVOKABLE void startAcquisition();
    Q_INVOKABLE void stopAcquisition();
    Q_INVOKABLE void captureFrame();
    Q_INVOKABLE void saveImage(const QString &filePath);
    Q_INVOKABLE void setParameter(const QString &name, const QVariant &value);
    Q_INVOKABLE QVariant getParameter(const QString &name) const;
    Q_INVOKABLE void resetToDefaults();
    Q_INVOKABLE void loadConfiguration(const QString &filePath);
    Q_INVOKABLE void saveConfiguration(const QString &filePath);
    Q_INVOKABLE void setROI(const QVariantMap &roi);
    Q_INVOKABLE void clearROI();
    Q_INVOKABLE QVariantMap getStatistics() const;
    
signals:
    // Property change signals
    void deviceIdChanged();
    void deviceNameChanged();
    void deviceListChanged();
    void connectedChanged();
    void acquiringChanged();
    void connectionStatusChanged();
    void exposureTimeChanged();
    void gainChanged();
    void frameRateChanged();
    void widthChanged();
    void heightChanged();
    void fpsChanged();
    void frameCountChanged();
    void droppedFramesChanged();
    void imageUrlChanged();
    void hasNewFrameChanged();
    void roiChanged();
    void roiEnabledChanged();
    
    // Event signals
    void frameReady(const QString &imageUrl, qint64 timestamp);
    void errorOccurred(const QString &error);
    void deviceDiscovered(const QVariantMap &device);
    void configurationLoaded();
    void configurationSaved();
    
    // Request signals (to component)
    void requestScanDevices();
    void requestConnect(const QString &deviceId);
    void requestDisconnect();
    void requestStartAcquisition();
    void requestStopAcquisition();
    void requestCaptureFrame();
    
private slots:
    // Slots for component signals (string-based connection)
    void onComponentFrameReady(const QImage &image, qint64 timestamp);
    void onComponentStateChanged(int state);
    void onComponentErrorOccurred(const QString &error);
    void onComponentDeviceListUpdated(const QVariantList &devices);
    void onComponentConnected();
    void onComponentDisconnected();
    void onComponentParameterChanged(const QString &name, const QVariant &value);
    void onComponentStatisticsUpdated(const QVariantMap &stats);
    
private:
    void setupComponentConnections(QObject *component);
    void teardownComponentConnections(QObject *component);
    void updateImageUrl(const QImage &image);
    void updateStatistics();
    
    // Component reference (no direct type dependency)
    QObject *m_component = nullptr;
    
    // Cached state
    QString m_deviceId;
    QString m_deviceName;
    QVariantList m_deviceList;
    std::atomic<bool> m_connected{false};
    std::atomic<bool> m_acquiring{false};
    QString m_connectionStatus;
    double m_exposureTime = 100.0;
    double m_gain = 1.0;
    double m_frameRate = 30.0;
    int m_width = 0;
    int m_height = 0;
    double m_currentFps = 0.0;
    std::atomic<qint64> m_frameCount{0};
    std::atomic<qint64> m_droppedFrames{0};
    std::atomic<bool> m_hasNewFrame{false};
    
    // ROI parameters
    int m_roiX = 0;
    int m_roiY = 0;
    int m_roiWidth = 0;
    int m_roiHeight = 0;
    bool m_roiEnabled = false;
    
    // Thread safety
    mutable QMutex m_imageMutex;
    QImage m_currentImage;
    QString m_currentImageId;
    
    // Statistics update timer
    QTimer *m_statsTimer = nullptr;
    
    // Image provider reference (if used)
    QString m_imageProviderId;
    static int s_bridgeCounter;
};

#endif // CAMERA_BRIDGE_H