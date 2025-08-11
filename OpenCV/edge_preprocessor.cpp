#include "edge_preprocessor.h"
#include <QDebug>

namespace ComponentsForest {
namespace OpenCV {

EdgePreProcessor::EdgePreProcessor(QObject* parent)
    : PreProcessorBase("EdgeDetection", parent)
    , m_algorithm(EdgeAlgorithm::Canny)
    , m_cannyLowThreshold(50.0)
    , m_cannyHighThreshold(150.0)
    , m_cannyApertureSize(3)
    , m_cannyUseL2Gradient(false)
    , m_sobelDx(1)
    , m_sobelDy(0)
    , m_sobelKernelSize(3)
    , m_sobelScale(1.0)
    , m_sobelDelta(0.0)
    , m_laplacianKernelSize(3)
    , m_laplacianScale(1.0)
    , m_laplacianDelta(0.0)
    , m_applyGaussianBlur(true)
    , m_blurKernelSize(5)
    , m_blurSigma(1.4)
    , m_convertToGrayscale(true)
    , m_normalizeOutput(false) {
    
    // Register configurable parameters
    registerParameter("algorithm", QVariant("canny"),
                     "Edge detection algorithm (canny, sobel, laplacian, scharr)");
    
    // Canny parameters
    registerParameter("cannyLowThreshold", QVariant(m_cannyLowThreshold),
                     "Lower threshold for Canny edge detection");
    registerParameter("cannyHighThreshold", QVariant(m_cannyHighThreshold),
                     "Upper threshold for Canny edge detection");
    registerParameter("cannyApertureSize", QVariant(m_cannyApertureSize),
                     "Aperture size for Sobel operator in Canny");
    registerParameter("cannyUseL2Gradient", QVariant(m_cannyUseL2Gradient),
                     "Use L2 norm for gradient calculation in Canny");
    
    // Sobel parameters
    registerParameter("sobelDx", QVariant(m_sobelDx),
                     "Order of derivative in X direction for Sobel");
    registerParameter("sobelDy", QVariant(m_sobelDy),
                     "Order of derivative in Y direction for Sobel");
    registerParameter("sobelKernelSize", QVariant(m_sobelKernelSize),
                     "Sobel kernel size (1, 3, 5, or 7)");
    registerParameter("sobelScale", QVariant(m_sobelScale),
                     "Scale factor for Sobel output");
    registerParameter("sobelDelta", QVariant(m_sobelDelta),
                     "Delta value added to Sobel output");
    
    // Laplacian parameters
    registerParameter("laplacianKernelSize", QVariant(m_laplacianKernelSize),
                     "Laplacian kernel size (must be positive and odd)");
    registerParameter("laplacianScale", QVariant(m_laplacianScale),
                     "Scale factor for Laplacian output");
    registerParameter("laplacianDelta", QVariant(m_laplacianDelta),
                     "Delta value added to Laplacian output");
    
    // Common parameters
    registerParameter("applyGaussianBlur", QVariant(m_applyGaussianBlur),
                     "Apply Gaussian blur before edge detection");
    registerParameter("blurKernelSize", QVariant(m_blurKernelSize),
                     "Gaussian blur kernel size");
    registerParameter("blurSigma", QVariant(m_blurSigma),
                     "Gaussian blur sigma value");
    registerParameter("convertToGrayscale", QVariant(m_convertToGrayscale),
                     "Convert to grayscale before processing");
    registerParameter("normalizeOutput", QVariant(m_normalizeOutput),
                     "Normalize output to 0-255 range");
}

bool EdgePreProcessor::initializePreprocessorImplementation() {
    logInfo("Initializing Edge Detection preprocessor");
    
    // Set default algorithm string for configuration
    QString algoStr = algorithmToString(m_algorithm);
    setParameter("algorithm", algoStr);
    
    logInfo("Edge Detection preprocessor initialized successfully");
    return true;
}

void EdgePreProcessor::cleanupPreprocessorImplementation() {
    logInfo("Cleaning up Edge Detection preprocessor");
    
    // No specific resources to clean up for edge detection
    
    logInfo("Edge Detection preprocessor cleanup completed");
}

cv::Mat EdgePreProcessor::processImplementation(const cv::Mat& input) {
    // Preprocess the image
    cv::Mat processed = preprocessImage(input);
    
    // Apply selected edge detection algorithm
    cv::Mat edges;
    switch (m_algorithm) {
    case EdgeAlgorithm::Canny:
        edges = applyCanny(processed);
        break;
        
    case EdgeAlgorithm::Sobel:
        edges = applySobel(processed);
        break;
        
    case EdgeAlgorithm::Laplacian:
        edges = applyLaplacian(processed);
        break;
        
    case EdgeAlgorithm::Scharr:
        edges = applyScharr(processed);
        break;
        
    default:
        setProcessingError("Unknown edge detection algorithm");
        return input.clone();
    }
    
    // Postprocess the result
    return postprocessImage(edges);
}

cv::Mat EdgePreProcessor::preprocessImage(const cv::Mat& input) {
    cv::Mat processed = input;
    
    // Convert to grayscale if needed and requested
    if (m_convertToGrayscale && processed.channels() > 1) {
        cv::cvtColor(processed, processed, cv::COLOR_BGR2GRAY);
    }
    
    // Apply Gaussian blur if requested
    if (m_applyGaussianBlur) {
        cv::GaussianBlur(processed, processed,
                        cv::Size(m_blurKernelSize, m_blurKernelSize),
                        m_blurSigma);
    }
    
    return processed;
}

cv::Mat EdgePreProcessor::postprocessImage(const cv::Mat& edges) {
    cv::Mat output = edges;
    
    // Normalize if requested
    if (m_normalizeOutput) {
        cv::normalize(output, output, 0, 255, cv::NORM_MINMAX);
    }
    
    // Convert to 8-bit if not already
    if (output.depth() != CV_8U) {
        output.convertTo(output, CV_8U);
    }
    
    // Convert single channel to BGR for consistency
    if (output.channels() == 1) {
        cv::cvtColor(output, output, cv::COLOR_GRAY2BGR);
    }
    
    return output;
}

cv::Mat EdgePreProcessor::applyCanny(const cv::Mat& input) {
    cv::Mat edges;
    
    try {
        // Ensure input is grayscale
        cv::Mat gray;
        if (input.channels() > 1) {
            cv::cvtColor(input, gray, cv::COLOR_BGR2GRAY);
        } else {
            gray = input;
        }
        
        // Apply Canny edge detection
        cv::Canny(gray, edges,
                 m_cannyLowThreshold,
                 m_cannyHighThreshold,
                 m_cannyApertureSize,
                 m_cannyUseL2Gradient);
        
        if (isDebugMode()) {
            logDebug(QString("Canny edge detection applied: thresholds=%1-%2")
                    .arg(m_cannyLowThreshold).arg(m_cannyHighThreshold));
        }
        
    } catch (const cv::Exception& e) {
        setProcessingError(QString("Canny edge detection failed: %1").arg(e.what()));
        emit processingError(ProcessingError::AlgorithmError, getLastError());
        return cv::Mat();
    }
    
    return edges;
}

cv::Mat EdgePreProcessor::applySobel(const cv::Mat& input) {
    cv::Mat edges;
    
    try {
        // Calculate gradients
        cv::Mat gradX, gradY;
        
        // Sobel in X direction
        if (m_sobelDx > 0) {
            cv::Sobel(input, gradX, CV_64F,
                     m_sobelDx, 0,
                     m_sobelKernelSize,
                     m_sobelScale, m_sobelDelta);
        }
        
        // Sobel in Y direction
        if (m_sobelDy > 0) {
            cv::Sobel(input, gradY, CV_64F,
                     0, m_sobelDy,
                     m_sobelKernelSize,
                     m_sobelScale, m_sobelDelta);
        }
        
        // Combine gradients
        if (m_sobelDx > 0 && m_sobelDy > 0) {
            // Calculate magnitude
            cv::magnitude(gradX, gradY, edges);
        } else if (m_sobelDx > 0) {
            edges = cv::abs(gradX);
        } else if (m_sobelDy > 0) {
            edges = cv::abs(gradY);
        } else {
            setProcessingError("Sobel requires at least one derivative direction");
            return cv::Mat();
        }
        
        // Convert to proper type
        edges.convertTo(edges, CV_8U);
        
        if (isDebugMode()) {
            logDebug(QString("Sobel edge detection applied: dx=%1 dy=%2 kernel=%3")
                    .arg(m_sobelDx).arg(m_sobelDy).arg(m_sobelKernelSize));
        }
        
    } catch (const cv::Exception& e) {
        setProcessingError(QString("Sobel edge detection failed: %1").arg(e.what()));
        emit processingError(ProcessingError::AlgorithmError, getLastError());
        return cv::Mat();
    }
    
    return edges;
}

cv::Mat EdgePreProcessor::applyLaplacian(const cv::Mat& input) {
    cv::Mat edges;
    
    try {
        // Apply Laplacian
        cv::Mat laplacian;
        cv::Laplacian(input, laplacian, CV_64F,
                     m_laplacianKernelSize,
                     m_laplacianScale,
                     m_laplacianDelta);
        
        // Convert to absolute values
        edges = cv::abs(laplacian);
        
        // Convert to proper type
        edges.convertTo(edges, CV_8U);
        
        if (isDebugMode()) {
            logDebug(QString("Laplacian edge detection applied: kernel=%1")
                    .arg(m_laplacianKernelSize));
        }
        
    } catch (const cv::Exception& e) {
        setProcessingError(QString("Laplacian edge detection failed: %1").arg(e.what()));
        emit processingError(ProcessingError::AlgorithmError, getLastError());
        return cv::Mat();
    }
    
    return edges;
}

cv::Mat EdgePreProcessor::applyScharr(const cv::Mat& input) {
    cv::Mat edges;
    
    try {
        // Scharr is a special case of Sobel with specific kernel values
        cv::Mat gradX, gradY;
        
        // Scharr in X direction
        cv::Scharr(input, gradX, CV_64F, 1, 0);
        
        // Scharr in Y direction
        cv::Scharr(input, gradY, CV_64F, 0, 1);
        
        // Calculate magnitude
        cv::magnitude(gradX, gradY, edges);
        
        // Convert to proper type
        edges.convertTo(edges, CV_8U);
        
        if (isDebugMode()) {
            logDebug("Scharr edge detection applied");
        }
        
    } catch (const cv::Exception& e) {
        setProcessingError(QString("Scharr edge detection failed: %1").arg(e.what()));
        emit processingError(ProcessingError::AlgorithmError, getLastError());
        return cv::Mat();
    }
    
    return edges;
}

bool EdgePreProcessor::configurePreprocessorImplementation(const QVariantMap& config) {
    // Update algorithm
    if (config.contains("algorithm")) {
        QString algoStr = config["algorithm"].toString();
        m_algorithm = stringToAlgorithm(algoStr);
    }
    
    // Update Canny parameters
    if (config.contains("cannyLowThreshold")) {
        m_cannyLowThreshold = config["cannyLowThreshold"].toDouble();
    }
    if (config.contains("cannyHighThreshold")) {
        m_cannyHighThreshold = config["cannyHighThreshold"].toDouble();
    }
    if (config.contains("cannyApertureSize")) {
        int size = config["cannyApertureSize"].toInt();
        if (size == 3 || size == 5 || size == 7) {
            m_cannyApertureSize = size;
        } else {
            setProcessingError("Canny aperture size must be 3, 5, or 7");
            return false;
        }
    }
    if (config.contains("cannyUseL2Gradient")) {
        m_cannyUseL2Gradient = config["cannyUseL2Gradient"].toBool();
    }
    
    // Update Sobel parameters
    if (config.contains("sobelDx")) {
        m_sobelDx = config["sobelDx"].toInt();
    }
    if (config.contains("sobelDy")) {
        m_sobelDy = config["sobelDy"].toInt();
    }
    if (config.contains("sobelKernelSize")) {
        int size = config["sobelKernelSize"].toInt();
        if (size == 1 || size == 3 || size == 5 || size == 7) {
            m_sobelKernelSize = size;
        } else {
            setProcessingError("Sobel kernel size must be 1, 3, 5, or 7");
            return false;
        }
    }
    if (config.contains("sobelScale")) {
        m_sobelScale = config["sobelScale"].toDouble();
    }
    if (config.contains("sobelDelta")) {
        m_sobelDelta = config["sobelDelta"].toDouble();
    }
    
    // Update Laplacian parameters
    if (config.contains("laplacianKernelSize")) {
        int size = config["laplacianKernelSize"].toInt();
        if (size > 0 && size % 2 == 1) {
            m_laplacianKernelSize = size;
        } else {
            setProcessingError("Laplacian kernel size must be positive and odd");
            return false;
        }
    }
    if (config.contains("laplacianScale")) {
        m_laplacianScale = config["laplacianScale"].toDouble();
    }
    if (config.contains("laplacianDelta")) {
        m_laplacianDelta = config["laplacianDelta"].toDouble();
    }
    
    // Update common parameters
    if (config.contains("applyGaussianBlur")) {
        m_applyGaussianBlur = config["applyGaussianBlur"].toBool();
    }
    if (config.contains("blurKernelSize")) {
        int size = config["blurKernelSize"].toInt();
        if (size > 0 && size % 2 == 1) {
            m_blurKernelSize = size;
        } else {
            setProcessingError("Blur kernel size must be positive and odd");
            return false;
        }
    }
    if (config.contains("blurSigma")) {
        m_blurSigma = config["blurSigma"].toDouble();
    }
    if (config.contains("convertToGrayscale")) {
        m_convertToGrayscale = config["convertToGrayscale"].toBool();
    }
    if (config.contains("normalizeOutput")) {
        m_normalizeOutput = config["normalizeOutput"].toBool();
    }
    
    return true;
}

bool EdgePreProcessor::validateInput(const cv::Mat& input) {
    if (input.empty()) {
        setProcessingError("Input image is empty");
        return false;
    }
    
    // Check supported data types
    int depth = input.depth();
    if (depth != CV_8U && depth != CV_16U && depth != CV_32F) {
        setProcessingError("Unsupported image depth for edge detection");
        return false;
    }
    
    // Check minimum size
    if (input.cols < 10 || input.rows < 10) {
        setProcessingError("Image too small for edge detection");
        return false;
    }
    
    return true;
}

EdgePreProcessor::EdgeAlgorithm EdgePreProcessor::stringToAlgorithm(const QString& str) {
    QString lower = str.toLower();
    if (lower == "canny") return EdgeAlgorithm::Canny;
    if (lower == "sobel") return EdgeAlgorithm::Sobel;
    if (lower == "laplacian") return EdgeAlgorithm::Laplacian;
    if (lower == "scharr") return EdgeAlgorithm::Scharr;
    return EdgeAlgorithm::Canny;  // Default
}

QString EdgePreProcessor::algorithmToString(EdgeAlgorithm algo) {
    switch (algo) {
    case EdgeAlgorithm::Canny: return "canny";
    case EdgeAlgorithm::Sobel: return "sobel";
    case EdgeAlgorithm::Laplacian: return "laplacian";
    case EdgeAlgorithm::Scharr: return "scharr";
    default: return "unknown";
    }
}

} // namespace OpenCV
} // namespace ComponentsForest