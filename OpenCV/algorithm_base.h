#pragma once

#include "../components/base_component.h"
#include <QImage>
#include <QVariant>
#include <opencv2/opencv.hpp>

namespace ComponentsForest {
namespace OpenCV {

/**
 * @brief Base class for all image analysis algorithm components.
 *
 * This class provides the foundation for algorithms that take an image
 * as input and produce some form of data (e.g., numbers, points, structs)
 * as output. It inherits from BaseComponent to get standard features like
 * threading, state management, and configuration.
 */
class AlgorithmBase : public BaseComponent {
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param name Component name for identification
     * @param parent Parent QObject
     */
    explicit AlgorithmBase(const QString& name, QObject* parent = nullptr);
    virtual ~AlgorithmBase() = default;

    // BaseComponent interface override
    QString componentType() const override { return "Algorithm"; }

public slots:
    /**
     * @brief Input slot to receive an image for processing.
     * @param frame The input image.
     */
    void onFrameReceived(const QImage& frame);

signals:
    /**
     * @brief Emitted when the algorithm has finished processing.
     * @param result The result of the algorithm, wrapped in a QVariant.
     *               This can be any data type (e.g., double, QRectF, QList<QPointF>).
     */
    void resultReady(const QVariant& result);

    /**
     * @brief Emitted when an error occurs during processing.
     * @param details A string describing the error.
     */
    void algorithmError(const QString& details);

protected:
    // --- Pure virtual methods for subclasses to implement ---

    /**
     * @brief The core processing logic of the algorithm.
     * @param input The input image as an OpenCV Mat.
     * @return The calculated result wrapped in a QVariant.
     */
    [[nodiscard]] virtual QVariant processImplementation(const cv::Mat& input) = 0;

    // --- Optional virtual methods for subclasses ---

    /**
     * @brief Validates if the input image is suitable for this algorithm.
     * @param input The input cv::Mat to validate.
     * @return True if the input is valid, false otherwise.
     */
    virtual bool validateInput(const cv::Mat& input);

    /**
     * @brief Converts a QImage to a cv::Mat for processing.
     * @param image The input QImage.
     * @return The converted cv::Mat.
     */
    virtual cv::Mat qImageToMat(const QImage& image);

private:
    /**
     * @brief Internal processing function that calls the implementation.
     * @param frame The QImage to process.
     */
    void processFrame(const QImage& frame);
};

} // namespace OpenCV
} // namespace ComponentsForest
