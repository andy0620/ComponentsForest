#ifndef ROI_TYPES_H
#define ROI_TYPES_H

#include <opencv2/opencv.hpp>
#include <QObject>
#include <QJsonObject>
#include <QUuid>
#include <QImage>
#include <QColor>
#include <memory>
#include <functional>
#include <atomic>

namespace ComponentsForest {
namespace OpenCV {

// Forward declarations
class ROIBase;
using ROIPtr = std::shared_ptr<ROIBase>;
using ROIWeakPtr = std::weak_ptr<ROIBase>;

/**
 * @brief ROI type enumeration
 */
enum class ROIType {
    Rectangle,
    Circle,
    Polygon,
    Mask,
    Dynamic,
    Custom
};

/**
 * @brief ROI tracking methods
 */
enum class TrackingMethod {
    None,
    TemplateMatching,
    OpticalFlow,
    CSRT,           // OpenCV CSRT tracker
    KCF,            // Kernelized Correlation Filter
    MIL,            // Multiple Instance Learning
    DeepLearning    // DNN-based tracking
};

/**
 * @brief ROI visual states for UI feedback
 */
enum class ROIVisualState {
    Normal,
    Hover,
    Selected,
    Processing,
    Error,
    Disabled
};

/**
 * @brief ROI composition modes for overlapping regions
 */
enum class CompositionMode {
    Replace,        // Replace underlying pixels
    Add,           // Add to underlying pixels
    Multiply,      // Multiply with underlying pixels
    Subtract,      // Subtract from underlying pixels
    Intersect,     // Keep only intersection
    Union,         // Combine regions
    Xor            // Exclusive or
};

/**
 * @brief ROI processing parameters
 */
struct ROIProcessingParams {
    // Preprocessing
    bool enableBlur = false;
    int blurSize = 5;
    
    bool enableThreshold = false;
    double thresholdValue = 128;
    
    bool enableMorphology = false;
    cv::MorphTypes morphOperation = cv::MORPH_CLOSE;
    int morphSize = 3;
    
    // Enhancement
    bool enableHistogramEqualization = false;
    bool enableCLAHE = false;
    double claheClipLimit = 2.0;
    
    // Custom processing function
    std::function<cv::Mat(const cv::Mat&)> customProcessor;
    
    QJsonObject toJson() const;
    static ROIProcessingParams fromJson(const QJsonObject& json);
};

/**
 * @brief ROI metadata and statistics
 */
struct ROIMetadata {
    QString id;
    QString name;
    QString category;
    QDateTime created;
    QDateTime modified;
    
    // Statistics
    double meanIntensity = 0;
    double stdDeviation = 0;
    cv::Scalar meanColor;
    cv::Mat histogram;
    
    // Tracking info
    bool trackingEnabled = false;
    TrackingMethod trackingMethod = TrackingMethod::None;
    float trackingConfidence = 0;
    
    QJsonObject toJson() const;
    static ROIMetadata fromJson(const QJsonObject& json);
};

/**
 * @brief Base class for all ROI types
 */
class ROIBase : public QObject, public std::enable_shared_from_this<ROIBase> {
    Q_OBJECT
    
public:
    explicit ROIBase(QObject* parent = nullptr);
    virtual ~ROIBase() = default;
    
    // Pure virtual methods
    virtual ROIType getType() const = 0;
    virtual cv::Mat getMask(const cv::Size& frameSize) const = 0;
    virtual cv::Rect2f getBoundingBox() const = 0;
    virtual bool contains(const cv::Point2f& point) const = 0;
    virtual ROIPtr clone() const = 0;
    
    // Virtual methods with default implementation
    virtual cv::Mat extract(const cv::Mat& source) const;
    virtual void apply(cv::Mat& target, const cv::Mat& roiData) const;
    virtual void draw(cv::Mat& canvas, const cv::Scalar& color = cv::Scalar(0, 255, 0), 
                     int thickness = 2) const;
    virtual bool isValid() const;
    virtual QJsonObject toJson() const;
    virtual void fromJson(const QJsonObject& json);
    
    // Common properties
    QString getId() const { return m_metadata.id; }
    QString getName() const { return m_metadata.name; }
    void setName(const QString& name);
    
    bool isEnabled() const { return m_enabled; }
    void setEnabled(bool enabled);
    
    ROIVisualState getVisualState() const { return m_visualState; }
    void setVisualState(ROIVisualState state);
    
    ROIProcessingParams& processingParams() { return m_processingParams; }
    const ROIProcessingParams& processingParams() const { return m_processingParams; }
    
    ROIMetadata& metadata() { return m_metadata; }
    const ROIMetadata& metadata() const { return m_metadata; }
    
    // Hierarchy support
    ROIWeakPtr getParent() const { return m_parent; }
    void setParent(ROIPtr parent);
    
    std::vector<ROIPtr> getChildren() const { return m_children; }
    void addChild(ROIPtr child);
    void removeChild(const QString& childId);
    
    // Transform methods
    virtual void translate(const cv::Point2f& offset);
    virtual void scale(float factor);
    virtual void rotate(float angle);
    virtual void transform(const cv::Mat& transformMatrix);
    
signals:
    void modified();
    void enabledChanged(bool enabled);
    void visualStateChanged(ROIVisualState state);
    void trackingUpdated(const cv::Rect2f& newBounds);
    
protected:
    ROIMetadata m_metadata;
    ROIProcessingParams m_processingParams;
    std::atomic<bool> m_enabled{true};
    ROIVisualState m_visualState{ROIVisualState::Normal};
    
    // Hierarchy
    ROIWeakPtr m_parent;
    std::vector<ROIPtr> m_children;
    
    // Caching
    mutable cv::Mat m_cachedMask;
    mutable cv::Size m_cachedMaskSize;
    mutable bool m_maskDirty{true};
    
    void invalidateCache() const { m_maskDirty = true; }
};

/**
 * @brief Rectangular ROI implementation
 */
class RectangularROI : public ROIBase {
    Q_OBJECT
    
public:
    explicit RectangularROI(QObject* parent = nullptr);
    RectangularROI(const cv::Rect2f& bounds, QObject* parent = nullptr);
    
    ROIType getType() const override { return ROIType::Rectangle; }
    cv::Mat getMask(const cv::Size& frameSize) const override;
    cv::Rect2f getBoundingBox() const override { return m_bounds; }
    bool contains(const cv::Point2f& point) const override;
    ROIPtr clone() const override;
    
    // Rectangle-specific methods
    cv::Rect2f getBounds() const { return m_bounds; }
    void setBounds(const cv::Rect2f& bounds);
    
    float getRotation() const { return m_rotation; }
    void setRotation(float rotation);
    
    cv::Point2f getCenter() const;
    cv::Size2f getSize() const;
    
    void draw(cv::Mat& canvas, const cv::Scalar& color = cv::Scalar(0, 255, 0), 
             int thickness = 2) const override;
    
    QJsonObject toJson() const override;
    void fromJson(const QJsonObject& json) override;
    
protected:
    cv::Rect2f m_bounds;
    float m_rotation{0};  // Rotation in degrees
};

/**
 * @brief Circular ROI implementation
 */
class CircularROI : public ROIBase {
    Q_OBJECT
    
public:
    explicit CircularROI(QObject* parent = nullptr);
    CircularROI(const cv::Point2f& center, float radius, QObject* parent = nullptr);
    
    ROIType getType() const override { return ROIType::Circle; }
    cv::Mat getMask(const cv::Size& frameSize) const override;
    cv::Rect2f getBoundingBox() const override;
    bool contains(const cv::Point2f& point) const override;
    ROIPtr clone() const override;
    
    // Circle-specific methods
    cv::Point2f getCenter() const { return m_center; }
    void setCenter(const cv::Point2f& center);
    
    float getRadius() const { return m_radius; }
    void setRadius(float radius);
    
    float getInnerRadius() const { return m_innerRadius; }
    void setInnerRadius(float radius);  // For annular ROIs
    
    void draw(cv::Mat& canvas, const cv::Scalar& color = cv::Scalar(0, 255, 0), 
             int thickness = 2) const override;
    
    QJsonObject toJson() const override;
    void fromJson(const QJsonObject& json) override;
    
protected:
    cv::Point2f m_center;
    float m_radius;
    float m_innerRadius{0};  // For annular ROIs
};

/**
 * @brief Polygonal ROI implementation
 */
class PolygonalROI : public ROIBase {
    Q_OBJECT
    
public:
    explicit PolygonalROI(QObject* parent = nullptr);
    PolygonalROI(const std::vector<cv::Point2f>& vertices, QObject* parent = nullptr);
    
    ROIType getType() const override { return ROIType::Polygon; }
    cv::Mat getMask(const cv::Size& frameSize) const override;
    cv::Rect2f getBoundingBox() const override;
    bool contains(const cv::Point2f& point) const override;
    ROIPtr clone() const override;
    
    // Polygon-specific methods
    std::vector<cv::Point2f> getVertices() const { return m_vertices; }
    void setVertices(const std::vector<cv::Point2f>& vertices);
    void addVertex(const cv::Point2f& vertex);
    void removeVertex(int index);
    void moveVertex(int index, const cv::Point2f& newPos);
    
    bool isClosed() const { return m_closed; }
    void setClosed(bool closed);
    
    bool isConvex() const;
    double getArea() const;
    double getPerimeter() const;
    
    void draw(cv::Mat& canvas, const cv::Scalar& color = cv::Scalar(0, 255, 0), 
             int thickness = 2) const override;
    
    QJsonObject toJson() const override;
    void fromJson(const QJsonObject& json) override;
    
protected:
    std::vector<cv::Point2f> m_vertices;
    bool m_closed{true};
    mutable bool m_convexityCached{false};
    mutable bool m_isConvex{false};
};

/**
 * @brief Mask-based ROI implementation
 */
class MaskROI : public ROIBase {
    Q_OBJECT
    
public:
    explicit MaskROI(QObject* parent = nullptr);
    MaskROI(const cv::Mat& mask, QObject* parent = nullptr);
    
    ROIType getType() const override { return ROIType::Mask; }
    cv::Mat getMask(const cv::Size& frameSize) const override;
    cv::Rect2f getBoundingBox() const override;
    bool contains(const cv::Point2f& point) const override;
    ROIPtr clone() const override;
    
    // Mask-specific methods
    void setMask(const cv::Mat& mask);
    cv::Mat getMask() const { return m_mask; }
    
    void setWeighted(bool weighted);
    bool isWeighted() const { return m_weighted; }
    
    void addToMask(const cv::Mat& addition);
    void subtractFromMask(const cv::Mat& subtraction);
    void intersectWithMask(const cv::Mat& other);
    
    // Morphological operations
    void dilate(int size = 3);
    void erode(int size = 3);
    void morphologyEx(cv::MorphTypes operation, int size = 3);
    
    void draw(cv::Mat& canvas, const cv::Scalar& color = cv::Scalar(0, 255, 0), 
             int thickness = 2) const override;
    
    QJsonObject toJson() const override;
    void fromJson(const QJsonObject& json) override;
    
protected:
    cv::Mat m_mask;
    cv::Rect m_boundingBox;
    bool m_weighted{false};  // Binary vs weighted mask
    
    void updateBoundingBox();
};

/**
 * @brief Dynamic/Adaptive ROI implementation
 */
class DynamicROI : public ROIBase {
    Q_OBJECT
    
public:
    explicit DynamicROI(QObject* parent = nullptr);
    DynamicROI(ROIPtr baseROI, QObject* parent = nullptr);
    
    ROIType getType() const override { return ROIType::Dynamic; }
    cv::Mat getMask(const cv::Size& frameSize) const override;
    cv::Rect2f getBoundingBox() const override;
    bool contains(const cv::Point2f& point) const override;
    ROIPtr clone() const override;
    
    // Dynamic-specific methods
    void setBaseROI(ROIPtr baseROI);
    ROIPtr getBaseROI() const { return m_baseROI; }
    
    void setAdaptFunction(std::function<ROIPtr(const cv::Mat&, ROIPtr)> func);
    void adapt(const cv::Mat& frame);
    
    // Tracking
    void enableTracking(TrackingMethod method);
    void disableTracking();
    void updateTracking(const cv::Mat& frame);
    float getTrackingConfidence() const { return m_trackingConfidence; }
    
    // Prediction
    void enablePrediction(int frameAhead = 1);
    void disablePrediction();
    cv::Rect2f predictPosition(int framesAhead = 1) const;
    
    void draw(cv::Mat& canvas, const cv::Scalar& color = cv::Scalar(0, 255, 0), 
             int thickness = 2) const override;
    
    QJsonObject toJson() const override;
    void fromJson(const QJsonObject& json) override;
    
signals:
    void trackingLost();
    void trackingRecovered();
    
protected:
    ROIPtr m_baseROI;
    ROIPtr m_currentROI;
    std::function<ROIPtr(const cv::Mat&, ROIPtr)> m_adaptFunction;
    
    // Tracking
    cv::Ptr<cv::Tracker> m_tracker;
    TrackingMethod m_trackingMethod{TrackingMethod::None};
    float m_trackingConfidence{0};
    cv::Mat m_referenceFrame;
    
    // Prediction (Kalman filter)
    cv::KalmanFilter m_kalmanFilter;
    bool m_predictionEnabled{false};
    
    void initializeTracker(const cv::Mat& frame);
    void initializeKalmanFilter();
};

/**
 * @brief Factory class for creating ROI instances
 */
class ROIFactory {
public:
    static ROIPtr create(ROIType type);
    static ROIPtr createFromJson(const QJsonObject& json);
    
    // Convenience creators
    static ROIPtr createRectangle(const cv::Rect2f& bounds);
    static ROIPtr createCircle(const cv::Point2f& center, float radius);
    static ROIPtr createPolygon(const std::vector<cv::Point2f>& vertices);
    static ROIPtr createMask(const cv::Mat& mask);
    static ROIPtr createDynamic(ROIPtr baseROI);
    
    // Template creators
    static ROIPtr createFromTemplate(const QString& templateName);
    static void registerTemplate(const QString& name, const QJsonObject& templateData);
    
private:
    static std::map<QString, QJsonObject> s_templates;
};

} // namespace OpenCV
} // namespace ComponentsForest

#endif // ROI_TYPES_H