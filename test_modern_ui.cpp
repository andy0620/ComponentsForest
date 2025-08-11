/**
 * @file test_modern_ui.cpp
 * @brief Test application to demonstrate the modern rounded button UI
 */

#include <QApplication>
#include <QMainWindow>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QComboBox>
#include <QSpinBox>
#include <QSlider>
#include <QCheckBox>
#include <QRadioButton>
#include <QToolBar>
#include <QMenuBar>
#include <QStatusBar>

// Include the enhanced Material theme
const QString MODERN_THEME = R"(
/* Modern Rounded Material Theme */
QPushButton {
    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
                                stop: 0 #35363F, stop: 1 #2C2D36);
    color: #FFFFFF;
    border: 1px solid rgba(74, 158, 255, 0.3);
    border-radius: 20px;
    padding: 14px 28px;
    font-weight: 600;
    font-size: 14px;
    min-width: 120px;
    min-height: 44px;
    text-transform: uppercase;
    letter-spacing: 0.5px;
}

QPushButton:hover {
    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
                                stop: 0 #3D3E48, stop: 1 #35363F);
    border: 1px solid rgba(74, 158, 255, 0.6);
    color: #4A9EFF;
}

QPushButton:pressed {
    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
                                stop: 0 #25262E, stop: 1 #2C2D36);
    border: 1px solid rgba(74, 158, 255, 0.8);
}

QPushButton[primary="true"] {
    background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 1,
                                stop: 0 #7DB8FF, stop: 0.5 #4A9EFF, stop: 1 #3D82E4);
    color: #FFFFFF;
    font-weight: 700;
    border: none;
    border-radius: 22px;
    text-shadow: 0 1px 2px rgba(0, 0, 0, 0.2);
}

QPushButton[primary="true"]:hover {
    background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 1,
                                stop: 0 #8FC4FF, stop: 0.5 #5AAAFF, stop: 1 #4A92EA);
}

QWidget {
    background-color: #1E1F26;
    color: #FFFFFF;
    font-family: "Inter", "Segoe UI", "Roboto", sans-serif;
    font-size: 14px;
}

QGroupBox {
    background: #25262E;
    border: 1px solid #3A3B45;
    border-radius: 16px;
    margin-top: 20px;
    padding-top: 20px;
    font-weight: 600;
}

QComboBox, QSpinBox {
    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
                                stop: 0 #35363F, stop: 1 #2C2D36);
    color: #FFFFFF;
    border: 1px solid rgba(74, 158, 255, 0.2);
    border-radius: 16px;
    padding: 12px;
    min-height: 40px;
}

QCheckBox::indicator {
    width: 20px;
    height: 20px;
    border-radius: 6px;
    border: 2px solid #6B7280;
    background: #2C2D36;
}

QSlider::handle:horizontal {
    background: #4A9EFF;
    width: 20px;
    height: 20px;
    margin: -8px 0;
    border-radius: 10px;
}
)";

class ModernUIDemo : public QMainWindow {
public:
    ModernUIDemo() {
        setWindowTitle("Modern Rounded UI Demo - ComponentsForest");
        resize(800, 600);
        
        // Apply modern theme
        setStyleSheet(MODERN_THEME);
        
        // Create central widget
        QWidget* central = new QWidget(this);
        setCentralWidget(central);
        
        QVBoxLayout* mainLayout = new QVBoxLayout(central);
        mainLayout->setSpacing(20);
        mainLayout->setContentsMargins(30, 30, 30, 30);
        
        // Title
        QLabel* title = new QLabel("Modern Rounded Button UI Demo");
        title->setAlignment(Qt::AlignCenter);
        title->setStyleSheet("font-size: 24px; font-weight: bold; color: #4A9EFF; margin-bottom: 20px;");
        mainLayout->addWidget(title);
        
        // Primary buttons group
        QGroupBox* primaryGroup = new QGroupBox("Primary Actions");
        QHBoxLayout* primaryLayout = new QHBoxLayout(primaryGroup);
        
        QPushButton* startBtn = new QPushButton("START");
        startBtn->setProperty("primary", true);
        primaryLayout->addWidget(startBtn);
        
        QPushButton* stopBtn = new QPushButton("STOP");
        stopBtn->setProperty("primary", true);
        primaryLayout->addWidget(stopBtn);
        
        QPushButton* captureBtn = new QPushButton("CAPTURE");
        captureBtn->setProperty("primary", true);
        primaryLayout->addWidget(captureBtn);
        
        mainLayout->addWidget(primaryGroup);
        
        // Secondary buttons group
        QGroupBox* secondaryGroup = new QGroupBox("Secondary Actions");
        QHBoxLayout* secondaryLayout = new QHBoxLayout(secondaryGroup);
        
        QPushButton* settingsBtn = new QPushButton("Settings");
        secondaryLayout->addWidget(settingsBtn);
        
        QPushButton* exportBtn = new QPushButton("Export");
        secondaryLayout->addWidget(exportBtn);
        
        QPushButton* helpBtn = new QPushButton("Help");
        secondaryLayout->addWidget(helpBtn);
        
        mainLayout->addWidget(secondaryGroup);
        
        // Controls group
        QGroupBox* controlsGroup = new QGroupBox("Modern Controls");
        QVBoxLayout* controlsLayout = new QVBoxLayout(controlsGroup);
        
        QComboBox* combo = new QComboBox();
        combo->addItems({"Option 1", "Option 2", "Option 3"});
        controlsLayout->addWidget(combo);
        
        QSpinBox* spin = new QSpinBox();
        spin->setRange(0, 100);
        spin->setValue(50);
        controlsLayout->addWidget(spin);
        
        QSlider* slider = new QSlider(Qt::Horizontal);
        slider->setRange(0, 100);
        slider->setValue(50);
        controlsLayout->addWidget(slider);
        
        QCheckBox* check = new QCheckBox("Enable advanced features");
        controlsLayout->addWidget(check);
        
        mainLayout->addWidget(controlsGroup);
        
        mainLayout->addStretch();
        
        // Status bar
        statusBar()->showMessage("Modern UI with rounded buttons - QML-like appearance achieved!");
    }
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    ModernUIDemo window;
    window.show();
    
    return app.exec();
}

#include "test_modern_ui.moc"