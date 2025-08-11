/**
 * @file main.cpp
 * @brief Main entry point for QML Camera Viewer application
 * 
 * This application provides a modern QML-based interface for camera control
 * using the ComponentsForest architecture with proper thread separation.
 */

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QThread>
#include <QLoggingCategory>
#include <QCommandLineParser>
#include <QFontDatabase>
#include <QQuickWindow>
#include <QSurfaceFormat>
#include <QDir>
#include <QStandardPaths>
#include <QDateTime>
#include <QFile>
#include <QIcon>
#include <memory>

// Bridge classes
#include "../../qml_bridge/camera_bridge.h"
#include "../../qml_bridge/machine_bridge.h"

// Core components  
#include "../do3think_camera_viewer/machine.h"

// Use the ComponentsForest namespace
using namespace ComponentsForest;

// Image provider
#include "qml_image_provider.h"

// Qt6 has moved QQuickStyle to QtQuick/QQuickStyle in Qt 6.9
// Using the proper Qt6 include path

// Enable logging categories
Q_LOGGING_CATEGORY(mainCategory, "ComponentsForest.QMLViewer.Main")
Q_LOGGING_CATEGORY(performanceCategory, "ComponentsForest.QMLViewer.Performance")

/**
 * @brief Configure application settings and metadata
 */
void configureApplication(QGuiApplication& app) {
    app.setOrganizationName("ComponentsForest");
    app.setOrganizationDomain("componentsforest.io");
    app.setApplicationName("QML Camera Viewer");
    app.setApplicationVersion("1.0.0");
    
    // Set application icon if available
    QIcon appIcon(":/icons/app_icon.png");
    if (!appIcon.isNull()) {
        app.setWindowIcon(appIcon);
    }
}

/**
 * @brief Configure Qt Quick settings for optimal performance
 */
void configureQuickSettings() {
    // Set OpenGL rendering for better performance
    QSurfaceFormat format;
    format.setRenderableType(QSurfaceFormat::OpenGL);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setVersion(3, 3);
    format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
    format.setSamples(4); // Anti-aliasing
    QSurfaceFormat::setDefaultFormat(format);
    
    // Configure Quick window settings
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);
    
    // Enable threaded rendering for better performance
    qputenv("QSG_RENDER_LOOP", "threaded");
    
    // Set Material style for industrial look
    QQuickStyle::setStyle("Material");
    
    // Configure Material theme
    qputenv("QT_QUICK_CONTROLS_MATERIAL_THEME", "Dark");
    qputenv("QT_QUICK_CONTROLS_MATERIAL_ACCENT", "#2196F3");
    qputenv("QT_QUICK_CONTROLS_MATERIAL_PRIMARY", "#1976D2");
    qputenv("QT_QUICK_CONTROLS_MATERIAL_FOREGROUND", "#FFFFFF");
    qputenv("QT_QUICK_CONTROLS_MATERIAL_BACKGROUND", "#121212");
}

/**
 * @brief Configure logging based on command line arguments
 */
void configureLogging(bool verbose, bool debug) {
    QString rules;
    
    if (debug) {
        rules = "ComponentsForest.*=true\n"
                "qt.qml.debug=true\n"
                "qt.quick.debug=true";
    } else if (verbose) {
        rules = "ComponentsForest.*=true\n"
                "qt.qml.warning=true\n"
                "qt.quick.warning=true";
    } else {
        rules = "ComponentsForest.*.warning=true\n"
                "ComponentsForest.*.critical=true\n"
                "qt.qml.warning=true";
    }
    
    QLoggingCategory::setFilterRules(rules);
    
    // Set up custom message handler for formatted output
    qInstallMessageHandler([](QtMsgType type, const QMessageLogContext& context, const QString& msg) {
        const QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
        const QString category = context.category ? context.category : "default";
        
        QString level;
        switch (type) {
            case QtDebugMsg:    level = "DEBUG"; break;
            case QtInfoMsg:     level = "INFO "; break;
            case QtWarningMsg:  level = "WARN "; break;
            case QtCriticalMsg: level = "ERROR"; break;
            case QtFatalMsg:    level = "FATAL"; break;
        }
        
        fprintf(stderr, "[%s] [%s] [%s] %s\n",
                timestamp.toLocal8Bit().constData(),
                level.toLocal8Bit().constData(),
                category,
                msg.toLocal8Bit().constData());
        
        if (type == QtFatalMsg) {
            abort();
        }
    });
}

/**
 * @brief Parse command line arguments
 */
struct CommandLineOptions {
    bool verbose = false;
    bool debug = false;
    bool fullscreen = false;
    QString configFile;
    int maxCameras = 4;
};

CommandLineOptions parseCommandLine(QGuiApplication& app) {
    QCommandLineParser parser;
    parser.setApplicationDescription("QML Camera Viewer - Modern camera control interface");
    parser.addHelpOption();
    parser.addVersionOption();
    
    // Add options
    QCommandLineOption verboseOption(QStringList() << "v" << "verbose",
                                     "Enable verbose output");
    parser.addOption(verboseOption);
    
    QCommandLineOption debugOption(QStringList() << "d" << "debug",
                                  "Enable debug output");
    parser.addOption(debugOption);
    
    QCommandLineOption fullscreenOption(QStringList() << "f" << "fullscreen",
                                        "Start in fullscreen mode");
    parser.addOption(fullscreenOption);
    
    QCommandLineOption configOption(QStringList() << "c" << "config",
                                   "Configuration file path",
                                   "file");
    parser.addOption(configOption);
    
    QCommandLineOption maxCamerasOption(QStringList() << "m" << "max-cameras",
                                        "Maximum number of cameras (default: 4)",
                                        "number", "4");
    parser.addOption(maxCamerasOption);
    
    parser.process(app);
    
    CommandLineOptions options;
    options.verbose = parser.isSet(verboseOption);
    options.debug = parser.isSet(debugOption);
    options.fullscreen = parser.isSet(fullscreenOption);
    options.configFile = parser.value(configOption);
    
    bool ok;
    int maxCams = parser.value(maxCamerasOption).toInt(&ok);
    if (ok && maxCams > 0 && maxCams <= 16) {
        options.maxCameras = maxCams;
    }
    
    return options;
}

/**
 * @brief Register QML types and singletons
 */
void registerQmlTypes(QQmlApplicationEngine& engine, MachineBridge* machineBridge) {
    // Register bridge types
    qmlRegisterType<CameraBridge>("ComponentsForest", 1, 0, "CameraBridge");
    qmlRegisterSingletonInstance("ComponentsForest", 1, 0, "MachineBridge", machineBridge);
    
    // Register enums for QML access
    qmlRegisterUncreatableMetaObject(
        CameraBridge::staticMetaObject,
        "ComponentsForest",
        1, 0,
        "CameraState",
        "Access to CameraState enum"
    );
    
    // Register image provider for efficient image display
    engine.addImageProvider("camera", new QmlImageProvider());
    
    // Set context properties
    engine.rootContext()->setContextProperty("appVersion", QGuiApplication::applicationVersion());
    engine.rootContext()->setContextProperty("qtVersion", qVersion());
}

/**
 * @brief Initialize and configure the machine
 */
std::unique_ptr<Do3ThinkCameraMachine> createAndConfigureMachine(int maxCameras) {
    auto machine = std::make_unique<Do3ThinkCameraMachine>(nullptr);
    
    // Configure default camera settings
    QVariantMap config;
    config["maxCameras"] = maxCameras;
    config["autoReconnect"] = true;
    config["reconnectInterval"] = 5000;
    config["performanceMonitoring"] = true;
    config["debugMode"] = false;
    
    // Set default configuration for cameras
    machine->setDefaultCameraConfig(config);
    
    return machine;
}

/**
 * @brief Set up connections between machine and bridge
 */
void setupConnections(Do3ThinkCameraMachine* machine, MachineBridge* bridge) {
    // Connect machine signals to bridge
    QObject::connect(machine, &Do3ThinkCameraMachine::cameraAdded,
                     bridge, &MachineBridge::onComponentAdded);
    
    QObject::connect(machine, &Do3ThinkCameraMachine::cameraRemoved,
                     bridge, &MachineBridge::onComponentRemoved);
    
    QObject::connect(machine, &Do3ThinkCameraMachine::machineStarted,
                     bridge, [bridge]() { bridge->onMachineStateChanged("started"); });
    
    QObject::connect(machine, &Do3ThinkCameraMachine::machineStopped,
                     bridge, [bridge]() { bridge->onMachineStateChanged("stopped"); });
    
    // Connect bridge requests to machine
    QObject::connect(bridge, &MachineBridge::startAllRequested,
                     machine, &Do3ThinkCameraMachine::startAllCameras);
    
    QObject::connect(bridge, &MachineBridge::stopAllRequested,
                     machine, &Do3ThinkCameraMachine::stopAllCameras);
    
    QObject::connect(bridge, &MachineBridge::refreshRequested,
                     machine, &Do3ThinkCameraMachine::refreshDeviceList);
}

/**
 * @brief Main application entry point
 */
int main(int argc, char *argv[])
{
    // Qt 6.9 has automatic high DPI support, these attributes are deprecated
    // High DPI scaling is enabled by default in Qt6
    
    // Create application instance
    QGuiApplication app(argc, argv);
    
    // Configure application metadata
    configureApplication(app);
    
    // Parse command line options
    CommandLineOptions options = parseCommandLine(app);
    
    // Configure logging
    configureLogging(options.verbose, options.debug);
    
    qCInfo(mainCategory) << "Starting QML Camera Viewer" << app.applicationVersion();
    qCInfo(mainCategory) << "Qt Version:" << qVersion();
    qCInfo(mainCategory) << "Max Cameras:" << options.maxCameras;
    
    // Configure Qt Quick settings
    configureQuickSettings();
    
    // Create machine thread
    auto machineThread = std::make_unique<QThread>();
    machineThread->setObjectName("MachineThread");
    
    // Create machine
    auto machine = createAndConfigureMachine(options.maxCameras);
    if (!machine) {
        qCCritical(mainCategory) << "Failed to create machine";
        return -1;
    }
    
    // Move machine to its thread
    machine->moveToThread(machineThread.get());
    
    // Create machine bridge (stays in main thread for QML access)
    auto machineBridge = std::make_unique<MachineBridge>(nullptr);
    
    // Setup connections
    setupConnections(machine.get(), machineBridge.get());
    
    // Start machine thread
    QObject::connect(machineThread.get(), &QThread::started,
                     machine.get(), &Do3ThinkCameraMachine::start);
    
    QObject::connect(machineThread.get(), &QThread::finished,
                     machine.get(), &Do3ThinkCameraMachine::stop);
    
    machineThread->start();
    
    // Create QML engine
    QQmlApplicationEngine engine;
    
    // Register QML types
    registerQmlTypes(engine, machineBridge.get());
    
    // Handle QML loading errors
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
                     &app, []() { 
                         qCCritical(mainCategory) << "QML loading failed!";
                         QCoreApplication::exit(-1); 
                     }, Qt::QueuedConnection);
    
    // Load configuration file if specified
    if (!options.configFile.isEmpty()) {
        QFile configFile(options.configFile);
        if (configFile.exists()) {
            qCInfo(mainCategory) << "Loading configuration from:" << options.configFile;
            engine.rootContext()->setContextProperty("configFile", options.configFile);
        } else {
            qCWarning(mainCategory) << "Configuration file not found:" << options.configFile;
        }
    }
    
    // Set fullscreen mode if requested
    engine.rootContext()->setContextProperty("startFullscreen", options.fullscreen);
    
    // Load main QML file
    // First try to load from resources, then from file system for development
    QUrl url(QStringLiteral("qrc:/qml/main.qml"));
    if (!QFile::exists(url.toString())) {
        // Try loading from file system for development
        QString qmlPath = QDir::currentPath() + "/qml/main.qml";
        if (QFile::exists(qmlPath)) {
            url = QUrl::fromLocalFile(qmlPath);
            qCInfo(mainCategory) << "Loading QML from file system:" << qmlPath;
        } else {
            qCCritical(mainCategory) << "Cannot find main.qml in resources or file system";
            return -1;
        }
    }
    engine.load(url);
    
    if (engine.rootObjects().isEmpty()) {
        qCCritical(mainCategory) << "No root objects loaded from QML";
        return -1;
    }
    
    // Handle application shutdown
    QObject::connect(&app, &QGuiApplication::aboutToQuit, [&]() {
        qCInfo(mainCategory) << "Application shutting down...";
        
        // Stop machine
        if (machine) {
            machine->stop();
        }
        
        // Stop and wait for machine thread
        if (machineThread && machineThread->isRunning()) {
            machineThread->quit();
            if (!machineThread->wait(5000)) {
                qCWarning(mainCategory) << "Machine thread did not stop gracefully";
                machineThread->terminate();
                machineThread->wait();
            }
        }
        
        qCInfo(mainCategory) << "Shutdown complete";
    });
    
    // Run application event loop
    int result = app.exec();
    
    qCInfo(mainCategory) << "Application exited with code:" << result;
    return result;
}