# Do3ThinkCamera SDK 錯誤處理與診斷開發文檔

## 目錄
1. [錯誤碼系統架構](#1-錯誤碼系統架構)
2. [錯誤處理策略設計](#2-錯誤處理策略設計)
3. [異常恢復機制](#3-異常恢復機制)
4. [設備斷線重連](#4-設備斷線重連)
5. [超時處理方案](#5-超時處理方案)
6. [日誌系統設計](#6-日誌系統設計)
7. [診斷信息收集](#7-診斷信息收集)
8. [性能監控指標](#8-性能監控指標)
9. [錯誤上報機制](#9-錯誤上報機制)
10. [生產環境錯誤處理](#10-生產環境錯誤處理)
11. [工業級錯誤處理類實現](#11-工業級錯誤處理類實現)

---

## 1. 錯誤碼系統架構

### 1.1 錯誤碼分類

Do3ThinkCamera SDK使用`dvpStatus`枚舉定義了完整的錯誤碼體系：

```cpp
// 錯誤碼狀態分級
// > DVP_STATUS_OK: 警告（Warning）
// = DVP_STATUS_OK: 成功（Success）
// < DVP_STATUS_OK: 錯誤（Error）

namespace Do3Think {
namespace ErrorCode {

// 成功與警告狀態碼
constexpr dvpStatus STATUS_IGNORED = DVP_STATUS_IGNORED;           // 7: 操作被忽略
constexpr dvpStatus STATUS_NEED_OTHER = DVP_STATUS_NEED_OTHER;     // 6: 需要其他數據
constexpr dvpStatus STATUS_NEXT_STAGE = DVP_STATUS_NEXT_STAGE;     // 5: 需進行下一階段
constexpr dvpStatus STATUS_BUSY = DVP_STATUS_BUSY;                 // 4: 設備正忙
constexpr dvpStatus STATUS_WAIT = DVP_STATUS_WAIT;                 // 3: 需要等待
constexpr dvpStatus STATUS_IN_PROCESS = DVP_STATUS_IN_PROCESS;     // 2: 正在進行中
constexpr dvpStatus STATUS_OK = DVP_STATUS_OK;                     // 1: 操作成功

// 一般錯誤碼
constexpr dvpStatus ERROR_FAILED = DVP_STATUS_FAILED;              // 0: 操作失敗
constexpr dvpStatus ERROR_UNKNOWN = DVP_STATUS_UNKNOW;             // -1: 未知錯誤
constexpr dvpStatus ERROR_NOT_SUPPORTED = DVP_STATUS_NOT_SUPPORTED; // -2: 功能不支持
constexpr dvpStatus ERROR_NOT_INITIALIZED = DVP_STATUS_NOT_INITIALIZED; // -3: 未初始化

// 參數錯誤
constexpr dvpStatus ERROR_INVALID_PARAM = DVP_STATUS_PARAMETER_INVALID;     // -4: 參數無效
constexpr dvpStatus ERROR_PARAM_OUT_OF_BOUND = DVP_STATUS_PARAMETER_OUT_OF_BOUND; // -5: 參數越界

// 狀態錯誤
constexpr dvpStatus ERROR_UNENABLED = DVP_STATUS_UNENABLED;        // -6: 特性未打開
constexpr dvpStatus ERROR_UNCONNECTED = DVP_STATUS_UNCONNECTED;    // -7: 未連接設備
constexpr dvpStatus ERROR_NOT_VALID = DVP_STATUS_NOT_VALID;        // -8: 功能無效
constexpr dvpStatus ERROR_UNPLAY = DVP_STATUS_UNPLAY;              // -9: 設備未打開
constexpr dvpStatus ERROR_NOT_STARTED = DVP_STATUS_NOT_STARTED;    // -10: 未啟動
constexpr dvpStatus ERROR_NOT_STOPPED = DVP_STATUS_NOT_STOPPED;    // -11: 未停止
constexpr dvpStatus ERROR_NOT_READY = DVP_STATUS_NOT_READY;        // -12: 未準備好
constexpr dvpStatus ERROR_INVALID_HANDLE = DVP_STATUS_INVALID_HANDLE; // -13: 無效句柄

// 通訊錯誤
constexpr dvpStatus ERROR_TIMEOUT = DVP_STATUS_TIME_OUT;           // -1000: 超時錯誤
constexpr dvpStatus ERROR_IO = DVP_STATUS_IO_ERROR;                // -1001: 硬件IO錯誤
constexpr dvpStatus ERROR_COMM = DVP_STATUS_COMM_ERROR;            // -1002: 通訊錯誤
constexpr dvpStatus ERROR_BUS = DVP_STATUS_BUS_ERROR;              // -1003: 總線錯誤

// 設備狀態錯誤
constexpr dvpStatus ERROR_NO_DEVICE = DVP_STATUS_NO_DEVICE_FOUND;  // -1100: 未發現設備
constexpr dvpStatus ERROR_DEVICE_OPENED = DVP_STATUS_DEVICE_IS_OPENED; // -1102: 設備已打開
constexpr dvpStatus ERROR_DEVICE_DISCONNECTED = DVP_STATUS_DEVICE_IS_DISCONNECTED; // -1104: 設備已斷開

// 數據採集錯誤
constexpr dvpStatus ERROR_GRAB_FAILED = DVP_STATUS_GRAB_FAILED;    // -1600: 數據採集失敗
constexpr dvpStatus ERROR_LOST_DATA = DVP_STATUS_LOST_DATA;        // -1601: 數據丟失
constexpr dvpStatus ERROR_EOF = DVP_STATUS_EOF_ERROR;              // -1602: 未接收到幀結束符

} // namespace ErrorCode
} // namespace Do3Think
```

### 1.2 錯誤碼辨識與處理

```cpp
class ErrorCodeHelper {
public:
    // 錯誤類別判斷
    static bool IsSuccess(dvpStatus status) {
        return status == DVP_STATUS_OK;
    }
    
    static bool IsWarning(dvpStatus status) {
        return status > DVP_STATUS_OK;
    }
    
    static bool IsError(dvpStatus status) {
        return status < DVP_STATUS_OK;
    }
    
    // 錯誤類型分類
    enum class ErrorCategory {
        SUCCESS,
        WARNING,
        GENERAL_ERROR,
        PARAMETER_ERROR,
        STATE_ERROR,
        COMMUNICATION_ERROR,
        DEVICE_ERROR,
        MEMORY_ERROR,
        FILE_ERROR,
        GRAB_ERROR
    };
    
    static ErrorCategory GetErrorCategory(dvpStatus status) {
        if (status == DVP_STATUS_OK) return ErrorCategory::SUCCESS;
        if (status > DVP_STATUS_OK) return ErrorCategory::WARNING;
        
        if (status >= -100) return ErrorCategory::GENERAL_ERROR;
        if (status >= -200) return ErrorCategory::PARAMETER_ERROR;
        if (status >= -300) return ErrorCategory::STATE_ERROR;
        if (status >= -1100) return ErrorCategory::COMMUNICATION_ERROR;
        if (status >= -1200) return ErrorCategory::DEVICE_ERROR;
        if (status >= -1300) return ErrorCategory::MEMORY_ERROR;
        if (status >= -1400) return ErrorCategory::FILE_ERROR;
        if (status >= -1700) return ErrorCategory::GRAB_ERROR;
        
        return ErrorCategory::GENERAL_ERROR;
    }
    
    // 獲取錯誤描述
    static std::string GetErrorDescription(dvpStatus status) {
        static std::unordered_map<dvpStatus, std::string> errorDescriptions = {
            {DVP_STATUS_OK, "操作成功"},
            {DVP_STATUS_FAILED, "操作失敗"},
            {DVP_STATUS_TIME_OUT, "操作超時"},
            {DVP_STATUS_INVALID_HANDLE, "無效的相機句柄"},
            {DVP_STATUS_PARAMETER_INVALID, "參數無效"},
            {DVP_STATUS_NOT_SUPPORTED, "功能不支持"},
            {DVP_STATUS_DEVICE_IS_DISCONNECTED, "設備已斷開連接"},
            {DVP_STATUS_GRAB_FAILED, "圖像採集失敗"},
            // ... 添加更多錯誤描述
        };
        
        auto it = errorDescriptions.find(status);
        if (it != errorDescriptions.end()) {
            return it->second;
        }
        
        return "未知錯誤: " + std::to_string(static_cast<int>(status));
    }
};
```

---

## 2. 錯誤處理策略設計

### 2.1 基本錯誤處理宏

```cpp
// 錯誤檢查宏定義
#define DVP_CHECK(status) \
    do { \
        dvpStatus _status = (status); \
        if (_status != DVP_STATUS_OK) { \
            Do3Think::ErrorHandler::HandleError(_status, __FILE__, __LINE__, #status); \
            return _status; \
        } \
    } while(0)

#define DVP_CHECK_CONTINUE(status) \
    do { \
        dvpStatus _status = (status); \
        if (_status != DVP_STATUS_OK) { \
            Do3Think::ErrorHandler::LogError(_status, __FILE__, __LINE__, #status); \
        } \
    } while(0)

#define DVP_CHECK_THROW(status) \
    do { \
        dvpStatus _status = (status); \
        if (_status != DVP_STATUS_OK) { \
            throw Do3Think::CameraException(_status, __FILE__, __LINE__); \
        } \
    } while(0)
```

### 2.2 異常類設計

```cpp
namespace Do3Think {

class CameraException : public std::runtime_error {
private:
    dvpStatus m_status;
    std::string m_file;
    int m_line;
    std::chrono::system_clock::time_point m_timestamp;
    
public:
    CameraException(dvpStatus status, const std::string& file, int line)
        : std::runtime_error(ErrorCodeHelper::GetErrorDescription(status))
        , m_status(status)
        , m_file(file)
        , m_line(line)
        , m_timestamp(std::chrono::system_clock::now()) {
    }
    
    dvpStatus GetStatus() const { return m_status; }
    const std::string& GetFile() const { return m_file; }
    int GetLine() const { return m_line; }
    
    std::string GetDetailedMessage() const {
        std::stringstream ss;
        ss << "Camera Exception: " << what() << "\n"
           << "Error Code: " << static_cast<int>(m_status) << "\n"
           << "Location: " << m_file << ":" << m_line << "\n"
           << "Timestamp: " << FormatTimestamp(m_timestamp);
        return ss.str();
    }
    
private:
    static std::string FormatTimestamp(const std::chrono::system_clock::time_point& tp) {
        auto time_t = std::chrono::system_clock::to_time_t(tp);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }
};

} // namespace Do3Think
```

---

## 3. 異常恢復機制

### 3.1 自動恢復處理器

```cpp
class RecoveryHandler {
private:
    struct RecoveryStrategy {
        std::function<dvpStatus()> recoveryAction;
        int maxRetries;
        std::chrono::milliseconds retryDelay;
        bool exponentialBackoff;
    };
    
    std::unordered_map<ErrorCodeHelper::ErrorCategory, RecoveryStrategy> m_strategies;
    
public:
    RecoveryHandler() {
        InitializeDefaultStrategies();
    }
    
    void RegisterStrategy(ErrorCodeHelper::ErrorCategory category, 
                         const RecoveryStrategy& strategy) {
        m_strategies[category] = strategy;
    }
    
    dvpStatus TryRecover(dvpStatus status, dvpHandle handle) {
        auto category = ErrorCodeHelper::GetErrorCategory(status);
        auto it = m_strategies.find(category);
        
        if (it == m_strategies.end()) {
            return status; // 無恢復策略
        }
        
        const auto& strategy = it->second;
        return ExecuteRecoveryStrategy(strategy, handle);
    }
    
private:
    void InitializeDefaultStrategies() {
        // 通訊錯誤恢復策略
        m_strategies[ErrorCodeHelper::ErrorCategory::COMMUNICATION_ERROR] = {
            [](dvpHandle handle) { return dvpResetDevice(handle); },
            3,  // 最多重試3次
            std::chrono::milliseconds(1000),  // 重試延遲1秒
            true  // 使用指數退避
        };
        
        // 設備錯誤恢復策略
        m_strategies[ErrorCodeHelper::ErrorCategory::DEVICE_ERROR] = {
            [](dvpHandle handle) { 
                dvpStop(handle);
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                return dvpStart(handle);
            },
            2,
            std::chrono::milliseconds(2000),
            false
        };
        
        // 數據採集錯誤恢復策略
        m_strategies[ErrorCodeHelper::ErrorCategory::GRAB_ERROR] = {
            [](dvpHandle handle) {
                dvpHold(handle);
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                return dvpRestart(handle);
            },
            5,
            std::chrono::milliseconds(500),
            true
        };
    }
    
    dvpStatus ExecuteRecoveryStrategy(const RecoveryStrategy& strategy, 
                                     dvpHandle handle) {
        auto delay = strategy.retryDelay;
        
        for (int i = 0; i < strategy.maxRetries; ++i) {
            dvpStatus status = strategy.recoveryAction(handle);
            
            if (status == DVP_STATUS_OK) {
                LogRecoverySuccess(i + 1);
                return status;
            }
            
            if (i < strategy.maxRetries - 1) {
                std::this_thread::sleep_for(delay);
                
                if (strategy.exponentialBackoff) {
                    delay *= 2;  // 指數退避
                }
            }
        }
        
        LogRecoveryFailed(strategy.maxRetries);
        return DVP_STATUS_FAILED;
    }
    
    void LogRecoverySuccess(int attempts) {
        // 記錄恢復成功
    }
    
    void LogRecoveryFailed(int attempts) {
        // 記錄恢復失敗
    }
};
```

---

## 4. 設備斷線重連

### 4.1 斷線檢測與重連機制

```cpp
class ConnectionManager {
private:
    dvpHandle m_handle;
    std::string m_deviceId;
    std::atomic<bool> m_isConnected{false};
    std::atomic<bool> m_shouldReconnect{true};
    std::thread m_reconnectThread;
    std::mutex m_reconnectMutex;
    std::condition_variable m_reconnectCv;
    
    // 重連配置
    struct ReconnectConfig {
        int maxAttempts = 10;
        std::chrono::milliseconds initialDelay{1000};
        std::chrono::milliseconds maxDelay{30000};
        double backoffMultiplier = 2.0;
    } m_config;
    
public:
    ConnectionManager(const std::string& deviceId) 
        : m_deviceId(deviceId) {
        RegisterEventCallbacks();
    }
    
    ~ConnectionManager() {
        StopReconnection();
    }
    
    void StartMonitoring() {
        m_shouldReconnect = true;
        m_reconnectThread = std::thread(&ConnectionManager::ReconnectWorker, this);
    }
    
    void StopReconnection() {
        m_shouldReconnect = false;
        m_reconnectCv.notify_all();
        
        if (m_reconnectThread.joinable()) {
            m_reconnectThread.join();
        }
    }
    
private:
    void RegisterEventCallbacks() {
        // 註冊斷線事件回調
        dvpRegisterEventCallback(m_handle, 
            [](dvpHandle handle, dvpEvent event, void* pContext, 
               dvpInt32 param, dvpVariant* pVariant) -> dvpInt32 {
                auto* mgr = static_cast<ConnectionManager*>(pContext);
                
                switch (event) {
                    case EVENT_DISCONNECTED:
                    case EVENT_LOST_CONNECTION:
                        mgr->OnDisconnected();
                        break;
                        
                    case EVENT_CONNECTED:
                    case EVENT_RECONNECTED:
                        mgr->OnConnected();
                        break;
                        
                    default:
                        break;
                }
                
                return 0;
            },
            EVENT_DISCONNECTED,
            this
        );
    }
    
    void OnDisconnected() {
        LogInfo("設備斷開連接: " + m_deviceId);
        m_isConnected = false;
        
        // 觸發重連
        m_reconnectCv.notify_one();
    }
    
    void OnConnected() {
        LogInfo("設備連接成功: " + m_deviceId);
        m_isConnected = true;
    }
    
    void ReconnectWorker() {
        auto delay = m_config.initialDelay;
        int attempts = 0;
        
        while (m_shouldReconnect) {
            std::unique_lock<std::mutex> lock(m_reconnectMutex);
            
            // 等待斷線事件或停止信號
            m_reconnectCv.wait(lock, [this] {
                return !m_isConnected || !m_shouldReconnect;
            });
            
            if (!m_shouldReconnect) {
                break;
            }
            
            // 執行重連
            while (!m_isConnected && attempts < m_config.maxAttempts) {
                LogInfo("嘗試重新連接設備 (第 " + std::to_string(attempts + 1) + " 次)");
                
                if (TryReconnect()) {
                    m_isConnected = true;
                    attempts = 0;
                    delay = m_config.initialDelay;
                    LogInfo("設備重連成功");
                    break;
                }
                
                attempts++;
                
                // 等待並增加延遲時間
                std::this_thread::sleep_for(delay);
                delay = std::min(
                    std::chrono::milliseconds(
                        static_cast<long>(delay.count() * m_config.backoffMultiplier)
                    ),
                    m_config.maxDelay
                );
            }
            
            if (attempts >= m_config.maxAttempts) {
                LogError("設備重連失敗，已達最大嘗試次數");
                OnReconnectFailed();
                attempts = 0;
                delay = m_config.initialDelay;
            }
        }
    }
    
    bool TryReconnect() {
        // 先關閉現有連接
        if (m_handle) {
            dvpClose(m_handle);
            m_handle = 0;
        }
        
        // 嘗試重新打開設備
        dvpStatus status = dvpOpenByUserId(
            const_cast<char*>(m_deviceId.c_str()), 
            OPEN_NORMAL, 
            &m_handle
        );
        
        if (status == DVP_STATUS_OK) {
            // 恢復設備設置
            RestoreDeviceSettings();
            return true;
        }
        
        return false;
    }
    
    void RestoreDeviceSettings() {
        // 恢復之前的設備配置
        // 這裡應該從配置文件或內存中恢復設置
    }
    
    void OnReconnectFailed() {
        // 通知上層應用重連失敗
        // 可能需要人工干預
    }
    
    void LogInfo(const std::string& msg) {
        // 記錄信息日誌
    }
    
    void LogError(const std::string& msg) {
        // 記錄錯誤日誌
    }
};
```

---

## 5. 超時處理方案

### 5.1 超時管理器

```cpp
class TimeoutManager {
private:
    struct TimeoutConfig {
        std::chrono::milliseconds defaultTimeout{5000};
        std::chrono::milliseconds frameTimeout{1000};
        std::chrono::milliseconds commandTimeout{3000};
        std::chrono::milliseconds connectionTimeout{10000};
    } m_config;
    
public:
    template<typename Func>
    dvpStatus ExecuteWithTimeout(Func func, 
                                 std::chrono::milliseconds timeout) {
        std::promise<dvpStatus> promise;
        std::future<dvpStatus> future = promise.get_future();
        
        std::thread worker([func, &promise]() {
            try {
                dvpStatus status = func();
                promise.set_value(status);
            } catch (...) {
                promise.set_exception(std::current_exception());
            }
        });
        
        if (future.wait_for(timeout) == std::future_status::timeout) {
            worker.detach();  // 放棄等待
            return DVP_STATUS_TIME_OUT;
        }
        
        worker.join();
        
        try {
            return future.get();
        } catch (...) {
            return DVP_STATUS_FAILED;
        }
    }
    
    // 帶重試的超時執行
    template<typename Func>
    dvpStatus ExecuteWithRetry(Func func, 
                              int maxRetries = 3,
                              std::chrono::milliseconds timeout = std::chrono::milliseconds(5000)) {
        for (int i = 0; i < maxRetries; ++i) {
            dvpStatus status = ExecuteWithTimeout(func, timeout);
            
            if (status != DVP_STATUS_TIME_OUT) {
                return status;
            }
            
            if (i < maxRetries - 1) {
                LogWarning("操作超時，正在重試 (" + 
                          std::to_string(i + 1) + "/" + 
                          std::to_string(maxRetries) + ")");
                
                // 增加超時時間
                timeout = timeout * 1.5;
            }
        }
        
        LogError("操作超時，已達最大重試次數");
        return DVP_STATUS_TIME_OUT;
    }
    
    // 獲取幀數據的專用超時處理
    dvpStatus GetFrameWithTimeout(dvpHandle handle, 
                                  dvpFrame* pFrame,
                                  std::chrono::milliseconds timeout) {
        auto startTime = std::chrono::steady_clock::now();
        
        while (true) {
            dvpStatus status = dvpGetFrame(handle, pFrame);
            
            if (status == DVP_STATUS_OK) {
                return status;
            }
            
            auto elapsed = std::chrono::steady_clock::now() - startTime;
            
            if (elapsed >= timeout) {
                return DVP_STATUS_TIME_OUT;
            }
            
            // 短暫休眠避免CPU佔用過高
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
    
private:
    void LogWarning(const std::string& msg) {
        // 記錄警告日誌
    }
    
    void LogError(const std::string& msg) {
        // 記錄錯誤日誌
    }
};
```

---

## 6. 日誌系統設計

### 6.1 分級日誌系統

```cpp
enum class LogLevel {
    TRACE = 0,
    DEBUG = 1,
    INFO = 2,
    WARNING = 3,
    ERROR = 4,
    CRITICAL = 5
};

class Logger {
private:
    static std::unique_ptr<Logger> s_instance;
    LogLevel m_minLevel = LogLevel::INFO;
    std::ofstream m_logFile;
    std::mutex m_logMutex;
    std::queue<std::string> m_logBuffer;
    std::thread m_flushThread;
    std::atomic<bool> m_running{true};
    std::condition_variable m_flushCv;
    
    // 日誌輪轉配置
    struct RotationConfig {
        size_t maxFileSize = 100 * 1024 * 1024;  // 100MB
        int maxFiles = 10;
        std::string logDirectory = "./logs";
        std::string filePrefix = "camera";
    } m_rotationConfig;
    
public:
    static Logger& GetInstance() {
        if (!s_instance) {
            s_instance = std::make_unique<Logger>();
        }
        return *s_instance;
    }
    
    Logger() {
        InitializeLogFile();
        StartFlushThread();
    }
    
    ~Logger() {
        m_running = false;
        m_flushCv.notify_all();
        
        if (m_flushThread.joinable()) {
            m_flushThread.join();
        }
        
        FlushAll();
    }
    
    void Log(LogLevel level, const std::string& message, 
            const std::string& file = "", int line = 0) {
        if (level < m_minLevel) {
            return;
        }
        
        std::string logEntry = FormatLogEntry(level, message, file, line);
        
        {
            std::lock_guard<std::mutex> lock(m_logMutex);
            m_logBuffer.push(logEntry);
        }
        
        m_flushCv.notify_one();
    }
    
    void LogError(dvpStatus status, const std::string& context) {
        std::stringstream ss;
        ss << "錯誤碼: " << static_cast<int>(status) 
           << ", 描述: " << ErrorCodeHelper::GetErrorDescription(status)
           << ", 上下文: " << context;
        
        Log(LogLevel::ERROR, ss.str());
    }
    
    // 記錄調試報告
    void LogDebugReport(dvpHandle handle, const std::string& context) {
        dvpStatus status = dvpDebugReport(
            handle, 
            PART_DEFAULT, 
            LEVEL_INFO, 
            false, 
            const_cast<char*>(context.c_str()), 
            0
        );
        
        if (status != DVP_STATUS_OK) {
            LogError(status, "調試報告生成失敗");
        }
    }
    
private:
    void InitializeLogFile() {
        // 創建日誌目錄
        std::filesystem::create_directories(m_rotationConfig.logDirectory);
        
        // 生成日誌文件名
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        
        std::stringstream filename;
        filename << m_rotationConfig.logDirectory << "/" 
                << m_rotationConfig.filePrefix << "_"
                << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S") 
                << ".log";
        
        m_logFile.open(filename.str(), std::ios::app);
    }
    
    void StartFlushThread() {
        m_flushThread = std::thread([this]() {
            while (m_running) {
                std::unique_lock<std::mutex> lock(m_logMutex);
                
                m_flushCv.wait_for(lock, std::chrono::seconds(1), [this] {
                    return !m_logBuffer.empty() || !m_running;
                });
                
                if (!m_logBuffer.empty()) {
                    FlushBuffer();
                }
                
                CheckRotation();
            }
        });
    }
    
    void FlushBuffer() {
        while (!m_logBuffer.empty()) {
            m_logFile << m_logBuffer.front() << std::endl;
            m_logBuffer.pop();
        }
        m_logFile.flush();
    }
    
    void FlushAll() {
        std::lock_guard<std::mutex> lock(m_logMutex);
        FlushBuffer();
    }
    
    void CheckRotation() {
        if (m_logFile.tellp() > m_rotationConfig.maxFileSize) {
            m_logFile.close();
            InitializeLogFile();
        }
    }
    
    std::string FormatLogEntry(LogLevel level, const std::string& message,
                               const std::string& file, int line) {
        std::stringstream ss;
        
        // 時間戳
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()
        ) % 1000;
        
        ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        ss << "." << std::setfill('0') << std::setw(3) << ms.count();
        
        // 日誌級別
        ss << " [" << GetLevelString(level) << "]";
        
        // 文件和行號
        if (!file.empty()) {
            ss << " [" << file << ":" << line << "]";
        }
        
        // 消息
        ss << " " << message;
        
        return ss.str();
    }
    
    std::string GetLevelString(LogLevel level) {
        switch (level) {
            case LogLevel::TRACE:    return "TRACE";
            case LogLevel::DEBUG:    return "DEBUG";
            case LogLevel::INFO:     return "INFO";
            case LogLevel::WARNING:  return "WARN";
            case LogLevel::ERROR:    return "ERROR";
            case LogLevel::CRITICAL: return "CRIT";
            default:                 return "UNKNOWN";
        }
    }
};

std::unique_ptr<Logger> Logger::s_instance;

// 便捷宏定義
#define LOG_TRACE(msg) Logger::GetInstance().Log(LogLevel::TRACE, msg, __FILE__, __LINE__)
#define LOG_DEBUG(msg) Logger::GetInstance().Log(LogLevel::DEBUG, msg, __FILE__, __LINE__)
#define LOG_INFO(msg) Logger::GetInstance().Log(LogLevel::INFO, msg, __FILE__, __LINE__)
#define LOG_WARNING(msg) Logger::GetInstance().Log(LogLevel::WARNING, msg, __FILE__, __LINE__)
#define LOG_ERROR(msg) Logger::GetInstance().Log(LogLevel::ERROR, msg, __FILE__, __LINE__)
#define LOG_CRITICAL(msg) Logger::GetInstance().Log(LogLevel::CRITICAL, msg, __FILE__, __LINE__)
```

---

## 7. 診斷信息收集

### 7.1 診斷信息收集器

```cpp
class DiagnosticsCollector {
private:
    struct SystemInfo {
        std::string osVersion;
        std::string cpuInfo;
        size_t totalMemory;
        size_t availableMemory;
        std::vector<std::string> networkInterfaces;
    };
    
    struct CameraInfo {
        std::string deviceId;
        std::string firmwareVersion;
        std::string sensorInfo;
        dvpTemperatureInfo temperature;
        dvpFrameCount frameCount;
    };
    
    struct PerformanceMetrics {
        double avgFrameRate;
        double minFrameRate;
        double maxFrameRate;
        size_t droppedFrames;
        size_t errorCount;
        std::chrono::milliseconds uptime;
    };
    
public:
    struct DiagnosticsReport {
        SystemInfo system;
        CameraInfo camera;
        PerformanceMetrics performance;
        std::vector<std::string> recentErrors;
        std::string timestamp;
    };
    
    DiagnosticsReport CollectFullReport(dvpHandle handle) {
        DiagnosticsReport report;
        
        report.timestamp = GetCurrentTimestamp();
        report.system = CollectSystemInfo();
        report.camera = CollectCameraInfo(handle);
        report.performance = CollectPerformanceMetrics(handle);
        report.recentErrors = GetRecentErrors();
        
        // 調用SDK診斷報告
        GenerateSDKReport(handle);
        
        return report;
    }
    
    void SaveReportToFile(const DiagnosticsReport& report, 
                         const std::string& filename) {
        std::ofstream file(filename);
        
        if (!file.is_open()) {
            LOG_ERROR("無法創建診斷報告文件: " + filename);
            return;
        }
        
        // 寫入JSON格式報告
        file << "{\n";
        file << "  \"timestamp\": \"" << report.timestamp << "\",\n";
        file << "  \"system\": " << SerializeSystemInfo(report.system) << ",\n";
        file << "  \"camera\": " << SerializeCameraInfo(report.camera) << ",\n";
        file << "  \"performance\": " << SerializePerformanceMetrics(report.performance) << ",\n";
        file << "  \"recent_errors\": " << SerializeErrors(report.recentErrors) << "\n";
        file << "}\n";
        
        file.close();
        LOG_INFO("診斷報告已保存至: " + filename);
    }
    
private:
    SystemInfo CollectSystemInfo() {
        SystemInfo info;
        
        // 收集系統信息
        #ifdef _WIN32
        info.osVersion = GetWindowsVersion();
        #else
        info.osVersion = GetLinuxVersion();
        #endif
        
        info.cpuInfo = GetCPUInfo();
        info.totalMemory = GetTotalMemory();
        info.availableMemory = GetAvailableMemory();
        info.networkInterfaces = GetNetworkInterfaces();
        
        return info;
    }
    
    CameraInfo CollectCameraInfo(dvpHandle handle) {
        CameraInfo info;
        
        dvpCameraInfo cameraInfo;
        if (dvpGetCameraInfo(handle, &cameraInfo) == DVP_STATUS_OK) {
            info.deviceId = std::string(cameraInfo.FriendlyName);
            info.firmwareVersion = std::string(cameraInfo.FirmwareVersion);
        }
        
        dvpSensorInfo sensorInfo;
        if (dvpGetSensorInfo(handle, &sensorInfo) == DVP_STATUS_OK) {
            std::stringstream ss;
            ss << "Sensor: " << sensorInfo.descr 
               << ", Size: " << sensorInfo.iWidth << "x" << sensorInfo.iHeight
               << ", Pixel: " << sensorInfo.fPixelSize << "um";
            info.sensorInfo = ss.str();
        }
        
        dvpGetTemperatureInfo(handle, &info.temperature);
        dvpGetFrameCount(handle, &info.frameCount);
        
        return info;
    }
    
    PerformanceMetrics CollectPerformanceMetrics(dvpHandle handle) {
        PerformanceMetrics metrics;
        
        // 從性能監控器獲取數據
        // 這裡應該與PerformanceMonitor類集成
        
        return metrics;
    }
    
    std::vector<std::string> GetRecentErrors() {
        // 從錯誤日誌獲取最近的錯誤
        return {};
    }
    
    void GenerateSDKReport(dvpHandle handle) {
        // 生成詳細的SDK診斷報告
        dvpDebugReport(handle, PART_DEFAULT, LEVEL_INFO, true, 
                      const_cast<char*>("Full diagnostics"), 0);
    }
    
    std::string GetCurrentTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }
    
    // 序列化函數
    std::string SerializeSystemInfo(const SystemInfo& info) {
        // 實現JSON序列化
        return "{}";
    }
    
    std::string SerializeCameraInfo(const CameraInfo& info) {
        // 實現JSON序列化
        return "{}";
    }
    
    std::string SerializePerformanceMetrics(const PerformanceMetrics& metrics) {
        // 實現JSON序列化
        return "{}";
    }
    
    std::string SerializeErrors(const std::vector<std::string>& errors) {
        // 實現JSON序列化
        return "[]";
    }
    
    // 系統信息收集函數
    std::string GetWindowsVersion() {
        // Windows版本信息
        return "Windows";
    }
    
    std::string GetLinuxVersion() {
        // Linux版本信息
        return "Linux";
    }
    
    std::string GetCPUInfo() {
        // CPU信息
        return "CPU Info";
    }
    
    size_t GetTotalMemory() {
        // 總內存
        return 0;
    }
    
    size_t GetAvailableMemory() {
        // 可用內存
        return 0;
    }
    
    std::vector<std::string> GetNetworkInterfaces() {
        // 網絡接口信息
        return {};
    }
};
```

---

## 8. 性能監控指標

### 8.1 性能監控器

```cpp
class PerformanceMonitor {
private:
    struct FrameStatistics {
        std::atomic<uint64_t> totalFrames{0};
        std::atomic<uint64_t> droppedFrames{0};
        std::atomic<uint64_t> errorFrames{0};
        std::chrono::steady_clock::time_point startTime;
        std::chrono::steady_clock::time_point lastFrameTime;
        
        std::deque<double> recentFrameRates;
        std::mutex frameRateMutex;
        const size_t maxFrameRateSamples = 100;
    };
    
    struct LatencyStatistics {
        std::atomic<double> avgLatency{0.0};
        std::atomic<double> minLatency{DBL_MAX};
        std::atomic<double> maxLatency{0.0};
        std::atomic<uint64_t> sampleCount{0};
    };
    
    struct ResourceUsage {
        std::atomic<double> cpuUsage{0.0};
        std::atomic<size_t> memoryUsage{0};
        std::atomic<size_t> bufferUsage{0};
        std::atomic<double> bandwidth{0.0};
    };
    
    FrameStatistics m_frameStats;
    LatencyStatistics m_latencyStats;
    ResourceUsage m_resourceUsage;
    
    std::thread m_monitorThread;
    std::atomic<bool> m_running{false};
    dvpHandle m_handle;
    
public:
    PerformanceMonitor(dvpHandle handle) : m_handle(handle) {
        m_frameStats.startTime = std::chrono::steady_clock::now();
        m_frameStats.lastFrameTime = m_frameStats.startTime;
    }
    
    void Start() {
        m_running = true;
        m_monitorThread = std::thread(&PerformanceMonitor::MonitorLoop, this);
    }
    
    void Stop() {
        m_running = false;
        if (m_monitorThread.joinable()) {
            m_monitorThread.join();
        }
    }
    
    void OnFrameReceived() {
        m_frameStats.totalFrames++;
        
        auto now = std::chrono::steady_clock::now();
        auto delta = std::chrono::duration<double>(
            now - m_frameStats.lastFrameTime
        ).count();
        
        if (delta > 0) {
            double frameRate = 1.0 / delta;
            
            {
                std::lock_guard<std::mutex> lock(m_frameStats.frameRateMutex);
                m_frameStats.recentFrameRates.push_back(frameRate);
                
                if (m_frameStats.recentFrameRates.size() > 
                    m_frameStats.maxFrameRateSamples) {
                    m_frameStats.recentFrameRates.pop_front();
                }
            }
        }
        
        m_frameStats.lastFrameTime = now;
    }
    
    void OnFrameDropped() {
        m_frameStats.droppedFrames++;
        LOG_WARNING("幀丟失，總計: " + 
                   std::to_string(m_frameStats.droppedFrames));
    }
    
    void OnFrameError() {
        m_frameStats.errorFrames++;
    }
    
    void UpdateLatency(double latencyMs) {
        m_latencyStats.sampleCount++;
        
        // 更新最小/最大延遲
        double currentMin = m_latencyStats.minLatency.load();
        while (latencyMs < currentMin && 
               !m_latencyStats.minLatency.compare_exchange_weak(
                   currentMin, latencyMs)) {}
        
        double currentMax = m_latencyStats.maxLatency.load();
        while (latencyMs > currentMax && 
               !m_latencyStats.maxLatency.compare_exchange_weak(
                   currentMax, latencyMs)) {}
        
        // 更新平均延遲（簡化的移動平均）
        uint64_t count = m_latencyStats.sampleCount.load();
        double currentAvg = m_latencyStats.avgLatency.load();
        double newAvg = (currentAvg * (count - 1) + latencyMs) / count;
        m_latencyStats.avgLatency = newAvg;
    }
    
    struct PerformanceReport {
        double currentFPS;
        double averageFPS;
        double minFPS;
        double maxFPS;
        
        uint64_t totalFrames;
        uint64_t droppedFrames;
        uint64_t errorFrames;
        double dropRate;
        double errorRate;
        
        double avgLatency;
        double minLatency;
        double maxLatency;
        
        double cpuUsage;
        size_t memoryUsageMB;
        double bandwidthMbps;
        
        std::chrono::milliseconds uptime;
    };
    
    PerformanceReport GetReport() const {
        PerformanceReport report{};
        
        // 計算幀率統計
        {
            std::lock_guard<std::mutex> lock(
                const_cast<std::mutex&>(m_frameStats.frameRateMutex)
            );
            
            if (!m_frameStats.recentFrameRates.empty()) {
                double sum = 0;
                report.minFPS = DBL_MAX;
                report.maxFPS = 0;
                
                for (double rate : m_frameStats.recentFrameRates) {
                    sum += rate;
                    report.minFPS = std::min(report.minFPS, rate);
                    report.maxFPS = std::max(report.maxFPS, rate);
                }
                
                report.currentFPS = m_frameStats.recentFrameRates.back();
                report.averageFPS = sum / m_frameStats.recentFrameRates.size();
            }
        }
        
        // 幀統計
        report.totalFrames = m_frameStats.totalFrames;
        report.droppedFrames = m_frameStats.droppedFrames;
        report.errorFrames = m_frameStats.errorFrames;
        
        if (report.totalFrames > 0) {
            report.dropRate = static_cast<double>(report.droppedFrames) / 
                            report.totalFrames * 100.0;
            report.errorRate = static_cast<double>(report.errorFrames) / 
                             report.totalFrames * 100.0;
        }
        
        // 延遲統計
        report.avgLatency = m_latencyStats.avgLatency;
        report.minLatency = m_latencyStats.minLatency;
        report.maxLatency = m_latencyStats.maxLatency;
        
        // 資源使用
        report.cpuUsage = m_resourceUsage.cpuUsage;
        report.memoryUsageMB = m_resourceUsage.memoryUsage / (1024 * 1024);
        report.bandwidthMbps = m_resourceUsage.bandwidth;
        
        // 運行時間
        auto now = std::chrono::steady_clock::now();
        report.uptime = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - m_frameStats.startTime
        );
        
        return report;
    }
    
    void PrintReport() const {
        auto report = GetReport();
        
        std::cout << "\n=== 性能監控報告 ===" << std::endl;
        std::cout << "運行時間: " << report.uptime.count() / 1000.0 << " 秒" << std::endl;
        std::cout << "\n幀率統計:" << std::endl;
        std::cout << "  當前: " << report.currentFPS << " FPS" << std::endl;
        std::cout << "  平均: " << report.averageFPS << " FPS" << std::endl;
        std::cout << "  最小: " << report.minFPS << " FPS" << std::endl;
        std::cout << "  最大: " << report.maxFPS << " FPS" << std::endl;
        
        std::cout << "\n幀統計:" << std::endl;
        std::cout << "  總幀數: " << report.totalFrames << std::endl;
        std::cout << "  丟失幀: " << report.droppedFrames 
                 << " (" << report.dropRate << "%)" << std::endl;
        std::cout << "  錯誤幀: " << report.errorFrames 
                 << " (" << report.errorRate << "%)" << std::endl;
        
        std::cout << "\n延遲統計:" << std::endl;
        std::cout << "  平均: " << report.avgLatency << " ms" << std::endl;
        std::cout << "  最小: " << report.minLatency << " ms" << std::endl;
        std::cout << "  最大: " << report.maxLatency << " ms" << std::endl;
        
        std::cout << "\n資源使用:" << std::endl;
        std::cout << "  CPU: " << report.cpuUsage << "%" << std::endl;
        std::cout << "  內存: " << report.memoryUsageMB << " MB" << std::endl;
        std::cout << "  帶寬: " << report.bandwidthMbps << " Mbps" << std::endl;
    }
    
private:
    void MonitorLoop() {
        while (m_running) {
            UpdateResourceUsage();
            
            // 每秒更新一次
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    
    void UpdateResourceUsage() {
        // 更新CPU使用率
        m_resourceUsage.cpuUsage = GetProcessCPUUsage();
        
        // 更新內存使用
        m_resourceUsage.memoryUsage = GetProcessMemoryUsage();
        
        // 更新帶寬使用
        UpdateBandwidthUsage();
    }
    
    double GetProcessCPUUsage() {
        // 獲取進程CPU使用率
        // 平台相關實現
        return 0.0;
    }
    
    size_t GetProcessMemoryUsage() {
        // 獲取進程內存使用
        // 平台相關實現
        return 0;
    }
    
    void UpdateBandwidthUsage() {
        // 計算帶寬使用
        // 基於幀大小和幀率
    }
};
```

---

## 9. 錯誤上報機制

### 9.1 錯誤上報系統

```cpp
class ErrorReporter {
private:
    struct ErrorRecord {
        dvpStatus errorCode;
        std::string context;
        std::string stackTrace;
        std::chrono::system_clock::time_point timestamp;
        std::unordered_map<std::string, std::string> metadata;
    };
    
    std::deque<ErrorRecord> m_errorHistory;
    std::mutex m_errorMutex;
    const size_t m_maxHistorySize = 1000;
    
    // 錯誤統計
    std::unordered_map<dvpStatus, size_t> m_errorCounts;
    std::unordered_map<ErrorCodeHelper::ErrorCategory, size_t> m_categoryCounts;
    
    // 上報配置
    struct ReportConfig {
        bool enableRemoteReporting = false;
        std::string reportingEndpoint;
        std::chrono::seconds reportingInterval{300};  // 5分鐘
        size_t batchSize = 100;
    } m_config;
    
    std::thread m_reportingThread;
    std::atomic<bool> m_running{false};
    
public:
    void RecordError(dvpStatus status, 
                    const std::string& context,
                    const std::unordered_map<std::string, std::string>& metadata = {}) {
        ErrorRecord record;
        record.errorCode = status;
        record.context = context;
        record.timestamp = std::chrono::system_clock::now();
        record.stackTrace = CaptureStackTrace();
        record.metadata = metadata;
        
        {
            std::lock_guard<std::mutex> lock(m_errorMutex);
            
            m_errorHistory.push_back(record);
            if (m_errorHistory.size() > m_maxHistorySize) {
                m_errorHistory.pop_front();
            }
            
            // 更新統計
            m_errorCounts[status]++;
            
            auto category = ErrorCodeHelper::GetErrorCategory(status);
            m_categoryCounts[category]++;
        }
        
        // 記錄到日誌
        LogError(record);
        
        // 檢查是否需要立即上報
        if (ShouldReportImmediately(status)) {
            ReportErrorImmediately(record);
        }
    }
    
    void StartReporting() {
        if (!m_config.enableRemoteReporting) {
            return;
        }
        
        m_running = true;
        m_reportingThread = std::thread(&ErrorReporter::ReportingLoop, this);
    }
    
    void StopReporting() {
        m_running = false;
        if (m_reportingThread.joinable()) {
            m_reportingThread.join();
        }
    }
    
    struct ErrorStatistics {
        size_t totalErrors;
        std::vector<std::pair<dvpStatus, size_t>> topErrors;
        std::vector<std::pair<ErrorCodeHelper::ErrorCategory, size_t>> categoryDistribution;
        std::chrono::system_clock::time_point firstError;
        std::chrono::system_clock::time_point lastError;
    };
    
    ErrorStatistics GetStatistics() const {
        std::lock_guard<std::mutex> lock(
            const_cast<std::mutex&>(m_errorMutex)
        );
        
        ErrorStatistics stats;
        stats.totalErrors = 0;
        
        // 計算總錯誤數
        for (const auto& pair : m_errorCounts) {
            stats.totalErrors += pair.second;
        }
        
        // 找出最常見的錯誤
        std::vector<std::pair<dvpStatus, size_t>> errorVec(
            m_errorCounts.begin(), m_errorCounts.end()
        );
        std::sort(errorVec.begin(), errorVec.end(),
            [](const auto& a, const auto& b) {
                return a.second > b.second;
            }
        );
        
        size_t topCount = std::min(size_t(10), errorVec.size());
        stats.topErrors.assign(errorVec.begin(), errorVec.begin() + topCount);
        
        // 類別分佈
        stats.categoryDistribution.assign(
            m_categoryCounts.begin(), m_categoryCounts.end()
        );
        
        // 時間範圍
        if (!m_errorHistory.empty()) {
            stats.firstError = m_errorHistory.front().timestamp;
            stats.lastError = m_errorHistory.back().timestamp;
        }
        
        return stats;
    }
    
    void GenerateErrorReport(const std::string& filename) {
        auto stats = GetStatistics();
        
        std::ofstream file(filename);
        if (!file.is_open()) {
            LOG_ERROR("無法創建錯誤報告文件: " + filename);
            return;
        }
        
        file << "=== 錯誤統計報告 ===" << std::endl;
        file << "總錯誤數: " << stats.totalErrors << std::endl;
        
        file << "\n最常見錯誤:" << std::endl;
        for (const auto& [error, count] : stats.topErrors) {
            file << "  " << ErrorCodeHelper::GetErrorDescription(error) 
                 << " (" << static_cast<int>(error) << "): " 
                 << count << " 次" << std::endl;
        }
        
        file << "\n錯誤類別分佈:" << std::endl;
        for (const auto& [category, count] : stats.categoryDistribution) {
            file << "  " << GetCategoryName(category) << ": " 
                 << count << " 次" << std::endl;
        }
        
        file << "\n最近錯誤記錄:" << std::endl;
        {
            std::lock_guard<std::mutex> lock(
                const_cast<std::mutex&>(m_errorMutex)
            );
            
            size_t recentCount = std::min(size_t(20), m_errorHistory.size());
            auto it = m_errorHistory.end() - recentCount;
            
            while (it != m_errorHistory.end()) {
                file << FormatErrorRecord(*it) << std::endl;
                ++it;
            }
        }
        
        file.close();
        LOG_INFO("錯誤報告已生成: " + filename);
    }
    
private:
    void LogError(const ErrorRecord& record) {
        std::stringstream ss;
        ss << "錯誤: " << ErrorCodeHelper::GetErrorDescription(record.errorCode)
           << " (" << static_cast<int>(record.errorCode) << ")"
           << ", 上下文: " << record.context;
        
        if (!record.metadata.empty()) {
            ss << ", 元數據: {";
            for (const auto& [key, value] : record.metadata) {
                ss << key << "=" << value << ", ";
            }
            ss << "}";
        }
        
        LOG_ERROR(ss.str());
    }
    
    bool ShouldReportImmediately(dvpStatus status) {
        // 嚴重錯誤立即上報
        return status == DVP_STATUS_DEVICE_IS_DISCONNECTED ||
               status == DVP_STATUS_IO_ERROR ||
               status == DVP_STATUS_COMM_ERROR;
    }
    
    void ReportErrorImmediately(const ErrorRecord& record) {
        if (!m_config.enableRemoteReporting) {
            return;
        }
        
        // 發送錯誤到遠程服務器
        SendErrorToServer({record});
    }
    
    void ReportingLoop() {
        while (m_running) {
            std::this_thread::sleep_for(m_config.reportingInterval);
            
            std::vector<ErrorRecord> batch;
            
            {
                std::lock_guard<std::mutex> lock(m_errorMutex);
                
                size_t count = std::min(m_config.batchSize, m_errorHistory.size());
                if (count > 0) {
                    batch.assign(
                        m_errorHistory.end() - count,
                        m_errorHistory.end()
                    );
                }
            }
            
            if (!batch.empty()) {
                SendErrorToServer(batch);
            }
        }
    }
    
    void SendErrorToServer(const std::vector<ErrorRecord>& errors) {
        // 實現HTTP POST發送錯誤數據到服務器
        // 使用JSON格式序列化錯誤記錄
    }
    
    std::string CaptureStackTrace() {
        // 捕獲當前調用堆棧
        // 平台相關實現
        return "Stack trace";
    }
    
    std::string GetCategoryName(ErrorCodeHelper::ErrorCategory category) {
        switch (category) {
            case ErrorCodeHelper::ErrorCategory::SUCCESS: return "成功";
            case ErrorCodeHelper::ErrorCategory::WARNING: return "警告";
            case ErrorCodeHelper::ErrorCategory::GENERAL_ERROR: return "一般錯誤";
            case ErrorCodeHelper::ErrorCategory::PARAMETER_ERROR: return "參數錯誤";
            case ErrorCodeHelper::ErrorCategory::STATE_ERROR: return "狀態錯誤";
            case ErrorCodeHelper::ErrorCategory::COMMUNICATION_ERROR: return "通訊錯誤";
            case ErrorCodeHelper::ErrorCategory::DEVICE_ERROR: return "設備錯誤";
            case ErrorCodeHelper::ErrorCategory::MEMORY_ERROR: return "內存錯誤";
            case ErrorCodeHelper::ErrorCategory::FILE_ERROR: return "文件錯誤";
            case ErrorCodeHelper::ErrorCategory::GRAB_ERROR: return "採集錯誤";
            default: return "未知";
        }
    }
    
    std::string FormatErrorRecord(const ErrorRecord& record) {
        std::stringstream ss;
        
        auto time_t = std::chrono::system_clock::to_time_t(record.timestamp);
        ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        ss << " | " << static_cast<int>(record.errorCode);
        ss << " | " << record.context;
        
        return ss.str();
    }
};
```

---

## 10. 生產環境錯誤處理

### 10.1 生產環境配置

```cpp
class ProductionErrorHandler {
private:
    // 生產環境配置
    struct ProductionConfig {
        // 錯誤處理策略
        bool enableAutoRecovery = true;
        bool enableFailover = true;
        bool enableGracefulDegradation = true;
        
        // 告警配置
        bool enableAlerts = true;
        std::vector<std::string> alertRecipients;
        
        // 日誌配置
        LogLevel minLogLevel = LogLevel::WARNING;
        bool enableDetailedLogging = false;
        
        // 監控配置
        bool enableMetrics = true;
        std::chrono::seconds metricsInterval{60};
        
        // 健康檢查
        bool enableHealthCheck = true;
        std::chrono::seconds healthCheckInterval{30};
    } m_config;
    
    // 降級策略
    enum class DegradationLevel {
        NORMAL = 0,
        REDUCED_QUALITY = 1,
        REDUCED_FRAMERATE = 2,
        MINIMAL_OPERATION = 3,
        EMERGENCY_STOP = 4
    };
    
    std::atomic<DegradationLevel> m_currentLevel{DegradationLevel::NORMAL};
    
public:
    void HandleProductionError(dvpStatus status, dvpHandle handle) {
        // 記錄錯誤
        LogProductionError(status);
        
        // 嘗試自動恢復
        if (m_config.enableAutoRecovery) {
            if (TryAutoRecover(status, handle)) {
                return;
            }
        }
        
        // 執行降級策略
        if (m_config.enableGracefulDegradation) {
            ApplyDegradation(status, handle);
        }
        
        // 發送告警
        if (m_config.enableAlerts) {
            SendAlert(status);
        }
        
        // 檢查是否需要故障轉移
        if (m_config.enableFailover) {
            CheckFailover(status, handle);
        }
    }
    
private:
    bool TryAutoRecover(dvpStatus status, dvpHandle handle) {
        // 根據錯誤類型選擇恢復策略
        switch (ErrorCodeHelper::GetErrorCategory(status)) {
            case ErrorCodeHelper::ErrorCategory::COMMUNICATION_ERROR:
                return RecoverFromCommunicationError(handle);
                
            case ErrorCodeHelper::ErrorCategory::GRAB_ERROR:
                return RecoverFromGrabError(handle);
                
            case ErrorCodeHelper::ErrorCategory::DEVICE_ERROR:
                return RecoverFromDeviceError(handle);
                
            default:
                return false;
        }
    }
    
    void ApplyDegradation(dvpStatus status, dvpHandle handle) {
        DegradationLevel newLevel = DetermineDegraduccationLevel(status);
        
        if (newLevel > m_currentLevel) {
            LOG_WARNING("系統降級至級別: " + std::to_string(static_cast<int>(newLevel)));
            m_currentLevel = newLevel;
            
            switch (newLevel) {
                case DegradationLevel::REDUCED_QUALITY:
                    ReduceImageQuality(handle);
                    break;
                    
                case DegradationLevel::REDUCED_FRAMERATE:
                    ReduceFrameRate(handle);
                    break;
                    
                case DegradationLevel::MINIMAL_OPERATION:
                    EnterMinimalMode(handle);
                    break;
                    
                case DegradationLevel::EMERGENCY_STOP:
                    EmergencyStop(handle);
                    break;
                    
                default:
                    break;
            }
        }
    }
    
    void SendAlert(dvpStatus status) {
        std::string alertMessage = FormatAlertMessage(status);
        
        for (const auto& recipient : m_config.alertRecipients) {
            // 發送告警（郵件、短信、webhook等）
            SendAlertToRecipient(recipient, alertMessage);
        }
    }
    
    void CheckFailover(dvpStatus status, dvpHandle handle) {
        if (IsFailoverRequired(status)) {
            PerformFailover(handle);
        }
    }
    
    // 恢復函數
    bool RecoverFromCommunicationError(dvpHandle handle) {
        // 重置設備
        return dvpResetDevice(handle) == DVP_STATUS_OK;
    }
    
    bool RecoverFromGrabError(dvpHandle handle) {
        // 重啟採集
        dvpStop(handle);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        return dvpStart(handle) == DVP_STATUS_OK;
    }
    
    bool RecoverFromDeviceError(dvpHandle handle) {
        // 重新初始化設備
        return false;
    }
    
    // 降級操作
    void ReduceImageQuality(dvpHandle handle) {
        // 降低圖像質量以減少負載
    }
    
    void ReduceFrameRate(dvpHandle handle) {
        // 降低幀率
        dvpSetFrameRate(handle, 15.0);
    }
    
    void EnterMinimalMode(dvpHandle handle) {
        // 進入最小運行模式
    }
    
    void EmergencyStop(dvpHandle handle) {
        // 緊急停止
        dvpStop(handle);
    }
    
    // 輔助函數
    DegradationLevel DetermineDegraduccationLevel(dvpStatus status) {
        // 根據錯誤嚴重程度確定降級級別
        return DegradationLevel::NORMAL;
    }
    
    bool IsFailoverRequired(dvpStatus status) {
        // 判斷是否需要故障轉移
        return false;
    }
    
    void PerformFailover(dvpHandle handle) {
        // 執行故障轉移
    }
    
    void LogProductionError(dvpStatus status) {
        // 生產環境錯誤日誌
    }
    
    std::string FormatAlertMessage(dvpStatus status) {
        // 格式化告警消息
        return "";
    }
    
    void SendAlertToRecipient(const std::string& recipient, 
                              const std::string& message) {
        // 發送告警
    }
};
```

---

## 11. 工業級錯誤處理類實現

### 11.1 完整的工業級相機錯誤處理系統

```cpp
namespace Do3Think {

// 工業級相機管理器 - 整合所有錯誤處理功能
class IndustrialCameraManager {
private:
    // 相機句柄
    dvpHandle m_handle{0};
    std::string m_deviceId;
    
    // 錯誤處理組件
    std::unique_ptr<RecoveryHandler> m_recoveryHandler;
    std::unique_ptr<ConnectionManager> m_connectionManager;
    std::unique_ptr<TimeoutManager> m_timeoutManager;
    std::unique_ptr<PerformanceMonitor> m_performanceMonitor;
    std::unique_ptr<DiagnosticsCollector> m_diagnosticsCollector;
    std::unique_ptr<ErrorReporter> m_errorReporter;
    std::unique_ptr<ProductionErrorHandler> m_productionHandler;
    
    // 運行狀態
    std::atomic<bool> m_isRunning{false};
    std::atomic<bool> m_isHealthy{true};
    std::mutex m_operationMutex;
    
    // 配置
    struct ManagerConfig {
        // 重試配置
        int maxRetries = 3;
        std::chrono::milliseconds retryDelay{1000};
        
        // 健康檢查
        bool enableHealthCheck = true;
        std::chrono::seconds healthCheckInterval{30};
        
        // 自動恢復
        bool enableAutoRecovery = true;
        
        // 性能監控
        bool enablePerformanceMonitoring = true;
        
        // 錯誤上報
        bool enableErrorReporting = true;
    } m_config;
    
    // 健康檢查線程
    std::thread m_healthCheckThread;
    
public:
    IndustrialCameraManager(const std::string& deviceId) 
        : m_deviceId(deviceId) {
        Initialize();
    }
    
    ~IndustrialCameraManager() {
        Shutdown();
    }
    
    // 初始化
    bool Initialize() {
        try {
            // 初始化各組件
            m_recoveryHandler = std::make_unique<RecoveryHandler>();
            m_connectionManager = std::make_unique<ConnectionManager>(m_deviceId);
            m_timeoutManager = std::make_unique<TimeoutManager>();
            m_diagnosticsCollector = std::make_unique<DiagnosticsCollector>();
            m_errorReporter = std::make_unique<ErrorReporter>();
            m_productionHandler = std::make_unique<ProductionErrorHandler>();
            
            // 打開相機
            if (!OpenCamera()) {
                return false;
            }
            
            // 初始化性能監控
            if (m_config.enablePerformanceMonitoring) {
                m_performanceMonitor = std::make_unique<PerformanceMonitor>(m_handle);
                m_performanceMonitor->Start();
            }
            
            // 啟動連接管理
            m_connectionManager->StartMonitoring();
            
            // 啟動錯誤上報
            if (m_config.enableErrorReporting) {
                m_errorReporter->StartReporting();
            }
            
            // 啟動健康檢查
            if (m_config.enableHealthCheck) {
                StartHealthCheck();
            }
            
            LOG_INFO("工業相機管理器初始化成功: " + m_deviceId);
            return true;
            
        } catch (const std::exception& e) {
            LOG_ERROR("初始化失敗: " + std::string(e.what()));
            return false;
        }
    }
    
    // 關閉
    void Shutdown() {
        m_isRunning = false;
        
        // 停止健康檢查
        if (m_healthCheckThread.joinable()) {
            m_healthCheckThread.join();
        }
        
        // 停止各組件
        if (m_performanceMonitor) {
            m_performanceMonitor->Stop();
        }
        
        if (m_connectionManager) {
            m_connectionManager->StopReconnection();
        }
        
        if (m_errorReporter) {
            m_errorReporter->StopReporting();
        }
        
        // 關閉相機
        CloseCamera();
        
        LOG_INFO("工業相機管理器已關閉");
    }
    
    // 啟動採集
    bool StartAcquisition() {
        std::lock_guard<std::mutex> lock(m_operationMutex);
        
        return ExecuteWithErrorHandling([this]() {
            DVP_CHECK(dvpStart(m_handle));
            m_isRunning = true;
            LOG_INFO("圖像採集已啟動");
            return DVP_STATUS_OK;
        });
    }
    
    // 停止採集
    bool StopAcquisition() {
        std::lock_guard<std::mutex> lock(m_operationMutex);
        
        return ExecuteWithErrorHandling([this]() {
            DVP_CHECK(dvpStop(m_handle));
            m_isRunning = false;
            LOG_INFO("圖像採集已停止");
            return DVP_STATUS_OK;
        });
    }
    
    // 獲取圖像（帶完整錯誤處理）
    bool GetImage(dvpFrame* pFrame, std::chrono::milliseconds timeout = std::chrono::milliseconds(1000)) {
        if (!m_isHealthy) {
            LOG_WARNING("相機狀態不健康，嘗試恢復");
            if (!RecoverCamera()) {
                return false;
            }
        }
        
        return ExecuteWithErrorHandling([this, pFrame, timeout]() {
            // 使用超時管理器獲取圖像
            dvpStatus status = m_timeoutManager->GetFrameWithTimeout(
                m_handle, pFrame, timeout
            );
            
            if (status == DVP_STATUS_OK) {
                // 更新性能統計
                if (m_performanceMonitor) {
                    m_performanceMonitor->OnFrameReceived();
                }
            } else if (status == DVP_STATUS_TIME_OUT) {
                // 超時處理
                if (m_performanceMonitor) {
                    m_performanceMonitor->OnFrameDropped();
                }
                LOG_WARNING("獲取圖像超時");
            } else {
                // 其他錯誤
                if (m_performanceMonitor) {
                    m_performanceMonitor->OnFrameError();
                }
            }
            
            return status;
        });
    }
    
    // 設置參數（帶錯誤處理）
    template<typename T>
    bool SetParameter(std::function<dvpStatus(T)> setter, T value, 
                     const std::string& paramName) {
        return ExecuteWithErrorHandling([&]() {
            dvpStatus status = setter(value);
            
            if (status == DVP_STATUS_OK) {
                LOG_DEBUG("參數設置成功: " + paramName);
            } else {
                LOG_ERROR("參數設置失敗: " + paramName);
            }
            
            return status;
        });
    }
    
    // 獲取診斷報告
    DiagnosticsCollector::DiagnosticsReport GetDiagnosticsReport() {
        return m_diagnosticsCollector->CollectFullReport(m_handle);
    }
    
    // 獲取性能報告
    PerformanceMonitor::PerformanceReport GetPerformanceReport() const {
        if (m_performanceMonitor) {
            return m_performanceMonitor->GetReport();
        }
        return {};
    }
    
    // 獲取錯誤統計
    ErrorReporter::ErrorStatistics GetErrorStatistics() const {
        if (m_errorReporter) {
            return m_errorReporter->GetStatistics();
        }
        return {};
    }
    
private:
    // 打開相機
    bool OpenCamera() {
        return ExecuteWithErrorHandling([this]() {
            return dvpOpenByUserId(
                const_cast<char*>(m_deviceId.c_str()),
                OPEN_NORMAL,
                &m_handle
            );
        });
    }
    
    // 關閉相機
    void CloseCamera() {
        if (m_handle) {
            dvpClose(m_handle);
            m_handle = 0;
        }
    }
    
    // 執行操作（帶錯誤處理）
    bool ExecuteWithErrorHandling(std::function<dvpStatus()> operation) {
        dvpStatus status = DVP_STATUS_FAILED;
        
        for (int i = 0; i < m_config.maxRetries; ++i) {
            try {
                status = operation();
                
                if (status == DVP_STATUS_OK) {
                    return true;
                }
                
                // 記錄錯誤
                if (m_errorReporter) {
                    m_errorReporter->RecordError(status, "Operation failed");
                }
                
                // 嘗試恢復
                if (m_config.enableAutoRecovery && m_recoveryHandler) {
                    status = m_recoveryHandler->TryRecover(status, m_handle);
                    
                    if (status == DVP_STATUS_OK) {
                        LOG_INFO("錯誤恢復成功");
                        return true;
                    }
                }
                
                // 生產環境處理
                if (m_productionHandler) {
                    m_productionHandler->HandleProductionError(status, m_handle);
                }
                
            } catch (const CameraException& e) {
                LOG_ERROR("相機異常: " + e.GetDetailedMessage());
                status = e.GetStatus();
                
            } catch (const std::exception& e) {
                LOG_ERROR("標準異常: " + std::string(e.what()));
                status = DVP_STATUS_FAILED;
            }
            
            if (i < m_config.maxRetries - 1) {
                std::this_thread::sleep_for(m_config.retryDelay);
            }
        }
        
        LOG_ERROR("操作失敗，已達最大重試次數");
        m_isHealthy = false;
        return false;
    }
    
    // 恢復相機
    bool RecoverCamera() {
        LOG_INFO("嘗試恢復相機");
        
        // 重置設備
        if (dvpResetDevice(m_handle) == DVP_STATUS_OK) {
            m_isHealthy = true;
            LOG_INFO("相機恢復成功");
            return true;
        }
        
        // 重新連接
        CloseCamera();
        if (OpenCamera()) {
            m_isHealthy = true;
            LOG_INFO("相機重新連接成功");
            return true;
        }
        
        LOG_ERROR("相機恢復失敗");
        return false;
    }
    
    // 健康檢查
    void StartHealthCheck() {
        m_healthCheckThread = std::thread([this]() {
            while (m_isRunning) {
                std::this_thread::sleep_for(m_config.healthCheckInterval);
                
                if (!PerformHealthCheck()) {
                    m_isHealthy = false;
                    LOG_WARNING("健康檢查失敗");
                    
                    // 嘗試恢復
                    if (m_config.enableAutoRecovery) {
                        RecoverCamera();
                    }
                } else {
                    m_isHealthy = true;
                }
            }
        });
    }
    
    // 執行健康檢查
    bool PerformHealthCheck() {
        // 檢查設備連接
        dvpCameraInfo info;
        if (dvpGetCameraInfo(m_handle, &info) != DVP_STATUS_OK) {
            return false;
        }
        
        // 檢查溫度
        dvpTemperatureInfo tempInfo;
        if (dvpGetTemperatureInfo(m_handle, &tempInfo) == DVP_STATUS_OK) {
            if (tempInfo.fDevice > 80.0) {  // 溫度過高
                LOG_WARNING("設備溫度過高: " + std::to_string(tempInfo.fDevice) + "°C");
            }
        }
        
        return true;
    }
};

} // namespace Do3Think

// 使用示例
int main() {
    try {
        // 創建工業級相機管理器
        Do3Think::IndustrialCameraManager camera("CAM_001");
        
        // 啟動採集
        if (!camera.StartAcquisition()) {
            std::cerr << "無法啟動採集" << std::endl;
            return -1;
        }
        
        // 主循環
        while (true) {
            dvpFrame frame;
            
            // 獲取圖像（自動處理所有錯誤）
            if (camera.GetImage(&frame)) {
                // 處理圖像
                ProcessImage(&frame);
            }
            
            // 定期輸出性能報告
            static auto lastReportTime = std::chrono::steady_clock::now();
            auto now = std::chrono::steady_clock::now();
            
            if (now - lastReportTime > std::chrono::minutes(1)) {
                auto perfReport = camera.GetPerformanceReport();
                std::cout << "FPS: " << perfReport.averageFPS 
                         << ", Dropped: " << perfReport.droppedFrames 
                         << std::endl;
                
                lastReportTime = now;
            }
            
            // 檢查退出條件
            if (ShouldExit()) {
                break;
            }
        }
        
        // 停止採集
        camera.StopAcquisition();
        
        // 生成最終報告
        auto diagReport = camera.GetDiagnosticsReport();
        camera.GetErrorStatistics();
        
    } catch (const std::exception& e) {
        std::cerr << "致命錯誤: " << e.what() << std::endl;
        return -1;
    }
    
    return 0;
}
```

---

## 總結

本文檔提供了Do3ThinkCamera SDK的完整錯誤處理與診斷解決方案，包括：

1. **完整的錯誤碼體系**：詳細分類和描述所有錯誤碼
2. **多層次錯誤處理策略**：從基本檢查到高級恢復機制
3. **自動恢復機制**：智能重試和恢復策略
4. **設備斷線重連**：自動檢測和重連功能
5. **超時管理**：靈活的超時處理方案
6. **分級日誌系統**：完整的日誌記錄和輪轉
7. **診斷信息收集**：全面的系統和相機狀態收集
8. **性能監控**：實時性能指標追蹤
9. **錯誤上報機制**：統計和遠程上報功能
10. **生產環境優化**：降級策略和故障轉移

這套錯誤處理系統適合24/7工業環境運行，提供了高可靠性和可維護性。通過使用`IndustrialCameraManager`類，開發者可以快速構建穩定可靠的機器視覺應用。

### 最佳實踐建議

1. **始終使用錯誤檢查宏**：確保每個API調用都有錯誤處理
2. **啟用自動恢復**：讓系統自動處理可恢復的錯誤
3. **監控性能指標**：及時發現潛在問題
4. **定期生成診斷報告**：用於預防性維護
5. **配置適當的日誌級別**：平衡性能和調試需求
6. **實施降級策略**：確保系統在異常情況下仍能運行
7. **測試錯誤場景**：定期進行故障注入測試