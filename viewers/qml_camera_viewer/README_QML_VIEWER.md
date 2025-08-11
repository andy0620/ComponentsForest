# QML Camera Viewer

Modern QML-based camera control interface for the ComponentsForest ecosystem.

## Features

- **Modern QML Interface**: Fluid, responsive UI built with Qt Quick
- **Material Design**: Professional industrial look with Material style
- **High Performance**: Efficient image display with custom image provider
- **Thread-Safe Architecture**: Components run in separate threads
- **Multi-Camera Support**: Control up to 16 cameras simultaneously
- **Real-Time Statistics**: FPS, frame count, and performance metrics
- **Cross-Platform**: Runs on Windows and Linux

## Architecture

```
QML UI Layer (Main Thread)
    ↓
Bridge Classes (Main Thread)
    ↓ (Signal/Slot)
Machine & Components (Worker Thread)
    ↓
Hardware/SDK Layer
```

### Key Components

1. **main.cpp**: Application entry point
   - Configures Qt Quick environment
   - Sets up threading model
   - Registers QML types
   - Manages application lifecycle

2. **QmlImageProvider**: High-performance image delivery
   - Thread-safe image caching
   - LRU cache eviction
   - Format optimization
   - Performance statistics

3. **Bridge Classes**: QML-C++ interface
   - CameraBridge: Camera control interface
   - MachineBridge: System management interface

## Building

### Prerequisites

- Qt 6.2 or later (6.9 recommended)
- C++17 compatible compiler
- CMake 3.16+ (optional)
- Do3Think SDK (for Do3Think cameras)

### Build Methods

#### Method 1: CMake (Recommended)

```bash
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --parallel
```

#### Method 2: Direct Make (Linux)

```bash
cd viewers/qml_camera_viewer
make
```

#### Method 3: Batch Script (Windows)

```batch
cd viewers\qml_camera_viewer
build_qml_viewer.bat
```

## Running

### Command Line Options

```bash
QMLCameraViewer [options]

Options:
  -h, --help          Show help information
  -v, --version       Show version information
  -d, --debug         Enable debug output
  -V, --verbose       Enable verbose output
  -f, --fullscreen    Start in fullscreen mode
  -c, --config FILE   Load configuration from FILE
  -m, --max-cameras N Set maximum cameras (1-16, default: 4)
```

### Examples

```bash
# Run with default settings
./QMLCameraViewer

# Run with debug output and 8 cameras
./QMLCameraViewer -d -m 8

# Run fullscreen with config file
./QMLCameraViewer -f -c config.json
```

## QML Structure

```
qml/
├── main.qml                      # Main application window
├── components/
│   ├── CameraControlPanel.qml    # Individual camera control
│   ├── CameraListDelegate.qml    # Camera list item
│   ├── CameraParameters.qml      # Parameter editor
│   ├── ConnectionIndicator.qml   # Connection status
│   ├── SettingsPanel.qml         # Global settings
│   └── StatisticsPanel.qml       # Performance stats
└── controls/
    ├── ConnectionControls.qml    # Connect/disconnect
    └── AcquisitionControls.qml   # Start/stop acquisition
```

## Image Provider

The custom image provider offers:

- **Caching**: Configurable cache size (default 100MB)
- **Performance**: Zero-copy where possible
- **Statistics**: Track cache hits, misses, bytes delivered
- **Optimization**: Automatic format conversion
- **Thread Safety**: Concurrent access from multiple QML items

### Usage in QML

```qml
Image {
    source: "image://camera/" + cameraId + "/" + frameNumber
    cache: false  // Provider handles caching
    asynchronous: true
}
```

## Performance Tuning

### Image Display
- Adjust cache size: `-m 200` (200MB cache)
- Enable/disable caching per camera
- Use hardware acceleration (OpenGL)

### Threading
- Components run in dedicated threads
- UI updates limited to 30 FPS
- Asynchronous image loading

### Memory Management
- LRU cache eviction
- Pinned images for critical cameras
- Automatic cleanup of old frames

## Logging

### Log Categories
- `ComponentsForest.QMLViewer.Main`: Application lifecycle
- `ComponentsForest.QMLViewer.Performance`: Performance metrics
- `ComponentsForest.Camera.*`: Camera operations
- `ComponentsForest.Machine.*`: System management

### Enable Debug Logging

```bash
# Via command line
QMLCameraViewer -d

# Via environment variable
export QT_LOGGING_RULES="ComponentsForest.*=true"
QMLCameraViewer
```

## Troubleshooting

### Application Won't Start
- Check Qt platform plugins: `platforms/qwindows.dll` (Windows)
- Verify Qt installation path
- Check Do3Think SDK DLL availability

### No Cameras Detected
- Ensure SDK is initialized
- Check USB connections
- Verify camera drivers installed

### Poor Performance
- Reduce number of active cameras
- Increase cache size
- Check CPU/GPU usage
- Disable debug logging

### QML Loading Errors
- Verify QML files exist
- Check QML2_IMPORT_PATH
- Validate QML syntax

## Development

### Adding Custom Components

1. Create QML component in `qml/components/`
2. Register in `CMakeLists.txt` or `Makefile`
3. Import in main.qml or parent component

### Extending Bridge Classes

1. Add Q_PROPERTY to bridge header
2. Implement getter/setter/signal
3. Connect to component signals
4. Access from QML

### Custom Image Processing

1. Subclass QmlImageProvider
2. Override requestImage/requestPixmap
3. Register with engine in main.cpp

## Material Theme Customization

Environment variables control the Material theme:

```bash
export QT_QUICK_CONTROLS_MATERIAL_THEME=Dark
export QT_QUICK_CONTROLS_MATERIAL_ACCENT=#2196F3
export QT_QUICK_CONTROLS_MATERIAL_PRIMARY=#1976D2
export QT_QUICK_CONTROLS_MATERIAL_FOREGROUND=#FFFFFF
export QT_QUICK_CONTROLS_MATERIAL_BACKGROUND=#121212
```

## License

Part of the ComponentsForest project. See main LICENSE file.