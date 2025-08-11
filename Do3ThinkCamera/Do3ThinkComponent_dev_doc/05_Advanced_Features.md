# Do3Think Camera SDK - 進階功能開發指南

## 目錄
- [1. 觸發模式應用](#1-觸發模式應用)
- [2. 多相機同步](#2-多相機同步)
- [3. 硬體ISP功能](#3-硬體isp功能)
- [4. 高級圖像處理](#4-高級圖像處理)
- [5. 線掃相機支援](#5-線掃相機支援)
- [6. 事件與回調系統](#6-事件與回調系統)
- [7. 配置管理](#7-配置管理)
- [8. 性能優化](#8-性能優化)

---

## 1. 觸發模式應用

### 1.1 觸發源設置

Do3Think SDK支援多種觸發源，可根據工業應用需求靈活配置：

```cpp
// 觸發源類型枚舉
enum dvpTriggerSource {
    TRIGGER_SOURCE_SOFTWARE = 0,  // 軟體觸發
    TRIGGER_SOURCE_LINE0 = 1,     // Line0硬體觸發
    TRIGGER_SOURCE_LINE1 = 2,     // Line1硬體觸發
    TRIGGER_SOURCE_LINE2 = 3,     // Line2硬體觸發
    TRIGGER_SOURCE_LINE3 = 4      // Line3硬體觸發
};

// 設置觸發源
dvpStatus status = dvpSetTriggerSource(handle, TRIGGER_SOURCE_LINE0);
```

### 1.2 硬體觸發配置

硬體觸發適用於需要精確同步的工業應用：

```cpp
class HardwareTriggerManager {
private:
    dvpHandle m_handle;
    dvpTriggerInputType m_triggerType;
    int m_triggerDelay;
    
public:
    bool ConfigureHardwareTrigger() {
        dvpStatus status;
        
        // 啟用觸發模式
        status = dvpSetBoolValue(m_handle, V_TRIGGER_MODE_B, true);
        if (status != DVP_STATUS_OK) return false;
        
        // 設置觸發源為Line0
        status = dvpSetTriggerSource(m_handle, TRIGGER_SOURCE_LINE0);
        if (status != DVP_STATUS_OK) return false;
        
        // 配置觸發類型（上升沿觸發）
        status = dvpSetEnumValue(m_handle, V_LINE_MODE_E, TRIGGER_POS_EDGE);
        if (status != DVP_STATUS_OK) return false;
        
        // 設置觸發延遲（微秒）
        status = dvpSetInt32Value(m_handle, V_TRIGGER_DELAY_I, m_triggerDelay);
        if (status != DVP_STATUS_OK) return false;
        
        // 配置Line反相器（可選）
        status = dvpSetBoolValue(m_handle, V_LINE_INVERTER_B, false);
        
        return status == DVP_STATUS_OK;
    }
    
    // 配置觸發輸出（Strobe）
    bool ConfigureStrobeOutput() {
        dvpStatus status;
        
        // 設置Strobe輸出模式
        status = dvpSetEnumValue(m_handle, V_LINE_SOURCE_E, STROBE_OUT_HIGH);
        if (status != DVP_STATUS_OK) return false;
        
        // 設置Strobe驅動方式
        status = dvpSetEnumValue(m_handle, V_STROBE_DRIVER_E, FRAME_DURATION);
        
        return status == DVP_STATUS_OK;
    }
};
```

### 1.3 軟體觸發實現

軟體觸發適用於需要程式控制的場景：

```cpp
class SoftwareTriggerController {
private:
    dvpHandle m_handle;
    bool m_triggerEnabled;
    
public:
    bool EnableSoftwareTrigger() {
        dvpStatus status;
        
        // 設置軟體觸發源
        status = dvpSetTriggerSource(m_handle, TRIGGER_SOURCE_SOFTWARE);
        if (status != DVP_STATUS_OK) return false;
        
        // 啟用觸發模式
        status = dvpSetBoolValue(m_handle, V_TRIGGER_MODE_B, true);
        m_triggerEnabled = (status == DVP_STATUS_OK);
        
        return m_triggerEnabled;
    }
    
    // 發送軟體觸發信號
    bool TriggerCapture() {
        if (!m_triggerEnabled) return false;
        
        dvpStatus status = dvpTriggerFire(m_handle);
        return status == DVP_STATUS_OK;
    }
    
    // 循環觸發設置
    bool SetupLoopTrigger(double periodUs) {
        dvpStatus status;
        
        // 設置循環觸發週期
        status = dvpSetSoftTriggerLoop(m_handle, periodUs);
        if (status != DVP_STATUS_OK) return false;
        
        // 啟用循環觸發
        status = dvpSetSoftTriggerLoopState(m_handle, true);
        
        return status == DVP_STATUS_OK;
    }
};
```

### 1.4 Qt整合觸發控制

```cpp
class QtTriggerWidget : public QWidget {
    Q_OBJECT
    
private:
    dvpHandle m_handle;
    QComboBox* m_triggerSourceCombo;
    QSpinBox* m_triggerDelaySpinBox;
    QPushButton* m_softTriggerBtn;
    
private slots:
    void onTriggerSourceChanged(int index) {
        dvpTriggerSource sources[] = {
            TRIGGER_SOURCE_SOFTWARE,
            TRIGGER_SOURCE_LINE0,
            TRIGGER_SOURCE_LINE1
        };
        
        dvpSetTriggerSource(m_handle, sources[index]);
        
        // 軟體觸發時啟用觸發按鈕
        m_softTriggerBtn->setEnabled(index == 0);
    }
    
    void onSoftwareTrigger() {
        dvpTriggerFire(m_handle);
    }
    
    void onTriggerDelayChanged(int value) {
        dvpSetInt32Value(m_handle, V_TRIGGER_DELAY_I, value);
    }
};
```

---

## 2. 多相機同步

### 2.1 同步採集策略

實現多相機精確同步採集的架構：

```cpp
class MultiCameraSyncManager {
private:
    struct CameraUnit {
        dvpHandle handle;
        std::string serialNumber;
        bool isMaster;
        std::thread captureThread;
        std::atomic<bool> isRunning;
    };
    
    std::vector<CameraUnit> m_cameras;
    std::mutex m_syncMutex;
    std::condition_variable m_syncCV;
    std::atomic<uint64_t> m_syncTimestamp;
    
public:
    // 初始化多相機系統
    bool InitializeCameras(const std::vector<std::string>& serialNumbers) {
        dvpUint32 count;
        dvpRefresh(&count);
        
        for (const auto& sn : serialNumbers) {
            CameraUnit unit;
            unit.serialNumber = sn;
            unit.isMaster = (m_cameras.empty()); // 第一個相機作為主相機
            
            // 打開相機
            dvpStatus status = dvpOpenByName(sn.c_str(), 
                                            OPEN_NORMAL, 
                                            &unit.handle);
            if (status == DVP_STATUS_OK) {
                ConfigureCamera(unit);
                m_cameras.push_back(unit);
            }
        }
        
        return !m_cameras.empty();
    }
    
private:
    void ConfigureCamera(CameraUnit& unit) {
        if (unit.isMaster) {
            // 主相機配置為觸發源
            ConfigureMasterCamera(unit.handle);
        } else {
            // 從相機配置為硬體觸發
            ConfigureSlaveCamera(unit.handle);
        }
    }
    
    void ConfigureMasterCamera(dvpHandle handle) {
        // 主相機使用內部觸發或軟體觸發
        dvpSetTriggerSource(handle, TRIGGER_SOURCE_SOFTWARE);
        dvpSetBoolValue(handle, V_TRIGGER_MODE_B, false);
        
        // 配置觸發輸出給從相機
        dvpSetEnumValue(handle, V_LINE_MODE_E, STROBE_OUT_HIGH);
        dvpSetEnumValue(handle, V_STROBE_DRIVER_E, FRAME_DURATION);
    }
    
    void ConfigureSlaveCamera(dvpHandle handle) {
        // 從相機使用硬體觸發
        dvpSetTriggerSource(handle, TRIGGER_SOURCE_LINE0);
        dvpSetBoolValue(handle, V_TRIGGER_MODE_B, true);
        dvpSetEnumValue(handle, V_LINE_MODE_E, TRIGGER_POS_EDGE);
    }
};
```

### 2.2 時間戳同步機制

```cpp
class TimestampSynchronizer {
private:
    struct FrameInfo {
        dvpHandle cameraHandle;
        uint64_t frameId;
        uint64_t timestamp;
        cv::Mat image;
    };
    
    std::map<dvpHandle, std::queue<FrameInfo>> m_frameQueues;
    std::mutex m_queueMutex;
    uint64_t m_syncTolerance = 1000; // 1ms容差
    
public:
    // 添加採集的幀
    void AddFrame(dvpHandle handle, const dvpFrame* frame, void* buffer) {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        
        FrameInfo info;
        info.cameraHandle = handle;
        info.frameId = frame->uFrameID;
        info.timestamp = frame->uTimestamp;
        
        // 複製圖像數據
        CopyFrameData(frame, buffer, info.image);
        
        m_frameQueues[handle].push(info);
        
        // 嘗試同步
        TrySync();
    }
    
private:
    void TrySync() {
        // 檢查所有相機是否都有幀
        for (auto& pair : m_frameQueues) {
            if (pair.second.empty()) return;
        }
        
        // 獲取最早的時間戳
        uint64_t minTimestamp = UINT64_MAX;
        for (auto& pair : m_frameQueues) {
            minTimestamp = std::min(minTimestamp, 
                                   pair.second.front().timestamp);
        }
        
        // 檢查時間戳是否在容差範圍內
        bool canSync = true;
        for (auto& pair : m_frameQueues) {
            uint64_t diff = pair.second.front().timestamp - minTimestamp;
            if (diff > m_syncTolerance) {
                canSync = false;
                break;
            }
        }
        
        if (canSync) {
            ProcessSyncedFrames();
        }
    }
    
    void ProcessSyncedFrames() {
        std::vector<FrameInfo> syncedFrames;
        
        for (auto& pair : m_frameQueues) {
            syncedFrames.push_back(pair.second.front());
            pair.second.pop();
        }
        
        // 處理同步的幀
        OnFramesSynchronized(syncedFrames);
    }
    
    virtual void OnFramesSynchronized(const std::vector<FrameInfo>& frames) {
        // 由派生類實現具體處理邏輯
    }
};
```

### 2.3 幀同步回調實現

```cpp
class SyncCaptureCallback {
private:
    std::atomic<int> m_cameraCount;
    std::atomic<int> m_readyCount;
    std::condition_variable m_syncCondition;
    std::mutex m_syncMutex;
    
public:
    static int StreamCallback(dvpHandle handle, 
                            dvpStreamEvent event, 
                            void* pContext, 
                            dvpFrame* pFrame, 
                            void* pBuffer) {
        auto* pThis = static_cast<SyncCaptureCallback*>(pContext);
        
        if (event == STREAM_EVENT_PROCESSED) {
            pThis->OnFrameProcessed(handle, pFrame, pBuffer);
        }
        
        return 0;
    }
    
private:
    void OnFrameProcessed(dvpHandle handle, 
                         dvpFrame* frame, 
                         void* buffer) {
        // 增加就緒計數
        int ready = ++m_readyCount;
        
        if (ready == m_cameraCount) {
            // 所有相機都準備好了
            std::lock_guard<std::mutex> lock(m_syncMutex);
            m_syncCondition.notify_all();
            m_readyCount = 0;
        } else {
            // 等待其他相機
            std::unique_lock<std::mutex> lock(m_syncMutex);
            m_syncCondition.wait_for(lock, 
                                    std::chrono::milliseconds(10));
        }
        
        // 同步處理幀數據
        ProcessSynchronizedFrame(handle, frame, buffer);
    }
};
```

---

## 3. 硬體ISP功能

### 3.1 硬體ISP控制

```cpp
class HardwareISPController {
private:
    dvpHandle m_handle;
    bool m_hwIspSupported;
    
public:
    bool Initialize(dvpHandle handle) {
        m_handle = handle;
        
        // 檢查硬體ISP支援
        dvpFunctionInfo funcInfo;
        dvpStatus status = dvpGetFunctionInfo(m_handle, 
                                             FUNC_HARDWARE_ISP, 
                                             &funcInfo);
        m_hwIspSupported = (status == DVP_STATUS_OK && funcInfo.bIsSupported);
        
        return m_hwIspSupported;
    }
    
    // 啟用硬體ISP
    bool EnableHardwareISP(bool enable) {
        if (!m_hwIspSupported) return false;
        
        dvpStatus status = dvpSetHardwareIspState(m_handle, enable);
        return status == DVP_STATUS_OK;
    }
    
    // 配置硬體ISP參數
    bool ConfigureISPFeatures() {
        dvpStatus status;
        
        // 啟用硬體ISP
        status = dvpSetHardwareIspState(m_handle, true);
        if (status != DVP_STATUS_OK) return false;
        
        // 自動彩點消除
        status = dvpSetAutoDefectFixState(m_handle, true);
        
        // 設置降噪等級
        status = dvpSetNoiseReduct2d(m_handle, 50);
        status = dvpSetNoiseReduct2dState(m_handle, true);
        
        // 設置銳化
        status = dvpSetSharpness(m_handle, 70);
        status = dvpSetSharpnessState(m_handle, true);
        
        return true;
    }
};
```

### 3.2 彩點消除與降噪

```cpp
class DefectPixelCorrection {
private:
    dvpHandle m_handle;
    std::vector<cv::Point2i> m_defectPixels;
    
public:
    // 自動彩點檢測與修復
    bool EnableAutoDefectFix() {
        // 啟用自動彩點消除
        dvpStatus status = dvpSetAutoDefectFixState(m_handle, true);
        
        if (status == DVP_STATUS_OK) {
            // 創建彩點信息（自動學習）
            status = dvpCreateDefectFixInfo(m_handle);
        }
        
        return status == DVP_STATUS_OK;
    }
    
    // 手動彩點標記
    bool ManualDefectCorrection() {
        // 停用自動模式
        dvpSetAutoDefectFixState(m_handle, false);
        
        // 啟用手動彩點消除
        dvpStatus status = dvpSetDefectFixState(m_handle, true);
        
        // 加載已保存的彩點信息
        status = dvpLoadDefectInfo(m_handle);
        
        return status == DVP_STATUS_OK;
    }
    
    // 配置降噪參數
    bool ConfigureNoiseReduction() {
        dvpStatus status;
        
        // 2D降噪（空間域）
        dvpIntDescr nr2dDescr;
        status = dvpGetNoiseReduct2dDescr(m_handle, &nr2dDescr);
        if (status == DVP_STATUS_OK) {
            int optimalValue = (nr2dDescr.iMin + nr2dDescr.iMax) / 2;
            status = dvpSetNoiseReduct2d(m_handle, optimalValue);
            status = dvpSetNoiseReduct2dState(m_handle, true);
        }
        
        // 3D降噪（時間域）
        dvpIntDescr nr3dDescr;
        status = dvpGetNoiseReduct3dDescr(m_handle, &nr3dDescr);
        if (status == DVP_STATUS_OK) {
            // 3D降噪會產生拖影，適度使用
            status = dvpSetNoiseReduct3d(m_handle, 30);
            status = dvpSetNoiseReduct3dState(m_handle, true);
        }
        
        return true;
    }
};
```

### 3.3 顏色校正

```cpp
class ColorCorrectionManager {
private:
    dvpHandle m_handle;
    dvpColorCorrection m_colorCorrection;
    dvpColorMatrix m_colorMatrix;
    
public:
    // 獲取當前顏色校正參數
    bool GetColorCorrection() {
        dvpStatus status = dvpGetColorCorrection(m_handle, 
                                                &m_colorCorrection);
        return status == DVP_STATUS_OK;
    }
    
    // 設置顏色校正係數
    bool SetColorCorrection(float bGain, float gGain, float rGain) {
        m_colorCorrection.bgr[0] = bGain;
        m_colorCorrection.bgr[1] = gGain;
        m_colorCorrection.bgr[2] = rGain;
        
        dvpStatus status = dvpSetColorCorrection(m_handle, 
                                                m_colorCorrection);
        return status == DVP_STATUS_OK;
    }
    
    // 設置顏色矩陣（高級校正）
    bool SetColorMatrix(const float matrix[4][3]) {
        memcpy(m_colorMatrix.matrix, matrix, sizeof(dvpMatrix));
        
        dvpStatus status = dvpSetColorMatrix(m_handle, m_colorMatrix);
        return status == DVP_STATUS_OK;
    }
    
    // 自動白平衡校正
    bool PerformAutoWhiteBalance() {
        // 設置白平衡ROI
        dvpRegion roi = {100, 100, 200, 200};
        dvpSetAwbRoi(m_handle, roi);
        
        // 執行一次自動白平衡
        dvpStatus status = dvpSetAwbOperation(m_handle, AWB_OP_ONCE);
        
        if (status == DVP_STATUS_OK) {
            // 獲取計算結果
            float bgr[3];
            status = dvpGetColorSolution(m_handle, bgr);
            
            if (status == DVP_STATUS_OK) {
                // 應用白平衡結果
                return SetColorCorrection(bgr[0], bgr[1], bgr[2]);
            }
        }
        
        return false;
    }
};
```

---

## 4. 高級圖像處理

### 4.1 曲線LUT處理

```cpp
class CurveLUTProcessor {
private:
    dvpHandle m_handle;
    std::unique_ptr<dvpCurveLut> m_curveLut;
    
public:
    CurveLUTProcessor() : m_curveLut(new dvpCurveLut()) {}
    
    // 設置Gamma曲線
    bool SetGammaCurve(float gamma) {
        m_curveLut->enable = true;
        m_curveLut->style = CURVE_STYLE_GAMMA;
        
        // 生成Gamma曲線錨點
        for (int ch = 0; ch < 3; ch++) {
            for (int i = 0; i < 256; i++) {
                float normalized = i / 255.0f;
                float corrected = pow(normalized, 1.0f / gamma);
                
                m_curveLut->array.point[ch][i].X = i;
                m_curveLut->array.point[ch][i].Y = corrected * 255;
            }
            m_curveLut->array.cols[ch] = 256;
        }
        m_curveLut->array.rows = 3;
        
        dvpStatus status = dvpSetCurveLut(m_handle, *m_curveLut);
        return status == DVP_STATUS_OK;
    }
    
    // 設置自定義曲線
    bool SetCustomCurve(const std::vector<cv::Point2f>& anchorPoints) {
        m_curveLut->enable = true;
        m_curveLut->style = CURVE_STYLE_CUBIC_SPLINE;
        
        // 使用錨點生成樣條曲線
        int numPoints = std::min((int)anchorPoints.size(), 32);
        
        for (int ch = 0; ch < 3; ch++) {
            for (int i = 0; i < numPoints; i++) {
                m_curveLut->array.point[ch][i].X = anchorPoints[i].x;
                m_curveLut->array.point[ch][i].Y = anchorPoints[i].y;
            }
            m_curveLut->array.cols[ch] = numPoints;
        }
        m_curveLut->array.rows = 3;
        
        dvpStatus status = dvpSetCurveLut(m_handle, *m_curveLut);
        return status == DVP_STATUS_OK;
    }
    
    // 獲取當前LUT
    bool GetCurrentLUT(std::vector<uint8_t>& lut) {
        dvpStatus status = dvpGetCurveLut(m_handle, m_curveLut.get());
        if (status != DVP_STATUS_OK) return false;
        
        // 從曲線生成完整LUT
        lut.resize(256 * 3);
        GenerateLUTFromCurve(lut);
        
        return true;
    }
};
```

### 4.2 平場校正

```cpp
class FlatFieldCorrection {
private:
    dvpHandle m_handle;
    cv::Mat m_darkFrame;
    cv::Mat m_brightFrame;
    cv::Mat m_gainMap;
    
public:
    // 採集暗場圖像
    bool CaptureDarkFrame(int numFrames = 10) {
        std::vector<cv::Mat> darkFrames;
        
        // 關閉光源，採集多幀
        for (int i = 0; i < numFrames; i++) {
            dvpFrame frame;
            void* buffer = nullptr;
            
            dvpStatus status = dvpGetFrame(m_handle, &frame, 
                                          &buffer, 3000);
            if (status == DVP_STATUS_OK) {
                cv::Mat img = ConvertToMat(&frame, buffer);
                darkFrames.push_back(img);
            }
        }
        
        // 計算平均暗場
        m_darkFrame = cv::Mat::zeros(darkFrames[0].size(), CV_32F);
        for (const auto& frame : darkFrames) {
            cv::Mat floatFrame;
            frame.convertTo(floatFrame, CV_32F);
            m_darkFrame += floatFrame;
        }
        m_darkFrame /= numFrames;
        
        return true;
    }
    
    // 採集亮場圖像
    bool CaptureBrightFrame(int numFrames = 10) {
        std::vector<cv::Mat> brightFrames;
        
        // 均勻光照下採集
        for (int i = 0; i < numFrames; i++) {
            dvpFrame frame;
            void* buffer = nullptr;
            
            dvpStatus status = dvpGetFrame(m_handle, &frame, 
                                          &buffer, 3000);
            if (status == DVP_STATUS_OK) {
                cv::Mat img = ConvertToMat(&frame, buffer);
                brightFrames.push_back(img);
            }
        }
        
        // 計算平均亮場
        m_brightFrame = cv::Mat::zeros(brightFrames[0].size(), CV_32F);
        for (const auto& frame : brightFrames) {
            cv::Mat floatFrame;
            frame.convertTo(floatFrame, CV_32F);
            m_brightFrame += floatFrame;
        }
        m_brightFrame /= numFrames;
        
        // 計算增益圖
        CalculateGainMap();
        
        return true;
    }
    
private:
    void CalculateGainMap() {
        // 減去暗場
        cv::Mat correctedBright = m_brightFrame - m_darkFrame;
        
        // 計算平均值
        cv::Scalar meanValue = cv::mean(correctedBright);
        
        // 計算增益圖
        m_gainMap = meanValue[0] / correctedBright;
        
        // 限制增益範圍
        cv::threshold(m_gainMap, m_gainMap, 0.5, 0.5, cv::THRESH_TRUNC);
        cv::threshold(m_gainMap, m_gainMap, 2.0, 2.0, cv::THRESH_TRUNC);
    }
    
public:
    // 應用平場校正
    cv::Mat ApplyCorrection(const cv::Mat& rawImage) {
        cv::Mat floatImage;
        rawImage.convertTo(floatImage, CV_32F);
        
        // 應用校正: (Raw - Dark) * Gain
        cv::Mat corrected = (floatImage - m_darkFrame).mul(m_gainMap);
        
        // 轉換回原始格式
        cv::Mat result;
        corrected.convertTo(result, rawImage.type());
        
        return result;
    }
};
```

### 4.3 圖像旋轉與翻轉

```cpp
class ImageTransformController {
private:
    dvpHandle m_handle;
    bool m_flipH = false;
    bool m_flipV = false;
    bool m_rotate = false;
    bool m_rotateOpposite = false;
    
public:
    // 設置水平翻轉
    bool SetHorizontalFlip(bool enable) {
        dvpStatus status = dvpSetFlipHorizontalState(m_handle, enable);
        if (status == DVP_STATUS_OK) {
            m_flipH = enable;
        }
        return status == DVP_STATUS_OK;
    }
    
    // 設置垂直翻轉
    bool SetVerticalFlip(bool enable) {
        dvpStatus status = dvpSetFlipVerticalState(m_handle, enable);
        if (status == DVP_STATUS_OK) {
            m_flipV = enable;
        }
        return status == DVP_STATUS_OK;
    }
    
    // 設置90度旋轉
    bool SetRotation(bool enable, bool counterClockwise = false) {
        dvpStatus status;
        
        // 設置旋轉狀態
        status = dvpSetRotateState(m_handle, enable);
        if (status != DVP_STATUS_OK) return false;
        
        // 設置旋轉方向
        if (enable) {
            status = dvpSetRotateOpposite(m_handle, counterClockwise);
        }
        
        m_rotate = enable;
        m_rotateOpposite = counterClockwise;
        
        return status == DVP_STATUS_OK;
    }
    
    // 獲取變換後的圖像尺寸
    cv::Size GetTransformedSize(cv::Size originalSize) {
        if (m_rotate) {
            // 90度旋轉後寬高交換
            return cv::Size(originalSize.height, originalSize.width);
        }
        return originalSize;
    }
    
    // 組合變換設置
    bool SetCombinedTransform(int rotationDegrees, 
                              bool mirrorH, 
                              bool mirrorV) {
        // 處理旋轉（0, 90, 180, 270）
        switch (rotationDegrees) {
            case 0:
                SetRotation(false);
                SetHorizontalFlip(mirrorH);
                SetVerticalFlip(mirrorV);
                break;
                
            case 90:
                SetRotation(true, false);
                SetHorizontalFlip(mirrorH);
                SetVerticalFlip(mirrorV);
                break;
                
            case 180:
                SetRotation(false);
                SetHorizontalFlip(!mirrorH);
                SetVerticalFlip(!mirrorV);
                break;
                
            case 270:
                SetRotation(true, true);
                SetHorizontalFlip(mirrorH);
                SetVerticalFlip(mirrorV);
                break;
                
            default:
                return false;
        }
        
        return true;
    }
};
```

---

## 5. 線掃相機支援

### 5.1 行頻設置與控制

```cpp
class LineScanController {
private:
    dvpHandle m_handle;
    double m_lineRate;
    dvpLineTriggerConfig m_trigConfig;
    
public:
    // 初始化線掃參數
    bool Initialize() {
        dvpStatus status;
        
        // 獲取行頻範圍
        dvpDoubleDescr lineRateDescr;
        status = dvpGetLineRateDescr(m_handle, &lineRateDescr);
        if (status != DVP_STATUS_OK) return false;
        
        // 設置初始行頻（Hz）
        m_lineRate = (lineRateDescr.fMin + lineRateDescr.fMax) / 2;
        status = dvpSetLineRate(m_handle, m_lineRate);
        
        return status == DVP_STATUS_OK;
    }
    
    // 設置行頻
    bool SetLineRate(double rateHz) {
        dvpStatus status = dvpSetLineRate(m_handle, rateHz);
        if (status == DVP_STATUS_OK) {
            m_lineRate = rateHz;
        }
        return status == DVP_STATUS_OK;
    }
    
    // 配置線觸發
    bool ConfigureLineTrigger(bool enable, 
                             dvpTriggerSource source = TRIGGER_SOURCE_LINE0) {
        m_trigConfig.enable = enable;
        m_trigConfig.source = source;
        m_trigConfig.activation = TRIGGER_POS_EDGE;
        m_trigConfig.delay = 0;
        
        dvpStatus status = dvpSetLineTriggerConfig(m_handle, m_trigConfig);
        return status == DVP_STATUS_OK;
    }
    
    // 計算編碼器參數
    bool SetupEncoderTrigger(double encoderResolution, 
                            double objectSpeed) {
        // 計算行頻 = 編碼器解析度 * 物體速度
        double calculatedRate = encoderResolution * objectSpeed;
        
        // 配置外部觸發
        ConfigureLineTrigger(true, TRIGGER_SOURCE_LINE0);
        
        // 設置預期行頻（用於緩衝管理）
        SetLineRate(calculatedRate);
        
        return true;
    }
};
```

### 5.2 線掃圖像拼接

```cpp
class LineScanImageBuilder {
private:
    cv::Mat m_accumulator;
    int m_currentLine;
    int m_maxLines;
    int m_lineHeight;
    std::mutex m_bufferMutex;
    
public:
    LineScanImageBuilder(int width, int maxLines, int lineHeight = 1) 
        : m_maxLines(maxLines), m_lineHeight(lineHeight), m_currentLine(0) {
        m_accumulator = cv::Mat::zeros(maxLines * lineHeight, width, CV_8UC3);
    }
    
    // 添加掃描線
    bool AddScanLine(const dvpFrame* frame, void* buffer) {
        std::lock_guard<std::mutex> lock(m_bufferMutex);
        
        if (m_currentLine >= m_maxLines) {
            return false; // 緩衝區已滿
        }
        
        // 轉換幀數據為OpenCV格式
        cv::Mat line = ConvertFrameToMat(frame, buffer);
        
        // 複製到累積器
        cv::Rect roi(0, m_currentLine * m_lineHeight, 
                    line.cols, m_lineHeight);
        line.copyTo(m_accumulator(roi));
        
        m_currentLine++;
        
        return true;
    }
    
    // 獲取拼接圖像
    cv::Mat GetImage() {
        std::lock_guard<std::mutex> lock(m_bufferMutex);
        
        // 返回已掃描部分
        cv::Rect validRegion(0, 0, m_accumulator.cols, 
                            m_currentLine * m_lineHeight);
        return m_accumulator(validRegion).clone();
    }
    
    // 重置緩衝區
    void Reset() {
        std::lock_guard<std::mutex> lock(m_bufferMutex);
        m_accumulator.setTo(cv::Scalar(0, 0, 0));
        m_currentLine = 0;
    }
    
    // 滾動緩衝區實現
    bool AddScanLineRolling(const dvpFrame* frame, void* buffer) {
        std::lock_guard<std::mutex> lock(m_bufferMutex);
        
        cv::Mat line = ConvertFrameToMat(frame, buffer);
        
        if (m_currentLine >= m_maxLines) {
            // 滾動緩衝區
            cv::Mat temp = m_accumulator.clone();
            cv::Rect srcRoi(0, m_lineHeight, 
                           temp.cols, temp.rows - m_lineHeight);
            cv::Rect dstRoi(0, 0, 
                           temp.cols, temp.rows - m_lineHeight);
            temp(srcRoi).copyTo(m_accumulator(dstRoi));
            
            // 添加新行到底部
            cv::Rect newLineRoi(0, (m_maxLines - 1) * m_lineHeight, 
                              line.cols, m_lineHeight);
            line.copyTo(m_accumulator(newLineRoi));
        } else {
            cv::Rect roi(0, m_currentLine * m_lineHeight, 
                       line.cols, m_lineHeight);
            line.copyTo(m_accumulator(roi));
            m_currentLine++;
        }
        
        return true;
    }
};
```

### 5.3 線掃模式優化

```cpp
class LineScanOptimizer {
private:
    dvpHandle m_handle;
    
public:
    // 優化緩衝區配置
    bool OptimizeBufferConfiguration() {
        dvpStatus status;
        
        // 增加緩衝區數量以應對高速掃描
        status = dvpSetBufferQueueSize(m_handle, 32);
        if (status != DVP_STATUS_OK) return false;
        
        // 配置緩衝區模式
        dvpBufferConfig config;
        config.mode = BUFFER_MODE_ZERO_COPY; // 零拷貝模式
        config.count = 32;
        config.size = 0; // 自動計算
        
        status = dvpSetBufferConfig(m_handle, config);
        
        return status == DVP_STATUS_OK;
    }
    
    // 設置快速ROI（僅掃描感興趣區域）
    bool SetScanROI(int startLine, int numLines) {
        dvpRegion roi;
        roi.X = 0;
        roi.Y = startLine;
        roi.W = 0; // 0表示全寬
        roi.H = numLines;
        
        dvpStatus status = dvpSetRoi(m_handle, roi);
        return status == DVP_STATUS_OK;
    }
    
    // 配置方向感知掃描
    bool SetupDirectionalScan(bool forward) {
        // 根據掃描方向調整圖像翻轉
        dvpStatus status;
        
        if (!forward) {
            // 反向掃描時垂直翻轉
            status = dvpSetFlipVerticalState(m_handle, true);
        } else {
            status = dvpSetFlipVerticalState(m_handle, false);
        }
        
        return status == DVP_STATUS_OK;
    }
};
```

---

## 6. 事件與回調系統

### 6.1 事件回調註冊

```cpp
class EventCallbackManager {
private:
    dvpHandle m_handle;
    std::map<dvpEvent, std::function<void(dvpHandle, void*)>> m_eventHandlers;
    
public:
    // 事件回調函數
    static int EventCallback(dvpHandle handle, 
                           dvpEvent event, 
                           void* pContext, 
                           dvpVariant* pVariant) {
        auto* pThis = static_cast<EventCallbackManager*>(pContext);
        return pThis->HandleEvent(handle, event, pVariant);
    }
    
    // 註冊事件處理器
    void RegisterEventHandler(dvpEvent event, 
                             std::function<void(dvpHandle, void*)> handler) {
        m_eventHandlers[event] = handler;
    }
    
    // 設置事件回調
    bool SetupEventCallback() {
        dvpStatus status = dvpRegisterEventCallback(m_handle, 
                                                   EventCallback, 
                                                   DVP_EVENT_ALL, 
                                                   this);
        return status == DVP_STATUS_OK;
    }
    
private:
    int HandleEvent(dvpHandle handle, dvpEvent event, dvpVariant* pVariant) {
        switch (event) {
            case EVENT_CONNECTED:
                OnDeviceConnected(handle);
                break;
                
            case EVENT_DISCONNECTED:
                OnDeviceDisconnected(handle);
                break;
                
            case EVENT_FRAME_LOST:
                OnFrameLost(handle, pVariant);
                break;
                
            case EVENT_FRAME_TIMEOUT:
                OnFrameTimeout(handle);
                break;
                
            case EVENT_LOST_CONNECTION:
                OnConnectionLost(handle);
                break;
                
            case EVENT_RECONNECTED:
                OnReconnected(handle);
                break;
                
            default:
                break;
        }
        
        // 調用用戶註冊的處理器
        auto it = m_eventHandlers.find(event);
        if (it != m_eventHandlers.end()) {
            it->second(handle, pVariant ? pVariant->pData : nullptr);
        }
        
        return 0;
    }
    
    void OnFrameLost(dvpHandle handle, dvpVariant* pVariant) {
        // 記錄丟幀信息
        if (pVariant) {
            uint64_t lostFrameId = *(uint64_t*)pVariant->pData;
            LogError("Frame lost: ID = %llu", lostFrameId);
        }
    }
};
```

### 6.2 流回調實現

```cpp
class StreamCallbackHandler {
private:
    dvpHandle m_handle;
    std::atomic<uint64_t> m_frameCount{0};
    std::atomic<uint64_t> m_errorCount{0};
    
public:
    // 流回調函數
    static int StreamCallback(dvpHandle handle,
                            dvpStreamEvent event,
                            void* pContext,
                            dvpFrame* pFrame,
                            void* pBuffer) {
        auto* pThis = static_cast<StreamCallbackHandler*>(pContext);
        return pThis->ProcessStream(handle, event, pFrame, pBuffer);
    }
    
    // 註冊多個流事件
    bool RegisterStreamCallbacks() {
        dvpStatus status;
        
        // 註冊幀到達事件
        status = dvpRegisterStreamCallback(m_handle, 
                                         StreamCallback,
                                         STREAM_EVENT_ARRIVED,
                                         this);
        if (status != DVP_STATUS_OK) return false;
        
        // 註冊幀處理完成事件
        status = dvpRegisterStreamCallback(m_handle,
                                         StreamCallback,
                                         STREAM_EVENT_PROCESSED,
                                         this);
        
        return status == DVP_STATUS_OK;
    }
    
private:
    int ProcessStream(dvpHandle handle,
                     dvpStreamEvent event,
                     dvpFrame* pFrame,
                     void* pBuffer) {
        switch (event) {
            case STREAM_EVENT_ARRIVED:
                return OnFrameArrived(pFrame, pBuffer);
                
            case STREAM_EVENT_CORRECTED:
                return OnFrameCorrected(pFrame, pBuffer);
                
            case STREAM_EVENT_PROCESSED:
                return OnFrameProcessed(pFrame, pBuffer);
                
            default:
                break;
        }
        
        return 0;
    }
    
    int OnFrameArrived(dvpFrame* pFrame, void* pBuffer) {
        m_frameCount++;
        
        // 執行快速預處理
        if (NeedPreprocessing(pFrame)) {
            PreprocessFrame(pFrame, pBuffer);
        }
        
        return 0; // 繼續處理
    }
    
    int OnFrameProcessed(dvpFrame* pFrame, void* pBuffer) {
        // 執行後處理
        PostprocessFrame(pFrame, pBuffer);
        
        // 返回1表示保留緩衝區
        return ShouldKeepBuffer(pFrame) ? 1 : 0;
    }
};
```

### 6.3 異步事件管理

```cpp
class AsyncEventManager {
private:
    struct EventData {
        dvpEvent event;
        dvpHandle handle;
        std::chrono::steady_clock::time_point timestamp;
        std::any userData;
    };
    
    std::queue<EventData> m_eventQueue;
    std::mutex m_queueMutex;
    std::condition_variable m_queueCV;
    std::thread m_processingThread;
    std::atomic<bool> m_running{true};
    
public:
    AsyncEventManager() {
        m_processingThread = std::thread(&AsyncEventManager::ProcessEvents, this);
    }
    
    ~AsyncEventManager() {
        m_running = false;
        m_queueCV.notify_all();
        if (m_processingThread.joinable()) {
            m_processingThread.join();
        }
    }
    
    // 添加事件到隊列
    void QueueEvent(dvpEvent event, dvpHandle handle, std::any userData = {}) {
        {
            std::lock_guard<std::mutex> lock(m_queueMutex);
            EventData data;
            data.event = event;
            data.handle = handle;
            data.timestamp = std::chrono::steady_clock::now();
            data.userData = userData;
            m_eventQueue.push(data);
        }
        m_queueCV.notify_one();
    }
    
private:
    void ProcessEvents() {
        while (m_running) {
            std::unique_lock<std::mutex> lock(m_queueMutex);
            m_queueCV.wait(lock, [this] { 
                return !m_eventQueue.empty() || !m_running; 
            });
            
            while (!m_eventQueue.empty()) {
                EventData data = m_eventQueue.front();
                m_eventQueue.pop();
                lock.unlock();
                
                // 處理事件
                HandleAsyncEvent(data);
                
                lock.lock();
            }
        }
    }
    
    void HandleAsyncEvent(const EventData& data) {
        // 根據事件類型執行相應操作
        switch (data.event) {
            case EVENT_FRAME_LOST:
                // 異步處理丟幀恢復
                RecoverFromFrameLoss(data.handle);
                break;
                
            case EVENT_LOST_CONNECTION:
                // 異步重連
                AttemptReconnection(data.handle);
                break;
                
            default:
                break;
        }
    }
};
```

---

## 7. 配置管理

### 7.1 XML配置管理

```cpp
class XmlConfigManager {
private:
    dvpHandle m_handle;
    std::string m_configPath;
    
public:
    // 獲取相機XML配置
    bool ExportConfiguration(const std::string& filepath) {
        char xmlBuffer[65536];
        uint32_t bufferSize = sizeof(xmlBuffer);
        
        dvpStatus status = dvpGetXml(m_handle, xmlBuffer, &bufferSize);
        if (status != DVP_STATUS_OK) return false;
        
        // 保存到文件
        std::ofstream file(filepath);
        if (!file.is_open()) return false;
        
        file << xmlBuffer;
        file.close();
        
        return true;
    }
    
    // 解析並應用XML配置
    bool ImportConfiguration(const std::string& filepath) {
        // 讀取XML文件
        std::ifstream file(filepath);
        if (!file.is_open()) return false;
        
        std::string xmlContent((std::istreambuf_iterator<char>(file)),
                              std::istreambuf_iterator<char>());
        
        // 解析並應用配置
        return ParseAndApplyXml(xmlContent);
    }
    
private:
    bool ParseAndApplyXml(const std::string& xml) {
        // 使用簡單的XML解析
        // 實際應用中建議使用成熟的XML庫如TinyXML2
        
        // 示例：解析曝光時間
        auto exposurePos = xml.find("<Exposure>");
        if (exposurePos != std::string::npos) {
            auto endPos = xml.find("</Exposure>", exposurePos);
            std::string value = xml.substr(exposurePos + 10, 
                                          endPos - exposurePos - 10);
            double exposure = std::stod(value);
            dvpSetExposure(m_handle, exposure);
        }
        
        // 解析其他參數...
        
        return true;
    }
};
```

### 7.2 通用屬性操作

```cpp
class GenericPropertyManager {
private:
    dvpHandle m_handle;
    
public:
    // 通過字符串名稱設置屬性
    bool SetPropertyByName(const std::string& propertyName, 
                          const std::string& value) {
        dvpStatus status;
        
        // 構建屬性字符串
        std::string propString = propertyName + "=" + value;
        
        // 使用通用接口設置
        status = dvpSetConfigString(m_handle, propString.c_str());
        
        return status == DVP_STATUS_OK;
    }
    
    // 獲取屬性值
    bool GetPropertyByName(const std::string& propertyName, 
                          std::string& value) {
        char buffer[256];
        dvpStatus status = dvpGetConfigString(m_handle, 
                                             propertyName.c_str(), 
                                             buffer, 
                                             sizeof(buffer));
        if (status == DVP_STATUS_OK) {
            value = buffer;
            return true;
        }
        return false;
    }
    
    // 批量設置屬性
    bool SetProperties(const std::map<std::string, std::string>& properties) {
        for (const auto& [key, value] : properties) {
            if (!SetPropertyByName(key, value)) {
                return false;
            }
        }
        return true;
    }
};
```

### 7.3 用戶設置管理

```cpp
class UserSettingsManager {
private:
    dvpHandle m_handle;
    
public:
    // 保存當前設置到用戶槽
    bool SaveUserSettings(int slot) {
        if (slot < 0 || slot > 3) return false;
        
        dvpStatus status = dvpSaveUserSet(m_handle, slot);
        return status == DVP_STATUS_OK;
    }
    
    // 加載用戶設置
    bool LoadUserSettings(int slot) {
        if (slot < 0 || slot > 3) return false;
        
        dvpStatus status = dvpLoadUserSet(m_handle, slot);
        return status == DVP_STATUS_OK;
    }
    
    // 導出設置到文件
    bool ExportSettings(const std::string& filepath) {
        std::ofstream file(filepath, std::ios::binary);
        if (!file.is_open()) return false;
        
        // 導出所有重要參數
        CameraSettings settings;
        CollectCurrentSettings(settings);
        
        file.write(reinterpret_cast<const char*>(&settings), 
                  sizeof(settings));
        file.close();
        
        return true;
    }
    
    // 從文件導入設置
    bool ImportSettings(const std::string& filepath) {
        std::ifstream file(filepath, std::ios::binary);
        if (!file.is_open()) return false;
        
        CameraSettings settings;
        file.read(reinterpret_cast<char*>(&settings), 
                 sizeof(settings));
        file.close();
        
        return ApplySettings(settings);
    }
    
private:
    struct CameraSettings {
        double exposure;
        float analogGain;
        dvpRegion roi;
        bool flipH, flipV, rotate;
        dvpColorCorrection colorCorrection;
        // 其他參數...
    };
    
    void CollectCurrentSettings(CameraSettings& settings) {
        dvpGetExposure(m_handle, &settings.exposure);
        dvpGetAnalogGain(m_handle, &settings.analogGain);
        dvpGetRoi(m_handle, &settings.roi);
        dvpGetFlipHorizontalState(m_handle, &settings.flipH);
        dvpGetFlipVerticalState(m_handle, &settings.flipV);
        dvpGetRotateState(m_handle, &settings.rotate);
        dvpGetColorCorrection(m_handle, &settings.colorCorrection);
    }
    
    bool ApplySettings(const CameraSettings& settings) {
        dvpSetExposure(m_handle, settings.exposure);
        dvpSetAnalogGain(m_handle, settings.analogGain);
        dvpSetRoi(m_handle, settings.roi);
        dvpSetFlipHorizontalState(m_handle, settings.flipH);
        dvpSetFlipVerticalState(m_handle, settings.flipV);
        dvpSetRotateState(m_handle, settings.rotate);
        dvpSetColorCorrection(m_handle, settings.colorCorrection);
        
        return true;
    }
};
```

---

## 8. 性能優化

### 8.1 緩衝區優化

```cpp
class BufferOptimizer {
private:
    dvpHandle m_handle;
    
public:
    // 配置高性能緩衝區
    bool ConfigureHighPerformanceBuffer() {
        dvpStatus status;
        
        // 獲取緩衝區大小描述
        dvpIntDescr bufferSizeDescr;
        status = dvpGetBufferQueueSizeDescr(m_handle, &bufferSizeDescr);
        if (status != DVP_STATUS_OK) return false;
        
        // 設置優化的緩衝區數量（根據內存和需求調整）
        int optimalBuffers = CalculateOptimalBuffers();
        status = dvpSetBufferQueueSize(m_handle, optimalBuffers);
        if (status != DVP_STATUS_OK) return false;
        
        // 配置緩衝區參數
        dvpBufferConfig config;
        config.mode = BUFFER_MODE_ZERO_COPY;
        config.count = optimalBuffers;
        config.allocator = ALLOCATOR_PAGE_LOCKED; // 頁鎖定內存
        
        status = dvpSetBufferConfig(m_handle, config);
        
        return status == DVP_STATUS_OK;
    }
    
private:
    int CalculateOptimalBuffers() {
        // 根據幀率和處理時間計算
        double fps, exposureTime;
        dvpGetFrameRate(m_handle, &fps);
        dvpGetExposure(m_handle, &exposureTime);
        
        // 處理時間估算（毫秒）
        double processingTime = 10.0; // 假設10ms處理時間
        
        // 計算需要的緩衝區數量
        int buffers = (int)ceil((processingTime * fps) / 1000.0) + 2;
        
        // 限制範圍
        return std::clamp(buffers, 4, 32);
    }
};
```

### 8.2 Zero-Copy技術

```cpp
class ZeroCopyProcessor {
private:
    dvpHandle m_handle;
    
public:
    // 配置零拷貝處理管道
    bool SetupZeroCopyPipeline() {
        // 使用專用線程處理
        dvpStatus status = dvpRegisterStreamCallback(m_handle,
                                                    ProcessCallback,
                                                    STREAM_EVENT_FRAME_THREAD,
                                                    this);
        if (status != DVP_STATUS_OK) return false;
        
        // 配置直接內存訪問
        dvpBufferConfig config;
        config.mode = BUFFER_MODE_ZERO_COPY;
        config.allocator = ALLOCATOR_DMA_CAPABLE;
        
        status = dvpSetBufferConfig(m_handle, config);
        
        return status == DVP_STATUS_OK;
    }
    
    // 零拷貝處理回調
    static int ProcessCallback(dvpHandle handle,
                              dvpStreamEvent event,
                              void* pContext,
                              dvpFrame* pFrame,
                              void* pBuffer) {
        auto* pThis = static_cast<ZeroCopyProcessor*>(pContext);
        
        // 直接在原始緩衝區上處理
        pThis->ProcessInPlace(pFrame, pBuffer);
        
        // 返回1保持緩衝區，避免拷貝
        return 1;
    }
    
private:
    void ProcessInPlace(dvpFrame* pFrame, void* pBuffer) {
        // 使用SIMD指令直接處理
        ProcessWithSIMD(pBuffer, pFrame->uBytes);
        
        // GPU處理（如果可用）
        #ifdef USE_CUDA
        ProcessWithCUDA(pBuffer, pFrame->iWidth, pFrame->iHeight);
        #endif
    }
    
    void ProcessWithSIMD(void* buffer, uint32_t size) {
        // 使用AVX2/SSE進行向量化處理
        __m256i* data = reinterpret_cast<__m256i*>(buffer);
        size_t vecSize = size / sizeof(__m256i);
        
        for (size_t i = 0; i < vecSize; i++) {
            // SIMD處理示例
            __m256i vec = _mm256_load_si256(&data[i]);
            vec = _mm256_add_epi8(vec, _mm256_set1_epi8(1));
            _mm256_store_si256(&data[i], vec);
        }
    }
};
```

### 8.3 多線程優化

```cpp
class MultiThreadOptimizer {
private:
    dvpHandle m_handle;
    std::vector<std::thread> m_processingThreads;
    std::queue<std::pair<cv::Mat, uint64_t>> m_frameQueue;
    std::mutex m_queueMutex;
    std::condition_variable m_queueCV;
    std::atomic<bool> m_running{true};
    
public:
    // 初始化多線程處理
    bool InitializeThreadPool(int numThreads = 0) {
        if (numThreads == 0) {
            numThreads = std::thread::hardware_concurrency();
        }
        
        for (int i = 0; i < numThreads; i++) {
            m_processingThreads.emplace_back(
                &MultiThreadOptimizer::ProcessingThread, this);
        }
        
        return true;
    }
    
    // 添加幀到處理隊列
    void QueueFrame(const cv::Mat& frame, uint64_t frameId) {
        {
            std::lock_guard<std::mutex> lock(m_queueMutex);
            m_frameQueue.push({frame.clone(), frameId});
        }
        m_queueCV.notify_one();
    }
    
private:
    void ProcessingThread() {
        while (m_running) {
            std::unique_lock<std::mutex> lock(m_queueMutex);
            m_queueCV.wait(lock, [this] { 
                return !m_frameQueue.empty() || !m_running; 
            });
            
            if (!m_frameQueue.empty()) {
                auto [frame, frameId] = m_frameQueue.front();
                m_frameQueue.pop();
                lock.unlock();
                
                // 處理幀
                ProcessFrame(frame, frameId);
            }
        }
    }
    
    void ProcessFrame(const cv::Mat& frame, uint64_t frameId) {
        // CPU密集型處理
        cv::Mat processed;
        
        // 並行處理不同區域
        std::vector<std::future<cv::Mat>> futures;
        int numSections = 4;
        int sectionHeight = frame.rows / numSections;
        
        for (int i = 0; i < numSections; i++) {
            futures.push_back(std::async(std::launch::async, [&, i]() {
                cv::Rect roi(0, i * sectionHeight, 
                           frame.cols, sectionHeight);
                return ProcessSection(frame(roi));
            }));
        }
        
        // 合併結果
        std::vector<cv::Mat> sections;
        for (auto& f : futures) {
            sections.push_back(f.get());
        }
        cv::vconcat(sections, processed);
        
        // 輸出處理結果
        OnFrameProcessed(processed, frameId);
    }
    
    cv::Mat ProcessSection(const cv::Mat& section) {
        cv::Mat result;
        // 執行圖像處理算法
        cv::GaussianBlur(section, result, cv::Size(5, 5), 1.0);
        return result;
    }
    
    virtual void OnFrameProcessed(const cv::Mat& frame, uint64_t frameId) {
        // 由派生類實現
    }
};
```

### 8.4 Qt整合性能監控

```cpp
class QtPerformanceMonitor : public QWidget {
    Q_OBJECT
    
private:
    dvpHandle m_handle;
    QTimer* m_updateTimer;
    QLabel* m_fpsLabel;
    QLabel* m_bandwidthLabel;
    QLabel* m_cpuLabel;
    QProgressBar* m_bufferUsage;
    
    struct PerformanceMetrics {
        double fps;
        double bandwidth; // MB/s
        int bufferUsage;  // percentage
        double cpuUsage;  // percentage
        uint64_t droppedFrames;
    };
    
    PerformanceMetrics m_metrics;
    
public:
    QtPerformanceMonitor(dvpHandle handle, QWidget* parent = nullptr) 
        : QWidget(parent), m_handle(handle) {
        setupUI();
        
        m_updateTimer = new QTimer(this);
        connect(m_updateTimer, &QTimer::timeout, 
                this, &QtPerformanceMonitor::updateMetrics);
        m_updateTimer->start(100); // 10Hz更新
    }
    
private slots:
    void updateMetrics() {
        // 獲取幀率
        dvpGetFrameRate(m_handle, &m_metrics.fps);
        
        // 計算帶寬
        dvpFrame frame;
        dvpGetFrameInfo(m_handle, &frame);
        m_metrics.bandwidth = (frame.uBytes * m_metrics.fps) / (1024.0 * 1024.0);
        
        // 獲取緩衝區使用情況
        int used, total;
        dvpGetBufferUsage(m_handle, &used, &total);
        m_metrics.bufferUsage = (used * 100) / total;
        
        // 更新UI
        m_fpsLabel->setText(QString("FPS: %1").arg(m_metrics.fps, 0, 'f', 1));
        m_bandwidthLabel->setText(QString("Bandwidth: %1 MB/s")
                                 .arg(m_metrics.bandwidth, 0, 'f', 1));
        m_bufferUsage->setValue(m_metrics.bufferUsage);
        
        // 檢查性能問題
        if (m_metrics.fps < 25.0) {
            emit performanceWarning("Low frame rate detected");
        }
        
        if (m_metrics.bufferUsage > 90) {
            emit performanceWarning("Buffer overflow risk");
        }
    }
    
signals:
    void performanceWarning(const QString& message);
};
```

---

## 工業AOI應用案例

### 案例1：高速同步檢測系統

```cpp
class IndustrialAOISystem {
private:
    std::vector<dvpHandle> m_cameras;
    HardwareTriggerManager m_triggerManager;
    MultiThreadOptimizer m_processor;
    
public:
    // 初始化AOI系統
    bool Initialize() {
        // 1. 初始化多相機
        InitializeCameras();
        
        // 2. 配置硬體觸發同步
        ConfigureSynchronization();
        
        // 3. 優化性能設置
        OptimizePerformance();
        
        // 4. 啟動處理管道
        StartProcessingPipeline();
        
        return true;
    }
    
private:
    void ConfigureSynchronization() {
        // 配置主從同步模式
        for (size_t i = 0; i < m_cameras.size(); i++) {
            if (i == 0) {
                // 主相機
                m_triggerManager.ConfigureHardwareTrigger();
            } else {
                // 從相機
                dvpSetTriggerSource(m_cameras[i], TRIGGER_SOURCE_LINE0);
                dvpSetBoolValue(m_cameras[i], V_TRIGGER_MODE_B, true);
            }
        }
    }
};
```

### 案例2：線掃描檢測應用

```cpp
class LineScanInspection {
private:
    LineScanController m_controller;
    LineScanImageBuilder m_imageBuilder;
    
public:
    // 配置產線檢測
    bool SetupProductionLine(double conveyorSpeed, double resolution) {
        // 計算行頻
        double lineRate = conveyorSpeed * resolution;
        
        // 設置線掃參數
        m_controller.SetLineRate(lineRate);
        m_controller.ConfigureLineTrigger(true, TRIGGER_SOURCE_LINE0);
        
        // 配置圖像拼接
        int imageWidth = 4096;  // 相機寬度
        int maxLines = 10000;   // 最大掃描行數
        m_imageBuilder = LineScanImageBuilder(imageWidth, maxLines);
        
        return true;
    }
};
```

---

## 總結

本文檔詳細介紹了Do3Think Camera SDK的進階功能，包括：

1. **觸發模式**：支援軟體/硬體觸發，精確控制採集時機
2. **多相機同步**：主從模式、時間戳同步，滿足多視角檢測需求
3. **硬體ISP**：利用相機內建ISP，降低CPU負載
4. **圖像處理**：曲線LUT、平場校正等專業功能
5. **線掃支援**：完整的線掃相機控制和圖像拼接
6. **事件系統**：豐富的事件回調機制
7. **配置管理**：XML配置、用戶設置管理
8. **性能優化**：零拷貝、多線程、緩衝區優化

這些進階功能為工業視覺應用提供了強大的技術支撐，能夠滿足高速、高精度、高可靠性的工業檢測需求。建議開發者根據具體應用場景，選擇合適的功能組合，構建高效的機器視覺系統。