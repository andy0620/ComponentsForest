/**
 * @file futuristic_theme.cpp
 * @brief Implementation of the Futuristic Cyberpunk Theme System
 */

#include "futuristic_theme.h"
#include <QPainter>
#include <QPainterPath>
#include <QApplication>
#include <QStyleFactory>
#include <QTimer>
#include <QRandomGenerator>
#include <QDebug>
#include <QToolTip>
#include <QMenu>
#include <cmath>

namespace ComponentsForest {
namespace FuturisticTheme {

// Static instance
ThemeManager* ThemeManager::m_instance = nullptr;

// ==================== FuturisticThemeEngine Implementation ====================
FuturisticThemeEngine::FuturisticThemeEngine() 
    : QProxyStyle(QStyleFactory::create("Fusion")) {
    
    // Animation timer disabled for professional look
    m_animationTimer = nullptr;
    m_animationFrame = 0;
}

void FuturisticThemeEngine::polish(QWidget* widget) {
    QProxyStyle::polish(widget);
    
    // Apply futuristic attributes
    widget->setAttribute(Qt::WA_StyledBackground, true);
    widget->setAttribute(Qt::WA_TranslucentBackground, false);
    
    // No transparency for professional look
    if (qobject_cast<QMenu*>(widget)) {
        widget->setAttribute(Qt::WA_TranslucentBackground, false);
        widget->setWindowOpacity(1.0);
    }
}

void FuturisticThemeEngine::unpolish(QWidget* widget) {
    QProxyStyle::unpolish(widget);
}

void FuturisticThemeEngine::drawPrimitive(PrimitiveElement element, const QStyleOption* option,
                                         QPainter* painter, const QWidget* widget) const {
    
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    
    switch (element) {
        case PE_PanelButtonCommand: {
            // Custom button drawing with glow
            QRect rect = option->rect;
            QPainterPath path;
            path.addRoundedRect(rect, 0, 0); // Sharp corners for tech look
            
            // Background gradient
            QLinearGradient gradient(rect.topLeft(), rect.bottomRight());
            
            if (option->state & State_MouseOver) {
                gradient.setColorAt(0, QColor(0, 61, 82));  // Solid color
                gradient.setColorAt(1, QColor(102, 0, 68));  // Solid color
                
                if (m_glowEnabled) {
                    // Static glow, no animation
                    drawGlowEffect(painter, rect, QColor(0, 212, 255));
                }
            } else {
                gradient.setColorAt(0, QColor(31, 41, 55));  // Solid color
                gradient.setColorAt(1, QColor(20, 24, 36));  // Solid color
            }
            
            painter->fillPath(path, gradient);
            
            // Border with solid color
            painter->setPen(QPen(QColor(0, 128, 170), 1));  // Solid color
            painter->drawPath(path);
            
            if (m_holographicEnabled && (option->state & State_MouseOver)) {
                drawHolographicBorder(painter, rect);
            }
            
            painter->restore();
            return;
        }
        
        case PE_FrameFocusRect: {
            // Custom focus rectangle with animated glow
            if (m_glowEnabled) {
                QRect rect = option->rect;
                QPen pen(QColor(0, 212, 255), 2);
                pen.setStyle(Qt::DashLine);
                painter->setPen(pen);
                painter->drawRect(rect);
                
                // Static glow, no animation
                drawGlowEffect(painter, rect, QColor(0, 212, 255, 150));  // Fixed alpha
            }
            painter->restore();
            return;
        }
        
        case PE_IndicatorCheckBox: {
            // Custom checkbox with neon effect
            QRect rect = option->rect;
            
            // Background with solid colors
            QLinearGradient bg(rect.topLeft(), rect.bottomRight());
            bg.setColorAt(0, QColor(20, 24, 36));  // Solid
            bg.setColorAt(1, QColor(31, 41, 55));  // Solid
            painter->fillRect(rect, bg);
            
            // Border with solid color
            painter->setPen(QPen(QColor(0, 170, 212), 2));  // Solid
            painter->drawRect(rect);
            
            // Checked state
            if (option->state & State_On) {
                // Draw checkmark with glow
                QPainterPath checkPath;
                checkPath.moveTo(rect.left() + rect.width() * 0.2, rect.center().y());
                checkPath.lineTo(rect.left() + rect.width() * 0.4, rect.bottom() - rect.height() * 0.2);
                checkPath.lineTo(rect.right() - rect.width() * 0.2, rect.top() + rect.height() * 0.2);
                
                painter->setPen(QPen(QColor(0, 255, 136), 3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
                painter->drawPath(checkPath);
                
                if (m_glowEnabled) {
                    // Static glow
                    drawGlowEffect(painter, rect, QColor(0, 255, 136));  // Solid color
                }
            }
            
            painter->restore();
            return;
        }
        
        case PE_IndicatorRadioButton: {
            // Custom radio button with circular glow
            QRect rect = option->rect;
            QPoint center = rect.center();
            int radius = rect.width() / 2 - 2;
            
            // Background with solid colors
            QRadialGradient bg(center, radius);
            bg.setColorAt(0, QColor(31, 41, 55));  // Solid
            bg.setColorAt(1, QColor(20, 24, 36));  // Solid
            painter->setBrush(bg);
            painter->setPen(QPen(QColor(0, 170, 212), 2));  // Solid
            painter->drawEllipse(center, radius, radius);
            
            // Checked state
            if (option->state & State_On) {
                // Inner circle with glow
                QRadialGradient innerGradient(center, radius/2);
                innerGradient.setColorAt(0, QColor(0, 212, 255));
                innerGradient.setColorAt(0.5, QColor(255, 0, 170));
                innerGradient.setColorAt(1, QColor(0, 170, 212));  // Solid
                
                painter->setBrush(innerGradient);
                painter->setPen(Qt::NoPen);
                painter->drawEllipse(center, radius/2, radius/2);
                
                if (m_glowEnabled) {
                    // Static glow
                    drawGlowEffect(painter, rect, QColor(0, 212, 255));  // Solid
                }
            }
            
            painter->restore();
            return;
        }
        
        default:
            break;
    }
    
    painter->restore();
    QProxyStyle::drawPrimitive(element, option, painter, widget);
}

void FuturisticThemeEngine::drawControl(ControlElement element, const QStyleOption* option,
                                       QPainter* painter, const QWidget* widget) const {
    
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    
    switch (element) {
        case CE_PushButton: {
            // Draw futuristic button
            const QStyleOptionButton* buttonOption = qstyleoption_cast<const QStyleOptionButton*>(option);
            if (buttonOption) {
                drawPrimitive(PE_PanelButtonCommand, option, painter, widget);
                
                // Draw text with neon effect
                QRect textRect = buttonOption->rect;
                QString text = buttonOption->text;
                
                if (!text.isEmpty()) {
                    QColor textColor = QColor(224, 232, 240);
                    if (option->state & State_MouseOver) {
                        textColor = QColor(0, 212, 255);
                        if (m_glowEnabled) {
                            drawNeonText(painter, textRect, text, textColor);
                        }
                    }
                    
                    painter->setPen(textColor);
                    painter->setFont(widget->font());
                    painter->drawText(textRect, Qt::AlignCenter, text);
                }
            }
            painter->restore();
            return;
        }
        
        case CE_ProgressBar: {
            // Draw futuristic progress bar
            const QStyleOptionProgressBar* progressOption = qstyleoption_cast<const QStyleOptionProgressBar*>(option);
            if (progressOption) {
                QRect rect = progressOption->rect;
                
                // Background with solid color
                painter->fillRect(rect, QColor(20, 24, 36));  // Solid
                painter->setPen(QPen(QColor(0, 128, 170), 1));  // Solid
                painter->drawRect(rect);
                
                // Progress
                if (progressOption->progress > 0) {
                    int progressWidth = (rect.width() * progressOption->progress) / progressOption->maximum;
                    QRect progressRect(rect.left(), rect.top(), progressWidth, rect.height());
                    
                    // Static gradient, no animation
                    QLinearGradient gradient(progressRect.left(), 0, progressRect.right(), 0);
                    gradient.setColorAt(0, QColor(0, 212, 255));
                    gradient.setColorAt(0.5, QColor(255, 0, 170));
                    gradient.setColorAt(1, QColor(0, 153, 255));
                    
                    painter->fillRect(progressRect, gradient);
                    
                    // Static glow effect at the end
                    if (m_glowEnabled) {
                        QRect glowRect(progressRect.right() - 10, progressRect.top(), 10, progressRect.height());
                        drawGlowEffect(painter, glowRect, QColor(255, 255, 255));  // Solid
                    }
                }
                
                // Text
                if (progressOption->textVisible) {
                    QString text = QString("%1%").arg((progressOption->progress * 100) / progressOption->maximum);
                    painter->setPen(QColor(0, 212, 255));
                    painter->drawText(rect, Qt::AlignCenter, text);
                }
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

int FuturisticThemeEngine::pixelMetric(PixelMetric metric, const QStyleOption* option,
                                      const QWidget* widget) const {
    switch (metric) {
        case PM_ButtonMargin:
            return 8;
        case PM_ButtonDefaultIndicator:
            return 0;
        case PM_MenuButtonIndicator:
            return 12;
        case PM_ButtonShiftHorizontal:
        case PM_ButtonShiftVertical:
            return 0;
        default:
            return QProxyStyle::pixelMetric(metric, option, widget);
    }
}

void FuturisticThemeEngine::drawGlowEffect(QPainter* painter, const QRect& rect, const QColor& color) const {
    painter->save();
    
    // Static glow effect - single solid border
    QColor glowColor = color;
    if (glowColor.alpha() == 255) {
        glowColor.setAlpha(100);  // Reduce intensity but keep solid
    }
    
    QPen pen(glowColor, 2);
    painter->setPen(pen);
    painter->setBrush(Qt::NoBrush);
    
    QRect glowRect = rect.adjusted(-2, -2, 2, 2);
    painter->drawRect(glowRect);
    
    painter->restore();
}

void FuturisticThemeEngine::drawHolographicBorder(QPainter* painter, const QRect& rect) const {
    painter->save();
    
    // Static holographic border
    QLinearGradient borderGradient(rect.topLeft(), rect.topRight());
    borderGradient.setColorAt(0, QColor(0, 212, 255));
    borderGradient.setColorAt(0.5, QColor(255, 0, 170));
    borderGradient.setColorAt(1, QColor(0, 212, 255));
    
    QPen pen(borderGradient, 2);
    painter->setPen(pen);
    painter->drawRect(rect);
    
    painter->restore();
}

void FuturisticThemeEngine::drawScanlines(QPainter* painter, const QRect& rect) const {
    painter->save();
    
    // Draw static horizontal scanlines
    painter->setPen(QPen(QColor(0, 212, 255, 40), 1));  // More visible, static
    
    for (int y = rect.top(); y < rect.bottom(); y += 3) {
        painter->drawLine(rect.left(), y, rect.right(), y);
    }
    
    painter->restore();
}

void FuturisticThemeEngine::drawNeonText(QPainter* painter, const QRect& rect, 
                                        const QString& text, const QColor& color) const {
    painter->save();
    
    // Draw text with multiple layers for neon effect
    QFont font = painter->font();
    font.setBold(true);
    painter->setFont(font);
    
    // Outer glow with solid color
    QColor outerGlow = color;
    outerGlow.setAlpha(80);  // Solid but dimmer
    painter->setPen(outerGlow);
    for (int dx = -2; dx <= 2; ++dx) {
        for (int dy = -2; dy <= 2; ++dy) {
            if (dx != 0 || dy != 0) {
                painter->drawText(rect.adjusted(dx, dy, dx, dy), Qt::AlignCenter, text);
            }
        }
    }
    
    // Inner glow with solid color
    QColor innerGlow = color;
    innerGlow.setAlpha(120);  // Solid but dimmer
    painter->setPen(innerGlow);
    for (int dx = -1; dx <= 1; ++dx) {
        for (int dy = -1; dy <= 1; ++dy) {
            if (dx != 0 || dy != 0) {
                painter->drawText(rect.adjusted(dx, dy, dx, dy), Qt::AlignCenter, text);
            }
        }
    }
    
    // Main text
    painter->setPen(QColor(255, 255, 255));
    painter->drawText(rect, Qt::AlignCenter, text);
    
    painter->restore();
}

// ==================== ThemeManager Implementation ====================
ThemeManager::ThemeManager() : QObject() {
    m_themeEngine = new FuturisticThemeEngine();
    m_currentTheme = "futuristic";
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
    
    // Apply the futuristic stylesheet
    widget->setStyleSheet(getFuturisticStylesheet());
    
    // Set the custom style
    QApplication::setStyle(m_themeEngine);
    
    // Apply special effects based on widget type
    if (widget->property("holographic").toBool()) {
        holographicEffect(widget);
    }
    
    if (widget->property("neon").toBool()) {
        glowWidget(widget, QColor("#00D4FF"));
    }
    
    if (widget->property("scanlines").toBool()) {
        scanlineEffect(widget);
    }
    
    // Ensure proper font
    QFont techFont("Orbitron");
    if (!techFont.exactMatch()) {
        techFont = QFont("Consolas", 10);
    }
    widget->setFont(techFont);
}

void ThemeManager::setThemeMode(const QString& mode) {
    m_currentTheme = mode;
    
    if (mode == "holographic") {
        m_themeEngine->enableHolographicEffects(true);
        m_themeEngine->enableGlowEffects(true);
        m_themeEngine->enableScanlineEffect(false);
    } else if (mode == "matrix") {
        m_themeEngine->enableHolographicEffects(false);
        m_themeEngine->enableGlowEffects(true);
        m_themeEngine->enableScanlineEffect(true);
    } else if (mode == "neon") {
        m_themeEngine->enableHolographicEffects(false);
        m_themeEngine->enableGlowEffects(true);
        m_themeEngine->enableScanlineEffect(false);
    } else { // futuristic (default)
        m_themeEngine->enableHolographicEffects(true);
        m_themeEngine->enableGlowEffects(true);
        m_themeEngine->enableScanlineEffect(false);
    }
    
    emit themeChanged(mode);
}

void ThemeManager::enableAnimations(bool enable) {
    // Control global animation state
    if (enable) {
        for (auto* animation : m_widgetAnimations) {
            if (animation->state() != QAbstractAnimation::Running) {
                animation->start();
            }
        }
    } else {
        for (auto* animation : m_widgetAnimations) {
            animation->pause();
        }
    }
}

void ThemeManager::setAnimationSpeed(int speed) {
    // Adjust animation duration based on speed (1-10)
    int duration = 2000 / speed; // Faster speed = shorter duration
    
    for (auto* animation : m_widgetAnimations) {
        if (auto* propertyAnim = qobject_cast<QPropertyAnimation*>(animation)) {
            propertyAnim->setDuration(duration);
        }
    }
}

void ThemeManager::setGlowIntensity(int intensity) {
    // Adjust glow effect intensity (1-10)
    for (auto& effectList : m_widgetEffects) {
        for (auto* effect : effectList) {
            if (auto* glowEffect = qobject_cast<QGraphicsDropShadowEffect*>(effect)) {
                glowEffect->setBlurRadius(10 + intensity * 3);
            }
        }
    }
}

void ThemeManager::setNeonColors(const QColor& primary, const QColor& secondary) {
    // Update neon color scheme dynamically
    // This would require regenerating the stylesheet with new colors
    // For now, we'll just update effects
    for (auto& effectList : m_widgetEffects) {
        for (auto* effect : effectList) {
            if (auto* colorEffect = qobject_cast<QGraphicsColorizeEffect*>(effect)) {
                colorEffect->setColor(primary);
            }
        }
    }
}

void ThemeManager::pulseWidget(QWidget* widget, const QColor& color) {
    if (!widget) return;
    
    // Create static glow effect (no pulsing)
    auto* glowEffect = new QGraphicsDropShadowEffect(widget);
    glowEffect->setBlurRadius(15);  // Fixed radius
    glowEffect->setColor(color);
    glowEffect->setOffset(0, 0);
    
    widget->setGraphicsEffect(glowEffect);
    
    // No animation - just store the static effect
    m_widgetEffects[widget].append(glowEffect);
}

void ThemeManager::glowWidget(QWidget* widget, const QColor& color) {
    if (!widget) return;
    
    auto* glowEffect = new QGraphicsDropShadowEffect(widget);
    glowEffect->setBlurRadius(25);
    glowEffect->setColor(color);
    glowEffect->setOffset(0, 0);
    
    widget->setGraphicsEffect(glowEffect);
    m_widgetEffects[widget].append(glowEffect);
}

void ThemeManager::scanlineEffect(QWidget* widget) {
    if (!widget) return;
    
    // Create static scanlines effect
    widget->setProperty("scanlines", true);
    
    // No animation timer - scanlines are static
    widget->update();  // Just update once
}

void ThemeManager::holographicEffect(QWidget* widget) {
    if (!widget) return;
    
    // Apply static holographic color effect
    auto* colorEffect = new QGraphicsColorizeEffect(widget);
    colorEffect->setColor(QColor(0, 212, 255));
    colorEffect->setStrength(0.1);  // Subtle, static tint
    
    widget->setGraphicsEffect(colorEffect);
    
    // No animation - just store the static effect
    m_widgetEffects[widget].append(colorEffect);
}

void ThemeManager::matrixRainEffect(QWidget* widget) {
    if (!widget) return;
    
    // Apply static Matrix-style green tint
    auto* colorEffect = new QGraphicsColorizeEffect(widget);
    colorEffect->setColor(QColor(0, 255, 136));
    colorEffect->setStrength(0.15);  // Subtle, static tint
    
    widget->setGraphicsEffect(colorEffect);
    m_widgetEffects[widget].append(colorEffect);
    
    // Add static scanlines for Matrix effect
    scanlineEffect(widget);
}

} // namespace FuturisticTheme
} // namespace ComponentsForest