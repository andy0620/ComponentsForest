# 🎯 ROI 管理系統設計總覽

## 📚 新增的 ROI 文檔結構

```
OpenCV/
├── 📄 06_ROI_Management_System.md      # ROI 管理系統架構
├── 📄 07_ROI_Interactive_Design.md     # 互動式 ROI 設計
├── 💻 roi_types.h                      # ROI 類型定義
├── 💻 roi_manager.h/cpp                # ROI 管理器
├── 💻 roi_selector_widget.h/cpp        # ROI 選擇器 UI
└── 💻 roi_preprocessor_base.h/cpp      # 增強版處理器
```

## 🎨 ROI 類型設計

### 1. **多種 ROI 形狀**

```cpp
// 矩形 ROI（支援旋轉）
auto rect = ROIFactory::createRectangle(cv::Rect2f(x, y, w, h), rotation);

// 圓形 ROI（含環形）
auto circle = ROIFactory::createCircle(cv::Point2f(cx, cy), radius);

// 多邊形 ROI（凸/凹多邊形）
std::vector<cv::Point2f> points = {{100,100}, {200,100}, {150,200}};
auto polygon = ROIFactory::createPolygon(points);

// 遮罩 ROI（二值/權重遮罩）
cv::Mat mask = cv::imread("mask.png", cv::IMREAD_GRAYSCALE);
auto maskROI = ROIFactory::createMask(mask);

// 動態 ROI（自動追蹤）
auto dynamic = ROIFactory::createDynamic(initialRect, TrackingMethod::CSRT);
```

### 2. **ROI 層次結構**

```cpp
class ROIBase {
public:
    // 核心介面
    virtual cv::Mat extractRegion(const cv::Mat& image) const = 0;
    virtual cv::Mat getMask(cv::Size imageSize) const = 0;
    virtual bool contains(const cv::Point2f& point) const = 0;
    virtual void move(const cv::Point2f& delta) = 0;
    virtual void scale(float factor) = 0;
    virtual void rotate(float angle) = 0;
    
    // 進階功能
    virtual void track(const cv::Mat& frame);
    virtual ROIStatistics calculateStats(const cv::Mat& region);
};
```

## 🚀 高效能設計

### 1. **零拷貝提取**
```cpp
// 矩形 ROI 直接返回原圖的子區域（零拷貝）
cv::Mat roi = image(rect);  // 共享記憶體，無拷貝

// 非矩形 ROI 使用遮罩（優化路徑）
cv::Mat masked;
image.copyTo(masked, mask);  // 只拷貝 ROI 區域
```

### 2. **平行處理**
```cpp
// 多個 ROI 平行處理
std::vector<cv::Mat> results(rois.size());
#pragma omp parallel for
for (int i = 0; i < rois.size(); ++i) {
    results[i] = processROI(image, rois[i]);
}
```

### 3. **GPU 加速**
```cpp
// GPU 批次處理
cv::cuda::GpuMat gpuImage(image);
std::vector<cv::cuda::GpuMat> gpuROIs;
for (const auto& roi : rois) {
    gpuROIs.push_back(extractROIonGPU(gpuImage, roi));
}
```

### 4. **智慧快取**
```cpp
// LRU 快取遮罩，避免重複計算
class MaskCache {
    std::unordered_map<ROIId, cv::Mat> cache;
    std::list<ROIId> lru_list;
    size_t max_size = 100;
    
    cv::Mat getMask(const ROIBase* roi) {
        if (cache.find(roi->id) != cache.end()) {
            // 快取命中，更新 LRU
            return cache[roi->id];
        }
        // 計算並快取
        auto mask = roi->generateMask();
        addToCache(roi->id, mask);
        return mask;
    }
};
```

## 🎮 互動式 ROI 設計

### 1. **視覺回饋系統**
```cpp
enum class ROIState {
    Normal,      // 藍色邊框
    Hover,       // 亮藍色 + 陰影
    Selected,    // 綠色 + 控制點
    Editing,     // 橙色 + 即時預覽
    Processing,  // 動畫效果
    Error        // 紅色警示
};
```

### 2. **操作模式**
```cpp
enum class InteractionMode {
    Select,      // 選擇 ROI
    Create,      // 創建新 ROI
    Edit,        // 編輯現有 ROI
    Delete,      // 刪除 ROI
    Pan,         // 平移視圖
    Zoom         // 縮放視圖
};
```

### 3. **智慧對齊**
```cpp
class SmartAlignment {
    // 磁性吸附
    void snapToGrid(cv::Point2f& point, int gridSize = 10);
    void snapToROI(cv::Point2f& point, const std::vector<ROIBase*>& rois);
    
    // 對齊輔助線
    void showAlignmentGuides(const ROIBase* activeROI);
    void alignToCenter(ROIBase* roi1, ROIBase* roi2);
    void distributeEvenly(std::vector<ROIBase*>& rois);
};
```

## 💡 進階功能

### 1. **ROI 追蹤**
```cpp
enum class TrackingMethod {
    TemplateMatching,  // 模板匹配
    OpticalFlow,       // 光流法
    CSRT,             // 判別相關濾波器
    KCF,              // 核相關濾波器
    DeepSORT          // 深度學習追蹤
};

// 使用範例
dynamicROI->setTrackingMethod(TrackingMethod::CSRT);
dynamicROI->track(nextFrame);
```

### 2. **ROI 模板系統**
```cpp
// 儲存 ROI 配置
ROITemplate template;
template.name = "PCB_Inspection";
template.addROI("IC_Area", rect1);
template.addROI("Solder_Points", polygon1);
template.save("templates/pcb_inspection.json");

// 載入並套用
ROITemplate loaded("templates/pcb_inspection.json");
roiManager.applyTemplate(loaded);
```

### 3. **階層式 ROI**
```cpp
// 父子關係
auto parentROI = ROIFactory::createRectangle(largeRect);
auto childROI1 = ROIFactory::createCircle(circle1);
auto childROI2 = ROIFactory::createCircle(circle2);

parentROI->addChild(childROI1);
parentROI->addChild(childROI2);

// 移動父 ROI 時，子 ROI 跟隨移動
parentROI->move(delta);  // 子 ROI 自動更新位置
```

### 4. **ROI 專屬處理參數**
```cpp
struct ROIProcessingParams {
    std::string algorithm;
    QVariantMap parameters;
    int priority;
    bool skipIfFailed;
};

// 每個 ROI 可以有不同的處理參數
roi1->setProcessingParams({
    .algorithm = "EdgeDetection",
    .parameters = {{"threshold", 100}},
    .priority = 1
});

roi2->setProcessingParams({
    .algorithm = "Denoising",
    .parameters = {{"strength", 0.5}},
    .priority = 2
});
```

## 📊 ROI 統計分析

```cpp
struct ROIStatistics {
    // 基本統計
    cv::Scalar mean;
    cv::Scalar stdDev;
    double min, max;
    cv::Point minLoc, maxLoc;
    
    // 直方圖
    cv::Mat histogram;
    
    // 形態學特徵
    double area;
    double perimeter;
    cv::Moments moments;
    
    // 紋理特徵
    double contrast;
    double homogeneity;
    double entropy;
    
    // 品質指標
    double sharpness;
    double snr;  // 信噪比
};

// 計算 ROI 統計
auto stats = roi->calculateStatistics(image);
```

## 🔧 整合到 PreProcessor

```cpp
class ROIPreProcessorBase : public PreProcessorBase {
protected:
    // 覆寫處理函數，加入 ROI 支援
    cv::Mat processImplementation(const cv::Mat& input) override {
        // 取得所有 ROI
        auto rois = m_roiManager->getActiveROIs();
        
        // 選擇處理策略
        switch (m_strategy) {
            case ROIProcessingStrategy::Sequential:
                return processSequential(input, rois);
                
            case ROIProcessingStrategy::Parallel:
                return processParallel(input, rois);
                
            case ROIProcessingStrategy::GPU:
                return processGPU(input, rois);
        }
    }
    
    // ROI 專屬處理
    virtual cv::Mat processROI(const cv::Mat& roi, 
                               const ROIProcessingParams& params) = 0;
};
```

## 🎯 使用範例

### 簡單範例
```cpp
// 1. 創建 ROI 管理器
ROIManager roiManager;

// 2. 添加 ROI
auto defectArea = ROIFactory::createRectangle(cv::Rect2f(100, 100, 200, 200));
defectArea->setName("Defect_Detection_Area");
roiManager.addROI(defectArea);

// 3. 創建處理器
auto processor = std::make_unique<DefectDetector>();
processor->setROIManager(&roiManager);

// 4. 處理影像
cv::Mat frame = camera.capture();
auto result = processor->process(frame);
```

### 進階範例
```cpp
// 動態追蹤 + 缺陷檢測
auto trackingROI = ROIFactory::createDynamic(
    initialRect, 
    TrackingMethod::CSRT
);

trackingROI->setProcessingParams({
    .algorithm = "DefectDetection",
    .parameters = {
        {"sensitivity", 0.95},
        {"minDefectSize", 10}
    }
});

// 處理迴圈
while (camera.isCapturing()) {
    cv::Mat frame = camera.getFrame();
    
    // ROI 自動追蹤移動物體
    trackingROI->track(frame);
    
    // 在追蹤的 ROI 內進行缺陷檢測
    auto defects = processor->detectDefectsInROI(frame, trackingROI);
    
    // 視覺化結果
    visualizer.drawROI(frame, trackingROI);
    visualizer.markDefects(frame, defects);
}
```

## 🏆 設計優勢

1. **靈活性** - 支援各種 ROI 形狀和追蹤方式
2. **高效能** - 零拷貝、GPU 加速、平行處理
3. **易用性** - 簡潔的 API，豐富的預設功能
4. **互動性** - 完整的 UI 支援，視覺回饋
5. **可擴展** - 工廠模式，易於添加新 ROI 類型
6. **智慧化** - 自動追蹤、智慧對齊、模板系統

這個 ROI 管理系統完美整合到 PreProcessor 架構中，為工業 AOI 應用提供強大而優雅的區域處理能力！