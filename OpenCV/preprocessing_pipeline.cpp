#include "preprocessing_pipeline.h"
#include <QTimer>
#include <QDebug>
#include <QtConcurrent>
#include <chrono>

// Private implementation for PreProcessingPipeline
class PreProcessingPipeline::Private {
public:
    QString name;
    QList<PreProcessorBase*> stages;
    ExecutionMode executionMode = ExecutionMode::Sequential;
    BackpressureStrategy backpressureStrategy = BackpressureStrategy::DropOldest;
    
    // State
    std::atomic<bool> isRunning{false};
    std::atomic<bool> isPaused{false};
    
    // Configuration
    int bufferSize = 100;
    int maxThreads = QThread::idealThreadCount();
    bool gpuEnabled = false;
    int batchSize = 1;
    
    // Metrics
    std::atomic<size_t> processedFrames{0};
    std::atomic<size_t> droppedFrames{0};
    QMap<QString, QList<qint64>> stageLatencies;
    QMutex metricsMutex;
    QTimer* metricsTimer = nullptr;
    
    // Thread pool for parallel execution
    QThreadPool* threadPool = nullptr;
};

// Constructor
PreProcessingPipeline::PreProcessingPipeline(const QString& name, QObject* parent)
    : QObject(parent)
    , d(std::make_unique<Private>()) {
    
    d->name = name;
    
    // Initialize thread pool
    d->threadPool = new QThreadPool(this);
    d->threadPool->setMaxThreadCount(d->maxThreads);
    
    // Setup metrics timer
    d->metricsTimer = new QTimer(this);
    d->metricsTimer->setInterval(1000);  // Update every second
    connect(d->metricsTimer, &QTimer::timeout, this, &PreProcessingPipeline::updateMetrics);
}

// Destructor
PreProcessingPipeline::~PreProcessingPipeline() {
    stop();
    clearStages();
    
    if (d->threadPool) {
        d->threadPool->waitForDone();
    }
}

// Pipeline construction
void PreProcessingPipeline::addStage(PreProcessorBase* processor) {
    if (!processor) return;
    
    d->stages.append(processor);
    
    // Connect signals if pipeline is running
    if (d->isRunning) {
        connectStages();
    }
}

void PreProcessingPipeline::addStage(const QString& name, 
                                    std::function<cv::Mat(const cv::Mat&)> func) {
    // Create a lambda-based processor
    class LambdaProcessor : public PreProcessorBase {
    public:
        LambdaProcessor(const QString& name, std::function<cv::Mat(const cv::Mat&)> f)
            : PreProcessorBase(name), m_func(f) {}
        
    protected:
        cv::Mat processImplementation(const cv::Mat& input) override {
            return m_func(input);
        }
        
    private:
        std::function<cv::Mat(const cv::Mat&)> m_func;
    };
    
    auto* processor = new LambdaProcessor(name, func);
    processor->setParent(this);
    addStage(processor);
}

void PreProcessingPipeline::insertStage(int index, PreProcessorBase* processor) {
    if (!processor || index < 0 || index > d->stages.size()) return;
    
    d->stages.insert(index, processor);
    
    if (d->isRunning) {
        disconnectStages();
        connectStages();
    }
}

void PreProcessingPipeline::removeStage(int index) {
    if (index < 0 || index >= d->stages.size()) return;
    
    auto* stage = d->stages.takeAt(index);
    
    if (d->isRunning) {
        disconnectStages();
        connectStages();
    }
    
    // Don't delete if it has a parent (managed elsewhere)
    if (!stage->parent()) {
        delete stage;
    }
}

void PreProcessingPipeline::removeStage(PreProcessorBase* processor) {
    int index = d->stages.indexOf(processor);
    if (index != -1) {
        removeStage(index);
    }
}

void PreProcessingPipeline::clearStages() {
    if (d->isRunning) {
        disconnectStages();
    }
    
    // Delete stages that don't have parents
    for (auto* stage : d->stages) {
        if (!stage->parent()) {
            delete stage;
        }
    }
    
    d->stages.clear();
}

// Stage management
int PreProcessingPipeline::stageCount() const {
    return d->stages.size();
}

PreProcessorBase* PreProcessingPipeline::stage(int index) const {
    if (index < 0 || index >= d->stages.size()) {
        return nullptr;
    }
    return d->stages[index];
}

QList<PreProcessorBase*> PreProcessingPipeline::stages() const {
    return d->stages;
}

void PreProcessingPipeline::setStageEnabled(int index, bool enabled) {
    if (index >= 0 && index < d->stages.size()) {
        // Store enabled state in stage's configuration
        d->stages[index]->setParameter("_enabled", enabled);
    }
}

bool PreProcessingPipeline::isStageEnabled(int index) const {
    if (index >= 0 && index < d->stages.size()) {
        return d->stages[index]->getParameter("_enabled").toBool();
    }
    return false;
}

// Pipeline configuration
void PreProcessingPipeline::setExecutionMode(ExecutionMode mode) {
    d->executionMode = mode;
}

PreProcessingPipeline::ExecutionMode PreProcessingPipeline::getExecutionMode() const {
    return d->executionMode;
}

void PreProcessingPipeline::setBackpressureStrategy(BackpressureStrategy strategy) {
    d->backpressureStrategy = strategy;
}

PreProcessingPipeline::BackpressureStrategy PreProcessingPipeline::getBackpressureStrategy() const {
    return d->backpressureStrategy;
}

void PreProcessingPipeline::setBufferSize(int size) {
    d->bufferSize = size;
}

int PreProcessingPipeline::getBufferSize() const {
    return d->bufferSize;
}

void PreProcessingPipeline::setMaxThreads(int threads) {
    d->maxThreads = threads;
    if (d->threadPool) {
        d->threadPool->setMaxThreadCount(threads);
    }
}

int PreProcessingPipeline::getMaxThreads() const {
    return d->maxThreads;
}

// Processing control
void PreProcessingPipeline::start() {
    if (!d->isRunning) {
        d->isRunning = true;
        d->isPaused = false;
        
        // Start all stages
        for (auto* stage : d->stages) {
            stage->startProcessing();
        }
        
        // Connect stages
        connectStages();
        
        // Start metrics timer
        d->metricsTimer->start();
        
        emit pipelineStarted();
    }
}

void PreProcessingPipeline::pause() {
    if (d->isRunning && !d->isPaused) {
        d->isPaused = true;
        
        // Pause all stages
        for (auto* stage : d->stages) {
            stage->pauseProcessing();
        }
    }
}

void PreProcessingPipeline::stop() {
    if (d->isRunning) {
        d->isRunning = false;
        
        // Stop all stages
        for (auto* stage : d->stages) {
            stage->stopProcessing();
        }
        
        // Disconnect stages
        disconnectStages();
        
        // Stop metrics timer
        d->metricsTimer->stop();
        
        emit pipelineStopped();
    }
}

void PreProcessingPipeline::flush() {
    for (auto* stage : d->stages) {
        stage->flushBuffers();
    }
}

bool PreProcessingPipeline::isRunning() const {
    return d->isRunning;
}

// Performance
void PreProcessingPipeline::setGPUEnabled(bool enable) {
    d->gpuEnabled = enable;
    
    // Enable GPU for all stages
    for (auto* stage : d->stages) {
        stage->setGPUEnabled(enable);
    }
}

bool PreProcessingPipeline::isGPUEnabled() const {
    return d->gpuEnabled;
}

void PreProcessingPipeline::setBatchSize(int size) {
    d->batchSize = size;
}

int PreProcessingPipeline::getBatchSize() const {
    return d->batchSize;
}

// Statistics
PreProcessingPipeline::PipelineMetrics PreProcessingPipeline::getMetrics() const {
    PipelineMetrics metrics;
    
    QMutexLocker lock(&d->metricsMutex);
    
    metrics.processedFrames = d->processedFrames;
    metrics.droppedFrames = d->droppedFrames;
    
    // Calculate throughput
    if (metrics.processedFrames > 0) {
        // TODO: Track actual time span
        metrics.throughput = metrics.processedFrames;  // Simplified
    }
    
    // Calculate stage latencies
    metrics.totalLatency = 0;
    for (auto it = d->stageLatencies.begin(); it != d->stageLatencies.end(); ++it) {
        if (!it.value().isEmpty()) {
            qint64 sum = 0;
            for (qint64 latency : it.value()) {
                sum += latency;
            }
            double avgLatency = sum / it.value().size() / 1000.0;  // Convert to ms
            metrics.stageLatencies[it.key()] = avgLatency;
            metrics.totalLatency += avgLatency;
        }
    }
    
    // TODO: Add CPU/GPU usage monitoring
    metrics.cpuUsage = 0.0;
    metrics.gpuUsage = 0.0;
    metrics.memoryUsage = 0;
    
    return metrics;
}

void PreProcessingPipeline::resetMetrics() {
    QMutexLocker lock(&d->metricsMutex);
    
    d->processedFrames = 0;
    d->droppedFrames = 0;
    d->stageLatencies.clear();
}

// Slot implementations
void PreProcessingPipeline::processSingle(const QImage& image, const FrameMetadata& metadata) {
    if (!d->isRunning || d->isPaused) {
        return;
    }
    
    // Convert to cv::Mat
    cv::Mat input = PreProcessorBase::qImageToMat(image);
    cv::Mat result;
    
    // Execute based on mode
    switch (d->executionMode) {
    case ExecutionMode::Sequential:
        result = executeSequential(input);
        break;
        
    case ExecutionMode::Parallel:
        result = executeParallel(input);
        break;
        
    case ExecutionMode::Conditional:
        result = executeConditional(input);
        break;
        
    case ExecutionMode::Adaptive:
        result = executeAdaptive(input);
        break;
    }
    
    if (!result.empty()) {
        // Convert back to QImage and emit
        QImage outputImage = PreProcessorBase::matToQImage(result);
        emit pipelineCompleted(outputImage, metadata);
        
        d->processedFrames++;
    }
}

void PreProcessingPipeline::processBatch(const QList<QImage>& images) {
    if (!d->isRunning || d->isPaused) {
        return;
    }
    
    QList<QImage> results;
    
    // Process each image
    for (const QImage& image : images) {
        FrameMetadata metadata;
        metadata.timestamp = QDateTime::currentMSecsSinceEpoch();
        
        cv::Mat input = PreProcessorBase::qImageToMat(image);
        cv::Mat result = executeSequential(input);  // Use sequential for batch
        
        if (!result.empty()) {
            results.append(PreProcessorBase::matToQImage(result));
        }
    }
    
    if (!results.isEmpty()) {
        emit batchCompleted(results);
        d->processedFrames += results.size();
    }
}

void PreProcessingPipeline::updateConfiguration(const QVariantMap& config) {
    // Update pipeline configuration
    if (config.contains("executionMode")) {
        QString mode = config["executionMode"].toString();
        if (mode == "sequential") {
            setExecutionMode(ExecutionMode::Sequential);
        } else if (mode == "parallel") {
            setExecutionMode(ExecutionMode::Parallel);
        } else if (mode == "conditional") {
            setExecutionMode(ExecutionMode::Conditional);
        } else if (mode == "adaptive") {
            setExecutionMode(ExecutionMode::Adaptive);
        }
    }
    
    if (config.contains("bufferSize")) {
        setBufferSize(config["bufferSize"].toInt());
    }
    
    if (config.contains("maxThreads")) {
        setMaxThreads(config["maxThreads"].toInt());
    }
    
    if (config.contains("gpuEnabled")) {
        setGPUEnabled(config["gpuEnabled"].toBool());
    }
    
    // Update stage configurations
    if (config.contains("stages")) {
        QVariantList stageConfigs = config["stages"].toList();
        for (int i = 0; i < stageConfigs.size() && i < d->stages.size(); ++i) {
            d->stages[i]->configure(stageConfigs[i].toMap());
        }
    }
}

void PreProcessingPipeline::onStageCompleted(const QImage& result, const FrameMetadata& metadata) {
    // Handle stage completion
    auto* stage = qobject_cast<PreProcessorBase*>(sender());
    if (stage) {
        QString stageName = stage->getName();
        
        // Record timing
        QMutexLocker lock(&d->metricsMutex);
        // TODO: Implement proper timing
        
        emit stageCompleted(stageName, 0);
    }
}

void PreProcessingPipeline::onStageError(ProcessingError error, const QString& details) {
    auto* stage = qobject_cast<PreProcessorBase*>(sender());
    if (stage) {
        emit pipelineError(stage->getName(), details);
    }
}

void PreProcessingPipeline::updateMetrics() {
    emit metricsUpdated(getMetrics());
}

// Protected virtual methods
void PreProcessingPipeline::connectStages() {
    if (d->stages.isEmpty()) return;
    
    // Connect stages in sequence by default
    for (int i = 0; i < d->stages.size() - 1; ++i) {
        connect(d->stages[i], &PreProcessorBase::frameProcessed,
                d->stages[i + 1], &PreProcessorBase::onFrameReceived,
                Qt::DirectConnection);
    }
    
    // Connect last stage to pipeline output
    if (!d->stages.isEmpty()) {
        connect(d->stages.last(), &PreProcessorBase::frameProcessed,
                this, &PreProcessingPipeline::pipelineCompleted,
                Qt::DirectConnection);
    }
    
    // Connect error signals
    for (auto* stage : d->stages) {
        connect(stage, &PreProcessorBase::processingError,
                this, &PreProcessingPipeline::onStageError);
    }
}

void PreProcessingPipeline::disconnectStages() {
    // Disconnect all stage connections
    for (auto* stage : d->stages) {
        disconnect(stage, nullptr, this, nullptr);
        disconnect(stage, nullptr, nullptr, nullptr);
    }
}

cv::Mat PreProcessingPipeline::executeSequential(const cv::Mat& input) {
    cv::Mat result = input;
    
    for (auto* stage : d->stages) {
        if (!stage->getParameter("_enabled").toBool()) {
            continue;  // Skip disabled stages
        }
        
        auto startTime = std::chrono::high_resolution_clock::now();
        
        // Process through stage (direct call for efficiency)
        // Note: In real implementation, we'd need to access the protected method
        // For now, we'll use the signal/slot mechanism
        
        // This is a simplified version - actual implementation would need
        // to properly invoke the processing
        QImage img = PreProcessorBase::matToQImage(result);
        FrameMetadata metadata;
        stage->onFrameReceived(img, metadata);
        
        // Wait for result (simplified - real implementation needs proper synchronization)
        // result = ...
        
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>
                       (endTime - startTime).count();
        
        // Record stage latency
        QMutexLocker lock(&d->metricsMutex);
        d->stageLatencies[stage->getName()].append(duration);
        if (d->stageLatencies[stage->getName()].size() > 100) {
            d->stageLatencies[stage->getName()].removeFirst();
        }
    }
    
    return result;
}

cv::Mat PreProcessingPipeline::executeParallel(const cv::Mat& input) {
    // Parallel execution for independent stages
    // This is a simplified implementation
    return executeSequential(input);  // Fall back to sequential for now
}

cv::Mat PreProcessingPipeline::executeConditional(const cv::Mat& input) {
    // Conditional execution based on image properties
    // This is a simplified implementation
    return executeSequential(input);  // Fall back to sequential for now
}

cv::Mat PreProcessingPipeline::executeAdaptive(const cv::Mat& input) {
    // Adaptive execution based on performance metrics
    // This is a simplified implementation
    return executeSequential(input);  // Fall back to sequential for now
}

// SequentialPipeline implementation
SequentialPipeline::SequentialPipeline(const QString& name, QObject* parent)
    : PreProcessingPipeline(name, parent) {
    setExecutionMode(ExecutionMode::Sequential);
}

void SequentialPipeline::connectStages() {
    PreProcessingPipeline::connectStages();
}

// Static factory members
QMap<QString, std::function<PreProcessorBase*()>> PipelineStageFactory::s_stageFactories;
QMap<QString, std::function<PreProcessingPipeline*(const QString&)>> PipelineStageFactory::s_pipelineFactories;

PreProcessorBase* PipelineStageFactory::createStage(const QString& type, const QVariantMap& config) {
    auto it = s_stageFactories.find(type);
    if (it != s_stageFactories.end()) {
        PreProcessorBase* stage = it.value()();
        if (stage) {
            stage->configure(config);
        }
        return stage;
    }
    return nullptr;
}

PreProcessingPipeline* PipelineStageFactory::createPipeline(const QString& type, const QString& name) {
    auto it = s_pipelineFactories.find(type);
    if (it != s_pipelineFactories.end()) {
        return it.value()(name);
    }
    
    // Default pipelines
    if (type == "sequential") {
        return new SequentialPipeline(name);
    } else if (type == "parallel") {
        return new ParallelPipeline(name);
    } else if (type == "conditional") {
        return new ConditionalPipeline(name);
    } else if (type == "adaptive") {
        return new AdaptivePipeline(name);
    }
    
    return nullptr;
}

void PipelineStageFactory::registerStageType(const QString& type,
                                            std::function<PreProcessorBase*()> factory) {
    s_stageFactories[type] = factory;
}

void PipelineStageFactory::registerPipelineType(const QString& type,
                                               std::function<PreProcessingPipeline*(const QString&)> factory) {
    s_pipelineFactories[type] = factory;
}