#pragma once

#include "preprocessor_base.h"

namespace ComponentsForest {
namespace OpenCV {

/**
 * @brief Denoising preprocessor
 * 
 * Implements various denoising algorithms including bilateral filter,
 * non-local means, and morphological denoising
 */
class DenoisePreProcessor : public PreProcessorBase {
    Q_OBJECT
    
public:
    enum class DenoiseAlgorithm {
        Bilateral,
        NonLocalMeans,
        MedianBlur,
        Morphological,
        FastNlMeans,
        Wiener  // Future implementation
    };
    Q_ENUM(DenoiseAlgorithm)
    
    explicit DenoisePreProcessor(QObject* parent = nullptr);
    ~DenoisePreProcessor() override;
    
protected:
    cv::Mat processImplementation(const cv::Mat& input) override;
    bool configurePreprocessorImplementation(const QVariantMap& config) override;
    bool validateInput(const cv::Mat& input) override;
    bool initializePreprocessorImplementation() override;
    void cleanupPreprocessorImplementation() override;
    
private:
    // Denoising algorithm
    DenoiseAlgorithm m_algorithm;
    
    // Bilateral filter parameters
    int m_bilateralD;
    double m_bilateralSigmaColor;
    double m_bilateralSigmaSpace;
    
    // Non-local means parameters
    float m_nlMeansH;
    float m_nlMeansHColor;
    int m_nlMeansTemplateWindowSize;
    int m_nlMeansSearchWindowSize;
    
    // Median blur parameters
    int m_medianKernelSize;
    
    // Morphological denoising parameters
    int m_morphKernelSize;
    int m_morphShape;  // cv::MorphShapes
    int m_morphOperation;  // cv::MorphTypes
    int m_morphIterations;
    
    // Multi-stage denoising
    bool m_useMultiStage;
    QList<DenoiseAlgorithm> m_stages;
    
    // Adaptive parameters
    bool m_adaptiveMode;
    double m_noiseEstimate;
    
    // Processing methods for each algorithm
    cv::Mat applyBilateral(const cv::Mat& input);
    cv::Mat applyNonLocalMeans(const cv::Mat& input);
    cv::Mat applyMedianBlur(const cv::Mat& input);
    cv::Mat applyMorphological(const cv::Mat& input);
    cv::Mat applyFastNlMeans(const cv::Mat& input);
    
    // Helper methods
    cv::Mat applyMultiStageDenoising(const cv::Mat& input);
    double estimateNoise(const cv::Mat& input);
    void adaptParameters(const cv::Mat& input);
    DenoiseAlgorithm stringToAlgorithm(const QString& str);
    QString algorithmToString(DenoiseAlgorithm algo);
    
    // Cache for morphological kernels
    cv::Mat m_morphKernel;
    bool m_morphKernelDirty;
    void updateMorphKernel();
};

} // namespace OpenCV
} // namespace ComponentsForest