#include "denoise_preprocessor.h"
#include <QDebug>
#include <opencv2/photo.hpp>

namespace ComponentsForest {
namespace OpenCV {

DenoisePreProcessor::DenoisePreProcessor(QObject* parent)
    : PreProcessorBase("Denoise", parent)
    , m_algorithm(DenoiseAlgorithm::Bilateral)
    , m_bilateralD(9)
    , m_bilateralSigmaColor(75.0)
    , m_bilateralSigmaSpace(75.0)
    , m_nlMeansH(10.0f)
    , m_nlMeansHColor(10.0f)
    , m_nlMeansTemplateWindowSize(7)
    , m_nlMeansSearchWindowSize(21)
    , m_medianKernelSize(5)
    , m_morphKernelSize(3)
    , m_morphShape(cv::MORPH_RECT)
    , m_morphOperation(cv::MORPH_OPEN)
    , m_morphIterations(1)
    , m_useMultiStage(false)
    , m_adaptiveMode(false)
    , m_noiseEstimate(0.0)
    , m_morphKernelDirty(true) {
    
    // Register configurable parameters
    registerParameter("algorithm", QVariant("bilateral"),
                     "Denoising algorithm (bilateral, nlmeans, median, morphological, fastnlmeans)");
    
    // Bilateral filter parameters
    registerParameter("bilateralD", QVariant(m_bilateralD),
                     "Diameter of pixel neighborhood for bilateral filter");
    registerParameter("bilateralSigmaColor", QVariant(m_bilateralSigmaColor),
                     "Filter sigma in the color space for bilateral filter");
    registerParameter("bilateralSigmaSpace", QVariant(m_bilateralSigmaSpace),
                     "Filter sigma in the coordinate space for bilateral filter");
    
    // Non-local means parameters
    registerParameter("nlMeansH", QVariant(m_nlMeansH),
                     "Filter strength for luminance component");
    registerParameter("nlMeansHColor", QVariant(m_nlMeansHColor),
                     "Filter strength for color components");
    registerParameter("nlMeansTemplateWindowSize", QVariant(m_nlMeansTemplateWindowSize),
                     "Size of template patch (should be odd)");
    registerParameter("nlMeansSearchWindowSize", QVariant(m_nlMeansSearchWindowSize),
                     "Size of search window (should be odd)");
    
    // Median blur parameters
    registerParameter("medianKernelSize", QVariant(m_medianKernelSize),
                     "Kernel size for median filter (must be odd)");
    
    // Morphological parameters
    registerParameter("morphKernelSize", QVariant(m_morphKernelSize),
                     "Kernel size for morphological operations");
    registerParameter("morphShape", QVariant("rect"),
                     "Kernel shape (rect, cross, ellipse)");
    registerParameter("morphOperation", QVariant("open"),
                     "Morphological operation (open, close, gradient, tophat, blackhat)");
    registerParameter("morphIterations", QVariant(m_morphIterations),
                     "Number of morphological iterations");
    
    // Multi-stage and adaptive parameters
    registerParameter("useMultiStage", QVariant(m_useMultiStage),
                     "Use multiple denoising stages");
    registerParameter("stages", QVariant(QStringList()),
                     "List of denoising stages to apply");
    registerParameter("adaptiveMode", QVariant(m_adaptiveMode),
                     "Automatically adapt parameters based on noise level");
}

DenoisePreProcessor::~DenoisePreProcessor() {
    // Cleanup handled by base class
}

bool DenoisePreProcessor::initializePreprocessorImplementation() {
    logInfo("Initializing Denoise preprocessor");
    
    // Initialize morphological kernel
    updateMorphKernel();
    
    // Set default algorithm string for configuration
    QString algoStr = algorithmToString(m_algorithm);
    setParameter("algorithm", algoStr);
    
    logInfo("Denoise preprocessor initialized successfully");
    return true;
}

void DenoisePreProcessor::cleanupPreprocessorImplementation() {
    logInfo("Cleaning up Denoise preprocessor");
    
    // Release any specific resources
    m_morphKernel.release();
    
    logInfo("Denoise preprocessor cleanup completed");
}

cv::Mat DenoisePreProcessor::processImplementation(const cv::Mat& input) {
    // Adapt parameters if in adaptive mode
    if (m_adaptiveMode) {
        adaptParameters(input);
    }
    
    // Apply multi-stage denoising if enabled
    if (m_useMultiStage && !m_stages.isEmpty()) {
        return applyMultiStageDenoising(input);
    }
    
    // Apply single algorithm
    switch (m_algorithm) {
    case DenoiseAlgorithm::Bilateral:
        return applyBilateral(input);
        
    case DenoiseAlgorithm::NonLocalMeans:
        return applyNonLocalMeans(input);
        
    case DenoiseAlgorithm::MedianBlur:
        return applyMedianBlur(input);
        
    case DenoiseAlgorithm::Morphological:
        return applyMorphological(input);
        
    case DenoiseAlgorithm::FastNlMeans:
        return applyFastNlMeans(input);
        
    default:
        setProcessingError("Unknown denoising algorithm");
        return input.clone();
    }
}

cv::Mat DenoisePreProcessor::applyBilateral(const cv::Mat& input) {
    cv::Mat output;
    
    try {
        cv::bilateralFilter(input, output,
                           m_bilateralD,
                           m_bilateralSigmaColor,
                           m_bilateralSigmaSpace);
        
        if (isDebugMode()) {
            logDebug(QString("Bilateral filter applied: d=%1 sigmaColor=%2 sigmaSpace=%3")
                    .arg(m_bilateralD).arg(m_bilateralSigmaColor).arg(m_bilateralSigmaSpace));
        }
        
    } catch (const cv::Exception& e) {
        setProcessingError(QString("Bilateral filter failed: %1").arg(e.what()));
        emit processingError(ProcessingError::AlgorithmError, getLastError());
        return input.clone();
    }
    
    return output;
}

cv::Mat DenoisePreProcessor::applyNonLocalMeans(const cv::Mat& input) {
    cv::Mat output;
    
    try {
        if (input.channels() == 1) {
            // Grayscale image
            cv::fastNlMeansDenoising(input, output,
                                    m_nlMeansH,
                                    m_nlMeansTemplateWindowSize,
                                    m_nlMeansSearchWindowSize);
        } else {
            // Color image
            cv::fastNlMeansDenoisingColored(input, output,
                                           m_nlMeansH,
                                           m_nlMeansHColor,
                                           m_nlMeansTemplateWindowSize,
                                           m_nlMeansSearchWindowSize);
        }
        
        if (isDebugMode()) {
            logDebug(QString("Non-local means denoising applied: h=%1 templateWindow=%2 searchWindow=%3")
                    .arg(m_nlMeansH).arg(m_nlMeansTemplateWindowSize).arg(m_nlMeansSearchWindowSize));
        }
        
    } catch (const cv::Exception& e) {
        setProcessingError(QString("Non-local means denoising failed: %1").arg(e.what()));
        emit processingError(ProcessingError::AlgorithmError, getLastError());
        return input.clone();
    }
    
    return output;
}

cv::Mat DenoisePreProcessor::applyMedianBlur(const cv::Mat& input) {
    cv::Mat output;
    
    try {
        cv::medianBlur(input, output, m_medianKernelSize);
        
        if (isDebugMode()) {
            logDebug(QString("Median blur applied: kernel=%1")
                    .arg(m_medianKernelSize));
        }
        
    } catch (const cv::Exception& e) {
        setProcessingError(QString("Median blur failed: %1").arg(e.what()));
        emit processingError(ProcessingError::AlgorithmError, getLastError());
        return input.clone();
    }
    
    return output;
}

cv::Mat DenoisePreProcessor::applyMorphological(const cv::Mat& input) {
    cv::Mat output;
    
    try {
        // Ensure kernel is up to date
        if (m_morphKernelDirty) {
            updateMorphKernel();
        }
        
        // Apply morphological operation
        cv::morphologyEx(input, output,
                        m_morphOperation,
                        m_morphKernel,
                        cv::Point(-1, -1),
                        m_morphIterations);
        
        if (isDebugMode()) {
            logDebug(QString("Morphological denoising applied: operation=%1 iterations=%2")
                    .arg(m_morphOperation).arg(m_morphIterations));
        }
        
    } catch (const cv::Exception& e) {
        setProcessingError(QString("Morphological denoising failed: %1").arg(e.what()));
        emit processingError(ProcessingError::AlgorithmError, getLastError());
        return input.clone();
    }
    
    return output;
}

cv::Mat DenoisePreProcessor::applyFastNlMeans(const cv::Mat& input) {
    cv::Mat output;
    
    try {
        // Fast NL-means with automatic parameter selection
        float h = m_nlMeansH;
        
        if (m_adaptiveMode && m_noiseEstimate > 0) {
            // Adjust h based on noise estimate
            h = m_noiseEstimate * 0.8f;  // Empirical factor
        }
        
        if (input.channels() == 1) {
            cv::fastNlMeansDenoising(input, output, h);
        } else {
            cv::fastNlMeansDenoisingColored(input, output, h, h);
        }
        
        if (isDebugMode()) {
            logDebug(QString("Fast NL-means denoising applied: h=%1")
                    .arg(h));
        }
        
    } catch (const cv::Exception& e) {
        setProcessingError(QString("Fast NL-means denoising failed: %1").arg(e.what()));
        emit processingError(ProcessingError::AlgorithmError, getLastError());
        return input.clone();
    }
    
    return output;
}

cv::Mat DenoisePreProcessor::applyMultiStageDenoising(const cv::Mat& input) {
    cv::Mat result = input;
    
    for (const auto& stage : m_stages) {
        // Temporarily switch algorithm
        DenoiseAlgorithm originalAlgo = m_algorithm;
        m_algorithm = stage;
        
        // Apply stage
        switch (stage) {
        case DenoiseAlgorithm::Bilateral:
            result = applyBilateral(result);
            break;
            
        case DenoiseAlgorithm::NonLocalMeans:
            result = applyNonLocalMeans(result);
            break;
            
        case DenoiseAlgorithm::MedianBlur:
            result = applyMedianBlur(result);
            break;
            
        case DenoiseAlgorithm::Morphological:
            result = applyMorphological(result);
            break;
            
        case DenoiseAlgorithm::FastNlMeans:
            result = applyFastNlMeans(result);
            break;
            
        default:
            break;
        }
        
        // Restore original algorithm
        m_algorithm = originalAlgo;
        
        if (isDebugMode()) {
            logDebug(QString("Multi-stage denoising: completed stage %1")
                    .arg(algorithmToString(stage)));
        }
    }
    
    return result;
}

double DenoisePreProcessor::estimateNoise(const cv::Mat& input) {
    // Simple noise estimation using Laplacian variance
    cv::Mat gray;
    if (input.channels() > 1) {
        cv::cvtColor(input, gray, cv::COLOR_BGR2GRAY);
    } else {
        gray = input;
    }
    
    cv::Mat laplacian;
    cv::Laplacian(gray, laplacian, CV_64F);
    
    cv::Scalar mean, stddev;
    cv::meanStdDev(laplacian, mean, stddev);
    
    // Median absolute deviation method for robust noise estimation
    double noise = stddev[0] * 0.6745;  // Scale factor for Gaussian noise
    
    if (isDebugMode()) {
        logDebug(QString("Estimated noise level: %1").arg(noise));
    }
    
    return noise;
}

void DenoisePreProcessor::adaptParameters(const cv::Mat& input) {
    // Estimate noise level
    m_noiseEstimate = estimateNoise(input);
    
    // Adapt parameters based on noise level
    if (m_noiseEstimate < 5.0) {
        // Low noise - use gentle denoising
        m_bilateralSigmaColor = 50.0;
        m_bilateralSigmaSpace = 50.0;
        m_nlMeansH = 5.0f;
        m_medianKernelSize = 3;
        
    } else if (m_noiseEstimate < 15.0) {
        // Medium noise - standard denoising
        m_bilateralSigmaColor = 75.0;
        m_bilateralSigmaSpace = 75.0;
        m_nlMeansH = 10.0f;
        m_medianKernelSize = 5;
        
    } else {
        // High noise - aggressive denoising
        m_bilateralSigmaColor = 100.0;
        m_bilateralSigmaSpace = 100.0;
        m_nlMeansH = 15.0f;
        m_medianKernelSize = 7;
    }
    
    if (isDebugMode()) {
        logDebug(QString("Adapted parameters for noise level: %1").arg(m_noiseEstimate));
    }
}

void DenoisePreProcessor::updateMorphKernel() {
    m_morphKernel = cv::getStructuringElement(
        m_morphShape,
        cv::Size(m_morphKernelSize, m_morphKernelSize)
    );
    m_morphKernelDirty = false;
}

bool DenoisePreProcessor::configurePreprocessorImplementation(const QVariantMap& config) {
    // Update algorithm
    if (config.contains("algorithm")) {
        QString algoStr = config["algorithm"].toString();
        m_algorithm = stringToAlgorithm(algoStr);
    }
    
    // Update bilateral parameters
    if (config.contains("bilateralD")) {
        m_bilateralD = config["bilateralD"].toInt();
    }
    if (config.contains("bilateralSigmaColor")) {
        m_bilateralSigmaColor = config["bilateralSigmaColor"].toDouble();
    }
    if (config.contains("bilateralSigmaSpace")) {
        m_bilateralSigmaSpace = config["bilateralSigmaSpace"].toDouble();
    }
    
    // Update NL-means parameters
    if (config.contains("nlMeansH")) {
        m_nlMeansH = config["nlMeansH"].toFloat();
    }
    if (config.contains("nlMeansHColor")) {
        m_nlMeansHColor = config["nlMeansHColor"].toFloat();
    }
    if (config.contains("nlMeansTemplateWindowSize")) {
        int size = config["nlMeansTemplateWindowSize"].toInt();
        if (size > 0 && size % 2 == 1) {
            m_nlMeansTemplateWindowSize = size;
        } else {
            setProcessingError("Template window size must be positive and odd");
            return false;
        }
    }
    if (config.contains("nlMeansSearchWindowSize")) {
        int size = config["nlMeansSearchWindowSize"].toInt();
        if (size > 0 && size % 2 == 1) {
            m_nlMeansSearchWindowSize = size;
        } else {
            setProcessingError("Search window size must be positive and odd");
            return false;
        }
    }
    
    // Update median blur parameters
    if (config.contains("medianKernelSize")) {
        int size = config["medianKernelSize"].toInt();
        if (size > 0 && size % 2 == 1) {
            m_medianKernelSize = size;
        } else {
            setProcessingError("Median kernel size must be positive and odd");
            return false;
        }
    }
    
    // Update morphological parameters
    if (config.contains("morphKernelSize")) {
        m_morphKernelSize = config["morphKernelSize"].toInt();
        m_morphKernelDirty = true;
    }
    if (config.contains("morphShape")) {
        QString shape = config["morphShape"].toString().toLower();
        if (shape == "rect") {
            m_morphShape = cv::MORPH_RECT;
        } else if (shape == "cross") {
            m_morphShape = cv::MORPH_CROSS;
        } else if (shape == "ellipse") {
            m_morphShape = cv::MORPH_ELLIPSE;
        }
        m_morphKernelDirty = true;
    }
    if (config.contains("morphOperation")) {
        QString op = config["morphOperation"].toString().toLower();
        if (op == "open") {
            m_morphOperation = cv::MORPH_OPEN;
        } else if (op == "close") {
            m_morphOperation = cv::MORPH_CLOSE;
        } else if (op == "gradient") {
            m_morphOperation = cv::MORPH_GRADIENT;
        } else if (op == "tophat") {
            m_morphOperation = cv::MORPH_TOPHAT;
        } else if (op == "blackhat") {
            m_morphOperation = cv::MORPH_BLACKHAT;
        }
    }
    if (config.contains("morphIterations")) {
        m_morphIterations = config["morphIterations"].toInt();
    }
    
    // Update multi-stage parameters
    if (config.contains("useMultiStage")) {
        m_useMultiStage = config["useMultiStage"].toBool();
    }
    if (config.contains("stages")) {
        m_stages.clear();
        QStringList stageList = config["stages"].toStringList();
        for (const QString& stage : stageList) {
            m_stages.append(stringToAlgorithm(stage));
        }
    }
    
    // Update adaptive mode
    if (config.contains("adaptiveMode")) {
        m_adaptiveMode = config["adaptiveMode"].toBool();
    }
    
    return true;
}

bool DenoisePreProcessor::validateInput(const cv::Mat& input) {
    if (input.empty()) {
        setProcessingError("Input image is empty");
        return false;
    }
    
    // Check supported data types
    int depth = input.depth();
    if (depth != CV_8U && depth != CV_16U) {
        setProcessingError("Denoising requires 8-bit or 16-bit images");
        return false;
    }
    
    // Check minimum size for some algorithms
    if (m_algorithm == DenoiseAlgorithm::NonLocalMeans ||
        m_algorithm == DenoiseAlgorithm::FastNlMeans) {
        int minSize = m_nlMeansSearchWindowSize + m_nlMeansTemplateWindowSize;
        if (input.cols < minSize || input.rows < minSize) {
            setProcessingError("Image too small for NL-means denoising");
            return false;
        }
    }
    
    return true;
}

DenoisePreProcessor::DenoiseAlgorithm DenoisePreProcessor::stringToAlgorithm(const QString& str) {
    QString lower = str.toLower();
    if (lower == "bilateral") return DenoiseAlgorithm::Bilateral;
    if (lower == "nlmeans" || lower == "nonlocalmeans") return DenoiseAlgorithm::NonLocalMeans;
    if (lower == "median") return DenoiseAlgorithm::MedianBlur;
    if (lower == "morphological") return DenoiseAlgorithm::Morphological;
    if (lower == "fastnlmeans") return DenoiseAlgorithm::FastNlMeans;
    return DenoiseAlgorithm::Bilateral;  // Default
}

QString DenoisePreProcessor::algorithmToString(DenoiseAlgorithm algo) {
    switch (algo) {
    case DenoiseAlgorithm::Bilateral: return "bilateral";
    case DenoiseAlgorithm::NonLocalMeans: return "nlmeans";
    case DenoiseAlgorithm::MedianBlur: return "median";
    case DenoiseAlgorithm::Morphological: return "morphological";
    case DenoiseAlgorithm::FastNlMeans: return "fastnlmeans";
    case DenoiseAlgorithm::Wiener: return "wiener";
    default: return "unknown";
    }
}

} // namespace OpenCV
} // namespace ComponentsForest