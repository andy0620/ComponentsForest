# Processing Pipeline Design

## Overview

The PreProcessing Pipeline provides a flexible, high-performance framework for chaining multiple image processing operations. It supports both sequential and parallel processing paths with automatic resource management and optimization.

## Pipeline Architecture

### Core Components

```
┌─────────────┐     ┌──────────┐     ┌──────────┐     ┌─────────────┐
│   Input     │────▶│  Stage 1 │────▶│  Stage 2 │────▶│   Output    │
│   Queue     │     │  Process │     │  Process │     │   Queue     │
└─────────────┘     └──────────┘     └──────────┘     └─────────────┘
                           │                │
                           ▼                ▼
                    ┌──────────┐     ┌──────────┐
                    │  Buffer  │     │  Buffer  │
                    │   Pool   │     │   Pool   │
                    └──────────┘     └──────────┘
```

## Pipeline Stages

### 1. Input Stage
```cpp
class InputStage {
    // Receives frames from camera components
    void onFrameReceived(const QImage& frame, const FrameMetadata& metadata) {
        ProcessingUnit unit{frame, metadata, QDateTime::currentMSecsSinceEpoch()};
        m_inputQueue.enqueue(unit);
        m_processCondition.wakeOne();
    }
    
private:
    ThreadSafeQueue<ProcessingUnit> m_inputQueue;
    QWaitCondition m_processCondition;
};
```

### 2. Processing Stages
```cpp
class ProcessingStage {
    // Each stage runs in its own thread
    void process() {
        while (m_active) {
            ProcessingUnit unit = m_inputQueue.dequeue();
            
            // Apply preprocessing
            cv::Mat processed = executeAlgorithm(unit.image);
            
            // Forward to next stage or output
            if (m_nextStage) {
                m_nextStage->enqueue(processed, unit.metadata);
            } else {
                emit stageCompleted(processed, unit.metadata);
            }
        }
    }
    
protected:
    virtual cv::Mat executeAlgorithm(const cv::Mat& input) = 0;
};
```

### 3. Output Stage
```cpp
class OutputStage {
    // Collects and emits processed frames
    void onProcessingComplete(const cv::Mat& result, const FrameMetadata& metadata) {
        QImage output = MatToQImage(result);
        emit pipelineCompleted(output, metadata);
        
        // Update statistics
        updatePerformanceMetrics(metadata.timestamp);
    }
};
```

## Pipeline Patterns

### Sequential Pipeline
```cpp
class SequentialPipeline : public PreProcessingPipeline {
    void addStage(PreProcessorBase* processor) {
        if (!m_stages.empty()) {
            connect(m_stages.back(), &PreProcessorBase::frameProcessed,
                    processor, &PreProcessorBase::onFrameReceived);
        }
        m_stages.append(processor);
    }
};
```

### Parallel Pipeline
```cpp
class ParallelPipeline : public PreProcessingPipeline {
    void process(const QImage& input) {
        // Split processing across multiple paths
        for (auto* branch : m_branches) {
            QtConcurrent::run([branch, input]() {
                branch->process(input);
            });
        }
    }
    
    void mergeBranches(const QList<cv::Mat>& results) {
        cv::Mat merged = combineResults(results);
        emit frameProcessed(MatToQImage(merged));
    }
};
```

### Conditional Pipeline
```cpp
class ConditionalPipeline : public PreProcessingPipeline {
    void process(const QImage& input) {
        if (evaluateCondition(input)) {
            m_pathA->process(input);
        } else {
            m_pathB->process(input);
        }
    }
    
protected:
    virtual bool evaluateCondition(const QImage& input) = 0;
};
```

## Buffer Management

### Ring Buffer Implementation
```cpp
template<typename T>
class RingBuffer {
    explicit RingBuffer(size_t capacity)
        : m_capacity(capacity)
        , m_buffer(capacity)
        , m_head(0)
        , m_tail(0)
        , m_size(0) {}
    
    bool push(T&& item) {
        QMutexLocker lock(&m_mutex);
        if (m_size == m_capacity) {
            return false; // Buffer full
        }
        
        m_buffer[m_tail] = std::move(item);
        m_tail = (m_tail + 1) % m_capacity;
        ++m_size;
        m_notEmpty.wakeOne();
        return true;
    }
    
    T pop() {
        QMutexLocker lock(&m_mutex);
        while (m_size == 0) {
            m_notEmpty.wait(&m_mutex);
        }
        
        T item = std::move(m_buffer[m_head]);
        m_head = (m_head + 1) % m_capacity;
        --m_size;
        return item;
    }
    
private:
    std::vector<T> m_buffer;
    size_t m_capacity, m_head, m_tail, m_size;
    QMutex m_mutex;
    QWaitCondition m_notEmpty;
};
```

### Memory Pool
```cpp
class MemoryPool {
    cv::Mat acquire(int rows, int cols, int type) {
        QMutexLocker lock(&m_mutex);
        
        // Try to reuse existing buffer
        for (auto it = m_available.begin(); it != m_available.end(); ++it) {
            if (it->rows >= rows && it->cols >= cols && it->type() == type) {
                cv::Mat mat = *it;
                m_available.erase(it);
                m_inUse.insert(mat.data);
                return mat(cv::Rect(0, 0, cols, rows));
            }
        }
        
        // Allocate new buffer
        cv::Mat mat(rows, cols, type);
        m_inUse.insert(mat.data);
        return mat;
    }
    
    void release(const cv::Mat& mat) {
        QMutexLocker lock(&m_mutex);
        if (m_inUse.erase(mat.data)) {
            m_available.push_back(mat);
        }
    }
    
private:
    std::list<cv::Mat> m_available;
    std::unordered_set<void*> m_inUse;
    QMutex m_mutex;
};
```

## Flow Control

### Backpressure Handling
```cpp
class BackpressureManager {
    enum class Strategy {
        DropOldest,    // Drop oldest frames when buffer full
        DropNewest,    // Drop new frames when buffer full
        Block,         // Block until space available
        Throttle       // Slow down input rate
    };
    
    void handleBackpressure(Strategy strategy) {
        switch (strategy) {
        case Strategy::DropOldest:
            if (m_queue.size() >= m_maxSize) {
                m_queue.pop_front();
                ++m_droppedFrames;
            }
            break;
            
        case Strategy::Throttle:
            if (m_queue.size() > m_highWatermark) {
                emit requestSlowdown();
            }
            break;
        }
    }
};
```

### Dynamic Load Balancing
```cpp
class LoadBalancer {
    PreProcessorBase* selectProcessor() {
        // Select processor with lowest load
        PreProcessorBase* selected = nullptr;
        double minLoad = std::numeric_limits<double>::max();
        
        for (auto* processor : m_processors) {
            double load = processor->getCurrentLoad();
            if (load < minLoad) {
                minLoad = load;
                selected = processor;
            }
        }
        
        return selected;
    }
    
    void distributeWork(const QImage& frame) {
        PreProcessorBase* processor = selectProcessor();
        if (processor) {
            processor->process(frame);
        }
    }
};
```

## Pipeline Configuration

### YAML Configuration
```yaml
pipeline:
  name: "EdgeDetectionPipeline"
  stages:
    - type: "GaussianBlur"
      config:
        kernelSize: 5
        sigma: 1.0
        useGPU: true
        
    - type: "CannyEdge"
      config:
        lowThreshold: 50
        highThreshold: 150
        
    - type: "Morphology"
      config:
        operation: "close"
        kernelSize: 3
        
  flow_control:
    strategy: "DropOldest"
    bufferSize: 100
    
  performance:
    maxThreads: 4
    gpuEnabled: true
    batchSize: 10
```

### Runtime Configuration
```cpp
class PipelineConfigurator {
    void configurePipeline(PreProcessingPipeline* pipeline, const QVariantMap& config) {
        // Configure stages
        for (const auto& stageConfig : config["stages"].toList()) {
            auto stage = createStage(stageConfig.toMap());
            pipeline->addStage(stage);
        }
        
        // Configure flow control
        auto flowConfig = config["flow_control"].toMap();
        pipeline->setBackpressureStrategy(
            stringToStrategy(flowConfig["strategy"].toString())
        );
        pipeline->setBufferSize(flowConfig["bufferSize"].toInt());
        
        // Configure performance
        auto perfConfig = config["performance"].toMap();
        pipeline->setMaxThreads(perfConfig["maxThreads"].toInt());
        pipeline->setGPUEnabled(perfConfig["gpuEnabled"].toBool());
    }
};
```

## Performance Optimization

### Batch Processing
```cpp
class BatchProcessor {
    void processBatch(const std::vector<cv::Mat>& batch) {
        if (m_useGPU && batch.size() >= m_minBatchSize) {
            // Process batch on GPU
            cv::cuda::GpuMat gpuBatch;
            for (const auto& frame : batch) {
                cv::cuda::GpuMat gpuFrame;
                gpuFrame.upload(frame);
                
                // Apply GPU processing
                cv::cuda::bilateralFilter(gpuFrame, gpuResult, -1, 50, 50);
                
                cv::Mat result;
                gpuResult.download(result);
                emit frameProcessed(result);
            }
        } else {
            // Process individually on CPU
            for (const auto& frame : batch) {
                cv::Mat result;
                cv::bilateralFilter(frame, result, -1, 50, 50);
                emit frameProcessed(result);
            }
        }
    }
};
```

### Pipeline Metrics
```cpp
struct PipelineMetrics {
    double averageLatency;      // ms
    double throughput;           // frames/sec
    size_t droppedFrames;
    size_t processedFrames;
    double cpuUsage;            // percentage
    double gpuUsage;            // percentage
    size_t memoryUsage;         // bytes
    
    QMap<QString, StageMetrics> stageMetrics;
};

class MetricsCollector {
    PipelineMetrics collect() const {
        PipelineMetrics metrics;
        
        // Calculate throughput
        auto duration = m_endTime - m_startTime;
        metrics.throughput = m_processedFrames / (duration / 1000.0);
        
        // Calculate average latency
        metrics.averageLatency = m_totalLatency / m_processedFrames;
        
        // Collect per-stage metrics
        for (const auto& stage : m_stages) {
            metrics.stageMetrics[stage->name()] = stage->getMetrics();
        }
        
        return metrics;
    }
};
```