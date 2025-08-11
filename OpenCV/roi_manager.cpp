#include "roi_manager.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QFile>
#include <QDebug>
#include <QtConcurrent>
#include <QRandomGenerator>
#ifdef HAS_CUDA
#include <opencv2/cudaarithm.hpp>
#include <opencv2/cudaimgproc.hpp>
#endif

namespace ComponentsForest {
namespace OpenCV {

// ============ ROITemplate Implementation ============

QJsonObject ROITemplate::toJson() const {
    QJsonObject json;
    json["name"] = name;
    json["category"] = category;
    json["description"] = description;
    json["created"] = created.toString(Qt::ISODate);
    json["parameters"] = parameters;
    
    QJsonArray roisArray;
    for (const auto& roi : rois) {
        if (roi) {
            roisArray.append(roi->toJson());
        }
    }
    json["rois"] = roisArray;
    
    // Convert thumbnail to base64
    if (!thumbnail.isNull()) {
        QByteArray ba;
        QBuffer buffer(&ba);
        buffer.open(QIODevice::WriteOnly);
        thumbnail.save(&buffer, "PNG");
        json["thumbnail"] = QString::fromLatin1(ba.toBase64());
    }
    
    return json;
}

ROITemplate ROITemplate::fromJson(const QJsonObject& json) {
    ROITemplate template_;
    
    template_.name = json["name"].toString();
    template_.category = json["category"].toString();
    template_.description = json["description"].toString();
    template_.created = QDateTime::fromString(json["created"].toString(), Qt::ISODate);
    template_.parameters = json["parameters"].toObject();
    
    QJsonArray roisArray = json["rois"].toArray();
    for (const auto& value : roisArray) {
        auto roi = ROIFactory::createFromJson(value.toObject());
        if (roi) {
            template_.rois.push_back(roi);
        }
    }
    
    // Decode thumbnail from base64
    if (json.contains("thumbnail")) {
        QByteArray ba = QByteArray::fromBase64(json["thumbnail"].toString().toLatin1());
        template_.thumbnail.loadFromData(ba, "PNG");
    }
    
    return template_;
}

// ============ ROIGroup Implementation ============

QJsonObject ROIGroup::toJson() const {
    QJsonObject json;
    json["id"] = id;
    json["name"] = name;
    json["color"] = color.name();
    json["visible"] = visible;
    json["locked"] = locked;
    
    QJsonArray idsArray;
    for (const auto& roiId : roiIds) {
        idsArray.append(roiId);
    }
    json["roiIds"] = idsArray;
    
    return json;
}

ROIGroup ROIGroup::fromJson(const QJsonObject& json) {
    ROIGroup group;
    
    group.id = json["id"].toString();
    group.name = json["name"].toString();
    group.color = QColor(json["color"].toString());
    group.visible = json["visible"].toBool(true);
    group.locked = json["locked"].toBool(false);
    
    QJsonArray idsArray = json["roiIds"].toArray();
    for (const auto& value : idsArray) {
        group.roiIds.push_back(value.toString());
    }
    
    return group;
}

// ============ ROIMaskCache Implementation ============

ROIMaskCache::ROIMaskCache(size_t maxSize) : m_maxSize(maxSize) {}

cv::Mat ROIMaskCache::getMask(const QString& roiId, const cv::Size& frameSize,
                              std::function<cv::Mat()> generator) {
    QWriteLocker locker(&m_lock);
    
    CacheKey key{roiId, frameSize};
    
    auto it = m_cache.find(key);
    if (it != m_cache.end()) {
        // Cache hit - update LRU
        updateLRU(key);
        return it->second;
    }
    
    // Cache miss - generate mask
    cv::Mat mask = generator();
    
    // Add to cache
    if (m_cache.size() >= m_maxSize) {
        evictLRU();
    }
    
    m_cache[key] = mask;
    m_lruList.push_front(key);
    m_lruMap[key] = m_lruList.begin();
    
    return mask;
}

void ROIMaskCache::invalidate(const QString& roiId) {
    QWriteLocker locker(&m_lock);
    
    // Remove all entries for this ROI
    auto it = m_cache.begin();
    while (it != m_cache.end()) {
        if (it->first.roiId == roiId) {
            auto lruIt = m_lruMap.find(it->first);
            if (lruIt != m_lruMap.end()) {
                m_lruList.erase(lruIt->second);
                m_lruMap.erase(lruIt);
            }
            it = m_cache.erase(it);
        } else {
            ++it;
        }
    }
}

void ROIMaskCache::clear() {
    QWriteLocker locker(&m_lock);
    m_cache.clear();
    m_lruList.clear();
    m_lruMap.clear();
}

void ROIMaskCache::setMaxSize(size_t size) {
    QWriteLocker locker(&m_lock);
    m_maxSize = size;
    
    while (m_cache.size() > m_maxSize) {
        evictLRU();
    }
}

void ROIMaskCache::evictLRU() {
    if (m_lruList.empty()) return;
    
    CacheKey key = m_lruList.back();
    m_lruList.pop_back();
    m_lruMap.erase(key);
    m_cache.erase(key);
}

void ROIMaskCache::updateLRU(const CacheKey& key) {
    auto it = m_lruMap.find(key);
    if (it != m_lruMap.end()) {
        m_lruList.erase(it->second);
        m_lruList.push_front(key);
        m_lruMap[key] = m_lruList.begin();
    }
}

// ============ ROIGPUAccelerator Implementation ============

ROIGPUAccelerator::ROIGPUAccelerator() {
    checkGPUAvailability();
    if (m_available) {
        m_stream = cv::cuda::Stream();
    }
}

ROIGPUAccelerator::~ROIGPUAccelerator() {}

std::vector<cv::cuda::GpuMat> ROIGPUAccelerator::extractROIBatch(
    const cv::cuda::GpuMat& source,
    const std::vector<ROIPtr>& rois) {
    
    std::vector<cv::cuda::GpuMat> results;
    
    if (!m_available || source.empty()) {
        return results;
    }
    
    for (const auto& roi : rois) {
        if (!roi || !roi->isEnabled()) continue;
        
        cv::cuda::GpuMat mask = generateMask(roi, source.size());
        cv::cuda::GpuMat extracted;
        
#ifdef HAS_CUDA
        cv::cuda::bitwise_and(source, source, extracted, mask, m_stream);
#else
        // Fallback to CPU implementation when CUDA not available
        qWarning() << "CUDA not available, falling back to CPU processing";
        cv::Mat cpuSource, cpuMask, cpuExtracted;
        source.download(cpuSource);
        mask.download(cpuMask);
        cv::bitwise_and(cpuSource, cpuSource, cpuExtracted, cpuMask);
        extracted.upload(cpuExtracted);
#endif
        results.push_back(extracted);
    }
    
    m_stream.waitForCompletion();
    return results;
}

cv::cuda::GpuMat ROIGPUAccelerator::generateMask(const ROIPtr& roi, const cv::Size& size) {
    if (!m_available || !roi) {
        return cv::cuda::GpuMat();
    }
    
    // Generate mask on CPU and upload to GPU
    cv::Mat cpuMask = roi->getMask(size);
    cv::cuda::GpuMat gpuMask;
    gpuMask.upload(cpuMask, m_stream);
    
    return gpuMask;
}

void ROIGPUAccelerator::applyMasks(cv::cuda::GpuMat& target,
                                   const std::vector<cv::cuda::GpuMat>& masks,
                                   const std::vector<cv::cuda::GpuMat>& roiData) {
    if (!m_available || target.empty()) return;
    
    size_t count = std::min(masks.size(), roiData.size());
    
    for (size_t i = 0; i < count; ++i) {
        if (masks[i].empty() || roiData[i].empty()) continue;
        
        cv::cuda::GpuMat temp;
#ifdef HAS_CUDA
        cv::cuda::bitwise_and(roiData[i], roiData[i], temp, masks[i], m_stream);
        cv::cuda::add(target, temp, target, cv::cuda::GpuMat(), -1, m_stream);
#else
        // Fallback to CPU implementation
        cv::Mat cpuRoi, cpuMask, cpuTemp, cpuTarget;
        roiData[i].download(cpuRoi);
        masks[i].download(cpuMask);
        target.download(cpuTarget);
        cv::bitwise_and(cpuRoi, cpuRoi, cpuTemp, cpuMask);
        cv::add(cpuTarget, cpuTemp, cpuTarget);
        target.upload(cpuTarget);
#endif
    }
    
    m_stream.waitForCompletion();
}

void ROIGPUAccelerator::checkGPUAvailability() {
    m_available = cv::cuda::getCudaEnabledDeviceCount() > 0;
    
    if (m_available) {
        cv::cuda::DeviceInfo deviceInfo;
        qDebug() << "GPU Device:" << QString::fromStdString(deviceInfo.name());
        qDebug() << "CUDA Version:" << deviceInfo.majorVersion() << "." << deviceInfo.minorVersion();
    }
}

// ============ ROIManager Implementation ============

ROIManager::ROIManager(QObject* parent) 
    : QObject(parent), m_maskCache(100) {
    
    m_statistics = {};
}

ROIManager::~ROIManager() {
    if (m_autoSaveTimer) {
        m_autoSaveTimer->stop();
        delete m_autoSaveTimer;
    }
}

void ROIManager::addROI(ROIPtr roi) {
    if (!roi) return;
    
    QWriteLocker locker(&m_lock);
    
    QString id = roi->getId();
    m_rois[id] = roi;
    m_roiOrder.push_back(id);
    
    connectROISignals(roi);
    
    locker.unlock();
    
    emit roiAdded(roi);
    updateStatistics();
}

void ROIManager::removeROI(const QString& id) {
    QWriteLocker locker(&m_lock);
    
    auto it = m_rois.find(id);
    if (it == m_rois.end()) return;
    
    disconnectROISignals(it->second);
    m_rois.erase(it);
    
    auto orderIt = std::find(m_roiOrder.begin(), m_roiOrder.end(), id);
    if (orderIt != m_roiOrder.end()) {
        m_roiOrder.erase(orderIt);
    }
    
    m_selectedROIs.erase(id);
    
    // Remove from groups
    for (auto& [groupId, group] : m_groups) {
        auto roiIt = std::find(group.roiIds.begin(), group.roiIds.end(), id);
        if (roiIt != group.roiIds.end()) {
            group.roiIds.erase(roiIt);
        }
    }
    
    m_maskCache.invalidate(id);
    
    locker.unlock();
    
    emit roiRemoved(id);
    updateStatistics();
}

void ROIManager::updateROI(const QString& id, ROIPtr roi) {
    if (!roi) return;
    
    QWriteLocker locker(&m_lock);
    
    auto it = m_rois.find(id);
    if (it == m_rois.end()) return;
    
    disconnectROISignals(it->second);
    it->second = roi;
    connectROISignals(roi);
    
    m_maskCache.invalidate(id);
    
    locker.unlock();
    
    emit roiModified(id, roi);
    updateStatistics();
}

ROIPtr ROIManager::getROI(const QString& id) const {
    QReadLocker locker(&m_lock);
    
    auto it = m_rois.find(id);
    return (it != m_rois.end()) ? it->second : nullptr;
}

std::vector<ROIPtr> ROIManager::getAllROIs() const {
    QReadLocker locker(&m_lock);
    
    std::vector<ROIPtr> result;
    for (const auto& id : m_roiOrder) {
        auto it = m_rois.find(id);
        if (it != m_rois.end()) {
            result.push_back(it->second);
        }
    }
    
    return result;
}

std::vector<ROIPtr> ROIManager::getActiveROIs() const {
    QReadLocker locker(&m_lock);
    
    std::vector<ROIPtr> result;
    for (const auto& id : m_roiOrder) {
        auto it = m_rois.find(id);
        if (it != m_rois.end() && it->second->isEnabled()) {
            result.push_back(it->second);
        }
    }
    
    return result;
}

void ROIManager::clear() {
    QWriteLocker locker(&m_lock);
    
    for (auto& [id, roi] : m_rois) {
        disconnectROISignals(roi);
    }
    
    m_rois.clear();
    m_roiOrder.clear();
    m_selectedROIs.clear();
    m_groups.clear();
    m_maskCache.clear();
    
    locker.unlock();
    
    updateStatistics();
}

void ROIManager::setROIEnabled(const QString& id, bool enabled) {
    QReadLocker locker(&m_lock);
    
    auto it = m_rois.find(id);
    if (it != m_rois.end()) {
        it->second->setEnabled(enabled);
        locker.unlock();
        
        emit roiEnabledChanged(id, enabled);
        updateStatistics();
    }
}

bool ROIManager::isROIEnabled(const QString& id) const {
    QReadLocker locker(&m_lock);
    
    auto it = m_rois.find(id);
    return (it != m_rois.end()) ? it->second->isEnabled() : false;
}

void ROIManager::selectROI(const QString& id, bool selected) {
    QWriteLocker locker(&m_lock);
    
    if (selected) {
        m_selectedROIs.insert(id);
        
        auto it = m_rois.find(id);
        if (it != m_rois.end()) {
            it->second->setVisualState(ROIVisualState::Selected);
        }
    } else {
        m_selectedROIs.erase(id);
        
        auto it = m_rois.find(id);
        if (it != m_rois.end()) {
            it->second->setVisualState(ROIVisualState::Normal);
        }
    }
    
    locker.unlock();
    
    if (selected) {
        emit roiSelected(id);
    } else {
        emit roiDeselected(id);
    }
    
    emit selectionChanged(getSelectedROIs());
    updateStatistics();
}

void ROIManager::deselectROI(const QString& id) {
    selectROI(id, false);
}

void ROIManager::selectAll() {
    QWriteLocker locker(&m_lock);
    
    m_selectedROIs.clear();
    for (const auto& [id, roi] : m_rois) {
        m_selectedROIs.insert(id);
        roi->setVisualState(ROIVisualState::Selected);
    }
    
    locker.unlock();
    
    emit selectionChanged(getSelectedROIs());
    updateStatistics();
}

void ROIManager::deselectAll() {
    QWriteLocker locker(&m_lock);
    
    for (const auto& id : m_selectedROIs) {
        auto it = m_rois.find(id);
        if (it != m_rois.end()) {
            it->second->setVisualState(ROIVisualState::Normal);
        }
    }
    
    m_selectedROIs.clear();
    
    locker.unlock();
    
    emit selectionChanged(std::vector<QString>());
    updateStatistics();
}

std::vector<QString> ROIManager::getSelectedROIs() const {
    QReadLocker locker(&m_lock);
    return std::vector<QString>(m_selectedROIs.begin(), m_selectedROIs.end());
}

void ROIManager::createGroup(const QString& name, const std::vector<QString>& roiIds) {
    QWriteLocker locker(&m_lock);
    
    ROIGroup group;
    group.id = QUuid::createUuid().toString();
    group.name = name;
    group.color = QColor::fromHsv(QRandomGenerator::global()->bounded(360), 200, 200);
    group.roiIds = roiIds;
    
    m_groups[group.id] = group;
    
    locker.unlock();
    
    emit groupCreated(group);
}

void ROIManager::deleteGroup(const QString& groupId) {
    QWriteLocker locker(&m_lock);
    
    m_groups.erase(groupId);
    
    locker.unlock();
    
    emit groupDeleted(groupId);
}

void ROIManager::addToGroup(const QString& groupId, const QString& roiId) {
    QWriteLocker locker(&m_lock);
    
    auto it = m_groups.find(groupId);
    if (it != m_groups.end()) {
        if (std::find(it->second.roiIds.begin(), it->second.roiIds.end(), roiId) 
            == it->second.roiIds.end()) {
            it->second.roiIds.push_back(roiId);
            
            locker.unlock();
            emit groupModified(groupId);
        }
    }
}

void ROIManager::removeFromGroup(const QString& groupId, const QString& roiId) {
    QWriteLocker locker(&m_lock);
    
    auto it = m_groups.find(groupId);
    if (it != m_groups.end()) {
        auto roiIt = std::find(it->second.roiIds.begin(), it->second.roiIds.end(), roiId);
        if (roiIt != it->second.roiIds.end()) {
            it->second.roiIds.erase(roiIt);
            
            locker.unlock();
            emit groupModified(groupId);
        }
    }
}

std::vector<ROIGroup> ROIManager::getGroups() const {
    QReadLocker locker(&m_lock);
    
    std::vector<ROIGroup> result;
    for (const auto& [id, group] : m_groups) {
        result.push_back(group);
    }
    
    return result;
}

void ROIManager::saveTemplate(const ROITemplate& template_) {
    QWriteLocker locker(&m_lock);
    
    m_templates[template_.name] = template_;
    
    locker.unlock();
    
    emit templateSaved(template_);
}

void ROIManager::loadTemplate(const QString& templateName) {
    QReadLocker locker(&m_lock);
    
    auto it = m_templates.find(templateName);
    if (it == m_templates.end()) return;
    
    ROITemplate template_ = it->second;
    
    locker.unlock();
    
    // Add ROIs from template
    for (const auto& roi : template_.rois) {
        if (roi) {
            auto cloned = roi->clone();
            addROI(cloned);
        }
    }
    
    emit templateLoaded(templateName);
}

void ROIManager::deleteTemplate(const QString& templateName) {
    QWriteLocker locker(&m_lock);
    
    m_templates.erase(templateName);
    
    locker.unlock();
    
    emit templateDeleted(templateName);
}

std::vector<ROITemplate> ROIManager::getTemplates() const {
    QReadLocker locker(&m_lock);
    
    std::vector<ROITemplate> result;
    for (const auto& [name, template_] : m_templates) {
        result.push_back(template_);
    }
    
    return result;
}

cv::Mat ROIManager::processFrame(const cv::Mat& frame) {
    if (frame.empty()) return frame;
    
    m_processingTimer.start();
    emit processingStarted();
    
    cv::Mat result = frame.clone();
    auto activeROIs = getActiveROIs();
    
    int current = 0;
    for (const auto& roi : activeROIs) {
        cv::Mat processed = processROI(frame, roi);
        if (!processed.empty()) {
            roi->apply(result, processed);
        }
        
        current++;
        emit processingProgress(current, activeROIs.size());
    }
    
    double elapsed = m_processingTimer.elapsed();
    m_statistics.avgProcessingTime = 
        (m_statistics.avgProcessingTime * 0.9) + (elapsed * 0.1);
    
    emit processingCompleted();
    
    return result;
}

QFuture<std::vector<cv::Mat>> ROIManager::processROIsParallel(
    const cv::Mat& frame,
    const std::vector<ROIPtr>& rois) {
    
    return QtConcurrent::run([this, frame, rois]() {
        std::vector<cv::Mat> results;
        
        for (const auto& roi : rois) {
            if (roi && roi->isEnabled()) {
                results.push_back(processROI(frame, roi));
            }
        }
        
        return results;
    });
}

void ROIManager::enableTracking(const QString& roiId, TrackingMethod method) {
    QReadLocker locker(&m_lock);
    
    auto it = m_rois.find(roiId);
    if (it != m_rois.end()) {
        if (it->second->getType() == ROIType::Dynamic) {
            auto dynamicROI = std::dynamic_pointer_cast<DynamicROI>(it->second);
            if (dynamicROI) {
                dynamicROI->enableTracking(method);
            }
        }
    }
}

void ROIManager::disableTracking(const QString& roiId) {
    QReadLocker locker(&m_lock);
    
    auto it = m_rois.find(roiId);
    if (it != m_rois.end()) {
        if (it->second->getType() == ROIType::Dynamic) {
            auto dynamicROI = std::dynamic_pointer_cast<DynamicROI>(it->second);
            if (dynamicROI) {
                dynamicROI->disableTracking();
            }
        }
    }
}

void ROIManager::updateTracking(const cv::Mat& frame) {
    auto rois = getAllROIs();
    
    for (const auto& roi : rois) {
        if (roi && roi->metadata().trackingEnabled) {
            if (roi->getType() == ROIType::Dynamic) {
                auto dynamicROI = std::dynamic_pointer_cast<DynamicROI>(roi);
                if (dynamicROI) {
                    dynamicROI->updateTracking(frame);
                }
            }
        }
    }
}

void ROIManager::save(const QString& filename) {
    QJsonObject json = toJson();
    QJsonDocument doc(json);
    
    QFile file(filename);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        file.close();
    }
}

void ROIManager::load(const QString& filename) {
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) return;
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonDocument doc = QJsonDocument::fromJson(data);
    fromJson(doc.object());
}

QJsonObject ROIManager::toJson() const {
    QReadLocker locker(&m_lock);
    
    QJsonObject json;
    
    // ROIs
    QJsonArray roisArray;
    for (const auto& id : m_roiOrder) {
        auto it = m_rois.find(id);
        if (it != m_rois.end()) {
            roisArray.append(it->second->toJson());
        }
    }
    json["rois"] = roisArray;
    
    // Groups
    QJsonArray groupsArray;
    for (const auto& [id, group] : m_groups) {
        groupsArray.append(group.toJson());
    }
    json["groups"] = groupsArray;
    
    // Templates
    QJsonArray templatesArray;
    for (const auto& [name, template_] : m_templates) {
        templatesArray.append(template_.toJson());
    }
    json["templates"] = templatesArray;
    
    // Settings
    json["useGPU"] = m_useGPU;
    json["cacheSize"] = static_cast<int>(m_maskCache.maxSize());
    
    return json;
}

void ROIManager::fromJson(const QJsonObject& json) {
    clear();
    
    QWriteLocker locker(&m_lock);
    
    // ROIs
    QJsonArray roisArray = json["rois"].toArray();
    for (const auto& value : roisArray) {
        auto roi = ROIFactory::createFromJson(value.toObject());
        if (roi) {
            QString id = roi->getId();
            m_rois[id] = roi;
            m_roiOrder.push_back(id);
            connectROISignals(roi);
        }
    }
    
    // Groups
    QJsonArray groupsArray = json["groups"].toArray();
    for (const auto& value : groupsArray) {
        ROIGroup group = ROIGroup::fromJson(value.toObject());
        m_groups[group.id] = group;
    }
    
    // Templates
    QJsonArray templatesArray = json["templates"].toArray();
    for (const auto& value : templatesArray) {
        ROITemplate template_ = ROITemplate::fromJson(value.toObject());
        m_templates[template_.name] = template_;
    }
    
    // Settings
    m_useGPU = json["useGPU"].toBool(true);
    m_maskCache.setMaxSize(json["cacheSize"].toInt(100));
    
    locker.unlock();
    
    updateStatistics();
}

void ROIManager::setUseGPU(bool use) {
    m_useGPU = use && m_gpuAccelerator.isAvailable();
}

ROIManager::Statistics ROIManager::getStatistics() const {
    return m_statistics;
}

ROIPtr ROIManager::hitTest(const cv::Point2f& point) const {
    QReadLocker locker(&m_lock);
    
    // Test in reverse order (top to bottom)
    for (auto it = m_roiOrder.rbegin(); it != m_roiOrder.rend(); ++it) {
        auto roiIt = m_rois.find(*it);
        if (roiIt != m_rois.end() && roiIt->second->contains(point)) {
            return roiIt->second;
        }
    }
    
    return nullptr;
}

std::vector<ROIPtr> ROIManager::hitTestAll(const cv::Point2f& point) const {
    QReadLocker locker(&m_lock);
    
    std::vector<ROIPtr> result;
    
    for (const auto& id : m_roiOrder) {
        auto it = m_rois.find(id);
        if (it != m_rois.end() && it->second->contains(point)) {
            result.push_back(it->second);
        }
    }
    
    return result;
}

void ROIManager::bringToFront(const QString& roiId) {
    QWriteLocker locker(&m_lock);
    
    auto it = std::find(m_roiOrder.begin(), m_roiOrder.end(), roiId);
    if (it != m_roiOrder.end()) {
        m_roiOrder.erase(it);
        m_roiOrder.push_back(roiId);
    }
}

void ROIManager::sendToBack(const QString& roiId) {
    QWriteLocker locker(&m_lock);
    
    auto it = std::find(m_roiOrder.begin(), m_roiOrder.end(), roiId);
    if (it != m_roiOrder.end()) {
        m_roiOrder.erase(it);
        m_roiOrder.insert(m_roiOrder.begin(), roiId);
    }
}

void ROIManager::bringForward(const QString& roiId) {
    QWriteLocker locker(&m_lock);
    
    auto it = std::find(m_roiOrder.begin(), m_roiOrder.end(), roiId);
    if (it != m_roiOrder.end() && it != m_roiOrder.end() - 1) {
        std::iter_swap(it, it + 1);
    }
}

void ROIManager::sendBackward(const QString& roiId) {
    QWriteLocker locker(&m_lock);
    
    auto it = std::find(m_roiOrder.begin(), m_roiOrder.end(), roiId);
    if (it != m_roiOrder.end() && it != m_roiOrder.begin()) {
        std::iter_swap(it, it - 1);
    }
}

void ROIManager::enableROIs(const std::vector<QString>& ids) {
    for (const auto& id : ids) {
        setROIEnabled(id, true);
    }
}

void ROIManager::disableROIs(const std::vector<QString>& ids) {
    for (const auto& id : ids) {
        setROIEnabled(id, false);
    }
}

void ROIManager::deleteROIs(const std::vector<QString>& ids) {
    for (const auto& id : ids) {
        removeROI(id);
    }
}

void ROIManager::enableAutoSave(const QString& filename, int intervalMs) {
    m_autoSaveFilename = filename;
    
    if (!m_autoSaveTimer) {
        m_autoSaveTimer = new QTimer(this);
        connect(m_autoSaveTimer, &QTimer::timeout, this, &ROIManager::onAutoSave);
    }
    
    m_autoSaveTimer->start(intervalMs);
}

void ROIManager::disableAutoSave() {
    if (m_autoSaveTimer) {
        m_autoSaveTimer->stop();
    }
}

void ROIManager::connectROISignals(ROIPtr roi) {
    if (!roi) return;
    
    connect(roi.get(), &ROIBase::modified, 
            this, &ROIManager::onROIModified);
}

void ROIManager::disconnectROISignals(ROIPtr roi) {
    if (!roi) return;
    
    disconnect(roi.get(), &ROIBase::modified, 
               this, &ROIManager::onROIModified);
}

cv::Mat ROIManager::processROI(const cv::Mat& frame, ROIPtr roi) {
    if (!roi || !roi->isEnabled()) {
        return cv::Mat();
    }
    
    // Use cache for mask generation
    cv::Mat mask = m_maskCache.getMask(roi->getId(), frame.size(),
        [roi, &frame]() { return roi->getMask(frame.size()); });
    
    // Extract ROI
    cv::Mat extracted = roi->extract(frame);
    
    // Update statistics
    if (!extracted.empty()) {
        cv::Scalar mean, stddev;
        cv::meanStdDev(extracted, mean, stddev, mask);
        
        roi->metadata().meanIntensity = mean[0];
        roi->metadata().stdDeviation = stddev[0];
        roi->metadata().meanColor = mean;
    }
    
    return extracted;
}

void ROIManager::updateStatistics() const {
    QReadLocker locker(&m_lock);
    
    m_statistics.totalROIs = m_rois.size();
    m_statistics.activeROIs = 0;
    m_statistics.trackingROIs = 0;
    
    for (const auto& [id, roi] : m_rois) {
        if (roi->isEnabled()) {
            m_statistics.activeROIs++;
        }
        if (roi->metadata().trackingEnabled) {
            m_statistics.trackingROIs++;
        }
    }
    
    m_statistics.selectedROIs = m_selectedROIs.size();
}

void ROIManager::onROIModified() {
    ROIBase* roi = qobject_cast<ROIBase*>(sender());
    if (roi) {
        QString id = roi->getId();
        m_maskCache.invalidate(id);
        
        auto it = m_rois.find(id);
        if (it != m_rois.end()) {
            emit roiModified(id, it->second);
        }
    }
}

void ROIManager::onAutoSave() {
    if (!m_autoSaveFilename.isEmpty()) {
        save(m_autoSaveFilename);
    }
}

// ============ ROIProcessor Implementation ============

ROIProcessor::Result ROIProcessor::processROI(const cv::Mat& frame, ROIPtr roi) {
    Result result;
    result.roiId = roi ? roi->getId() : "";
    result.success = false;
    
    if (!roi || frame.empty()) {
        result.error = "Invalid ROI or frame";
        return result;
    }
    
    QElapsedTimer timer;
    timer.start();
    
    try {
        result.processedData = roi->extract(frame);
        result.success = true;
        result.processingTime = timer.elapsed();
    } catch (const cv::Exception& e) {
        result.error = QString::fromStdString(e.what());
    } catch (...) {
        result.error = "Unknown error during processing";
    }
    
    return result;
}

std::vector<ROIProcessor::Result> ROIProcessor::processROIBatch(
    const cv::Mat& frame,
    const std::vector<ROIPtr>& rois) {
    
    std::vector<Result> results;
    
    for (const auto& roi : rois) {
        results.push_back(processROI(frame, roi));
    }
    
    return results;
}

QFuture<ROIProcessor::Result> ROIProcessor::processROIAsync(const cv::Mat& frame, ROIPtr roi) {
    return QtConcurrent::run([frame, roi]() {
        return processROI(frame, roi);
    });
}

QFuture<std::vector<ROIProcessor::Result>> ROIProcessor::processROIBatchAsync(
    const cv::Mat& frame,
    const std::vector<ROIPtr>& rois) {
    
    return QtConcurrent::run([frame, rois]() {
        return processROIBatch(frame, rois);
    });
}

// ============ ROIImportExport Implementation ============

void ROIImportExport::exportROIs(const std::vector<ROIPtr>& rois,
                                 const QString& filename,
                                 Format format) {
    QByteArray data = exportROIsToBuffer(rois, format);
    
    QFile file(filename);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(data);
        file.close();
    }
}

QByteArray ROIImportExport::exportROIsToBuffer(const std::vector<ROIPtr>& rois,
                                               Format format) {
    switch (format) {
        case Format::JSON: {
            QJsonArray array;
            for (const auto& roi : rois) {
                if (roi) {
                    array.append(roi->toJson());
                }
            }
            QJsonDocument doc(array);
            return doc.toJson();
        }
        
        case Format::XML: {
            // XML export implementation
            return QByteArray();
        }
        
        case Format::Binary: {
            // Binary export implementation
            return QByteArray();
        }
        
        case Format::CSV: {
            // CSV export implementation
            QString csv;
            csv += "ID,Name,Type,Enabled,X,Y,Width,Height\n";
            
            for (const auto& roi : rois) {
                if (roi) {
                    cv::Rect2f bbox = roi->getBoundingBox();
                    csv += QString("%1,%2,%3,%4,%5,%6,%7,%8\n")
                        .arg(roi->getId())
                        .arg(roi->getName())
                        .arg(static_cast<int>(roi->getType()))
                        .arg(roi->isEnabled())
                        .arg(bbox.x)
                        .arg(bbox.y)
                        .arg(bbox.width)
                        .arg(bbox.height);
                }
            }
            
            return csv.toUtf8();
        }
        
        default:
            return QByteArray();
    }
}

std::vector<ROIPtr> ROIImportExport::importROIs(const QString& filename,
                                                Format format) {
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        return std::vector<ROIPtr>();
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    return importROIsFromBuffer(data, format);
}

std::vector<ROIPtr> ROIImportExport::importROIsFromBuffer(const QByteArray& data,
                                                         Format format) {
    std::vector<ROIPtr> rois;
    
    switch (format) {
        case Format::JSON: {
            QJsonDocument doc = QJsonDocument::fromJson(data);
            QJsonArray array = doc.array();
            
            for (const auto& value : array) {
                auto roi = ROIFactory::createFromJson(value.toObject());
                if (roi) {
                    rois.push_back(roi);
                }
            }
            break;
        }
        
        case Format::XML: {
            // XML import implementation
            break;
        }
        
        case Format::Binary: {
            // Binary import implementation
            break;
        }
        
        case Format::CSV: {
            // CSV import implementation
            QString csv = QString::fromUtf8(data);
            QStringList lines = csv.split('\n', Qt::SkipEmptyParts);
            
            if (lines.size() > 1) {
                // Skip header
                for (int i = 1; i < lines.size(); ++i) {
                    QStringList fields = lines[i].split(',');
                    if (fields.size() >= 8) {
                        cv::Rect2f bounds(
                            fields[4].toFloat(),
                            fields[5].toFloat(),
                            fields[6].toFloat(),
                            fields[7].toFloat()
                        );
                        
                        auto roi = ROIFactory::createRectangle(bounds);
                        if (roi) {
                            roi->setName(fields[1]);
                            roi->setEnabled(fields[3].toInt());
                            rois.push_back(roi);
                        }
                    }
                }
            }
            break;
        }
    }
    
    return rois;
}

QByteArray ROIImportExport::convertFormat(const QByteArray& data,
                                         Format from,
                                         Format to) {
    auto rois = importROIsFromBuffer(data, from);
    return exportROIsToBuffer(rois, to);
}

} // namespace OpenCV
} // namespace ComponentsForest