#ifndef ROI_SELECTOR_WIDGET_H
#define ROI_SELECTOR_WIDGET_H

#include "roi_manager.h"
#include <QWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QUndoStack>
#include <QPropertyAnimation>

namespace ComponentsForest {
namespace OpenCV {

/**
 * @brief Interaction modes for ROI selector
 */
enum class InteractionMode {
    Select,
    Create,
    Edit,
    Pan,
    Zoom
};

/**
 * @brief Creation tool types
 */
enum class CreationTool {
    Rectangle,
    Circle,
    Polygon,
    Freehand,
    MagicWand
};

/**
 * @brief Visual style for ROIs
 */
struct ROIVisualStyle {
    // Normal state
    QColor borderColor{0, 120, 215};
    float borderWidth{2.0f};
    Qt::PenStyle borderStyle{Qt::SolidLine};
    float opacity{1.0f};
    
    // Selected state
    QColor selectedColor{255, 165, 0};
    float selectedWidth{3.0f};
    bool showHandles{true};
    
    // Hover state
    QColor hoverColor{0, 200, 255};
    float hoverWidth{2.5f};
    bool showTooltip{true};
    
    // Active/Processing state
    QColor activeColor{0, 255, 0};
    bool animateBorder{true};
    float pulseSpeed{2.0f};
    
    // Error state
    QColor errorColor{255, 0, 0};
    bool flashOnError{true};
    int flashDuration{500};
    
    // Handles
    float handleSize{8.0f};
    QColor handleFillColor{Qt::white};
    QColor handleBorderColor{0, 120, 215};
    float handleBorderWidth{2.0f};
};

/**
 * @brief Snapping options for alignment
 */
struct SnappingOptions {
    bool enableSnapping{true};
    float snapThreshold{10.0f};
    
    bool snapToGrid{true};
    QSizeF gridSize{10, 10};
    
    bool snapToGuides{true};
    QList<QLineF> guides;
    
    bool snapToROIs{true};
    bool snapToImageEdges{true};
};

/**
 * @brief Handle types for ROI manipulation
 */
enum class HandleType {
    None,
    TopLeft, TopCenter, TopRight,
    MiddleLeft, MiddleRight,
    BottomLeft, BottomCenter, BottomRight,
    Rotation,
    Center
};

// Forward declarations
class ROIGraphicsItem;
class ROIHandle;

/**
 * @brief Custom graphics item for ROI visualization
 */
class ROIGraphicsItem : public QObject, public QGraphicsItem {
    Q_OBJECT
    Q_INTERFACES(QGraphicsItem)
public:
    explicit ROIGraphicsItem(ROIPtr roi, QGraphicsItem* parent = nullptr);
    
    // QGraphicsItem interface
    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
              QWidget* widget = nullptr) override;
    QPainterPath shape() const override;
    
    // ROI access
    ROIPtr getROI() const { return m_roi; }
    void updateFromROI();
    
    // Visual state
    void setVisualStyle(const ROIVisualStyle& style);
    ROIVisualStyle getVisualStyle() const { return m_style; }
    
    void setSelected(bool selected);
    void setHovered(bool hovered);
    void setProcessing(bool processing);
    void setError(bool error);
    
    // Handles
    void showHandles(bool show);
    bool handlesVisible() const { return m_handlesVisible; }
    HandleType getHandleAt(const QPointF& pos) const;
    
    // Animation
    void startPulseAnimation();
    void stopPulseAnimation();
    void flashError();
    
protected:
    void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;
    
private:
    ROIPtr m_roi;
    ROIVisualStyle m_style;
    bool m_selected{false};
    bool m_hovered{false};
    bool m_processing{false};
    bool m_error{false};
    bool m_handlesVisible{false};
    
    QList<ROIHandle*> m_handles;
    QPropertyAnimation* m_pulseAnimation{nullptr};
    QPropertyAnimation* m_errorAnimation{nullptr};
    
    void createHandles();
    void updateHandles();
    void drawROI(QPainter* painter);
    void drawHandles(QPainter* painter);
    void drawInfo(QPainter* painter);
};

/**
 * @brief Handle for ROI manipulation
 */
class ROIHandle : public QGraphicsItem {
public:
    ROIHandle(HandleType type, ROIGraphicsItem* parent);
    
    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
              QWidget* widget = nullptr) override;
    
    HandleType getType() const { return m_type; }
    void setPosition(const QPointF& pos);
    
protected:
    void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;
    
private:
    HandleType m_type;
    ROIGraphicsItem* m_roiItem;
    bool m_hovered{false};
    
    QCursor getCursorForHandle() const;
};

/**
 * @brief Main ROI selector widget
 */
class ROISelectorWidget : public QGraphicsView {
    Q_OBJECT
    
public:
    explicit ROISelectorWidget(QWidget* parent = nullptr);
    ~ROISelectorWidget();
    
    // ROI Manager
    void setROIManager(ROIManager* manager);
    ROIManager* roiManager() const { return m_roiManager; }
    
    // Image
    void setImage(const cv::Mat& image);
    void clearImage();
    
    // Interaction mode
    void setInteractionMode(InteractionMode mode);
    InteractionMode interactionMode() const { return m_interactionMode; }
    
    // Creation tool
    void setCreationTool(CreationTool tool);
    CreationTool creationTool() const { return m_creationTool; }
    
    // Visual style
    void setDefaultVisualStyle(const ROIVisualStyle& style);
    ROIVisualStyle defaultVisualStyle() const { return m_defaultStyle; }
    
    // Snapping
    void setSnappingOptions(const SnappingOptions& options);
    SnappingOptions snappingOptions() const { return m_snappingOptions; }
    
    // Grid
    void setShowGrid(bool show);
    bool showGrid() const { return m_showGrid; }
    void setGridSize(const QSizeF& size);
    
    // Guides
    void addGuide(const QLineF& guide);
    void removeGuide(int index);
    void clearGuides();
    
    // Selection
    void selectROI(const QString& roiId);
    void selectROIs(const QList<QString>& roiIds);
    void selectAll();
    void deselectAll();
    QList<QString> selectedROIs() const;
    
    // Undo/Redo
    QUndoStack* undoStack() const { return m_undoStack; }
    
    // Display options
    void setShowROIInfo(bool show) { m_showROIInfo = show; }
    void setShowDimensions(bool show) { m_showDimensions = show; }
    void setShowCoordinates(bool show) { m_showCoordinates = show; }
    void setShowStatistics(bool show) { m_showStatistics = show; }
    
    // Zoom and pan
    void zoomIn();
    void zoomOut();
    void zoomFit();
    void zoomActual();
    void setZoom(qreal zoom);
    qreal zoom() const { return m_zoom; }
    
signals:
    // ROI signals
    void roiCreated(ROIPtr roi);
    void roiModified(const QString& roiId);
    void roiDeleted(const QString& roiId);
    
    // Selection signals
    void selectionChanged(const QList<QString>& selectedIds);
    
    // Interaction signals
    void interactionModeChanged(InteractionMode mode);
    void creationToolChanged(CreationTool tool);
    
    // Mouse position
    void mousePositionChanged(const QPointF& imagePos);
    
public slots:
    // ROI operations
    void deleteSelectedROIs();
    void duplicateSelectedROIs();
    void groupSelectedROIs();
    void ungroupSelectedROIs();
    
    // Arrangement
    void bringToFront();
    void sendToBack();
    void bringForward();
    void sendBackward();
    
    // Alignment
    void alignLeft();
    void alignRight();
    void alignTop();
    void alignBottom();
    void alignHCenter();
    void alignVCenter();
    
    // Distribution
    void distributeHorizontally();
    void distributeVertically();
    
    // Transform
    void rotateSelectedROIs(qreal angle);
    void scaleSelectedROIs(qreal factor);
    void flipHorizontal();
    void flipVertical();
    
protected:
    // Event handlers
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    
    // Drag and drop
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    
    // Drawing
    void drawBackground(QPainter* painter, const QRectF& rect) override;
    void drawForeground(QPainter* painter, const QRectF& rect) override;
    
private:
    // Core components
    ROIManager* m_roiManager{nullptr};
    QGraphicsScene* m_scene;
    QUndoStack* m_undoStack;
    
    // Image
    QGraphicsPixmapItem* m_imageItem{nullptr};
    cv::Mat m_currentImage;
    
    // ROI items
    std::map<QString, ROIGraphicsItem*> m_roiItems;
    
    // Interaction state
    InteractionMode m_interactionMode{InteractionMode::Select};
    CreationTool m_creationTool{CreationTool::Rectangle};
    
    // Creation state
    bool m_creating{false};
    QPointF m_creationStart;
    QPointF m_creationCurrent;
    ROIPtr m_creationROI;
    ROIGraphicsItem* m_creationPreview{nullptr};
    
    // Selection state
    bool m_selecting{false};
    QPointF m_selectionStart;
    QRectF m_selectionRect;
    QGraphicsRectItem* m_selectionRubberBand{nullptr};
    
    // Drag state
    bool m_dragging{false};
    QPointF m_dragStart;
    HandleType m_dragHandle{HandleType::None};
    ROIGraphicsItem* m_dragItem{nullptr};
    
    // Visual settings
    ROIVisualStyle m_defaultStyle;
    SnappingOptions m_snappingOptions;
    
    // Display options
    bool m_showGrid{false};
    bool m_showROIInfo{true};
    bool m_showDimensions{true};
    bool m_showCoordinates{false};
    bool m_showStatistics{false};
    
    // Zoom and pan
    qreal m_zoom{1.0};
    bool m_panning{false};
    QPointF m_panStart;
    
    // Keyboard modifiers
    bool m_shiftPressed{false};
    bool m_ctrlPressed{false};
    bool m_altPressed{false};
    
    // Helper methods
    void setupScene();
    void connectROIManager();
    void disconnectROIManager();
    
    // ROI item management
    void addROIItem(ROIPtr roi);
    void removeROIItem(const QString& roiId);
    void updateROIItem(const QString& roiId);
    
    // Creation helpers
    void startCreation(const QPointF& pos);
    void updateCreation(const QPointF& pos);
    void finishCreation();
    void cancelCreation();
    
    // Selection helpers
    void startSelection(const QPointF& pos);
    void updateSelection(const QPointF& pos);
    void finishSelection();
    
    // Drag helpers
    void startDrag(const QPointF& pos);
    void updateDrag(const QPointF& pos);
    void finishDrag();
    
    // Snapping
    QPointF snapPoint(const QPointF& point);
    void showSnapIndicators(const QPointF& snappedPoint);
    void hideSnapIndicators();
    
    // Context menu
    void showContextMenu(const QPoint& pos);
    
    // Coordinate conversion
    QPointF sceneToImage(const QPointF& scenePos) const;
    QPointF imageToScene(const QPointF& imagePos) const;
    
private slots:
    // ROI manager slots
    void onROIAdded(ROIPtr roi);
    void onROIRemoved(const QString& roiId);
    void onROIModified(const QString& roiId, ROIPtr roi);
    void onROISelectionChanged(const std::vector<QString>& selectedIds);
};

/**
 * @brief Undo command for ROI operations
 */
class ROICommand : public QUndoCommand {
public:
    explicit ROICommand(ROIManager* manager, QUndoCommand* parent = nullptr);
    
protected:
    ROIManager* m_manager;
};

/**
 * @brief Command for creating ROIs
 */
class CreateROICommand : public ROICommand {
public:
    CreateROICommand(ROIManager* manager, ROIPtr roi, QUndoCommand* parent = nullptr);
    
    void undo() override;
    void redo() override;
    
private:
    ROIPtr m_roi;
    QString m_roiId;
};

/**
 * @brief Command for deleting ROIs
 */
class DeleteROICommand : public ROICommand {
public:
    DeleteROICommand(ROIManager* manager, const QString& roiId, QUndoCommand* parent = nullptr);
    
    void undo() override;
    void redo() override;
    
private:
    QString m_roiId;
    ROIPtr m_roi;
};

/**
 * @brief Command for modifying ROIs
 */
class ModifyROICommand : public ROICommand {
public:
    ModifyROICommand(ROIManager* manager, const QString& roiId,
                     ROIPtr oldROI, ROIPtr newROI, QUndoCommand* parent = nullptr);
    
    void undo() override;
    void redo() override;
    
private:
    QString m_roiId;
    ROIPtr m_oldROI;
    ROIPtr m_newROI;
};

} // namespace OpenCV
} // namespace ComponentsForest

#endif // ROI_SELECTOR_WIDGET_H