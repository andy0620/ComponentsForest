#include "roi_types.h"
#include <QJsonArray>
#include <QDateTime>
#include <opencv2/imgproc.hpp>
// Note: Tracking functionality temporarily disabled due to OpenCV version compatibility

namespace ComponentsForest {
namespace OpenCV {

// Static template storage for ROIFactory
std::map<QString, QJsonObject> ROIFactory::s_templates;

// ============ ROIProcessingParams Implementation ============

QJsonObject ROIProcessingParams::toJson() const {
    QJsonObject json;
    
    // Preprocessing
    json["enableBlur"] = enableBlur;
    json["blurSize"] = blurSize;
    json["enableThreshold"] = enableThreshold;
    json["thresholdValue"] = thresholdValue;
    json["enableMorphology"] = enableMorphology;
    json["morphOperation"] = static_cast<int>(morphOperation);
    json["morphSize"] = morphSize;
    
    // Enhancement
    json["enableHistogramEqualization"] = enableHistogramEqualization;
    json["enableCLAHE"] = enableCLAHE;
    json["claheClipLimit"] = claheClipLimit;
    
    return json;
}

ROIProcessingParams ROIProcessingParams::fromJson(const QJsonObject& json) {
    ROIProcessingParams params;
    
    // Preprocessing
    params.enableBlur = json["enableBlur"].toBool();
    params.blurSize = json["blurSize"].toInt(5);
    params.enableThreshold = json["enableThreshold"].toBool();
    params.thresholdValue = json["thresholdValue"].toDouble(128);
    params.enableMorphology = json["enableMorphology"].toBool();
    params.morphOperation = static_cast<cv::MorphTypes>(json["morphOperation"].toInt());
    params.morphSize = json["morphSize"].toInt(3);
    
    // Enhancement
    params.enableHistogramEqualization = json["enableHistogramEqualization"].toBool();
    params.enableCLAHE = json["enableCLAHE"].toBool();
    params.claheClipLimit = json["claheClipLimit"].toDouble(2.0);
    
    return params;
}

// ============ ROIMetadata Implementation ============

QJsonObject ROIMetadata::toJson() const {
    QJsonObject json;
    
    json["id"] = id;
    json["name"] = name;
    json["category"] = category;
    json["created"] = created.toString(Qt::ISODate);
    json["modified"] = modified.toString(Qt::ISODate);
    
    // Statistics
    json["meanIntensity"] = meanIntensity;
    json["stdDeviation"] = stdDeviation;
    
    QJsonArray colorArray;
    colorArray.append(meanColor[0]);
    colorArray.append(meanColor[1]);
    colorArray.append(meanColor[2]);
    colorArray.append(meanColor[3]);
    json["meanColor"] = colorArray;
    
    // Tracking
    json["trackingEnabled"] = trackingEnabled;
    json["trackingMethod"] = static_cast<int>(trackingMethod);
    json["trackingConfidence"] = trackingConfidence;
    
    return json;
}

ROIMetadata ROIMetadata::fromJson(const QJsonObject& json) {
    ROIMetadata metadata;
    
    metadata.id = json["id"].toString();
    metadata.name = json["name"].toString();
    metadata.category = json["category"].toString();
    metadata.created = QDateTime::fromString(json["created"].toString(), Qt::ISODate);
    metadata.modified = QDateTime::fromString(json["modified"].toString(), Qt::ISODate);
    
    // Statistics
    metadata.meanIntensity = json["meanIntensity"].toDouble();
    metadata.stdDeviation = json["stdDeviation"].toDouble();
    
    QJsonArray colorArray = json["meanColor"].toArray();
    if (colorArray.size() >= 4) {
        metadata.meanColor = cv::Scalar(
            colorArray[0].toDouble(),
            colorArray[1].toDouble(),
            colorArray[2].toDouble(),
            colorArray[3].toDouble()
        );
    }
    
    // Tracking
    metadata.trackingEnabled = json["trackingEnabled"].toBool();
    metadata.trackingMethod = static_cast<TrackingMethod>(json["trackingMethod"].toInt());
    metadata.trackingConfidence = json["trackingConfidence"].toDouble();
    
    return metadata;
}

// ============ ROIBase Implementation ============

ROIBase::ROIBase(QObject* parent) : QObject(parent) {
    m_metadata.id = QUuid::createUuid().toString();
    m_metadata.created = QDateTime::currentDateTime();
    m_metadata.modified = QDateTime::currentDateTime();
}

cv::Mat ROIBase::extract(const cv::Mat& source) const {
    if (source.empty()) return cv::Mat();
    
    cv::Mat mask = getMask(source.size());
    if (mask.empty()) return cv::Mat();
    
    cv::Mat result;
    source.copyTo(result, mask);
    
    // Apply processing if enabled
    if (m_processingParams.customProcessor) {
        result = m_processingParams.customProcessor(result);
    } else {
        // Apply built-in processing
        if (m_processingParams.enableBlur) {
            cv::GaussianBlur(result, result, 
                cv::Size(m_processingParams.blurSize, m_processingParams.blurSize), 0);
        }
        
        if (m_processingParams.enableThreshold) {
            cv::threshold(result, result, 
                m_processingParams.thresholdValue, 255, cv::THRESH_BINARY);
        }
        
        if (m_processingParams.enableMorphology) {
            cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT,
                cv::Size(m_processingParams.morphSize, m_processingParams.morphSize));
            cv::morphologyEx(result, result, m_processingParams.morphOperation, kernel);
        }
        
        if (m_processingParams.enableHistogramEqualization && result.channels() == 1) {
            cv::equalizeHist(result, result);
        }
        
        if (m_processingParams.enableCLAHE && result.channels() == 1) {
            auto clahe = cv::createCLAHE(m_processingParams.claheClipLimit, cv::Size(8, 8));
            clahe->apply(result, result);
        }
    }
    
    return result;
}

void ROIBase::apply(cv::Mat& target, const cv::Mat& roiData) const {
    if (target.empty() || roiData.empty()) return;
    
    cv::Mat mask = getMask(target.size());
    if (mask.empty()) return;
    
    roiData.copyTo(target, mask);
}

void ROIBase::draw(cv::Mat& canvas, const cv::Scalar& color, int thickness) const {
    cv::Rect2f bbox = getBoundingBox();
    cv::rectangle(canvas, bbox, color, thickness);
}

bool ROIBase::isValid() const {
    return !m_metadata.id.isEmpty();
}

QJsonObject ROIBase::toJson() const {
    QJsonObject json;
    json["type"] = static_cast<int>(getType());
    json["metadata"] = m_metadata.toJson();
    json["processingParams"] = m_processingParams.toJson();
    json["enabled"] = m_enabled.load();
    json["visualState"] = static_cast<int>(m_visualState);
    return json;
}

void ROIBase::fromJson(const QJsonObject& json) {
    m_metadata = ROIMetadata::fromJson(json["metadata"].toObject());
    m_processingParams = ROIProcessingParams::fromJson(json["processingParams"].toObject());
    m_enabled = json["enabled"].toBool(true);
    m_visualState = static_cast<ROIVisualState>(json["visualState"].toInt());
}

void ROIBase::setName(const QString& name) {
    m_metadata.name = name;
    m_metadata.modified = QDateTime::currentDateTime();
    invalidateCache();
    emit modified();
}

void ROIBase::setEnabled(bool enabled) {
    if (m_enabled != enabled) {
        m_enabled = enabled;
        emit enabledChanged(enabled);
    }
}

void ROIBase::setVisualState(ROIVisualState state) {
    if (m_visualState != state) {
        m_visualState = state;
        emit visualStateChanged(state);
    }
}

void ROIBase::setParent(ROIPtr parent) {
    m_parent = parent;
    invalidateCache();
    emit modified();
}

void ROIBase::addChild(ROIPtr child) {
    if (child && child.get() != this) {
        m_children.push_back(child);
        child->setParent(shared_from_this());
        invalidateCache();
        emit modified();
    }
}

void ROIBase::removeChild(const QString& childId) {
    auto it = std::remove_if(m_children.begin(), m_children.end(),
        [&childId](const ROIPtr& child) {
            return child->getId() == childId;
        });
    
    if (it != m_children.end()) {
        m_children.erase(it, m_children.end());
        invalidateCache();
        emit modified();
    }
}

void ROIBase::translate(const cv::Point2f& offset) {
    // Default implementation - derived classes should override
    invalidateCache();
    emit modified();
}

void ROIBase::scale(float factor) {
    // Default implementation - derived classes should override
    invalidateCache();
    emit modified();
}

void ROIBase::rotate(float angle) {
    // Default implementation - derived classes should override
    invalidateCache();
    emit modified();
}

void ROIBase::transform(const cv::Mat& transformMatrix) {
    // Default implementation - derived classes should override
    invalidateCache();
    emit modified();
}

// ============ RectangularROI Implementation ============

RectangularROI::RectangularROI(QObject* parent) : ROIBase(parent) {
    m_metadata.name = "Rectangle";
}

RectangularROI::RectangularROI(const cv::Rect2f& bounds, QObject* parent) 
    : ROIBase(parent), m_bounds(bounds) {
    m_metadata.name = "Rectangle";
}

cv::Mat RectangularROI::getMask(const cv::Size& frameSize) const {
    if (!m_maskDirty && m_cachedMaskSize == frameSize && !m_cachedMask.empty()) {
        return m_cachedMask;
    }
    
    cv::Mat mask = cv::Mat::zeros(frameSize, CV_8UC1);
    
    if (m_rotation == 0) {
        // Simple non-rotated rectangle
        cv::Rect rect(
            static_cast<int>(m_bounds.x),
            static_cast<int>(m_bounds.y),
            static_cast<int>(m_bounds.width),
            static_cast<int>(m_bounds.height)
        );
        
        // Clip to frame bounds
        rect &= cv::Rect(0, 0, frameSize.width, frameSize.height);
        
        if (rect.width > 0 && rect.height > 0) {
            mask(rect) = 255;
        }
    } else {
        // Rotated rectangle
        cv::Point2f center = getCenter();
        cv::Size2f size = getSize();
        cv::RotatedRect rotRect(center, size, m_rotation);
        
        // Get the vertices of the rotated rectangle
        cv::Point2f vertices[4];
        rotRect.points(vertices);
        
        // Convert to integer points
        std::vector<cv::Point> points;
        for (int i = 0; i < 4; ++i) {
            points.push_back(cv::Point(static_cast<int>(vertices[i].x), 
                                       static_cast<int>(vertices[i].y)));
        }
        
        // Fill the polygon
        cv::fillPoly(mask, std::vector<std::vector<cv::Point>>{points}, cv::Scalar(255));
    }
    
    m_cachedMask = mask;
    m_cachedMaskSize = frameSize;
    m_maskDirty = false;
    
    return m_cachedMask;
}

bool RectangularROI::contains(const cv::Point2f& point) const {
    if (m_rotation == 0) {
        return point.x >= m_bounds.x && 
               point.x <= m_bounds.x + m_bounds.width &&
               point.y >= m_bounds.y && 
               point.y <= m_bounds.y + m_bounds.height;
    } else {
        // For rotated rectangle, transform point to local coordinates
        cv::Point2f center = getCenter();
        float angle = -m_rotation * CV_PI / 180.0f;
        float cos_a = cos(angle);
        float sin_a = sin(angle);
        
        cv::Point2f local;
        local.x = (point.x - center.x) * cos_a - (point.y - center.y) * sin_a;
        local.y = (point.x - center.x) * sin_a + (point.y - center.y) * cos_a;
        
        return std::abs(local.x) <= m_bounds.width / 2 &&
               std::abs(local.y) <= m_bounds.height / 2;
    }
}

ROIPtr RectangularROI::clone() const {
    auto cloned = std::make_shared<RectangularROI>(m_bounds);
    cloned->m_rotation = m_rotation;
    cloned->m_metadata = m_metadata;
    cloned->m_metadata.id = QUuid::createUuid().toString();
    cloned->m_processingParams = m_processingParams;
    cloned->m_enabled = m_enabled.load();
    cloned->m_visualState = m_visualState;
    return cloned;
}

void RectangularROI::setBounds(const cv::Rect2f& bounds) {
    m_bounds = bounds;
    invalidateCache();
    emit modified();
}

void RectangularROI::setRotation(float rotation) {
    m_rotation = rotation;
    invalidateCache();
    emit modified();
}

cv::Point2f RectangularROI::getCenter() const {
    return cv::Point2f(m_bounds.x + m_bounds.width / 2,
                       m_bounds.y + m_bounds.height / 2);
}

cv::Size2f RectangularROI::getSize() const {
    return cv::Size2f(m_bounds.width, m_bounds.height);
}

void RectangularROI::draw(cv::Mat& canvas, const cv::Scalar& color, int thickness) const {
    if (m_rotation == 0) {
        cv::rectangle(canvas, m_bounds, color, thickness);
    } else {
        cv::RotatedRect rotRect(getCenter(), getSize(), m_rotation);
        cv::Point2f vertices[4];
        rotRect.points(vertices);
        
        for (int i = 0; i < 4; ++i) {
            cv::line(canvas, vertices[i], vertices[(i + 1) % 4], color, thickness);
        }
    }
    
    // Draw center point if selected
    if (m_visualState == ROIVisualState::Selected) {
        cv::Point2f center = getCenter();
        cv::circle(canvas, center, 3, color, -1);
    }
}

QJsonObject RectangularROI::toJson() const {
    QJsonObject json = ROIBase::toJson();
    
    QJsonObject boundsObj;
    boundsObj["x"] = m_bounds.x;
    boundsObj["y"] = m_bounds.y;
    boundsObj["width"] = m_bounds.width;
    boundsObj["height"] = m_bounds.height;
    json["bounds"] = boundsObj;
    
    json["rotation"] = m_rotation;
    
    return json;
}

void RectangularROI::fromJson(const QJsonObject& json) {
    ROIBase::fromJson(json);
    
    QJsonObject boundsObj = json["bounds"].toObject();
    m_bounds.x = boundsObj["x"].toDouble();
    m_bounds.y = boundsObj["y"].toDouble();
    m_bounds.width = boundsObj["width"].toDouble();
    m_bounds.height = boundsObj["height"].toDouble();
    
    m_rotation = json["rotation"].toDouble();
}

// ============ CircularROI Implementation ============

CircularROI::CircularROI(QObject* parent) : ROIBase(parent) {
    m_metadata.name = "Circle";
}

CircularROI::CircularROI(const cv::Point2f& center, float radius, QObject* parent)
    : ROIBase(parent), m_center(center), m_radius(radius) {
    m_metadata.name = "Circle";
}

cv::Mat CircularROI::getMask(const cv::Size& frameSize) const {
    if (!m_maskDirty && m_cachedMaskSize == frameSize && !m_cachedMask.empty()) {
        return m_cachedMask;
    }
    
    cv::Mat mask = cv::Mat::zeros(frameSize, CV_8UC1);
    
    if (m_innerRadius > 0 && m_innerRadius < m_radius) {
        // Annular ROI
        cv::circle(mask, m_center, static_cast<int>(m_radius), cv::Scalar(255), -1);
        cv::circle(mask, m_center, static_cast<int>(m_innerRadius), cv::Scalar(0), -1);
    } else {
        // Regular circle
        cv::circle(mask, m_center, static_cast<int>(m_radius), cv::Scalar(255), -1);
    }
    
    m_cachedMask = mask;
    m_cachedMaskSize = frameSize;
    m_maskDirty = false;
    
    return m_cachedMask;
}

cv::Rect2f CircularROI::getBoundingBox() const {
    return cv::Rect2f(m_center.x - m_radius, m_center.y - m_radius,
                      2 * m_radius, 2 * m_radius);
}

bool CircularROI::contains(const cv::Point2f& point) const {
    float dist = cv::norm(point - m_center);
    if (m_innerRadius > 0) {
        return dist <= m_radius && dist >= m_innerRadius;
    }
    return dist <= m_radius;
}

ROIPtr CircularROI::clone() const {
    auto cloned = std::make_shared<CircularROI>(m_center, m_radius);
    cloned->m_innerRadius = m_innerRadius;
    cloned->m_metadata = m_metadata;
    cloned->m_metadata.id = QUuid::createUuid().toString();
    cloned->m_processingParams = m_processingParams;
    cloned->m_enabled = m_enabled.load();
    cloned->m_visualState = m_visualState;
    return cloned;
}

void CircularROI::setCenter(const cv::Point2f& center) {
    m_center = center;
    invalidateCache();
    emit modified();
}

void CircularROI::setRadius(float radius) {
    m_radius = radius;
    invalidateCache();
    emit modified();
}

void CircularROI::setInnerRadius(float radius) {
    m_innerRadius = radius;
    invalidateCache();
    emit modified();
}

void CircularROI::draw(cv::Mat& canvas, const cv::Scalar& color, int thickness) const {
    cv::circle(canvas, m_center, static_cast<int>(m_radius), color, thickness);
    
    if (m_innerRadius > 0) {
        cv::circle(canvas, m_center, static_cast<int>(m_innerRadius), color, thickness);
    }
    
    // Draw center point if selected
    if (m_visualState == ROIVisualState::Selected) {
        cv::circle(canvas, m_center, 3, color, -1);
    }
}

QJsonObject CircularROI::toJson() const {
    QJsonObject json = ROIBase::toJson();
    
    QJsonObject centerObj;
    centerObj["x"] = m_center.x;
    centerObj["y"] = m_center.y;
    json["center"] = centerObj;
    
    json["radius"] = m_radius;
    json["innerRadius"] = m_innerRadius;
    
    return json;
}

void CircularROI::fromJson(const QJsonObject& json) {
    ROIBase::fromJson(json);
    
    QJsonObject centerObj = json["center"].toObject();
    m_center.x = centerObj["x"].toDouble();
    m_center.y = centerObj["y"].toDouble();
    
    m_radius = json["radius"].toDouble();
    m_innerRadius = json["innerRadius"].toDouble();
}

// ============ PolygonalROI Implementation ============

PolygonalROI::PolygonalROI(QObject* parent) : ROIBase(parent) {
    m_metadata.name = "Polygon";
}

PolygonalROI::PolygonalROI(const std::vector<cv::Point2f>& vertices, QObject* parent)
    : ROIBase(parent), m_vertices(vertices) {
    m_metadata.name = "Polygon";
}

cv::Mat PolygonalROI::getMask(const cv::Size& frameSize) const {
    if (!m_maskDirty && m_cachedMaskSize == frameSize && !m_cachedMask.empty()) {
        return m_cachedMask;
    }
    
    cv::Mat mask = cv::Mat::zeros(frameSize, CV_8UC1);
    
    if (m_vertices.size() >= 3) {
        std::vector<cv::Point> points;
        for (const auto& vertex : m_vertices) {
            points.push_back(cv::Point(static_cast<int>(vertex.x), 
                                       static_cast<int>(vertex.y)));
        }
        
        cv::fillPoly(mask, std::vector<std::vector<cv::Point>>{points}, cv::Scalar(255));
    }
    
    m_cachedMask = mask;
    m_cachedMaskSize = frameSize;
    m_maskDirty = false;
    
    return m_cachedMask;
}

cv::Rect2f PolygonalROI::getBoundingBox() const {
    if (m_vertices.empty()) {
        return cv::Rect2f();
    }
    
    float minX = m_vertices[0].x, maxX = m_vertices[0].x;
    float minY = m_vertices[0].y, maxY = m_vertices[0].y;
    
    for (const auto& vertex : m_vertices) {
        minX = std::min(minX, vertex.x);
        maxX = std::max(maxX, vertex.x);
        minY = std::min(minY, vertex.y);
        maxY = std::max(maxY, vertex.y);
    }
    
    return cv::Rect2f(minX, minY, maxX - minX, maxY - minY);
}

bool PolygonalROI::contains(const cv::Point2f& point) const {
    if (m_vertices.size() < 3) return false;
    
    std::vector<cv::Point2f> contour(m_vertices);
    return cv::pointPolygonTest(contour, point, false) >= 0;
}

ROIPtr PolygonalROI::clone() const {
    auto cloned = std::make_shared<PolygonalROI>(m_vertices);
    cloned->m_closed = m_closed;
    cloned->m_metadata = m_metadata;
    cloned->m_metadata.id = QUuid::createUuid().toString();
    cloned->m_processingParams = m_processingParams;
    cloned->m_enabled = m_enabled.load();
    cloned->m_visualState = m_visualState;
    return cloned;
}

void PolygonalROI::setVertices(const std::vector<cv::Point2f>& vertices) {
    m_vertices = vertices;
    m_convexityCached = false;
    invalidateCache();
    emit modified();
}

void PolygonalROI::addVertex(const cv::Point2f& vertex) {
    m_vertices.push_back(vertex);
    m_convexityCached = false;
    invalidateCache();
    emit modified();
}

void PolygonalROI::removeVertex(int index) {
    if (index >= 0 && index < static_cast<int>(m_vertices.size())) {
        m_vertices.erase(m_vertices.begin() + index);
        m_convexityCached = false;
        invalidateCache();
        emit modified();
    }
}

void PolygonalROI::moveVertex(int index, const cv::Point2f& newPos) {
    if (index >= 0 && index < static_cast<int>(m_vertices.size())) {
        m_vertices[index] = newPos;
        m_convexityCached = false;
        invalidateCache();
        emit modified();
    }
}

void PolygonalROI::setClosed(bool closed) {
    m_closed = closed;
    invalidateCache();
    emit modified();
}

bool PolygonalROI::isConvex() const {
    if (!m_convexityCached) {
        if (m_vertices.size() < 3) {
            m_isConvex = false;
        } else {
            std::vector<cv::Point2f> contour(m_vertices);
            m_isConvex = cv::isContourConvex(contour);
        }
        m_convexityCached = true;
    }
    return m_isConvex;
}

double PolygonalROI::getArea() const {
    if (m_vertices.size() < 3) return 0;
    
    std::vector<cv::Point2f> contour(m_vertices);
    return cv::contourArea(contour);
}

double PolygonalROI::getPerimeter() const {
    if (m_vertices.size() < 2) return 0;
    
    std::vector<cv::Point2f> contour(m_vertices);
    return cv::arcLength(contour, m_closed);
}

void PolygonalROI::draw(cv::Mat& canvas, const cv::Scalar& color, int thickness) const {
    if (m_vertices.size() < 2) return;
    
    std::vector<cv::Point> points;
    for (const auto& vertex : m_vertices) {
        points.push_back(cv::Point(static_cast<int>(vertex.x), 
                                   static_cast<int>(vertex.y)));
    }
    
    if (m_closed && points.size() >= 3) {
        cv::polylines(canvas, std::vector<std::vector<cv::Point>>{points}, 
                     true, color, thickness);
    } else {
        cv::polylines(canvas, std::vector<std::vector<cv::Point>>{points}, 
                     false, color, thickness);
    }
    
    // Draw vertices if selected
    if (m_visualState == ROIVisualState::Selected) {
        for (const auto& point : points) {
            cv::circle(canvas, point, 3, color, -1);
        }
    }
}

QJsonObject PolygonalROI::toJson() const {
    QJsonObject json = ROIBase::toJson();
    
    QJsonArray verticesArray;
    for (const auto& vertex : m_vertices) {
        QJsonObject vertexObj;
        vertexObj["x"] = vertex.x;
        vertexObj["y"] = vertex.y;
        verticesArray.append(vertexObj);
    }
    json["vertices"] = verticesArray;
    json["closed"] = m_closed;
    
    return json;
}

void PolygonalROI::fromJson(const QJsonObject& json) {
    ROIBase::fromJson(json);
    
    m_vertices.clear();
    QJsonArray verticesArray = json["vertices"].toArray();
    for (const auto& value : verticesArray) {
        QJsonObject vertexObj = value.toObject();
        cv::Point2f vertex;
        vertex.x = vertexObj["x"].toDouble();
        vertex.y = vertexObj["y"].toDouble();
        m_vertices.push_back(vertex);
    }
    
    m_closed = json["closed"].toBool(true);
    m_convexityCached = false;
}

// ============ MaskROI Implementation ============

MaskROI::MaskROI(QObject* parent) : ROIBase(parent) {
    m_metadata.name = "Mask";
}

MaskROI::MaskROI(const cv::Mat& mask, QObject* parent) 
    : ROIBase(parent), m_mask(mask) {
    m_metadata.name = "Mask";
    updateBoundingBox();
}

cv::Mat MaskROI::getMask(const cv::Size& frameSize) const {
    if (m_mask.empty()) {
        return cv::Mat::zeros(frameSize, CV_8UC1);
    }
    
    if (m_mask.size() == frameSize) {
        return m_mask;
    }
    
    // Resize mask to match frame size
    cv::Mat resized;
    cv::resize(m_mask, resized, frameSize, 0, 0, cv::INTER_NEAREST);
    return resized;
}

cv::Rect2f MaskROI::getBoundingBox() const {
    return cv::Rect2f(m_boundingBox.x, m_boundingBox.y, 
                      m_boundingBox.width, m_boundingBox.height);
}

bool MaskROI::contains(const cv::Point2f& point) const {
    if (m_mask.empty()) return false;
    
    int x = static_cast<int>(point.x);
    int y = static_cast<int>(point.y);
    
    if (x < 0 || x >= m_mask.cols || y < 0 || y >= m_mask.rows) {
        return false;
    }
    
    return m_mask.at<uchar>(y, x) > 0;
}

ROIPtr MaskROI::clone() const {
    auto cloned = std::make_shared<MaskROI>(m_mask.clone());
    cloned->m_weighted = m_weighted;
    cloned->m_metadata = m_metadata;
    cloned->m_metadata.id = QUuid::createUuid().toString();
    cloned->m_processingParams = m_processingParams;
    cloned->m_enabled = m_enabled.load();
    cloned->m_visualState = m_visualState;
    return cloned;
}

void MaskROI::setMask(const cv::Mat& mask) {
    m_mask = mask.clone();
    updateBoundingBox();
    invalidateCache();
    emit modified();
}

void MaskROI::setWeighted(bool weighted) {
    m_weighted = weighted;
    invalidateCache();
    emit modified();
}

void MaskROI::addToMask(const cv::Mat& addition) {
    if (m_mask.empty()) {
        m_mask = addition.clone();
    } else if (m_mask.size() == addition.size()) {
        cv::bitwise_or(m_mask, addition, m_mask);
    }
    updateBoundingBox();
    invalidateCache();
    emit modified();
}

void MaskROI::subtractFromMask(const cv::Mat& subtraction) {
    if (!m_mask.empty() && m_mask.size() == subtraction.size()) {
        cv::Mat inverted;
        cv::bitwise_not(subtraction, inverted);
        cv::bitwise_and(m_mask, inverted, m_mask);
        updateBoundingBox();
        invalidateCache();
        emit modified();
    }
}

void MaskROI::intersectWithMask(const cv::Mat& other) {
    if (!m_mask.empty() && m_mask.size() == other.size()) {
        cv::bitwise_and(m_mask, other, m_mask);
        updateBoundingBox();
        invalidateCache();
        emit modified();
    }
}

void MaskROI::dilate(int size) {
    if (!m_mask.empty()) {
        cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(size, size));
        cv::dilate(m_mask, m_mask, kernel);
        updateBoundingBox();
        invalidateCache();
        emit modified();
    }
}

void MaskROI::erode(int size) {
    if (!m_mask.empty()) {
        cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(size, size));
        cv::erode(m_mask, m_mask, kernel);
        updateBoundingBox();
        invalidateCache();
        emit modified();
    }
}

void MaskROI::morphologyEx(cv::MorphTypes operation, int size) {
    if (!m_mask.empty()) {
        cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(size, size));
        cv::morphologyEx(m_mask, m_mask, operation, kernel);
        updateBoundingBox();
        invalidateCache();
        emit modified();
    }
}

void MaskROI::draw(cv::Mat& canvas, const cv::Scalar& color, int thickness) const {
    if (m_mask.empty()) return;
    
    // Find contours
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(m_mask.clone(), contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    
    // Draw contours
    cv::drawContours(canvas, contours, -1, color, thickness);
    
    // Draw bounding box if selected
    if (m_visualState == ROIVisualState::Selected) {
        cv::rectangle(canvas, m_boundingBox, color, 1);
    }
}

QJsonObject MaskROI::toJson() const {
    QJsonObject json = ROIBase::toJson();
    
    // Convert mask to base64 encoded PNG
    std::vector<uchar> buffer;
    cv::imencode(".png", m_mask, buffer);
    QByteArray maskData(reinterpret_cast<const char*>(buffer.data()), buffer.size());
    json["maskData"] = QString::fromLatin1(maskData.toBase64());
    
    json["weighted"] = m_weighted;
    
    return json;
}

void MaskROI::fromJson(const QJsonObject& json) {
    ROIBase::fromJson(json);
    
    // Decode mask from base64
    QString maskDataStr = json["maskData"].toString();
    QByteArray maskData = QByteArray::fromBase64(maskDataStr.toLatin1());
    std::vector<uchar> buffer(maskData.begin(), maskData.end());
    m_mask = cv::imdecode(buffer, cv::IMREAD_GRAYSCALE);
    
    m_weighted = json["weighted"].toBool();
    updateBoundingBox();
}

void MaskROI::updateBoundingBox() {
    if (m_mask.empty()) {
        m_boundingBox = cv::Rect();
        return;
    }
    
    // Find non-zero pixels
    std::vector<cv::Point> points;
    cv::findNonZero(m_mask, points);
    
    if (points.empty()) {
        m_boundingBox = cv::Rect();
    } else {
        m_boundingBox = cv::boundingRect(points);
    }
}

// ============ DynamicROI Implementation ============

DynamicROI::DynamicROI(QObject* parent) : ROIBase(parent) {
    m_metadata.name = "Dynamic";
}

DynamicROI::DynamicROI(ROIPtr baseROI, QObject* parent) 
    : ROIBase(parent), m_baseROI(baseROI), m_currentROI(baseROI) {
    m_metadata.name = "Dynamic";
}

cv::Mat DynamicROI::getMask(const cv::Size& frameSize) const {
    if (m_currentROI) {
        return m_currentROI->getMask(frameSize);
    }
    return cv::Mat::zeros(frameSize, CV_8UC1);
}

cv::Rect2f DynamicROI::getBoundingBox() const {
    if (m_currentROI) {
        return m_currentROI->getBoundingBox();
    }
    return cv::Rect2f();
}

bool DynamicROI::contains(const cv::Point2f& point) const {
    if (m_currentROI) {
        return m_currentROI->contains(point);
    }
    return false;
}

ROIPtr DynamicROI::clone() const {
    auto cloned = std::make_shared<DynamicROI>(m_baseROI ? m_baseROI->clone() : nullptr);
    cloned->m_metadata = m_metadata;
    cloned->m_metadata.id = QUuid::createUuid().toString();
    cloned->m_processingParams = m_processingParams;
    cloned->m_enabled = m_enabled.load();
    cloned->m_visualState = m_visualState;
    cloned->m_trackingMethod = m_trackingMethod;
    cloned->m_adaptFunction = m_adaptFunction;
    return cloned;
}

void DynamicROI::setBaseROI(ROIPtr baseROI) {
    m_baseROI = baseROI;
    m_currentROI = baseROI;
    invalidateCache();
    emit modified();
}

void DynamicROI::setAdaptFunction(std::function<ROIPtr(const cv::Mat&, ROIPtr)> func) {
    m_adaptFunction = func;
}

void DynamicROI::adapt(const cv::Mat& frame) {
    if (m_adaptFunction && m_currentROI) {
        auto newROI = m_adaptFunction(frame, m_currentROI);
        if (newROI) {
            m_currentROI = newROI;
            invalidateCache();
            emit modified();
        }
    }
}

void DynamicROI::enableTracking(TrackingMethod method) {
    m_trackingMethod = method;
    m_metadata.trackingEnabled = true;
    m_metadata.trackingMethod = method;
    m_tracker = nullptr;  // Will be initialized on first frame
}

void DynamicROI::disableTracking() {
    m_trackingMethod = TrackingMethod::None;
    m_metadata.trackingEnabled = false;
    m_tracker = nullptr;
    m_trackingConfidence = 0;
}

void DynamicROI::updateTracking(const cv::Mat& frame) {
    if (m_trackingMethod == TrackingMethod::None || !m_currentROI) {
        return;
    }
    
    if (!m_tracker) {
        initializeTracker(frame);
        return;
    }
    
    // Note: Tracking functionality temporarily disabled
    qWarning() << "Tracking update called but tracking is disabled";
    return;
    
    // Tracking-related code removed due to above return statement
    // TODO: Implement proper tracking when OpenCV version issues are resolved
}

void DynamicROI::enablePrediction(int framesAhead) {
    m_predictionEnabled = true;
    initializeKalmanFilter();
}

void DynamicROI::disablePrediction() {
    m_predictionEnabled = false;
}

cv::Rect2f DynamicROI::predictPosition(int framesAhead) const {
    if (!m_predictionEnabled || !m_currentROI) {
        return m_currentROI ? m_currentROI->getBoundingBox() : cv::Rect2f();
    }
    
    // Simplified prediction - return current position
    // Full implementation would use Kalman filter
    return m_currentROI->getBoundingBox();
}

void DynamicROI::draw(cv::Mat& canvas, const cv::Scalar& color, int thickness) const {
    if (m_currentROI) {
        m_currentROI->draw(canvas, color, thickness);
        
        // Draw tracking confidence indicator
        if (m_trackingMethod != TrackingMethod::None) {
            cv::Rect2f bbox = m_currentROI->getBoundingBox();
            cv::Scalar confColor = m_trackingConfidence > 0.5 ? 
                cv::Scalar(0, 255, 0) : cv::Scalar(0, 0, 255);
            
            // Draw confidence bar
            int barHeight = 5;
            int barWidth = static_cast<int>(bbox.width * m_trackingConfidence);
            cv::rectangle(canvas, 
                cv::Rect(bbox.x, bbox.y - barHeight - 2, barWidth, barHeight),
                confColor, -1);
        }
    }
}

QJsonObject DynamicROI::toJson() const {
    QJsonObject json = ROIBase::toJson();
    
    if (m_baseROI) {
        json["baseROI"] = m_baseROI->toJson();
    }
    
    json["trackingMethod"] = static_cast<int>(m_trackingMethod);
    json["trackingConfidence"] = m_trackingConfidence;
    json["predictionEnabled"] = m_predictionEnabled;
    
    return json;
}

void DynamicROI::fromJson(const QJsonObject& json) {
    ROIBase::fromJson(json);
    
    if (json.contains("baseROI")) {
        m_baseROI = ROIFactory::createFromJson(json["baseROI"].toObject());
        m_currentROI = m_baseROI;
    }
    
    m_trackingMethod = static_cast<TrackingMethod>(json["trackingMethod"].toInt());
    m_trackingConfidence = json["trackingConfidence"].toDouble();
    m_predictionEnabled = json["predictionEnabled"].toBool();
}

void DynamicROI::initializeTracker(const cv::Mat& frame) {
    if (!m_currentROI) return;
    
    cv::Rect2d bbox = m_currentROI->getBoundingBox();
    
    // Note: Tracking functionality temporarily disabled due to OpenCV compatibility issues
    // TODO: Implement alternative tracking approach or use different OpenCV version
    qWarning() << "Tracking functionality is currently disabled";
    return;
    
    if (m_tracker) {
        m_tracker->init(frame, bbox);
        m_referenceFrame = frame.clone();
        m_trackingConfidence = 1.0f;
    }
}

void DynamicROI::initializeKalmanFilter() {
    // Initialize 4-state Kalman filter (x, y, vx, vy)
    m_kalmanFilter = cv::KalmanFilter(4, 2, 0);
    
    // Transition matrix
    m_kalmanFilter.transitionMatrix = (cv::Mat_<float>(4, 4) <<
        1, 0, 1, 0,
        0, 1, 0, 1,
        0, 0, 1, 0,
        0, 0, 0, 1);
    
    // Measurement matrix
    m_kalmanFilter.measurementMatrix = (cv::Mat_<float>(2, 4) <<
        1, 0, 0, 0,
        0, 1, 0, 0);
    
    // Process noise
    cv::setIdentity(m_kalmanFilter.processNoiseCov, cv::Scalar::all(1e-4));
    
    // Measurement noise
    cv::setIdentity(m_kalmanFilter.measurementNoiseCov, cv::Scalar::all(1e-1));
    
    // Error covariance
    cv::setIdentity(m_kalmanFilter.errorCovPost, cv::Scalar::all(1));
    
    // Initial state
    if (m_currentROI) {
        cv::Rect2f bbox = m_currentROI->getBoundingBox();
        m_kalmanFilter.statePre.at<float>(0) = bbox.x + bbox.width / 2;
        m_kalmanFilter.statePre.at<float>(1) = bbox.y + bbox.height / 2;
        m_kalmanFilter.statePre.at<float>(2) = 0;
        m_kalmanFilter.statePre.at<float>(3) = 0;
    }
}

// ============ ROIFactory Implementation ============

ROIPtr ROIFactory::create(ROIType type) {
    switch (type) {
        case ROIType::Rectangle:
            return std::make_shared<RectangularROI>();
        case ROIType::Circle:
            return std::make_shared<CircularROI>();
        case ROIType::Polygon:
            return std::make_shared<PolygonalROI>();
        case ROIType::Mask:
            return std::make_shared<MaskROI>();
        case ROIType::Dynamic:
            return std::make_shared<DynamicROI>();
        default:
            return nullptr;
    }
}

ROIPtr ROIFactory::createFromJson(const QJsonObject& json) {
    ROIType type = static_cast<ROIType>(json["type"].toInt());
    ROIPtr roi = create(type);
    
    if (roi) {
        roi->fromJson(json);
    }
    
    return roi;
}

ROIPtr ROIFactory::createRectangle(const cv::Rect2f& bounds) {
    return std::make_shared<RectangularROI>(bounds);
}

ROIPtr ROIFactory::createCircle(const cv::Point2f& center, float radius) {
    return std::make_shared<CircularROI>(center, radius);
}

ROIPtr ROIFactory::createPolygon(const std::vector<cv::Point2f>& vertices) {
    return std::make_shared<PolygonalROI>(vertices);
}

ROIPtr ROIFactory::createMask(const cv::Mat& mask) {
    return std::make_shared<MaskROI>(mask);
}

ROIPtr ROIFactory::createDynamic(ROIPtr baseROI) {
    return std::make_shared<DynamicROI>(baseROI);
}

ROIPtr ROIFactory::createFromTemplate(const QString& templateName) {
    auto it = s_templates.find(templateName);
    if (it != s_templates.end()) {
        return createFromJson(it->second);
    }
    return nullptr;
}

void ROIFactory::registerTemplate(const QString& name, const QJsonObject& templateData) {
    s_templates[name] = templateData;
}

} // namespace OpenCV
} // namespace ComponentsForest