# Child Class Implementation Guide

## Overview

This guide provides comprehensive instructions for implementing child classes that inherit from PreProcessorBase, ensuring minimal boilerplate code while leveraging the powerful base functionality.

## Quick Start Template

### Minimal Child Class Implementation

```cpp
// my_preprocessor.h
#pragma once
#include "preprocessor_base.h"

class MyPreProcessor : public PreProcessorBase {
    Q_OBJECT
    
public:
    explicit MyPreProcessor(QObject* parent = nullptr);
    
protected:
    // Only implement this pure virtual method
    cv::Mat processImplementation(const cv::Mat& input) override;
    
    // Optional: Override for custom configuration
    bool configureImplementation(const QVariantMap& config) override;
};

// my_preprocessor.cpp
#include "my_preprocessor.h"

MyPreProcessor::MyPreProcessor(QObject* parent)
    : PreProcessorBase("MyPreProcessor", parent) {
    // Base class handles all setup
}

cv::Mat MyPreProcessor::processImplementation(const cv::Mat& input) {
    cv::Mat output;
    // Your processing logic here
    cv::GaussianBlur(input, output, cv::Size(5, 5), 1.0);
    return output;
}

bool MyPreProcessor::configureImplementation(const QVariantMap& config) {
    // Handle your specific configuration
    if (config.contains("customParam")) {
        m_customParam = config["customParam"].toDouble();
    }
    return true;
}
```

## Implementation Patterns

### 1. Simple Filter Implementation

```cpp
class GaussianBlurPreProcessor : public PreProcessorBase {
    Q_OBJECT
    
public:
    explicit GaussianBlurPreProcessor(QObject* parent = nullptr)
        : PreProcessorBase("GaussianBlur", parent)
        , m_kernelSize(5)
        , m_sigmaX(1.0)
        , m_sigmaY(1.0) {
        
        // Register configurable parameters
        registerParameter("kernelSize", QVariant(m_kernelSize), 
                         "Gaussian kernel size (odd number)");
        registerParameter("sigmaX", QVariant(m_sigmaX),
                         "Standard deviation in X direction");
        registerParameter("sigmaY", QVariant(m_sigmaY),
                         "Standard deviation in Y direction");
    }
    
protected:
    cv::Mat processImplementation(const cv::Mat& input) override {
        cv::Mat output;
        cv::GaussianBlur(input, output, 
                        cv::Size(m_kernelSize, m_kernelSize),
                        m_sigmaX, m_sigmaY);
        return output;
    }
    
    bool configureImplementation(const QVariantMap& config) override {
        if (config.contains("kernelSize")) {
            int size = config["kernelSize"].toInt();
            if (size % 2 == 0) size++;  // Ensure odd
            m_kernelSize = size;
        }
        
        if (config.contains("sigmaX")) {
            m_sigmaX = config["sigmaX"].toDouble();
        }
        
        if (config.contains("sigmaY")) {
            m_sigmaY = config["sigmaY"].toDouble();
        }
        
        return true;
    }
    
private:
    int m_kernelSize;
    double m_sigmaX;
    double m_sigmaY;
};
```

### 2. Edge Detection Implementation

```cpp
class CannyEdgePreProcessor : public PreProcessorBase {
    Q_OBJECT
    
public:
    explicit CannyEdgePreProcessor(QObject* parent = nullptr)
        : PreProcessorBase("CannyEdge", parent) {
        
        setDefaultConfiguration({
            {"lowThreshold", 50},
            {"highThreshold", 150},
            {"kernelSize", 3},
            {"useL2Gradient", false}
        });
    }
    
protected:
    cv::Mat processImplementation(const cv::Mat& input) override {
        cv::Mat gray, edges;
        
        // Convert to grayscale if needed
        if (input.channels() > 1) {
            cv::cvtColor(input, gray, cv::COLOR_BGR2GRAY);
        } else {
            gray = input;
        }
        
        // Apply Gaussian blur to reduce noise
        cv::GaussianBlur(gray, gray, cv::Size(5, 5), 1.4);
        
        // Apply Canny edge detection
        cv::Canny(gray, edges,
                 m_config["lowThreshold"].toDouble(),
                 m_config["highThreshold"].toDouble(),
                 m_config["kernelSize"].toInt(),
                 m_config["useL2Gradient"].toBool());
        
        // Convert back to BGR for consistency
        cv::Mat output;
        cv::cvtColor(edges, output, cv::COLOR_GRAY2BGR);
        
        return output;
    }
    
    bool validateInput(const cv::Mat& input) override {
        if (input.empty()) {
            setLastError("Input image is empty");
            return false;
        }
        
        if (input.depth() != CV_8U) {
            setLastError("Canny edge detection requires 8-bit input");
            return false;
        }
        
        return true;
    }
};
```

### 3. Morphological Operations

```cpp
class MorphologyPreProcessor : public PreProcessorBase {
    Q_OBJECT
    
public:
    enum Operation {
        Erosion,
        Dilation,
        Opening,
        Closing,
        Gradient,
        TopHat,
        BlackHat
    };
    Q_ENUM(Operation)
    
    explicit MorphologyPreProcessor(QObject* parent = nullptr)
        : PreProcessorBase("Morphology", parent) {
        
        // Initialize with sensible defaults
        m_operation = Opening;
        m_kernelShape = cv::MORPH_RECT;
        m_kernelSize = cv::Size(3, 3);
        m_iterations = 1;
    }
    
protected:
    cv::Mat processImplementation(const cv::Mat& input) override {
        cv::Mat output;
        cv::Mat kernel = cv::getStructuringElement(
            m_kernelShape, m_kernelSize
        );
        
        switch (m_operation) {
        case Erosion:
            cv::erode(input, output, kernel, cv::Point(-1, -1), m_iterations);
            break;
            
        case Dilation:
            cv::dilate(input, output, kernel, cv::Point(-1, -1), m_iterations);
            break;
            
        case Opening:
            cv::morphologyEx(input, output, cv::MORPH_OPEN, kernel,
                           cv::Point(-1, -1), m_iterations);
            break;
            
        case Closing:
            cv::morphologyEx(input, output, cv::MORPH_CLOSE, kernel,
                           cv::Point(-1, -1), m_iterations);
            break;
            
        case Gradient:
            cv::morphologyEx(input, output, cv::MORPH_GRADIENT, kernel,
                           cv::Point(-1, -1), m_iterations);
            break;
            
        case TopHat:
            cv::morphologyEx(input, output, cv::MORPH_TOPHAT, kernel,
                           cv::Point(-1, -1), m_iterations);
            break;
            
        case BlackHat:
            cv::morphologyEx(input, output, cv::MORPH_BLACKHAT, kernel,
                           cv::Point(-1, -1), m_iterations);
            break;
        }
        
        return output;
    }
    
    bool configureImplementation(const QVariantMap& config) override {
        if (config.contains("operation")) {
            QString op = config["operation"].toString();
            m_operation = stringToOperation(op);
        }
        
        if (config.contains("kernelShape")) {
            QString shape = config["kernelShape"].toString();
            m_kernelShape = stringToKernelShape(shape);
        }
        
        if (config.contains("kernelSize")) {
            int size = config["kernelSize"].toInt();
            m_kernelSize = cv::Size(size, size);
        }
        
        if (config.contains("iterations")) {
            m_iterations = config["iterations"].toInt();
        }
        
        return true;
    }
    
private:
    Operation m_operation;
    int m_kernelShape;
    cv::Size m_kernelSize;
    int m_iterations;
    
    Operation stringToOperation(const QString& str) {
        static QMap<QString, Operation> map = {
            {"erosion", Erosion},
            {"dilation", Dilation},
            {"opening", Opening},
            {"closing", Closing},
            {"gradient", Gradient},
            {"tophat", TopHat},
            {"blackhat", BlackHat}
        };
        return map.value(str.toLower(), Opening);
    }
    
    int stringToKernelShape(const QString& str) {
        static QMap<QString, int> map = {
            {"rect", cv::MORPH_RECT},
            {"cross", cv::MORPH_CROSS},
            {"ellipse", cv::MORPH_ELLIPSE}
        };
        return map.value(str.toLower(), cv::MORPH_RECT);
    }
};
```

## Advanced Implementation Patterns

### 1. GPU-Accelerated Preprocessor

```cpp
class GPUBlurPreProcessor : public PreProcessorBase {
    Q_OBJECT
    
public:
    explicit GPUBlurPreProcessor(QObject* parent = nullptr)
        : PreProcessorBase("GPUBlur", parent) {
        
        // Enable GPU by default if available
        setGPUEnabled(cv::cuda::getCudaEnabledDeviceCount() > 0);
    }
    
protected:
    cv::Mat processImplementation(const cv::Mat& input) override {
        if (isGPUEnabled() && cv::cuda::getCudaEnabledDeviceCount() > 0) {
            return processOnGPU(input);
        }
        return processOnCPU(input);
    }
    
private:
    cv::Mat processOnGPU(const cv::Mat& input) {
        cv::cuda::GpuMat gpuInput, gpuOutput;
        
        // Upload to GPU
        gpuInput.upload(input);
        
        // Process on GPU
        auto filter = cv::cuda::createGaussianFilter(
            gpuInput.type(), -1, cv::Size(21, 21), 5.0
        );
        filter->apply(gpuInput, gpuOutput);
        
        // Download result
        cv::Mat output;
        gpuOutput.download(output);
        
        return output;
    }
    
    cv::Mat processOnCPU(const cv::Mat& input) {
        cv::Mat output;
        cv::GaussianBlur(input, output, cv::Size(21, 21), 5.0);
        return output;
    }
};
```

### 2. Multi-Stage Preprocessor

```cpp
class DenoisePreProcessor : public PreProcessorBase {
    Q_OBJECT
    
public:
    explicit DenoisePreProcessor(QObject* parent = nullptr)
        : PreProcessorBase("Denoise", parent) {
        
        // Configure pipeline stages
        m_stages = {
            {"bilateral", true},
            {"morphological", true},
            {"median", false}
        };
    }
    
protected:
    cv::Mat processImplementation(const cv::Mat& input) override {
        cv::Mat result = input.clone();
        
        // Apply enabled stages in sequence
        for (const auto& [stage, enabled] : m_stages) {
            if (!enabled) continue;
            
            if (stage == "bilateral") {
                cv::bilateralFilter(result.clone(), result, 9, 75, 75);
            } else if (stage == "morphological") {
                cv::Mat kernel = cv::getStructuringElement(
                    cv::MORPH_RECT, cv::Size(3, 3)
                );
                cv::morphologyEx(result, result, cv::MORPH_OPEN, kernel);
            } else if (stage == "median") {
                cv::medianBlur(result, result, 5);
            }
            
            // Emit intermediate result if debugging
            if (isDebugMode()) {
                emit intermediateResult(stage, result);
            }
        }
        
        return result;
    }
    
    bool configureImplementation(const QVariantMap& config) override {
        if (config.contains("stages")) {
            auto stages = config["stages"].toMap();
            for (auto& [stage, enabled] : m_stages) {
                if (stages.contains(stage)) {
                    enabled = stages[stage].toBool();
                }
            }
        }
        return true;
    }
    
signals:
    void intermediateResult(const QString& stage, const cv::Mat& result);
    
private:
    QList<QPair<QString, bool>> m_stages;
};
```

### 3. Adaptive Preprocessor

```cpp
class AdaptiveThresholdPreProcessor : public PreProcessorBase {
    Q_OBJECT
    
public:
    explicit AdaptiveThresholdPreProcessor(QObject* parent = nullptr)
        : PreProcessorBase("AdaptiveThreshold", parent) {
        
        m_adaptiveMethod = cv::ADAPTIVE_THRESH_GAUSSIAN_C;
        m_thresholdType = cv::THRESH_BINARY;
        m_blockSize = 11;
        m_C = 2;
    }
    
protected:
    cv::Mat processImplementation(const cv::Mat& input) override {
        cv::Mat gray;
        
        // Convert to grayscale
        if (input.channels() > 1) {
            cv::cvtColor(input, gray, cv::COLOR_BGR2GRAY);
        } else {
            gray = input;
        }
        
        // Analyze image characteristics
        analyzeImage(gray);
        
        // Apply adaptive threshold
        cv::Mat binary;
        cv::adaptiveThreshold(gray, binary, 255,
                            m_adaptiveMethod,
                            m_thresholdType,
                            m_blockSize, m_C);
        
        // Convert back to BGR
        cv::Mat output;
        cv::cvtColor(binary, output, cv::COLOR_GRAY2BGR);
        
        return output;
    }
    
private:
    void analyzeImage(const cv::Mat& image) {
        // Calculate image statistics
        cv::Scalar mean, stddev;
        cv::meanStdDev(image, mean, stddev);
        
        // Adapt parameters based on image characteristics
        if (stddev[0] < 20) {
            // Low contrast image
            m_blockSize = 7;
            m_C = 1;
        } else if (stddev[0] > 50) {
            // High contrast image
            m_blockSize = 15;
            m_C = 5;
        }
        
        // Ensure block size is odd
        if (m_blockSize % 2 == 0) m_blockSize++;
        
        // Emit analysis results
        emit imageAnalyzed(mean[0], stddev[0]);
    }
    
signals:
    void imageAnalyzed(double mean, double stddev);
    
private:
    int m_adaptiveMethod;
    int m_thresholdType;
    int m_blockSize;
    double m_C;
};
```

## Best Practices

### 1. Parameter Validation

```cpp
class ValidatedPreProcessor : public PreProcessorBase {
protected:
    bool configureImplementation(const QVariantMap& config) override {
        // Validate parameters before applying
        if (config.contains("threshold")) {
            double threshold = config["threshold"].toDouble();
            if (threshold < 0 || threshold > 255) {
                setLastError("Threshold must be between 0 and 255");
                return false;
            }
            m_threshold = threshold;
        }
        
        if (config.contains("kernelSize")) {
            int size = config["kernelSize"].toInt();
            if (size < 3 || size > 31 || size % 2 == 0) {
                setLastError("Kernel size must be odd and between 3 and 31");
                return false;
            }
            m_kernelSize = size;
        }
        
        return true;
    }
};
```

### 2. Resource Management

```cpp
class ResourceManagedPreProcessor : public PreProcessorBase {
public:
    ~ResourceManagedPreProcessor() override {
        // Cleanup is handled by base class
        // Only add specific cleanup if needed
        if (m_customResource) {
            delete m_customResource;
        }
    }
    
protected:
    void initializeImplementation() override {
        // Initialize resources that need setup
        m_customResource = new CustomResource();
    }
    
    void cleanupImplementation() override {
        // Clean up resources before destruction
        if (m_customResource) {
            m_customResource->release();
        }
    }
    
private:
    CustomResource* m_customResource = nullptr;
};
```

### 3. Error Handling

```cpp
class RobustPreProcessor : public PreProcessorBase {
protected:
    cv::Mat processImplementation(const cv::Mat& input) override {
        try {
            cv::Mat output;
            
            // Potentially failing operation
            riskyOperation(input, output);
            
            return output;
            
        } catch (const cv::Exception& e) {
            // OpenCV exception
            setLastError(QString("OpenCV error: %1").arg(e.what()));
            emit processingError(ProcessingError::AlgorithmError, e.what());
            
            // Return input unchanged or empty Mat
            return input.clone();
            
        } catch (const std::exception& e) {
            // Standard exception
            setLastError(QString("Processing error: %1").arg(e.what()));
            emit processingError(ProcessingError::UnknownError, e.what());
            
            return cv::Mat();
        }
    }
    
    bool validateInput(const cv::Mat& input) override {
        if (input.empty()) {
            setLastError("Input image is empty");
            return false;
        }
        
        if (input.depth() != CV_8U && input.depth() != CV_16U) {
            setLastError("Unsupported image depth");
            return false;
        }
        
        if (input.cols < 10 || input.rows < 10) {
            setLastError("Image too small");
            return false;
        }
        
        return true;
    }
};
```

## Testing Your Preprocessor

### Unit Test Template

```cpp
class TestMyPreProcessor : public QObject {
    Q_OBJECT
    
private slots:
    void initTestCase() {
        m_processor = new MyPreProcessor();
    }
    
    void testProcessing() {
        // Create test image
        cv::Mat testImage(100, 100, CV_8UC3, cv::Scalar(128, 128, 128));
        QImage qImage = matToQImage(testImage);
        
        // Connect to result signal
        QSignalSpy spy(m_processor, &PreProcessorBase::frameProcessed);
        
        // Process image
        m_processor->onFrameReceived(qImage, FrameMetadata());
        
        // Wait for processing
        QVERIFY(spy.wait(1000));
        
        // Verify result
        QCOMPARE(spy.count(), 1);
        QList<QVariant> arguments = spy.takeFirst();
        QImage result = qvariant_cast<QImage>(arguments.at(0));
        
        QVERIFY(!result.isNull());
        QCOMPARE(result.size(), qImage.size());
    }
    
    void testConfiguration() {
        QVariantMap config;
        config["parameter1"] = 10;
        config["parameter2"] = "value";
        
        bool success = m_processor->configure(config);
        QVERIFY(success);
        
        // Verify configuration was applied
        QCOMPARE(m_processor->getParameter("parameter1").toInt(), 10);
    }
    
    void cleanupTestCase() {
        delete m_processor;
    }
    
private:
    MyPreProcessor* m_processor;
};
```

## Common Pitfalls and Solutions

### 1. Memory Leaks
**Problem**: Forgetting to release OpenCV matrices
**Solution**: Use RAII and let cv::Mat handle memory automatically

### 2. Thread Safety
**Problem**: Accessing member variables from processing thread
**Solution**: Use atomic variables or mutex protection for shared state

### 3. Performance
**Problem**: Unnecessary copying of image data
**Solution**: Use cv::Mat references and in-place operations when possible

### 4. Signal/Slot Connections
**Problem**: Forgetting to use Q_OBJECT macro
**Solution**: Always include Q_OBJECT in class definition and run moc

### 5. Configuration
**Problem**: Not validating configuration parameters
**Solution**: Always validate in configureImplementation()