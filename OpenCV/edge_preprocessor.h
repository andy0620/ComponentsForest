#pragma once

#include "preprocessor_base.h"

namespace ComponentsForest {
namespace OpenCV {

/**
 * @brief Edge detection preprocessor
 * 
 * Implements various edge detection algorithms including Canny, Sobel, and Laplacian
 */
class EdgePreProcessor : public PreProcessorBase {
    Q_OBJECT
    
public:
    enum class EdgeAlgorithm {
        Canny,
        Sobel,
        Laplacian,
        Scharr
    };
    Q_ENUM(EdgeAlgorithm)
    
    explicit EdgePreProcessor(QObject* parent = nullptr);
    
protected:
    cv::Mat processImplementation(const cv::Mat& input) override;
    bool configurePreprocessorImplementation(const QVariantMap& config) override;
    bool validateInput(const cv::Mat& input) override;
    bool initializePreprocessorImplementation() override;
    void cleanupPreprocessorImplementation() override;
    
private:
    // Edge detection algorithm
    EdgeAlgorithm m_algorithm;
    
    // Canny parameters
    double m_cannyLowThreshold;
    double m_cannyHighThreshold;
    int m_cannyApertureSize;
    bool m_cannyUseL2Gradient;
    
    // Sobel parameters
    int m_sobelDx;
    int m_sobelDy;
    int m_sobelKernelSize;
    double m_sobelScale;
    double m_sobelDelta;
    
    // Laplacian parameters
    int m_laplacianKernelSize;
    double m_laplacianScale;
    double m_laplacianDelta;
    
    // Common parameters
    bool m_applyGaussianBlur;
    int m_blurKernelSize;
    double m_blurSigma;
    bool m_convertToGrayscale;
    bool m_normalizeOutput;
    
    // Processing methods for each algorithm
    cv::Mat applyCanny(const cv::Mat& input);
    cv::Mat applySobel(const cv::Mat& input);
    cv::Mat applyLaplacian(const cv::Mat& input);
    cv::Mat applyScharr(const cv::Mat& input);
    
    // Helper methods
    cv::Mat preprocessImage(const cv::Mat& input);
    cv::Mat postprocessImage(const cv::Mat& edges);
    EdgeAlgorithm stringToAlgorithm(const QString& str);
    QString algorithmToString(EdgeAlgorithm algo);
};

} // namespace OpenCV
} // namespace ComponentsForest