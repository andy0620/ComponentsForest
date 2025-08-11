/**
 * @file modern_theme.h
 * @brief Modern UI Theme System for Do3Think Camera Viewer
 * 
 * Professional dark theme with vibrant gradients and glass-morphism effects
 */

#ifndef MODERN_THEME_H
#define MODERN_THEME_H

#include <QString>
#include <QWidget>
#include <QPainter>
#include <QPainterPath>
#include <QGraphicsDropShadowEffect>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QRect>
#include <QEasingCurve>

namespace ComponentsForest {
namespace ModernTheme {

// Color Palette
namespace Colors {
    // Primary colors
    constexpr const char* BACKGROUND_PRIMARY = "#0d1117";     // Deep dark blue-black
    constexpr const char* BACKGROUND_SECONDARY = "#161b22";   // Slightly lighter
    constexpr const char* BACKGROUND_TERTIARY = "#21262d";    // Card backgrounds
    
    // Accent colors
    constexpr const char* ACCENT_PRIMARY = "#58a6ff";        // Bright blue
    constexpr const char* ACCENT_SECONDARY = "#8b5cf6";      // Purple
    constexpr const char* ACCENT_SUCCESS = "#3fb950";        // Green
    constexpr const char* ACCENT_WARNING = "#d29922";        // Yellow
    constexpr const char* ACCENT_ERROR = "#f85149";          // Red
    
    // Text colors
    constexpr const char* TEXT_PRIMARY = "#f0f6fc";          // Almost white
    constexpr const char* TEXT_SECONDARY = "#8b949e";        // Muted gray
    constexpr const char* TEXT_DISABLED = "#484f58";         // Dark gray
    
    // Border colors
    constexpr const char* BORDER_DEFAULT = "#30363d";        // Subtle border
    constexpr const char* BORDER_HOVER = "#58a6ff";          // Blue on hover
    constexpr const char* BORDER_FOCUS = "#8b5cf6";          // Purple on focus
}

// Complete Modern Stylesheet
constexpr const char* MODERN_DARK_STYLESHEET = R"(
/* ==================== Global Styles ==================== */
QWidget {
    background-color: #0d1117;
    color: #f0f6fc;
    font-family: "Inter", "Segoe UI", -apple-system, BlinkMacSystemFont, sans-serif;
    font-size: 13px;
}

QMainWindow {
    background-color: #0d1117;
}

/* ==================== Dock Widgets ==================== */
QDockWidget {
    background-color: #161b22;
    border: none;
    titlebar-close-icon: url(:/icons/close.png);
    titlebar-normal-icon: url(:/icons/float.png);
}

QDockWidget::title {
    background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0,
                                 stop: 0 #161b22, stop: 0.5 #21262d, stop: 1 #161b22);
    text-align: left;
    padding: 8px 12px;
    border-bottom: 1px solid #30363d;
}

QDockWidget::close-button, QDockWidget::float-button {
    background: transparent;
    border: none;
    padding: 4px;
    icon-size: 16px;
}

QDockWidget::close-button:hover, QDockWidget::float-button:hover {
    background-color: rgba(88, 166, 255, 0.1);
    border-radius: 4px;
}

/* ==================== Menu Bar ==================== */
QMenuBar {
    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
                                stop: 0 #161b22, stop: 1 #0d1117);
    border-bottom: 1px solid #30363d;
    padding: 4px;
    spacing: 2px;
}

QMenuBar::item {
    background: transparent;
    padding: 6px 12px;
    border-radius: 6px;
    margin: 0 2px;
}

QMenuBar::item:selected {
    background: rgba(88, 166, 255, 0.15);
    color: #58a6ff;
}

QMenuBar::item:pressed {
    background: rgba(88, 166, 255, 0.25);
}

/* ==================== Menus ==================== */
QMenu {
    background-color: #1c2128;
    border: 1px solid #30363d;
    border-radius: 8px;
    padding: 4px;
}

QMenu::item {
    padding: 8px 24px 8px 12px;
    border-radius: 4px;
    margin: 2px 4px;
}

QMenu::item:selected {
    background: rgba(88, 166, 255, 0.15);
    color: #58a6ff;
}

QMenu::separator {
    height: 1px;
    background: #30363d;
    margin: 4px 8px;
}

QMenu::icon {
    padding-left: 8px;
}

/* ==================== Tool Bars ==================== */
QToolBar {
    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
                                stop: 0 #1c2128, stop: 1 #161b22);
    border: none;
    padding: 4px;
    spacing: 4px;
}

QToolBar::separator {
    background: #30363d;
    width: 1px;
    margin: 6px 8px;
}

QToolButton {
    background-color: transparent;
    border: 1px solid transparent;
    border-radius: 6px;
    padding: 6px;
    margin: 2px;
}

QToolButton:hover {
    background: rgba(88, 166, 255, 0.1);
    border: 1px solid rgba(88, 166, 255, 0.3);
}

QToolButton:pressed, QToolButton:checked {
    background: rgba(88, 166, 255, 0.2);
    border: 1px solid #58a6ff;
}

/* ==================== Tab Widget ==================== */
QTabWidget::pane {
    background-color: #161b22;
    border: 1px solid #30363d;
    border-radius: 8px;
    border-top-left-radius: 0;
    margin-top: -1px;
}

QTabBar::tab {
    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
                                stop: 0 #21262d, stop: 1 #1c2128);
    color: #8b949e;
    padding: 10px 16px;
    margin-right: 2px;
    border: 1px solid #30363d;
    border-bottom: none;
    border-top-left-radius: 8px;
    border-top-right-radius: 8px;
}

QTabBar::tab:selected {
    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
                                stop: 0 #161b22, stop: 1 #161b22);
    color: #f0f6fc;
    border-bottom: 2px solid #58a6ff;
}

QTabBar::tab:hover:!selected {
    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
                                stop: 0 #2d333b, stop: 1 #21262d);
    color: #f0f6fc;
}

QTabBar::close-button {
    image: url(:/icons/close-small.png);
    subcontrol-position: right;
    padding: 2px;
}

QTabBar::close-button:hover {
    background: rgba(248, 81, 73, 0.2);
    border-radius: 3px;
}

/* ==================== Buttons ==================== */
QPushButton {
    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
                                stop: 0 #21262d, stop: 1 #1c2128);
    color: #f0f6fc;
    border: 1px solid #30363d;
    border-radius: 6px;
    padding: 8px 16px;
    font-weight: 500;
    min-width: 80px;
}

QPushButton:hover {
    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
                                stop: 0 #2d333b, stop: 1 #21262d);
    border: 1px solid #58a6ff;
}

QPushButton:pressed {
    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
                                stop: 0 #1c2128, stop: 1 #161b22);
    border: 1px solid #58a6ff;
}

QPushButton:disabled {
    background: #161b22;
    color: #484f58;
    border: 1px solid #21262d;
}

/* Primary Buttons */
QPushButton[primary="true"] {
    background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 1,
                                stop: 0 #58a6ff, stop: 1 #8b5cf6);
    border: none;
    color: white;
    font-weight: 600;
}

QPushButton[primary="true"]:hover {
    background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 1,
                                stop: 0 #6bb1ff, stop: 1 #9d71f7);
}

QPushButton[primary="true"]:pressed {
    background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 1,
                                stop: 0 #4d9bf0, stop: 1 #7a4be5);
}

/* ==================== Group Boxes ==================== */
QGroupBox {
    background-color: #161b22;
    border: 1px solid #30363d;
    border-radius: 8px;
    margin-top: 12px;
    padding-top: 12px;
    font-weight: 600;
}

QGroupBox::title {
    subcontrol-origin: margin;
    subcontrol-position: top left;
    background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0,
                                stop: 0 #58a6ff, stop: 1 #8b5cf6);
    -webkit-background-clip: text;
    color: #58a6ff;
    padding: 0 8px;
    left: 12px;
}

/* ==================== Sliders ==================== */
QSlider {
    min-height: 20px;
}

QSlider::groove:horizontal {
    background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0,
                                stop: 0 #21262d, stop: 1 #21262d);
    height: 6px;
    border-radius: 3px;
    border: 1px solid #30363d;
}

QSlider::handle:horizontal {
    background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 1,
                                stop: 0 #58a6ff, stop: 1 #8b5cf6);
    width: 18px;
    height: 18px;
    margin: -6px 0;
    border-radius: 9px;
    border: 2px solid #161b22;
}

QSlider::handle:horizontal:hover {
    background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 1,
                                stop: 0 #6bb1ff, stop: 1 #9d71f7);
    border: 2px solid #58a6ff;
}

QSlider::sub-page:horizontal {
    background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0,
                                stop: 0 #58a6ff, stop: 1 #8b5cf6);
    border-radius: 3px;
}

/* ==================== Spin Boxes ==================== */
QSpinBox, QDoubleSpinBox {
    background-color: #21262d;
    border: 1px solid #30363d;
    border-radius: 6px;
    padding: 6px;
    selection-background-color: #58a6ff;
}

QSpinBox:hover, QDoubleSpinBox:hover {
    border: 1px solid #58a6ff;
}

QSpinBox:focus, QDoubleSpinBox:focus {
    border: 1px solid #8b5cf6;
    background-color: #1c2128;
}

QSpinBox::up-button, QDoubleSpinBox::up-button,
QSpinBox::down-button, QDoubleSpinBox::down-button {
    background-color: transparent;
    border: none;
    width: 16px;
}

QSpinBox::up-button:hover, QDoubleSpinBox::up-button:hover,
QSpinBox::down-button:hover, QDoubleSpinBox::down-button:hover {
    background-color: rgba(88, 166, 255, 0.1);
}

/* ==================== Combo Boxes ==================== */
QComboBox {
    background-color: #21262d;
    border: 1px solid #30363d;
    border-radius: 6px;
    padding: 6px 8px;
    min-width: 100px;
}

QComboBox:hover {
    border: 1px solid #58a6ff;
}

QComboBox:focus {
    border: 1px solid #8b5cf6;
    background-color: #1c2128;
}

QComboBox::drop-down {
    border: none;
    width: 20px;
}

QComboBox::down-arrow {
    image: url(:/icons/chevron-down.png);
    width: 12px;
    height: 12px;
}

QComboBox QAbstractItemView {
    background-color: #1c2128;
    border: 1px solid #30363d;
    border-radius: 6px;
    selection-background-color: rgba(88, 166, 255, 0.2);
    outline: none;
}

/* ==================== Check Boxes & Radio Buttons ==================== */
QCheckBox, QRadioButton {
    spacing: 8px;
    color: #f0f6fc;
}

QCheckBox::indicator, QRadioButton::indicator {
    width: 18px;
    height: 18px;
    border: 2px solid #30363d;
    background-color: #21262d;
}

QCheckBox::indicator {
    border-radius: 4px;
}

QRadioButton::indicator {
    border-radius: 9px;
}

QCheckBox::indicator:hover, QRadioButton::indicator:hover {
    border: 2px solid #58a6ff;
    background-color: rgba(88, 166, 255, 0.1);
}

QCheckBox::indicator:checked, QRadioButton::indicator:checked {
    background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 1,
                                stop: 0 #58a6ff, stop: 1 #8b5cf6);
    border: 2px solid #58a6ff;
}

QCheckBox::indicator:checked {
    image: url(:/icons/check.png);
}

QRadioButton::indicator:checked {
    background: qradialgradient(cx: 0.5, cy: 0.5, radius: 0.5,
                                fx: 0.5, fy: 0.5,
                                stop: 0 white, stop: 0.3 white,
                                stop: 0.4 transparent, stop: 1 transparent);
}

/* ==================== Progress Bars ==================== */
QProgressBar {
    background-color: #21262d;
    border: 1px solid #30363d;
    border-radius: 6px;
    text-align: center;
    height: 20px;
}

QProgressBar::chunk {
    background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0,
                                stop: 0 #58a6ff, stop: 1 #8b5cf6);
    border-radius: 5px;
}

/* ==================== List Widgets ==================== */
QListWidget {
    background-color: #161b22;
    border: 1px solid #30363d;
    border-radius: 6px;
    padding: 4px;
    outline: none;
}

QListWidget::item {
    background-color: transparent;
    padding: 6px;
    border-radius: 4px;
    margin: 2px;
}

QListWidget::item:hover {
    background-color: rgba(88, 166, 255, 0.1);
}

QListWidget::item:selected {
    background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0,
                                stop: 0 rgba(88, 166, 255, 0.2),
                                stop: 1 rgba(139, 92, 246, 0.2));
    border: 1px solid #58a6ff;
}

/* ==================== Text Edit ==================== */
QTextEdit, QPlainTextEdit {
    background-color: #161b22;
    border: 1px solid #30363d;
    border-radius: 6px;
    padding: 8px;
    selection-background-color: #58a6ff;
    selection-color: white;
}

QTextEdit:focus, QPlainTextEdit:focus {
    border: 1px solid #8b5cf6;
}

/* ==================== Scroll Bars ==================== */
QScrollBar:vertical, QScrollBar:horizontal {
    background-color: #161b22;
    border: none;
    width: 12px;
    height: 12px;
}

QScrollBar::handle:vertical, QScrollBar::handle:horizontal {
    background-color: #30363d;
    border-radius: 6px;
    min-height: 30px;
    min-width: 30px;
}

QScrollBar::handle:vertical:hover, QScrollBar::handle:horizontal:hover {
    background-color: #484f58;
}

QScrollBar::add-line, QScrollBar::sub-line {
    border: none;
    background: none;
}

QScrollBar::add-page, QScrollBar::sub-page {
    background: none;
}

/* ==================== Status Bar ==================== */
QStatusBar {
    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
                                stop: 0 #161b22, stop: 1 #0d1117);
    border-top: 1px solid #30363d;
    padding: 4px;
    min-height: 24px;
}

QStatusBar::item {
    border: none;
}

QStatusBar QLabel {
    color: #8b949e;
    padding: 0 8px;
}

/* ==================== Splitters ==================== */
QSplitter::handle {
    background-color: #30363d;
}

QSplitter::handle:horizontal {
    width: 2px;
}

QSplitter::handle:vertical {
    height: 2px;
}

QSplitter::handle:hover {
    background-color: #58a6ff;
}

/* ==================== Tool Tips ==================== */
QToolTip {
    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
                                stop: 0 #2d333b, stop: 1 #21262d);
    color: #f0f6fc;
    border: 1px solid #58a6ff;
    border-radius: 6px;
    padding: 6px 8px;
    font-size: 12px;
}

/* ==================== Special Elements ==================== */
/* Glass Card Effect */
QWidget[card="true"] {
    background: rgba(22, 27, 34, 0.8);
    border: 1px solid rgba(48, 54, 61, 0.5);
    border-radius: 12px;
    padding: 16px;
}

/* Floating Action Button */
QPushButton[floating="true"] {
    background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 1,
                                stop: 0 #58a6ff, stop: 1 #8b5cf6);
    border: none;
    border-radius: 28px;
    padding: 14px;
    min-width: 56px;
    min-height: 56px;
    font-size: 24px;
    color: white;
}

QPushButton[floating="true"]:hover {
    background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 1,
                                stop: 0 #6bb1ff, stop: 1 #9d71f7);
}

/* Toggle Switch Style */
QCheckBox[toggle="true"] {
    min-width: 48px;
    max-width: 48px;
    min-height: 24px;
    max-height: 24px;
}

QCheckBox[toggle="true"]::indicator {
    width: 48px;
    height: 24px;
    border-radius: 12px;
    border: 2px solid #30363d;
    background-color: #21262d;
}

QCheckBox[toggle="true"]::indicator:checked {
    background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0,
                                stop: 0 #58a6ff, stop: 1 #8b5cf6);
    border: 2px solid #58a6ff;
}

QCheckBox[toggle="true"]::indicator:checked::after {
    content: "";
    position: absolute;
    width: 18px;
    height: 18px;
    border-radius: 9px;
    background: white;
    left: 26px;
    top: 3px;
}

/* Animated Loading Indicator */
QLabel[loading="true"] {
    background: transparent;
    border: 3px solid #21262d;
    border-top: 3px solid #58a6ff;
    border-radius: 50%;
    width: 24px;
    height: 24px;
}
)";

// Helper class for custom widgets
class ModernCard : public QWidget {
public:
    ModernCard(QWidget* parent = nullptr) : QWidget(parent) {
        setProperty("card", true);
        
        // Add drop shadow effect
        auto* shadow = new QGraphicsDropShadowEffect(this);
        shadow->setBlurRadius(20);
        shadow->setXOffset(0);
        shadow->setYOffset(4);
        shadow->setColor(QColor(0, 0, 0, 60));
        setGraphicsEffect(shadow);
    }
    
protected:
    void paintEvent(QPaintEvent* event) override {
        Q_UNUSED(event);
        
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        
        // Draw glass effect background
        QPainterPath path;
        path.addRoundedRect(rect(), 12, 12);
        
        // Gradient background
        QLinearGradient gradient(0, 0, 0, height());
        gradient.setColorAt(0, QColor(22, 27, 34, 200));
        gradient.setColorAt(1, QColor(22, 27, 34, 180));
        
        painter.fillPath(path, gradient);
        
        // Draw border
        painter.setPen(QPen(QColor(48, 54, 61, 127), 1));
        painter.drawPath(path);
    }
};

// Animation helper functions
inline void animateWidgetOpacity(QWidget* widget, qreal from, qreal to, int duration = 300) {
    auto* effect = new QGraphicsOpacityEffect(widget);
    widget->setGraphicsEffect(effect);
    
    auto* animation = new QPropertyAnimation(effect, "opacity");
    animation->setDuration(duration);
    animation->setStartValue(from);
    animation->setEndValue(to);
    animation->setEasingCurve(QEasingCurve::OutCubic);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

inline void animateWidgetGeometry(QWidget* widget, const QRect& from, const QRect& to, int duration = 300) {
    auto* animation = new QPropertyAnimation(widget, "geometry");
    animation->setDuration(duration);
    animation->setStartValue(from);
    animation->setEndValue(to);
    animation->setEasingCurve(QEasingCurve::OutCubic);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

} // namespace ModernTheme
} // namespace ComponentsForest

#endif // MODERN_THEME_H