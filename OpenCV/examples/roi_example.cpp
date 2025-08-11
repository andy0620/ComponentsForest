#include <QApplication>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QImage>
#include <QPainter>
#include <opencv2/opencv.hpp>
#include "../roi_selector_widget.h"
#include "../roi_manager.h"
#include "../roi_types.h"

using namespace OpenCV;

class ROIExampleWindow : public QMainWindow {
    Q_OBJECT
    
public:
    ROIExampleWindow(QWidget* parent = nullptr) : QMainWindow(parent) {
        setupUI();
        loadSampleImage();
        connectSignals();
    }
    
private:
    void setupUI() {
        auto* centralWidget = new QWidget(this);
        auto* layout = new QVBoxLayout(centralWidget);
        
        // ROI selector widget
        m_roiSelector = new ROISelectorWidget(this);
        m_roiSelector->setMinimumSize(800, 600);
        
        // Control buttons
        auto* buttonLayout = new QHBoxLayout();
        
        auto* rectButton = new QPushButton("Rectangle ROI", this);
        rectButton->setCheckable(true);
        connect(rectButton, &QPushButton::clicked, [this]() {
            m_roiSelector->setCreationMode(ROISelectorWidget::CreationMode::Rectangle);
        });
        
        auto* circleButton = new QPushButton("Circle ROI", this);
        circleButton->setCheckable(true);
        connect(circleButton, &QPushButton::clicked, [this]() {
            m_roiSelector->setCreationMode(ROISelectorWidget::CreationMode::Circle);
        });
        
        auto* polygonButton = new QPushButton("Polygon ROI", this);
        polygonButton->setCheckable(true);
        connect(polygonButton, &QPushButton::clicked, [this]() {
            m_roiSelector->setCreationMode(ROISelectorWidget::CreationMode::Polygon);
        });
        
        auto* clearButton = new QPushButton("Clear All", this);
        connect(clearButton, &QPushButton::clicked, [this]() {
            m_roiSelector->clearAllROIs();
        });
        
        auto* analyzeButton = new QPushButton("Analyze ROIs", this);
        connect(analyzeButton, &QPushButton::clicked, this, &ROIExampleWindow::analyzeROIs);
        
        buttonLayout->addWidget(rectButton);
        buttonLayout->addWidget(circleButton);
        buttonLayout->addWidget(polygonButton);
        buttonLayout->addWidget(clearButton);
        buttonLayout->addWidget(analyzeButton);
        buttonLayout->addStretch();
        
        // Status label
        m_statusLabel = new QLabel("Ready", this);
        m_statusLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);
        
        // Assemble layout
        layout->addWidget(m_roiSelector);
        layout->addLayout(buttonLayout);
        layout->addWidget(m_statusLabel);
        
        setCentralWidget(centralWidget);
        setWindowTitle("ROI Interactive Selection Example");
    }
    
    void loadSampleImage() {
        // Create a sample image with some patterns
        cv::Mat image(600, 800, CV_8UC3);
        
        // Create gradient background
        for (int y = 0; y < image.rows; ++y) {
            for (int x = 0; x < image.cols; ++x) {
                image.at<cv::Vec3b>(y, x) = cv::Vec3b(
                    255 * x / image.cols,  // Blue gradient
                    255 * y / image.rows,  // Green gradient
                    128                     // Constant red
                );
            }
        }
        
        // Add some circles
        cv::circle(image, cv::Point(200, 200), 50, cv::Scalar(255, 255, 0), -1);
        cv::circle(image, cv::Point(600, 200), 80, cv::Scalar(0, 255, 255), -1);
        cv::circle(image, cv::Point(400, 400), 60, cv::Scalar(255, 0, 255), -1);
        
        // Add rectangles
        cv::rectangle(image, cv::Rect(100, 400, 150, 100), cv::Scalar(0, 255, 0), -1);
        cv::rectangle(image, cv::Rect(550, 450, 200, 120), cv::Scalar(255, 128, 0), -1);
        
        // Convert to QImage and set as background
        QImage qimg(image.data, image.cols, image.rows, image.step, QImage::Format_RGB888);
        qimg = qimg.rgbSwapped();  // OpenCV uses BGR, Qt uses RGB
        
        m_roiSelector->setBackgroundImage(qimg);
    }
    
    void connectSignals() {
        // Connect ROI selector signals
        connect(m_roiSelector, &ROISelectorWidget::roiCreated, 
                [this](const QString& id) {
            m_statusLabel->setText(QString("ROI created: %1").arg(id));
        });
        
        connect(m_roiSelector, &ROISelectorWidget::roiSelected,
                [this](const QString& id) {
            m_statusLabel->setText(QString("ROI selected: %1").arg(id));
        });
        
        connect(m_roiSelector, &ROISelectorWidget::roiDeleted,
                [this](const QString& id) {
            m_statusLabel->setText(QString("ROI deleted: %1").arg(id));
        });
        
        connect(m_roiSelector, &ROISelectorWidget::roiModified,
                [this](const QString& id) {
            m_statusLabel->setText(QString("ROI modified: %1").arg(id));
        });
    }
    
private slots:
    void analyzeROIs() {
        auto rois = m_roiSelector->getAllROIs();
        
        if (rois.isEmpty()) {
            m_statusLabel->setText("No ROIs to analyze");
            return;
        }
        
        QString analysis = QString("Analyzing %1 ROIs:\n").arg(rois.size());
        
        // Get the background image as cv::Mat
        QImage qimg = m_roiSelector->getBackgroundImage();
        cv::Mat image(qimg.height(), qimg.width(), CV_8UC3, 
                     const_cast<uchar*>(qimg.bits()), qimg.bytesPerLine());
        
        for (const auto& [id, roi] : rois.toStdMap()) {
            // Extract ROI region
            cv::Mat roiRegion = roi->extractRegion(image);
            
            // Calculate statistics
            cv::Scalar mean, stddev;
            cv::meanStdDev(roiRegion, mean, stddev);
            
            analysis += QString("\nROI %1: Mean=(%.1f, %.1f, %.1f), StdDev=(%.1f, %.1f, %.1f)")
                .arg(id)
                .arg(mean[0]).arg(mean[1]).arg(mean[2])
                .arg(stddev[0]).arg(stddev[1]).arg(stddev[2]);
        }
        
        m_statusLabel->setText(analysis);
    }
    
private:
    ROISelectorWidget* m_roiSelector;
    QLabel* m_statusLabel;
};

#include "roi_example.moc"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    
    ROIExampleWindow window;
    window.show();
    
    return app.exec();
}