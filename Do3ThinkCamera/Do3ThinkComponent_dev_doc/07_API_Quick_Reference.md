# Do3Think Camera SDK API 快速參考手冊

## 目錄
1. [API分類索引](#1-api分類索引)
2. [常用API速查表](#2-常用api速查表)
3. [數據結構參考](#3-數據結構參考)
4. [參數名稱常量](#4-參數名稱常量)
5. [錯誤碼對照表](#5-錯誤碼對照表)
6. [代碼片段庫](#6-代碼片段庫)
7. [常見使用模式](#7-常見使用模式)
8. [版本兼容性說明](#8-版本兼容性說明)

---

## 1. API分類索引

### 1.1 基礎功能API

| 函數名稱 | 功能描述 | 頁面參考 |
|---------|---------|----------|
| `dvpGetCameraCount` | 獲取相機數量 | [詳細](#dvpGetCameraCount) |
| `dvpRefresh` | 刷新設備列表 | [詳細](#dvpRefresh) |
| `dvpEnum` | 枚舉相機設備 | [詳細](#dvpEnum) |
| `dvpGetCameraInfo` | 獲取相機信息 | [詳細](#dvpGetCameraInfo) |
| `dvpOpenByUserId` | 通過用戶ID打開相機 | [詳細](#dvpOpenByUserId) |
| `dvpOpen` | 打開相機 | [詳細](#dvpOpen) |
| `dvpClose` | 關閉相機 | [詳細](#dvpClose) |
| `dvpIsValid` | 檢查句柄有效性 | [詳細](#dvpIsValid) |

### 1.2 參數控制API

| 函數名稱 | 功能描述 | 參數類型 |
|---------|---------|----------|
| `dvpGetExposureRange` | 獲取曝光範圍 | 浮點型 |
| `dvpGetExposure` | 獲取當前曝光值 | 浮點型 |
| `dvpSetExposure` | 設置曝光時間 | 浮點型 |
| `dvpGetGainRange` | 獲取增益範圍 | 浮點型 |
| `dvpGetGain` | 獲取當前增益值 | 浮點型 |
| `dvpSetGain` | 設置增益值 | 浮點型 |
| `dvpGetRoi` | 獲取ROI區域 | 結構體 |
| `dvpSetRoi` | 設置ROI區域 | 結構體 |
| `dvpGetAwbRoi` | 獲取白平衡ROI | 結構體 |
| `dvpSetAwbRoi` | 設置白平衡ROI | 結構體 |

### 1.3 圖像獲取API

| 函數名稱 | 功能描述 | 返回類型 |
|---------|---------|----------|
| `dvpStart` | 開始視頻流 | dvpStatus |
| `dvpStop` | 停止視頻流 | dvpStatus |
| `dvpGetFrame` | 獲取圖像幀 | dvpStatus |
| `dvpGetFrameBuffer` | 獲取幀緩衝區 | dvpStatus |
| `dvpHold` | 持有當前幀 | dvpStatus |
| `dvpGetTriggerState` | 獲取觸發狀態 | dvpStatus |
| `dvpTriggerFire` | 軟件觸發 | dvpStatus |

### 1.4 觸發控制API

| 函數名稱 | 功能描述 | 觸發類型 |
|---------|---------|----------|
| `dvpGetTriggerSource` | 獲取觸發源 | 枚舉 |
| `dvpSetTriggerSource` | 設置觸發源 | 枚舉 |
| `dvpGetTriggerMode` | 獲取觸發模式 | 布爾 |
| `dvpSetTriggerMode` | 設置觸發模式 | 布爾 |
| `dvpGetSoftTriggerLoopState` | 獲取軟觸發循環狀態 | 布爾 |
| `dvpSetSoftTriggerLoop` | 設置軟觸發循環 | 布爾 |
| `dvpGetTriggerLineMode` | 獲取觸發線模式 | 枚舉 |
| `dvpSetTriggerLineMode` | 設置觸發線模式 | 枚舉 |

### 1.5 事件回調API

| 函數名稱 | 功能描述 | 回調類型 |
|---------|---------|----------|
| `dvpSetEventCallback` | 設置事件回調 | 函數指針 |
| `dvpSetFrameCallback` | 設置幀回調 | 函數指針 |
| `dvpSetSourceBufferCallback` | 設置源緩衝區回調 | 函數指針 |
| `dvpRegisterStreamCallback` | 註冊流回調 | 函數指針 |

### 1.6 錯誤處理API

| 函數名稱 | 功能描述 | 返回值 |
|---------|---------|----------|
| `dvpGetLastError` | 獲取最後錯誤 | dvpStatus |
| `dvpGetErrorString` | 獲取錯誤字符串 | const char* |
| `dvpResetDevice` | 重置設備 | dvpStatus |
| `dvpRestoreDefault` | 恢復默認設置 | dvpStatus |

### 1.7 進階功能API

| 函數名稱 | 功能描述 | 應用場景 |
|---------|---------|----------|
| `dvpGetFlatFieldInfo` | 獲取平場校正信息 | 圖像校正 |
| `dvpSetFlatFieldEnabled` | 啟用平場校正 | 圖像校正 |
| `dvpGetDefectFixInfo` | 獲取壞點修復信息 | 圖像優化 |
| `dvpSetDefectFixEnabled` | 啟用壞點修復 | 圖像優化 |
| `dvpSaveConfig` | 保存配置 | 參數管理 |
| `dvpLoadConfig` | 加載配置 | 參數管理 |

---

## 2. 常用API速查表

### <a id="dvpGetCameraCount"></a>dvpGetCameraCount
```c
dvpStatus dvpGetCameraCount(dvpUint32 *pCount);
```
**功能**: 獲取當前連接的相機數量  
**參數**:
- `pCount`: [輸出] 相機數量指針

**返回值**: `DVP_STATUS_OK` 成功，其他為錯誤碼

**使用範例**:
```c
dvpUint32 count;
dvpStatus status = dvpGetCameraCount(&count);
if (status == DVP_STATUS_OK) {
    printf("找到 %d 台相機\n", count);
}
```

### <a id="dvpOpenByUserId"></a>dvpOpenByUserId
```c
dvpStatus dvpOpenByUserId(const char *UserId, 
                          dvpOpenMode OpenMode, 
                          dvpHandle *pHandle);
```
**功能**: 通過用戶ID打開指定相機  
**參數**:
- `UserId`: 相機用戶ID字符串
- `OpenMode`: 打開模式（ONLINE/OFFLINE）
- `pHandle`: [輸出] 相機句柄指針

**返回值**: `DVP_STATUS_OK` 成功

**使用範例**:
```c
dvpHandle camera;
dvpStatus status = dvpOpenByUserId("Camera_001", 
                                   OPEN_ONLINE, 
                                   &camera);
```

### <a id="dvpGetFrame"></a>dvpGetFrame
```c
dvpStatus dvpGetFrame(dvpHandle handle, 
                     dvpFrame *pFrame, 
                     dvpUint32 *pFrameCount, 
                     dvpUint32 timeout);
```
**功能**: 獲取一幀圖像數據  
**參數**:
- `handle`: 相機句柄
- `pFrame`: [輸出] 幀數據結構指針
- `pFrameCount`: [輸出] 幀計數指針（可選）
- `timeout`: 超時時間（毫秒）

**返回值**: `DVP_STATUS_OK` 成功獲取

**使用範例**:
```c
dvpFrame frame;
dvpUint32 frameCount;
dvpStatus status = dvpGetFrame(camera, &frame, 
                               &frameCount, 3000);
if (status == DVP_STATUS_OK) {
    // 處理圖像數據
    ProcessImage(frame.pData, frame.iWidth, frame.iHeight);
}
```

### dvpSetExposure
```c
dvpStatus dvpSetExposure(dvpHandle handle, float exposure);
```
**功能**: 設置相機曝光時間  
**參數**:
- `handle`: 相機句柄
- `exposure`: 曝光時間（微秒）

**返回值**: `DVP_STATUS_OK` 成功

**注意事項**:
- 曝光值必須在有效範圍內
- 使用`dvpGetExposureRange`獲取範圍
- 自動曝光啟用時此設置無效

### dvpSetGain
```c
dvpStatus dvpSetGain(dvpHandle handle, float gain);
```
**功能**: 設置相機增益值  
**參數**:
- `handle`: 相機句柄
- `gain`: 增益值（分貝）

**返回值**: `DVP_STATUS_OK` 成功

**使用範例**:
```c
float minGain, maxGain, gain = 10.0f;
dvpGetGainRange(camera, &minGain, &maxGain);
if (gain >= minGain && gain <= maxGain) {
    dvpSetGain(camera, gain);
}
```

---

## 3. 數據結構參考

### dvpHandle
```c
typedef void* dvpHandle;
```
相機句柄，用於標識打開的相機實例。

### dvpCameraInfo
```c
typedef struct dvpCameraInfo {
    char Vendor[64];        // 廠商名稱
    char Model[64];         // 型號
    char Family[64];        // 產品系列
    char LinkName[64];      // 連接名稱
    char SensorInfo[64];    // 傳感器信息
    char HardwareVersion[64]; // 硬件版本
    char FirmwareVersion[64]; // 固件版本
    char KernelVersion[64];   // 內核版本
    char DscpVersion[64];      // DSCP版本
    char FriendlyName[64];     // 友好名稱
    char PortInfo[64];         // 端口信息
    char SerialNumber[64];     // 序列號
    char CameraId[64];         // 相機ID
    char UserId[64];           // 用戶ID
    dvpUint32 Reserved[8];     // 保留
} dvpCameraInfo;
```

### dvpFrame
```c
typedef struct dvpFrame {
    dvpStreamFormat format;  // 圖像格式
    dvpBits bits;           // 位深度
    dvpInt32 iWidth;        // 圖像寬度
    dvpInt32 iHeight;       // 圖像高度
    dvpInt32 iStride;       // 行字節數
    dvpUint64 uFrameID;     // 幀ID
    dvpUint64 uTimestamp;   // 時間戳
    dvpUint8* pData;        // 圖像數據指針
    dvpUint32 uDataSize;    // 數據大小
    dvpUint32 Reserved[8];  // 保留
} dvpFrame;
```

### dvpRegion
```c
typedef struct dvpRegion {
    dvpInt32 X;      // 起始X坐標
    dvpInt32 Y;      // 起始Y坐標
    dvpInt32 W;      // 寬度
    dvpInt32 H;      // 高度
} dvpRegion;
```

### dvpStatus
```c
typedef enum dvpStatus {
    DVP_STATUS_OK = 0,              // 操作成功
    DVP_STATUS_FAILED = -1,         // 操作失敗
    DVP_STATUS_PARAMETER_ERROR = -2, // 參數錯誤
    DVP_STATUS_NOT_SUPPORTED = -3,   // 不支持
    DVP_STATUS_NOT_INITIALIZED = -4, // 未初始化
    DVP_STATUS_INVALID_HANDLE = -5,  // 無效句柄
    DVP_STATUS_TIMEOUT = -6,         // 超時
    DVP_STATUS_BUSY = -7,            // 設備忙
    DVP_STATUS_NO_MEMORY = -8,       // 內存不足
    DVP_STATUS_DEVICE_LOST = -9,     // 設備丟失
    // ... 更多錯誤碼
} dvpStatus;
```

### dvpStreamState
```c
typedef enum dvpStreamState {
    STATE_STOPED = 0,    // 已停止
    STATE_STARTED = 1,   // 已啟動
    STATE_PAUSED = 2     // 已暫停
} dvpStreamState;
```

### dvpTriggerSource
```c
typedef enum dvpTriggerSource {
    TRIGGER_SOURCE_SOFTWARE = 0,  // 軟件觸發
    TRIGGER_SOURCE_LINE0 = 1,     // 硬件線路0
    TRIGGER_SOURCE_LINE1 = 2,     // 硬件線路1
    TRIGGER_SOURCE_LINE2 = 3,     // 硬件線路2
    TRIGGER_SOURCE_LINE3 = 4      // 硬件線路3
} dvpTriggerSource;
```

---

## 4. 參數名稱常量

### 常用參數常量定義
```c
// 曝光控制
#define V_EXPOSURE_TIME_F        "ExposureTime"      // 曝光時間（浮點）
#define V_AUTO_EXPOSURE_B        "AutoExposure"      // 自動曝光（布爾）
#define V_AE_TARGET_I            "AeTarget"          // 自動曝光目標（整數）
#define V_AE_MODE_E              "AeMode"            // 自動曝光模式（枚舉）

// 增益控制
#define V_GAIN_F                 "Gain"              // 增益值（浮點）
#define V_AUTO_GAIN_B            "AutoGain"          // 自動增益（布爾）
#define V_GAIN_RANGE_F           "GainRange"         // 增益範圍（浮點）

// 白平衡控制
#define V_WHITE_BALANCE_B        "WhiteBalance"      // 白平衡開關（布爾）
#define V_AWB_MODE_E             "AwbMode"           // 自動白平衡模式（枚舉）
#define V_R_GAIN_F               "RGain"             // 紅色增益（浮點）
#define V_G_GAIN_F               "GGain"             // 綠色增益（浮點）
#define V_B_GAIN_F               "BGain"             // 藍色增益（浮點）

// 觸發控制
#define V_TRIGGER_MODE_B         "TriggerMode"       // 觸發模式（布爾）
#define V_TRIGGER_SOURCE_E       "TriggerSource"     // 觸發源（枚舉）
#define V_TRIGGER_DELAY_F        "TriggerDelay"      // 觸發延遲（浮點）
#define V_TRIGGER_JITTER_F       "TriggerJitter"     // 觸發抖動（浮點）
#define V_TRIGGER_EDGE_E         "TriggerEdge"       // 觸發邊沿（枚舉）

// 圖像格式
#define V_FORMAT_E               "Format"            // 圖像格式（枚舉）
#define V_BITS_E                 "Bits"              // 位深度（枚舉）
#define V_COLOR_CORRECTION_B     "ColorCorrection"   // 顏色校正（布爾）
#define V_GAMMA_F                "Gamma"             // Gamma值（浮點）

// ROI控制
#define V_ROI_X_I                "RoiX"              // ROI X坐標（整數）
#define V_ROI_Y_I                "RoiY"              // ROI Y坐標（整數）
#define V_ROI_WIDTH_I            "RoiWidth"          // ROI寬度（整數）
#define V_ROI_HEIGHT_I           "RoiHeight"         // ROI高度（整數）

// 幀率控制
#define V_FRAME_RATE_F           "FrameRate"         // 幀率（浮點）
#define V_FRAME_RATE_LIMIT_B     "FrameRateLimit"    // 幀率限制（布爾）

// 設備信息
#define V_DEVICE_NAME_S          "DeviceName"        // 設備名稱（字符串）
#define V_SERIAL_NUMBER_S        "SerialNumber"      // 序列號（字符串）
#define V_FIRMWARE_VERSION_S     "FirmwareVersion"   // 固件版本（字符串）

// 緩衝區控制
#define V_BUFFER_COUNT_I         "BufferCount"       // 緩衝區數量（整數）
#define V_BUFFER_MODE_E          "BufferMode"        // 緩衝區模式（枚舉）

// 圖像增強
#define V_SHARPNESS_F            "Sharpness"         // 銳度（浮點）
#define V_SATURATION_F           "Saturation"        // 飽和度（浮點）
#define V_BRIGHTNESS_F           "Brightness"        // 亮度（浮點）
#define V_CONTRAST_F             "Contrast"          // 對比度（浮點）
```

### 參數類型標識
- `_B`: Boolean (布爾型)
- `_I`: Integer (整數型)
- `_F`: Float (浮點型)
- `_S`: String (字符串型)
- `_E`: Enum (枚舉型)

---

## 5. 錯誤碼對照表

| 錯誤碼 | 名稱 | 說明 | 處理建議 |
|--------|------|------|----------|
| 0 | `DVP_STATUS_OK` | 操作成功 | 無需處理 |
| -1 | `DVP_STATUS_FAILED` | 通用錯誤 | 檢查操作條件 |
| -2 | `DVP_STATUS_PARAMETER_ERROR` | 參數錯誤 | 驗證輸入參數 |
| -3 | `DVP_STATUS_NOT_SUPPORTED` | 功能不支持 | 檢查設備能力 |
| -4 | `DVP_STATUS_NOT_INITIALIZED` | 未初始化 | 先調用初始化函數 |
| -5 | `DVP_STATUS_INVALID_HANDLE` | 無效句柄 | 檢查句柄有效性 |
| -6 | `DVP_STATUS_TIMEOUT` | 操作超時 | 增加超時時間或重試 |
| -7 | `DVP_STATUS_BUSY` | 設備忙 | 等待或停止當前操作 |
| -8 | `DVP_STATUS_NO_MEMORY` | 內存不足 | 釋放內存或減少緩衝區 |
| -9 | `DVP_STATUS_DEVICE_LOST` | 設備斷開 | 重新連接設備 |
| -10 | `DVP_STATUS_ACCESS_DENIED` | 訪問被拒 | 檢查權限或獨占訪問 |
| -11 | `DVP_STATUS_RESOURCE_LOCKED` | 資源鎖定 | 等待資源釋放 |
| -12 | `DVP_STATUS_BUFFER_EMPTY` | 緩衝區空 | 等待新數據 |
| -13 | `DVP_STATUS_BUFFER_OVERFLOW` | 緩衝區溢出 | 增加緩衝區大小 |
| -14 | `DVP_STATUS_INVALID_PARAMETER` | 無效參數值 | 檢查參數範圍 |
| -15 | `DVP_STATUS_FILE_ERROR` | 文件操作錯誤 | 檢查文件路徑和權限 |

### 錯誤處理示例
```c
void HandleError(dvpStatus status) {
    if (status != DVP_STATUS_OK) {
        const char* errorString = dvpGetErrorString(status);
        printf("錯誤: %s (代碼: %d)\n", errorString, status);
        
        switch(status) {
            case DVP_STATUS_TIMEOUT:
                // 重試操作
                break;
            case DVP_STATUS_DEVICE_LOST:
                // 嘗試重新連接
                break;
            case DVP_STATUS_PARAMETER_ERROR:
                // 驗證參數
                break;
            default:
                // 通用錯誤處理
                break;
        }
    }
}
```

---

## 6. 代碼片段庫

### 6.1 初始化相機
```c
// 基本初始化流程
dvpStatus InitializeCamera(dvpHandle* pCamera, const char* userId) {
    dvpStatus status;
    
    // 刷新設備列表
    status = dvpRefresh();
    if (status != DVP_STATUS_OK) return status;
    
    // 獲取相機數量
    dvpUint32 count;
    status = dvpGetCameraCount(&count);
    if (status != DVP_STATUS_OK) return status;
    
    if (count == 0) {
        printf("未找到相機設備\n");
        return DVP_STATUS_FAILED;
    }
    
    // 打開相機
    if (userId != NULL) {
        status = dvpOpenByUserId(userId, OPEN_ONLINE, pCamera);
    } else {
        dvpCameraInfo info;
        status = dvpEnum(0, &info);
        if (status == DVP_STATUS_OK) {
            status = dvpOpen(0, OPEN_ONLINE, pCamera);
        }
    }
    
    if (status == DVP_STATUS_OK) {
        // 設置默認參數
        dvpSetStreamState(*pCamera, STATE_STOPED);
        dvpSetTriggerMode(*pCamera, false);
        dvpSetExposure(*pCamera, 10000.0f);
        dvpSetGain(*pCamera, 1.0f);
    }
    
    return status;
}
```

### 6.2 設置參數
```c
// 參數設置助手函數
typedef struct CameraParams {
    float exposure;
    float gain;
    dvpRegion roi;
    bool autoExposure;
    bool autoGain;
} CameraParams;

dvpStatus SetCameraParameters(dvpHandle camera, const CameraParams* params) {
    dvpStatus status;
    
    // 設置曝光
    if (!params->autoExposure) {
        status = dvpSetExposure(camera, params->exposure);
        if (status != DVP_STATUS_OK) return status;
    }
    status = dvpSetAutoExposure(camera, params->autoExposure);
    if (status != DVP_STATUS_OK) return status;
    
    // 設置增益
    if (!params->autoGain) {
        status = dvpSetGain(camera, params->gain);
        if (status != DVP_STATUS_OK) return status;
    }
    status = dvpSetAutoGain(camera, params->autoGain);
    if (status != DVP_STATUS_OK) return status;
    
    // 設置ROI
    status = dvpSetRoi(camera, &params->roi);
    if (status != DVP_STATUS_OK) return status;
    
    return DVP_STATUS_OK;
}
```

### 6.3 獲取圖像
```c
// 連續採集模式
void ContinuousCapture(dvpHandle camera, int frameCount) {
    dvpStatus status;
    dvpFrame frame;
    dvpUint32 count;
    
    // 啟動視頻流
    status = dvpStart(camera);
    if (status != DVP_STATUS_OK) {
        printf("啟動視頻流失敗\n");
        return;
    }
    
    for (int i = 0; i < frameCount; i++) {
        status = dvpGetFrame(camera, &frame, &count, 3000);
        if (status == DVP_STATUS_OK) {
            printf("獲取第 %d 幀, 大小: %dx%d\n", 
                   i+1, frame.iWidth, frame.iHeight);
            
            // 處理圖像數據
            ProcessImageData(frame.pData, frame.uDataSize);
        } else {
            printf("獲取幀失敗: %d\n", status);
        }
    }
    
    // 停止視頻流
    dvpStop(camera);
}

// 單幀採集模式
dvpStatus SingleFrameCapture(dvpHandle camera, dvpFrame* pFrame) {
    dvpStatus status;
    
    // 設置觸發模式
    status = dvpSetTriggerMode(camera, true);
    if (status != DVP_STATUS_OK) return status;
    
    status = dvpSetTriggerSource(camera, TRIGGER_SOURCE_SOFTWARE);
    if (status != DVP_STATUS_OK) return status;
    
    // 啟動視頻流
    status = dvpStart(camera);
    if (status != DVP_STATUS_OK) return status;
    
    // 軟件觸發
    status = dvpTriggerFire(camera);
    if (status != DVP_STATUS_OK) {
        dvpStop(camera);
        return status;
    }
    
    // 獲取圖像
    dvpUint32 count;
    status = dvpGetFrame(camera, pFrame, &count, 5000);
    
    // 停止視頻流
    dvpStop(camera);
    
    return status;
}
```

### 6.4 錯誤處理
```c
// 綜合錯誤處理框架
typedef struct ErrorHandler {
    int maxRetries;
    int retryDelay;
    void (*onError)(dvpStatus);
    void (*onSuccess)(void);
} ErrorHandler;

dvpStatus ExecuteWithRetry(dvpHandle camera, 
                           dvpStatus (*operation)(dvpHandle),
                           ErrorHandler* handler) {
    dvpStatus status;
    int retries = 0;
    
    while (retries < handler->maxRetries) {
        status = operation(camera);
        
        if (status == DVP_STATUS_OK) {
            if (handler->onSuccess) {
                handler->onSuccess();
            }
            return status;
        }
        
        if (handler->onError) {
            handler->onError(status);
        }
        
        // 特定錯誤不重試
        if (status == DVP_STATUS_NOT_SUPPORTED ||
            status == DVP_STATUS_INVALID_HANDLE ||
            status == DVP_STATUS_PARAMETER_ERROR) {
            break;
        }
        
        retries++;
        if (retries < handler->maxRetries) {
            Sleep(handler->retryDelay);
        }
    }
    
    return status;
}
```

### 6.5 資源釋放
```c
// 安全釋放資源
void SafeReleaseCamera(dvpHandle* pCamera) {
    if (pCamera && *pCamera) {
        dvpStatus status;
        
        // 停止視頻流
        dvpStreamState state;
        status = dvpGetStreamState(*pCamera, &state);
        if (status == DVP_STATUS_OK && state != STATE_STOPED) {
            dvpStop(*pCamera);
        }
        
        // 關閉相機
        status = dvpClose(*pCamera);
        if (status == DVP_STATUS_OK) {
            *pCamera = NULL;
            printf("相機資源已釋放\n");
        } else {
            printf("釋放相機資源失敗: %d\n", status);
        }
    }
}

// RAII風格的相機管理（C++）
class CameraGuard {
private:
    dvpHandle camera;
    
public:
    CameraGuard(dvpHandle cam) : camera(cam) {}
    
    ~CameraGuard() {
        if (camera) {
            dvpStop(camera);
            dvpClose(camera);
        }
    }
    
    operator dvpHandle() { return camera; }
};
```

---

## 7. 常見使用模式

### 7.1 單相機採集
```c
// 標準單相機採集流程
void SingleCameraAcquisition() {
    dvpHandle camera;
    dvpStatus status;
    
    // 初始化
    status = InitializeCamera(&camera, NULL);
    if (status != DVP_STATUS_OK) {
        printf("初始化失敗\n");
        return;
    }
    
    // 配置參數
    CameraParams params = {
        .exposure = 10000.0f,
        .gain = 1.0f,
        .roi = {0, 0, 1920, 1080},
        .autoExposure = false,
        .autoGain = false
    };
    SetCameraParameters(camera, &params);
    
    // 採集圖像
    ContinuousCapture(camera, 100);
    
    // 釋放資源
    SafeReleaseCamera(&camera);
}
```

### 7.2 多相機同步
```c
// 多相機同步採集
typedef struct MultiCamera {
    dvpHandle* cameras;
    int count;
    bool synchronized;
} MultiCamera;

dvpStatus InitializeMultiCameras(MultiCamera* mc, int cameraCount) {
    dvpStatus status;
    
    mc->cameras = (dvpHandle*)malloc(sizeof(dvpHandle) * cameraCount);
    mc->count = cameraCount;
    
    for (int i = 0; i < cameraCount; i++) {
        dvpCameraInfo info;
        status = dvpEnum(i, &info);
        if (status != DVP_STATUS_OK) {
            // 清理已打開的相機
            for (int j = 0; j < i; j++) {
                dvpClose(mc->cameras[j]);
            }
            free(mc->cameras);
            return status;
        }
        
        status = dvpOpen(i, OPEN_ONLINE, &mc->cameras[i]);
        if (status != DVP_STATUS_OK) {
            // 清理已打開的相機
            for (int j = 0; j < i; j++) {
                dvpClose(mc->cameras[j]);
            }
            free(mc->cameras);
            return status;
        }
        
        // 配置硬件觸發同步
        if (mc->synchronized) {
            dvpSetTriggerMode(mc->cameras[i], true);
            dvpSetTriggerSource(mc->cameras[i], TRIGGER_SOURCE_LINE0);
        }
    }
    
    return DVP_STATUS_OK;
}

void CaptureFromMultiCameras(MultiCamera* mc) {
    dvpFrame* frames = (dvpFrame*)malloc(sizeof(dvpFrame) * mc->count);
    
    // 啟動所有相機
    for (int i = 0; i < mc->count; i++) {
        dvpStart(mc->cameras[i]);
    }
    
    // 如果是軟件同步，同時觸發
    if (mc->synchronized) {
        for (int i = 0; i < mc->count; i++) {
            dvpTriggerFire(mc->cameras[i]);
        }
    }
    
    // 從所有相機獲取圖像
    for (int i = 0; i < mc->count; i++) {
        dvpUint32 count;
        dvpGetFrame(mc->cameras[i], &frames[i], &count, 5000);
    }
    
    // 處理多相機圖像
    ProcessMultiCameraFrames(frames, mc->count);
    
    // 停止所有相機
    for (int i = 0; i < mc->count; i++) {
        dvpStop(mc->cameras[i]);
    }
    
    free(frames);
}
```

### 7.3 觸發模式應用
```c
// 外部硬件觸發模式
dvpStatus SetupHardwareTrigger(dvpHandle camera, 
                               dvpTriggerSource source,
                               dvpTriggerEdge edge) {
    dvpStatus status;
    
    // 啟用觸發模式
    status = dvpSetTriggerMode(camera, true);
    if (status != DVP_STATUS_OK) return status;
    
    // 設置觸發源
    status = dvpSetTriggerSource(camera, source);
    if (status != DVP_STATUS_OK) return status;
    
    // 設置觸發邊沿
    status = dvpSetTriggerEdge(camera, edge);
    if (status != DVP_STATUS_OK) return status;
    
    // 設置觸發延遲（可選）
    status = dvpSetTriggerDelay(camera, 0.0f);
    if (status != DVP_STATUS_OK) return status;
    
    return DVP_STATUS_OK;
}

// 軟件週期觸發
void PeriodicSoftwareTrigger(dvpHandle camera, 
                             int count, 
                             int intervalMs) {
    dvpStatus status;
    dvpFrame frame;
    dvpUint32 frameCount;
    
    // 設置軟件觸發
    dvpSetTriggerMode(camera, true);
    dvpSetTriggerSource(camera, TRIGGER_SOURCE_SOFTWARE);
    
    // 啟動視頻流
    dvpStart(camera);
    
    for (int i = 0; i < count; i++) {
        // 觸發採集
        status = dvpTriggerFire(camera);
        if (status == DVP_STATUS_OK) {
            // 獲取圖像
            status = dvpGetFrame(camera, &frame, &frameCount, 5000);
            if (status == DVP_STATUS_OK) {
                printf("觸發採集第 %d 幀\n", i+1);
                ProcessFrame(&frame);
            }
        }
        
        // 週期延遲
        Sleep(intervalMs);
    }
    
    dvpStop(camera);
}
```

### 7.4 參數動態調整
```c
// 自適應曝光控制
typedef struct AdaptiveExposure {
    float targetBrightness;
    float minExposure;
    float maxExposure;
    float adjustStep;
} AdaptiveExposure;

void AdjustExposureAdaptively(dvpHandle camera, 
                              AdaptiveExposure* ae,
                              dvpFrame* frame) {
    // 計算圖像平均亮度
    float avgBrightness = CalculateAverageBrightness(frame);
    
    // 獲取當前曝光值
    float currentExposure;
    dvpGetExposure(camera, &currentExposure);
    
    // 調整曝光
    float newExposure = currentExposure;
    if (avgBrightness < ae->targetBrightness - 10) {
        // 圖像太暗，增加曝光
        newExposure = currentExposure + ae->adjustStep;
        if (newExposure > ae->maxExposure) {
            newExposure = ae->maxExposure;
        }
    } else if (avgBrightness > ae->targetBrightness + 10) {
        // 圖像太亮，減少曝光
        newExposure = currentExposure - ae->adjustStep;
        if (newExposure < ae->minExposure) {
            newExposure = ae->minExposure;
        }
    }
    
    if (newExposure != currentExposure) {
        dvpSetExposure(camera, newExposure);
        printf("調整曝光: %.2f -> %.2f\n", 
               currentExposure, newExposure);
    }
}

// 動態ROI調整
void DynamicROIAdjustment(dvpHandle camera, 
                         dvpRegion* targetRegion,
                         int trackingMode) {
    dvpRegion currentRoi;
    dvpGetRoi(camera, &currentRoi);
    
    switch(trackingMode) {
        case 0: // 中心區域
            targetRegion->X = (currentRoi.W - targetRegion->W) / 2;
            targetRegion->Y = (currentRoi.H - targetRegion->H) / 2;
            break;
            
        case 1: // 跟蹤目標
            // 根據目標檢測結果更新ROI
            UpdateROIFromTracking(targetRegion);
            break;
            
        case 2: // 自動擴展
            if (NeedExpansion()) {
                targetRegion->W = min(targetRegion->W * 1.2, 
                                     currentRoi.W);
                targetRegion->H = min(targetRegion->H * 1.2, 
                                     currentRoi.H);
            }
            break;
    }
    
    dvpSetRoi(camera, targetRegion);
}
```

---

## 8. 版本兼容性說明

### 8.1 API版本信息

| SDK版本 | 發布日期 | 主要更新 | 兼容性 |
|---------|----------|----------|---------|
| v2.0.0 | 2024-01 | 全新架構，支持多平台 | 不兼容v1.x |
| v1.5.0 | 2023-06 | 增加觸發功能 | 兼容v1.4.x |
| v1.4.0 | 2023-01 | 優化性能 | 兼容v1.3.x |
| v1.3.0 | 2022-08 | 增加回調機制 | 兼容v1.2.x |

### 8.2 新增功能列表

#### v2.0.0 新增功能
- 多線程安全支持
- 異步圖像採集
- GPU加速處理
- 擴展觸發模式
- 增強的錯誤處理
- 配置文件管理

```c
// v2.0.0 新增API示例
dvpStatus dvpSetAsyncMode(dvpHandle handle, bool async);
dvpStatus dvpGetFrameAsync(dvpHandle handle, 
                           dvpFrameCallback callback,
                           void* userData);
dvpStatus dvpEnableGPU(dvpHandle handle, bool enable);
```

#### v1.5.0 新增功能
- 硬件觸發支持
- 多相機同步
- 擴展的圖像格式

```c
// v1.5.0 新增API示例
dvpStatus dvpSetTriggerDelay(dvpHandle handle, float delay);
dvpStatus dvpSetTriggerJitter(dvpHandle handle, float jitter);
dvpStatus dvpSyncCameras(dvpHandle* cameras, int count);
```

### 8.3 廢棄API說明

| 廢棄API | 替代API | 廢棄版本 | 移除版本 |
|---------|---------|----------|----------|
| `dvpGetImage` | `dvpGetFrame` | v1.3.0 | v2.0.0 |
| `dvpSetROI` | `dvpSetRoi` | v1.4.0 | v2.1.0 |
| `dvpGetProperty` | 具體屬性API | v1.5.0 | v2.2.0 |

### 8.4 遷移指南

#### 從v1.x遷移到v2.0
```c
// 舊版本代碼 (v1.x)
dvpImage image;
dvpGetImage(camera, &image, 1000);
dvpSetROI(camera, 0, 0, 640, 480);

// 新版本代碼 (v2.0)
dvpFrame frame;
dvpUint32 count;
dvpGetFrame(camera, &frame, &count, 1000);
dvpRegion roi = {0, 0, 640, 480};
dvpSetRoi(camera, &roi);
```

### 8.5 平台特定說明

| 平台 | 支持版本 | 特殊要求 | 注意事項 |
|------|----------|----------|----------|
| Windows | v1.0+ | VS2015+ | 需要運行時庫 |
| Linux | v1.2+ | GCC 4.8+ | 需要libusb |
| macOS | v1.5+ | Xcode 10+ | 需要簽名 |
| ARM | v2.0+ | 交叉編譯 | 性能受限 |

### 8.6 版本檢查

```c
// 運行時版本檢查
void CheckSDKVersion() {
    dvpUint32 major, minor, patch;
    dvpGetSDKVersion(&major, &minor, &patch);
    
    printf("SDK版本: %d.%d.%d\n", major, minor, patch);
    
    // 檢查最低版本要求
    if (major < 2) {
        printf("警告: SDK版本過低，建議升級到v2.0以上\n");
    }
    
    // 檢查API兼容性
    if (major == 2 && minor >= 0) {
        printf("支持所有v2.0 API\n");
    }
}

// 編譯時版本檢查
#if DVP_SDK_VERSION_MAJOR < 2
    #error "需要SDK v2.0或更高版本"
#endif
```

---

## 附錄：快速查找索引

### 按功能分類
- **設備管理**: dvpRefresh, dvpEnum, dvpOpen, dvpClose
- **參數設置**: dvpSetExposure, dvpSetGain, dvpSetRoi
- **圖像採集**: dvpStart, dvpStop, dvpGetFrame
- **觸發控制**: dvpSetTriggerMode, dvpTriggerFire
- **錯誤處理**: dvpGetLastError, dvpGetErrorString

### 按使用頻率
1. **最常用**: dvpOpen, dvpGetFrame, dvpClose
2. **常用**: dvpSetExposure, dvpSetGain, dvpStart, dvpStop
3. **偶爾使用**: dvpSetTriggerMode, dvpSetRoi, dvpGetCameraInfo
4. **特殊場景**: dvpSaveConfig, dvpLoadConfig, dvpResetDevice

### 按開發階段
- **初始開發**: dvpEnum, dvpOpen, dvpGetFrame
- **參數調試**: dvpSetExposure, dvpSetGain, dvpGetRange系列
- **性能優化**: dvpSetBufferCount, dvpSetStreamMode
- **生產部署**: dvpSaveConfig, dvpLoadConfig, 錯誤處理API

---

## 更新日誌

| 日期 | 版本 | 更新內容 |
|------|------|----------|
| 2024-01-15 | 1.0 | 初始版本發布 |
| 2024-01-20 | 1.1 | 增加多相機同步章節 |
| 2024-01-25 | 1.2 | 更新錯誤碼對照表 |
| 2024-01-30 | 1.3 | 添加版本兼容性說明 |

---

## 技術支持

如需更多幫助，請聯繫：
- 技術支持郵箱：support@do3think.com
- 開發者論壇：https://forum.do3think.com
- SDK下載：https://www.do3think.com/downloads
- 在線文檔：https://docs.do3think.com

---

*本文檔基於Do3Think Camera SDK v2.0編寫，持續更新中。*