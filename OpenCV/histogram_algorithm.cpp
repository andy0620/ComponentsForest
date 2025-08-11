#include "histogram_algorithm.h"
#include <QDebug>
#include <QVariantList>
#include <vector>

namespace ComponentsForest {
namespace OpenCV {

HistogramAlgorithm::HistogramAlgorithm(QObject* parent)
    : AlgorithmBase("Histogram", parent),
      m_histSize(256)
{
    m_range[0] = 0.0f;
    m_range[1] = 256.0f; // The upper boundary is exclusive
    registerParameter("hist_size", m_histSize, "Number of bins in the histogram.");
}

bool HistogramAlgorithm::validateInput(const cv::Mat& input)
{
    if (input.empty()) {
        qWarning() << "HistogramAlgorithm: Input image is empty.";
        return false;
    }
    return true;
}

QVariant HistogramAlgorithm::processImplementation(const cv::Mat& input)
{
    std::vector<cv::Mat> channels;
    cv::split(input, channels);

    QVariantList histograms;

    for (size_t i = 0; i < channels.size(); ++i) {
        cv::Mat hist;
        const float* histRange = { m_range };
        bool uniform = true;
        bool accumulate = false;

        cv::calcHist(&channels[i], 1, 0, cv::Mat(), hist, 1, &m_histSize, &histRange, uniform, accumulate);

        // Normalize the result to be between 0 and 1 for easier visualization
        cv::normalize(hist, hist, 0, 1, cv::NORM_MINMAX, -1, cv::Mat());

        // Convert the histogram (cv::Mat) to a QVariantList of floats
        QVariantList channelHist;
        for (int j = 0; j < m_histSize; ++j) {
            channelHist.append(hist.at<float>(j));
        }
        histograms.append(QVariant(channelHist));
    }

    if (isDebugMode()) {
        qDebug() << "Calculated histogram for" << channels.size() << "channels.";
    }

    return histograms;
}

} // namespace OpenCV
} // namespace ComponentsForest
