#pragma once

#include "algorithm_base.h"

namespace ComponentsForest {
namespace OpenCV {

/**
 * @brief An algorithm that finds contours in a binary image and calculates their areas.
 *
 * This component takes a binary image as input and outputs a QVariantList
 * containing the area (as a double) of each contour found.
 */
class ContourAreaAlgorithm : public AlgorithmBase {
    Q_OBJECT

public:
    explicit ContourAreaAlgorithm(QObject* parent = nullptr);
    ~ContourAreaAlgorithm() override = default;

protected:
    // --- AlgorithmBase overrides ---
    QVariant processImplementation(const cv::Mat& input) override;
    bool validateInput(const cv::Mat& input) override;

private:
    // Configuration for contour finding can be added here as members
    // e.g., retrieval mode, approximation method, min_area
};

} // namespace OpenCV
} // namespace ComponentsForest
