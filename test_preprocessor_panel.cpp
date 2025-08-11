/**
 * @file test_preprocessor_panel.cpp
 * @brief Test application to demonstrate preprocessor control panel
 * 
 * This creates a simple window showing the preprocessor control panel
 * with a working edge detection preprocessor.
 */

#include <QApplication>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QThread>
#include <QDebug>
#include <QTimer>

#include "OpenCV/preprocessor_control_panel.h"
#include "OpenCV/simple_edge_preprocessor.h"

using namespace ComponentsForest::OpenCV;

class TestWindow : public QMainWindow {
    Q_OBJECT
    
public:
    TestWindow(QWidget* parent = nullptr) : QMainWindow(parent) {
        setWindowTitle("Preprocessor Control Panel Test");
        
        // Create central widget
        QWidget* central = new QWidget(this);
        QVBoxLayout* layout = new QVBoxLayout(central);
        
        // Create status label
        statusLabel = new QLabel("Preprocessor Panel Test - Ready", this);
        statusLabel->setStyleSheet("QLabel { background-color: #2b2b2b; color: #00ff00; padding: 10px; font-size: 14px; }");
        layout->addWidget(statusLabel);
        
        // Create preprocessor control panel
        preprocessorPanel = new PreProcessorControlPanel(this);
        layout->addWidget(preprocessorPanel);
        
        // Create edge processor
        edgeProcessor = new SimpleEdgePreProcessor("TestEdgeProcessor", this);
        
        // Create thread for processor
        processorThread = new QThread(this);
        edgeProcessor->moveToThread(processorThread);
        
        // Initialize processor when thread starts
        connect(processorThread, &QThread::started, [this]() {
            QVariantMap config;
            config["lowThreshold"] = 100.0;
            config["highThreshold"] = 200.0;
            config["kernelSize"] = 5;
            config["debugMode"] = true;
            
            edgeProcessor->configurePreprocessor(config);
            edgeProcessor->initialize();
            statusLabel->setText("Edge Processor Initialized");
        });
        
        // Connect panel to processor
        preprocessorPanel->connectToPreprocessor(edgeProcessor);
        
        // Start processor thread
        processorThread->start();
        
        // Connect status signals
        connect(edgeProcessor, &PreProcessorBase::stateChanged, 
                this, [this](int state) {
            statusLabel->setText(QString("Processor State: %1").arg(state));
        });
        
        connect(preprocessorPanel, &PreProcessorControlPanel::panelConnected,
                this, [this](const QString& id) {
            statusLabel->setText(QString("Panel Connected: %1").arg(id));
        });
        
        setCentralWidget(central);
        resize(600, 800);
        
        // Add test button
        QPushButton* testBtn = new QPushButton("Send Test Frame", this);
        layout->addWidget(testBtn);
        
        connect(testBtn, &QPushButton::clicked, this, &TestWindow::sendTestFrame);
    }
    
    ~TestWindow() {
        if (edgeProcessor) {
            edgeProcessor->stop();
        }
        if (processorThread) {
            processorThread->quit();
            processorThread->wait(5000);
        }
    }
    
private slots:
    void sendTestFrame() {
        // Create a test image
        QImage testImage(640, 480, QImage::Format_RGB888);
        testImage.fill(Qt::gray);
        
        // Draw some test pattern
        QPainter painter(&testImage);
        painter.setPen(Qt::white);
        for (int i = 0; i < 10; i++) {
            painter.drawLine(i * 64, 0, i * 64, 480);
            painter.drawLine(0, i * 48, 640, i * 48);
        }
        
        // Send to processor
        FrameMetadata metadata;
        metadata.timestamp = QDateTime::currentMSecsSinceEpoch();
        metadata.frameNumber = frameCounter++;
        metadata.originalSize = testImage.size();
        
        edgeProcessor->onFrameReceived(testImage, metadata);
        
        statusLabel->setText(QString("Test frame %1 sent").arg(metadata.frameNumber));
    }
    
private:
    QLabel* statusLabel;
    PreProcessorControlPanel* preprocessorPanel;
    SimpleEdgePreProcessor* edgeProcessor;
    QThread* processorThread;
    int frameCounter = 0;
};

#include "test_preprocessor_panel.moc"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    
    // Register metatypes
    qRegisterMetaType<FrameMetadata>("FrameMetadata");
    qRegisterMetaType<ProcessingError>("ProcessingError");
    qRegisterMetaType<ProcessingMode>("ProcessingMode");
    qRegisterMetaType<ProcessingStats>("ProcessingStats");
    qRegisterMetaType<PerformanceMetrics>("PerformanceMetrics");
    
    TestWindow window;
    window.show();
    
    return app.exec();
}