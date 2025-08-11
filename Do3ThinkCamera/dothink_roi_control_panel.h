#ifndef DOTHINK_ROI_CONTROL_PANEL_H
#define DOTHINK_ROI_CONTROL_PANEL_H

#include "dothink_camera_control_panel.h"
#include "../OpenCV/roi_selector_widget.h"
#include "../OpenCV/roi_manager.h"
#include <QSplitter>
#include <QListWidget>
#include <QToolBar>
#include <QDockWidget>

namespace ComponentsForest {

// Use OpenCV namespace for convenience
using namespace ComponentsForest::OpenCV;

/**
 * @brief Enhanced Do3Think Camera Control Panel with ROI functionality
 * Combines camera control with interactive ROI selection and management
 */
class Do3ThinkROIControlPanel : public Do3ThinkCameraControlPanel {
    Q_OBJECT
    
public:
    explicit Do3ThinkROIControlPanel(QWidget* parent = nullptr);
    ~Do3ThinkROIControlPanel() override;
    
    // ROI Manager access
    ROIManager* roiManager() const { return m_roiManager; }
    
    // ROI Operations
    Q_INVOKABLE void createROI(ROIType type);
    Q_INVOKABLE void deleteSelectedROIs();
    Q_INVOKABLE void clearAllROIs();
    
    // ROI Import/Export
    Q_INVOKABLE bool saveROIConfiguration(const QString& filename);
    Q_INVOKABLE bool loadROIConfiguration(const QString& filename);
    Q_INVOKABLE bool exportROIData(const QString& filename, const QString& format = "JSON");
    
    // ROI Templates
    Q_INVOKABLE void saveAsTemplate(const QString& name);
    Q_INVOKABLE void loadTemplate(const QString& name);
    Q_INVOKABLE QStringList availableTemplates() const;
    
    // ROI Analysis
    Q_INVOKABLE void enableROIStatistics(bool enable);
    Q_INVOKABLE void setROIProcessingEnabled(bool enable);
    Q_INVOKABLE QJsonObject getROIStatistics(const QString& roiId) const;
    
    // Display options
    Q_INVOKABLE void setROIOverlayEnabled(bool enable);
    Q_INVOKABLE void setROIInfoDisplayEnabled(bool enable);
    Q_INVOKABLE void setROIColorScheme(const QString& scheme);
    
signals:
    // ROI signals
    void roiCreated(const QString& roiId);
    void roiDeleted(const QString& roiId);
    void roiModified(const QString& roiId);
    void roiSelectionChanged(const QStringList& selectedIds);
    
    // Analysis signals
    void roiStatisticsUpdated(const QString& roiId, const QJsonObject& stats);
    void roiProcessingCompleted(const QString& roiId);
    
protected:
    // Override from base class
    void setupUI() override;
    void connectSignals() override;
    
    // Image processing override
    void processImage(const QImage& image) override;
    
private slots:
    // Image slots
    void onImageReceived(const QImage& image, qint64 timestamp);
    void onFrameProcessed(const cv::Mat& frame);
    
    // ROI list management
    void onROIListSelectionChanged();
    void onROIListItemDoubleClicked(QListWidgetItem* item);
    void onROIListContextMenu(const QPoint& pos);
    
    // ROI manager slots
    void onROIAdded(const ROIPtr& roi);
    void onROIRemoved(const QString& roiId);
    void onROIModified(const QString& roiId);
    void onROIEnabledChanged(const QString& roiId, bool enabled);
    
    // Tool actions
    void onSelectToolTriggered();
    void onRectangleToolTriggered();
    void onCircleToolTriggered();
    void onPolygonToolTriggered();
    void onDeleteToolTriggered();
    
    // ROI operations
    void onGroupROIsTriggered();
    void onUngroupROIsTriggered();
    void onDuplicateROIsTriggered();
    
    // View actions
    void onZoomInTriggered();
    void onZoomOutTriggered();
    void onZoomFitTriggered();
    void onZoomActualTriggered();
    
    // Statistics update
    void updateROIStatistics();
    
private:
    // UI setup helpers
    void setupROIToolBar();
    void setupROIListWidget();
    void setupROIPropertiesWidget();
    void setupROIStatisticsWidget();
    void createROIActions();
    void createROIMenus();
    
    // ROI processing
    void processROIs(const cv::Mat& frame);
    void drawROIOverlay(cv::Mat& frame);
    void calculateROIStatistics(const cv::Mat& frame, const ROIPtr& roi);
    
    // Utility methods
    void updateROIListItem(const QString& roiId);
    void selectROIInList(const QString& roiId);
    QColor getROIColor(int index) const;
    
private:
    // Core components
    ROIManager* m_roiManager;
    ROISelectorWidget* m_roiSelector;
    
    // UI components
    QSplitter* m_mainSplitter;
    QWidget* m_roiControlWidget;
    QToolBar* m_roiToolBar;
    QListWidget* m_roiListWidget;
    QWidget* m_roiPropertiesWidget;
    QWidget* m_roiStatisticsWidget;
    
    // Dock widgets
    QDockWidget* m_roiListDock;
    QDockWidget* m_roiPropertiesDock;
    QDockWidget* m_roiStatisticsDock;
    
    // Actions
    QAction* m_selectAction;
    QAction* m_rectangleAction;
    QAction* m_circleAction;
    QAction* m_polygonAction;
    QAction* m_deleteAction;
    QAction* m_clearAllAction;
    QAction* m_groupAction;
    QAction* m_ungroupAction;
    QAction* m_duplicateAction;
    QAction* m_zoomInAction;
    QAction* m_zoomOutAction;
    QAction* m_zoomFitAction;
    QAction* m_zoomActualAction;
    
    // State
    bool m_roiOverlayEnabled{true};
    bool m_roiInfoDisplayEnabled{true};
    bool m_roiProcessingEnabled{false};
    bool m_roiStatisticsEnabled{false};
    QString m_roiColorScheme{"default"};
    
    // Current frame for processing
    cv::Mat m_currentFrame;
    
    // Statistics cache
    std::map<QString, QJsonObject> m_roiStatisticsCache;
    
    // Performance
    QElapsedTimer m_roiProcessingTimer;
    double m_avgROIProcessingTime{0};
};

/**
 * @brief ROI List Item Widget
 * Custom widget for ROI list items with checkbox and color indicator
 */
class ROIListItemWidget : public QWidget {
    Q_OBJECT
    
public:
    explicit ROIListItemWidget(const ROIPtr& roi, QWidget* parent = nullptr);
    
    void setROI(const ROIPtr& roi);
    void setColor(const QColor& color);
    void updateFromROI();
    
    QString roiId() const { return m_roiId; }
    bool isChecked() const;
    void setChecked(bool checked);
    
signals:
    void enabledChanged(bool enabled);
    void editRequested();
    void deleteRequested();
    
private:
    QString m_roiId;
    QCheckBox* m_checkbox;
    QLabel* m_colorLabel;
    QLabel* m_nameLabel;
    QLabel* m_typeLabel;
    QPushButton* m_editButton;
    QPushButton* m_deleteButton;
};

/**
 * @brief ROI Properties Widget
 * Widget for editing ROI properties
 */
class ROIPropertiesWidget : public QWidget {
    Q_OBJECT
    
public:
    explicit ROIPropertiesWidget(QWidget* parent = nullptr);
    
    void setROI(const ROIPtr& roi);
    void clearROI();
    
signals:
    void roiModified(const QString& roiId);
    
private slots:
    void onNameChanged();
    void onColorChanged();
    void onProcessingToggled(bool checked);
    void onTrackingToggled(bool checked);
    void onParameterChanged();
    
private:
    void setupUI();
    void updateFromROI();
    void applyChanges();
    
private:
    ROIPtr m_currentROI;
    
    // Property editors
    QLineEdit* m_nameEdit;
    QPushButton* m_colorButton;
    QSpinBox* m_xSpinBox;
    QSpinBox* m_ySpinBox;
    QSpinBox* m_widthSpinBox;
    QSpinBox* m_heightSpinBox;
    
    // Processing options
    QCheckBox* m_enableProcessingCheck;
    QCheckBox* m_enableBlurCheck;
    QSpinBox* m_blurSizeSpinBox;
    QCheckBox* m_enableThresholdCheck;
    QSlider* m_thresholdSlider;
    
    // Tracking options
    QCheckBox* m_enableTrackingCheck;
    QComboBox* m_trackingMethodCombo;
    QProgressBar* m_trackingConfidenceBar;
    
    bool m_updating{false};
};

/**
 * @brief ROI Statistics Widget
 * Widget for displaying ROI statistics and analysis results
 */
class ROIStatisticsWidget : public QWidget {
    Q_OBJECT
    
public:
    explicit ROIStatisticsWidget(QWidget* parent = nullptr);
    
    void setROI(const ROIPtr& roi);
    void updateStatistics(const QJsonObject& stats);
    void clearStatistics();
    
    void setHistogramEnabled(bool enable);
    void setRealtimeUpdateEnabled(bool enable);
    
private:
    void setupUI();
    void plotHistogram(const cv::Mat& histogram);
    
private:
    // Labels for statistics
    QLabel* m_roiNameLabel;
    QLabel* m_pixelCountLabel;
    QLabel* m_meanIntensityLabel;
    QLabel* m_stdDeviationLabel;
    QLabel* m_minValueLabel;
    QLabel* m_maxValueLabel;
    QLabel* m_meanColorLabel;
    
    // Histogram display
    QtCharts::QChartView* m_histogramView;
    QtCharts::QChart* m_histogramChart;
    QtCharts::QLineSeries* m_histogramSeries;
    
    // Options
    QCheckBox* m_realtimeUpdateCheck;
    QCheckBox* m_showHistogramCheck;
    
    bool m_histogramEnabled{true};
    bool m_realtimeUpdateEnabled{true};
};

} // namespace ComponentsForest

#endif // DOTHINK_ROI_CONTROL_PANEL_H