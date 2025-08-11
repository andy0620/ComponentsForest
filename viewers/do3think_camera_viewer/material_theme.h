/**
 * @file material_theme.h
 * @brief Modern Material Design Theme System for Do3Think Camera Viewer
 * 
 * High-end Material Design 3 inspired interface with modern rounded components,
 * smooth shadows, and professional industrial aesthetics
 */

#ifndef MATERIAL_THEME_H
#define MATERIAL_THEME_H

#include <QWidget>
#include <QString>
#include <QColor>
#include <QPainter>
#include <QPainterPath>
#include <QPropertyAnimation>
#include <QGraphicsEffect>
#include <QGraphicsDropShadowEffect>
#include <QLinearGradient>
#include <QRadialGradient>
#include <QTimer>
#include <QEasingCurve>
#include <QStyle>
#include <QProxyStyle>
#include <QStyleOption>
#include <memory>

namespace ComponentsForest {
namespace MaterialTheme {

// ==================== Material Design 3 Color Palette ====================
namespace Colors {
    // Surface colors (dark theme)
    constexpr const char* SURFACE_0 = "#121318";          // Darkest surface
    constexpr const char* SURFACE_1 = "#1E1F26";          // Base surface
    constexpr const char* SURFACE_2 = "#25262E";          // Elevated surface
    constexpr const char* SURFACE_3 = "#2C2D36";          // More elevated
    constexpr const char* SURFACE_4 = "#35363F";          // Highest elevation
    
    // Primary colors (industrial blue)
    constexpr const char* PRIMARY = "#4A9EFF";            // Primary blue
    constexpr const char* PRIMARY_VARIANT = "#3D82E4";    // Darker primary
    constexpr const char* PRIMARY_LIGHT = "#7DB8FF";      // Light primary
    constexpr const char* PRIMARY_DARK = "#2968C8";       // Dark primary
    
    // Tech accent colors
    constexpr const char* TECH_CYAN = "#00FFD4";          // Tech cyan accent
    constexpr const char* TECH_TEAL = "#00E5CC";          // Tech teal
    constexpr const char* MATRIX_GREEN = "#00FF41";       // Matrix green for active
    constexpr const char* TECH_ORANGE = "#FF6B35";        // Tech warning orange
    
    // Secondary colors (accent teal)
    constexpr const char* SECONDARY = "#4DD0E1";          // Secondary teal
    constexpr const char* SECONDARY_VARIANT = "#00ACC1";  // Darker secondary
    constexpr const char* SECONDARY_LIGHT = "#88E4F0";    // Light secondary
    
    // State colors
    constexpr const char* SUCCESS = "#4CAF50";            // Success green
    constexpr const char* WARNING = "#FF9800";            // Warning orange
    constexpr const char* ERROR_COLOR = "#F44336";       // Error red
    constexpr const char* INFO = "#2196F3";               // Info blue
    
    // Text colors
    constexpr const char* TEXT_PRIMARY = "#FFFFFF";       // Primary text (87% opacity)
    constexpr const char* TEXT_SECONDARY = "#B3B8C4";     // Secondary text (60% opacity)
    constexpr const char* TEXT_DISABLED = "#6B7280";      // Disabled text (38% opacity)
    constexpr const char* TEXT_ON_PRIMARY = "#000000";    // Text on primary color
    
    // Border and divider
    constexpr const char* DIVIDER = "#2F3039";           // Divider color
    constexpr const char* BORDER = "#3A3B45";            // Border color
    constexpr const char* BORDER_FOCUS = "#4A9EFF";      // Focus border
    
    // Shadow colors
    inline QColor shadowColor() { return QColor(0, 0, 0, 60); }
    inline QColor shadowColorLight() { return QColor(0, 0, 0, 30); }
    inline QColor glowColor() { return QColor(74, 158, 255, 40); }
}

// ==================== Material Design Stylesheet ====================
// Split into parts to avoid MSVC string literal size limit
constexpr const char* MATERIAL_STYLESHEET_PART1A = R"(
/* ==================== Global Reset ==================== */
* {
    margin: 0;
    padding: 0;
    border: 0;
    outline: none;
}

/* ==================== Main Widget ==================== */
QWidget {
    background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 1,
                                stop: 0 #25262E,
                                stop: 0.3 #1E1F26,
                                stop: 0.7 #191A20,
                                stop: 1 #16171C);
    color: #FFFFFF;
    font-family: "JetBrains Mono", "Fira Code", "Source Code Pro", "Roboto Mono", "SF Mono", "Monaco", "Cascadia Code", "Consolas", monospace;
    font-size: 13px;
    font-weight: 400;
    selection-background-color: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0,
                                                stop: 0 #5AAAFF,
                                                stop: 0.5 #4A9EFF,
                                                stop: 1 #3D82E4);
    selection-color: #FFFFFF;
}

QMainWindow {
    background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 1,
                                stop: 0 #191A20,
                                stop: 0.25 #121318,
                                stop: 0.75 #0E0F14,
                                stop: 1 #121318);
    /* Subtle tech grid pattern with gradient overlay */
    background-image: repeating-linear-gradient(
        0deg,
        transparent,
        transparent 39px,
        rgba(0, 255, 212, 0.03) 39px,
        rgba(0, 255, 212, 0.03) 40px
    ),
    repeating-linear-gradient(
        90deg,
        transparent,
        transparent 39px,
        rgba(0, 255, 212, 0.03) 39px,
        rgba(0, 255, 212, 0.03) 40px
    );
}

/* ==================== Material Cards (Dock Widgets - Glass) ==================== */
QDockWidget {
    background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 1,
                                stop: 0 rgba(45, 46, 56, 0.9),
                                stop: 0.3 rgba(37, 38, 46, 0.92),
                                stop: 0.7 rgba(30, 31, 38, 0.95),
                                stop: 1 rgba(37, 38, 46, 0.98));
    border: 1px solid qlineargradient(x1: 0, y1: 0, x2: 1, y2: 1,
                                      stop: 0 rgba(255, 255, 255, 0.12),
                                      stop: 0.5 rgba(255, 255, 255, 0.08),
                                      stop: 1 rgba(255, 255, 255, 0.12));
    border-radius: 20px;
    margin: 6px;
    backdrop-filter: blur(12px);
    box-shadow: 0 12px 40px rgba(0, 0, 0, 0.15);
}

QDockWidget::title {
    background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0,
                                stop: 0 rgba(54, 55, 66, 0.95),
                                stop: 0.25 rgba(44, 45, 54, 0.98),
                                stop: 0.75 rgba(44, 45, 54, 0.98),
                                stop: 1 rgba(54, 55, 66, 0.95));
    text-align: left;
    padding: 16px 24px;
    border-top-left-radius: 20px;
    border-top-right-radius: 20px;
    border-bottom: 2px solid qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0,
                                             stop: 0 rgba(0, 255, 212, 0.1),
                                             stop: 0.5 rgba(0, 229, 204, 0.25),
                                             stop: 1 rgba(0, 255, 212, 0.1));
    font-family: "JetBrains Mono", "Fira Code", monospace;
    font-weight: 600;
    font-size: 14px;
    color: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0,
                          stop: 0 #FFFFFF,
                          stop: 0.5 #F0F4F8,
                          stop: 1 #FFFFFF);
    /* Tech corner brackets with gradient */
    border-left: 2px solid qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
                                           stop: 0 rgba(0, 255, 212, 0.6),
                                           stop: 1 rgba(0, 255, 212, 0.2));
}

/* ==================== Material Menu Bar ==================== */
QMenuBar {
    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
                                stop: 0 #25262E,
                                stop: 0.5 #1E1F26,
                                stop: 1 #191A20);
    border: none;
    padding: 8px 12px;
    spacing: 8px;
}

QMenuBar::item {
    background: transparent;
    color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
                          stop: 0 #C8CDD8,
                          stop: 1 #B3B8C4);
    padding: 8px 14px;
    margin: 0 4px;
    border-radius: 8px;
    font-family: "JetBrains Mono", monospace;
    font-weight: 500;
    font-size: 12px;
}

QMenuBar::item:selected,
QMenuBar::item:hover {
    background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 1,
                                stop: 0 #35363F,
                                stop: 0.5 #2C2D36,
                                stop: 1 #35363F);
    color: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0,
                          stop: 0 #00FFD4,
                          stop: 0.5 #FFFFFF,
                          stop: 1 #4A9EFF);
}

QMenuBar::item:pressed {
    background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 1,
                                stop: 0 #3D3E48,
                                stop: 0.5 #35363F,
                                stop: 1 #3D3E48);
    color: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0,
                          stop: 0 #5AAAFF,
                          stop: 0.5 #4A9EFF,
                          stop: 1 #3D82E4);
}

/* ==================== Material Menus ==================== */
QMenu {
    background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 1,
                                stop: 0 #2C2D36,
                                stop: 0.5 #25262E,
                                stop: 1 #2C2D36);
    border: 1px solid qlineargradient(x1: 0, y1: 0, x2: 1, y2: 1,
                                      stop: 0 #4A4B55,
                                      stop: 0.5 #3A3B45,
                                      stop: 1 #4A4B55);
    border-radius: 8px;
    padding: 8px 0;
}

QMenu::item {
    background: transparent;
    color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
                          stop: 0 #FFFFFF,
                          stop: 1 #E8EAF0);
    padding: 12px 24px 12px 16px;
    margin: 0 8px;
    border-radius: 6px;
}

QMenu::item:selected {
    background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0,
                                stop: 0 #3D3E48,
                                stop: 0.5 #35363F,
                                stop: 1 #3D3E48);
    color: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0,
                          stop: 0 #5AAAFF,
                          stop: 0.5 #4A9EFF,
                          stop: 1 #3D82E4);
}

QMenu::separator {
    height: 1px;
    background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0,
                                stop: 0 transparent,
                                stop: 0.1 #2F3039,
                                stop: 0.9 #2F3039,
                                stop: 1 transparent);
    margin: 8px 16px;
}

/* ==================== Material Tool Bar (Ultra-Modern) ==================== */
QToolBar {
    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
                                stop: 0 rgba(37, 38, 46, 0.98),
                                stop: 1 rgba(30, 31, 38, 0.98));
    border: none;
    padding: 12px;
    spacing: 12px;
    border-bottom: 1px solid rgba(255, 255, 255, 0.05);
}

QToolButton {
    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
                                stop: 0 rgba(44, 45, 54, 0.9),
                                stop: 1 rgba(37, 38, 46, 0.9));
    color: #B3B8C4;
    border: 1px solid rgba(0, 229, 204, 0.15);
    border-radius: 18px;
    padding: 8px;
    margin: 3px;
    font-family: "JetBrains Mono", monospace;
    font-weight: 500;
    font-size: 12px;
    min-width: 36px;
    min-height: 36px;
    box-shadow: 0 2px 8px rgba(0, 0, 0, 0.1),
                inset 0 1px 0 rgba(255, 255, 255, 0.03);
}

QToolButton:hover {
    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
                                stop: 0 #35363F, stop: 1 #2C2D36);
    color: #00FFD4;
    border: 1px solid rgba(0, 255, 212, 0.5);
    box-shadow: 0 0 8px rgba(0, 255, 212, 0.2);
}

QToolButton:pressed,
QToolButton:checked {
    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
                                stop: 0 #3D3E48, stop: 1 #35363F);
    color: #00FF41;
    border: 1px solid rgba(0, 255, 65, 0.5);
    box-shadow: 0 0 12px rgba(0, 255, 65, 0.3);
}

/* ==================== Material Tab Widget (Ultra-Modern) ==================== */
QTabWidget::pane {
    background: rgba(37, 38, 46, 0.95);
    border: 1px solid rgba(255, 255, 255, 0.08);
    border-radius: 20px;
    margin-top: -1px;
    backdrop-filter: blur(10px);
}

QTabBar::tab {
    background: transparent;
    color: #B3B8C4;
    padding: 14px 28px;
    margin-right: 8px;
    border: none;
    border-top-left-radius: 16px;
    border-top-right-radius: 16px;
    font-weight: 500;
    min-width: 100px;
}

QTabBar::tab:selected {
    background: #25262E;
    color: #4A9EFF;
    border-bottom: 3px solid #4A9EFF;
}

QTabBar::tab:hover:!selected {
    background: #2C2D36;
    color: #FFFFFF;
}

/* ==================== Material Buttons (Tech-Styled Compact) ==================== */
QPushButton {
    background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 1,
                                stop: 0 rgba(63, 64, 76, 0.95),
                                stop: 0.25 rgba(53, 54, 63, 0.97),
                                stop: 0.5 rgba(44, 45, 54, 0.98),
                                stop: 0.75 rgba(44, 45, 54, 0.98),
                                stop: 1 rgba(37, 38, 46, 0.95));
    color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
                          stop: 0 #FFFFFF,
                          stop: 1 #F0F4F8);
    border: 1px solid qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0,
                                      stop: 0 rgba(0, 255, 212, 0.15),
                                      stop: 0.5 rgba(0, 229, 204, 0.35),
                                      stop: 1 rgba(0, 255, 212, 0.15));
    border-radius: 16px;
    padding: 8px 16px;
    font-family: "JetBrains Mono", "Fira Code", monospace;
    font-weight: 500;
    font-size: 13px;
    min-width: 80px;
    min-height: 32px;
    text-transform: none;
    letter-spacing: 0.5px;
    box-shadow: 0 4px 12px rgba(0, 0, 0, 0.15),
                0 2px 4px rgba(0, 0, 0, 0.1),
                inset 0 1px 0 rgba(255, 255, 255, 0.05);
}

QPushButton:hover {
    background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 1,
                                stop: 0 rgba(71, 72, 84, 0.98),
                                stop: 0.25 rgba(61, 62, 72, 0.99),
                                stop: 0.5 rgba(53, 54, 63, 1.0),
                                stop: 0.75 rgba(53, 54, 63, 1.0),
                                stop: 1 rgba(45, 46, 55, 0.98));
    border: 1px solid qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0,
                                      stop: 0 rgba(0, 255, 212, 0.4),
                                      stop: 0.5 rgba(0, 255, 212, 0.8),
                                      stop: 1 rgba(0, 255, 212, 0.4));
    color: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0,
                          stop: 0 #00FFD4,
                          stop: 0.5 #00E5CC,
                          stop: 1 #00FFD4);
    box-shadow: 0 0 15px rgba(0, 255, 212, 0.3),
                0 3px 8px rgba(0, 0, 0, 0.15),
                inset 0 1px 0 rgba(255, 255, 255, 0.08);
    transform: translateY(-1px);
}

QPushButton:pressed {
    background: qlineargradient(x1: 0, y1: 1, x2: 1, y2: 0,
                                stop: 0 rgba(44, 45, 54, 1.0),
                                stop: 0.5 rgba(37, 38, 46, 1.0),
                                stop: 1 rgba(44, 45, 54, 1.0));
    border: 1px solid qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0,
                                      stop: 0 rgba(0, 255, 65, 0.5),
                                      stop: 0.5 rgba(0, 255, 65, 0.9),
                                      stop: 1 rgba(0, 255, 65, 0.5));
    color: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0,
                          stop: 0 #00FF41,
                          stop: 0.5 #00FF88,
                          stop: 1 #00FF41);
    box-shadow: 0 0 10px rgba(0, 255, 65, 0.4),
                inset 0 1px 3px rgba(0, 0, 0, 0.3);
    transform: translateY(0px);
}

QPushButton:disabled {
    background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 1,
                                stop: 0 #252630,
                                stop: 0.5 #1E1F26,
                                stop: 1 #191A20);
    color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
                          stop: 0 #7B8290,
                          stop: 1 #6B7280);
    border: 1px solid qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0,
                                      stop: 0 rgba(111, 114, 128, 0.1),
                                      stop: 0.5 rgba(111, 114, 128, 0.3),
                                      stop: 1 rgba(111, 114, 128, 0.1));
}

/* Primary Material Button (Tech-Modern Compact) */
QPushButton[primary="true"] {
    background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 1,
                                stop: 0 #5AAAFF,
                                stop: 0.25 #4A9EFF,
                                stop: 0.5 #3D82E4,
                                stop: 0.75 #2968C8,
                                stop: 1 #4A9EFF);
    color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
                          stop: 0 #FFFFFF,
                          stop: 0.5 #F0F8FF,
                          stop: 1 #FFFFFF);
    font-family: "JetBrains Mono", monospace;
    font-weight: 600;
    font-size: 13px;
    border: 1px solid qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0,
                                      stop: 0 rgba(255, 255, 255, 0.15),
                                      stop: 0.5 rgba(255, 255, 255, 0.3),
                                      stop: 1 rgba(255, 255, 255, 0.15));
    border-radius: 18px;
    padding: 10px 20px;
    text-shadow: 0 1px 2px rgba(0, 0, 0, 0.15);
    box-shadow: 0 0 20px rgba(74, 158, 255, 0.4),
                0 4px 8px rgba(0, 0, 0, 0.1),
                inset 0 1px 0 rgba(255, 255, 255, 0.3),
                inset 0 -1px 0 rgba(0, 0, 0, 0.1);
    min-width: 90px;
    min-height: 36px;
}

QPushButton[primary="true"]:hover {
    background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 1,
                                stop: 0 #00FFD4,
                                stop: 0.25 #00E5CC,
                                stop: 0.5 #00D4BB,
                                stop: 0.75 #00C4AA,
                                stop: 1 #00FFD4);
    color: #0A0E1A;
    border: 1px solid qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0,
                                      stop: 0 rgba(0, 255, 212, 0.3),
                                      stop: 0.5 rgba(0, 255, 212, 0.6),
                                      stop: 1 rgba(0, 255, 212, 0.3));
    box-shadow: 0 0 25px rgba(0, 255, 212, 0.5),
                0 4px 12px rgba(0, 0, 0, 0.2),
                inset 0 1px 0 rgba(255, 255, 255, 0.4);
}

QPushButton[primary="true"]:pressed {
    background: qlineargradient(x1: 0, y1: 1, x2: 1, y2: 0,
                                stop: 0 #2968C8,
                                stop: 0.25 #3D82E4,
                                stop: 0.5 #4A9EFF,
                                stop: 0.75 #3D82E4,
                                stop: 1 #2968C8);
    padding: 11px 20px 9px 20px;
    box-shadow: inset 0 2px 4px rgba(0, 0, 0, 0.2),
                inset 0 -1px 0 rgba(255, 255, 255, 0.1);
}

QPushButton[primary="true"]:disabled {
    background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 1,
                                stop: 0 #4B5563,
                                stop: 0.5 #3A5A7B,
                                stop: 1 #2C4A6B);
    color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
                          stop: 0 #7B8290,
                          stop: 1 #6B7280);
}
)";

constexpr const char* MATERIAL_STYLESHEET_PART1B = R"(
/* Secondary Material Button (Tech Glass) */
QPushButton[secondary="true"] {
    background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 1,
                                stop: 0 rgba(0, 255, 212, 0.12),
                                stop: 0.3 rgba(0, 229, 204, 0.08),
                                stop: 0.7 rgba(0, 229, 204, 0.08),
                                stop: 1 rgba(0, 255, 212, 0.12));
    color: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0,
                          stop: 0 #00FFD4,
                          stop: 0.5 #00E5CC,
                          stop: 1 #00FFD4);
    border: 1.5px solid qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0,
                                        stop: 0 rgba(0, 255, 212, 0.3),
                                        stop: 0.5 rgba(0, 255, 212, 0.7),
                                        stop: 1 rgba(0, 255, 212, 0.3));
    border-radius: 16px;
    padding: 8px 16px;
    font-family: "JetBrains Mono", monospace;
    font-weight: 500;
    font-size: 12px;
    backdrop-filter: blur(10px);
    box-shadow: 0 4px 12px rgba(0, 0, 0, 0.05),
                inset 0 1px 0 rgba(255, 255, 255, 0.05);
    min-width: 80px;
    min-height: 32px;
}

QPushButton[secondary="true"]:hover {
    background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 1,
                                stop: 0 rgba(0, 255, 212, 0.2),
                                stop: 0.5 rgba(0, 255, 212, 0.15),
                                stop: 1 rgba(0, 255, 212, 0.2));
    border: 2px solid qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0,
                                      stop: 0 #00FFD4,
                                      stop: 0.5 #00E5CC,
                                      stop: 1 #00FFD4);
    color: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0,
                          stop: 0 #00FFD4,
                          stop: 0.5 #FFFFFF,
                          stop: 1 #00FFD4);
    box-shadow: 0 0 15px rgba(0, 255, 212, 0.3);
}

QPushButton[secondary="true"]:pressed {
    background: qlineargradient(x1: 0, y1: 1, x2: 1, y2: 0,
                                stop: 0 rgba(0, 255, 65, 0.3),
                                stop: 0.5 rgba(0, 255, 65, 0.25),
                                stop: 1 rgba(0, 255, 65, 0.3));
    border: 2px solid qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0,
                                      stop: 0 #00FF41,
                                      stop: 0.5 #00FF88,
                                      stop: 1 #00FF41);
    color: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0,
                          stop: 0 #00FF41,
                          stop: 0.5 #00FF88,
                          stop: 1 #00FF41);
    padding: 9px 16px 7px 16px;
}

/* ==================== Material Cards (Group Boxes - Glass) ==================== */
QGroupBox {
    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
                                stop: 0 rgba(37, 38, 46, 0.85),
                                stop: 1 rgba(37, 38, 46, 0.95));
    border: 1px solid rgba(0, 229, 204, 0.15);
    border-radius: 20px;
    margin-top: 20px;
    padding-top: 20px;
    font-family: "JetBrains Mono", monospace;
    font-weight: 600;
    font-size: 13px;
    backdrop-filter: blur(10px);
    box-shadow: 0 8px 32px rgba(0, 0, 0, 0.1),
                inset 0 1px 0 rgba(255, 255, 255, 0.05);
    /* Tech corner accents */
    border-top-left-radius: 4px;
    border-bottom-right-radius: 4px;
}

QGroupBox::title {
    subcontrol-origin: margin;
    subcontrol-position: top left;
    color: #00FFD4;
    padding: 0 12px;
    left: 20px;
    font-family: "JetBrains Mono", monospace;
    font-size: 12px;
    font-weight: 600;
    text-transform: uppercase;
    letter-spacing: 1px;
}

/* ==================== Material Sliders (Ultra-Modern) ==================== */
QSlider {
    min-height: 48px;
}

QSlider::groove:horizontal {
    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
                                stop: 0 rgba(44, 45, 54, 0.6),
                                stop: 1 rgba(44, 45, 54, 0.8));
    height: 8px;
    border-radius: 4px;
    border: 1px solid rgba(255, 255, 255, 0.05);
}

QSlider::handle:horizontal {
    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
                                stop: 0 #7DB8FF,
                                stop: 1 #4A9EFF);
    width: 24px;
    height: 24px;
    margin: -8px 0;
    border-radius: 12px;
    border: 2px solid rgba(255, 255, 255, 0.3);
    box-shadow: 0 2px 8px rgba(74, 158, 255, 0.4),
                0 1px 3px rgba(0, 0, 0, 0.2);
}

QSlider::handle:horizontal:hover {
    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
                                stop: 0 #8FC4FF,
                                stop: 1 #5AAAFF);
    width: 28px;
    height: 28px;
    margin: -10px 0;
    border-radius: 14px;
    box-shadow: 0 4px 12px rgba(74, 158, 255, 0.5),
                0 2px 4px rgba(0, 0, 0, 0.2);
}

QSlider::sub-page:horizontal {
    background: #4A9EFF;
    border-radius: 2px;
}

/* ==================== Material Input Fields (Tech Glass) ==================== */
QLineEdit, QTextEdit, QPlainTextEdit {
    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
                                stop: 0 rgba(53, 54, 63, 0.6),
                                stop: 1 rgba(44, 45, 54, 0.7));
    color: #FFFFFF;
    border: 1px solid rgba(0, 229, 204, 0.2);
    border-radius: 12px;
    padding: 10px 14px;
    font-family: "JetBrains Mono", "Fira Code", monospace;
    font-size: 13px;
    backdrop-filter: blur(8px);
    box-shadow: 0 4px 16px rgba(0, 0, 0, 0.08),
                inset 0 1px 0 rgba(255, 255, 255, 0.04);
    /* Tech corner bracket effect */
    border-top-left-radius: 2px;
    border-bottom-right-radius: 2px;
}

QLineEdit:hover, QTextEdit:hover, QPlainTextEdit:hover {
    border: 1px solid rgba(74, 158, 255, 0.5);
    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
                                stop: 0 rgba(53, 54, 63, 0.75),
                                stop: 1 rgba(44, 45, 54, 0.85));
    box-shadow: 0 6px 20px rgba(74, 158, 255, 0.15),
                inset 0 1px 0 rgba(255, 255, 255, 0.06);
}
)";

constexpr const char* MATERIAL_STYLESHEET_PART2 = R"(
QLineEdit:focus, QTextEdit:focus, QPlainTextEdit:focus {
    border: 1.5px solid rgba(0, 255, 65, 0.8);
    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
                                stop: 0 rgba(53, 54, 63, 0.85),
                                stop: 1 rgba(44, 45, 54, 0.95));
    color: #00FF41;
    box-shadow: 0 0 20px rgba(0, 255, 65, 0.3),
                0 0 0 3px rgba(0, 255, 65, 0.1),
                inset 0 1px 0 rgba(255, 255, 255, 0.08);
}

QLineEdit:disabled, QTextEdit:disabled, QPlainTextEdit:disabled {
    background: #1E1F26;
    color: #6B7280;
    border: 2px solid #2F3039;
}

/* ==================== Material Spin Boxes (Ultra-Modern) ==================== */
QSpinBox, QDoubleSpinBox {
    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
                                stop: 0 rgba(53, 54, 63, 0.7),
                                stop: 1 rgba(44, 45, 54, 0.8));
    color: #FFFFFF;
    border: 1px solid rgba(255, 255, 255, 0.12);
    border-radius: 20px;
    padding: 14px 16px;
    font-size: 14px;
    min-height: 48px;
    backdrop-filter: blur(6px);
    box-shadow: 0 4px 12px rgba(0, 0, 0, 0.08),
                inset 0 1px 0 rgba(255, 255, 255, 0.04);
}

QSpinBox:hover, QDoubleSpinBox:hover {
    border: 2px solid #4A9EFF;
    background: #35363F;
}

QSpinBox:focus, QDoubleSpinBox:focus {
    border: 2px solid #4A9EFF;
    background: #35363F;
}

QSpinBox::up-button, QDoubleSpinBox::up-button,
QSpinBox::down-button, QDoubleSpinBox::down-button {
    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
                                stop: 0 rgba(53, 54, 63, 0.9),
                                stop: 1 rgba(44, 45, 54, 0.9));
    border: 1px solid rgba(255, 255, 255, 0.08);
    width: 24px;
    border-radius: 10px;
    margin: 3px;
}

QSpinBox::up-button:hover, QDoubleSpinBox::up-button:hover,
QSpinBox::down-button:hover, QDoubleSpinBox::down-button:hover {
    background: #4A9EFF;
}

/* ==================== Material Combo Boxes (Ultra-Modern) ==================== */
QComboBox {
    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
                                stop: 0 rgba(53, 54, 63, 0.7),
                                stop: 1 rgba(44, 45, 54, 0.8));
    color: #FFFFFF;
    border: 1px solid rgba(255, 255, 255, 0.12);
    border-radius: 20px;
    padding: 14px 18px;
    min-width: 140px;
    min-height: 48px;
    font-size: 14px;
    backdrop-filter: blur(6px);
    box-shadow: 0 4px 12px rgba(0, 0, 0, 0.08),
                inset 0 1px 0 rgba(255, 255, 255, 0.04);
}

QComboBox:hover {
    border: 1px solid rgba(74, 158, 255, 0.5);
    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
                                stop: 0 rgba(53, 54, 63, 0.85),
                                stop: 1 rgba(44, 45, 54, 0.95));
    box-shadow: 0 6px 20px rgba(74, 158, 255, 0.15),
                inset 0 1px 0 rgba(255, 255, 255, 0.06);
}

QComboBox:focus {
    border: 2px solid #4A9EFF;
    background: #35363F;
}

QComboBox::drop-down {
    border: none;
    width: 32px;
    background: transparent;
    border-left: 1px solid rgba(255, 255, 255, 0.08);
    border-top-right-radius: 20px;
    border-bottom-right-radius: 20px;
}

QComboBox::down-arrow {
    image: none;
    border-left: 5px solid transparent;
    border-right: 5px solid transparent;
    border-top: 5px solid #B3B8C4;
    width: 0;
    height: 0;
    margin-right: 10px;
}

QComboBox:hover::down-arrow {
    border-top: 5px solid #4A9EFF;
}

QComboBox QAbstractItemView {
    background: rgba(37, 38, 46, 0.98);
    border: 1px solid rgba(255, 255, 255, 0.12);
    border-radius: 16px;
    selection-background-color: rgba(74, 158, 255, 0.2);
    color: #FFFFFF;
    outline: none;
    padding: 8px;
    backdrop-filter: blur(10px);
    box-shadow: 0 8px 32px rgba(0, 0, 0, 0.2);
}

/* ==================== Material Check Boxes & Radio Buttons (Modern) ==================== */
QCheckBox, QRadioButton {
    spacing: 12px;
    color: #FFFFFF;
    font-weight: 500;
}

QCheckBox::indicator, QRadioButton::indicator {
    width: 22px;
    height: 22px;
    border: 2px solid rgba(107, 114, 128, 0.6);
    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
                                stop: 0 rgba(44, 45, 54, 0.7),
                                stop: 1 rgba(44, 45, 54, 0.9));
    box-shadow: 0 2px 4px rgba(0, 0, 0, 0.1),
                inset 0 1px 0 rgba(255, 255, 255, 0.03);
}

QCheckBox::indicator {
    border-radius: 8px;
}

QRadioButton::indicator {
    border-radius: 11px;
}

QCheckBox::indicator:hover, QRadioButton::indicator:hover {
    border: 2px solid rgba(74, 158, 255, 0.7);
    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
                                stop: 0 rgba(53, 54, 63, 0.8),
                                stop: 1 rgba(53, 54, 63, 1.0));
    box-shadow: 0 3px 8px rgba(74, 158, 255, 0.2),
                inset 0 1px 0 rgba(255, 255, 255, 0.05);
}

QCheckBox::indicator:checked {
    background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 1,
                                stop: 0 #00FF41,
                                stop: 1 #00E5CC);
    border: 2px solid rgba(0, 255, 65, 0.8);
    image: url(:/icons/check.svg);
    box-shadow: 0 0 10px rgba(0, 255, 65, 0.4),
                inset 0 1px 0 rgba(255, 255, 255, 0.2);
}

QRadioButton::indicator:checked {
    background: #4A9EFF;
    border: 2px solid #4A9EFF;
}

QRadioButton::indicator:checked::after {
    content: "";
    width: 8px;
    height: 8px;
    border-radius: 4px;
    background: #FFFFFF;
    position: absolute;
    top: 6px;
    left: 6px;
}

/* ==================== Material Progress Bars (Modern) ==================== */
QProgressBar {
    background: rgba(44, 45, 54, 0.5);
    border: 1px solid rgba(255, 255, 255, 0.05);
    border-radius: 12px;
    text-align: center;
    color: #FFFFFF;
    font-weight: 600;
    height: 10px;
    box-shadow: inset 0 1px 3px rgba(0, 0, 0, 0.2);
}

QProgressBar::chunk {
    background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0,
                                stop: 0 #7DB8FF,
                                stop: 0.5 #4A9EFF,
                                stop: 1 #3D82E4);
    border-radius: 11px;
    box-shadow: 0 2px 4px rgba(74, 158, 255, 0.3);
}

/* ==================== Material List Widgets ==================== */
QListWidget, QListView {
    background: #25262E;
    border: none;
    border-radius: 8px;
    padding: 8px;
    outline: none;
}

QListWidget::item, QListView::item {
    background: transparent;
    color: #FFFFFF;
    padding: 12px;
    border-radius: 6px;
    margin: 2px 0;
}

QListWidget::item:hover, QListView::item:hover {
    background: #2C2D36;
}

QListWidget::item:selected, QListView::item:selected {
    background: #35363F;
    color: #4A9EFF;
}

/* ==================== Material Tree Widget ==================== */
QTreeWidget, QTreeView {
    background: #25262E;
    border: none;
    border-radius: 8px;
    padding: 8px;
    outline: none;
}
)";

constexpr const char* MATERIAL_STYLESHEET_PART3 = R"(
QTreeWidget::item, QTreeView::item {
    padding: 8px;
    border-radius: 4px;
    color: #FFFFFF;
}

QTreeWidget::item:hover, QTreeView::item:hover {
    background: #2C2D36;
}

QTreeWidget::item:selected, QTreeView::item:selected {
    background: #35363F;
    color: #4A9EFF;
}

/* ==================== Material Scroll Bars (Ultra-Smooth) ==================== */
QScrollBar:vertical, QScrollBar:horizontal {
    background: rgba(30, 31, 38, 0.3);
    width: 14px;
    height: 14px;
    margin: 0;
    border-radius: 7px;
}

QScrollBar::handle:vertical, QScrollBar::handle:horizontal {
    background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0,
                                stop: 0 rgba(58, 59, 69, 0.7),
                                stop: 1 rgba(58, 59, 69, 0.9));
    border-radius: 6px;
    min-height: 40px;
    min-width: 40px;
    border: 1px solid rgba(255, 255, 255, 0.05);
    margin: 1px;
}

QScrollBar::handle:vertical:hover, QScrollBar::handle:horizontal:hover {
    background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0,
                                stop: 0 rgba(74, 158, 255, 0.6),
                                stop: 1 rgba(74, 158, 255, 0.8));
    border: 1px solid rgba(74, 158, 255, 0.3);
}

QScrollBar::add-line, QScrollBar::sub-line {
    border: none;
    background: none;
    height: 0;
    width: 0;
}

QScrollBar::add-page, QScrollBar::sub-page {
    background: none;
}

/* ==================== Material Status Bar ==================== */
QStatusBar {
    background: #1E1F26;
    border-top: 1px solid #2F3039;
    padding: 8px;
    min-height: 32px;
}

QStatusBar::item {
    border: none;
}

QStatusBar QLabel {
    color: #B3B8C4;
    padding: 0 12px;
    font-size: 13px;
}

/* ==================== Material Tool Tips ==================== */
QToolTip {
    background: #35363F;
    color: #FFFFFF;
    border: 1px solid #3A3B45;
    border-radius: 6px;
    padding: 8px 12px;
    font-size: 13px;
}

/* ==================== Material Special Elements ==================== */
/* Elevated Card */
QWidget[card="true"] {
    background: #25262E;
    border-radius: 12px;
    padding: 20px;
}

/* Floating Action Button (Tech-Modern) */
QPushButton[fab="true"] {
    background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 1,
                                stop: 0 #00FFD4,
                                stop: 1 #00E5CC);
    color: #0A0E1A;
    border-radius: 28px;
    padding: 0;
    width: 56px;
    height: 56px;
    font-family: "JetBrains Mono", monospace;
    font-size: 20px;
    font-weight: 600;
    border: 2px solid rgba(255, 255, 255, 0.2);
    box-shadow: 0 0 25px rgba(0, 255, 212, 0.5),
                0 4px 8px rgba(0, 0, 0, 0.15),
                inset 0 1px 0 rgba(255, 255, 255, 0.3);
}

QPushButton[fab="true"]:hover {
    background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 1,
                                stop: 0 #8FC4FF,
                                stop: 1 #5AAAFF);
    transform: scale(1.05);
    box-shadow: 0 12px 32px rgba(74, 158, 255, 0.5),
                0 6px 12px rgba(0, 0, 0, 0.15);
}

QPushButton[fab="true"]:pressed {
    background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 1,
                                stop: 0 #3D82E4,
                                stop: 1 #2968C8);
    transform: scale(0.98);
}

/* Material Chip */
QWidget[chip="true"] {
    background: #2C2D36;
    border-radius: 16px;
    padding: 6px 12px;
    color: #FFFFFF;
    font-size: 13px;
}

/* Success/Warning/Error States (Tech Colors) */
QWidget[state="success"] {
    background: #00FF41;
    color: #0A0E1A;
    border: 1px solid rgba(0, 255, 65, 0.8);
    box-shadow: 0 0 15px rgba(0, 255, 65, 0.4);
}

QWidget[state="warning"] {
    background: #FF6B35;
    color: #FFFFFF;
    border: 1px solid rgba(255, 107, 53, 0.8);
    box-shadow: 0 0 15px rgba(255, 107, 53, 0.4);
}

QWidget[state="error"] {
    background: #F44336;
    color: #FFFFFF;
    border: 1px solid rgba(244, 67, 54, 0.8);
    box-shadow: 0 0 15px rgba(244, 67, 54, 0.4);
}

/* Material Divider */
QFrame[divider="true"] {
    background: #2F3039;
    max-height: 1px;
    min-height: 1px;
}
)";

// Helper function to get the complete stylesheet
inline QString getCompleteStylesheet() {
    return QString(MATERIAL_STYLESHEET_PART1A) + 
           QString(MATERIAL_STYLESHEET_PART1B) + 
           QString(MATERIAL_STYLESHEET_PART2) + 
           QString(MATERIAL_STYLESHEET_PART3);
}

// ==================== Material Theme Engine ====================
class MaterialThemeEngine : public QProxyStyle {
public:
    MaterialThemeEngine();
    
    void polish(QWidget* widget) override;
    void unpolish(QWidget* widget) override;
    
    void drawPrimitive(PrimitiveElement element, const QStyleOption* option,
                      QPainter* painter, const QWidget* widget = nullptr) const override;
    
    void drawControl(ControlElement element, const QStyleOption* option,
                    QPainter* painter, const QWidget* widget = nullptr) const override;
    
    int pixelMetric(PixelMetric metric, const QStyleOption* option = nullptr,
                   const QWidget* widget = nullptr) const override;
    
private:
    void drawMaterialShadow(QPainter* painter, const QRect& rect, int elevation) const;
    void drawRoundedRect(QPainter* painter, const QRect& rect, int radius, const QColor& color) const;
    void drawMaterialButton(QPainter* painter, const QRect& rect, bool isHovered, bool isPressed) const;
};

// ==================== Theme Manager ====================
class ThemeManager : public QObject {
    Q_OBJECT
    
public:
    static ThemeManager* instance();
    
    void applyTheme(QWidget* widget);
    void applyElevation(QWidget* widget, int level); // 0-24 dp elevation levels
    void applyRippleEffect(QWidget* widget);
    
signals:
    void themeChanged();
    
private:
    ThemeManager();
    ~ThemeManager();
    
    static ThemeManager* m_instance;
    MaterialThemeEngine* m_themeEngine;
};

// ==================== Helper Functions ====================
inline void applyMaterialTheme(QWidget* widget) {
    ThemeManager::instance()->applyTheme(widget);
}

inline void addElevation(QWidget* widget, int level = 4) {
    auto* shadow = new QGraphicsDropShadowEffect(widget);
    shadow->setBlurRadius(level * 2);
    shadow->setColor(Colors::shadowColor());
    shadow->setOffset(0, level / 2);
    widget->setGraphicsEffect(shadow);
}

inline void addMaterialCard(QWidget* widget) {
    widget->setProperty("card", true);
    addElevation(widget, 2);
}

} // namespace MaterialTheme
} // namespace ComponentsForest

#endif // MATERIAL_THEME_H