/**
 * @file futuristic_widgets.cpp
 * @brief Implementation of Futuristic Custom Widgets
 */

#include "futuristic_widgets.h"
#include <QPainter>
#include <QPainterPath>
#include <QLinearGradient>
#include <QRadialGradient>
#include <QConicalGradient>
#include <QKeyEvent>
#include <QDebug>
#include <QRandomGenerator>
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace ComponentsForest {
namespace FuturisticWidgets {

// ==================== HolographicToggle Implementation ====================
HolographicToggle::HolographicToggle(QWidget* parent) : QWidget(parent) {
    setFixedSize(80, 40);
    setCursor(Qt::PointingHandCursor);
    
    // Position animation
    m_posAnimation = new QPropertyAnimation(this, "switchPosition", this);
    m_posAnimation->setDuration(300);
    m_posAnimation->setEasingCurve(QEasingCurve::OutExpo);
    
    // Glow animation
    m_glowAnimation = new QPropertyAnimation(this, "glowIntensity", this);
    m_glowAnimation->setDuration(200);
    m_glowAnimation->setEasingCurve(QEasingCurve::OutQuad);
    
    // Energy field animation disabled for professional look
    m_energyTimer = nullptr;
    m_energyPhase = 0;  // Static phase
}

void HolographicToggle::setChecked(bool checked) {
    if (m_checked != checked) {
        m_checked = checked;
        animateSwitch();
        emit toggled(checked);
    }
}

void HolographicToggle::setSwitchPosition(qreal pos) {
    m_switchPos = pos;
    update();
}

void HolographicToggle::setGlowIntensity(qreal intensity) {
    m_glowIntensity = intensity;
    update();
}

void HolographicToggle::toggle() {
    setChecked(!m_checked);
}

void HolographicToggle::animateSwitch() {
    m_posAnimation->setStartValue(m_switchPos);
    m_posAnimation->setEndValue(m_checked ? 1.0 : 0.0);
    m_posAnimation->start();
}

void HolographicToggle::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    drawHolographicTrack(&painter);
    drawEnergyField(&painter);
    drawGlowingHandle(&painter);
}

void HolographicToggle::drawHolographicTrack(QPainter* painter) {
    QRectF trackRect(5, 10, width() - 10, 20);
    QPainterPath trackPath;
    trackPath.addRoundedRect(trackRect, 10, 10);
    
    // Background gradient with solid colors
    QLinearGradient bgGradient(trackRect.left(), 0, trackRect.right(), 0);
    if (m_checked) {
        bgGradient.setColorAt(0, QColor(0, 128, 170));  // Solid
        bgGradient.setColorAt(0.5, QColor(170, 0, 102));  // Solid
        bgGradient.setColorAt(1, QColor(0, 128, 170));  // Solid
    } else {
        bgGradient.setColorAt(0, QColor(31, 41, 55));  // Solid
        bgGradient.setColorAt(1, QColor(20, 24, 36));  // Solid
    }
    
    painter->fillPath(trackPath, bgGradient);
    
    // Static holographic border
    QPen borderPen;
    if (m_isHovered || m_checked) {
        QLinearGradient borderGradient(trackRect.left(), 0, trackRect.right(), 0);
        borderGradient.setColorAt(0, QColor(0, 212, 255));
        borderGradient.setColorAt(0.5, QColor(255, 0, 170));
        borderGradient.setColorAt(1, QColor(0, 212, 255));
        borderPen = QPen(borderGradient, 2);
    } else {
        borderPen = QPen(QColor(0, 128, 170), 1);  // Solid
    }
    
    painter->setPen(borderPen);
    painter->drawPath(trackPath);
}

void HolographicToggle::drawGlowingHandle(QPainter* painter) {
    qreal handleX = 15 + (width() - 40) * m_switchPos;
    QPointF center(handleX, height() / 2);
    
    // Static outer glow
    if (m_glowIntensity > 0) {
        QRadialGradient glowGradient(center, 25);
        QColor glowColor = m_checked ? QColor(0, 212, 255) : QColor(255, 0, 170);
        glowColor.setAlpha(100);  // Fixed alpha
        glowGradient.setColorAt(0, glowColor);
        glowGradient.setColorAt(1, QColor(glowColor.red(), glowColor.green(), glowColor.blue(), 0));
        
        painter->setBrush(glowGradient);
        painter->setPen(Qt::NoPen);
        painter->drawEllipse(center, 20, 20);
    }
    
    // Handle gradient
    QRadialGradient handleGradient(center, 15);
    if (m_checked) {
        handleGradient.setColorAt(0, QColor(255, 255, 255));
        handleGradient.setColorAt(0.5, QColor(0, 212, 255));
        handleGradient.setColorAt(1, QColor(0, 153, 255));
    } else {
        handleGradient.setColorAt(0, QColor(200, 200, 200));
        handleGradient.setColorAt(0.5, QColor(100, 100, 100));
        handleGradient.setColorAt(1, QColor(60, 60, 60));
    }
    
    painter->setBrush(handleGradient);
    painter->setPen(QPen(QColor(0, 212, 255), 2));  // Solid
    painter->drawEllipse(center, 15, 15);
    
    // Inner core
    if (m_checked) {
        painter->setBrush(QColor(255, 255, 255));
        painter->setPen(Qt::NoPen);
        painter->drawEllipse(center, 5, 5);
    }
}

void HolographicToggle::drawEnergyField(QPainter* painter) {
    if (!m_checked) return;
    
    painter->save();
    
    // Draw static energy lines
    QPen energyPen(QColor(0, 212, 255, 80), 1);  // Static transparency
    painter->setPen(energyPen);
    
    qreal handleX = 15 + (width() - 40) * m_switchPos;
    // Draw simple static lines
    painter->drawLine(QPointF(5, height()/2), 
                     QPointF(handleX - 20, height()/2));
    
    painter->restore();
}

void HolographicToggle::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_glowAnimation->setStartValue(m_glowIntensity);
        m_glowAnimation->setEndValue(1.0);
        m_glowAnimation->start();
    }
}

void HolographicToggle::mouseReleaseEvent(QMouseEvent* event) {
    if (rect().contains(event->pos())) {
        toggle();
        emit clicked();
    }
    m_glowAnimation->setStartValue(m_glowIntensity);
    m_glowAnimation->setEndValue(m_isHovered ? 0.5 : 0.0);
    m_glowAnimation->start();
}

void HolographicToggle::enterEvent(QEnterEvent*) {
    m_isHovered = true;
    m_glowAnimation->setStartValue(m_glowIntensity);
    m_glowAnimation->setEndValue(0.5);
    m_glowAnimation->start();
}

void HolographicToggle::leaveEvent(QEvent*) {
    m_isHovered = false;
    m_glowAnimation->setStartValue(m_glowIntensity);
    m_glowAnimation->setEndValue(0.0);
    m_glowAnimation->start();
}

// ==================== NeonGlowButton Implementation ====================
NeonGlowButton::NeonGlowButton(const QString& text, QWidget* parent) 
    : QPushButton(text, parent) {
    
    setMinimumHeight(45);
    setCursor(Qt::PointingHandCursor);
    m_glowColor = QColor(0, 212, 255);
    
    // Glow animation
    m_glowAnimation = new QPropertyAnimation(this, "glowRadius", this);
    m_glowAnimation->setDuration(1500);
    m_glowAnimation->setStartValue(0);
    m_glowAnimation->setEndValue(20);
    m_glowAnimation->setEasingCurve(QEasingCurve::InOutSine);
    m_glowAnimation->setLoopCount(-1);
    
    // Pulse animation
    m_pulseAnimation = new QPropertyAnimation(this, "pulsePhase", this);
    m_pulseAnimation->setDuration(2000);
    m_pulseAnimation->setStartValue(0.0);
    m_pulseAnimation->setEndValue(1.0);
    m_pulseAnimation->setLoopCount(-1);
    
    // Animations disabled for professional look
    m_animationGroup = nullptr;
    m_isPulsing = false;  // Disable pulsing
}

void NeonGlowButton::setNeonColor(const QColor& color) {
    m_glowColor = color;
    update();
}

void NeonGlowButton::enablePulse(bool enable) {
    // Pulsing disabled for professional look
    m_isPulsing = false;
    Q_UNUSED(enable);
}

void NeonGlowButton::setGlowIntensity(int intensity) {
    m_glowIntensity = qBound(0, intensity, 10);
    update();
}

void NeonGlowButton::setGlowRadius(qreal radius) {
    m_glowRadius = radius;
    update();
}

void NeonGlowButton::setPulsePhase(qreal phase) {
    m_pulsePhase = phase;
    update();
}

void NeonGlowButton::setGlowColor(const QColor& color) {
    m_glowColor = color;
    update();
}

void NeonGlowButton::startGlowAnimation() {
    // Animation disabled - just update appearance
    update();
}

void NeonGlowButton::stopGlowAnimation() {
    // Animation disabled - just update appearance
    update();
}

void NeonGlowButton::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    drawGlowEffect(&painter);
    drawNeonBorder(&painter);
    drawHolographicShimmer(&painter);
    
    // Draw text with glow
    QFont font = this->font();
    font.setBold(true);
    font.setCapitalization(QFont::AllUppercase);
    painter.setFont(font);
    
    // Text shadow with solid color
    painter.setPen(QColor(0, 0, 0));
    painter.drawText(rect().adjusted(1, 1, 1, 1), Qt::AlignCenter, text());
    
    // Main text
    painter.setPen(QColor(255, 255, 255));
    painter.drawText(rect(), Qt::AlignCenter, text());
}

void NeonGlowButton::drawNeonBorder(QPainter* painter) {
    QRect buttonRect = rect().adjusted(2, 2, -2, -2);
    QPainterPath path;
    path.addRoundedRect(buttonRect, 0, 0);
    
    // Single layer solid neon border
    painter->setPen(QPen(m_glowColor, 2));
    painter->drawPath(path);
}

void NeonGlowButton::drawGlowEffect(QPainter* painter) {
    if (m_glowRadius <= 0) return;
    
    painter->save();
    
    // Single static glow layer
    QRect glowRect = rect();
    QColor glowColor = m_glowColor;
    glowColor.setAlpha(80);  // Fixed alpha
    
    QRadialGradient gradient(glowRect.center(), glowRect.width() / 2 + 10);
    gradient.setColorAt(0, glowColor);
    gradient.setColorAt(1, QColor(glowColor.red(), glowColor.green(), glowColor.blue(), 0));
    
    painter->setBrush(gradient);
    painter->setPen(Qt::NoPen);
    painter->drawRoundedRect(glowRect.adjusted(-10, -10, 10, 10), 0, 0);
    
    painter->restore();
}

void NeonGlowButton::drawHolographicShimmer(QPainter* painter) {
    painter->save();
    
    // Static shimmer effect
    QLinearGradient shimmer(rect().topLeft(), rect().topRight());
    shimmer.setColorAt(0, QColor(255, 255, 255, 0));
    shimmer.setColorAt(0.5, QColor(255, 255, 255, 30));  // Subtle static highlight
    shimmer.setColorAt(1, QColor(255, 255, 255, 0));
    
    painter->fillRect(rect(), shimmer);
    painter->restore();
}

void NeonGlowButton::enterEvent(QEnterEvent*) {
    // Static glow on hover
    m_glowRadius = 20;
    update();
}

void NeonGlowButton::leaveEvent(QEvent*) {
    // Reduce glow on leave
    m_glowRadius = 10;
    update();
}

// ==================== CyberpunkProgressRing Implementation ====================
CyberpunkProgressRing::CyberpunkProgressRing(QWidget* parent) : QWidget(parent) {
    setMinimumSize(150, 150);
    m_primaryColor = QColor(0, 212, 255);
    m_secondaryColor = QColor(255, 0, 170);
    
    // Animations disabled for professional look
    m_rotationAnimation = nullptr;
    m_pulseAnimation = nullptr;
    m_rotation = 0;
    m_glowPulse = 1.0;
    
    // Static data points (no animation)
    m_dataTimer = nullptr;
    // Generate initial static data points
    for (int i = 0; i < 8; ++i) {
        qreal angle = i * 45 * M_PI / 180.0;
        qreal radius = 55;  // Fixed radius
        m_dataPoints.append(QPointF(radius * cos(angle), radius * sin(angle)));
    }
    
    enableAnimation(true);
}

void CyberpunkProgressRing::setRange(int min, int max) {
    m_minimum = min;
    m_maximum = max;
    update();
}

void CyberpunkProgressRing::setColors(const QColor& primary, const QColor& secondary) {
    m_primaryColor = primary;
    m_secondaryColor = secondary;
    update();
}

void CyberpunkProgressRing::enableAnimation(bool enable) {
    // Animations disabled for professional look
    Q_UNUSED(enable);
    update();
}

void CyberpunkProgressRing::setLabel(const QString& label) {
    m_label = label;
    update();
}

void CyberpunkProgressRing::setValue(int value) {
    if (m_value != value) {
        m_value = qBound(m_minimum, value, m_maximum);
        emit valueChanged(m_value);
        update();
    }
}

void CyberpunkProgressRing::setRotationAngle(qreal angle) {
    m_rotation = angle;
    update();
}

void CyberpunkProgressRing::setGlowPulse(qreal pulse) {
    m_glowPulse = pulse;
    update();
}

void CyberpunkProgressRing::reset() {
    setValue(m_minimum);
}

void CyberpunkProgressRing::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.translate(width() / 2, height() / 2);
    
    drawHolographicGrid(&painter);
    drawOuterRing(&painter);
    drawDataPoints(&painter);
    drawProgressArc(&painter);
    drawInnerDisplay(&painter);
}

void CyberpunkProgressRing::drawOuterRing(QPainter* painter) {
    painter->save();
    
    int radius = qMin(width(), height()) / 2 - 10;
    
    // Static outer glow
    QColor glowColor = m_primaryColor;
    glowColor.setAlpha(60);  // Fixed alpha
    painter->setPen(QPen(glowColor, 3));
    painter->drawEllipse(QPoint(0, 0), radius + 3, radius + 3);
    
    // Main ring
    painter->setPen(QPen(m_primaryColor, 2));
    painter->drawEllipse(QPoint(0, 0), radius, radius);
    
    painter->restore();
}

void CyberpunkProgressRing::drawProgressArc(QPainter* painter) {
    painter->save();
    painter->rotate(m_rotation);
    
    int radius = qMin(width(), height()) / 2 - 20;
    QRect arcRect(-radius, -radius, radius * 2, radius * 2);
    
    qreal progress = (m_value - m_minimum) / (qreal)(m_maximum - m_minimum);
    int spanAngle = -progress * 360 * 16; // Qt uses 1/16th degree units
    
    // Progress gradient
    QConicalGradient gradient(0, 0, -90);
    gradient.setColorAt(0, m_primaryColor);
    gradient.setColorAt(0.5, m_secondaryColor);
    gradient.setColorAt(1, m_primaryColor);
    
    painter->setPen(QPen(QBrush(gradient), 4, Qt::SolidLine, Qt::RoundCap));
    painter->drawArc(arcRect, 90 * 16, spanAngle);
    
    // Leading edge glow
    if (progress > 0) {
        qreal endAngle = 90 - progress * 360;
        qreal x = radius * cos(endAngle * M_PI / 180);
        qreal y = radius * sin(endAngle * M_PI / 180);
        
        QRadialGradient glowGradient(x, -y, 15);
        glowGradient.setColorAt(0, QColor(255, 255, 255));
        glowGradient.setColorAt(0.5, m_secondaryColor);
        glowGradient.setColorAt(1, Qt::transparent);
        
        painter->setBrush(glowGradient);
        painter->setPen(Qt::NoPen);
        painter->drawEllipse(QPointF(x, -y), 10, 10);
    }
    
    painter->restore();
}

void CyberpunkProgressRing::drawInnerDisplay(QPainter* painter) {
    painter->save();
    
    // Digital value display
    QFont font("Orbitron", 24, QFont::Bold);
    painter->setFont(font);
    painter->setPen(m_primaryColor);
    
    QString valueText = QString::number(m_value);
    painter->drawText(QRect(-50, -15, 100, 30), Qt::AlignCenter, valueText);
    
    // Label
    if (!m_label.isEmpty()) {
        font.setPointSize(10);
        font.setWeight(QFont::Normal);
        painter->setFont(font);
        painter->setPen(QColor(160, 176, 192));
        painter->drawText(QRect(-50, 10, 100, 20), Qt::AlignCenter, m_label.toUpper());
    }
    
    // Percentage
    qreal percentage = (m_value - m_minimum) / (qreal)(m_maximum - m_minimum) * 100;
    font.setPointSize(8);
    painter->setFont(font);
    painter->setPen(m_secondaryColor);
    painter->drawText(QRect(-50, -30, 100, 15), Qt::AlignCenter, 
                     QString("%1%").arg(percentage, 0, 'f', 1));
    
    painter->restore();
}

void CyberpunkProgressRing::drawDataPoints(QPainter* painter) {
    if (m_dataPoints.isEmpty()) return;
    
    painter->save();
    painter->rotate(m_rotation * 2);
    
    // Connect data points
    painter->setPen(QPen(QColor(0, 255, 136, 100), 1));
    for (int i = 0; i < m_dataPoints.size(); ++i) {
        int next = (i + 1) % m_dataPoints.size();
        painter->drawLine(m_dataPoints[i], m_dataPoints[next]);
    }
    
    // Draw points
    painter->setPen(Qt::NoPen);
    for (const auto& point : m_dataPoints) {
        QRadialGradient pointGradient(point, 5);
        pointGradient.setColorAt(0, QColor(0, 255, 136));
        pointGradient.setColorAt(1, Qt::transparent);
        painter->setBrush(pointGradient);
        painter->drawEllipse(point, 3, 3);
    }
    
    painter->restore();
}

void CyberpunkProgressRing::drawHolographicGrid(QPainter* painter) {
    painter->save();
    painter->setOpacity(0.1);
    painter->setPen(QPen(m_primaryColor, 1, Qt::DotLine));
    
    // Radial lines
    for (int i = 0; i < 12; ++i) {
        painter->rotate(30);
        painter->drawLine(0, 0, qMin(width(), height()) / 2 - 5, 0);
    }
    
    // Concentric circles
    for (int i = 20; i < qMin(width(), height()) / 2; i += 20) {
        painter->drawEllipse(QPoint(0, 0), i, i);
    }
    
    painter->restore();
}

void CyberpunkProgressRing::resizeEvent(QResizeEvent*) {
    update();
}

// ==================== MatrixDataStream Implementation ====================
MatrixDataStream::MatrixDataStream(QWidget* parent) : QWidget(parent) {
    setMinimumSize(200, 300);
    setAttribute(Qt::WA_OpaquePaintEvent);
    
    m_updateTimer = new QTimer(this);
    connect(m_updateTimer, &QTimer::timeout, [this]() {
        updateColumns();
        update();
    });
}

void MatrixDataStream::setStreamSpeed(int speed) {
    m_streamSpeed = qBound(1, speed, 10);
    m_updateTimer->setInterval(100 - m_streamSpeed * 8);
}

void MatrixDataStream::setDataDensity(int density) {
    m_dataDensity = qBound(1, density, 10);
}

void MatrixDataStream::setTextMode(bool binary) {
    m_binaryMode = binary;
}

void MatrixDataStream::startStream() {
    // Initialize columns
    m_columns.clear();
    int columnWidth = 15;
    int numColumns = width() / columnWidth;
    
    for (int i = 0; i < numColumns; ++i) {
        DataColumn column;
        column.x = i * columnWidth + 5;
        column.y = QRandomGenerator::global()->bounded(height());
        column.speed = 1 + QRandomGenerator::global()->bounded(3);
        generateColumnData(column);
        m_columns.push_back(column);
    }
    
    m_updateTimer->start(50);
}

void MatrixDataStream::stopStream() {
    // No timer to stop - static display
}

void MatrixDataStream::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.fillRect(rect(), QColor(10, 14, 26));
    
    QFont font("Consolas", 10);
    painter.setFont(font);
    
    for (const auto& column : m_columns) {
        for (int i = 0; i < column.data.length(); ++i) {
            int y = column.y - i * 15;
            if (y < 0 || y > height()) continue;
            
            // Static fade effect (no animation)
            qreal fade = 1.0 - (i / (qreal)column.data.length());
            QColor color = column.color;
            color.setAlpha(200 * fade);  // Fixed alpha calculation
            
            // Head of stream is brighter
            if (i == 0) {
                color = QColor(255, 255, 255);
            }
            
            painter.setPen(color);
            painter.drawText(column.x, y, column.data[i]);
        }
    }
}

void MatrixDataStream::updateColumns() {
    // No animation - columns stay static
    // Keep method for compatibility but do nothing
}

void MatrixDataStream::generateColumnData(DataColumn& column) {
    column.data.clear();
    int length = 10;  // Fixed length for static display
    
    for (int i = 0; i < length; ++i) {
        if (m_binaryMode) {
            column.data += QString::number(QRandomGenerator::global()->bounded(2));
        } else {
            // Random characters (Japanese, numbers, letters)
            int type = QRandomGenerator::global()->bounded(3);
            if (type == 0) {
                column.data += QChar(0x30A0 + QRandomGenerator::global()->bounded(96));
            } else if (type == 1) {
                column.data += QString::number(QRandomGenerator::global()->bounded(10));
            } else {
                column.data += QChar('A' + QRandomGenerator::global()->bounded(26));
            }
        }
    }
    
    // Random color (mostly green, sometimes other colors)
    int colorType = QRandomGenerator::global()->bounded(10);
    if (colorType < 7) {
        column.color = QColor(0, 255, 136); // Matrix green
    } else if (colorType < 9) {
        column.color = QColor(0, 212, 255); // Cyan
    } else {
        column.color = QColor(255, 0, 170); // Magenta
    }
}

// ==================== Missing Constructor Implementations ====================

FuturisticGauge::FuturisticGauge(QWidget* parent) : QWidget(parent) {
    setMinimumSize(200, 200);
    
    // Initialize default colors
    m_normalColor = QColor(0, 212, 255);
    m_warningColor = QColor(255, 170, 0);
    m_criticalColor = QColor(255, 0, 85);
    
    // Initialize animation
    m_needleAnimation = nullptr;
    m_warningTimer = new QTimer(this);
    m_warningTimer->setInterval(500);
    
    // Set default title and unit
    m_title = "Value";
    m_unit = "%";
}

CyberTerminal::CyberTerminal(QWidget* parent) : QWidget(parent) {
    setMinimumSize(400, 300);
    setFocusPolicy(Qt::StrongFocus);
    
    // Set up basic terminal appearance
    setStyleSheet(R"(
        CyberTerminal {
            background-color: #0A0E1A;
            color: #00FF88;
            font-family: 'Consolas', monospace;
            font-size: 12px;
            border: 2px solid #00D4FF;
        }
    )");
}

ParticleEffectOverlay::ParticleEffectOverlay(QWidget* parent) : QWidget(parent) {
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAttribute(Qt::WA_NoSystemBackground);
    setStyleSheet("background: transparent;");
    
    // Initialize particle system (basic setup)
    // Actual particle logic would be in paintEvent
}

// ==================== Q_PROPERTY Implementation Methods ====================

// HolographicSlider properties
void HolographicSlider::setGlowPosition(qreal pos) {
    if (pos != m_glowPosition) {
        m_glowPosition = pos;
        update();
        emit glowPositionChanged();
    }
}

void HolographicSlider::setWavePhase(qreal phase) {
    if (phase != m_wavePhase) {
        m_wavePhase = phase;
        update();
        emit wavePhaseChanged();
    }
}

qreal HolographicSlider::glowPosition() const {
    return m_glowPosition;
}

qreal HolographicSlider::wavePhase() const {
    return m_wavePhase;
}

// FuturisticGauge properties
void FuturisticGauge::setValue(qreal value) {
    if (value != m_value) {
        m_value = qBound(m_minimum, value, m_maximum);
        update();
        emit valueChanged(m_value);
    }
}

void FuturisticGauge::setNeedleAngle(qreal angle) {
    if (angle != m_needleAngle) {
        m_needleAngle = angle;
        update();
        emit needleAngleChanged();
    }
}

qreal FuturisticGauge::value() const {
    return m_value;
}

qreal FuturisticGauge::needleAngle() const {
    return m_needleAngle;
}

void FuturisticGauge::animateToValue(qreal target) {
    if (m_needleAnimation) {
        m_needleAnimation->stop();
        m_needleAnimation->deleteLater();
    }
    
    m_needleAnimation = new QPropertyAnimation(this, "needleAngle");
    m_needleAnimation->setDuration(800);
    m_needleAnimation->setStartValue(m_needleAngle);
    m_needleAnimation->setEndValue(target);
    m_needleAnimation->setEasingCurve(QEasingCurve::OutElastic);
    m_needleAnimation->start();
}

// HolographicStatusCard properties
void HolographicStatusCard::setAnimationPhase(qreal phase) {
    if (phase != m_animationPhase) {
        m_animationPhase = phase;
        update();
        emit animationPhaseChanged();
    }
}

qreal HolographicStatusCard::animationPhase() const {
    return m_animationPhase;
}

void HolographicStatusCard::pulse() {
    if (m_phaseAnimation) {
        m_phaseAnimation->stop();
        m_phaseAnimation->deleteLater();
    }
    
    m_phaseAnimation = new QPropertyAnimation(this, "animationPhase");
    m_phaseAnimation->setDuration(1000);
    m_phaseAnimation->setStartValue(0.0);
    m_phaseAnimation->setEndValue(1.0);
    m_phaseAnimation->setEasingCurve(QEasingCurve::InOutSine);
    m_phaseAnimation->start();
}

// ==================== Missing Virtual Method Implementations ====================

void FuturisticGauge::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Basic circular gauge implementation
    QRectF rect = this->rect().adjusted(10, 10, -10, -10);
    painter.setPen(QPen(m_normalColor, 3));
    painter.drawArc(rect, 16 * -135, 16 * 270); // Draw arc from -135 to 135 degrees
    
    // Draw needle (simple line)
    QPointF center = rect.center();
    qreal angle = m_needleAngle * M_PI / 180.0;
    qreal radius = qMin(rect.width(), rect.height()) * 0.4;
    QPointF needleEnd(center.x() + radius * cos(angle), center.y() + radius * sin(angle));
    
    painter.setPen(QPen(Qt::white, 2));
    painter.drawLine(center, needleEnd);
    
    // Draw value text
    painter.setPen(m_normalColor);
    painter.setFont(QFont("Arial", 14));
    painter.drawText(rect.adjusted(0, rect.height() * 0.7, 0, 0), Qt::AlignCenter, 
                     QString::number(m_value, 'f', 1) + " " + m_unit);
}

void FuturisticGauge::resizeEvent(QResizeEvent* event) {
    Q_UNUSED(event);
    // Handle resize if needed
}

void CyberTerminal::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.fillRect(rect(), QColor(10, 14, 26)); // Dark background
    
    // Draw terminal text (placeholder)
    painter.setPen(QColor(0, 255, 136));
    painter.setFont(QFont("Consolas", 12));
    painter.drawText(10, 30, "Cyber Terminal Ready...");
    painter.drawText(10, 50, "> ");
}

void CyberTerminal::keyPressEvent(QKeyEvent* event) {
    Q_UNUSED(event);
    // Handle key press (placeholder)
    update();
}

void ParticleEffectOverlay::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Draw some basic particles (placeholder)
    painter.setPen(QPen(QColor(0, 212, 255, 128), 1));
    for (int i = 0; i < 10; ++i) {
        QPointF pos(i * 20 + 10, 10);
        painter.drawPoint(pos);
    }
}

} // namespace FuturisticWidgets
} // namespace ComponentsForest