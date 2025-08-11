#ifndef DO3THINK_CAMERA_VIEWER_MAIN_UI_H
#define DO3THINK_CAMERA_VIEWER_MAIN_UI_H

#include <QMainWindow>
#include <QMap>
#include <memory>

QT_BEGIN_NAMESPACE
class QAction;
class QMenu;
class QToolBar;
class QStatusBar;
class QDockWidget;
class QTabWidget;
class QTextEdit;
class QListWidget;
class QSplitter;
class QPushButton;
class QTextEdit;
QT_END_NAMESPACE

namespace ComponentsForest {

class Do3ThinkCameraControlPanel;
class Do3ThinkCameraMachine;
class BaseComponent;

namespace OpenCV {
    class PreProcessorControlPanel;
    class PreProcessorBase;
}

class Do3ThinkCameraViewerMainUI : public QMainWindow
{
    Q_OBJECT

public:
    explicit Do3ThinkCameraViewerMainUI(QWidget *parent = nullptr);
    ~Do3ThinkCameraViewerMainUI();

    // Machine connection
    void connectToMachine(Do3ThinkCameraMachine *machine);
    void disconnectFromMachine();
    
    // Control panel management
    bool addCameraPanel(const QString &cameraId, BaseComponent *component);
    bool removeCameraPanel(const QString &cameraId);
    void clearAllPanels();
    QList<Do3ThinkCameraControlPanel*> getAllPanels() const;
    Do3ThinkCameraControlPanel* getActivePanel() const;
    
    // Preprocessor panel management
    bool addPreprocessorPanel(const QString &processorId, OpenCV::PreProcessorBase *preprocessor);
    bool removePreprocessorPanel(const QString &processorId);
    QList<OpenCV::PreProcessorControlPanel*> getAllPreprocessorPanels() const;
    OpenCV::PreProcessorControlPanel* getActivePreprocessorPanel() const;
    
    // UI state
    void setFullScreen(bool fullScreen);
    void showStatusMessage(const QString &message, int timeout = 5000);
    
    // Layout modes
    enum LayoutMode {
        SingleView,      // One camera at a time
        GridView,        // 2x2 grid
        TabView,         // Tabbed interface
        SplitView        // Customizable split
    };
    void setLayoutMode(LayoutMode mode);
    LayoutMode getLayoutMode() const;

signals:
    // Panel management actions
    void addCameraRequested();
    void removeCameraRequested(const QString &cameraId);
    void refreshDevicesRequested();
    
    // Application-level actions
    void settingsRequested();
    void layoutModeChangeRequested(LayoutMode mode);
    void fullScreenToggled(bool fullScreen);
    
    // Status notifications
    void panelActivated(const QString &cameraId);
    void panelDeactivated(const QString &cameraId);

public slots:
    // Machine connection slots
    void onMachineStarted();
    void onMachineStopped();
    void onMachineError(const QString &error);
    void onCameraAdded(const QString &cameraId);
    void onCameraRemoved(const QString &cameraId);
    void onCameraStarted(const QString &cameraId);
    void onCameraStopped(const QString &cameraId);
    void onCameraError(const QString &cameraId, const QString &error);
    void onDevicesDiscovered(const QStringList &devices);
    void onContourAnalysisResult(const QString &cameraId, const QVariantList &areas);

private slots:
    void onAnalyzeClicked();
    
    // Menu actions
    void onAddCamera();
    void onRemoveCamera();
    void onStartAllPanels();  // Batch operation through panels
    void onStopAllPanels();   // Batch operation through panels
    void onRefreshDevices();
    void onShowSettings();
    void onShowAbout();
    void onToggleFullScreen();
    void onChangeLayout();
    
    // Panel management
    void onPanelClosed(const QString &cameraId);
    void onCurrentCameraChanged(int index);
    
    // Logging
    void appendLog(const QString &message, const QString &level = "INFO");

private:
    void setupUI();
    void createMenus();
    void createToolBars();
    void createStatusBar();
    void createDockWidgets();
    void createCentralWidget();
    void connectSignals();
    void updateActions();
    
    // Layout management
    void applyLayoutMode();
    void arrangePanelsInGrid();
    void arrangePanelsInTabs();
    void arrangePanelsInSplit();
    
    // Helper methods
    QString getCurrentCameraId() const;
    Do3ThinkCameraControlPanel* getPanel(const QString &cameraId) const;
    void saveSettings();
    void loadSettings();
    void showWelcomeScreen();
    void updateStatusBar();

private:
    class Private;
    std::unique_ptr<Private> d;
    
    // UI Elements
    QMenu *m_fileMenu;
    QMenu *m_cameraMenu;
    QMenu *m_viewMenu;
    QMenu *m_processingMenu;  // New menu for processing options
    QMenu *m_toolsMenu;
    QMenu *m_helpMenu;
    
    QToolBar *m_mainToolBar;
    QToolBar *m_cameraToolBar;
    QToolBar *m_processingToolBar;  // New toolbar for processing
    
    QAction *m_addCameraAction;
    QAction *m_removeCameraAction;
    QAction *m_startAllPanelsAction;
    QAction *m_stopAllPanelsAction;
    QAction *m_refreshDevicesAction;
    QAction *m_fullScreenAction;
    QAction *m_settingsAction;
    QAction *m_aboutAction;
    QAction *m_exitAction;
    QAction *m_preprocessAction;  // Action to show/hide preprocessor dock
    
    // ROI actions (optional feature)
    QAction *m_roiModeAction;
    QAction *m_roiRectAction;
    QAction *m_roiCircleAction;
    QAction *m_roiPolygonAction;
    
    // Dock widgets
    QDockWidget *m_deviceListDock;
    QDockWidget *m_logDock;
    QDockWidget *m_propertiesDock;
    QDockWidget *m_imageProcessingDock;  // New dock for preprocessor panels
    
    QListWidget *m_deviceListWidget;
    QTextEdit *m_logTextEdit;
    QTabWidget *m_propertiesTabWidget;
    QTabWidget *m_imageProcessingTabWidget;  // Tab widget for preprocessor panels
    QDockWidget *m_analysisDock;
    QTextEdit* m_analysisResultEdit;
    
    // Central widget
    QWidget *m_centralContainer;
    QTabWidget *m_cameraTabWidget;
    QSplitter *m_cameraSplitter;
    
    // Panel management
    QMap<QString, Do3ThinkCameraControlPanel*> m_panels;
    QMap<QString, QWidget*> m_panelContainers;
    
    // Preprocessor panel management
    QMap<QString, OpenCV::PreProcessorControlPanel*> m_preprocessorPanels;
    QMap<QString, QWidget*> m_preprocessorContainers;
    
    // Machine connection
    Do3ThinkCameraMachine *m_machine;
    
    // Current state
    LayoutMode m_layoutMode;
    QString m_currentCameraId;
    bool m_isRecording;
    
    // ROI features (optional)
    bool m_roiFeaturesEnabled = false;
    QList<QDockWidget*> m_controlPanels;
    
// ROI-specific methods (defined in main_ui_roi.cpp)
public:
    void addROIEnabledControlPanel(class Do3ThinkCameraComponent* component, const QString& title);
    void enableROIFeatures(bool enable);
    void createROIMenuActions();
    void setROICreationMode(int mode);  // Uses OpenCV::ROISelectorWidget::CreationMode
    void clearAllROIs();
    void saveROIConfiguration();
    void loadROIConfiguration();
    void exportROIMasks();
    void showROIStatistics(bool show);
    void updateStatus(const QString& message) { showStatusMessage(message); }
};

} // namespace ComponentsForest

#endif // DO3THINK_CAMERA_VIEWER_MAIN_UI_H