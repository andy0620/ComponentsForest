#include "contour_area_algorithm.h"
#include <QDebug>
#include <QVariantList>
#include <vector>

namespace ComponentsForest {
namespace OpenCV {

ContourAreaAlgorithm::ContourAreaAlgorithm(QObject* parent)
    : AlgorithmBase("ContourArea", parent)
{
    // In a real implementation, we would register parameters here
    // e.g., registerParameter("min_area", 10.0, "Minimum contour area to report.");
}

bool ContourAreaAlgorithm::validateInput(const cv::Mat& input)
{
    // cv::findContours requires a single-channel, 8-bit image.
    if (input.empty()) {
        qWarning() << "ContourAreaAlgorithm: Input image is empty.";
        return false;
    }
    if (input.type() != CV_8UC1) {
        qWarning() << "ContourAreaAlgorithm: Input image must be a single-channel 8-bit image (CV_8UC1).";
        return false;
    }
    return true;
}

QVariant ContourAreaAlgorithm::processImplementation(const cv::Mat& input)
{
    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Vec4i> hierarchy;

    // Find contours in the binary image
    cv::findContours(input, contours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    QVariantList areaList;
    for (const auto& contour : contours) {
        double area = cv::contourArea(contour);
        areaList.append(area);
    }

    if (isDebugMode()) {
        qDebug() << "Found" << contours.size() << "contours. Areas:" << areaList;
    }

    // Return the list of areas
    return areaList;
}

} // namespace OpenCV
} // namespace ComponentsForest
