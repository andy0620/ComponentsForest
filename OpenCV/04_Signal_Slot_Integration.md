# Signal/Slot Integration

## Overview

This document details the Signal/Slot integration patterns for the PreProcessor architecture, ensuring seamless communication with camera components, UI elements, and other processing stages while maintaining complete decoupling.

## Core Signal/Slot Architecture

### Base Signals and Slots

```cpp
class PreProcessorBase : public BaseComponent {
    Q_OBJECT
    
signals:
    // Output signals
    void frameProcessed(const QImage& processedFrame, const FrameMetadata& metadata);
    void batchProcessed(const QList<QImage>& frames, const QList<FrameMetadata>& metadata);
    void processingError(ProcessingError error, const QString& details);
    void processingStatistics(const ProcessingStats& stats);
    
    // Pipeline control signals
    void processingStarted();
    void processingPaused();
    void processingStopped();
    
    // Performance monitoring
    void performanceUpdate(const PerformanceMetrics& metrics);
    void bufferStatus(int used, int total);
    
public slots:
    // Input slots
    void onFrameReceived(const QImage& frame, const FrameMetadata& metadata);
    void onBatchReceived(const QList<QImage>& frames);
    
    // Control slots
    void startProcessing();
    void pauseProcessing();
    void stopProcessing();
    void flushBuffers();
    
    // Configuration slots
    void updateConfiguration(const QVariantMap& config);
    void setProcessingMode(ProcessingMode mode);
    void enableGPU(bool enable);
};
```

## Connection Patterns

### 1. Camera to PreProcessor

```cpp
class CameraPreProcessorConnector {
    void connectCameraToPreProcessor(
        CameraComponent* camera,
        PreProcessorBase* preprocessor) {
        
        // Basic frame connection
        connect(camera, &CameraComponent::frameReady,
                preprocessor, &PreProcessorBase::onFrameReceived,
                Qt::QueuedConnection);  // Cross-thread safe
        
        // Error propagation
        connect(camera, &CameraComponent::errorOccurred,
                preprocessor, [preprocessor](const QString& error) {
                    preprocessor->handleUpstreamError(error);
                });
        
        // State synchronization
        connect(camera, &CameraComponent::stateChanged,
                preprocessor, [preprocessor](ComponentState state) {
                    if (state == ComponentState::Stopped) {
                        preprocessor->pauseProcessing();
                    } else if (state == ComponentState::Running) {
                        preprocessor->startProcessing();
                    }
                });
        
        // Metadata forwarding
        connect(camera, &CameraComponent::metadataUpdated,
                preprocessor, &PreProcessorBase::updateMetadata);
    }
};
```

### 2. PreProcessor Chain

```cpp
class PreProcessorChain {
    void chainPreProcessors(const QList<PreProcessorBase*>& processors) {
        for (int i = 0; i < processors.size() - 1; ++i) {
            // Connect output to input of next processor
            connect(processors[i], &PreProcessorBase::frameProcessed,
                    processors[i + 1], &PreProcessorBase::onFrameReceived,
                    Qt::DirectConnection);  // Same thread for efficiency
            
            // Error propagation through chain
            connect(processors[i], &PreProcessorBase::processingError,
                    processors[i + 1], [next = processors[i + 1]](
                        ProcessingError error, const QString& details) {
                        next->handleChainError(error, details);
                    });
            
            // Backpressure signaling
            connect(processors[i + 1], &PreProcessorBase::bufferFull,
                    processors[i], &PreProcessorBase::pauseProcessing);
            
            connect(processors[i + 1], &PreProcessorBase::bufferAvailable,
                    processors[i], &PreProcessorBase::resumeProcessing);
        }
    }
};
```

### 3. PreProcessor to UI

```cpp
class PreProcessorUIConnector {
    void connectToUI(PreProcessorBase* preprocessor, QWidget* uiWidget) {
        // Image display with rate limiting
        auto* rateLimiter = new SignalRateLimiter(30, preprocessor);  // 30 FPS max
        
        connect(preprocessor, &PreProcessorBase::frameProcessed,
                rateLimiter, &SignalRateLimiter::handleFrame);
        
        connect(rateLimiter, &SignalRateLimiter::frameReady,
                uiWidget, [uiWidget](const QImage& frame) {
                    QMetaObject::invokeMethod(uiWidget, [uiWidget, frame]() {
                        uiWidget->updateDisplay(frame);
                    }, Qt::QueuedConnection);
                });
        
        // Statistics display
        connect(preprocessor, &PreProcessorBase::processingStatistics,
                uiWidget, [uiWidget](const ProcessingStats& stats) {
                    QMetaObject::invokeMethod(uiWidget, [uiWidget, stats]() {
                        uiWidget->updateStatistics(stats);
                    }, Qt::QueuedConnection);
                });
        
        // Error notifications
        connect(preprocessor, &PreProcessorBase::processingError,
                uiWidget, [uiWidget](ProcessingError error, const QString& details) {
                    QMetaObject::invokeMethod(uiWidget, [uiWidget, error, details]() {
                        uiWidget->showError(error, details);
                    }, Qt::QueuedConnection);
                });
    }
};
```

## Advanced Connection Patterns

### 1. Fan-Out Pattern

```cpp
class FanOutProcessor : public PreProcessorBase {
    void addOutput(QObject* receiver, const char* slot) {
        connect(this, SIGNAL(frameProcessed(QImage,FrameMetadata)),
                receiver, slot, Qt::QueuedConnection);
        
        m_outputs.append(receiver);
    }
    
    void removeOutput(QObject* receiver) {
        disconnect(this, SIGNAL(frameProcessed(QImage,FrameMetadata)),
                   receiver, nullptr);
        
        m_outputs.removeAll(receiver);
    }
    
protected:
    void distributeFrame(const QImage& frame, const FrameMetadata& metadata) {
        // Emit to all connected receivers
        emit frameProcessed(frame, metadata);
        
        // Track distribution statistics
        m_distributionCount++;
        m_lastDistributionTime = QDateTime::currentMSecsSinceEpoch();
    }
    
private:
    QList<QObject*> m_outputs;
    std::atomic<uint64_t> m_distributionCount{0};
    std::atomic<qint64> m_lastDistributionTime{0};
};
```

### 2. Aggregation Pattern

```cpp
class AggregatingProcessor : public PreProcessorBase {
    Q_OBJECT
    
public:
    void addInput(PreProcessorBase* input) {
        connect(input, &PreProcessorBase::frameProcessed,
                this, &AggregatingProcessor::onInputFrameReceived);
        
        m_inputs[input] = InputState();
    }
    
private slots:
    void onInputFrameReceived(const QImage& frame, const FrameMetadata& metadata) {
        auto* sender = qobject_cast<PreProcessorBase*>(QObject::sender());
        if (!sender) return;
        
        // Store frame from this input
        m_inputs[sender].lastFrame = frame;
        m_inputs[sender].lastMetadata = metadata;
        m_inputs[sender].hasNewData = true;
        
        // Check if all inputs have new data
        if (allInputsReady()) {
            aggregateAndProcess();
        }
    }
    
private:
    struct InputState {
        QImage lastFrame;
        FrameMetadata lastMetadata;
        bool hasNewData = false;
    };
    
    bool allInputsReady() const {
        return std::all_of(m_inputs.begin(), m_inputs.end(),
            [](const auto& pair) { return pair.second.hasNewData; });
    }
    
    void aggregateAndProcess() {
        QList<QImage> frames;
        QList<FrameMetadata> metadata;
        
        for (auto& [input, state] : m_inputs) {
            frames.append(state.lastFrame);
            metadata.append(state.lastMetadata);
            state.hasNewData = false;
        }
        
        // Process aggregated frames
        QImage result = performAggregation(frames);
        emit frameProcessed(result, metadata.first());
    }
    
    QMap<PreProcessorBase*, InputState> m_inputs;
};
```

### 3. Conditional Routing

```cpp
class ConditionalRouter : public QObject {
    Q_OBJECT
    
public:
    void addRoute(std::function<bool(const QImage&)> condition,
                  PreProcessorBase* target) {
        m_routes.append({condition, target});
    }
    
public slots:
    void routeFrame(const QImage& frame, const FrameMetadata& metadata) {
        for (const auto& route : m_routes) {
            if (route.condition(frame)) {
                QMetaObject::invokeMethod(
                    route.target, "onFrameReceived",
                    Qt::QueuedConnection,
                    Q_ARG(QImage, frame),
                    Q_ARG(FrameMetadata, metadata)
                );
                return;  // Route to first matching condition
            }
        }
        
        // Default route if no conditions match
        if (m_defaultTarget) {
            QMetaObject::invokeMethod(
                m_defaultTarget, "onFrameReceived",
                Qt::QueuedConnection,
                Q_ARG(QImage, frame),
                Q_ARG(FrameMetadata, metadata)
            );
        }
    }
    
private:
    struct Route {
        std::function<bool(const QImage&)> condition;
        PreProcessorBase* target;
    };
    
    QList<Route> m_routes;
    PreProcessorBase* m_defaultTarget = nullptr;
};
```

## Thread-Safe Signal/Slot Patterns

### 1. Cross-Thread Communication

```cpp
class ThreadSafeProcessor : public PreProcessorBase {
protected:
    void emitThreadSafe(const QImage& frame, const FrameMetadata& metadata) {
        // Ensure we're in the correct thread
        if (QThread::currentThread() != thread()) {
            QMetaObject::invokeMethod(this, [this, frame, metadata]() {
                emit frameProcessed(frame, metadata);
            }, Qt::QueuedConnection);
        } else {
            emit frameProcessed(frame, metadata);
        }
    }
    
    void processInWorkerThread(const QImage& frame) {
        QtConcurrent::run([this, frame]() {
            // Heavy processing in thread pool
            cv::Mat processed = performHeavyProcessing(frame);
            
            // Emit result in object's thread
            QImage result = matToQImage(processed);
            emitThreadSafe(result, FrameMetadata());
        });
    }
};
```

### 2. Signal Buffering

```cpp
class BufferedSignalEmitter : public QObject {
    Q_OBJECT
    
public:
    BufferedSignalEmitter(int bufferSize = 100)
        : m_bufferSize(bufferSize) {
        m_flushTimer = new QTimer(this);
        m_flushTimer->setInterval(100);  // Flush every 100ms
        connect(m_flushTimer, &QTimer::timeout,
                this, &BufferedSignalEmitter::flush);
        m_flushTimer->start();
    }
    
    void bufferFrame(const QImage& frame, const FrameMetadata& metadata) {
        QMutexLocker lock(&m_mutex);
        
        m_buffer.append({frame, metadata});
        
        if (m_buffer.size() >= m_bufferSize) {
            flush();
        }
    }
    
signals:
    void batchReady(const QList<QImage>& frames,
                    const QList<FrameMetadata>& metadata);
    
private slots:
    void flush() {
        QMutexLocker lock(&m_mutex);
        
        if (m_buffer.isEmpty()) return;
        
        QList<QImage> frames;
        QList<FrameMetadata> metadata;
        
        for (const auto& item : m_buffer) {
            frames.append(item.frame);
            metadata.append(item.metadata);
        }
        
        m_buffer.clear();
        
        emit batchReady(frames, metadata);
    }
    
private:
    struct BufferItem {
        QImage frame;
        FrameMetadata metadata;
    };
    
    QList<BufferItem> m_buffer;
    QMutex m_mutex;
    QTimer* m_flushTimer;
    int m_bufferSize;
};
```

## Dynamic Connection Management

### 1. Connection Registry

```cpp
class ConnectionRegistry {
public:
    void registerConnection(const QString& id,
                           QObject* sender,
                           const char* signal,
                           QObject* receiver,
                           const char* slot,
                           Qt::ConnectionType type = Qt::AutoConnection) {
        QMetaObject::Connection conn = connect(sender, signal, receiver, slot, type);
        m_connections[id] = {conn, sender, receiver};
    }
    
    void unregisterConnection(const QString& id) {
        auto it = m_connections.find(id);
        if (it != m_connections.end()) {
            disconnect(it->second.connection);
            m_connections.erase(it);
        }
    }
    
    void enableConnection(const QString& id) {
        auto it = m_connections.find(id);
        if (it != m_connections.end() && !it->second.enabled) {
            it->second.connection = connect(
                it->second.sender,
                it->second.signal,
                it->second.receiver,
                it->second.slot,
                it->second.type
            );
            it->second.enabled = true;
        }
    }
    
    void disableConnection(const QString& id) {
        auto it = m_connections.find(id);
        if (it != m_connections.end() && it->second.enabled) {
            disconnect(it->second.connection);
            it->second.enabled = false;
        }
    }
    
private:
    struct ConnectionInfo {
        QMetaObject::Connection connection;
        QObject* sender;
        QObject* receiver;
        const char* signal;
        const char* slot;
        Qt::ConnectionType type;
        bool enabled = true;
    };
    
    QMap<QString, ConnectionInfo> m_connections;
};
```

### 2. Signal Spy

```cpp
class SignalSpy : public QObject {
    Q_OBJECT
    
public:
    void spy(QObject* object, const char* signal) {
        connect(object, signal, this, SLOT(onSignalEmitted()));
        m_spiedSignals[object].append(signal);
    }
    
    int count(QObject* object, const char* signal) const {
        auto key = QString("%1::%2").arg(object->objectName()).arg(signal);
        return m_signalCounts.value(key, 0);
    }
    
    QVariantList lastArguments(QObject* object, const char* signal) const {
        auto key = QString("%1::%2").arg(object->objectName()).arg(signal);
        return m_lastArguments.value(key);
    }
    
private slots:
    void onSignalEmitted() {
        QObject* senderObj = sender();
        int signalIndex = senderSignalIndex();
        
        QMetaMethod signal = senderObj->metaObject()->method(signalIndex);
        QString key = QString("%1::%2").arg(senderObj->objectName())
                                       .arg(signal.name());
        
        m_signalCounts[key]++;
        m_lastArguments[key] = captureArguments();
        
        emit signalDetected(senderObj, signal.name());
    }
    
signals:
    void signalDetected(QObject* sender, const QString& signal);
    
private:
    QVariantList captureArguments() {
        // Implementation to capture signal arguments
        return QVariantList();
    }
    
    QMap<QObject*, QStringList> m_spiedSignals;
    QMap<QString, int> m_signalCounts;
    QMap<QString, QVariantList> m_lastArguments;
};
```

## Performance Monitoring

### Signal/Slot Metrics

```cpp
class SignalSlotMetrics : public QObject {
    Q_OBJECT
    
public:
    void monitorConnection(QObject* sender, const char* signal,
                          QObject* receiver, const char* slot) {
        QString key = QString("%1::%2 -> %3::%4")
            .arg(sender->objectName()).arg(signal)
            .arg(receiver->objectName()).arg(slot);
        
        // Intercept signal
        connect(sender, signal, this, [this, key]() {
            recordSignalEmission(key);
        });
    }
    
    struct ConnectionMetrics {
        uint64_t totalEmissions = 0;
        double averageDeliveryTime = 0.0;  // microseconds
        double maxDeliveryTime = 0.0;
        double minDeliveryTime = std::numeric_limits<double>::max();
        QDateTime lastEmission;
    };
    
    ConnectionMetrics getMetrics(const QString& connectionKey) const {
        return m_metrics.value(connectionKey);
    }
    
    void printReport() const {
        qDebug() << "Signal/Slot Performance Report:";
        for (auto it = m_metrics.begin(); it != m_metrics.end(); ++it) {
            qDebug() << it.key() << ":";
            qDebug() << "  Emissions:" << it->totalEmissions;
            qDebug() << "  Avg Time:" << it->averageDeliveryTime << "µs";
            qDebug() << "  Max Time:" << it->maxDeliveryTime << "µs";
        }
    }
    
private:
    void recordSignalEmission(const QString& key) {
        auto& metrics = m_metrics[key];
        metrics.totalEmissions++;
        metrics.lastEmission = QDateTime::currentDateTime();
        
        // Record timing if instrumented
        // This would require more complex instrumentation
    }
    
    QMap<QString, ConnectionMetrics> m_metrics;
};
```