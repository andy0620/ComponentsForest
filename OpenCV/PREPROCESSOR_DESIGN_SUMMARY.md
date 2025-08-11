# 🎯 PreProcessor 設計架構總覽

## 📚 架構文檔結構

### 1. **核心設計文檔**
```
OpenCV/
├── 01_PreProcessor_Architecture_Overview.md    # 架構總覽
├── 02_Processing_Pipeline_Design.md           # 處理管線設計
├── 03_Performance_Optimization_Strategy.md    # 效能優化策略
├── 04_Signal_Slot_Integration.md             # Signal/Slot 整合
└── 05_Child_Class_Implementation_Guide.md    # 子類實作指南
```

### 2. **實作檔案**
```
OpenCV/
├── preprocessor_base.h/cpp          # PreProcessor 基礎類別
├── preprocessing_pipeline.h/cpp     # 處理管線框架
├── blur_preprocessor.h/cpp         # 模糊處理範例
├── edge_preprocessor.h/cpp         # 邊緣偵測範例
└── denoise_preprocessor.h/cpp      # 降噪處理範例
```

## 🏗️ 核心架構設計

### 繼承層次
```
QObject 
  └── BaseComponent 
      └── PreProcessorBase (抽象基類)
          ├── BlurPreProcessor    (高斯模糊)
          ├── EdgePreProcessor    (邊緣偵測)
          ├── DenoisePreProcessor (降噪處理)
          └── [您的自訂處理器]
```

### 設計模式
- **Template Method Pattern**: 定義處理骨架，子類實作具體演算法
- **Chain of Responsibility**: 支援多個處理器串聯
- **Strategy Pattern**: 演算法可在執行時期切換
- **Observer Pattern**: 透過 Signal/Slot 實現解耦

## 💎 優雅設計特點

### 1. **最小化子類程式碼**
子類只需實作一個純虛函數：
```cpp
class YourPreProcessor : public PreProcessorBase {
protected:
    cv::Mat processImplementation(const cv::Mat& input) override {
        // 您的核心處理邏輯
        return processedImage;
    }
};
```

### 2. **自動化功能**
基類自動處理：
- ✅ 線程管理（獨立處理線程）
- ✅ 記憶體管理（緩衝池、零拷貝優化）
- ✅ 效能監控（FPS、延遲、CPU/GPU 使用率）
- ✅ 錯誤處理與恢復
- ✅ GPU 加速（自動偵測與切換）
- ✅ 批次處理支援
- ✅ 背壓控制

### 3. **Signal/Slot 整合**
```cpp
signals:
    void frameProcessed(const QImage& processed, const FrameMetadata& metadata);
    void processingError(ProcessingError error, const QString& details);
    void statisticsUpdated(const ProcessingStats& stats);
    
public slots:
    void onFrameReceived(const QImage& frame, const FrameMetadata& metadata);
    void onBatchReceived(const QVector<QImage>& frames);
```

## 🚀 高效能特性

### 1. **零拷貝優化**
- QImage ↔ cv::Mat 高效轉換
- 共享記憶體機制
- In-place 處理

### 2. **GPU 加速**
```cpp
// 自動 GPU/CPU 切換
if (m_useGPU && cv::cuda::getCudaEnabledDeviceCount() > 0) {
    cv::cuda::GpuMat gpuInput, gpuOutput;
    gpuInput.upload(input);
    processOnGPU(gpuInput, gpuOutput);
    gpuOutput.download(output);
} else {
    processOnCPU(input, output);
}
```

### 3. **處理管線**
支援四種執行模式：
- **Sequential**: 依序處理
- **Parallel**: 平行處理
- **Conditional**: 條件分支
- **Adaptive**: 自適應調整

## 📊 實作範例

### 1. **BlurPreProcessor** (高斯模糊)
```cpp
class BlurPreProcessor : public PreProcessorBase {
    // 參數：kernelSize, sigmaX, sigmaY
    // 支援 CPU/GPU 雙路徑
    // 自動參數驗證
};
```

### 2. **EdgePreProcessor** (邊緣偵測)
```cpp
class EdgePreProcessor : public PreProcessorBase {
    // 演算法：Canny, Sobel, Laplacian, Scharr
    // 多階段處理：前處理→邊緣偵測→後處理
    // 自適應閾值
};
```

### 3. **DenoisePreProcessor** (降噪)
```cpp
class DenoisePreProcessor : public PreProcessorBase {
    // 演算法：Bilateral, NL-means, Median, Morphological
    // 噪音估計與自適應參數
    // 多階段降噪支援
};
```

## 🔧 使用範例

### 簡單使用
```cpp
// 1. 創建處理器
auto* blurProcessor = new BlurPreProcessor();

// 2. 配置參數
blurProcessor->configure({
    {"kernelSize", 7},
    {"sigmaX", 2.0},
    {"useGPU", true}
});

// 3. 連接訊號
connect(camera, &CameraComponent::frameReady,
        blurProcessor, &PreProcessorBase::onFrameReceived);
        
connect(blurProcessor, &PreProcessorBase::frameProcessed,
        display, &DisplayWidget::updateImage);

// 4. 開始處理
blurProcessor->startProcessing();
```

### 管線串聯
```cpp
// 相機 → 降噪 → 邊緣偵測 → 顯示
camera → denoiser → edgeDetector → display

// 程式碼實現
connect(camera, &CameraComponent::frameReady,
        denoiser, &PreProcessorBase::onFrameReceived);
        
connect(denoiser, &PreProcessorBase::frameProcessed,
        edgeDetector, &PreProcessorBase::onFrameReceived);
        
connect(edgeDetector, &PreProcessorBase::frameProcessed,
        display, &DisplayWidget::updateImage);
```

## 📈 效能監控

### 內建指標
```cpp
struct ProcessingStats {
    double fps;                    // 每秒處理幀數
    double averageProcessingTime;  // 平均處理時間(ms)
    qint64 totalFramesProcessed;   // 總處理幀數
    qint64 framesDropped;          // 丟棄幀數
    double cpuUsage;               // CPU 使用率
    double gpuUsage;               // GPU 使用率
    size_t memoryUsage;            // 記憶體使用量
};
```

### 即時監控
```cpp
connect(processor, &PreProcessorBase::statisticsUpdated,
        [](const ProcessingStats& stats) {
            qDebug() << "FPS:" << stats.fps
                     << "Processing:" << stats.averageProcessingTime << "ms"
                     << "GPU:" << stats.gpuUsage << "%";
        });
```

## 🎯 設計優勢

1. **極簡子類實作** - 只需實作核心演算法
2. **高效能** - GPU加速、零拷貝、多線程
3. **完全解耦** - Signal/Slot 機制
4. **易於擴展** - 清晰的介面定義
5. **生產就緒** - 完整的錯誤處理與監控
6. **靈活配置** - 執行時期參數調整
7. **管線支援** - 多個處理器串聯

## 🔍 文檔導覽

### 深入了解特定主題：
- **架構細節** → [01_PreProcessor_Architecture_Overview.md](01_PreProcessor_Architecture_Overview.md)
- **管線設計** → [02_Processing_Pipeline_Design.md](02_Processing_Pipeline_Design.md)
- **效能優化** → [03_Performance_Optimization_Strategy.md](03_Performance_Optimization_Strategy.md)
- **Signal/Slot** → [04_Signal_Slot_Integration.md](04_Signal_Slot_Integration.md)
- **實作指南** → [05_Child_Class_Implementation_Guide.md](05_Child_Class_Implementation_Guide.md)

## 🚀 下一步

1. **查看範例實作** - 了解如何實作自己的處理器
2. **整合到專案** - 將 PreProcessor 加入 CMakeLists.txt
3. **創建自訂處理器** - 繼承 PreProcessorBase 實作您的演算法
4. **建立處理管線** - 串聯多個處理器完成複雜任務

這個架構提供了強大、高效且優雅的影像前處理基礎，讓您可以專注於核心演算法的實現！