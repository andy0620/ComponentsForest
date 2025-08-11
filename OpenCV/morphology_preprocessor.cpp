#include "morphology_preprocessor.h"
#include <QDebug>

namespace ComponentsForest {
namespace OpenCV {

MorphologyPreProcessor::MorphologyPreProcessor(QObject* parent)
    : PreProcessorBase("Morphology", parent),
      m_operation(Operation::Erode),
      m_kernelShape(cv::MORPH_RECT),
      m_kernelWidth(3),
      m_kernelHeight(3),
      m_iterations(1)
{
    // Register parameters for external configuration
    registerParameter("operation", static_cast<int>(m_operation), "Morphological operation type (0=Erode, 1=Dilate, 2=Open, ...)");
    registerParameter("kernel_shape", m_kernelShape, "Shape of the structuring element (0=Rect, 1=Cross, 2=Ellipse)");
    registerParameter("kernel_width", m_kernelWidth, "Width of the structuring element.");
    registerParameter("kernel_height", m_kernelHeight, "Height of the structuring element.");
    registerParameter("iterations", m_iterations, "Number of times the operation is applied.");
}

cv::Mat MorphologyPreProcessor::processImplementation(const cv::Mat& input)
{
    cv::Mat kernel = cv::getStructuringElement(m_kernelShape, cv::Size(m_kernelWidth, m_kernelHeight));

    cv::Mat outputImage;
    cv::morphologyEx(input, outputImage, static_cast<int>(m_operation), kernel, cv::Point(-1, -1), m_iterations);

    return outputImage;
}

bool MorphologyPreProcessor::configurePreprocessorImplementation(const QVariantMap& config)
{
    if (config.contains("operation")) {
        m_operation = static_cast<Operation>(config["operation"].toInt());
    }
    if (config.contains("kernel_shape")) {
        m_kernelShape = config["kernel_shape"].toInt();
    }
    if (config.contains("kernel_width")) {
        m_kernelWidth = config["kernel_width"].toInt();
    }
    if (config.contains("kernel_height")) {
        m_kernelHeight = config["kernel_height"].toInt();
    }
    if (config.contains("iterations")) {
        m_iterations = config["iterations"].toInt();
    }
    qDebug() << "MorphologyPreProcessor configured: operation=" << static_cast<int>(m_operation);
    return true;
}

} // namespace OpenCV
} // namespace ComponentsForest
