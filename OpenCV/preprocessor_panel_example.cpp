/**
 * @file preprocessor_panel_example.cpp
 * @brief Example showing how to integrate preprocessor control panel
 * 
 * This example demonstrates how to:
 * 1. Create a preprocessor component
 * 2. Create a preprocessor control panel
 * 3. Connect them together following Signal/Slot architecture
 * 4. Add the panel to MainUI
 */

#include <QApplication>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QThread>
#include <QDebug>

#include "preprocessor_control_panel.h"
#include "simple_edge_preprocessor.h"
#include "../Do3ThinkCamera/dothink_camera.h"
#include "../components/camera_control_panel.h"
#include "../Do3ThinkCamera/dothink_camera_control_panel.h"

using namespace ComponentsForest;
using namespace ComponentsForest::OpenCV;

/**
 * Example window showing camera with preprocessor panel
 */
class PreprocessorExampleWindow : public QMainWindow {
    Q_OBJECT
    
public:
    PreprocessorExampleWindow(QWidget* parent = nullptr) : QMainWindow(parent) {
        setupUI();
        setupComponents();
        connectSignals();
    }
    
    ~PreprocessorExampleWindow() {
        // Stop components before destruction
        if (edgeProcessor) {
            edgeProcessor->stop();
        }
        if (processorThread) {
            processorThread->quit();
            processorThread->wait(5000);
        }
    }
    
private:
    void setupUI() {
        // Create central widget with horizontal layout
        QWidget* centralWidget = new QWidget(this);
        QHBoxLayout* mainLayout = new QHBoxLayout(centralWidget);
        
        // Create camera control panel (left side)
        cameraPanel = new Do3ThinkCameraControlPanel(this);
        cameraPanel->setMinimumWidth(400);
        cameraPanel->setMaximumWidth(500);
        
        // Create preprocessor control panel (right side)
        preprocessorPanel = new PreProcessorControlPanel(this);
        preprocessorPanel->setMinimumWidth(400);
        preprocessorPanel->setMaximumWidth(500);
        
        // Add panels to layout
        mainLayout->addWidget(cameraPanel);
        mainLayout->addWidget(preprocessorPanel);
        mainLayout->addStretch();
        
        setCentralWidget(centralWidget);
        setWindowTitle("Camera with Preprocessor Example");
        resize(1200, 800);
    }
    
    void setupComponents() {
        // Create edge processor component
        edgeProcessor = new SimpleEdgePreProcessor("EdgeProcessor", this);
        
        // Create thread for processor (following architecture)
        processorThread = new QThread(this);
        edgeProcessor->moveToThread(processorThread);
        
        // Configure processor
        QVariantMap config;
        config["lowThreshold"] = 50.0;
        config["highThreshold"] = 150.0;
        config["kernelSize"] = 3;
        config["debugMode"] = true;
        edgeProcessor->configurePreprocessor(config);
        
        // Initialize and start processor
        connect(processorThread, &QThread::started, [this]() {
            edgeProcessor->initialize();
            edgeProcessor->start();
        });
        
        processorThread->start();
        
        // Connect preprocessor to control panel
        preprocessorPanel->connectToPreprocessor(edgeProcessor);
        
        qDebug() << "Preprocessor component created and connected to panel";
    }
    
    void connectSignals() {
        // Connect camera output to preprocessor input
        // This would typically be done through the Machine class
        // For this example, we show the direct connection pattern
        
        /*
        // Example: Connect camera frames to preprocessor
        if (camera && edgeProcessor) {
            // Using string-based connection for decoupling
            connect(camera, SIGNAL(frameReady(QImage, FrameMetadata)),
                    edgeProcessor, SLOT(onFrameReceived(QImage, FrameMetadata)));
        }
        */
        
        // Connect preprocessor output (for display or further processing)
        connect(edgeProcessor, &PreProcessorBase::frameProcessed,
                this, [](const QImage& frame, const FrameMetadata& metadata) {
            // Handle processed frame
            qDebug() << "Processed frame received at" << metadata.timestamp;
        });
        
        // Connect error signals
        connect(edgeProcessor, &PreProcessorBase::processingError,
                this, [](ProcessingError error, const QString& details) {
            qWarning() << "Processing error:" << static_cast<int>(error) << details;
        });
        
        // Connect panel status
        connect(preprocessorPanel, &PreProcessorControlPanel::panelConnected,
                this, [](const QString& panelId) {
            qDebug() << "Preprocessor panel connected:" << panelId;
        });
        
        connect(preprocessorPanel, &PreProcessorControlPanel::panelError,
                this, [](const QString& error) {
            qWarning() << "Panel error:" << error;
        });
    }
    
private:
    // UI components
    Do3ThinkCameraControlPanel* cameraPanel = nullptr;
    PreProcessorControlPanel* preprocessorPanel = nullptr;
    
    // Processing components
    SimpleEdgePreProcessor* edgeProcessor = nullptr;
    QThread* processorThread = nullptr;
};

/**
 * Example of integrating preprocessor panel into existing MainUI
 */
void integratePreprocessorIntoMainUI() {
    /*
    // In your MainUI class, add a method like this:
    
    void MainUI::addPreprocessorPanel(const QString& processorId, 
                                      OpenCV::PreProcessorBase* preprocessor) {
        // Create control panel
        auto* panel = new OpenCV::PreProcessorControlPanel(this);
        panel->setPanelId(processorId);
        
        // Connect to preprocessor
        panel->connectToPreprocessor(preprocessor);
        
        // Add to UI layout (similar to camera panels)
        if (m_layoutMode == TabView) {
            m_tabWidget->addTab(panel, processorId);
        } else if (m_layoutMode == SplitView) {
            m_splitter->addWidget(panel);
        } else {
            m_gridLayout->addWidget(panel);
        }
        
        // Store reference
        m_preprocessorPanels[processorId] = panel;
        
        // Connect panel signals
        connect(panel, &OpenCV::PreProcessorControlPanel::panelError,
                this, &MainUI::onPanelError);
    }
    */
}

/**
 * Example of creating a processing pipeline
 */
void createProcessingPipeline() {
    /*
    // Example: Camera → Edge Processor → Display
    
    // 1. Create camera component
    auto* camera = new Do3ThinkCameraComponent("Camera1");
    
    // 2. Create edge processor
    auto* edgeProcessor = new SimpleEdgePreProcessor("EdgeProc1");
    
    // 3. Connect camera output to processor input
    connect(camera, SIGNAL(frameReady(QImage, FrameMetadata)),
            edgeProcessor, SLOT(onFrameReceived(QImage, FrameMetadata)));
    
    // 4. Create control panels
    auto* cameraPanel = new Do3ThinkCameraControlPanel();
    cameraPanel->connectToCamera(camera);
    
    auto* processorPanel = new PreProcessorControlPanel();
    processorPanel->connectToPreprocessor(edgeProcessor);
    
    // 5. Add panels to UI
    mainUI->addCameraPanel("Camera1", camera);
    mainUI->addPreprocessorPanel("EdgeProc1", edgeProcessor);
    */
}

// Main function for standalone testing
int main_example(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    // Register metatypes for Signal/Slot
    qRegisterMetaType<ComponentsForest::OpenCV::FrameMetadata>("FrameMetadata");
    qRegisterMetaType<ComponentsForest::OpenCV::ProcessingError>("ProcessingError");
    qRegisterMetaType<ComponentsForest::OpenCV::ProcessingStats>("ProcessingStats");
    qRegisterMetaType<ComponentsForest::OpenCV::PerformanceMetrics>("PerformanceMetrics");
    
    // Create and show example window
    PreprocessorExampleWindow window;
    window.show();
    
    return app.exec();
}

// Include MOC for Q_OBJECT
#include "preprocessor_panel_example.moc"