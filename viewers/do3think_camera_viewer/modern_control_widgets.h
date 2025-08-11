/**
 * @file modern_control_widgets.h
 * @brief Modern custom widgets for camera control panel
 */

#ifndef MODERN_CONTROL_WIDGETS_H
#define MODERN_CONTROL_WIDGETS_H

#include <QWidget>
#include <QPushButton>
#include <QSlider>
#include <QCheckBox>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QPropertyAnimation>
#include <QGraphicsDropShadowEffect>
#include <QMouseEvent>
#include <QEnterEvent>
#include <QTimer>

namespace ComponentsForest {
namespace ModernWidgets {

// ==================== Modern Toggle Switch ====================
class ModernToggleSwitch : public QWidget {
    Q_OBJECT
    Q_PROPERTY(bool checked READ isChecked WRITE setChecked NOTIFY toggled)
    Q_PROPERTY(qreal position READ position WRITE setPosition)
    
public:
    explicit ModernToggleSwitch(QWidget* parent = nullptr) : QWidget(parent), m_checked(false), m_position(0.0) {
        setFixedSize(56, 28);
        setCursor(Qt::PointingHandCursor);
        
        m_animation = new QPropertyAnimation(this, "position", this);
        m_animation->setDuration(200);
        m_animation->setEasingCurve(QEasingCurve::OutCubic);
    }
    
    bool isChecked() const { return m_checked; }
    qreal position() const { return m_position; }
    
public slots:
    void setChecked(bool checked) {
        if (m_checked != checked) {
            m_checked = checked;
            m_animation->setStartValue(m_position);
            m_animation->setEndValue(checked ? 1.0 : 0.0);
            m_animation->start();
            emit toggled(checked);
        }
    }
    
    void setPosition(qreal pos) {
        m_position = pos;
        update();
    }
    
signals:
    void toggled(bool checked);
    
protected:
    void paintEvent(QPaintEvent*) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        
        // Draw track
        QRectF trackRect(0, 4, width(), 20);
        QPainterPath trackPath;
        trackPath.addRoundedRect(trackRect, 10, 10);
        
        QLinearGradient trackGradient(trackRect.topLeft(), trackRect.topRight());
        if (m_checked) {
            trackGradient.setColorAt(0, QColor("#58a6ff"));
            trackGradient.setColorAt(1, QColor("#8b5cf6"));
        } else {
            trackGradient.setColorAt(0, QColor("#30363d"));
            trackGradient.setColorAt(1, QColor("#21262d"));
        }
        
        painter.fillPath(trackPath, trackGradient);
        
        // Draw handle
        qreal handleX = 4 + (width() - 28) * m_position;
        QRectF handleRect(handleX, 4, 20, 20);
        
        painter.setBrush(Qt::white);
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(handleRect);
        
        // Add inner shadow to handle
        QRadialGradient handleGradient(handleRect.center(), 10);
        handleGradient.setColorAt(0, QColor(255, 255, 255, 200));
        handleGradient.setColorAt(1, QColor(255, 255, 255, 255));
        QPainterPath handlePath;
        handlePath.addEllipse(handleRect);
        painter.fillPath(handlePath, handleGradient);
    }
    
    void mousePressEvent(QMouseEvent* event) override {
        if (event->button() == Qt::LeftButton) {
            setChecked(!m_checked);
        }
    }
    
private:
    bool m_checked;
    qreal m_position;
    QPropertyAnimation* m_animation;
};

// ==================== Modern Circular Progress ====================
class ModernCircularProgress : public QWidget {
    Q_OBJECT
    Q_PROPERTY(int value READ value WRITE setValue)
    Q_PROPERTY(qreal rotation READ rotation WRITE setRotation)
    
public:
    explicit ModernCircularProgress(QWidget* parent = nullptr) 
        : QWidget(parent), m_value(0), m_rotation(0) {
        setFixedSize(100, 100);
        
        // Animation for continuous rotation when indeterminate
        m_rotationAnimation = new QPropertyAnimation(this, "rotation", this);
        m_rotationAnimation->setDuration(1500);
        m_rotationAnimation->setStartValue(0);
        m_rotationAnimation->setEndValue(360);
        m_rotationAnimation->setLoopCount(-1);
    }
    
    int value() const { return m_value; }
    qreal rotation() const { return m_rotation; }
    
    void setIndeterminate(bool indeterminate) {
        if (indeterminate) {
            m_rotationAnimation->start();
        } else {
            m_rotationAnimation->stop();
            m_rotation = 0;
        }
        update();
    }
    
public slots:
    void setValue(int value) {
        if (m_value != value) {
            m_value = qBound(0, value, 100);
            update();
        }
    }
    
    void setRotation(qreal rotation) {
        m_rotation = rotation;
        update();
    }
    
protected:
    void paintEvent(QPaintEvent*) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        
        QRectF rect(10, 10, 80, 80);
        
        // Background circle
        painter.setPen(QPen(QColor("#21262d"), 8, Qt::SolidLine, Qt::RoundCap));
        painter.drawEllipse(rect);
        
        // Progress arc
        painter.save();
        painter.translate(50, 50);
        painter.rotate(m_rotation - 90);
        
        QConicalGradient gradient(0, 0, 0);
        gradient.setColorAt(0, QColor("#58a6ff"));
        gradient.setColorAt(1, QColor("#8b5cf6"));
        
        painter.setPen(QPen(QBrush(gradient), 8, Qt::SolidLine, Qt::RoundCap));
        painter.drawArc(QRectF(-40, -40, 80, 80), 0, -m_value * 36);
        painter.restore();
        
        // Center text
        painter.setPen(QColor("#f0f6fc"));
        painter.setFont(QFont("Inter", 16, QFont::Bold));
        painter.drawText(rect, Qt::AlignCenter, QString::number(m_value) + "%");
    }
    
private:
    int m_value;
    qreal m_rotation;
    QPropertyAnimation* m_rotationAnimation;
};

// ==================== Modern Gradient Button ====================
class ModernGradientButton : public QPushButton {
    Q_OBJECT
    Q_PROPERTY(qreal hoverProgress READ hoverProgress WRITE setHoverProgress)
    
public:
    explicit ModernGradientButton(const QString& text, QWidget* parent = nullptr)
        : QPushButton(text, parent), m_hoverProgress(0.0) {
        
        setMinimumHeight(40);
        setCursor(Qt::PointingHandCursor);
        
        // Hover animation
        m_hoverAnimation = new QPropertyAnimation(this, "hoverProgress", this);
        m_hoverAnimation->setDuration(200);
        m_hoverAnimation->setEasingCurve(QEasingCurve::OutCubic);
        
        // Drop shadow
        auto* shadow = new QGraphicsDropShadowEffect(this);
        shadow->setBlurRadius(15);
        shadow->setXOffset(0);
        shadow->setYOffset(3);
        shadow->setColor(QColor(0, 0, 0, 80));
        setGraphicsEffect(shadow);
    }
    
    qreal hoverProgress() const { return m_hoverProgress; }
    
    void setHoverProgress(qreal progress) {
        m_hoverProgress = progress;
        update();
    }
    
    void setPrimary(bool primary) {
        m_isPrimary = primary;
        update();
    }
    
protected:
    void paintEvent(QPaintEvent*) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        
        QRectF rect = this->rect().adjusted(1, 1, -1, -1);
        QPainterPath path;
        path.addRoundedRect(rect, 8, 8);
        
        // Background gradient
        QLinearGradient gradient(rect.topLeft(), rect.bottomRight());
        
        if (m_isPrimary) {
            gradient.setColorAt(0, QColor("#58a6ff").lighter(100 + m_hoverProgress * 20));
            gradient.setColorAt(1, QColor("#8b5cf6").lighter(100 + m_hoverProgress * 20));
        } else {
            QColor baseColor = QColor("#21262d");
            gradient.setColorAt(0, baseColor.lighter(100 + m_hoverProgress * 30));
            gradient.setColorAt(1, baseColor.lighter(100 + m_hoverProgress * 20));
        }
        
        painter.fillPath(path, gradient);
        
        // Border
        if (!m_isPrimary) {
            painter.setPen(QPen(QColor("#30363d"), 1));
            painter.drawPath(path);
        }
        
        // Text
        painter.setPen(m_isPrimary ? Qt::white : QColor("#f0f6fc"));
        painter.setFont(QFont("Inter", 11, QFont::Medium));
        painter.drawText(rect, Qt::AlignCenter, text());
    }
    
    void enterEvent(QEnterEvent* event) override {
        Q_UNUSED(event)
        m_hoverAnimation->setStartValue(m_hoverProgress);
        m_hoverAnimation->setEndValue(1.0);
        m_hoverAnimation->start();
    }
    
    void leaveEvent(QEvent* event) override {
        Q_UNUSED(event)
        m_hoverAnimation->setStartValue(m_hoverProgress);
        m_hoverAnimation->setEndValue(0.0);
        m_hoverAnimation->start();
    }
    
private:
    qreal m_hoverProgress;
    bool m_isPrimary = false;
    QPropertyAnimation* m_hoverAnimation;
};

// ==================== Modern Statistics Card ====================
class ModernStatsCard : public QWidget {
    Q_OBJECT
    
public:
    explicit ModernStatsCard(const QString& title, QWidget* parent = nullptr)
        : QWidget(parent), m_title(title), m_value("--"), m_trend(0) {
        
        setMinimumSize(200, 100);
        setProperty("card", true);
        
        // Drop shadow effect
        auto* shadow = new QGraphicsDropShadowEffect(this);
        shadow->setBlurRadius(20);
        shadow->setXOffset(0);
        shadow->setYOffset(4);
        shadow->setColor(QColor(0, 0, 0, 60));
        setGraphicsEffect(shadow);
    }
    
    void setValue(const QString& value) {
        m_value = value;
        update();
    }
    
    void setTrend(int trend) {
        m_trend = trend;
        update();
    }
    
    void setIcon(const QPixmap& icon) {
        m_icon = icon;
        update();
    }
    
protected:
    void paintEvent(QPaintEvent*) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        
        // Card background
        QRectF rect = this->rect();
        QPainterPath path;
        path.addRoundedRect(rect, 12, 12);
        
        QLinearGradient bgGradient(rect.topLeft(), rect.bottomLeft());
        bgGradient.setColorAt(0, QColor("#161b22"));
        bgGradient.setColorAt(1, QColor("#0d1117"));
        painter.fillPath(path, bgGradient);
        
        // Border
        painter.setPen(QPen(QColor("#30363d"), 1));
        painter.drawPath(path);
        
        // Icon
        if (!m_icon.isNull()) {
            painter.drawPixmap(16, 16, m_icon.scaled(32, 32, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }
        
        // Title
        painter.setPen(QColor("#8b949e"));
        painter.setFont(QFont("Inter", 10));
        painter.drawText(QRectF(m_icon.isNull() ? 16 : 56, 16, rect.width() - 32, 20), 
                        Qt::AlignLeft, m_title);
        
        // Value
        painter.setPen(QColor("#f0f6fc"));
        painter.setFont(QFont("Inter", 20, QFont::Bold));
        painter.drawText(QRectF(m_icon.isNull() ? 16 : 56, 36, rect.width() - 80, 40), 
                        Qt::AlignLeft, m_value);
        
        // Trend indicator
        if (m_trend != 0) {
            painter.save();
            painter.translate(rect.width() - 40, rect.height() / 2);
            
            QPainterPath arrow;
            if (m_trend > 0) {
                painter.setPen(QPen(QColor("#3fb950"), 2));
                arrow.moveTo(0, 5);
                arrow.lineTo(5, 0);
                arrow.lineTo(10, 5);
            } else {
                painter.setPen(QPen(QColor("#f85149"), 2));
                arrow.moveTo(0, 0);
                arrow.lineTo(5, 5);
                arrow.lineTo(10, 0);
            }
            painter.drawPath(arrow);
            painter.restore();
        }
    }
    
private:
    QString m_title;
    QString m_value;
    int m_trend;
    QPixmap m_icon;
};

// ==================== Modern Animated Loading ====================
class ModernLoadingIndicator : public QWidget {
    Q_OBJECT
    Q_PROPERTY(qreal rotation READ rotation WRITE setRotation)
    
public:
    explicit ModernLoadingIndicator(QWidget* parent = nullptr)
        : QWidget(parent), m_rotation(0) {
        
        setFixedSize(48, 48);
        
        // Rotation animation
        m_animation = new QPropertyAnimation(this, "rotation", this);
        m_animation->setDuration(1000);
        m_animation->setStartValue(0);
        m_animation->setEndValue(360);
        m_animation->setLoopCount(-1);
        m_animation->setEasingCurve(QEasingCurve::Linear);
    }
    
    qreal rotation() const { return m_rotation; }
    
    void setRotation(qreal rotation) {
        m_rotation = rotation;
        update();
    }
    
    void start() {
        m_animation->start();
        show();
    }
    
    void stop() {
        m_animation->stop();
        hide();
    }
    
protected:
    void paintEvent(QPaintEvent*) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        
        painter.translate(24, 24);
        painter.rotate(m_rotation);
        
        // Draw rotating arcs
        QConicalGradient gradient(0, 0, 0);
        gradient.setColorAt(0, QColor("#58a6ff"));
        gradient.setColorAt(0.5, QColor("#8b5cf6"));
        gradient.setColorAt(1, QColor("#58a6ff"));
        
        painter.setPen(QPen(QBrush(gradient), 3, Qt::SolidLine, Qt::RoundCap));
        painter.drawArc(QRectF(-20, -20, 40, 40), 0, 270 * 16);
    }
    
private:
    qreal m_rotation;
    QPropertyAnimation* m_animation;
};

} // namespace ModernWidgets
} // namespace ComponentsForest

#endif // MODERN_CONTROL_WIDGETS_H