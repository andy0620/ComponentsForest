#pragma once

#include "preprocessor_base.h"

namespace ComponentsForest {
namespace OpenCV {

/**
 * @brief Gaussian blur preprocessor
 * 
 * Applies Gaussian blur to reduce noise and smooth images
 */
class BlurPreProcessor : public PreProcessorBase {
    Q_OBJECT
    
public:
    explicit BlurPreProcessor(QObject* parent = nullptr);
    
protected:
    cv::Mat processImplementation(const cv::Mat& input) override;
    bool configurePreprocessorImplementation(const QVariantMap& config) override;
    bool validateInput(const cv::Mat& input) override;
    bool initializePreprocessorImplementation() override;
    void cleanupPreprocessorImplementation() override;
    
private:
    // Blur parameters
    int m_kernelWidth;
    int m_kernelHeight;
    double m_sigmaX;
    double m_sigmaY;
    int m_borderType;
    
    // Optimization flags
    bool m_useGPU;
    bool m_useSeparableFilter;
    
    // GPU resources (if available)
    void* m_gpuFilter;  // cv::Ptr<cv::cuda::Filter> stored as void* to avoid header dependency
    
    // Helper methods
    cv::Mat processOnCPU(const cv::Mat& input);
    cv::Mat processOnGPU(const cv::Mat& input);
    void initializeGPUResources();
    void releaseGPUResources();
};

} // namespace OpenCV
} // namespace ComponentsForest