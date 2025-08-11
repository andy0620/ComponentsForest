#include "dothink_roi_control_panel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QMenu>
#include <QFileDialog>
#include <QColorDialog>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonArray>
#include <opencv2/imgproc.hpp>

namespace ComponentsForest {

// Use OpenCV namespace for convenience
using namespace ComponentsForest::OpenCV;

// ============ Do3ThinkROIControlPanel Implementation ============

Do3ThinkROIControlPanel::Do3ThinkROIControlPanel(QWidget* parent)
    : Do3ThinkCameraControlPanel(parent) {
    
    // Create ROI manager
    m_roiManager = new ROIManager(this);
    
    // Setup enhanced UI
    setupUI();
    connectSignals();
}

Do3ThinkROIControlPanel::~Do3ThinkROIControlPanel() {
    // Cleanup handled by Qt parent-child relationships
}

void Do3ThinkROIControlPanel::setupUI() {
    // Call base class setup first
    Do3ThinkCameraControlPanel::setupUI();
    
    // Get the main layout from base class
    QVBoxLayout* mainLayout = qobject_cast<QVBoxLayout*>(layout());
    if (!mainLayout) {
        mainLayout = new QVBoxLayout(this);
    }
    
    // Create main splitter for image and controls
    m_mainSplitter = new QSplitter(Qt::Horizontal, this);
    
    // Left side - ROI selector widget
    m_roiSelector = new ROISelectorWidget(this);
    m_roiSelector->setROIManager(m_roiManager);
    m_roiSelector->setMinimumSize(640, 480);
    
    // Right side - ROI controls
    m_roiControlWidget = new QWidget(this);
    QVBoxLayout* controlLayout = new QVBoxLayout(m_roiControlWidget);
    
    // Setup ROI components
    setupROIToolBar();
    setupROIListWidget();
    setupROIPropertiesWidget();
    setupROIStatisticsWidget();
    
    // Add toolbar to control widget
    controlLayout->addWidget(m_roiToolBar);
    
    // Create tab widget for ROI panels
    QTabWidget* roiTabs = new QTabWidget(this);
    roiTabs->addTab(m_roiListWidget, "ROI List");
    roiTabs->addTab(m_roiPropertiesWidget, "Properties");
    roiTabs->addTab(m_roiStatisticsWidget, "Statistics");
    controlLayout->addWidget(roiTabs);
    
    // Add to splitter
    m_mainSplitter->addWidget(m_roiSelector);
    m_mainSplitter->addWidget(m_roiControlWidget);
    m_mainSplitter->setStretchFactor(0, 3);
    m_mainSplitter->setStretchFactor(1, 1);
    
    // Add splitter to main layout
    mainLayout->addWidget(m_mainSplitter);
    
    // Create actions and menus
    createROIActions();
    createROIMenus();
}

void Do3ThinkROIControlPanel::connectSignals() {
    // Call base class connections
    Do3ThinkCameraControlPanel::connectSignals();
    
    // Connect ROI manager signals
    connect(m_roiManager, &ROIManager::roiAdded,
            this, &Do3ThinkROIControlPanel::onROIAdded);
    connect(m_roiManager, &ROIManager::roiRemoved,
            this, &Do3ThinkROIControlPanel::onROIRemoved);
    connect(m_roiManager, &ROIManager::roiModified,
            this, &Do3ThinkROIControlPanel::onROIModified);
    connect(m_roiManager, &ROIManager::roiEnabledChanged,
            this, &Do3ThinkROIControlPanel::onROIEnabledChanged);
    
    // Connect ROI selector signals
    connect(m_roiSelector, &ROISelectorWidget::roiCreated,
            [this](ROIPtr roi) { emit roiCreated(roi->getId()); });
    connect(m_roiSelector, &ROISelectorWidget::roiDeleted,
            this, &Do3ThinkROIControlPanel::roiDeleted);
    connect(m_roiSelector, &ROISelectorWidget::roiModified,
            this, &Do3ThinkROIControlPanel::roiModified);
    connect(m_roiSelector, &ROISelectorWidget::selectionChanged,
            [this](const QList<QString>& ids) { 
                emit roiSelectionChanged(QStringList(ids.begin(), ids.end())); 
            });
    
    // Connect ROI list widget
    connect(m_roiListWidget, &QListWidget::itemSelectionChanged,
            this, &Do3ThinkROIControlPanel::onROIListSelectionChanged);
    connect(m_roiListWidget, &QListWidget::itemDoubleClicked,
            this, &Do3ThinkROIControlPanel::onROIListItemDoubleClicked);
    connect(m_roiListWidget, &QListWidget::customContextMenuRequested,
            this, &Do3ThinkROIControlPanel::onROIListContextMenu);
}

void Do3ThinkROIControlPanel::setupROIToolBar() {
    m_roiToolBar = new QToolBar("ROI Tools", this);
    m_roiToolBar->setIconSize(QSize(24, 24));
    
    // Selection tools
    m_selectAction = m_roiToolBar->addAction(QIcon(":/icons/select.png"), "Select");
    m_selectAction->setCheckable(true);
    m_selectAction->setChecked(true);
    m_selectAction->setToolTip("Select and move ROIs");
    connect(m_selectAction, &QAction::triggered, this, &Do3ThinkROIControlPanel::onSelectToolTriggered);
    
    m_roiToolBar->addSeparator();
    
    // Creation tools
    m_rectangleAction = m_roiToolBar->addAction(QIcon(":/icons/rectangle.png"), "Rectangle");
    m_rectangleAction->setCheckable(true);
    m_rectangleAction->setToolTip("Create rectangular ROI");
    connect(m_rectangleAction, &QAction::triggered, this, &Do3ThinkROIControlPanel::onRectangleToolTriggered);
    
    m_circleAction = m_roiToolBar->addAction(QIcon(":/icons/circle.png"), "Circle");
    m_circleAction->setCheckable(true);
    m_circleAction->setToolTip("Create circular ROI");
    connect(m_circleAction, &QAction::triggered, this, &Do3ThinkROIControlPanel::onCircleToolTriggered);
    
    m_polygonAction = m_roiToolBar->addAction(QIcon(":/icons/polygon.png"), "Polygon");
    m_polygonAction->setCheckable(true);
    m_polygonAction->setToolTip("Create polygonal ROI");
    connect(m_polygonAction, &QAction::triggered, this, &Do3ThinkROIControlPanel::onPolygonToolTriggered);
    
    m_roiToolBar->addSeparator();
    
    // Edit tools
    m_deleteAction = m_roiToolBar->addAction(QIcon(":/icons/delete.png"), "Delete");
    m_deleteAction->setToolTip("Delete selected ROIs");
    connect(m_deleteAction, &QAction::triggered, this, &Do3ThinkROIControlPanel::onDeleteToolTriggered);
    
    m_clearAllAction = m_roiToolBar->addAction(QIcon(":/icons/clear.png"), "Clear All");
    m_clearAllAction->setToolTip("Clear all ROIs");
    connect(m_clearAllAction, &QAction::triggered, this, &Do3ThinkROIControlPanel::clearAllROIs);
    
    m_roiToolBar->addSeparator();
    
    // Group tools
    m_groupAction = m_roiToolBar->addAction(QIcon(":/icons/group.png"), "Group");
    m_groupAction->setToolTip("Group selected ROIs");
    connect(m_groupAction, &QAction::triggered, this, &Do3ThinkROIControlPanel::onGroupROIsTriggered);
    
    m_ungroupAction = m_roiToolBar->addAction(QIcon(":/icons/ungroup.png"), "Ungroup");
    m_ungroupAction->setToolTip("Ungroup selected ROIs");
    connect(m_ungroupAction, &QAction::triggered, this, &Do3ThinkROIControlPanel::onUngroupROIsTriggered);
    
    m_duplicateAction = m_roiToolBar->addAction(QIcon(":/icons/duplicate.png"), "Duplicate");
    m_duplicateAction->setToolTip("Duplicate selected ROIs");
    connect(m_duplicateAction, &QAction::triggered, this, &Do3ThinkROIControlPanel::onDuplicateROIsTriggered);
    
    m_roiToolBar->addSeparator();
    
    // View tools
    m_zoomInAction = m_roiToolBar->addAction(QIcon(":/icons/zoom_in.png"), "Zoom In");
    m_zoomInAction->setToolTip("Zoom in");
    connect(m_zoomInAction, &QAction::triggered, this, &Do3ThinkROIControlPanel::onZoomInTriggered);
    
    m_zoomOutAction = m_roiToolBar->addAction(QIcon(":/icons/zoom_out.png"), "Zoom Out");
    m_zoomOutAction->setToolTip("Zoom out");
    connect(m_zoomOutAction, &QAction::triggered, this, &Do3ThinkROIControlPanel::onZoomOutTriggered);
    
    m_zoomFitAction = m_roiToolBar->addAction(QIcon(":/icons/zoom_fit.png"), "Zoom Fit");
    m_zoomFitAction->setToolTip("Fit to window");
    connect(m_zoomFitAction, &QAction::triggered, this, &Do3ThinkROIControlPanel::onZoomFitTriggered);
    
    m_zoomActualAction = m_roiToolBar->addAction(QIcon(":/icons/zoom_actual.png"), "Actual Size");
    m_zoomActualAction->setToolTip("Actual size (100%)");
    connect(m_zoomActualAction, &QAction::triggered, this, &Do3ThinkROIControlPanel::onZoomActualTriggered);
    
    // Create action group for tools
    QActionGroup* toolGroup = new QActionGroup(this);
    toolGroup->addAction(m_selectAction);
    toolGroup->addAction(m_rectangleAction);
    toolGroup->addAction(m_circleAction);
    toolGroup->addAction(m_polygonAction);
}

void Do3ThinkROIControlPanel::setupROIListWidget() {
    m_roiListWidget = new QListWidget(this);
    m_roiListWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_roiListWidget->setContextMenuPolicy(Qt::CustomContextMenu);
}

void Do3ThinkROIControlPanel::setupROIPropertiesWidget() {
    m_roiPropertiesWidget = new ROIPropertiesWidget(this);
    
    connect(m_roiPropertiesWidget, &ROIPropertiesWidget::roiModified,
            [this](const QString& roiId) {
                auto roi = m_roiManager->getROI(roiId);
                if (roi) {
                    m_roiManager->updateROI(roiId, roi);
                }
            });
}

void Do3ThinkROIControlPanel::setupROIStatisticsWidget() {
    m_roiStatisticsWidget = new ROIStatisticsWidget(this);
}

void Do3ThinkROIControlPanel::createROIActions() {
    // Additional actions not in toolbar
    // These can be added to menus or used programmatically
}

void Do3ThinkROIControlPanel::createROIMenus() {
    // ROI menu can be added to parent window's menu bar
    // or as a context menu
}

// ROI Operations

void Do3ThinkROIControlPanel::createROI(ROIType type) {
    ROIPtr roi;
    
    switch (type) {
        case ROIType::Rectangle:
            roi = ROIFactory::createRectangle(cv::Rect2f(100, 100, 200, 150));
            break;
        case ROIType::Circle:
            roi = ROIFactory::createCircle(cv::Point2f(200, 200), 100);
            break;
        case ROIType::Polygon:
            roi = ROIFactory::createPolygon({
                cv::Point2f(100, 100),
                cv::Point2f(200, 100),
                cv::Point2f(200, 200),
                cv::Point2f(100, 200)
            });
            break;
        default:
            return;
    }
    
    if (roi) {
        m_roiManager->addROI(roi);
    }
}

void Do3ThinkROIControlPanel::deleteSelectedROIs() {
    m_roiSelector->deleteSelectedROIs();
}

void Do3ThinkROIControlPanel::clearAllROIs() {
    int ret = QMessageBox::question(this, "Clear All ROIs",
                                    "Are you sure you want to delete all ROIs?",
                                    QMessageBox::Yes | QMessageBox::No);
    
    if (ret == QMessageBox::Yes) {
        m_roiManager->clear();
    }
}

bool Do3ThinkROIControlPanel::saveROIConfiguration(const QString& filename) {
    m_roiManager->save(filename);
    return true;
}

bool Do3ThinkROIControlPanel::loadROIConfiguration(const QString& filename) {
    m_roiManager->load(filename);
    return true;
}

bool Do3ThinkROIControlPanel::exportROIData(const QString& filename, const QString& format) {
    auto rois = m_roiManager->getAllROIs();
    
    ROIImportExport::Format exportFormat = ROIImportExport::Format::JSON;
    if (format == "XML") {
        exportFormat = ROIImportExport::Format::XML;
    } else if (format == "CSV") {
        exportFormat = ROIImportExport::Format::CSV;
    } else if (format == "Binary") {
        exportFormat = ROIImportExport::Format::Binary;
    }
    
    ROIImportExport::exportROIs(rois, filename, exportFormat);
    return true;
}

void Do3ThinkROIControlPanel::saveAsTemplate(const QString& name) {
    ROITemplate template_;
    template_.name = name;
    template_.created = QDateTime::currentDateTime();
    template_.rois = m_roiManager->getAllROIs();
    
    m_roiManager->saveTemplate(template_);
}

void Do3ThinkROIControlPanel::loadTemplate(const QString& name) {
    m_roiManager->loadTemplate(name);
}

QStringList Do3ThinkROIControlPanel::availableTemplates() const {
    QStringList templates;
    for (const auto& t : m_roiManager->getTemplates()) {
        templates << t.name;
    }
    return templates;
}

void Do3ThinkROIControlPanel::enableROIStatistics(bool enable) {
    m_roiStatisticsEnabled = enable;
    
    if (enable) {
        updateROIStatistics();
    }
}

void Do3ThinkROIControlPanel::setROIProcessingEnabled(bool enable) {
    m_roiProcessingEnabled = enable;
}

QJsonObject Do3ThinkROIControlPanel::getROIStatistics(const QString& roiId) const {
    auto it = m_roiStatisticsCache.find(roiId);
    if (it != m_roiStatisticsCache.end()) {
        return it->second;
    }
    return QJsonObject();
}

void Do3ThinkROIControlPanel::setROIOverlayEnabled(bool enable) {
    m_roiOverlayEnabled = enable;
}

void Do3ThinkROIControlPanel::setROIInfoDisplayEnabled(bool enable) {
    m_roiInfoDisplayEnabled = enable;
}

void Do3ThinkROIControlPanel::setROIColorScheme(const QString& scheme) {
    m_roiColorScheme = scheme;
}

// Protected overrides

void Do3ThinkROIControlPanel::processImage(const QImage& image) {
    // Convert QImage to cv::Mat
    cv::Mat mat;
    
    if (image.format() == QImage::Format_RGB888) {
        mat = cv::Mat(image.height(), image.width(), CV_8UC3, 
                     const_cast<uchar*>(image.bits()), image.bytesPerLine());
        cv::cvtColor(mat, mat, cv::COLOR_RGB2BGR);
    } else if (image.format() == QImage::Format_Grayscale8) {
        mat = cv::Mat(image.height(), image.width(), CV_8UC1,
                     const_cast<uchar*>(image.bits()), image.bytesPerLine());
    } else {
        // Convert to RGB888 first
        QImage rgb = image.convertToFormat(QImage::Format_RGB888);
        mat = cv::Mat(rgb.height(), rgb.width(), CV_8UC3,
                     const_cast<uchar*>(rgb.bits()), rgb.bytesPerLine());
        cv::cvtColor(mat, mat, cv::COLOR_RGB2BGR);
    }
    
    m_currentFrame = mat.clone();
    
    // Update ROI selector with new image
    m_roiSelector->setImage(m_currentFrame);
    
    // Process ROIs if enabled
    if (m_roiProcessingEnabled) {
        processROIs(m_currentFrame);
    }
    
    // Update statistics if enabled
    if (m_roiStatisticsEnabled) {
        updateROIStatistics();
    }
    
    // Draw overlay if enabled
    if (m_roiOverlayEnabled) {
        cv::Mat overlayFrame = m_currentFrame.clone();
        drawROIOverlay(overlayFrame);
        
        // Convert back to QImage for display
        QImage overlayImage;
        if (overlayFrame.channels() == 1) {
            overlayImage = QImage(overlayFrame.data, overlayFrame.cols, overlayFrame.rows,
                                 overlayFrame.step, QImage::Format_Grayscale8);
        } else {
            cv::cvtColor(overlayFrame, overlayFrame, cv::COLOR_BGR2RGB);
            overlayImage = QImage(overlayFrame.data, overlayFrame.cols, overlayFrame.rows,
                                 overlayFrame.step, QImage::Format_RGB888);
        }
        
        // Update display
        m_roiSelector->setImage(overlayFrame);
    }
}

// Private slots

void Do3ThinkROIControlPanel::onImageReceived(const QImage& image, qint64 timestamp) {
    Q_UNUSED(timestamp)
    processImage(image);
}

void Do3ThinkROIControlPanel::onFrameProcessed(const cv::Mat& frame) {
    m_currentFrame = frame.clone();
    
    // Update ROI tracking if enabled
    m_roiManager->updateTracking(frame);
    
    // Process ROIs
    if (m_roiProcessingEnabled) {
        processROIs(frame);
    }
}

void Do3ThinkROIControlPanel::onROIListSelectionChanged() {
    QList<QListWidgetItem*> selected = m_roiListWidget->selectedItems();
    
    if (selected.isEmpty()) {
        m_roiPropertiesWidget->clearROI();
        m_roiStatisticsWidget->clearStatistics();
        return;
    }
    
    // Get first selected ROI
    QString roiId = selected.first()->data(Qt::UserRole).toString();
    auto roi = m_roiManager->getROI(roiId);
    
    if (roi) {
        // Update properties widget
        m_roiPropertiesWidget->setROI(roi);
        
        // Update statistics widget
        m_roiStatisticsWidget->setROI(roi);
        auto stats = getROIStatistics(roiId);
        if (!stats.isEmpty()) {
            m_roiStatisticsWidget->updateStatistics(stats);
        }
        
        // Select in ROI selector
        m_roiSelector->selectROI(roiId);
    }
}

void Do3ThinkROIControlPanel::onROIListItemDoubleClicked(QListWidgetItem* item) {
    QString roiId = item->data(Qt::UserRole).toString();
    auto roi = m_roiManager->getROI(roiId);
    
    if (roi) {
        // Focus on ROI in selector
        m_roiSelector->selectROI(roiId);
        
        // Could open edit dialog here
    }
}

void Do3ThinkROIControlPanel::onROIListContextMenu(const QPoint& pos) {
    QListWidgetItem* item = m_roiListWidget->itemAt(pos);
    if (!item) return;
    
    QString roiId = item->data(Qt::UserRole).toString();
    
    QMenu menu(this);
    menu.addAction("Edit", [this, roiId]() {
        // Open edit dialog
    });
    menu.addAction("Duplicate", [this, roiId]() {
        auto roi = m_roiManager->getROI(roiId);
        if (roi) {
            auto cloned = roi->clone();
            m_roiManager->addROI(cloned);
        }
    });
    menu.addSeparator();
    menu.addAction("Delete", [this, roiId]() {
        m_roiManager->removeROI(roiId);
    });
    
    menu.exec(m_roiListWidget->mapToGlobal(pos));
}

void Do3ThinkROIControlPanel::onROIAdded(const ROIPtr& roi) {
    if (!roi) return;
    
    // Add to list widget
    QListWidgetItem* item = new QListWidgetItem(m_roiListWidget);
    item->setText(roi->getName());
    item->setData(Qt::UserRole, roi->getId());
    
    // Set color based on index
    int index = m_roiListWidget->count() - 1;
    QColor color = getROIColor(index);
    item->setBackground(QBrush(color.lighter(180)));
    
    emit roiCreated(roi->getId());
}

void Do3ThinkROIControlPanel::onROIRemoved(const QString& roiId) {
    // Remove from list widget
    for (int i = 0; i < m_roiListWidget->count(); ++i) {
        QListWidgetItem* item = m_roiListWidget->item(i);
        if (item->data(Qt::UserRole).toString() == roiId) {
            delete m_roiListWidget->takeItem(i);
            break;
        }
    }
    
    // Remove from statistics cache
    m_roiStatisticsCache.erase(roiId);
    
    emit roiDeleted(roiId);
}

void Do3ThinkROIControlPanel::onROIModified(const QString& roiId) {
    updateROIListItem(roiId);
    emit roiModified(roiId);
}

void Do3ThinkROIControlPanel::onROIEnabledChanged(const QString& roiId, bool enabled) {
    Q_UNUSED(enabled)
    updateROIListItem(roiId);
}

// Tool action slots

void Do3ThinkROIControlPanel::onSelectToolTriggered() {
    m_roiSelector->setInteractionMode(InteractionMode::Select);
}

void Do3ThinkROIControlPanel::onRectangleToolTriggered() {
    m_roiSelector->setInteractionMode(InteractionMode::Create);
    m_roiSelector->setCreationTool(CreationTool::Rectangle);
}

void Do3ThinkROIControlPanel::onCircleToolTriggered() {
    m_roiSelector->setInteractionMode(InteractionMode::Create);
    m_roiSelector->setCreationTool(CreationTool::Circle);
}

void Do3ThinkROIControlPanel::onPolygonToolTriggered() {
    m_roiSelector->setInteractionMode(InteractionMode::Create);
    m_roiSelector->setCreationTool(CreationTool::Polygon);
}

void Do3ThinkROIControlPanel::onDeleteToolTriggered() {
    deleteSelectedROIs();
}

void Do3ThinkROIControlPanel::onGroupROIsTriggered() {
    m_roiSelector->groupSelectedROIs();
}

void Do3ThinkROIControlPanel::onUngroupROIsTriggered() {
    m_roiSelector->ungroupSelectedROIs();
}

void Do3ThinkROIControlPanel::onDuplicateROIsTriggered() {
    m_roiSelector->duplicateSelectedROIs();
}

void Do3ThinkROIControlPanel::onZoomInTriggered() {
    m_roiSelector->zoomIn();
}

void Do3ThinkROIControlPanel::onZoomOutTriggered() {
    m_roiSelector->zoomOut();
}

void Do3ThinkROIControlPanel::onZoomFitTriggered() {
    m_roiSelector->zoomFit();
}

void Do3ThinkROIControlPanel::onZoomActualTriggered() {
    m_roiSelector->zoomActual();
}

// Statistics update

void Do3ThinkROIControlPanel::updateROIStatistics() {
    if (m_currentFrame.empty()) return;
    
    auto rois = m_roiManager->getActiveROIs();
    
    for (const auto& roi : rois) {
        calculateROIStatistics(m_currentFrame, roi);
    }
}

// Private methods

void Do3ThinkROIControlPanel::processROIs(const cv::Mat& frame) {
    if (frame.empty()) return;
    
    m_roiProcessingTimer.start();
    
    // Process frame with ROI manager
    cv::Mat processed = m_roiManager->processFrame(frame);
    
    double elapsed = m_roiProcessingTimer.elapsed();
    m_avgROIProcessingTime = (m_avgROIProcessingTime * 0.9) + (elapsed * 0.1);
    
    // Emit signal for each processed ROI
    auto activeROIs = m_roiManager->getActiveROIs();
    for (const auto& roi : activeROIs) {
        emit roiProcessingCompleted(roi->getId());
    }
}

void Do3ThinkROIControlPanel::drawROIOverlay(cv::Mat& frame) {
    auto rois = m_roiManager->getAllROIs();
    
    int index = 0;
    for (const auto& roi : rois) {
        if (!roi->isEnabled()) continue;
        
        QColor color = getROIColor(index++);
        cv::Scalar cvColor(color.blue(), color.green(), color.red());
        
        roi->draw(frame, cvColor, 2);
        
        if (m_roiInfoDisplayEnabled) {
            // Draw ROI info
            cv::Rect2f bbox = roi->getBoundingBox();
            std::string info = roi->getName().toStdString();
            cv::putText(frame, info, cv::Point(bbox.x, bbox.y - 5),
                       cv::FONT_HERSHEY_SIMPLEX, 0.5, cvColor, 1);
        }
    }
}

void Do3ThinkROIControlPanel::calculateROIStatistics(const cv::Mat& frame, const ROIPtr& roi) {
    if (!roi || frame.empty()) return;
    
    cv::Mat roiData = roi->extract(frame);
    if (roiData.empty()) return;
    
    QJsonObject stats;
    stats["roiId"] = roi->getId();
    stats["roiName"] = roi->getName();
    
    // Basic statistics
    cv::Scalar mean, stddev;
    cv::meanStdDev(roiData, mean, stddev);
    
    stats["meanIntensity"] = mean[0];
    stats["stdDeviation"] = stddev[0];
    
    if (roiData.channels() == 3) {
        QJsonArray meanColor;
        meanColor.append(mean[0]);
        meanColor.append(mean[1]);
        meanColor.append(mean[2]);
        stats["meanColor"] = meanColor;
    }
    
    // Min/Max values
    double minVal, maxVal;
    cv::minMaxLoc(roiData, &minVal, &maxVal);
    stats["minValue"] = minVal;
    stats["maxValue"] = maxVal;
    
    // Pixel count
    int pixelCount = cv::countNonZero(roi->getMask(frame.size()));
    stats["pixelCount"] = pixelCount;
    
    // Calculate histogram
    cv::Mat histogram;
    int histSize = 256;
    float range[] = {0, 256};
    const float* histRange = {range};
    cv::calcHist(&roiData, 1, 0, roi->getMask(frame.size()),
                histogram, 1, &histSize, &histRange);
    
    // Store in cache
    m_roiStatisticsCache[roi->getId()] = stats;
    
    // Emit signal
    emit roiStatisticsUpdated(roi->getId(), stats);
    
    // Update statistics widget if this ROI is selected
    if (m_roiStatisticsWidget) {
        m_roiStatisticsWidget->updateStatistics(stats);
    }
}

void Do3ThinkROIControlPanel::updateROIListItem(const QString& roiId) {
    for (int i = 0; i < m_roiListWidget->count(); ++i) {
        QListWidgetItem* item = m_roiListWidget->item(i);
        if (item->data(Qt::UserRole).toString() == roiId) {
            auto roi = m_roiManager->getROI(roiId);
            if (roi) {
                item->setText(roi->getName());
                
                // Update appearance based on state
                if (!roi->isEnabled()) {
                    item->setForeground(Qt::gray);
                } else {
                    item->setForeground(Qt::black);
                }
            }
            break;
        }
    }
}

void Do3ThinkROIControlPanel::selectROIInList(const QString& roiId) {
    for (int i = 0; i < m_roiListWidget->count(); ++i) {
        QListWidgetItem* item = m_roiListWidget->item(i);
        if (item->data(Qt::UserRole).toString() == roiId) {
            m_roiListWidget->setCurrentItem(item);
            break;
        }
    }
}

QColor Do3ThinkROIControlPanel::getROIColor(int index) const {
    // Predefined color palette
    static const QColor colors[] = {
        QColor(255, 0, 0),     // Red
        QColor(0, 255, 0),     // Green
        QColor(0, 0, 255),     // Blue
        QColor(255, 255, 0),   // Yellow
        QColor(255, 0, 255),   // Magenta
        QColor(0, 255, 255),   // Cyan
        QColor(255, 128, 0),   // Orange
        QColor(128, 0, 255),   // Purple
        QColor(0, 255, 128),   // Spring Green
        QColor(255, 0, 128)    // Rose
    };
    
    const int numColors = sizeof(colors) / sizeof(colors[0]);
    return colors[index % numColors];
}

// ============ ROIPropertiesWidget Implementation ============

ROIPropertiesWidget::ROIPropertiesWidget(QWidget* parent) : QWidget(parent) {
    setupUI();
}

void ROIPropertiesWidget::setupUI() {
    QVBoxLayout* layout = new QVBoxLayout(this);
    
    // General properties
    QGroupBox* generalGroup = new QGroupBox("General", this);
    QGridLayout* generalLayout = new QGridLayout(generalGroup);
    
    generalLayout->addWidget(new QLabel("Name:"), 0, 0);
    m_nameEdit = new QLineEdit(this);
    connect(m_nameEdit, &QLineEdit::editingFinished, this, &ROIPropertiesWidget::onNameChanged);
    generalLayout->addWidget(m_nameEdit, 0, 1);
    
    generalLayout->addWidget(new QLabel("Color:"), 1, 0);
    m_colorButton = new QPushButton(this);
    m_colorButton->setFixedSize(50, 25);
    connect(m_colorButton, &QPushButton::clicked, this, &ROIPropertiesWidget::onColorChanged);
    generalLayout->addWidget(m_colorButton, 1, 1);
    
    layout->addWidget(generalGroup);
    
    // Geometry properties
    QGroupBox* geometryGroup = new QGroupBox("Geometry", this);
    QGridLayout* geometryLayout = new QGridLayout(geometryGroup);
    
    geometryLayout->addWidget(new QLabel("X:"), 0, 0);
    m_xSpinBox = new QSpinBox(this);
    m_xSpinBox->setRange(0, 10000);
    connect(m_xSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &ROIPropertiesWidget::onParameterChanged);
    geometryLayout->addWidget(m_xSpinBox, 0, 1);
    
    geometryLayout->addWidget(new QLabel("Y:"), 0, 2);
    m_ySpinBox = new QSpinBox(this);
    m_ySpinBox->setRange(0, 10000);
    connect(m_ySpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &ROIPropertiesWidget::onParameterChanged);
    geometryLayout->addWidget(m_ySpinBox, 0, 3);
    
    geometryLayout->addWidget(new QLabel("Width:"), 1, 0);
    m_widthSpinBox = new QSpinBox(this);
    m_widthSpinBox->setRange(1, 10000);
    connect(m_widthSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &ROIPropertiesWidget::onParameterChanged);
    geometryLayout->addWidget(m_widthSpinBox, 1, 1);
    
    geometryLayout->addWidget(new QLabel("Height:"), 1, 2);
    m_heightSpinBox = new QSpinBox(this);
    m_heightSpinBox->setRange(1, 10000);
    connect(m_heightSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &ROIPropertiesWidget::onParameterChanged);
    geometryLayout->addWidget(m_heightSpinBox, 1, 3);
    
    layout->addWidget(geometryGroup);
    
    // Processing options
    QGroupBox* processingGroup = new QGroupBox("Processing", this);
    QVBoxLayout* processingLayout = new QVBoxLayout(processingGroup);
    
    m_enableProcessingCheck = new QCheckBox("Enable Processing", this);
    connect(m_enableProcessingCheck, &QCheckBox::toggled,
            this, &ROIPropertiesWidget::onProcessingToggled);
    processingLayout->addWidget(m_enableProcessingCheck);
    
    m_enableBlurCheck = new QCheckBox("Enable Blur", this);
    processingLayout->addWidget(m_enableBlurCheck);
    
    QHBoxLayout* blurLayout = new QHBoxLayout();
    blurLayout->addWidget(new QLabel("Blur Size:"));
    m_blurSizeSpinBox = new QSpinBox(this);
    m_blurSizeSpinBox->setRange(1, 31);
    m_blurSizeSpinBox->setSingleStep(2);
    m_blurSizeSpinBox->setValue(5);
    blurLayout->addWidget(m_blurSizeSpinBox);
    processingLayout->addLayout(blurLayout);
    
    m_enableThresholdCheck = new QCheckBox("Enable Threshold", this);
    processingLayout->addWidget(m_enableThresholdCheck);
    
    QHBoxLayout* threshLayout = new QHBoxLayout();
    threshLayout->addWidget(new QLabel("Threshold:"));
    m_thresholdSlider = new QSlider(Qt::Horizontal, this);
    m_thresholdSlider->setRange(0, 255);
    m_thresholdSlider->setValue(128);
    threshLayout->addWidget(m_thresholdSlider);
    processingLayout->addLayout(threshLayout);
    
    layout->addWidget(processingGroup);
    
    // Tracking options
    QGroupBox* trackingGroup = new QGroupBox("Tracking", this);
    QVBoxLayout* trackingLayout = new QVBoxLayout(trackingGroup);
    
    m_enableTrackingCheck = new QCheckBox("Enable Tracking", this);
    connect(m_enableTrackingCheck, &QCheckBox::toggled,
            this, &ROIPropertiesWidget::onTrackingToggled);
    trackingLayout->addWidget(m_enableTrackingCheck);
    
    QHBoxLayout* methodLayout = new QHBoxLayout();
    methodLayout->addWidget(new QLabel("Method:"));
    m_trackingMethodCombo = new QComboBox(this);
    m_trackingMethodCombo->addItems({"None", "Template Matching", "Optical Flow", 
                                     "CSRT", "KCF", "MIL"});
    methodLayout->addWidget(m_trackingMethodCombo);
    trackingLayout->addLayout(methodLayout);
    
    QHBoxLayout* confLayout = new QHBoxLayout();
    confLayout->addWidget(new QLabel("Confidence:"));
    m_trackingConfidenceBar = new QProgressBar(this);
    m_trackingConfidenceBar->setRange(0, 100);
    confLayout->addWidget(m_trackingConfidenceBar);
    trackingLayout->addLayout(confLayout);
    
    layout->addWidget(trackingGroup);
    
    layout->addStretch();
}

void ROIPropertiesWidget::setROI(const ROIPtr& roi) {
    m_currentROI = roi;
    updateFromROI();
}

void ROIPropertiesWidget::clearROI() {
    m_currentROI = nullptr;
    
    m_nameEdit->clear();
    m_colorButton->setStyleSheet("");
    m_xSpinBox->setValue(0);
    m_ySpinBox->setValue(0);
    m_widthSpinBox->setValue(1);
    m_heightSpinBox->setValue(1);
    
    setEnabled(false);
}

void ROIPropertiesWidget::updateFromROI() {
    if (!m_currentROI) return;
    
    m_updating = true;
    
    setEnabled(true);
    
    m_nameEdit->setText(m_currentROI->getName());
    
    cv::Rect2f bbox = m_currentROI->getBoundingBox();
    m_xSpinBox->setValue(static_cast<int>(bbox.x));
    m_ySpinBox->setValue(static_cast<int>(bbox.y));
    m_widthSpinBox->setValue(static_cast<int>(bbox.width));
    m_heightSpinBox->setValue(static_cast<int>(bbox.height));
    
    // Processing params
    auto& params = m_currentROI->processingParams();
    m_enableBlurCheck->setChecked(params.enableBlur);
    m_blurSizeSpinBox->setValue(params.blurSize);
    m_enableThresholdCheck->setChecked(params.enableThreshold);
    m_thresholdSlider->setValue(static_cast<int>(params.thresholdValue));
    
    // Tracking
    auto& metadata = m_currentROI->metadata();
    m_enableTrackingCheck->setChecked(metadata.trackingEnabled);
    m_trackingMethodCombo->setCurrentIndex(static_cast<int>(metadata.trackingMethod));
    m_trackingConfidenceBar->setValue(static_cast<int>(metadata.trackingConfidence * 100));
    
    m_updating = false;
}

void ROIPropertiesWidget::applyChanges() {
    if (!m_currentROI || m_updating) return;
    
    // Apply processing parameters
    auto& params = m_currentROI->processingParams();
    params.enableBlur = m_enableBlurCheck->isChecked();
    params.blurSize = m_blurSizeSpinBox->value();
    params.enableThreshold = m_enableThresholdCheck->isChecked();
    params.thresholdValue = m_thresholdSlider->value();
    
    emit roiModified(m_currentROI->getId());
}

void ROIPropertiesWidget::onNameChanged() {
    if (!m_currentROI || m_updating) return;
    
    m_currentROI->setName(m_nameEdit->text());
    emit roiModified(m_currentROI->getId());
}

void ROIPropertiesWidget::onColorChanged() {
    if (!m_currentROI) return;
    
    QColor color = QColorDialog::getColor(Qt::white, this, "Select ROI Color");
    if (color.isValid()) {
        m_colorButton->setStyleSheet(QString("background-color: %1").arg(color.name()));
        // Store color in ROI metadata or custom property
        emit roiModified(m_currentROI->getId());
    }
}

void ROIPropertiesWidget::onProcessingToggled(bool checked) {
    if (!m_currentROI || m_updating) return;
    
    // Enable/disable processing controls
    m_enableBlurCheck->setEnabled(checked);
    m_blurSizeSpinBox->setEnabled(checked && m_enableBlurCheck->isChecked());
    m_enableThresholdCheck->setEnabled(checked);
    m_thresholdSlider->setEnabled(checked && m_enableThresholdCheck->isChecked());
    
    applyChanges();
}

void ROIPropertiesWidget::onTrackingToggled(bool checked) {
    if (!m_currentROI || m_updating) return;
    
    auto& metadata = m_currentROI->metadata();
    metadata.trackingEnabled = checked;
    
    m_trackingMethodCombo->setEnabled(checked);
    
    emit roiModified(m_currentROI->getId());
}

void ROIPropertiesWidget::onParameterChanged() {
    if (!m_currentROI || m_updating) return;
    
    // Update ROI geometry if it's a rectangle
    if (m_currentROI->getType() == ROIType::Rectangle) {
        auto rectROI = std::dynamic_pointer_cast<RectangularROI>(m_currentROI);
        if (rectROI) {
            cv::Rect2f bounds(
                m_xSpinBox->value(),
                m_ySpinBox->value(),
                m_widthSpinBox->value(),
                m_heightSpinBox->value()
            );
            rectROI->setBounds(bounds);
            emit roiModified(m_currentROI->getId());
        }
    }
}

// ============ ROIStatisticsWidget Implementation ============

ROIStatisticsWidget::ROIStatisticsWidget(QWidget* parent) : QWidget(parent) {
    setupUI();
}

void ROIStatisticsWidget::setupUI() {
    QVBoxLayout* layout = new QVBoxLayout(this);
    
    // ROI info
    m_roiNameLabel = new QLabel("ROI: None", this);
    m_roiNameLabel->setStyleSheet("font-weight: bold;");
    layout->addWidget(m_roiNameLabel);
    
    // Statistics group
    QGroupBox* statsGroup = new QGroupBox("Statistics", this);
    QGridLayout* statsLayout = new QGridLayout(statsGroup);
    
    statsLayout->addWidget(new QLabel("Pixel Count:"), 0, 0);
    m_pixelCountLabel = new QLabel("0", this);
    statsLayout->addWidget(m_pixelCountLabel, 0, 1);
    
    statsLayout->addWidget(new QLabel("Mean Intensity:"), 1, 0);
    m_meanIntensityLabel = new QLabel("0.0", this);
    statsLayout->addWidget(m_meanIntensityLabel, 1, 1);
    
    statsLayout->addWidget(new QLabel("Std Deviation:"), 2, 0);
    m_stdDeviationLabel = new QLabel("0.0", this);
    statsLayout->addWidget(m_stdDeviationLabel, 2, 1);
    
    statsLayout->addWidget(new QLabel("Min Value:"), 3, 0);
    m_minValueLabel = new QLabel("0", this);
    statsLayout->addWidget(m_minValueLabel, 3, 1);
    
    statsLayout->addWidget(new QLabel("Max Value:"), 4, 0);
    m_maxValueLabel = new QLabel("0", this);
    statsLayout->addWidget(m_maxValueLabel, 4, 1);
    
    statsLayout->addWidget(new QLabel("Mean Color:"), 5, 0);
    m_meanColorLabel = new QLabel("(0, 0, 0)", this);
    statsLayout->addWidget(m_meanColorLabel, 5, 1);
    
    layout->addWidget(statsGroup);
    
    // Histogram
    QGroupBox* histGroup = new QGroupBox("Histogram", this);
    QVBoxLayout* histLayout = new QVBoxLayout(histGroup);
    
    m_histogramChart = new QtCharts::QChart();
    m_histogramChart->setTitle("Intensity Distribution");
    m_histogramChart->legend()->hide();
    
    m_histogramSeries = new QtCharts::QLineSeries();
    m_histogramChart->addSeries(m_histogramSeries);
    m_histogramChart->createDefaultAxes();
    
    m_histogramView = new QtCharts::QChartView(m_histogramChart);
    m_histogramView->setRenderHint(QPainter::Antialiasing);
    m_histogramView->setMinimumHeight(200);
    
    histLayout->addWidget(m_histogramView);
    
    layout->addWidget(histGroup);
    
    // Options
    QGroupBox* optionsGroup = new QGroupBox("Options", this);
    QVBoxLayout* optionsLayout = new QVBoxLayout(optionsGroup);
    
    m_realtimeUpdateCheck = new QCheckBox("Realtime Update", this);
    m_realtimeUpdateCheck->setChecked(true);
    optionsLayout->addWidget(m_realtimeUpdateCheck);
    
    m_showHistogramCheck = new QCheckBox("Show Histogram", this);
    m_showHistogramCheck->setChecked(true);
    connect(m_showHistogramCheck, &QCheckBox::toggled,
            [this](bool checked) { m_histogramView->setVisible(checked); });
    optionsLayout->addWidget(m_showHistogramCheck);
    
    layout->addWidget(optionsGroup);
    
    layout->addStretch();
}

void ROIStatisticsWidget::setROI(const ROIPtr& roi) {
    if (!roi) {
        clearStatistics();
        return;
    }
    
    m_roiNameLabel->setText(QString("ROI: %1").arg(roi->getName()));
}

void ROIStatisticsWidget::updateStatistics(const QJsonObject& stats) {
    m_pixelCountLabel->setText(QString::number(stats["pixelCount"].toInt()));
    m_meanIntensityLabel->setText(QString::number(stats["meanIntensity"].toDouble(), 'f', 2));
    m_stdDeviationLabel->setText(QString::number(stats["stdDeviation"].toDouble(), 'f', 2));
    m_minValueLabel->setText(QString::number(stats["minValue"].toDouble(), 'f', 1));
    m_maxValueLabel->setText(QString::number(stats["maxValue"].toDouble(), 'f', 1));
    
    if (stats.contains("meanColor")) {
        QJsonArray color = stats["meanColor"].toArray();
        if (color.size() >= 3) {
            m_meanColorLabel->setText(QString("(%1, %2, %3)")
                .arg(static_cast<int>(color[0].toDouble()))
                .arg(static_cast<int>(color[1].toDouble()))
                .arg(static_cast<int>(color[2].toDouble())));
        }
    }
    
    // Update histogram if available
    // Implementation would parse histogram data from stats and plot it
}

void ROIStatisticsWidget::clearStatistics() {
    m_roiNameLabel->setText("ROI: None");
    m_pixelCountLabel->setText("0");
    m_meanIntensityLabel->setText("0.0");
    m_stdDeviationLabel->setText("0.0");
    m_minValueLabel->setText("0");
    m_maxValueLabel->setText("0");
    m_meanColorLabel->setText("(0, 0, 0)");
    
    m_histogramSeries->clear();
}

void ROIStatisticsWidget::setHistogramEnabled(bool enable) {
    m_histogramEnabled = enable;
    m_histogramView->setVisible(enable && m_showHistogramCheck->isChecked());
}

void ROIStatisticsWidget::setRealtimeUpdateEnabled(bool enable) {
    m_realtimeUpdateEnabled = enable;
    m_realtimeUpdateCheck->setChecked(enable);
}

void ROIStatisticsWidget::plotHistogram(const cv::Mat& histogram) {
    if (!m_histogramEnabled || histogram.empty()) return;
    
    m_histogramSeries->clear();
    
    for (int i = 0; i < histogram.rows; ++i) {
        float value = histogram.at<float>(i);
        m_histogramSeries->append(i, value);
    }
    
    m_histogramChart->removeSeries(m_histogramSeries);
    m_histogramChart->addSeries(m_histogramSeries);
    m_histogramChart->createDefaultAxes();
}

} // namespace ComponentsForest