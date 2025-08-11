#include "main_ui.h"
#include "../../Do3ThinkCamera/dothink_camera_roi_integration.h"
#include "../../Do3ThinkCamera/dothink_camera.h"
#include <QFileDialog>
#include <QMenuBar>
#include <QDockWidget>

// Extension functions for ROI support in the main UI

namespace ComponentsForest {

/**
 * @brief Create and add ROI-enabled camera control panel
 * 
 * This function creates a control panel with integrated ROI selection
 * capabilities and adds it to the main UI.
 */
void Do3ThinkCameraViewerMainUI::addROIEnabledControlPanel(
    Do3ThinkCameraComponent* component, 
    const QString& title) 
{
    // Create ROI-enabled control panel
    auto* roiPanel = new Do3Think::Do3ThinkCameraControlPanelWithROI(this);
    roiPanel->setCameraComponent(component);
    
    // Create dock widget for the panel
    QDockWidget* dock = new QDockWidget(title, this);
    dock->setWidget(roiPanel);
    dock->setObjectName(QString("ROI_%1_Dock").arg(title));
    
    // Add to main window
    addDockWidget(Qt::LeftDockWidgetArea, dock);
    
    // Store reference for later access
    m_controlPanels.append(dock);
    
    // Connect ROI signals to status updates
    auto* roiIntegration = roiPanel->getROIIntegration();
    
    connect(roiIntegration, &Do3Think::CameraROIIntegration::roiCreated,
            [this](const QString& id, const QRect& bounds) {
        updateStatus(QString("ROI created: %1 at (%2,%3) size %4x%5")
            .arg(id)
            .arg(bounds.x()).arg(bounds.y())
            .arg(bounds.width()).arg(bounds.height()));
    });
    
    connect(roiIntegration, &Do3Think::CameraROIIntegration::roiModified,
            [this](const QString& id, const QRect& bounds) {
        updateStatus(QString("ROI modified: %1").arg(id));
    });
    
    connect(roiIntegration, &Do3Think::CameraROIIntegration::roiDeleted,
            [this](const QString& id) {
        updateStatus(QString("ROI deleted: %1").arg(id));
    });
    
    // Connect processed frame output
    connect(roiPanel, &Do3Think::Do3ThinkCameraControlPanelWithROI::processedFrameReady,
            [this](const QImage& frame) {
        // Handle processed frame if needed
        // For example, save to file or display in separate window
    });
}

/**
 * @brief Enable ROI features for all camera control panels
 */
void Do3ThinkCameraViewerMainUI::enableROIFeatures(bool enable) 
{
    // This would be called to globally enable/disable ROI features
    m_roiFeaturesEnabled = enable;
    
    if (enable) {
        updateStatus("ROI features enabled - click and drag on camera view to create ROIs");
    } else {
        updateStatus("ROI features disabled");
    }
}

/**
 * @brief Create ROI menu actions
 */
void Do3ThinkCameraViewerMainUI::createROIMenuActions() 
{
    QMenu* roiMenu = menuBar()->addMenu(tr("&ROI"));
    
    // Enable/Disable ROI mode
    QAction* roiModeAction = roiMenu->addAction(tr("Enable ROI Mode"));
    roiModeAction->setCheckable(true);
    roiModeAction->setChecked(m_roiFeaturesEnabled);
    connect(roiModeAction, &QAction::toggled, this, 
            &Do3ThinkCameraViewerMainUI::enableROIFeatures);
    
    roiMenu->addSeparator();
    
    // ROI shapes
    QAction* rectROIAction = roiMenu->addAction(tr("Rectangle ROI"));
    rectROIAction->setShortcut(QKeySequence("Ctrl+R"));
    connect(rectROIAction, &QAction::triggered, [this]() {
        setROICreationMode(static_cast<int>(OpenCV::CreationTool::Rectangle));
    });
    
    QAction* circleROIAction = roiMenu->addAction(tr("Circle ROI"));
    circleROIAction->setShortcut(QKeySequence("Ctrl+C"));
    connect(circleROIAction, &QAction::triggered, [this]() {
        setROICreationMode(static_cast<int>(OpenCV::CreationTool::Circle));
    });
    
    QAction* polygonROIAction = roiMenu->addAction(tr("Polygon ROI"));
    polygonROIAction->setShortcut(QKeySequence("Ctrl+P"));
    connect(polygonROIAction, &QAction::triggered, [this]() {
        setROICreationMode(static_cast<int>(OpenCV::CreationTool::Polygon));
    });
    
    roiMenu->addSeparator();
    
    // ROI operations
    QAction* clearROIsAction = roiMenu->addAction(tr("Clear All ROIs"));
    clearROIsAction->setShortcut(QKeySequence("Ctrl+Shift+C"));
    connect(clearROIsAction, &QAction::triggered, this, 
            &Do3ThinkCameraViewerMainUI::clearAllROIs);
    
    roiMenu->addSeparator();
    
    // Save/Load ROI configurations
    QAction* saveROIAction = roiMenu->addAction(tr("Save ROI Configuration..."));
    saveROIAction->setShortcut(QKeySequence("Ctrl+Shift+S"));
    connect(saveROIAction, &QAction::triggered, this, 
            &Do3ThinkCameraViewerMainUI::saveROIConfiguration);
    
    QAction* loadROIAction = roiMenu->addAction(tr("Load ROI Configuration..."));
    loadROIAction->setShortcut(QKeySequence("Ctrl+Shift+L"));
    connect(loadROIAction, &QAction::triggered, this, 
            &Do3ThinkCameraViewerMainUI::loadROIConfiguration);
    
    roiMenu->addSeparator();
    
    // Export options
    QAction* exportMasksAction = roiMenu->addAction(tr("Export ROI Masks..."));
    connect(exportMasksAction, &QAction::triggered, this, 
            &Do3ThinkCameraViewerMainUI::exportROIMasks);
    
    // Statistics
    roiMenu->addSeparator();
    QAction* showStatsAction = roiMenu->addAction(tr("Show ROI Statistics"));
    showStatsAction->setCheckable(true);
    connect(showStatsAction, &QAction::toggled, this, 
            &Do3ThinkCameraViewerMainUI::showROIStatistics);
}

/**
 * @brief Set ROI creation mode for all panels
 */
void Do3ThinkCameraViewerMainUI::setROICreationMode(int mode) 
{
    // Iterate through all control panels and set mode
    for (auto* dockWidget : m_controlPanels) {
        auto* panel = qobject_cast<Do3Think::Do3ThinkCameraControlPanelWithROI*>(
            dockWidget->widget());
        if (panel) {
            panel->getROIIntegration()->getROISelectorWidget()->setCreationMode(mode);
        }
    }
    
    QString modeStr;
    switch (mode) {
    case OpenCV::ROISelectorWidget::CreationMode::Rectangle:
        modeStr = "Rectangle";
        break;
    case OpenCV::ROISelectorWidget::CreationMode::Circle:
        modeStr = "Circle";
        break;
    case OpenCV::ROISelectorWidget::CreationMode::Polygon:
        modeStr = "Polygon";
        break;
    default:
        modeStr = "None";
    }
    
    updateStatus(QString("ROI creation mode: %1").arg(modeStr));
}

/**
 * @brief Clear all ROIs from all panels
 */
void Do3ThinkCameraViewerMainUI::clearAllROIs() 
{
    for (auto* dockWidget : m_controlPanels) {
        auto* panel = qobject_cast<Do3Think::Do3ThinkCameraControlPanelWithROI*>(
            dockWidget->widget());
        if (panel) {
            panel->clearAllROIs();
        }
    }
    
    updateStatus("All ROIs cleared");
}

/**
 * @brief Save ROI configuration to file
 */
void Do3ThinkCameraViewerMainUI::saveROIConfiguration() 
{
    QString filename = QFileDialog::getSaveFileName(this, 
        tr("Save ROI Configuration"), 
        QString(), 
        tr("ROI Config (*.roi);;JSON Files (*.json)"));
    
    if (filename.isEmpty()) return;
    
    // Save from first active panel
    for (auto* dockWidget : m_controlPanels) {
        auto* panel = qobject_cast<Do3Think::Do3ThinkCameraControlPanelWithROI*>(
            dockWidget->widget());
        if (panel) {
            panel->getROIIntegration()->saveROIConfiguration(filename);
            updateStatus(QString("ROI configuration saved to %1").arg(filename));
            break;
        }
    }
}

/**
 * @brief Load ROI configuration from file
 */
void Do3ThinkCameraViewerMainUI::loadROIConfiguration() 
{
    QString filename = QFileDialog::getOpenFileName(this, 
        tr("Load ROI Configuration"), 
        QString(), 
        tr("ROI Config (*.roi);;JSON Files (*.json)"));
    
    if (filename.isEmpty()) return;
    
    // Load to all panels
    for (auto* dockWidget : m_controlPanels) {
        auto* panel = qobject_cast<Do3Think::Do3ThinkCameraControlPanelWithROI*>(
            dockWidget->widget());
        if (panel) {
            panel->getROIIntegration()->loadROIConfiguration(filename);
        }
    }
    
    updateStatus(QString("ROI configuration loaded from %1").arg(filename));
}

/**
 * @brief Export ROI masks to directory
 */
void Do3ThinkCameraViewerMainUI::exportROIMasks() 
{
    QString dir = QFileDialog::getExistingDirectory(this, 
        tr("Export ROI Masks to Directory"));
    
    if (dir.isEmpty()) return;
    
    // Export from first active panel
    for (auto* dockWidget : m_controlPanels) {
        auto* panel = qobject_cast<Do3Think::Do3ThinkCameraControlPanelWithROI*>(
            dockWidget->widget());
        if (panel) {
            panel->getROIIntegration()->exportROIMasks(dir);
            updateStatus(QString("ROI masks exported to %1").arg(dir));
            break;
        }
    }
}

/**
 * @brief Show/hide ROI statistics panel
 */
void Do3ThinkCameraViewerMainUI::showROIStatistics(bool show) 
{
    for (auto* dockWidget : m_controlPanels) {
        auto* panel = qobject_cast<Do3Think::Do3ThinkCameraControlPanelWithROI*>(
            dockWidget->widget());
        if (panel) {
            panel->setROIStatisticsVisible(show);
        }
    }
    
    updateStatus(show ? "ROI statistics enabled" : "ROI statistics disabled");
}

} // namespace ComponentsForest