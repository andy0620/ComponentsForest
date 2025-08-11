#ifndef ROI_MANAGER_H
#define ROI_MANAGER_H

#include "roi_types.h"
#include <QObject>
#include <QReadWriteLock>
#include <QTimer>
#include <QFuture>
#include <QtConcurrent>
#include <unordered_map>
#include <list>

namespace ComponentsForest {
namespace OpenCV {

/**
 * @brief ROI template structure for saving/loading configurations
 */
struct ROITemplate {
    QString name;
    QString category;
    QString description;
    QDateTime created;
    std::vector<ROIPtr> rois;
    QJsonObject parameters;
    QImage thumbnail;
    
    QJsonObject toJson() const;
    static ROITemplate fromJson(const QJsonObject& json);
};

/**
 * @brief ROI group for logical organization
 */
class ROIGroup {
public:
    QString id;
    QString name;
    QColor color;
    bool visible{true};
    bool locked{false};
    std::vector<QString> roiIds;
    
    QJsonObject toJson() const;
    static ROIGroup fromJson(const QJsonObject& json);
};

/**
 * @brief Cache for ROI masks with LRU eviction
 */
class ROIMaskCache {
public:
    explicit ROIMaskCache(size_t maxSize = 100);
    
    cv::Mat getMask(const QString& roiId, const cv::Size& frameSize,
                    std::function<cv::Mat()> generator);
    void invalidate(const QString& roiId);
    void clear();
    
    size_t size() const { return m_cache.size(); }
    size_t maxSize() const { return m_maxSize; }
    void setMaxSize(size_t size);
    
private:
    struct CacheKey {
        QString roiId;
        cv::Size frameSize;
        
        bool operator==(const CacheKey& other) const {
            return roiId == other.roiId && 
                   frameSize.width == other.frameSize.width &&
                   frameSize.height == other.frameSize.height;
        }
    };
    
    struct CacheKeyHash {
        size_t operator()(const CacheKey& key) const {
            return qHash(key.roiId) ^ 
                   (std::hash<int>()(key.frameSize.width) << 1) ^
                   (std::hash<int>()(key.frameSize.height) << 2);
        }
    };
    
    std::unordered_map<CacheKey, cv::Mat, CacheKeyHash> m_cache;
    std::list<CacheKey> m_lruList;
    std::unordered_map<CacheKey, std::list<CacheKey>::iterator, CacheKeyHash> m_lruMap;
    size_t m_maxSize;
    mutable QReadWriteLock m_lock;
    
    void evictLRU();
    void updateLRU(const CacheKey& key);
};

/**
 * @brief GPU accelerator for ROI operations
 */
class ROIGPUAccelerator {
public:
    ROIGPUAccelerator();
    ~ROIGPUAccelerator();
    
    bool isAvailable() const { return m_available; }
    
    // GPU-accelerated extraction
    std::vector<cv::cuda::GpuMat> extractROIBatch(
        const cv::cuda::GpuMat& source,
        const std::vector<ROIPtr>& rois);
    
    // GPU-accelerated mask generation
    cv::cuda::GpuMat generateMask(const ROIPtr& roi, const cv::Size& size);
    
    // GPU-accelerated mask application
    void applyMasks(cv::cuda::GpuMat& target,
                    const std::vector<cv::cuda::GpuMat>& masks,
                    const std::vector<cv::cuda::GpuMat>& roiData);
    
private:
    bool m_available{false};
    cv::cuda::Stream m_stream;
    
    void checkGPUAvailability();
};

/**
 * @brief Main ROI Manager class
 */
class ROIManager : public QObject {
    Q_OBJECT
    
public:
    explicit ROIManager(QObject* parent = nullptr);
    ~ROIManager();
    
    // ROI management
    void addROI(ROIPtr roi);
    void removeROI(const QString& id);
    void updateROI(const QString& id, ROIPtr roi);
    ROIPtr getROI(const QString& id) const;
    std::vector<ROIPtr> getAllROIs() const;
    std::vector<ROIPtr> getActiveROIs() const;
    void clear();
    
    // ROI state
    void setROIEnabled(const QString& id, bool enabled);
    bool isROIEnabled(const QString& id) const;
    
    // Selection
    void selectROI(const QString& id, bool selected = true);
    void deselectROI(const QString& id);
    void selectAll();
    void deselectAll();
    std::vector<QString> getSelectedROIs() const;
    
    // Groups
    void createGroup(const QString& name, const std::vector<QString>& roiIds);
    void deleteGroup(const QString& groupId);
    void addToGroup(const QString& groupId, const QString& roiId);
    void removeFromGroup(const QString& groupId, const QString& roiId);
    std::vector<ROIGroup> getGroups() const;
    
    // Templates
    void saveTemplate(const ROITemplate& template_);
    void loadTemplate(const QString& templateName);
    void deleteTemplate(const QString& templateName);
    std::vector<ROITemplate> getTemplates() const;
    
    // Processing
    cv::Mat processFrame(const cv::Mat& frame);
    QFuture<std::vector<cv::Mat>> processROIsParallel(
        const cv::Mat& frame,
        const std::vector<ROIPtr>& rois);
    
    // Tracking
    void enableTracking(const QString& roiId, TrackingMethod method);
    void disableTracking(const QString& roiId);
    void updateTracking(const cv::Mat& frame);
    
    // Serialization
    void save(const QString& filename);
    void load(const QString& filename);
    QJsonObject toJson() const;
    void fromJson(const QJsonObject& json);
    
    // Cache management
    ROIMaskCache& maskCache() { return m_maskCache; }
    void clearCache() { m_maskCache.clear(); }
    
    // GPU acceleration
    bool isGPUAvailable() const { return m_gpuAccelerator.isAvailable(); }
    void setUseGPU(bool use);
    bool useGPU() const { return m_useGPU && m_gpuAccelerator.isAvailable(); }
    
    // Statistics
    struct Statistics {
        int totalROIs;
        int activeROIs;
        int selectedROIs;
        int trackingROIs;
        double avgProcessingTime;
        size_t cacheHits;
        size_t cacheMisses;
    };
    Statistics getStatistics() const;
    
    // Hit testing
    ROIPtr hitTest(const cv::Point2f& point) const;
    std::vector<ROIPtr> hitTestAll(const cv::Point2f& point) const;
    
    // Arrangement
    void bringToFront(const QString& roiId);
    void sendToBack(const QString& roiId);
    void bringForward(const QString& roiId);
    void sendBackward(const QString& roiId);
    
signals:
    // ROI signals
    void roiAdded(const ROIPtr& roi);
    void roiRemoved(const QString& id);
    void roiModified(const QString& id, const ROIPtr& roi);
    void roiEnabledChanged(const QString& id, bool enabled);
    
    // Selection signals
    void roiSelected(const QString& id);
    void roiDeselected(const QString& id);
    void selectionChanged(const std::vector<QString>& selectedIds);
    
    // Group signals
    void groupCreated(const ROIGroup& group);
    void groupDeleted(const QString& groupId);
    void groupModified(const QString& groupId);
    
    // Template signals
    void templateSaved(const ROITemplate& template_);
    void templateLoaded(const QString& templateName);
    void templateDeleted(const QString& templateName);
    
    // Tracking signals
    void trackingUpdated(const QString& roiId, const cv::Rect2f& newBounds);
    void trackingLost(const QString& roiId);
    void trackingRecovered(const QString& roiId);
    
    // Processing signals
    void processingStarted();
    void processingProgress(int current, int total);
    void processingCompleted();
    
public slots:
    // Batch operations
    void enableROIs(const std::vector<QString>& ids);
    void disableROIs(const std::vector<QString>& ids);
    void deleteROIs(const std::vector<QString>& ids);
    
    // Auto-save
    void enableAutoSave(const QString& filename, int intervalMs = 60000);
    void disableAutoSave();
    
private:
    // ROI storage
    std::unordered_map<QString, ROIPtr> m_rois;
    std::vector<QString> m_roiOrder;  // Z-order
    std::unordered_set<QString> m_selectedROIs;
    
    // Groups
    std::unordered_map<QString, ROIGroup> m_groups;
    
    // Templates
    std::unordered_map<QString, ROITemplate> m_templates;
    
    // Caching
    ROIMaskCache m_maskCache;
    
    // GPU acceleration
    ROIGPUAccelerator m_gpuAccelerator;
    bool m_useGPU{true};
    
    // Thread safety
    mutable QReadWriteLock m_lock;
    
    // Auto-save
    QTimer* m_autoSaveTimer{nullptr};
    QString m_autoSaveFilename;
    
    // Statistics
    mutable Statistics m_statistics;
    QElapsedTimer m_processingTimer;
    
    // Helper methods
    void connectROISignals(ROIPtr roi);
    void disconnectROISignals(ROIPtr roi);
    cv::Mat processROI(const cv::Mat& frame, ROIPtr roi);
    void updateStatistics() const;
    
private slots:
    void onROIModified();
    void onAutoSave();
};

/**
 * @brief ROI processor for parallel processing
 */
class ROIProcessor {
public:
    struct Result {
        QString roiId;
        cv::Mat processedData;
        double processingTime;
        bool success;
        QString error;
    };
    
    static Result processROI(const cv::Mat& frame, ROIPtr roi);
    static std::vector<Result> processROIBatch(
        const cv::Mat& frame,
        const std::vector<ROIPtr>& rois);
    
    // Async processing
    static QFuture<Result> processROIAsync(const cv::Mat& frame, ROIPtr roi);
    static QFuture<std::vector<Result>> processROIBatchAsync(
        const cv::Mat& frame,
        const std::vector<ROIPtr>& rois);
};

/**
 * @brief ROI import/export utilities
 */
class ROIImportExport {
public:
    // File formats
    enum class Format {
        JSON,
        XML,
        Binary,
        CSV
    };
    
    // Export
    static void exportROIs(const std::vector<ROIPtr>& rois,
                          const QString& filename,
                          Format format = Format::JSON);
    
    static QByteArray exportROIsToBuffer(const std::vector<ROIPtr>& rois,
                                        Format format = Format::JSON);
    
    // Import
    static std::vector<ROIPtr> importROIs(const QString& filename,
                                         Format format = Format::JSON);
    
    static std::vector<ROIPtr> importROIsFromBuffer(const QByteArray& data,
                                                   Format format = Format::JSON);
    
    // Format conversion
    static QByteArray convertFormat(const QByteArray& data,
                                   Format from,
                                   Format to);
};

} // namespace OpenCV
} // namespace ComponentsForest

#endif // ROI_MANAGER_H