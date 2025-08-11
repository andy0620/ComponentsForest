# Do3ThinkCamera 圖像獲取與處理開發文檔

## 1. 概述

Do3ThinkCamera SDK提供了強大的圖像獲取與處理能力，支援同步/異步獲取模式、回調機制、高速緩衝管理等特性，特別適合工業AOI（自動光學檢測）的高速圖像獲取需求（100-1000fps）。

## 2. 核心API介紹

### 2.1 圖像獲取API

```cpp
// 同步獲取圖像
dvpStatus dvpGetFrame(
    dvpHandle handle,        // 相機句柄
    dvpFrame* pFrame,       // 幀信息結構
    void** pBuffer,         // 圖像數據緩衝區
    dvpUint32 timeout       // 超時時間（毫秒）
);

// 同時獲取原始幀和處理後的幀
dvpStatus dvpGetFrameBuffer(
    dvpHandle handle,
    dvpFrameBuffer* pRaw,   // 原始幀
    dvpFrameBuffer* pOut,   // 處理後的幀
    dvpUint32 timeout
);

// 註冊流回調函數
dvpStatus dvpRegisterStreamCallback(
    dvpHandle handle,
    dvpStreamCallback proc,  // 回調函數
    dvpStreamEvent event,    // 事件類型
    void* pContext          // 用戶上下文
);

// 處理圖像
dvpStatus dvpProcessImage(
    dvpHandle handle,
    const dvpFrame* pSourceFrame,
    const void* pSourceBuffer,
    dvpFrame* pTargetFrame,
    void* pTargetBuffer,
    dvpUint32 targetBufferSize,
    dvpStreamFormat targetFormat
);
```

### 2.2 圖像操作API

```cpp
// 繪製圖像到窗口
dvpStatus dvpDrawPicture(
    const dvpFrame* pFrame,
    const void* pBuffer,
    HWND hWnd,
    LPRECT sRect,
    LPRECT dRect
);

// 保存圖像到文件
dvpStatus dvpSavePicture(
    const dvpFrame* pFrame,
    const void* pBuffer,
    dvpStr file,
    dvpInt32 quality        // JPEG質量(1-100)
);

// 設置緩衝隊列大小
dvpStatus dvpSetBufferQueueSize(
    dvpHandle handle,
    dvpInt32 BufferQueueSize
);
```

## 3. 重要數據結構

### 3.1 dvpFrame - 幀信息結構

```cpp
typedef struct dvpFrame {
    dvpImageFormat format;      // 圖像格式
    dvpBits bits;              // 位深度
    dvpUint32 uBytes;          // 字節數
    dvpInt32 iWidth;           // 寬度
    dvpInt32 iHeight;          // 高度
    dvpUint64 uFrameID;        // 幀編號
    dvpUint64 uTimestamp;      // 時間戳
    double fExposure;          // 曝光時間(μs)
    float fAGain;              // 模擬增益
    dvpFirstPosition position;  // 第一個像素位置
    bool bFlipHorizontalState; // 水平翻轉
    bool bFlipVerticalState;   // 垂直翻轉
    bool bRotateState;         // 旋轉90度
    bool bRotateOpposite;      // 逆時針旋轉
    dvpUint64 uTriggerId;      // 觸發ID
    dvpUint64 userValue;       // 用戶自定義數據
    // ... 其他成員
} dvpFrame;
```

### 3.2 dvpImageFormat - 圖像格式

```cpp
typedef enum dvpImageFormat {
    FORMAT_MONO = 0,        // 黑白圖像
    FORMAT_BAYER_BG = 1,    // Bayer BGGR
    FORMAT_BAYER_GB = 2,    // Bayer GBRG
    FORMAT_BAYER_GR = 3,    // Bayer GRBG
    FORMAT_BAYER_RG = 4,    // Bayer RGGB
    FORMAT_BGR24 = 10,      // BGR 24位
    FORMAT_BGR32 = 11,      // BGRA 32位
    FORMAT_RGB24 = 14,      // RGB 24位
    FORMAT_RGB32 = 15,      // RGBA 32位
    // ... 更多格式
} dvpImageFormat;
```

### 3.3 dvpStreamEvent - 流事件類型

```cpp
typedef enum dvpStreamEvent {
    STREAM_EVENT_ARRIVED = 0,      // 圖像到達
    STREAM_EVENT_CORRECTED = 1,    // 圖像校正後
    STREAM_EVENT_PROCESSED = 2,    // 圖像處理後
    STREAM_EVENT_FRAME_THREAD = 3  // 專用線程模式
} dvpStreamEvent;
```

## 4. 圖像獲取模式對比

### 4.1 同步模式 - dvpGetFrame

**特點：**
- 阻塞式調用，直到獲取到圖像或超時
- 簡單直接，適合單線程應用
- 易於控制流程

**示例代碼：**

```cpp
class SyncImageAcquisition {
private:
    dvpHandle m_handle;
    std::atomic<bool> m_running;
    
public:
    void StartAcquisition() {
        dvpStatus status;
        dvpFrame frame;
        void* pBuffer = nullptr;
        const dvpUint32 timeout = 3000; // 3秒超時
        
        // 啟動視頻流
        status = dvpStart(m_handle);
        if (status != DVP_STATUS_OK) {
            return;
        }
        
        m_running = true;
        while (m_running) {
            // 同步獲取圖像
            status = dvpGetFrame(m_handle, &frame, &pBuffer, timeout);
            
            if (status == DVP_STATUS_OK) {
                // 處理圖像
                ProcessImage(&frame, pBuffer);
                
                // 顯示幀信息
                std::cout << "Frame ID: " << frame.uFrameID 
                         << ", Size: " << frame.iWidth << "x" << frame.iHeight
                         << ", Exposure: " << frame.fExposure << "us"
                         << std::endl;
            } else if (status == DVP_STATUS_FRAME_LOST) {
                std::cerr << "Frame lost!" << std::endl;
            } else if (status == DVP_STATUS_TIME_OUT) {
                std::cerr << "Timeout waiting for frame!" << std::endl;
            }
        }
        
        // 停止視頻流
        dvpStop(m_handle);
    }
    
    void ProcessImage(const dvpFrame* frame, void* buffer) {
        // 圖像處理邏輯
    }
};
```

### 4.2 異步模式 - 回調函數

**特點：**
- 非阻塞，事件驅動
- 高效率，低延遲
- 適合多線程和高速應用

**示例代碼：**

```cpp
class AsyncImageAcquisition {
private:
    dvpHandle m_handle;
    std::atomic<uint64_t> m_frameCount;
    std::mutex m_mutex;
    
    // 靜態回調函數
    static int DVP_CALL OnStreamCallback(
        dvpHandle handle,
        dvpStreamEvent event,
        void* pContext,
        dvpFrame* pFrame,
        void* pBuffer
    ) {
        AsyncImageAcquisition* pThis = (AsyncImageAcquisition*)pContext;
        return pThis->HandleFrame(handle, event, pFrame, pBuffer);
    }
    
    // 實例方法處理幀
    int HandleFrame(dvpHandle handle, dvpStreamEvent event, 
                   dvpFrame* pFrame, void* pBuffer) {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        m_frameCount++;
        
        switch(event) {
            case STREAM_EVENT_ARRIVED:
                // 原始圖像到達
                OnRawFrame(pFrame, pBuffer);
                break;
                
            case STREAM_EVENT_PROCESSED:
                // 處理後的圖像
                OnProcessedFrame(pFrame, pBuffer);
                break;
                
            case STREAM_EVENT_FRAME_THREAD:
                // 專用線程模式
                OnDedicatedThread(pFrame, pBuffer);
                break;
        }
        
        return 1; // 繼續接收
    }
    
public:
    bool Initialize() {
        dvpStatus status;
        
        // 註冊回調函數 - 處理後的圖像
        status = dvpRegisterStreamCallback(
            m_handle, 
            OnStreamCallback,
            STREAM_EVENT_PROCESSED,
            this
        );
        
        if (status != DVP_STATUS_OK) {
            return false;
        }
        
        // 可以註冊多個回調
        status = dvpRegisterStreamCallback(
            m_handle,
            OnStreamCallback,
            STREAM_EVENT_FRAME_THREAD,
            this
        );
        
        return status == DVP_STATUS_OK;
    }
    
    void OnRawFrame(dvpFrame* pFrame, void* pBuffer) {
        // 處理原始幀
    }
    
    void OnProcessedFrame(dvpFrame* pFrame, void* pBuffer) {
        // 處理已處理的幀
    }
    
    void OnDedicatedThread(dvpFrame* pFrame, void* pBuffer) {
        // 專用線程處理
    }
};
```

## 5. 幀緩衝管理策略

### 5.1 緩衝隊列配置

```cpp
class BufferQueueManager {
private:
    dvpHandle m_handle;
    
public:
    bool ConfigureBufferQueue(int queueSize = 4) {
        dvpStatus status;
        
        // 設置緩衝隊列大小
        status = dvpSetBufferQueueSize(m_handle, queueSize);
        if (status != DVP_STATUS_OK) {
            return false;
        }
        
        // 配置緩衝策略
        dvpBufferConfig config;
        config.mode = BUFFER_MODE_CYCLE;  // 循環模式
        config.count = queueSize;
        config.reserved = 0;
        
        status = dvpSetBufferConfig(m_handle, &config);
        
        return status == DVP_STATUS_OK;
    }
    
    // 獲取原始幀和處理幀
    bool GetDualFrames(dvpFrameBuffer& raw, dvpFrameBuffer& processed) {
        dvpStatus status = dvpGetFrameBuffer(
            m_handle, 
            &raw, 
            &processed, 
            3000
        );
        
        return status == DVP_STATUS_OK;
    }
};
```

### 5.2 零拷貝優化

```cpp
class ZeroCopyImageProcessor {
private:
    struct FramePool {
        std::vector<std::unique_ptr<uint8_t[]>> buffers;
        std::queue<uint8_t*> available;
        std::mutex mutex;
        
        FramePool(size_t count, size_t size) {
            for (size_t i = 0; i < count; ++i) {
                auto buffer = std::make_unique<uint8_t[]>(size);
                available.push(buffer.get());
                buffers.push_back(std::move(buffer));
            }
        }
        
        uint8_t* Acquire() {
            std::lock_guard<std::mutex> lock(mutex);
            if (available.empty()) return nullptr;
            
            uint8_t* buffer = available.front();
            available.pop();
            return buffer;
        }
        
        void Release(uint8_t* buffer) {
            std::lock_guard<std::mutex> lock(mutex);
            available.push(buffer);
        }
    };
    
    std::unique_ptr<FramePool> m_pool;
    
public:
    void Initialize(size_t poolSize, size_t frameSize) {
        m_pool = std::make_unique<FramePool>(poolSize, frameSize);
    }
    
    bool ProcessFrameZeroCopy(dvpHandle handle) {
        dvpFrame frame;
        void* pBuffer = nullptr;
        
        // 獲取幀（SDK內部緩衝）
        dvpStatus status = dvpGetFrame(handle, &frame, &pBuffer, 1000);
        if (status != DVP_STATUS_OK) {
            return false;
        }
        
        // 直接在原始緩衝區上處理（零拷貝）
        ProcessInPlace(&frame, pBuffer);
        
        // 或者使用預分配的緩衝池
        uint8_t* poolBuffer = m_pool->Acquire();
        if (poolBuffer) {
            // 處理到池緩衝區
            dvpFrame targetFrame = frame;
            dvpProcessImage(
                handle,
                &frame,
                pBuffer,
                &targetFrame,
                poolBuffer,
                frame.uBytes,
                S_RAW8
            );
            
            // 異步處理
            std::async(std::launch::async, [this, poolBuffer, targetFrame]() {
                // 處理邏輯
                ProcessAsync(&targetFrame, poolBuffer);
                // 釋放回池
                m_pool->Release(poolBuffer);
            });
        }
        
        return true;
    }
    
    void ProcessInPlace(dvpFrame* frame, void* buffer) {
        // 直接在原始緩衝區處理
    }
    
    void ProcessAsync(const dvpFrame* frame, void* buffer) {
        // 異步處理邏輯
    }
};
```

## 6. 圖像格式轉換

### 6.1 Bayer到RGB轉換

```cpp
class FormatConverter {
private:
    dvpHandle m_handle;
    
public:
    bool ConvertBayerToRGB(const dvpFrame* srcFrame, void* srcBuffer,
                           dvpFrame* dstFrame, void* dstBuffer) {
        // 設置目標格式
        dvpStreamFormat targetFormat = S_RGB24;
        
        // 計算目標緩衝區大小
        dvpUint32 targetSize = srcFrame->iWidth * srcFrame->iHeight * 3;
        
        // 執行格式轉換
        dvpStatus status = dvpProcessImage(
            m_handle,
            srcFrame,
            srcBuffer,
            dstFrame,
            dstBuffer,
            targetSize,
            targetFormat
        );
        
        return status == DVP_STATUS_OK;
    }
    
    // 支援多種格式轉換
    bool ConvertFormat(const dvpFrame* src, void* srcBuf,
                      dvpFrame* dst, void* dstBuf,
                      dvpImageFormat targetFormat) {
        dvpStreamFormat streamFormat;
        
        // 映射圖像格式到流格式
        switch(targetFormat) {
            case FORMAT_MONO:
                streamFormat = S_RAW8;
                break;
            case FORMAT_BGR24:
                streamFormat = S_BGR24;
                break;
            case FORMAT_RGB24:
                streamFormat = S_RGB24;
                break;
            case FORMAT_BGR32:
                streamFormat = S_BGR32;
                break;
            default:
                return false;
        }
        
        dvpUint32 pixelSize = GetPixelSize(targetFormat);
        dvpUint32 bufferSize = src->iWidth * src->iHeight * pixelSize;
        
        dvpStatus status = dvpProcessImage(
            m_handle,
            src,
            srcBuf,
            dst,
            dstBuf,
            bufferSize,
            streamFormat
        );
        
        return status == DVP_STATUS_OK;
    }
    
private:
    dvpUint32 GetPixelSize(dvpImageFormat format) {
        switch(format) {
            case FORMAT_MONO: return 1;
            case FORMAT_BGR24:
            case FORMAT_RGB24: return 3;
            case FORMAT_BGR32:
            case FORMAT_RGB32: return 4;
            default: return 0;
        }
    }
};
```

## 7. 高效率圖像處理流程

### 7.1 流水線處理架構

```cpp
class ImageProcessingPipeline {
private:
    struct PipelineStage {
        std::string name;
        std::function<void(dvpFrame*, void*)> processor;
        std::thread worker;
        std::queue<std::pair<dvpFrame, std::vector<uint8_t>>> queue;
        std::mutex mutex;
        std::condition_variable cv;
        std::atomic<bool> running;
    };
    
    std::vector<std::unique_ptr<PipelineStage>> m_stages;
    dvpHandle m_handle;
    
public:
    void AddStage(const std::string& name, 
                  std::function<void(dvpFrame*, void*)> processor) {
        auto stage = std::make_unique<PipelineStage>();
        stage->name = name;
        stage->processor = processor;
        stage->running = true;
        
        // 啟動工作線程
        stage->worker = std::thread([this, &stage = *stage]() {
            while (stage.running) {
                std::unique_lock<std::mutex> lock(stage.mutex);
                stage.cv.wait(lock, [&stage]() { 
                    return !stage.queue.empty() || !stage.running; 
                });
                
                if (!stage.queue.empty()) {
                    auto [frame, buffer] = std::move(stage.queue.front());
                    stage.queue.pop();
                    lock.unlock();
                    
                    // 處理
                    stage.processor(&frame, buffer.data());
                }
            }
        });
        
        m_stages.push_back(std::move(stage));
    }
    
    void ProcessFrame(dvpFrame* frame, void* buffer) {
        if (m_stages.empty()) return;
        
        // 複製數據到第一個階段
        std::vector<uint8_t> data(frame->uBytes);
        std::memcpy(data.data(), buffer, frame->uBytes);
        
        auto& firstStage = *m_stages[0];
        {
            std::lock_guard<std::mutex> lock(firstStage.mutex);
            firstStage.queue.push({*frame, std::move(data)});
        }
        firstStage.cv.notify_one();
    }
    
    void BuildPipeline() {
        // 階段1: 預處理
        AddStage("Preprocessing", [](dvpFrame* frame, void* buffer) {
            // 降噪、去馬賽克等
            ApplyDenoising(frame, buffer);
        });
        
        // 階段2: 增強
        AddStage("Enhancement", [](dvpFrame* frame, void* buffer) {
            // 對比度、銳化等
            EnhanceImage(frame, buffer);
        });
        
        // 階段3: 分析
        AddStage("Analysis", [](dvpFrame* frame, void* buffer) {
            // 缺陷檢測、測量等
            AnalyzeDefects(frame, buffer);
        });
    }
    
private:
    static void ApplyDenoising(dvpFrame* frame, void* buffer) {
        // 降噪算法實現
    }
    
    static void EnhanceImage(dvpFrame* frame, void* buffer) {
        // 增強算法實現
    }
    
    static void AnalyzeDefects(dvpFrame* frame, void* buffer) {
        // 缺陷檢測實現
    }
};
```

## 8. 多線程圖像獲取

### 8.1 多相機並行獲取

```cpp
class MultiCameraAcquisition {
private:
    struct CameraContext {
        dvpHandle handle;
        std::thread worker;
        std::atomic<bool> running;
        std::atomic<uint64_t> frameCount;
        std::function<void(dvpFrame*, void*, int)> callback;
        int cameraIndex;
    };
    
    std::vector<std::unique_ptr<CameraContext>> m_cameras;
    
public:
    bool AddCamera(dvpHandle handle, int index,
                   std::function<void(dvpFrame*, void*, int)> callback) {
        auto context = std::make_unique<CameraContext>();
        context->handle = handle;
        context->cameraIndex = index;
        context->callback = callback;
        context->running = true;
        context->frameCount = 0;
        
        // 為每個相機創建獨立的獲取線程
        context->worker = std::thread([this, ctx = context.get()]() {
            AcquisitionThread(ctx);
        });
        
        m_cameras.push_back(std::move(context));
        return true;
    }
    
    void AcquisitionThread(CameraContext* ctx) {
        dvpStatus status;
        dvpFrame frame;
        void* pBuffer = nullptr;
        
        // 設置線程親和性（綁定到特定CPU核心）
        SetThreadAffinity(ctx->cameraIndex);
        
        // 啟動視頻流
        status = dvpStart(ctx->handle);
        if (status != DVP_STATUS_OK) {
            return;
        }
        
        while (ctx->running) {
            // 獲取圖像
            status = dvpGetFrame(ctx->handle, &frame, &pBuffer, 1000);
            
            if (status == DVP_STATUS_OK) {
                ctx->frameCount++;
                
                // 回調處理
                if (ctx->callback) {
                    ctx->callback(&frame, pBuffer, ctx->cameraIndex);
                }
            }
        }
        
        dvpStop(ctx->handle);
    }
    
    void SetThreadAffinity(int coreIndex) {
#ifdef _WIN32
        DWORD_PTR mask = 1ULL << coreIndex;
        SetThreadAffinityMask(GetCurrentThread(), mask);
#else
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(coreIndex, &cpuset);
        pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
#endif
    }
    
    void StartAll() {
        for (auto& camera : m_cameras) {
            camera->running = true;
        }
    }
    
    void StopAll() {
        for (auto& camera : m_cameras) {
            camera->running = false;
        }
        
        for (auto& camera : m_cameras) {
            if (camera->worker.joinable()) {
                camera->worker.join();
            }
        }
    }
    
    void GetStatistics() {
        for (const auto& camera : m_cameras) {
            std::cout << "Camera " << camera->cameraIndex 
                     << ": " << camera->frameCount << " frames" 
                     << std::endl;
        }
    }
};
```

### 8.2 生產者-消費者模式

```cpp
template<typename T>
class ThreadSafeQueue {
private:
    std::queue<T> m_queue;
    mutable std::mutex m_mutex;
    std::condition_variable m_cv;
    size_t m_maxSize;
    
public:
    ThreadSafeQueue(size_t maxSize = 100) : m_maxSize(maxSize) {}
    
    void Push(T item) {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cv.wait(lock, [this]() { return m_queue.size() < m_maxSize; });
        
        m_queue.push(std::move(item));
        lock.unlock();
        m_cv.notify_one();
    }
    
    bool TryPop(T& item, std::chrono::milliseconds timeout) {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (!m_cv.wait_for(lock, timeout, [this]() { return !m_queue.empty(); })) {
            return false;
        }
        
        item = std::move(m_queue.front());
        m_queue.pop();
        lock.unlock();
        m_cv.notify_one();
        return true;
    }
    
    size_t Size() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.size();
    }
};

class ProducerConsumerAcquisition {
private:
    struct FrameData {
        dvpFrame frame;
        std::vector<uint8_t> buffer;
        std::chrono::steady_clock::time_point timestamp;
    };
    
    ThreadSafeQueue<FrameData> m_queue;
    std::vector<std::thread> m_producers;
    std::vector<std::thread> m_consumers;
    std::atomic<bool> m_running;
    
public:
    void StartProducer(dvpHandle handle) {
        m_producers.emplace_back([this, handle]() {
            ProducerThread(handle);
        });
    }
    
    void StartConsumer(std::function<void(FrameData&)> processor) {
        m_consumers.emplace_back([this, processor]() {
            ConsumerThread(processor);
        });
    }
    
private:
    void ProducerThread(dvpHandle handle) {
        dvpFrame frame;
        void* pBuffer = nullptr;
        
        dvpStart(handle);
        
        while (m_running) {
            dvpStatus status = dvpGetFrame(handle, &frame, &pBuffer, 1000);
            
            if (status == DVP_STATUS_OK) {
                FrameData data;
                data.frame = frame;
                data.buffer.resize(frame.uBytes);
                std::memcpy(data.buffer.data(), pBuffer, frame.uBytes);
                data.timestamp = std::chrono::steady_clock::now();
                
                m_queue.Push(std::move(data));
            }
        }
        
        dvpStop(handle);
    }
    
    void ConsumerThread(std::function<void(FrameData&)> processor) {
        while (m_running) {
            FrameData data;
            if (m_queue.TryPop(data, std::chrono::milliseconds(100))) {
                processor(data);
            }
        }
    }
};
```

## 9. 觸發模式圖像採集

### 9.1 硬體觸發配置

```cpp
class TriggerAcquisition {
private:
    dvpHandle m_handle;
    
public:
    bool ConfigureHardwareTrigger() {
        dvpStatus status;
        
        // 停止連續模式
        status = dvpStop(m_handle);
        
        // 設置觸發模式
        status = dvpSetTriggerState(m_handle, true);
        if (status != DVP_STATUS_OK) return false;
        
        // 設置觸發源為外部輸入
        status = dvpSetTriggerSource(m_handle, TRIGGER_SOURCE_LINE1);
        if (status != DVP_STATUS_OK) return false;
        
        // 配置觸發極性（上升沿）
        dvpTriggerInputType triggerType = TRIGGER_POS_EDGE;
        status = dvpSetInputIoConfig(m_handle, INPUT_IO_1, triggerType);
        if (status != DVP_STATUS_OK) return false;
        
        // 設置觸發延遲（微秒）
        double delay = 100.0;
        status = dvpSetTriggerDelay(m_handle, delay);
        
        // 設置觸發去抖動
        double jitter = 10.0;
        status = dvpSetTriggerJitter(m_handle, jitter);
        
        // 設置每次觸發的幀數
        dvpInt32 framesPerTrigger = 1;
        status = dvpSetFramesPerTrigger(m_handle, framesPerTrigger);
        
        // 啟動視頻流（等待觸發）
        status = dvpStart(m_handle);
        
        return status == DVP_STATUS_OK;
    }
    
    bool ConfigureSoftwareTrigger() {
        dvpStatus status;
        
        // 設置觸發模式
        status = dvpSetTriggerState(m_handle, true);
        if (status != DVP_STATUS_OK) return false;
        
        // 設置觸發源為軟體
        status = dvpSetTriggerSource(m_handle, TRIGGER_SOURCE_SOFTWARE);
        
        // 啟動視頻流
        status = dvpStart(m_handle);
        
        return status == DVP_STATUS_OK;
    }
    
    bool ExecuteSoftwareTrigger() {
        // 發送軟體觸發信號
        dvpStatus status = dvpTriggerFire(m_handle);
        return status == DVP_STATUS_OK;
    }
    
    // 批量觸發採集
    bool BurstTriggerAcquisition(int count) {
        std::vector<dvpFrame> frames;
        std::vector<std::vector<uint8_t>> buffers;
        
        frames.reserve(count);
        buffers.reserve(count);
        
        for (int i = 0; i < count; ++i) {
            // 發送觸發
            if (!ExecuteSoftwareTrigger()) {
                return false;
            }
            
            // 獲取圖像
            dvpFrame frame;
            void* pBuffer = nullptr;
            dvpStatus status = dvpGetFrame(m_handle, &frame, &pBuffer, 5000);
            
            if (status == DVP_STATUS_OK) {
                frames.push_back(frame);
                
                std::vector<uint8_t> buffer(frame.uBytes);
                std::memcpy(buffer.data(), pBuffer, frame.uBytes);
                buffers.push_back(std::move(buffer));
            } else {
                std::cerr << "Failed to get frame after trigger" << std::endl;
                return false;
            }
        }
        
        // 批量處理
        ProcessBatch(frames, buffers);
        
        return true;
    }
    
private:
    void ProcessBatch(const std::vector<dvpFrame>& frames,
                     const std::vector<std::vector<uint8_t>>& buffers) {
        // 批量處理邏輯
    }
};
```

### 9.2 多相機同步觸發

```cpp
class MultiCameraSyncTrigger {
private:
    std::vector<dvpHandle> m_handles;
    
public:
    bool ConfigureMasterSlave() {
        if (m_handles.size() < 2) return false;
        
        dvpStatus status;
        
        // 配置主相機
        dvpHandle master = m_handles[0];
        
        // 主相機產生觸發輸出
        status = dvpSetOutputIoFunction(master, OUTPUT_IO_1, OUTPUT_FUNCTION_STROBE);
        status = dvpSetStrobeDriver(master, OUTPUT_IO_1, FRAME_START);
        status = dvpSetStrobeOutputType(master, OUTPUT_IO_1, STROBE_OUT_HIGH);
        
        // 配置從相機
        for (size_t i = 1; i < m_handles.size(); ++i) {
            dvpHandle slave = m_handles[i];
            
            // 從相機接收外部觸發
            status = dvpSetTriggerState(slave, true);
            status = dvpSetTriggerSource(slave, TRIGGER_SOURCE_LINE1);
            status = dvpSetInputIoConfig(slave, INPUT_IO_1, TRIGGER_POS_EDGE);
        }
        
        return true;
    }
    
    bool StartSynchronizedAcquisition() {
        // 先啟動所有從相機
        for (size_t i = 1; i < m_handles.size(); ++i) {
            dvpStart(m_handles[i]);
        }
        
        // 最後啟動主相機
        dvpStart(m_handles[0]);
        
        // 並行獲取圖像
        std::vector<std::thread> threads;
        std::vector<dvpFrame> frames(m_handles.size());
        std::vector<std::vector<uint8_t>> buffers(m_handles.size());
        
        for (size_t i = 0; i < m_handles.size(); ++i) {
            threads.emplace_back([this, i, &frames, &buffers]() {
                dvpFrame frame;
                void* pBuffer = nullptr;
                
                dvpStatus status = dvpGetFrame(m_handles[i], &frame, &pBuffer, 5000);
                if (status == DVP_STATUS_OK) {
                    frames[i] = frame;
                    buffers[i].resize(frame.uBytes);
                    std::memcpy(buffers[i].data(), pBuffer, frame.uBytes);
                }
            });
        }
        
        // 等待所有相機完成
        for (auto& t : threads) {
            t.join();
        }
        
        // 驗證同步性
        uint64_t maxTimestamp = 0, minTimestamp = UINT64_MAX;
        for (const auto& frame : frames) {
            maxTimestamp = std::max(maxTimestamp, frame.uTimestamp);
            minTimestamp = std::min(minTimestamp, frame.uTimestamp);
        }
        
        uint64_t timeDiff = maxTimestamp - minTimestamp;
        std::cout << "Timestamp difference: " << timeDiff << " us" << std::endl;
        
        return true;
    }
};
```

## 10. Qt整合方案

### 10.1 QImage轉換

```cpp
class QtImageConverter {
public:
    static QImage ConvertToQImage(const dvpFrame* frame, void* buffer) {
        QImage image;
        
        switch(frame->format) {
            case FORMAT_MONO:
                image = QImage((uchar*)buffer, 
                              frame->iWidth, 
                              frame->iHeight,
                              frame->iWidth,
                              QImage::Format_Grayscale8);
                break;
                
            case FORMAT_BGR24:
                image = QImage((uchar*)buffer,
                              frame->iWidth,
                              frame->iHeight,
                              frame->iWidth * 3,
                              QImage::Format_BGR888);
                break;
                
            case FORMAT_RGB24:
                image = QImage((uchar*)buffer,
                              frame->iWidth,
                              frame->iHeight,
                              frame->iWidth * 3,
                              QImage::Format_RGB888);
                break;
                
            case FORMAT_BGR32:
            case FORMAT_RGB32:
                image = QImage((uchar*)buffer,
                              frame->iWidth,
                              frame->iHeight,
                              frame->iWidth * 4,
                              QImage::Format_RGBA8888);
                break;
                
            default:
                // 需要格式轉換
                image = ConvertBayerToQImage(frame, buffer);
                break;
        }
        
        return image;
    }
    
    static QImage ConvertBayerToQImage(const dvpFrame* frame, void* buffer) {
        // 分配RGB緩衝區
        std::vector<uint8_t> rgbBuffer(frame->iWidth * frame->iHeight * 3);
        
        // Bayer轉RGB（簡化版本）
        BayerToRGB((uint8_t*)buffer, rgbBuffer.data(), 
                   frame->iWidth, frame->iHeight, frame->format);
        
        return QImage(rgbBuffer.data(),
                     frame->iWidth,
                     frame->iHeight,
                     frame->iWidth * 3,
                     QImage::Format_RGB888).copy();
    }
    
private:
    static void BayerToRGB(uint8_t* bayer, uint8_t* rgb, 
                          int width, int height, 
                          dvpImageFormat bayerFormat) {
        // 簡單的雙線性插值
        // 實際應用中應使用更高質量的算法
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int idx = y * width + x;
                int rgbIdx = idx * 3;
                
                // 根據Bayer pattern計算RGB值
                // 這裡簡化處理，實際需要根據bayerFormat判斷
                rgb[rgbIdx] = bayer[idx];     // R
                rgb[rgbIdx + 1] = bayer[idx]; // G
                rgb[rgbIdx + 2] = bayer[idx]; // B
            }
        }
    }
};

// Qt Widget顯示
class CameraWidget : public QWidget {
    Q_OBJECT
    
private:
    dvpHandle m_handle;
    QLabel* m_imageLabel;
    QTimer* m_timer;
    
public:
    CameraWidget(QWidget* parent = nullptr) : QWidget(parent) {
        m_imageLabel = new QLabel(this);
        
        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->addWidget(m_imageLabel);
        
        m_timer = new QTimer(this);
        connect(m_timer, &QTimer::timeout, this, &CameraWidget::updateImage);
        m_timer->start(33); // 30 FPS
    }
    
private slots:
    void updateImage() {
        dvpFrame frame;
        void* pBuffer = nullptr;
        
        dvpStatus status = dvpGetFrame(m_handle, &frame, &pBuffer, 0);
        if (status == DVP_STATUS_OK) {
            QImage image = QtImageConverter::ConvertToQImage(&frame, pBuffer);
            
            // 縮放到窗口大小
            QPixmap pixmap = QPixmap::fromImage(image);
            pixmap = pixmap.scaled(m_imageLabel->size(), 
                                  Qt::KeepAspectRatio, 
                                  Qt::SmoothTransformation);
            
            m_imageLabel->setPixmap(pixmap);
            
            emit frameReceived(frame.uFrameID);
        }
    }
    
signals:
    void frameReceived(uint64_t frameId);
};
```

## 11. 性能優化建議

### 11.1 高速採集優化策略

```cpp
class HighSpeedOptimization {
private:
    dvpHandle m_handle;
    
public:
    bool OptimizeForHighSpeed() {
        dvpStatus status;
        
        // 1. 增加緩衝隊列大小
        status = dvpSetBufferQueueSize(m_handle, 16);
        
        // 2. 關閉不必要的圖像處理
        status = dvpSetColorSolutionState(m_handle, false);
        status = dvpSetDefectFixState(m_handle, false);
        status = dvpSetSharpnessState(m_handle, false);
        
        // 3. 使用最小ROI
        dvpRegion roi;
        roi.X = 0;
        roi.Y = 0;
        roi.W = 640;  // 根據需求調整
        roi.H = 480;
        status = dvpSetRoi(m_handle, roi);
        
        // 4. 設置像素格式為單色（如果可能）
        status = dvpSetSourceFormat(m_handle, S_RAW8);
        
        // 5. 優化曝光時間
        double exposure = 100.0; // 微秒
        status = dvpSetExposure(m_handle, exposure);
        
        // 6. 使用硬體加速ISP
        status = dvpSetHardwareIspState(m_handle, true);
        
        // 7. 設置高優先級
        SetHighPriority();
        
        return true;
    }
    
private:
    void SetHighPriority() {
#ifdef _WIN32
        SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
#else
        struct sched_param param;
        param.sched_priority = sched_get_priority_max(SCHED_FIFO);
        pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);
#endif
    }
};
```

### 11.2 記憶體管理優化

```cpp
class MemoryOptimization {
private:
    // 使用內存池減少分配開銷
    class MemoryPool {
    private:
        struct Block {
            std::unique_ptr<uint8_t[]> data;
            std::atomic<bool> inUse;
            
            Block(size_t size) : data(std::make_unique<uint8_t[]>(size)), inUse(false) {}
        };
        
        std::vector<Block> m_blocks;
        size_t m_blockSize;
        
    public:
        MemoryPool(size_t blockSize, size_t blockCount) 
            : m_blockSize(blockSize) {
            m_blocks.reserve(blockCount);
            for (size_t i = 0; i < blockCount; ++i) {
                m_blocks.emplace_back(blockSize);
            }
        }
        
        uint8_t* Allocate() {
            for (auto& block : m_blocks) {
                bool expected = false;
                if (block.inUse.compare_exchange_strong(expected, true)) {
                    return block.data.get();
                }
            }
            return nullptr;
        }
        
        void Free(uint8_t* ptr) {
            for (auto& block : m_blocks) {
                if (block.data.get() == ptr) {
                    block.inUse = false;
                    break;
                }
            }
        }
    };
    
    std::unique_ptr<MemoryPool> m_pool;
    
public:
    void Initialize(size_t frameSize, size_t poolSize) {
        // 預分配內存池
        m_pool = std::make_unique<MemoryPool>(frameSize, poolSize);
        
        // 鎖定內存防止換頁
#ifdef _WIN32
        SIZE_T minSize, maxSize;
        GetProcessWorkingSetSize(GetCurrentProcess(), &minSize, &maxSize);
        SetProcessWorkingSetSize(GetCurrentProcess(), 
                                minSize + frameSize * poolSize,
                                maxSize + frameSize * poolSize);
#else
        mlockall(MCL_CURRENT | MCL_FUTURE);
#endif
    }
    
    bool ProcessWithPool(dvpHandle handle) {
        dvpFrame frame;
        void* sdkBuffer = nullptr;
        
        // 從SDK獲取幀
        dvpStatus status = dvpGetFrame(handle, &frame, &sdkBuffer, 1000);
        if (status != DVP_STATUS_OK) return false;
        
        // 從池中獲取緩衝區
        uint8_t* poolBuffer = m_pool->Allocate();
        if (!poolBuffer) return false;
        
        // 複製數據
        std::memcpy(poolBuffer, sdkBuffer, frame.uBytes);
        
        // 異步處理
        std::thread([this, frame, poolBuffer]() {
            ProcessFrame(&frame, poolBuffer);
            m_pool->Free(poolBuffer);
        }).detach();
        
        return true;
    }
    
private:
    void ProcessFrame(const dvpFrame* frame, uint8_t* buffer) {
        // 處理邏輯
    }
};
```

### 11.3 性能監控

```cpp
class PerformanceMonitor {
private:
    struct FrameStatistics {
        std::atomic<uint64_t> totalFrames{0};
        std::atomic<uint64_t> droppedFrames{0};
        std::atomic<uint64_t> processedFrames{0};
        std::chrono::steady_clock::time_point startTime;
        std::atomic<double> totalProcessingTime{0};
        std::atomic<double> maxProcessingTime{0};
        std::atomic<double> minProcessingTime{1e9};
    };
    
    FrameStatistics m_stats;
    
public:
    void StartMonitoring() {
        m_stats.startTime = std::chrono::steady_clock::now();
    }
    
    void RecordFrame(bool dropped = false) {
        m_stats.totalFrames++;
        if (dropped) {
            m_stats.droppedFrames++;
        }
    }
    
    void RecordProcessing(double processingTimeMs) {
        m_stats.processedFrames++;
        m_stats.totalProcessingTime += processingTimeMs;
        
        double current = m_stats.maxProcessingTime.load();
        while (processingTimeMs > current && 
               !m_stats.maxProcessingTime.compare_exchange_weak(current, processingTimeMs));
        
        current = m_stats.minProcessingTime.load();
        while (processingTimeMs < current && 
               !m_stats.minProcessingTime.compare_exchange_weak(current, processingTimeMs));
    }
    
    void PrintStatistics() {
        auto now = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(
            now - m_stats.startTime).count();
        
        double fps = m_stats.totalFrames.load() / (double)duration;
        double dropRate = m_stats.droppedFrames.load() * 100.0 / m_stats.totalFrames.load();
        double avgProcessing = m_stats.totalProcessingTime.load() / m_stats.processedFrames.load();
        
        std::cout << "=== Performance Statistics ===" << std::endl;
        std::cout << "Duration: " << duration << " seconds" << std::endl;
        std::cout << "Total Frames: " << m_stats.totalFrames << std::endl;
        std::cout << "Dropped Frames: " << m_stats.droppedFrames 
                 << " (" << dropRate << "%)" << std::endl;
        std::cout << "FPS: " << fps << std::endl;
        std::cout << "Avg Processing: " << avgProcessing << " ms" << std::endl;
        std::cout << "Max Processing: " << m_stats.maxProcessingTime << " ms" << std::endl;
        std::cout << "Min Processing: " << m_stats.minProcessingTime << " ms" << std::endl;
    }
    
    // 監控幀率變化
    void MonitorFrameRate(dvpHandle handle) {
        dvpFrameCount count;
        dvpStatus status = dvpGetFrameCount(handle, &count);
        
        if (status == DVP_STATUS_OK) {
            static dvpFrameCount lastCount = count;
            static auto lastTime = std::chrono::steady_clock::now();
            
            auto now = std::chrono::steady_clock::now();
            auto deltaTime = std::chrono::duration<double>(now - lastTime).count();
            
            if (deltaTime > 1.0) { // 每秒更新
                uint64_t deltaFrames = count.uFrameCount - lastCount.uFrameCount;
                double currentFps = deltaFrames / deltaTime;
                
                std::cout << "Current FPS: " << currentFps 
                         << ", Frames: " << count.uFrameCount
                         << ", Errors: " << count.uErrorFrameCount
                         << std::endl;
                
                lastCount = count;
                lastTime = now;
            }
        }
    }
};
```

## 12. 完整示例：高速AOI檢測系統

```cpp
class AOIInspectionSystem {
private:
    dvpHandle m_handle;
    std::unique_ptr<MemoryOptimization> m_memOpt;
    std::unique_ptr<ImageProcessingPipeline> m_pipeline;
    std::unique_ptr<PerformanceMonitor> m_monitor;
    std::atomic<bool> m_running;
    
public:
    bool Initialize(const char* cameraName) {
        // 打開相機
        dvpStatus status = dvpOpenByName(cameraName, OPEN_NORMAL, &m_handle);
        if (status != DVP_STATUS_OK) {
            return false;
        }
        
        // 優化設置
        OptimizeForAOI();
        
        // 初始化內存管理
        dvpUint32 frameSize = GetFrameSize();
        m_memOpt = std::make_unique<MemoryOptimization>();
        m_memOpt->Initialize(frameSize, 32);
        
        // 構建處理流水線
        m_pipeline = std::make_unique<ImageProcessingPipeline>();
        BuildAOIPipeline();
        
        // 初始化性能監控
        m_monitor = std::make_unique<PerformanceMonitor>();
        
        return true;
    }
    
    void Start() {
        m_running = true;
        m_monitor->StartMonitoring();
        
        // 註冊回調
        dvpRegisterStreamCallback(m_handle, OnFrameCallback, 
                                 STREAM_EVENT_PROCESSED, this);
        
        // 啟動視頻流
        dvpStart(m_handle);
        
        // 監控線程
        std::thread monitorThread([this]() {
            while (m_running) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
                m_monitor->MonitorFrameRate(m_handle);
            }
        });
        
        monitorThread.detach();
    }
    
    void Stop() {
        m_running = false;
        dvpStop(m_handle);
        dvpClose(m_handle);
        m_monitor->PrintStatistics();
    }
    
private:
    void OptimizeForAOI() {
        // AOI特定優化
        dvpSetBufferQueueSize(m_handle, 32);
        dvpSetHardwareIspState(m_handle, true);
        dvpSetColorSolutionState(m_handle, false);
        
        // 設置合適的ROI
        dvpRegion roi = {0, 0, 1920, 1080};
        dvpSetRoi(m_handle, roi);
        
        // 優化曝光
        dvpSetExposure(m_handle, 500.0);
        dvpSetAnalogGain(m_handle, 1.0f);
    }
    
    dvpUint32 GetFrameSize() {
        dvpRegion roi;
        dvpGetRoi(m_handle, &roi);
        return roi.W * roi.H * 3; // 假設RGB格式
    }
    
    void BuildAOIPipeline() {
        // 預處理
        m_pipeline->AddStage("Preprocessing", [](dvpFrame* frame, void* buffer) {
            // 降噪、濾波
        });
        
        // 缺陷檢測
        m_pipeline->AddStage("DefectDetection", [](dvpFrame* frame, void* buffer) {
            // 檢測算法
        });
        
        // 測量
        m_pipeline->AddStage("Measurement", [](dvpFrame* frame, void* buffer) {
            // 尺寸測量
        });
        
        // 分類
        m_pipeline->AddStage("Classification", [](dvpFrame* frame, void* buffer) {
            // 缺陷分類
        });
    }
    
    static int DVP_CALL OnFrameCallback(dvpHandle handle, dvpStreamEvent event,
                                       void* pContext, dvpFrame* pFrame, void* pBuffer) {
        AOIInspectionSystem* system = (AOIInspectionSystem*)pContext;
        
        auto startTime = std::chrono::high_resolution_clock::now();
        
        // 處理幀
        system->m_pipeline->ProcessFrame(pFrame, pBuffer);
        
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<double, std::milli>(endTime - startTime).count();
        
        system->m_monitor->RecordFrame();
        system->m_monitor->RecordProcessing(duration);
        
        return 1;
    }
};

// 使用示例
int main() {
    AOIInspectionSystem system;
    
    if (system.Initialize("Do3Think Camera")) {
        system.Start();
        
        // 運行一段時間
        std::this_thread::sleep_for(std::chrono::minutes(5));
        
        system.Stop();
    }
    
    return 0;
}
```

## 13. 故障排查指南

### 13.1 常見問題與解決方案

| 問題 | 可能原因 | 解決方案 |
|------|---------|----------|
| 獲取圖像超時 | 曝光時間過長 | 增加超時時間或減少曝光時間 |
| 丟幀嚴重 | 處理速度跟不上採集速度 | 優化處理算法或增加緩衝隊列 |
| 圖像撕裂 | 多線程訪問衝突 | 使用互斥鎖或原子操作 |
| 內存洩漏 | 未正確釋放資源 | 使用RAII或智能指針 |
| 延遲過高 | 緩衝隊列過大 | 減少緩衝隊列大小 |

### 13.2 調試技巧

```cpp
// 啟用調試日誌
dvpSetDebugLevel(m_handle, DEBUG_LEVEL_VERBOSE);

// 獲取詳細錯誤信息
char errorMsg[256];
dvpGetLastErrorString(errorMsg, sizeof(errorMsg));

// 監控相機狀態
dvpCameraStatus status;
dvpGetCameraStatus(m_handle, &status);
```

## 14. 總結

Do3ThinkCamera SDK提供了完整的圖像獲取與處理解決方案，通過合理使用同步/異步模式、優化緩衝管理、實現零拷貝等技術，可以實現高性能的工業視覺應用。在實際開發中，應根據具體需求選擇合適的架構和優化策略，確保系統的穩定性和實時性。

關鍵要點：
- 根據應用場景選擇合適的獲取模式
- 合理配置緩衝隊列大小
- 使用多線程提高並行處理能力
- 實施內存池減少分配開銷
- 監控性能指標及時優化
- 遵循RAII原則確保資源安全