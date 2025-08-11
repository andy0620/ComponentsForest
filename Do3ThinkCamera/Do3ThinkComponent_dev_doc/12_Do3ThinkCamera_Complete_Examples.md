# Do3ThinkCamera Component 完整範例程式碼
*Complete Examples for Do3ThinkCamera Component Development*

## 1. 項目結構

```
Do3ThinkCameraExample/
├── CMakeLists.txt              # CMake構建文件
├── main.cpp                     # 主程式入口
├── components/
│   ├── Do3ThinkCameraComponent.h
│   ├── Do3ThinkCameraComponent.cpp
│   └── BaseComponent.h
├── controlpanels/
│   ├── Do3ThinkCameraControlPanel.h
│   └── Do3ThinkCameraControlPanel.cpp
├── examples/
│   ├── SimpleAcquisition.cpp   # 簡單採集示例
│   ├── MultiCamera.cpp          # 多相機示例
│   ├── AOIInspection.cpp        # AOI檢測示例
│   └── HighSpeed.cpp            # 高速採集示例
├── resources/
│   ├── icons/
│   └── styles/
└── config/
    └── camera_config.json       # 相機配置文件
```

## 2. CMake構建配置

```cmake
cmake_minimum_required(VERSION 3.16)
project(Do3ThinkCameraExample VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

# Find packages
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Concurrent)
find_package(OpenCV REQUIRED)

# Do3Think SDK
set(DVP_SDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/sdk")
include_directories(${DVP_SDK_DIR}/include)
link_directories(${DVP_SDK_DIR}/lib)

# Sources
set(SOURCES
    main.cpp
    components/Do3ThinkCameraComponent.cpp
    controlpanels/Do3ThinkCameraControlPanel.cpp
)

set(HEADERS
    components/Do3ThinkCameraComponent.h
    components/BaseComponent.h
    controlpanels/Do3ThinkCameraControlPanel.h
)

# Create executable
add_executable(Do3ThinkCameraExample ${SOURCES} ${HEADERS})

# Link libraries
target_link_libraries(Do3ThinkCameraExample
    Qt6::Core
    Qt6::Widgets
    Qt6::Concurrent
    ${OpenCV_LIBS}
    DVPCamera64  # Do3Think SDK library
)

# Install
install(TARGETS Do3ThinkCameraExample
    RUNTIME DESTINATION bin
)
```

## 3. 主應用程式

```cpp
// main.cpp
#include <QApplication>
#include <QMainWindow>
#include <QHBoxLayout>
#include <QThread>
#include <QDebug>
#include "components/Do3ThinkCameraComponent.h"
#include "controlpanels/Do3ThinkCameraControlPanel.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget* parent = nullptr) : QMainWindow(parent) {
        setupUI();
        setupComponent();
        connectSignals();
    }
    
    ~MainWindow() {
        // 清理資源
        if (m_cameraComponent) {
            m_cameraComponent->stop();
            m_cameraThread->quit();
            m_cameraThread->wait();
        }
    }
    
private:
    void setupUI() {
        auto* centralWidget = new QWidget(this);
        auto* layout = new QHBoxLayout(centralWidget);
        
        // 創建控制面板
        m_controlPanel = new Do3ThinkCameraControlPanel(this);
        layout->addWidget(m_controlPanel);
        
        setCentralWidget(centralWidget);
        setWindowTitle("Do3Think Camera Component Example");
        resize(1200, 800);
    }
    
    void setupComponent() {
        // 創建相機元件
        m_cameraComponent = new Do3ThinkCameraComponent();
        
        // 創建工作線程
        m_cameraThread = new QThread(this);
        
        // 移動元件到線程
        m_cameraComponent->moveToThread(m_cameraThread);
        
        // 載入配置
        QJsonObject config;
        config["autoConnect"] = true;
        config["defaultExposure"] = 10000;
        config["defaultGain"] = 1.0;
        m_cameraComponent->initialize(config);
        
        // 啟動線程
        m_cameraThread->start();
    }
    
    void connectSignals() {
        // 連接元件與控制面板
        m_controlPanel->connectToComponent(m_cameraComponent);
        
        // 連接錯誤處理
        connect(m_cameraComponent, &Do3ThinkCameraComponent::errorOccurred,
                this, [](const QString& error) {
            qCritical() << "Camera Error:" << error;
        });
    }
    
private:
    Do3ThinkCameraComponent* m_cameraComponent = nullptr;
    Do3ThinkCameraControlPanel* m_controlPanel = nullptr;
    QThread* m_cameraThread = nullptr;
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    MainWindow window;
    window.show();
    
    return app.exec();
}

#include "main.moc"
```

## 4. 簡單採集示例

```cpp
// examples/SimpleAcquisition.cpp
class SimpleAcquisitionExample : public QWidget {
    Q_OBJECT
public:
    SimpleAcquisitionExample(QWidget* parent = nullptr) : QWidget(parent) {
        setupUI();
        setupCamera();
    }
    
private:
    void setupUI() {
        auto* layout = new QVBoxLayout(this);
        
        // 圖像顯示
        m_imageLabel = new QLabel(this);
        m_imageLabel->setMinimumSize(640, 480);
        m_imageLabel->setScaledContents(true);
        layout->addWidget(m_imageLabel);
        
        // 控制按鈕
        auto* btnLayout = new QHBoxLayout();
        m_connectBtn = new QPushButton("Connect", this);
        m_startBtn = new QPushButton("Start", this);
        m_stopBtn = new QPushButton("Stop", this);
        
        btnLayout->addWidget(m_connectBtn);
        btnLayout->addWidget(m_startBtn);
        btnLayout->addWidget(m_stopBtn);
        layout->addLayout(btnLayout);
    }
    
    void setupCamera() {
        m_camera = new Do3ThinkCameraComponent(this);
        
        // 連接信號
        connect(m_camera, &Do3ThinkCameraComponent::imageAcquired,
                this, &SimpleAcquisitionExample::updateImage);
        
        connect(m_connectBtn, &QPushButton::clicked, [this]() {
            m_camera->scanDevices();
            auto devices = m_camera->getDeviceList();
            if (!devices.isEmpty()) {
                m_camera->connectCamera(devices.first().serialNumber);
            }
        });
        
        connect(m_startBtn, &QPushButton::clicked, [this]() {
            m_camera->startAcquisition();
        });
        
        connect(m_stopBtn, &QPushButton::clicked, [this]() {
            m_camera->stopAcquisition();
        });
    }
    
private slots:
    void updateImage(const QImage& image, qint64 timestamp) {
        m_imageLabel->setPixmap(QPixmap::fromImage(image));
    }
    
private:
    Do3ThinkCameraComponent* m_camera = nullptr;
    QLabel* m_imageLabel = nullptr;
    QPushButton* m_connectBtn = nullptr;
    QPushButton* m_startBtn = nullptr;
    QPushButton* m_stopBtn = nullptr;
};
```

## 5. 多相機同步示例

```cpp
// examples/MultiCamera.cpp
class MultiCameraExample : public QWidget {
    Q_OBJECT
public:
    MultiCameraExample(QWidget* parent = nullptr) : QWidget(parent) {
        setupUI();
        setupCameras();
    }
    
private:
    void setupCameras() {
        // 掃描所有相機
        Do3ThinkCameraComponent tempCamera;
        tempCamera.scanDevices();
        auto devices = tempCamera.getDeviceList();
        
        // 為每個相機創建元件
        for (const auto& device : devices) {
            auto* camera = new Do3ThinkCameraComponent();
            auto* thread = new QThread(this);
            
            camera->moveToThread(thread);
            thread->start();
            
            // 連接相機
            camera->connectCamera(device.serialNumber);
            
            // 設置硬體觸發同步
            camera->setTriggerMode(Do3ThinkCameraComponent::TriggerMode::Hardware);
            
            m_cameras.append(camera);
            m_threads.append(thread);
            
            // 連接信號
            connect(camera, &Do3ThinkCameraComponent::imageAcquired,
                    this, [this, camera](const QImage& image, qint64 timestamp) {
                onImageReceived(camera, image, timestamp);
            });
        }
    }
    
    void startSynchronizedAcquisition() {
        // 啟動所有相機
        for (auto* camera : m_cameras) {
            camera->startAcquisition();
        }
        
        // 發送同步觸發信號
        if (!m_cameras.isEmpty()) {
            m_cameras.first()->executeSoftwareTrigger();
        }
    }
    
private slots:
    void onImageReceived(Do3ThinkCameraComponent* camera, 
                        const QImage& image, qint64 timestamp) {
        int index = m_cameras.indexOf(camera);
        if (index >= 0 && index < m_imageLabels.size()) {
            m_imageLabels[index]->setPixmap(QPixmap::fromImage(image));
        }
    }
    
private:
    QList<Do3ThinkCameraComponent*> m_cameras;
    QList<QThread*> m_threads;
    QList<QLabel*> m_imageLabels;
};
```

## 6. AOI檢測示例

```cpp
// examples/AOIInspection.cpp
class AOIInspectionExample : public QWidget {
    Q_OBJECT
public:
    AOIInspectionExample(QWidget* parent = nullptr) : QWidget(parent) {
        setupComponents();
        connectPipeline();
    }
    
private:
    void setupComponents() {
        // 相機元件
        m_cameraComponent = new Do3ThinkCameraComponent();
        m_cameraThread = new QThread(this);
        m_cameraComponent->moveToThread(m_cameraThread);
        m_cameraThread->start();
        
        // 影像處理元件
        m_imageProcessor = new ImageProcessingComponent();
        m_processorThread = new QThread(this);
        m_imageProcessor->moveToThread(m_processorThread);
        m_processorThread->start();
        
        // 演算法元件
        m_algorithm = new AlgorithmComponent();
        m_algorithmThread = new QThread(this);
        m_algorithm->moveToThread(m_algorithmThread);
        m_algorithmThread->start();
        
        // 資料儲存元件
        m_dataStorage = new DataStorageComponent();
        m_storageThread = new QThread(this);
        m_dataStorage->moveToThread(m_storageThread);
        m_storageThread->start();
    }
    
    void connectPipeline() {
        // 建立處理管線
        // Camera -> ImageProcessor
        connect(m_cameraComponent, &Do3ThinkCameraComponent::imageAcquired,
                m_imageProcessor, &ImageProcessingComponent::processImage);
        
        // ImageProcessor -> Algorithm
        connect(m_imageProcessor, &ImageProcessingComponent::imageProcessed,
                m_algorithm, &AlgorithmComponent::detectDefects);
        
        // Algorithm -> DataStorage
        connect(m_algorithm, &AlgorithmComponent::defectsDetected,
                m_dataStorage, &DataStorageComponent::saveResults);
        
        // Algorithm -> UI
        connect(m_algorithm, &AlgorithmComponent::defectsDetected,
                this, &AOIInspectionExample::displayResults);
    }
    
private slots:
    void displayResults(const QList<Defect>& defects) {
        // 顯示檢測結果
        m_resultTable->setRowCount(defects.size());
        for (int i = 0; i < defects.size(); ++i) {
            const auto& defect = defects[i];
            m_resultTable->setItem(i, 0, new QTableWidgetItem(defect.type));
            m_resultTable->setItem(i, 1, new QTableWidgetItem(
                QString::number(defect.confidence)));
            m_resultTable->setItem(i, 2, new QTableWidgetItem(
                QString("(%1,%2)").arg(defect.x).arg(defect.y)));
        }
        
        // 更新統計
        updateStatistics(defects);
    }
    
private:
    Do3ThinkCameraComponent* m_cameraComponent = nullptr;
    ImageProcessingComponent* m_imageProcessor = nullptr;
    AlgorithmComponent* m_algorithm = nullptr;
    DataStorageComponent* m_dataStorage = nullptr;
    
    QThread* m_cameraThread = nullptr;
    QThread* m_processorThread = nullptr;
    QThread* m_algorithmThread = nullptr;
    QThread* m_storageThread = nullptr;
    
    QTableWidget* m_resultTable = nullptr;
};
```

## 7. 高速採集示例

```cpp
// examples/HighSpeed.cpp
class HighSpeedAcquisitionExample : public QObject {
    Q_OBJECT
public:
    HighSpeedAcquisitionExample(QObject* parent = nullptr) : QObject(parent) {
        setupHighSpeedMode();
    }
    
private:
    void setupHighSpeedMode() {
        m_camera = new Do3ThinkCameraComponent();
        
        // 配置高速模式
        QJsonObject config;
        config["bufferCount"] = 100;        // 大緩衝區
        config["useCallback"] = true;       // 使用回調模式
        config["zeroCopy"] = true;          // 零拷貝
        config["gpuAcceleration"] = true;   // GPU加速
        
        m_camera->initialize(config);
        
        // 設置最小曝光時間
        m_camera->setExposureTime(100);     // 100μs
        
        // 設置最大幀率
        m_camera->setFrameRate(1000);       // 1000fps
        
        // 設置小ROI以提高速度
        m_camera->setROI(400, 300, 640, 480);
        
        // 使用原始數據回調以避免轉換開銷
        connect(m_camera, &Do3ThinkCameraComponent::rawDataAcquired,
                this, &HighSpeedAcquisitionExample::processRawData,
                Qt::DirectConnection);  // 直接連接避免事件循環延遲
    }
    
private slots:
    void processRawData(const QByteArray& data, const QJsonObject& metadata) {
        // 高速處理
        m_frameCount++;
        
        // 每100幀更新一次統計
        if (m_frameCount % 100 == 0) {
            auto now = QDateTime::currentMSecsSinceEpoch();
            double fps = 100000.0 / (now - m_lastTime);
            m_lastTime = now;
            
            emit fpsUpdated(fps);
        }
        
        // 異步處理避免阻塞採集
        QtConcurrent::run([data, metadata]() {
            // 在線程池中處理數據
            processFrame(data, metadata);
        });
    }
    
signals:
    void fpsUpdated(double fps);
    
private:
    Do3ThinkCameraComponent* m_camera = nullptr;
    qint64 m_frameCount = 0;
    qint64 m_lastTime = 0;
};
```

## 8. 配置文件示例

```json
// config/camera_config.json
{
    "camera": {
        "autoConnect": true,
        "preferredDevice": "12345678",
        "connectionTimeout": 5000,
        "parameters": {
            "exposureTime": 10000,
            "gain": 1.0,
            "gamma": 1.0,
            "triggerMode": "FreeRun",
            "pixelFormat": "Mono8",
            "frameRate": 30
        },
        "roi": {
            "x": 0,
            "y": 0,
            "width": 1920,
            "height": 1080
        },
        "buffer": {
            "count": 10,
            "mode": "circular"
        }
    },
    "processing": {
        "enableGPU": true,
        "threads": 4
    },
    "storage": {
        "path": "./data",
        "format": "HDF5",
        "compression": true
    }
}
```

## 9. 錯誤處理示例

```cpp
class ErrorHandlingExample : public QObject {
    Q_OBJECT
public:
    ErrorHandlingExample() {
        setupErrorHandling();
    }
    
private:
    void setupErrorHandling() {
        m_camera = new Do3ThinkCameraComponent();
        
        // 連接錯誤信號
        connect(m_camera, &Do3ThinkCameraComponent::errorOccurred,
                this, &ErrorHandlingExample::handleError);
        
        // 連接狀態變化信號
        connect(m_camera, &Do3ThinkCameraComponent::statusChanged,
                this, &ErrorHandlingExample::handleStatusChange);
        
        // 設置自動重連
        m_reconnectTimer = new QTimer(this);
        m_reconnectTimer->setInterval(5000);  // 5秒重試
        connect(m_reconnectTimer, &QTimer::timeout,
                this, &ErrorHandlingExample::attemptReconnect);
    }
    
private slots:
    void handleError(const QString& error) {
        qWarning() << "Camera error:" << error;
        
        // 分析錯誤類型
        if (error.contains("disconnected")) {
            // 啟動重連
            m_reconnectTimer->start();
        } else if (error.contains("timeout")) {
            // 重置相機
            m_camera->reset();
        } else if (error.contains("buffer overflow")) {
            // 清空緩衝區
            m_camera->clearBuffers();
        }
        
        // 記錄到日誌
        logError(error);
    }
    
    void attemptReconnect() {
        if (m_camera->reconnect()) {
            m_reconnectTimer->stop();
            qInfo() << "Successfully reconnected";
        } else {
            m_reconnectAttempts++;
            if (m_reconnectAttempts > 10) {
                m_reconnectTimer->stop();
                qCritical() << "Failed to reconnect after 10 attempts";
            }
        }
    }
    
private:
    Do3ThinkCameraComponent* m_camera = nullptr;
    QTimer* m_reconnectTimer = nullptr;
    int m_reconnectAttempts = 0;
};
```

## 10. 部署腳本

### Windows部署
```batch
@echo off
REM deploy_windows.bat

echo Deploying Do3ThinkCamera Example...

REM Create deployment directory
mkdir deploy
cd deploy

REM Copy executable
copy ..\build\Release\Do3ThinkCameraExample.exe .

REM Deploy Qt libraries
windeployqt.exe Do3ThinkCameraExample.exe

REM Copy Do3Think SDK libraries
copy ..\sdk\bin\DVPCamera64.dll .
copy ..\sdk\bin\*.dll .

REM Copy configuration
mkdir config
copy ..\config\*.json config\

REM Create installer
makensis ..\installer\setup.nsi

echo Deployment complete!
```

### Linux部署
```bash
#!/bin/bash
# deploy_linux.sh

echo "Deploying Do3ThinkCamera Example..."

# Create AppImage
mkdir -p AppDir/usr/bin
mkdir -p AppDir/usr/lib

# Copy executable
cp build/Do3ThinkCameraExample AppDir/usr/bin/

# Copy libraries
cp /usr/local/lib/libDVPCamera.so* AppDir/usr/lib/
cp -r /opt/Qt/6.9.0/gcc_64/lib/* AppDir/usr/lib/

# Create desktop file
cat > AppDir/Do3ThinkCamera.desktop <<EOF
[Desktop Entry]
Type=Application
Name=Do3ThinkCamera
Exec=Do3ThinkCameraExample
Icon=camera
Categories=Development;
EOF

# Create AppImage
appimagetool AppDir Do3ThinkCamera.AppImage

echo "Deployment complete!"
```

## 11. 常用代碼片段

### 快速連接相機
```cpp
// 一鍵連接第一個可用相機
auto connectFirstCamera = [](Do3ThinkCameraComponent* camera) {
    camera->scanDevices();
    auto devices = camera->getDeviceList();
    if (!devices.isEmpty()) {
        return camera->connectCamera(devices.first().serialNumber);
    }
    return false;
};
```

### 批量設置參數
```cpp
// 批量設置相機參數
void batchSetParameters(Do3ThinkCameraComponent* camera) {
    QVariantMap params;
    params["exposure"] = 10000;
    params["gain"] = 1.5;
    params["gamma"] = 1.2;
    params["frameRate"] = 60;
    
    camera->setParameters(params);
}
```

### 自動保存圖像
```cpp
// 自動保存圖像到時間戳文件名
connect(camera, &Do3ThinkCameraComponent::imageAcquired,
        [](const QImage& image, qint64 timestamp) {
    QString filename = QString("image_%1.png").arg(timestamp);
    image.save(filename);
});
```

## 總結

這些範例展示了Do3ThinkCameraComponent的各種使用場景，從簡單的圖像採集到複雜的AOI檢測系統。每個範例都遵循ComponentsForest的架構規範，確保元件的可重用性和可維護性。

關鍵要點：
1. 始終使用Signal/Slot進行元件通訊
2. 元件運行在獨立的QThread中
3. Control Panel與Component完全解耦
4. 實施適當的錯誤處理和恢復機制
5. 根據應用需求優化性能設置