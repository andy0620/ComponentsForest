/**
 * @file material_theme.cpp
 * @brief Implementation of the Modern Material Design Theme System
 */

#include "material_theme.h"
#include <QPainter>
#include <QPainterPath>
#include <QApplication>
#include <QStyleFactory>
#include <QTimer>
#include <QDebug>
#include <cmath>

namespace ComponentsForest {
namespace MaterialTheme {

// Static instance
ThemeManager* ThemeManager::m_instance = nullptr;

// ==================== MaterialThemeEngine Implementation ====================
MaterialThemeEngine::MaterialThemeEngine() 
    : QProxyStyle(QStyleFactory::create("Fusion")) {
}

void MaterialThemeEngine::polish(QWidget* widget) {
    QProxyStyle::polish(widget);
    
    // Apply material design attributes
    widget->setAttribute(Qt::WA_StyledBackground, true);
    
    // Enable hover effects for interactive widgets
    widget->setAttribute(Qt::WA_Hover, true);
}

void MaterialThemeEngine::unpolish(QWidget* widget) {
    QProxyStyle::unpolish(widget);
}

void MaterialThemeEngine::drawPrimitive(PrimitiveElement element, const QStyleOption* option,
                                        QPainter* painter, const QWidget* widget) const {
    
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setRenderHint(QPainter::SmoothPixmapTransform, true);
    
    switch (element) {
        case PE_PanelButtonCommand: {
            // Material Design button with rounded corners
            QRect rect = option->rect;
            bool isHovered = option->state & State_MouseOver;
            bool isPressed = option->state & State_Sunken;
            
            drawMaterialButton(painter, rect, isHovered, isPressed);
            
            painter->restore();
            return;
        }
        
        case PE_FrameFocusRect: {
            // Material Design focus indicator
            if (option->state & State_HasFocus) {
                QRect rect = option->rect.adjusted(2, 2, -2, -2);
                QPen pen(QColor(74, 158, 255), 2);
                pen.setStyle(Qt::SolidLine);
                painter->setPen(pen);
                painter->setBrush(Qt::NoBrush);
                painter->drawRoundedRect(rect, 8, 8);
            }
            painter->restore();
            return;
        }
        
        case PE_IndicatorCheckBox: {
            // Material Design checkbox
            QRect rect = option->rect;
            int radius = 4;
            
            // Background
            QColor bgColor = option->state & State_MouseOver ? 
                QColor(53, 54, 63) : QColor(44, 45, 54);
            drawRoundedRect(painter, rect, radius, bgColor);
            
            // Border
            QColor borderColor = option->state & State_MouseOver ?
                QColor(74, 158, 255) : QColor(107, 114, 128);
            painter->setPen(QPen(borderColor, 2));
            painter->drawRoundedRect(rect, radius, radius);
            
            // Checked state
            if (option->state & State_On) {
                // Fill with primary color
                drawRoundedRect(painter, rect, radius, QColor(74, 158, 255));
                
                // Draw checkmark
                QPainterPath checkPath;
                int padding = rect.width() / 4;
                checkPath.moveTo(rect.left() + padding, rect.center().y());
                checkPath.lineTo(rect.left() + rect.width() * 0.4, rect.bottom() - padding);
                checkPath.lineTo(rect.right() - padding, rect.top() + padding);
                
                painter->setPen(QPen(Qt::white, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
                painter->drawPath(checkPath);
            }
            
            painter->restore();
            return;
        }
        
        case PE_IndicatorRadioButton: {
            // Material Design radio button
            QRect rect = option->rect;
            QPoint center = rect.center();
            int radius = rect.width() / 2 - 2;
            
            // Background
            QColor bgColor = option->state & State_MouseOver ? 
                QColor(53, 54, 63) : QColor(44, 45, 54);
            painter->setBrush(bgColor);
            
            // Border
            QColor borderColor = option->state & State_MouseOver ?
                QColor(74, 158, 255) : QColor(107, 114, 128);
            painter->setPen(QPen(borderColor, 2));
            painter->drawEllipse(center, radius, radius);
            
            // Checked state
            if (option->state & State_On) {
                // Outer circle with primary color
                painter->setBrush(QColor(74, 158, 255));
                painter->setPen(QPen(QColor(74, 158, 255), 2));
                painter->drawEllipse(center, radius, radius);
                
                // Inner circle
                painter->setBrush(Qt::white);
                painter->setPen(Qt::NoPen);
                painter->drawEllipse(center, radius/2, radius/2);
            }
            
            painter->restore();
            return;
        }
        
        case PE_PanelLineEdit: {
            // Material Design text field
            QRect rect = option->rect;
            
            // Background
            QColor bgColor = option->state & State_HasFocus ?
                QColor(53, 54, 63) : QColor(44, 45, 54);
            drawRoundedRect(painter, rect, 8, bgColor);
            
            // Border
            QColor borderColor = option->state & State_HasFocus ?
                QColor(74, 158, 255) : QColor(58, 59, 69);
            painter->setPen(QPen(borderColor, 2));
            painter->setBrush(Qt::NoBrush);
            painter->drawRoundedRect(rect, 8, 8);
            
            painter->restore();
            return;
        }
        
        default:
            break;
    }
    
    painter->restore();
    QProxyStyle::drawPrimitive(element, option, painter, widget);
}

void MaterialThemeEngine::drawControl(ControlElement element, const QStyleOption* option,
                                      QPainter* painter, const QWidget* widget) const {
    
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    
    switch (element) {
        case CE_PushButton: {
            // Draw material design button
            const QStyleOptionButton* buttonOption = qstyleoption_cast<const QStyleOptionButton*>(option);
            if (buttonOption) {
                // Draw button background
                drawPrimitive(PE_PanelButtonCommand, option, painter, widget);
                
                // Draw button text
                QRect textRect = buttonOption->rect;
                QString text = buttonOption->text;
                
                if (!text.isEmpty()) {
                    QColor textColor = Qt::white;
                    if (option->state & State_Enabled) {
                        if (option->state & State_MouseOver) {
                            textColor = Qt::white;
                        }
                    } else {
                        textColor = QColor(107, 114, 128);
                    }
                    
                    painter->setPen(textColor);
                    QFont font = widget ? widget->font() : QFont();
                    font.setWeight(QFont::DemiBold);
                    painter->setFont(font);
                    painter->drawText(textRect, Qt::AlignCenter, text);
                }
            }
            painter->restore();
            return;
        }
        
        case CE_ProgressBar: {
            // Draw material design progress bar
            const QStyleOptionProgressBar* progressOption = 
                qstyleoption_cast<const QStyleOptionProgressBar*>(option);
            if (progressOption) {
                QRect rect = progressOption->rect;
                
                // Background track
                drawRoundedRect(painter, rect, 4, QColor(44, 45, 54));
                
                // Progress fill
                if (progressOption->progress > 0 && progressOption->maximum > 0) {
                    int progressWidth = (rect.width() * progressOption->progress) / progressOption->maximum;
                    QRect progressRect(rect.left(), rect.top(), progressWidth, rect.height());
                    drawRoundedRect(painter, progressRect, 4, QColor(74, 158, 255));
                }
                
                // Text
                if (progressOption->textVisible) {
                    int percentage = (progressOption->progress * 100) / progressOption->maximum;
                    QString text = QString("%1%").arg(percentage);
                    painter->setPen(Qt::white);
                    painter->drawText(rect, Qt::AlignCenter, text);
                }
            }
            painter->restore();
            return;
        }
        
        case CE_TabBarTab: {
            // Draw material design tab
            const QStyleOptionTab* tabOption = qstyleoption_cast<const QStyleOptionTab*>(option);
            if (tabOption) {
                QRect rect = tabOption->rect;
                bool isSelected = tabOption->state & State_Selected;
                bool isHovered = tabOption->state & State_MouseOver;
                
                // Background
                if (isSelected) {
                    painter->fillRect(rect, QColor(37, 38, 46));
                    // Draw selection indicator
                    QRect indicatorRect(rect.left(), rect.bottom() - 3, rect.width(), 3);
                    painter->fillRect(indicatorRect, QColor(74, 158, 255));
                } else if (isHovered) {
                    painter->fillRect(rect, QColor(44, 45, 54));
                }
                
                // Text
                QColor textColor = isSelected ? QColor(74, 158, 255) : 
                                  isHovered ? Qt::white : QColor(179, 184, 196);
                painter->setPen(textColor);
                
                QFont font = widget ? widget->font() : QFont();
                font.setWeight(isSelected ? QFont::DemiBold : QFont::Normal);
                painter->setFont(font);
                
                painter->drawText(rect, Qt::AlignCenter, tabOption->text);
            }
            painter->restore();
            return;
        }
        
        default:
            break;
    }
    
    painter->restore();
    QProxyStyle::drawControl(element, option, painter, widget);
}

int MaterialThemeEngine::pixelMetric(PixelMetric metric, const QStyleOption* option,
                                     const QWidget* widget) const {
    switch (metric) {
        case PM_ButtonMargin:
            return 12;
        case PM_ButtonDefaultIndicator:
            return 0;
        case PM_MenuButtonIndicator:
            return 12;
        case PM_ButtonShiftHorizontal:
        case PM_ButtonShiftVertical:
            return 0;
        case PM_DefaultFrameWidth:
            return 2;
        case PM_SpinBoxFrameWidth:
            return 2;
        case PM_ComboBoxFrameWidth:
            return 2;
        case PM_ScrollBarExtent:
            return 12;
        case PM_ScrollBarSliderMin:
            return 40;
        case PM_SliderThickness:
            return 20;
        case PM_SliderControlThickness:
            return 20;
        case PM_SliderLength:
            return 20;
        case PM_DockWidgetTitleBarButtonMargin:
            return 4;
        case PM_DockWidgetTitleMargin:
            return 4;
        case PM_TabBarTabHSpace:
            return 24;
        case PM_TabBarTabVSpace:
            return 14;
        case PM_TabBarBaseHeight:
            return 3;
        case PM_ProgressBarChunkWidth:
            return 0; // Continuous progress bar
        default:
            return QProxyStyle::pixelMetric(metric, option, widget);
    }
}

void MaterialThemeEngine::drawMaterialShadow(QPainter* painter, const QRect& rect, int elevation) const {
    painter->save();
    
    // Material Design elevation shadows
    // Using multiple layers for realistic shadow
    int layers = std::min(elevation / 2, 3);
    
    for (int i = layers; i > 0; --i) {
        int offset = elevation / (i * 2);
        int blur = elevation * i;
        QColor shadowColor(0, 0, 0, 20 + (i * 10));
        
        QRect shadowRect = rect.adjusted(-blur/2, offset, blur/2, offset + blur);
        
        // Draw shadow gradient
        QLinearGradient gradient(shadowRect.topLeft(), shadowRect.bottomLeft());
        gradient.setColorAt(0, shadowColor);
        gradient.setColorAt(1, Qt::transparent);
        
        painter->setPen(Qt::NoPen);
        painter->setBrush(gradient);
        painter->drawRoundedRect(shadowRect, 12, 12);
    }
    
    painter->restore();
}

void MaterialThemeEngine::drawRoundedRect(QPainter* painter, const QRect& rect, 
                                          int radius, const QColor& color) const {
    painter->save();
    painter->setPen(Qt::NoPen);
    painter->setBrush(color);
    painter->drawRoundedRect(rect, radius, radius);
    painter->restore();
}

void MaterialThemeEngine::drawMaterialButton(QPainter* painter, const QRect& rect,
                                             bool isHovered, bool isPressed) const {
    // Draw button shadow (elevation)
    int elevation = isPressed ? 2 : (isHovered ? 8 : 4);
    drawMaterialShadow(painter, rect, elevation);
    
    // Button background
    QColor bgColor;
    if (isPressed) {
        bgColor = QColor(37, 38, 46);
    } else if (isHovered) {
        bgColor = QColor(53, 54, 63);
    } else {
        bgColor = QColor(44, 45, 54);
    }
    
    // Draw button with rounded corners
    drawRoundedRect(painter, rect, 12, bgColor);
    
    // Add subtle gradient overlay for depth
    if (!isPressed) {
        QLinearGradient gradient(rect.topLeft(), rect.bottomLeft());
        gradient.setColorAt(0, QColor(255, 255, 255, 5));
        gradient.setColorAt(1, QColor(0, 0, 0, 10));
        
        painter->save();
        painter->setPen(Qt::NoPen);
        painter->setBrush(gradient);
        painter->drawRoundedRect(rect, 12, 12);
        painter->restore();
    }
}

// ==================== ThemeManager Implementation ====================
ThemeManager::ThemeManager() : QObject() {
    m_themeEngine = new MaterialThemeEngine();
}

ThemeManager::~ThemeManager() {
    delete m_themeEngine;
}

ThemeManager* ThemeManager::instance() {
    if (!m_instance) {
        m_instance = new ThemeManager();
    }
    return m_instance;
}

void ThemeManager::applyTheme(QWidget* widget) {
    if (!widget) return;
    
    // Apply the material design stylesheet
    widget->setStyleSheet(getCompleteStylesheet());
    
    // Set the custom style engine
    QApplication::setStyle(m_themeEngine);
    
    // Apply default elevation to certain widget types
    if (widget->property("card").toBool()) {
        applyElevation(widget, 2);
    } else if (widget->property("fab").toBool()) {
        applyElevation(widget, 6);
    }
    
    // Set modern font
    QFont modernFont("Inter");
    if (!modernFont.exactMatch()) {
        modernFont = QFont("Segoe UI", 10);
        if (!modernFont.exactMatch()) {
            modernFont = QFont("Roboto", 10);
            if (!modernFont.exactMatch()) {
                modernFont = QFont("Arial", 10);
            }
        }
    }
    modernFont.setStyleHint(QFont::SansSerif);
    widget->setFont(modernFont);
}

void ThemeManager::applyElevation(QWidget* widget, int level) {
    if (!widget || level < 0 || level > 24) return;
    
    auto* shadow = new QGraphicsDropShadowEffect(widget);
    
    // Material Design elevation values
    // Each dp of elevation creates specific shadow values
    int blurRadius = level * 2;
    int yOffset = level / 2;
    int opacity = std::min(60, 30 + level);
    
    shadow->setBlurRadius(blurRadius);
    shadow->setColor(QColor(0, 0, 0, opacity));
    shadow->setOffset(0, yOffset);
    
    widget->setGraphicsEffect(shadow);
}

void ThemeManager::applyRippleEffect(QWidget* widget) {
    if (!widget) return;
    
    // Create a subtle hover effect instead of ripple for now
    // Full ripple implementation would require custom widget painting
    widget->setProperty("ripple", true);
    
    // Apply hover elevation change
    auto* shadow = new QGraphicsDropShadowEffect(widget);
    shadow->setBlurRadius(8);
    shadow->setColor(QColor(74, 158, 255, 30));
    shadow->setOffset(0, 2);
    widget->setGraphicsEffect(shadow);
}

} // namespace MaterialTheme
} // namespace ComponentsForest