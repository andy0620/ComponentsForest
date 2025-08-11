# Do3ThinkCamera SDK - 設備連線與初始化開發指南

## 目錄
1. [概述](#概述)
2. [SDK初始化流程](#sdk初始化流程)
3. [設備掃描與枚舉](#設備掃描與枚舉)
4. [設備信息結構](#設備信息結構)
5. [打開模式選擇](#打開模式選擇)
6. [多相機管理策略](#多相機管理策略)
7. [連接狀態管理](#連接狀態管理)
8. [資源釋放流程](#資源釋放流程)
9. [完整程式碼範例](#完整程式碼範例)
10. [Qt整合建議](#qt整合建議)
11. [常見問題與解決方案](#常見問題與解決方案)

---

## 概述

Do3ThinkCamera SDK提供了完整的相機連線與初始化API，支援USB3.0和GigE Vision工業相機。本文檔詳細說明SDK的初始化流程、設備管理以及最佳實踐。

### 主要特性
- 支援熱插拔檢測
- 多相機同時連接
- 自動設備識別
- 穩定的連接管理
- 完整的錯誤處理機制

---

## SDK初始化流程

### 流程圖
```
┌─────────────┐
│  應用啟動   │
└──────┬──────┘
       ↓
┌─────────────┐
│ 載入SDK DLL │
└──────┬──────┘
       ↓
┌─────────────┐
│ dvpRefresh()│
└──────┬──────┘
       ↓
┌─────────────┐
│  dvpEnum()  │
└──────┬──────┘
       ↓
┌─────────────┐
│ 選擇設備    │
└──────┬──────┘
       ↓
┌─────────────┐
│dvpOpenByName│
└──────┬──────┘
       ↓
┌─────────────┐
│ 初始化參數  │
└─────────────┘
```

### 初始化程式碼範例

```cpp
#include "DVPCamera.h"
#include <iostream>
#include <vector>

class CameraInitializer {
private:
    dvpHandle m_handle;
    bool m_isInitialized;
    
public:
    CameraInitializer() : m_handle(0), m_isInitialized(false) {}
    
    dvpStatus Initialize() {
        dvpStatus status;
        
        // Step 1: 刷新設備列表
        status = dvpRefresh();
        if (status != DVP_STATUS_OK) {
            std::cerr << "Failed to refresh device list: " << status << std::endl;
            return status;
        }
        
        // Step 2: 獲取設備數量
        dvpUint32 deviceCount = 0;
        status = dvpEnum(&deviceCount, nullptr);
        if (status != DVP_STATUS_OK || deviceCount == 0) {
            std::cerr << "No devices found" << std::endl;
            return DVP_STATUS_NO_DEVICE;
        }
        
        std::cout << "Found " << deviceCount << " device(s)" << std::endl;
        
        // Step 3: 枚舉設備信息
        std::vector<dvpCameraInfo> deviceInfoList(deviceCount);
        status = dvpEnum(&deviceCount, deviceInfoList.data());
        if (status != DVP_STATUS_OK) {
            std::cerr << "Failed to enumerate devices: " << status << std::endl;
            return status;
        }
        
        // Step 4: 打開第一個設備
        status = dvpOpenByName(deviceInfoList[0].FriendlyName, 
                              OPEN_NORMAL, &m_handle);
        if (status != DVP_STATUS_OK) {
            std::cerr << "Failed to open device: " << status << std::endl;
            return status;
        }
        
        m_isInitialized = true;
        std::cout << "Successfully opened device: " 
                  << deviceInfoList[0].FriendlyName << std::endl;
        
        return DVP_STATUS_OK;
    }
    
    ~CameraInitializer() {
        if (m_isInitialized && m_handle != 0) {
            dvpClose(m_handle);
        }
    }
};
```

---

## 設備掃描與枚舉

### API詳解

#### dvpRefresh()
```cpp
dvpStatus dvpRefresh();
```
**功能**：刷新系統中的相機設備列表  
**返回值**：DVP_STATUS_OK表示成功  
**使用場景**：
- 應用程式啟動時
- 熱插拔事件後
- 手動重新掃描設備

#### dvpEnum()
```cpp
dvpStatus dvpEnum(dvpUint32* pCount, dvpCameraInfo* pCameraInfo);
```
**功能**：枚舉設備信息  
**參數**：
- `pCount`：輸入/輸出參數，設備數量
- `pCameraInfo`：設備信息陣列，可為nullptr（僅獲取數量）

### 設備掃描範例

```cpp
class DeviceScanner {
public:
    struct ScanResult {
        dvpUint32 deviceCount;
        std::vector<dvpCameraInfo> deviceList;
        dvpStatus lastError;
    };
    
    static ScanResult ScanDevices(bool autoRefresh = true) {
        ScanResult result;
        result.deviceCount = 0;
        result.lastError = DVP_STATUS_OK;
        
        // 可選：自動刷新
        if (autoRefresh) {
            result.lastError = dvpRefresh();
            if (result.lastError != DVP_STATUS_OK) {
                return result;
            }
        }
        
        // 第一次調用：獲取設備數量
        result.lastError = dvpEnum(&result.deviceCount, nullptr);
        if (result.lastError != DVP_STATUS_OK || result.deviceCount == 0) {
            return result;
        }
        
        // 第二次調用：獲取設備信息
        result.deviceList.resize(result.deviceCount);
        result.lastError = dvpEnum(&result.deviceCount, result.deviceList.data());
        
        return result;
    }
    
    static void PrintDeviceInfo(const dvpCameraInfo& info) {
        std::cout << "=== Device Information ===" << std::endl;
        std::cout << "Friendly Name: " << info.FriendlyName << std::endl;
        std::cout << "Serial Number: " << info.SerialNumber << std::endl;
        std::cout << "Product Name: " << info.ProductName << std::endl;
        std::cout << "Manufacturer: " << info.Manufacturer << std::endl;
        std::cout << "Vendor Name: " << info.VendorName << std::endl;
        std::cout << "Model Name: " << info.ModelName << std::endl;
        std::cout << "Camera Type: " << info.CameraType << std::endl;
        std::cout << "Sensor Type: " << info.SensorType << std::endl;
        std::cout << "Port Type: " << info.PortType << std::endl;
        std::cout << "User ID: " << info.UserID << std::endl;
        std::cout << "=========================" << std::endl;
    }
};
```

---

## 設備信息結構

### dvpCameraInfo結構體詳解

```cpp
typedef struct _dvpCameraInfo {
    char FriendlyName[DVP_MAX_DEVICE_NAME_LEN];  // 設備友好名稱
    char PortType[DVP_MAX_DEVICE_NAME_LEN];      // 連接埠類型 (USB3.0/GigE)
    char SerialNumber[DVP_MAX_DEVICE_NAME_LEN];  // 序列號
    char CameraType[DVP_MAX_DEVICE_NAME_LEN];    // 相機型號
    char SensorType[DVP_MAX_DEVICE_NAME_LEN];    // 感測器型號
    char ProductName[DVP_MAX_DEVICE_NAME_LEN];   // 產品名稱
    char Manufacturer[DVP_MAX_DEVICE_NAME_LEN];  // 製造商
    char VendorName[DVP_MAX_DEVICE_NAME_LEN];    // 供應商名稱
    char ModelName[DVP_MAX_DEVICE_NAME_LEN];     // 模型名稱
    char UserID[DVP_MAX_DEVICE_NAME_LEN];        // 用戶自定義ID
    dvpUint32 Reserved[32];                      // 保留欄位
} dvpCameraInfo;
```

### 設備識別策略

```cpp
class DeviceIdentifier {
public:
    // 通過序列號識別特定設備
    static bool FindDeviceBySerial(const std::string& serialNumber,
                                   dvpCameraInfo& outInfo) {
        auto scanResult = DeviceScanner::ScanDevices();
        
        for (const auto& device : scanResult.deviceList) {
            if (std::string(device.SerialNumber) == serialNumber) {
                outInfo = device;
                return true;
            }
        }
        return false;
    }
    
    // 通過型號篩選設備
    static std::vector<dvpCameraInfo> FilterByModel(const std::string& modelName) {
        std::vector<dvpCameraInfo> filtered;
        auto scanResult = DeviceScanner::ScanDevices();
        
        for (const auto& device : scanResult.deviceList) {
            if (std::string(device.ModelName).find(modelName) != std::string::npos) {
                filtered.push_back(device);
            }
        }
        return filtered;
    }
    
    // 通過連接埠類型篩選
    static std::vector<dvpCameraInfo> FilterByPortType(const std::string& portType) {
        std::vector<dvpCameraInfo> filtered;
        auto scanResult = DeviceScanner::ScanDevices();
        
        for (const auto& device : scanResult.deviceList) {
            if (std::string(device.PortType) == portType) {
                filtered.push_back(device);
            }
        }
        return filtered;
    }
};
```

---

## 打開模式選擇

### 打開模式枚舉

```cpp
typedef enum _dvpOpenMode {
    OPEN_NORMAL = 0,    // 正常模式（推薦）
    OPEN_DEBUG = 1,     // 調試模式
    OPEN_OFFLINE = 2,   // 離線模式
    OPEN_EXCLUSIVE = 4  // 獨占模式
} dvpOpenMode;
```

### 各模式詳解與使用場景

| 模式 | 說明 | 使用場景 |
|------|------|----------|
| OPEN_NORMAL | 標準打開模式，最常用 | 一般應用開發 |
| OPEN_DEBUG | 啟用調試輸出 | 開發階段排錯 |
| OPEN_OFFLINE | 離線模式，不連接實體設備 | 模擬測試 |
| OPEN_EXCLUSIVE | 獨占訪問，阻止其他程式訪問 | 關鍵應用 |

### 打開設備範例

```cpp
class DeviceOpener {
private:
    dvpHandle m_handle;
    dvpOpenMode m_openMode;
    std::string m_deviceName;
    
public:
    // 通過名稱打開設備
    dvpStatus OpenByName(const std::string& friendlyName, 
                         dvpOpenMode mode = OPEN_NORMAL) {
        dvpStatus status = dvpOpenByName(friendlyName.c_str(), mode, &m_handle);
        if (status == DVP_STATUS_OK) {
            m_deviceName = friendlyName;
            m_openMode = mode;
            std::cout << "Device opened successfully: " << friendlyName << std::endl;
        } else {
            HandleOpenError(status);
        }
        return status;
    }
    
    // 通過用戶ID打開設備
    dvpStatus OpenByUserId(const std::string& userId,
                           dvpOpenMode mode = OPEN_NORMAL) {
        dvpStatus status = dvpOpenByUserId(userId.c_str(), mode, &m_handle);
        if (status == DVP_STATUS_OK) {
            m_deviceName = userId;
            m_openMode = mode;
            std::cout << "Device opened by UserID: " << userId << std::endl;
        } else {
            HandleOpenError(status);
        }
        return status;
    }
    
    // 智能打開（優先序列號，其次友好名稱）
    dvpStatus SmartOpen(const dvpCameraInfo& info,
                        dvpOpenMode mode = OPEN_NORMAL) {
        // 優先使用序列號（唯一性保證）
        if (strlen(info.SerialNumber) > 0) {
            std::string userId = std::string("SN:") + info.SerialNumber;
            return OpenByUserId(userId, mode);
        }
        // 備選：使用友好名稱
        return OpenByName(info.FriendlyName, mode);
    }
    
private:
    void HandleOpenError(dvpStatus status) {
        switch(status) {
            case DVP_STATUS_NO_DEVICE:
                std::cerr << "Error: No device found" << std::endl;
                break;
            case DVP_STATUS_DEVICE_BUSY:
                std::cerr << "Error: Device is already in use" << std::endl;
                break;
            case DVP_STATUS_ACCESS_DENIED:
                std::cerr << "Error: Access denied (check permissions)" << std::endl;
                break;
            case DVP_STATUS_INVALID_PARAMETER:
                std::cerr << "Error: Invalid parameter" << std::endl;
                break;
            default:
                std::cerr << "Error: Failed to open device (code: " 
                         << status << ")" << std::endl;
        }
    }
    
public:
    dvpHandle GetHandle() const { return m_handle; }
    bool IsOpen() const { return m_handle != 0; }
    
    ~DeviceOpener() {
        if (IsOpen()) {
            dvpClose(m_handle);
        }
    }
};
```

---

## 多相機管理策略

### 多相機管理器設計

```cpp
#include <map>
#include <memory>
#include <mutex>
#include <thread>

class MultiCameraManager {
private:
    struct CameraInstance {
        dvpHandle handle;
        dvpCameraInfo info;
        bool isStreaming;
        std::thread workerThread;
        bool shouldStop;
        
        CameraInstance() : handle(0), isStreaming(false), shouldStop(false) {}
    };
    
    std::map<std::string, std::unique_ptr<CameraInstance>> m_cameras;
    std::mutex m_mutex;
    
public:
    // 添加相機
    dvpStatus AddCamera(const dvpCameraInfo& info) {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        std::string key = info.SerialNumber;
        if (m_cameras.find(key) != m_cameras.end()) {
            return DVP_STATUS_DEVICE_BUSY;  // 已存在
        }
        
        auto camera = std::make_unique<CameraInstance>();
        camera->info = info;
        
        // 打開相機
        dvpStatus status = dvpOpenByName(info.FriendlyName, 
                                         OPEN_NORMAL, 
                                         &camera->handle);
        if (status != DVP_STATUS_OK) {
            return status;
        }
        
        m_cameras[key] = std::move(camera);
        std::cout << "Camera added: " << info.FriendlyName << std::endl;
        
        return DVP_STATUS_OK;
    }
    
    // 移除相機
    dvpStatus RemoveCamera(const std::string& serialNumber) {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        auto it = m_cameras.find(serialNumber);
        if (it == m_cameras.end()) {
            return DVP_STATUS_NO_DEVICE;
        }
        
        auto& camera = it->second;
        
        // 停止串流
        if (camera->isStreaming) {
            StopStream(serialNumber);
        }
        
        // 關閉設備
        if (camera->handle != 0) {
            dvpClose(camera->handle);
        }
        
        m_cameras.erase(it);
        std::cout << "Camera removed: " << serialNumber << std::endl;
        
        return DVP_STATUS_OK;
    }
    
    // 啟動指定相機的串流
    dvpStatus StartStream(const std::string& serialNumber) {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        auto it = m_cameras.find(serialNumber);
        if (it == m_cameras.end()) {
            return DVP_STATUS_NO_DEVICE;
        }
        
        auto& camera = it->second;
        if (camera->isStreaming) {
            return DVP_STATUS_OK;  // 已在串流中
        }
        
        dvpStatus status = dvpStart(camera->handle);
        if (status == DVP_STATUS_OK) {
            camera->isStreaming = true;
            
            // 啟動工作執行緒
            camera->shouldStop = false;
            camera->workerThread = std::thread([this, serialNumber]() {
                ProcessFrames(serialNumber);
            });
        }
        
        return status;
    }
    
    // 停止指定相機的串流
    dvpStatus StopStream(const std::string& serialNumber) {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        auto it = m_cameras.find(serialNumber);
        if (it == m_cameras.end()) {
            return DVP_STATUS_NO_DEVICE;
        }
        
        auto& camera = it->second;
        if (!camera->isStreaming) {
            return DVP_STATUS_OK;
        }
        
        // 設置停止標誌
        camera->shouldStop = true;
        
        // 等待工作執行緒結束
        if (camera->workerThread.joinable()) {
            camera->workerThread.join();
        }
        
        dvpStatus status = dvpStop(camera->handle);
        camera->isStreaming = false;
        
        return status;
    }
    
    // 啟動所有相機
    void StartAllCameras() {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        for (auto& pair : m_cameras) {
            const std::string& serialNumber = pair.first;
            StartStream(serialNumber);
        }
    }
    
    // 停止所有相機
    void StopAllCameras() {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        for (auto& pair : m_cameras) {
            const std::string& serialNumber = pair.first;
            StopStream(serialNumber);
        }
    }
    
    // 獲取相機數量
    size_t GetCameraCount() const {
        std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(m_mutex));
        return m_cameras.size();
    }
    
    // 獲取相機句柄
    dvpHandle GetCameraHandle(const std::string& serialNumber) {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        auto it = m_cameras.find(serialNumber);
        if (it != m_cameras.end()) {
            return it->second->handle;
        }
        return 0;
    }
    
private:
    void ProcessFrames(const std::string& serialNumber) {
        auto it = m_cameras.find(serialNumber);
        if (it == m_cameras.end()) return;
        
        auto& camera = it->second;
        dvpFrame frame;
        
        while (!camera->shouldStop) {
            dvpStatus status = dvpGetFrame(camera->handle, &frame, 1000);
            if (status == DVP_STATUS_OK) {
                // 處理影像幀
                OnFrameReceived(serialNumber, frame);
            }
        }
    }
    
    void OnFrameReceived(const std::string& serialNumber, const dvpFrame& frame) {
        // 實現影像處理邏輯
        std::cout << "Frame received from " << serialNumber 
                  << " - Size: " << frame.iWidth << "x" << frame.iHeight 
                  << std::endl;
    }
    
public:
    ~MultiCameraManager() {
        StopAllCameras();
        
        for (auto& pair : m_cameras) {
            if (pair.second->handle != 0) {
                dvpClose(pair.second->handle);
            }
        }
    }
};
```

### 同步採集策略

```cpp
class SynchronizedCapture {
private:
    std::vector<dvpHandle> m_handles;
    std::mutex m_frameMutex;
    std::map<dvpHandle, dvpFrame> m_latestFrames;
    
public:
    // 軟體觸發同步
    void SoftwareTriggerSync() {
        // 設置所有相機為軟體觸發模式
        for (auto handle : m_handles) {
            dvpSetTriggerSource(handle, TRIGGER_SOURCE_SOFTWARE);
            dvpSetTriggerMode(handle, TRIGGER_MODE_ON);
        }
        
        // 同時觸發所有相機
        for (auto handle : m_handles) {
            dvpSoftwareTrigger(handle);
        }
        
        // 收集所有影像
        CollectFrames();
    }
    
    // 硬體觸發同步（外部觸發源）
    void HardwareTriggerSync() {
        // 設置所有相機為硬體觸發模式
        for (auto handle : m_handles) {
            dvpSetTriggerSource(handle, TRIGGER_SOURCE_LINE0);
            dvpSetTriggerMode(handle, TRIGGER_MODE_ON);
        }
        
        // 等待外部觸發信號
        // 影像將自動採集
    }
    
private:
    void CollectFrames() {
        std::vector<std::thread> collectors;
        
        for (auto handle : m_handles) {
            collectors.emplace_back([this, handle]() {
                dvpFrame frame;
                if (dvpGetFrame(handle, &frame, 5000) == DVP_STATUS_OK) {
                    std::lock_guard<std::mutex> lock(m_frameMutex);
                    m_latestFrames[handle] = frame;
                }
            });
        }
        
        // 等待所有採集完成
        for (auto& t : collectors) {
            t.join();
        }
    }
};
```

---

## 連接狀態管理

### 狀態檢查與管理

```cpp
class ConnectionManager {
private:
    dvpHandle m_handle;
    bool m_isConnected;
    std::thread m_monitorThread;
    bool m_shouldStop;
    
public:
    ConnectionManager() : m_handle(0), m_isConnected(false), m_shouldStop(false) {}
    
    // 檢查句柄有效性
    bool IsValid() const {
        return dvpIsValid(m_handle) == DVP_STATUS_OK;
    }
    
    // 獲取串流狀態
    dvpStreamState GetStreamState() {
        dvpStreamState state;
        dvpStatus status = dvpGetStreamState(m_handle, &state);
        
        if (status == DVP_STATUS_OK) {
            return state;
        }
        
        return STREAM_STATE_STOPPED;  // 預設為停止狀態
    }
    
    // 啟動連接監控
    void StartMonitoring() {
        m_shouldStop = false;
        m_monitorThread = std::thread([this]() {
            MonitorConnection();
        });
    }
    
    // 停止連接監控
    void StopMonitoring() {
        m_shouldStop = true;
        if (m_monitorThread.joinable()) {
            m_monitorThread.join();
        }
    }
    
private:
    void MonitorConnection() {
        while (!m_shouldStop) {
            if (m_isConnected) {
                // 檢查連接狀態
                if (!IsValid()) {
                    OnConnectionLost();
                    m_isConnected = false;
                }
                
                // 檢查串流狀態
                dvpStreamState state = GetStreamState();
                OnStreamStateChanged(state);
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }
    
    void OnConnectionLost() {
        std::cerr << "Connection lost! Attempting to reconnect..." << std::endl;
        // 實現重連邏輯
        AttemptReconnect();
    }
    
    void OnStreamStateChanged(dvpStreamState state) {
        static dvpStreamState lastState = STREAM_STATE_STOPPED;
        
        if (state != lastState) {
            switch (state) {
                case STREAM_STATE_STARTED:
                    std::cout << "Stream started" << std::endl;
                    break;
                case STREAM_STATE_STOPPED:
                    std::cout << "Stream stopped" << std::endl;
                    break;
                case STREAM_STATE_PAUSED:
                    std::cout << "Stream paused" << std::endl;
                    break;
            }
            lastState = state;
        }
    }
    
    void AttemptReconnect() {
        int maxRetries = 5;
        int retryCount = 0;
        
        while (retryCount < maxRetries && !m_isConnected) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            
            // 嘗試重新連接
            if (TryReconnect()) {
                m_isConnected = true;
                std::cout << "Reconnection successful!" << std::endl;
                break;
            }
            
            retryCount++;
            std::cout << "Reconnection attempt " << retryCount 
                     << " failed" << std::endl;
        }
        
        if (!m_isConnected) {
            std::cerr << "Failed to reconnect after " 
                     << maxRetries << " attempts" << std::endl;
        }
    }
    
    bool TryReconnect() {
        // 實現重連邏輯
        // 這裡需要保存原始的設備信息以便重連
        return false;  // 簡化示例
    }
    
public:
    ~ConnectionManager() {
        StopMonitoring();
        if (m_handle != 0) {
            dvpClose(m_handle);
        }
    }
};
```

### 錯誤恢復機制

```cpp
class ErrorRecovery {
public:
    static bool RecoverFromError(dvpHandle handle, dvpStatus error) {
        switch (error) {
            case DVP_STATUS_FRAME_LOST:
                return HandleFrameLost(handle);
                
            case DVP_STATUS_TIMEOUT:
                return HandleTimeout(handle);
                
            case DVP_STATUS_DEVICE_LOST:
                return HandleDeviceLost(handle);
                
            case DVP_STATUS_BUFFER_OVERFLOW:
                return HandleBufferOverflow(handle);
                
            default:
                return false;
        }
    }
    
private:
    static bool HandleFrameLost(dvpHandle handle) {
        std::cout << "Frame lost, clearing buffer..." << std::endl;
        
        // 清除緩衝區
        dvpClearFrameBuffer(handle);
        
        // 重新開始串流
        dvpStop(handle);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        return dvpStart(handle) == DVP_STATUS_OK;
    }
    
    static bool HandleTimeout(dvpHandle handle) {
        std::cout << "Timeout occurred, checking device..." << std::endl;
        
        // 檢查設備狀態
        if (dvpIsValid(handle) != DVP_STATUS_OK) {
            return false;  // 設備已斷開
        }
        
        // 增加超時時間
        dvpSetTimeout(handle, 5000);
        return true;
    }
    
    static bool HandleDeviceLost(dvpHandle handle) {
        std::cout << "Device lost, marking for reconnection..." << std::endl;
        
        // 關閉當前句柄
        dvpClose(handle);
        
        // 標記需要重連
        return false;  // 需要完整的重連流程
    }
    
    static bool HandleBufferOverflow(dvpHandle handle) {
        std::cout << "Buffer overflow, adjusting buffer size..." << std::endl;
        
        // 增加緩衝區大小
        dvpSetFrameBufferSize(handle, 10);
        
        // 清除現有緩衝區
        dvpClearFrameBuffer(handle);
        
        return true;
    }
};
```

---

## 資源釋放流程

### RAII模式實現

```cpp
class CameraRAII {
private:
    dvpHandle m_handle;
    bool m_ownsHandle;
    
public:
    explicit CameraRAII(dvpHandle handle = 0) 
        : m_handle(handle), m_ownsHandle(handle != 0) {}
    
    CameraRAII(CameraRAII&& other) noexcept 
        : m_handle(other.m_handle), m_ownsHandle(other.m_ownsHandle) {
        other.m_handle = 0;
        other.m_ownsHandle = false;
    }
    
    CameraRAII& operator=(CameraRAII&& other) noexcept {
        if (this != &other) {
            Release();
            m_handle = other.m_handle;
            m_ownsHandle = other.m_ownsHandle;
            other.m_handle = 0;
            other.m_ownsHandle = false;
        }
        return *this;
    }
    
    // 禁止拷貝
    CameraRAII(const CameraRAII&) = delete;
    CameraRAII& operator=(const CameraRAII&) = delete;
    
    void Release() {
        if (m_ownsHandle && m_handle != 0) {
            // 停止串流
            dvpStreamState state;
            if (dvpGetStreamState(m_handle, &state) == DVP_STATUS_OK) {
                if (state == STREAM_STATE_STARTED) {
                    dvpStop(m_handle);
                }
            }
            
            // 關閉設備
            dvpClose(m_handle);
            m_handle = 0;
            m_ownsHandle = false;
        }
    }
    
    dvpHandle Get() const { return m_handle; }
    
    dvpHandle Release() {
        dvpHandle temp = m_handle;
        m_handle = 0;
        m_ownsHandle = false;
        return temp;
    }
    
    ~CameraRAII() {
        Release();
    }
};
```

### 完整的資源清理流程

```cpp
class ResourceCleanup {
public:
    static void CleanupCamera(dvpHandle handle) {
        if (handle == 0) return;
        
        // Step 1: 停止觸發
        dvpSetTriggerMode(handle, TRIGGER_MODE_OFF);
        
        // Step 2: 停止串流
        dvpStreamState state;
        if (dvpGetStreamState(handle, &state) == DVP_STATUS_OK) {
            if (state == STREAM_STATE_STARTED) {
                dvpStop(handle);
                std::cout << "Stream stopped" << std::endl;
            }
        }
        
        // Step 3: 清理緩衝區
        dvpClearFrameBuffer(handle);
        std::cout << "Frame buffer cleared" << std::endl;
        
        // Step 4: 釋放回調
        dvpSetFrameCallback(handle, nullptr, nullptr);
        
        // Step 5: 關閉設備
        dvpStatus status = dvpClose(handle);
        if (status == DVP_STATUS_OK) {
            std::cout << "Device closed successfully" << std::endl;
        } else {
            std::cerr << "Failed to close device: " << status << std::endl;
        }
    }
    
    static void CleanupMultipleCameras(const std::vector<dvpHandle>& handles) {
        // 並行清理多個相機
        std::vector<std::thread> cleanupThreads;
        
        for (auto handle : handles) {
            cleanupThreads.emplace_back([handle]() {
                CleanupCamera(handle);
            });
        }
        
        // 等待所有清理完成
        for (auto& t : cleanupThreads) {
            t.join();
        }
        
        std::cout << "All cameras cleaned up" << std::endl;
    }
};
```

---

## 完整程式碼範例

### 工業AOI應用完整範例

```cpp
#include "DVPCamera.h"
#include <iostream>
#include <memory>
#include <vector>
#include <chrono>
#include <thread>
#include <atomic>

class IndustrialAOICamera {
private:
    // 相機管理
    dvpHandle m_handle;
    dvpCameraInfo m_info;
    bool m_isInitialized;
    
    // 狀態管理
    std::atomic<bool> m_isStreaming;
    std::atomic<bool> m_shouldStop;
    std::thread m_captureThread;
    
    // 影像處理回調
    std::function<void(const dvpFrame&)> m_frameCallback;
    
    // 統計信息
    std::atomic<uint64_t> m_frameCount;
    std::atomic<uint64_t> m_errorCount;
    
public:
    IndustrialAOICamera() 
        : m_handle(0)
        , m_isInitialized(false)
        , m_isStreaming(false)
        , m_shouldStop(false)
        , m_frameCount(0)
        , m_errorCount(0) {}
    
    // 初始化相機
    bool Initialize(const std::string& serialNumber = "") {
        try {
            // Step 1: 刷新並枚舉設備
            if (!RefreshAndEnumerate()) {
                return false;
            }
            
            // Step 2: 選擇設備
            if (!SelectDevice(serialNumber)) {
                return false;
            }
            
            // Step 3: 打開設備
            if (!OpenDevice()) {
                return false;
            }
            
            // Step 4: 配置相機參數
            if (!ConfigureCamera()) {
                return false;
            }
            
            m_isInitialized = true;
            std::cout << "Camera initialized successfully" << std::endl;
            return true;
            
        } catch (const std::exception& e) {
            std::cerr << "Initialization failed: " << e.what() << std::endl;
            return false;
        }
    }
    
    // 開始採集
    bool StartCapture(std::function<void(const dvpFrame&)> callback) {
        if (!m_isInitialized) {
            std::cerr << "Camera not initialized" << std::endl;
            return false;
        }
        
        if (m_isStreaming) {
            std::cout << "Already streaming" << std::endl;
            return true;
        }
        
        m_frameCallback = callback;
        
        // 啟動串流
        dvpStatus status = dvpStart(m_handle);
        if (status != DVP_STATUS_OK) {
            std::cerr << "Failed to start stream: " << status << std::endl;
            return false;
        }
        
        m_isStreaming = true;
        m_shouldStop = false;
        
        // 啟動採集執行緒
        m_captureThread = std::thread([this]() {
            CaptureLoop();
        });
        
        std::cout << "Capture started" << std::endl;
        return true;
    }
    
    // 停止採集
    void StopCapture() {
        if (!m_isStreaming) {
            return;
        }
        
        m_shouldStop = true;
        
        if (m_captureThread.joinable()) {
            m_captureThread.join();
        }
        
        dvpStop(m_handle);
        m_isStreaming = false;
        
        std::cout << "Capture stopped" << std::endl;
        PrintStatistics();
    }
    
    // 軟體觸發
    bool SoftwareTrigger() {
        if (!m_isInitialized) {
            return false;
        }
        
        return dvpSoftwareTrigger(m_handle) == DVP_STATUS_OK;
    }
    
    // 獲取相機信息
    const dvpCameraInfo& GetCameraInfo() const {
        return m_info;
    }
    
    // 獲取統計信息
    void PrintStatistics() const {
        std::cout << "=== Camera Statistics ===" << std::endl;
        std::cout << "Total frames: " << m_frameCount << std::endl;
        std::cout << "Error count: " << m_errorCount << std::endl;
        if (m_frameCount > 0) {
            double errorRate = (double)m_errorCount / m_frameCount * 100;
            std::cout << "Error rate: " << errorRate << "%" << std::endl;
        }
        std::cout << "========================" << std::endl;
    }
    
private:
    bool RefreshAndEnumerate() {
        // 刷新設備列表
        dvpStatus status = dvpRefresh();
        if (status != DVP_STATUS_OK) {
            std::cerr << "Failed to refresh devices" << std::endl;
            return false;
        }
        
        // 獲取設備數量
        dvpUint32 count = 0;
        status = dvpEnum(&count, nullptr);
        if (status != DVP_STATUS_OK || count == 0) {
            std::cerr << "No devices found" << std::endl;
            return false;
        }
        
        std::cout << "Found " << count << " device(s)" << std::endl;
        return true;
    }
    
    bool SelectDevice(const std::string& serialNumber) {
        dvpUint32 count = 0;
        dvpEnum(&count, nullptr);
        
        std::vector<dvpCameraInfo> devices(count);
        dvpEnum(&count, devices.data());
        
        // 如果指定序列號，查找特定設備
        if (!serialNumber.empty()) {
            for (const auto& device : devices) {
                if (std::string(device.SerialNumber) == serialNumber) {
                    m_info = device;
                    return true;
                }
            }
            std::cerr << "Device with serial number " << serialNumber 
                     << " not found" << std::endl;
            return false;
        }
        
        // 否則選擇第一個設備
        m_info = devices[0];
        return true;
    }
    
    bool OpenDevice() {
        dvpStatus status = dvpOpenByName(m_info.FriendlyName, 
                                         OPEN_NORMAL, 
                                         &m_handle);
        if (status != DVP_STATUS_OK) {
            std::cerr << "Failed to open device: " << status << std::endl;
            return false;
        }
        
        std::cout << "Opened device: " << m_info.FriendlyName << std::endl;
        return true;
    }
    
    bool ConfigureCamera() {
        // 設置觸發模式
        dvpSetTriggerMode(m_handle, TRIGGER_MODE_OFF);  // 連續模式
        
        // 設置曝光時間
        dvpSetExposureTime(m_handle, 10000.0);  // 10ms
        
        // 設置增益
        dvpSetAnalogGain(m_handle, 1.0);
        
        // 設置影像格式
        dvpSetImageFormat(m_handle, FORMAT_MONO);
        
        // 設置ROI（全畫面）
        dvpRegion roi;
        dvpGetRoi(m_handle, &roi);
        std::cout << "ROI: " << roi.W << "x" << roi.H 
                  << " @ (" << roi.X << "," << roi.Y << ")" << std::endl;
        
        // 設置緩衝區
        dvpSetFrameBufferSize(m_handle, 5);
        
        return true;
    }
    
    void CaptureLoop() {
        dvpFrame frame;
        const int timeout = 1000;  // 1秒超時
        
        while (!m_shouldStop) {
            dvpStatus status = dvpGetFrame(m_handle, &frame, timeout);
            
            if (status == DVP_STATUS_OK) {
                m_frameCount++;
                
                // 調用回調函數處理影像
                if (m_frameCallback) {
                    m_frameCallback(frame);
                }
                
            } else if (status == DVP_STATUS_TIMEOUT) {
                // 超時是正常的，繼續等待
                continue;
                
            } else {
                m_errorCount++;
                std::cerr << "Frame capture error: " << status << std::endl;
                
                // 嘗試錯誤恢復
                if (!RecoverFromError(status)) {
                    std::cerr << "Unable to recover, stopping capture" << std::endl;
                    break;
                }
            }
        }
    }
    
    bool RecoverFromError(dvpStatus error) {
        switch (error) {
            case DVP_STATUS_FRAME_LOST:
                // 清理緩衝區並繼續
                dvpClearFrameBuffer(m_handle);
                return true;
                
            case DVP_STATUS_DEVICE_LOST:
                // 設備斷開，無法恢復
                return false;
                
            default:
                // 嘗試重啟串流
                dvpStop(m_handle);
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                return dvpStart(m_handle) == DVP_STATUS_OK;
        }
    }
    
public:
    ~IndustrialAOICamera() {
        if (m_isStreaming) {
            StopCapture();
        }
        
        if (m_handle != 0) {
            dvpClose(m_handle);
            std::cout << "Camera closed" << std::endl;
        }
    }
};

// 使用範例
int main() {
    IndustrialAOICamera camera;
    
    // 初始化相機
    if (!camera.Initialize()) {
        std::cerr << "Failed to initialize camera" << std::endl;
        return -1;
    }
    
    // 顯示相機信息
    auto info = camera.GetCameraInfo();
    std::cout << "Using camera: " << info.FriendlyName << std::endl;
    std::cout << "Serial: " << info.SerialNumber << std::endl;
    
    // 定義影像處理回調
    auto frameProcessor = [](const dvpFrame& frame) {
        static int frameIndex = 0;
        if (frameIndex++ % 30 == 0) {  // 每30幀輸出一次
            std::cout << "Processing frame - Size: " 
                     << frame.iWidth << "x" << frame.iHeight
                     << ", Format: " << frame.format << std::endl;
        }
        
        // 在這裡添加AOI檢測邏輯
        // ProcessAOI(frame);
    };
    
    // 開始採集
    if (!camera.StartCapture(frameProcessor)) {
        std::cerr << "Failed to start capture" << std::endl;
        return -1;
    }
    
    // 運行10秒
    std::cout << "Capturing for 10 seconds..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(10));
    
    // 停止採集
    camera.StopCapture();
    
    return 0;
}
```

---

## Qt整合建議

### Qt整合架構

```cpp
// CameraWidget.h
#ifndef CAMERA_WIDGET_H
#define CAMERA_WIDGET_H

#include <QWidget>
#include <QImage>
#include <QTimer>
#include <memory>
#include "DVPCamera.h"

class QLabel;
class QPushButton;
class QComboBox;

class CameraWidget : public QWidget {
    Q_OBJECT
    
public:
    explicit CameraWidget(QWidget* parent = nullptr);
    ~CameraWidget();
    
signals:
    void frameReceived(const QImage& image);
    void errorOccurred(const QString& error);
    
public slots:
    void refreshDevices();
    void connectCamera();
    void disconnectCamera();
    void startCapture();
    void stopCapture();
    
private slots:
    void updateDeviceList();
    void processFrame();
    void handleError(const QString& error);
    
private:
    // UI元件
    QComboBox* m_deviceCombo;
    QPushButton* m_connectBtn;
    QPushButton* m_startBtn;
    QLabel* m_imageLabel;
    
    // 相機管理
    dvpHandle m_handle;
    bool m_isConnected;
    bool m_isCapturing;
    
    // 定時器
    QTimer* m_captureTimer;
    
    // 輔助方法
    QImage convertFrameToQImage(const dvpFrame& frame);
    void updateUI();
};

#endif // CAMERA_WIDGET_H
```

```cpp
// CameraWidget.cpp
#include "CameraWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QMessageBox>
#include <QPixmap>

CameraWidget::CameraWidget(QWidget* parent)
    : QWidget(parent)
    , m_handle(0)
    , m_isConnected(false)
    , m_isCapturing(false) {
    
    setupUI();
    setupConnections();
    
    // 初始掃描設備
    refreshDevices();
}

void CameraWidget::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    
    // 控制面板
    auto* controlLayout = new QHBoxLayout();
    
    m_deviceCombo = new QComboBox();
    m_connectBtn = new QPushButton("Connect");
    m_startBtn = new QPushButton("Start");
    m_startBtn->setEnabled(false);
    
    controlLayout->addWidget(new QLabel("Device:"));
    controlLayout->addWidget(m_deviceCombo);
    controlLayout->addWidget(m_connectBtn);
    controlLayout->addWidget(m_startBtn);
    controlLayout->addStretch();
    
    // 影像顯示區
    m_imageLabel = new QLabel();
    m_imageLabel->setMinimumSize(640, 480);
    m_imageLabel->setStyleSheet("QLabel { background-color: black; }");
    m_imageLabel->setScaledContents(true);
    
    mainLayout->addLayout(controlLayout);
    mainLayout->addWidget(m_imageLabel);
    
    // 設置定時器
    m_captureTimer = new QTimer(this);
    m_captureTimer->setInterval(33);  // 30 FPS
}

void CameraWidget::setupConnections() {
    connect(m_connectBtn, &QPushButton::clicked, [this]() {
        if (m_isConnected) {
            disconnectCamera();
        } else {
            connectCamera();
        }
    });
    
    connect(m_startBtn, &QPushButton::clicked, [this]() {
        if (m_isCapturing) {
            stopCapture();
        } else {
            startCapture();
        }
    });
    
    connect(m_captureTimer, &QTimer::timeout, 
            this, &CameraWidget::processFrame);
}

void CameraWidget::refreshDevices() {
    m_deviceCombo->clear();
    
    // 刷新設備列表
    dvpRefresh();
    
    // 枚舉設備
    dvpUint32 count = 0;
    dvpEnum(&count, nullptr);
    
    if (count == 0) {
        m_deviceCombo->addItem("No devices found");
        return;
    }
    
    std::vector<dvpCameraInfo> devices(count);
    dvpEnum(&count, devices.data());
    
    for (const auto& device : devices) {
        QString displayName = QString("%1 (SN: %2)")
            .arg(device.FriendlyName)
            .arg(device.SerialNumber);
        m_deviceCombo->addItem(displayName);
        m_deviceCombo->setItemData(m_deviceCombo->count() - 1,
                                   device.FriendlyName,
                                   Qt::UserRole);
    }
}

void CameraWidget::connectCamera() {
    if (m_deviceCombo->count() == 0) {
        QMessageBox::warning(this, "Warning", "No devices available");
        return;
    }
    
    QString deviceName = m_deviceCombo->currentData(Qt::UserRole).toString();
    
    dvpStatus status = dvpOpenByName(deviceName.toStdString().c_str(),
                                     OPEN_NORMAL,
                                     &m_handle);
    
    if (status != DVP_STATUS_OK) {
        QMessageBox::critical(this, "Error", 
                             QString("Failed to open device: %1").arg(status));
        return;
    }
    
    m_isConnected = true;
    m_connectBtn->setText("Disconnect");
    m_startBtn->setEnabled(true);
    
    emit frameReceived(QImage());  // 清空顯示
}

void CameraWidget::disconnectCamera() {
    if (m_isCapturing) {
        stopCapture();
    }
    
    if (m_handle != 0) {
        dvpClose(m_handle);
        m_handle = 0;
    }
    
    m_isConnected = false;
    m_connectBtn->setText("Connect");
    m_startBtn->setEnabled(false);
}

void CameraWidget::startCapture() {
    if (!m_isConnected) return;
    
    dvpStatus status = dvpStart(m_handle);
    if (status != DVP_STATUS_OK) {
        QMessageBox::critical(this, "Error",
                             QString("Failed to start capture: %1").arg(status));
        return;
    }
    
    m_isCapturing = true;
    m_startBtn->setText("Stop");
    m_captureTimer->start();
}

void CameraWidget::stopCapture() {
    m_captureTimer->stop();
    
    if (m_handle != 0) {
        dvpStop(m_handle);
    }
    
    m_isCapturing = false;
    m_startBtn->setText("Start");
}

void CameraWidget::processFrame() {
    if (!m_isCapturing) return;
    
    dvpFrame frame;
    dvpStatus status = dvpGetFrame(m_handle, &frame, 100);
    
    if (status == DVP_STATUS_OK) {
        QImage image = convertFrameToQImage(frame);
        m_imageLabel->setPixmap(QPixmap::fromImage(image));
        emit frameReceived(image);
    }
}

QImage CameraWidget::convertFrameToQImage(const dvpFrame& frame) {
    QImage::Format format;
    
    switch (frame.format) {
        case FORMAT_MONO:
            format = QImage::Format_Grayscale8;
            break;
        case FORMAT_BGR24:
            format = QImage::Format_RGB888;
            break;
        case FORMAT_RGB24:
            format = QImage::Format_RGB888;
            break;
        default:
            return QImage();
    }
    
    QImage image(frame.pData, frame.iWidth, frame.iHeight, 
                 frame.iStride, format);
    
    if (frame.format == FORMAT_BGR24) {
        return image.rgbSwapped();
    }
    
    return image.copy();  // 深拷貝以避免資料問題
}

CameraWidget::~CameraWidget() {
    disconnectCamera();
}
```

### Qt執行緒管理建議

```cpp
// CameraThread.h
class CameraThread : public QThread {
    Q_OBJECT
    
public:
    explicit CameraThread(dvpHandle handle, QObject* parent = nullptr);
    
    void startCapture();
    void stopCapture();
    
signals:
    void frameReady(const QImage& image);
    void errorOccurred(const QString& error);
    
protected:
    void run() override;
    
private:
    dvpHandle m_handle;
    std::atomic<bool> m_shouldStop;
    QImage convertFrame(const dvpFrame& frame);
};

// CameraThread.cpp
void CameraThread::run() {
    dvpFrame frame;
    
    while (!m_shouldStop) {
        dvpStatus status = dvpGetFrame(m_handle, &frame, 1000);
        
        if (status == DVP_STATUS_OK) {
            QImage image = convertFrame(frame);
            emit frameReady(image);
        } else if (status != DVP_STATUS_TIMEOUT) {
            emit errorOccurred(QString("Capture error: %1").arg(status));
        }
    }
}
```

---

## 常見問題與解決方案

### 問題診斷與解決

| 問題 | 可能原因 | 解決方案 |
|------|----------|----------|
| 找不到設備 | 驅動未安裝 | 安裝Do3Think驅動程式 |
| | USB電源不足 | 使用外接電源或更換USB埠 |
| | 防火牆阻擋(GigE) | 配置防火牆規則 |
| 打開設備失敗 | 設備被占用 | 關閉其他使用該設備的程式 |
| | 權限不足 | 以管理員身份運行 |
| | 設備異常 | 重新插拔設備 |
| 影像採集失敗 | 緩衝區溢出 | 增加緩衝區大小 |
| | 超時設置過短 | 調整超時參數 |
| | 觸發模式錯誤 | 檢查觸發設置 |
| 影像異常 | 參數設置錯誤 | 檢查曝光、增益等參數 |
| | 資料傳輸錯誤 | 更換USB線纜 |
| | 格式不匹配 | 確認影像格式設置 |

### 效能優化建議

```cpp
class PerformanceOptimizer {
public:
    // 優化緩衝區設置
    static void OptimizeBuffers(dvpHandle handle) {
        // 設置較大的緩衝區以減少丟幀
        dvpSetFrameBufferSize(handle, 10);
        
        // 預分配記憶體
        dvpSetPreAllocatedMemory(handle, true);
    }
    
    // 優化傳輸設置
    static void OptimizeTransfer(dvpHandle handle) {
        // 啟用Jumbo Packet (GigE)
        dvpSetPacketSize(handle, 9000);
        
        // 設置傳輸延遲
        dvpSetPacketDelay(handle, 100);
    }
    
    // 優化CPU使用
    static void OptimizeCPU(dvpHandle handle) {
        // 設置處理器親和性
        dvpSetProcessorAffinity(handle, 0x01);
        
        // 降低串流優先級
        dvpSetStreamPriority(handle, PRIORITY_NORMAL);
    }
};
```

### 調試技巧

```cpp
class DebugHelper {
public:
    static void EnableDebugOutput(dvpHandle handle) {
        // 啟用調試日誌
        dvpEnableDebugLog(true);
        
        // 設置日誌級別
        dvpSetLogLevel(LOG_LEVEL_VERBOSE);
        
        // 設置日誌文件
        dvpSetLogFile("camera_debug.log");
    }
    
    static void PrintDeviceCapabilities(dvpHandle handle) {
        dvpCapability cap;
        dvpGetCapability(handle, &cap);
        
        std::cout << "=== Device Capabilities ===" << std::endl;
        std::cout << "Max Resolution: " << cap.maxWidth 
                  << "x" << cap.maxHeight << std::endl;
        std::cout << "Max Frame Rate: " << cap.maxFrameRate << std::endl;
        std::cout << "Supported Formats: " << cap.supportedFormats << std::endl;
        std::cout << "Trigger Modes: " << cap.triggerModes << std::endl;
        std::cout << "==========================" << std::endl;
    }
    
    static void CheckSystemRequirements() {
        // 檢查USB控制器
        if (!CheckUSBController()) {
            std::cerr << "Warning: USB 3.0 controller not detected" << std::endl;
        }
        
        // 檢查網路配置 (GigE)
        if (!CheckNetworkConfiguration()) {
            std::cerr << "Warning: Network not optimized for GigE Vision" << std::endl;
        }
        
        // 檢查系統資源
        if (!CheckSystemResources()) {
            std::cerr << "Warning: Insufficient system resources" << std::endl;
        }
    }
    
private:
    static bool CheckUSBController() {
        // 實現USB控制器檢查邏輯
        return true;
    }
    
    static bool CheckNetworkConfiguration() {
        // 實現網路配置檢查邏輯
        return true;
    }
    
    static bool CheckSystemResources() {
        // 實現系統資源檢查邏輯
        return true;
    }
};
```

---

## 總結

本文檔詳細介紹了Do3ThinkCamera SDK的設備連線與初始化流程，包括：

1. **完整的初始化流程**：從SDK載入到設備配置
2. **設備管理策略**：單相機和多相機管理
3. **錯誤處理機制**：完善的錯誤恢復和資源釋放
4. **Qt整合方案**：UI整合和執行緒管理
5. **最佳實踐**：效能優化和調試技巧

遵循本文檔的指導，您可以快速開發穩定可靠的工業AOI應用程式。

### 重要提示

- 始終使用RAII模式管理資源
- 實現完善的錯誤處理機制
- 注意多執行緒同步問題
- 定期檢查連接狀態
- 優化緩衝區和傳輸設置

### 相關資源

- Do3ThinkCamera SDK文檔
- 範例程式碼庫
- 技術支援論壇
- API參考手冊

---

*文檔版本：1.0*  
*最後更新：2025*  
*適用SDK版本：2.x及以上*