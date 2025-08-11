#pragma once

#include "preprocessor_base.h"

namespace ComponentsForest {
namespace OpenCV {

/**
 * @brief A preprocessor that applies a fixed-level threshold to an image.
 *
 * This component converts a grayscale image to a binary image.
 */
class ThresholdPreProcessor : public PreProcessorBase {
    Q_OBJECT

public:
    explicit ThresholdPreProcessor(QObject* parent = nullptr);
    ~ThresholdPreProcessor() override = default;

protected:
    // --- PreProcessorBase overrides ---
    cv::Mat processImplementation(const cv::Mat& input) override;
    bool configurePreprocessorImplementation(const QVariantMap& config) override;

private:
    double m_thresholdValue;
    double m_maxValue;
    int m_thresholdType; // Corresponds to cv::ThresholdTypes
};

} // namespace OpenCV
} // namespace ComponentsForest
