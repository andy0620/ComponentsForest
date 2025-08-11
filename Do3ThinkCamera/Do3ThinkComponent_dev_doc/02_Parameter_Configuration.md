# Do3ThinkCamera 參數配置開發文檔

## 1. 概述

Do3ThinkCamera SDK提供了豐富的參數配置接口，涵蓋相機的各種功能設定。本文檔詳細說明了參數的分類、配置方法、持久化管理以及在Qt元件中的整合方式。

## 2. 參數分類與說明

### 2.1 曝光參數
| 參數名稱 | 參數鍵值 | 數據類型 | 說明 |
|---------|---------|---------|------|
| 曝光時間 | V_EXPOSURE_TIME_F | Float | 單位：微秒(μs) |
| 自動曝光 | V_EXPOSURE_AUTO_E | Enum | Off/Once/Continuous |
| 曝光模式 | V_EXPOSURE_MODE_E | Enum | Timed/TriggerWidth |
| 防頻閃 | V_ANTI_FLICK_E | Enum | Disable/50Hz/60Hz |

### 2.2 增益控制
| 參數名稱 | 參數鍵值 | 數據類型 | 說明 |
|---------|---------|---------|------|
| 模擬增益 | V_GAIN_F | Float | 範圍：通常1.0-16.0 |
| 自動增益 | V_GAIN_AUTO_E | Enum | Off/Once/Continuous |
| 黑電平 | V_BLACK_LEVEL_F | Float | 黑電平校正值 |
| RGB增益(R) | V_GAIN_R_F | Float | 紅色通道增益 |
| RGB增益(G) | V_GAIN_G_F | Float | 綠色通道增益 |
| RGB增益(B) | V_GAIN_B_F | Float | 藍色通道增益 |

### 2.3 白平衡
| 參數名稱 | 參數鍵值 | 數據類型 | 說明 |
|---------|---------|---------|------|
| 自動白平衡 | V_AWB_E | Enum | Disable/Once/Continuous |
| 色溫設定 | ColorTemperature | Int32 | 單位：K(2800-8000) |

### 2.4 圖像格式
| 參數名稱 | 參數鍵值 | 數據類型 | 說明 |
|---------|---------|---------|------|
| 像素格式 | V_PIXEL_FORMAT_E | Enum | Mono8/BayerRG8/RGB24等 |
| 目標格式 | V_TARGET_FORMAT_E | Enum | 輸出圖像格式 |
| 分辨率模式 | V_RESOLUTION_MODE_E | Enum | 預設分辨率選項 |

### 2.5 ROI設定
| 參數名稱 | 參數鍵值 | 數據類型 | 說明 |
|---------|---------|---------|------|
| X偏移 | V_OFFSET_X_I | Int32 | ROI起始X座標 |
| Y偏移 | V_OFFSET_Y_I | Int32 | ROI起始Y座標 |
| 寬度 | V_WIDTH_I | Int32 | ROI寬度 |
| 高度 | V_HEIGHT_I | Int32 | ROI高度 |
| 水平翻轉 | V_HFLIP_B | Bool | 水平鏡像 |
| 垂直翻轉 | V_VFLIP_B | Bool | 垂直鏡像 |

### 2.6 幀率控制
| 參數名稱 | 參數鍵值 | 數據類型 | 說明 |
|---------|---------|---------|------|
| 幀率使能 | V_ACQ_FRAME_RATE_ENABLE_B | Bool | 啟用幀率控制 |
| 採集幀率 | V_ACQ_FRAME_RATE_F | Float | 目標幀率(fps) |

### 2.7 觸發模式
| 參數名稱 | 參數鍵值 | 數據類型 | 說明 |
|---------|---------|---------|------|
| 觸發模式 | V_TRIGGER_MODE_B | Bool | 啟用/禁用觸發 |
| 觸發源 | V_TRIGGER_SOURCE_E | Enum | Software/Line0/Line1等 |
| 觸發激活 | V_TRIGGER_ACTIVATION_E | Enum | RisingEdge/FallingEdge |
| 觸發延遲 | V_TRIGGER_DELAY_I | Int32 | 觸發延遲時間(μs) |

### 2.8 圖像增強
| 參數名稱 | 參數鍵值 | 數據類型 | 說明 |
|---------|---------|---------|------|
| Gamma校正 | V_GAMMA_F | Float | 範圍：0.1-4.0 |
| 對比度 | V_CONTRAST_I | Int32 | 範圍：-100~100 |
| 亮度 | V_BRIGHTNESS_I | Int32 | 範圍：-100~100 |
| 飽和度 | V_SATURATION_F | Float | 範圍：0.0-2.0 |
| 銳度 | V_SHARPNESS_I | Int32 | 範圍：0-100 |
| 2D降噪 | V_NOISE_REDUCT_2D_I | Int32 | 空間降噪強度 |
| 3D降噪 | V_NOISE_REDUCT_3D_I | Int32 | 時域降噪強度 |

## 3. 參數讀寫方法

### 3.1 整數型參數
```cpp
// 讀取整數參數
dvpInt32 value;
dvpIntDescr descr;
dvpStatus status = dvpGetInt32Value(handle, "Brightness", &value, &descr);

// 設置整數參數
status = dvpSetInt32Value(handle, "Brightness", 50);
```

### 3.2 浮點型參數
```cpp
// 讀取浮點參數
float exposureTime;
dvpFloatDescr floatDescr;
dvpStatus status = dvpGetFloatValue(handle, V_EXPOSURE_TIME_F, &exposureTime, &floatDescr);

// 設置浮點參數
status = dvpSetFloatValue(handle, V_EXPOSURE_TIME_F, 10000.0f);
```

### 3.3 布爾型參數
```cpp
// 讀取布爾參數
bool triggerMode;
dvpStatus status = dvpGetBoolValue(handle, V_TRIGGER_MODE_B, &triggerMode);

// 設置布爾參數
status = dvpSetBoolValue(handle, V_TRIGGER_MODE_B, true);
```

### 3.4 枚舉型參數
```cpp
// 讀取枚舉參數（通過字符串）
char enumValue[64];
dvpStatus status = dvpGetEnumValueByString(handle, V_AWB_E, enumValue);

// 設置枚舉參數（通過字符串）
status = dvpSetEnumValueByString(handle, V_AWB_E, "Continuous");

// 讀取枚舉參數（通過整數值）
dvpInt32 pixelFormat;
dvpStatus status = dvpGetEnumValue(handle, V_PIXEL_FORMAT_E, &pixelFormat, NULL, NULL);
```

## 4. 參數範圍獲取

### 4.1 整數參數範圍
```cpp
dvpInt32 brightness;
dvpIntDescr intDescr;
dvpGetInt32Value(handle, "Brightness", &brightness, &intDescr);

// 獲取參數範圍
int minValue = intDescr.iMin;      // 最小值
int maxValue = intDescr.iMax;      // 最大值
int stepValue = intDescr.iStep;    // 步進值
int defaultValue = intDescr.iDefault; // 默認值
```

### 4.2 浮點參數範圍
```cpp
float gain;
dvpFloatDescr floatDescr;
dvpGetFloatValue(handle, V_GAIN_F, &gain, &floatDescr);

// 獲取參數範圍
float minGain = floatDescr.fMin;      // 最小值
float maxGain = floatDescr.fMax;      // 最大值  
float stepGain = floatDescr.fStep;    // 步進值
float defaultGain = floatDescr.fDefault; // 默認值
```

### 4.3 枚舉參數選項
```cpp
// 獲取枚舉支持的值
dvpInt32 supportValues[64];
dvpUint32 supportNum;
dvpInt32 currentValue;
dvpGetEnumValue(handle, V_PIXEL_FORMAT_E, &currentValue, supportValues, &supportNum);

// 遍歷所有支持的選項
for(dvpUint32 i = 0; i < supportNum; i++) {
    dvpEnumDescr enumDescr;
    dvpGetEnumDescr(handle, V_PIXEL_FORMAT_E, i, &enumDescr);
    printf("Option %d: %s (Value: %d)\n", i, enumDescr.strText, enumDescr.iValue);
}
```

## 5. 自動模式配置

### 5.1 自動曝光配置
```cpp
// 配置自動曝光
struct AutoExposureConfig {
    dvpAeMode mode;           // AE模式
    dvpAeOperation operation; // AE操作
    float targetBrightness;   // 目標亮度
    float minExposure;        // 最小曝光時間
    float maxExposure;        // 最大曝光時間
    float minGain;           // 最小增益
    float maxGain;           // 最大增益
};

void configureAutoExposure(dvpHandle handle, const AutoExposureConfig& config) {
    // 設置AE模式
    dvpSetAeMode(handle, config.mode);
    
    // 設置AE操作
    dvpSetAeOperation(handle, config.operation);
    
    // 設置目標亮度
    dvpSetAeTarget(handle, config.targetBrightness);
    
    // 設置曝光範圍
    dvpAeConfig aeConfig;
    aeConfig.fExposureMin = config.minExposure;
    aeConfig.fExposureMax = config.maxExposure;
    aeConfig.fGainMin = config.minGain;
    aeConfig.fGainMax = config.maxGain;
    dvpSetAeConfig(handle, aeConfig);
}
```

### 5.2 自動白平衡配置
```cpp
void configureAutoWhiteBalance(dvpHandle handle, bool enable) {
    if (enable) {
        // 啟用連續自動白平衡
        dvpSetEnumValueByString(handle, V_AWB_E, "Continuous");
    } else {
        // 執行一次白平衡
        dvpSetEnumValueByString(handle, V_AWB_E, "Once");
        
        // 重置RGB增益
        dvpSetFloatValue(handle, V_GAIN_R_F, 1.0f);
        dvpSetFloatValue(handle, V_GAIN_G_F, 1.0f);
        dvpSetFloatValue(handle, V_GAIN_B_F, 1.0f);
    }
}
```

## 6. 用戶配置管理

### 6.1 配置保存與載入
```cpp
// 保存當前配置到文件
dvpStatus saveConfiguration(dvpHandle handle, const char* filepath) {
    return dvpSaveConfig(handle, filepath);
}

// 從文件載入配置
dvpStatus loadConfiguration(dvpHandle handle, const char* filepath) {
    return dvpLoadConfig(handle, filepath);
}

// 使用用戶設定集
void useUserSet(dvpHandle handle, dvpUserSet userSet) {
    // 保存到用戶設定
    dvpSaveUserSet(handle, userSet);
    
    // 載入用戶設定
    dvpLoadUserSet(handle, userSet);
}
```

### 6.2 配置集管理
```cpp
class CameraConfigManager {
private:
    dvpHandle m_handle;
    std::string m_configDir;
    
public:
    // 保存配置集
    bool saveConfigSet(const std::string& name) {
        std::string filepath = m_configDir + "/" + name + ".cfg";
        return dvpSaveConfig(m_handle, filepath.c_str()) == DVP_STATUS_OK;
    }
    
    // 載入配置集
    bool loadConfigSet(const std::string& name) {
        std::string filepath = m_configDir + "/" + name + ".cfg";
        return dvpLoadConfig(m_handle, filepath.c_str()) == DVP_STATUS_OK;
    }
    
    // 保存默認配置
    void saveAsDefault() {
        dvpSaveUserSet(m_handle, USER_SET_DEFAULT);
    }
    
    // 恢復出廠設置
    void restoreFactory() {
        dvpLoadUserSet(m_handle, USER_SET_DEFAULT);
    }
};
```

## 7. 參數持久化

### 7.1 自動保存機制
```cpp
class AutoSaveManager {
private:
    dvpHandle m_handle;
    std::thread m_saveThread;
    std::atomic<bool> m_running;
    int m_saveInterval; // 保存間隔(秒)
    
public:
    void startAutoSave(int interval = 60) {
        m_saveInterval = interval;
        m_running = true;
        
        m_saveThread = std::thread([this]() {
            while (m_running) {
                std::this_thread::sleep_for(std::chrono::seconds(m_saveInterval));
                if (m_running) {
                    dvpSaveConfig(m_handle, nullptr); // 保存到默認位置
                }
            }
        });
    }
    
    void stopAutoSave() {
        m_running = false;
        if (m_saveThread.joinable()) {
            m_saveThread.join();
        }
    }
};
```

### 7.2 配置版本管理
```cpp
class ConfigVersionManager {
private:
    struct ConfigVersion {
        std::string name;
        std::string timestamp;
        std::string filepath;
        std::map<std::string, std::string> parameters;
    };
    
    std::vector<ConfigVersion> m_versions;
    
public:
    // 創建配置快照
    void createSnapshot(dvpHandle handle, const std::string& name) {
        ConfigVersion version;
        version.name = name;
        version.timestamp = getCurrentTimestamp();
        version.filepath = generateFilepath(name);
        
        // 保存配置
        dvpSaveConfig(handle, version.filepath.c_str());
        
        // 記錄關鍵參數
        float exposure;
        dvpGetFloatValue(handle, V_EXPOSURE_TIME_F, &exposure, nullptr);
        version.parameters["ExposureTime"] = std::to_string(exposure);
        
        float gain;
        dvpGetFloatValue(handle, V_GAIN_F, &gain, nullptr);
        version.parameters["Gain"] = std::to_string(gain);
        
        m_versions.push_back(version);
    }
    
    // 恢復到指定版本
    bool restoreVersion(dvpHandle handle, size_t index) {
        if (index < m_versions.size()) {
            return dvpLoadConfig(handle, m_versions[index].filepath.c_str()) == DVP_STATUS_OK;
        }
        return false;
    }
};
```

## 8. 實時參數調整

### 8.1 參數調整類
```cpp
class ParameterAdjuster {
private:
    dvpHandle m_handle;
    
public:
    // 平滑調整曝光時間
    void smoothAdjustExposure(float targetExposure, int steps = 10) {
        float currentExposure;
        dvpGetFloatValue(m_handle, V_EXPOSURE_TIME_F, &currentExposure, nullptr);
        
        float step = (targetExposure - currentExposure) / steps;
        
        for (int i = 0; i < steps; i++) {
            currentExposure += step;
            dvpSetFloatValue(m_handle, V_EXPOSURE_TIME_F, currentExposure);
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
    
    // 自適應增益調整
    void adaptiveGainAdjust(float targetBrightness) {
        // 獲取當前圖像亮度
        float currentBrightness = getCurrentImageBrightness();
        
        // 計算增益調整量
        float gainAdjust = targetBrightness / currentBrightness;
        
        float currentGain;
        dvpFloatDescr gainDescr;
        dvpGetFloatValue(m_handle, V_GAIN_F, &currentGain, &gainDescr);
        
        float newGain = currentGain * gainAdjust;
        newGain = std::max(gainDescr.fMin, std::min(gainDescr.fMax, newGain));
        
        dvpSetFloatValue(m_handle, V_GAIN_F, newGain);
    }
    
private:
    float getCurrentImageBrightness() {
        // 實現圖像亮度計算邏輯
        return 128.0f; // 示例值
    }
};
```

### 8.2 參數聯動調整
```cpp
class LinkedParameterAdjuster {
private:
    dvpHandle m_handle;
    
    struct ParameterLink {
        std::string masterParam;
        std::string slaveParam;
        std::function<float(float)> linkFunction;
    };
    
    std::vector<ParameterLink> m_links;
    
public:
    // 添加參數聯動關係
    void addLink(const std::string& master, const std::string& slave, 
                std::function<float(float)> func) {
        m_links.push_back({master, slave, func});
    }
    
    // 調整主參數並更新從參數
    void adjustParameter(const std::string& param, float value) {
        // 設置主參數
        dvpSetFloatValue(m_handle, param.c_str(), value);
        
        // 更新聯動的從參數
        for (const auto& link : m_links) {
            if (link.masterParam == param) {
                float slaveValue = link.linkFunction(value);
                dvpSetFloatValue(m_handle, link.slaveParam.c_str(), slaveValue);
            }
        }
    }
};
```

## 9. 參數依賴關係

### 9.1 參數依賴管理
```cpp
class ParameterDependencyManager {
private:
    struct Dependency {
        std::string parameter;
        std::vector<std::string> dependencies;
        std::function<bool()> condition;
    };
    
    std::vector<Dependency> m_dependencies;
    dvpHandle m_handle;
    
public:
    // 檢查參數是否可用
    bool isParameterAvailable(const std::string& param) {
        for (const auto& dep : m_dependencies) {
            if (dep.parameter == param) {
                return dep.condition();
            }
        }
        return true;
    }
    
    // 初始化依賴關係
    void initializeDependencies() {
        // 示例：RGB增益依賴於彩色相機
        m_dependencies.push_back({
            "RGBGain",
            {"PixelFormat"},
            [this]() {
                char format[64];
                dvpGetEnumValueByString(m_handle, V_PIXEL_FORMAT_E, format);
                return std::string(format).find("Mono") == std::string::npos;
            }
        });
        
        // 示例：觸發延遲依賴於觸發模式
        m_dependencies.push_back({
            "TriggerDelay",
            {"TriggerMode"},
            [this]() {
                bool triggerMode;
                dvpGetBoolValue(m_handle, V_TRIGGER_MODE_B, &triggerMode);
                return triggerMode;
            }
        });
    }
};
```

## 10. Qt元件整合類設計

### 10.1 參數配置元件
```cpp
class CameraParameterWidget : public QWidget {
    Q_OBJECT
    
private:
    dvpHandle m_handle;
    QMap<QString, QWidget*> m_paramWidgets;
    
public:
    CameraParameterWidget(dvpHandle handle, QWidget* parent = nullptr) 
        : QWidget(parent), m_handle(handle) {
        initializeUI();
    }
    
private:
    void initializeUI() {
        QVBoxLayout* layout = new QVBoxLayout(this);
        
        // 創建參數分組
        createExposureGroup(layout);
        createGainGroup(layout);
        createImageEnhancementGroup(layout);
        createTriggerGroup(layout);
    }
    
    void createExposureGroup(QVBoxLayout* parentLayout) {
        QGroupBox* group = new QGroupBox("曝光控制", this);
        QGridLayout* layout = new QGridLayout(group);
        
        // 曝光時間滑塊
        QSlider* exposureSlider = new QSlider(Qt::Horizontal);
        dvpFloatDescr descr;
        float currentValue;
        dvpGetFloatValue(m_handle, V_EXPOSURE_TIME_F, &currentValue, &descr);
        
        exposureSlider->setRange(descr.fMin, descr.fMax);
        exposureSlider->setValue(currentValue);
        
        QSpinBox* exposureSpinBox = new QSpinBox();
        exposureSpinBox->setRange(descr.fMin, descr.fMax);
        exposureSpinBox->setValue(currentValue);
        
        // 連接信號槽
        connect(exposureSlider, &QSlider::valueChanged, [this, exposureSpinBox](int value) {
            exposureSpinBox->setValue(value);
            dvpSetFloatValue(m_handle, V_EXPOSURE_TIME_F, value);
            emit parameterChanged("ExposureTime", value);
        });
        
        connect(exposureSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), 
                exposureSlider, &QSlider::setValue);
        
        layout->addWidget(new QLabel("曝光時間(μs):"), 0, 0);
        layout->addWidget(exposureSlider, 0, 1);
        layout->addWidget(exposureSpinBox, 0, 2);
        
        // 自動曝光
        QCheckBox* autoExposure = new QCheckBox("自動曝光");
        connect(autoExposure, &QCheckBox::toggled, [this](bool checked) {
            dvpSetEnumValueByString(m_handle, V_EXPOSURE_AUTO_E, 
                                  checked ? "Continuous" : "Off");
        });
        layout->addWidget(autoExposure, 1, 0, 1, 3);
        
        parentLayout->addWidget(group);
        
        // 保存控件引用
        m_paramWidgets["ExposureSlider"] = exposureSlider;
        m_paramWidgets["ExposureSpinBox"] = exposureSpinBox;
        m_paramWidgets["AutoExposure"] = autoExposure;
    }
    
    void createGainGroup(QVBoxLayout* parentLayout) {
        QGroupBox* group = new QGroupBox("增益控制", this);
        QGridLayout* layout = new QGridLayout(group);
        
        // 類似創建增益控制UI
        // ...
        
        parentLayout->addWidget(group);
    }
    
    void createImageEnhancementGroup(QVBoxLayout* parentLayout) {
        QGroupBox* group = new QGroupBox("圖像增強", this);
        QGridLayout* layout = new QGridLayout(group);
        
        // 創建Gamma、對比度、亮度、飽和度等控件
        // ...
        
        parentLayout->addWidget(group);
    }
    
    void createTriggerGroup(QVBoxLayout* parentLayout) {
        QGroupBox* group = new QGroupBox("觸發設置", this);
        QGridLayout* layout = new QGridLayout(group);
        
        // 創建觸發模式、觸發源等控件
        // ...
        
        parentLayout->addWidget(group);
    }
    
public slots:
    void updateParameters() {
        // 更新所有參數顯示
        for (auto it = m_paramWidgets.begin(); it != m_paramWidgets.end(); ++it) {
            updateWidget(it.key(), it.value());
        }
    }
    
    void saveConfiguration() {
        QString filename = QFileDialog::getSaveFileName(this, 
            "保存配置", "", "配置文件 (*.cfg)");
        if (!filename.isEmpty()) {
            dvpSaveConfig(m_handle, filename.toStdString().c_str());
        }
    }
    
    void loadConfiguration() {
        QString filename = QFileDialog::getOpenFileName(this, 
            "載入配置", "", "配置文件 (*.cfg)");
        if (!filename.isEmpty()) {
            dvpLoadConfig(m_handle, filename.toStdString().c_str());
            updateParameters();
        }
    }
    
signals:
    void parameterChanged(const QString& name, const QVariant& value);
    
private:
    void updateWidget(const QString& name, QWidget* widget) {
        // 根據控件類型更新顯示值
        if (QSlider* slider = qobject_cast<QSlider*>(widget)) {
            // 更新滑塊值
        } else if (QSpinBox* spinBox = qobject_cast<QSpinBox*>(widget)) {
            // 更新數值框值
        } else if (QCheckBox* checkBox = qobject_cast<QCheckBox*>(widget)) {
            // 更新複選框狀態
        }
    }
};
```

### 10.2 參數配置管理器
```cpp
class CameraConfigurationManager : public QObject {
    Q_OBJECT
    
private:
    dvpHandle m_handle;
    QTimer* m_autoSaveTimer;
    QString m_configPath;
    
    // 參數快照
    struct ParameterSnapshot {
        QDateTime timestamp;
        QMap<QString, QVariant> parameters;
    };
    
    QList<ParameterSnapshot> m_snapshots;
    
public:
    CameraConfigurationManager(dvpHandle handle, QObject* parent = nullptr)
        : QObject(parent), m_handle(handle) {
        m_configPath = QStandardPaths::writableLocation(
            QStandardPaths::AppConfigLocation) + "/camera_configs";
        QDir().mkpath(m_configPath);
        
        // 設置自動保存
        m_autoSaveTimer = new QTimer(this);
        connect(m_autoSaveTimer, &QTimer::timeout, this, &CameraConfigurationManager::autoSave);
    }
    
    // 獲取所有參數
    QMap<QString, QVariant> getAllParameters() {
        QMap<QString, QVariant> params;
        
        // 曝光參數
        float exposure;
        dvpGetFloatValue(m_handle, V_EXPOSURE_TIME_F, &exposure, nullptr);
        params["ExposureTime"] = exposure;
        
        // 增益參數
        float gain;
        dvpGetFloatValue(m_handle, V_GAIN_F, &gain, nullptr);
        params["Gain"] = gain;
        
        // 其他參數...
        
        return params;
    }
    
    // 設置所有參數
    void setAllParameters(const QMap<QString, QVariant>& params) {
        for (auto it = params.begin(); it != params.end(); ++it) {
            setParameter(it.key(), it.value());
        }
    }
    
    // 設置單個參數
    void setParameter(const QString& name, const QVariant& value) {
        if (name == "ExposureTime") {
            dvpSetFloatValue(m_handle, V_EXPOSURE_TIME_F, value.toFloat());
        } else if (name == "Gain") {
            dvpSetFloatValue(m_handle, V_GAIN_F, value.toFloat());
        }
        // 處理其他參數...
    }
    
    // 創建參數快照
    void createSnapshot(const QString& name = "") {
        ParameterSnapshot snapshot;
        snapshot.timestamp = QDateTime::currentDateTime();
        snapshot.parameters = getAllParameters();
        
        m_snapshots.append(snapshot);
        
        // 保存到文件
        if (!name.isEmpty()) {
            QString filepath = m_configPath + "/" + name + ".json";
            saveSnapshotToFile(snapshot, filepath);
        }
        
        emit snapshotCreated(snapshot.timestamp);
    }
    
    // 恢復快照
    void restoreSnapshot(int index) {
        if (index >= 0 && index < m_snapshots.size()) {
            setAllParameters(m_snapshots[index].parameters);
            emit snapshotRestored(m_snapshots[index].timestamp);
        }
    }
    
    // 啟用自動保存
    void enableAutoSave(int intervalSeconds = 60) {
        m_autoSaveTimer->start(intervalSeconds * 1000);
    }
    
    // 禁用自動保存
    void disableAutoSave() {
        m_autoSaveTimer->stop();
    }
    
private slots:
    void autoSave() {
        QString filepath = m_configPath + "/autosave.cfg";
        dvpSaveConfig(m_handle, filepath.toStdString().c_str());
    }
    
signals:
    void snapshotCreated(const QDateTime& timestamp);
    void snapshotRestored(const QDateTime& timestamp);
    void configurationSaved(const QString& filepath);
    void configurationLoaded(const QString& filepath);
    
private:
    void saveSnapshotToFile(const ParameterSnapshot& snapshot, const QString& filepath) {
        QJsonObject json;
        json["timestamp"] = snapshot.timestamp.toString(Qt::ISODate);
        
        QJsonObject params;
        for (auto it = snapshot.parameters.begin(); it != snapshot.parameters.end(); ++it) {
            params[it.key()] = QJsonValue::fromVariant(it.value());
        }
        json["parameters"] = params;
        
        QJsonDocument doc(json);
        QFile file(filepath);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(doc.toJson());
            file.close();
        }
    }
};
```

## 11. 工業AOI典型配置

### 11.1 高速檢測配置
```cpp
class HighSpeedInspectionConfig {
public:
    static void apply(dvpHandle handle) {
        // 設置高幀率模式
        dvpSetBoolValue(handle, V_ACQ_FRAME_RATE_ENABLE_B, true);
        dvpSetFloatValue(handle, V_ACQ_FRAME_RATE_F, 100.0f); // 100 fps
        
        // 短曝光時間
        dvpSetFloatValue(handle, V_EXPOSURE_TIME_F, 1000.0f); // 1ms
        
        // 固定增益
        dvpSetEnumValueByString(handle, V_GAIN_AUTO_E, "Off");
        dvpSetFloatValue(handle, V_GAIN_F, 1.0f);
        
        // 硬件觸發模式
        dvpSetBoolValue(handle, V_TRIGGER_MODE_B, true);
        dvpSetEnumValueByString(handle, V_TRIGGER_SOURCE_E, "Line0");
        
        // 關閉圖像增強以提高速度
        dvpSetGammaState(handle, false);
        dvpSetContrastState(handle, false);
        dvpSetSaturationState(handle, false);
        
        // 設置最小ROI
        dvpRegion roi = {0, 0, 640, 480};
        dvpSetRoi(handle, roi);
    }
};
```

### 11.2 高精度測量配置
```cpp
class HighPrecisionMeasurementConfig {
public:
    static void apply(dvpHandle handle) {
        // 低幀率高質量模式
        dvpSetBoolValue(handle, V_ACQ_FRAME_RATE_ENABLE_B, true);
        dvpSetFloatValue(handle, V_ACQ_FRAME_RATE_F, 10.0f); // 10 fps
        
        // 較長曝光時間
        dvpSetFloatValue(handle, V_EXPOSURE_TIME_F, 10000.0f); // 10ms
        
        // 啟用圖像增強
        dvpSetGammaState(handle, true);
        dvpSetGamma(handle, 0.75f);
        
        // 啟用降噪
        dvpSetNoiseReduct2dState(handle, true);
        dvpSetNoiseReduct2d(handle, 3);
        
        // 最高分辨率
        dvpSetResolutionModeSel(handle, 0); // 全分辨率
        
        // 禁用ROI
        dvpSetRoiState(handle, false);
        
        // 高位深度
        dvpSetTargetFormatSel(handle, 2); // 16-bit 輸出
    }
};
```

### 11.3 色彩檢測配置
```cpp
class ColorInspectionConfig {
public:
    static void apply(dvpHandle handle) {
        // 設置彩色輸出格式
        dvpSetEnumValueByString(handle, V_TARGET_FORMAT_E, "RGB24");
        
        // 啟用自動白平衡
        dvpSetEnumValueByString(handle, V_AWB_E, "Continuous");
        
        // 色彩增強
        dvpSetSaturationState(handle, true);
        dvpSetSaturation(handle, 1.2f);
        
        // 適中的曝光設置
        dvpSetFloatValue(handle, V_EXPOSURE_TIME_F, 5000.0f);
        dvpSetFloatValue(handle, V_GAIN_F, 1.5f);
        
        // 啟用Gamma校正
        dvpSetGammaState(handle, true);
        dvpSetGamma(handle, 0.65f);
        
        // 色溫設置（日光）
        dvpSetColorTemperatureState(handle, true);
        dvpSetColorTemperature(handle, 5500);
    }
};
```

### 11.4 低光環境配置
```cpp
class LowLightConfig {
public:
    static void apply(dvpHandle handle) {
        // 長曝光時間
        dvpFloatDescr exposureDescr;
        float exposure;
        dvpGetFloatValue(handle, V_EXPOSURE_TIME_F, &exposure, &exposureDescr);
        dvpSetFloatValue(handle, V_EXPOSURE_TIME_F, exposureDescr.fMax * 0.8f);
        
        // 高增益
        dvpFloatDescr gainDescr;
        float gain;
        dvpGetFloatValue(handle, V_GAIN_F, &gain, &gainDescr);
        dvpSetFloatValue(handle, V_GAIN_F, gainDescr.fMax * 0.7f);
        
        // 啟用3D降噪
        dvpSetNoiseReduct3dState(handle, true);
        dvpSetNoiseReduct3d(handle, 5);
        
        // 提高Gamma值增強暗部細節
        dvpSetGammaState(handle, true);
        dvpSetGamma(handle, 0.45f);
        
        // 提高亮度
        dvpSetBrightnessState(handle, true);
        dvpSetBrightness(handle, 20);
        
        // 低幀率以獲得更好的信噪比
        dvpSetBoolValue(handle, V_ACQ_FRAME_RATE_ENABLE_B, true);
        dvpSetFloatValue(handle, V_ACQ_FRAME_RATE_F, 5.0f);
    }
};
```

## 12. 完整示例程式

### 12.1 參數配置應用示例
```cpp
#include <iostream>
#include <DVPCamera.h>
#include <dvpParam.h>

class CameraParameterConfigurator {
private:
    dvpHandle m_handle;
    
public:
    CameraParameterConfigurator() : m_handle(0) {}
    
    bool initialize() {
        // 枚舉並打開第一個相機
        dvpUint32 count;
        if (dvpRefresh(&count) != DVP_STATUS_OK || count == 0) {
            std::cerr << "No camera found!" << std::endl;
            return false;
        }
        
        if (dvpOpen(0, OPEN_NORMAL, &m_handle) != DVP_STATUS_OK) {
            std::cerr << "Failed to open camera!" << std::endl;
            return false;
        }
        
        return true;
    }
    
    void configureForApplication(const std::string& appType) {
        if (appType == "HighSpeed") {
            HighSpeedInspectionConfig::apply(m_handle);
        } else if (appType == "Precision") {
            HighPrecisionMeasurementConfig::apply(m_handle);
        } else if (appType == "Color") {
            ColorInspectionConfig::apply(m_handle);
        } else if (appType == "LowLight") {
            LowLightConfig::apply(m_handle);
        }
    }
    
    void printCurrentConfiguration() {
        std::cout << "=== Current Camera Configuration ===" << std::endl;
        
        // 曝光時間
        float exposure;
        dvpGetFloatValue(m_handle, V_EXPOSURE_TIME_F, &exposure, nullptr);
        std::cout << "Exposure Time: " << exposure << " μs" << std::endl;
        
        // 增益
        float gain;
        dvpGetFloatValue(m_handle, V_GAIN_F, &gain, nullptr);
        std::cout << "Gain: " << gain << std::endl;
        
        // 幀率
        float frameRate;
        dvpGetFloatValue(m_handle, V_ACQ_FRAME_RATE_F, &frameRate, nullptr);
        std::cout << "Frame Rate: " << frameRate << " fps" << std::endl;
        
        // 觸發模式
        bool triggerMode;
        dvpGetBoolValue(m_handle, V_TRIGGER_MODE_B, &triggerMode);
        std::cout << "Trigger Mode: " << (triggerMode ? "Enabled" : "Disabled") << std::endl;
        
        // ROI
        dvpRegion roi;
        if (dvpGetRoi(m_handle, &roi) == DVP_STATUS_OK) {
            std::cout << "ROI: [" << roi.X << ", " << roi.Y 
                     << ", " << roi.W << ", " << roi.H << "]" << std::endl;
        }
    }
    
    void interactiveConfiguration() {
        while (true) {
            std::cout << "\n=== Parameter Configuration Menu ===" << std::endl;
            std::cout << "1. Set Exposure Time" << std::endl;
            std::cout << "2. Set Gain" << std::endl;
            std::cout << "3. Enable/Disable Auto Exposure" << std::endl;
            std::cout << "4. Set Frame Rate" << std::endl;
            std::cout << "5. Configure Trigger Mode" << std::endl;
            std::cout << "6. Save Configuration" << std::endl;
            std::cout << "7. Load Configuration" << std::endl;
            std::cout << "8. Print Current Settings" << std::endl;
            std::cout << "0. Exit" << std::endl;
            std::cout << "Choice: ";
            
            int choice;
            std::cin >> choice;
            
            switch (choice) {
                case 1: {
                    float exposure;
                    std::cout << "Enter exposure time (μs): ";
                    std::cin >> exposure;
                    dvpSetFloatValue(m_handle, V_EXPOSURE_TIME_F, exposure);
                    break;
                }
                case 2: {
                    float gain;
                    std::cout << "Enter gain: ";
                    std::cin >> gain;
                    dvpSetFloatValue(m_handle, V_GAIN_F, gain);
                    break;
                }
                case 3: {
                    std::cout << "Enable auto exposure? (1/0): ";
                    int enable;
                    std::cin >> enable;
                    dvpSetEnumValueByString(m_handle, V_EXPOSURE_AUTO_E, 
                                          enable ? "Continuous" : "Off");
                    break;
                }
                case 4: {
                    float fps;
                    std::cout << "Enter frame rate (fps): ";
                    std::cin >> fps;
                    dvpSetBoolValue(m_handle, V_ACQ_FRAME_RATE_ENABLE_B, true);
                    dvpSetFloatValue(m_handle, V_ACQ_FRAME_RATE_F, fps);
                    break;
                }
                case 5: {
                    std::cout << "Enable trigger mode? (1/0): ";
                    int enable;
                    std::cin >> enable;
                    dvpSetBoolValue(m_handle, V_TRIGGER_MODE_B, enable);
                    if (enable) {
                        std::cout << "Select trigger source (0=Software, 1=Line0): ";
                        int source;
                        std::cin >> source;
                        dvpSetEnumValueByString(m_handle, V_TRIGGER_SOURCE_E, 
                                              source ? "Line0" : "Software");
                    }
                    break;
                }
                case 6: {
                    std::string filename;
                    std::cout << "Enter configuration filename: ";
                    std::cin >> filename;
                    if (dvpSaveConfig(m_handle, filename.c_str()) == DVP_STATUS_OK) {
                        std::cout << "Configuration saved successfully!" << std::endl;
                    }
                    break;
                }
                case 7: {
                    std::string filename;
                    std::cout << "Enter configuration filename: ";
                    std::cin >> filename;
                    if (dvpLoadConfig(m_handle, filename.c_str()) == DVP_STATUS_OK) {
                        std::cout << "Configuration loaded successfully!" << std::endl;
                    }
                    break;
                }
                case 8:
                    printCurrentConfiguration();
                    break;
                case 0:
                    return;
            }
        }
    }
    
    ~CameraParameterConfigurator() {
        if (m_handle) {
            dvpClose(m_handle);
        }
    }
};

int main() {
    CameraParameterConfigurator configurator;
    
    if (!configurator.initialize()) {
        return -1;
    }
    
    // 選擇應用場景
    std::cout << "Select application type:" << std::endl;
    std::cout << "1. High Speed Inspection" << std::endl;
    std::cout << "2. Precision Measurement" << std::endl;
    std::cout << "3. Color Inspection" << std::endl;
    std::cout << "4. Low Light Environment" << std::endl;
    std::cout << "5. Manual Configuration" << std::endl;
    
    int choice;
    std::cin >> choice;
    
    switch (choice) {
        case 1:
            configurator.configureForApplication("HighSpeed");
            break;
        case 2:
            configurator.configureForApplication("Precision");
            break;
        case 3:
            configurator.configureForApplication("Color");
            break;
        case 4:
            configurator.configureForApplication("LowLight");
            break;
        case 5:
            configurator.interactiveConfiguration();
            break;
    }
    
    configurator.printCurrentConfiguration();
    
    return 0;
}
```

## 13. 注意事項

### 13.1 參數設置最佳實踐
1. **參數驗證**：設置參數前應先獲取參數描述信息，確保設置值在有效範圍內
2. **順序依賴**：某些參數設置有先後順序要求，如先設置像素格式再設置ROI
3. **性能影響**：頻繁的參數調整可能影響圖像採集性能，應在停止採集時批量設置
4. **保存時機**：重要參數變更後應及時保存配置，避免意外斷電丟失

### 13.2 常見問題處理
1. **參數衝突**：某些參數組合可能衝突，SDK會自動調整或返回錯誤
2. **精度限制**：實際設置值可能因硬件精度限制與請求值略有差異
3. **響應延遲**：某些參數變更需要時間生效，特別是涉及硬件調整的參數
4. **權限問題**：某些參數可能需要特定的相機開啟模式才能修改

## 14. 總結

本文檔詳細介紹了Do3ThinkCamera SDK的參數配置系統，包括：
- 完整的參數分類與說明
- 各種數據類型的參數讀寫方法
- 參數範圍獲取和驗證
- 自動模式配置
- 用戶配置管理和持久化
- 實時參數調整策略
- 參數依賴關係管理
- Qt元件整合方案
- 工業AOI典型配置方案

通過本文檔提供的類設計和示例代碼，開發者可以快速實現專業的相機參數配置功能，滿足各種機器視覺應用需求。