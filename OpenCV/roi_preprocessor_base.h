#ifndef ROI_PREPROCESSOR_BASE_H
#define ROI_PREPROCESSOR_BASE_H

#include "preprocessor_base.h"
#include "roi_manager.h"
#include "roi_types.h"
#include <QThreadPool>

namespace ComponentsForest {
namespace OpenCV {

/**
 * @brief ROI processing strategy
 */
enum class ROIProcessingStrategy {
    Sequential,      // Process ROIs one by one
    Parallel,        // Process ROIs in parallel
    Batched,         // Process ROIs in batches
    GPU              // GPU-accelerated processing
};

/**
 * @brief Processing result with metadata
 */
struct ProcessedFrame {
    cv::Mat frame;
    QVariantMap metadata;
    bool success{true};
    QString errorMessage;
    
    ProcessedFrame() = default;
    ProcessedFrame(const cv::Mat& f) : frame(f) {}
    ProcessedFrame(const cv::Mat& f, const QVariantMap& meta) : frame(f), metadata(meta) {}
};

/**
 * @brief ROI processing result
 */
struct ROIProcessingResult {
    QString roiId;
    cv::Mat processedData;
    double processingTime;
    bool success;
    QString error;
    QVariantMap metadata;
};

/**
 * @brief Enhanced preprocessor base with ROI support
 */
class ROIPreProcessorBase : public PreProcessorBase {
    Q_OBJECT
    
public:
    explicit ROIPreProcessorBase(QObject* parent = nullptr);
    virtual ~ROIPreProcessorBase();
    
    // ROI Manager
    void setROIManager(ROIManager* manager);
    ROIManager* roiManager() const { return m_roiManager; }
    
    // Processing strategy
    void setROIProcessingStrategy(ROIProcessingStrategy strategy);
    ROIProcessingStrategy roiProcessingStrategy() const { return m_roiStrategy; }
    
    // Batch size for batched processing
    void setBatchSize(int size) { m_batchSize = size; }
    int batchSize() const { return m_batchSize; }
    
    // Enable/disable ROI processing
    void setROIProcessingEnabled(bool enabled);
    bool isROIProcessingEnabled() const { return m_roiProcessingEnabled; }
    
    // Per-ROI processing parameters
    void setROIROIProcessingParams(const QString& roiId, const ROIProcessingParams& params);
    ROIProcessingParams getROIROIProcessingParams(const QString& roiId) const;
    
    // Processing masks
    void setProcessingMask(const cv::Mat& mask);
    cv::Mat processingMask() const { return m_processingMask; }
    
    // ROI-specific preprocessing
    virtual cv::Mat preprocessROI(const cv::Mat& roiData, 
                                  const ROIPtr& roi,
                                  const ROIProcessingParams& params);
    
    // ROI post-processing
    virtual cv::Mat postprocessROI(const cv::Mat& roiData,
                                   const ROIPtr& roi);
    
    // Composite processed ROIs back to frame
    virtual cv::Mat compositeROIs(const cv::Mat& baseFrame,
                                  const std::vector<ROIProcessingResult>& results);
    
    // Statistics per ROI
    struct ROIStatistics {
        double meanIntensity;
        double stdDeviation;
        cv::Scalar meanColor;
        cv::Mat histogram;
        QVariantMap custom;
    };
    
    ROIStatistics calculateROIStatistics(const cv::Mat& roiData, const ROIPtr& roi);
    
    // Main processing method
    ProcessedFrame process(const cv::Mat& frame);
    
signals:
    // ROI processing signals
    void roiProcessingStarted(const QString& roiId);
    void roiProcessingCompleted(const QString& roiId, const ROIProcessingResult& result);
    void roiProcessingFailed(const QString& roiId, const QString& error);
    
    // Batch processing
    void batchProcessingStarted(int totalROIs);
    void batchProcessingProgress(int current, int total);
    void batchProcessingCompleted(const std::vector<ROIProcessingResult>& results);
    
    // Statistics
    void roiStatisticsCalculated(const QString& roiId, const ROIStatistics& stats);
    
public slots:
    // Process specific ROIs
    void processROI(const QString& roiId);
    void processROIs(const QStringList& roiIds);
    
    // Update ROI processing params
    void updateROIParams(const QString& roiId, const QVariantMap& params);
    
    // Clear ROI-specific settings
    void clearROISettings(const QString& roiId);
    void clearAllROISettings();
    
protected:
    // Core processing method to override
    virtual ProcessedFrame processFrame(const cv::Mat& frame);
    
    // ROI-specific processing to override
    virtual ROIProcessingResult processROIInternal(const cv::Mat& frame,
                                                   const ROIPtr& roi,
                                                   const ROIProcessingParams& params);
    
    // Helper methods
    std::vector<ROIPtr> getROIsToProcess() const;
    std::vector<ROIProcessingResult> processROIsSequential(const cv::Mat& frame,
                                                          const std::vector<ROIPtr>& rois);
    std::vector<ROIProcessingResult> processROIsParallel(const cv::Mat& frame,
                                                        const std::vector<ROIPtr>& rois);
    std::vector<ROIProcessingResult> processROIsBatched(const cv::Mat& frame,
                                                       const std::vector<ROIPtr>& rois);
    std::vector<ROIProcessingResult> processROIsGPU(const cv::Mat& frame,
                                                    const std::vector<ROIPtr>& rois);
    
private:
    ROIManager* m_roiManager{nullptr};
    ROIProcessingStrategy m_roiStrategy{ROIProcessingStrategy::Sequential};
    bool m_roiProcessingEnabled{true};
    int m_batchSize{10};
    
    // Per-ROI parameters
    std::map<QString, ROIProcessingParams> m_roiParams;
    ROIProcessingParams m_params;  // Default parameters
    
    // Processing mask
    cv::Mat m_processingMask;
    
    // Thread pool for parallel processing
    QThreadPool* m_threadPool;
    
    // GPU processing support
    bool m_gpuAvailable{false};
    
    // Statistics cache
    mutable std::map<QString, ROIStatistics> m_statsCache;
    
    void initializeGPU();
};

/**
 * @brief Advanced ROI preprocessor with machine learning support
 */
class AdvancedROIPreProcessor : public ROIPreProcessorBase {
    Q_OBJECT
    
public:
    explicit AdvancedROIPreProcessor(QObject* parent = nullptr);
    
    // ML model for ROI detection
    void loadDetectionModel(const QString& modelPath);
    void enableAutoROIDetection(bool enable);
    
    // Adaptive ROI adjustment
    void enableAdaptiveROI(bool enable);
    void setAdaptiveThreshold(double threshold);
    
    // ROI tracking
    void enableROITracking(bool enable);
    void setTrackingMethod(TrackingMethod method);
    
    // Content-aware processing
    void enableContentAwareProcessing(bool enable);
    
    // ROI priority
    void setROIPriority(const QString& roiId, int priority);
    int getROIPriority(const QString& roiId) const;
    
protected:
    ProcessedFrame processFrame(const cv::Mat& frame) override;
    
private:
    // ML model
    cv::dnn::Net m_detectionModel;
    bool m_autoDetectionEnabled{false};
    
    // Adaptive ROI
    bool m_adaptiveEnabled{false};
    double m_adaptiveThreshold{0.8};
    
    // Tracking
    bool m_trackingEnabled{false};
    TrackingMethod m_trackingMethod{TrackingMethod::CSRT};
    
    // Content-aware
    bool m_contentAwareEnabled{false};
    
    // ROI priorities
    std::map<QString, int> m_roiPriorities;
    
    // Helper methods
    std::vector<ROIPtr> detectROIs(const cv::Mat& frame);
    void adjustROIs(const cv::Mat& frame, std::vector<ROIPtr>& rois);
    void updateTracking(const cv::Mat& frame);
    ROIProcessingParams getContentAwareParams(const cv::Mat& roiData);
};

/**
 * @brief ROI processing pipeline builder
 */
class ROIProcessingPipeline {
public:
    ROIProcessingPipeline();
    
    // Builder pattern
    ROIProcessingPipeline& withROIManager(ROIManager* manager);
    ROIProcessingPipeline& withStrategy(ROIProcessingStrategy strategy);
    ROIProcessingPipeline& withBatchSize(int size);
    ROIProcessingPipeline& withGPU(bool enable);
    ROIProcessingPipeline& withTracking(TrackingMethod method);
    ROIProcessingPipeline& withAutoDetection(const QString& modelPath);
    
    // Add processing stages
    ROIProcessingPipeline& addPreprocessor(std::function<cv::Mat(const cv::Mat&)> processor);
    ROIProcessingPipeline& addPostprocessor(std::function<cv::Mat(const cv::Mat&)> processor);
    
    // Build the pipeline
    std::unique_ptr<ROIPreProcessorBase> build();
    
private:
    struct PipelineConfig {
        ROIManager* roiManager{nullptr};
        ROIProcessingStrategy strategy{ROIProcessingStrategy::Sequential};
        int batchSize{10};
        bool useGPU{false};
        bool enableTracking{false};
        TrackingMethod trackingMethod{TrackingMethod::None};
        bool enableAutoDetection{false};
        QString detectionModelPath;
        std::vector<std::function<cv::Mat(const cv::Mat&)>> preprocessors;
        std::vector<std::function<cv::Mat(const cv::Mat&)>> postprocessors;
    };
    
    PipelineConfig m_config;
};

/**
 * @brief Utility class for ROI-based image analysis
 */
class ROIAnalyzer {
public:
    // Analysis types
    enum class AnalysisType {
        Histogram,
        Contours,
        Edges,
        Texture,
        Color,
        Motion,
        Quality
    };
    
    struct AnalysisResult {
        AnalysisType type;
        QString roiId;
        QVariantMap data;
        double confidence;
        QString description;
    };
    
    // Analyze single ROI
    static AnalysisResult analyze(const cv::Mat& roiData,
                                  const ROIPtr& roi,
                                  AnalysisType type);
    
    // Analyze multiple ROIs
    static std::vector<AnalysisResult> analyzeBatch(
        const cv::Mat& frame,
        const std::vector<ROIPtr>& rois,
        AnalysisType type);
    
    // Compare ROIs
    static double compareROIs(const cv::Mat& roi1,
                             const cv::Mat& roi2,
                             AnalysisType comparisonType);
    
    // Find anomalies in ROIs
    static std::vector<QString> findAnomalies(
        const cv::Mat& frame,
        const std::vector<ROIPtr>& rois,
        double threshold = 0.8);
    
    // Generate ROI report
    static QJsonObject generateReport(
        const cv::Mat& frame,
        const std::vector<ROIPtr>& rois,
        const std::vector<AnalysisType>& analyses);
};

/**
 * @brief ROI-based defect detection
 */
class ROIDefectDetector : public ROIPreProcessorBase {
    Q_OBJECT
    
public:
    explicit ROIDefectDetector(QObject* parent = nullptr);
    
    // Defect types
    enum class DefectType {
        Scratch,
        Dent,
        Stain,
        MissingPart,
        Misalignment,
        ColorDeviation,
        TextureAbnormality,
        Custom
    };
    
    struct Defect {
        DefectType type;
        QString roiId;
        cv::Rect location;
        double severity;  // 0-1
        double confidence;  // 0-1
        QString description;
        cv::Mat mask;
    };
    
    // Detection settings
    void setDetectionSensitivity(double sensitivity);
    void enableDefectType(DefectType type, bool enable);
    void setReferenceImage(const cv::Mat& reference);
    
    // Custom defect detection
    void addCustomDetector(const QString& name,
                          std::function<std::vector<Defect>(const cv::Mat&, const ROIPtr&)> detector);
    
    // Get detected defects
    std::vector<Defect> getDefects() const { return m_detectedDefects; }
    std::vector<Defect> getDefectsForROI(const QString& roiId) const;
    
signals:
    void defectDetected(const Defect& defect);
    void defectsCleared();
    
protected:
    ProcessedFrame processFrame(const cv::Mat& frame) override;
    ROIProcessingResult processROIInternal(const cv::Mat& frame,
                                          const ROIPtr& roi,
                                          const ROIProcessingParams& params) override;
    
private:
    double m_sensitivity{0.5};
    std::set<DefectType> m_enabledDefectTypes;
    cv::Mat m_referenceImage;
    std::vector<Defect> m_detectedDefects;
    
    std::map<QString, std::function<std::vector<Defect>(const cv::Mat&, const ROIPtr&)>> m_customDetectors;
    
    // Detection methods
    std::vector<Defect> detectScratches(const cv::Mat& roiData, const ROIPtr& roi);
    std::vector<Defect> detectStains(const cv::Mat& roiData, const ROIPtr& roi);
    std::vector<Defect> detectMissingParts(const cv::Mat& roiData, const ROIPtr& roi);
    std::vector<Defect> detectColorDeviations(const cv::Mat& roiData, const ROIPtr& roi);
};

} // namespace OpenCV
} // namespace ComponentsForest

#endif // ROI_PREPROCESSOR_BASE_H