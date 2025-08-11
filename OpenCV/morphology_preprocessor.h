#pragma once

#include "preprocessor_base.h"

namespace ComponentsForest {
namespace OpenCV {

/**
 * @brief A preprocessor that performs morphological operations on an image.
 *
 * This component can perform Erode, Dilate, Open, Close, Gradient,
 * Top Hat, and Black Hat operations.
 */
class MorphologyPreProcessor : public PreProcessorBase {
    Q_OBJECT

public:
    enum class Operation {
        Erode = cv::MORPH_ERODE,
        Dilate = cv::MORPH_DILATE,
        Open = cv::MORPH_OPEN,
        Close = cv::MORPH_CLOSE,
        Gradient = cv::MORPH_GRADIENT,
        TopHat = cv::MORPH_TOPHAT,
        BlackHat = cv::MORPH_BLACKHAT
    };
    Q_ENUM(Operation)

    explicit MorphologyPreProcessor(QObject* parent = nullptr);
    ~MorphologyPreProcessor() override = default;

protected:
    // --- PreProcessorBase overrides ---
    cv::Mat processImplementation(const cv::Mat& input) override;
    bool configurePreprocessorImplementation(const QVariantMap& config) override;

private:
    Operation m_operation;
    int m_kernelShape; // cv::MorphShapes
    int m_kernelWidth;
    int m_kernelHeight;
    int m_iterations;
};

} // namespace OpenCV
} // namespace ComponentsForest
