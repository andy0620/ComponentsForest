/**
 * @file qml_image_provider.cpp
 * @brief Implementation of efficient image provider for QML camera display
 */

#include "qml_image_provider.h"
#include <QDebug>
#include <QPainter>
#include <QDateTime>
#include <algorithm>

// ============================================================================
// QmlImageProvider Implementation
// ============================================================================

QmlImageProvider::QmlImageProvider(int cacheSize)
    : QQuickImageProvider(QQuickImageProvider::Image)
    , m_maxCacheSize(cacheSize * 1024 * 1024)  // Convert MB to bytes
    , m_currentCacheSize(0)
    , m_cachingEnabled(true)
    , m_preferredFormat(QImage::Format_RGB888)
{
    createPlaceholder();
}

QmlImageProvider::~QmlImageProvider()
{
    QMutexLocker locker(&m_mutex);
    m_cache.clear();
}

QImage QmlImageProvider::requestImage(const QString& id, QSize* size, const QSize& requestedSize)
{
    QMutexLocker locker(&m_mutex);
    
    // Update statistics
    m_stats.totalRequests++;
    
    // Check if ID is valid
    if (id.isEmpty()) {
        qWarning() << "QmlImageProvider: Empty image ID requested";
        if (size) *size = m_placeholderImage.size();
        return m_placeholderImage;
    }
    
    // Try to get from cache
    auto it = m_cache.find(id);
    if (it != m_cache.end() && it.value()) {
        m_stats.cacheHits++;
        updateAccessInfo(id);
        
        QImage result = it.value()->image;
        
        // Optimize if size requested
        if (requestedSize.isValid() && requestedSize != result.size()) {
            result = optimizeImage(result, requestedSize);
        }
        
        if (size) *size = result.size();
        m_stats.totalBytesDelivered += imageMemorySize(result);
        
        return result;
    }
    
    // Cache miss
    m_stats.cacheMisses++;
    
    // Parse ID to check if it's a valid camera/frame reference
    QString cameraId;
    qint64 frameNumber;
    if (parseId(id, cameraId, frameNumber)) {
        // Try to find the latest frame for this camera
        QString latestId = generateId(cameraId, m_frameNumbers.value(cameraId, -1));
        auto latestIt = m_cache.find(latestId);
        if (latestIt != m_cache.end() && latestIt.value()) {
            QImage result = latestIt.value()->image;
            if (requestedSize.isValid() && requestedSize != result.size()) {
                result = optimizeImage(result, requestedSize);
            }
            if (size) *size = result.size();
            m_stats.totalBytesDelivered += imageMemorySize(result);
            return result;
        }
    }
    
    // Return placeholder
    qDebug() << "QmlImageProvider: Image not found:" << id;
    if (size) *size = m_placeholderImage.size();
    return m_placeholderImage;
}

QPixmap QmlImageProvider::requestPixmap(const QString& id, QSize* size, const QSize& requestedSize)
{
    // Convert image request to pixmap
    QImage img = requestImage(id, size, requestedSize);
    return QPixmap::fromImage(img);
}

void QmlImageProvider::updateImage(const QString& cameraId, const QImage& image, 
                                  qint64 frameNumber, bool pin)
{
    if (!m_cachingEnabled || image.isNull()) {
        return;
    }
    
    QMutexLocker locker(&m_mutex);
    
    // Auto-increment frame number if not provided
    if (frameNumber < 0) {
        frameNumber = m_frameNumbers.value(cameraId, 0) + 1;
    }
    m_frameNumbers[cameraId] = frameNumber;
    
    QString id = generateId(cameraId, frameNumber);
    
    // Calculate image size
    qint64 imageSize = imageMemorySize(image);
    
    // Check if we need to evict images
    if (m_currentCacheSize + imageSize > m_maxCacheSize) {
        evictLRU(imageSize);
    }
    
    // Create cached entry
    auto cached = std::make_shared<CachedImage>();
    cached->image = image;
    cached->timestamp = QDateTime::currentMSecsSinceEpoch();
    cached->accessCount = 0;
    cached->isPinned = pin;
    
    // Convert to preferred format if needed
    if (image.format() != m_preferredFormat) {
        cached->image = image.convertToFormat(m_preferredFormat);
    }
    
    // Remove old entry if exists
    auto oldIt = m_cache.find(id);
    if (oldIt != m_cache.end() && oldIt.value()) {
        m_currentCacheSize -= imageMemorySize(oldIt.value()->image);
    }
    
    // Add to cache
    m_cache[id] = cached;
    m_currentCacheSize += imageSize;
    
    // Also store as "latest" for quick access
    QString latestId = cameraId + "/latest";
    m_cache[latestId] = cached;
    
    // Periodic cleanup
    if (++m_requestCounter >= CLEANUP_INTERVAL) {
        m_requestCounter = 0;
        // Clean up old access history
        qint64 cutoff = QDateTime::currentMSecsSinceEpoch() - 60000; // 1 minute
        m_accessHistory.erase(
            std::remove_if(m_accessHistory.begin(), m_accessHistory.end(),
                          [cutoff](const auto& pair) { return pair.first < cutoff; }),
            m_accessHistory.end()
        );
    }
}

void QmlImageProvider::clearCamera(const QString& cameraId)
{
    QMutexLocker locker(&m_mutex);
    
    // Find all images for this camera
    QStringList toRemove;
    for (auto it = m_cache.begin(); it != m_cache.end(); ++it) {
        if (it.key().startsWith(cameraId + "/")) {
            toRemove.append(it.key());
        }
    }
    
    // Remove found images
    for (const QString& id : toRemove) {
        auto it = m_cache.find(id);
        if (it != m_cache.end() && it.value()) {
            m_currentCacheSize -= imageMemorySize(it.value()->image);
            m_cache.erase(it);
        }
    }
    
    // Clear frame number
    m_frameNumbers.remove(cameraId);
}

void QmlImageProvider::clearAll()
{
    QMutexLocker locker(&m_mutex);
    m_cache.clear();
    m_frameNumbers.clear();
    m_accessHistory.clear();
    m_currentCacheSize = 0;
}

void QmlImageProvider::setPinned(const QString& id, bool pin)
{
    QMutexLocker locker(&m_mutex);
    auto it = m_cache.find(id);
    if (it != m_cache.end() && it.value()) {
        it.value()->isPinned = pin;
    }
}

qint64 QmlImageProvider::cacheSize() const
{
    QMutexLocker locker(&m_mutex);
    return m_currentCacheSize;
}

void QmlImageProvider::setMaxCacheSize(int sizeInMB)
{
    QMutexLocker locker(&m_mutex);
    m_maxCacheSize = sizeInMB * 1024 * 1024;
    
    // Evict if over new limit
    if (m_currentCacheSize > m_maxCacheSize) {
        evictLRU(m_currentCacheSize - m_maxCacheSize);
    }
}

QStringList QmlImageProvider::cachedImages() const
{
    QMutexLocker locker(&m_mutex);
    return m_cache.keys();
}

void QmlImageProvider::preloadImage(const QString& id, const QImage& image, bool pin)
{
    if (!m_cachingEnabled || image.isNull()) {
        return;
    }
    
    QMutexLocker locker(&m_mutex);
    
    qint64 imageSize = imageMemorySize(image);
    
    // Check if we need to evict images
    if (m_currentCacheSize + imageSize > m_maxCacheSize) {
        evictLRU(imageSize);
    }
    
    // Create cached entry
    auto cached = std::make_shared<CachedImage>();
    cached->image = image;
    cached->timestamp = QDateTime::currentMSecsSinceEpoch();
    cached->accessCount = 0;
    cached->isPinned = pin;
    
    // Convert to preferred format if needed
    if (image.format() != m_preferredFormat) {
        cached->image = image.convertToFormat(m_preferredFormat);
    }
    
    // Add to cache
    m_cache[id] = cached;
    m_currentCacheSize += imageSize;
}

QString QmlImageProvider::generateId(const QString& cameraId, qint64 frameNumber) const
{
    return QString("%1/%2").arg(cameraId).arg(frameNumber);
}

bool QmlImageProvider::parseId(const QString& id, QString& cameraId, qint64& frameNumber) const
{
    int slashIndex = id.lastIndexOf('/');
    if (slashIndex <= 0) {
        return false;
    }
    
    cameraId = id.left(slashIndex);
    QString frameStr = id.mid(slashIndex + 1);
    
    if (frameStr == "latest") {
        frameNumber = -1;
        return true;
    }
    
    bool ok;
    frameNumber = frameStr.toLongLong(&ok);
    return ok;
}

QImage QmlImageProvider::optimizeImage(const QImage& image, const QSize& requestedSize) const
{
    if (!requestedSize.isValid() || image.size() == requestedSize) {
        return image;
    }
    
    // Use smooth scaling for quality
    return image.scaled(requestedSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

qint64 QmlImageProvider::imageMemorySize(const QImage& image) const
{
    // Approximate memory usage
    return image.sizeInBytes();
}

void QmlImageProvider::evictLRU(qint64 requiredSpace)
{
    // Sort by access time (oldest first)
    QList<QPair<qint64, QString>> candidates;
    
    for (auto it = m_cache.begin(); it != m_cache.end(); ++it) {
        if (it.value() && !it.value()->isPinned) {
            candidates.append({it.value()->timestamp, it.key()});
        }
    }
    
    std::sort(candidates.begin(), candidates.end());
    
    qint64 freed = 0;
    for (const auto& pair : candidates) {
        if (freed >= requiredSpace) {
            break;
        }
        
        auto it = m_cache.find(pair.second);
        if (it != m_cache.end() && it.value()) {
            qint64 size = imageMemorySize(it.value()->image);
            m_cache.erase(it);
            m_currentCacheSize -= size;
            freed += size;
        }
    }
}

void QmlImageProvider::updateAccessInfo(const QString& id)
{
    auto it = m_cache.find(id);
    if (it != m_cache.end() && it.value()) {
        it.value()->timestamp = QDateTime::currentMSecsSinceEpoch();
        it.value()->accessCount++;
        m_accessHistory.append({it.value()->timestamp, id});
    }
}

void QmlImageProvider::createPlaceholder()
{
    // Create a simple placeholder image
    m_placeholderImage = QImage(640, 480, QImage::Format_RGB888);
    m_placeholderImage.fill(Qt::darkGray);
    
    // Draw placeholder text
    QPainter painter(&m_placeholderImage);
    painter.setPen(Qt::white);
    painter.setFont(QFont("Arial", 24));
    painter.drawText(m_placeholderImage.rect(), Qt::AlignCenter, "No Image");
    
    // Draw border
    painter.setPen(QPen(Qt::gray, 2));
    painter.drawRect(m_placeholderImage.rect().adjusted(1, 1, -1, -1));
}

// ============================================================================
// QmlImageProviderManager Implementation
// ============================================================================

QmlImageProviderManager& QmlImageProviderManager::instance()
{
    static QmlImageProviderManager instance;
    return instance;
}

QmlImageProvider* QmlImageProviderManager::provider()
{
    QMutexLocker locker(&m_mutex);
    if (!m_provider) {
        m_provider = new QmlImageProvider();
    }
    return m_provider;
}

void QmlImageProviderManager::setProvider(QmlImageProvider* provider)
{
    QMutexLocker locker(&m_mutex);
    if (m_provider && m_provider != provider) {
        delete m_provider;
    }
    m_provider = provider;
}

void QmlImageProviderManager::updateImage(const QString& cameraId, const QImage& image, qint64 frameNumber)
{
    QMutexLocker locker(&m_mutex);
    if (m_provider) {
        m_provider->updateImage(cameraId, image, frameNumber);
    }
}