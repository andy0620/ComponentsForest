# ComponentsForest QML Camera Viewer

## Overview

This is a modern QML-based UI implementation for the ComponentsForest camera viewer application. It provides a rich, responsive interface while maintaining complete Signal/Slot decoupling with the underlying C++ components.

## Architecture

### Three-Layer Architecture

1. **C++ Components Layer**
   - `BaseComponent`, `CameraComponent`, `Do3ThinkCameraComponent`
   - Business logic and hardware interaction
   - Runs in separate threads

2. **Bridge Layer** (`qml_bridge/`)
   - `CameraBridge`: Exposes camera functionality to QML
   - `MachineBridge`: Manages multiple cameras and machine lifecycle
   - Maintains Signal/Slot decoupling
   - Thread-safe communication

3. **QML UI Layer**
   - Modern Material Design interface
   - Responsive and animated
   - Component-based architecture

## Features

### Camera Management
- Multiple camera support
- Real-time image display with zoom/pan
- Device discovery and connection
- Batch operations (start/stop all)

### Camera Controls
- Exposure time adjustment
- Gain control
- ROI (Region of Interest) configuration
- Software trigger
- Frame saving

### Statistics & Monitoring
- Real-time FPS display
- Frame counter
- Resolution information
- Data rate estimation
- Performance metrics

### User Interface
- Material Design theme
- Dark/Light theme switching
- Customizable colors
- Fullscreen mode
- Responsive layout

## Building

### Prerequisites
- Qt 6.2 or later
- CMake 3.16 or later
- C++17 compiler
- Do3Think SDK (for camera support)

### Build Instructions

```bash
# Create build directory
mkdir build
cd build

# Configure with Qt6
cmake .. -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x.x/gcc_64

# Build
cmake --build . --parallel

# Run
./QMLCameraViewer
```

### Windows Build
```batch
# Use Qt Creator or Visual Studio
cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH=C:/Qt/6.x.x/msvc2019_64
cmake --build . --config Release
```

## Project Structure

```
qml_camera_viewer/
├── main.cpp                    # Application entry point
├── CMakeLists.txt              # Build configuration
├── qml.qrc                     # Resource file
├── qml/
│   ├── main.qml               # Main window
│   ├── components/
│   │   ├── CameraControlPanel.qml
│   │   ├── CameraListDelegate.qml
│   │   ├── CameraParameters.qml
│   │   ├── ConnectionIndicator.qml
│   │   ├── SettingsPanel.qml
│   │   └── StatisticsPanel.qml
│   └── controls/
│       ├── ConnectionControls.qml
│       └── AcquisitionControls.qml
└── README_QML.md

qml_bridge/
├── camera_bridge.h/cpp         # Camera-QML bridge
└── machine_bridge.h/cpp        # Machine-QML bridge
```

## Key Design Patterns

### Bridge Pattern
The bridge classes expose C++ functionality to QML while maintaining decoupling:

```cpp
// C++ Bridge
class CameraBridge : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isConnected READ isConnected NOTIFY connectionStatusChanged)
    // ...
};

// QML Usage
CameraControlPanel {
    cameraBridge: currentCameraBridge
}
```

### Signal/Slot Communication
All component communication uses Qt's Signal/Slot mechanism:

```cpp
// String-based connections for decoupling
connect(component, SIGNAL(frameReady(QImage,FrameMetadata)),
        bridge, SLOT(onComponentFrameReady(QImage,FrameMetadata)));
```

### Property Binding
QML properties automatically update when C++ properties change:

```qml
Label {
    text: cameraBridge ? cameraBridge.fps.toFixed(1) + " fps" : "0.0 fps"
    color: cameraBridge && cameraBridge.fps > 25 ? 
           Material.Green : Material.Orange
}
```

## Customization

### Themes
Modify theme in `main.qml`:
```qml
Material.theme: Material.Dark
Material.primary: Material.Cyan
Material.accent: Material.Pink
```

### Adding New Camera Types
1. Inherit from `CameraComponent`
2. Create corresponding bridge class
3. Register with QML in `main.cpp`

### Custom Controls
Add new QML components in `qml/components/` or `qml/controls/`

## Performance Considerations

- **Image Updates**: Limited to 30 FPS to prevent UI bottlenecks
- **Thread Safety**: All camera operations run in separate threads
- **Memory Management**: Automatic cleanup through Qt parent-child relationships
- **Lazy Loading**: Components created on-demand

## Troubleshooting

### Application Won't Start
- Verify Qt platform plugins are deployed (`platforms/qwindows.dll` on Windows)
- Check QML2_IMPORT_PATH environment variable
- Ensure all Qt dependencies are available

### No Camera Image
- Check camera connection status
- Verify Do3Think SDK is properly linked
- Check debug output for errors

### Performance Issues
- Reduce image update rate in `CameraControlPanel.qml`
- Disable statistics updates when not needed
- Check CPU/GPU usage in task manager

## License

This implementation follows the ComponentsForest architecture guidelines and is designed for industrial automation AOI equipment.

## Support

For issues or questions, refer to the main ComponentsForest documentation or create an issue in the project repository.