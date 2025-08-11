#pragma once

#include "preprocessor_base.h"

namespace ComponentsForest {
namespace OpenCV {

/**
 * @brief Simple edge detection preprocessor for demonstration
 * 
 * This preprocessor implements basic Canny edge detection
 * to demonstrate the preprocessor architecture.
 */
class SimpleEdgePreProcessor : public PreProcessorBase {
    Q_OBJECT
    
public:
    explicit SimpleEdgePreProcessor(const QString& name = "SimpleEdgePreProcessor", 
                                   QObject* parent = nullptr);
    virtual ~SimpleEdgePreProcessor();
    
    // Override component info
    QString componentType() const override { return "EdgeDetection"; }
    QString componentVersion() const override { return "1.0.0"; }
    
    // Edge detection parameters
    void setLowThreshold(double threshold);
    double getLowThreshold() const;
    void setHighThreshold(double threshold);
    double getHighThreshold() const;
    void setKernelSize(int size);
    int getKernelSize() const;
    
signals:
    void thresholdsChanged(double low, double high);
    void kernelSizeChanged(int size);
    
public slots:
    void updateThresholds(double low, double high);
    void updateKernelSize(int size);
    
protected:
    // Implementation of pure virtual method from PreProcessorBase
    cv::Mat processImplementation(const cv::Mat& input) override;
    
    // Optional overrides
    bool validateInput(const cv::Mat& input) override;
    bool configurePreprocessorImplementation(const QVariantMap& config) override;
    bool initializePreprocessorImplementation() override;
    void cleanupPreprocessorImplementation() override;
    
private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace OpenCV
} // namespace ComponentsForest