/**
 * @file futuristic_widgets.h
 * @brief Futuristic Custom Widget Implementations for Cyberpunk UI
 */

#ifndef FUTURISTIC_WIDGETS_H
#define FUTURISTIC_WIDGETS_H

#include <QWidget>
#include <QPushButton>
#include <QSlider>
#include <QDial>
#include <QProgressBar>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QSequentialAnimationGroup>
#include <QGraphicsDropShadowEffect>
#include <QGraphicsBlurEffect>
#include <QTimer>
#include <QMouseEvent>
#include <QEnterEvent>
#include <QVector>
#include <QPointF>
#include <memory>
#include <vector>
#include <random>

namespace ComponentsForest {
namespace FuturisticWidgets {

// ==================== Holographic Toggle Switch ====================
class HolographicToggle : public QWidget {
    Q_OBJECT
    Q_PROPERTY(qreal switchPosition READ switchPosition WRITE setSwitchPosition)
    Q_PROPERTY(qreal glowIntensity READ glowIntensity WRITE setGlowIntensity)
    
public:
    explicit HolographicToggle(QWidget* parent = nullptr);
    
    bool isChecked() const { return m_checked; }
    qreal switchPosition() const { return m_switchPos; }
    qreal glowIntensity() const { return m_glowIntensity; }
    
public slots:
    void setChecked(bool checked);
    void setSwitchPosition(qreal pos);
    void setGlowIntensity(qreal intensity);
    void toggle();
    
signals:
    void toggled(bool checked);
    void clicked();
    
protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
    
private:
    void animateSwitch();
    void drawHolographicTrack(QPainter* painter);
    void drawGlowingHandle(QPainter* painter);
    void drawEnergyField(QPainter* painter);
    
    bool m_checked = false;
    qreal m_switchPos = 0.0;
    qreal m_glowIntensity = 0.0;
    QPropertyAnimation* m_posAnimation;
    QPropertyAnimation* m_glowAnimation;
    QTimer* m_energyTimer;
    int m_energyPhase = 0;
    bool m_isHovered = false;
};

// ==================== Neon Glow Button ====================
class NeonGlowButton : public QPushButton {
    Q_OBJECT
    Q_PROPERTY(qreal glowRadius READ glowRadius WRITE setGlowRadius)
    Q_PROPERTY(qreal pulsePhase READ pulsePhase WRITE setPulsePhase)
    Q_PROPERTY(QColor glowColor READ glowColor WRITE setGlowColor)
    
public:
    explicit NeonGlowButton(const QString& text, QWidget* parent = nullptr);
    
    qreal glowRadius() const { return m_glowRadius; }
    qreal pulsePhase() const { return m_pulsePhase; }
    QColor glowColor() const { return m_glowColor; }
    
    void setNeonColor(const QColor& color);
    void enablePulse(bool enable);
    void setGlowIntensity(int intensity); // 0-10
    
public slots:
    void setGlowRadius(qreal radius);
    void setPulsePhase(qreal phase);
    void setGlowColor(const QColor& color);
    void startGlowAnimation();
    void stopGlowAnimation();
    
protected:
    void paintEvent(QPaintEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
    
private:
    void drawNeonBorder(QPainter* painter);
    void drawGlowEffect(QPainter* painter);
    void drawHolographicShimmer(QPainter* painter);
    
    qreal m_glowRadius = 0.0;
    qreal m_pulsePhase = 0.0;
    QColor m_glowColor;
    QPropertyAnimation* m_glowAnimation;
    QPropertyAnimation* m_pulseAnimation;
    QParallelAnimationGroup* m_animationGroup;
    bool m_isPulsing = true;
    int m_glowIntensity = 5;
};

// ==================== Cyberpunk Progress Ring ====================
class CyberpunkProgressRing : public QWidget {
    Q_OBJECT
    Q_PROPERTY(int value READ value WRITE setValue)
    Q_PROPERTY(qreal rotationAngle READ rotationAngle WRITE setRotationAngle)
    Q_PROPERTY(qreal glowPulse READ glowPulse WRITE setGlowPulse)
    
public:
    explicit CyberpunkProgressRing(QWidget* parent = nullptr);
    
    int value() const { return m_value; }
    qreal rotationAngle() const { return m_rotation; }
    qreal glowPulse() const { return m_glowPulse; }
    
    void setRange(int min, int max);
    void setColors(const QColor& primary, const QColor& secondary);
    void enableAnimation(bool enable);
    void setLabel(const QString& label);
    
public slots:
    void setValue(int value);
    void setRotationAngle(qreal angle);
    void setGlowPulse(qreal pulse);
    void reset();
    
signals:
    void valueChanged(int value);
    
protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    
private:
    void drawOuterRing(QPainter* painter);
    void drawProgressArc(QPainter* painter);
    void drawInnerDisplay(QPainter* painter);
    void drawDataPoints(QPainter* painter);
    void drawHolographicGrid(QPainter* painter);
    
    int m_value = 0;
    int m_minimum = 0;
    int m_maximum = 100;
    qreal m_rotation = 0.0;
    qreal m_glowPulse = 1.0;
    QString m_label;
    QColor m_primaryColor;
    QColor m_secondaryColor;
    QPropertyAnimation* m_rotationAnimation;
    QPropertyAnimation* m_pulseAnimation;
    QTimer* m_dataTimer;
    QVector<QPointF> m_dataPoints;
};

// ==================== Holographic Slider ====================
class HolographicSlider : public QSlider {
    Q_OBJECT
    Q_PROPERTY(qreal glowPosition READ glowPosition WRITE setGlowPosition)
    Q_PROPERTY(qreal wavePhase READ wavePhase WRITE setWavePhase)
    
public:
    explicit HolographicSlider(Qt::Orientation orientation, QWidget* parent = nullptr);
    
    qreal glowPosition() const;
    qreal wavePhase() const;
    
    void setNeonColors(const QColor& start, const QColor& end);
    void enableWaveAnimation(bool enable);
    void setTrackStyle(const QString& style); // "neon", "plasma", "energy"
    
public slots:
    void setGlowPosition(qreal pos);
    void setWavePhase(qreal phase);
    
signals:
    void glowPositionChanged();
    void wavePhaseChanged();
    
protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    
private:
    void drawHolographicTrack(QPainter* painter);
    void drawEnergyWave(QPainter* painter);
    void drawNeonHandle(QPainter* painter);
    void drawValueIndicator(QPainter* painter);
    
    qreal m_glowPosition = 0.0;
    qreal m_wavePhase = 0.0;
    QColor m_startColor;
    QColor m_endColor;
    QString m_trackStyle;
    QPropertyAnimation* m_glowAnimation;
    QPropertyAnimation* m_waveAnimation;
    bool m_isDragging = false;
    QPoint m_lastPos;
};

// ==================== Matrix Data Stream Widget ====================
class MatrixDataStream : public QWidget {
    Q_OBJECT
    
public:
    explicit MatrixDataStream(QWidget* parent = nullptr);
    
    void setStreamSpeed(int speed); // 1-10
    void setDataDensity(int density); // 1-10
    void setTextMode(bool binary); // true for binary, false for characters
    void startStream();
    void stopStream();
    
protected:
    void paintEvent(QPaintEvent* event) override;
    
private:
    struct DataColumn {
        int x;
        int y;
        int speed;
        QString data;
        QColor color;
        qreal opacity;
    };
    
    void updateColumns();
    void generateColumnData(DataColumn& column);
    
    QTimer* m_updateTimer;
    std::vector<DataColumn> m_columns;
    int m_streamSpeed = 5;
    int m_dataDensity = 5;
    bool m_binaryMode = false;
    std::mt19937 m_randomGen;
};

// ==================== Futuristic Gauge ====================
class FuturisticGauge : public QWidget {
    Q_OBJECT
    Q_PROPERTY(qreal value READ value WRITE setValue)
    Q_PROPERTY(qreal needleAngle READ needleAngle WRITE setNeedleAngle)
    
public:
    explicit FuturisticGauge(QWidget* parent = nullptr);
    
    qreal value() const;
    qreal needleAngle() const;
    
    void setRange(qreal min, qreal max);
    void setUnit(const QString& unit);
    void setTitle(const QString& title);
    void setWarningThreshold(qreal threshold);
    void setCriticalThreshold(qreal threshold);
    void setColors(const QColor& normal, const QColor& warning, const QColor& critical);
    
public slots:
    void setValue(qreal value);
    void setNeedleAngle(qreal angle);
    void animateToValue(qreal value);
    
signals:
    void valueChanged(qreal value);
    void needleAngleChanged();
    void warningThreshold();
    void criticalThreshold();
    
protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    
private:
    void drawOuterRing(QPainter* painter);
    void drawScaleMarks(QPainter* painter);
    void drawNeonArc(QPainter* painter);
    void drawNeedle(QPainter* painter);
    void drawCenterHub(QPainter* painter);
    void drawDigitalDisplay(QPainter* painter);
    void drawWarningIndicators(QPainter* painter);
    
    qreal m_value = 0.0;
    qreal m_minimum = 0.0;
    qreal m_maximum = 100.0;
    qreal m_needleAngle = -135.0;
    qreal m_warningThreshold = 75.0;
    qreal m_criticalThreshold = 90.0;
    QString m_unit;
    QString m_title;
    QColor m_normalColor;
    QColor m_warningColor;
    QColor m_criticalColor;
    QPropertyAnimation* m_needleAnimation;
    QTimer* m_warningTimer;
    bool m_warningActive = false;
};

// ==================== Holographic Status Card ====================
class HolographicStatusCard : public QWidget {
    Q_OBJECT
    Q_PROPERTY(qreal animationPhase READ animationPhase WRITE setAnimationPhase)
    
public:
    explicit HolographicStatusCard(const QString& title, QWidget* parent = nullptr);
    
    qreal animationPhase() const;
    
    void setValue(const QString& value);
    void setSubtext(const QString& subtext);
    void setIcon(const QPixmap& icon);
    void setStatus(const QString& status); // "online", "offline", "warning", "error"
    void setTrend(qreal percentage); // -100 to +100
    
public slots:
    void setAnimationPhase(qreal phase);
    void pulse();
    
signals:
    void animationPhaseChanged();
    
protected:
    void paintEvent(QPaintEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
    
private:
    void drawHolographicFrame(QPainter* painter);
    void drawStatusIndicator(QPainter* painter);
    void drawTrendGraph(QPainter* painter);
    void drawGlowEffect(QPainter* painter);
    
    QString m_title;
    QString m_value;
    QString m_subtext;
    QString m_status = "online";
    QPixmap m_icon;
    qreal m_trend = 0.0;
    qreal m_animationPhase = 0.0;
    QPropertyAnimation* m_phaseAnimation;
    QVector<qreal> m_trendData;
    bool m_isHovered = false;
};

// ==================== Cyber Terminal Widget ====================
class CyberTerminal : public QWidget {
    Q_OBJECT
    
public:
    explicit CyberTerminal(QWidget* parent = nullptr);
    
    void appendLine(const QString& text, const QColor& color = QColor("#00FF88"));
    void setPrompt(const QString& prompt);
    void clear();
    void enableScanlines(bool enable);
    void setGlowEffect(bool enable);
    
signals:
    void commandEntered(const QString& command);
    
protected:
    void paintEvent(QPaintEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    
private:
    void drawScanlines(QPainter* painter);
    void drawTextWithGlow(QPainter* painter);
    void scrollToBottom();
    
    QStringList m_lines;
    QString m_currentInput;
    QString m_prompt = "> ";
    int m_cursorPosition = 0;
    bool m_cursorVisible = true;
    QTimer* m_cursorTimer;
    QTimer* m_scanlineTimer;
    int m_scanlineOffset = 0;
    bool m_scanlinesEnabled = true;
    bool m_glowEnabled = true;
    int m_maxLines = 100;
};

// ==================== Particle Effect Overlay ====================
class ParticleEffectOverlay : public QWidget {
    Q_OBJECT
    
public:
    explicit ParticleEffectOverlay(QWidget* parent = nullptr);
    
    void setParticleCount(int count);
    void setParticleColor(const QColor& color);
    void setParticleSpeed(qreal speed);
    void setEffectType(const QString& type); // "stars", "matrix", "energy", "sparks"
    void start();
    void stop();
    
protected:
    void paintEvent(QPaintEvent* event) override;
    
private:
    struct Particle {
        QPointF position;
        QPointF velocity;
        qreal size;
        qreal opacity;
        qreal life;
        QColor color;
    };
    
    void updateParticles();
    void spawnParticle(Particle& particle);
    void drawParticle(QPainter* painter, const Particle& particle);
    
    QTimer* m_updateTimer;
    std::vector<Particle> m_particles;
    int m_particleCount = 50;
    QColor m_particleColor;
    qreal m_particleSpeed = 1.0;
    QString m_effectType = "energy";
    std::mt19937 m_randomGen;
};

} // namespace FuturisticWidgets
} // namespace ComponentsForest

#endif // FUTURISTIC_WIDGETS_H