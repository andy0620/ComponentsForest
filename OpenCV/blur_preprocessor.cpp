#include "blur_preprocessor.h"
#include <QDebug>

// Only include CUDA headers if available
#ifdef HAVE_OPENCV_CUDA
#include <opencv2/cudafilters.hpp>
#endif

namespace ComponentsForest {
namespace OpenCV {

BlurPreProcessor::BlurPreProcessor(QObject* parent)
    : PreProcessorBase("GaussianBlur", parent)
    , m_kernelWidth(5)
    , m_kernelHeight(5)
    , m_sigmaX(1.0)
    , m_sigmaY(1.0)
    , m_borderType(cv::BORDER_DEFAULT)
    , m_useGPU(false)
    , m_useSeparableFilter(true)
    , m_gpuFilter(nullptr) {
    
    // Register configurable parameters
    registerParameter("kernelWidth", QVariant(m_kernelWidth),
                     "Width of the Gaussian kernel (must be positive and odd)");
    registerParameter("kernelHeight", QVariant(m_kernelHeight),
                     "Height of the Gaussian kernel (must be positive and odd)");
    registerParameter("sigmaX", QVariant(m_sigmaX),
                     "Gaussian kernel standard deviation in X direction");
    registerParameter("sigmaY", QVariant(m_sigmaY),
                     "Gaussian kernel standard deviation in Y direction");
    registerParameter("borderType", QVariant(QString("default")),
                     "Pixel extrapolation method (default, constant, reflect, replicate)");
    registerParameter("useGPU", QVariant(m_useGPU),
                     "Use GPU acceleration if available");
    registerParameter("useSeparableFilter", QVariant(m_useSeparableFilter),
                     "Use separable filter for better performance");
    
    // Check GPU availability
#ifdef HAVE_OPENCV_CUDA
    if (cv::cuda::getCudaEnabledDeviceCount() > 0) {
        logInfo("GPU acceleration available for GaussianBlur");
        initializeGPUResources();
    }
#endif
}

cv::Mat BlurPreProcessor::processImplementation(const cv::Mat& input) {
    // Choose processing path based on configuration and availability
    if (m_useGPU && m_gpuFilter) {
        return processOnGPU(input);
    } else {
        return processOnCPU(input);
    }
}

cv::Mat BlurPreProcessor::processOnCPU(const cv::Mat& input) {
    cv::Mat output;
    
    try {
        // Apply Gaussian blur
        cv::GaussianBlur(input, output,
                        cv::Size(m_kernelWidth, m_kernelHeight),
                        m_sigmaX, m_sigmaY,
                        m_borderType);
        
        if (isDebugMode()) {
            logDebug(QString("GaussianBlur applied: kernel=%1x%2 sigma=%3,%4")
                    .arg(m_kernelWidth).arg(m_kernelHeight)
                    .arg(m_sigmaX).arg(m_sigmaY));
        }
        
    } catch (const cv::Exception& e) {
        QString error = QString("OpenCV error in GaussianBlur: %1").arg(e.what());
        setProcessingError(error);
        emit processingError(ProcessingError::AlgorithmError, error);
        return input.clone();  // Return unchanged on error
    }
    
    return output;
}

cv::Mat BlurPreProcessor::processOnGPU(const cv::Mat& input) {
#ifdef HAVE_OPENCV_CUDA
    if (!m_gpuFilter) {
        // Fallback to CPU if GPU not initialized
        return processOnCPU(input);
    }
    
    try {
        cv::cuda::GpuMat gpuInput, gpuOutput;
        
        // Upload to GPU
        gpuInput.upload(input);
        
        // Apply filter
        auto* filter = static_cast<cv::Ptr<cv::cuda::Filter>*>(m_gpuFilter);
        (*filter)->apply(gpuInput, gpuOutput);
        
        // Download result
        cv::Mat output;
        gpuOutput.download(output);
        
        if (isDebugMode()) {
            logDebug("GaussianBlur applied on GPU");
        }
        
        return output;
        
    } catch (const cv::Exception& e) {
        QString error = QString("GPU processing error: %1").arg(e.what());
        setProcessingError(error);
        logWarning("Falling back to CPU processing: " + error);
        return processOnCPU(input);
    }
#else
    return processOnCPU(input);
#endif
}

bool BlurPreProcessor::configurePreprocessorImplementation(const QVariantMap& config) {
    bool needsGPUReinitialization = false;
    
    // Update kernel size
    if (config.contains("kernelWidth")) {
        int width = config["kernelWidth"].toInt();
        if (width > 0 && width % 2 == 1) {  // Must be positive and odd
            m_kernelWidth = width;
            needsGPUReinitialization = true;
        } else {
            setProcessingError("Kernel width must be positive and odd");
            return false;
        }
    }
    
    if (config.contains("kernelHeight")) {
        int height = config["kernelHeight"].toInt();
        if (height > 0 && height % 2 == 1) {  // Must be positive and odd
            m_kernelHeight = height;
            needsGPUReinitialization = true;
        } else {
            setProcessingError("Kernel height must be positive and odd");
            return false;
        }
    }
    
    // Update sigma values
    if (config.contains("sigmaX")) {
        double sigma = config["sigmaX"].toDouble();
        if (sigma >= 0) {
            m_sigmaX = sigma;
            needsGPUReinitialization = true;
        } else {
            setProcessingError("Sigma X must be non-negative");
            return false;
        }
    }
    
    if (config.contains("sigmaY")) {
        double sigma = config["sigmaY"].toDouble();
        if (sigma >= 0) {
            m_sigmaY = sigma;
            needsGPUReinitialization = true;
        } else {
            setProcessingError("Sigma Y must be non-negative");
            return false;
        }
    }
    
    // Update border type
    if (config.contains("borderType")) {
        QString borderStr = config["borderType"].toString().toLower();
        if (borderStr == "constant") {
            m_borderType = cv::BORDER_CONSTANT;
        } else if (borderStr == "reflect") {
            m_borderType = cv::BORDER_REFLECT;
        } else if (borderStr == "replicate") {
            m_borderType = cv::BORDER_REPLICATE;
        } else if (borderStr == "wrap") {
            m_borderType = cv::BORDER_WRAP;
        } else {
            m_borderType = cv::BORDER_DEFAULT;
        }
        needsGPUReinitialization = true;
    }
    
    // Update GPU usage
    if (config.contains("useGPU")) {
        bool useGPU = config["useGPU"].toBool();
        if (useGPU != m_useGPU) {
            m_useGPU = useGPU;
            if (m_useGPU && !m_gpuFilter) {
                initializeGPUResources();
            }
        }
    }
    
    // Update separable filter flag
    if (config.contains("useSeparableFilter")) {
        m_useSeparableFilter = config["useSeparableFilter"].toBool();
        needsGPUReinitialization = true;
    }
    
    // Reinitialize GPU resources if parameters changed
    if (needsGPUReinitialization && m_useGPU) {
        releaseGPUResources();
        initializeGPUResources();
    }
    
    return true;
}

bool BlurPreProcessor::validateInput(const cv::Mat& input) {
    if (input.empty()) {
        setProcessingError("Input image is empty");
        return false;
    }
    
    // Check if image is too small for the kernel
    if (input.cols < m_kernelWidth || input.rows < m_kernelHeight) {
        QString error = QString("Image too small for kernel size %1x%2")
                       .arg(m_kernelWidth).arg(m_kernelHeight);
        setProcessingError(error);
        return false;
    }
    
    // Check supported data types
    int depth = input.depth();
    if (depth != CV_8U && depth != CV_16U && depth != CV_32F && depth != CV_64F) {
        setProcessingError("Unsupported image depth");
        return false;
    }
    
    return true;
}

void BlurPreProcessor::initializeGPUResources() {
#ifdef HAVE_OPENCV_CUDA
    if (cv::cuda::getCudaEnabledDeviceCount() > 0) {
        try {
            // Create Gaussian filter for GPU
            auto filter = cv::cuda::createGaussianFilter(
                CV_8UC3, CV_8UC3,  // Assuming 3-channel input/output
                cv::Size(m_kernelWidth, m_kernelHeight),
                m_sigmaX, m_sigmaY,
                m_borderType
            );
            
            m_gpuFilter = new cv::Ptr<cv::cuda::Filter>(filter);
            
            logInfo("GPU Gaussian filter initialized");
            
        } catch (const cv::Exception& e) {
            QString error = QString("Failed to initialize GPU resources: %1").arg(e.what());
            logWarning(error);
            m_gpuFilter = nullptr;
        }
    }
#endif
}

void BlurPreProcessor::releaseGPUResources() {
#ifdef HAVE_OPENCV_CUDA
    if (m_gpuFilter) {
        delete static_cast<cv::Ptr<cv::cuda::Filter>*>(m_gpuFilter);
        m_gpuFilter = nullptr;
    }
#endif
}

// BaseComponent lifecycle implementation

bool BlurPreProcessor::initializePreprocessorImplementation() {
    logInfo("Initializing Gaussian Blur preprocessor");
    
    // Initialize GPU resources if GPU is enabled
    if (isGPUEnabled()) {
        initializeGPUResources();
    }
    
    logInfo("Gaussian Blur preprocessor initialized successfully");
    return true;
}

void BlurPreProcessor::cleanupPreprocessorImplementation() {
    logInfo("Cleaning up Gaussian Blur preprocessor");
    
    // Release GPU resources
    releaseGPUResources();
    
    logInfo("Gaussian Blur preprocessor cleanup completed");
}

} // namespace OpenCV
} // namespace ComponentsForest