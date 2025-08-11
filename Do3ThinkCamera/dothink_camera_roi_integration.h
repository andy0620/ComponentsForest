#ifndef DOTHINK_CAMERA_ROI_INTEGRATION_H
#define DOTHINK_CAMERA_ROI_INTEGRATION_H

#include <QWidget>
#include <QDockWidget>
#include <memory>
#include "../OpenCV/roi_selector_widget.h"
#include "../OpenCV/roi_manager.h"
#include "../OpenCV/preprocessor_base.h"
#include "do3thinkcameracomponent_export.h"

// Forward declarations
namespace ComponentsForest {
    class Do3ThinkCameraComponent;
}

namespace Do3Think {

/**
 * @brief ROI integration module for Do3Think Camera Control Panel
 * 
 * This class provides seamless integration of the ROI selection system
 * into the camera control panel, enabling interactive ROI creation and
 * real-time preprocessing on camera frames.
 */
class DO3THINKCAMERACOMPONENT_EXPORT CameraROIIntegration : public QObject {
    Q_OBJECT
    
public:
    explicit CameraROIIntegration(QWidget* parent = nullptr);
    ~CameraROIIntegration();
    
    // Get the ROI selector widget for embedding in control panel
    ComponentsForest::OpenCV::ROISelectorWidget* getROISelectorWidget() const;
    
    // Get the ROI manager for programmatic control
    ComponentsForest::OpenCV::ROIManager* getROIManager() const;
    
    // Create a dockable ROI panel
    QDockWidget* createROIDockWidget(const QString& title = "ROI Selection");
    
    // Enable/disable ROI selection mode
    void setROISelectionEnabled(bool enabled);
    
    // Apply ROI preprocessing to a frame
    cv::Mat processFrameWithROIs(const cv::Mat& frame);
    
    // Add a preprocessor to the ROI pipeline
    void addPreprocessor(std::shared_ptr<ComponentsForest::OpenCV::PreProcessorBase> preprocessor);
    
    // Clear all preprocessors
    void clearPreprocessors();
    
    // ROI statistics
    struct ROIStatistics {
        QString roiId;
        double meanIntensity;
        double stdDev;
        int pixelCount;
        cv::Scalar meanColor;
        double contrast;
        double sharpness;
    };
    
    // Get statistics for all ROIs in the current frame
    QVector<ROIStatistics> calculateROIStatistics(const cv::Mat& frame);
    
public slots:
    // Update the display image (called when new frame arrives)
    void updateDisplayImage(const QImage& image);
    
    // Update with cv::Mat directly
    void updateDisplayImage(const cv::Mat& frame);
    
    // Save current ROI configuration
    void saveROIConfiguration(const QString& filepath);
    
    // Load ROI configuration
    void loadROIConfiguration(const QString& filepath);
    
    // Export ROI masks
    void exportROIMasks(const QString& directory);
    
signals:
    // Emitted when a ROI is created
    void roiCreated(const QString& roiId, const QRect& bounds);
    
    // Emitted when a ROI is modified
    void roiModified(const QString& roiId, const QRect& newBounds);
    
    // Emitted when a ROI is deleted  
    void roiDeleted(const QString& roiId);
    
    // Emitted when ROI statistics are calculated
    void roiStatisticsReady(const QVector<ROIStatistics>& stats);
    
    // Emitted when frame is processed with ROIs
    void frameProcessed(const cv::Mat& processedFrame);
    
private:
    class Private;
    std::unique_ptr<Private> d;
    
    void connectSignals();
    void setupDefaultPreprocessors();
};

/**
 * @brief Enhanced camera control panel with integrated ROI support
 * 
 * This extends the standard Do3ThinkCameraControlPanel to include
 * interactive ROI selection and preprocessing capabilities.
 */
class DO3THINKCAMERACOMPONENT_EXPORT Do3ThinkCameraControlPanelWithROI : public QWidget {
    Q_OBJECT
    
public:
    explicit Do3ThinkCameraControlPanelWithROI(QWidget* parent = nullptr);
    ~Do3ThinkCameraControlPanelWithROI();
    
    // Set the camera component to control
    void setCameraComponent(ComponentsForest::Do3ThinkCameraComponent* component);
    
    // Get the ROI integration module
    CameraROIIntegration* getROIIntegration() const;
    
    // Enable/disable ROI overlay on live view
    void setROIOverlayEnabled(bool enabled);
    
    // Enable/disable real-time ROI preprocessing
    void setROIPreprocessingEnabled(bool enabled);
    
    // Show/hide ROI statistics panel
    void setROIStatisticsVisible(bool visible);
    
public slots:
    // Handle incoming camera frames
    void onFrameReceived(const QImage& frame);
    
    // Toggle ROI selection mode
    void toggleROIMode();
    
    // Clear all ROIs
    void clearAllROIs();
    
signals:
    // Emitted when processed frame is ready
    void processedFrameReady(const QImage& frame);
    
    // Emitted when ROI selection mode changes
    void roiModeChanged(bool enabled);
    
private:
    class Private;
    std::unique_ptr<Private> d;
    
    void setupUI();
    void createROIToolbar();
    void createROIStatisticsPanel();
};

} // namespace Do3Think

#endif // DOTHINK_CAMERA_ROI_INTEGRATION_H