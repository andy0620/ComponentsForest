#include "threshold_preprocessor.h"
#include <QDebug>

namespace ComponentsForest {
namespace OpenCV {

ThresholdPreProcessor::ThresholdPreProcessor(QObject* parent)
    : PreProcessorBase("Threshold", parent),
      m_thresholdValue(127.0),
      m_maxValue(255.0),
      m_thresholdType(cv::THRESH_BINARY)
{
    // Register parameters for external configuration
    registerParameter("threshold_value", m_thresholdValue, "Threshold value for binarization.");
    registerParameter("max_value", m_maxValue, "Value to use for pixels above the threshold.");
    registerParameter("threshold_type", m_thresholdType, "Thresholding type (e.g., THRESH_BINARY, THRESH_BINARY_INV).");
}

cv::Mat ThresholdPreProcessor::processImplementation(const cv::Mat& input)
{
    cv::Mat grayImage;
    // Ensure the image is grayscale for thresholding
    if (input.channels() > 1) {
        cv::cvtColor(input, grayImage, cv::COLOR_BGR2GRAY);
    } else {
        grayImage = input;
    }

    cv::Mat binaryImage;
    cv::threshold(grayImage, binaryImage, m_thresholdValue, m_maxValue, m_thresholdType);

    return binaryImage;
}

bool ThresholdPreProcessor::configurePreprocessorImplementation(const QVariantMap& config)
{
    if (config.contains("threshold_value")) {
        m_thresholdValue = config["threshold_value"].toDouble();
    }
    if (config.contains("max_value")) {
        m_maxValue = config["max_value"].toDouble();
    }
    if (config.contains("threshold_type")) {
        m_thresholdType = config["threshold_type"].toInt();
    }
    qDebug() << "ThresholdPreProcessor configured: threshold=" << m_thresholdValue;
    return true;
}

} // namespace OpenCV
} // namespace ComponentsForest
