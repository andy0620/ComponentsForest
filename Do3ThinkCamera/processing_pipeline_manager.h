#ifndef PROCESSING_PIPELINE_MANAGER_H
#define PROCESSING_PIPELINE_MANAGER_H

#include <QObject>
#include <QImage>
#include <QStringList>
#include <QVariantMap>
#include <memory>

// Forward declarations
namespace ComponentsForest {
namespace OpenCV {
class PreProcessorBase;
class PreProcessingPipeline;
}
}

class ProcessingPipelineManager : public QObject {
    Q_OBJECT

public:
    explicit ProcessingPipelineManager(QObject* parent = nullptr);
    ~ProcessingPipelineManager();

    void initialize();
    void cleanup();

    QImage processImage(const QImage& inputImage);

public slots:
    void enableImageProcessing(bool enable);
    bool isImageProcessingEnabled() const;

    void enablePreprocessor(const QString& type, bool enable);
    bool isPreprocessorEnabled(const QString& type) const;

    void setPreprocessorParameter(const QString& type, const QString& parameter, const QVariant& value);
    QVariant getPreprocessorParameter(const QString& type, const QString& parameter) const;

    void setProcessingOrder(const QStringList& order);
    QStringList getProcessingOrder() const;

signals:
    void processingEnabledChanged(bool enabled);
    void preprocessorEnabledChanged(const QString& type, bool enabled);
    void processingOrderChanged(const QStringList& order);
    void processingPerformanceUpdate(double fps, double latency);
    void errorOccurred(const QString& error);

private:
    void updateProcessingPipeline();
    void setupPreprocessorConnections();

    struct ImageProcessingState {
        bool processingEnabled{false};
        bool blurEnabled{false};
        bool edgeEnabled{false};
        bool denoiseEnabled{false};
        QStringList processingOrder;
        QMutex processingMutex;
        double processingLatency{0.0};
    } m_processingState;

    std::unique_ptr<ComponentsForest::OpenCV::PreProcessingPipeline> m_processingPipeline;
    std::unique_ptr<ComponentsForest::OpenCV::PreProcessorBase> m_blurProcessor;
    std::unique_ptr<ComponentsForest::OpenCV::PreProcessorBase> m_edgeProcessor;
    std::unique_ptr<ComponentsForest::OpenCV::PreProcessorBase> m_denoiseProcessor;
};

#endif // PROCESSING_PIPELINE_MANAGER_H
