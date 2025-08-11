# QML vs QWidget Implementation Comparison

## Overview
ComponentsForest now provides two UI implementations while maintaining the same core architecture:
- **QWidget Version**: Traditional C++ widgets (original)
- **QML Version**: Modern declarative UI (new)

Both versions use identical component logic (BaseComponent → CameraComponent → Do3ThinkCameraComponent).

## Architecture Comparison

### QWidget Version
```
MainUI (QWidget) → CameraControlPanel (QWidget) → Do3ThinkCameraComponent
                                                    ↑
                                          Direct Signal/Slot
```

### QML Version
```
main.qml → CameraControlPanel.qml → CameraBridge → Do3ThinkCameraComponent
                                         ↑
                              Bridge Layer (NEW)
```

## Key Differences

| Aspect | QWidget | QML |
|--------|---------|-----|
| **UI Definition** | C++ code | Declarative QML files |
| **Component Access** | Direct Signal/Slot | Via Bridge Classes |
| **Styling** | C++ stylesheets | QML property bindings |
| **Animation** | QPropertyAnimation | QML animations |
| **Performance** | CPU rendering | GPU accelerated |
| **Development** | Compile for changes | Hot reload support |
| **File Size** | Larger executable | Smaller exe + QML files |
| **Touch Support** | Basic | Native support |

## File Structure

### QWidget Version
```
viewers/do3think_camera_viewer/
├── main_ui.cpp/h         # Main window
├── machine.cpp/h         # Component management
└── CMakeLists.txt
```

### QML Version
```
viewers/qml_camera_viewer/
├── main.cpp              # Application entry
├── qml/                  # UI files
│   ├── main.qml
│   └── components/
└── CMakeLists.txt

qml_bridge/               # Bridge layer (new)
├── camera_bridge.cpp/h
└── machine_bridge.cpp/h
```

## Build Commands

### QWidget Version
```bash
# Windows
build_viewer_with_preprocessor.bat

# Linux
mkdir build && cd build
cmake .. -DBUILD_VIEWERS=ON
make -j$(nproc)
```

### QML Version
```bash
# Windows
build_qml_viewer.bat

# Linux
./build_qml_viewer.sh
```

## Feature Parity

Both versions support:
- ✅ Multiple camera connections
- ✅ Real-time image display
- ✅ Parameter control (exposure, gain, etc.)
- ✅ ROI selection
- ✅ Statistics display
- ✅ Device discovery
- ✅ Configuration save/load
- ✅ Thread-safe operation

## Performance Characteristics

### QWidget Version
- **Pros**: 
  - Lower memory usage
  - Faster startup time
  - Better for simple UIs
- **Cons**:
  - Higher CPU usage for complex UIs
  - Limited animation capabilities

### QML Version
- **Pros**:
  - GPU-accelerated rendering
  - Smooth animations
  - Better touch/gesture support
  - Modern look and feel
- **Cons**:
  - Higher memory usage
  - Requires QML runtime
  - Slower initial load

## When to Use Which Version

### Use QWidget When:
- Running on low-memory systems
- Need fastest possible startup
- Prefer traditional desktop UI
- Working with legacy code

### Use QML When:
- Need modern, fluid UI
- Supporting touch screens
- Want rapid UI prototyping
- Need complex animations
- Building for multiple platforms

## Migration Guide

### From QWidget to QML

1. **Keep all component code unchanged**
2. **Create bridge classes for your components**:
```cpp
class YourBridge : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool connected READ isConnected NOTIFY connectedChanged)
    // ... expose properties to QML
};
```

3. **Register bridge with QML**:
```cpp
qmlRegisterType<YourBridge>("YourModule", 1, 0, "YourBridge");
```

4. **Create QML UI files**:
```qml
import YourModule 1.0

Item {
    YourBridge {
        id: bridge
        onConnectedChanged: console.log("Connected:", connected)
    }
}
```

## Code Examples

### Connecting to Camera - QWidget
```cpp
// Direct connection in C++
connect(m_cameraComponent, SIGNAL(frameReady(QImage,qint64)),
        this, SLOT(onFrameReceived(QImage,qint64)));
```

### Connecting to Camera - QML
```qml
// Via bridge in QML
CameraBridge {
    id: camera
    onFrameReady: imageView.source = imageUrl
}
```

## Component Compatibility

Both versions use the exact same component implementations:
- `BaseComponent` - Foundation class (unchanged)
- `CameraComponent` - Abstract camera (unchanged)  
- `Do3ThinkCameraComponent` - Do3Think implementation (unchanged)
- `PreProcessorComponent` - Image processing (unchanged)

The bridge layer in QML version provides the interface without modifying components.

## Future Considerations

### QWidget Version
- Will continue to be maintained
- Best for industrial applications requiring stability
- Recommended for Windows-only deployments

### QML Version
- Primary focus for new UI features
- Better suited for cross-platform deployment
- Recommended for modern applications

## Summary

The QML version provides a modern UI layer while maintaining 100% compatibility with existing ComponentsForest architecture. Choose based on your specific requirements:

- **Need stability and simplicity?** → Use QWidget
- **Want modern UI and flexibility?** → Use QML

Both versions will continue to be supported and share the same component codebase, ensuring feature parity and maintainability.