#pragma once

#include "algorithm_base.h"

namespace ComponentsForest {
namespace OpenCV {

/**
 * @brief An algorithm that calculates the histogram of an image.
 *
 * This component takes an image as input and outputs a QVariantList
 * containing the histogram data for each channel.
 */
class HistogramAlgorithm : public AlgorithmBase {
    Q_OBJECT

public:
    explicit HistogramAlgorithm(QObject* parent = nullptr);
    ~HistogramAlgorithm() override = default;

protected:
    // --- AlgorithmBase overrides ---
    QVariant processImplementation(const cv::Mat& input) override;
    bool validateInput(const cv::Mat& input) override;

private:
    int m_histSize;     // Number of bins
    float m_range[2];   // The range of pixels (0-255)
};

} // namespace OpenCV
} // namespace ComponentsForest
