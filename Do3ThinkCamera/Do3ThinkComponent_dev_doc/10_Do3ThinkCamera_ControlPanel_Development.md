# Do3ThinkCamera Control Panel Development Guide

## 1. Control Panel架構設計

### 1.1 設計原則
- **完全解耦**: Control Panel與Component之間無直接依賴
- **純Signal/Slot通訊**: 所有交互通過Qt信號槽機制
- **MVC/MVP模式**: 清晰的視圖-模型分離
- **可擴展性**: 支援多種UI實現（QWidget、QML）
- **響應式設計**: 自適應不同屏幕尺寸

### 1.2 架構概覽
```
┌─────────────────────────────────────┐
│     Do3ThinkCameraControlPanel      │
├─────────────────────────────────────┤
│  ┌──────────────┐  ┌──────────────┐│
│  │ Connection   │  │  Parameter   ││
│  │   Control    │  │   Control    ││
│  └──────────────┘  └──────────────┘│
│  ┌──────────────┐  ┌──────────────┐│
│  │    Image     │  │  Acquisition ││
│  │   Display    │  │   Control    ││
│  └──────────────┘  └──────────────┘│
│  ┌──────────────────────────────────┤
│  │        Status Monitor            ││
│  └──────────────────────────────────┘│
└─────────────────────────────────────┘
         ↕ Signal/Slot Only ↕
┌─────────────────────────────────────┐
│     Do3ThinkCameraComponent         │
└─────────────────────────────────────┘
```

## 2. QWidget版本實現

### 2.1 主控制面板（Do3ThinkCameraControlPanel.h）

```cpp
#ifndef DO3THINKCAMERACONTROLPANEL_H
#define DO3THINKCAMERACONTROLPANEL_H

#include <QWidget>
#include <QPointer>
#include <memory>

QT_BEGIN_NAMESPACE
class QComboBox;
class QPushButton;
class QSlider;
class QSpinBox;
class QLabel;
class QGroupBox;
class QStatusBar;
class QTabWidget;
QT_END_NAMESPACE

class Do3ThinkCameraComponent;
class ImageDisplayWidget;
class ParameterWidget;
class StatusMonitorWidget;

class Do3ThinkCameraControlPanel : public QWidget {
    Q_OBJECT
    
public:
    explicit Do3ThinkCameraControlPanel(QWidget* parent = nullptr);
    ~Do3ThinkCameraControlPanel();
    
    // Component連接管理
    void connectToComponent(Do3ThinkCameraComponent* component);
    void disconnectFromComponent();
    bool isConnected() const;
    
    // UI配置
    void setCompactMode(bool compact);
    void setTheme(const QString& theme);
    
public slots:
    // 從Component接收的槽
    void onImageReceived(const QImage& image);
    void onStatusChanged(const QString& status);
    void onParameterChanged(const QString& name, const QVariant& value);
    void onErrorOccurred(const QString& error);
    void onDeviceListUpdated(const QStringList& devices);
    
signals:
    // 發送到Component的信號
    void requestConnect(const QString& deviceName);
    void requestDisconnect();
    void requestStartAcquisition();
    void requestStopAcquisition();
    void requestSingleCapture();
    void requestParameterChange(const QString& name, const QVariant& value);
    void requestSaveImage(const QString& path);
    
private:
    void setupUI();
    void createConnectionControl();
    void createParameterControl();
    void createImageDisplay();
    void createAcquisitionControl();
    void createStatusMonitor();
    void createMenuBar();
    void createToolBar();
    
    void updateConnectionState(bool connected);
    void loadSettings();
    void saveSettings();
    
private:
    struct Impl;
    std::unique_ptr<Impl> d;
    QPointer<Do3ThinkCameraComponent> m_component;
};

#endif // DO3THINKCAMERACONTROLPANEL_H
```

### 2.2 主控制面板實現（Do3ThinkCameraControlPanel.cpp）

```cpp
#include "Do3ThinkCameraControlPanel.h"
#include "ImageDisplayWidget.h"
#include "ParameterWidget.h"
#include "StatusMonitorWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QComboBox>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QLabel>
#include <QTabWidget>
#include <QSplitter>
#include <QSettings>
#include <QTimer>

struct Do3ThinkCameraControlPanel::Impl {
    // Connection Control
    QComboBox* deviceCombo;
    QPushButton* connectBtn;
    QLabel* connectionStatus;
    
    // Parameter Control
    ParameterWidget* parameterWidget;
    
    // Image Display
    ImageDisplayWidget* imageDisplay;
    
    // Acquisition Control
    QPushButton* startBtn;
    QPushButton* stopBtn;
    QPushButton* singleCaptureBtn;
    QPushButton* saveImageBtn;
    
    // Status Monitor
    StatusMonitorWidget* statusMonitor;
    
    // Layout
    QTabWidget* tabWidget;
    QSplitter* mainSplitter;
    
    // State
    bool isConnected = false;
    QTimer* updateTimer;
};

Do3ThinkCameraControlPanel::Do3ThinkCameraControlPanel(QWidget* parent)
    : QWidget(parent)
    , d(std::make_unique<Impl>())
{
    setupUI();
    loadSettings();
    
    // 設置更新定時器
    d->updateTimer = new QTimer(this);
    d->updateTimer->setInterval(100); // 10Hz更新率
    connect(d->updateTimer, &QTimer::timeout, 
            this, [this]() {
                if (d->statusMonitor) {
                    d->statusMonitor->updateStatistics();
                }
            });
}

Do3ThinkCameraControlPanel::~Do3ThinkCameraControlPanel() {
    saveSettings();
}

void Do3ThinkCameraControlPanel::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    
    // 創建主分割器
    d->mainSplitter = new QSplitter(Qt::Horizontal, this);
    
    // 左側控制面板
    auto* controlWidget = new QWidget();
    auto* controlLayout = new QVBoxLayout(controlWidget);
    
    createConnectionControl();
    controlLayout->addWidget(createConnectionGroup());
    
    createParameterControl();
    controlLayout->addWidget(d->parameterWidget);
    
    createAcquisitionControl();
    controlLayout->addWidget(createAcquisitionGroup());
    
    controlLayout->addStretch();
    
    // 右側顯示區域
    auto* displayWidget = new QWidget();
    auto* displayLayout = new QVBoxLayout(displayWidget);
    
    createImageDisplay();
    displayLayout->addWidget(d->imageDisplay);
    
    createStatusMonitor();
    displayLayout->addWidget(d->statusMonitor);
    
    // 添加到分割器
    d->mainSplitter->addWidget(controlWidget);
    d->mainSplitter->addWidget(displayWidget);
    d->mainSplitter->setStretchFactor(0, 1);
    d->mainSplitter->setStretchFactor(1, 3);
    
    mainLayout->addWidget(d->mainSplitter);
}

void Do3ThinkCameraControlPanel::connectToComponent(Do3ThinkCameraComponent* component) {
    if (m_component) {
        disconnectFromComponent();
    }
    
    m_component = component;
    if (!m_component) return;
    
    // 連接從Component來的信號
    connect(m_component, SIGNAL(imageAcquired(QImage)),
            this, SLOT(onImageReceived(QImage)));
    connect(m_component, SIGNAL(statusChanged(QString)),
            this, SLOT(onStatusChanged(QString)));
    connect(m_component, SIGNAL(parameterChanged(QString, QVariant)),
            this, SLOT(onParameterChanged(QString, QVariant)));
    connect(m_component, SIGNAL(errorOccurred(QString)),
            this, SLOT(onErrorOccurred(QString)));
    connect(m_component, SIGNAL(deviceListUpdated(QStringList)),
            this, SLOT(onDeviceListUpdated(QStringList)));
    
    // 連接到Component的信號
    connect(this, SIGNAL(requestConnect(QString)),
            m_component, SLOT(connectCamera(QString)));
    connect(this, SIGNAL(requestDisconnect()),
            m_component, SLOT(disconnectCamera()));
    connect(this, SIGNAL(requestStartAcquisition()),
            m_component, SLOT(startAcquisition()));
    connect(this, SIGNAL(requestStopAcquisition()),
            m_component, SLOT(stopAcquisition()));
    connect(this, SIGNAL(requestSingleCapture()),
            m_component, SLOT(singleCapture()));
    connect(this, SIGNAL(requestParameterChange(QString, QVariant)),
            m_component, SLOT(setParameter(QString, QVariant)));
    
    d->updateTimer->start();
}

void Do3ThinkCameraControlPanel::disconnectFromComponent() {
    if (m_component) {
        disconnect(m_component, nullptr, this, nullptr);
        disconnect(this, nullptr, m_component, nullptr);
        m_component = nullptr;
    }
    d->updateTimer->stop();
}
```

### 2.3 圖像顯示Widget（ImageDisplayWidget.h）

```cpp
#ifndef IMAGEDISPLAYWIDGET_H
#define IMAGEDISPLAYWIDGET_H

#include <QWidget>
#include <QImage>
#include <memory>

class ImageDisplayWidget : public QWidget {
    Q_OBJECT
    
public:
    explicit ImageDisplayWidget(QWidget* parent = nullptr);
    ~ImageDisplayWidget();
    
    // 圖像操作
    void setImage(const QImage& image);
    void clearImage();
    
    // 顯示控制
    void setFitToWindow(bool fit);
    void setZoom(double factor);
    void resetView();
    
    // ROI操作
    void enableROISelection(bool enable);
    QRect getROI() const;
    void setROI(const QRect& roi);
    
    // 測量工具
    void enableMeasurement(bool enable);
    void setMeasurementTool(const QString& tool);
    
    // 圖像增強
    void setContrast(double value);
    void setBrightness(double value);
    void setGamma(double value);
    
    // 覆蓋顯示
    void showHistogram(bool show);
    void showCrosshair(bool show);
    void showGrid(bool show);
    void showPixelInfo(bool show);
    
signals:
    void roiSelected(const QRect& roi);
    void pixelClicked(const QPoint& pos, const QRgb& value);
    void measurementCompleted(const QString& result);
    void zoomChanged(double factor);
    
protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    
private:
    void drawImage(QPainter& painter);
    void drawROI(QPainter& painter);
    void drawMeasurement(QPainter& painter);
    void drawOverlays(QPainter& painter);
    void updateTransform();
    QPointF imageToWidget(const QPointF& imagePos) const;
    QPointF widgetToImage(const QPointF& widgetPos) const;
    
private:
    struct Impl;
    std::unique_ptr<Impl> d;
};

#endif // IMAGEDISPLAYWIDGET_H
```

### 2.4 參數控制Widget（ParameterWidget.h）

```cpp
#ifndef PARAMETERWIDGET_H
#define PARAMETERWIDGET_H

#include <QWidget>
#include <QVariant>
#include <memory>

class ParameterWidget : public QWidget {
    Q_OBJECT
    
public:
    explicit ParameterWidget(QWidget* parent = nullptr);
    ~ParameterWidget();
    
    // 參數定義
    struct ParameterInfo {
        QString name;
        QString displayName;
        QVariant::Type type;
        QVariant minValue;
        QVariant maxValue;
        QVariant defaultValue;
        QVariant currentValue;
        QString unit;
        QString tooltip;
        bool readOnly;
        QStringList enumValues; // for enum types
    };
    
    // 參數管理
    void addParameter(const ParameterInfo& info);
    void removeParameter(const QString& name);
    void clearParameters();
    
    // 參數值操作
    void setParameterValue(const QString& name, const QVariant& value);
    QVariant getParameterValue(const QString& name) const;
    void setParameterEnabled(const QString& name, bool enabled);
    
    // 預設管理
    void savePreset(const QString& name);
    void loadPreset(const QString& name);
    void deletePreset(const QString& name);
    QStringList getPresetList() const;
    
    // 批量操作
    void setAllToDefault();
    QVariantMap getAllParameters() const;
    void setAllParameters(const QVariantMap& params);
    
signals:
    void parameterChanged(const QString& name, const QVariant& value);
    void presetLoaded(const QString& presetName);
    void presetSaved(const QString& presetName);
    
private:
    void createParameterUI(const ParameterInfo& info);
    void updateParameterUI(const QString& name);
    
private:
    struct Impl;
    std::unique_ptr<Impl> d;
};

#endif // PARAMETERWIDGET_H
```

### 2.5 狀態監控Widget（StatusMonitorWidget.h）

```cpp
#ifndef STATUSMONITORWIDGET_H
#define STATUSMONITORWIDGET_H

#include <QWidget>
#include <memory>

class StatusMonitorWidget : public QWidget {
    Q_OBJECT
    
public:
    explicit StatusMonitorWidget(QWidget* parent = nullptr);
    ~StatusMonitorWidget();
    
    // 統計信息更新
    void updateFPS(double fps);
    void updateFrameCount(qint64 count);
    void updateDroppedFrames(qint64 count);
    void updateBandwidth(double mbps);
    void updateTemperature(double temp);
    
    // 狀態信息
    void setConnectionStatus(const QString& status);
    void setAcquisitionStatus(const QString& status);
    void setErrorMessage(const QString& error);
    void clearError();
    
    // 性能監控
    void startMonitoring();
    void stopMonitoring();
    void resetStatistics();
    
    // 日誌
    void addLogMessage(const QString& message, int level = 0);
    void clearLog();
    void saveLog(const QString& filename);
    
public slots:
    void updateStatistics();
    
signals:
    void errorClicked(const QString& error);
    void logExported(const QString& filename);
    
private:
    void setupUI();
    void updateDisplay();
    
private:
    struct Impl;
    std::unique_ptr<Impl> d;
};

#endif // STATUSMONITORWIDGET_H
```

## 3. QML版本實現（可選）

### 3.1 主QML界面（Do3ThinkCameraControl.qml）

```qml
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls.Material 2.15
import Do3Think.Camera 1.0

ApplicationWindow {
    id: window
    width: 1280
    height: 800
    visible: true
    title: qsTr("Do3Think Camera Control")
    
    Material.theme: Material.Dark
    Material.accent: Material.Blue
    
    // C++後端接口
    CameraController {
        id: cameraController
        onImageReceived: imageDisplay.updateImage(image)
        onStatusChanged: statusBar.updateStatus(status)
        onErrorOccurred: errorDialog.show(error)
    }
    
    // 主佈局
    RowLayout {
        anchors.fill: parent
        spacing: 0
        
        // 左側控制面板
        ControlPanel {
            id: controlPanel
            Layout.preferredWidth: 350
            Layout.fillHeight: true
            controller: cameraController
            
            ConnectionSection {
                id: connectionSection
                onConnectRequested: cameraController.connectCamera(device)
                onDisconnectRequested: cameraController.disconnectCamera()
            }
            
            ParameterSection {
                id: parameterSection
                onParameterChanged: cameraController.setParameter(name, value)
            }
            
            AcquisitionSection {
                id: acquisitionSection
                onStartRequested: cameraController.startAcquisition()
                onStopRequested: cameraController.stopAcquisition()
                onSingleCaptureRequested: cameraController.singleCapture()
            }
        }
        
        // 中間分隔線
        Rectangle {
            Layout.preferredWidth: 1
            Layout.fillHeight: true
            color: Material.dividerColor
        }
        
        // 右側顯示區域
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0
            
            // 工具欄
            ToolBar {
                Layout.fillWidth: true
                height: 48
                
                RowLayout {
                    anchors.fill: parent
                    
                    ToolButton {
                        icon.source: "qrc:/icons/zoom_in.svg"
                        onClicked: imageDisplay.zoomIn()
                    }
                    
                    ToolButton {
                        icon.source: "qrc:/icons/zoom_out.svg"
                        onClicked: imageDisplay.zoomOut()
                    }
                    
                    ToolButton {
                        icon.source: "qrc:/icons/fit_screen.svg"
                        onClicked: imageDisplay.fitToWindow()
                    }
                    
                    ToolSeparator {}
                    
                    ToolButton {
                        icon.source: "qrc:/icons/roi.svg"
                        checkable: true
                        onCheckedChanged: imageDisplay.roiMode = checked
                    }
                    
                    ToolButton {
                        icon.source: "qrc:/icons/measure.svg"
                        checkable: true
                        onCheckedChanged: imageDisplay.measureMode = checked
                    }
                    
                    Item { Layout.fillWidth: true }
                    
                    Label {
                        text: "FPS: " + statusMonitor.fps.toFixed(1)
                        font.family: "Consolas"
                    }
                }
            }
            
            // 圖像顯示
            ImageDisplay {
                id: imageDisplay
                Layout.fillWidth: true
                Layout.fillHeight: true
                
                onRoiSelected: cameraController.setROI(roi)
                onPixelInfoRequested: pixelInfoPopup.show(x, y, value)
            }
            
            // 狀態欄
            StatusBar {
                id: statusBar
                Layout.fillWidth: true
                height: 32
                
                StatusMonitor {
                    id: statusMonitor
                    anchors.fill: parent
                }
            }
        }
    }
    
    // 錯誤對話框
    ErrorDialog {
        id: errorDialog
    }
    
    // 像素信息彈出窗口
    PixelInfoPopup {
        id: pixelInfoPopup
    }
    
    // 設置對話框
    SettingsDialog {
        id: settingsDialog
        onThemeChanged: {
            Material.theme = (theme === "dark") ? Material.Dark : Material.Light
        }
    }
}
```

### 3.2 QML與C++整合（CameraController.h）

```cpp
#ifndef CAMERACONTROLLER_H
#define CAMERACONTROLLER_H

#include <QObject>
#include <QImage>
#include <QVariant>
#include <QStringList>

class Do3ThinkCameraComponent;

class CameraController : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool connected READ isConnected NOTIFY connectedChanged)
    Q_PROPERTY(bool acquiring READ isAcquiring NOTIFY acquiringChanged)
    Q_PROPERTY(double fps READ fps NOTIFY fpsChanged)
    Q_PROPERTY(QStringList deviceList READ deviceList NOTIFY deviceListChanged)
    
public:
    explicit CameraController(QObject* parent = nullptr);
    ~CameraController();
    
    // 屬性訪問器
    bool isConnected() const;
    bool isAcquiring() const;
    double fps() const;
    QStringList deviceList() const;
    
    // 註冊到QML
    static void registerQmlTypes();
    
public slots:
    // 相機控制
    void connectCamera(const QString& device);
    void disconnectCamera();
    void startAcquisition();
    void stopAcquisition();
    void singleCapture();
    
    // 參數設置
    void setParameter(const QString& name, const QVariant& value);
    QVariant getParameter(const QString& name) const;
    
    // ROI設置
    void setROI(const QRect& roi);
    QRect getROI() const;
    
    // 圖像保存
    void saveImage(const QString& path);
    void saveImageAs();
    
signals:
    // 屬性變化信號
    void connectedChanged(bool connected);
    void acquiringChanged(bool acquiring);
    void fpsChanged(double fps);
    void deviceListChanged(const QStringList& devices);
    
    // 事件信號
    void imageReceived(const QImage& image);
    void statusChanged(const QString& status);
    void errorOccurred(const QString& error);
    void parameterChanged(const QString& name, const QVariant& value);
    
private:
    void setupComponent();
    void updateStatistics();
    
private:
    class Private;
    std::unique_ptr<Private> d;
};

#endif // CAMERACONTROLLER_H
```

### 3.3 QML註冊（main.cpp片段）

```cpp
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "CameraController.h"
#include "ImageProvider.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    
    // 註冊QML類型
    CameraController::registerQmlTypes();
    
    // 註冊圖像提供者
    QQmlApplicationEngine engine;
    engine.addImageProvider("camera", new CameraImageProvider());
    
    // 設置全局上下文屬性
    CameraController controller;
    engine.rootContext()->setContextProperty("cameraController", &controller);
    
    // 載入主QML文件
    const QUrl url(QStringLiteral("qrc:/qml/Do3ThinkCameraControl.qml"));
    engine.load(url);
    
    return app.exec();
}

// CameraController.cpp中的註冊函數
void CameraController::registerQmlTypes()
{
    qmlRegisterType<CameraController>("Do3Think.Camera", 1, 0, "CameraController");
    qmlRegisterUncreatableType<CameraStatus>("Do3Think.Camera", 1, 0, "CameraStatus",
                                              "CameraStatus is an enum type");
}
```

## 4. Signal/Slot連接設計

### 4.1 從Component到Panel的信號流

```cpp
// 圖像數據流
connect(component, &Do3ThinkCameraComponent::imageAcquired,
        panel, [panel](const QImage& image, qint64 timestamp) {
            panel->updateDisplay(image);
            panel->updateTimestamp(timestamp);
        });

// 狀態更新流
connect(component, &Do3ThinkCameraComponent::statusChanged,
        panel, &Do3ThinkCameraControlPanel::onStatusChanged);

// 參數同步
connect(component, &Do3ThinkCameraComponent::parameterChanged,
        panel, [panel](const QString& name, const QVariant& value) {
            panel->updateParameterUI(name, value);
            panel->logParameterChange(name, value);
        });

// 錯誤處理
connect(component, &Do3ThinkCameraComponent::errorOccurred,
        panel, [panel](int code, const QString& message) {
            panel->showError(QString("Error %1: %2").arg(code).arg(message));
            panel->stopAcquisitionUI();
        });

// 設備列表更新
connect(component, &Do3ThinkCameraComponent::deviceListUpdated,
        panel, &Do3ThinkCameraControlPanel::updateDeviceList);

// 性能統計
connect(component, &Do3ThinkCameraComponent::statisticsUpdated,
        panel, [panel](const CameraStatistics& stats) {
            panel->updateFPS(stats.fps);
            panel->updateDroppedFrames(stats.droppedFrames);
            panel->updateBandwidth(stats.bandwidth);
        });
```

### 4.2 從Panel到Component的控制流

```cpp
// 連接控制
connect(panel->connectButton, &QPushButton::clicked,
        [component, panel]() {
            QString device = panel->getSelectedDevice();
            component->connectCamera(device);
        });

// 參數調整
connect(panel->exposureSlider, &QSlider::valueChanged,
        [component](int value) {
            component->setExposureTime(value);
        });

connect(panel->gainSlider, &QSlider::valueChanged,
        [component](int value) {
            component->setGain(value);
        });

// 觸發模式
connect(panel->triggerModeCombo, 
        QOverload<int>::of(&QComboBox::currentIndexChanged),
        [component](int index) {
            component->setTriggerMode(static_cast<TriggerMode>(index));
        });

// 採集控制
connect(panel->startButton, &QPushButton::clicked,
        component, &Do3ThinkCameraComponent::startAcquisition);

connect(panel->stopButton, &QPushButton::clicked,
        component, &Do3ThinkCameraComponent::stopAcquisition);

// ROI設置
connect(panel->imageDisplay, &ImageDisplayWidget::roiSelected,
        [component](const QRect& roi) {
            component->setROI(roi);
        });

// 圖像保存
connect(panel->saveButton, &QPushButton::clicked,
        [component, panel]() {
            QString path = panel->getSaveFilePath();
            if (!path.isEmpty()) {
                component->saveImage(path);
            }
        });
```

### 4.3 雙向綁定模式

```cpp
class ParameterBinding : public QObject {
    Q_OBJECT
public:
    static void bind(QWidget* widget, 
                    Do3ThinkCameraComponent* component,
                    const QString& parameterName) {
        
        if (auto* slider = qobject_cast<QSlider*>(widget)) {
            // Widget -> Component
            connect(slider, &QSlider::valueChanged,
                    [component, parameterName](int value) {
                        component->setParameter(parameterName, value);
                    });
            
            // Component -> Widget
            connect(component, &Do3ThinkCameraComponent::parameterChanged,
                    [slider, parameterName](const QString& name, const QVariant& value) {
                        if (name == parameterName) {
                            slider->blockSignals(true);
                            slider->setValue(value.toInt());
                            slider->blockSignals(false);
                        }
                    });
        }
        // 支援其他Widget類型...
    }
};
```

## 5. 高級功能

### 5.1 參數預設管理

```cpp
class PresetManager : public QObject {
    Q_OBJECT
public:
    struct Preset {
        QString name;
        QString description;
        QVariantMap parameters;
        QDateTime created;
        QDateTime modified;
    };
    
    // 預設操作
    void savePreset(const QString& name, const QVariantMap& params);
    void loadPreset(const QString& name);
    void deletePreset(const QString& name);
    void renamePreset(const QString& oldName, const QString& newName);
    
    // 預設列表
    QStringList getPresetNames() const;
    Preset getPreset(const QString& name) const;
    
    // 導入/導出
    void exportPreset(const QString& name, const QString& filename);
    void importPreset(const QString& filename);
    void exportAllPresets(const QString& filename);
    void importAllPresets(const QString& filename);
    
    // 默認預設
    void setDefaultPreset(const QString& name);
    QString getDefaultPreset() const;
    
signals:
    void presetSaved(const QString& name);
    void presetLoaded(const QString& name);
    void presetDeleted(const QString& name);
    void presetListChanged();
    
private:
    void saveToSettings();
    void loadFromSettings();
    
private:
    QMap<QString, Preset> m_presets;
    QString m_defaultPreset;
};
```

### 5.2 圖像工具

```cpp
class ImageTools : public QObject {
    Q_OBJECT
public:
    // 測量工具
    class MeasurementTool {
    public:
        virtual void startMeasurement(const QPoint& start) = 0;
        virtual void updateMeasurement(const QPoint& current) = 0;
        virtual void endMeasurement(const QPoint& end) = 0;
        virtual QString getResult() const = 0;
        virtual void draw(QPainter& painter) const = 0;
    };
    
    class LineMeasurement : public MeasurementTool {
        // 測量兩點距離
    };
    
    class AngleMeasurement : public MeasurementTool {
        // 測量角度
    };
    
    class AreaMeasurement : public MeasurementTool {
        // 測量面積
    };
    
    // ROI工具
    class ROITool {
    public:
        enum Shape { Rectangle, Circle, Polygon, Freehand };
        
        void setShape(Shape shape);
        void startSelection(const QPoint& start);
        void updateSelection(const QPoint& current);
        void endSelection(const QPoint& end);
        QRegion getROI() const;
        void draw(QPainter& painter) const;
    };
    
    // 圖像標註
    class AnnotationTool {
    public:
        void addText(const QPoint& pos, const QString& text);
        void addArrow(const QPoint& start, const QPoint& end);
        void addShape(const QRect& rect, const QColor& color);
        void clearAnnotations();
        void saveAnnotations(const QString& filename);
        void loadAnnotations(const QString& filename);
        void draw(QPainter& painter) const;
    };
    
    // 直方圖分析
    class HistogramAnalyzer {
    public:
        void analyze(const QImage& image);
        QVector<int> getRedHistogram() const;
        QVector<int> getGreenHistogram() const;
        QVector<int> getBlueHistogram() const;
        QVector<int> getGrayHistogram() const;
        double getMean() const;
        double getStdDev() const;
        int getMin() const;
        int getMax() const;
        void draw(QPainter& painter, const QRect& rect) const;
    };
    
    // 圖像增強
    class ImageEnhancer {
    public:
        QImage adjustBrightness(const QImage& image, int value);
        QImage adjustContrast(const QImage& image, double value);
        QImage adjustGamma(const QImage& image, double gamma);
        QImage equalizeHistogram(const QImage& image);
        QImage applyColorMap(const QImage& image, const QString& map);
    };
};
```

### 5.3 診斷工具

```cpp
class DiagnosticTools : public QWidget {
    Q_OBJECT
public:
    // 連接診斷
    class ConnectionDiagnostic {
    public:
        struct Result {
            bool success;
            QString deviceInfo;
            QString driverVersion;
            QString firmwareVersion;
            QStringList supportedFeatures;
            QStringList issues;
        };
        
        Result diagnoseConnection(const QString& device);
        void testBandwidth();
        void testLatency();
        void testStability();
    };
    
    // 性能分析
    class PerformanceAnalyzer {
    public:
        void startProfiling();
        void stopProfiling();
        
        struct Profile {
            double avgFPS;
            double minFPS;
            double maxFPS;
            qint64 totalFrames;
            qint64 droppedFrames;
            double avgLatency;
            double cpuUsage;
            double memoryUsage;
            QVector<double> fpsHistory;
        };
        
        Profile getProfile() const;
        void exportProfile(const QString& filename);
    };
    
    // 日誌查看器
    class LogViewer : public QWidget {
    public:
        void addLog(const QString& message, LogLevel level);
        void setFilter(LogLevel minLevel);
        void setTextFilter(const QString& text);
        void exportLogs(const QString& filename);
        void clearLogs();
    };
    
    // 錯誤報告
    class ErrorReporter {
    public:
        struct ErrorReport {
            QDateTime timestamp;
            QString errorCode;
            QString message;
            QString stackTrace;
            QVariantMap systemInfo;
            QVariantMap cameraState;
        };
        
        void reportError(const ErrorReport& report);
        void sendReport(const QString& email);
        void saveReport(const QString& filename);
    };
};
```

## 6. 樣式與主題

### 6.1 樣式表定義

```css
/* Do3ThinkCameraControl.qss */

/* 深色主題 */
Do3ThinkCameraControlPanel[theme="dark"] {
    background-color: #2b2b2b;
    color: #ffffff;
}

Do3ThinkCameraControlPanel[theme="dark"] QGroupBox {
    border: 1px solid #555555;
    border-radius: 4px;
    margin-top: 8px;
    padding-top: 8px;
}

Do3ThinkCameraControlPanel[theme="dark"] QGroupBox::title {
    color: #ffffff;
    subcontrol-origin: margin;
    left: 10px;
    padding: 0 5px 0 5px;
}

/* 連接狀態指示器 */
QLabel#connectionStatus[connected="true"] {
    background-color: #4CAF50;
    border-radius: 8px;
    padding: 2px 8px;
}

QLabel#connectionStatus[connected="false"] {
    background-color: #F44336;
    border-radius: 8px;
    padding: 2px 8px;
}

/* 按鈕樣式 */
QPushButton {
    background-color: #0d7ed8;
    color: white;
    border: none;
    border-radius: 4px;
    padding: 6px 12px;
    font-weight: bold;
}

QPushButton:hover {
    background-color: #0b6bc0;
}

QPushButton:pressed {
    background-color: #0958a8;
}

QPushButton:disabled {
    background-color: #cccccc;
    color: #666666;
}

/* 滑塊樣式 */
QSlider::groove:horizontal {
    height: 6px;
    background: #d3d3d3;
    border-radius: 3px;
}

QSlider::handle:horizontal {
    width: 18px;
    height: 18px;
    background: #0d7ed8;
    border-radius: 9px;
    margin: -6px 0;
}

QSlider::sub-page:horizontal {
    background: #0d7ed8;
    border-radius: 3px;
}

/* 淺色主題 */
Do3ThinkCameraControlPanel[theme="light"] {
    background-color: #f5f5f5;
    color: #333333;
}

/* 其他主題樣式... */
```

### 6.2 主題管理器

```cpp
class ThemeManager : public QObject {
    Q_OBJECT
public:
    enum Theme {
        Light,
        Dark,
        Blue,
        Custom
    };
    
    static ThemeManager* instance();
    
    void setTheme(Theme theme);
    Theme currentTheme() const;
    
    void loadCustomTheme(const QString& filename);
    void saveCustomTheme(const QString& filename);
    
    QString getStyleSheet() const;
    QPalette getPalette() const;
    QMap<QString, QColor> getColorScheme() const;
    
    // 圖標管理
    QIcon getIcon(const QString& name) const;
    void setIconTheme(const QString& theme);
    
signals:
    void themeChanged(Theme theme);
    
private:
    ThemeManager();
    void applyTheme();
    
private:
    Theme m_currentTheme;
    QString m_customStyleSheet;
    QMap<QString, QIcon> m_icons;
};
```

## 7. 國際化支援

### 7.1 翻譯系統

```cpp
class TranslationManager : public QObject {
    Q_OBJECT
public:
    static TranslationManager* instance();
    
    // 語言管理
    void setLanguage(const QString& language);
    QString currentLanguage() const;
    QStringList availableLanguages() const;
    
    // 翻譯載入
    bool loadTranslation(const QString& language);
    void reloadTranslations();
    
    // 動態翻譯
    QString translate(const QString& context, 
                     const QString& key,
                     const QString& disambiguation = QString()) const;
    
signals:
    void languageChanged(const QString& language);
    
private:
    void installTranslators();
    void removeTranslators();
    
private:
    QTranslator* m_qtTranslator;
    QTranslator* m_appTranslator;
    QString m_currentLanguage;
};

// 使用宏簡化翻譯
#define TR(key) TranslationManager::instance()->translate("Do3ThinkCamera", key)
```

### 7.2 翻譯文件示例

```xml
<!-- Do3ThinkCamera_zh_CN.ts -->
<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="zh_CN">
<context>
    <name>Do3ThinkCamera</name>
    <message>
        <source>Connect</source>
        <translation>連接</translation>
    </message>
    <message>
        <source>Disconnect</source>
        <translation>斷開</translation>
    </message>
    <message>
        <source>Start Acquisition</source>
        <translation>開始採集</translation>
    </message>
    <message>
        <source>Stop Acquisition</source>
        <translation>停止採集</translation>
    </message>
    <message>
        <source>Exposure Time</source>
        <translation>曝光時間</translation>
    </message>
    <message>
        <source>Gain</source>
        <translation>增益</translation>
    </message>
    <message>
        <source>Trigger Mode</source>
        <translation>觸發模式</translation>
    </message>
    <message>
        <source>Save Image</source>
        <translation>保存圖像</translation>
    </message>
</context>
</TS>
```

## 8. 部署考慮

### 8.1 資源管理

```cmake
# CMakeLists.txt片段
# 資源文件
qt5_add_resources(RESOURCES
    resources/icons.qrc
    resources/themes.qrc
    resources/translations.qrc
)

# 翻譯文件
qt5_add_translation(QM_FILES
    translations/Do3ThinkCamera_zh_CN.ts
    translations/Do3ThinkCamera_en_US.ts
    translations/Do3ThinkCamera_ja_JP.ts
)

# 安裝規則
install(TARGETS Do3ThinkCameraControlPanel
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(FILES ${QM_FILES}
    DESTINATION translations
)

install(DIRECTORY resources/icons
    DESTINATION share/do3think/camera
)

install(FILES resources/themes/*.qss
    DESTINATION share/do3think/camera/themes
)
```

### 8.2 依賴管理

```cmake
# 查找依賴
find_package(Qt5 REQUIRED COMPONENTS
    Core
    Widgets
    Gui
    Quick
    QuickControls2
    LinguistTools
)

# 可選依賴
find_package(Qt5Charts)
if(Qt5Charts_FOUND)
    add_definitions(-DHAS_QT_CHARTS)
    target_link_libraries(Do3ThinkCameraControlPanel Qt5::Charts)
endif()

# 打包依賴
if(WIN32)
    # Windows部署
    add_custom_command(TARGET Do3ThinkCameraControlPanel POST_BUILD
        COMMAND ${CMAKE_PREFIX_PATH}/bin/windeployqt.exe 
                $<TARGET_FILE:Do3ThinkCameraControlPanel>
    )
elseif(APPLE)
    # macOS部署
    add_custom_command(TARGET Do3ThinkCameraControlPanel POST_BUILD
        COMMAND ${CMAKE_PREFIX_PATH}/bin/macdeployqt 
                $<TARGET_FILE:Do3ThinkCameraControlPanel>
    )
endif()
```

### 8.3 安裝程序配置

```nsis
; Windows NSIS安裝腳本
Name "Do3Think Camera Control Panel"
OutFile "Do3ThinkCameraPanel-Setup.exe"
InstallDir "$PROGRAMFILES\Do3Think\CameraPanel"

Section "Main Application"
    SetOutPath "$INSTDIR"
    File "Do3ThinkCameraControlPanel.exe"
    File "*.dll"
    
    SetOutPath "$INSTDIR\platforms"
    File "platforms\*.dll"
    
    SetOutPath "$INSTDIR\styles"
    File "styles\*.dll"
    
    SetOutPath "$INSTDIR\translations"
    File "translations\*.qm"
    
    SetOutPath "$INSTDIR\themes"
    File "themes\*.qss"
    
    CreateShortcut "$DESKTOP\Do3Think Camera.lnk" "$INSTDIR\Do3ThinkCameraControlPanel.exe"
    CreateShortcut "$SMPROGRAMS\Do3Think Camera.lnk" "$INSTDIR\Do3ThinkCameraControlPanel.exe"
SectionEnd

Section "Uninstall"
    Delete "$INSTDIR\*.*"
    RMDir /r "$INSTDIR"
    Delete "$DESKTOP\Do3Think Camera.lnk"
    Delete "$SMPROGRAMS\Do3Think Camera.lnk"
SectionEnd
```

## 9. 測試指南

### 9.1 單元測試

```cpp
class TestControlPanel : public QObject {
    Q_OBJECT
private slots:
    void initTestCase();
    void cleanupTestCase();
    
    void testConnection();
    void testParameterBinding();
    void testImageDisplay();
    void testPresetManagement();
    void testSignalSlotConnection();
    void testThemeSwitch();
    void testLanguageSwitch();
    
private:
    Do3ThinkCameraControlPanel* m_panel;
    MockCameraComponent* m_mockComponent;
};
```

### 9.2 集成測試

```cpp
class IntegrationTest : public QObject {
    Q_OBJECT
private slots:
    void testFullWorkflow();
    void testErrorHandling();
    void testPerformance();
    void testMemoryLeaks();
};
```

## 10. 性能優化建議

### 10.1 圖像顯示優化
- 使用OpenGL加速渲染
- 實現圖像緩存機制
- 異步圖像處理
- 降採樣顯示大圖像

### 10.2 UI響應優化
- 參數更新防抖動
- 異步操作不阻塞UI
- 使用線程池處理重任務
- 延遲載入非關鍵組件

### 10.3 內存管理
- 圖像緩衝區循環使用
- 及時釋放不用資源
- 使用智能指針管理生命週期
- 監控內存使用情況

## 總結

本開發手冊提供了Do3ThinkCamera Control Panel的完整開發指南，包括：

1. **架構設計**: 完全解耦的MVC架構，純Signal/Slot通訊
2. **QWidget實現**: 傳統桌面應用UI，功能完整
3. **QML實現**: 現代化UI，支援動畫和響應式設計
4. **高級功能**: 預設管理、圖像工具、診斷工具
5. **主題和國際化**: 支援多主題和多語言
6. **部署方案**: 完整的打包和安裝配置

Control Panel設計充分考慮了可擴展性和可維護性，與Component完全解耦，可以獨立開發和測試。通過標準的Qt Signal/Slot機制通訊，確保了系統的穩定性和可靠性。