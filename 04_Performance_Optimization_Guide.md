# Qt高效能優化指南：工業AOI設備的極限效能追求

## 目錄
1. [效能瓶頸分析方法](#1-效能瓶頸分析方法)
2. [多執行緒架構選擇策略](#2-多執行緒架構選擇策略)
3. [執行緒池設計與管理](#3-執行緒池設計與管理)
4. [Lock-free程式設計實踐](#4-lock-free程式設計實踐)
5. [影像處理並行化策略](#5-影像處理並行化策略)
6. [GPU加速整合方案](#6-gpu加速整合方案)
7. [記憶體優化技術](#7-記憶體優化技術)
8. [Zero-copy技術實現](#8-zero-copy技術實現)
9. [Windows平台特定優化](#9-windows平台特定優化)
10. [即時性保證機制](#10-即時性保證機制)
11. [效能測試基準與工具](#11-效能測試基準與工具)

---

## 1. 效能瓶頸分析方法

### 1.1 效能分析工具集

```cpp
// PerformanceProfiler.h
#pragma once
#include <QElapsedTimer>
#include <QHash>
#include <QMutex>
#include <chrono>
#include <atomic>

class PerformanceProfiler {
public:
    static PerformanceProfiler& instance() {
        static PerformanceProfiler instance;
        return instance;
    }
    
    class ScopedTimer {
    public:
        ScopedTimer(const QString& name) 
            : m_name(name), m_start(std::chrono::high_resolution_clock::now()) {}
        
        ~ScopedTimer() {
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - m_start);
            PerformanceProfiler::instance().addMeasurement(m_name, duration.count());
        }
        
    private:
        QString m_name;
        std::chrono::high_resolution_clock::time_point m_start;
    };
    
    struct Metrics {
        std::atomic<qint64> totalTime{0};
        std::atomic<qint64> count{0};
        std::atomic<qint64> minTime{LLONG_MAX};
        std::atomic<qint64> maxTime{0};
        
        double averageTime() const {
            return count > 0 ? static_cast<double>(totalTime) / count : 0;
        }
        
        double fps() const {
            return count > 0 && totalTime > 0 ? 
                (count * 1000000.0) / totalTime : 0;
        }
    };
    
    void addMeasurement(const QString& name, qint64 microseconds) {
        QMutexLocker locker(&m_mutex);
        auto& metric = m_metrics[name];
        metric.totalTime += microseconds;
        metric.count++;
        metric.minTime = std::min(metric.minTime.load(), microseconds);
        metric.maxTime = std::max(metric.maxTime.load(), microseconds);
    }
    
    QString generateReport() const {
        QMutexLocker locker(&m_mutex);
        QString report = "Performance Report:\n";
        report += QString("-").repeated(80) + "\n";
        report += QString("%-30s %10s %10s %10s %10s %10s\n")
            .arg("Function")
            .arg("Avg(µs)")
            .arg("Min(µs)")
            .arg("Max(µs)")
            .arg("Count")
            .arg("FPS");
        report += QString("-").repeated(80) + "\n";
        
        for (auto it = m_metrics.begin(); it != m_metrics.end(); ++it) {
            const auto& metric = it.value();
            report += QString("%-30s %10.2f %10lld %10lld %10lld %10.2f\n")
                .arg(it.key())
                .arg(metric.averageTime())
                .arg(metric.minTime.load())
                .arg(metric.maxTime.load())
                .arg(metric.count.load())
                .arg(metric.fps());
        }
        
        return report;
    }
    
    void reset() {
        QMutexLocker locker(&m_mutex);
        m_metrics.clear();
    }
    
private:
    mutable QMutex m_mutex;
    QHash<QString, Metrics> m_metrics;
};

#define PROFILE_SCOPE(name) PerformanceProfiler::ScopedTimer _timer(name)
```

### 1.2 記憶體分析工具

```cpp
// MemoryProfiler.h
#pragma once
#include <QObject>
#include <QTimer>
#include <atomic>
#include <memory>

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#endif

class MemoryProfiler : public QObject {
    Q_OBJECT
    
public:
    struct MemoryStats {
        size_t workingSetSize = 0;      // 工作集大小
        size_t peakWorkingSetSize = 0;  // 峰值工作集
        size_t privateBytes = 0;        // 私有位元組
        size_t virtualMemory = 0;       // 虛擬記憶體
        size_t allocatedObjects = 0;    // 已分配物件數
        
        QString toString() const {
            return QString("Memory Stats:\n"
                          "  Working Set: %1 MB\n"
                          "  Peak Working Set: %2 MB\n"
                          "  Private Bytes: %3 MB\n"
                          "  Virtual Memory: %4 MB\n"
                          "  Allocated Objects: %5")
                .arg(workingSetSize / (1024.0 * 1024.0), 0, 'f', 2)
                .arg(peakWorkingSetSize / (1024.0 * 1024.0), 0, 'f', 2)
                .arg(privateBytes / (1024.0 * 1024.0), 0, 'f', 2)
                .arg(virtualMemory / (1024.0 * 1024.0), 0, 'f', 2)
                .arg(allocatedObjects);
        }
    };
    
    static MemoryProfiler& instance() {
        static MemoryProfiler instance;
        return instance;
    }
    
    void startMonitoring(int intervalMs = 1000) {
        m_timer->start(intervalMs);
    }
    
    void stopMonitoring() {
        m_timer->stop();
    }
    
    MemoryStats getCurrentStats() const {
        MemoryStats stats;
        
#ifdef _WIN32
        PROCESS_MEMORY_COUNTERS_EX pmc;
        if (GetProcessMemoryInfo(GetCurrentProcess(), 
                                (PROCESS_MEMORY_COUNTERS*)&pmc, 
                                sizeof(pmc))) {
            stats.workingSetSize = pmc.WorkingSetSize;
            stats.peakWorkingSetSize = pmc.PeakWorkingSetSize;
            stats.privateBytes = pmc.PrivateUsage;
            stats.virtualMemory = pmc.PagefileUsage;
        }
#endif
        
        stats.allocatedObjects = m_allocatedObjects.load();
        return stats;
    }
    
    void trackAllocation() { m_allocatedObjects++; }
    void trackDeallocation() { m_allocatedObjects--; }
    
signals:
    void memoryUsageUpdated(const MemoryStats& stats);
    
private:
    MemoryProfiler() : m_timer(new QTimer(this)) {
        connect(m_timer, &QTimer::timeout, [this]() {
            emit memoryUsageUpdated(getCurrentStats());
        });
    }
    
    QTimer* m_timer;
    std::atomic<size_t> m_allocatedObjects{0};
};

// 自訂記憶體追蹤分配器
template<typename T>
class TrackingAllocator {
public:
    using value_type = T;
    
    TrackingAllocator() = default;
    
    template<typename U>
    TrackingAllocator(const TrackingAllocator<U>&) {}
    
    T* allocate(std::size_t n) {
        MemoryProfiler::instance().trackAllocation();
        return static_cast<T*>(::operator new(n * sizeof(T)));
    }
    
    void deallocate(T* p, std::size_t) {
        MemoryProfiler::instance().trackDeallocation();
        ::operator delete(p);
    }
};
```

---

## 2. 多執行緒架構選擇策略

### 2.1 QThread vs std::thread vs QtConcurrent 比較

```cpp
// ThreadingComparison.h
#pragma once
#include <QThread>
#include <QtConcurrent>
#include <thread>
#include <future>

class ThreadingBenchmark {
public:
    // QThread實現 - 適合需要Qt事件循環的場景
    class QtWorkerThread : public QThread {
    protected:
        void run() override {
            // 建立事件循環
            exec();
        }
    };
    
    // std::thread實現 - 適合純計算密集型任務
    class StdThreadWorker {
    public:
        void start() {
            m_thread = std::thread([this]() {
                while (!m_stop) {
                    processTask();
                }
            });
        }
        
        void stop() {
            m_stop = true;
            if (m_thread.joinable()) {
                m_thread.join();
            }
        }
        
    private:
        std::thread m_thread;
        std::atomic<bool> m_stop{false};
        
        void processTask() {
            // 處理任務
        }
    };
    
    // QtConcurrent實現 - 適合並行數據處理
    template<typename Container, typename Func>
    static auto parallelProcess(Container& data, Func func) {
        return QtConcurrent::map(data, func);
    }
    
    // 效能比較測試
    static void runBenchmark() {
        const int dataSize = 1000000;
        std::vector<int> data(dataSize);
        std::iota(data.begin(), data.end(), 0);
        
        auto complexCalculation = [](int& value) {
            value = std::pow(value, 2) + std::sqrt(value);
        };
        
        // QThread測試
        {
            PROFILE_SCOPE("QThread Processing");
            // QThread實現
        }
        
        // std::thread測試
        {
            PROFILE_SCOPE("std::thread Processing");
            const int numThreads = std::thread::hardware_concurrency();
            std::vector<std::thread> threads;
            int chunkSize = dataSize / numThreads;
            
            for (int i = 0; i < numThreads; ++i) {
                int start = i * chunkSize;
                int end = (i == numThreads - 1) ? dataSize : start + chunkSize;
                
                threads.emplace_back([&data, start, end, complexCalculation]() {
                    for (int j = start; j < end; ++j) {
                        complexCalculation(data[j]);
                    }
                });
            }
            
            for (auto& t : threads) {
                t.join();
            }
        }
        
        // QtConcurrent測試
        {
            PROFILE_SCOPE("QtConcurrent Processing");
            QtConcurrent::blockingMap(data, complexCalculation);
        }
    }
};
```

### 2.2 moveToThread最佳實踐

```cpp
// OptimizedWorker.h
#pragma once
#include <QObject>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <queue>

class OptimizedWorker : public QObject {
    Q_OBJECT
    
public:
    explicit OptimizedWorker(QObject* parent = nullptr) 
        : QObject(parent), m_abort(false) {}
    
    ~OptimizedWorker() {
        requestInterruption();
        m_condition.wakeAll();
    }
    
    void requestInterruption() {
        QMutexLocker locker(&m_mutex);
        m_abort = true;
    }
    
public slots:
    void process() {
        while (!m_abort) {
            Task task;
            {
                QMutexLocker locker(&m_mutex);
                while (m_taskQueue.empty() && !m_abort) {
                    m_condition.wait(&m_mutex);
                }
                
                if (m_abort) break;
                
                task = m_taskQueue.front();
                m_taskQueue.pop();
            }
            
            // 處理任務
            processTask(task);
            
            emit resultReady(task.id);
        }
        
        emit finished();
    }
    
    void addTask(const Task& task) {
        QMutexLocker locker(&m_mutex);
        m_taskQueue.push(task);
        m_condition.wakeOne();
    }
    
signals:
    void resultReady(int taskId);
    void finished();
    
private:
    struct Task {
        int id;
        std::function<void()> work;
    };
    
    void processTask(const Task& task) {
        if (task.work) {
            task.work();
        }
    }
    
    mutable QMutex m_mutex;
    QWaitCondition m_condition;
    std::queue<Task> m_taskQueue;
    bool m_abort;
};

// 使用範例
class WorkerController : public QObject {
    Q_OBJECT
    
public:
    WorkerController() {
        m_worker = new OptimizedWorker;
        m_worker->moveToThread(&m_workerThread);
        
        // 重要：確保正確的連接
        connect(&m_workerThread, &QThread::started, 
                m_worker, &OptimizedWorker::process);
        connect(m_worker, &OptimizedWorker::finished, 
                &m_workerThread, &QThread::quit);
        connect(m_worker, &OptimizedWorker::finished, 
                m_worker, &OptimizedWorker::deleteLater);
        connect(&m_workerThread, &QThread::finished, 
                &m_workerThread, &QThread::deleteLater);
        
        m_workerThread.start();
    }
    
    ~WorkerController() {
        m_worker->requestInterruption();
        m_workerThread.quit();
        m_workerThread.wait();
    }
    
private:
    QThread m_workerThread;
    OptimizedWorker* m_worker;
};
```

---

## 3. 執行緒池設計與管理

### 3.1 高效能執行緒池實現

```cpp
// HighPerformanceThreadPool.h
#pragma once
#include <QObject>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <queue>
#include <vector>
#include <functional>
#include <atomic>
#include <memory>

class HighPerformanceThreadPool : public QObject {
    Q_OBJECT
    
public:
    using Task = std::function<void()>;
    using Priority = int;
    
    struct PriorityTask {
        Priority priority;
        Task task;
        std::chrono::steady_clock::time_point submitTime;
        
        bool operator<(const PriorityTask& other) const {
            return priority < other.priority;
        }
    };
    
    explicit HighPerformanceThreadPool(size_t numThreads = 0, QObject* parent = nullptr)
        : QObject(parent), m_stop(false) {
        
        if (numThreads == 0) {
            numThreads = std::thread::hardware_concurrency();
        }
        
        // 創建工作執行緒
        for (size_t i = 0; i < numThreads; ++i) {
            m_workers.emplace_back([this, i]() {
                workerLoop(i);
            });
        }
        
        // 設置執行緒親和性（Windows特定）
#ifdef _WIN32
        for (size_t i = 0; i < m_workers.size(); ++i) {
            DWORD_PTR mask = 1 << (i % std::thread::hardware_concurrency());
            SetThreadAffinityMask(m_workers[i].native_handle(), mask);
        }
#endif
    }
    
    ~HighPerformanceThreadPool() {
        shutdown();
    }
    
    // 提交任務
    template<typename F, typename... Args>
    auto submit(Priority priority, F&& f, Args&&... args) 
        -> std::future<decltype(f(args...))> {
        
        using ReturnType = decltype(f(args...));
        
        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
        
        std::future<ReturnType> result = task->get_future();
        
        {
            QMutexLocker lock(&m_mutex);
            
            if (m_stop) {
                throw std::runtime_error("ThreadPool is stopped");
            }
            
            m_tasks.push({
                priority,
                [task]() { (*task)(); },
                std::chrono::steady_clock::now()
            });
            
            m_taskCount++;
        }
        
        m_condition.wakeOne();
        return result;
    }
    
    // 批量提交任務
    template<typename Container, typename Func>
    void submitBatch(Priority priority, const Container& items, Func func) {
        QMutexLocker lock(&m_mutex);
        
        for (const auto& item : items) {
            m_tasks.push({
                priority,
                [func, item]() { func(item); },
                std::chrono::steady_clock::now()
            });
            m_taskCount++;
        }
        
        m_condition.wakeAll();
    }
    
    // 獲取統計信息
    struct Stats {
        size_t pendingTasks;
        size_t completedTasks;
        size_t activeThreads;
        double averageWaitTime;
        double averageExecutionTime;
    };
    
    Stats getStats() const {
        QMutexLocker lock(&m_mutex);
        
        Stats stats;
        stats.pendingTasks = m_tasks.size();
        stats.completedTasks = m_completedTasks.load();
        stats.activeThreads = m_activeThreads.load();
        
        if (!m_waitTimes.empty()) {
            double totalWait = std::accumulate(m_waitTimes.begin(), 
                                              m_waitTimes.end(), 0.0);
            stats.averageWaitTime = totalWait / m_waitTimes.size();
        }
        
        if (!m_executionTimes.empty()) {
            double totalExec = std::accumulate(m_executionTimes.begin(), 
                                              m_executionTimes.end(), 0.0);
            stats.averageExecutionTime = totalExec / m_executionTimes.size();
        }
        
        return stats;
    }
    
    void shutdown() {
        {
            QMutexLocker lock(&m_mutex);
            m_stop = true;
        }
        
        m_condition.wakeAll();
        
        for (auto& worker : m_workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }
    
signals:
    void taskCompleted(size_t threadId);
    void allTasksCompleted();
    
private:
    void workerLoop(size_t threadId) {
        while (true) {
            PriorityTask task;
            
            {
                QMutexLocker lock(&m_mutex);
                
                while (m_tasks.empty() && !m_stop) {
                    m_condition.wait(&m_mutex);
                }
                
                if (m_stop && m_tasks.empty()) {
                    break;
                }
                
                task = m_tasks.top();
                m_tasks.pop();
                m_taskCount--;
                
                // 記錄等待時間
                auto waitTime = std::chrono::steady_clock::now() - task.submitTime;
                m_waitTimes.push_back(
                    std::chrono::duration<double, std::milli>(waitTime).count()
                );
                
                m_activeThreads++;
            }
            
            // 執行任務
            auto startTime = std::chrono::steady_clock::now();
            task.task();
            auto endTime = std::chrono::steady_clock::now();
            
            {
                QMutexLocker lock(&m_mutex);
                
                // 記錄執行時間
                auto execTime = endTime - startTime;
                m_executionTimes.push_back(
                    std::chrono::duration<double, std::milli>(execTime).count()
                );
                
                m_activeThreads--;
                m_completedTasks++;
                
                // 限制統計數據大小
                if (m_waitTimes.size() > 1000) {
                    m_waitTimes.erase(m_waitTimes.begin());
                }
                if (m_executionTimes.size() > 1000) {
                    m_executionTimes.erase(m_executionTimes.begin());
                }
            }
            
            emit taskCompleted(threadId);
            
            if (m_taskCount == 0) {
                emit allTasksCompleted();
            }
        }
    }
    
    mutable QMutex m_mutex;
    QWaitCondition m_condition;
    std::vector<std::thread> m_workers;
    std::priority_queue<PriorityTask> m_tasks;
    std::atomic<bool> m_stop;
    std::atomic<size_t> m_taskCount{0};
    std::atomic<size_t> m_completedTasks{0};
    std::atomic<size_t> m_activeThreads{0};
    std::vector<double> m_waitTimes;
    std::vector<double> m_executionTimes;
};
```

---

## 4. Lock-free程式設計實踐

### 4.1 Lock-free Queue實現

```cpp
// LockFreeQueue.h
#pragma once
#include <atomic>
#include <memory>

template<typename T>
class LockFreeQueue {
private:
    struct Node {
        std::atomic<T*> data;
        std::atomic<Node*> next;
        
        Node() : data(nullptr), next(nullptr) {}
    };
    
    std::atomic<Node*> head;
    std::atomic<Node*> tail;
    
public:
    LockFreeQueue() {
        Node* dummy = new Node;
        head.store(dummy);
        tail.store(dummy);
    }
    
    ~LockFreeQueue() {
        while (Node* oldHead = head.load()) {
            head.store(oldHead->next);
            delete oldHead;
        }
    }
    
    void enqueue(T item) {
        Node* newNode = new Node;
        T* data = new T(std::move(item));
        newNode->data.store(data);
        
        Node* prevTail = tail.exchange(newNode);
        prevTail->next.store(newNode);
    }
    
    bool dequeue(T& result) {
        Node* head_node = head.load();
        Node* next = head_node->next.load();
        
        if (next == nullptr) {
            return false;
        }
        
        T* data = next->data.exchange(nullptr);
        if (data == nullptr) {
            return false;
        }
        
        head.store(next);
        result = std::move(*data);
        
        delete data;
        delete head_node;
        return true;
    }
    
    bool empty() const {
        Node* head_node = head.load();
        return head_node->next.load() == nullptr;
    }
};

// Lock-free Ring Buffer
template<typename T, size_t Size>
class LockFreeRingBuffer {
private:
    alignas(64) std::atomic<size_t> m_writeIndex{0};
    alignas(64) std::atomic<size_t> m_readIndex{0};
    alignas(64) std::array<T, Size> m_buffer;
    
    static constexpr size_t capacity = Size;
    
public:
    bool push(const T& item) {
        size_t currentWrite = m_writeIndex.load(std::memory_order_relaxed);
        size_t nextWrite = (currentWrite + 1) % capacity;
        
        if (nextWrite == m_readIndex.load(std::memory_order_acquire)) {
            return false; // Buffer full
        }
        
        m_buffer[currentWrite] = item;
        m_writeIndex.store(nextWrite, std::memory_order_release);
        return true;
    }
    
    bool pop(T& item) {
        size_t currentRead = m_readIndex.load(std::memory_order_relaxed);
        
        if (currentRead == m_writeIndex.load(std::memory_order_acquire)) {
            return false; // Buffer empty
        }
        
        item = m_buffer[currentRead];
        m_readIndex.store((currentRead + 1) % capacity, std::memory_order_release);
        return true;
    }
    
    size_t size() const {
        size_t write = m_writeIndex.load(std::memory_order_acquire);
        size_t read = m_readIndex.load(std::memory_order_acquire);
        
        if (write >= read) {
            return write - read;
        } else {
            return capacity - (read - write);
        }
    }
    
    bool empty() const {
        return m_readIndex.load(std::memory_order_acquire) == 
               m_writeIndex.load(std::memory_order_acquire);
    }
};
```

### 4.2 Lock-free Memory Pool

```cpp
// LockFreeMemoryPool.h
#pragma once
#include <atomic>
#include <array>
#include <memory>

template<typename T, size_t PoolSize>
class LockFreeMemoryPool {
private:
    struct Node {
        alignas(T) char storage[sizeof(T)];
        std::atomic<Node*> next;
    };
    
    std::array<Node, PoolSize> m_nodes;
    std::atomic<Node*> m_head;
    std::atomic<size_t> m_allocatedCount{0};
    
public:
    LockFreeMemoryPool() {
        // 初始化free list
        for (size_t i = 0; i < PoolSize - 1; ++i) {
            m_nodes[i].next.store(&m_nodes[i + 1]);
        }
        m_nodes[PoolSize - 1].next.store(nullptr);
        m_head.store(&m_nodes[0]);
    }
    
    template<typename... Args>
    T* allocate(Args&&... args) {
        Node* node = nullptr;
        
        // 嘗試從free list獲取節點
        do {
            node = m_head.load(std::memory_order_acquire);
            if (node == nullptr) {
                return nullptr; // Pool exhausted
            }
        } while (!m_head.compare_exchange_weak(
            node, 
            node->next.load(std::memory_order_relaxed),
            std::memory_order_release,
            std::memory_order_relaxed
        ));
        
        // 在獲取的記憶體上構造物件
        T* obj = new (node->storage) T(std::forward<Args>(args)...);
        m_allocatedCount.fetch_add(1, std::memory_order_relaxed);
        
        return obj;
    }
    
    void deallocate(T* obj) {
        if (obj == nullptr) return;
        
        // 呼叫析構函數
        obj->~T();
        
        // 將節點返回到free list
        Node* node = reinterpret_cast<Node*>(obj);
        Node* head = nullptr;
        
        do {
            head = m_head.load(std::memory_order_relaxed);
            node->next.store(head, std::memory_order_relaxed);
        } while (!m_head.compare_exchange_weak(
            head,
            node,
            std::memory_order_release,
            std::memory_order_relaxed
        ));
        
        m_allocatedCount.fetch_sub(1, std::memory_order_relaxed);
    }
    
    size_t allocated() const {
        return m_allocatedCount.load(std::memory_order_relaxed);
    }
    
    size_t available() const {
        return PoolSize - allocated();
    }
};
```

---

## 5. 影像處理並行化策略

### 5.1 OpenMP整合

```cpp
// ParallelImageProcessor.h
#pragma once
#include <QImage>
#include <opencv2/opencv.hpp>
#include <omp.h>
#include <tbb/parallel_for.h>
#include <tbb/blocked_range2d.h>

class ParallelImageProcessor {
public:
    // OpenMP並行處理
    static void processWithOpenMP(cv::Mat& image, 
                                  std::function<void(cv::Mat&, int, int)> pixelOp) {
        const int rows = image.rows;
        const int cols = image.cols;
        
        #pragma omp parallel for collapse(2) schedule(dynamic, 16)
        for (int y = 0; y < rows; ++y) {
            for (int x = 0; x < cols; ++x) {
                pixelOp(image, x, y);
            }
        }
    }
    
    // TBB並行處理
    static void processWithTBB(cv::Mat& image,
                               std::function<void(cv::Mat&, int, int)> pixelOp) {
        tbb::parallel_for(
            tbb::blocked_range2d<int>(0, image.rows, 32, 0, image.cols, 32),
            [&](const tbb::blocked_range2d<int>& range) {
                for (int y = range.rows().begin(); y < range.rows().end(); ++y) {
                    for (int x = range.cols().begin(); x < range.cols().end(); ++x) {
                        pixelOp(image, x, y);
                    }
                }
            }
        );
    }
    
    // SIMD優化的影像處理
    static void applyGaussianBlurSIMD(const cv::Mat& src, cv::Mat& dst) {
        CV_Assert(src.type() == CV_8UC1);
        dst.create(src.size(), src.type());
        
        const int width = src.cols;
        const int height = src.rows;
        
        // 使用OpenCV的SIMD優化
        #pragma omp parallel for
        for (int y = 1; y < height - 1; ++y) {
            const uchar* prevRow = src.ptr<uchar>(y - 1);
            const uchar* currRow = src.ptr<uchar>(y);
            const uchar* nextRow = src.ptr<uchar>(y + 1);
            uchar* dstRow = dst.ptr<uchar>(y);
            
            int x = 1;
            
            // SIMD處理主體部分
            #ifdef __SSE2__
            for (; x <= width - 17; x += 16) {
                __m128i p00 = _mm_loadu_si128((__m128i*)(prevRow + x - 1));
                __m128i p01 = _mm_loadu_si128((__m128i*)(prevRow + x));
                __m128i p02 = _mm_loadu_si128((__m128i*)(prevRow + x + 1));
                
                __m128i p10 = _mm_loadu_si128((__m128i*)(currRow + x - 1));
                __m128i p11 = _mm_loadu_si128((__m128i*)(currRow + x));
                __m128i p12 = _mm_loadu_si128((__m128i*)(currRow + x + 1));
                
                __m128i p20 = _mm_loadu_si128((__m128i*)(nextRow + x - 1));
                __m128i p21 = _mm_loadu_si128((__m128i*)(nextRow + x));
                __m128i p22 = _mm_loadu_si128((__m128i*)(nextRow + x + 1));
                
                // Gaussian kernel: [1 2 1; 2 4 2; 1 2 1] / 16
                __m128i sum = _mm_add_epi16(
                    _mm_add_epi16(p00, _mm_slli_epi16(p01, 1)),
                    _mm_add_epi16(p02, _mm_slli_epi16(p10, 1))
                );
                sum = _mm_add_epi16(sum, 
                    _mm_add_epi16(_mm_slli_epi16(p11, 2), _mm_slli_epi16(p12, 1))
                );
                sum = _mm_add_epi16(sum,
                    _mm_add_epi16(p20, _mm_add_epi16(_mm_slli_epi16(p21, 1), p22))
                );
                
                __m128i result = _mm_srli_epi16(sum, 4); // Divide by 16
                _mm_storeu_si128((__m128i*)(dstRow + x), result);
            }
            #endif
            
            // 處理剩餘的像素
            for (; x < width - 1; ++x) {
                int sum = prevRow[x-1] + 2*prevRow[x] + prevRow[x+1] +
                         2*currRow[x-1] + 4*currRow[x] + 2*currRow[x+1] +
                         nextRow[x-1] + 2*nextRow[x] + nextRow[x+1];
                dstRow[x] = sum >> 4;
            }
        }
    }
    
    // 分塊處理策略
    template<typename ProcessFunc>
    static void processTiled(cv::Mat& image, int tileSize, ProcessFunc func) {
        const int rows = image.rows;
        const int cols = image.cols;
        
        #pragma omp parallel for collapse(2)
        for (int y = 0; y < rows; y += tileSize) {
            for (int x = 0; x < cols; x += tileSize) {
                int tileWidth = std::min(tileSize, cols - x);
                int tileHeight = std::min(tileSize, rows - y);
                
                cv::Rect roi(x, y, tileWidth, tileHeight);
                cv::Mat tile = image(roi);
                func(tile);
            }
        }
    }
};
```

### 5.2 Pipeline並行處理

```cpp
// ImageProcessingPipeline.h
#pragma once
#include <QObject>
#include <opencv2/opencv.hpp>
#include <tbb/pipeline.h>
#include <queue>
#include <future>

class ImageProcessingPipeline : public QObject {
    Q_OBJECT
    
public:
    struct Frame {
        cv::Mat image;
        int frameNumber;
        std::chrono::steady_clock::time_point timestamp;
    };
    
    class PipelineStage {
    public:
        virtual ~PipelineStage() = default;
        virtual Frame process(Frame frame) = 0;
        virtual std::string getName() const = 0;
    };
    
    // 預處理階段
    class PreprocessingStage : public PipelineStage {
    public:
        Frame process(Frame frame) override {
            PROFILE_SCOPE("Preprocessing");
            
            // 降噪
            cv::GaussianBlur(frame.image, frame.image, cv::Size(5, 5), 1.0);
            
            // 直方圖均衡化
            if (frame.image.channels() == 1) {
                cv::equalizeHist(frame.image, frame.image);
            }
            
            return frame;
        }
        
        std::string getName() const override { return "Preprocessing"; }
    };
    
    // 特徵檢測階段
    class FeatureDetectionStage : public PipelineStage {
    public:
        Frame process(Frame frame) override {
            PROFILE_SCOPE("Feature Detection");
            
            std::vector<cv::KeyPoint> keypoints;
            cv::Ptr<cv::ORB> orb = cv::ORB::create();
            orb->detect(frame.image, keypoints);
            
            // 在frame中儲存關鍵點（實際應用中可能需要更複雜的數據結構）
            cv::drawKeypoints(frame.image, keypoints, frame.image);
            
            return frame;
        }
        
        std::string getName() const override { return "Feature Detection"; }
    };
    
    // 後處理階段
    class PostprocessingStage : public PipelineStage {
    public:
        Frame process(Frame frame) override {
            PROFILE_SCOPE("Postprocessing");
            
            // 銳化
            cv::Mat kernel = (cv::Mat_<float>(3, 3) <<
                0, -1, 0,
                -1, 5, -1,
                0, -1, 0);
            cv::filter2D(frame.image, frame.image, -1, kernel);
            
            return frame;
        }
        
        std::string getName() const override { return "Postprocessing"; }
    };
    
    ImageProcessingPipeline(int maxParallelism = 4) 
        : m_maxParallelism(maxParallelism) {
        
        // 初始化階段
        m_stages.push_back(std::make_unique<PreprocessingStage>());
        m_stages.push_back(std::make_unique<FeatureDetectionStage>());
        m_stages.push_back(std::make_unique<PostprocessingStage>());
    }
    
    // TBB Pipeline實現
    void processBatchTBB(std::vector<Frame>& frames) {
        std::atomic<size_t> inputIndex(0);
        std::atomic<size_t> outputIndex(0);
        std::vector<Frame> results(frames.size());
        
        tbb::parallel_pipeline(m_maxParallelism,
            // 輸入階段
            tbb::make_filter<void, Frame*>(
                tbb::filter::serial_in_order,
                [&](tbb::flow_control& fc) -> Frame* {
                    size_t idx = inputIndex.fetch_add(1);
                    if (idx < frames.size()) {
                        return &frames[idx];
                    } else {
                        fc.stop();
                        return nullptr;
                    }
                }
            ) &
            // 處理階段
            tbb::make_filter<Frame*, Frame*>(
                tbb::filter::parallel,
                [this](Frame* frame) -> Frame* {
                    if (!frame) return nullptr;
                    
                    for (auto& stage : m_stages) {
                        *frame = stage->process(*frame);
                    }
                    return frame;
                }
            ) &
            // 輸出階段
            tbb::make_filter<Frame*, void>(
                tbb::filter::serial_in_order,
                [&](Frame* frame) {
                    if (frame) {
                        size_t idx = outputIndex.fetch_add(1);
                        results[idx] = *frame;
                        emit frameProcessed(frame->frameNumber);
                    }
                }
            )
        );
        
        frames = std::move(results);
    }
    
    // 異步處理
    std::future<Frame> processAsync(Frame frame) {
        return std::async(std::launch::async, [this, frame]() mutable {
            for (auto& stage : m_stages) {
                frame = stage->process(frame);
            }
            return frame;
        });
    }
    
signals:
    void frameProcessed(int frameNumber);
    
private:
    std::vector<std::unique_ptr<PipelineStage>> m_stages;
    int m_maxParallelism;
};
```

---

## 6. GPU加速整合方案

### 6.1 OpenGL加速渲染

```cpp
// OpenGLAcceleratedRenderer.h
#pragma once
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLTexture>
#include <opencv2/opencv.hpp>

class OpenGLAcceleratedRenderer : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT
    
public:
    OpenGLAcceleratedRenderer(QWidget* parent = nullptr) 
        : QOpenGLWidget(parent), m_texture(nullptr) {
        
        // 設置OpenGL格式
        QSurfaceFormat format;
        format.setDepthBufferSize(24);
        format.setStencilBufferSize(8);
        format.setVersion(4, 5);
        format.setProfile(QSurfaceFormat::CoreProfile);
        setFormat(format);
    }
    
    ~OpenGLAcceleratedRenderer() {
        makeCurrent();
        delete m_texture;
        doneCurrent();
    }
    
    void updateImage(const cv::Mat& image) {
        makeCurrent();
        
        if (m_texture) {
            delete m_texture;
        }
        
        // 轉換OpenCV影像到QImage
        QImage qimage;
        if (image.channels() == 1) {
            qimage = QImage(image.data, image.cols, image.rows, 
                           image.step, QImage::Format_Grayscale8);
        } else if (image.channels() == 3) {
            cv::Mat rgb;
            cv::cvtColor(image, rgb, cv::COLOR_BGR2RGB);
            qimage = QImage(rgb.data, rgb.cols, rgb.rows, 
                           rgb.step, QImage::Format_RGB888);
        }
        
        // 創建OpenGL紋理
        m_texture = new QOpenGLTexture(qimage);
        m_texture->setMinificationFilter(QOpenGLTexture::Linear);
        m_texture->setMagnificationFilter(QOpenGLTexture::Linear);
        
        doneCurrent();
        update();
    }
    
protected:
    void initializeGL() override {
        initializeOpenGLFunctions();
        
        // 編譯著色器
        const char* vertexShaderSource = R"(
            #version 450 core
            layout (location = 0) in vec3 position;
            layout (location = 1) in vec2 texCoord;
            out vec2 TexCoord;
            
            uniform mat4 transform;
            
            void main() {
                gl_Position = transform * vec4(position, 1.0);
                TexCoord = texCoord;
            }
        )";
        
        const char* fragmentShaderSource = R"(
            #version 450 core
            in vec2 TexCoord;
            out vec4 FragColor;
            
            uniform sampler2D texture1;
            uniform float brightness;
            uniform float contrast;
            
            void main() {
                vec4 color = texture(texture1, TexCoord);
                
                // 應用亮度和對比度調整
                color.rgb = (color.rgb - 0.5) * contrast + 0.5 + brightness;
                
                FragColor = color;
            }
        )";
        
        m_program.addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource);
        m_program.addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource);
        m_program.link();
        
        // 設置頂點數據
        GLfloat vertices[] = {
            // 位置           // 紋理坐標
            -1.0f, -1.0f, 0.0f,  0.0f, 1.0f,
             1.0f, -1.0f, 0.0f,  1.0f, 1.0f,
             1.0f,  1.0f, 0.0f,  1.0f, 0.0f,
            -1.0f,  1.0f, 0.0f,  0.0f, 0.0f
        };
        
        GLuint indices[] = {
            0, 1, 2,
            2, 3, 0
        };
        
        m_vbo.create();
        m_vbo.bind();
        m_vbo.allocate(vertices, sizeof(vertices));
        
        m_ebo.create();
        m_ebo.bind();
        m_ebo.allocate(indices, sizeof(indices));
    }
    
    void paintGL() override {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        if (!m_texture) return;
        
        m_program.bind();
        m_texture->bind();
        
        // 設置uniform變量
        m_program.setUniformValue("brightness", m_brightness);
        m_program.setUniformValue("contrast", m_contrast);
        
        // 設置變換矩陣
        QMatrix4x4 transform;
        transform.scale(m_zoom);
        transform.rotate(m_rotation, 0, 0, 1);
        m_program.setUniformValue("transform", transform);
        
        // 繪製
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        
        m_texture->release();
        m_program.release();
    }
    
    void resizeGL(int width, int height) override {
        glViewport(0, 0, width, height);
    }
    
private:
    QOpenGLShaderProgram m_program;
    QOpenGLBuffer m_vbo{QOpenGLBuffer::VertexBuffer};
    QOpenGLBuffer m_ebo{QOpenGLBuffer::IndexBuffer};
    QOpenGLTexture* m_texture;
    
    float m_brightness = 0.0f;
    float m_contrast = 1.0f;
    float m_zoom = 1.0f;
    float m_rotation = 0.0f;
};
```

### 6.2 CUDA加速處理

```cpp
// CudaImageProcessor.h
#pragma once
#ifdef USE_CUDA

#include <cuda_runtime.h>
#include <opencv2/opencv.hpp>
#include <opencv2/cudaarithm.hpp>
#include <opencv2/cudaimgproc.hpp>

class CudaImageProcessor {
public:
    class CudaTimer {
    public:
        CudaTimer() {
            cudaEventCreate(&m_start);
            cudaEventCreate(&m_stop);
        }
        
        ~CudaTimer() {
            cudaEventDestroy(m_start);
            cudaEventDestroy(m_stop);
        }
        
        void start() {
            cudaEventRecord(m_start);
        }
        
        float stop() {
            cudaEventRecord(m_stop);
            cudaEventSynchronize(m_stop);
            
            float milliseconds = 0;
            cudaEventElapsedTime(&milliseconds, m_start, m_stop);
            return milliseconds;
        }
        
    private:
        cudaEvent_t m_start, m_stop;
    };
    
    // CUDA記憶體池
    class CudaMemoryPool {
    public:
        CudaMemoryPool(size_t initialSize = 100 * 1024 * 1024) // 100MB
            : m_totalSize(initialSize), m_usedSize(0) {
            
            cudaMalloc(&m_basePtr, m_totalSize);
            m_currentPtr = m_basePtr;
        }
        
        ~CudaMemoryPool() {
            cudaFree(m_basePtr);
        }
        
        void* allocate(size_t size) {
            size_t alignedSize = (size + 255) & ~255; // 256-byte alignment
            
            if (m_usedSize + alignedSize > m_totalSize) {
                return nullptr; // Pool exhausted
            }
            
            void* ptr = m_currentPtr;
            m_currentPtr = static_cast<char*>(m_currentPtr) + alignedSize;
            m_usedSize += alignedSize;
            
            return ptr;
        }
        
        void reset() {
            m_currentPtr = m_basePtr;
            m_usedSize = 0;
        }
        
    private:
        void* m_basePtr;
        void* m_currentPtr;
        size_t m_totalSize;
        size_t m_usedSize;
    };
    
    // 高效能影像處理
    static void processImageBatch(const std::vector<cv::Mat>& cpuImages,
                                  std::vector<cv::Mat>& results) {
        CudaTimer timer;
        timer.start();
        
        // 使用CUDA流進行並行處理
        const int numStreams = 4;
        std::vector<cudaStream_t> streams(numStreams);
        for (auto& stream : streams) {
            cudaStreamCreate(&stream);
        }
        
        std::vector<cv::cuda::GpuMat> gpuImages(cpuImages.size());
        std::vector<cv::cuda::GpuMat> gpuResults(cpuImages.size());
        
        // 異步上傳到GPU
        for (size_t i = 0; i < cpuImages.size(); ++i) {
            int streamIdx = i % numStreams;
            gpuImages[i].upload(cpuImages[i], streams[streamIdx]);
        }
        
        // 並行處理
        for (size_t i = 0; i < gpuImages.size(); ++i) {
            int streamIdx = i % numStreams;
            
            // 高斯模糊
            cv::cuda::GaussianBlur(gpuImages[i], gpuResults[i], 
                                   cv::Size(5, 5), 1.0, 1.0,
                                   cv::BORDER_DEFAULT, streams[streamIdx]);
            
            // Sobel邊緣檢測
            cv::cuda::Sobel(gpuResults[i], gpuResults[i], 
                           CV_32F, 1, 1, 3, 1, 0,
                           cv::BORDER_DEFAULT, streams[streamIdx]);
        }
        
        // 異步下載結果
        results.resize(cpuImages.size());
        for (size_t i = 0; i < gpuResults.size(); ++i) {
            int streamIdx = i % numStreams;
            gpuResults[i].download(results[i], streams[streamIdx]);
        }
        
        // 同步所有流
        for (auto& stream : streams) {
            cudaStreamSynchronize(stream);
            cudaStreamDestroy(stream);
        }
        
        float elapsed = timer.stop();
        qDebug() << "CUDA processing time:" << elapsed << "ms";
        qDebug() << "Throughput:" << (cpuImages.size() * 1000.0 / elapsed) << "fps";
    }
    
    // 自定義CUDA核心
    static void launchCustomKernel(cv::cuda::GpuMat& image);
};

// CustomKernels.cu
__global__ void customImageProcessingKernel(uchar* data, int width, int height, int step) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;
    
    if (x >= width || y >= height) return;
    
    // 共享記憶體優化
    __shared__ uchar tile[18][18]; // 16x16 tile with 1-pixel border
    
    int tx = threadIdx.x + 1;
    int ty = threadIdx.y + 1;
    
    // 載入tile數據
    tile[ty][tx] = data[y * step + x];
    
    // 載入邊界
    if (threadIdx.x == 0 && x > 0)
        tile[ty][tx - 1] = data[y * step + (x - 1)];
    if (threadIdx.x == blockDim.x - 1 && x < width - 1)
        tile[ty][tx + 1] = data[y * step + (x + 1)];
    if (threadIdx.y == 0 && y > 0)
        tile[ty - 1][tx] = data[(y - 1) * step + x];
    if (threadIdx.y == blockDim.y - 1 && y < height - 1)
        tile[ty + 1][tx] = data[(y + 1) * step + x];
    
    __syncthreads();
    
    // 執行3x3卷積
    if (x > 0 && x < width - 1 && y > 0 && y < height - 1) {
        int sum = tile[ty - 1][tx - 1] + tile[ty - 1][tx] + tile[ty - 1][tx + 1] +
                  tile[ty][tx - 1] + tile[ty][tx] * 4 + tile[ty][tx + 1] +
                  tile[ty + 1][tx - 1] + tile[ty + 1][tx] + tile[ty + 1][tx + 1];
        
        data[y * step + x] = min(255, sum / 12);
    }
}

void CudaImageProcessor::launchCustomKernel(cv::cuda::GpuMat& image) {
    dim3 blockSize(16, 16);
    dim3 gridSize((image.cols + blockSize.x - 1) / blockSize.x,
                  (image.rows + blockSize.y - 1) / blockSize.y);
    
    customImageProcessingKernel<<<gridSize, blockSize>>>(
        image.data, image.cols, image.rows, image.step
    );
    
    cudaDeviceSynchronize();
}

#endif // USE_CUDA
```

---

## 7. 記憶體優化技術

### 7.1 記憶體池實現

```cpp
// MemoryPool.h
#pragma once
#include <vector>
#include <stack>
#include <mutex>
#include <memory>

template<typename T>
class MemoryPool {
public:
    explicit MemoryPool(size_t blockSize = 1024) 
        : m_blockSize(blockSize) {
        allocateBlock();
    }
    
    ~MemoryPool() {
        for (auto& block : m_blocks) {
            ::operator delete(block);
        }
    }
    
    template<typename... Args>
    T* construct(Args&&... args) {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        if (m_available.empty()) {
            allocateBlock();
        }
        
        T* obj = m_available.top();
        m_available.pop();
        
        new (obj) T(std::forward<Args>(args)...);
        return obj;
    }
    
    void destroy(T* obj) {
        if (!obj) return;
        
        obj->~T();
        
        std::lock_guard<std::mutex> lock(m_mutex);
        m_available.push(obj);
    }
    
    size_t capacity() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_blocks.size() * m_blockSize;
    }
    
    size_t available() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_available.size();
    }
    
private:
    void allocateBlock() {
        // 分配一個連續的記憶體塊
        T* block = static_cast<T*>(::operator new(m_blockSize * sizeof(T)));
        m_blocks.push_back(block);
        
        // 將所有物件加入可用棧
        for (size_t i = 0; i < m_blockSize; ++i) {
            m_available.push(block + i);
        }
    }
    
    mutable std::mutex m_mutex;
    std::vector<T*> m_blocks;
    std::stack<T*> m_available;
    size_t m_blockSize;
};

// 物件池
template<typename T>
class ObjectPool {
public:
    using Ptr = std::shared_ptr<T>;
    
    ObjectPool(size_t maxSize = 100) : m_maxSize(maxSize) {}
    
    template<typename... Args>
    Ptr acquire(Args&&... args) {
        std::unique_lock<std::mutex> lock(m_mutex);
        
        if (!m_pool.empty()) {
            Ptr obj = m_pool.back();
            m_pool.pop_back();
            
            // 重新初始化物件
            *obj = T(std::forward<Args>(args)...);
            
            return Ptr(obj.get(), [this](T* p) { release(p); });
        }
        
        lock.unlock();
        
        // 創建新物件
        auto obj = std::make_shared<T>(std::forward<Args>(args)...);
        return Ptr(obj.get(), [this, obj](T* p) { release(p); });
    }
    
private:
    void release(T* obj) {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        if (m_pool.size() < m_maxSize) {
            // 查找對應的shared_ptr
            for (auto& poolObj : m_pool) {
                if (poolObj.get() == obj) {
                    return; // 已在池中
                }
            }
            
            // 這裡需要更複雜的邏輯來正確管理shared_ptr
            // 簡化版本：假設外部管理shared_ptr
        }
    }
    
    std::mutex m_mutex;
    std::vector<Ptr> m_pool;
    size_t m_maxSize;
};
```

### 7.2 影像緩衝池

```cpp
// ImageBufferPool.h
#pragma once
#include <opencv2/opencv.hpp>
#include <queue>
#include <mutex>
#include <condition_variable>

class ImageBufferPool {
public:
    struct Buffer {
        cv::Mat image;
        size_t id;
        bool inUse;
        std::chrono::steady_clock::time_point lastUsed;
    };
    
    ImageBufferPool(size_t poolSize, int width, int height, int type)
        : m_width(width), m_height(height), m_type(type) {
        
        for (size_t i = 0; i < poolSize; ++i) {
            auto buffer = std::make_unique<Buffer>();
            buffer->image = cv::Mat(height, width, type);
            buffer->id = i;
            buffer->inUse = false;
            m_availableBuffers.push(buffer.get());
            m_buffers.push_back(std::move(buffer));
        }
    }
    
    cv::Mat acquire() {
        std::unique_lock<std::mutex> lock(m_mutex);
        
        m_condition.wait(lock, [this] { return !m_availableBuffers.empty(); });
        
        Buffer* buffer = m_availableBuffers.front();
        m_availableBuffers.pop();
        buffer->inUse = true;
        buffer->lastUsed = std::chrono::steady_clock::now();
        
        return buffer->image;
    }
    
    void release(const cv::Mat& image) {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        for (auto& buffer : m_buffers) {
            if (buffer->image.data == image.data) {
                buffer->inUse = false;
                m_availableBuffers.push(buffer.get());
                m_condition.notify_one();
                break;
            }
        }
    }
    
    size_t availableCount() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_availableBuffers.size();
    }
    
    void resize(int width, int height) {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        m_width = width;
        m_height = height;
        
        for (auto& buffer : m_buffers) {
            if (!buffer->inUse) {
                buffer->image = cv::Mat(height, width, m_type);
            }
        }
    }
    
private:
    mutable std::mutex m_mutex;
    std::condition_variable m_condition;
    std::queue<Buffer*> m_availableBuffers;
    std::vector<std::unique_ptr<Buffer>> m_buffers;
    int m_width, m_height, m_type;
};
```

---

## 8. Zero-copy技術實現

### 8.1 共享記憶體實現

```cpp
// SharedMemoryBuffer.h
#pragma once
#include <QSharedMemory>
#include <QSystemSemaphore>
#include <opencv2/opencv.hpp>

class SharedMemoryBuffer {
public:
    struct Header {
        int width;
        int height;
        int type;
        size_t dataSize;
        uint64_t frameNumber;
        int64_t timestamp;
    };
    
    SharedMemoryBuffer(const QString& key, size_t size, bool create = false)
        : m_sharedMemory(key), m_semaphore(key + "_sem", 1) {
        
        if (create) {
            if (m_sharedMemory.create(size)) {
                initialize();
            }
        } else {
            m_sharedMemory.attach();
        }
    }
    
    bool writeImage(const cv::Mat& image, uint64_t frameNumber) {
        if (!m_sharedMemory.isAttached()) return false;
        
        m_semaphore.acquire();
        
        m_sharedMemory.lock();
        
        char* buffer = static_cast<char*>(m_sharedMemory.data());
        Header* header = reinterpret_cast<Header*>(buffer);
        
        header->width = image.cols;
        header->height = image.rows;
        header->type = image.type();
        header->dataSize = image.total() * image.elemSize();
        header->frameNumber = frameNumber;
        header->timestamp = std::chrono::steady_clock::now().time_since_epoch().count();
        
        // Zero-copy: 直接寫入共享記憶體
        memcpy(buffer + sizeof(Header), image.data, header->dataSize);
        
        m_sharedMemory.unlock();
        
        m_semaphore.release();
        
        return true;
    }
    
    bool readImage(cv::Mat& image, uint64_t& frameNumber) {
        if (!m_sharedMemory.isAttached()) return false;
        
        m_semaphore.acquire();
        
        m_sharedMemory.lock();
        
        const char* buffer = static_cast<const char*>(m_sharedMemory.constData());
        const Header* header = reinterpret_cast<const Header*>(buffer);
        
        // Zero-copy: 創建Mat header指向共享記憶體
        image = cv::Mat(header->height, header->width, header->type, 
                       const_cast<char*>(buffer + sizeof(Header)));
        frameNumber = header->frameNumber;
        
        // 注意：這裡image只是一個視圖，實際數據在共享記憶體中
        // 如果需要持久化，應該clone()
        
        m_sharedMemory.unlock();
        
        m_semaphore.release();
        
        return true;
    }
    
private:
    void initialize() {
        m_sharedMemory.lock();
        memset(m_sharedMemory.data(), 0, m_sharedMemory.size());
        m_sharedMemory.unlock();
    }
    
    QSharedMemory m_sharedMemory;
    QSystemSemaphore m_semaphore;
};
```

### 8.2 記憶體映射文件

```cpp
// MemoryMappedRingBuffer.h
#pragma once
#include <QFile>
#include <opencv2/opencv.hpp>
#include <atomic>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#endif

class MemoryMappedRingBuffer {
public:
    struct FrameHeader {
        uint32_t magic;         // 魔術數字用於驗證
        uint32_t frameSize;     // 幀大小
        uint64_t frameNumber;   // 幀編號
        int64_t timestamp;      // 時間戳
        int width;
        int height;
        int type;
    };
    
    static constexpr uint32_t MAGIC = 0xDEADBEEF;
    
    MemoryMappedRingBuffer(const std::string& filename, 
                           size_t bufferSize, 
                           size_t frameCount,
                           bool create = false)
        : m_bufferSize(bufferSize), m_frameCount(frameCount) {
        
        m_frameSize = bufferSize / frameCount;
        
#ifdef _WIN32
        if (create) {
            // 創建文件
            m_fileHandle = CreateFileA(
                filename.c_str(),
                GENERIC_READ | GENERIC_WRITE,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                NULL,
                CREATE_ALWAYS,
                FILE_ATTRIBUTE_NORMAL,
                NULL
            );
            
            // 設置文件大小
            LARGE_INTEGER size;
            size.QuadPart = bufferSize;
            SetFilePointerEx(m_fileHandle, size, NULL, FILE_BEGIN);
            SetEndOfFile(m_fileHandle);
            
            // 創建文件映射
            m_mappingHandle = CreateFileMapping(
                m_fileHandle,
                NULL,
                PAGE_READWRITE,
                size.HighPart,
                size.LowPart,
                NULL
            );
        } else {
            // 打開現有文件
            m_fileHandle = CreateFileA(
                filename.c_str(),
                GENERIC_READ | GENERIC_WRITE,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                NULL,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL,
                NULL
            );
            
            m_mappingHandle = CreateFileMapping(
                m_fileHandle,
                NULL,
                PAGE_READWRITE,
                0, 0,
                NULL
            );
        }
        
        // 映射視圖
        m_buffer = static_cast<char*>(MapViewOfFile(
            m_mappingHandle,
            FILE_MAP_ALL_ACCESS,
            0, 0,
            bufferSize
        ));
#else
        if (create) {
            m_fd = open(filename.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0666);
            ftruncate(m_fd, bufferSize);
        } else {
            m_fd = open(filename.c_str(), O_RDWR);
        }
        
        m_buffer = static_cast<char*>(mmap(
            nullptr,
            bufferSize,
            PROT_READ | PROT_WRITE,
            MAP_SHARED,
            m_fd,
            0
        ));
#endif
        
        // 初始化索引
        m_writeIndex = reinterpret_cast<std::atomic<size_t>*>(m_buffer);
        m_readIndex = reinterpret_cast<std::atomic<size_t>*>(m_buffer + sizeof(size_t));
        m_dataStart = m_buffer + 2 * sizeof(size_t);
        
        if (create) {
            m_writeIndex->store(0);
            m_readIndex->store(0);
        }
    }
    
    ~MemoryMappedRingBuffer() {
#ifdef _WIN32
        if (m_buffer) {
            UnmapViewOfFile(m_buffer);
        }
        if (m_mappingHandle) {
            CloseHandle(m_mappingHandle);
        }
        if (m_fileHandle != INVALID_HANDLE_VALUE) {
            CloseHandle(m_fileHandle);
        }
#else
        if (m_buffer) {
            munmap(m_buffer, m_bufferSize);
        }
        if (m_fd >= 0) {
            close(m_fd);
        }
#endif
    }
    
    bool writeFrame(const cv::Mat& image, uint64_t frameNumber) {
        size_t writeIdx = m_writeIndex->load(std::memory_order_acquire);
        size_t nextIdx = (writeIdx + 1) % m_frameCount;
        
        if (nextIdx == m_readIndex->load(std::memory_order_acquire)) {
            return false; // Buffer full
        }
        
        char* frameBuffer = m_dataStart + writeIdx * m_frameSize;
        FrameHeader* header = reinterpret_cast<FrameHeader*>(frameBuffer);
        
        // 寫入頭部
        header->magic = MAGIC;
        header->frameSize = image.total() * image.elemSize();
        header->frameNumber = frameNumber;
        header->timestamp = std::chrono::steady_clock::now().time_since_epoch().count();
        header->width = image.cols;
        header->height = image.rows;
        header->type = image.type();
        
        // Zero-copy寫入數據
        if (header->frameSize + sizeof(FrameHeader) <= m_frameSize) {
            memcpy(frameBuffer + sizeof(FrameHeader), image.data, header->frameSize);
            
            // 確保數據寫入完成
            std::atomic_thread_fence(std::memory_order_release);
            
            m_writeIndex->store(nextIdx, std::memory_order_release);
            return true;
        }
        
        return false;
    }
    
    bool readFrame(cv::Mat& image, uint64_t& frameNumber) {
        size_t readIdx = m_readIndex->load(std::memory_order_acquire);
        
        if (readIdx == m_writeIndex->load(std::memory_order_acquire)) {
            return false; // Buffer empty
        }
        
        char* frameBuffer = m_dataStart + readIdx * m_frameSize;
        FrameHeader* header = reinterpret_cast<FrameHeader*>(frameBuffer);
        
        if (header->magic != MAGIC) {
            return false; // Invalid frame
        }
        
        // Zero-copy創建Mat
        image = cv::Mat(header->height, header->width, header->type,
                       frameBuffer + sizeof(FrameHeader));
        frameNumber = header->frameNumber;
        
        m_readIndex->store((readIdx + 1) % m_frameCount, std::memory_order_release);
        
        return true;
    }
    
private:
    char* m_buffer;
    char* m_dataStart;
    std::atomic<size_t>* m_writeIndex;
    std::atomic<size_t>* m_readIndex;
    size_t m_bufferSize;
    size_t m_frameCount;
    size_t m_frameSize;
    
#ifdef _WIN32
    HANDLE m_fileHandle;
    HANDLE m_mappingHandle;
#else
    int m_fd;
#endif
};
```

---

## 9. Windows平台特定優化

### 9.1 IOCP (I/O Completion Port)

```cpp
// IOCPServer.h
#pragma once
#ifdef _WIN32

#include <windows.h>
#include <vector>
#include <thread>
#include <functional>

class IOCPServer {
public:
    enum OperationType {
        OP_READ,
        OP_WRITE,
        OP_PROCESS
    };
    
    struct IOContext {
        OVERLAPPED overlapped;
        OperationType operation;
        WSABUF wsaBuf;
        char buffer[4096];
        std::function<void(IOContext*)> callback;
        
        IOContext() {
            ZeroMemory(&overlapped, sizeof(overlapped));
            wsaBuf.buf = buffer;
            wsaBuf.len = sizeof(buffer);
        }
    };
    
    IOCPServer(size_t numThreads = 0) {
        if (numThreads == 0) {
            SYSTEM_INFO sysInfo;
            GetSystemInfo(&sysInfo);
            numThreads = sysInfo.dwNumberOfProcessors * 2;
        }
        
        // 創建IOCP
        m_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, numThreads);
        
        // 創建工作線程
        for (size_t i = 0; i < numThreads; ++i) {
            m_threads.emplace_back([this]() { workerThread(); });
        }
    }
    
    ~IOCPServer() {
        shutdown();
    }
    
    bool associateDevice(HANDLE device, ULONG_PTR completionKey) {
        HANDLE result = CreateIoCompletionPort(
            device, 
            m_iocp, 
            completionKey, 
            0
        );
        return result == m_iocp;
    }
    
    bool postCompletion(DWORD bytesTransferred, 
                        ULONG_PTR completionKey, 
                        IOContext* context) {
        return PostQueuedCompletionStatus(
            m_iocp,
            bytesTransferred,
            completionKey,
            &context->overlapped
        );
    }
    
    void shutdown() {
        m_running = false;
        
        // 發送關閉信號到所有工作線程
        for (size_t i = 0; i < m_threads.size(); ++i) {
            PostQueuedCompletionStatus(m_iocp, 0, 0, nullptr);
        }
        
        // 等待所有線程結束
        for (auto& thread : m_threads) {
            if (thread.joinable()) {
                thread.join();
            }
        }
        
        CloseHandle(m_iocp);
    }
    
private:
    void workerThread() {
        DWORD bytesTransferred;
        ULONG_PTR completionKey;
        LPOVERLAPPED overlapped;
        
        while (m_running) {
            BOOL result = GetQueuedCompletionStatus(
                m_iocp,
                &bytesTransferred,
                &completionKey,
                &overlapped,
                INFINITE
            );
            
            if (!result) {
                DWORD error = GetLastError();
                if (error != ERROR_OPERATION_ABORTED) {
                    // 處理錯誤
                }
                continue;
            }
            
            if (completionKey == 0 && overlapped == nullptr) {
                break; // Shutdown signal
            }
            
            IOContext* context = CONTAINING_RECORD(overlapped, IOContext, overlapped);
            
            if (context->callback) {
                context->callback(context);
            }
            
            // 處理不同的操作類型
            switch (context->operation) {
                case OP_READ:
                    handleRead(context, bytesTransferred);
                    break;
                case OP_WRITE:
                    handleWrite(context, bytesTransferred);
                    break;
                case OP_PROCESS:
                    handleProcess(context, bytesTransferred);
                    break;
            }
        }
    }
    
    void handleRead(IOContext* context, DWORD bytesTransferred) {
        // 處理讀取完成
    }
    
    void handleWrite(IOContext* context, DWORD bytesTransferred) {
        // 處理寫入完成
    }
    
    void handleProcess(IOContext* context, DWORD bytesTransferred) {
        // 處理自定義操作
    }
    
    HANDLE m_iocp;
    std::vector<std::thread> m_threads;
    std::atomic<bool> m_running{true};
};

#endif // _WIN32
```

### 9.2 Windows Thread Pool API

```cpp
// WindowsThreadPool.h
#pragma once
#ifdef _WIN32

#include <windows.h>
#include <functional>
#include <memory>

class WindowsThreadPool {
public:
    WindowsThreadPool() {
        // 創建線程池
        m_pool = CreateThreadpool(NULL);
        
        // 設置線程池大小
        SetThreadpoolThreadMaximum(m_pool, 32);
        SetThreadpoolThreadMinimum(m_pool, 4);
        
        // 初始化回調環境
        InitializeThreadpoolEnvironment(&m_environment);
        SetThreadpoolCallbackPool(&m_environment, m_pool);
        
        // 創建清理組
        m_cleanupGroup = CreateThreadpoolCleanupGroup();
        SetThreadpoolCallbackCleanupGroup(&m_environment, m_cleanupGroup, NULL);
    }
    
    ~WindowsThreadPool() {
        // 等待所有工作完成
        CloseThreadpoolCleanupGroupMembers(m_cleanupGroup, FALSE, NULL);
        CloseThreadpoolCleanupGroup(m_cleanupGroup);
        
        DestroyThreadpoolEnvironment(&m_environment);
        CloseThreadpool(m_pool);
    }
    
    // 提交工作項
    template<typename Func>
    bool submitWork(Func&& func) {
        auto* context = new WorkContext<Func>(std::forward<Func>(func));
        
        PTP_WORK work = CreateThreadpoolWork(
            WorkCallback<Func>,
            context,
            &m_environment
        );
        
        if (work == NULL) {
            delete context;
            return false;
        }
        
        SubmitThreadpoolWork(work);
        return true;
    }
    
    // 提交定時器
    template<typename Func>
    PTP_TIMER submitTimer(DWORD delayMs, DWORD periodMs, Func&& func) {
        auto* context = new WorkContext<Func>(std::forward<Func>(func));
        
        PTP_TIMER timer = CreateThreadpoolTimer(
            TimerCallback<Func>,
            context,
            &m_environment
        );
        
        if (timer == NULL) {
            delete context;
            return NULL;
        }
        
        FILETIME dueTime;
        ULARGE_INTEGER ulDueTime;
        ulDueTime.QuadPart = (ULONGLONG) -(10000LL * delayMs);
        dueTime.dwHighDateTime = ulDueTime.HighPart;
        dueTime.dwLowDateTime = ulDueTime.LowPart;
        
        SetThreadpoolTimer(timer, &dueTime, periodMs, 0);
        
        return timer;
    }
    
    // 提交I/O操作
    template<typename Func>
    PTP_IO submitIO(HANDLE handle, Func&& func) {
        auto* context = new WorkContext<Func>(std::forward<Func>(func));
        
        PTP_IO io = CreateThreadpoolIo(
            handle,
            IOCallback<Func>,
            context,
            &m_environment
        );
        
        if (io == NULL) {
            delete context;
            return NULL;
        }
        
        StartThreadpoolIo(io);
        
        return io;
    }
    
    // 等待所有工作完成
    void waitForAll() {
        CloseThreadpoolCleanupGroupMembers(m_cleanupGroup, FALSE, NULL);
    }
    
private:
    template<typename Func>
    struct WorkContext {
        Func function;
        explicit WorkContext(Func&& f) : function(std::forward<Func>(f)) {}
    };
    
    template<typename Func>
    static VOID CALLBACK WorkCallback(PTP_CALLBACK_INSTANCE instance,
                                      PVOID context,
                                      PTP_WORK work) {
        auto* workContext = static_cast<WorkContext<Func>*>(context);
        workContext->function();
        CloseThreadpoolWork(work);
        delete workContext;
    }
    
    template<typename Func>
    static VOID CALLBACK TimerCallback(PTP_CALLBACK_INSTANCE instance,
                                       PVOID context,
                                       PTP_TIMER timer) {
        auto* workContext = static_cast<WorkContext<Func>*>(context);
        workContext->function();
    }
    
    template<typename Func>
    static VOID CALLBACK IOCallback(PTP_CALLBACK_INSTANCE instance,
                                    PVOID context,
                                    PVOID overlapped,
                                    ULONG ioResult,
                                    ULONG_PTR bytesTransferred,
                                    PTP_IO io) {
        auto* workContext = static_cast<WorkContext<Func>*>(context);
        workContext->function();
    }
    
    PTP_POOL m_pool;
    TP_CALLBACK_ENVIRON m_environment;
    PTP_CLEANUP_GROUP m_cleanupGroup;
};

#endif // _WIN32
```

---

## 10. 即時性保證機制

### 10.1 即時調度器

```cpp
// RealtimeScheduler.h
#pragma once
#include <QObject>
#include <QTimer>
#include <chrono>
#include <queue>
#include <functional>

#ifdef _WIN32
#include <windows.h>
#include <timeapi.h>
#pragma comment(lib, "winmm.lib")
#endif

class RealtimeScheduler : public QObject {
    Q_OBJECT
    
public:
    struct Task {
        std::chrono::steady_clock::time_point deadline;
        std::function<void()> callback;
        int priority;
        
        bool operator<(const Task& other) const {
            if (deadline == other.deadline) {
                return priority < other.priority;
            }
            return deadline > other.deadline;
        }
    };
    
    RealtimeScheduler(QObject* parent = nullptr) 
        : QObject(parent) {
        
#ifdef _WIN32
        // 設置高精度計時器
        timeBeginPeriod(1);
        
        // 提升進程優先級
        SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
        
        // 設置線程優先級
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
#endif
        
        // 使用高精度計時器
        m_timer = new QTimer(this);
        m_timer->setTimerType(Qt::PreciseTimer);
        m_timer->setInterval(1); // 1ms精度
        connect(m_timer, &QTimer::timeout, this, &RealtimeScheduler::processTasks);
        m_timer->start();
    }
    
    ~RealtimeScheduler() {
#ifdef _WIN32
        timeEndPeriod(1);
#endif
    }
    
    void scheduleTask(std::function<void()> callback, 
                     std::chrono::milliseconds delay,
                     int priority = 0) {
        auto deadline = std::chrono::steady_clock::now() + delay;
        
        std::lock_guard<std::mutex> lock(m_mutex);
        m_tasks.push({deadline, callback, priority});
    }
    
    void schedulePeriodicTask(std::function<void()> callback,
                             std::chrono::milliseconds period,
                             int priority = 0) {
        m_periodicTasks.push_back({callback, period, priority});
    }
    
private slots:
    void processTasks() {
        auto now = std::chrono::steady_clock::now();
        
        std::lock_guard<std::mutex> lock(m_mutex);
        
        // 處理到期的任務
        while (!m_tasks.empty() && m_tasks.top().deadline <= now) {
            auto task = m_tasks.top();
            m_tasks.pop();
            
            // 測量執行時間
            auto start = std::chrono::high_resolution_clock::now();
            task.callback();
            auto end = std::chrono::high_resolution_clock::now();
            
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            
            if (duration.count() > 1000) { // 超過1ms
                qWarning() << "Task execution took" << duration.count() << "µs";
            }
        }
        
        // 處理週期性任務
        for (auto& [callback, period, priority] : m_periodicTasks) {
            static std::map<void*, std::chrono::steady_clock::time_point> lastExecution;
            
            auto& lastTime = lastExecution[&callback];
            if (now - lastTime >= period) {
                callback();
                lastTime = now;
            }
        }
    }
    
private:
    QTimer* m_timer;
    std::mutex m_mutex;
    std::priority_queue<Task> m_tasks;
    std::vector<std::tuple<std::function<void()>, 
                           std::chrono::milliseconds, 
                           int>> m_periodicTasks;
};
```

### 10.2 幀率控制器

```cpp
// FrameRateController.h
#pragma once
#include <QObject>
#include <chrono>
#include <atomic>
#include <deque>

class FrameRateController : public QObject {
    Q_OBJECT
    
public:
    FrameRateController(double targetFps, QObject* parent = nullptr)
        : QObject(parent), m_targetFps(targetFps) {
        
        m_targetFrameTime = std::chrono::microseconds(
            static_cast<int64_t>(1000000.0 / targetFps)
        );
    }
    
    void beginFrame() {
        m_frameStart = std::chrono::high_resolution_clock::now();
    }
    
    void endFrame() {
        auto frameEnd = std::chrono::high_resolution_clock::now();
        auto frameDuration = frameEnd - m_frameStart;
        
        // 記錄幀時間
        m_frameTimes.push_back(frameDuration);
        if (m_frameTimes.size() > 100) {
            m_frameTimes.pop_front();
        }
        
        // 計算統計信息
        updateStatistics();
        
        // 自適應延遲
        auto sleepTime = m_targetFrameTime - frameDuration;
        if (sleepTime > std::chrono::microseconds(0)) {
            // 高精度睡眠
            preciseSleep(sleepTime);
        } else {
            // 掉幀警告
            m_droppedFrames++;
            emit frameDropped();
        }
        
        m_frameCount++;
    }
    
    double getCurrentFps() const {
        return m_currentFps.load();
    }
    
    double getAverageFps() const {
        return m_averageFps.load();
    }
    
    double getMinFps() const {
        return m_minFps.load();
    }
    
    double getMaxFps() const {
        return m_maxFps.load();
    }
    
    uint64_t getDroppedFrames() const {
        return m_droppedFrames.load();
    }
    
    double getJitter() const {
        return m_jitter.load();
    }
    
signals:
    void frameDropped();
    void performanceWarning(const QString& message);
    
private:
    void preciseSleep(std::chrono::microseconds duration) {
#ifdef _WIN32
        // Windows高精度睡眠
        LARGE_INTEGER frequency;
        QueryPerformanceFrequency(&frequency);
        
        LARGE_INTEGER start;
        QueryPerformanceCounter(&start);
        
        double targetSeconds = duration.count() / 1000000.0;
        LONGLONG targetTicks = static_cast<LONGLONG>(targetSeconds * frequency.QuadPart);
        
        // 混合策略：先Sleep大部分時間，然後忙等待
        auto sleepMs = duration.count() / 1000 - 1;
        if (sleepMs > 0) {
            Sleep(sleepMs);
        }
        
        // 忙等待剩餘時間
        LARGE_INTEGER current;
        do {
            QueryPerformanceCounter(&current);
            YieldProcessor(); // CPU hint
        } while (current.QuadPart - start.QuadPart < targetTicks);
#else
        std::this_thread::sleep_for(duration);
#endif
    }
    
    void updateStatistics() {
        if (m_frameTimes.empty()) return;
        
        // 計算平均FPS
        auto totalTime = std::chrono::microseconds(0);
        for (const auto& time : m_frameTimes) {
            totalTime += time;
        }
        
        double avgFrameTime = totalTime.count() / static_cast<double>(m_frameTimes.size());
        m_averageFps = 1000000.0 / avgFrameTime;
        
        // 計算當前FPS（最近10幀）
        int recentFrames = std::min(10, static_cast<int>(m_frameTimes.size()));
        auto recentTime = std::chrono::microseconds(0);
        for (int i = m_frameTimes.size() - recentFrames; i < m_frameTimes.size(); ++i) {
            recentTime += m_frameTimes[i];
        }
        double currentFrameTime = recentTime.count() / static_cast<double>(recentFrames);
        m_currentFps = 1000000.0 / currentFrameTime;
        
        // 計算最小/最大FPS
        auto minTime = *std::min_element(m_frameTimes.begin(), m_frameTimes.end());
        auto maxTime = *std::max_element(m_frameTimes.begin(), m_frameTimes.end());
        
        m_maxFps = 1000000.0 / minTime.count();
        m_minFps = 1000000.0 / maxTime.count();
        
        // 計算抖動（標準差）
        double variance = 0;
        for (const auto& time : m_frameTimes) {
            double diff = time.count() - avgFrameTime;
            variance += diff * diff;
        }
        variance /= m_frameTimes.size();
        m_jitter = std::sqrt(variance) / 1000.0; // 轉換為毫秒
        
        // 性能警告
        if (m_currentFps < m_targetFps * 0.9) {
            emit performanceWarning(
                QString("FPS below target: %1 < %2")
                    .arg(m_currentFps.load())
                    .arg(m_targetFps)
            );
        }
        
        if (m_jitter > 5.0) { // 抖動超過5ms
            emit performanceWarning(
                QString("High frame time jitter: %1 ms")
                    .arg(m_jitter.load())
            );
        }
    }
    
    double m_targetFps;
    std::chrono::microseconds m_targetFrameTime;
    std::chrono::high_resolution_clock::time_point m_frameStart;
    
    std::deque<std::chrono::microseconds> m_frameTimes;
    
    std::atomic<double> m_currentFps{0};
    std::atomic<double> m_averageFps{0};
    std::atomic<double> m_minFps{0};
    std::atomic<double> m_maxFps{0};
    std::atomic<uint64_t> m_frameCount{0};
    std::atomic<uint64_t> m_droppedFrames{0};
    std::atomic<double> m_jitter{0};
};
```

---

## 11. 效能測試基準與工具

### 11.1 綜合效能測試套件

```cpp
// PerformanceBenchmark.h
#pragma once
#include <QObject>
#include <opencv2/opencv.hpp>
#include <vector>
#include <map>
#include <fstream>

class PerformanceBenchmark : public QObject {
    Q_OBJECT
    
public:
    struct BenchmarkResult {
        QString testName;
        double avgTime;      // 平均時間（毫秒）
        double minTime;      // 最小時間
        double maxTime;      // 最大時間
        double stdDev;       // 標準差
        double throughput;   // 吞吐量（fps或MB/s）
        size_t iterations;   // 迭代次數
        
        QString toString() const {
            return QString("%1: Avg=%2ms, Min=%3ms, Max=%4ms, StdDev=%5ms, Throughput=%6")
                .arg(testName)
                .arg(avgTime, 0, 'f', 3)
                .arg(minTime, 0, 'f', 3)
                .arg(maxTime, 0, 'f', 3)
                .arg(stdDev, 0, 'f', 3)
                .arg(throughput, 0, 'f', 2);
        }
    };
    
    // 基準測試：影像處理
    BenchmarkResult benchmarkImageProcessing(int imageSize, int iterations = 100) {
        BenchmarkResult result;
        result.testName = QString("Image Processing %1x%1").arg(imageSize);
        result.iterations = iterations;
        
        cv::Mat image(imageSize, imageSize, CV_8UC3);
        cv::randu(image, 0, 255);
        
        std::vector<double> times;
        
        for (int i = 0; i < iterations; ++i) {
            auto start = std::chrono::high_resolution_clock::now();
            
            // 執行影像處理操作
            cv::Mat gray, edges;
            cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
            cv::GaussianBlur(gray, gray, cv::Size(5, 5), 1.0);
            cv::Canny(gray, edges, 50, 150);
            
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration<double, std::milli>(end - start);
            times.push_back(duration.count());
        }
        
        calculateStatistics(times, result);
        result.throughput = 1000.0 / result.avgTime; // FPS
        
        return result;
    }
    
    // 基準測試：多執行緒
    BenchmarkResult benchmarkMultithreading(int numThreads, int workload) {
        BenchmarkResult result;
        result.testName = QString("Multithreading %1 threads").arg(numThreads);
        
        std::vector<double> times;
        const int iterations = 50;
        
        for (int iter = 0; iter < iterations; ++iter) {
            auto start = std::chrono::high_resolution_clock::now();
            
            std::vector<std::thread> threads;
            std::atomic<int> counter{0};
            
            for (int i = 0; i < numThreads; ++i) {
                threads.emplace_back([&counter, workload]() {
                    for (int j = 0; j < workload; ++j) {
                        counter.fetch_add(1, std::memory_order_relaxed);
                        // 模擬工作負載
                        volatile double x = 0;
                        for (int k = 0; k < 1000; ++k) {
                            x += std::sqrt(k);
                        }
                    }
                });
            }
            
            for (auto& t : threads) {
                t.join();
            }
            
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration<double, std::milli>(end - start);
            times.push_back(duration.count());
        }
        
        calculateStatistics(times, result);
        result.throughput = (numThreads * workload) / (result.avgTime / 1000.0);
        
        return result;
    }
    
    // 基準測試：記憶體頻寬
    BenchmarkResult benchmarkMemoryBandwidth(size_t size) {
        BenchmarkResult result;
        result.testName = QString("Memory Bandwidth %1 MB").arg(size / (1024 * 1024));
        
        std::vector<char> src(size);
        std::vector<char> dst(size);
        
        // 初始化數據
        std::generate(src.begin(), src.end(), std::rand);
        
        std::vector<double> times;
        const int iterations = 100;
        
        for (int i = 0; i < iterations; ++i) {
            auto start = std::chrono::high_resolution_clock::now();
            
            std::memcpy(dst.data(), src.data(), size);
            
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration<double, std::milli>(end - start);
            times.push_back(duration.count());
        }
        
        calculateStatistics(times, result);
        result.throughput = (size / (1024.0 * 1024.0)) / (result.avgTime / 1000.0); // MB/s
        
        return result;
    }
    
    // 執行完整基準測試套件
    void runFullBenchmark() {
        std::vector<BenchmarkResult> results;
        
        emit benchmarkStarted();
        
        // 影像處理測試
        for (int size : {640, 1280, 1920, 3840}) {
            results.push_back(benchmarkImageProcessing(size));
            emit progressUpdated(results.size(), 10);
        }
        
        // 多執行緒測試
        for (int threads : {1, 2, 4, 8, 16}) {
            results.push_back(benchmarkMultithreading(threads, 10000));
            emit progressUpdated(results.size(), 10);
        }
        
        // 記憶體頻寬測試
        for (size_t size : {1, 10, 100, 1000}) {
            results.push_back(benchmarkMemoryBandwidth(size * 1024 * 1024));
            emit progressUpdated(results.size(), 10);
        }
        
        // 生成報告
        generateReport(results);
        
        emit benchmarkCompleted(results);
    }
    
    // 生成HTML報告
    void generateReport(const std::vector<BenchmarkResult>& results) {
        std::ofstream report("benchmark_report.html");
        
        report << R"(
<!DOCTYPE html>
<html>
<head>
    <title>Performance Benchmark Report</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; }
        table { border-collapse: collapse; width: 100%; }
        th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }
        th { background-color: #4CAF50; color: white; }
        tr:nth-child(even) { background-color: #f2f2f2; }
        .chart { width: 100%; height: 400px; margin: 20px 0; }
    </style>
    <script src="https://cdn.plot.ly/plotly-latest.min.js"></script>
</head>
<body>
    <h1>Performance Benchmark Report</h1>
    <p>Generated: )" << QDateTime::currentDateTime().toString().toStdString() << R"(</p>
    
    <h2>Test Results</h2>
    <table>
        <tr>
            <th>Test Name</th>
            <th>Avg Time (ms)</th>
            <th>Min Time (ms)</th>
            <th>Max Time (ms)</th>
            <th>Std Dev (ms)</th>
            <th>Throughput</th>
        </tr>
)";
        
        for (const auto& result : results) {
            report << "<tr>"
                  << "<td>" << result.testName.toStdString() << "</td>"
                  << "<td>" << result.avgTime << "</td>"
                  << "<td>" << result.minTime << "</td>"
                  << "<td>" << result.maxTime << "</td>"
                  << "<td>" << result.stdDev << "</td>"
                  << "<td>" << result.throughput << "</td>"
                  << "</tr>\n";
        }
        
        report << R"(
    </table>
    
    <h2>Performance Charts</h2>
    <div id="avgTimeChart" class="chart"></div>
    <div id="throughputChart" class="chart"></div>
    
    <script>
        // Average Time Chart
        var avgTimeData = [{
            x: [)" << getTestNames(results) << R"(],
            y: [)" << getAvgTimes(results) << R"(],
            type: 'bar',
            name: 'Average Time'
        }];
        
        var avgTimeLayout = {
            title: 'Average Processing Time',
            xaxis: { title: 'Test' },
            yaxis: { title: 'Time (ms)' }
        };
        
        Plotly.newPlot('avgTimeChart', avgTimeData, avgTimeLayout);
        
        // Throughput Chart
        var throughputData = [{
            x: [)" << getTestNames(results) << R"(],
            y: [)" << getThroughputs(results) << R"(],
            type: 'bar',
            name: 'Throughput',
            marker: { color: 'orange' }
        }];
        
        var throughputLayout = {
            title: 'Throughput Performance',
            xaxis: { title: 'Test' },
            yaxis: { title: 'Throughput' }
        };
        
        Plotly.newPlot('throughputChart', throughputData, throughputLayout);
    </script>
</body>
</html>
)";
        
        report.close();
    }
    
signals:
    void benchmarkStarted();
    void progressUpdated(int current, int total);
    void benchmarkCompleted(const std::vector<BenchmarkResult>& results);
    
private:
    void calculateStatistics(const std::vector<double>& times, BenchmarkResult& result) {
        if (times.empty()) return;
        
        // 計算平均值
        double sum = std::accumulate(times.begin(), times.end(), 0.0);
        result.avgTime = sum / times.size();
        
        // 計算最小值和最大值
        result.minTime = *std::min_element(times.begin(), times.end());
        result.maxTime = *std::max_element(times.begin(), times.end());
        
        // 計算標準差
        double variance = 0;
        for (double time : times) {
            variance += std::pow(time - result.avgTime, 2);
        }
        variance /= times.size();
        result.stdDev = std::sqrt(variance);
        
        result.iterations = times.size();
    }
    
    std::string getTestNames(const std::vector<BenchmarkResult>& results) {
        std::string names;
        for (const auto& r : results) {
            if (!names.empty()) names += ", ";
            names += "'" + r.testName.toStdString() + "'";
        }
        return names;
    }
    
    std::string getAvgTimes(const std::vector<BenchmarkResult>& results) {
        std::string times;
        for (const auto& r : results) {
            if (!times.empty()) times += ", ";
            times += std::to_string(r.avgTime);
        }
        return times;
    }
    
    std::string getThroughputs(const std::vector<BenchmarkResult>& results) {
        std::string throughputs;
        for (const auto& r : results) {
            if (!throughputs.empty()) throughputs += ", ";
            throughputs += std::to_string(r.throughput);
        }
        return throughputs;
    }
};
```

### 11.2 即時監控儀表板

```cpp
// PerformanceMonitor.h
#pragma once
#include <QWidget>
#include <QTimer>
#include <QtCharts>
#include <deque>

class PerformanceMonitor : public QWidget {
    Q_OBJECT
    
public:
    PerformanceMonitor(QWidget* parent = nullptr) : QWidget(parent) {
        setupUI();
        setupCharts();
        
        // 更新計時器
        m_updateTimer = new QTimer(this);
        connect(m_updateTimer, &QTimer::timeout, this, &PerformanceMonitor::updateData);
        m_updateTimer->start(100); // 100ms更新一次
    }
    
private:
    void setupUI() {
        auto* layout = new QVBoxLayout(this);
        
        // FPS圖表
        m_fpsChart = new QChart();
        m_fpsChart->setTitle("Frame Rate");
        m_fpsChartView = new QChartView(m_fpsChart);
        layout->addWidget(m_fpsChartView);
        
        // CPU使用率圖表
        m_cpuChart = new QChart();
        m_cpuChart->setTitle("CPU Usage");
        m_cpuChartView = new QChartView(m_cpuChart);
        layout->addWidget(m_cpuChartView);
        
        // 記憶體使用圖表
        m_memoryChart = new QChart();
        m_memoryChart->setTitle("Memory Usage");
        m_memoryChartView = new QChartView(m_memoryChart);
        layout->addWidget(m_memoryChartView);
    }
    
    void setupCharts() {
        // FPS線條
        m_fpsSeries = new QLineSeries();
        m_fpsSeries->setName("FPS");
        m_fpsChart->addSeries(m_fpsSeries);
        
        // CPU線條
        m_cpuSeries = new QLineSeries();
        m_cpuSeries->setName("CPU %");
        m_cpuChart->addSeries(m_cpuSeries);
        
        // 記憶體線條
        m_memorySeries = new QLineSeries();
        m_memorySeries->setName("Memory MB");
        m_memoryChart->addSeries(m_memorySeries);
        
        // 設置坐標軸
        setupAxes(m_fpsChart, 0, 1000);
        setupAxes(m_cpuChart, 0, 100);
        setupAxes(m_memoryChart, 0, 4096);
    }
    
    void setupAxes(QChart* chart, int minY, int maxY) {
        auto* axisX = new QValueAxis();
        axisX->setRange(0, 60); // 60秒歷史
        axisX->setLabelFormat("%d");
        axisX->setTitleText("Time (s)");
        
        auto* axisY = new QValueAxis();
        axisY->setRange(minY, maxY);
        axisY->setLabelFormat("%d");
        
        chart->addAxis(axisX, Qt::AlignBottom);
        chart->addAxis(axisY, Qt::AlignLeft);
        
        for (auto* series : chart->series()) {
            series->attachAxis(axisX);
            series->attachAxis(axisY);
        }
    }
    
    void updateData() {
        static int timeCounter = 0;
        timeCounter++;
        
        double time = timeCounter * 0.1; // 秒
        
        // 獲取當前數據
        double fps = getCurrentFPS();
        double cpu = getCPUUsage();
        double memory = getMemoryUsage();
        
        // 更新數據點
        updateSeries(m_fpsSeries, time, fps, 600);
        updateSeries(m_cpuSeries, time, cpu, 600);
        updateSeries(m_memorySeries, time, memory, 600);
        
        // 更新圖表範圍
        if (time > 60) {
            updateAxisRange(m_fpsChart, time - 60, time);
            updateAxisRange(m_cpuChart, time - 60, time);
            updateAxisRange(m_memoryChart, time - 60, time);
        }
    }
    
    void updateSeries(QLineSeries* series, double x, double y, int maxPoints) {
        series->append(x, y);
        
        while (series->count() > maxPoints) {
            series->remove(0);
        }
    }
    
    void updateAxisRange(QChart* chart, double minX, double maxX) {
        for (auto* axis : chart->axes(Qt::Horizontal)) {
            if (auto* valueAxis = qobject_cast<QValueAxis*>(axis)) {
                valueAxis->setRange(minX, maxX);
            }
        }
    }
    
    double getCurrentFPS() {
        // 實際實現應該從FrameRateController獲取
        return 60.0 + (std::rand() % 20 - 10);
    }
    
    double getCPUUsage() {
#ifdef _WIN32
        static ULARGE_INTEGER lastCPU, lastSysCPU, lastUserCPU;
        static int numProcessors = 0;
        static HANDLE self = GetCurrentProcess();
        
        if (numProcessors == 0) {
            SYSTEM_INFO sysInfo;
            GetSystemInfo(&sysInfo);
            numProcessors = sysInfo.dwNumberOfProcessors;
        }
        
        FILETIME ftime, fsys, fuser;
        ULARGE_INTEGER now, sys, user;
        
        GetSystemTimeAsFileTime(&ftime);
        memcpy(&now, &ftime, sizeof(FILETIME));
        
        GetProcessTimes(self, &ftime, &ftime, &fsys, &fuser);
        memcpy(&sys, &fsys, sizeof(FILETIME));
        memcpy(&user, &fuser, sizeof(FILETIME));
        
        double percent = (sys.QuadPart - lastSysCPU.QuadPart) +
                        (user.QuadPart - lastUserCPU.QuadPart);
        percent /= (now.QuadPart - lastCPU.QuadPart);
        percent /= numProcessors;
        percent *= 100;
        
        lastCPU = now;
        lastUserCPU = user;
        lastSysCPU = sys;
        
        return percent;
#else
        return 0;
#endif
    }
    
    double getMemoryUsage() {
        auto stats = MemoryProfiler::instance().getCurrentStats();
        return stats.workingSetSize / (1024.0 * 1024.0);
    }
    
private:
    QTimer* m_updateTimer;
    
    QChart* m_fpsChart;
    QChart* m_cpuChart;
    QChart* m_memoryChart;
    
    QChartView* m_fpsChartView;
    QChartView* m_cpuChartView;
    QChartView* m_memoryChartView;
    
    QLineSeries* m_fpsSeries;
    QLineSeries* m_cpuSeries;
    QLineSeries* m_memorySeries;
};
```

---

## 總結

本指南提供了全面的Qt高效能優化策略，專門針對工業AOI設備的高fps需求（幾百到上千fps）。主要優化要點：

### 關鍵優化策略

1. **多執行緒架構**
   - 根據任務特性選擇合適的執行緒模型
   - 使用Lock-free數據結構減少競爭
   - 實現高效的執行緒池管理

2. **記憶體優化**
   - 實現記憶體池和物件池
   - 使用Zero-copy技術
   - 優化記憶體佈局和訪問模式

3. **並行處理**
   - 整合OpenMP和TBB
   - 實現Pipeline並行
   - 利用SIMD指令集

4. **GPU加速**
   - OpenGL/Vulkan渲染加速
   - CUDA計算加速
   - 混合CPU/GPU處理

5. **平台特定優化**
   - Windows IOCP和Thread Pool API
   - 高精度計時器
   - 處理器親和性設置

### 效能目標

- **幀率**: 500-1000+ fps（根據影像大小）
- **延遲**: < 10ms端到端處理
- **CPU使用率**: < 80%（留有餘地）
- **記憶體使用**: 可預測且穩定
- **抖動**: < 5ms幀時間變化

### 測試與監控

- 持續的效能分析和監控
- 自動化基準測試
- 即時效能儀表板
- 詳細的效能報告

### 最佳實踐

1. 始終先測量，再優化
2. 關注最大的瓶頸
3. 使用合適的工具和技術
4. 保持代碼的可維護性
5. 建立效能回歸測試

通過實施這些優化策略，可以顯著提升Qt應用程序在工業AOI設備中的效能表現，達到高fps的要求。