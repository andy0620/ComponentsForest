# 開源AOI和視覺檢測最佳實踐指南

## 1. 執行摘要

本文檔整合了2024-2025年開源自動光學檢測(AOI)和機器視覺領域的最新發展，為Qt6.9架構下的工業視覺系統開發提供全面的技術指南。重點關注可直接應用於生產環境的開源解決方案、整合策略和最佳實踐。

### 關鍵發現
- OpenCV仍是最成熟且活躍的開源視覺庫，但需要更多編程工作
- YOLO系列（特別是YOLOv8/v9）在實時缺陷檢測中表現優異
- OpenVINO 2025顯著提升了邊緣部署性能（延遲降低50%）
- ROS2整合為工業機器人視覺提供了標準化解決方案

## 2. 開源專案比較矩陣

### 2.1 核心視覺處理框架

| 專案名稱 | 開源協議 | 主要功能 | Qt6整合難度 | 工業應用成熟度 | 2024更新狀態 |
|---------|---------|---------|------------|--------------|-------------|
| **OpenCV 4.9+** | Apache 2.0 | 全面的視覺處理 | 中等 | ★★★★★ | 活躍 |
| **PCL 1.14** | BSD | 3D點雲處理 | 高 | ★★★★☆ | 活躍 |
| **SimpleCV** | BSD | 簡化視覺開發 | 低 | ★★☆☆☆ | 停止維護 |
| **ImageJ/Fiji** | GPL | 科學影像分析 | 高 | ★★★☆☆ | 活躍 |
| **ViSP 3.6** | GPL | 視覺伺服控制 | 中等 | ★★★★☆ | 活躍 |

### 2.2 深度學習檢測框架

| 框架 | 授權 | 精度(mAP) | 速度(FPS) | 部署難度 | 適用場景 |
|-----|------|-----------|-----------|---------|---------|
| **YOLOv8** | AGPL-3.0 | 53.9% | 280 | 低 | 實時缺陷檢測 |
| **YOLOv9** | GPL-3.0 | 55.6% | 240 | 低 | 高精度檢測 |
| **Detectron2** | Apache 2.0 | 58.2% | 45 | 中 | 複雜缺陷分析 |
| **OpenVINO** | Apache 2.0 | - | 優化40% | 低 | 邊緣部署 |

### 2.3 專門化AOI工具

| 工具 | 開源協議 | 應用領域 | 硬體需求 | 社群活躍度 |
|-----|---------|---------|---------|-----------|
| **OpenPnP** | GPL-3.0 | SMT貼片檢測 | 低 | ★★★★★ |
| **LinuxCNC Vision** | GPL-2.0 | CNC視覺定位 | 中 | ★★★☆☆ |
| **ROS2 EPD** | Apache 2.0 | 機器人視覺 | 高 | ★★★★★ |
| **pcb-aoi** | MIT | PCB檢測 | 低 | ★★☆☆☆ |

## 3. 可重用程式碼模組

### 3.1 相機校準模組

```cpp
// 基於OpenCV的工業相機校準模組
class IndustrialCameraCalibrator {
public:
    struct CalibrationParams {
        cv::Size boardSize;        // 標定板尺寸
        float squareSize;           // 方格實際尺寸(mm)
        int calibrationFlags;       // 校準標誌
        double reprojectionError;   // 重投影誤差閾值
    };
    
    struct CalibrationResult {
        cv::Mat cameraMatrix;       // 內參矩陣
        cv::Mat distCoeffs;         // 畸變係數
        std::vector<cv::Mat> rvecs; // 旋轉向量
        std::vector<cv::Mat> tvecs; // 平移向量
        double rmsError;            // RMS誤差
    };
    
    bool calibrate(const std::vector<cv::Mat>& images, 
                   const CalibrationParams& params,
                   CalibrationResult& result) {
        std::vector<std::vector<cv::Point2f>> imagePoints;
        std::vector<std::vector<cv::Point3f>> objectPoints;
        
        // 檢測標定板角點
        for(const auto& image : images) {
            std::vector<cv::Point2f> corners;
            bool found = cv::findChessboardCorners(
                image, params.boardSize, corners,
                cv::CALIB_CB_ADAPTIVE_THRESH | 
                cv::CALIB_CB_NORMALIZE_IMAGE
            );
            
            if(found) {
                cv::Mat gray;
                cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
                cv::cornerSubPix(gray, corners, 
                    cv::Size(11,11), cv::Size(-1,-1),
                    cv::TermCriteria(cv::TermCriteria::EPS + 
                                   cv::TermCriteria::COUNT, 30, 0.1)
                );
                imagePoints.push_back(corners);
                objectPoints.push_back(generateObjectPoints(params));
            }
        }
        
        // 執行相機校準
        result.rmsError = cv::calibrateCamera(
            objectPoints, imagePoints, images[0].size(),
            result.cameraMatrix, result.distCoeffs,
            result.rvecs, result.tvecs,
            params.calibrationFlags
        );
        
        return result.rmsError < params.reprojectionError;
    }
    
private:
    std::vector<cv::Point3f> generateObjectPoints(
        const CalibrationParams& params) {
        std::vector<cv::Point3f> points;
        for(int i = 0; i < params.boardSize.height; i++) {
            for(int j = 0; j < params.boardSize.width; j++) {
                points.push_back(cv::Point3f(
                    j * params.squareSize,
                    i * params.squareSize,
                    0
                ));
            }
        }
        return points;
    }
};
```

### 3.2 YOLOv8缺陷檢測整合模組

```python
# YOLOv8工業缺陷檢測封裝
import cv2
import numpy as np
from ultralytics import YOLO
from typing import List, Dict, Tuple, Optional
import torch

class IndustrialDefectDetector:
    """工業缺陷檢測器 - 基於YOLOv8"""
    
    def __init__(self, model_path: str, 
                 confidence_threshold: float = 0.5,
                 iou_threshold: float = 0.45,
                 device: str = 'auto'):
        """
        初始化檢測器
        
        Args:
            model_path: YOLOv8模型路徑
            confidence_threshold: 置信度閾值
            iou_threshold: IoU閾值
            device: 運行設備 ('cpu', 'cuda', 'auto')
        """
        if device == 'auto':
            device = 'cuda' if torch.cuda.is_available() else 'cpu'
            
        self.model = YOLO(model_path)
        self.model.to(device)
        self.confidence_threshold = confidence_threshold
        self.iou_threshold = iou_threshold
        self.device = device
        
        # 缺陷類型映射
        self.defect_classes = {
            0: 'scratch',      # 刮痕
            1: 'crack',        # 裂紋
            2: 'hole',         # 孔洞
            3: 'stain',        # 污漬
            4: 'deformation',  # 變形
            5: 'inclusion',    # 夾雜
            6: 'bubble',       # 氣泡
            7: 'missing_part'  # 缺件
        }
        
    def detect(self, image: np.ndarray, 
               preprocess: bool = True) -> Dict:
        """
        執行缺陷檢測
        
        Args:
            image: 輸入圖像
            preprocess: 是否預處理
            
        Returns:
            檢測結果字典
        """
        if preprocess:
            image = self._preprocess_image(image)
            
        # 執行推理
        results = self.model(
            image, 
            conf=self.confidence_threshold,
            iou=self.iou_threshold,
            verbose=False
        )[0]
        
        # 解析結果
        detections = []
        if results.boxes is not None:
            for box in results.boxes:
                detection = {
                    'bbox': box.xyxy[0].cpu().numpy().tolist(),
                    'confidence': float(box.conf[0]),
                    'class_id': int(box.cls[0]),
                    'class_name': self.defect_classes.get(
                        int(box.cls[0]), 'unknown'
                    )
                }
                detections.append(detection)
                
        return {
            'detections': detections,
            'total_defects': len(detections),
            'processing_time': results.speed['inference'],
            'image_size': image.shape[:2]
        }
    
    def _preprocess_image(self, image: np.ndarray) -> np.ndarray:
        """圖像預處理"""
        # 自適應直方圖均衡化
        if len(image.shape) == 2:
            clahe = cv2.createCLAHE(clipLimit=2.0, tileGridSize=(8,8))
            image = clahe.apply(image)
            image = cv2.cvtColor(image, cv2.COLOR_GRAY2BGR)
        
        # 降噪
        image = cv2.bilateralFilter(image, 9, 75, 75)
        
        return image
    
    def batch_detect(self, images: List[np.ndarray]) -> List[Dict]:
        """批量檢測"""
        results = []
        for image in images:
            result = self.detect(image)
            results.append(result)
        return results
    
    def visualize_results(self, image: np.ndarray, 
                          detections: Dict,
                          output_path: Optional[str] = None) -> np.ndarray:
        """可視化檢測結果"""
        vis_image = image.copy()
        
        for detection in detections['detections']:
            bbox = detection['bbox']
            x1, y1, x2, y2 = map(int, bbox)
            
            # 繪製邊界框
            color = self._get_color_by_class(detection['class_id'])
            cv2.rectangle(vis_image, (x1, y1), (x2, y2), color, 2)
            
            # 添加標籤
            label = f"{detection['class_name']}: {detection['confidence']:.2f}"
            cv2.putText(vis_image, label, (x1, y1-10),
                       cv2.FONT_HERSHEY_SIMPLEX, 0.5, color, 2)
            
        if output_path:
            cv2.imwrite(output_path, vis_image)
            
        return vis_image
    
    def _get_color_by_class(self, class_id: int) -> Tuple[int, int, int]:
        """根據類別獲取顏色"""
        colors = [
            (255, 0, 0),    # 紅色 - 刮痕
            (0, 0, 255),    # 藍色 - 裂紋
            (255, 255, 0),  # 黃色 - 孔洞
            (255, 0, 255),  # 紫色 - 污漬
            (0, 255, 255),  # 青色 - 變形
            (128, 0, 128),  # 深紫 - 夾雜
            (255, 128, 0),  # 橙色 - 氣泡
            (0, 255, 0)     # 綠色 - 缺件
        ]
        return colors[class_id % len(colors)]
```

### 3.3 PCL點雲處理模組

```cpp
// PCL 3D AOI檢測模組
#include <pcl/point_types.h>
#include <pcl/io/pcd_io.h>
#include <pcl/filters/statistical_outlier_removal.h>
#include <pcl/filters/voxel_grid.h>
#include <pcl/features/normal_3d.h>
#include <pcl/surface/poisson.h>
#include <pcl/registration/icp.h>

class PCL3DInspector {
public:
    using PointT = pcl::PointXYZRGB;
    using PointCloud = pcl::PointCloud<PointT>;
    using PointCloudPtr = PointCloud::Ptr;
    
    struct InspectionResult {
        bool passed;
        std::vector<DefectRegion> defects;
        double deviationRMS;
        pcl::PointCloud<pcl::PointNormal>::Ptr surfaceNormals;
    };
    
    struct DefectRegion {
        pcl::PointIndices indices;
        std::string type;  // "hole", "bump", "scratch"
        double severity;    // 0.0 - 1.0
        Eigen::Vector3f centroid;
    };
    
    // 點雲預處理
    PointCloudPtr preprocessPointCloud(PointCloudPtr cloud) {
        PointCloudPtr filtered(new PointCloud);
        
        // 1. 統計濾波去除離群點
        pcl::StatisticalOutlierRemoval<PointT> sor;
        sor.setInputCloud(cloud);
        sor.setMeanK(50);
        sor.setStddevMulThresh(1.0);
        sor.filter(*filtered);
        
        // 2. 體素網格下採樣
        pcl::VoxelGrid<PointT> vg;
        vg.setInputCloud(filtered);
        vg.setLeafSize(0.01f, 0.01f, 0.01f);
        vg.filter(*filtered);
        
        return filtered;
    }
    
    // 表面缺陷檢測
    InspectionResult inspect3DSurface(
        PointCloudPtr testCloud,
        PointCloudPtr referenceCloud,
        double toleranceMM = 0.5) {
        
        InspectionResult result;
        
        // 預處理
        testCloud = preprocessPointCloud(testCloud);
        referenceCloud = preprocessPointCloud(referenceCloud);
        
        // ICP配準
        pcl::IterativeClosestPoint<PointT, PointT> icp;
        icp.setInputSource(testCloud);
        icp.setInputTarget(referenceCloud);
        icp.setMaximumIterations(50);
        icp.setTransformationEpsilon(1e-8);
        
        PointCloudPtr aligned(new PointCloud);
        icp.align(*aligned);
        
        // 計算偏差
        result.deviationRMS = computeRMSDeviation(aligned, referenceCloud);
        
        // 檢測缺陷區域
        result.defects = detectDefectRegions(
            aligned, referenceCloud, toleranceMM
        );
        
        // 計算表面法線
        result.surfaceNormals = computeSurfaceNormals(aligned);
        
        result.passed = (result.deviationRMS < toleranceMM) && 
                       result.defects.empty();
        
        return result;
    }
    
private:
    double computeRMSDeviation(PointCloudPtr cloud1, 
                               PointCloudPtr cloud2) {
        double sum = 0.0;
        int count = 0;
        
        pcl::KdTreeFLANN<PointT> kdtree;
        kdtree.setInputCloud(cloud2);
        
        for(const auto& point : cloud1->points) {
            std::vector<int> indices;
            std::vector<float> distances;
            
            if(kdtree.nearestKSearch(point, 1, indices, distances) > 0) {
                sum += distances[0];
                count++;
            }
        }
        
        return std::sqrt(sum / count);
    }
    
    std::vector<DefectRegion> detectDefectRegions(
        PointCloudPtr testCloud,
        PointCloudPtr referenceCloud,
        double threshold) {
        
        std::vector<DefectRegion> defects;
        
        // 使用歐幾里得聚類檢測缺陷區域
        pcl::search::KdTree<PointT>::Ptr tree(new pcl::search::KdTree<PointT>);
        tree->setInputCloud(testCloud);
        
        std::vector<pcl::PointIndices> cluster_indices;
        pcl::EuclideanClusterExtraction<PointT> ec;
        ec.setClusterTolerance(threshold * 2);
        ec.setMinClusterSize(10);
        ec.setMaxClusterSize(1000);
        ec.setSearchMethod(tree);
        ec.setInputCloud(testCloud);
        ec.extract(cluster_indices);
        
        for(const auto& cluster : cluster_indices) {
            DefectRegion defect;
            defect.indices = cluster;
            defect.type = classifyDefect(testCloud, cluster);
            defect.severity = calculateSeverity(testCloud, cluster, threshold);
            defect.centroid = calculateCentroid(testCloud, cluster);
            defects.push_back(defect);
        }
        
        return defects;
    }
    
    pcl::PointCloud<pcl::PointNormal>::Ptr computeSurfaceNormals(
        PointCloudPtr cloud) {
        
        pcl::NormalEstimation<PointT, pcl::Normal> ne;
        ne.setInputCloud(cloud);
        
        pcl::search::KdTree<PointT>::Ptr tree(new pcl::search::KdTree<PointT>());
        ne.setSearchMethod(tree);
        
        pcl::PointCloud<pcl::Normal>::Ptr cloud_normals(
            new pcl::PointCloud<pcl::Normal>
        );
        ne.setRadiusSearch(0.03);
        ne.compute(*cloud_normals);
        
        pcl::PointCloud<pcl::PointNormal>::Ptr cloud_with_normals(
            new pcl::PointCloud<pcl::PointNormal>
        );
        pcl::concatenateFields(*cloud, *cloud_normals, *cloud_with_normals);
        
        return cloud_with_normals;
    }
    
    std::string classifyDefect(PointCloudPtr cloud, 
                               const pcl::PointIndices& indices) {
        // 簡化的缺陷分類邏輯
        // 實際應用中需要更複雜的算法
        return "surface_deviation";
    }
    
    double calculateSeverity(PointCloudPtr cloud,
                            const pcl::PointIndices& indices,
                            double threshold) {
        // 計算嚴重程度
        return 0.5; // 簡化實現
    }
    
    Eigen::Vector3f calculateCentroid(PointCloudPtr cloud,
                                     const pcl::PointIndices& indices) {
        Eigen::Vector3f centroid(0, 0, 0);
        for(int idx : indices.indices) {
            centroid += cloud->points[idx].getVector3fMap();
        }
        centroid /= indices.indices.size();
        return centroid;
    }
};
```

### 3.4 OpenVINO邊緣部署優化模組

```python
# OpenVINO 2025邊緣部署優化
import openvino as ov
import numpy as np
import cv2
from pathlib import Path
from typing import Dict, List, Tuple, Optional
import time

class OpenVINOEdgeDeployer:
    """OpenVINO邊緣部署優化器"""
    
    def __init__(self, model_path: str, 
                 device: str = "AUTO",
                 performance_mode: str = "LATENCY"):
        """
        初始化OpenVINO推理引擎
        
        Args:
            model_path: 模型路徑 (.xml, .onnx, .pb等)
            device: 目標設備 (CPU, GPU, AUTO等)
            performance_mode: 性能模式 (LATENCY, THROUGHPUT)
        """
        self.core = ov.Core()
        self.model_path = Path(model_path)
        self.device = device
        self.performance_mode = performance_mode
        
        # 配置性能優化
        self.config = self._get_performance_config()
        
        # 載入並編譯模型
        self.model = self.core.read_model(str(self.model_path))
        self._optimize_model()
        self.compiled_model = self.core.compile_model(
            self.model, device, self.config
        )
        
        # 創建推理請求
        self.infer_request = self.compiled_model.create_infer_request()
        
        # 獲取輸入輸出資訊
        self.input_layer = self.compiled_model.input(0)
        self.output_layer = self.compiled_model.output(0)
        
    def _get_performance_config(self) -> Dict:
        """獲取性能配置"""
        config = {}
        
        if self.performance_mode == "LATENCY":
            config = {
                "PERFORMANCE_HINT": "LATENCY",
                "PERFORMANCE_HINT_NUM_REQUESTS": "1",
                "CACHE_DIR": "./model_cache"
            }
        elif self.performance_mode == "THROUGHPUT":
            config = {
                "PERFORMANCE_HINT": "THROUGHPUT",
                "PERFORMANCE_HINT_NUM_REQUESTS": "0",  # 自動
                "CACHE_DIR": "./model_cache"
            }
            
        # CPU特定優化
        if "CPU" in self.device:
            config.update({
                "CPU_THREADS_NUM": "0",  # 自動
                "CPU_BIND_THREAD": "NUMA",
                "CPU_THROUGHPUT_STREAMS": "AUTO"
            })
            
        # GPU特定優化
        if "GPU" in self.device:
            config.update({
                "GPU_THROUGHPUT_STREAMS": "AUTO",
                "GPU_PLUGIN_THROTTLE": "1"
            })
            
        return config
    
    def _optimize_model(self):
        """模型優化"""
        # 應用量化優化
        pass_manager = ov.passes.Manager()
        
        # FP16優化 (適用於GPU)
        if "GPU" in self.device:
            pass_manager.register_pass(
                ov.passes.ConvertPrecision(
                    ov.Type.f32, ov.Type.f16
                )
            )
            
        # INT8量化準備
        # 注意：實際INT8量化需要校準數據集
        
        pass_manager.run_passes(self.model)
        
    def preprocess(self, image: np.ndarray) -> np.ndarray:
        """圖像預處理"""
        # 獲取模型輸入尺寸
        input_shape = self.input_layer.shape
        
        if len(input_shape) == 4:  # NCHW格式
            _, channels, height, width = input_shape
        else:
            height, width = input_shape[-2:]
            channels = 3
            
        # 調整尺寸
        resized = cv2.resize(image, (width, height))
        
        # 歸一化
        normalized = resized.astype(np.float32) / 255.0
        
        # 轉換為NCHW格式
        if len(normalized.shape) == 2:  # 灰度圖
            normalized = np.expand_dims(normalized, axis=0)
        else:  # BGR to RGB
            normalized = cv2.cvtColor(normalized, cv2.COLOR_BGR2RGB)
            normalized = normalized.transpose(2, 0, 1)
            
        # 添加批次維度
        batch = np.expand_dims(normalized, axis=0)
        
        return batch
    
    def infer(self, image: np.ndarray) -> Dict:
        """執行推理"""
        start_time = time.perf_counter()
        
        # 預處理
        input_data = self.preprocess(image)
        
        # 執行推理
        self.infer_request.infer({self.input_layer.any_name: input_data})
        
        # 獲取結果
        output = self.infer_request.get_output_tensor(0).data
        
        # 計算延遲
        latency = (time.perf_counter() - start_time) * 1000  # ms
        
        return {
            "output": output,
            "latency": latency,
            "input_shape": input_data.shape,
            "output_shape": output.shape
        }
    
    def benchmark(self, num_iterations: int = 100) -> Dict:
        """性能基準測試"""
        # 創建虛擬輸入
        input_shape = self.input_layer.shape
        dummy_input = np.random.randn(*input_shape).astype(np.float32)
        
        # 預熱
        for _ in range(10):
            self.infer_request.infer({self.input_layer.any_name: dummy_input})
            
        # 基準測試
        latencies = []
        for _ in range(num_iterations):
            start = time.perf_counter()
            self.infer_request.infer({self.input_layer.any_name: dummy_input})
            latencies.append((time.perf_counter() - start) * 1000)
            
        return {
            "avg_latency": np.mean(latencies),
            "min_latency": np.min(latencies),
            "max_latency": np.max(latencies),
            "std_latency": np.std(latencies),
            "throughput": 1000 / np.mean(latencies),  # FPS
            "device": self.device,
            "performance_mode": self.performance_mode
        }
    
    def export_optimized_model(self, output_path: str):
        """導出優化後的模型"""
        ov.save_model(self.model, output_path)
        print(f"優化模型已保存至: {output_path}")
```

## 4. Qt6.9整合策略

### 4.1 CMake整合配置

```cmake
# CMakeLists.txt - Qt6與OpenCV/PCL整合
cmake_minimum_required(VERSION 3.16)
project(IndustrialVisionSystem VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 查找Qt6
find_package(Qt6 6.9 REQUIRED COMPONENTS 
    Core 
    Widgets 
    Concurrent 
    Network 
    Multimedia
)

# 查找OpenCV
find_package(OpenCV 4.9 REQUIRED)

# 查找PCL (可選)
find_package(PCL 1.12 QUIET COMPONENTS 
    common 
    io 
    filters 
    features 
    registration
)

# 查找OpenVINO (可選)
find_package(OpenVINO QUIET)

# 定義源文件
set(SOURCES
    src/main.cpp
    src/vision/VisionProcessor.cpp
    src/vision/CameraCalibration.cpp
    src/vision/DefectDetector.cpp
    src/ui/MainWindow.cpp
    src/ui/VisionWidget.cpp
)

set(HEADERS
    include/vision/VisionProcessor.h
    include/vision/CameraCalibration.h
    include/vision/DefectDetector.h
    include/ui/MainWindow.h
    include/ui/VisionWidget.h
)

# 創建可執行文件
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# 鏈接庫
target_link_libraries(${PROJECT_NAME} 
    PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Concurrent
    Qt6::Network
    Qt6::Multimedia
    ${OpenCV_LIBS}
)

# PCL支持
if(PCL_FOUND)
    target_compile_definitions(${PROJECT_NAME} PRIVATE HAS_PCL)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${PCL_LIBRARIES})
    target_include_directories(${PROJECT_NAME} PRIVATE ${PCL_INCLUDE_DIRS})
endif()

# OpenVINO支持
if(OpenVINO_FOUND)
    target_compile_definitions(${PROJECT_NAME} PRIVATE HAS_OPENVINO)
    target_link_libraries(${PROJECT_NAME} PRIVATE openvino::runtime)
endif()

# 設置包含目錄
target_include_directories(${PROJECT_NAME} 
    PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${OpenCV_INCLUDE_DIRS}
)

# Windows特定設置
if(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()

# 安裝規則
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
```

### 4.2 Qt6視覺處理核心類

```cpp
// VisionProcessor.h - Qt6整合的視覺處理核心
#pragma once

#include <QObject>
#include <QImage>
#include <QFuture>
#include <QFutureWatcher>
#include <QtConcurrent>
#include <opencv2/opencv.hpp>
#include <memory>

class VisionProcessor : public QObject {
    Q_OBJECT
    
public:
    explicit VisionProcessor(QObject *parent = nullptr);
    ~VisionProcessor();
    
    // 圖像轉換工具
    static QImage matToQImage(const cv::Mat& mat);
    static cv::Mat qImageToMat(const QImage& image);
    
    // 異步處理介面
    void processImageAsync(const QImage& image);
    void detectDefectsAsync(const cv::Mat& image);
    
    // 配置方法
    void setProcessingPipeline(const QJsonObject& config);
    void loadModel(const QString& modelPath);
    
signals:
    void processingCompleted(const QImage& result);
    void defectsDetected(const QJsonArray& defects);
    void errorOccurred(const QString& error);
    void progressUpdated(int percentage);
    
private slots:
    void handleProcessingResult();
    void handleDefectDetectionResult();
    
private:
    struct Impl;
    std::unique_ptr<Impl> d;
    
    // 處理管線
    cv::Mat preprocessImage(const cv::Mat& input);
    cv::Mat enhanceImage(const cv::Mat& input);
    std::vector<cv::Rect> detectRegions(const cv::Mat& input);
    
    // 異步處理
    QFutureWatcher<cv::Mat>* m_processWatcher;
    QFutureWatcher<QJsonArray>* m_defectWatcher;
};

// VisionProcessor.cpp - 實現
#include "VisionProcessor.h"
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>

struct VisionProcessor::Impl {
    cv::Ptr<cv::Feature2D> featureDetector;
    cv::Ptr<cv::DescriptorMatcher> matcher;
    std::vector<cv::Mat> processingPipeline;
    
    // YOLOv8整合
    bool useYolo = false;
    std::string yoloModelPath;
    
    // OpenVINO整合
    #ifdef HAS_OPENVINO
    bool useOpenVINO = false;
    #endif
};

VisionProcessor::VisionProcessor(QObject *parent) 
    : QObject(parent), d(std::make_unique<Impl>()) {
    
    m_processWatcher = new QFutureWatcher<cv::Mat>(this);
    m_defectWatcher = new QFutureWatcher<QJsonArray>(this);
    
    connect(m_processWatcher, &QFutureWatcher<cv::Mat>::finished,
            this, &VisionProcessor::handleProcessingResult);
    connect(m_defectWatcher, &QFutureWatcher<QJsonArray>::finished,
            this, &VisionProcessor::handleDefectDetectionResult);
    
    // 初始化特徵檢測器
    d->featureDetector = cv::ORB::create();
    d->matcher = cv::DescriptorMatcher::create("BruteForce-Hamming");
}

VisionProcessor::~VisionProcessor() = default;

QImage VisionProcessor::matToQImage(const cv::Mat& mat) {
    if(mat.empty()) return QImage();
    
    switch(mat.type()) {
        case CV_8UC1: {
            QImage image(mat.data, mat.cols, mat.rows, 
                        mat.step, QImage::Format_Grayscale8);
            return image.copy();
        }
        case CV_8UC3: {
            cv::Mat rgb;
            cv::cvtColor(mat, rgb, cv::COLOR_BGR2RGB);
            QImage image(rgb.data, rgb.cols, rgb.rows, 
                        rgb.step, QImage::Format_RGB888);
            return image.copy();
        }
        case CV_8UC4: {
            QImage image(mat.data, mat.cols, mat.rows, 
                        mat.step, QImage::Format_RGBA8888);
            return image.copy();
        }
        default:
            return QImage();
    }
}

cv::Mat VisionProcessor::qImageToMat(const QImage& image) {
    switch(image.format()) {
        case QImage::Format_RGB888: {
            cv::Mat mat(image.height(), image.width(), CV_8UC3, 
                       const_cast<uchar*>(image.bits()), image.bytesPerLine());
            cv::Mat bgr;
            cv::cvtColor(mat, bgr, cv::COLOR_RGB2BGR);
            return bgr.clone();
        }
        case QImage::Format_RGBA8888: {
            cv::Mat mat(image.height(), image.width(), CV_8UC4,
                       const_cast<uchar*>(image.bits()), image.bytesPerLine());
            return mat.clone();
        }
        case QImage::Format_Grayscale8: {
            cv::Mat mat(image.height(), image.width(), CV_8UC1,
                       const_cast<uchar*>(image.bits()), image.bytesPerLine());
            return mat.clone();
        }
        default: {
            QImage converted = image.convertToFormat(QImage::Format_RGB888);
            return qImageToMat(converted);
        }
    }
}

void VisionProcessor::processImageAsync(const QImage& image) {
    auto future = QtConcurrent::run([this, image]() {
        cv::Mat mat = qImageToMat(image);
        cv::Mat processed = preprocessImage(mat);
        processed = enhanceImage(processed);
        return processed;
    });
    
    m_processWatcher->setFuture(future);
}

void VisionProcessor::detectDefectsAsync(const cv::Mat& image) {
    auto future = QtConcurrent::run([this, image]() {
        QJsonArray defects;
        
        // 檢測缺陷區域
        std::vector<cv::Rect> regions = detectRegions(image);
        
        for(const auto& rect : regions) {
            QJsonObject defect;
            defect["x"] = rect.x;
            defect["y"] = rect.y;
            defect["width"] = rect.width;
            defect["height"] = rect.height;
            defect["confidence"] = 0.95;
            defect["type"] = "surface_defect";
            defects.append(defect);
        }
        
        return defects;
    });
    
    m_defectWatcher->setFuture(future);
}

void VisionProcessor::handleProcessingResult() {
    cv::Mat result = m_processWatcher->result();
    QImage qimage = matToQImage(result);
    emit processingCompleted(qimage);
}

void VisionProcessor::handleDefectDetectionResult() {
    QJsonArray defects = m_defectWatcher->result();
    emit defectsDetected(defects);
}

cv::Mat VisionProcessor::preprocessImage(const cv::Mat& input) {
    cv::Mat output;
    
    // 降噪
    cv::bilateralFilter(input, output, 9, 75, 75);
    
    // 增強對比度
    cv::Mat lab;
    cv::cvtColor(output, lab, cv::COLOR_BGR2Lab);
    std::vector<cv::Mat> channels;
    cv::split(lab, channels);
    
    cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE();
    clahe->setClipLimit(2.0);
    clahe->setTilesGridSize(cv::Size(8, 8));
    clahe->apply(channels[0], channels[0]);
    
    cv::merge(channels, lab);
    cv::cvtColor(lab, output, cv::COLOR_Lab2BGR);
    
    return output;
}

cv::Mat VisionProcessor::enhanceImage(const cv::Mat& input) {
    cv::Mat output;
    
    // 銳化
    cv::Mat kernel = (cv::Mat_<float>(3, 3) << 
        0, -1, 0,
        -1, 5, -1,
        0, -1, 0);
    cv::filter2D(input, output, -1, kernel);
    
    return output;
}

std::vector<cv::Rect> VisionProcessor::detectRegions(const cv::Mat& input) {
    std::vector<cv::Rect> regions;
    
    // 簡化的缺陷檢測邏輯
    cv::Mat gray, binary;
    cv::cvtColor(input, gray, cv::COLOR_BGR2GRAY);
    cv::threshold(gray, binary, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);
    
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(binary, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    
    for(const auto& contour : contours) {
        double area = cv::contourArea(contour);
        if(area > 100 && area < 10000) {
            regions.push_back(cv::boundingRect(contour));
        }
    }
    
    return regions;
}
```

## 5. 最佳實踐和技術選型建議

### 5.1 影像處理管線設計

#### 標準處理流程
1. **圖像獲取** → 2. **預處理** → 3. **特徵提取** → 4. **缺陷檢測** → 5. **後處理** → 6. **結果輸出**

#### 預處理最佳實踐
- 使用自適應直方圖均衡化(CLAHE)改善對比度
- 雙邊濾波保邊降噪
- 形態學操作去除小噪點
- 透視變換校正畸變

### 5.2 校準流程標準化

#### 相機校準檢查清單
- [ ] 標定板覆蓋視野50%以上
- [ ] 至少採集20張不同角度圖像
- [ ] 重投影誤差 < 0.5像素
- [ ] 定期重新校準（建議每月）
- [ ] 多相機系統需要外參校準

### 5.3 缺陷檢測演算法選擇

| 缺陷類型 | 推薦算法 | 開源工具 | 準確率 |
|---------|---------|---------|--------|
| 表面刮痕 | YOLOv8 + 邊緣檢測 | OpenCV + Ultralytics | >95% |
| 尺寸測量 | 亞像素邊緣定位 | OpenCV | ±0.1mm |
| 3D缺陷 | 點雲配準 + 偏差分析 | PCL | >90% |
| 紋理異常 | 深度學習分類 | Detectron2 | >92% |
| 組裝錯誤 | 模板匹配 + SIFT | OpenCV | >98% |

### 5.4 實時處理架構

#### 多線程處理模型
```
主線程(UI) ─┬─> 採集線程 ──> 圖像隊列
           │
           ├─> 處理線程1 ─┬─> 結果隊列
           ├─> 處理線程2 ─┤
           └─> 處理線程N ─┘
```

#### 性能優化建議
- 使用OpenVINO進行模型推理加速
- 實施圖像金字塔減少計算量
- ROI處理避免全圖掃描
- 批處理提高GPU利用率

### 5.5 測量精度保證

#### 精度影響因素
1. **硬體因素**
   - 相機分辨率（建議 > 500萬像素）
   - 鏡頭畸變（使用低畸變鏡頭）
   - 光源穩定性（LED光源 + 控制器）

2. **軟體因素**
   - 亞像素精度算法
   - 多幀平均降噪
   - 溫度補償

## 6. 授權合規指南

### 6.1 開源授權對比

| 授權類型 | 商業使用 | 修改後閉源 | 專利授權 | 代表專案 |
|---------|---------|-----------|---------|---------|
| Apache 2.0 | ✓ | ✓ | ✓ | OpenCV, OpenVINO |
| BSD | ✓ | ✓ | - | PCL, SimpleCV |
| MIT | ✓ | ✓ | - | 多數npm包 |
| GPL-3.0 | ✓ | ✗ | - | YOLOv8, LinuxCNC |
| LGPL | ✓ | 有限制 | - | Qt (開源版) |

### 6.2 合規建議
1. **商業專案優先選擇**：Apache 2.0、BSD、MIT授權
2. **GPL注意事項**：避免靜態鏈接，使用動態庫
3. **文檔要求**：保留原始授權聲明和版權信息
4. **專利風險**：Apache 2.0提供專利保護

## 7. 技術選型決策樹

```
開始
│
├─ 2D檢測需求?
│  ├─ 是 → 實時要求?
│  │      ├─ 高 → YOLOv8/v9 + OpenVINO
│  │      └─ 低 → Detectron2 + OpenCV
│  └─ 否 → 3D檢測?
│         ├─ 是 → PCL + Open3D
│         └─ 否 → 傳統視覺
│
├─ 部署環境?
│  ├─ 邊緣設備 → OpenVINO + TensorRT Lite
│  ├─ 工業PC → OpenCV + CUDA
│  └─ 雲端 → 任意框架
│
└─ 開發資源?
   ├─ 充足 → 自定義開發
   └─ 有限 → 整合現有方案
```

## 8. 整合範例專案結構

```
IndustrialVisionSystem/
├── CMakeLists.txt
├── README.md
├── LICENSE
├── config/
│   ├── camera_calibration.yaml
│   ├── detection_params.json
│   └── system_config.xml
├── src/
│   ├── main.cpp
│   ├── core/
│   │   ├── VisionEngine.cpp
│   │   ├── ImageProcessor.cpp
│   │   └── DefectAnalyzer.cpp
│   ├── detection/
│   │   ├── YoloDetector.cpp
│   │   ├── PCLInspector.cpp
│   │   └── EdgeDetector.cpp
│   ├── calibration/
│   │   ├── CameraCalibrator.cpp
│   │   └── StereoCalibrator.cpp
│   └── ui/
│       ├── MainWindow.cpp
│       ├── VisionWidget.cpp
│       └── ResultDisplay.cpp
├── include/
│   └── [對應頭文件]
├── models/
│   ├── yolov8_defects.onnx
│   └── openvino_optimized.xml
├── scripts/
│   ├── train_model.py
│   ├── benchmark.py
│   └── deploy.sh
└── tests/
    ├── unit/
    └── integration/
```

## 9. 性能基準和優化指標

### 9.1 2024年性能基準

| 框架/工具 | CPU推理(ms) | GPU推理(ms) | 模型大小(MB) | 準確率 |
|----------|------------|-------------|-------------|--------|
| YOLOv8s | 45 | 8 | 22 | 95.5% |
| YOLOv9c | 52 | 10 | 25 | 97.2% |
| OpenVINO優化 | 18 | 5 | 15 | 95.0% |
| TensorRT | - | 4 | 18 | 95.2% |

### 9.2 優化目標
- **延遲目標**：< 50ms（20 FPS）
- **準確率目標**：> 95%
- **誤報率**：< 2%
- **漏檢率**：< 1%

## 10. 結論和未來展望

### 10.1 當前最佳組合
- **基礎框架**：Qt6.9 + OpenCV 4.9
- **深度學習**：YOLOv8/v9 + OpenVINO
- **3D處理**：PCL 1.14
- **機器人整合**：ROS2 + MoveIt2

### 10.2 2025年趨勢預測
1. **邊緣AI普及**：更多模型直接部署在相機端
2. **多模態融合**：2D + 3D + 熱成像綜合檢測
3. **自監督學習**：減少標註數據需求
4. **實時3D重建**：毫秒級點雲處理
5. **量子計算應用**：複雜優化問題求解

### 10.3 持續更新建議
- 訂閱相關GitHub專案更新
- 參與開源社群討論
- 定期評估新技術
- 建立內部知識庫
- 進行定期培訓

---

**文檔版本**：1.0.0  
**最後更新**：2024年12月  
**維護團隊**：工業視覺開發組  
**聯繫方式**：[內部郵件列表]

**附錄**：
- A. 完整API參考
- B. 故障排除指南
- C. 性能調優檢查表
- D. 供應商聯繫清單