# QML Version Quick Start Guide

## Prerequisites
- Qt 6.9.1 with Quick and QuickControls2 modules
- CMake 3.16+
- C++17 compiler
- OpenCV (for preprocessor components)

## Quick Build & Run

### Windows
```batch
# Build
build_qml_viewer.bat

# Run
cd build_qml\viewers\qml_camera_viewer\Release
QmlCameraViewer.exe
```

### Linux/WSL2
```bash
# Build
./build_qml_viewer.sh

# Run
cd build_qml/viewers/qml_camera_viewer
./QmlCameraViewer
```

## Project Structure

```
ComponentsForest/
├── qml_bridge/              # C++/QML Bridge Layer
│   ├── camera_bridge.h/cpp  # Camera component bridge
│   └── machine_bridge.h/cpp # Machine management bridge
│
├── viewers/qml_camera_viewer/
│   ├── main.cpp            # Application entry point
│   ├── CMakeLists.txt      # Build configuration
│   └── qml/                # QML UI files
│       ├── main.qml        # Main window
│       ├── components/     # UI components
│       └── controls/       # Reusable controls
│
└── components/             # Core components (unchanged)
    ├── base_component.*
    ├── camera_component.*
    └── ...
```

## Creating a Custom Camera Bridge

```cpp
// my_camera_bridge.h
#include "qml_bridge/camera_bridge.h"

class MyCameraBridge : public CameraBridge {
    Q_OBJECT
    Q_PROPERTY(int customProperty READ customProperty WRITE setCustomProperty NOTIFY customPropertyChanged)
    
public:
    Q_INVOKABLE void customMethod() {
        // Your implementation
    }
    
signals:
    void customPropertyChanged();
};
```

Register it in main.cpp:
```cpp
qmlRegisterType<MyCameraBridge>("MyModule", 1, 0, "MyCameraBridge");
```

## Using the Bridge in QML

```qml
import QtQuick
import MyModule 1.0

Item {
    MyCameraBridge {
        id: camera
        
        onConnectedChanged: {
            console.log("Camera connected:", connected)
        }
        
        onFrameReady: {
            // Handle new frame
            imageView.source = imageUrl
        }
    }
    
    Button {
        text: "Custom Action"
        onClicked: camera.customMethod()
    }
}
```

## Adding New QML Components

1. Create QML file in `qml/components/`:
```qml
// MyCustomPanel.qml
import QtQuick
import QtQuick.Controls

Item {
    property var cameraBridge: null
    
    // Your UI implementation
}
```

2. Use in main.qml:
```qml
MyCustomPanel {
    cameraBridge: activeCameras[0]
}
```

## Debugging QML

### Enable QML debugging
```cpp
// In main.cpp
qputenv("QML_ENABLE_TEXT_IMAGE_CACHE", "0");
qputenv("QT_LOGGING_RULES", "qt.qml.debug=true");
```

### Use QML profiler
```bash
# Run with profiler
QmlCameraViewer -qmljsdebugger=port:3768,block
```

### Live reload during development
```cpp
// Add to main.cpp for development
#ifdef QT_DEBUG
    engine.rootContext()->setContextProperty("debugMode", true);
    QFileSystemWatcher watcher;
    watcher.addPath("qml/main.qml");
    QObject::connect(&watcher, &QFileSystemWatcher::fileChanged,
        [&engine]() { engine.load(QUrl("qml/main.qml")); });
#endif
```

## Common Patterns

### Exposing C++ Objects to QML
```cpp
// C++ side
class MyObject : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
public:
    QString status() const { return m_status; }
signals:
    void statusChanged();
private:
    QString m_status;
};

// Register in main.cpp
engine.rootContext()->setContextProperty("myObject", myObjectInstance);
```

### QML side
```qml
Text {
    text: myObject.status
}
```

### Handling Images from C++
```cpp
// In bridge class
Q_PROPERTY(QString imageUrl READ imageUrl NOTIFY imageUrlChanged)

QString imageUrl() const {
    return QString("image://camera/%1").arg(m_imageId);
}

// Update image
void updateImage(const QImage& image) {
    m_imageProvider->updateImage(m_imageId, image);
    emit imageUrlChanged();
}
```

### QML Image display
```qml
Image {
    source: cameraBridge.imageUrl
    cache: false  // Important for live updates
}
```

## Styling and Theming

### Create a theme file
```qml
// themes/IndustrialTheme.qml
pragma Singleton
import QtQuick

QtObject {
    property color primary: "#2196F3"
    property color accent: "#FF5722"
    property color background: "#121212"
    property color surface: "#1E1E1E"
    property color text: "#FFFFFF"
    property int borderRadius: 4
    property int spacing: 8
}
```

### Register as singleton
```cpp
qmlRegisterSingletonType(QUrl("qrc:/themes/IndustrialTheme.qml"),
    "Themes", 1, 0, "Theme");
```

### Use in QML
```qml
import Themes 1.0

Rectangle {
    color: Theme.background
    radius: Theme.borderRadius
}
```

## Performance Tips

1. **Use Loader for heavy components**:
```qml
Loader {
    active: tabBar.currentIndex === 0
    sourceComponent: CameraControlPanel {}
}
```

2. **Optimize bindings**:
```qml
// Bad - evaluates every frame
Text {
    text: Math.random() > 0.5 ? "A" : "B"
}

// Good - only when property changes
Text {
    text: camera.connected ? "Connected" : "Disconnected"
}
```

3. **Use ListView for multiple cameras**:
```qml
ListView {
    model: cameraBridges
    delegate: CameraControlPanel {
        cameraBridge: modelData
    }
}
```

## Troubleshooting

| Issue | Solution |
|-------|----------|
| QML file not found | Check qml.qrc resource file |
| Property not updating | Ensure NOTIFY signal is emitted |
| Slow performance | Profile with QML profiler |
| Bridge not available | Check qmlRegisterType call |
| Images not displaying | Verify image provider registration |

## Next Steps

1. Review `QML_VS_QWIDGET_COMPARISON.md` for architecture details
2. Study bridge classes in `qml_bridge/` directory
3. Explore QML components in `viewers/qml_camera_viewer/qml/`
4. Read Qt Quick documentation for advanced features