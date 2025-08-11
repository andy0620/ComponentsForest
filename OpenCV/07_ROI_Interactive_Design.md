# ROI Interactive Design Guide

## Overview

This document describes the interactive design patterns and UI/UX considerations for the ROI management system, focusing on user interaction, visual feedback, and real-time editing capabilities.

## Interactive ROI Selection

### Visual Design Principles

```
┌─────────────────────────────────────────────┐
│          ROI Selector Widget                 │
├─────────────────────────────────────────────┤
│                                              │
│  ┌──────────────────────────────────────┐   │
│  │                                      │   │
│  │     ┌─────────────┐                 │   │
│  │     │    ROI 1    │ ● ─ ─ ○        │   │
│  │     │             │                  │   │
│  │     └─────────────┘                 │   │
│  │            ↑                         │   │
│  │     Resize Handles                   │   │
│  │                                      │   │
│  │   ○──────────────○                  │   │
│  │   │    ROI 2     │                  │   │
│  │   │  (Circular)  │                  │   │
│  │   ○──────────────○                  │   │
│  │                                      │   │
│  └──────────────────────────────────────┘   │
│                                              │
│  [Rectangle] [Circle] [Polygon] [Free]       │
│  [Select] [Move] [Resize] [Rotate] [Delete]  │
└─────────────────────────────────────────────┘
```

## Interaction Modes

### 1. Selection Mode
- Single click to select ROI
- Ctrl+Click for multi-selection
- Click and drag for lasso selection
- Double-click to edit properties

### 2. Creation Mode
- Click and drag to create
- Shift to constrain proportions
- Alt to create from center
- Esc to cancel creation

### 3. Edit Mode
- Drag handles to resize
- Drag center to move
- Ctrl+Drag for rotation
- Right-click for context menu

## Visual Feedback System

### ROI States and Appearance

```cpp
struct ROIVisualStyle {
    // Normal state
    QColor borderColor = QColor(0, 120, 215);      // Blue
    float borderWidth = 2.0f;
    Qt::PenStyle borderStyle = Qt::SolidLine;
    
    // Selected state
    QColor selectedColor = QColor(255, 165, 0);    // Orange
    float selectedWidth = 3.0f;
    bool showHandles = true;
    
    // Hover state
    QColor hoverColor = QColor(0, 200, 255);       // Cyan
    float hoverWidth = 2.5f;
    bool showTooltip = true;
    
    // Active/Processing state
    QColor activeColor = QColor(0, 255, 0);        // Green
    bool animateBorder = true;
    float pulseSpeed = 2.0f;
    
    // Error state
    QColor errorColor = QColor(255, 0, 0);         // Red
    bool flashOnError = true;
    int flashDuration = 500;  // ms
};
```

### Handle System

```cpp
enum class HandleType {
    TopLeft,     TopCenter,    TopRight,
    MiddleLeft,                MiddleRight,
    BottomLeft,  BottomCenter, BottomRight,
    RotationHandle,
    CenterHandle
};

struct HandleStyle {
    float size = 8.0f;
    QColor fillColor = Qt::white;
    QColor borderColor = QColor(0, 120, 215);
    float borderWidth = 2.0f;
    QCursor cursor;  // Cursor per handle type
};
```

## User Interaction Patterns

### Mouse Interactions

```cpp
class ROIInteractionHandler {
    // Mouse events
    void mousePressEvent(QMouseEvent* event) {
        if (event->button() == Qt::LeftButton) {
            // Start interaction based on mode
            startInteraction(event->pos());
        } else if (event->button() == Qt::RightButton) {
            // Show context menu
            showContextMenu(event->pos());
        }
    }
    
    void mouseMoveEvent(QMouseEvent* event) {
        if (m_interacting) {
            // Update ROI based on interaction type
            updateROI(event->pos());
            // Show real-time feedback
            updateVisualFeedback();
        } else {
            // Update hover state
            updateHoverState(event->pos());
        }
    }
    
    void mouseReleaseEvent(QMouseEvent* event) {
        if (m_interacting) {
            // Finish interaction
            finalizeROI();
            // Emit change signal
            emit roiModified(m_currentROI);
        }
    }
    
    void wheelEvent(QWheelEvent* event) {
        if (event->modifiers() & Qt::ControlModifier) {
            // Zoom ROI
            zoomROI(event->angleDelta().y());
        }
    }
};
```

### Keyboard Shortcuts

| Shortcut | Action | Context |
|----------|--------|---------|
| `R` | Rectangle tool | Creation mode |
| `C` | Circle tool | Creation mode |
| `P` | Polygon tool | Creation mode |
| `V` | Selection tool | Any mode |
| `Delete` | Delete selected ROIs | Selection mode |
| `Ctrl+A` | Select all ROIs | Selection mode |
| `Ctrl+D` | Duplicate selected | Selection mode |
| `Ctrl+G` | Group selected | Multiple selected |
| `Ctrl+Z` | Undo last action | Any mode |
| `Ctrl+Y` | Redo last action | Any mode |
| `Arrow Keys` | Nudge selected ROI | Selection mode |
| `Shift+Arrows` | Fine nudge (1px) | Selection mode |
| `Tab` | Next ROI | Selection mode |
| `Shift+Tab` | Previous ROI | Selection mode |

### Touch Gestures (for touch-enabled displays)

```cpp
class TouchGestureHandler {
    // Pinch to zoom ROI
    void handlePinchGesture(QPinchGesture* gesture) {
        float scale = gesture->scaleFactor();
        scaleSelectedROI(scale);
    }
    
    // Two-finger rotate
    void handleRotateGesture(QRotateGesture* gesture) {
        float angle = gesture->rotationAngle();
        rotateSelectedROI(angle);
    }
    
    // Long press for context menu
    void handleLongPress(QTapAndHoldGesture* gesture) {
        showContextMenu(gesture->position());
    }
};
```

## Advanced Interaction Features

### Magnetic Snapping

```cpp
struct SnappingOptions {
    bool enableSnapping = true;
    float snapThreshold = 10.0f;  // pixels
    
    bool snapToGrid = true;
    QSize gridSize = QSize(10, 10);
    
    bool snapToGuides = true;
    QList<QLineF> guides;
    
    bool snapToROIs = true;
    bool snapToImageEdges = true;
};

class MagneticSnapping {
    QPointF snapPoint(const QPointF& point) {
        QPointF snapped = point;
        
        // Grid snapping
        if (options.snapToGrid) {
            snapped.setX(round(point.x() / gridSize.width()) * gridSize.width());
            snapped.setY(round(point.y() / gridSize.height()) * gridSize.height());
        }
        
        // Guide snapping
        for (const auto& guide : options.guides) {
            if (distanceToLine(point, guide) < options.snapThreshold) {
                snapped = nearestPointOnLine(point, guide);
                showSnapIndicator(snapped);
                break;
            }
        }
        
        return snapped;
    }
};
```

### Smart Alignment

```cpp
class SmartAlignment {
    void showAlignmentGuides(const ROIPtr& movingROI) {
        // Check alignment with other ROIs
        for (const auto& otherROI : m_rois) {
            if (otherROI == movingROI) continue;
            
            // Vertical alignment
            if (abs(movingROI->center().x() - otherROI->center().x()) < threshold) {
                drawAlignmentLine(Qt::Vertical, otherROI->center().x());
            }
            
            // Horizontal alignment
            if (abs(movingROI->center().y() - otherROI->center().y()) < threshold) {
                drawAlignmentLine(Qt::Horizontal, otherROI->center().y());
            }
            
            // Edge alignment
            checkEdgeAlignment(movingROI, otherROI);
        }
    }
};
```

### Constraint System

```cpp
class ROIConstraints {
    // Size constraints
    struct SizeConstraints {
        QSizeF minimum = QSizeF(10, 10);
        QSizeF maximum = QSizeF(INFINITY, INFINITY);
        float aspectRatio = 0;  // 0 = unconstrained
        bool maintainArea = false;
    };
    
    // Position constraints
    struct PositionConstraints {
        QRectF bounds;  // Bounding area
        bool constrainToImage = true;
        bool allowOverlap = true;
    };
    
    // Apply constraints during interaction
    ROIPtr applyConstraints(ROIPtr roi) {
        // Apply size constraints
        roi = applySizeConstraints(roi);
        
        // Apply position constraints
        roi = applyPositionConstraints(roi);
        
        // Check collision if needed
        if (!constraints.allowOverlap) {
            roi = resolveCollisions(roi);
        }
        
        return roi;
    }
};
```

## Visual Overlays

### ROI Information Display

```cpp
class ROIInfoOverlay : public QWidget {
    void paintEvent(QPaintEvent* event) override {
        QPainter painter(this);
        
        for (const auto& roi : m_selectedROIs) {
            // Draw ROI outline
            drawROIOutline(painter, roi);
            
            // Draw size information
            if (m_showDimensions) {
                drawDimensions(painter, roi);
            }
            
            // Draw coordinates
            if (m_showCoordinates) {
                drawCoordinates(painter, roi);
            }
            
            // Draw name label
            if (m_showLabels) {
                drawLabel(painter, roi);
            }
            
            // Draw statistics
            if (m_showStats && roi->hasStatistics()) {
                drawStatistics(painter, roi);
            }
        }
    }
    
    void drawDimensions(QPainter& painter, const ROIPtr& roi) {
        auto bounds = roi->getBounds();
        QString text = QString("%1 x %2").arg(bounds.width()).arg(bounds.height());
        
        // Position text outside ROI
        QPointF textPos = calculateTextPosition(roi);
        
        // Draw with background for readability
        drawTextWithBackground(painter, text, textPos);
    }
};
```

### Real-time Feedback

```cpp
class RealtimeFeedback {
    // Live measurements during creation/resize
    void showLiveMeasurements(const QPointF& start, const QPointF& current) {
        float width = abs(current.x() - start.x());
        float height = abs(current.y() - start.y());
        float diagonal = sqrt(width*width + height*height);
        
        // Show tooltip at cursor
        QString info = QString("W: %1px\nH: %2px\nD: %3px")
            .arg(width, 0, 'f', 1)
            .arg(height, 0, 'f', 1)
            .arg(diagonal, 0, 'f', 1);
        
        QToolTip::showText(QCursor::pos(), info);
    }
    
    // Angle feedback during rotation
    void showRotationFeedback(float angle) {
        // Draw rotation arc
        drawRotationArc(m_rotationCenter, m_startAngle, angle);
        
        // Show angle value
        QString text = QString("%1°").arg(angle, 0, 'f', 1);
        drawTextAtCursor(text);
    }
};
```

## Animation System

### ROI Animations

```cpp
class ROIAnimator {
    // Smooth transitions
    QPropertyAnimation* animateROIChange(ROIPtr from, ROIPtr to, int duration = 300) {
        auto animation = new QPropertyAnimation(this, "roiGeometry");
        animation->setStartValue(from->getGeometry());
        animation->setEndValue(to->getGeometry());
        animation->setDuration(duration);
        animation->setEasingCurve(QEasingCurve::InOutCubic);
        return animation;
    }
    
    // Pulse animation for active ROI
    void pulseROI(const ROIPtr& roi) {
        auto animation = new QSequentialAnimationGroup;
        
        auto expand = new QPropertyAnimation(this, "roiScale");
        expand->setStartValue(1.0);
        expand->setEndValue(1.1);
        expand->setDuration(200);
        
        auto contract = new QPropertyAnimation(this, "roiScale");
        contract->setStartValue(1.1);
        contract->setEndValue(1.0);
        contract->setDuration(200);
        
        animation->addAnimation(expand);
        animation->addAnimation(contract);
        animation->setLoopCount(-1);  // Infinite loop
        animation->start();
    }
    
    // Highlight animation on selection
    void highlightROI(const ROIPtr& roi) {
        auto colorAnimation = new QPropertyAnimation(this, "roiColor");
        colorAnimation->setStartValue(roi->getColor());
        colorAnimation->setEndValue(m_highlightColor);
        colorAnimation->setDuration(150);
        colorAnimation->start();
    }
};
```

## Context Menu System

### Dynamic Context Menu

```cpp
class ROIContextMenu : public QMenu {
    void buildMenu(const ROIPtr& roi, const QPoint& pos) {
        clear();
        
        // Basic operations
        addAction("Cut", [=]() { cutROI(roi); });
        addAction("Copy", [=]() { copyROI(roi); });
        addAction("Paste", [=]() { pasteROI(pos); });
        addAction("Delete", [=]() { deleteROI(roi); });
        
        addSeparator();
        
        // Transform menu
        auto transformMenu = addMenu("Transform");
        transformMenu->addAction("Rotate 90° CW", [=]() { rotateROI(roi, 90); });
        transformMenu->addAction("Rotate 90° CCW", [=]() { rotateROI(roi, -90); });
        transformMenu->addAction("Flip Horizontal", [=]() { flipROI(roi, Qt::Horizontal); });
        transformMenu->addAction("Flip Vertical", [=]() { flipROI(roi, Qt::Vertical); });
        
        // Arrange menu
        auto arrangeMenu = addMenu("Arrange");
        arrangeMenu->addAction("Bring to Front", [=]() { bringToFront(roi); });
        arrangeMenu->addAction("Send to Back", [=]() { sendToBack(roi); });
        arrangeMenu->addAction("Bring Forward", [=]() { bringForward(roi); });
        arrangeMenu->addAction("Send Backward", [=]() { sendBackward(roi); });
        
        addSeparator();
        
        // ROI-specific actions
        if (roi->supportsTracking()) {
            auto trackingAction = addAction("Enable Tracking");
            trackingAction->setCheckable(true);
            trackingAction->setChecked(roi->isTrackingEnabled());
            connect(trackingAction, &QAction::toggled, 
                    [=](bool checked) { roi->setTrackingEnabled(checked); });
        }
        
        // Properties
        addAction("Properties...", [=]() { showROIProperties(roi); });
    }
};
```

## Undo/Redo System

### Command Pattern Implementation

```cpp
class ROICommand : public QUndoCommand {
public:
    virtual void undo() override = 0;
    virtual void redo() override = 0;
};

class MoveROICommand : public ROICommand {
    ROIPtr m_roi;
    QPointF m_oldPos, m_newPos;
    
public:
    void undo() override {
        m_roi->setPosition(m_oldPos);
    }
    
    void redo() override {
        m_roi->setPosition(m_newPos);
    }
};

class ROIUndoStack {
    QUndoStack m_stack;
    
    void executeCommand(ROICommand* cmd) {
        m_stack.push(cmd);
        emit commandExecuted(cmd);
    }
    
    // Macro for complex operations
    void beginMacro(const QString& text) {
        m_stack.beginMacro(text);
    }
    
    void endMacro() {
        m_stack.endMacro();
    }
};
```

## Accessibility Features

### Keyboard Navigation

```cpp
class ROIKeyboardNavigation {
    void handleKeyPress(QKeyEvent* event) {
        switch(event->key()) {
            case Qt::Key_Tab:
                selectNextROI();
                break;
            case Qt::Key_Space:
                toggleROISelection();
                break;
            case Qt::Key_Return:
                editSelectedROI();
                break;
            case Qt::Key_F2:
                renameSelectedROI();
                break;
        }
    }
    
    // Voice feedback for visually impaired users
    void announceROI(const ROIPtr& roi) {
        QString announcement = QString("ROI %1, %2, position %3, %4, size %5 by %6")
            .arg(roi->getName())
            .arg(roi->getType())
            .arg(roi->x()).arg(roi->y())
            .arg(roi->width()).arg(roi->height());
        
        QTextToSpeech::say(announcement);
    }
};
```

### High Contrast Mode

```cpp
struct HighContrastStyle : public ROIVisualStyle {
    HighContrastStyle() {
        borderColor = Qt::white;
        borderWidth = 3.0f;
        selectedColor = Qt::yellow;
        selectedWidth = 4.0f;
        
        // High contrast handles
        handleStyle.size = 12.0f;
        handleStyle.fillColor = Qt::black;
        handleStyle.borderColor = Qt::white;
        handleStyle.borderWidth = 3.0f;
    }
};
```

## Performance Considerations

### Rendering Optimization

```cpp
class OptimizedROIRenderer {
    // Use OpenGL for large numbers of ROIs
    void renderWithOpenGL(const std::vector<ROIPtr>& rois) {
        // Batch render similar ROI types
        std::map<ROIType, std::vector<ROIPtr>> batches;
        for (const auto& roi : rois) {
            batches[roi->getType()].push_back(roi);
        }
        
        for (const auto& [type, batch] : batches) {
            renderBatch(batch);
        }
    }
    
    // Level-of-detail for zoomed out views
    void renderWithLOD(const ROIPtr& roi, float zoomLevel) {
        if (zoomLevel < 0.5f) {
            // Simple bounding box only
            renderBoundingBox(roi);
        } else if (zoomLevel < 2.0f) {
            // Basic shape
            renderSimplified(roi);
        } else {
            // Full detail
            renderFull(roi);
        }
    }
};
```

## Mobile/Tablet Optimization

### Touch-Optimized Controls

```cpp
class TouchOptimizedROISelector : public ROISelectorWidget {
    void enableTouchMode() {
        // Larger handles for touch
        m_handleSize = 16.0f;
        
        // Wider selection threshold
        m_selectionThreshold = 20.0f;
        
        // Enable gesture recognition
        grabGesture(Qt::PinchGesture);
        grabGesture(Qt::SwipeGesture);
        grabGesture(Qt::TapAndHoldGesture);
        
        // Touch-friendly margins
        m_touchMargin = 30.0f;
    }
};
```

## Integration Examples

### With PreProcessor

```cpp
// Interactive ROI adjustment during processing
preprocessor->onROIModified([](const ROIPtr& roi) {
    // Re-process only affected ROI
    reprocessROI(roi);
    
    // Update preview in real-time
    updatePreview(roi);
});

// Visual feedback during processing
preprocessor->onROIProcessing([](const ROIPtr& roi) {
    // Highlight currently processing ROI
    roi->setVisualState(ROIVisualState::Processing);
});
```

### With Camera Component

```cpp
// Live ROI overlay on camera feed
camera->onFrameReady([this](const cv::Mat& frame) {
    // Draw ROIs on live feed
    cv::Mat display = frame.clone();
    m_roiManager->drawROIs(display);
    
    // Show ROI statistics
    m_roiManager->updateStatistics(frame);
    
    emit displayReady(display);
});
```