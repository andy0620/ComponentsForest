# Do3ThinkCameraComponent 整合測試與部署手冊

## 1. 單元測試

### 1.1 Component測試

```cpp
// Do3ThinkCameraComponentTest.h
#ifndef DO3THINKCAMERACOMPONENTTEST_H
#define DO3THINKCAMERACOMPONENTTEST_H

#include <QObject>
#include <QTest>
#include <QSignalSpy>
#include <memory>
#include "Do3ThinkCameraComponent.h"
#include "MockDo3ThinkCamera.h"

class Do3ThinkCameraComponentTest : public QObject {
    Q_OBJECT
    
private slots:
    // 測試生命週期
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();
    
    // 功能測試
    void testInitialization();
    void testConnection();
    void testDisconnection();
    void testParameterSetting();
    void testAcquisition();
    void testSignalEmission();
    void testErrorHandling();
    void testThreadSafety();
    void testMemoryManagement();
    
    // 邊界測試
    void testInvalidParameters();
    void testResourceLimits();
    void testConcurrentAccess();
    
    // 性能測試
    void benchmarkInitialization();
    void benchmarkAcquisition();
    void benchmarkProcessing();
    
private:
    std::unique_ptr<Do3ThinkCameraComponent> m_component;
    std::unique_ptr<MockDo3ThinkCamera> m_mockCamera;
};

// 測試實現示例
void Do3ThinkCameraComponentTest::testInitialization() {
    // Arrange
    Do3ThinkCameraComponent component;
    
    // Act
    bool result = component.initialize();
    
    // Assert
    QVERIFY(result);
    QCOMPARE(component.getState(), ComponentState::Initialized);
    QVERIFY(component.getAvailableDevices().count() >= 0);
}

void Do3ThinkCameraComponentTest::testConnection() {
    // Arrange
    QSignalSpy connectedSpy(m_component.get(), &Do3ThinkCameraComponent::connected);
    QSignalSpy errorSpy(m_component.get(), &Do3ThinkCameraComponent::errorOccurred);
    
    // Act
    bool result = m_component->connectDevice(0);
    
    // Assert
    QVERIFY(result);
    QCOMPARE(connectedSpy.count(), 1);
    QCOMPARE(errorSpy.count(), 0);
    QVERIFY(m_component->isConnected());
}

void Do3ThinkCameraComponentTest::testParameterSetting() {
    // Test exposure time setting
    {
        double exposureTime = 1000.0; // 1ms
        bool result = m_component->setExposureTime(exposureTime);
        QVERIFY(result);
        QCOMPARE(m_component->getExposureTime(), exposureTime);
    }
    
    // Test gain setting
    {
        double gain = 2.0;
        bool result = m_component->setGain(gain);
        QVERIFY(result);
        QCOMPARE(m_component->getGain(), gain);
    }
    
    // Test invalid parameters
    {
        bool result = m_component->setExposureTime(-1.0);
        QVERIFY(!result);
    }
}

void Do3ThinkCameraComponentTest::testSignalEmission() {
    // Arrange
    QSignalSpy frameReadySpy(m_component.get(), &Do3ThinkCameraComponent::frameReady);
    QSignalSpy fpsUpdatedSpy(m_component.get(), &Do3ThinkCameraComponent::fpsUpdated);
    
    // Act
    m_component->startAcquisition();
    QTest::qWait(1100); // Wait for at least 1 second
    m_component->stopAcquisition();
    
    // Assert
    QVERIFY(frameReadySpy.count() > 0);
    QVERIFY(fpsUpdatedSpy.count() > 0);
}
```

### 1.2 測試用例設計

#### 正常流程測試
```cpp
class NormalFlowTest : public QObject {
    Q_OBJECT
private slots:
    void testCompleteWorkflow() {
        // 1. 初始化組件
        Do3ThinkCameraComponent component;
        QVERIFY(component.initialize());
        
        // 2. 枚舉設備
        auto devices = component.getAvailableDevices();
        QVERIFY(!devices.isEmpty());
        
        // 3. 連接相機
        QVERIFY(component.connectDevice(0));
        
        // 4. 設置參數
        QVERIFY(component.setExposureTime(5000));
        QVERIFY(component.setGain(1.5));
        QVERIFY(component.setROI(QRect(0, 0, 640, 480)));
        
        // 5. 開始採集
        QVERIFY(component.startAcquisition());
        
        // 6. 採集數據
        QSignalSpy spy(&component, &Do3ThinkCameraComponent::frameReady);
        QTest::qWait(1000);
        QVERIFY(spy.count() > 0);
        
        // 7. 停止採集
        component.stopAcquisition();
        
        // 8. 斷開連接
        component.disconnectDevice();
    }
};
```

#### 異常流程測試
```cpp
class ExceptionFlowTest : public QObject {
    Q_OBJECT
private slots:
    void testCameraDisconnectionDuringAcquisition() {
        // Simulate camera disconnection
        Do3ThinkCameraComponent component;
        component.initialize();
        component.connectDevice(0);
        component.startAcquisition();
        
        // Simulate disconnect
        simulateCameraDisconnect();
        
        // Verify error handling
        QSignalSpy errorSpy(&component, &Do3ThinkCameraComponent::errorOccurred);
        QVERIFY(errorSpy.wait());
        QVERIFY(!component.isConnected());
    }
    
    void testInvalidParameterRecovery() {
        Do3ThinkCameraComponent component;
        component.initialize();
        component.connectDevice(0);
        
        // Set invalid parameter
        QVERIFY(!component.setExposureTime(-100));
        
        // Verify component still functional
        QVERIFY(component.setExposureTime(1000));
        QVERIFY(component.startAcquisition());
    }
};
```

#### 邊界條件測試
```cpp
class BoundaryConditionTest : public QObject {
    Q_OBJECT
private slots:
    void testMaximumResolution() {
        Do3ThinkCameraComponent component;
        component.initialize();
        component.connectDevice(0);
        
        // Set maximum resolution
        auto maxRes = component.getMaxResolution();
        QVERIFY(component.setResolution(maxRes));
        
        // Test acquisition at max resolution
        QVERIFY(component.startAcquisition());
        QTest::qWait(1000);
        component.stopAcquisition();
    }
    
    void testMinimumExposure() {
        Do3ThinkCameraComponent component;
        component.initialize();
        component.connectDevice(0);
        
        double minExposure = component.getMinExposureTime();
        QVERIFY(component.setExposureTime(minExposure));
        
        // Verify high FPS
        component.startAcquisition();
        QTest::qWait(1000);
        QVERIFY(component.getCurrentFPS() > 100);
    }
};
```

## 2. 整合測試

### 2.1 Component與Panel整合

```cpp
class ComponentPanelIntegrationTest : public QObject {
    Q_OBJECT
private slots:
    void testSignalSlotConnection() {
        // Create component and panel
        Do3ThinkCameraComponent component;
        Do3ThinkCameraPanel panel;
        
        // Connect them
        panel.setComponent(&component);
        
        // Test UI to Component
        QSignalSpy spy(&component, &Do3ThinkCameraComponent::parameterChanged);
        panel.setExposureValue(2000);
        QVERIFY(spy.wait());
        
        // Test Component to UI
        component.updateFPS(120.5);
        QCOMPARE(panel.getFPSDisplay(), "120.5");
    }
    
    void testDataFlow() {
        Do3ThinkCameraComponent component;
        Do3ThinkCameraPanel panel;
        panel.setComponent(&component);
        
        // Start acquisition through UI
        panel.clickStartButton();
        
        // Verify data flow
        QSignalSpy frameSpy(&component, &Do3ThinkCameraComponent::frameReady);
        QVERIFY(frameSpy.wait());
        
        // Check display update
        QVERIFY(panel.isDisplayingFrame());
    }
};
```

### 2.2 系統整合測試

```cpp
class SystemIntegrationTest : public QObject {
    Q_OBJECT
private slots:
    void testMultiComponentCooperation() {
        // Create system components
        Do3ThinkCameraComponent camera;
        ImageProcessingComponent processor;
        DataStorageComponent storage;
        
        // Connect components
        connect(&camera, &Do3ThinkCameraComponent::frameReady,
                &processor, &ImageProcessingComponent::processFrame);
        connect(&processor, &ImageProcessingComponent::processingComplete,
                &storage, &DataStorageComponent::saveData);
        
        // Test workflow
        camera.initialize();
        camera.connectDevice(0);
        camera.startAcquisition();
        
        QSignalSpy storageSpy(&storage, &DataStorageComponent::dataSaved);
        QVERIFY(storageSpy.wait(5000));
    }
    
    void testStateMachineIntegration() {
        SystemStateMachine stateMachine;
        Do3ThinkCameraComponent camera;
        
        // Register component
        stateMachine.registerComponent(&camera);
        
        // Test state transitions
        stateMachine.transitionTo(SystemState::Initializing);
        QVERIFY(camera.getState() == ComponentState::Initialized);
        
        stateMachine.transitionTo(SystemState::Running);
        QVERIFY(camera.isAcquiring());
        
        stateMachine.transitionTo(SystemState::Stopped);
        QVERIFY(!camera.isAcquiring());
    }
    
    void testEventBusCommunication() {
        EventBus eventBus;
        Do3ThinkCameraComponent camera;
        
        // Subscribe to events
        camera.subscribeToEventBus(&eventBus);
        
        // Send command through event bus
        CameraCommand cmd;
        cmd.type = CommandType::StartAcquisition;
        eventBus.publish(cmd);
        
        // Verify response
        QSignalSpy eventSpy(&eventBus, &EventBus::eventPublished);
        QVERIFY(eventSpy.wait());
        
        auto response = eventSpy.takeFirst().at(0).value<CameraResponse>();
        QVERIFY(response.success);
    }
};
```

### 2.3 多相機測試

```cpp
class MultiCameraTest : public QObject {
    Q_OBJECT
private slots:
    void testSynchronousAcquisition() {
        const int cameraCount = 4;
        std::vector<std::unique_ptr<Do3ThinkCameraComponent>> cameras;
        
        // Initialize cameras
        for (int i = 0; i < cameraCount; ++i) {
            auto camera = std::make_unique<Do3ThinkCameraComponent>();
            camera->initialize();
            camera->connectDevice(i);
            cameras.push_back(std::move(camera));
        }
        
        // Start synchronized acquisition
        for (auto& camera : cameras) {
            camera->enableHardwareTrigger(TriggerSource::External);
        }
        
        // Send trigger signal
        sendHardwareTrigger();
        
        // Verify synchronization
        std::vector<QSignalSpy*> spies;
        for (auto& camera : cameras) {
            spies.push_back(new QSignalSpy(camera.get(), 
                &Do3ThinkCameraComponent::frameReady));
        }
        
        // Check all cameras captured within 10ms
        QElapsedTimer timer;
        timer.start();
        for (auto* spy : spies) {
            QVERIFY(spy->wait(100));
        }
        QVERIFY(timer.elapsed() < 10);
        
        // Cleanup
        for (auto* spy : spies) {
            delete spy;
        }
    }
    
    void testResourceCompetition() {
        // Test bandwidth limitation
        const int maxCameras = 8;
        std::vector<std::unique_ptr<Do3ThinkCameraComponent>> cameras;
        
        for (int i = 0; i < maxCameras; ++i) {
            auto camera = std::make_unique<Do3ThinkCameraComponent>();
            camera->initialize();
            
            if (camera->connectDevice(i)) {
                cameras.push_back(std::move(camera));
            } else {
                // Reached resource limit
                break;
            }
        }
        
        // Start all cameras
        for (auto& camera : cameras) {
            camera->startAcquisition();
        }
        
        // Monitor performance
        QTest::qWait(5000);
        
        for (auto& camera : cameras) {
            double fps = camera->getCurrentFPS();
            double targetFps = camera->getTargetFPS();
            QVERIFY(fps >= targetFps * 0.9); // Allow 10% deviation
        }
    }
    
    void testPerformanceScaling() {
        // Test CPU usage with multiple cameras
        SystemMonitor monitor;
        std::vector<std::unique_ptr<Do3ThinkCameraComponent>> cameras;
        
        auto baseline = monitor.getCPUUsage();
        
        for (int i = 1; i <= 4; ++i) {
            auto camera = std::make_unique<Do3ThinkCameraComponent>();
            camera->initialize();
            camera->connectDevice(i - 1);
            camera->startAcquisition();
            cameras.push_back(std::move(camera));
            
            QTest::qWait(1000);
            auto cpuUsage = monitor.getCPUUsage();
            
            // CPU usage should scale linearly
            double expectedUsage = baseline + (i * 10); // ~10% per camera
            QVERIFY(cpuUsage < expectedUsage * 1.2); // Allow 20% deviation
        }
    }
};
```

## 3. 性能測試

### 3.1 基準測試

```cpp
// PerformanceBenchmark.h
class PerformanceBenchmark : public QObject {
    Q_OBJECT
    
private:
    struct BenchmarkResult {
        QString testName;
        double averageTime;
        double minTime;
        double maxTime;
        double standardDeviation;
        int iterations;
    };
    
private slots:
    void benchmarkAcquisitionSpeed() {
        Do3ThinkCameraComponent camera;
        camera.initialize();
        camera.connectDevice(0);
        
        const int iterations = 1000;
        std::vector<double> frameTimes;
        
        camera.startAcquisition();
        
        QElapsedTimer timer;
        for (int i = 0; i < iterations; ++i) {
            timer.restart();
            auto frame = camera.getNextFrame();
            frameTimes.push_back(timer.nsecsElapsed() / 1000000.0); // Convert to ms
        }
        
        camera.stopAcquisition();
        
        // Calculate statistics
        auto result = calculateStatistics(frameTimes);
        
        // Report
        qDebug() << "Acquisition Speed Benchmark:";
        qDebug() << "  Average:" << result.averageTime << "ms";
        qDebug() << "  Min:" << result.minTime << "ms";
        qDebug() << "  Max:" << result.maxTime << "ms";
        qDebug() << "  StdDev:" << result.standardDeviation << "ms";
        qDebug() << "  FPS:" << 1000.0 / result.averageTime;
        
        // Verify performance requirements
        QVERIFY(result.averageTime < 10.0); // < 10ms per frame (>100 FPS)
    }
    
    void benchmarkProcessingLatency() {
        Do3ThinkCameraComponent camera;
        camera.initialize();
        camera.connectDevice(0);
        
        std::vector<double> latencies;
        
        // Measure end-to-end latency
        for (int i = 0; i < 100; ++i) {
            QElapsedTimer timer;
            timer.start();
            
            camera.triggerSoftware();
            QSignalSpy spy(&camera, &Do3ThinkCameraComponent::frameReady);
            spy.wait();
            
            latencies.push_back(timer.elapsed());
        }
        
        auto result = calculateStatistics(latencies);
        
        qDebug() << "Processing Latency Benchmark:";
        qDebug() << "  Average:" << result.averageTime << "ms";
        qDebug() << "  99th percentile:" << calculate99thPercentile(latencies) << "ms";
        
        // Verify latency requirements
        QVERIFY(result.averageTime < 100.0); // < 100ms average latency
        QVERIFY(calculate99thPercentile(latencies) < 150.0); // < 150ms 99th percentile
    }
    
    void benchmarkMemoryUsage() {
        SystemMonitor monitor;
        Do3ThinkCameraComponent camera;
        
        // Baseline memory
        auto baselineMemory = monitor.getMemoryUsage();
        
        camera.initialize();
        camera.connectDevice(0);
        
        // After initialization
        auto initMemory = monitor.getMemoryUsage();
        
        camera.startAcquisition();
        QTest::qWait(10000); // Run for 10 seconds
        
        // During acquisition
        auto runMemory = monitor.getMemoryUsage();
        
        camera.stopAcquisition();
        
        // Report
        qDebug() << "Memory Usage Benchmark:";
        qDebug() << "  Baseline:" << baselineMemory << "MB";
        qDebug() << "  After Init:" << initMemory << "MB";
        qDebug() << "  During Run:" << runMemory << "MB";
        qDebug() << "  Init Overhead:" << (initMemory - baselineMemory) << "MB";
        qDebug() << "  Run Overhead:" << (runMemory - initMemory) << "MB";
        
        // Verify memory requirements
        QVERIFY((initMemory - baselineMemory) < 100); // < 100MB init overhead
        QVERIFY((runMemory - initMemory) < 500); // < 500MB runtime overhead
    }
    
    void benchmarkCPUUsage() {
        SystemMonitor monitor;
        Do3ThinkCameraComponent camera;
        
        camera.initialize();
        camera.connectDevice(0);
        camera.setTargetFPS(100);
        
        // Baseline CPU
        auto baselineCPU = monitor.getCPUUsage();
        
        camera.startAcquisition();
        QTest::qWait(5000); // Stabilize
        
        // Measure CPU over 10 seconds
        std::vector<double> cpuSamples;
        for (int i = 0; i < 10; ++i) {
            cpuSamples.push_back(monitor.getCPUUsage());
            QTest::qWait(1000);
        }
        
        camera.stopAcquisition();
        
        auto result = calculateStatistics(cpuSamples);
        
        qDebug() << "CPU Usage Benchmark:";
        qDebug() << "  Baseline:" << baselineCPU << "%";
        qDebug() << "  Average:" << result.averageTime << "%";
        qDebug() << "  Peak:" << result.maxTime << "%";
        
        // Verify CPU requirements
        QVERIFY(result.averageTime < 30.0); // < 30% average CPU
        QVERIFY(result.maxTime < 50.0); // < 50% peak CPU
    }
    
private:
    BenchmarkResult calculateStatistics(const std::vector<double>& data);
    double calculate99thPercentile(std::vector<double> data);
};
```

### 3.2 性能指標

```cpp
class PerformanceMetrics {
public:
    struct FPSTest {
        double targetFPS;
        double achievedFPS;
        double stability; // Standard deviation
        bool passed;
    };
    
    struct LatencyTest {
        double averageLatency;
        double p50Latency;
        double p95Latency;
        double p99Latency;
        double maxLatency;
        bool passed;
    };
    
    struct ResourceTest {
        double cpuUsage;
        double memoryUsage;
        double bandwidthUsage;
        double diskIORate;
        bool passed;
    };
    
    struct StabilityTest {
        double runDuration; // hours
        int errorCount;
        int frameDropCount;
        double memoryLeakRate; // MB/hour
        bool passed;
    };
    
    // FPS Testing (100-1000 fps)
    static FPSTest testFPS(Do3ThinkCameraComponent& camera, double targetFPS) {
        FPSTest result;
        result.targetFPS = targetFPS;
        
        camera.setTargetFPS(targetFPS);
        camera.startAcquisition();
        
        std::vector<double> fpsReadings;
        for (int i = 0; i < 60; ++i) { // 1 minute test
            QTest::qWait(1000);
            fpsReadings.push_back(camera.getCurrentFPS());
        }
        
        camera.stopAcquisition();
        
        // Calculate metrics
        result.achievedFPS = std::accumulate(fpsReadings.begin(), 
            fpsReadings.end(), 0.0) / fpsReadings.size();
        result.stability = calculateStandardDeviation(fpsReadings);
        result.passed = (result.achievedFPS >= targetFPS * 0.95) && 
                       (result.stability < targetFPS * 0.05);
        
        return result;
    }
    
    // Latency Testing (< 100ms)
    static LatencyTest testLatency(Do3ThinkCameraComponent& camera) {
        LatencyTest result;
        std::vector<double> latencies;
        
        for (int i = 0; i < 1000; ++i) {
            QElapsedTimer timer;
            timer.start();
            
            camera.triggerSoftware();
            auto frame = camera.waitForFrame(1000);
            
            if (frame) {
                latencies.push_back(timer.elapsed());
            }
        }
        
        // Sort for percentile calculation
        std::sort(latencies.begin(), latencies.end());
        
        result.averageLatency = std::accumulate(latencies.begin(), 
            latencies.end(), 0.0) / latencies.size();
        result.p50Latency = latencies[latencies.size() * 0.50];
        result.p95Latency = latencies[latencies.size() * 0.95];
        result.p99Latency = latencies[latencies.size() * 0.99];
        result.maxLatency = latencies.back();
        
        result.passed = (result.averageLatency < 100.0) && 
                       (result.p99Latency < 150.0);
        
        return result;
    }
    
    // 24/7 Stability Testing
    static StabilityTest testStability(Do3ThinkCameraComponent& camera, 
                                       double durationHours = 24) {
        StabilityTest result;
        result.runDuration = durationHours;
        result.errorCount = 0;
        result.frameDropCount = 0;
        
        SystemMonitor monitor;
        auto startMemory = monitor.getMemoryUsage();
        
        QElapsedTimer timer;
        timer.start();
        
        camera.startAcquisition();
        
        int expectedFrames = 0;
        int receivedFrames = 0;
        
        while (timer.elapsed() < durationHours * 3600 * 1000) {
            QSignalSpy frameSpy(&camera, &Do3ThinkCameraComponent::frameReady);
            QSignalSpy errorSpy(&camera, &Do3ThinkCameraComponent::errorOccurred);
            
            QTest::qWait(60000); // Check every minute
            
            expectedFrames += camera.getTargetFPS() * 60;
            receivedFrames += frameSpy.count();
            result.errorCount += errorSpy.count();
            
            // Check for recovery from errors
            if (!camera.isAcquiring()) {
                qWarning() << "Camera stopped, attempting restart";
                camera.startAcquisition();
            }
        }
        
        camera.stopAcquisition();
        
        auto endMemory = monitor.getMemoryUsage();
        result.memoryLeakRate = (endMemory - startMemory) / durationHours;
        result.frameDropCount = expectedFrames - receivedFrames;
        
        result.passed = (result.errorCount < 10) && 
                       (result.frameDropCount < expectedFrames * 0.001) && // < 0.1% drops
                       (result.memoryLeakRate < 10.0); // < 10 MB/hour
        
        return result;
    }
};
```

## 4. 自動化測試

### 4.1 CI/CD配置

#### GitHub Actions配置
```yaml
# .github/workflows/ci.yml
name: Do3ThinkCamera CI/CD

on:
  push:
    branches: [ main, develop ]
  pull_request:
    branches: [ main ]
  schedule:
    - cron: '0 2 * * *' # Nightly build

jobs:
  build-and-test:
    runs-on: ${{ matrix.os }}
    strategy:
      matrix:
        os: [ubuntu-latest, windows-latest]
        qt_version: [6.4.0, 6.5.0]
        build_type: [Debug, Release]
    
    steps:
    - uses: actions/checkout@v3
      with:
        submodules: recursive
    
    - name: Setup Qt
      uses: jurplel/install-qt-action@v3
      with:
        version: ${{ matrix.qt_version }}
        modules: 'qtcharts qtnetworkauth'
    
    - name: Setup Do3Think SDK
      run: |
        if [ "$RUNNER_OS" == "Windows" ]; then
          ./scripts/install_do3think_sdk_windows.ps1
        else
          ./scripts/install_do3think_sdk_linux.sh
        fi
      shell: bash
    
    - name: Configure CMake
      run: |
        cmake -B build \
          -DCMAKE_BUILD_TYPE=${{ matrix.build_type }} \
          -DENABLE_TESTING=ON \
          -DENABLE_COVERAGE=ON
    
    - name: Build
      run: cmake --build build --config ${{ matrix.build_type }} --parallel
    
    - name: Run Unit Tests
      run: |
        cd build
        ctest -C ${{ matrix.build_type }} --output-on-failure
    
    - name: Run Integration Tests
      if: matrix.os == 'windows-latest' # Requires hardware
      run: |
        cd build
        ./tests/integration_tests --gtest_output=xml:test_results.xml
    
    - name: Code Coverage
      if: matrix.build_type == 'Debug'
      run: |
        lcov --capture --directory . --output-file coverage.info
        lcov --remove coverage.info '/usr/*' --output-file coverage.info
        lcov --list coverage.info
    
    - name: Upload Coverage
      if: matrix.build_type == 'Debug'
      uses: codecov/codecov-action@v3
      with:
        file: ./coverage.info
        flags: unittests
    
    - name: Performance Regression Test
      if: matrix.build_type == 'Release'
      run: |
        cd build
        ./tests/performance_benchmark --benchmark_out=benchmark.json
        python ../scripts/check_performance_regression.py benchmark.json
    
    - name: Static Analysis
      run: |
        cppcheck --enable=all --suppress=missingIncludeSystem \
          --error-exitcode=1 --xml --xml-version=2 \
          src/ 2>cppcheck.xml
    
    - name: Build Documentation
      run: |
        doxygen Doxyfile
    
    - name: Package
      if: matrix.build_type == 'Release'
      run: |
        cmake --build build --target package
    
    - name: Upload Artifacts
      uses: actions/upload-artifact@v3
      with:
        name: Do3ThinkCamera-${{ matrix.os }}-${{ matrix.build_type }}
        path: build/packages/*

  deployment:
    needs: build-and-test
    runs-on: ubuntu-latest
    if: github.ref == 'refs/heads/main'
    
    steps:
    - name: Download Artifacts
      uses: actions/download-artifact@v3
    
    - name: Create Release
      uses: softprops/action-gh-release@v1
      with:
        files: |
          Do3ThinkCamera-*/Do3ThinkCamera-*.exe
          Do3ThinkCamera-*/Do3ThinkCamera-*.deb
          Do3ThinkCamera-*/Do3ThinkCamera-*.rpm
        tag_name: v${{ github.run_number }}
        draft: false
        prerelease: false
```

#### GitLab CI配置
```yaml
# .gitlab-ci.yml
stages:
  - build
  - test
  - analyze
  - deploy

variables:
  GIT_SUBMODULE_STRATEGY: recursive

before_script:
  - apt-get update -qq
  - apt-get install -qq qt6-base-dev cmake ninja-build

build:
  stage: build
  script:
    - mkdir build
    - cd build
    - cmake .. -GNinja -DCMAKE_BUILD_TYPE=Release
    - ninja
  artifacts:
    paths:
      - build/
    expire_in: 1 week

unit-tests:
  stage: test
  dependencies:
    - build
  script:
    - cd build
    - ctest --output-on-failure
  coverage: '/^TOTAL.*\s+(\d+\%)$/'
  artifacts:
    reports:
      junit: build/test-results/*.xml
      coverage_report:
        coverage_format: cobertura
        path: build/coverage.xml

integration-tests:
  stage: test
  dependencies:
    - build
  script:
    - cd build
    - ./tests/integration_tests
  only:
    - main
    - develop

performance-tests:
  stage: test
  dependencies:
    - build
  script:
    - cd build
    - ./tests/performance_benchmark --benchmark_format=json > benchmark.json
    - python3 ../scripts/analyze_performance.py benchmark.json
  artifacts:
    paths:
      - build/benchmark.json
    reports:
      performance: build/benchmark.json

code-quality:
  stage: analyze
  script:
    - clang-tidy src/*.cpp -- -I./include
    - cppcheck --enable=all src/
  allow_failure: true

deploy:
  stage: deploy
  dependencies:
    - build
  script:
    - ./scripts/deploy.sh
  environment:
    name: production
    url: https://camera.example.com
  only:
    - main
  when: manual
```

### 4.2 測試框架整合

#### Qt Test整合
```cpp
// TestRunner.cpp
#include <QTest>
#include <QCoreApplication>
#include "Do3ThinkCameraComponentTest.h"
#include "IntegrationTest.h"
#include "PerformanceTest.h"

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    
    int status = 0;
    
    // Run unit tests
    {
        Do3ThinkCameraComponentTest test;
        status |= QTest::qExec(&test, argc, argv);
    }
    
    // Run integration tests
    {
        IntegrationTest test;
        status |= QTest::qExec(&test, argc, argv);
    }
    
    // Run performance tests
    if (app.arguments().contains("--benchmark")) {
        PerformanceTest test;
        status |= QTest::qExec(&test, argc, argv);
    }
    
    return status;
}
```

#### Google Test整合
```cpp
// GTestMain.cpp
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <QCoreApplication>
#include "Do3ThinkCameraGTest.h"

class QtEnvironment : public ::testing::Environment {
public:
    void SetUp() override {
        static int argc = 0;
        static QCoreApplication app(argc, nullptr);
    }
};

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new QtEnvironment);
    return RUN_ALL_TESTS();
}

// Do3ThinkCameraGTest.h
TEST(Do3ThinkCameraTest, Initialization) {
    Do3ThinkCameraComponent camera;
    EXPECT_TRUE(camera.initialize());
    EXPECT_EQ(camera.getState(), ComponentState::Initialized);
}

TEST(Do3ThinkCameraTest, ConnectionWithMock) {
    MockDo3ThinkCamera mockCamera;
    EXPECT_CALL(mockCamera, connect()).WillOnce(Return(true));
    EXPECT_CALL(mockCamera, isConnected()).WillRepeatedly(Return(true));
    
    Do3ThinkCameraComponent component(&mockCamera);
    EXPECT_TRUE(component.connectDevice(0));
}
```

#### Catch2整合
```cpp
// Catch2Tests.cpp
#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>
#include "Do3ThinkCameraComponent.h"

TEST_CASE("Do3ThinkCamera initialization", "[camera]") {
    Do3ThinkCameraComponent camera;
    
    SECTION("successful initialization") {
        REQUIRE(camera.initialize());
        REQUIRE(camera.getState() == ComponentState::Initialized);
    }
    
    SECTION("device enumeration") {
        REQUIRE(camera.initialize());
        auto devices = camera.getAvailableDevices();
        REQUIRE(devices.size() >= 0);
    }
}

TEST_CASE("Do3ThinkCamera parameters", "[camera]") {
    Do3ThinkCameraComponent camera;
    camera.initialize();
    camera.connectDevice(0);
    
    SECTION("exposure time") {
        double exposure = 1000.0;
        REQUIRE(camera.setExposureTime(exposure));
        REQUIRE(camera.getExposureTime() == Approx(exposure));
    }
    
    SECTION("gain") {
        double gain = 2.0;
        REQUIRE(camera.setGain(gain));
        REQUIRE(camera.getGain() == Approx(gain));
    }
}
```

## 5. 部署方案

### 5.1 構建配置

```cmake
# CMakeLists.txt
cmake_minimum_required(VERSION 3.20)
project(Do3ThinkCameraComponent VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

# Find dependencies
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Concurrent)
find_package(Threads REQUIRED)

# Do3Think SDK
if(WIN32)
    set(DO3THINK_SDK_PATH "C:/Program Files/Do3Think/SDK")
    set(DO3THINK_LIB "${DO3THINK_SDK_PATH}/lib/DVPCamera64.lib")
elseif(UNIX)
    set(DO3THINK_SDK_PATH "/opt/Do3Think/SDK")
    set(DO3THINK_LIB "${DO3THINK_SDK_PATH}/lib/libDVPCamera.so")
endif()

# Component library
add_library(Do3ThinkCameraComponent SHARED
    src/Do3ThinkCameraComponent.cpp
    src/Do3ThinkCameraComponent.h
    src/Do3ThinkCameraWorker.cpp
    src/Do3ThinkCameraWorker.h
    src/FrameBuffer.cpp
    src/FrameBuffer.h
    src/CameraParameters.cpp
    src/CameraParameters.h
)

target_include_directories(Do3ThinkCameraComponent 
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${DO3THINK_SDK_PATH}/include
)

target_link_libraries(Do3ThinkCameraComponent
    PUBLIC
        Qt6::Core
        Qt6::Widgets
        Qt6::Concurrent
    PRIVATE
        ${DO3THINK_LIB}
        Threads::Threads
)

# Export configuration
include(GNUInstallDirs)
install(TARGETS Do3ThinkCameraComponent
    EXPORT Do3ThinkCameraComponentTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Package configuration
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    Do3ThinkCameraComponentConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(EXPORT Do3ThinkCameraComponentTargets
    FILE Do3ThinkCameraComponentTargets.cmake
    NAMESPACE Do3Think::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Do3ThinkCameraComponent
)

# CPack configuration
set(CPACK_PACKAGE_NAME "Do3ThinkCameraComponent")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION "Do3Think Camera Component for Industrial Vision")
set(CPACK_PACKAGE_VENDOR "Your Company")

if(WIN32)
    set(CPACK_GENERATOR "NSIS;ZIP")
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
    set(CPACK_NSIS_MODIFY_PATH ON)
elseif(UNIX)
    set(CPACK_GENERATOR "DEB;RPM;TGZ")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "qtbase6 (>= 6.4.0)")
    set(CPACK_RPM_PACKAGE_REQUIRES "qt6-qtbase >= 6.4.0")
endif()

include(CPack)

# Testing
if(ENABLE_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()
```

### 5.2 依賴管理

```cmake
# Dependencies.cmake
include(FetchContent)

# Manage third-party dependencies
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.12.0
)

FetchContent_Declare(
    yaml-cpp
    GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
    GIT_TAG yaml-cpp-0.7.0
)

FetchContent_MakeAvailable(spdlog yaml-cpp)

# Version control
set(MINIMUM_QT_VERSION "6.4.0")
set(MINIMUM_CMAKE_VERSION "3.20")
set(DO3THINK_SDK_VERSION "2.5.0")

# Check versions
if(Qt6_VERSION VERSION_LESS ${MINIMUM_QT_VERSION})
    message(FATAL_ERROR "Qt version ${Qt6_VERSION} is too old. Minimum required: ${MINIMUM_QT_VERSION}")
endif()

# Dependency verification script
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/scripts/verify_dependencies.sh.in
    ${CMAKE_CURRENT_BINARY_DIR}/verify_dependencies.sh
    @ONLY
)
```

### 5.3 安裝程序

#### Windows Installer (NSIS)
```nsis
; Do3ThinkCameraComponent.nsi
!include "MUI2.nsh"

Name "Do3Think Camera Component"
OutFile "Do3ThinkCameraComponent-Setup.exe"
InstallDir "$PROGRAMFILES64\Do3ThinkCamera"
RequestExecutionLevel admin

; Pages
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "LICENSE.txt"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_WELCOME
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

!insertmacro MUI_LANGUAGE "English"

Section "Main Component" SEC01
    SetOutPath "$INSTDIR"
    
    ; Check for Do3Think SDK
    IfFileExists "$PROGRAMFILES64\Do3Think\SDK\DVPCamera64.dll" sdk_found sdk_not_found
    
    sdk_not_found:
        MessageBox MB_YESNO "Do3Think SDK not found. Download and install?" IDYES download_sdk
        Abort "Installation cancelled"
    
    download_sdk:
        ExecWait '"$INSTDIR\install_sdk.exe"'
    
    sdk_found:
    ; Install files
    File "Do3ThinkCameraComponent.dll"
    File "Do3ThinkCameraPanel.dll"
    File "Qt6Core.dll"
    File "Qt6Widgets.dll"
    
    ; Register DLLs
    ExecWait 'regsvr32 /s "$INSTDIR\Do3ThinkCameraComponent.dll"'
    
    ; Create shortcuts
    CreateDirectory "$SMPROGRAMS\Do3ThinkCamera"
    CreateShortcut "$SMPROGRAMS\Do3ThinkCamera\Uninstall.lnk" "$INSTDIR\Uninstall.exe"
    
    ; Write registry
    WriteRegStr HKLM "Software\Do3ThinkCamera" "InstallPath" "$INSTDIR"
    WriteRegStr HKLM "Software\Do3ThinkCamera" "Version" "1.0.0"
    
    ; Create uninstaller
    WriteUninstaller "$INSTDIR\Uninstall.exe"
SectionEnd

Section "Uninstall"
    Delete "$INSTDIR\*.*"
    RMDir "$INSTDIR"
    
    Delete "$SMPROGRAMS\Do3ThinkCamera\*.*"
    RMDir "$SMPROGRAMS\Do3ThinkCamera"
    
    DeleteRegKey HKLM "Software\Do3ThinkCamera"
SectionEnd
```

#### Linux Package (.deb)
```bash
#!/bin/bash
# create_deb_package.sh

PACKAGE_NAME="do3thinkcamera"
VERSION="1.0.0"
ARCH="amd64"

# Create package structure
mkdir -p ${PACKAGE_NAME}_${VERSION}_${ARCH}/DEBIAN
mkdir -p ${PACKAGE_NAME}_${VERSION}_${ARCH}/usr/lib
mkdir -p ${PACKAGE_NAME}_${VERSION}_${ARCH}/usr/include
mkdir -p ${PACKAGE_NAME}_${VERSION}_${ARCH}/usr/share/doc/${PACKAGE_NAME}
mkdir -p ${PACKAGE_NAME}_${VERSION}_${ARCH}/etc/udev/rules.d

# Control file
cat > ${PACKAGE_NAME}_${VERSION}_${ARCH}/DEBIAN/control << EOF
Package: ${PACKAGE_NAME}
Version: ${VERSION}
Section: libs
Priority: optional
Architecture: ${ARCH}
Depends: libqt6core6 (>= 6.4.0), libqt6widgets6 (>= 6.4.0)
Maintainer: Your Name <your.email@example.com>
Description: Do3Think Camera Component for Industrial Vision
 High-performance camera component for Do3Think industrial cameras
 with Qt6 integration.
EOF

# Post-install script
cat > ${PACKAGE_NAME}_${VERSION}_${ARCH}/DEBIAN/postinst << EOF
#!/bin/bash
set -e

# Add user to video group for camera access
usermod -a -G video \$SUDO_USER

# Reload udev rules
udevadm control --reload-rules
udevadm trigger

# Update library cache
ldconfig

echo "Do3Think Camera Component installed successfully"
echo "Please log out and log back in for group changes to take effect"
EOF

chmod 755 ${PACKAGE_NAME}_${VERSION}_${ARCH}/DEBIAN/postinst

# udev rules for camera access
cat > ${PACKAGE_NAME}_${VERSION}_${ARCH}/etc/udev/rules.d/99-do3think.rules << EOF
# Do3Think Camera USB rules
SUBSYSTEM=="usb", ATTRS{idVendor}=="1234", MODE="0666", GROUP="video"
EOF

# Copy files
cp build/libDo3ThinkCameraComponent.so ${PACKAGE_NAME}_${VERSION}_${ARCH}/usr/lib/
cp include/*.h ${PACKAGE_NAME}_${VERSION}_${ARCH}/usr/include/
cp README.md LICENSE ${PACKAGE_NAME}_${VERSION}_${ARCH}/usr/share/doc/${PACKAGE_NAME}/

# Build package
dpkg-deb --build ${PACKAGE_NAME}_${VERSION}_${ARCH}
```

## 6. 診斷與調試

### 6.1 調試工具

```cpp
// DebugTools.h
class DebugTools {
public:
    // 日誌系統配置
    static void configureLogging() {
        // Setup spdlog
        auto console = spdlog::stdout_color_mt("console");
        auto file = spdlog::rotating_logger_mt("file", 
            "logs/do3think_camera.log", 1048576 * 5, 3);
        
        spdlog::set_default_logger(console);
        spdlog::set_level(spdlog::level::debug);
        spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%t] %v");
        
        // Qt message handler
        qInstallMessageHandler([](QtMsgType type, 
            const QMessageLogContext& context, const QString& msg) {
            
            switch (type) {
            case QtDebugMsg:
                spdlog::debug("{}", msg.toStdString());
                break;
            case QtInfoMsg:
                spdlog::info("{}", msg.toStdString());
                break;
            case QtWarningMsg:
                spdlog::warn("{}", msg.toStdString());
                break;
            case QtCriticalMsg:
            case QtFatalMsg:
                spdlog::error("{}", msg.toStdString());
                break;
            }
        });
    }
    
    // 遠程調試支援
    class RemoteDebugger {
    public:
        void startServer(int port = 9999) {
            m_server = new QTcpServer();
            m_server->listen(QHostAddress::Any, port);
            
            connect(m_server, &QTcpServer::newConnection, [this]() {
                auto socket = m_server->nextPendingConnection();
                handleDebugConnection(socket);
            });
        }
        
        void sendDebugInfo(const QJsonObject& info) {
            for (auto* socket : m_clients) {
                socket->write(QJsonDocument(info).toJson());
            }
        }
        
    private:
        QTcpServer* m_server;
        QList<QTcpSocket*> m_clients;
        
        void handleDebugConnection(QTcpSocket* socket);
    };
    
    // 性能分析工具
    class PerformanceProfiler {
    public:
        void startProfiling() {
            m_profilingData.clear();
            m_startTime = std::chrono::high_resolution_clock::now();
        }
        
        void markEvent(const QString& event) {
            auto now = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>
                (now - m_startTime).count();
            
            m_profilingData.append({event, duration});
        }
        
        void generateReport(const QString& filename) {
            QFile file(filename);
            if (file.open(QIODevice::WriteOnly)) {
                QTextStream stream(&file);
                stream << "Event,Time(us)\n";
                for (const auto& entry : m_profilingData) {
                    stream << entry.event << "," << entry.time << "\n";
                }
            }
        }
        
    private:
        struct ProfilingEntry {
            QString event;
            qint64 time;
        };
        
        QList<ProfilingEntry> m_profilingData;
        std::chrono::high_resolution_clock::time_point m_startTime;
    };
    
    // 記憶體洩漏檢測
    class MemoryLeakDetector {
    public:
        void startDetection() {
#ifdef _WIN32
            _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
            _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
#endif
        }
        
        void checkLeaks() {
#ifdef _WIN32
            _CrtDumpMemoryLeaks();
#elif __linux__
            // Use valgrind or AddressSanitizer
            system("valgrind --leak-check=full --show-leak-kinds=all ./app");
#endif
        }
    };
};
```

### 6.2 診斷報告

```cpp
// DiagnosticReport.h
class DiagnosticReport {
public:
    struct SystemInfo {
        QString osName;
        QString osVersion;
        QString cpuInfo;
        qint64 totalMemory;
        qint64 availableMemory;
        QString qtVersion;
        QString compilerVersion;
    };
    
    struct CameraInfo {
        QString deviceName;
        QString serialNumber;
        QString firmwareVersion;
        QString driverVersion;
        bool isConnected;
        double currentFPS;
        QSize resolution;
        double exposureTime;
        double gain;
    };
    
    struct PerformanceMetrics {
        double averageFPS;
        double minFPS;
        double maxFPS;
        double averageLatency;
        double cpuUsage;
        double memoryUsage;
        qint64 framesProcessed;
        qint64 framesDropped;
        qint64 errors;
    };
    
    void generateSystemInfo() {
        m_systemInfo.osName = QSysInfo::productType();
        m_systemInfo.osVersion = QSysInfo::productVersion();
        m_systemInfo.cpuInfo = QSysInfo::currentCpuArchitecture();
        
#ifdef Q_OS_WIN
        MEMORYSTATUSEX statex;
        statex.dwLength = sizeof(statex);
        GlobalMemoryStatusEx(&statex);
        m_systemInfo.totalMemory = statex.ullTotalPhys;
        m_systemInfo.availableMemory = statex.ullAvailPhys;
#elif defined(Q_OS_LINUX)
        struct sysinfo info;
        sysinfo(&info);
        m_systemInfo.totalMemory = info.totalram * info.mem_unit;
        m_systemInfo.availableMemory = info.freeram * info.mem_unit;
#endif
        
        m_systemInfo.qtVersion = qVersion();
        m_systemInfo.compilerVersion = QString("%1 %2")
            .arg(QT_VERSION_STR)
            .arg(__VERSION__);
    }
    
    void collectCameraInfo(Do3ThinkCameraComponent* camera) {
        if (!camera) return;
        
        m_cameraInfo.deviceName = camera->getDeviceName();
        m_cameraInfo.serialNumber = camera->getSerialNumber();
        m_cameraInfo.firmwareVersion = camera->getFirmwareVersion();
        m_cameraInfo.driverVersion = camera->getDriverVersion();
        m_cameraInfo.isConnected = camera->isConnected();
        m_cameraInfo.currentFPS = camera->getCurrentFPS();
        m_cameraInfo.resolution = camera->getResolution();
        m_cameraInfo.exposureTime = camera->getExposureTime();
        m_cameraInfo.gain = camera->getGain();
    }
    
    void analyzePerformance(const QList<double>& fpsHistory,
                           const QList<double>& latencyHistory) {
        if (fpsHistory.isEmpty()) return;
        
        // FPS analysis
        m_performanceMetrics.averageFPS = std::accumulate(
            fpsHistory.begin(), fpsHistory.end(), 0.0) / fpsHistory.size();
        
        auto [minIt, maxIt] = std::minmax_element(
            fpsHistory.begin(), fpsHistory.end());
        m_performanceMetrics.minFPS = *minIt;
        m_performanceMetrics.maxFPS = *maxIt;
        
        // Latency analysis
        if (!latencyHistory.isEmpty()) {
            m_performanceMetrics.averageLatency = std::accumulate(
                latencyHistory.begin(), latencyHistory.end(), 0.0) / 
                latencyHistory.size();
        }
        
        // System resources
        SystemMonitor monitor;
        m_performanceMetrics.cpuUsage = monitor.getCPUUsage();
        m_performanceMetrics.memoryUsage = monitor.getMemoryUsage();
    }
    
    void exportReport(const QString& path) {
        QJsonObject report;
        
        // System information
        QJsonObject sysInfo;
        sysInfo["os_name"] = m_systemInfo.osName;
        sysInfo["os_version"] = m_systemInfo.osVersion;
        sysInfo["cpu_info"] = m_systemInfo.cpuInfo;
        sysInfo["total_memory"] = m_systemInfo.totalMemory;
        sysInfo["available_memory"] = m_systemInfo.availableMemory;
        sysInfo["qt_version"] = m_systemInfo.qtVersion;
        sysInfo["compiler_version"] = m_systemInfo.compilerVersion;
        report["system"] = sysInfo;
        
        // Camera information
        QJsonObject camInfo;
        camInfo["device_name"] = m_cameraInfo.deviceName;
        camInfo["serial_number"] = m_cameraInfo.serialNumber;
        camInfo["firmware_version"] = m_cameraInfo.firmwareVersion;
        camInfo["driver_version"] = m_cameraInfo.driverVersion;
        camInfo["is_connected"] = m_cameraInfo.isConnected;
        camInfo["current_fps"] = m_cameraInfo.currentFPS;
        camInfo["resolution"] = QString("%1x%2")
            .arg(m_cameraInfo.resolution.width())
            .arg(m_cameraInfo.resolution.height());
        camInfo["exposure_time"] = m_cameraInfo.exposureTime;
        camInfo["gain"] = m_cameraInfo.gain;
        report["camera"] = camInfo;
        
        // Performance metrics
        QJsonObject perfMetrics;
        perfMetrics["average_fps"] = m_performanceMetrics.averageFPS;
        perfMetrics["min_fps"] = m_performanceMetrics.minFPS;
        perfMetrics["max_fps"] = m_performanceMetrics.maxFPS;
        perfMetrics["average_latency"] = m_performanceMetrics.averageLatency;
        perfMetrics["cpu_usage"] = m_performanceMetrics.cpuUsage;
        perfMetrics["memory_usage"] = m_performanceMetrics.memoryUsage;
        perfMetrics["frames_processed"] = m_performanceMetrics.framesProcessed;
        perfMetrics["frames_dropped"] = m_performanceMetrics.framesDropped;
        perfMetrics["errors"] = m_performanceMetrics.errors;
        report["performance"] = perfMetrics;
        
        // Timestamp
        report["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
        
        // Write to file
        QFile file(path);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(QJsonDocument(report).toJson(QJsonDocument::Indented));
        }
        
        // Also generate HTML report
        generateHTMLReport(path.replace(".json", ".html"), report);
    }
    
private:
    SystemInfo m_systemInfo;
    CameraInfo m_cameraInfo;
    PerformanceMetrics m_performanceMetrics;
    
    void generateHTMLReport(const QString& path, const QJsonObject& data);
};
```

## 7. 維護與更新

### 7.1 版本管理

```cpp
// VersionManager.h
class VersionManager {
public:
    struct Version {
        int major;
        int minor;
        int patch;
        QString build;
        
        QString toString() const {
            return QString("%1.%2.%3-%4")
                .arg(major).arg(minor).arg(patch).arg(build);
        }
        
        bool operator<(const Version& other) const {
            if (major != other.major) return major < other.major;
            if (minor != other.minor) return minor < other.minor;
            if (patch != other.patch) return patch < other.patch;
            return build < other.build;
        }
    };
    
    // 語義化版本
    static Version getCurrentVersion() {
        return {1, 0, 0, "stable"};
    }
    
    // 更新檢查機制
    class UpdateChecker : public QObject {
        Q_OBJECT
    public:
        void checkForUpdates() {
            QNetworkAccessManager* manager = new QNetworkAccessManager(this);
            QNetworkRequest request(QUrl("https://api.example.com/version"));
            
            connect(manager, &QNetworkAccessManager::finished,
                [this](QNetworkReply* reply) {
                    if (reply->error() == QNetworkReply::NoError) {
                        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
                        Version latestVersion = parseVersion(doc.object());
                        
                        if (latestVersion > getCurrentVersion()) {
                            emit updateAvailable(latestVersion);
                        }
                    }
                    reply->deleteLater();
                });
            
            manager->get(request);
        }
        
    signals:
        void updateAvailable(const Version& version);
        
    private:
        Version parseVersion(const QJsonObject& obj);
    };
    
    // 自動更新支援
    class AutoUpdater : public QObject {
        Q_OBJECT
    public:
        void downloadUpdate(const QString& url) {
            QNetworkAccessManager* manager = new QNetworkAccessManager(this);
            QNetworkRequest request(QUrl(url));
            
            m_reply = manager->get(request);
            
            connect(m_reply, &QNetworkReply::downloadProgress,
                this, &AutoUpdater::updateProgress);
            
            connect(m_reply, &QNetworkReply::finished, [this]() {
                if (m_reply->error() == QNetworkReply::NoError) {
                    QFile file("update.exe");
                    if (file.open(QIODevice::WriteOnly)) {
                        file.write(m_reply->readAll());
                        file.close();
                        emit updateDownloaded();
                    }
                }
                m_reply->deleteLater();
            });
        }
        
        void installUpdate() {
            // Windows
#ifdef Q_OS_WIN
            QProcess::startDetached("update.exe", QStringList() << "/S");
#elif defined(Q_OS_LINUX)
            QProcess::startDetached("sh", QStringList() << "update.sh");
#endif
            QCoreApplication::quit();
        }
        
    signals:
        void updateProgress(qint64 bytesReceived, qint64 bytesTotal);
        void updateDownloaded();
        
    private:
        QNetworkReply* m_reply = nullptr;
    };
};
```

### 7.2 向後兼容

```cpp
// BackwardCompatibility.h
class BackwardCompatibility {
public:
    // API版本控制
    enum class APIVersion {
        V1_0_0,
        V1_1_0,
        V2_0_0
    };
    
    // 配置遷移
    class ConfigMigration {
    public:
        static bool migrateConfig(const QString& configPath) {
            QFile file(configPath);
            if (!file.open(QIODevice::ReadWrite)) {
                return false;
            }
            
            QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
            QJsonObject root = doc.object();
            
            // Check version
            QString version = root["version"].toString();
            
            if (version == "1.0.0") {
                // Migrate from 1.0.0 to 1.1.0
                migrateFrom100To110(root);
                version = "1.1.0";
            }
            
            if (version == "1.1.0") {
                // Migrate from 1.1.0 to 2.0.0
                migrateFrom110To200(root);
                version = "2.0.0";
            }
            
            root["version"] = version;
            
            // Write back
            file.seek(0);
            file.write(QJsonDocument(root).toJson());
            file.resize(file.pos());
            
            return true;
        }
        
    private:
        static void migrateFrom100To110(QJsonObject& config) {
            // Rename old fields
            if (config.contains("camera_settings")) {
                config["device_settings"] = config["camera_settings"];
                config.remove("camera_settings");
            }
            
            // Add new default values
            if (!config.contains("performance")) {
                QJsonObject perf;
                perf["target_fps"] = 30;
                perf["buffer_size"] = 10;
                config["performance"] = perf;
            }
        }
        
        static void migrateFrom110To200(QJsonObject& config) {
            // Structure changes for v2.0.0
            QJsonObject newStructure;
            
            // Reorganize settings
            newStructure["devices"] = QJsonArray{config["device_settings"]};
            newStructure["global"] = config["performance"];
            
            config = newStructure;
        }
    };
    
    // 數據格式兼容
    class DataFormatCompatibility {
    public:
        static QImage convertLegacyImageFormat(const QByteArray& data, 
                                               const QString& format) {
            if (format == "LEGACY_RAW") {
                // Convert old raw format to QImage
                int width = 640;  // Legacy default
                int height = 480;
                return QImage((const uchar*)data.data(), width, height, 
                             QImage::Format_Grayscale8);
            }
            
            return QImage::fromData(data);
        }
        
        static QJsonObject convertLegacyMetadata(const QJsonObject& legacy) {
            QJsonObject modern;
            
            // Map old field names to new ones
            modern["timestamp"] = legacy["capture_time"];
            modern["exposure_us"] = legacy["exposure_time"] * 1000;
            modern["gain_db"] = legacy["gain_value"];
            
            return modern;
        }
    };
    
    // Deprecated API wrapper
    class DeprecatedAPI {
    public:
        [[deprecated("Use connectDevice() instead")]]
        bool openCamera(int index) {
            qWarning() << "openCamera() is deprecated. Use connectDevice() instead.";
            return m_component->connectDevice(index);
        }
        
        [[deprecated("Use disconnectDevice() instead")]]
        void closeCamera() {
            qWarning() << "closeCamera() is deprecated. Use disconnectDevice() instead.";
            m_component->disconnectDevice();
        }
        
        [[deprecated("Use setExposureTime() with microseconds instead")]]
        bool setExposure(double milliseconds) {
            qWarning() << "setExposure() is deprecated. Use setExposureTime() instead.";
            return m_component->setExposureTime(milliseconds * 1000);
        }
        
    private:
        Do3ThinkCameraComponent* m_component;
    };
};
```

## 8. 故障排查指南

### 8.1 常見問題

```cpp
// TroubleshootingGuide.h
class TroubleshootingGuide {
public:
    enum class ProblemType {
        ConnectionFailed,
        NoDeviceFound,
        AcquisitionFailed,
        LowFPS,
        HighLatency,
        ImageCorruption,
        Crash,
        MemoryLeak
    };
    
    struct Problem {
        ProblemType type;
        QString description;
        QStringList symptoms;
        QStringList causes;
        QStringList solutions;
    };
    
    static QList<Problem> getCommonProblems() {
        return {
            {
                ProblemType::ConnectionFailed,
                "相機無法連接",
                {"連接函數返回false", "錯誤碼: DEVICE_NOT_FOUND"},
                {"USB線纜未連接", "驅動未安裝", "權限不足", "設備被占用"},
                {"檢查USB連接", "安裝Do3Think驅動", "以管理員權限運行", "關閉其他相機軟件"}
            },
            {
                ProblemType::NoDeviceFound,
                "找不到相機設備",
                {"枚舉設備返回空列表", "設備管理器無設備"},
                {"相機未上電", "USB控制器問題", "驅動版本不匹配"},
                {"檢查相機電源", "更換USB端口", "更新驅動到最新版本"}
            },
            {
                ProblemType::AcquisitionFailed,
                "圖像採集失敗",
                {"startAcquisition返回false", "無圖像回調"},
                {"緩衝區配置錯誤", "帶寬不足", "相機參數無效"},
                {"增加緩衝區大小", "降低分辨率或幀率", "重置相機參數"}
            },
            {
                ProblemType::LowFPS,
                "幀率過低",
                {"實際FPS低於目標值", "圖像顯示卡頓"},
                {"曝光時間過長", "CPU負載過高", "USB帶寬限制"},
                {"減少曝光時間", "優化圖像處理算法", "使用USB3.0端口"}
            },
            {
                ProblemType::HighLatency,
                "延遲過高",
                {"觸發到接收圖像時間>100ms", "實時性差"},
                {"處理隊列堆積", "系統調度延遲", "網絡傳輸慢"},
                {"減少緩衝區深度", "提高線程優先級", "使用本地處理"}
            },
            {
                ProblemType::ImageCorruption,
                "圖像損壞",
                {"圖像出現條紋", "像素值異常", "圖像不完整"},
                {"數據傳輸錯誤", "緩衝區溢出", "時序不同步"},
                {"檢查USB線纜質量", "增加緩衝區", "啟用錯誤校驗"}
            },
            {
                ProblemType::Crash,
                "程序崩潰",
                {"訪問違例", "段錯誤", "異常終止"},
                {"空指針訪問", "內存越界", "資源競爭"},
                {"檢查指針有效性", "使用邊界檢查", "加鎖保護共享資源"}
            },
            {
                ProblemType::MemoryLeak,
                "內存洩漏",
                {"內存持續增長", "最終OOM"},
                {"圖像未釋放", "循環引用", "事件未清理"},
                {"確保delete配對", "使用智能指針", "定期清理緩存"}
            }
        };
    }
    
    // 自動診斷
    class AutoDiagnostic {
    public:
        struct DiagnosticResult {
            bool passed;
            ProblemType problemType;
            QString details;
            QStringList suggestedActions;
        };
        
        static DiagnosticResult diagnose(Do3ThinkCameraComponent* camera) {
            DiagnosticResult result;
            result.passed = true;
            
            // Check connection
            if (!camera->isConnected()) {
                result.passed = false;
                result.problemType = ProblemType::ConnectionFailed;
                result.details = "Camera is not connected";
                
                // Try to identify specific cause
                auto devices = camera->getAvailableDevices();
                if (devices.isEmpty()) {
                    result.suggestedActions << "Check USB connection";
                    result.suggestedActions << "Install camera driver";
                } else {
                    result.suggestedActions << "Try reconnecting";
                    result.suggestedActions << "Check if device is in use";
                }
                return result;
            }
            
            // Check performance
            double fps = camera->getCurrentFPS();
            double targetFps = camera->getTargetFPS();
            
            if (fps < targetFps * 0.8) {
                result.passed = false;
                result.problemType = ProblemType::LowFPS;
                result.details = QString("FPS: %1/%2").arg(fps).arg(targetFps);
                
                if (camera->getExposureTime() > 1000000 / targetFps) {
                    result.suggestedActions << "Reduce exposure time";
                } else {
                    result.suggestedActions << "Check CPU usage";
                    result.suggestedActions << "Reduce image processing load";
                }
            }
            
            return result;
        }
    };
};
```

### 8.2 解決方案矩陣

| 問題類型 | 診斷方法 | 解決方案 | 預防措施 |
|---------|---------|---------|---------|
| 連接失敗 | 檢查設備列表、驅動狀態 | 重新安裝驅動、更換USB端口 | 定期更新驅動 |
| 低幀率 | 監控FPS、CPU使用率 | 優化參數、升級硬件 | 性能基準測試 |
| 內存洩漏 | 內存監控、Valgrind | 修復洩漏、使用智能指針 | 代碼審查、靜態分析 |
| 圖像錯誤 | CRC校驗、圖像分析 | 更換線纜、降低傳輸速率 | 使用高質量線纜 |
| 系統崩潰 | 崩潰轉儲分析 | 修復bug、異常處理 | 單元測試、壓力測試 |

## 9. 生產環境配置

### 9.1 推薦配置

```yaml
# production_config.yaml
hardware:
  cpu:
    minimum: "Intel Core i5-8400 or AMD Ryzen 5 2600"
    recommended: "Intel Core i7-10700 or AMD Ryzen 7 3700X"
    cores: 6
    threads: 12
  
  memory:
    minimum: 8GB
    recommended: 16GB
    type: "DDR4-2666 or better"
  
  storage:
    system: 
      type: "SSD"
      capacity: "256GB"
    data:
      type: "NVMe SSD"
      capacity: "1TB"
      speed: "3000MB/s read"
  
  usb:
    version: "USB 3.0 minimum, USB 3.2 Gen 2 recommended"
    dedicated_controller: true
    power: "Powered hub for multiple cameras"

software:
  os:
    windows:
      version: "Windows 10 Pro 64-bit version 20H2 or later"
      updates: "Latest security updates installed"
    linux:
      distribution: "Ubuntu 20.04 LTS or CentOS 8"
      kernel: "5.4 or later with PREEMPT_RT patch for real-time"
  
  runtime:
    qt:
      version: "6.4.0 or later"
      modules: ["Core", "Widgets", "Concurrent", "Network"]
    
    compiler:
      windows: "MSVC 2019 or later"
      linux: "GCC 9.0 or later"
    
    dependencies:
      - name: "Do3Think SDK"
        version: "2.5.0"
      - name: "Visual C++ Redistributable"
        version: "2019"

network:
  bandwidth: "1Gbps for remote monitoring"
  latency: "< 1ms for real-time control"
  protocols: ["TCP/IP", "UDP for streaming"]

security:
  user_permissions: "Standard user with video group membership"
  firewall: 
    - "Allow port 8080 for web interface"
    - "Allow port 9999 for debug server"
  antivirus_exclusions:
    - "Camera application directory"
    - "Data storage directory"
```

### 9.2 性能調優

```cpp
// PerformanceOptimization.h
class PerformanceOptimization {
public:
    // 系統優化
    static void optimizeSystem() {
#ifdef Q_OS_WIN
        // Windows optimizations
        // Set process priority
        SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
        
        // Disable CPU throttling
        SYSTEM_POWER_POLICY powerPolicy;
        CallNtPowerInformation(SystemPowerPolicyAc, nullptr, 0, 
            &powerPolicy, sizeof(powerPolicy));
        powerPolicy.MinSleep = PowerSystemWorking;
        powerPolicy.MaxSleep = PowerSystemWorking;
        
        // Set thread affinity for camera threads
        DWORD_PTR mask = 0x0F; // First 4 cores
        SetThreadAffinityMask(GetCurrentThread(), mask);
        
#elif defined(Q_OS_LINUX)
        // Linux optimizations
        // Set real-time scheduling
        struct sched_param param;
        param.sched_priority = sched_get_priority_max(SCHED_FIFO);
        pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);
        
        // Set CPU affinity
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(0, &cpuset);
        CPU_SET(1, &cpuset);
        pthread_setaffinity_np(pthread_self(), sizeof(cpuset), &cpuset);
        
        // Increase resource limits
        struct rlimit rlim;
        rlim.rlim_cur = RLIM_INFINITY;
        rlim.rlim_max = RLIM_INFINITY;
        setrlimit(RLIMIT_MEMLOCK, &rlim);
#endif
    }
    
    // 驅動配置
    static void optimizeDriver() {
        // USB driver optimization
#ifdef Q_OS_WIN
        // Registry settings for USB performance
        HKEY hKey;
        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
            L"SYSTEM\\CurrentControlSet\\Services\\USBSTOR", 
            0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
            
            DWORD value = 0; // Disable selective suspend
            RegSetValueEx(hKey, L"SelectiveSuspend", 0, REG_DWORD, 
                (BYTE*)&value, sizeof(value));
            RegCloseKey(hKey);
        }
#elif defined(Q_OS_LINUX)
        // Configure usbcore parameters
        system("echo 0 > /sys/module/usbcore/parameters/autosuspend");
        system("echo 64 > /sys/module/usbcore/parameters/usbfs_memory_mb");
#endif
    }
    
    // 資源分配
    class ResourceAllocator {
    public:
        static void preallocateBuffers(int count, size_t size) {
            m_bufferPool.clear();
            m_bufferPool.reserve(count);
            
            for (int i = 0; i < count; ++i) {
                auto buffer = std::make_unique<uint8_t[]>(size);
                
                // Touch pages to ensure physical allocation
                for (size_t j = 0; j < size; j += 4096) {
                    buffer[j] = 0;
                }
                
                // Lock in memory
#ifdef Q_OS_LINUX
                mlock(buffer.get(), size);
#elif defined(Q_OS_WIN)
                VirtualLock(buffer.get(), size);
#endif
                
                m_bufferPool.push_back(std::move(buffer));
            }
        }
        
        static uint8_t* getBuffer() {
            if (m_currentBuffer >= m_bufferPool.size()) {
                m_currentBuffer = 0;
            }
            return m_bufferPool[m_currentBuffer++].get();
        }
        
    private:
        static std::vector<std::unique_ptr<uint8_t[]>> m_bufferPool;
        static size_t m_currentBuffer;
    };
    
    // 實時性保證
    class RealtimeGuarantee {
    public:
        static void setupRealtimeThread(QThread* thread) {
            thread->setPriority(QThread::TimeCriticalPriority);
            
#ifdef Q_OS_LINUX
            // Set SCHED_FIFO
            pthread_t handle = thread->nativeHandle();
            struct sched_param param;
            param.sched_priority = 90;
            pthread_setschedparam(handle, SCHED_FIFO, &param);
            
            // Disable memory swapping
            mlockall(MCL_CURRENT | MCL_FUTURE);
#elif defined(Q_OS_WIN)
            // Set Windows real-time priority
            HANDLE handle = thread->nativeHandle();
            SetThreadPriority(handle, THREAD_PRIORITY_TIME_CRITICAL);
            
            // Boost thread scheduling
            SetThreadPriorityBoost(handle, FALSE);
#endif
        }
        
        static void disableInterrupts() {
#ifdef Q_OS_LINUX
            // Isolate CPUs for camera processing
            system("echo 1 > /sys/devices/system/cpu/cpu2/online");
            system("echo 0 > /proc/irq/default_smp_affinity");
#endif
        }
    };
};
```

## 總結

本手冊提供了Do3ThinkCameraComponent的完整測試策略和部署方案：

### 測試覆蓋
- ✅ 單元測試：組件功能、邊界條件、錯誤處理
- ✅ 整合測試：系統協同、多相機同步、性能擴展
- ✅ 性能測試：FPS、延遲、資源使用、穩定性
- ✅ 自動化測試：CI/CD、覆蓋率、回歸測試

### 部署準備
- ✅ 構建配置：CMake、依賴管理、打包
- ✅ 安裝程序：Windows/Linux安裝包
- ✅ 診斷工具：日誌、性能分析、遠程調試
- ✅ 版本管理：語義化版本、自動更新

### 生產保障
- ✅ 故障排查：問題診斷、解決方案
- ✅ 性能優化：系統調優、實時性保證
- ✅ 向後兼容：API版本控制、數據遷移
- ✅ 24/7運行：穩定性測試、錯誤恢復

通過完善的測試和部署流程，確保組件在各種生產環境中都能穩定、高效地運行。