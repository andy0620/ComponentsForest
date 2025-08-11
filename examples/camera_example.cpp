/**
 * @file camera_example.cpp
 * @brief Example application demonstrating Do3ThinkCameraComponent usage
 * @author ComponentsForest Team
 * @date 2025-01-08
 */

#include <QApplication>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QThread>
#include <QDebug>
#include <QMessageBox>
#include "../Do3ThinkCamera/dothink_camera.h"
#include "../Do3ThinkCamera/dothink_camera_control_panel.h"

using namespace ComponentsForest;

/**
 * @brief Main Application Window
 * Demonstrates the complete decoupling between Component and Control Panel
 */
class CameraExampleWindow : public QMainWindow {
    Q_OBJECT
public:
    CameraExampleWindow(QWidget* parent = nullptr) : QMainWindow(parent) {
        qDebug() << "Creating Camera Example Window...";
        
        // Setup UI
        setupUI();
        
        // Create and initialize camera component
        setupCameraComponent();
        
        // Connect component to control panel (Signal/Slot only)
        connectComponents();
        
        // Start component
        startSystem();
    }
    
    ~CameraExampleWindow() {
        qDebug() << "Shutting down Camera Example...";
        
        // Proper shutdown sequence
        if (m_cameraComponent) {
            m_cameraComponent->stop();
            m_cameraThread->quit();
            m_cameraThread->wait(5000);  // Wait up to 5 seconds
            delete m_cameraComponent;
        }
    }
    
private:
    void setupUI() {
        // Create central widget
        auto* centralWidget = new QWidget(this);
        auto* mainLayout = new QVBoxLayout(centralWidget);
        
        // Create control panel
        m_controlPanel = new Do3ThinkCameraControlPanel(this);
        m_controlPanel->setPanelMode(Do3ThinkCameraControlPanel::PanelMode::Advanced);
        mainLayout->addWidget(m_controlPanel);
        
        setCentralWidget(centralWidget);
        setWindowTitle("ComponentsForest - Do3Think Camera Example");
        resize(1400, 900);
        
        // Add status bar
        statusBar()->showMessage("Ready");
    }
    
    void setupCameraComponent() {
        qDebug() << "Setting up camera component...";
        
        // Create camera component
        m_cameraComponent = new Do3ThinkCameraComponent();
        
        // Create dedicated thread for camera component
        m_cameraThread = new QThread(this);
        
        // Move component to its own thread (as per architecture requirement)
        m_cameraComponent->moveToThread(m_cameraThread);
        
        // Configure component
        QJsonObject config;
        config["autoConnect"] = false;  // Manual connection for demo
        config["defaultExposure"] = 10000;
        config["defaultGain"] = 1.0;
        config["bufferCount"] = 10;
        config["useCallback"] = true;
        config["enableHealthCheck"] = true;
        config["healthCheckInterval"] = 5000;
        
        // Initialize component
        if (!m_cameraComponent->initialize(config)) {
            QMessageBox::critical(this, "Error", "Failed to initialize camera component!");
            return;
        }
        
        qDebug() << "Camera component initialized successfully";
    }
    
    void connectComponents() {
        qDebug() << "Connecting components via Signal/Slot...";
        
        // Connect control panel to camera component
        // This demonstrates complete decoupling - control panel only knows about signals/slots
        m_controlPanel->connectToComponent(m_cameraComponent);
        
        // Connect component signals to application slots
        connect(m_cameraComponent, &Do3ThinkCameraComponent::stateChanged,
                this, &CameraExampleWindow::onComponentStateChanged);
        
        connect(m_cameraComponent, &Do3ThinkCameraComponent::errorOccurred,
                this, &CameraExampleWindow::onComponentError);
        
        // Use CameraComponent signals (from base class)
        connect(m_cameraComponent, &Do3ThinkCameraComponent::frameReady,
                this, [this](const QImage& image, const FrameMetadata& metadata) {
            onImageAcquired(image, metadata.timestamp);
        });
        
        connect(m_cameraComponent, &Do3ThinkCameraComponent::performanceMetricsUpdated,
                this, &CameraExampleWindow::onPerformanceUpdate);
        
        // Connect component lifecycle to thread
        connect(m_cameraThread, &QThread::started,
                m_cameraComponent, [this]() {
            qDebug() << "Camera component thread started";
        });
        
        connect(m_cameraThread, &QThread::finished,
                m_cameraComponent, &QObject::deleteLater);
        
        qDebug() << "Components connected successfully";
    }
    
    void startSystem() {
        qDebug() << "Starting system...";
        
        // Start camera thread
        m_cameraThread->start();
        
        // Start camera component
        QMetaObject::invokeMethod(m_cameraComponent, [this]() {
            if (m_cameraComponent->start()) {
                qDebug() << "Camera component started successfully";
            } else {
                qWarning() << "Failed to start camera component";
            }
        }, Qt::QueuedConnection);
        
        statusBar()->showMessage("System running");
    }
    
private slots:
    void onComponentStateChanged(ComponentState newState, ComponentState oldState) {
        Q_UNUSED(oldState)
        
        QString stateStr;
        switch (newState) {
            case ComponentState::Uninitialized:
                stateStr = "Uninitialized";
                break;
            case ComponentState::Initialized:
                stateStr = "Initialized";
                break;
            case ComponentState::Running:
                stateStr = "Running";
                statusBar()->showMessage("Camera component running", 2000);
                break;
            case ComponentState::Stopped:
                stateStr = "Stopped";
                break;
            case ComponentState::Error:
                stateStr = "Error";
                statusBar()->showMessage("Camera component error!", 5000);
                break;
            default:
                stateStr = "Unknown";
        }
        
        qDebug() << "Component state changed to:" << stateStr;
    }
    
    void onComponentError(const QString& error) {
        qCritical() << "Component error:" << error;
        statusBar()->showMessage(QString("Error: %1").arg(error), 5000);
        
        // Show error dialog for critical errors
        if (error.contains("critical", Qt::CaseInsensitive)) {
            QMessageBox::critical(this, "Camera Error", error);
        }
    }
    
    void onImageAcquired(const QImage& image, qint64 timestamp) {
        Q_UNUSED(image)
        Q_UNUSED(timestamp)
        
        // Update frame counter
        m_frameCount++;
        
        // Update status every 30 frames
        if (m_frameCount % 30 == 0) {
            statusBar()->showMessage(QString("Frames: %1").arg(m_frameCount), 1000);
        }
    }
    
    void onPerformanceUpdate(const QJsonObject& metrics) {
        double fps = metrics["fps"].toDouble();
        double cpuUsage = metrics["cpuUsage"].toDouble();
        double memoryUsage = metrics["memoryUsage"].toDouble();
        
        QString perfStr = QString("FPS: %.1f | CPU: %.1f%% | Memory: %.1f MB")
                         .arg(fps)
                         .arg(cpuUsage)
                         .arg(memoryUsage);
        
        statusBar()->showMessage(perfStr, 2000);
    }
    
private:
    // Components
    Do3ThinkCameraComponent* m_cameraComponent = nullptr;
    Do3ThinkCameraControlPanel* m_controlPanel = nullptr;
    
    // Thread management
    QThread* m_cameraThread = nullptr;
    
    // Statistics
    qint64 m_frameCount = 0;
};

/**
 * @brief Main entry point
 */
int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    // Set application metadata
    app.setApplicationName("ComponentsForest Camera Example");
    app.setOrganizationName("ComponentsForest");
    app.setApplicationVersion("1.0.0");
    
    // Set application style
    app.setStyle("Fusion");
    
    // Create and show main window
    CameraExampleWindow window;
    window.show();
    
    qDebug() << "ComponentsForest Camera Example started";
    qDebug() << "Qt Version:" << QT_VERSION_STR;
    qDebug() << "Architecture: Complete Signal/Slot Decoupling";
    qDebug() << "Component runs in separate QThread";
    
    return app.exec();
}

#include "camera_example.moc"