# 測試策略與效能基準 - 工業AOI設備元件生態系統

## 目錄
1. [測試策略總覽](#測試策略總覽)
2. [單元測試框架](#單元測試框架)
3. [整合測試](#整合測試)
4. [效能測試](#效能測試)
5. [系統測試](#系統測試)
6. [自動化測試](#自動化測試)
7. [效能基準](#效能基準)
8. [測試最佳實踐](#測試最佳實踐)

---

## 測試策略總覽

### 測試金字塔架構
```
         /\
        /  \  系統測試 (10%)
       /    \
      /------\  整合測試 (30%)
     /        \
    /----------\  單元測試 (60%)
```

### 測試覆蓋率目標
- 單元測試覆蓋率: >80%
- 整合測試覆蓋率: >70%
- 關鍵路徑覆蓋率: 100%
- 錯誤處理覆蓋率: >90%

---

## 單元測試框架

### 1. Qt Test Framework 實作

```cpp
// tests/unit/test_image_processor.cpp
#include <QtTest>
#include <QSignalSpy>
#include "components/image_processor.h"

class TestImageProcessor : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        // 測試套件初始化
        m_processor = new ImageProcessor();
    }

    void cleanupTestCase() {
        // 測試套件清理
        delete m_processor;
    }

    void init() {
        // 每個測試案例前執行
        m_processor->reset();
    }

    void cleanup() {
        // 每個測試案例後執行
    }

    // 測試影像載入功能
    void testImageLoading() {
        // Arrange
        QString testImagePath = "test_data/sample.png";
        
        // Act
        bool result = m_processor->loadImage(testImagePath);
        
        // Assert
        QVERIFY(result);
        QCOMPARE(m_processor->getImageWidth(), 1920);
        QCOMPARE(m_processor->getImageHeight(), 1080);
    }

    // 測試Signal/Slot機制
    void testProcessingSignals() {
        // Arrange
        QSignalSpy startSpy(m_processor, &ImageProcessor::processingStarted);
        QSignalSpy completeSpy(m_processor, &ImageProcessor::processingCompleted);
        
        // Act
        m_processor->startProcessing();
        
        // Assert
        QCOMPARE(startSpy.count(), 1);
        QTRY_COMPARE_WITH_TIMEOUT(completeSpy.count(), 1, 5000);
    }

    // 測試效能基準
    void benchmarkProcessing() {
        QBENCHMARK {
            m_processor->processFrame(m_testFrame);
        }
    }

    // 測試錯誤處理
    void testErrorHandling() {
        // Arrange
        QString invalidPath = "non_existent.png";
        
        // Act & Assert
        QVERIFY_EXCEPTION_THROWN(
            m_processor->loadImage(invalidPath),
            std::runtime_error
        );
    }

private:
    ImageProcessor* m_processor;
    cv::Mat m_testFrame;
};

QTEST_MAIN(TestImageProcessor)
#include "test_image_processor.moc"
```

### 2. Google Test 整合

```cpp
// tests/unit/test_defect_detector.cpp
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "components/defect_detector.h"
#include "mocks/mock_image_source.h"

using ::testing::Return;
using ::testing::_;
using ::testing::AtLeast;

// Mock物件定義
class MockImageSource : public IImageSource {
public:
    MOCK_METHOD(cv::Mat, getFrame, (), (override));
    MOCK_METHOD(bool, isReady, (), (const, override));
    MOCK_METHOD(void, setFormat, (ImageFormat format), (override));
};

// Test Fixture
class DefectDetectorTest : public ::testing::Test {
protected:
    void SetUp() override {
        mockSource = std::make_shared<MockImageSource>();
        detector = std::make_unique<DefectDetector>(mockSource);
        
        // 準備測試數據
        testImage = cv::imread("test_data/defect_sample.png");
    }

    void TearDown() override {
        // 清理資源
    }

    std::shared_ptr<MockImageSource> mockSource;
    std::unique_ptr<DefectDetector> detector;
    cv::Mat testImage;
};

// 測試缺陷檢測精度
TEST_F(DefectDetectorTest, DetectsDefectsAccurately) {
    // Arrange
    EXPECT_CALL(*mockSource, getFrame())
        .WillOnce(Return(testImage));
    EXPECT_CALL(*mockSource, isReady())
        .WillRepeatedly(Return(true));
    
    // Act
    auto defects = detector->detectDefects();
    
    // Assert
    ASSERT_EQ(defects.size(), 3);
    EXPECT_NEAR(defects[0].confidence, 0.95, 0.05);
    EXPECT_EQ(defects[0].type, DefectType::SCRATCH);
}

// 測試效能要求
TEST_F(DefectDetectorTest, MeetsPerformanceRequirements) {
    // Arrange
    const int iterations = 1000;
    auto start = std::chrono::high_resolution_clock::now();
    
    // Act
    for (int i = 0; i < iterations; ++i) {
        detector->processFrame(testImage);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Assert - 應達到100+ FPS
    double fps = (iterations * 1000.0) / duration.count();
    EXPECT_GT(fps, 100.0);
}

// 參數化測試
class DefectDetectorParamTest : public DefectDetectorTest,
                                public ::testing::WithParamInterface<std::tuple<int, int, double>> {
};

TEST_P(DefectDetectorParamTest, HandlesVariousImageSizes) {
    // 獲取測試參數
    int width = std::get<0>(GetParam());
    int height = std::get<1>(GetParam());
    double expectedFps = std::get<2>(GetParam());
    
    // 創建測試影像
    cv::Mat testImg(height, width, CV_8UC3);
    
    // 測試處理速度
    auto start = std::chrono::high_resolution_clock::now();
    detector->processFrame(testImg);
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    double actualFps = 1000000.0 / duration.count();
    
    EXPECT_GE(actualFps, expectedFps);
}

INSTANTIATE_TEST_SUITE_P(
    ImageSizeTests,
    DefectDetectorParamTest,
    ::testing::Values(
        std::make_tuple(640, 480, 500.0),    // VGA
        std::make_tuple(1920, 1080, 200.0),  // Full HD
        std::make_tuple(3840, 2160, 100.0)   // 4K
    )
);
```

### 3. Mock物件設計模式

```cpp
// tests/mocks/mock_communication.h
#pragma once
#include <gmock/gmock.h>
#include "interfaces/icommunication.h"

class MockCommunication : public ICommunication {
public:
    MOCK_METHOD(bool, connect, (const QString& address, int port), (override));
    MOCK_METHOD(void, disconnect, (), (override));
    MOCK_METHOD(bool, sendData, (const QByteArray& data), (override));
    MOCK_METHOD(QByteArray, receiveData, (int timeout), (override));
    MOCK_METHOD(bool, isConnected, (), (const, override));
    MOCK_METHOD(void, setCallback, (std::function<void(const QByteArray&)> callback), (override));
};

// 測試輔助類
class TestHelper {
public:
    static std::unique_ptr<MockCommunication> createMockComm() {
        auto mock = std::make_unique<MockCommunication>();
        
        // 設定預設行為
        ON_CALL(*mock, isConnected()).WillByDefault(Return(true));
        ON_CALL(*mock, sendData(_)).WillByDefault(Return(true));
        
        return mock;
    }
    
    // 測試數據工廠
    static cv::Mat createTestImage(int width, int height, int channels = 3) {
        cv::Mat img(height, width, CV_MAKETYPE(CV_8U, channels));
        cv::randu(img, cv::Scalar::all(0), cv::Scalar::all(255));
        return img;
    }
    
    // 缺陷數據生成器
    static DefectData createTestDefect(DefectType type, double confidence) {
        DefectData defect;
        defect.type = type;
        defect.confidence = confidence;
        defect.boundingBox = cv::Rect(100, 100, 50, 50);
        defect.timestamp = QDateTime::currentDateTime();
        return defect;
    }
};
```

---

## 整合測試

### 1. 元件間通訊測試

```cpp
// tests/integration/test_component_communication.cpp
#include <gtest/gtest.h>
#include "components/image_acquisition.h"
#include "components/image_processor.h"
#include "components/defect_detector.h"

class ComponentIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 初始化元件
        acquisition = std::make_shared<ImageAcquisition>();
        processor = std::make_shared<ImageProcessor>();
        detector = std::make_shared<DefectDetector>();
        
        // 建立元件連接
        connectComponents();
    }
    
    void connectComponents() {
        // 連接信號槽
        QObject::connect(
            acquisition.get(), &ImageAcquisition::frameReady,
            processor.get(), &ImageProcessor::processFrame
        );
        
        QObject::connect(
            processor.get(), &ImageProcessor::frameProcessed,
            detector.get(), &DefectDetector::analyzeFrame
        );
    }
    
    std::shared_ptr<ImageAcquisition> acquisition;
    std::shared_ptr<ImageProcessor> processor;
    std::shared_ptr<DefectDetector> detector;
};

// 測試資料流管線
TEST_F(ComponentIntegrationTest, DataFlowPipeline) {
    // Arrange
    QSignalSpy detectSpy(detector.get(), &DefectDetector::defectDetected);
    
    // Act - 觸發影像擷取
    acquisition->captureFrame();
    
    // Assert - 驗證完整管線執行
    QTRY_VERIFY_WITH_TIMEOUT(detectSpy.count() > 0, 5000);
    
    // 驗證處理時間
    auto totalTime = detector->getLastProcessingTime();
    EXPECT_LT(totalTime, 100); // < 100ms
}
```

### 2. 狀態機轉換測試

```cpp
// tests/integration/test_state_machine.cpp
class StateMachineTest : public ::testing::Test {
protected:
    void SetUp() override {
        stateMachine = std::make_unique<SystemStateMachine>();
    }
    
    std::unique_ptr<SystemStateMachine> stateMachine;
};

TEST_F(StateMachineTest, ValidStateTransitions) {
    // 測試正常狀態轉換
    EXPECT_EQ(stateMachine->currentState(), State::IDLE);
    
    EXPECT_TRUE(stateMachine->transitionTo(State::INITIALIZING));
    EXPECT_EQ(stateMachine->currentState(), State::INITIALIZING);
    
    EXPECT_TRUE(stateMachine->transitionTo(State::READY));
    EXPECT_EQ(stateMachine->currentState(), State::READY);
    
    EXPECT_TRUE(stateMachine->transitionTo(State::PROCESSING));
    EXPECT_EQ(stateMachine->currentState(), State::PROCESSING);
}

TEST_F(StateMachineTest, InvalidStateTransitions) {
    // 測試無效狀態轉換
    EXPECT_FALSE(stateMachine->transitionTo(State::PROCESSING));
    EXPECT_EQ(stateMachine->currentState(), State::IDLE);
}
```

### 3. 並發測試

```cpp
// tests/integration/test_concurrency.cpp
TEST(ConcurrencyTest, ThreadSafeImageBuffer) {
    ThreadSafeImageBuffer buffer(100);
    std::atomic<int> producedCount{0};
    std::atomic<int> consumedCount{0};
    
    // 生產者執行緒
    std::vector<std::thread> producers;
    for (int i = 0; i < 4; ++i) {
        producers.emplace_back([&buffer, &producedCount]() {
            for (int j = 0; j < 250; ++j) {
                cv::Mat img = TestHelper::createTestImage(640, 480);
                buffer.push(img);
                producedCount++;
            }
        });
    }
    
    // 消費者執行緒
    std::vector<std::thread> consumers;
    for (int i = 0; i < 4; ++i) {
        consumers.emplace_back([&buffer, &consumedCount]() {
            for (int j = 0; j < 250; ++j) {
                cv::Mat img;
                if (buffer.pop(img, 100)) {
                    consumedCount++;
                }
            }
        });
    }
    
    // 等待所有執行緒完成
    for (auto& t : producers) t.join();
    for (auto& t : consumers) t.join();
    
    // 驗證數據一致性
    EXPECT_EQ(producedCount.load(), 1000);
    EXPECT_EQ(consumedCount.load(), 1000);
}
```

### 4. 記憶體洩漏檢測

```cpp
// tests/integration/test_memory_leaks.cpp
#include <valgrind/memcheck.h>

class MemoryLeakTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 記錄初始記憶體使用
        initialMemory = getCurrentMemoryUsage();
    }
    
    void TearDown() override {
        // 檢查記憶體洩漏
        size_t finalMemory = getCurrentMemoryUsage();
        size_t leakedMemory = finalMemory - initialMemory;
        
        EXPECT_LT(leakedMemory, 1024 * 1024); // < 1MB洩漏容忍度
    }
    
    size_t getCurrentMemoryUsage() {
        // 獲取當前進程記憶體使用量
        std::ifstream status("/proc/self/status");
        std::string line;
        while (std::getline(status, line)) {
            if (line.substr(0, 6) == "VmRSS:") {
                std::istringstream iss(line);
                std::string label;
                size_t memory;
                iss >> label >> memory;
                return memory * 1024; // 轉換為bytes
            }
        }
        return 0;
    }
    
    size_t initialMemory;
};

TEST_F(MemoryLeakTest, ImageProcessingNoLeak) {
    // 重複執行影像處理操作
    for (int i = 0; i < 1000; ++i) {
        auto processor = std::make_unique<ImageProcessor>();
        cv::Mat img = TestHelper::createTestImage(1920, 1080);
        processor->processFrame(img);
    }
    // TearDown會自動檢查記憶體洩漏
}
```

---

## 效能測試

### 1. FPS基準測試

```cpp
// tests/performance/test_fps_benchmark.cpp
class FPSBenchmark : public ::testing::Test {
public:
    struct BenchmarkResult {
        double avgFPS;
        double minFPS;
        double maxFPS;
        double stdDev;
        std::vector<double> frameTimes;
    };
    
    BenchmarkResult runBenchmark(
        std::function<void(const cv::Mat&)> processFunc,
        int width, int height, int frames = 1000
    ) {
        BenchmarkResult result;
        result.frameTimes.reserve(frames);
        
        cv::Mat testFrame(height, width, CV_8UC3);
        cv::randu(testFrame, cv::Scalar::all(0), cv::Scalar::all(255));
        
        // 預熱
        for (int i = 0; i < 10; ++i) {
            processFunc(testFrame);
        }
        
        // 實際測試
        for (int i = 0; i < frames; ++i) {
            auto start = std::chrono::high_resolution_clock::now();
            processFunc(testFrame);
            auto end = std::chrono::high_resolution_clock::now();
            
            double frameTime = std::chrono::duration<double, std::milli>(end - start).count();
            result.frameTimes.push_back(frameTime);
        }
        
        // 計算統計數據
        calculateStatistics(result);
        return result;
    }
    
private:
    void calculateStatistics(BenchmarkResult& result) {
        double sum = 0;
        result.minFPS = std::numeric_limits<double>::max();
        result.maxFPS = 0;
        
        for (double time : result.frameTimes) {
            double fps = 1000.0 / time;
            sum += fps;
            result.minFPS = std::min(result.minFPS, fps);
            result.maxFPS = std::max(result.maxFPS, fps);
        }
        
        result.avgFPS = sum / result.frameTimes.size();
        
        // 計算標準差
        double variance = 0;
        for (double time : result.frameTimes) {
            double fps = 1000.0 / time;
            variance += std::pow(fps - result.avgFPS, 2);
        }
        result.stdDev = std::sqrt(variance / result.frameTimes.size());
    }
};

TEST_F(FPSBenchmark, ImageProcessingFPS) {
    ImageProcessor processor;
    
    // 測試不同解析度
    std::vector<std::pair<int, int>> resolutions = {
        {640, 480},    // VGA
        {1280, 720},   // HD
        {1920, 1080},  // Full HD
        {3840, 2160}   // 4K
    };
    
    for (const auto& [width, height] : resolutions) {
        auto result = runBenchmark(
            [&processor](const cv::Mat& frame) {
                processor.processFrame(frame);
            },
            width, height
        );
        
        std::cout << "Resolution: " << width << "x" << height << "\n";
        std::cout << "  Average FPS: " << result.avgFPS << "\n";
        std::cout << "  Min FPS: " << result.minFPS << "\n";
        std::cout << "  Max FPS: " << result.maxFPS << "\n";
        std::cout << "  Std Dev: " << result.stdDev << "\n";
        
        // 驗證效能目標
        if (width <= 1920 && height <= 1080) {
            EXPECT_GT(result.avgFPS, 100); // Full HD應達到100+ FPS
        }
    }
}
```

### 2. 延遲測試

```cpp
// tests/performance/test_latency.cpp
class LatencyTest : public ::testing::Test {
protected:
    struct LatencyMetrics {
        double p50;  // 中位數
        double p95;  // 95百分位
        double p99;  // 99百分位
        double max;  // 最大值
    };
    
    LatencyMetrics measureLatency(
        std::function<void()> operation,
        int iterations = 1000
    ) {
        std::vector<double> latencies;
        latencies.reserve(iterations);
        
        for (int i = 0; i < iterations; ++i) {
            auto start = std::chrono::high_resolution_clock::now();
            operation();
            auto end = std::chrono::high_resolution_clock::now();
            
            double latency = std::chrono::duration<double, std::milli>(end - start).count();
            latencies.push_back(latency);
        }
        
        std::sort(latencies.begin(), latencies.end());
        
        LatencyMetrics metrics;
        metrics.p50 = latencies[iterations * 0.5];
        metrics.p95 = latencies[iterations * 0.95];
        metrics.p99 = latencies[iterations * 0.99];
        metrics.max = latencies.back();
        
        return metrics;
    }
};

TEST_F(LatencyTest, EndToEndLatency) {
    SystemPipeline pipeline;
    pipeline.initialize();
    
    auto metrics = measureLatency([&pipeline]() {
        cv::Mat input = TestHelper::createTestImage(1920, 1080);
        pipeline.processFrame(input);
    });
    
    // 驗證延遲要求
    EXPECT_LT(metrics.p50, 50);   // 中位數 < 50ms
    EXPECT_LT(metrics.p95, 100);  // 95% < 100ms
    EXPECT_LT(metrics.p99, 150);  // 99% < 150ms
    
    std::cout << "End-to-End Latency:\n";
    std::cout << "  P50: " << metrics.p50 << " ms\n";
    std::cout << "  P95: " << metrics.p95 << " ms\n";
    std::cout << "  P99: " << metrics.p99 << " ms\n";
    std::cout << "  Max: " << metrics.max << " ms\n";
}
```

### 3. 資源使用測試

```cpp
// tests/performance/test_resource_usage.cpp
class ResourceUsageTest : public ::testing::Test {
protected:
    struct ResourceMetrics {
        double cpuUsage;      // CPU使用率 (%)
        size_t memoryUsage;   // 記憶體使用 (MB)
        double gpuUsage;      // GPU使用率 (%)
        size_t gpuMemory;     // GPU記憶體 (MB)
    };
    
    ResourceMetrics measureResourceUsage(
        std::function<void()> workload,
        int durationSeconds = 10
    ) {
        ResourceMonitor monitor;
        monitor.start();
        
        auto endTime = std::chrono::steady_clock::now() + 
                      std::chrono::seconds(durationSeconds);
        
        while (std::chrono::steady_clock::now() < endTime) {
            workload();
        }
        
        monitor.stop();
        return monitor.getAverageMetrics();
    }
};

TEST_F(ResourceUsageTest, CPUUsageUnderLoad) {
    ImageProcessor processor;
    processor.enableMultithreading(std::thread::hardware_concurrency());
    
    auto metrics = measureResourceUsage([&processor]() {
        cv::Mat frame = TestHelper::createTestImage(1920, 1080);
        processor.processFrame(frame);
    });
    
    // 驗證資源使用限制
    EXPECT_LT(metrics.cpuUsage, 80);     // CPU < 80%
    EXPECT_LT(metrics.memoryUsage, 2048); // RAM < 2GB
    
    std::cout << "Resource Usage:\n";
    std::cout << "  CPU: " << metrics.cpuUsage << "%\n";
    std::cout << "  Memory: " << metrics.memoryUsage << " MB\n";
    std::cout << "  GPU: " << metrics.gpuUsage << "%\n";
    std::cout << "  GPU Memory: " << metrics.gpuMemory << " MB\n";
}
```

---

## 系統測試

### 1. 端到端測試場景

```cpp
// tests/system/test_e2e_scenarios.cpp
class E2EScenarioTest : public ::testing::Test {
protected:
    void SetUp() override {
        system = std::make_unique<AOISystem>();
        system->initialize();
    }
    
    void TearDown() override {
        system->shutdown();
    }
    
    std::unique_ptr<AOISystem> system;
};

// 場景1: 正常生產流程
TEST_F(E2EScenarioTest, NormalProductionFlow) {
    // 載入生產配方
    ASSERT_TRUE(system->loadRecipe("recipes/product_a.json"));
    
    // 開始生產
    ASSERT_TRUE(system->startProduction());
    
    // 模擬產品檢測
    for (int i = 0; i < 100; ++i) {
        ProductData product;
        product.id = "PROD_" + std::to_string(i);
        product.image = TestHelper::createTestImage(1920, 1080);
        
        auto result = system->inspectProduct(product);
        
        // 驗證檢測結果
        EXPECT_TRUE(result.isComplete);
        EXPECT_LT(result.processingTime, 100); // < 100ms
        
        // 驗證結果記錄
        EXPECT_TRUE(system->hasInspectionRecord(product.id));
    }
    
    // 停止生產
    ASSERT_TRUE(system->stopProduction());
    
    // 驗證統計數據
    auto stats = system->getProductionStatistics();
    EXPECT_EQ(stats.totalInspected, 100);
    EXPECT_GT(stats.averageFPS, 100);
}

// 場景2: 異常處理流程
TEST_F(E2EScenarioTest, ErrorRecoveryFlow) {
    ASSERT_TRUE(system->startProduction());
    
    // 模擬相機斷線
    system->simulateError(ErrorType::CAMERA_DISCONNECTED);
    
    // 驗證系統進入錯誤狀態
    EXPECT_EQ(system->getState(), SystemState::ERROR);
    
    // 驗證錯誤通知
    auto errors = system->getActiveErrors();
    EXPECT_EQ(errors.size(), 1);
    EXPECT_EQ(errors[0].type, ErrorType::CAMERA_DISCONNECTED);
    
    // 執行恢復程序
    ASSERT_TRUE(system->recoverFromError());
    
    // 驗證系統恢復正常
    EXPECT_EQ(system->getState(), SystemState::READY);
    EXPECT_TRUE(system->getActiveErrors().empty());
}
```

### 2. 壓力測試

```cpp
// tests/system/test_stress.cpp
class StressTest : public ::testing::Test {
protected:
    void runStressTest(
        int threads,
        int duration,
        std::function<void()> workload
    ) {
        std::atomic<bool> running{true};
        std::atomic<size_t> totalOperations{0};
        std::atomic<size_t> failures{0};
        
        // 啟動工作執行緒
        std::vector<std::thread> workers;
        for (int i = 0; i < threads; ++i) {
            workers.emplace_back([&]() {
                while (running) {
                    try {
                        workload();
                        totalOperations++;
                    } catch (...) {
                        failures++;
                    }
                }
            });
        }
        
        // 運行指定時間
        std::this_thread::sleep_for(std::chrono::seconds(duration));
        running = false;
        
        // 等待所有執行緒完成
        for (auto& worker : workers) {
            worker.join();
        }
        
        // 計算統計
        double opsPerSecond = totalOperations.load() / static_cast<double>(duration);
        double failureRate = failures.load() / static_cast<double>(totalOperations.load());
        
        std::cout << "Stress Test Results:\n";
        std::cout << "  Total Operations: " << totalOperations << "\n";
        std::cout << "  Operations/Second: " << opsPerSecond << "\n";
        std::cout << "  Failure Rate: " << failureRate * 100 << "%\n";
        
        // 驗證穩定性
        EXPECT_LT(failureRate, 0.001); // < 0.1% 失敗率
    }
};

TEST_F(StressTest, HighLoadProcessing) {
    AOISystem system;
    system.initialize();
    
    runStressTest(
        16,  // 執行緒數
        60,  // 持續時間(秒)
        [&system]() {
            cv::Mat frame = TestHelper::createTestImage(1920, 1080);
            system.processFrame(frame);
        }
    );
}
```

### 3. 穩定性測試（長時間運行）

```cpp
// tests/system/test_stability.cpp
class StabilityTest : public ::testing::Test {
protected:
    struct StabilityMetrics {
        size_t totalFrames;
        double avgMemoryUsage;
        double maxMemoryUsage;
        double avgCpuUsage;
        size_t errorCount;
        std::chrono::duration<double> uptime;
    };
    
    StabilityMetrics runLongTermTest(int hours) {
        AOISystem system;
        system.initialize();
        system.startProduction();
        
        StabilityMetrics metrics{};
        ResourceMonitor monitor;
        monitor.start();
        
        auto startTime = std::chrono::steady_clock::now();
        auto endTime = startTime + std::chrono::hours(hours);
        
        while (std::chrono::steady_clock::now() < endTime) {
            try {
                cv::Mat frame = TestHelper::createTestImage(1920, 1080);
                system.processFrame(frame);
                metrics.totalFrames++;
                
                // 每小時記錄一次狀態
                if (metrics.totalFrames % 360000 == 0) {
                    auto resources = monitor.getCurrentMetrics();
                    std::cout << "Hour " << metrics.totalFrames / 360000 << ": "
                             << "Memory=" << resources.memoryUsage << "MB, "
                             << "CPU=" << resources.cpuUsage << "%\n";
                }
            } catch (...) {
                metrics.errorCount++;
            }
        }
        
        metrics.uptime = std::chrono::steady_clock::now() - startTime;
        auto resources = monitor.getAverageMetrics();
        metrics.avgMemoryUsage = resources.memoryUsage;
        metrics.avgCpuUsage = resources.cpuUsage;
        metrics.maxMemoryUsage = monitor.getMaxMemoryUsage();
        
        return metrics;
    }
};

TEST_F(StabilityTest, LongTermOperation) {
    auto metrics = runLongTermTest(24); // 24小時測試
    
    // 驗證穩定性指標
    EXPECT_GT(metrics.totalFrames, 8640000); // > 100 FPS * 24 hours
    EXPECT_LT(metrics.errorCount, 100);      // < 100 errors in 24 hours
    EXPECT_LT(metrics.maxMemoryUsage - metrics.avgMemoryUsage, 500); // 記憶體洩漏檢查
    
    double errorRate = metrics.errorCount / static_cast<double>(metrics.totalFrames);
    EXPECT_LT(errorRate, 0.00001); // < 0.001% 錯誤率
}
```

---

## 自動化測試

### 1. CI/CD管線配置 - Jenkins

```groovy
// Jenkinsfile
pipeline {
    agent any
    
    environment {
        QT_VERSION = '5.15.2'
        CMAKE_VERSION = '3.20'
        OPENCV_VERSION = '4.5.5'
    }
    
    stages {
        stage('Checkout') {
            steps {
                checkout scm
            }
        }
        
        stage('Build') {
            parallel {
                stage('Debug Build') {
                    steps {
                        sh '''
                            mkdir -p build-debug
                            cd build-debug
                            cmake -DCMAKE_BUILD_TYPE=Debug ..
                            make -j$(nproc)
                        '''
                    }
                }
                stage('Release Build') {
                    steps {
                        sh '''
                            mkdir -p build-release
                            cd build-release
                            cmake -DCMAKE_BUILD_TYPE=Release ..
                            make -j$(nproc)
                        '''
                    }
                }
            }
        }
        
        stage('Unit Tests') {
            steps {
                sh '''
                    cd build-debug
                    ctest --output-on-failure -L unit
                '''
                junit 'build-debug/test-results/*.xml'
            }
        }
        
        stage('Integration Tests') {
            steps {
                sh '''
                    cd build-debug
                    ctest --output-on-failure -L integration
                '''
            }
        }
        
        stage('Performance Tests') {
            steps {
                sh '''
                    cd build-release
                    ctest --output-on-failure -L performance
                '''
                publishHTML([
                    reportDir: 'build-release/performance-reports',
                    reportFiles: 'index.html',
                    reportName: 'Performance Report'
                ])
            }
        }
        
        stage('Code Coverage') {
            steps {
                sh '''
                    cd build-debug
                    make coverage
                    lcov --capture --directory . --output-file coverage.info
                    lcov --remove coverage.info '/usr/*' --output-file coverage.info
                    lcov --remove coverage.info '*/test/*' --output-file coverage.info
                    genhtml coverage.info --output-directory coverage-report
                '''
                publishHTML([
                    reportDir: 'build-debug/coverage-report',
                    reportFiles: 'index.html',
                    reportName: 'Coverage Report'
                ])
            }
        }
        
        stage('Static Analysis') {
            parallel {
                stage('Clang-Tidy') {
                    steps {
                        sh 'run-clang-tidy.py -p build-debug'
                    }
                }
                stage('CPPCheck') {
                    steps {
                        sh 'cppcheck --enable=all --xml --xml-version=2 src 2> cppcheck.xml'
                        recordIssues(
                            enabledForFailure: true,
                            tool: cppCheck(pattern: 'cppcheck.xml')
                        )
                    }
                }
            }
        }
        
        stage('Memory Check') {
            steps {
                sh '''
                    cd build-debug
                    valgrind --leak-check=full --xml=yes --xml-file=valgrind.xml ./tests/unit_tests
                '''
                publishValgrind(
                    pattern: 'build-debug/valgrind.xml',
                    failBuildOnMissingReports: false
                )
            }
        }
        
        stage('Deploy Test Environment') {
            when {
                branch 'develop'
            }
            steps {
                sh '''
                    docker build -t aoi-system:test .
                    docker-compose -f docker-compose.test.yml up -d
                '''
            }
        }
    }
    
    post {
        always {
            cleanWs()
        }
        success {
            emailext(
                subject: "Build Success: ${env.JOB_NAME} - ${env.BUILD_NUMBER}",
                body: "The build was successful. View report: ${env.BUILD_URL}",
                to: 'team@example.com'
            )
        }
        failure {
            emailext(
                subject: "Build Failed: ${env.JOB_NAME} - ${env.BUILD_NUMBER}",
                body: "The build failed. Check logs: ${env.BUILD_URL}console",
                to: 'team@example.com'
            )
        }
    }
}
```

### 2. GitLab CI配置

```yaml
# .gitlab-ci.yml
stages:
  - build
  - test
  - analyze
  - performance
  - deploy

variables:
  CMAKE_BUILD_TYPE: "Debug"
  GIT_SUBMODULE_STRATEGY: recursive

before_script:
  - apt-get update -qq
  - apt-get install -y -qq cmake g++ libopencv-dev qtbase5-dev

# Build Jobs
build:debug:
  stage: build
  script:
    - mkdir -p build-debug
    - cd build-debug
    - cmake -DCMAKE_BUILD_TYPE=Debug ..
    - make -j$(nproc)
  artifacts:
    paths:
      - build-debug/
    expire_in: 1 hour

build:release:
  stage: build
  script:
    - mkdir -p build-release
    - cd build-release
    - cmake -DCMAKE_BUILD_TYPE=Release ..
    - make -j$(nproc)
  artifacts:
    paths:
      - build-release/
    expire_in: 1 hour

# Test Jobs
test:unit:
  stage: test
  dependencies:
    - build:debug
  script:
    - cd build-debug
    - ctest --output-on-failure -L unit
    - ./tests/generate_junit_report.sh
  artifacts:
    reports:
      junit: build-debug/test-results/*.xml
    paths:
      - build-debug/test-results/
    expire_in: 1 week

test:integration:
  stage: test
  dependencies:
    - build:debug
  script:
    - cd build-debug
    - ctest --output-on-failure -L integration
  artifacts:
    reports:
      junit: build-debug/test-results/*.xml

test:coverage:
  stage: test
  dependencies:
    - build:debug
  script:
    - cd build-debug
    - make coverage
    - lcov --capture --directory . --output-file coverage.info
    - lcov --remove coverage.info '/usr/*' '*/test/*' --output-file coverage.info
  coverage: '/\s*lines\.+:\s*(\d+\.\d+)%/'
  artifacts:
    reports:
      cobertura: build-debug/coverage.xml
    paths:
      - build-debug/coverage-report/

# Analysis Jobs
analyze:static:
  stage: analyze
  dependencies:
    - build:debug
  script:
    - run-clang-tidy.py -p build-debug
    - cppcheck --enable=all --xml --xml-version=2 src 2> cppcheck.xml
  artifacts:
    reports:
      codequality: cppcheck.xml

analyze:memory:
  stage: analyze
  dependencies:
    - build:debug
  script:
    - cd build-debug
    - valgrind --leak-check=full --xml=yes --xml-file=valgrind.xml ./tests/unit_tests
  artifacts:
    paths:
      - build-debug/valgrind.xml

# Performance Jobs
performance:benchmark:
  stage: performance
  dependencies:
    - build:release
  script:
    - cd build-release
    - ./tests/performance_benchmark --output=json > performance.json
    - python3 ../scripts/analyze_performance.py performance.json
  artifacts:
    reports:
      performance: build-release/performance.json
    paths:
      - build-release/performance-report/

performance:load:
  stage: performance
  dependencies:
    - build:release
  script:
    - cd build-release
    - ./tests/load_test --duration=3600 --threads=16
  artifacts:
    paths:
      - build-release/load-test-results/

# Deploy Jobs
deploy:test:
  stage: deploy
  dependencies:
    - build:release
  script:
    - docker build -t $CI_REGISTRY_IMAGE:$CI_COMMIT_REF_NAME .
    - docker push $CI_REGISTRY_IMAGE:$CI_COMMIT_REF_NAME
  only:
    - develop

deploy:production:
  stage: deploy
  dependencies:
    - build:release
  script:
    - docker build -t $CI_REGISTRY_IMAGE:latest .
    - docker push $CI_REGISTRY_IMAGE:latest
  only:
    - master
  when: manual
```

### 3. 自動化測試腳本

```python
#!/usr/bin/env python3
# scripts/run_tests.py

import subprocess
import json
import sys
import argparse
from pathlib import Path
from datetime import datetime
import xml.etree.ElementTree as ET

class TestRunner:
    def __init__(self, build_dir):
        self.build_dir = Path(build_dir)
        self.results = {
            'timestamp': datetime.now().isoformat(),
            'tests': {},
            'summary': {
                'total': 0,
                'passed': 0,
                'failed': 0,
                'skipped': 0
            }
        }
    
    def run_unit_tests(self):
        """執行單元測試"""
        print("Running unit tests...")
        result = subprocess.run(
            ['ctest', '--output-on-failure', '-L', 'unit', '--output-junit', 'unit_tests.xml'],
            cwd=self.build_dir,
            capture_output=True,
            text=True
        )
        
        self.parse_junit_results('unit_tests.xml', 'unit')
        return result.returncode == 0
    
    def run_integration_tests(self):
        """執行整合測試"""
        print("Running integration tests...")
        result = subprocess.run(
            ['ctest', '--output-on-failure', '-L', 'integration', '--output-junit', 'integration_tests.xml'],
            cwd=self.build_dir,
            capture_output=True,
            text=True
        )
        
        self.parse_junit_results('integration_tests.xml', 'integration')
        return result.returncode == 0
    
    def run_performance_tests(self):
        """執行效能測試"""
        print("Running performance tests...")
        result = subprocess.run(
            ['./tests/performance_benchmark', '--json'],
            cwd=self.build_dir,
            capture_output=True,
            text=True
        )
        
        if result.returncode == 0:
            perf_data = json.loads(result.stdout)
            self.results['performance'] = perf_data
            self.check_performance_requirements(perf_data)
        
        return result.returncode == 0
    
    def check_performance_requirements(self, perf_data):
        """檢查效能需求"""
        requirements = {
            'fps_1080p': 100,
            'latency_p95': 100,
            'memory_usage': 2048
        }
        
        violations = []
        
        if perf_data.get('fps_1080p', 0) < requirements['fps_1080p']:
            violations.append(f"FPS below requirement: {perf_data['fps_1080p']} < {requirements['fps_1080p']}")
        
        if perf_data.get('latency_p95', float('inf')) > requirements['latency_p95']:
            violations.append(f"Latency exceeds limit: {perf_data['latency_p95']} > {requirements['latency_p95']}")
        
        if perf_data.get('memory_usage', float('inf')) > requirements['memory_usage']:
            violations.append(f"Memory usage too high: {perf_data['memory_usage']} > {requirements['memory_usage']}")
        
        if violations:
            print("Performance requirement violations:")
            for v in violations:
                print(f"  - {v}")
            return False
        
        return True
    
    def run_coverage_analysis(self):
        """執行覆蓋率分析"""
        print("Generating coverage report...")
        subprocess.run(['make', 'coverage'], cwd=self.build_dir)
        
        # 解析覆蓋率報告
        coverage_file = self.build_dir / 'coverage.json'
        if coverage_file.exists():
            with open(coverage_file) as f:
                coverage_data = json.load(f)
                self.results['coverage'] = coverage_data
                
                line_coverage = coverage_data.get('line_coverage', 0)
                if line_coverage < 80:
                    print(f"Warning: Line coverage below 80%: {line_coverage}%")
                    return False
        
        return True
    
    def parse_junit_results(self, xml_file, test_type):
        """解析JUnit測試結果"""
        xml_path = self.build_dir / xml_file
        if not xml_path.exists():
            return
        
        tree = ET.parse(xml_path)
        root = tree.getroot()
        
        for testsuite in root.findall('testsuite'):
            suite_name = testsuite.get('name')
            
            for testcase in testsuite.findall('testcase'):
                test_name = f"{suite_name}.{testcase.get('name')}"
                status = 'passed'
                
                if testcase.find('failure') is not None:
                    status = 'failed'
                elif testcase.find('skipped') is not None:
                    status = 'skipped'
                
                self.results['tests'][test_name] = {
                    'type': test_type,
                    'status': status,
                    'time': float(testcase.get('time', 0))
                }
                
                self.results['summary']['total'] += 1
                self.results['summary'][status] += 1
    
    def generate_report(self):
        """生成測試報告"""
        report_file = self.build_dir / 'test_report.json'
        with open(report_file, 'w') as f:
            json.dump(self.results, f, indent=2)
        
        # 生成HTML報告
        self.generate_html_report()
        
        # 打印摘要
        print("\n" + "="*50)
        print("Test Summary:")
        print(f"  Total: {self.results['summary']['total']}")
        print(f"  Passed: {self.results['summary']['passed']}")
        print(f"  Failed: {self.results['summary']['failed']}")
        print(f"  Skipped: {self.results['summary']['skipped']}")
        
        if 'coverage' in self.results:
            print(f"  Coverage: {self.results['coverage'].get('line_coverage', 0)}%")
        
        if 'performance' in self.results:
            print(f"  FPS (1080p): {self.results['performance'].get('fps_1080p', 0)}")
            print(f"  Latency P95: {self.results['performance'].get('latency_p95', 0)}ms")
        
        print("="*50)
    
    def generate_html_report(self):
        """生成HTML測試報告"""
        html_template = """
        <!DOCTYPE html>
        <html>
        <head>
            <title>Test Report</title>
            <style>
                body {{ font-family: Arial, sans-serif; margin: 20px; }}
                .summary {{ background: #f0f0f0; padding: 15px; border-radius: 5px; }}
                .passed {{ color: green; }}
                .failed {{ color: red; }}
                .skipped {{ color: orange; }}
                table {{ width: 100%; border-collapse: collapse; margin-top: 20px; }}
                th, td {{ border: 1px solid #ddd; padding: 8px; text-align: left; }}
                th {{ background: #4CAF50; color: white; }}
                tr:nth-child(even) {{ background: #f2f2f2; }}
            </style>
        </head>
        <body>
            <h1>Test Report - {timestamp}</h1>
            <div class="summary">
                <h2>Summary</h2>
                <p>Total: {total}</p>
                <p class="passed">Passed: {passed}</p>
                <p class="failed">Failed: {failed}</p>
                <p class="skipped">Skipped: {skipped}</p>
            </div>
            <h2>Test Results</h2>
            <table>
                <tr><th>Test Name</th><th>Type</th><th>Status</th><th>Time (s)</th></tr>
                {test_rows}
            </table>
        </body>
        </html>
        """
        
        test_rows = ""
        for name, data in self.results['tests'].items():
            status_class = data['status']
            test_rows += f"""
                <tr>
                    <td>{name}</td>
                    <td>{data['type']}</td>
                    <td class="{status_class}">{data['status']}</td>
                    <td>{data['time']:.3f}</td>
                </tr>
            """
        
        html_content = html_template.format(
            timestamp=self.results['timestamp'],
            total=self.results['summary']['total'],
            passed=self.results['summary']['passed'],
            failed=self.results['summary']['failed'],
            skipped=self.results['summary']['skipped'],
            test_rows=test_rows
        )
        
        report_file = self.build_dir / 'test_report.html'
        with open(report_file, 'w') as f:
            f.write(html_content)
        
        print(f"HTML report generated: {report_file}")

def main():
    parser = argparse.ArgumentParser(description='Run automated tests')
    parser.add_argument('build_dir', help='Build directory path')
    parser.add_argument('--unit', action='store_true', help='Run unit tests')
    parser.add_argument('--integration', action='store_true', help='Run integration tests')
    parser.add_argument('--performance', action='store_true', help='Run performance tests')
    parser.add_argument('--coverage', action='store_true', help='Generate coverage report')
    parser.add_argument('--all', action='store_true', help='Run all tests')
    
    args = parser.parse_args()
    
    runner = TestRunner(args.build_dir)
    success = True
    
    if args.all or args.unit:
        success &= runner.run_unit_tests()
    
    if args.all or args.integration:
        success &= runner.run_integration_tests()
    
    if args.all or args.performance:
        success &= runner.run_performance_tests()
    
    if args.all or args.coverage:
        success &= runner.run_coverage_analysis()
    
    runner.generate_report()
    
    sys.exit(0 if success else 1)

if __name__ == '__main__':
    main()
```

---

## 效能基準

### 1. 影像處理基準測試套件

```cpp
// benchmarks/image_processing_benchmark.cpp
#include <benchmark/benchmark.h>
#include "components/image_processor.h"

// 不同解析度的基準測試
static void BM_ImageProcessing_VGA(benchmark::State& state) {
    ImageProcessor processor;
    cv::Mat image(480, 640, CV_8UC3);
    cv::randu(image, cv::Scalar::all(0), cv::Scalar::all(255));
    
    for (auto _ : state) {
        processor.processFrame(image);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * image.total() * image.elemSize());
}
BENCHMARK(BM_ImageProcessing_VGA);

static void BM_ImageProcessing_HD(benchmark::State& state) {
    ImageProcessor processor;
    cv::Mat image(720, 1280, CV_8UC3);
    cv::randu(image, cv::Scalar::all(0), cv::Scalar::all(255));
    
    for (auto _ : state) {
        processor.processFrame(image);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * image.total() * image.elemSize());
}
BENCHMARK(BM_ImageProcessing_HD);

static void BM_ImageProcessing_FullHD(benchmark::State& state) {
    ImageProcessor processor;
    cv::Mat image(1080, 1920, CV_8UC3);
    cv::randu(image, cv::Scalar::all(0), cv::Scalar::all(255));
    
    for (auto _ : state) {
        processor.processFrame(image);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * image.total() * image.elemSize());
}
BENCHMARK(BM_ImageProcessing_FullHD);

static void BM_ImageProcessing_4K(benchmark::State& state) {
    ImageProcessor processor;
    cv::Mat image(2160, 3840, CV_8UC3);
    cv::randu(image, cv::Scalar::all(0), cv::Scalar::all(255));
    
    for (auto _ : state) {
        processor.processFrame(image);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * image.total() * image.elemSize());
}
BENCHMARK(BM_ImageProcessing_4K);

// 多執行緒效能測試
static void BM_ImageProcessing_Threaded(benchmark::State& state) {
    ImageProcessor processor;
    processor.enableMultithreading(state.range(0));
    
    cv::Mat image(1080, 1920, CV_8UC3);
    cv::randu(image, cv::Scalar::all(0), cv::Scalar::all(255));
    
    for (auto _ : state) {
        processor.processFrame(image);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ImageProcessing_Threaded)->Range(1, 16);

BENCHMARK_MAIN();
```

### 2. 演算法執行時間基準

```cpp
// benchmarks/algorithm_benchmark.cpp
#include <benchmark/benchmark.h>
#include "algorithms/defect_detection.h"
#include "algorithms/pattern_matching.h"
#include "algorithms/edge_detection.h"

// 缺陷檢測演算法基準
static void BM_DefectDetection_Simple(benchmark::State& state) {
    DefectDetector detector(DetectionMode::SIMPLE);
    cv::Mat image = cv::imread("benchmark_data/defect_sample.png");
    
    for (auto _ : state) {
        auto defects = detector.detect(image);
        benchmark::DoNotOptimize(defects);
    }
}
BENCHMARK(BM_DefectDetection_Simple);

static void BM_DefectDetection_Advanced(benchmark::State& state) {
    DefectDetector detector(DetectionMode::ADVANCED);
    cv::Mat image = cv::imread("benchmark_data/defect_sample.png");
    
    for (auto _ : state) {
        auto defects = detector.detect(image);
        benchmark::DoNotOptimize(defects);
    }
}
BENCHMARK(BM_DefectDetection_Advanced);

// 模式匹配演算法基準
static void BM_PatternMatching_TemplateMatching(benchmark::State& state) {
    PatternMatcher matcher;
    cv::Mat image = cv::imread("benchmark_data/pattern_sample.png");
    cv::Mat template_img = cv::imread("benchmark_data/template.png");
    
    for (auto _ : state) {
        auto result = matcher.matchTemplate(image, template_img);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_PatternMatching_TemplateMatching);

// 邊緣檢測演算法基準
static void BM_EdgeDetection_Canny(benchmark::State& state) {
    EdgeDetector detector;
    cv::Mat image = cv::imread("benchmark_data/edge_sample.png", cv::IMREAD_GRAYSCALE);
    
    for (auto _ : state) {
        auto edges = detector.detectCanny(image, 50, 150);
        benchmark::DoNotOptimize(edges);
    }
}
BENCHMARK(BM_EdgeDetection_Canny);

static void BM_EdgeDetection_Sobel(benchmark::State& state) {
    EdgeDetector detector;
    cv::Mat image = cv::imread("benchmark_data/edge_sample.png", cv::IMREAD_GRAYSCALE);
    
    for (auto _ : state) {
        auto edges = detector.detectSobel(image);
        benchmark::DoNotOptimize(edges);
    }
}
BENCHMARK(BM_EdgeDetection_Sobel);
```

### 3. 系統啟動時間基準

```cpp
// benchmarks/startup_benchmark.cpp
class StartupBenchmark {
public:
    struct StartupMetrics {
        double totalTime;
        double initializationTime;
        double configLoadTime;
        double cameraConnectionTime;
        double calibrationTime;
        double uiLoadTime;
    };
    
    static StartupMetrics measureStartupTime() {
        StartupMetrics metrics{};
        
        auto totalStart = std::chrono::high_resolution_clock::now();
        
        // 測量初始化時間
        auto initStart = std::chrono::high_resolution_clock::now();
        AOISystem system;
        auto initEnd = std::chrono::high_resolution_clock::now();
        metrics.initializationTime = std::chrono::duration<double, std::milli>(initEnd - initStart).count();
        
        // 測量配置載入時間
        auto configStart = std::chrono::high_resolution_clock::now();
        system.loadConfiguration("config/system.json");
        auto configEnd = std::chrono::high_resolution_clock::now();
        metrics.configLoadTime = std::chrono::duration<double, std::milli>(configEnd - configStart).count();
        
        // 測量相機連接時間
        auto cameraStart = std::chrono::high_resolution_clock::now();
        system.connectCameras();
        auto cameraEnd = std::chrono::high_resolution_clock::now();
        metrics.cameraConnectionTime = std::chrono::duration<double, std::milli>(cameraEnd - cameraStart).count();
        
        // 測量校準時間
        auto calibStart = std::chrono::high_resolution_clock::now();
        system.performCalibration();
        auto calibEnd = std::chrono::high_resolution_clock::now();
        metrics.calibrationTime = std::chrono::duration<double, std::milli>(calibEnd - calibStart).count();
        
        // 測量UI載入時間
        auto uiStart = std::chrono::high_resolution_clock::now();
        system.loadUserInterface();
        auto uiEnd = std::chrono::high_resolution_clock::now();
        metrics.uiLoadTime = std::chrono::duration<double, std::milli>(uiEnd - uiStart).count();
        
        auto totalEnd = std::chrono::high_resolution_clock::now();
        metrics.totalTime = std::chrono::duration<double, std::milli>(totalEnd - totalStart).count();
        
        return metrics;
    }
    
    static void printMetrics(const StartupMetrics& metrics) {
        std::cout << "Startup Time Breakdown:\n";
        std::cout << "  Total: " << metrics.totalTime << " ms\n";
        std::cout << "  Initialization: " << metrics.initializationTime << " ms\n";
        std::cout << "  Config Load: " << metrics.configLoadTime << " ms\n";
        std::cout << "  Camera Connection: " << metrics.cameraConnectionTime << " ms\n";
        std::cout << "  Calibration: " << metrics.calibrationTime << " ms\n";
        std::cout << "  UI Load: " << metrics.uiLoadTime << " ms\n";
    }
};

TEST(StartupBenchmark, MeasureStartupTime) {
    auto metrics = StartupBenchmark::measureStartupTime();
    StartupBenchmark::printMetrics(metrics);
    
    // 驗證啟動時間需求
    EXPECT_LT(metrics.totalTime, 5000); // < 5秒總啟動時間
    EXPECT_LT(metrics.cameraConnectionTime, 2000); // < 2秒相機連接
}
```

---

## 測試最佳實踐

### 1. 測試設計原則

```markdown
## 測試設計檢查清單

### 單元測試
- [ ] 每個公開方法都有對應的測試
- [ ] 測試覆蓋正常路徑和異常路徑
- [ ] 使用Mock物件隔離依賴
- [ ] 測試名稱清楚描述測試內容
- [ ] 遵循AAA模式（Arrange-Act-Assert）
- [ ] 每個測試只驗證一個行為
- [ ] 測試獨立且可重複執行

### 整合測試
- [ ] 測試元件間的介面
- [ ] 驗證資料流正確性
- [ ] 測試並發場景
- [ ] 檢查資源管理（記憶體、檔案句柄等）
- [ ] 測試錯誤傳播

### 效能測試
- [ ] 定義明確的效能指標
- [ ] 使用真實數據進行測試
- [ ] 測試不同負載條件
- [ ] 監控資源使用
- [ ] 建立效能基線
- [ ] 追蹤效能趨勢

### 系統測試
- [ ] 覆蓋關鍵使用案例
- [ ] 測試端到端流程
- [ ] 驗證系統需求
- [ ] 測試故障恢復
- [ ] 長時間穩定性測試
```

### 2. 測試資料管理

```cpp
// test_data/test_data_factory.h
class TestDataFactory {
public:
    // 影像測試資料生成
    static cv::Mat createTestImage(const ImageSpec& spec) {
        cv::Mat image(spec.height, spec.width, CV_8UC3);
        
        switch (spec.pattern) {
            case Pattern::RANDOM:
                cv::randu(image, cv::Scalar::all(0), cv::Scalar::all(255));
                break;
            case Pattern::GRADIENT:
                createGradient(image);
                break;
            case Pattern::CHECKERBOARD:
                createCheckerboard(image, spec.squareSize);
                break;
            case Pattern::WITH_DEFECTS:
                createImageWithDefects(image, spec.defects);
                break;
        }
        
        if (spec.noise > 0) {
            addNoise(image, spec.noise);
        }
        
        return image;
    }
    
    // 配置檔案生成
    static json createTestConfig(const ConfigSpec& spec) {
        json config;
        config["version"] = "1.0";
        config["system"] = {
            {"fps_target", spec.fpsTarget},
            {"resolution", {spec.width, spec.height}},
            {"threads", spec.threadCount}
        };
        return config;
    }
    
    // 測試場景生成
    static TestScenario createScenario(ScenarioType type) {
        TestScenario scenario;
        
        switch (type) {
            case ScenarioType::NORMAL_PRODUCTION:
                scenario.duration = 3600; // 1小時
                scenario.productRate = 100; // 100個/小時
                scenario.defectRate = 0.02; // 2%缺陷率
                break;
            case ScenarioType::HIGH_LOAD:
                scenario.duration = 1800; // 30分鐘
                scenario.productRate = 500; // 500個/小時
                scenario.defectRate = 0.05; // 5%缺陷率
                break;
            case ScenarioType::ERROR_RECOVERY:
                scenario.duration = 600; // 10分鐘
                scenario.productRate = 100;
                scenario.defectRate = 0.02;
                scenario.errors = {
                    {300, ErrorType::CAMERA_DISCONNECTED},
                    {450, ErrorType::NETWORK_ERROR}
                };
                break;
        }
        
        return scenario;
    }
    
private:
    static void createGradient(cv::Mat& image);
    static void createCheckerboard(cv::Mat& image, int squareSize);
    static void createImageWithDefects(cv::Mat& image, const std::vector<Defect>& defects);
    static void addNoise(cv::Mat& image, double noiseLevel);
};
```

### 3. 測試報告模板

```html
<!-- test_report_template.html -->
<!DOCTYPE html>
<html>
<head>
    <title>AOI System Test Report</title>
    <style>
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            margin: 0;
            padding: 20px;
            background: #f5f5f5;
        }
        .header {
            background: #2c3e50;
            color: white;
            padding: 20px;
            border-radius: 5px;
            margin-bottom: 20px;
        }
        .metrics {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
            gap: 15px;
            margin-bottom: 30px;
        }
        .metric-card {
            background: white;
            padding: 15px;
            border-radius: 5px;
            box-shadow: 0 2px 4px rgba(0,0,0,0.1);
        }
        .metric-value {
            font-size: 24px;
            font-weight: bold;
            color: #2c3e50;
        }
        .metric-label {
            color: #7f8c8d;
            font-size: 12px;
            text-transform: uppercase;
        }
        .chart-container {
            background: white;
            padding: 20px;
            border-radius: 5px;
            box-shadow: 0 2px 4px rgba(0,0,0,0.1);
            margin-bottom: 20px;
        }
        .passed { color: #27ae60; }
        .failed { color: #e74c3c; }
        .warning { color: #f39c12; }
        table {
            width: 100%;
            border-collapse: collapse;
            background: white;
        }
        th {
            background: #34495e;
            color: white;
            padding: 10px;
            text-align: left;
        }
        td {
            padding: 8px;
            border-bottom: 1px solid #ecf0f1;
        }
        tr:hover {
            background: #f8f9fa;
        }
    </style>
    <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
</head>
<body>
    <div class="header">
        <h1>AOI System Test Report</h1>
        <p>Generated: {{timestamp}}</p>
        <p>Build: {{build_number}} | Branch: {{branch}}</p>
    </div>
    
    <div class="metrics">
        <div class="metric-card">
            <div class="metric-value passed">{{passed_tests}}</div>
            <div class="metric-label">Tests Passed</div>
        </div>
        <div class="metric-card">
            <div class="metric-value failed">{{failed_tests}}</div>
            <div class="metric-label">Tests Failed</div>
        </div>
        <div class="metric-card">
            <div class="metric-value">{{coverage}}%</div>
            <div class="metric-label">Code Coverage</div>
        </div>
        <div class="metric-card">
            <div class="metric-value">{{fps_average}} FPS</div>
            <div class="metric-label">Average Performance</div>
        </div>
    </div>
    
    <div class="chart-container">
        <h2>Test Execution Trend</h2>
        <canvas id="trendChart"></canvas>
    </div>
    
    <div class="chart-container">
        <h2>Performance Metrics</h2>
        <canvas id="perfChart"></canvas>
    </div>
    
    <div class="chart-container">
        <h2>Test Results Detail</h2>
        <table>
            <thead>
                <tr>
                    <th>Test Suite</th>
                    <th>Test Case</th>
                    <th>Status</th>
                    <th>Duration</th>
                    <th>Message</th>
                </tr>
            </thead>
            <tbody>
                {{test_results}}
            </tbody>
        </table>
    </div>
    
    <script>
        // 趨勢圖表
        const trendCtx = document.getElementById('trendChart').getContext('2d');
        new Chart(trendCtx, {
            type: 'line',
            data: {{trend_data}},
            options: {
                responsive: true,
                plugins: {
                    legend: {
                        position: 'top',
                    }
                }
            }
        });
        
        // 效能圖表
        const perfCtx = document.getElementById('perfChart').getContext('2d');
        new Chart(perfCtx, {
            type: 'bar',
            data: {{performance_data}},
            options: {
                responsive: true,
                plugins: {
                    legend: {
                        display: false
                    }
                }
            }
        });
    </script>
</body>
</html>
```

### 4. 持續改進流程

```python
#!/usr/bin/env python3
# scripts/test_analysis.py

import json
import sqlite3
from datetime import datetime, timedelta
import matplotlib.pyplot as plt
import pandas as pd

class TestAnalyzer:
    def __init__(self, db_path):
        self.conn = sqlite3.connect(db_path)
        self.init_database()
    
    def init_database(self):
        """初始化測試結果資料庫"""
        self.conn.execute('''
            CREATE TABLE IF NOT EXISTS test_results (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                timestamp DATETIME,
                build_number TEXT,
                test_name TEXT,
                status TEXT,
                duration REAL,
                coverage REAL,
                fps REAL,
                memory_usage REAL
            )
        ''')
    
    def analyze_trends(self, days=30):
        """分析測試趨勢"""
        query = '''
            SELECT DATE(timestamp) as date,
                   AVG(CASE WHEN status = 'passed' THEN 1 ELSE 0 END) * 100 as pass_rate,
                   AVG(duration) as avg_duration,
                   AVG(coverage) as avg_coverage,
                   AVG(fps) as avg_fps
            FROM test_results
            WHERE timestamp > datetime('now', '-{} days')
            GROUP BY DATE(timestamp)
            ORDER BY date
        '''.format(days)
        
        df = pd.read_sql_query(query, self.conn)
        
        # 生成趨勢圖
        fig, axes = plt.subplots(2, 2, figsize=(12, 8))
        
        # 通過率趨勢
        axes[0, 0].plot(df['date'], df['pass_rate'])
        axes[0, 0].set_title('Test Pass Rate Trend')
        axes[0, 0].set_ylabel('Pass Rate (%)')
        
        # 執行時間趨勢
        axes[0, 1].plot(df['date'], df['avg_duration'])
        axes[0, 1].set_title('Average Test Duration')
        axes[0, 1].set_ylabel('Duration (s)')
        
        # 覆蓋率趨勢
        axes[1, 0].plot(df['date'], df['avg_coverage'])
        axes[1, 0].set_title('Code Coverage Trend')
        axes[1, 0].set_ylabel('Coverage (%)')
        
        # 效能趨勢
        axes[1, 1].plot(df['date'], df['avg_fps'])
        axes[1, 1].set_title('Performance Trend')
        axes[1, 1].set_ylabel('FPS')
        
        plt.tight_layout()
        plt.savefig('test_trends.png')
        
        return df
    
    def identify_flaky_tests(self, threshold=0.1):
        """識別不穩定的測試"""
        query = '''
            SELECT test_name,
                   COUNT(*) as total_runs,
                   SUM(CASE WHEN status = 'failed' THEN 1 ELSE 0 END) as failures,
                   AVG(duration) as avg_duration,
                   STDEV(duration) as duration_stddev
            FROM test_results
            WHERE timestamp > datetime('now', '-7 days')
            GROUP BY test_name
            HAVING failures > 0 AND failures < total_runs
        '''
        
        df = pd.read_sql_query(query, self.conn)
        df['failure_rate'] = df['failures'] / df['total_runs']
        
        # 識別不穩定測試
        flaky_tests = df[
            (df['failure_rate'] > threshold) & 
            (df['failure_rate'] < 1 - threshold)
        ]
        
        return flaky_tests
    
    def generate_recommendations(self):
        """生成改進建議"""
        recommendations = []
        
        # 分析覆蓋率
        avg_coverage = self.get_average_metric('coverage', 7)
        if avg_coverage < 80:
            recommendations.append({
                'type': 'coverage',
                'severity': 'high',
                'message': f'Code coverage is {avg_coverage:.1f}%, below target of 80%',
                'action': 'Add more unit tests for uncovered code paths'
            })
        
        # 分析效能
        avg_fps = self.get_average_metric('fps', 7)
        if avg_fps < 100:
            recommendations.append({
                'type': 'performance',
                'severity': 'high',
                'message': f'Average FPS is {avg_fps:.1f}, below target of 100',
                'action': 'Profile and optimize performance bottlenecks'
            })
        
        # 分析不穩定測試
        flaky_tests = self.identify_flaky_tests()
        if len(flaky_tests) > 0:
            recommendations.append({
                'type': 'stability',
                'severity': 'medium',
                'message': f'Found {len(flaky_tests)} flaky tests',
                'action': 'Review and fix non-deterministic test behavior',
                'details': flaky_tests['test_name'].tolist()
            })
        
        return recommendations
    
    def get_average_metric(self, metric, days):
        """獲取指定時間範圍的平均指標"""
        query = f'''
            SELECT AVG({metric}) as avg_value
            FROM test_results
            WHERE timestamp > datetime('now', '-{days} days')
        '''
        result = self.conn.execute(query).fetchone()
        return result[0] if result[0] else 0

def main():
    analyzer = TestAnalyzer('test_results.db')
    
    # 分析趨勢
    trends = analyzer.analyze_trends(30)
    print("Test Trends Analysis:")
    print(trends.tail())
    
    # 識別不穩定測試
    flaky_tests = analyzer.identify_flaky_tests()
    if not flaky_tests.empty:
        print("\nFlaky Tests Detected:")
        print(flaky_tests)
    
    # 生成建議
    recommendations = analyzer.generate_recommendations()
    print("\nRecommendations:")
    for rec in recommendations:
        print(f"[{rec['severity'].upper()}] {rec['type']}: {rec['message']}")
        print(f"  Action: {rec['action']}")
        if 'details' in rec:
            print(f"  Details: {rec['details']}")

if __name__ == '__main__':
    main()
```

---

## 總結

本測試策略文檔為工業AOI設備元件生態系統提供了完整的品質保證框架，包括：

1. **全面的測試覆蓋**：從單元測試到系統測試的完整測試金字塔
2. **自動化測試流程**：CI/CD整合確保持續品質
3. **效能基準測試**：明確的效能目標和驗證方法
4. **測試最佳實踐**：可重複使用的測試模式和工具

### 關鍵成果指標
- 單元測試覆蓋率 >80%
- 系統FPS效能 >100 (Full HD)
- 回應延遲 <100ms (P95)
- 24/7穩定運行能力
- 自動化測試執行率 >95%

### 持續改進
- 定期檢視測試結果趨勢
- 優化不穩定的測試案例
- 更新效能基準
- 擴展測試場景覆蓋
- 強化自動化程度