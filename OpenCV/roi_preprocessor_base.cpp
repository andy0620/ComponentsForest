#include "roi_preprocessor_base.h"
#include <QtConcurrent>
#include <QDebug>
#include <execution>

namespace ComponentsForest {
namespace OpenCV {

// ROIPreProcessorBase implementation
ROIPreProcessorBase::ROIPreProcessorBase(QObject* parent)
    : PreProcessorBase("ROIPreProcessor", parent)
    , m_threadPool(new QThreadPool(this)) {
    
    // Configure thread pool
    m_threadPool->setMaxThreadCount(QThread::idealThreadCount());
    
    // Check GPU availability
    initializeGPU();
}

ROIPreProcessorBase::~ROIPreProcessorBase() {
    if (m_threadPool) {
        m_threadPool->waitForDone();
    }
}

void ROIPreProcessorBase::setROIManager(ROIManager* manager) {
    m_roiManager = manager;
}

void ROIPreProcessorBase::setROIProcessingStrategy(ROIProcessingStrategy strategy) {
    m_roiStrategy = strategy;
}

void ROIPreProcessorBase::setROIProcessingEnabled(bool enabled) {
    m_roiProcessingEnabled = enabled;
}

void ROIPreProcessorBase::setROIROIProcessingParams(const QString& roiId, 
                                                 const ROIProcessingParams& params) {
    m_roiParams[roiId] = params;
}

ROIProcessingParams ROIPreProcessorBase::getROIROIProcessingParams(const QString& roiId) const {
    auto it = m_roiParams.find(roiId);
    if (it != m_roiParams.end()) {
        return it->second;
    }
    return m_params;  // Return default params
}

void ROIPreProcessorBase::setProcessingMask(const cv::Mat& mask) {
    m_processingMask = mask.clone();
}

cv::Mat ROIPreProcessorBase::preprocessROI(const cv::Mat& roiData,
                                           const ROIPtr& roi,
                                           const ROIProcessingParams& params) {
    if (roiData.empty()) {
        return roiData;
    }
    
    cv::Mat result = roiData.clone();
    
    // Apply standard preprocessing based on params
    if (params.enableBlur) {
        cv::GaussianBlur(result, result, 
                        cv::Size(params.blurSize, params.blurSize),
                        0.0, 0.0);
    }
    
    if (params.enableThreshold) {
        cv::threshold(result, result, params.thresholdValue, 255, cv::THRESH_BINARY);
    }
    
    if (params.enableMorphology) {
        cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, 
                                                  cv::Size(params.morphSize, params.morphSize));
        cv::morphologyEx(result, result, params.morphOperation, kernel);
    }
    
    if (params.enableHistogramEqualization) {
        if (result.channels() == 1) {
            cv::equalizeHist(result, result);
        } else {
            std::vector<cv::Mat> channels;
            cv::split(result, channels);
            for (auto& channel : channels) {
                cv::equalizeHist(channel, channel);
            }
            cv::merge(channels, result);
        }
    }
    
    if (params.enableCLAHE) {
        auto clahe = cv::createCLAHE(params.claheClipLimit);
        if (result.channels() == 1) {
            clahe->apply(result, result);
        } else {
            cv::Mat lab;
            cv::cvtColor(result, lab, cv::COLOR_BGR2Lab);
            std::vector<cv::Mat> channels;
            cv::split(lab, channels);
            clahe->apply(channels[0], channels[0]);
            cv::merge(channels, lab);
            cv::cvtColor(lab, result, cv::COLOR_Lab2BGR);
        }
    }
    
    // Additional processing can be handled by customProcessor if needed
    if (params.customProcessor) {
        result = params.customProcessor(result);
    }
    
    return result;
}

cv::Mat ROIPreProcessorBase::postprocessROI(const cv::Mat& roiData,
                                           const ROIPtr& roi) {
    // Default implementation - no post-processing
    return roiData;
}

cv::Mat ROIPreProcessorBase::compositeROIs(const cv::Mat& baseFrame,
                                          const std::vector<ROIProcessingResult>& results) {
    cv::Mat composite = baseFrame.clone();
    
    for (const auto& result : results) {
        if (!result.success || result.processedData.empty()) {
            continue;
        }
        
        auto roi = m_roiManager->getROI(result.roiId);
        if (roi) {
            roi->apply(composite, result.processedData);
        }
    }
    
    return composite;
}

ROIPreProcessorBase::ROIStatistics ROIPreProcessorBase::calculateROIStatistics(
    const cv::Mat& roiData, const ROIPtr& roi) {
    
    ROIStatistics stats;
    
    if (roiData.empty()) {
        return stats;
    }
    
    // Calculate mean and standard deviation
    cv::Scalar mean, stddev;
    cv::meanStdDev(roiData, mean, stddev);
    
    stats.meanIntensity = mean[0];
    stats.stdDeviation = stddev[0];
    stats.meanColor = mean;
    
    // Calculate histogram
    if (roiData.channels() == 1) {
        int histSize = 256;
        float range[] = {0, 256};
        const float* histRange = {range};
        
        cv::calcHist(&roiData, 1, 0, cv::Mat(),
                    stats.histogram, 1, &histSize, &histRange);
    } else {
        // For color images, calculate histogram for each channel
        std::vector<cv::Mat> channels;
        cv::split(roiData, channels);
        
        int histSize = 256;
        float range[] = {0, 256};
        const float* histRange = {range};
        
        std::vector<cv::Mat> histograms;
        for (const auto& channel : channels) {
            cv::Mat hist;
            cv::calcHist(&channel, 1, 0, cv::Mat(),
                        hist, 1, &histSize, &histRange);
            histograms.push_back(hist);
        }
        
        // Merge histograms
        cv::merge(histograms, stats.histogram);
    }
    
    return stats;
}

ProcessedFrame ROIPreProcessorBase::process(const cv::Mat& frame) {
    // Start timing
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Process the frame
    ProcessedFrame result = processFrame(frame);
    
    // Calculate processing time
    auto endTime = std::chrono::high_resolution_clock::now();
    result.metadata.processingTime = 
        std::chrono::duration<double, std::milli>(endTime - startTime).count();
    
    // Update statistics
    updateStatistics(result.metadata.processingTime);
    
    return result;
}

ProcessedFrame ROIPreProcessorBase::processFrame(const cv::Mat& frame) {
    ProcessedFrame result;
    result.frame = frame.clone();
    
    if (!m_roiProcessingEnabled || !m_roiManager) {
        // Apply global preprocessing only
        result.frame = applyProcessing(result.frame);
        return result;
    }
    
    // Get ROIs to process
    auto rois = getROIsToProcess();
    
    if (rois.empty()) {
        // No ROIs, apply global preprocessing
        result.frame = applyProcessing(result.frame);
        return result;
    }
    
    // Process ROIs based on strategy
    std::vector<ROIProcessingResult> roiResults;
    
    switch (m_roiStrategy) {
        case ROIProcessingStrategy::Sequential:
            roiResults = processROIsSequential(frame, rois);
            break;
            
        case ROIProcessingStrategy::Parallel:
            roiResults = processROIsParallel(frame, rois);
            break;
            
        case ROIProcessingStrategy::Batched:
            roiResults = processROIsBatched(frame, rois);
            break;
            
        case ROIProcessingStrategy::GPU:
            if (m_gpuAvailable) {
                roiResults = processROIsGPU(frame, rois);
            } else {
                roiResults = processROIsParallel(frame, rois);
            }
            break;
    }
    
    // Composite results back to frame
    result.frame = compositeROIs(result.frame, roiResults);
    
    // Store ROI results in metadata
    QVariantList roiMetadata;
    for (const auto& roiResult : roiResults) {
        QVariantMap roiMeta;
        roiMeta["id"] = roiResult.roiId;
        roiMeta["success"] = roiResult.success;
        roiMeta["processingTime"] = roiResult.processingTime;
        roiMeta["metadata"] = roiResult.metadata;
        roiMetadata.append(roiMeta);
    }
    result.metadata.custom["roiResults"] = roiMetadata;
    
    emit batchProcessingCompleted(roiResults);
    
    return result;
}

ROIProcessingResult ROIPreProcessorBase::processROIInternal(const cv::Mat& frame,
                                                           const ROIPtr& roi,
                                                           const ROIProcessingParams& params) {
    ROIProcessingResult result;
    result.roiId = roi->getId();
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    try {
        // Extract ROI data
        cv::Mat roiData = roi->extract(frame);
        
        if (roiData.empty()) {
            result.success = false;
            result.error = "Failed to extract ROI data";
            return result;
        }
        
        // Apply preprocessing
        roiData = preprocessROI(roiData, roi, params);
        
        // Apply post-processing
        roiData = postprocessROI(roiData, roi);
        
        // Calculate statistics
        auto stats = calculateROIStatistics(roiData, roi);
        result.metadata["meanIntensity"] = stats.meanIntensity;
        result.metadata["stdDeviation"] = stats.stdDeviation;
        
        emit roiStatisticsCalculated(roi->getId(), stats);
        
        result.processedData = roiData;
        result.success = true;
        
    } catch (const cv::Exception& e) {
        result.success = false;
        result.error = QString::fromStdString(e.what());
        emit roiProcessingFailed(roi->getId(), result.error);
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    result.processingTime = 
        std::chrono::duration<double, std::milli>(endTime - startTime).count();
    
    emit roiProcessingCompleted(roi->getId(), result);
    
    return result;
}

std::vector<ROIPtr> ROIPreProcessorBase::getROIsToProcess() const {
    if (!m_roiManager) {
        return {};
    }
    
    return m_roiManager->getActiveROIs();
}

std::vector<ROIProcessingResult> ROIPreProcessorBase::processROIsSequential(
    const cv::Mat& frame,
    const std::vector<ROIPtr>& rois) {
    
    std::vector<ROIProcessingResult> results;
    results.reserve(rois.size());
    
    emit batchProcessingStarted(rois.size());
    
    int current = 0;
    for (const auto& roi : rois) {
        emit roiProcessingStarted(roi->getId());
        
        auto params = getROIROIProcessingParams(roi->getId());
        auto result = processROIInternal(frame, roi, params);
        results.push_back(result);
        
        emit batchProcessingProgress(++current, rois.size());
    }
    
    return results;
}

std::vector<ROIProcessingResult> ROIPreProcessorBase::processROIsParallel(
    const cv::Mat& frame,
    const std::vector<ROIPtr>& rois) {
    
    emit batchProcessingStarted(rois.size());
    
    // Process ROIs in parallel using QtConcurrent
    QList<ROIPtr> roiList;
    for (const auto& roi : rois) {
        roiList.append(roi);
    }
    
    auto future = QtConcurrent::mapped(roiList, 
        [this, frame](const ROIPtr& roi) {
            emit roiProcessingStarted(roi->getId());
            auto params = getROIROIProcessingParams(roi->getId());
            return processROIInternal(frame, roi, params);
        });
    
    future.waitForFinished();
    
    std::vector<ROIProcessingResult> results;
    for (const auto& result : future.results()) {
        results.push_back(result);
    }
    
    return results;
}

std::vector<ROIProcessingResult> ROIPreProcessorBase::processROIsBatched(
    const cv::Mat& frame,
    const std::vector<ROIPtr>& rois) {
    
    std::vector<ROIProcessingResult> allResults;
    
    emit batchProcessingStarted(rois.size());
    
    // Process in batches
    for (size_t i = 0; i < rois.size(); i += m_batchSize) {
        size_t batchEnd = std::min(i + m_batchSize, rois.size());
        std::vector<ROIPtr> batch(rois.begin() + i, rois.begin() + batchEnd);
        
        // Process batch in parallel
        auto batchResults = processROIsParallel(frame, batch);
        allResults.insert(allResults.end(), batchResults.begin(), batchResults.end());
        
        emit batchProcessingProgress(batchEnd, rois.size());
    }
    
    return allResults;
}

std::vector<ROIProcessingResult> ROIPreProcessorBase::processROIsGPU(
    const cv::Mat& frame,
    const std::vector<ROIPtr>& rois) {
    
#ifdef HAVE_CUDA
    if (!m_gpuAvailable) {
        return processROIsParallel(frame, rois);
    }
    
    std::vector<ROIProcessingResult> results;
    results.reserve(rois.size());
    
    emit batchProcessingStarted(rois.size());
    
    // Upload frame to GPU
    cv::cuda::GpuMat gpuFrame;
    gpuFrame.upload(frame);
    
    // Process each ROI on GPU
    for (const auto& roi : rois) {
        ROIProcessingResult result;
        result.roiId = roi->getId();
        
        try {
            // Extract ROI on GPU
            cv::Rect bounds = roi->getBoundingBox();
            bounds &= cv::Rect(0, 0, frame.cols, frame.rows);
            
            if (bounds.area() > 0) {
                cv::cuda::GpuMat gpuROI = gpuFrame(bounds);
                
                // Apply GPU processing
                auto params = getROIROIProcessingParams(roi->getId());
                
                if (params.blur.enabled) {
                    cv::cuda::GaussianBlur(gpuROI, gpuROI,
                                          cv::Size(params.blur.kernelSize, params.blur.kernelSize),
                                          params.blur.sigmaX, params.blur.sigmaY);
                }
                
                // Download result
                gpuROI.download(result.processedData);
                result.success = true;
            } else {
                result.success = false;
                result.error = "Invalid ROI bounds";
            }
        } catch (const cv::Exception& e) {
            result.success = false;
            result.error = QString::fromStdString(e.what());
        }
        
        results.push_back(result);
        emit roiProcessingCompleted(roi->getId(), result);
    }
    
    return results;
#else
    // Fallback to CPU processing
    return processROIsParallel(frame, rois);
#endif
}

void ROIPreProcessorBase::processROI(const QString& roiId) {
    if (!m_roiManager) return;
    
    auto roi = m_roiManager->getROI(roiId);
    if (!roi) return;
    
    // Process using last frame
    if (!m_lastFrame.empty()) {
        auto params = getROIROIProcessingParams(roiId);
        processROIInternal(m_lastFrame, roi, params);
    }
}

void ROIPreProcessorBase::processROIs(const QStringList& roiIds) {
    for (const auto& roiId : roiIds) {
        processROI(roiId);
    }
}

void ROIPreProcessorBase::updateROIParams(const QString& roiId, const QVariantMap& params) {
    auto currentParams = getROIROIProcessingParams(roiId);
    
    // Update params from variant map
    if (params.contains("blur")) {
        auto blurMap = params["blur"].toMap();
        currentParams.blur.enabled = blurMap["enabled"].toBool();
        currentParams.blur.kernelSize = blurMap["kernelSize"].toInt();
        currentParams.blur.sigmaX = blurMap["sigmaX"].toDouble();
        currentParams.blur.sigmaY = blurMap["sigmaY"].toDouble();
    }
    
    // Add more parameter updates as needed...
    
    m_roiParams[roiId] = currentParams;
}

void ROIPreProcessorBase::clearROISettings(const QString& roiId) {
    m_roiParams.erase(roiId);
}

void ROIPreProcessorBase::clearAllROISettings() {
    m_roiParams.clear();
}

void ROIPreProcessorBase::initializeGPU() {
#ifdef HAVE_CUDA
    m_gpuAvailable = cv::cuda::getCudaEnabledDeviceCount() > 0;
    if (m_gpuAvailable) {
        qDebug() << "GPU acceleration available with" 
                 << cv::cuda::getCudaEnabledDeviceCount() << "device(s)";
    }
#else
    m_gpuAvailable = false;
#endif
}

// ROIProcessingPipeline implementation
ROIProcessingPipeline::ROIProcessingPipeline() {}

ROIProcessingPipeline& ROIProcessingPipeline::withROIManager(ROIManager* manager) {
    m_config.roiManager = manager;
    return *this;
}

ROIProcessingPipeline& ROIProcessingPipeline::withStrategy(ROIProcessingStrategy strategy) {
    m_config.strategy = strategy;
    return *this;
}

ROIProcessingPipeline& ROIProcessingPipeline::withBatchSize(int size) {
    m_config.batchSize = size;
    return *this;
}

ROIProcessingPipeline& ROIProcessingPipeline::withGPU(bool enable) {
    m_config.useGPU = enable;
    return *this;
}

ROIProcessingPipeline& ROIProcessingPipeline::withTracking(TrackingMethod method) {
    m_config.enableTracking = true;
    m_config.trackingMethod = method;
    return *this;
}

ROIProcessingPipeline& ROIProcessingPipeline::withAutoDetection(const QString& modelPath) {
    m_config.enableAutoDetection = true;
    m_config.detectionModelPath = modelPath;
    return *this;
}

ROIProcessingPipeline& ROIProcessingPipeline::addPreprocessor(
    std::function<cv::Mat(const cv::Mat&)> processor) {
    m_config.preprocessors.push_back(processor);
    return *this;
}

ROIProcessingPipeline& ROIProcessingPipeline::addPostprocessor(
    std::function<cv::Mat(const cv::Mat&)> processor) {
    m_config.postprocessors.push_back(processor);
    return *this;
}

std::unique_ptr<ROIPreProcessorBase> ROIProcessingPipeline::build() {
    auto processor = std::make_unique<AdvancedROIPreProcessor>();
    
    // Configure processor
    if (m_config.roiManager) {
        processor->setROIManager(m_config.roiManager);
    }
    
    processor->setROIProcessingStrategy(m_config.strategy);
    processor->setBatchSize(m_config.batchSize);
    
    if (m_config.enableTracking) {
        processor->enableROITracking(true);
        processor->setTrackingMethod(m_config.trackingMethod);
    }
    
    if (m_config.enableAutoDetection && !m_config.detectionModelPath.isEmpty()) {
        processor->loadDetectionModel(m_config.detectionModelPath);
        processor->enableAutoROIDetection(true);
    }
    
    return processor;
}

} // namespace OpenCV
} // namespace ComponentsForest