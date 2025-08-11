#pragma once

#include "preprocessor_base.h"
#include <QList>
#include <QPair>
#include <QFuture>
#include <QtConcurrent>
#include <memory>
#include <functional>

// Import types from ComponentsForest::OpenCV namespace for MOC
using ComponentsForest::OpenCV::FrameMetadata;
using ComponentsForest::OpenCV::ProcessingError;

// Forward declarations
class PreProcessorBase;

/**
 * @brief Pipeline stage interface
 */
class IPipelineStage {
public:
    virtual ~IPipelineStage() = default;
    virtual cv::Mat process(const cv::Mat& input) = 0;
    virtual QString getName() const = 0;
    virtual bool isEnabled() const = 0;
    virtual void setEnabled(bool enabled) = 0;
};

/**
 * @brief Base class for preprocessing pipelines
 * 
 * Manages chains of preprocessors with various execution strategies
 */
class PreProcessingPipeline : public QObject {
    Q_OBJECT
    
public:
    enum class ExecutionMode {
        Sequential,     // Process stages one after another
        Parallel,       // Process stages in parallel (when possible)
        Conditional,    // Choose path based on conditions
        Adaptive        // Dynamically adjust based on load
    };
    
    enum class BackpressureStrategy {
        DropOldest,     // Drop oldest frames when buffer full
        DropNewest,     // Drop new frames when buffer full
        Block,          // Block until space available
        Throttle        // Slow down input rate
    };
    
    explicit PreProcessingPipeline(const QString& name, QObject* parent = nullptr);
    virtual ~PreProcessingPipeline();
    
    // Pipeline construction
    void addStage(PreProcessorBase* processor);
    void addStage(const QString& name, std::function<cv::Mat(const cv::Mat&)> func);
    void insertStage(int index, PreProcessorBase* processor);
    void removeStage(int index);
    void removeStage(PreProcessorBase* processor);
    void clearStages();
    
    // Stage management
    int stageCount() const;
    PreProcessorBase* stage(int index) const;
    QList<PreProcessorBase*> stages() const;
    void setStageEnabled(int index, bool enabled);
    bool isStageEnabled(int index) const;
    
    // Pipeline configuration
    void setExecutionMode(ExecutionMode mode);
    ExecutionMode getExecutionMode() const;
    void setBackpressureStrategy(BackpressureStrategy strategy);
    BackpressureStrategy getBackpressureStrategy() const;
    void setBufferSize(int size);
    int getBufferSize() const;
    void setMaxThreads(int threads);
    int getMaxThreads() const;
    
    // Processing control
    void start();
    void pause();
    void stop();
    void flush();
    bool isRunning() const;
    
    // Performance
    void setGPUEnabled(bool enable);
    bool isGPUEnabled() const;
    void setBatchSize(int size);
    int getBatchSize() const;
    
    // Statistics
    struct PipelineMetrics {
        double totalLatency;        // ms
        double throughput;          // frames/sec
        size_t droppedFrames;
        size_t processedFrames;
        QMap<QString, double> stageLatencies;  // ms per stage
        double cpuUsage;
        double gpuUsage;
        size_t memoryUsage;
    };
    
    PipelineMetrics getMetrics() const;
    void resetMetrics();
    
signals:
    // Output signals
    void pipelineCompleted(const QImage& result, const FrameMetadata& metadata);
    void batchCompleted(const QList<QImage>& results);
    void pipelineError(const QString& stage, const QString& error);
    
    // Status signals
    void pipelineStarted();
    void pipelineStopped();
    void stageCompleted(const QString& stage, qint64 processingTime);
    
    // Performance signals
    void metricsUpdated(const PipelineMetrics& metrics);
    void backpressureDetected();
    
public slots:
    // Input slots
    void processSingle(const QImage& image, const FrameMetadata& metadata);
    void processBatch(const QList<QImage>& images);
    
    // Configuration
    void updateConfiguration(const QVariantMap& config);
    
private slots:
    void onStageCompleted(const QImage& result, const FrameMetadata& metadata);
    void onStageError(ProcessingError error, const QString& details);
    void updateMetrics();
    
protected:
    virtual void connectStages();
    virtual void disconnectStages();
    virtual cv::Mat executeSequential(const cv::Mat& input);
    virtual cv::Mat executeParallel(const cv::Mat& input);
    virtual cv::Mat executeConditional(const cv::Mat& input);
    virtual cv::Mat executeAdaptive(const cv::Mat& input);
    
private:
    class Private;
    std::unique_ptr<Private> d;
};

/**
 * @brief Sequential pipeline - processes stages one after another
 */
class SequentialPipeline : public PreProcessingPipeline {
    Q_OBJECT
    
public:
    explicit SequentialPipeline(const QString& name, QObject* parent = nullptr);
    
protected:
    void connectStages() override;
};

/**
 * @brief Parallel pipeline - processes independent stages in parallel
 */
class ParallelPipeline : public PreProcessingPipeline {
    Q_OBJECT
    
public:
    explicit ParallelPipeline(const QString& name, QObject* parent = nullptr);
    
    // Branch management
    void addBranch(PreProcessingPipeline* branch);
    void removeBranch(PreProcessingPipeline* branch);
    
    // Merge strategy
    enum class MergeStrategy {
        Average,        // Average all branch outputs
        Maximum,        // Take maximum values
        Minimum,        // Take minimum values
        Concatenate,    // Concatenate results
        Custom          // Use custom merge function
    };
    
    void setMergeStrategy(MergeStrategy strategy);
    void setCustomMergeFunction(std::function<cv::Mat(const QList<cv::Mat>&)> func);
    
signals:
    void branchCompleted(int branchIndex, const cv::Mat& result);
    
protected:
    cv::Mat executeParallel(const cv::Mat& input) override;
    
private:
    cv::Mat mergeBranches(const QList<cv::Mat>& results);
    
    QList<PreProcessingPipeline*> m_branches;
    MergeStrategy m_mergeStrategy = MergeStrategy::Average;
    std::function<cv::Mat(const QList<cv::Mat>&)> m_customMergeFunc;
};

/**
 * @brief Conditional pipeline - chooses processing path based on conditions
 */
class ConditionalPipeline : public PreProcessingPipeline {
    Q_OBJECT
    
public:
    explicit ConditionalPipeline(const QString& name, QObject* parent = nullptr);
    
    // Path management
    void setCondition(std::function<bool(const cv::Mat&)> condition);
    void setTruePath(PreProcessingPipeline* pipeline);
    void setFalsePath(PreProcessingPipeline* pipeline);
    
protected:
    cv::Mat executeConditional(const cv::Mat& input) override;
    
private:
    std::function<bool(const cv::Mat&)> m_condition;
    PreProcessingPipeline* m_truePath = nullptr;
    PreProcessingPipeline* m_falsePath = nullptr;
};

/**
 * @brief Adaptive pipeline - dynamically adjusts processing based on load
 */
class AdaptivePipeline : public PreProcessingPipeline {
    Q_OBJECT
    
public:
    explicit AdaptivePipeline(const QString& name, QObject* parent = nullptr);
    
    // Quality levels
    enum class QualityLevel {
        Low,        // Fast but lower quality
        Medium,     // Balanced
        High        // Best quality but slower
    };
    
    void setQualityLevel(QualityLevel level);
    QualityLevel getQualityLevel() const;
    void setAutoAdjust(bool enable);
    bool isAutoAdjust() const;
    
    // Configure quality presets
    void setLowQualityPipeline(PreProcessingPipeline* pipeline);
    void setMediumQualityPipeline(PreProcessingPipeline* pipeline);
    void setHighQualityPipeline(PreProcessingPipeline* pipeline);
    
signals:
    void qualityChanged(QualityLevel newLevel);
    
protected:
    cv::Mat executeAdaptive(const cv::Mat& input) override;
    
private slots:
    void adjustQuality();
    
private:
    QualityLevel m_currentQuality = QualityLevel::Medium;
    bool m_autoAdjust = true;
    
    PreProcessingPipeline* m_lowQualityPipeline = nullptr;
    PreProcessingPipeline* m_mediumQualityPipeline = nullptr;
    PreProcessingPipeline* m_highQualityPipeline = nullptr;
    
    // Performance tracking for auto-adjustment
    QList<qint64> m_recentLatencies;
    double m_targetLatency = 33.0;  // ~30 FPS
};

/**
 * @brief Factory for creating pipeline stages
 */
class PipelineStageFactory {
public:
    static PreProcessorBase* createStage(const QString& type, const QVariantMap& config);
    static PreProcessingPipeline* createPipeline(const QString& type, const QString& name);
    
    // Register custom stage types
    static void registerStageType(const QString& type, 
                                 std::function<PreProcessorBase*()> factory);
    static void registerPipelineType(const QString& type,
                                    std::function<PreProcessingPipeline*(const QString&)> factory);
    
private:
    static QMap<QString, std::function<PreProcessorBase*()>> s_stageFactories;
    static QMap<QString, std::function<PreProcessingPipeline*(const QString&)>> s_pipelineFactories;
};

/**
 * @brief Thread-safe buffer for pipeline stages
 */
template<typename T>
class PipelineBuffer {
public:
    explicit PipelineBuffer(size_t capacity)
        : m_capacity(capacity) {
        m_buffer.reserve(capacity);
    }
    
    bool push(T&& item) {
        QMutexLocker lock(&m_mutex);
        
        if (m_buffer.size() >= m_capacity) {
            return false;  // Buffer full
        }
        
        m_buffer.push_back(std::move(item));
        m_notEmpty.wakeOne();
        return true;
    }
    
    bool tryPush(T&& item, int timeoutMs) {
        QMutexLocker lock(&m_mutex);
        
        if (m_buffer.size() >= m_capacity) {
            if (!m_notFull.wait(&m_mutex, timeoutMs)) {
                return false;  // Timeout
            }
        }
        
        m_buffer.push_back(std::move(item));
        m_notEmpty.wakeOne();
        return true;
    }
    
    T pop() {
        QMutexLocker lock(&m_mutex);
        
        while (m_buffer.empty()) {
            m_notEmpty.wait(&m_mutex);
        }
        
        T item = std::move(m_buffer.front());
        m_buffer.erase(m_buffer.begin());
        m_notFull.wakeOne();
        return item;
    }
    
    bool tryPop(T& item, int timeoutMs) {
        QMutexLocker lock(&m_mutex);
        
        if (m_buffer.empty()) {
            if (!m_notEmpty.wait(&m_mutex, timeoutMs)) {
                return false;  // Timeout
            }
        }
        
        if (m_buffer.empty()) {
            return false;
        }
        
        item = std::move(m_buffer.front());
        m_buffer.erase(m_buffer.begin());
        m_notFull.wakeOne();
        return true;
    }
    
    size_t size() const {
        QMutexLocker lock(&m_mutex);
        return m_buffer.size();
    }
    
    bool empty() const {
        QMutexLocker lock(&m_mutex);
        return m_buffer.empty();
    }
    
    void clear() {
        QMutexLocker lock(&m_mutex);
        m_buffer.clear();
        m_notFull.wakeAll();
    }
    
private:
    mutable QMutex m_mutex;
    QWaitCondition m_notEmpty;
    QWaitCondition m_notFull;
    std::vector<T> m_buffer;
    size_t m_capacity;
};