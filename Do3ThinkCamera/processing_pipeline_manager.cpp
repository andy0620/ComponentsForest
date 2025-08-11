#include "processing_pipeline_manager.h"
#include "../OpenCV/preprocessor_base.h"
#include "../OpenCV/preprocessing_pipeline.h"
#include "../OpenCV/simple_edge_preprocessor.h" // Assuming concrete types are needed here
#include "../OpenCV/blur_preprocessor.h"
#include "../OpenCV/denoise_preprocessor.h"

#include <QElapsedTimer>
#include <QDebug>

#define DEBUG_LOG(msg) qDebug() << "[PipelineManager]" << msg

ProcessingPipelineManager::ProcessingPipelineManager(QObject* parent) : QObject(parent) {
    DEBUG_LOG("Constructor");
}

ProcessingPipelineManager::~ProcessingPipelineManager() {
    DEBUG_LOG("Destructor");
    cleanup();
}

void ProcessingPipelineManager::initialize() {
    DEBUG_LOG("Initializing...");
    try {
        using namespace ComponentsForest::OpenCV;

        m_processingPipeline = std::make_unique<SequentialPipeline>("Do3ThinkProcessing");

        // Create concrete preprocessors
        m_blurProcessor = std::make_unique<BlurPreProcessor>();
        m_edgeProcessor = std::make_unique<SimpleEdgePreProcessor>();
        m_denoiseProcessor = std::make_unique<DenoisePreProcessor>();

        // Initialize them (they are also BaseComponents)
        m_blurProcessor->initialize(QJsonObject());
        m_edgeProcessor->initialize(QJsonObject());
        m_denoiseProcessor->initialize(QJsonObject());

        setupPreprocessorConnections();
        DEBUG_LOG("Initialized successfully");
    } catch (const std::exception& e) {
        DEBUG_LOG("Exception during initialization:" << e.what());
    }
}

void ProcessingPipelineManager::cleanup() {
    DEBUG_LOG("Cleaning up...");
    try {
        if (m_processingPipeline) {
            m_processingPipeline.reset();
        }
        m_blurProcessor.reset();
        m_edgeProcessor.reset();
        m_denoiseProcessor.reset();
        DEBUG_LOG("Cleanup successful");
    } catch (const std::exception& e) {
        DEBUG_LOG("Exception during cleanup:" << e.what());
    }
}

QImage ProcessingPipelineManager::processImage(const QImage& inputImage) {
    if (!m_processingState.processingEnabled || !m_processingPipeline || m_processingPipeline->isEmpty()) {
        return inputImage;
    }

    QElapsedTimer timer;
    timer.start();

    try {
        // The pipeline should internally convert QImage to cv::Mat, process, and convert back.
        // This logic resides within the PreProcessingPipeline class.
        QImage processedImage = m_processingPipeline->process(inputImage);

        m_processingState.processingLatency = timer.elapsed();

        static int frameCount = 0;
        if (++frameCount % 30 == 0) { // Every 30 frames
            double fps = 1000.0 / qMax(1.0, m_processingState.processingLatency);
            emit processingPerformanceUpdate(fps, m_processingState.processingLatency);
        }

        return processedImage;

    } catch (const std::exception& e) {
        DEBUG_LOG("Exception processing image: " << e.what());
        emit errorOccurred(QString("Processing error: %1").arg(e.what()));
        return inputImage;
    }
}

void ProcessingPipelineManager::enableImageProcessing(bool enable) {
    QMutexLocker locker(&m_processingState.processingMutex);
    if (m_processingState.processingEnabled == enable) return;
    m_processingState.processingEnabled = enable;
    DEBUG_LOG("Image processing" << (enable ? "enabled" : "disabled"));
    emit processingEnabledChanged(enable);
}

bool ProcessingPipelineManager::isImageProcessingEnabled() const {
    return m_processingState.processingEnabled;
}

void ProcessingPipelineManager::enablePreprocessor(const QString& type, bool enable) {
    QMutexLocker locker(&m_processingState.processingMutex);
    bool changed = false;
    if (type.toLower() == "blur") {
        if (m_processingState.blurEnabled != enable) {
            m_processingState.blurEnabled = enable;
            changed = true;
        }
    } else if (type.toLower() == "edge") {
        if (m_processingState.edgeEnabled != enable) {
            m_processingState.edgeEnabled = enable;
            changed = true;
        }
    } else if (type.toLower() == "denoise") {
        if (m_processingState.denoiseEnabled != enable) {
            m_processingState.denoiseEnabled = enable;
            changed = true;
        }
    }

    if (changed) {
        DEBUG_LOG(type << "preprocessor" << (enable ? "enabled" : "disabled"));
        updateProcessingPipeline();
        emit preprocessorEnabledChanged(type, enable);
    }
}

bool ProcessingPipelineManager::isPreprocessorEnabled(const QString& type) const {
    if (type.toLower() == "blur") return m_processingState.blurEnabled;
    if (type.toLower() == "edge") return m_processingState.edgeEnabled;
    if (type.toLower() == "denoise") return m_processingState.denoiseEnabled;
    return false;
}

void ProcessingPipelineManager::setPreprocessorParameter(const QString& type, const QString& parameter, const QVariant& value) {
    using namespace ComponentsForest::OpenCV;
    PreProcessorBase* processor = nullptr;
    if (type.toLower() == "blur") processor = m_blurProcessor.get();
    else if (type.toLower() == "edge") processor = m_edgeProcessor.get();
    else if (type.toLower() == "denoise") processor = m_denoiseProcessor.get();

    if (processor) {
        processor->setParameter(parameter, value);
        DEBUG_LOG("Set parameter" << parameter << "for" << type << "to" << value);
    }
}

QVariant ProcessingPipelineManager::getPreprocessorParameter(const QString& type, const QString& parameter) const {
    using namespace ComponentsForest::OpenCV;
    const PreProcessorBase* processor = nullptr;
    if (type.toLower() == "blur") processor = m_blurProcessor.get();
    else if (type.toLower() == "edge") processor = m_edgeProcessor.get();
    else if (type.toLower() == "denoise") processor = m_denoiseProcessor.get();

    if (processor) {
        return processor->getParameter(parameter);
    }
    return QVariant();
}

void ProcessingPipelineManager::setProcessingOrder(const QStringList& order) {
    QMutexLocker locker(&m_processingState.processingMutex);
    if (m_processingState.processingOrder == order) return;
    m_processingState.processingOrder = order;
    updateProcessingPipeline();
    emit processingOrderChanged(order);
    DEBUG_LOG("Processing order changed to:" << order.join(", "));
}

QStringList ProcessingPipelineManager::getProcessingOrder() const {
    return m_processingState.processingOrder;
}

void ProcessingPipelineManager::updateProcessingPipeline() {
    if (!m_processingPipeline) return;

    m_processingPipeline->clearStages();
    for (const QString& processorName : m_processingState.processingOrder) {
        if (processorName.toLower() == "blur" && m_processingState.blurEnabled) {
            m_processingPipeline->addStage(m_blurProcessor.get());
        } else if (processorName.toLower() == "edge" && m_processingState.edgeEnabled) {
            m_processingPipeline->addStage(m_edgeProcessor.get());
        } else if (processorName.toLower() == "denoise" && m_processingState.denoiseEnabled) {
            m_processingPipeline->addStage(m_denoiseProcessor.get());
        }
    }
    DEBUG_LOG("Pipeline updated with new stage order.");
}

void ProcessingPipelineManager::setupPreprocessorConnections() {
    auto connectError = [this](ComponentsForest::OpenCV::PreProcessorBase* processor, const QString& name){
        connect(processor, &ComponentsForest::OpenCV::PreProcessorBase::errorOccurred, this, [this, name](const QString& error){
            DEBUG_LOG(name << "processor error:" << error);
            emit this->errorOccurred(QString("%1: %2").arg(name).arg(error));
        });
    };

    if (m_blurProcessor) connectError(m_blurProcessor.get(), "Blur");
    if (m_edgeProcessor) connectError(m_edgeProcessor.get(), "Edge");
    if (m_denoiseProcessor) connectError(m_denoiseProcessor.get(), "Denoise");
}
