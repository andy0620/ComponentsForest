# 標準元件開發規範
*Industrial AOI Equipment Standard Components Development Guide*

## 目錄
1. [相機元件 (Camera Component)](#相機元件-camera-component)
2. [影像處理元件 (Image Processing Component)](#影像處理元件-image-processing-component)
3. [演算法元件 (Algorithm Component)](#演算法元件-algorithm-component)
4. [運動控制元件 (Motion Control Component)](#運動控制元件-motion-control-component)
5. [資料儲存元件 (Data Storage Component)](#資料儲存元件-data-storage-component)

---

## 相機元件 (Camera Component)

### 元件介面定義

```cpp
// CameraComponent.h
#pragma once

#include "BaseComponent.h"
#include <QImage>
#include <QMutex>
#include <memory>
#include <atomic>

class CameraComponent : public BaseComponent {
    Q_OBJECT
    Q_PROPERTY(bool isAcquiring READ isAcquiring NOTIFY acquisitionStateChanged)
    Q_PROPERTY(double frameRate READ frameRate NOTIFY frameRateChanged)
    Q_PROPERTY(int exposureTime READ exposureTime WRITE setExposureTime NOTIFY exposureTimeChanged)
    Q_PROPERTY(double gain READ gain WRITE setGain NOTIFY gainChanged)

public:
    enum class TriggerMode {
        FreeRun,
        Software,
        Hardware,
        LineStart
    };
    Q_ENUM(TriggerMode)

    enum class PixelFormat {
        Mono8,
        Mono12,
        BayerRG8,
        BayerRG12,
        RGB8,
        BGR8
    };
    Q_ENUM(PixelFormat)

    explicit CameraComponent(QObject* parent = nullptr);
    ~CameraComponent() override;

    // BaseComponent interface
    bool initialize(const Configuration& config) override;
    bool start() override;
    bool stop() override;
    void cleanup() override;

    // Camera specific methods
    bool connectCamera(const QString& serialNumber);
    bool disconnectCamera();
    
    // Acquisition control
    bool startAcquisition();
    bool stopAcquisition();
    bool triggerSoftware();
    
    // Parameter access
    bool isAcquiring() const { return m_acquiring; }
    double frameRate() const { return m_frameRate; }
    int exposureTime() const { return m_exposureTime; }
    double gain() const { return m_gain; }
    
    // ROI management
    bool setROI(int x, int y, int width, int height);
    QRect getROI() const;

signals:
    // Image signals
    void imageAcquired(const QImage& image, qint64 timestamp);
    void rawImageAcquired(const QByteArray& data, int width, int height, 
                         PixelFormat format, qint64 timestamp);
    
    // State signals
    void acquisitionStateChanged(bool acquiring);
    void frameRateChanged(double fps);
    void exposureTimeChanged(int microseconds);
    void gainChanged(double gain);
    void cameraConnected(const QString& serialNumber);
    void cameraDisconnected();
    
    // Error signals
    void frameDropped(int count);
    void acquisitionError(const QString& error);

public slots:
    void setExposureTime(int microseconds);
    void setGain(double gain);
    void setTriggerMode(TriggerMode mode);
    void setPixelFormat(PixelFormat format);
    void setFrameRate(double fps);

private:
    class ImageBuffer {
    public:
        static constexpr size_t BUFFER_COUNT = 10;
        
        struct Frame {
            QByteArray data;
            int width = 0;
            int height = 0;
            PixelFormat format = PixelFormat::Mono8;
            qint64 timestamp = 0;
            std::atomic<bool> ready{false};
        };
        
        Frame* getNextWriteBuffer();
        Frame* getNextReadBuffer();
        void releaseReadBuffer(Frame* frame);
        
    private:
        std::array<Frame, BUFFER_COUNT> m_buffers;
        std::atomic<size_t> m_writeIndex{0};
        std::atomic<size_t> m_readIndex{0};
        QMutex m_mutex;
    };

    // Camera implementation (varies by SDK)
    class CameraImpl;
    std::unique_ptr<CameraImpl> m_impl;
    
    // State
    std::atomic<bool> m_acquiring{false};
    std::atomic<double> m_frameRate{30.0};
    std::atomic<int> m_exposureTime{10000};
    std::atomic<double> m_gain{1.0};
    
    // Buffers
    std::unique_ptr<ImageBuffer> m_imageBuffer;
    
    // Worker thread
    void acquisitionThread();
    std::thread m_acquisitionThread;
    std::atomic<bool> m_stopThread{false};
};
```

### 實作範例

```cpp
// CameraComponent.cpp
#include "CameraComponent.h"
#include <QTimer>
#include <QDebug>
#include <chrono>

CameraComponent::CameraComponent(QObject* parent)
    : BaseComponent(parent)
    , m_imageBuffer(std::make_unique<ImageBuffer>())
{
    setObjectName("CameraComponent");
}

CameraComponent::~CameraComponent() {
    if (m_acquiring) {
        stopAcquisition();
    }
    cleanup();
}

bool CameraComponent::initialize(const Configuration& config) {
    if (!BaseComponent::initialize(config)) {
        return false;
    }
    
    // Load camera configuration
    auto cameraConfig = config.value("camera").toObject();
    QString cameraType = cameraConfig["type"].toString("GigE");
    
    // Initialize camera SDK based on type
    if (cameraType == "GigE") {
        // Initialize GigE Vision SDK
        m_impl = std::make_unique<GigECameraImpl>();
    } else if (cameraType == "USB3") {
        // Initialize USB3 Vision SDK
        m_impl = std::make_unique<USB3CameraImpl>();
    }
    
    // Set default parameters
    setExposureTime(cameraConfig["exposureTime"].toInt(10000));
    setGain(cameraConfig["gain"].toDouble(1.0));
    setFrameRate(cameraConfig["frameRate"].toDouble(30.0));
    
    emit initialized();
    return true;
}

bool CameraComponent::connectCamera(const QString& serialNumber) {
    if (!m_impl) {
        emit errorOccurred("Camera not initialized");
        return false;
    }
    
    if (m_impl->connect(serialNumber)) {
        emit cameraConnected(serialNumber);
        return true;
    }
    
    emit errorOccurred("Failed to connect camera: " + serialNumber);
    return false;
}

bool CameraComponent::startAcquisition() {
    if (m_acquiring) {
        return true;
    }
    
    m_stopThread = false;
    m_acquisitionThread = std::thread(&CameraComponent::acquisitionThread, this);
    
    m_acquiring = true;
    emit acquisitionStateChanged(true);
    
    return true;
}

bool CameraComponent::stopAcquisition() {
    if (!m_acquiring) {
        return true;
    }
    
    m_stopThread = true;
    if (m_acquisitionThread.joinable()) {
        m_acquisitionThread.join();
    }
    
    m_acquiring = false;
    emit acquisitionStateChanged(false);
    
    return true;
}

void CameraComponent::acquisitionThread() {
    auto lastFpsUpdate = std::chrono::steady_clock::now();
    int frameCount = 0;
    
    while (!m_stopThread) {
        auto* buffer = m_imageBuffer->getNextWriteBuffer();
        if (!buffer) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }
        
        // Acquire image from camera
        if (m_impl->acquireImage(buffer->data, buffer->width, 
                                 buffer->height, buffer->format)) {
            buffer->timestamp = QDateTime::currentMSecsSinceEpoch();
            buffer->ready = true;
            
            // Convert to QImage for display
            QImage image(reinterpret_cast<const uchar*>(buffer->data.data()),
                        buffer->width, buffer->height,
                        QImage::Format_Grayscale8);
            
            emit imageAcquired(image, buffer->timestamp);
            emit rawImageAcquired(buffer->data, buffer->width, buffer->height,
                                buffer->format, buffer->timestamp);
            
            frameCount++;
        }
        
        // Update frame rate
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            now - lastFpsUpdate).count();
        
        if (elapsed >= 1) {
            m_frameRate = frameCount / static_cast<double>(elapsed);
            emit frameRateChanged(m_frameRate);
            frameCount = 0;
            lastFpsUpdate = now;
        }
    }
}

void CameraComponent::setExposureTime(int microseconds) {
    if (m_exposureTime == microseconds) return;
    
    if (m_impl && m_impl->setExposureTime(microseconds)) {
        m_exposureTime = microseconds;
        emit exposureTimeChanged(microseconds);
    }
}

void CameraComponent::setGain(double gain) {
    if (m_gain == gain) return;
    
    if (m_impl && m_impl->setGain(gain)) {
        m_gain = gain;
        emit gainChanged(gain);
    }
}
```

### Control Panel 設計 (QML)

```qml
// CameraControlPanel.qml
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import ComponentsForest 1.0

Rectangle {
    id: root
    color: "#2b2b2b"
    
    property CameraComponent camera
    
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 10
        
        // Connection controls
        GroupBox {
            title: "Connection"
            Layout.fillWidth: true
            
            RowLayout {
                anchors.fill: parent
                
                TextField {
                    id: serialNumberField
                    placeholderText: "Camera Serial Number"
                    Layout.fillWidth: true
                }
                
                Button {
                    text: camera && camera.isConnected ? "Disconnect" : "Connect"
                    onClicked: {
                        if (camera.isConnected) {
                            camera.disconnectCamera()
                        } else {
                            camera.connectCamera(serialNumberField.text)
                        }
                    }
                }
            }
        }
        
        // Acquisition controls
        GroupBox {
            title: "Acquisition"
            Layout.fillWidth: true
            
            GridLayout {
                anchors.fill: parent
                columns: 2
                
                Label { text: "Status:" }
                Label {
                    text: camera && camera.isAcquiring ? "Acquiring" : "Stopped"
                    color: camera && camera.isAcquiring ? "#4CAF50" : "#F44336"
                }
                
                Label { text: "Frame Rate:" }
                Label {
                    text: camera ? camera.frameRate.toFixed(1) + " fps" : "0.0 fps"
                }
                
                Button {
                    text: camera && camera.isAcquiring ? "Stop" : "Start"
                    Layout.columnSpan: 2
                    Layout.fillWidth: true
                    onClicked: {
                        if (camera.isAcquiring) {
                            camera.stopAcquisition()
                        } else {
                            camera.startAcquisition()
                        }
                    }
                }
            }
        }
        
        // Parameter controls
        GroupBox {
            title: "Parameters"
            Layout.fillWidth: true
            
            GridLayout {
                anchors.fill: parent
                columns: 2
                
                Label { text: "Exposure (μs):" }
                SpinBox {
                    from: 10
                    to: 1000000
                    value: camera ? camera.exposureTime : 10000
                    stepSize: 1000
                    editable: true
                    onValueChanged: {
                        if (camera) camera.exposureTime = value
                    }
                }
                
                Label { text: "Gain:" }
                Slider {
                    from: 1.0
                    to: 16.0
                    value: camera ? camera.gain : 1.0
                    onValueChanged: {
                        if (camera) camera.gain = value
                    }
                }
                
                Label { text: "Trigger Mode:" }
                ComboBox {
                    model: ["Free Run", "Software", "Hardware", "Line Start"]
                    onCurrentIndexChanged: {
                        if (camera) camera.setTriggerMode(currentIndex)
                    }
                }
                
                Button {
                    text: "Software Trigger"
                    Layout.columnSpan: 2
                    Layout.fillWidth: true
                    enabled: camera && camera.triggerMode === CameraComponent.Software
                    onClicked: camera.triggerSoftware()
                }
            }
        }
        
        // Image display
        GroupBox {
            title: "Preview"
            Layout.fillWidth: true
            Layout.fillHeight: true
            
            Image {
                id: preview
                anchors.fill: parent
                fillMode: Image.PreserveAspectFit
                cache: false
                
                Connections {
                    target: camera
                    function onImageAcquired(image, timestamp) {
                        preview.source = "image://camera/" + timestamp
                    }
                }
            }
        }
    }
}
```

---

## 影像處理元件 (Image Processing Component)

### 元件介面定義

```cpp
// ImageProcessingComponent.h
#pragma once

#include "BaseComponent.h"
#include <opencv2/opencv.hpp>
#include <QImage>
#include <memory>

class ImageProcessingComponent : public BaseComponent {
    Q_OBJECT
    Q_PROPERTY(int processingTime READ processingTime NOTIFY processingTimeChanged)
    Q_PROPERTY(bool gpuEnabled READ isGpuEnabled WRITE setGpuEnabled NOTIFY gpuEnabledChanged)

public:
    enum class FilterType {
        None,
        Gaussian,
        Median,
        Bilateral,
        MorphologyOpen,
        MorphologyClose,
        Sobel,
        Canny,
        HistogramEqualization,
        Custom
    };
    Q_ENUM(FilterType)

    explicit ImageProcessingComponent(QObject* parent = nullptr);
    ~ImageProcessingComponent() override;

    // BaseComponent interface
    bool initialize(const Configuration& config) override;
    bool start() override;
    bool stop() override;

    // Processing pipeline
    class ProcessingPipeline {
    public:
        struct Stage {
            FilterType type;
            QVariantMap parameters;
            std::function<cv::Mat(const cv::Mat&, const QVariantMap&)> processor;
        };
        
        void addStage(const Stage& stage);
        void removeStage(int index);
        void clearStages();
        cv::Mat process(const cv::Mat& input);
        
    private:
        std::vector<Stage> m_stages;
        mutable QMutex m_mutex;
    };

    // GPU acceleration
    bool isGpuEnabled() const { return m_gpuEnabled; }
    void setGpuEnabled(bool enabled);
    
    // Performance metrics
    int processingTime() const { return m_lastProcessingTime; }

signals:
    void imageProcessed(const QImage& result, qint64 timestamp);
    void processingTimeChanged(int milliseconds);
    void gpuEnabledChanged(bool enabled);
    void pipelineUpdated();
    void processingError(const QString& error);

public slots:
    void processImage(const QImage& image, qint64 timestamp);
    void processRawImage(const QByteArray& data, int width, int height);
    void addFilter(FilterType type, const QVariantMap& parameters);
    void removeFilter(int index);
    void clearFilters();
    void updateFilterParameters(int index, const QVariantMap& parameters);

private:
    // Batch processing
    class BatchProcessor {
    public:
        explicit BatchProcessor(int batchSize = 10);
        void addImage(const cv::Mat& image);
        std::vector<cv::Mat> processBatch(
            std::function<cv::Mat(const cv::Mat&)> processor);
        
    private:
        std::vector<cv::Mat> m_batch;
        int m_batchSize;
        QMutex m_mutex;
    };

    // Filter implementations
    cv::Mat applyGaussian(const cv::Mat& input, const QVariantMap& params);
    cv::Mat applyMedian(const cv::Mat& input, const QVariantMap& params);
    cv::Mat applyBilateral(const cv::Mat& input, const QVariantMap& params);
    cv::Mat applySobel(const cv::Mat& input, const QVariantMap& params);
    cv::Mat applyCanny(const cv::Mat& input, const QVariantMap& params);
    cv::Mat applyMorphology(const cv::Mat& input, const QVariantMap& params);
    cv::Mat applyHistogramEqualization(const cv::Mat& input, const QVariantMap& params);
    
    // GPU implementations
    cv::Mat applyGaussianGPU(const cv::Mat& input, const QVariantMap& params);
    cv::Mat applySobelGPU(const cv::Mat& input, const QVariantMap& params);
    
    // Members
    std::unique_ptr<ProcessingPipeline> m_pipeline;
    std::unique_ptr<BatchProcessor> m_batchProcessor;
    std::atomic<bool> m_gpuEnabled{false};
    std::atomic<int> m_lastProcessingTime{0};
    
    // Worker thread
    void processingWorker();
    std::thread m_workerThread;
    std::queue<std::pair<cv::Mat, qint64>> m_inputQueue;
    QMutex m_queueMutex;
    QWaitCondition m_queueCondition;
    std::atomic<bool> m_stopWorker{false};
};
```

### 實作範例

```cpp
// ImageProcessingComponent.cpp
#include "ImageProcessingComponent.h"
#include <QElapsedTimer>
#include <opencv2/imgproc.hpp>
#ifdef WITH_CUDA
#include <opencv2/cudafilters.hpp>
#include <opencv2/cudaimgproc.hpp>
#endif

ImageProcessingComponent::ImageProcessingComponent(QObject* parent)
    : BaseComponent(parent)
    , m_pipeline(std::make_unique<ProcessingPipeline>())
    , m_batchProcessor(std::make_unique<BatchProcessor>())
{
    setObjectName("ImageProcessingComponent");
}

bool ImageProcessingComponent::initialize(const Configuration& config) {
    if (!BaseComponent::initialize(config)) {
        return false;
    }
    
    // Load processing configuration
    auto processingConfig = config.value("processing").toObject();
    
    // Setup default pipeline
    auto filters = processingConfig["filters"].toArray();
    for (const auto& filterValue : filters) {
        auto filterObj = filterValue.toObject();
        FilterType type = static_cast<FilterType>(
            filterObj["type"].toInt());
        QVariantMap params = filterObj["parameters"].toObject().toVariantMap();
        addFilter(type, params);
    }
    
    // Check GPU availability
    #ifdef WITH_CUDA
    int deviceCount = cv::cuda::getCudaEnabledDeviceCount();
    if (deviceCount > 0) {
        setGpuEnabled(processingConfig["gpuEnabled"].toBool(true));
        qDebug() << "GPU acceleration available with" << deviceCount << "devices";
    }
    #endif
    
    return true;
}

bool ImageProcessingComponent::start() {
    if (!BaseComponent::start()) {
        return false;
    }
    
    m_stopWorker = false;
    m_workerThread = std::thread(&ImageProcessingComponent::processingWorker, this);
    
    return true;
}

void ImageProcessingComponent::processImage(const QImage& image, qint64 timestamp) {
    // Convert QImage to cv::Mat
    cv::Mat mat;
    switch (image.format()) {
        case QImage::Format_RGB888:
            mat = cv::Mat(image.height(), image.width(), CV_8UC3,
                         const_cast<uchar*>(image.bits()), image.bytesPerLine());
            cv::cvtColor(mat, mat, cv::COLOR_RGB2BGR);
            break;
        case QImage::Format_Grayscale8:
            mat = cv::Mat(image.height(), image.width(), CV_8UC1,
                         const_cast<uchar*>(image.bits()), image.bytesPerLine());
            break;
        default:
            emit processingError("Unsupported image format");
            return;
    }
    
    // Add to processing queue
    {
        QMutexLocker locker(&m_queueMutex);
        m_inputQueue.push({mat.clone(), timestamp});
    }
    m_queueCondition.wakeOne();
}

void ImageProcessingComponent::processingWorker() {
    while (!m_stopWorker) {
        cv::Mat inputImage;
        qint64 timestamp;
        
        // Get image from queue
        {
            QMutexLocker locker(&m_queueMutex);
            if (m_inputQueue.empty()) {
                m_queueCondition.wait(&m_queueMutex, 100);
                continue;
            }
            
            auto pair = m_inputQueue.front();
            m_inputQueue.pop();
            inputImage = pair.first;
            timestamp = pair.second;
        }
        
        // Process image
        QElapsedTimer timer;
        timer.start();
        
        try {
            cv::Mat result = m_pipeline->process(inputImage);
            
            // Convert back to QImage
            QImage processedImage;
            if (result.channels() == 1) {
                processedImage = QImage(result.data, result.cols, result.rows,
                                       result.step, QImage::Format_Grayscale8).copy();
            } else if (result.channels() == 3) {
                cv::cvtColor(result, result, cv::COLOR_BGR2RGB);
                processedImage = QImage(result.data, result.cols, result.rows,
                                       result.step, QImage::Format_RGB888).copy();
            }
            
            m_lastProcessingTime = timer.elapsed();
            emit processingTimeChanged(m_lastProcessingTime);
            emit imageProcessed(processedImage, timestamp);
            
        } catch (const cv::Exception& e) {
            emit processingError(QString::fromStdString(e.what()));
        }
    }
}

cv::Mat ImageProcessingComponent::applyGaussian(const cv::Mat& input, 
                                                const QVariantMap& params) {
    cv::Mat output;
    int kernelSize = params.value("kernelSize", 5).toInt();
    double sigmaX = params.value("sigmaX", 1.0).toDouble();
    double sigmaY = params.value("sigmaY", sigmaX).toDouble();
    
    if (m_gpuEnabled) {
        #ifdef WITH_CUDA
        return applyGaussianGPU(input, params);
        #endif
    }
    
    cv::GaussianBlur(input, output, cv::Size(kernelSize, kernelSize), 
                    sigmaX, sigmaY);
    return output;
}

cv::Mat ImageProcessingComponent::applyCanny(const cv::Mat& input,
                                            const QVariantMap& params) {
    cv::Mat output, gray;
    double threshold1 = params.value("threshold1", 50.0).toDouble();
    double threshold2 = params.value("threshold2", 150.0).toDouble();
    
    // Convert to grayscale if needed
    if (input.channels() == 3) {
        cv::cvtColor(input, gray, cv::COLOR_BGR2GRAY);
    } else {
        gray = input;
    }
    
    cv::Canny(gray, output, threshold1, threshold2);
    return output;
}

#ifdef WITH_CUDA
cv::Mat ImageProcessingComponent::applyGaussianGPU(const cv::Mat& input,
                                                   const QVariantMap& params) {
    cv::cuda::GpuMat gpuInput, gpuOutput;
    gpuInput.upload(input);
    
    int kernelSize = params.value("kernelSize", 5).toInt();
    double sigmaX = params.value("sigmaX", 1.0).toDouble();
    double sigmaY = params.value("sigmaY", sigmaX).toDouble();
    
    auto filter = cv::cuda::createGaussianFilter(
        input.type(), input.type(),
        cv::Size(kernelSize, kernelSize),
        sigmaX, sigmaY);
    
    filter->apply(gpuInput, gpuOutput);
    
    cv::Mat output;
    gpuOutput.download(output);
    return output;
}
#endif

void ImageProcessingComponent::addFilter(FilterType type, 
                                        const QVariantMap& parameters) {
    ProcessingPipeline::Stage stage;
    stage.type = type;
    stage.parameters = parameters;
    
    // Bind appropriate processor
    switch (type) {
        case FilterType::Gaussian:
            stage.processor = [this](const cv::Mat& input, const QVariantMap& params) {
                return applyGaussian(input, params);
            };
            break;
        case FilterType::Canny:
            stage.processor = [this](const cv::Mat& input, const QVariantMap& params) {
                return applyCanny(input, params);
            };
            break;
        // Add other filter bindings...
    }
    
    m_pipeline->addStage(stage);
    emit pipelineUpdated();
}
```

---

## 演算法元件 (Algorithm Component)

### 元件介面定義

```cpp
// AlgorithmComponent.h
#pragma once

#include "BaseComponent.h"
#include <opencv2/opencv.hpp>
#include <onnxruntime_cxx_api.h>

class AlgorithmComponent : public BaseComponent {
    Q_OBJECT
    Q_PROPERTY(QString currentAlgorithm READ currentAlgorithm NOTIFY algorithmChanged)
    Q_PROPERTY(double confidence READ confidence NOTIFY confidenceChanged)

public:
    struct DetectionResult {
        QRectF boundingBox;
        QString className;
        double confidence;
        QVariantMap metadata;
    };
    
    struct MeasurementResult {
        QString measurementType;
        double value;
        QString unit;
        double tolerance;
        bool inSpec;
    };

    explicit AlgorithmComponent(QObject* parent = nullptr);
    ~AlgorithmComponent() override;

    // Algorithm types
    enum class AlgorithmType {
        TemplateMatching,
        BlobAnalysis,
        EdgeDetection,
        ContourAnalysis,
        FeatureMatching,
        DeepLearning,
        Measurement,
        Custom
    };
    Q_ENUM(AlgorithmType)

    // Template matching methods
    cv::Mat performTemplateMatching(const cv::Mat& image, const cv::Mat& templ,
                                   int method = cv::TM_CCOEFF_NORMED);
    
    // Blob analysis
    std::vector<cv::KeyPoint> detectBlobs(const cv::Mat& image,
                                         const cv::SimpleBlobDetector::Params& params);
    
    // Measurement algorithms
    double measureDistance(const cv::Point2f& p1, const cv::Point2f& p2,
                          double pixelSize = 1.0);
    double measureAngle(const cv::Point2f& p1, const cv::Point2f& p2,
                       const cv::Point2f& p3);
    double measureCircularity(const std::vector<cv::Point>& contour);
    
    // Deep learning inference
    bool loadONNXModel(const QString& modelPath);
    std::vector<DetectionResult> runInference(const cv::Mat& image);

    QString currentAlgorithm() const { return m_currentAlgorithm; }
    double confidence() const { return m_confidence; }

signals:
    void detectionComplete(const QList<DetectionResult>& results);
    void measurementComplete(const QList<MeasurementResult>& results);
    void algorithmChanged(const QString& algorithm);
    void confidenceChanged(double confidence);
    void processingComplete(const QVariantMap& results);

public slots:
    void processImage(const cv::Mat& image);
    void setAlgorithm(AlgorithmType type, const QVariantMap& parameters);
    void setROI(const QRectF& roi);
    void calibrate(double pixelSizeX, double pixelSizeY);

private:
    // Sub-pixel accuracy refinement
    cv::Point2f refineCorner(const cv::Mat& image, const cv::Point2f& corner);
    std::vector<cv::Point2f> refineEdgePoints(const cv::Mat& image,
                                             const std::vector<cv::Point>& edge);
    
    // Algorithm chain management
    class AlgorithmChain {
    public:
        struct Node {
            AlgorithmType type;
            QVariantMap parameters;
            std::function<QVariant(const cv::Mat&, const QVariant&)> processor;
        };
        
        void addNode(const Node& node);
        QVariant execute(const cv::Mat& image);
        
    private:
        std::vector<Node> m_nodes;
    };
    
    // ONNX Runtime for deep learning
    std::unique_ptr<Ort::Session> m_ortSession;
    std::unique_ptr<Ort::Env> m_ortEnv;
    std::vector<const char*> m_inputNames;
    std::vector<const char*> m_outputNames;
    
    // Members
    QString m_currentAlgorithm;
    std::atomic<double> m_confidence{0.0};
    QRectF m_roi;
    double m_pixelSizeX = 1.0;
    double m_pixelSizeY = 1.0;
    std::unique_ptr<AlgorithmChain> m_chain;
};
```

---

## 運動控制元件 (Motion Control Component)

### 元件介面定義

```cpp
// MotionControlComponent.h
#pragma once

#include "BaseComponent.h"
#include <QPointF>
#include <QVector3D>

class MotionControlComponent : public BaseComponent {
    Q_OBJECT
    Q_PROPERTY(QVector3D currentPosition READ currentPosition NOTIFY positionChanged)
    Q_PROPERTY(QVector3D targetPosition READ targetPosition NOTIFY targetPositionChanged)
    Q_PROPERTY(bool isMoving READ isMoving NOTIFY movingStateChanged)
    Q_PROPERTY(bool isHomed READ isHomed NOTIFY homedStateChanged)

public:
    enum class AxisType {
        Linear,
        Rotary
    };
    Q_ENUM(AxisType)
    
    enum class MotionMode {
        Absolute,
        Relative,
        Jog,
        Continuous
    };
    Q_ENUM(MotionMode)
    
    enum class MotionProfile {
        Trapezoidal,
        SCurve,
        Electronic Gear
    };
    Q_ENUM(MotionProfile)

    struct AxisConfig {
        QString name;
        AxisType type;
        double maxVelocity;
        double maxAcceleration;
        double softLimitMin;
        double softLimitMax;
        double homeOffset;
        int encoderResolution;
    };

    struct MotionPath {
        std::vector<QVector3D> waypoints;
        std::vector<double> velocities;
        MotionProfile profile;
        bool blendedMove;
    };

    explicit MotionControlComponent(QObject* parent = nullptr);
    ~MotionControlComponent() override;

    // BaseComponent interface
    bool initialize(const Configuration& config) override;
    bool start() override;
    bool stop() override;

    // Axis management
    bool addAxis(const QString& name, const AxisConfig& config);
    bool removeAxis(const QString& name);
    bool configureAxis(const QString& name, const AxisConfig& config);
    
    // Motion commands
    bool moveAbsolute(const QVector3D& position, double velocity);
    bool moveRelative(const QVector3D& distance, double velocity);
    bool movePath(const MotionPath& path);
    bool jog(const QString& axis, double velocity);
    bool stopMotion(bool emergency = false);
    
    // Homing
    bool homeAxis(const QString& axis);
    bool homeAllAxes();
    
    // Synchronization
    bool setSyncGroup(const QStringList& axes);
    bool startSyncMotion();
    bool triggerCamera(int cameraId, double position);
    
    // Position feedback
    QVector3D currentPosition() const { return m_currentPosition; }
    QVector3D targetPosition() const { return m_targetPosition; }
    bool isMoving() const { return m_isMoving; }
    bool isHomed() const { return m_isHomed; }

signals:
    void positionChanged(const QVector3D& position);
    void targetPositionChanged(const QVector3D& target);
    void movingStateChanged(bool moving);
    void homedStateChanged(bool homed);
    void motionComplete();
    void motionError(const QString& error);
    void limitSwitchTriggered(const QString& axis, bool positive);
    void followingErrorExceeded(const QString& axis, double error);

public slots:
    void setVelocityOverride(double percentage);
    void enableAxis(const QString& axis, bool enable);
    void resetErrors();
    void updatePosition();

private:
    // Motion controller interface
    class MotionController {
    public:
        virtual ~MotionController() = default;
        virtual bool connect(const QString& address) = 0;
        virtual bool disconnect() = 0;
        virtual bool moveAxis(int axisId, double position, double velocity) = 0;
        virtual double readPosition(int axisId) = 0;
        virtual bool isInMotion(int axisId) = 0;
    };
    
    // EtherCAT implementation
    class EtherCATController : public MotionController {
        // Implementation specific to EtherCAT
    };
    
    // Path planning
    class PathPlanner {
    public:
        std::vector<QVector3D> interpolatePath(const MotionPath& path,
                                              double resolution = 0.1);
        std::vector<double> calculateVelocityProfile(const MotionPath& path);
        bool checkCollision(const QVector3D& position);
    };
    
    // Safety interlock
    class SafetyInterlock {
    public:
        bool checkMotionAllowed(const QVector3D& target);
        void registerInterlock(const QString& name,
                              std::function<bool()> condition);
        void emergency Stop();
        
    private:
        std::map<QString, std::function<bool()>> m_interlocks;
    };
    
    // Members
    std::unique_ptr<MotionController> m_controller;
    std::unique_ptr<PathPlanner> m_pathPlanner;
    std::unique_ptr<SafetyInterlock> m_safety;
    
    QMap<QString, AxisConfig> m_axes;
    QVector3D m_currentPosition;
    QVector3D m_targetPosition;
    std::atomic<bool> m_isMoving{false};
    std::atomic<bool> m_isHomed{false};
    
    // Position update thread
    void positionUpdateThread();
    std::thread m_updateThread;
    std::atomic<bool> m_stopThread{false};
};
```

---

## 資料儲存元件 (Data Storage Component)

### 元件介面定義

```cpp
// DataStorageComponent.h
#pragma once

#include "BaseComponent.h"
#include <QSqlDatabase>
#include <hdf5.h>

class DataStorageComponent : public BaseComponent {
    Q_OBJECT
    Q_PROPERTY(qint64 storageUsed READ storageUsed NOTIFY storageUsedChanged)
    Q_PROPERTY(qint64 storageLimit READ storageLimit WRITE setStorageLimit)
    Q_PROPERTY(bool isRecording READ isRecording NOTIFY recordingStateChanged)

public:
    enum class StorageFormat {
        HDF5,
        SQLite,
        PostgreSQL,
        Binary,
        CSV,
        JSON
    };
    Q_ENUM(StorageFormat)
    
    enum class CompressionType {
        None,
        LZ4,
        ZSTD,
        Gzip
    };
    Q_ENUM(CompressionType)

    struct StorageConfig {
        StorageFormat format;
        QString path;
        CompressionType compression;
        int compressionLevel;
        qint64 maxFileSize;
        int retentionDays;
        bool enableBuffering;
        int bufferSize;
    };

    struct ImageData {
        QByteArray data;
        int width;
        int height;
        int channels;
        QString format;
        qint64 timestamp;
        QVariantMap metadata;
    };

    struct ResultData {
        QString componentId;
        QString resultType;
        QVariant value;
        qint64 timestamp;
        QVariantMap metadata;
    };

    explicit DataStorageComponent(QObject* parent = nullptr);
    ~DataStorageComponent() override;

    // BaseComponent interface
    bool initialize(const Configuration& config) override;
    bool start() override;
    bool stop() override;

    // Storage operations
    bool createDataset(const QString& name, const StorageConfig& config);
    bool openDataset(const QString& name);
    bool closeDataset(const QString& name);
    
    // Data writing
    bool storeImage(const QString& dataset, const ImageData& image);
    bool storeResult(const QString& dataset, const ResultData& result);
    bool storeBatch(const QString& dataset, const QList<QVariant>& batch);
    
    // Data reading
    ImageData readImage(const QString& dataset, qint64 timestamp);
    QList<ResultData> readResults(const QString& dataset,
                                 qint64 startTime, qint64 endTime);
    QVariantMap queryData(const QString& query);
    
    // Recording control
    bool startRecording(const QString& sessionName);
    bool stopRecording();
    bool pauseRecording();
    bool resumeRecording();
    
    // Storage management
    qint64 storageUsed() const { return m_storageUsed; }
    qint64 storageLimit() const { return m_storageLimit; }
    void setStorageLimit(qint64 bytes);
    bool isRecording() const { return m_isRecording; }
    
    // Backup and restore
    bool backupDataset(const QString& dataset, const QString& backupPath);
    bool restoreDataset(const QString& backupPath, const QString& dataset);
    bool exportData(const QString& dataset, StorageFormat format,
                   const QString& exportPath);

signals:
    void storageUsedChanged(qint64 bytes);
    void recordingStateChanged(bool recording);
    void dataStored(const QString& dataset, qint64 recordId);
    void storageError(const QString& error);
    void storageLimitReached();
    void backupComplete(const QString& path);

public slots:
    void flushBuffers();
    void optimizeStorage();
    void cleanupOldData();

private:
    // Ring buffer for real-time data
    template<typename T>
    class RingBuffer {
    public:
        explicit RingBuffer(size_t size);
        bool push(const T& item);
        bool pop(T& item);
        size_t size() const;
        bool isEmpty() const;
        bool isFull() const;
        
    private:
        std::vector<T> m_buffer;
        size_t m_head = 0;
        size_t m_tail = 0;
        size_t m_size = 0;
        mutable QMutex m_mutex;
    };
    
    // HDF5 handler
    class HDF5Handler {
    public:
        bool createFile(const QString& path);
        bool openFile(const QString& path);
        bool closeFile();
        bool writeImage(const ImageData& image);
        bool writeDataset(const QString& name, const QVariant& data);
        QVariant readDataset(const QString& name);
        
    private:
        hid_t m_fileId = -1;
        QMutex m_mutex;
    };
    
    // Database handler
    class DatabaseHandler {
    public:
        bool connect(const QString& connectionString);
        bool disconnect();
        bool executeQuery(const QString& query);
        QVariantList select(const QString& query);
        bool beginTransaction();
        bool commitTransaction();
        bool rollbackTransaction();
        
    private:
        QSqlDatabase m_database;
        QMutex m_mutex;
    };
    
    // Compression handler
    class CompressionHandler {
    public:
        QByteArray compress(const QByteArray& data, CompressionType type,
                          int level = -1);
        QByteArray decompress(const QByteArray& data, CompressionType type);
    };
    
    // Members
    std::unique_ptr<HDF5Handler> m_hdf5Handler;
    std::unique_ptr<DatabaseHandler> m_dbHandler;
    std::unique_ptr<CompressionHandler> m_compressor;
    std::unique_ptr<RingBuffer<ImageData>> m_imageBuffer;
    std::unique_ptr<RingBuffer<ResultData>> m_resultBuffer;
    
    QMap<QString, StorageConfig> m_datasets;
    std::atomic<qint64> m_storageUsed{0};
    std::atomic<qint64> m_storageLimit{1000000000}; // 1GB default
    std::atomic<bool> m_isRecording{false};
    QString m_currentSession;
    
    // Worker thread for async writes
    void storageWorker();
    std::thread m_workerThread;
    std::atomic<bool> m_stopWorker{false};
    QWaitCondition m_workerCondition;
};
```

### Control Panel 設計範例 (Widget)

```cpp
// DataStorageControlPanel.h
#pragma once

#include <QWidget>
#include <QChartView>

QT_CHARTS_USE_NAMESPACE

class DataStorageComponent;

class DataStorageControlPanel : public QWidget {
    Q_OBJECT

public:
    explicit DataStorageControlPanel(DataStorageComponent* component,
                                    QWidget* parent = nullptr);

private slots:
    void updateStorageChart();
    void onStartRecording();
    void onStopRecording();
    void onExportData();
    void onCleanupOldData();

private:
    void setupUi();
    void createStorageChart();
    
    DataStorageComponent* m_component;
    
    // UI elements
    QPushButton* m_recordButton;
    QLineEdit* m_sessionNameEdit;
    QProgressBar* m_storageBar;
    QLabel* m_storageLabel;
    QChartView* m_chartView;
    QChart* m_storageChart;
    QComboBox* m_formatCombo;
    QSpinBox* m_retentionSpinBox;
    QCheckBox* m_compressionCheck;
    QTableWidget* m_datasetTable;
};
```

```cpp
// DataStorageControlPanel.cpp
#include "DataStorageControlPanel.h"
#include "DataStorageComponent.h"
#include <QtWidgets>
#include <QtCharts>

DataStorageControlPanel::DataStorageControlPanel(DataStorageComponent* component,
                                               QWidget* parent)
    : QWidget(parent)
    , m_component(component)
{
    setupUi();
    
    // Connect signals
    connect(m_component, &DataStorageComponent::storageUsedChanged,
            this, &DataStorageControlPanel::updateStorageChart);
    connect(m_component, &DataStorageComponent::recordingStateChanged,
            [this](bool recording) {
        m_recordButton->setText(recording ? "Stop Recording" : "Start Recording");
        m_sessionNameEdit->setEnabled(!recording);
    });
}

void DataStorageControlPanel::setupUi() {
    auto* layout = new QVBoxLayout(this);
    
    // Recording controls
    auto* recordingGroup = new QGroupBox("Recording");
    auto* recordingLayout = new QHBoxLayout(recordingGroup);
    
    m_sessionNameEdit = new QLineEdit;
    m_sessionNameEdit->setPlaceholderText("Session Name");
    recordingLayout->addWidget(m_sessionNameEdit);
    
    m_recordButton = new QPushButton("Start Recording");
    connect(m_recordButton, &QPushButton::clicked,
            this, &DataStorageControlPanel::onStartRecording);
    recordingLayout->addWidget(m_recordButton);
    
    layout->addWidget(recordingGroup);
    
    // Storage status
    auto* statusGroup = new QGroupBox("Storage Status");
    auto* statusLayout = new QVBoxLayout(statusGroup);
    
    m_storageBar = new QProgressBar;
    m_storageBar->setMaximum(100);
    statusLayout->addWidget(m_storageBar);
    
    m_storageLabel = new QLabel("0 MB / 1000 MB");
    statusLayout->addWidget(m_storageLabel);
    
    layout->addWidget(statusGroup);
    
    // Storage chart
    createStorageChart();
    m_chartView = new QChartView(m_storageChart);
    m_chartView->setRenderHint(QPainter::Antialiasing);
    layout->addWidget(m_chartView);
    
    // Configuration
    auto* configGroup = new QGroupBox("Configuration");
    auto* configLayout = new QFormLayout(configGroup);
    
    m_formatCombo = new QComboBox;
    m_formatCombo->addItems({"HDF5", "SQLite", "PostgreSQL", "Binary", "CSV"});
    configLayout->addRow("Format:", m_formatCombo);
    
    m_retentionSpinBox = new QSpinBox;
    m_retentionSpinBox->setRange(1, 365);
    m_retentionSpinBox->setValue(30);
    m_retentionSpinBox->setSuffix(" days");
    configLayout->addRow("Retention:", m_retentionSpinBox);
    
    m_compressionCheck = new QCheckBox("Enable Compression");
    configLayout->addRow(m_compressionCheck);
    
    layout->addWidget(configGroup);
    
    // Dataset table
    m_datasetTable = new QTableWidget;
    m_datasetTable->setColumnCount(4);
    m_datasetTable->setHorizontalHeaderLabels(
        {"Dataset", "Size", "Records", "Last Modified"});
    layout->addWidget(m_datasetTable);
    
    // Action buttons
    auto* buttonLayout = new QHBoxLayout;
    
    auto* exportButton = new QPushButton("Export Data");
    connect(exportButton, &QPushButton::clicked,
            this, &DataStorageControlPanel::onExportData);
    buttonLayout->addWidget(exportButton);
    
    auto* cleanupButton = new QPushButton("Cleanup Old Data");
    connect(cleanupButton, &QPushButton::clicked,
            this, &DataStorageControlPanel::onCleanupOldData);
    buttonLayout->addWidget(cleanupButton);
    
    buttonLayout->addStretch();
    layout->addLayout(buttonLayout);
}

void DataStorageControlPanel::createStorageChart() {
    m_storageChart = new QChart;
    m_storageChart->setTitle("Storage Usage Over Time");
    
    auto* series = new QLineSeries;
    series->setName("Storage Used");
    
    // Add sample data
    for (int i = 0; i < 24; ++i) {
        series->append(i, qrand() % 100);
    }
    
    m_storageChart->addSeries(series);
    m_storageChart->createDefaultAxes();
    m_storageChart->legend()->setAlignment(Qt::AlignBottom);
}

void DataStorageControlPanel::onStartRecording() {
    if (m_component->isRecording()) {
        m_component->stopRecording();
    } else {
        QString sessionName = m_sessionNameEdit->text();
        if (sessionName.isEmpty()) {
            sessionName = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
        }
        m_component->startRecording(sessionName);
    }
}

void DataStorageControlPanel::updateStorageChart() {
    qint64 used = m_component->storageUsed();
    qint64 limit = m_component->storageLimit();
    
    int percentage = (used * 100) / limit;
    m_storageBar->setValue(percentage);
    
    QString usedStr = QString::number(used / (1024.0 * 1024.0), 'f', 2);
    QString limitStr = QString::number(limit / (1024.0 * 1024.0), 'f', 2);
    m_storageLabel->setText(QString("%1 MB / %2 MB").arg(usedStr).arg(limitStr));
}
```

---

## 單元測試範例

```cpp
// test_camera_component.cpp
#include <QtTest>
#include "CameraComponent.h"

class TestCameraComponent : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    
    void testInitialization();
    void testConnection();
    void testAcquisition();
    void testParameterChange();
    void testSignalEmission();
    
private:
    CameraComponent* m_camera;
};

void TestCameraComponent::initTestCase() {
    m_camera = new CameraComponent;
    
    Configuration config;
    config.insert("camera", QJsonObject{
        {"type", "GigE"},
        {"exposureTime", 10000},
        {"gain", 1.0},
        {"frameRate", 30.0}
    });
    
    QVERIFY(m_camera->initialize(config));
}

void TestCameraComponent::testAcquisition() {
    QSignalSpy spy(m_camera, &CameraComponent::imageAcquired);
    
    QVERIFY(m_camera->startAcquisition());
    QVERIFY(m_camera->isAcquiring());
    
    // Wait for at least one image
    QVERIFY(spy.wait(1000));
    QVERIFY(spy.count() > 0);
    
    QVERIFY(m_camera->stopAcquisition());
    QVERIFY(!m_camera->isAcquiring());
}

void TestCameraComponent::testParameterChange() {
    QSignalSpy exposureSpy(m_camera, &CameraComponent::exposureTimeChanged);
    QSignalSpy gainSpy(m_camera, &CameraComponent::gainChanged);
    
    m_camera->setExposureTime(20000);
    QCOMPARE(m_camera->exposureTime(), 20000);
    QCOMPARE(exposureSpy.count(), 1);
    
    m_camera->setGain(2.0);
    QCOMPARE(m_camera->gain(), 2.0);
    QCOMPARE(gainSpy.count(), 1);
}

QTEST_MAIN(TestCameraComponent)
#include "test_camera_component.moc"
```

---

## 最佳實踐總結

### 1. 元件設計原則
- **單一職責**: 每個元件專注於一個功能領域
- **介面隔離**: 提供清晰、最小化的公共介面
- **依賴倒置**: 依賴抽象而非具體實現
- **開閉原則**: 對擴展開放，對修改關閉

### 2. Signal/Slot 使用規範
- 使用描述性的信號名稱
- 避免在信號處理中執行耗時操作
- 優先使用 Qt::QueuedConnection 進行跨執行緒通訊
- 注意信號參數的生命週期管理

### 3. 執行緒管理
- 每個元件使用獨立的 QThread
- 使用 moveToThread 而非繼承 QThread
- 確保執行緒安全的資料訪問
- 實現優雅的執行緒停止機制

### 4. 錯誤處理
- 使用異常處理關鍵錯誤
- 通過信號報告非關鍵錯誤
- 提供詳細的錯誤信息
- 實現錯誤恢復機制

### 5. 效能優化
- 使用物件池減少記憶體分配
- 實現批次處理減少開銷
- 利用 GPU 加速計算密集型任務
- 使用 Zero-copy 技術優化資料傳輸

### 6. 測試策略
- 為每個元件編寫單元測試
- 測試信號發射和槽函數調用
- 驗證錯誤處理路徑
- 進行效能基準測試

---

## 配置文件範例

```json
{
  "components": {
    "camera": {
      "type": "GigE",
      "serialNumber": "12345678",
      "exposureTime": 10000,
      "gain": 1.0,
      "frameRate": 30.0,
      "pixelFormat": "Mono8",
      "triggerMode": "FreeRun"
    },
    "imageProcessing": {
      "gpuEnabled": true,
      "batchSize": 10,
      "filters": [
        {
          "type": "Gaussian",
          "parameters": {
            "kernelSize": 5,
            "sigmaX": 1.0
          }
        },
        {
          "type": "Canny",
          "parameters": {
            "threshold1": 50,
            "threshold2": 150
          }
        }
      ]
    },
    "algorithm": {
      "type": "TemplateMatching",
      "modelPath": "/models/defect_detection.onnx",
      "confidenceThreshold": 0.8,
      "pixelSizeX": 0.01,
      "pixelSizeY": 0.01
    },
    "motion": {
      "controller": "EtherCAT",
      "address": "192.168.1.100",
      "axes": [
        {
          "name": "X",
          "type": "Linear",
          "maxVelocity": 100.0,
          "maxAcceleration": 1000.0,
          "softLimitMin": 0.0,
          "softLimitMax": 500.0
        },
        {
          "name": "Y",
          "type": "Linear",
          "maxVelocity": 100.0,
          "maxAcceleration": 1000.0,
          "softLimitMin": 0.0,
          "softLimitMax": 300.0
        }
      ]
    },
    "storage": {
      "format": "HDF5",
      "path": "/data/aoi_data",
      "compression": "LZ4",
      "compressionLevel": 3,
      "maxFileSize": 1073741824,
      "retentionDays": 30,
      "enableBuffering": true,
      "bufferSize": 100
    }
  }
}
```

---

## 使用範例

```cpp
// main.cpp - 整合所有元件
#include <QCoreApplication>
#include "CameraComponent.h"
#include "ImageProcessingComponent.h"
#include "AlgorithmComponent.h"
#include "MotionControlComponent.h"
#include "DataStorageComponent.h"

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    
    // Load configuration
    QFile configFile("config.json");
    configFile.open(QIODevice::ReadOnly);
    QJsonDocument doc = QJsonDocument::fromJson(configFile.readAll());
    Configuration config = doc.object().toVariantMap();
    
    // Create components
    CameraComponent camera;
    ImageProcessingComponent processing;
    AlgorithmComponent algorithm;
    MotionControlComponent motion;
    DataStorageComponent storage;
    
    // Initialize components
    camera.initialize(config["camera"].toMap());
    processing.initialize(config["imageProcessing"].toMap());
    algorithm.initialize(config["algorithm"].toMap());
    motion.initialize(config["motion"].toMap());
    storage.initialize(config["storage"].toMap());
    
    // Connect components via signals/slots
    QObject::connect(&camera, &CameraComponent::imageAcquired,
                    &processing, &ImageProcessingComponent::processImage);
    
    QObject::connect(&processing, &ImageProcessingComponent::imageProcessed,
                    [&algorithm](const QImage& image, qint64 timestamp) {
        cv::Mat mat = convertQImageToMat(image);
        algorithm.processImage(mat);
    });
    
    QObject::connect(&algorithm, &AlgorithmComponent::detectionComplete,
                    [&storage](const QList<DetectionResult>& results) {
        for (const auto& result : results) {
            ResultData data;
            data.componentId = "algorithm";
            data.resultType = "detection";
            data.value = QVariant::fromValue(result);
            data.timestamp = QDateTime::currentMSecsSinceEpoch();
            storage.storeResult("detections", data);
        }
    });
    
    // Start components
    camera.start();
    processing.start();
    algorithm.start();
    motion.start();
    storage.start();
    
    // Start acquisition
    camera.connectCamera("12345678");
    camera.startAcquisition();
    storage.startRecording("session_001");
    
    return app.exec();
}
```

這份文檔提供了完整的五大標準元件開發規範，每個元件都包含了詳細的介面定義、實作範例、Control Panel設計以及測試案例。所有元件都遵循統一的架構標準，使用Signal/Slot進行通訊，並針對工業AOI應用進行了優化。