/**
 * @file futuristic_theme.h
 * @brief Futuristic Cyberpunk Theme System for Do3Think Camera Viewer
 * 
 * Advanced sci-fi interface with neon accents, holographic effects, and animated gradients
 * Inspired by Iron Man's JARVIS interface and cyberpunk aesthetics
 */

#ifndef FUTURISTIC_THEME_H
#define FUTURISTIC_THEME_H

#include <QWidget>
#include <QString>
#include <QColor>
#include <QPainter>
#include <QPainterPath>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QSequentialAnimationGroup>
#include <QAbstractAnimation>
#include <QGraphicsEffect>
#include <QGraphicsDropShadowEffect>
#include <QGraphicsBlurEffect>
#include <QGraphicsColorizeEffect>
#include <QLinearGradient>
#include <QRadialGradient>
#include <QConicalGradient>
#include <QTimer>
#include <QEasingCurve>
#include <QStyle>
#include <QProxyStyle>
#include <QStyleOption>
#include <memory>
#include <vector>

namespace ComponentsForest {
namespace FuturisticTheme {

// ==================== Color Palette ====================
namespace Colors {
    // Dark base colors
    constexpr const char* VOID_BLACK = "#0A0E1A";           // Deep space black
    constexpr const char* DARK_MATTER = "#141824";          // Dark blue-gray
    constexpr const char* QUANTUM_GRAY = "#1F2937";         // Medium gray
    constexpr const char* PLASMA_DARK = "#2A3142";          // Lighter dark
    
    // Neon accent colors
    constexpr const char* NEON_CYAN = "#00D4FF";           // Bright cyan
    constexpr const char* NEON_MAGENTA = "#FF00AA";        // Hot magenta
    constexpr const char* NEON_BLUE = "#0099FF";           // Electric blue
    constexpr const char* NEON_PURPLE = "#9945FF";         // Deep purple
    constexpr const char* NEON_GREEN = "#00FF88";          // Matrix green
    constexpr const char* NEON_ORANGE = "#FF8800";         // Warning orange
    constexpr const char* NEON_RED = "#FF0044";            // Error red
    constexpr const char* NEON_YELLOW = "#FFD700";         // Gold yellow
    
    // Holographic colors
    constexpr const char* HOLO_BLUE = "#00B4D8";           // Holographic blue
    constexpr const char* HOLO_PURPLE = "#7209B7";         // Holographic purple
    constexpr const char* HOLO_PINK = "#F72585";           // Holographic pink
    
    // Text colors
    constexpr const char* TEXT_BRIGHT = "#E0E8F0";         // Bright text
    constexpr const char* TEXT_NORMAL = "#A0B0C0";         // Normal text
    constexpr const char* TEXT_DIM = "#607080";            // Dimmed text
    constexpr const char* TEXT_GLOW = "#FFFFFF";           // Glowing text
    
    // Glass colors (now fully opaque)
    inline QColor glassBackground() { return QColor(20, 24, 36, 255); }
    inline QColor glassBorder() { return QColor(0, 212, 255, 255); }
    inline QColor glassHighlight() { return QColor(255, 255, 255, 255); }
}

// ==================== Futuristic Style Sheet ====================
constexpr const char* FUTURISTIC_STYLESHEET_PART1 = R"(
/* ==================== Global Reset ==================== */
* {
    margin: 0;
    padding: 0;
    border: 0;
}

/* ==================== Main Widget ==================== */
QWidget {
    background-color: #0A0E1A;
    color: #E0E8F0;
    font-family: "JetBrains Mono", "Fira Code", "Source Code Pro", "Roboto Mono", "SF Mono", "Monaco", "Cascadia Code", "Consolas", monospace;
    font-size: 13px;
    font-weight: 400;
}

QMainWindow {
    background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 1,
                                stop: 0 #0A0E1A, 
                                stop: 0.5 #141824,
                                stop: 1 #0A0E1A);
    /* Subtle circuit board pattern */
    background-image: repeating-linear-gradient(
        45deg,
        transparent,
        transparent 49px,
        rgba(0, 255, 212, 0.02) 49px,
        rgba(0, 255, 212, 0.02) 50px
    ),
    repeating-linear-gradient(
        -45deg,
        transparent,
        transparent 49px,
        rgba(0, 229, 204, 0.02) 49px,
        rgba(0, 229, 204, 0.02) 50px
    );
}

/* ==================== Futuristic Dock Widgets (Glass Morphism) ==================== */
QDockWidget {
    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
                                stop: 0 rgba(20, 24, 36, 0.9),
                                stop: 1 rgba(10, 14, 26, 0.95));
    border: 1px solid rgba(0, 212, 255, 0.15);
    border-radius: 20px;
    margin: 6px;
    backdrop-filter: blur(12px);
    box-shadow: 0 12px 40px rgba(0, 0, 0, 0.2),
                0 0 30px rgba(0, 212, 255, 0.05),
                inset 0 1px 0 rgba(255, 255, 255, 0.02);
}

QDockWidget::title {
    background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0,
                                stop: 0 #1A2332,
                                stop: 0.5 #2A1A2E,
                                stop: 1 #1A2332);
    text-align: left;
    padding: 10px 15px;
    border-bottom: 2px solid qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0,
                                             stop: 0 #00FFD4,
                                             stop: 0.5 #00E5CC,
                                             stop: 1 #00FFD4);
    font-family: "JetBrains Mono", "Fira Code", monospace;
    font-weight: 600;
    font-size: 13px;
    text-transform: uppercase;
    letter-spacing: 1.5px;
    /* Tech corner brackets effect */
    border-left: 3px solid rgba(0, 255, 212, 0.6);
    border-right: 3px solid rgba(0, 255, 212, 0.6);
}

/* ==================== Holographic Menu Bar (Ultra-Modern) ==================== */
QMenuBar {
    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
                                stop: 0 rgba(20, 24, 36, 0.95),
                                stop: 1 rgba(10, 14, 26, 0.98));
    border-bottom: 1px solid rgba(0, 212, 255, 0.3);
    padding: 8px;
    spacing: 8px;
    backdrop-filter: blur(10px);
}

QMenuBar::item {
    background: transparent;
    color: #A0B0C0;
    padding: 8px 16px;
    margin: 0 4px;
    border-radius: 10px;
    font-family: "JetBrains Mono", monospace;
    font-weight: 500;
    font-size: 12px;
    text-transform: none;
    letter-spacing: 0.5px;
}

QMenuBar::item:selected {
    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
                                stop: 0 rgba(0, 61, 82, 0.7),
                                stop: 1 rgba(77, 0, 51, 0.7));
    color: #00D4FF;
    border: 1px solid rgba(0, 212, 255, 0.3);
    backdrop-filter: blur(6px);
    box-shadow: 0 4px 12px rgba(0, 212, 255, 0.15),
                inset 0 1px 0 rgba(255, 255, 255, 0.05);
}

QMenuBar::item:pressed {
    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
                                stop: 0 #004D66,
                                stop: 1 #660044);
    color: #FFFFFF;
}

/* ==================== Holographic Menus (Glass Morphism) ==================== */
QMenu {
    background: rgba(20, 24, 36, 0.98);
    border: 1px solid rgba(0, 212, 255, 0.2);
    border-radius: 16px;
    padding: 8px;
    backdrop-filter: blur(12px);
    box-shadow: 0 12px 40px rgba(0, 0, 0, 0.3),
                0 0 30px rgba(0, 212, 255, 0.1);
}

QMenu::item {
    background: transparent;
    color: #A0B0C0;
    padding: 12px 36px 12px 20px;
    margin: 2px 4px;
    border-left: 3px solid transparent;
    border-radius: 8px;
}

QMenu::item:selected {
    background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0,
                                stop: 0 rgba(0, 61, 82, 0.6),
                                stop: 1 rgba(20, 24, 36, 0.4));
    color: #00D4FF;
    border-left: 3px solid rgba(0, 212, 255, 0.8);
    box-shadow: 0 2px 8px rgba(0, 212, 255, 0.15),
                inset 0 1px 0 rgba(255, 255, 255, 0.03);
}

QMenu::separator {
    height: 1px;
    background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0,
                                stop: 0 transparent,
                                stop: 0.5 rgba(0, 212, 255, 0.3),
                                stop: 1 transparent);
    margin: 8px 16px;
}

/* ==================== Futuristic Tool Bar ==================== */
QToolBar {
    background: #141824;
    border: none;
    border-bottom: 1px solid #004D66;
    padding: 5px;
    spacing: 5px;
}

QToolButton {
    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
                                stop: 0 #2A3142,
                                stop: 1 #1F2937);
    color: #A0B0C0;
    border: 1px solid rgba(0, 255, 212, 0.2);
    border-radius: 18px;
    padding: 8px;
    margin: 3px;
    font-family: "JetBrains Mono", monospace;
    font-weight: 500;
    font-size: 11px;
    text-transform: uppercase;
    min-width: 36px;
    min-height: 36px;
    /* Tech corner accent */
    border-top-left-radius: 4px;
    border-bottom-right-radius: 4px;
}

QToolButton:hover {
    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
                                stop: 0 #003D52,
                                stop: 1 #002633);
    color: #00FFD4;
    border: 1px solid rgba(0, 255, 212, 0.6);
    box-shadow: 0 0 12px rgba(0, 255, 212, 0.3);
}

QToolButton:pressed,
QToolButton:checked {
    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
                                stop: 0 #004D66,
                                stop: 1 #003D52);
    color: #00FF41;
    border: 1px solid rgba(0, 255, 65, 0.8);
    box-shadow: 0 0 15px rgba(0, 255, 65, 0.4),
                inset 0 0 5px rgba(0, 255, 65, 0.2);
}

/* ==================== Holographic Tab Widget ==================== */
QTabWidget::pane {
    background: #141824;
    border: 1px solid #004D66;
    border-radius: 0px;
    margin-top: -1px;
}

QTabBar::tab {
    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
                                stop: 0 #1F2937,
                                stop: 1 #141824);
    color: #607080;
    padding: 12px 20px;
    margin-right: 2px;
    border: 1px solid #003D52;
    border-bottom: none;
    font-weight: 500;
    text-transform: uppercase;
    letter-spacing: 1px;
}

QTabBar::tab:selected {
    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
                                stop: 0 #00263A,
                                stop: 1 #141824);
    color: #00D4FF;
    border: 1px solid #0080AA;
    border-bottom: 3px solid #00D4FF;
}

QTabBar::tab:hover:!selected {
    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
                                stop: 0 #001A26,
                                stop: 1 #1F2937);
    color: #A0B0C0;
    border: 1px solid #004D66;
}

/* ==================== Neon Buttons (Compact Tech Style) ==================== */
QPushButton {
    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
                                stop: 0 #1F2937,
                                stop: 1 #141824);
    color: #A0B0C0;
    border: 1px solid rgba(0, 229, 204, 0.3);
    border-radius: 16px;
    padding: 8px 16px;
    font-family: "JetBrains Mono", "Fira Code", monospace;
    font-weight: 500;
    font-size: 12px;
    text-transform: uppercase;
    letter-spacing: 0.8px;
    min-width: 80px;
    min-height: 32px;
    /* Tech corner brackets */
    border-top-left-radius: 4px;
    border-bottom-right-radius: 4px;
}

QPushButton:hover {
    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
                                stop: 0 #003D52,
                                stop: 1 #002633);
    color: #00FFD4;
    border: 1px solid rgba(0, 255, 212, 0.8);
    box-shadow: 0 0 20px rgba(0, 255, 212, 0.4),
                inset 0 0 10px rgba(0, 255, 212, 0.1);
}

QPushButton:pressed {
    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
                                stop: 0 #004D66,
                                stop: 1 #003344);
    color: #00FF41;
    border: 1px solid rgba(0, 255, 65, 0.9);
    box-shadow: 0 0 25px rgba(0, 255, 65, 0.5),
                inset 0 0 15px rgba(0, 255, 65, 0.2);
}

QPushButton:disabled {
    background: #0F1218;
    color: #607080;
    border: 1px solid #303840;
}

/* Primary Neon Button (Compact Tech) */
QPushButton[primary="true"] {
    background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0,
                                stop: 0 #00FFD4,
                                stop: 0.5 #00E5CC,
                                stop: 1 #00FFD4);
    border: 1px solid rgba(0, 255, 212, 0.8);
    border-radius: 18px;
    padding: 10px 20px;
    color: #0A0E1A;
    font-family: "JetBrains Mono", monospace;
    font-weight: 600;
    font-size: 13px;
    text-shadow: 0 1px 2px rgba(0, 0, 0, 0.3);
    min-width: 90px;
    min-height: 36px;
    box-shadow: 0 0 30px rgba(0, 255, 212, 0.5),
                0 4px 12px rgba(0, 0, 0, 0.2);
}

QPushButton[primary="true"]:hover {
    background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0,
                                stop: 0 #00FF41,
                                stop: 0.5 #00FFD4,
                                stop: 1 #00FF41);
    color: #0A0E1A;
    border: 1px solid rgba(0, 255, 65, 0.9);
    text-shadow: 0 1px 3px rgba(0, 0, 0, 0.4);
    box-shadow: 0 0 40px rgba(0, 255, 65, 0.6),
                0 6px 20px rgba(0, 0, 0, 0.3);
}

QPushButton[primary="true"]:pressed {
    background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0,
                                stop: 0 #00CC33,
                                stop: 0.5 #00B3A0,
                                stop: 1 #00CC33);
    padding: 11px 20px 9px 20px;
}

/* ==================== Holographic Group Boxes ==================== */
QGroupBox {
    background: #141824;
    border: 1px solid #004D66;
    border-radius: 16px;
    margin-top: 15px;
    padding-top: 15px;
    font-weight: 600;
}

QGroupBox::title {
    subcontrol-origin: margin;
    subcontrol-position: top left;
    background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0,
                                stop: 0 #00FFD4,
                                stop: 0.5 #00E5CC,
                                stop: 1 #00FF41);
    color: #0A0E1A;
    padding: 5px 15px;
    left: 10px;
    border: 1px solid rgba(0, 255, 212, 0.8);
    font-family: "JetBrains Mono", monospace;
    text-transform: uppercase;
    letter-spacing: 1.5px;
    font-size: 11px;
    font-weight: 600;
    box-shadow: 0 0 15px rgba(0, 255, 212, 0.3);
}

/* ==================== Futuristic Sliders ==================== */
QSlider {
    min-height: 30px;
}

QSlider::groove:horizontal {
    background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0,
                                stop: 0 #1F2937,
                                stop: 0.5 #141824,
                                stop: 1 #1F2937);
    height: 8px;
    border-radius: 4px;
    border: 1px solid #004D66;
}

QSlider::handle:horizontal {
    background: qradialgradient(cx: 0.5, cy: 0.5, radius: 0.5,
                                fx: 0.5, fy: 0.5,
                                stop: 0 #00D4FF,
                                stop: 0.5 #0099FF,
                                stop: 1 #00A9CC);
    width: 20px;
    height: 20px;
    margin: -6px 0;
    border-radius: 10px;
    border: 2px solid #00D4FF;
}

QSlider::handle:horizontal:hover {
    background: qradialgradient(cx: 0.5, cy: 0.5, radius: 0.5,
                                fx: 0.5, fy: 0.5,
                                stop: 0 #FFFFFF,
                                stop: 0.3 #00D4FF,
                                stop: 1 #00B3E6);
    border: 2px solid #FFFFFF;
}

QSlider::sub-page:horizontal {
    background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0,
                                stop: 0 #00D4FF,
                                stop: 1 #FF00AA);
    border-radius: 4px;
}

)";

constexpr const char* FUTURISTIC_STYLESHEET_PART2 = R"(
/* ==================== Tech Spin Boxes ==================== */
QSpinBox, QDoubleSpinBox {
    background: #1F2937;
    color: #00FFD4;
    border: 1px solid rgba(0, 229, 204, 0.3);
    border-radius: 12px;
    padding: 8px;
    min-height: 32px;
    selection-background-color: rgba(0, 255, 212, 0.2);
    font-family: "JetBrains Mono", "Fira Code", monospace;
    font-size: 13px;
    /* Tech corner accent */
    border-top-left-radius: 2px;
    border-bottom-right-radius: 2px;
}

QSpinBox:hover, QDoubleSpinBox:hover {
    border: 1px solid #0099CC;
    background: #242C3D;
}

QSpinBox:focus, QDoubleSpinBox:focus {
    border: 1px solid rgba(0, 255, 65, 0.8);
    background: #001A26;
    color: #00FF41;
    box-shadow: 0 0 15px rgba(0, 255, 65, 0.3);
}

/* ==================== Cyber Combo Boxes ==================== */
QComboBox {
    background: #1F2937;
    color: #A0B0C0;
    border: 1px solid rgba(0, 229, 204, 0.3);
    border-radius: 12px;
    padding: 8px 10px;
    min-width: 120px;
    min-height: 32px;
    font-family: "JetBrains Mono", monospace;
    font-size: 12px;
    text-transform: uppercase;
    /* Tech corner accent */
    border-top-left-radius: 2px;
    border-bottom-right-radius: 2px;
}

QComboBox:hover {
    border: 1px solid rgba(0, 255, 212, 0.6);
    color: #00FFD4;
    box-shadow: 0 0 10px rgba(0, 255, 212, 0.2);
}

QComboBox:focus {
    border: 1px solid rgba(0, 255, 65, 0.8);
    background: #001A26;
    color: #00FF41;
    box-shadow: 0 0 15px rgba(0, 255, 65, 0.3);
}

QComboBox::drop-down {
    border: none;
    width: 25px;
    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
                                stop: 0 #003D52,
                                stop: 1 #141824);
}

QComboBox::down-arrow {
    image: none;
    border-left: 5px solid transparent;
    border-right: 5px solid transparent;
    border-top: 5px solid #00FFD4;
    width: 0;
    height: 0;
    margin-right: 5px;
}

QComboBox QAbstractItemView {
    background: #141824;
    border: 1px solid #0080AA;
    selection-background-color: #004D66;
    color: #A0B0C0;
    outline: none;
}

/* ==================== Neon Check Boxes & Radio Buttons ==================== */
QCheckBox, QRadioButton {
    spacing: 10px;
    color: #A0B0C0;
    font-weight: 500;
}

QCheckBox::indicator, QRadioButton::indicator {
    width: 20px;
    height: 20px;
    border: 2px solid #006680;
    background: #141824;
}

QCheckBox::indicator {
    border-radius: 6px;
}

QRadioButton::indicator {
    border-radius: 10px;
}

QCheckBox::indicator:hover, QRadioButton::indicator:hover {
    border: 2px solid #00CCFF;
    background: #001A26;
}

QCheckBox::indicator:checked {
    background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 1,
                                stop: 0 #00FF41,
                                stop: 1 #00FFD4);
    border: 2px solid rgba(0, 255, 65, 0.9);
    image: none;
    box-shadow: 0 0 10px rgba(0, 255, 65, 0.4);
}

QRadioButton::indicator:checked {
    background: qradialgradient(cx: 0.5, cy: 0.5, radius: 0.5,
                                fx: 0.5, fy: 0.5,
                                stop: 0 #00D4FF,
                                stop: 0.4 #00D4FF,
                                stop: 0.5 transparent,
                                stop: 1 transparent);
    border: 2px solid #00D4FF;
}

/* ==================== Holographic Progress Bars ==================== */
QProgressBar {
    background: #141824;
    border: 1px solid #004D66;
    border-radius: 0px;
    text-align: center;
    color: #00D4FF;
    font-weight: 600;
    height: 25px;
}

QProgressBar::chunk {
    background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0,
                                stop: 0 #00D4FF,
                                stop: 0.5 #FF00AA,
                                stop: 1 #0099FF);
    border-radius: 0px;
}

/* ==================== Matrix List Widgets (Glass Morphism) ==================== */
QListWidget {
    background: rgba(20, 24, 36, 0.9);
    border: 1px solid rgba(0, 212, 255, 0.15);
    border-radius: 16px;
    padding: 8px;
    outline: none;
    backdrop-filter: blur(8px);
    box-shadow: 0 4px 16px rgba(0, 0, 0, 0.15),
                inset 0 1px 0 rgba(255, 255, 255, 0.02);
}

QListWidget::item {
    background: transparent;
    color: #A0B0C0;
    padding: 10px 12px;
    border-bottom: 1px solid rgba(0, 26, 38, 0.3);
    margin: 2px;
    border-radius: 8px;
}

QListWidget::item:hover {
    background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0,
                                stop: 0 rgba(0, 61, 82, 0.5),
                                stop: 1 rgba(20, 24, 36, 0.3));
    color: #00D4FF;
    box-shadow: 0 2px 8px rgba(0, 212, 255, 0.1);
}

QListWidget::item:selected {
    background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0,
                                stop: 0 rgba(0, 77, 102, 0.6),
                                stop: 0.5 rgba(102, 0, 68, 0.5),
                                stop: 1 rgba(20, 24, 36, 0.4));
    border-left: 3px solid rgba(0, 212, 255, 0.8);
    color: #FFFFFF;
    box-shadow: 0 4px 12px rgba(0, 212, 255, 0.15),
                0 0 20px rgba(255, 0, 170, 0.1);
}

/* ==================== Tech Text Edit ==================== */
QTextEdit, QPlainTextEdit {
    background: #141824;
    color: #00FF88;
    border: 1px solid rgba(0, 255, 136, 0.3);
    border-radius: 12px;
    padding: 10px;
    selection-background-color: rgba(0, 255, 65, 0.2);
    selection-color: #FFFFFF;
    font-family: "JetBrains Mono", "Fira Code", monospace;
    font-size: 13px;
    /* Tech corner brackets */
    border-top-left-radius: 2px;
    border-bottom-right-radius: 2px;
}

QTextEdit:focus, QPlainTextEdit:focus {
    border: 1px solid rgba(0, 255, 65, 0.8);
    background: #0D1520;
    box-shadow: 0 0 20px rgba(0, 255, 65, 0.3),
                inset 0 0 10px rgba(0, 255, 65, 0.1);
}

/* ==================== Futuristic Scroll Bars ==================== */
QScrollBar:vertical, QScrollBar:horizontal {
    background: #0A0E1A;
    border: none;
    width: 12px;
    height: 12px;
}

QScrollBar::handle:vertical, QScrollBar::handle:horizontal {
    background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0,
                                stop: 0 #0080AA,
                                stop: 0.5 #AA0080,
                                stop: 1 #0080AA);
    border-radius: 0px;
    min-height: 40px;
    min-width: 40px;
}

QScrollBar::handle:vertical:hover, QScrollBar::handle:horizontal:hover {
    background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0,
                                stop: 0 #00CCFF,
                                stop: 0.5 #CC00AA,
                                stop: 1 #00CCFF);
}

QScrollBar::add-line, QScrollBar::sub-line {
    border: none;
    background: none;
}

QScrollBar::add-page, QScrollBar::sub-page {
    background: none;
}

/* ==================== Holographic Status Bar (Glass Morphism) ==================== */
QStatusBar {
    background: rgba(20, 24, 36, 0.95);
    border-top: 1px solid rgba(0, 212, 255, 0.3);
    padding: 8px;
    min-height: 36px;
    backdrop-filter: blur(10px);
    box-shadow: 0 -2px 8px rgba(0, 0, 0, 0.15);
}

QStatusBar::item {
    border: none;
}

QStatusBar QLabel {
    color: #00FFD4;
    padding: 0 12px;
    font-family: "JetBrains Mono", "Fira Code", monospace;
    font-size: 12px;
    text-transform: none;
    font-weight: 500;
    text-shadow: 0 0 8px rgba(0, 255, 212, 0.4);
    /* Add subtle scanline effect */
    background: repeating-linear-gradient(
        0deg,
        transparent,
        transparent 1px,
        rgba(0, 255, 212, 0.03) 1px,
        rgba(0, 255, 212, 0.03) 2px
    );
}

/* ==================== Glowing Tool Tips (Ultra-Modern) ==================== */
QToolTip {
    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
                                stop: 0 rgba(0, 179, 230, 0.95),
                                stop: 1 rgba(0, 179, 230, 0.85));
    color: #0A0E1A;
    border: 2px solid rgba(255, 255, 255, 0.8);
    border-radius: 12px;
    padding: 10px 16px;
    font-weight: 600;
    font-size: 13px;
    text-transform: none;
    backdrop-filter: blur(8px);
    box-shadow: 0 8px 24px rgba(0, 212, 255, 0.3),
                0 4px 8px rgba(0, 0, 0, 0.2),
                0 0 30px rgba(0, 212, 255, 0.2);
}

/* ==================== Special Futuristic Elements ==================== */
/* Holographic Card (Ultra-Modern Glass) */
QWidget[holographic="true"] {
    background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 1,
                                stop: 0 rgba(0, 26, 38, 0.85),
                                stop: 0.5 rgba(38, 0, 25, 0.8),
                                stop: 1 rgba(0, 19, 38, 0.85));
    border: 1px solid rgba(0, 212, 255, 0.3);
    border-radius: 24px;
    padding: 24px;
    backdrop-filter: blur(12px);
    box-shadow: 0 12px 40px rgba(0, 0, 0, 0.2),
                0 0 40px rgba(0, 212, 255, 0.08),
                inset 0 1px 0 rgba(255, 255, 255, 0.05);
}

/* Neon Glow Button (Compact) */
QPushButton[neon="true"] {
    background: transparent;
    color: #00FFD4;
    border: 2px solid #00FFD4;
    border-radius: 16px;
    padding: 8px 20px;
    font-family: "JetBrains Mono", monospace;
    text-transform: uppercase;
    font-weight: 600;
    font-size: 12px;
    letter-spacing: 1.5px;
    min-width: 80px;
    min-height: 32px;
    box-shadow: 0 0 15px rgba(0, 255, 212, 0.3);
}

QPushButton[neon="true"]:hover {
    background: rgba(0, 255, 212, 0.1);
    color: #00FF41;
    border: 2px solid #00FF41;
    text-shadow: 0 0 8px rgba(0, 255, 65, 0.6);
    box-shadow: 0 0 25px rgba(0, 255, 65, 0.5),
                inset 0 0 15px rgba(0, 255, 65, 0.1);
}

/* Matrix Terminal Style */
QTextEdit[terminal="true"] {
    background: #000000;
    color: #00FF41;
    border: 2px solid #00FF41;
    font-family: "JetBrains Mono", "Fira Code", monospace;
    font-size: 12px;
    padding: 10px;
    /* Matrix-style corner brackets */
    border-image: linear-gradient(45deg, #00FF41 0%, transparent 10%, transparent 90%, #00FF41 100%) 2;
    box-shadow: 0 0 20px rgba(0, 255, 65, 0.3),
                inset 0 0 20px rgba(0, 255, 65, 0.05);
}

/* Cyber Warning Label */
QLabel[warning="true"] {
    color: #FF6B35;
    background: rgba(255, 107, 53, 0.1);
    border: 1px solid #FF6B35;
    padding: 8px;
    font-family: "JetBrains Mono", monospace;
    font-weight: 600;
    font-size: 12px;
    text-transform: uppercase;
    letter-spacing: 1px;
    box-shadow: 0 0 15px rgba(255, 107, 53, 0.3);
}

/* Cyber Error Label */
QLabel[error="true"] {
    color: #FF0044;
    background: rgba(255, 0, 68, 0.1);
    border: 1px solid #FF0044;
    padding: 8px;
    font-family: "JetBrains Mono", monospace;
    font-weight: 600;
    font-size: 12px;
    text-transform: uppercase;
    letter-spacing: 1px;
    box-shadow: 0 0 15px rgba(255, 0, 68, 0.3);
}

/* Scan Line Animation Background */
QWidget[scanlines="true"] {
    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
                                stop: 0 #0D1A1F,
                                stop: 0.5 #0A0E1A,
                                stop: 1 #0D1A1F);
}
)";

// Combine stylesheet parts
inline QString getFuturisticStylesheet() {
    return QString(FUTURISTIC_STYLESHEET_PART1) + QString(FUTURISTIC_STYLESHEET_PART2);
}

// ==================== Futuristic Theme Application ====================
class FuturisticThemeEngine : public QProxyStyle {
public:
    FuturisticThemeEngine();
    
    void polish(QWidget* widget) override;
    void unpolish(QWidget* widget) override;
    
    void drawPrimitive(PrimitiveElement element, const QStyleOption* option,
                      QPainter* painter, const QWidget* widget = nullptr) const override;
    
    void drawControl(ControlElement element, const QStyleOption* option,
                    QPainter* painter, const QWidget* widget = nullptr) const override;
    
    int pixelMetric(PixelMetric metric, const QStyleOption* option = nullptr,
                   const QWidget* widget = nullptr) const override;
    
    // Enable advanced effects
    void enableHolographicEffects(bool enable) { m_holographicEnabled = enable; }
    void enableParticleEffects(bool enable) { m_particleEnabled = enable; }
    void enableGlowEffects(bool enable) { m_glowEnabled = enable; }
    void enableScanlineEffect(bool enable) { m_scanlineEnabled = enable; }
    
private:
    void drawGlowEffect(QPainter* painter, const QRect& rect, const QColor& color) const;
    void drawHolographicBorder(QPainter* painter, const QRect& rect) const;
    void drawScanlines(QPainter* painter, const QRect& rect) const;
    void drawNeonText(QPainter* painter, const QRect& rect, const QString& text, const QColor& color) const;
    
    bool m_holographicEnabled = true;
    bool m_particleEnabled = true;
    bool m_glowEnabled = true;
    bool m_scanlineEnabled = false;
    
    mutable int m_animationFrame = 0;
    mutable QTimer* m_animationTimer = nullptr;
};

// ==================== Theme Manager ====================
class ThemeManager : public QObject {
    Q_OBJECT
    
public:
    static ThemeManager* instance();
    
    void applyTheme(QWidget* widget);
    void setThemeMode(const QString& mode); // "futuristic", "holographic", "matrix", "neon"
    
    void enableAnimations(bool enable);
    void setAnimationSpeed(int speed); // 1-10
    void setGlowIntensity(int intensity); // 1-10
    void setNeonColors(const QColor& primary, const QColor& secondary);
    
    // Special effects
    void pulseWidget(QWidget* widget, const QColor& color = QColor("#00D4FF"));
    void glowWidget(QWidget* widget, const QColor& color = QColor("#00D4FF"));
    void scanlineEffect(QWidget* widget);
    void holographicEffect(QWidget* widget);
    void matrixRainEffect(QWidget* widget);
    
signals:
    void themeChanged(const QString& theme);
    
private:
    ThemeManager();
    ~ThemeManager();
    
    static ThemeManager* m_instance;
    FuturisticThemeEngine* m_themeEngine;
    QString m_currentTheme;
    
    QHash<QWidget*, QList<QGraphicsEffect*>> m_widgetEffects;
    QHash<QWidget*, QAbstractAnimation*> m_widgetAnimations;
};

// ==================== Helper Functions ====================
inline void applyFuturisticTheme(QWidget* widget) {
    ThemeManager::instance()->applyTheme(widget);
}

inline void addGlowEffect(QWidget* widget, const QColor& color = QColor("#00D4FF"), int blurRadius = 20) {
    auto* glow = new QGraphicsDropShadowEffect(widget);
    glow->setBlurRadius(blurRadius);
    glow->setColor(color);
    glow->setOffset(0, 0);
    widget->setGraphicsEffect(glow);
}

inline void addPulseAnimation(QWidget* widget, int duration = 1000) {
    // Pulse animation disabled for professional look
    // Keep function signature for compatibility
    Q_UNUSED(widget);
    Q_UNUSED(duration);
}

inline void addHolographicShimmer(QWidget* widget) {
    // Shimmer animation disabled for professional look
    // Apply static colorize effect instead
    auto* effect = new QGraphicsColorizeEffect(widget);
    effect->setColor(QColor("#00D4FF"));
    effect->setStrength(0.1);  // Static, subtle tint
    widget->setGraphicsEffect(effect);
}

} // namespace FuturisticTheme
} // namespace ComponentsForest

#endif // FUTURISTIC_THEME_H