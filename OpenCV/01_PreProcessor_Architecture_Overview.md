# PreProcessor Architecture Overview

## Executive Summary

The PreProcessor architecture provides a high-performance, thread-safe base class for image preprocessing components in the ComponentsForest ecosystem. Built on OpenCV and integrated with Qt's Signal/Slot mechanism, it enables efficient image processing pipelines while maintaining complete decoupling from acquisition components.

## Core Architecture Principles

### 1. Inheritance Hierarchy
```
QObject → BaseComponent → PreProcessorBase → Specific Preprocessors
                              ↓
                    (BlurPreProcessor, EdgePreProcessor, etc.)
```

### 2. Design Patterns

#### Template Method Pattern
PreProcessorBase defines the processing skeleton, child classes implement specific algorithms:
```cpp
class PreProcessorBase {
protected:
    virtual cv::Mat processImplementation(const cv::Mat& input) = 0;
public:
    void process(const QImage& image) {
        // Pre-processing setup
        cv::Mat mat = convertQImageToMat(image);
        
        // Child-specific processing
        cv::Mat result = processImplementation(mat);
        
        // Post-processing and emission
        emit preprocessed(convertMatToQImage(result));
    }
};
```

#### Chain of Responsibility
Multiple preprocessors can be chained:
```cpp
CameraComponent → PreProcessor1 → PreProcessor2 → Display/Analysis
```

#### Strategy Pattern
Processing algorithms are encapsulated and interchangeable at runtime.

## Key Components

### 1. PreProcessorBase Class
- **Purpose**: Abstract base class for all preprocessing components
- **Responsibilities**:
  - Image format conversion (QImage ↔ cv::Mat)
  - Thread management and safety
  - Performance monitoring
  - Signal/Slot integration
  - Resource management

### 2. PreProcessingPipeline Class
- **Purpose**: Manages chains of preprocessors
- **Responsibilities**:
  - Pipeline construction and configuration
  - Buffer management between stages
  - Parallel processing coordination
  - Performance optimization

### 3. ProcessingContext
- **Purpose**: Shared context for processing operations
- **Contains**:
  - GPU/CPU device selection
  - Memory pools
  - Performance metrics
  - Configuration parameters

## Integration Points

### Camera Component Integration
```cpp
// Automatic connection in PreProcessorBase
connect(cameraComponent, &CameraComponent::frameReady,
        this, &PreProcessorBase::onFrameReceived);
```

### UI Integration
```cpp
// Preprocessed images emit to UI
connect(preprocessor, &PreProcessorBase::frameProcessed,
        displayWidget, &DisplayWidget::updateImage);
```

## Memory Management Strategy

### Zero-Copy Optimization
- Use cv::Mat's reference counting for efficient data sharing
- Implement copy-on-write for modified data
- Maintain thread-local buffer pools

### GPU Memory Management
- Pin host memory for faster GPU transfers
- Implement GPU memory pool for CUDA operations
- Automatic fallback to CPU on GPU failure

## Thread Architecture

### Processing Thread
Each preprocessor runs in its own QThread:
```cpp
PreProcessorBase::PreProcessorBase() {
    m_processingThread = new QThread(this);
    moveToThread(m_processingThread);
    m_processingThread->start();
}
```

### Thread Safety Guarantees
- All public methods are thread-safe
- Internal state protected by mutexes
- Lock-free queues for high-frequency operations

## Configuration System

### Dynamic Configuration
```cpp
QVariantMap config;
config["algorithm"] = "gaussian";
config["kernelSize"] = 5;
config["sigma"] = 1.0;
config["useGPU"] = true;
preprocessor->configure(config);
```

### Runtime Parameter Adjustment
Parameters can be modified without stopping the pipeline:
```cpp
preprocessor->setParameter("sigma", 2.0);
```

## Performance Considerations

### Benchmarking Framework
Built-in performance monitoring:
- Frame processing time
- Memory usage
- GPU utilization
- Queue depths

### Optimization Strategies
1. **Batch Processing**: Process multiple frames together for GPU efficiency
2. **Lazy Evaluation**: Defer processing until results are needed
3. **Cache Optimization**: Align data structures for CPU cache efficiency
4. **SIMD Utilization**: Leverage OpenCV's optimized kernels

## Error Handling

### Graceful Degradation
- Automatic GPU → CPU fallback
- Skip frame on processing timeout
- Maintain pipeline continuity on component failure

### Error Recovery
```cpp
enum class ProcessingError {
    None,
    InvalidInput,
    ProcessingTimeout,
    GPUError,
    OutOfMemory
};

signal: void errorOccurred(ProcessingError error, QString details);
```

## Extensibility

### Creating New Preprocessors
Child classes only need to:
1. Inherit from PreProcessorBase
2. Implement `processImplementation()`
3. Optionally override `validateInput()` and `configureImplementation()`

### Plugin Architecture Support
Future support for dynamic loading of preprocessor plugins:
```cpp
class PreProcessorPlugin : public QObject, public PreProcessorInterface {
    Q_PLUGIN_METADATA(IID "org.componentsforest.PreProcessorInterface")
    Q_INTERFACES(PreProcessorInterface)
};
```