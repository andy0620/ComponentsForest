#include "algorithm_base.h"
#include <QDebug>

namespace ComponentsForest {
namespace OpenCV {

AlgorithmBase::AlgorithmBase(const QString& name, QObject* parent)
    : BaseComponent(parent) {
    setComponentName(name);
}

void AlgorithmBase::onFrameReceived(const QImage& frame) {
    // This slot is the entry point for processing.
    // In a more complex implementation, this would push the frame
    // to a queue that is processed by a separate worker thread.
    // For simplicity, we process it directly here.
    if (isRunning()) {
        processFrame(frame);
    }
}

void AlgorithmBase::processFrame(const QImage& frame) {
    try {
        cv::Mat inputMat = qImageToMat(frame);
        if (!validateInput(inputMat)) {
            emit algorithmError("Invalid input image.");
            return;
        }

        QVariant result = processImplementation(inputMat);

        // Only emit if the result is valid. An empty QVariant is considered invalid.
        if (result.isValid()) {
            emit resultReady(result);
        }
    } catch (const cv::Exception& e) {
        QString errorMsg = QString("OpenCV exception: %1").arg(e.what());
        qWarning() << errorMsg;
        emit algorithmError(errorMsg);
    } catch (const std::exception& e) {
        QString errorMsg = QString("Standard exception: %1").arg(e.what());
        qWarning() << errorMsg;
        emit algorithmError(errorMsg);
    }
}

bool AlgorithmBase::validateInput(const cv::Mat& input) {
    // Default implementation: just check if the image is empty.
    // Subclasses can override this for more specific checks (e.g., type, channels).
    if (input.empty()) {
        qWarning() << componentName() << "received an empty image.";
        return false;
    }
    return true;
}

cv::Mat AlgorithmBase::qImageToMat(const QImage& image) {
    // This is a simplified conversion. A real implementation would need to handle
    // different QImage formats more robustly.
    switch (image.format()) {
        case QImage::Format_RGB32:
        case QImage::Format_ARGB32:
        case QImage::Format_ARGB32_Premultiplied: {
            cv::Mat mat(image.height(), image.width(), CV_8UC4, const_cast<uchar*>(image.bits()), image.bytesPerLine());
            return mat.clone(); // Clone to have a mutable copy
        }
        case QImage::Format_RGB888: {
            cv::Mat mat(image.height(), image.width(), CV_8UC3, const_cast<uchar*>(image.bits()), image.bytesPerLine());
            cv::cvtColor(mat, mat, cv::COLOR_RGB2BGR);
            return mat;
        }
        case QImage::Format_Grayscale8:
        case QImage::Format_Indexed8: {
            cv::Mat mat(image.height(), image.width(), CV_8UC1, const_cast<uchar*>(image.bits()), image.bytesPerLine());
            return mat.clone();
        }
        default:
            qWarning() << "Unsupported QImage format for conversion:" << image.format();
            break;
    }
    return cv::Mat();
}

} // namespace OpenCV
} // namespace ComponentsForest
