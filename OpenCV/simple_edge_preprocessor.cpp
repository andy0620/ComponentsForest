#include "simple_edge_preprocessor.h"
#include <opencv2/imgproc.hpp>
#include <QDebug>
#include <QMutex>
#include <QMutexLocker>

namespace ComponentsForest {
namespace OpenCV {

// Private implementation
class SimpleEdgePreProcessor::Private {
public:
    Private() 
        : lowThreshold(50.0)
        , highThreshold(150.0)
        , kernelSize(3)
    {
    }
    
    double lowThreshold;
    double highThreshold;
    int kernelSize;
    QMutex paramMutex;
};

// Constructor
SimpleEdgePreProcessor::SimpleEdgePreProcessor(const QString& name, QObject* parent)
    : PreProcessorBase(name, parent)
    , d(std::make_unique<Private>())
{
    qDebug() << "SimpleEdgePreProcessor created:" << name;
    
    // Register parameters with base class
    registerParameter("lowThreshold", d->lowThreshold, "Canny low threshold");
    registerParameter("highThreshold", d->highThreshold, "Canny high threshold");
    registerParameter("kernelSize", d->kernelSize, "Gaussian kernel size");
    
    // Set default configuration
    QVariantMap defaultConfig;
    defaultConfig["lowThreshold"] = d->lowThreshold;
    defaultConfig["highThreshold"] = d->highThreshold;
    defaultConfig["kernelSize"] = d->kernelSize;
    defaultConfig["useGaussianBlur"] = true;
    setDefaultConfiguration(defaultConfig);
}

// Destructor
SimpleEdgePreProcessor::~SimpleEdgePreProcessor() {
    qDebug() << "SimpleEdgePreProcessor destroyed";
}

// Edge detection parameters
void SimpleEdgePreProcessor::setLowThreshold(double threshold) {
    QMutexLocker locker(&d->paramMutex);
    if (d->lowThreshold != threshold) {
        d->lowThreshold = threshold;
        setParameter("lowThreshold", threshold);
        emit thresholdsChanged(d->lowThreshold, d->highThreshold);
    }
}

double SimpleEdgePreProcessor::getLowThreshold() const {
    QMutexLocker locker(&d->paramMutex);
    return d->lowThreshold;
}

void SimpleEdgePreProcessor::setHighThreshold(double threshold) {
    QMutexLocker locker(&d->paramMutex);
    if (d->highThreshold != threshold) {
        d->highThreshold = threshold;
        setParameter("highThreshold", threshold);
        emit thresholdsChanged(d->lowThreshold, d->highThreshold);
    }
}

double SimpleEdgePreProcessor::getHighThreshold() const {
    QMutexLocker locker(&d->paramMutex);
    return d->highThreshold;
}

void SimpleEdgePreProcessor::setKernelSize(int size) {
    QMutexLocker locker(&d->paramMutex);
    // Ensure kernel size is odd and at least 3
    if (size % 2 == 0) {
        size++;
    }
    if (size < 3) {
        size = 3;
    }
    
    if (d->kernelSize != size) {
        d->kernelSize = size;
        setParameter("kernelSize", size);
        emit kernelSizeChanged(size);
    }
}

int SimpleEdgePreProcessor::getKernelSize() const {
    QMutexLocker locker(&d->paramMutex);
    return d->kernelSize;
}

// Public slots
void SimpleEdgePreProcessor::updateThresholds(double low, double high) {
    setLowThreshold(low);
    setHighThreshold(high);
}

void SimpleEdgePreProcessor::updateKernelSize(int size) {
    setKernelSize(size);
}

// Process implementation - the actual edge detection
cv::Mat SimpleEdgePreProcessor::processImplementation(const cv::Mat& input) {
    if (input.empty()) {
        qWarning() << "SimpleEdgePreProcessor: Empty input image";
        return cv::Mat();
    }
    
    cv::Mat gray, blurred, edges;
    
    try {
        // Convert to grayscale if needed
        if (input.channels() == 3 || input.channels() == 4) {
            cv::cvtColor(input, gray, cv::COLOR_BGR2GRAY);
        } else {
            gray = input.clone();
        }
        
        // Apply Gaussian blur to reduce noise
        int kernelSize = d->kernelSize;
        if (kernelSize > 1) {
            cv::GaussianBlur(gray, blurred, cv::Size(kernelSize, kernelSize), 0);
        } else {
            blurred = gray;
        }
        
        // Apply Canny edge detection
        double lowThreshold = d->lowThreshold;
        double highThreshold = d->highThreshold;
        cv::Canny(blurred, edges, lowThreshold, highThreshold);
        
        // Convert back to BGR for display (optional)
        cv::Mat output;
        cv::cvtColor(edges, output, cv::COLOR_GRAY2BGR);
        
        return output;
        
    } catch (const cv::Exception& e) {
        qWarning() << "OpenCV error in edge detection:" << e.what();
        setProcessingError(QString("Edge detection failed: %1").arg(e.what()));
        return cv::Mat();
    } catch (const std::exception& e) {
        qWarning() << "Error in edge detection:" << e.what();
        setProcessingError(QString("Edge detection failed: %1").arg(e.what()));
        return cv::Mat();
    }
}

// Validate input
bool SimpleEdgePreProcessor::validateInput(const cv::Mat& input) {
    if (input.empty()) {
        setProcessingError("Input image is empty");
        return false;
    }
    
    if (input.depth() != CV_8U && input.depth() != CV_16U) {
        setProcessingError("Input image must be 8-bit or 16-bit");
        return false;
    }
    
    return true;
}

// Configure implementation
bool SimpleEdgePreProcessor::configurePreprocessorImplementation(const QVariantMap& config) {
    bool success = true;
    
    if (config.contains("lowThreshold")) {
        setLowThreshold(config["lowThreshold"].toDouble());
    }
    
    if (config.contains("highThreshold")) {
        setHighThreshold(config["highThreshold"].toDouble());
    }
    
    if (config.contains("kernelSize")) {
        setKernelSize(config["kernelSize"].toInt());
    }
    
    qDebug() << "SimpleEdgePreProcessor configured with:"
             << "lowThreshold=" << d->lowThreshold
             << "highThreshold=" << d->highThreshold
             << "kernelSize=" << d->kernelSize;
    
    return success;
}

// Initialize implementation
bool SimpleEdgePreProcessor::initializePreprocessorImplementation() {
    qDebug() << "SimpleEdgePreProcessor initializing...";
    
    // Check OpenCV availability
    if (cv::getBuildInformation().empty()) {
        setProcessingError("OpenCV not properly initialized");
        return false;
    }
    
    qDebug() << "SimpleEdgePreProcessor initialized successfully";
    return true;
}

// Cleanup implementation
void SimpleEdgePreProcessor::cleanupPreprocessorImplementation() {
    qDebug() << "SimpleEdgePreProcessor cleaning up...";
    // No special cleanup needed for this simple implementation
}

} // namespace OpenCV
} // namespace ComponentsForest