/**
 * @file qml_image_provider.h
 * @brief Efficient image provider for QML camera display
 * 
 * This class provides high-performance image delivery to QML views
 * with support for caching, format conversion, and thread-safe access.
 */

#ifndef QML_IMAGE_PROVIDER_H
#define QML_IMAGE_PROVIDER_H

#include <QQuickImageProvider>
#include <QImage>
#include <QPixmap>
#include <QMutex>
#include <QHash>
#include <QCache>
#include <QElapsedTimer>
#include <atomic>
#include <memory>

/**
 * @brief Image statistics for performance monitoring
 */
struct ImageStatistics {
    std::atomic<quint64> totalRequests{0};
    std::atomic<quint64> cacheHits{0};
    std::atomic<quint64> cacheMisses{0};
    std::atomic<quint64> totalBytesDelivered{0};
    QElapsedTimer uptime;
    
    ImageStatistics() { uptime.start(); }
    
    double cacheHitRate() const {
        quint64 total = totalRequests.load();
        return total > 0 ? (cacheHits.load() * 100.0 / total) : 0.0;
    }
    
    double averageBytesPerRequest() const {
        quint64 total = totalRequests.load();
        return total > 0 ? (totalBytesDelivered.load() / double(total)) : 0.0;
    }
};

/**
 * @brief Cached image entry with metadata
 */
struct CachedImage {
    QImage image;
    QPixmap pixmap;
    qint64 timestamp;
    quint32 accessCount;
    bool isPinned;  // Pinned images are never evicted
    
    CachedImage() : timestamp(0), accessCount(0), isPinned(false) {}
};

/**
 * @brief High-performance image provider for QML
 * 
 * Features:
 * - Thread-safe image caching
 * - Automatic format conversion
 * - LRU cache eviction
 * - Performance statistics
 * - Support for pinned images (never evicted)
 * - Efficient memory management
 */
class QmlImageProvider : public QQuickImageProvider {
public:
    /**
     * @brief Constructor
     * @param cacheSize Maximum cache size in MB (default: 100MB)
     */
    explicit QmlImageProvider(int cacheSize = 100);
    
    /**
     * @brief Destructor
     */
    virtual ~QmlImageProvider();
    
    /**
     * @brief Request image from provider
     * @param id Image identifier (format: "cameraId/frameNumber")
     * @param size Requested size (used for optimization)
     * @param requestedSize Actual size of returned image
     * @return Requested image
     */
    QImage requestImage(const QString& id, QSize* size, const QSize& requestedSize) override;
    
    /**
     * @brief Request pixmap from provider (more efficient for display)
     * @param id Image identifier
     * @param size Requested size
     * @param requestedSize Actual size of returned pixmap
     * @return Requested pixmap
     */
    QPixmap requestPixmap(const QString& id, QSize* size, const QSize& requestedSize) override;
    
    /**
     * @brief Update image in cache
     * @param cameraId Camera identifier
     * @param image New image
     * @param frameNumber Frame number (optional, auto-increments if not provided)
     * @param pin If true, image won't be evicted from cache
     */
    void updateImage(const QString& cameraId, const QImage& image, 
                    qint64 frameNumber = -1, bool pin = false);
    
    /**
     * @brief Clear cache for specific camera
     * @param cameraId Camera identifier
     */
    void clearCamera(const QString& cameraId);
    
    /**
     * @brief Clear entire cache
     */
    void clearAll();
    
    /**
     * @brief Pin/unpin image in cache
     * @param id Image identifier
     * @param pin True to pin, false to unpin
     */
    void setPinned(const QString& id, bool pin);
    
    /**
     * @brief Get cache statistics
     * @return Current statistics
     */
    ImageStatistics statistics() const { return m_stats; }
    
    /**
     * @brief Get cache size in bytes
     * @return Current cache size
     */
    qint64 cacheSize() const;
    
    /**
     * @brief Set maximum cache size
     * @param sizeInMB Size in megabytes
     */
    void setMaxCacheSize(int sizeInMB);
    
    /**
     * @brief Enable/disable caching
     * @param enable True to enable caching
     */
    void setCachingEnabled(bool enable) { m_cachingEnabled = enable; }
    
    /**
     * @brief Check if caching is enabled
     * @return True if caching is enabled
     */
    bool isCachingEnabled() const { return m_cachingEnabled; }
    
    /**
     * @brief Set image format for optimization
     * @param format Preferred image format
     */
    void setPreferredFormat(QImage::Format format) { m_preferredFormat = format; }
    
    /**
     * @brief Get list of cached image IDs
     * @return List of image identifiers
     */
    QStringList cachedImages() const;
    
    /**
     * @brief Preload image into cache
     * @param id Image identifier  
     * @param image Image to preload
     * @param pin True to pin in cache
     */
    void preloadImage(const QString& id, const QImage& image, bool pin = false);
    
private:
    /**
     * @brief Generate image ID
     * @param cameraId Camera identifier
     * @param frameNumber Frame number
     * @return Combined identifier
     */
    QString generateId(const QString& cameraId, qint64 frameNumber) const;
    
    /**
     * @brief Parse image ID
     * @param id Combined identifier
     * @param cameraId Output camera ID
     * @param frameNumber Output frame number
     * @return True if parsing successful
     */
    bool parseId(const QString& id, QString& cameraId, qint64& frameNumber) const;
    
    /**
     * @brief Optimize image for display
     * @param image Source image
     * @param requestedSize Target size
     * @return Optimized image
     */
    QImage optimizeImage(const QImage& image, const QSize& requestedSize) const;
    
    /**
     * @brief Calculate image memory size
     * @param image Image to measure
     * @return Size in bytes
     */
    qint64 imageMemorySize(const QImage& image) const;
    
    /**
     * @brief Evict least recently used images
     * @param requiredSpace Space needed in bytes
     */
    void evictLRU(qint64 requiredSpace);
    
    /**
     * @brief Update access information
     * @param id Image identifier
     */
    void updateAccessInfo(const QString& id);
    
private:
    mutable QMutex m_mutex;                           // Thread safety
    QHash<QString, std::shared_ptr<CachedImage>> m_cache;  // Image cache
    QHash<QString, qint64> m_frameNumbers;            // Current frame numbers per camera
    QList<QPair<qint64, QString>> m_accessHistory;    // LRU tracking
    
    qint64 m_maxCacheSize;                            // Maximum cache size in bytes
    qint64 m_currentCacheSize;                        // Current cache size in bytes
    bool m_cachingEnabled;                            // Enable/disable caching
    QImage::Format m_preferredFormat;                 // Preferred image format
    
    mutable ImageStatistics m_stats;                  // Performance statistics
    
    // Performance optimization
    static constexpr int CLEANUP_INTERVAL = 100;      // Cleanup every N requests
    std::atomic<int> m_requestCounter{0};             // Request counter for cleanup
    
    // Default placeholder image
    QImage m_placeholderImage;
    
    /**
     * @brief Create placeholder image
     */
    void createPlaceholder();
};

/**
 * @brief Global image provider instance manager
 * 
 * Singleton pattern for managing the global image provider
 */
class QmlImageProviderManager {
public:
    /**
     * @brief Get singleton instance
     * @return Global instance
     */
    static QmlImageProviderManager& instance();
    
    /**
     * @brief Get or create image provider
     * @return Image provider instance
     */
    QmlImageProvider* provider();
    
    /**
     * @brief Set custom image provider
     * @param provider Custom provider instance
     */
    void setProvider(QmlImageProvider* provider);
    
    /**
     * @brief Update image for all QML views
     * @param cameraId Camera identifier
     * @param image New image
     * @param frameNumber Frame number
     */
    void updateImage(const QString& cameraId, const QImage& image, qint64 frameNumber = -1);
    
private:
    QmlImageProviderManager() = default;
    ~QmlImageProviderManager() = default;
    QmlImageProviderManager(const QmlImageProviderManager&) = delete;
    QmlImageProviderManager& operator=(const QmlImageProviderManager&) = delete;
    
    QmlImageProvider* m_provider = nullptr;
    QMutex m_mutex;
};

#endif // QML_IMAGE_PROVIDER_H