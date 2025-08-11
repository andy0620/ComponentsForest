#include "dothink_camera_roi_integration.h"
#include "dothink_camera.h"
#include "dothink_camera_control_panel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolBar>
#include <QAction>
#include <QGroupBox>
#include <QTableWidget>
#include <QSplitter>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDir>
#include <opencv2/imgproc.hpp>
#include "../OpenCV/blur_preprocessor.h"
#include "../OpenCV/edge_preprocessor.h"
#include "../OpenCV/denoise_preprocessor.h"

namespace Do3Think {

// Namespace alias for convenience
namespace CF_OpenCV = ComponentsForest::OpenCV;

// CameraROIIntegration::Private implementation
class CameraROIIntegration::Private {
public:
    CF_OpenCV::ROISelectorWidget* roiSelector;
    CF_OpenCV::ROIManager* roiManager;
    QVector<std::shared_ptr<CF_OpenCV::PreProcessorBase>> preprocessors;
    bool selectionEnabled = true;
    bool preprocessingEnabled = true;
    
    Private() {
        roiSelector = new CF_OpenCV::ROISelectorWidget();
        roiManager = new CF_OpenCV::ROIManager();
    }
    
    ~Private() {
        delete roiSelector;
        delete roiManager;
    }
};

CameraROIIntegration::CameraROIIntegration(QWidget* parent)
    : QObject(parent), d(std::make_unique<Private>()) {
    connectSignals();
    setupDefaultPreprocessors();
}

CameraROIIntegration::~CameraROIIntegration() = default;

CF_OpenCV::ROISelectorWidget* CameraROIIntegration::getROISelectorWidget() const {
    return d->roiSelector;
}

CF_OpenCV::ROIManager* CameraROIIntegration::getROIManager() const {
    return d->roiManager;
}

QDockWidget* CameraROIIntegration::createROIDockWidget(const QString& title) {
    auto* dock = new QDockWidget(title);
    dock->setWidget(d->roiSelector);
    dock->setFeatures(QDockWidget::DockWidgetMovable | 
                     QDockWidget::DockWidgetFloatable |
                     QDockWidget::DockWidgetClosable);
    return dock;
}

void CameraROIIntegration::setROISelectionEnabled(bool enabled) {
    d->selectionEnabled = enabled;
    d->roiSelector->setEnabled(enabled);
}

cv::Mat CameraROIIntegration::processFrameWithROIs(const cv::Mat& frame) {
    if (!d->preprocessingEnabled || d->preprocessors.isEmpty()) {
        return frame;
    }
    
    cv::Mat result = frame.clone();
    auto rois = d->roiManager->getAllROIs();
    
    for (const auto& roi : rois) {
        // Extract ROI region
        cv::Mat roiRegion = roi->extract(frame);
        
        // Apply preprocessors to ROI region
        cv::Mat processed = roiRegion;
        for (const auto& preprocessor : d->preprocessors) {
            processed = preprocessor->processImplementation(processed);
        }
        
        // Copy processed region back to result
        QRect bounds = roi->getBoundingRect();
        cv::Rect cvBounds(bounds.x(), bounds.y(), bounds.width(), bounds.height());
        
        // Ensure bounds are within frame
        cvBounds &= cv::Rect(0, 0, frame.cols, frame.rows);
        if (cvBounds.area() > 0) {
            processed.copyTo(result(cvBounds));
        }
    }
    
    return result;
}

void CameraROIIntegration::addPreprocessor(std::shared_ptr<CF_OpenCV::PreProcessorBase> preprocessor) {
    d->preprocessors.append(preprocessor);
}

void CameraROIIntegration::clearPreprocessors() {
    d->preprocessors.clear();
}

QVector<CameraROIIntegration::ROIStatistics> 
CameraROIIntegration::calculateROIStatistics(const cv::Mat& frame) {
    QVector<ROIStatistics> stats;
    auto rois = d->roiManager->getAllROIs();
    
    for (const auto& roi : rois) {
        ROIStatistics stat;
        stat.roiId = id;
        
        // Extract ROI region
        cv::Mat roiRegion = roi->extract(frame);
        if (roiRegion.empty()) continue;
        
        // Calculate mean and standard deviation
        cv::Scalar mean, stddev;
        cv::meanStdDev(roiRegion, mean, stddev);
        
        stat.meanColor = mean;
        stat.stdDev = (stddev[0] + stddev[1] + stddev[2]) / 3.0;
        stat.pixelCount = roiRegion.rows * roiRegion.cols;
        
        // Calculate mean intensity (grayscale)
        cv::Mat gray;
        if (roiRegion.channels() == 3) {
            cv::cvtColor(roiRegion, gray, cv::COLOR_BGR2GRAY);
        } else {
            gray = roiRegion;
        }
        stat.meanIntensity = cv::mean(gray)[0];
        
        // Calculate contrast (standard deviation of grayscale)
        cv::Scalar grayMean, grayStddev;
        cv::meanStdDev(gray, grayMean, grayStddev);
        stat.contrast = grayStddev[0];
        
        // Calculate sharpness (Laplacian variance)
        cv::Mat laplacian;
        cv::Laplacian(gray, laplacian, CV_64F);
        cv::Scalar lapMean, lapStddev;
        cv::meanStdDev(laplacian, lapMean, lapStddev);
        stat.sharpness = lapStddev[0] * lapStddev[0];
        
        stats.append(stat);
    }
    
    emit roiStatisticsReady(stats);
    return stats;
}

void CameraROIIntegration::updateDisplayImage(const QImage& image) {
    d->roiSelector->setBackgroundImage(image);
}

void CameraROIIntegration::updateDisplayImage(const cv::Mat& frame) {
    QImage qimg;
    if (frame.channels() == 3) {
        cv::Mat rgb;
        cv::cvtColor(frame, rgb, cv::COLOR_BGR2RGB);
        qimg = QImage(rgb.data, rgb.cols, rgb.rows, rgb.step, QImage::Format_RGB888);
    } else if (frame.channels() == 1) {
        qimg = QImage(frame.data, frame.cols, frame.rows, frame.step, QImage::Format_Grayscale8);
    }
    
    updateDisplayImage(qimg.copy());
}

void CameraROIIntegration::saveROIConfiguration(const QString& filepath) {
    QJsonObject config;
    QJsonArray roiArray;
    
    auto rois = d->roiManager->getAllROIs();
    for (const auto& roi : rois) {
        QJsonObject roiObj;
        roiObj["id"] = id;
        roiObj["type"] = QString::number(static_cast<int>(roi->getType()));
        
        QRect bounds = roi->getBoundingRect();
        roiObj["x"] = bounds.x();
        roiObj["y"] = bounds.y();
        roiObj["width"] = bounds.width();
        roiObj["height"] = bounds.height();
        
        roiArray.append(roiObj);
    }
    
    config["rois"] = roiArray;
    config["version"] = "1.0";
    
    QJsonDocument doc(config);
    QFile file(filepath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        file.close();
    }
}

void CameraROIIntegration::loadROIConfiguration(const QString& filepath) {
    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly)) return;
    
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    
    QJsonObject config = doc.object();
    QJsonArray roiArray = config["rois"].toArray();
    
    d->roiSelector->clearAllROIs();
    
    for (const auto& value : roiArray) {
        QJsonObject roiObj = value.toObject();
        QString id = roiObj["id"].toString();
        int type = roiObj["type"].toString().toInt();
        int x = roiObj["x"].toInt();
        int y = roiObj["y"].toInt();
        int width = roiObj["width"].toInt();
        int height = roiObj["height"].toInt();
        
        // Create ROI based on type
        if (type == static_cast<int>(CF_OpenCV::ROIType::Rectangle)) {
            auto roi = std::make_shared<CF_OpenCV::RectangleROI>(QRect(x, y, width, height));
            d->roiManager->addROI(id, roi);
        } else if (type == static_cast<int>(CF_OpenCV::ROIType::Circle)) {
            auto roi = std::make_shared<CF_OpenCV::CircleROI>(
                QPoint(x + width/2, y + height/2), std::min(width, height)/2);
            d->roiManager->addROI(id, roi);
        }
    }
}

void CameraROIIntegration::exportROIMasks(const QString& directory) {
    QDir dir(directory);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    
    auto rois = d->roiManager->getAllROIs();
    QImage baseImage = d->roiSelector->getBackgroundImage();
    
    for (const auto& roi : rois) {
        // Create mask
        cv::Mat mask = cv::Mat::zeros(baseImage.height(), baseImage.width(), CV_8UC1);
        roi->drawMask(mask);
        
        // Save mask
        QString filename = dir.absoluteFilePath(QString("%1_mask.png").arg(id));
        cv::imwrite(filename.toStdString(), mask);
    }
}

void CameraROIIntegration::connectSignals() {
    connect(d->roiSelector, &CF_OpenCV::ROISelectorWidget::roiCreated,
            [this](const QString& id) {
                auto roi = d->roiSelector->getROI(id);
                if (roi) {
                    emit roiCreated(id, roi->getBoundingRect());
                }
            });
    
    connect(d->roiSelector, &CF_OpenCV::ROISelectorWidget::roiModified,
            [this](const QString& id) {
                auto roi = d->roiSelector->getROI(id);
                if (roi) {
                    emit roiModified(id, roi->getBoundingRect());
                }
            });
    
    connect(d->roiSelector, &CF_OpenCV::ROISelectorWidget::roiDeleted,
            this, &CameraROIIntegration::roiDeleted);
}

void CameraROIIntegration::setupDefaultPreprocessors() {
    // Add some default preprocessors
    // These can be customized by the user
}

// Do3ThinkCameraControlPanelWithROI::Private implementation
class Do3ThinkCameraControlPanelWithROI::Private {
public:
    CameraROIIntegration* roiIntegration;
    Do3ThinkCameraControlPanel* basePanel;
    QToolBar* roiToolbar;
    QTableWidget* statsTable;
    bool roiOverlayEnabled = true;
    bool roiPreprocessingEnabled = false;
    
    Private() {
        roiIntegration = new CameraROIIntegration();
        basePanel = new Do3ThinkCameraControlPanel();
    }
    
    ~Private() {
        delete roiIntegration;
        delete basePanel;
    }
};

Do3ThinkCameraControlPanelWithROI::Do3ThinkCameraControlPanelWithROI(QWidget* parent)
    : QWidget(parent), d(std::make_unique<Private>()) {
    setupUI();
    createROIToolbar();
    createROIStatisticsPanel();
    
    // Connect frame updates
    connect(d->basePanel, SIGNAL(frameReceived(QImage)),
            this, SLOT(onFrameReceived(QImage)));
}

Do3ThinkCameraControlPanelWithROI::~Do3ThinkCameraControlPanelWithROI() = default;

void Do3ThinkCameraControlPanelWithROI::setCameraComponent(Do3ThinkCameraComponent* component) {
    d->basePanel->setCameraComponent(component);
}

CameraROIIntegration* Do3ThinkCameraControlPanelWithROI::getROIIntegration() const {
    return d->roiIntegration;
}

void Do3ThinkCameraControlPanelWithROI::setROIOverlayEnabled(bool enabled) {
    d->roiOverlayEnabled = enabled;
}

void Do3ThinkCameraControlPanelWithROI::setROIPreprocessingEnabled(bool enabled) {
    d->roiPreprocessingEnabled = enabled;
}

void Do3ThinkCameraControlPanelWithROI::setROIStatisticsVisible(bool visible) {
    if (d->statsTable) {
        d->statsTable->setVisible(visible);
    }
}

void Do3ThinkCameraControlPanelWithROI::onFrameReceived(const QImage& frame) {
    if (d->roiPreprocessingEnabled) {
        // Convert to cv::Mat for processing
        cv::Mat cvFrame(frame.height(), frame.width(), CV_8UC3,
                       const_cast<uchar*>(frame.bits()), frame.bytesPerLine());
        cv::Mat processed = d->roiIntegration->processFrameWithROIs(cvFrame);
        
        // Convert back to QImage
        QImage processedImage(processed.data, processed.cols, processed.rows,
                            processed.step, QImage::Format_RGB888);
        
        d->roiIntegration->updateDisplayImage(processedImage.copy());
        emit processedFrameReady(processedImage.copy());
    } else {
        d->roiIntegration->updateDisplayImage(frame);
        emit processedFrameReady(frame);
    }
    
    // Update statistics if visible
    if (d->statsTable && d->statsTable->isVisible()) {
        cv::Mat cvFrame(frame.height(), frame.width(), CV_8UC3,
                       const_cast<uchar*>(frame.bits()), frame.bytesPerLine());
        auto stats = d->roiIntegration->calculateROIStatistics(cvFrame);
        
        // Update stats table
        d->statsTable->setRowCount(stats.size());
        for (int i = 0; i < stats.size(); ++i) {
            const auto& stat = stats[i];
            d->statsTable->setItem(i, 0, new QTableWidgetItem(stat.roiId));
            d->statsTable->setItem(i, 1, new QTableWidgetItem(
                QString::number(stat.meanIntensity, 'f', 1)));
            d->statsTable->setItem(i, 2, new QTableWidgetItem(
                QString::number(stat.contrast, 'f', 1)));
            d->statsTable->setItem(i, 3, new QTableWidgetItem(
                QString::number(stat.sharpness, 'f', 2)));
        }
    }
}

void Do3ThinkCameraControlPanelWithROI::toggleROIMode() {
    bool enabled = d->roiIntegration->getROISelectorWidget()->isEnabled();
    d->roiIntegration->setROISelectionEnabled(!enabled);
    emit roiModeChanged(!enabled);
}

void Do3ThinkCameraControlPanelWithROI::clearAllROIs() {
    d->roiIntegration->getROISelectorWidget()->clearAllROIs();
}

void Do3ThinkCameraControlPanelWithROI::setupUI() {
    auto* layout = new QVBoxLayout(this);
    
    // Create splitter for base panel and ROI selector
    auto* splitter = new QSplitter(Qt::Horizontal, this);
    
    // Add base panel
    splitter->addWidget(d->basePanel);
    
    // Add ROI selector
    splitter->addWidget(d->roiIntegration->getROISelectorWidget());
    
    // Set initial sizes (60% for base panel, 40% for ROI)
    splitter->setSizes({600, 400});
    
    layout->addWidget(splitter);
}

void Do3ThinkCameraControlPanelWithROI::createROIToolbar() {
    d->roiToolbar = new QToolBar("ROI Tools", this);
    
    // ROI mode toggle
    auto* roiModeAction = d->roiToolbar->addAction("ROI Mode");
    roiModeAction->setCheckable(true);
    roiModeAction->setChecked(true);
    connect(roiModeAction, &QAction::triggered, this, &Do3ThinkCameraControlPanelWithROI::toggleROIMode);
    
    // ROI shape actions
    d->roiToolbar->addSeparator();
    
    auto* rectAction = d->roiToolbar->addAction("Rectangle");
    rectAction->setCheckable(true);
    connect(rectAction, &QAction::triggered, [this]() {
        d->roiIntegration->getROISelectorWidget()->setCreationMode(
            CF_OpenCV::ROISelectorWidget::CreationMode::Rectangle);
    });
    
    auto* circleAction = d->roiToolbar->addAction("Circle");
    circleAction->setCheckable(true);
    connect(circleAction, &QAction::triggered, [this]() {
        d->roiIntegration->getROISelectorWidget()->setCreationMode(
            CF_OpenCV::ROISelectorWidget::CreationMode::Circle);
    });
    
    auto* polygonAction = d->roiToolbar->addAction("Polygon");
    polygonAction->setCheckable(true);
    connect(polygonAction, &QAction::triggered, [this]() {
        d->roiIntegration->getROISelectorWidget()->setCreationMode(
            CF_OpenCV::ROISelectorWidget::CreationMode::Polygon);
    });
    
    // Clear action
    d->roiToolbar->addSeparator();
    auto* clearAction = d->roiToolbar->addAction("Clear All");
    connect(clearAction, &QAction::triggered, this, &Do3ThinkCameraControlPanelWithROI::clearAllROIs);
    
    // Preprocessing toggle
    d->roiToolbar->addSeparator();
    auto* preprocessAction = d->roiToolbar->addAction("Preprocessing");
    preprocessAction->setCheckable(true);
    connect(preprocessAction, &QAction::toggled, [this](bool checked) {
        setROIPreprocessingEnabled(checked);
    });
    
    // Add toolbar to layout
    static_cast<QVBoxLayout*>(layout())->insertWidget(0, d->roiToolbar);
}

void Do3ThinkCameraControlPanelWithROI::createROIStatisticsPanel() {
    auto* statsGroup = new QGroupBox("ROI Statistics", this);
    auto* statsLayout = new QVBoxLayout(statsGroup);
    
    d->statsTable = new QTableWidget(0, 4, statsGroup);
    d->statsTable->setHorizontalHeaderLabels({"ROI", "Mean", "Contrast", "Sharpness"});
    d->statsTable->horizontalHeader()->setStretchLastSection(true);
    d->statsTable->setAlternatingRowColors(true);
    
    statsLayout->addWidget(d->statsTable);
    
    // Add to main layout
    static_cast<QVBoxLayout*>(layout())->addWidget(statsGroup);
    
    // Initially hidden
    statsGroup->setVisible(false);
}

} // namespace Do3Think