#include "blur_preprocessor.h"
#include "edge_preprocessor.h"
#include "denoise_preprocessor.h"
#include <QApplication>
#include <QTimer>
#include <QDebug>
#include <QJsonObject>
#include <QJsonDocument>
#include <opencv2/opencv.hpp>
#include <memory>

using namespace ComponentsForest::OpenCV;

/**
 * @brief Test program demonstrating the new BaseComponent-based preprocessor system
 */
class PreprocessorTester : public QObject {
    Q_OBJECT
    
public:
    PreprocessorTester() {
        setupTest();
    }
    
private slots:
    void runTests() {
        qDebug() << "=== Testing New Preprocessor Architecture ===";
        
        testBlurPreprocessor();
        testEdgePreprocessor();
        testDenoisePreprocessor();
        testComponentManagement();
        testPerformanceOptimization();
        
        qDebug() << "=== All Tests Completed ===";
        QApplication::quit();
    }
    
    void onComponentStateChanged(ComponentsForest::ComponentState newState, 
                                ComponentsForest::ComponentState oldState) {
        auto* component = qobject_cast<PreProcessorBase*>(sender());
        if (component) {
            qDebug() << "Component" << component->componentName() 
                     << "state changed from" << static_cast<int>(oldState) 
                     << "to" << static_cast<int>(newState);
        }
    }
    
    void onFrameProcessed(const QImage& frame, const FrameMetadata& metadata) {
        qDebug() << "Frame processed:" << frame.size() << "timestamp:" << metadata.timestamp;
    }
    
    void onProcessingError(ProcessingError error, const QString& details) {
        qDebug() << "Processing error:" << static_cast<int>(error) << details;
    }
    
private:
    void setupTest() {
        // Create test image
        cv::Mat testMat = cv::Mat::ones(512, 512, CV_8UC3) * 128;
        cv::rectangle(testMat, cv::Rect(100, 100, 200, 200), cv::Scalar(255, 255, 255), -1);
        cv::circle(testMat, cv::Point(256, 256), 50, cv::Scalar(0, 0, 255), -1);
        
        testImage = PreProcessorBase::matToQImage(testMat);
        
        // Setup metadata
        testMetadata.timestamp = QDateTime::currentMSecsSinceEpoch();
        testMetadata.frameNumber = 1;
        testMetadata.originalSize = testImage.size();
        testMetadata.sourceId = "test";
    }
    
    void testBlurPreprocessor() {
        qDebug() << "\n--- Testing Blur Preprocessor ---";
        
        auto blur = std::make_unique<BlurPreProcessor>();
        
        // Connect signals
        connect(blur.get(), &PreProcessorBase::stateChanged,
                this, &PreprocessorTester::onComponentStateChanged);
        connect(blur.get(), &PreProcessorBase::frameProcessed,
                this, &PreprocessorTester::onFrameProcessed);
        connect(blur.get(), &PreProcessorBase::processingError,
                this, &PreprocessorTester::onProcessingError);
        
        // Test configuration
        QJsonObject config;
        config["kernelWidth"] = 7;
        config["kernelHeight"] = 7;
        config["sigmaX"] = 2.0;
        config["sigmaY"] = 2.0;
        config["gpuEnabled"] = false;
        
        // Initialize component
        if (!blur->initialize(config)) {
            qWarning() << "Failed to initialize blur preprocessor";
            return;
        }
        
        // Start processing
        if (!blur->start()) {
            qWarning() << "Failed to start blur preprocessor";
            return;
        }
        
        // Test processing
        blur->onFrameReceived(testImage, testMetadata);
        
        // Wait a bit for processing
        QTimer::singleShot(100, [&blur]() {
            // Stop component
            blur->stop();
            qDebug() << "Blur preprocessor test completed";
        });
    }
    
    void testEdgePreprocessor() {
        qDebug() << "\n--- Testing Edge Preprocessor ---";
        
        auto edge = std::make_unique<EdgePreProcessor>();
        
        // Connect signals
        connect(edge.get(), &PreProcessorBase::stateChanged,
                this, &PreprocessorTester::onComponentStateChanged);
        connect(edge.get(), &PreProcessorBase::frameProcessed,
                this, &PreprocessorTester::onFrameProcessed);
        
        // Test configuration
        QJsonObject config;
        config["algorithm"] = "canny";
        config["cannyLowThreshold"] = 50.0;
        config["cannyHighThreshold"] = 150.0;
        
        // Initialize and start
        if (edge->initialize(config) && edge->start()) {
            edge->onFrameReceived(testImage, testMetadata);
            
            QTimer::singleShot(100, [&edge]() {
                edge->stop();
                qDebug() << "Edge preprocessor test completed";
            });
        }
    }
    
    void testDenoisePreprocessor() {
        qDebug() << "\n--- Testing Denoise Preprocessor ---";
        
        auto denoise = std::make_unique<DenoisePreProcessor>();
        
        // Connect signals
        connect(denoise.get(), &PreProcessorBase::stateChanged,
                this, &PreprocessorTester::onComponentStateChanged);
        connect(denoise.get(), &PreProcessorBase::frameProcessed,
                this, &PreprocessorTester::onFrameProcessed);
        
        // Test configuration
        QJsonObject config;
        config["algorithm"] = "bilateral";
        config["bilateralD"] = 9;
        config["bilateralSigmaColor"] = 75.0;
        config["bilateralSigmaSpace"] = 75.0;
        
        // Initialize and start
        if (denoise->initialize(config) && denoise->start()) {
            denoise->onFrameReceived(testImage, testMetadata);
            
            QTimer::singleShot(100, [&denoise]() {
                denoise->stop();
                qDebug() << "Denoise preprocessor test completed";
            });
        }
    }
    
    void testComponentManagement() {
        qDebug() << "\n--- Testing Component Management ---";
        
        auto component = std::make_unique<BlurPreProcessor>();
        
        // Test component properties
        qDebug() << "Component ID:" << component->componentId();
        qDebug() << "Component Name:" << component->componentName();
        qDebug() << "Component Type:" << component->componentType();
        qDebug() << "Component Version:" << component->componentVersion();
        qDebug() << "Initial State:" << static_cast<int>(component->state());
        
        // Test health status
        auto health = component->getHealthStatus();
        qDebug() << "Health Status:" << health.status;
        qDebug() << "Is Healthy:" << health.isHealthy;
        
        // Test configuration schema
        QJsonObject schema = component->getConfigurationSchema();
        QJsonDocument doc(schema);
        qDebug() << "Configuration Schema:" << doc.toJson(QJsonDocument::Compact);
        
        qDebug() << "Component management test completed";
    }
    
    void testPerformanceOptimization() {
        qDebug() << "\n--- Testing Performance Optimization ---";
        
        auto component = std::make_unique<BlurPreProcessor>();
        
        // Test memory pool
        cv::Size testSize(640, 480);
        cv::Mat mat1 = component->getMatFromPool(testSize, CV_8UC3);
        cv::Mat mat2 = component->getMatFromPool(testSize, CV_8UC3);
        
        qDebug() << "Mat1 size:" << mat1.size().width << "x" << mat1.size().height;
        qDebug() << "Mat2 size:" << mat2.size().width << "x" << mat2.size().height;
        
        // Return to pool
        component->returnMatToPool(mat1);
        component->returnMatToPool(mat2);
        
        // Test zero-copy conversion
        cv::Mat testMat = cv::Mat::zeros(100, 100, CV_8UC3);
        QImage img1 = PreProcessorBase::matToQImage(testMat, true);  // Zero-copy
        QImage img2 = PreProcessorBase::matToQImage(testMat, false); // Safe copy
        
        qDebug() << "Zero-copy image size:" << img1.size();
        qDebug() << "Safe copy image size:" << img2.size();
        
        qDebug() << "Performance optimization test completed";
    }
    
private:
    QImage testImage;
    FrameMetadata testMetadata;
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    PreprocessorTester tester;
    
    // Start tests after event loop begins
    QTimer::singleShot(100, &tester, &PreprocessorTester::runTests);
    
    return app.exec();
}

#include "test_new_preprocessor.moc"