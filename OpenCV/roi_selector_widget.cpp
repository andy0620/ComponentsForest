#include "roi_selector_widget.h"
#include <QPainter>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QWheelEvent>
#include <QMenu>
#include <QAction>
#include <QFileDialog>
#include <QMessageBox>
#include <QElapsedTimer>
#include <QScrollBar>
#include <QDebug>
#include <opencv2/imgproc.hpp>

namespace ComponentsForest {
namespace OpenCV {

// ============ ROIGraphicsItem Implementation ============

ROIGraphicsItem::ROIGraphicsItem(ROIPtr roi, QGraphicsItem* parent)
    : QGraphicsItem(parent), m_roi(roi) {
    
    setAcceptHoverEvents(true);
    setFlag(ItemIsSelectable);
    setFlag(ItemIsMovable);
    setFlag(ItemSendsGeometryChanges);
    
    createHandles();
}

QRectF ROIGraphicsItem::boundingRect() const {
    if (!m_roi) return QRectF();
    
    cv::Rect2f bbox = m_roi->getBoundingBox();
    QRectF rect(bbox.x, bbox.y, bbox.width, bbox.height);
    
    // Expand for border and handles
    qreal padding = m_style.borderWidth + m_style.handleSize;
    return rect.adjusted(-padding, -padding, padding, padding);
}

void ROIGraphicsItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
                            QWidget* widget) {
    Q_UNUSED(option)
    Q_UNUSED(widget)
    
    if (!m_roi) return;
    
    painter->setRenderHint(QPainter::Antialiasing);
    
    // Draw ROI
    drawROI(painter);
    
    // Draw handles if selected
    if (m_selected && m_handlesVisible) {
        drawHandles(painter);
    }
    
    // Draw info if enabled
    if (m_style.showTooltip && (m_selected || m_hovered)) {
        drawInfo(painter);
    }
}

QPainterPath ROIGraphicsItem::shape() const {
    QPainterPath path;
    
    if (!m_roi) return path;
    
    cv::Rect2f bbox = m_roi->getBoundingBox();
    QRectF rect(bbox.x, bbox.y, bbox.width, bbox.height);
    
    switch (m_roi->getType()) {
        case ROIType::Rectangle:
            path.addRect(rect);
            break;
            
        case ROIType::Circle: {
            auto circleROI = std::dynamic_pointer_cast<CircularROI>(m_roi);
            if (circleROI) {
                cv::Point2f center = circleROI->getCenter();
                float radius = circleROI->getRadius();
                path.addEllipse(QPointF(center.x, center.y), radius, radius);
            }
            break;
        }
        
        case ROIType::Polygon: {
            auto polyROI = std::dynamic_pointer_cast<PolygonalROI>(m_roi);
            if (polyROI) {
                QPolygonF polygon;
                for (const auto& vertex : polyROI->getVertices()) {
                    polygon << QPointF(vertex.x, vertex.y);
                }
                path.addPolygon(polygon);
            }
            break;
        }
        
        default:
            path.addRect(rect);
            break;
    }
    
    return path;
}

void ROIGraphicsItem::updateFromROI() {
    prepareGeometryChange();
    updateHandles();
    update();
}

void ROIGraphicsItem::setVisualStyle(const ROIVisualStyle& style) {
    m_style = style;
    update();
}

void ROIGraphicsItem::setSelected(bool selected) {
    m_selected = selected;
    m_roi->setVisualState(selected ? ROIVisualState::Selected : ROIVisualState::Normal);
    updateHandles();
    update();
}

void ROIGraphicsItem::setHovered(bool hovered) {
    m_hovered = hovered;
    m_roi->setVisualState(hovered ? ROIVisualState::Hover : ROIVisualState::Normal);
    update();
}

void ROIGraphicsItem::setProcessing(bool processing) {
    m_processing = processing;
    m_roi->setVisualState(processing ? ROIVisualState::Processing : ROIVisualState::Normal);
    
    if (processing && m_style.animateBorder) {
        startPulseAnimation();
    } else {
        stopPulseAnimation();
    }
}

void ROIGraphicsItem::setError(bool error) {
    m_error = error;
    m_roi->setVisualState(error ? ROIVisualState::Error : ROIVisualState::Normal);
    
    if (error && m_style.flashOnError) {
        flashError();
    }
}

void ROIGraphicsItem::showHandles(bool show) {
    m_handlesVisible = show;
    
    for (auto handle : m_handles) {
        handle->setVisible(show);
    }
    
    update();
}

HandleType ROIGraphicsItem::getHandleAt(const QPointF& pos) const {
    for (auto handle : m_handles) {
        if (handle->contains(mapFromScene(pos))) {
            return handle->getType();
        }
    }
    return HandleType::None;
}

void ROIGraphicsItem::startPulseAnimation() {
    if (!m_pulseAnimation) {
        m_pulseAnimation = new QPropertyAnimation(this, "opacity");
        m_pulseAnimation->setDuration(1000 / m_style.pulseSpeed);
        m_pulseAnimation->setStartValue(1.0);
        m_pulseAnimation->setEndValue(0.5);
        m_pulseAnimation->setLoopCount(-1);
        m_pulseAnimation->setEasingCurve(QEasingCurve::InOutSine);
    }
    
    m_pulseAnimation->start();
}

void ROIGraphicsItem::stopPulseAnimation() {
    if (m_pulseAnimation) {
        m_pulseAnimation->stop();
        setOpacity(1.0);
    }
}

void ROIGraphicsItem::flashError() {
    if (!m_errorAnimation) {
        m_errorAnimation = new QPropertyAnimation(this, "opacity");
        m_errorAnimation->setDuration(m_style.flashDuration);
        m_errorAnimation->setKeyValueAt(0, 1.0);
        m_errorAnimation->setKeyValueAt(0.25, 0.3);
        m_errorAnimation->setKeyValueAt(0.5, 1.0);
        m_errorAnimation->setKeyValueAt(0.75, 0.3);
        m_errorAnimation->setEndValue(1.0);
    }
    
    m_errorAnimation->start();
}

void ROIGraphicsItem::hoverEnterEvent(QGraphicsSceneHoverEvent* event) {
    Q_UNUSED(event)
    setHovered(true);
}

void ROIGraphicsItem::hoverLeaveEvent(QGraphicsSceneHoverEvent* event) {
    Q_UNUSED(event)
    setHovered(false);
}

void ROIGraphicsItem::createHandles() {
    // Clear existing handles
    for (auto handle : m_handles) {
        delete handle;
    }
    m_handles.clear();
    
    // Create handles based on ROI type
    if (m_roi->getType() == ROIType::Rectangle) {
        // Corner handles
        m_handles.append(new ROIHandle(HandleType::TopLeft, this));
        m_handles.append(new ROIHandle(HandleType::TopRight, this));
        m_handles.append(new ROIHandle(HandleType::BottomLeft, this));
        m_handles.append(new ROIHandle(HandleType::BottomRight, this));
        
        // Edge handles
        m_handles.append(new ROIHandle(HandleType::TopCenter, this));
        m_handles.append(new ROIHandle(HandleType::BottomCenter, this));
        m_handles.append(new ROIHandle(HandleType::MiddleLeft, this));
        m_handles.append(new ROIHandle(HandleType::MiddleRight, this));
        
        // Rotation handle
        m_handles.append(new ROIHandle(HandleType::Rotation, this));
    } else if (m_roi->getType() == ROIType::Circle) {
        // Radius handles
        m_handles.append(new ROIHandle(HandleType::TopCenter, this));
        m_handles.append(new ROIHandle(HandleType::BottomCenter, this));
        m_handles.append(new ROIHandle(HandleType::MiddleLeft, this));
        m_handles.append(new ROIHandle(HandleType::MiddleRight, this));
        
        // Center handle
        m_handles.append(new ROIHandle(HandleType::Center, this));
    }
    
    updateHandles();
}

void ROIGraphicsItem::updateHandles() {
    if (!m_roi) return;
    
    cv::Rect2f bbox = m_roi->getBoundingBox();
    QRectF rect(bbox.x, bbox.y, bbox.width, bbox.height);
    
    for (auto handle : m_handles) {
        QPointF pos;
        
        switch (handle->getType()) {
            case HandleType::TopLeft:
                pos = rect.topLeft();
                break;
            case HandleType::TopCenter:
                pos = QPointF(rect.center().x(), rect.top());
                break;
            case HandleType::TopRight:
                pos = rect.topRight();
                break;
            case HandleType::MiddleLeft:
                pos = QPointF(rect.left(), rect.center().y());
                break;
            case HandleType::MiddleRight:
                pos = QPointF(rect.right(), rect.center().y());
                break;
            case HandleType::BottomLeft:
                pos = rect.bottomLeft();
                break;
            case HandleType::BottomCenter:
                pos = QPointF(rect.center().x(), rect.bottom());
                break;
            case HandleType::BottomRight:
                pos = rect.bottomRight();
                break;
            case HandleType::Rotation:
                pos = QPointF(rect.center().x(), rect.top() - 30);
                break;
            case HandleType::Center:
                pos = rect.center();
                break;
            default:
                break;
        }
        
        handle->setPosition(pos);
    }
}

void ROIGraphicsItem::drawROI(QPainter* painter) {
    if (!m_roi) return;
    
    // Determine color based on state
    QColor color = m_style.borderColor;
    float width = m_style.borderWidth;
    
    if (m_error) {
        color = m_style.errorColor;
    } else if (m_processing) {
        color = m_style.activeColor;
    } else if (m_selected) {
        color = m_style.selectedColor;
        width = m_style.selectedWidth;
    } else if (m_hovered) {
        color = m_style.hoverColor;
        width = m_style.hoverWidth;
    }
    
    QPen pen(color, width, m_style.borderStyle);
    painter->setPen(pen);
    painter->setBrush(Qt::NoBrush);
    
    // Draw based on ROI type
    cv::Rect2f bbox = m_roi->getBoundingBox();
    QRectF rect(bbox.x, bbox.y, bbox.width, bbox.height);
    
    switch (m_roi->getType()) {
        case ROIType::Rectangle:
            painter->drawRect(rect);
            break;
            
        case ROIType::Circle: {
            auto circleROI = std::dynamic_pointer_cast<CircularROI>(m_roi);
            if (circleROI) {
                cv::Point2f center = circleROI->getCenter();
                float radius = circleROI->getRadius();
                painter->drawEllipse(QPointF(center.x, center.y), radius, radius);
                
                // Draw inner circle for annular ROI
                float innerRadius = circleROI->getInnerRadius();
                if (innerRadius > 0) {
                    painter->drawEllipse(QPointF(center.x, center.y), innerRadius, innerRadius);
                }
            }
            break;
        }
        
        case ROIType::Polygon: {
            auto polyROI = std::dynamic_pointer_cast<PolygonalROI>(m_roi);
            if (polyROI) {
                QPolygonF polygon;
                for (const auto& vertex : polyROI->getVertices()) {
                    polygon << QPointF(vertex.x, vertex.y);
                }
                
                if (polyROI->isClosed()) {
                    painter->drawPolygon(polygon);
                } else {
                    painter->drawPolyline(polygon);
                }
                
                // Draw vertices if selected
                if (m_selected) {
                    painter->setBrush(color);
                    for (const auto& point : polygon) {
                        painter->drawEllipse(point, 3, 3);
                    }
                    painter->setBrush(Qt::NoBrush);
                }
            }
            break;
        }
        
        default:
            painter->drawRect(rect);
            break;
    }
}

void ROIGraphicsItem::drawHandles(QPainter* painter) {
    if (!m_handlesVisible) return;
    
    for (auto handle : m_handles) {
        handle->paint(painter, nullptr, nullptr);
    }
}

void ROIGraphicsItem::drawInfo(QPainter* painter) {
    if (!m_roi) return;
    
    cv::Rect2f bbox = m_roi->getBoundingBox();
    QRectF rect(bbox.x, bbox.y, bbox.width, bbox.height);
    
    // Prepare info text
    QString info = QString("%1\n%2x%3\n(%4, %5)")
        .arg(m_roi->getName())
        .arg(static_cast<int>(bbox.width))
        .arg(static_cast<int>(bbox.height))
        .arg(static_cast<int>(bbox.x))
        .arg(static_cast<int>(bbox.y));
    
    // Draw background
    QFontMetrics fm(painter->font());
    QRectF textRect = fm.boundingRect(info);
    textRect.moveTopLeft(rect.topLeft() + QPointF(5, -textRect.height() - 5));
    
    painter->fillRect(textRect.adjusted(-3, -3, 3, 3), 
                     QColor(0, 0, 0, 150));
    
    // Draw text
    painter->setPen(Qt::white);
    painter->drawText(textRect, Qt::AlignLeft, info);
}

// ============ ROIHandle Implementation ============

ROIHandle::ROIHandle(HandleType type, ROIGraphicsItem* parent)
    : QGraphicsItem(parent), m_type(type), m_roiItem(parent) {
    
    setAcceptHoverEvents(true);
    setFlag(ItemIsSelectable, false);
    setFlag(ItemIsMovable, false);
}

QRectF ROIHandle::boundingRect() const {
    float size = m_roiItem->getVisualStyle().handleSize;
    return QRectF(-size/2, -size/2, size, size);
}

void ROIHandle::paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
                     QWidget* widget) {
    Q_UNUSED(option)
    Q_UNUSED(widget)
    
    ROIVisualStyle style = m_roiItem->getVisualStyle();
    
    painter->setRenderHint(QPainter::Antialiasing);
    
    // Draw handle
    QRectF rect = boundingRect();
    
    QPen pen(style.handleBorderColor, style.handleBorderWidth);
    painter->setPen(pen);
    
    QBrush brush(m_hovered ? style.handleBorderColor : style.handleFillColor);
    painter->setBrush(brush);
    
    if (m_type == HandleType::Rotation) {
        // Draw rotation handle as circle
        painter->drawEllipse(rect);
    } else {
        // Draw resize handles as squares
        painter->drawRect(rect);
    }
}

void ROIHandle::setPosition(const QPointF& pos) {
    setPos(pos);
}

void ROIHandle::hoverEnterEvent(QGraphicsSceneHoverEvent* event) {
    Q_UNUSED(event)
    m_hovered = true;
    setCursor(getCursorForHandle());
    update();
}

void ROIHandle::hoverLeaveEvent(QGraphicsSceneHoverEvent* event) {
    Q_UNUSED(event)
    m_hovered = false;
    unsetCursor();
    update();
}

QCursor ROIHandle::getCursorForHandle() const {
    switch (m_type) {
        case HandleType::TopLeft:
        case HandleType::BottomRight:
            return Qt::SizeFDiagCursor;
            
        case HandleType::TopRight:
        case HandleType::BottomLeft:
            return Qt::SizeBDiagCursor;
            
        case HandleType::TopCenter:
        case HandleType::BottomCenter:
            return Qt::SizeVerCursor;
            
        case HandleType::MiddleLeft:
        case HandleType::MiddleRight:
            return Qt::SizeHorCursor;
            
        case HandleType::Rotation:
            return Qt::ClosedHandCursor;
            
        case HandleType::Center:
            return Qt::SizeAllCursor;
            
        default:
            return Qt::ArrowCursor;
    }
}

// ============ ROISelectorWidget Implementation ============

ROISelectorWidget::ROISelectorWidget(QWidget* parent)
    : QGraphicsView(parent) {
    
    setupScene();
    
    // Set view properties
    setRenderHint(QPainter::Antialiasing);
    setViewportUpdateMode(BoundingRectViewportUpdate);
    setDragMode(NoDrag);
    setOptimizationFlags(DontSavePainterState);
    setMouseTracking(true);
    
    // Create undo stack
    m_undoStack = new QUndoStack(this);
}

ROISelectorWidget::~ROISelectorWidget() {
    disconnectROIManager();
}

void ROISelectorWidget::setROIManager(ROIManager* manager) {
    if (m_roiManager) {
        disconnectROIManager();
    }
    
    m_roiManager = manager;
    
    if (m_roiManager) {
        connectROIManager();
        
        // Add existing ROIs
        auto rois = m_roiManager->getAllROIs();
        for (const auto& roi : rois) {
            addROIItem(roi);
        }
    }
}

void ROISelectorWidget::setImage(const cv::Mat& image) {
    if (image.empty()) return;
    
    m_currentImage = image.clone();
    
    // Convert to QImage
    QImage qImage;
    if (image.channels() == 1) {
        qImage = QImage(image.data, image.cols, image.rows, image.step, QImage::Format_Grayscale8);
    } else if (image.channels() == 3) {
        cv::Mat rgb;
        cv::cvtColor(image, rgb, cv::COLOR_BGR2RGB);
        qImage = QImage(rgb.data, rgb.cols, rgb.rows, rgb.step, QImage::Format_RGB888);
    } else if (image.channels() == 4) {
        cv::Mat rgba;
        cv::cvtColor(image, rgba, cv::COLOR_BGRA2RGBA);
        qImage = QImage(rgba.data, rgba.cols, rgba.rows, rgba.step, QImage::Format_RGBA8888);
    }
    
    // Update image item
    if (!m_imageItem) {
        m_imageItem = m_scene->addPixmap(QPixmap::fromImage(qImage));
        m_imageItem->setZValue(-1);  // Behind ROIs
    } else {
        m_imageItem->setPixmap(QPixmap::fromImage(qImage));
    }
    
    // Fit to view
    zoomFit();
}

void ROISelectorWidget::clearImage() {
    if (m_imageItem) {
        m_scene->removeItem(m_imageItem);
        delete m_imageItem;
        m_imageItem = nullptr;
    }
    
    m_currentImage = cv::Mat();
}

void ROISelectorWidget::setInteractionMode(InteractionMode mode) {
    m_interactionMode = mode;
    
    // Cancel any ongoing creation
    if (m_creating) {
        cancelCreation();
    }
    
    // Update cursor
    switch (mode) {
        case InteractionMode::Select:
            setCursor(Qt::ArrowCursor);
            break;
        case InteractionMode::Create:
            setCursor(Qt::CrossCursor);
            break;
        case InteractionMode::Pan:
            setCursor(Qt::OpenHandCursor);
            break;
        case InteractionMode::Zoom:
            setCursor(Qt::CrossCursor);  // Use CrossCursor instead of deprecated MagnifyingGlassCursor
            break;
        default:
            setCursor(Qt::ArrowCursor);
            break;
    }
    
    emit interactionModeChanged(mode);
}

void ROISelectorWidget::setCreationTool(CreationTool tool) {
    m_creationTool = tool;
    emit creationToolChanged(tool);
}

void ROISelectorWidget::setDefaultVisualStyle(const ROIVisualStyle& style) {
    m_defaultStyle = style;
    
    // Update existing items
    for (auto& [id, item] : m_roiItems) {
        item->setVisualStyle(style);
    }
}

void ROISelectorWidget::setSnappingOptions(const SnappingOptions& options) {
    m_snappingOptions = options;
}

void ROISelectorWidget::setShowGrid(bool show) {
    m_showGrid = show;
    update();
}

void ROISelectorWidget::setGridSize(const QSizeF& size) {
    m_snappingOptions.gridSize = size;
    if (m_showGrid) {
        update();
    }
}

void ROISelectorWidget::addGuide(const QLineF& guide) {
    m_snappingOptions.guides.append(guide);
    update();
}

void ROISelectorWidget::removeGuide(int index) {
    if (index >= 0 && index < m_snappingOptions.guides.size()) {
        m_snappingOptions.guides.removeAt(index);
        update();
    }
}

void ROISelectorWidget::clearGuides() {
    m_snappingOptions.guides.clear();
    update();
}

void ROISelectorWidget::selectROI(const QString& roiId) {
    if (m_roiManager) {
        m_roiManager->selectROI(roiId);
    }
    
    auto it = m_roiItems.find(roiId);
    if (it != m_roiItems.end()) {
        it->second->setSelected(true);
    }
}

void ROISelectorWidget::selectROIs(const QList<QString>& roiIds) {
    deselectAll();
    
    for (const auto& id : roiIds) {
        selectROI(id);
    }
}

void ROISelectorWidget::selectAll() {
    if (m_roiManager) {
        m_roiManager->selectAll();
    }
}

void ROISelectorWidget::deselectAll() {
    if (m_roiManager) {
        m_roiManager->deselectAll();
    }
}

QList<QString> ROISelectorWidget::selectedROIs() const {
    if (m_roiManager) {
        auto selected = m_roiManager->getSelectedROIs();
        return QList<QString>(selected.begin(), selected.end());
    }
    return QList<QString>();
}

void ROISelectorWidget::zoomIn() {
    scale(1.25, 1.25);
    m_zoom *= 1.25;
}

void ROISelectorWidget::zoomOut() {
    scale(0.8, 0.8);
    m_zoom *= 0.8;
}

void ROISelectorWidget::zoomFit() {
    if (!m_scene) return;
    
    QRectF sceneRect = m_scene->itemsBoundingRect();
    if (sceneRect.isEmpty()) return;
    
    fitInView(sceneRect, Qt::KeepAspectRatio);
    
    // Calculate zoom level
    QRectF viewRect = viewport()->rect();
    qreal scaleX = viewRect.width() / sceneRect.width();
    qreal scaleY = viewRect.height() / sceneRect.height();
    m_zoom = qMin(scaleX, scaleY);
}

void ROISelectorWidget::zoomActual() {
    resetTransform();
    m_zoom = 1.0;
}

void ROISelectorWidget::setZoom(qreal zoom) {
    qreal scale = zoom / m_zoom;
    this->scale(scale, scale);
    m_zoom = zoom;
}

// Slot implementations for ROI operations

void ROISelectorWidget::deleteSelectedROIs() {
    auto selected = selectedROIs();
    
    if (selected.isEmpty()) return;
    
    // Create compound command for undo
    auto compound = new QUndoCommand("Delete ROIs");
    
    for (const auto& id : selected) {
        new DeleteROICommand(m_roiManager, id, compound);
    }
    
    m_undoStack->push(compound);
}

void ROISelectorWidget::duplicateSelectedROIs() {
    auto selected = selectedROIs();
    
    if (selected.isEmpty() || !m_roiManager) return;
    
    for (const auto& id : selected) {
        auto roi = m_roiManager->getROI(id);
        if (roi) {
            auto cloned = roi->clone();
            
            // Offset the clone
            cv::Point2f offset(20, 20);
            cloned->translate(offset);
            
            m_roiManager->addROI(cloned);
        }
    }
}

void ROISelectorWidget::groupSelectedROIs() {
    auto selected = selectedROIs();
    
    if (selected.size() < 2 || !m_roiManager) return;
    
    std::vector<QString> ids(selected.begin(), selected.end());
    m_roiManager->createGroup("Group", ids);
}

void ROISelectorWidget::ungroupSelectedROIs() {
    // Implementation depends on group management
}

void ROISelectorWidget::bringToFront() {
    auto selected = selectedROIs();
    
    if (selected.isEmpty() || !m_roiManager) return;
    
    for (const auto& id : selected) {
        m_roiManager->bringToFront(id);
    }
}

void ROISelectorWidget::sendToBack() {
    auto selected = selectedROIs();
    
    if (selected.isEmpty() || !m_roiManager) return;
    
    for (const auto& id : selected) {
        m_roiManager->sendToBack(id);
    }
}

void ROISelectorWidget::bringForward() {
    auto selected = selectedROIs();
    
    if (selected.isEmpty() || !m_roiManager) return;
    
    for (const auto& id : selected) {
        m_roiManager->bringForward(id);
    }
}

void ROISelectorWidget::sendBackward() {
    auto selected = selectedROIs();
    
    if (selected.isEmpty() || !m_roiManager) return;
    
    for (const auto& id : selected) {
        m_roiManager->sendBackward(id);
    }
}

// Event handlers

void ROISelectorWidget::mousePressEvent(QMouseEvent* event) {
    QPointF scenePos = mapToScene(event->pos());
    QPointF imagePos = sceneToImage(scenePos);
    
    // Store keyboard modifiers
    m_shiftPressed = event->modifiers() & Qt::ShiftModifier;
    m_ctrlPressed = event->modifiers() & Qt::ControlModifier;
    m_altPressed = event->modifiers() & Qt::AltModifier;
    
    if (event->button() == Qt::LeftButton) {
        switch (m_interactionMode) {
            case InteractionMode::Select:
                if (m_roiManager) {
                    cv::Point2f cvPoint(imagePos.x(), imagePos.y());
                    auto roi = m_roiManager->hitTest(cvPoint);
                    
                    if (roi) {
                        // Check for handle
                        auto it = m_roiItems.find(roi->getId());
                        if (it != m_roiItems.end()) {
                            HandleType handle = it->second->getHandleAt(scenePos);
                            if (handle != HandleType::None) {
                                // Start handle drag
                                startDrag(scenePos);
                                m_dragHandle = handle;
                                m_dragItem = it->second;
                            } else {
                                // Select/deselect ROI
                                if (m_ctrlPressed) {
                                    // Toggle selection
                                    if (roi->getVisualState() == ROIVisualState::Selected) {
                                        m_roiManager->deselectROI(roi->getId());
                                    } else {
                                        m_roiManager->selectROI(roi->getId());
                                    }
                                } else if (!m_shiftPressed) {
                                    // Single selection
                                    m_roiManager->deselectAll();
                                    m_roiManager->selectROI(roi->getId());
                                }
                                
                                // Start drag
                                startDrag(scenePos);
                                m_dragItem = it->second;
                            }
                        }
                    } else {
                        // Start selection rectangle
                        if (!m_ctrlPressed && !m_shiftPressed) {
                            m_roiManager->deselectAll();
                        }
                        startSelection(scenePos);
                    }
                }
                break;
                
            case InteractionMode::Create:
                startCreation(imagePos);
                break;
                
            case InteractionMode::Pan:
                m_panning = true;
                m_panStart = event->pos();
                setCursor(Qt::ClosedHandCursor);
                break;
                
            case InteractionMode::Zoom:
                if (m_shiftPressed) {
                    zoomOut();
                } else {
                    zoomIn();
                }
                break;
        }
    } else if (event->button() == Qt::RightButton) {
        showContextMenu(event->pos());
    }
    
    QGraphicsView::mousePressEvent(event);
}

void ROISelectorWidget::mouseMoveEvent(QMouseEvent* event) {
    QPointF scenePos = mapToScene(event->pos());
    QPointF imagePos = sceneToImage(scenePos);
    
    emit mousePositionChanged(imagePos);
    
    if (event->buttons() & Qt::LeftButton) {
        if (m_creating) {
            updateCreation(imagePos);
        } else if (m_selecting) {
            updateSelection(scenePos);
        } else if (m_dragging) {
            updateDrag(scenePos);
        } else if (m_panning) {
            QPointF delta = event->pos() - m_panStart;
            horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
            verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());
            m_panStart = event->pos();
        }
    }
    
    QGraphicsView::mouseMoveEvent(event);
}

void ROISelectorWidget::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        if (m_creating) {
            finishCreation();
        } else if (m_selecting) {
            finishSelection();
        } else if (m_dragging) {
            finishDrag();
        } else if (m_panning) {
            m_panning = false;
            setCursor(Qt::OpenHandCursor);
        }
    }
    
    QGraphicsView::mouseReleaseEvent(event);
}

void ROISelectorWidget::wheelEvent(QWheelEvent* event) {
    if (m_ctrlPressed) {
        // Zoom with Ctrl+Wheel
        qreal scaleFactor = 1.15;
        if (event->angleDelta().y() > 0) {
            scale(scaleFactor, scaleFactor);
            m_zoom *= scaleFactor;
        } else {
            scale(1.0 / scaleFactor, 1.0 / scaleFactor);
            m_zoom /= scaleFactor;
        }
    } else {
        QGraphicsView::wheelEvent(event);
    }
}

void ROISelectorWidget::keyPressEvent(QKeyEvent* event) {
    m_shiftPressed = event->modifiers() & Qt::ShiftModifier;
    m_ctrlPressed = event->modifiers() & Qt::ControlModifier;
    m_altPressed = event->modifiers() & Qt::AltModifier;
    
    switch (event->key()) {
        case Qt::Key_Delete:
            deleteSelectedROIs();
            break;
            
        case Qt::Key_Escape:
            if (m_creating) {
                cancelCreation();
            } else {
                deselectAll();
            }
            break;
            
        case Qt::Key_A:
            if (m_ctrlPressed) {
                selectAll();
            }
            break;
            
        case Qt::Key_D:
            if (m_ctrlPressed) {
                duplicateSelectedROIs();
            }
            break;
            
        case Qt::Key_Z:
            if (m_ctrlPressed) {
                if (m_shiftPressed) {
                    m_undoStack->redo();
                } else {
                    m_undoStack->undo();
                }
            }
            break;
    }
    
    QGraphicsView::keyPressEvent(event);
}

void ROISelectorWidget::keyReleaseEvent(QKeyEvent* event) {
    m_shiftPressed = event->modifiers() & Qt::ShiftModifier;
    m_ctrlPressed = event->modifiers() & Qt::ControlModifier;
    m_altPressed = event->modifiers() & Qt::AltModifier;
    
    QGraphicsView::keyReleaseEvent(event);
}

void ROISelectorWidget::drawBackground(QPainter* painter, const QRectF& rect) {
    QGraphicsView::drawBackground(painter, rect);
    
    if (m_showGrid) {
        painter->setPen(QPen(QColor(200, 200, 200, 50), 0.5));
        
        qreal left = qFloor(rect.left() / m_snappingOptions.gridSize.width()) * 
                    m_snappingOptions.gridSize.width();
        qreal top = qFloor(rect.top() / m_snappingOptions.gridSize.height()) * 
                   m_snappingOptions.gridSize.height();
        
        for (qreal x = left; x <= rect.right(); x += m_snappingOptions.gridSize.width()) {
            painter->drawLine(QLineF(x, rect.top(), x, rect.bottom()));
        }
        
        for (qreal y = top; y <= rect.bottom(); y += m_snappingOptions.gridSize.height()) {
            painter->drawLine(QLineF(rect.left(), y, rect.right(), y));
        }
    }
}

void ROISelectorWidget::drawForeground(QPainter* painter, const QRectF& rect) {
    QGraphicsView::drawForeground(painter, rect);
    
    // Draw guides
    painter->setPen(QPen(QColor(0, 150, 255, 100), 1, Qt::DashLine));
    for (const auto& guide : m_snappingOptions.guides) {
        painter->drawLine(guide);
    }
}

// Private methods

void ROISelectorWidget::setupScene() {
    m_scene = new QGraphicsScene(this);
    setScene(m_scene);
}

void ROISelectorWidget::connectROIManager() {
    if (!m_roiManager) return;
    
    connect(m_roiManager, &ROIManager::roiAdded,
            this, &ROISelectorWidget::onROIAdded);
    connect(m_roiManager, &ROIManager::roiRemoved,
            this, &ROISelectorWidget::onROIRemoved);
    connect(m_roiManager, &ROIManager::roiModified,
            this, &ROISelectorWidget::onROIModified);
    connect(m_roiManager, &ROIManager::selectionChanged,
            this, &ROISelectorWidget::onROISelectionChanged);
}

void ROISelectorWidget::disconnectROIManager() {
    if (!m_roiManager) return;
    
    disconnect(m_roiManager, &ROIManager::roiAdded,
               this, &ROISelectorWidget::onROIAdded);
    disconnect(m_roiManager, &ROIManager::roiRemoved,
               this, &ROISelectorWidget::onROIRemoved);
    disconnect(m_roiManager, &ROIManager::roiModified,
               this, &ROISelectorWidget::onROIModified);
    disconnect(m_roiManager, &ROIManager::selectionChanged,
               this, &ROISelectorWidget::onROISelectionChanged);
    
    // Clear items
    for (auto& [id, item] : m_roiItems) {
        m_scene->removeItem(item);
        delete item;
    }
    m_roiItems.clear();
}

void ROISelectorWidget::addROIItem(ROIPtr roi) {
    if (!roi) return;
    
    auto item = new ROIGraphicsItem(roi);
    item->setVisualStyle(m_defaultStyle);
    
    m_scene->addItem(item);
    m_roiItems[roi->getId()] = item;
}

void ROISelectorWidget::removeROIItem(const QString& roiId) {
    auto it = m_roiItems.find(roiId);
    if (it != m_roiItems.end()) {
        m_scene->removeItem(it->second);
        delete it->second;
        m_roiItems.erase(it);
    }
}

void ROISelectorWidget::updateROIItem(const QString& roiId) {
    auto it = m_roiItems.find(roiId);
    if (it != m_roiItems.end()) {
        it->second->updateFromROI();
    }
}

void ROISelectorWidget::startCreation(const QPointF& pos) {
    m_creating = true;
    m_creationStart = snapPoint(pos);
    m_creationCurrent = m_creationStart;
    
    // Create initial ROI based on tool
    switch (m_creationTool) {
        case CreationTool::Rectangle:
            m_creationROI = ROIFactory::createRectangle(
                cv::Rect2f(m_creationStart.x(), m_creationStart.y(), 1, 1));
            break;
            
        case CreationTool::Circle:
            m_creationROI = ROIFactory::createCircle(
                cv::Point2f(m_creationStart.x(), m_creationStart.y()), 1);
            break;
            
        case CreationTool::Polygon:
            m_creationROI = ROIFactory::createPolygon({
                cv::Point2f(m_creationStart.x(), m_creationStart.y())
            });
            break;
            
        default:
            m_creating = false;
            return;
    }
    
    // Add preview item
    if (m_creationROI) {
        m_creationPreview = new ROIGraphicsItem(m_creationROI);
        m_creationPreview->setVisualStyle(m_defaultStyle);
        m_creationPreview->setOpacity(0.5);
        m_scene->addItem(m_creationPreview);
    }
}

void ROISelectorWidget::updateCreation(const QPointF& pos) {
    if (!m_creating || !m_creationROI) return;
    
    m_creationCurrent = snapPoint(pos);
    
    switch (m_creationTool) {
        case CreationTool::Rectangle: {
            auto rectROI = std::dynamic_pointer_cast<RectangularROI>(m_creationROI);
            if (rectROI) {
                float x = qMin(m_creationStart.x(), m_creationCurrent.x());
                float y = qMin(m_creationStart.y(), m_creationCurrent.y());
                float w = qAbs(m_creationCurrent.x() - m_creationStart.x());
                float h = qAbs(m_creationCurrent.y() - m_creationStart.y());
                
                if (m_shiftPressed) {
                    // Square constraint
                    w = h = qMax(w, h);
                }
                
                rectROI->setBounds(cv::Rect2f(x, y, w, h));
            }
            break;
        }
        
        case CreationTool::Circle: {
            auto circleROI = std::dynamic_pointer_cast<CircularROI>(m_creationROI);
            if (circleROI) {
                float dx = m_creationCurrent.x() - m_creationStart.x();
                float dy = m_creationCurrent.y() - m_creationStart.y();
                float radius = std::sqrt(dx * dx + dy * dy);
                
                circleROI->setRadius(radius);
            }
            break;
        }
        
        case CreationTool::Polygon: {
            auto polyROI = std::dynamic_pointer_cast<PolygonalROI>(m_creationROI);
            if (polyROI) {
                polyROI->addVertex(cv::Point2f(m_creationCurrent.x(), m_creationCurrent.y()));
            }
            break;
        }
    }
    
    if (m_creationPreview) {
        static_cast<ROIGraphicsItem*>(m_creationPreview)->updateFromROI();
    }
}

void ROISelectorWidget::finishCreation() {
    if (!m_creating || !m_creationROI) return;
    
    // Remove preview
    if (m_creationPreview) {
        m_scene->removeItem(m_creationPreview);
        delete m_creationPreview;
        m_creationPreview = nullptr;
    }
    
    // Add to manager
    if (m_roiManager) {
        auto command = new CreateROICommand(m_roiManager, m_creationROI);
        m_undoStack->push(command);
    }
    
    emit roiCreated(m_creationROI);
    
    m_creating = false;
    m_creationROI = nullptr;
}

void ROISelectorWidget::cancelCreation() {
    if (!m_creating) return;
    
    // Remove preview
    if (m_creationPreview) {
        m_scene->removeItem(m_creationPreview);
        delete m_creationPreview;
        m_creationPreview = nullptr;
    }
    
    m_creating = false;
    m_creationROI = nullptr;
}

void ROISelectorWidget::startSelection(const QPointF& pos) {
    m_selecting = true;
    m_selectionStart = pos;
    m_selectionRect = QRectF(pos, pos);
    
    // Create rubber band
    if (!m_selectionRubberBand) {
        m_selectionRubberBand = m_scene->addRect(m_selectionRect);
        m_selectionRubberBand->setPen(QPen(Qt::blue, 1, Qt::DashLine));
        m_selectionRubberBand->setBrush(QBrush(QColor(0, 0, 255, 30)));
    }
}

void ROISelectorWidget::updateSelection(const QPointF& pos) {
    if (!m_selecting) return;
    
    m_selectionRect = QRectF(m_selectionStart, pos).normalized();
    
    if (m_selectionRubberBand) {
        m_selectionRubberBand->setRect(m_selectionRect);
    }
}

void ROISelectorWidget::finishSelection() {
    if (!m_selecting) return;
    
    // Select ROIs in rectangle
    if (m_roiManager) {
        for (auto& [id, item] : m_roiItems) {
            if (m_selectionRect.intersects(item->boundingRect())) {
                m_roiManager->selectROI(id);
            }
        }
    }
    
    // Remove rubber band
    if (m_selectionRubberBand) {
        m_scene->removeItem(m_selectionRubberBand);
        delete m_selectionRubberBand;
        m_selectionRubberBand = nullptr;
    }
    
    m_selecting = false;
}

void ROISelectorWidget::startDrag(const QPointF& pos) {
    m_dragging = true;
    m_dragStart = pos;
}

void ROISelectorWidget::updateDrag(const QPointF& pos) {
    if (!m_dragging || !m_dragItem) return;
    
    QPointF delta = snapPoint(pos) - m_dragStart;
    
    if (m_dragHandle != HandleType::None) {
        // Handle drag - resize/rotate
        // Implementation depends on handle type and ROI type
    } else {
        // Move drag
        auto roi = m_dragItem->getROI();
        if (roi) {
            cv::Point2f offset(delta.x(), delta.y());
            roi->translate(offset);
            m_dragItem->updateFromROI();
        }
    }
    
    m_dragStart = snapPoint(pos);
}

void ROISelectorWidget::finishDrag() {
    m_dragging = false;
    m_dragHandle = HandleType::None;
    m_dragItem = nullptr;
}

QPointF ROISelectorWidget::snapPoint(const QPointF& point) {
    if (!m_snappingOptions.enableSnapping) {
        return point;
    }
    
    QPointF snapped = point;
    
    // Grid snapping
    if (m_snappingOptions.snapToGrid) {
        qreal gridX = m_snappingOptions.gridSize.width();
        qreal gridY = m_snappingOptions.gridSize.height();
        
        snapped.setX(qRound(point.x() / gridX) * gridX);
        snapped.setY(qRound(point.y() / gridY) * gridY);
    }
    
    // Guide snapping
    if (m_snappingOptions.snapToGuides) {
        for (const auto& guide : m_snappingOptions.guides) {
            // Simplified guide snapping
            // Full implementation would check distance to guide
        }
    }
    
    return snapped;
}

void ROISelectorWidget::showContextMenu(const QPoint& pos) {
    QMenu menu(this);
    
    // ROI operations
    auto deleteAction = menu.addAction("Delete", this, &ROISelectorWidget::deleteSelectedROIs);
    deleteAction->setShortcut(QKeySequence::Delete);
    deleteAction->setEnabled(!selectedROIs().isEmpty());
    
    auto duplicateAction = menu.addAction("Duplicate", this, &ROISelectorWidget::duplicateSelectedROIs);
    duplicateAction->setShortcut(QKeySequence("Ctrl+D"));
    duplicateAction->setEnabled(!selectedROIs().isEmpty());
    
    menu.addSeparator();
    
    // Arrangement
    auto arrangeMenu = menu.addMenu("Arrange");
    arrangeMenu->addAction("Bring to Front", this, &ROISelectorWidget::bringToFront);
    arrangeMenu->addAction("Send to Back", this, &ROISelectorWidget::sendToBack);
    arrangeMenu->addAction("Bring Forward", this, &ROISelectorWidget::bringForward);
    arrangeMenu->addAction("Send Backward", this, &ROISelectorWidget::sendBackward);
    
    // Alignment
    auto alignMenu = menu.addMenu("Align");
    alignMenu->addAction("Align Left", this, &ROISelectorWidget::alignLeft);
    alignMenu->addAction("Align Right", this, &ROISelectorWidget::alignRight);
    alignMenu->addAction("Align Top", this, &ROISelectorWidget::alignTop);
    alignMenu->addAction("Align Bottom", this, &ROISelectorWidget::alignBottom);
    alignMenu->addAction("Align H-Center", this, &ROISelectorWidget::alignHCenter);
    alignMenu->addAction("Align V-Center", this, &ROISelectorWidget::alignVCenter);
    
    menu.addSeparator();
    
    // Selection
    menu.addAction("Select All", this, &ROISelectorWidget::selectAll);
    menu.addAction("Deselect All", this, &ROISelectorWidget::deselectAll);
    
    menu.exec(mapToGlobal(pos));
}

QPointF ROISelectorWidget::sceneToImage(const QPointF& scenePos) const {
    // Direct mapping since image is at scene origin
    return scenePos;
}

QPointF ROISelectorWidget::imageToScene(const QPointF& imagePos) const {
    // Direct mapping since image is at scene origin
    return imagePos;
}

// ROI Manager slots

void ROISelectorWidget::onROIAdded(ROIPtr roi) {
    addROIItem(roi);
}

void ROISelectorWidget::onROIRemoved(const QString& roiId) {
    removeROIItem(roiId);
    emit roiDeleted(roiId);
}

void ROISelectorWidget::onROIModified(const QString& roiId, ROIPtr roi) {
    Q_UNUSED(roi)
    updateROIItem(roiId);
    emit roiModified(roiId);
}

void ROISelectorWidget::onROISelectionChanged(const std::vector<QString>& selectedIds) {
    // Update item selection states
    for (auto& [id, item] : m_roiItems) {
        bool selected = std::find(selectedIds.begin(), selectedIds.end(), id) != selectedIds.end();
        item->setSelected(selected);
        item->showHandles(selected);
    }
    
    QList<QString> qSelectedIds(selectedIds.begin(), selectedIds.end());
    emit selectionChanged(qSelectedIds);
}

// Alignment implementations

void ROISelectorWidget::alignLeft() {
    auto selected = selectedROIs();
    if (selected.size() < 2 || !m_roiManager) return;
    
    // Find leftmost edge
    float minX = std::numeric_limits<float>::max();
    for (const auto& id : selected) {
        auto roi = m_roiManager->getROI(id);
        if (roi) {
            cv::Rect2f bbox = roi->getBoundingBox();
            minX = std::min(minX, bbox.x);
        }
    }
    
    // Align all to leftmost
    for (const auto& id : selected) {
        auto roi = m_roiManager->getROI(id);
        if (roi) {
            cv::Rect2f bbox = roi->getBoundingBox();
            cv::Point2f offset(minX - bbox.x, 0);
            roi->translate(offset);
        }
    }
}

void ROISelectorWidget::alignRight() {
    auto selected = selectedROIs();
    if (selected.size() < 2 || !m_roiManager) return;
    
    // Find rightmost edge
    float maxX = std::numeric_limits<float>::min();
    for (const auto& id : selected) {
        auto roi = m_roiManager->getROI(id);
        if (roi) {
            cv::Rect2f bbox = roi->getBoundingBox();
            maxX = std::max(maxX, bbox.x + bbox.width);
        }
    }
    
    // Align all to rightmost
    for (const auto& id : selected) {
        auto roi = m_roiManager->getROI(id);
        if (roi) {
            cv::Rect2f bbox = roi->getBoundingBox();
            cv::Point2f offset(maxX - (bbox.x + bbox.width), 0);
            roi->translate(offset);
        }
    }
}

void ROISelectorWidget::alignTop() {
    auto selected = selectedROIs();
    if (selected.size() < 2 || !m_roiManager) return;
    
    // Find topmost edge
    float minY = std::numeric_limits<float>::max();
    for (const auto& id : selected) {
        auto roi = m_roiManager->getROI(id);
        if (roi) {
            cv::Rect2f bbox = roi->getBoundingBox();
            minY = std::min(minY, bbox.y);
        }
    }
    
    // Align all to topmost
    for (const auto& id : selected) {
        auto roi = m_roiManager->getROI(id);
        if (roi) {
            cv::Rect2f bbox = roi->getBoundingBox();
            cv::Point2f offset(0, minY - bbox.y);
            roi->translate(offset);
        }
    }
}

void ROISelectorWidget::alignBottom() {
    auto selected = selectedROIs();
    if (selected.size() < 2 || !m_roiManager) return;
    
    // Find bottommost edge
    float maxY = std::numeric_limits<float>::min();
    for (const auto& id : selected) {
        auto roi = m_roiManager->getROI(id);
        if (roi) {
            cv::Rect2f bbox = roi->getBoundingBox();
            maxY = std::max(maxY, bbox.y + bbox.height);
        }
    }
    
    // Align all to bottommost
    for (const auto& id : selected) {
        auto roi = m_roiManager->getROI(id);
        if (roi) {
            cv::Rect2f bbox = roi->getBoundingBox();
            cv::Point2f offset(0, maxY - (bbox.y + bbox.height));
            roi->translate(offset);
        }
    }
}

void ROISelectorWidget::alignHCenter() {
    auto selected = selectedROIs();
    if (selected.size() < 2 || !m_roiManager) return;
    
    // Calculate center X
    float sumX = 0;
    int count = 0;
    for (const auto& id : selected) {
        auto roi = m_roiManager->getROI(id);
        if (roi) {
            cv::Rect2f bbox = roi->getBoundingBox();
            sumX += bbox.x + bbox.width / 2;
            count++;
        }
    }
    
    if (count == 0) return;
    float centerX = sumX / count;
    
    // Align all to center
    for (const auto& id : selected) {
        auto roi = m_roiManager->getROI(id);
        if (roi) {
            cv::Rect2f bbox = roi->getBoundingBox();
            cv::Point2f offset(centerX - (bbox.x + bbox.width / 2), 0);
            roi->translate(offset);
        }
    }
}

void ROISelectorWidget::alignVCenter() {
    auto selected = selectedROIs();
    if (selected.size() < 2 || !m_roiManager) return;
    
    // Calculate center Y
    float sumY = 0;
    int count = 0;
    for (const auto& id : selected) {
        auto roi = m_roiManager->getROI(id);
        if (roi) {
            cv::Rect2f bbox = roi->getBoundingBox();
            sumY += bbox.y + bbox.height / 2;
            count++;
        }
    }
    
    if (count == 0) return;
    float centerY = sumY / count;
    
    // Align all to center
    for (const auto& id : selected) {
        auto roi = m_roiManager->getROI(id);
        if (roi) {
            cv::Rect2f bbox = roi->getBoundingBox();
            cv::Point2f offset(0, centerY - (bbox.y + bbox.height / 2));
            roi->translate(offset);
        }
    }
}

// Transform operations

void ROISelectorWidget::rotateSelectedROIs(qreal angle) {
    auto selected = selectedROIs();
    
    if (selected.isEmpty() || !m_roiManager) return;
    
    for (const auto& id : selected) {
        auto roi = m_roiManager->getROI(id);
        if (roi) {
            roi->rotate(angle);
        }
    }
}

void ROISelectorWidget::scaleSelectedROIs(qreal factor) {
    auto selected = selectedROIs();
    
    if (selected.isEmpty() || !m_roiManager) return;
    
    for (const auto& id : selected) {
        auto roi = m_roiManager->getROI(id);
        if (roi) {
            roi->scale(factor);
        }
    }
}

// ============ ROI Command Implementations ============

ROICommand::ROICommand(ROIManager* manager, QUndoCommand* parent)
    : QUndoCommand(parent), m_manager(manager) {}

// Create command

CreateROICommand::CreateROICommand(ROIManager* manager, ROIPtr roi, QUndoCommand* parent)
    : ROICommand(manager, parent), m_roi(roi) {
    
    m_roiId = roi->getId();
    setText(QString("Create %1").arg(roi->getName()));
}

void CreateROICommand::undo() {
    if (m_manager) {
        m_manager->removeROI(m_roiId);
    }
}

void CreateROICommand::redo() {
    if (m_manager && m_roi) {
        m_manager->addROI(m_roi);
    }
}

// Delete command

DeleteROICommand::DeleteROICommand(ROIManager* manager, const QString& roiId, QUndoCommand* parent)
    : ROICommand(manager, parent), m_roiId(roiId) {
    
    if (m_manager) {
        m_roi = m_manager->getROI(roiId);
        if (m_roi) {
            setText(QString("Delete %1").arg(m_roi->getName()));
        }
    }
}

void DeleteROICommand::undo() {
    if (m_manager && m_roi) {
        m_manager->addROI(m_roi);
    }
}

void DeleteROICommand::redo() {
    if (m_manager) {
        m_manager->removeROI(m_roiId);
    }
}

// Modify command

ModifyROICommand::ModifyROICommand(ROIManager* manager, const QString& roiId,
                                   ROIPtr oldROI, ROIPtr newROI, QUndoCommand* parent)
    : ROICommand(manager, parent), m_roiId(roiId), m_oldROI(oldROI), m_newROI(newROI) {
    
    setText(QString("Modify %1").arg(newROI ? newROI->getName() : "ROI"));
}

void ModifyROICommand::undo() {
    if (m_manager && m_oldROI) {
        m_manager->updateROI(m_roiId, m_oldROI);
    }
}

void ModifyROICommand::redo() {
    if (m_manager && m_newROI) {
        m_manager->updateROI(m_roiId, m_newROI);
    }
}

// Missing method implementations (stub implementations for now)
void ROISelectorWidget::mouseDoubleClickEvent(QMouseEvent* event) {
    QGraphicsView::mouseDoubleClickEvent(event);
    // TODO: Implement double-click behavior (e.g., edit properties)
}

void ROISelectorWidget::dragEnterEvent(QDragEnterEvent* event) {
    QGraphicsView::dragEnterEvent(event);
    // TODO: Implement drag and drop support
}

void ROISelectorWidget::dragMoveEvent(QDragMoveEvent* event) {
    QGraphicsView::dragMoveEvent(event);
    // TODO: Implement drag and drop support
}

void ROISelectorWidget::dropEvent(QDropEvent* event) {
    QGraphicsView::dropEvent(event);
    // TODO: Implement drag and drop support
}

void ROISelectorWidget::distributeHorizontally() {
    qWarning() << "distributeHorizontally() not yet implemented";
    // TODO: Implement horizontal distribution of selected ROIs
}

void ROISelectorWidget::distributeVertically() {
    qWarning() << "distributeVertically() not yet implemented";
    // TODO: Implement vertical distribution of selected ROIs
}

void ROISelectorWidget::flipHorizontal() {
    qWarning() << "flipHorizontal() not yet implemented";
    // TODO: Implement horizontal flipping of selected ROIs
}

void ROISelectorWidget::flipVertical() {
    qWarning() << "flipVertical() not yet implemented";
    // TODO: Implement vertical flipping of selected ROIs
}

} // namespace OpenCV
} // namespace ComponentsForest