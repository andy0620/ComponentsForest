# QML Camera Viewer Build Guide

## Overview

The QML Camera Viewer is a modern Qt Quick-based interface for the ComponentsForest camera control system. This guide provides complete instructions for building the viewer on both Windows and Linux/WSL.

## Build System Architecture

The QML viewer can be built in two modes:

1. **Integrated Build**: As part of the main ComponentsForest project
2. **Standalone Build**: Building the viewer independently

## Prerequisites

### Required Software
- Qt 6.9.1 or later with:
  - Qt Core
  - Qt GUI
  - Qt Quick
  - Qt Quick Controls 2
  - Qt QML
  - Qt Widgets (for camera control panels)
  - Qt Charts (for performance metrics)
  - Qt Concurrent
  - Qt Network
- CMake 3.16 or later
- C++17 compatible compiler
- Do3Think SDK (optional, for camera support)
- OpenCV (optional, for image processing features)

### Platform-Specific Requirements

#### Windows
- Visual Studio 2019/2022 or MinGW
- Windows SDK

#### Linux/WSL
- GCC 9+ or Clang 10+
- X11 development libraries (for GUI)
- pthread library

## File Structure

```
ComponentsForest/
├── CMakeLists.txt                    # Main project CMake
├── viewers/
│   └── qml_camera_viewer/
│       ├── CMakeLists.txt           # QML viewer CMake configuration
│       ├── qml.qrc                  # Qt resource file
│       ├── build_qml_viewer.sh      # Linux/WSL build script
│       ├── build_qml_viewer.bat     # Windows build script
│       ├── main.cpp                 # Application entry point
│       ├── qml_image_provider.cpp   # Image provider for QML
│       └── qml/                     # QML files
│           ├── main.qml
│           ├── components/          # QML components
│           └── controls/            # QML controls
├── qml_bridge/                       # C++/QML bridge components
│   ├── camera_bridge.cpp
│   ├── machine_bridge.cpp
│   └── ...
└── build_qml_viewer.sh              # Root-level build script
```

## Build Instructions

### Linux/WSL Build

#### Quick Build (Recommended)
```bash
# From project root
./build_qml_viewer.sh

# Or with options
./build_qml_viewer.sh Release 6.9.1 build_qml

# Clean rebuild
./build_qml_viewer.sh clean Release 6.9.1
```

#### Manual CMake Build
```bash
# Set Qt path
export QT_DIR=/opt/Qt/6.9.1/gcc_64

# Create build directory
mkdir build_qml && cd build_qml

# Configure
cmake .. \
    -DCMAKE_PREFIX_PATH=$QT_DIR \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_QML_VIEWER=ON \
    -DBUILD_OPENCV_COMPONENTS=OFF

# Build
make -j$(nproc) QMLCameraViewer

# Run
./bin/QMLCameraViewer
```

### Windows Build

#### Using Visual Studio
```batch
# Set Qt path
set QT_DIR=C:\Qt\6.9.1\msvc2022_64

# Create build directory
mkdir build_qml && cd build_qml

# Configure with Visual Studio generator
cmake .. -G "Visual Studio 17 2022" -A x64 ^
    -DCMAKE_PREFIX_PATH=%QT_DIR% ^
    -DBUILD_QML_VIEWER=ON

# Build
cmake --build . --config Release --target QMLCameraViewer

# Run
bin\Release\QMLCameraViewer.exe
```

#### Using MinGW
```batch
# Set Qt path
set QT_DIR=C:\Qt\6.9.1\mingw_64

# Configure
cmake .. -G "MinGW Makefiles" ^
    -DCMAKE_PREFIX_PATH=%QT_DIR% ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DBUILD_QML_VIEWER=ON

# Build
mingw32-make QMLCameraViewer
```

## CMake Configuration Options

| Option | Default | Description |
|--------|---------|-------------|
| `BUILD_QML_VIEWER` | ON | Build the QML camera viewer |
| `BUILD_VIEWERS` | ON | Build all viewer applications |
| `BUILD_OPENCV_COMPONENTS` | OFF | Enable OpenCV features (optional) |
| `BUILD_EXAMPLES` | OFF | Build example applications |
| `CMAKE_BUILD_TYPE` | Release | Build configuration (Debug/Release) |
| `CMAKE_PREFIX_PATH` | - | Path to Qt installation |

## Build Modes

### Integrated Build
When building as part of the main ComponentsForest project:
- Links to existing ComponentsForestCore and Do3ThinkCameraComponent libraries
- Shares common dependencies
- Smaller executable size

### Standalone Build
When building the viewer independently:
- Includes all necessary source files directly
- Self-contained executable
- Larger file size but no external dependencies

## Deployment

### Windows Deployment

The build system automatically handles:
1. Qt library deployment (via windeployqt)
2. Platform plugin copying (qwindows.dll)
3. QML module deployment
4. Do3Think SDK DLL copying

Critical files for Windows deployment:
```
QMLCameraViewer.exe
Qt6*.dll                    # Qt libraries
platforms/qwindows.dll      # REQUIRED - Platform plugin
QtQuick/                    # QML modules
QtQml/                      # QML runtime
DVPCamera64.dll            # Do3Think SDK (if used)
```

### Linux Deployment

For Linux deployment:
```bash
# The build sets RPATH automatically
# Libraries are found relative to executable

# To create portable package:
mkdir -p package/lib
cp bin/QMLCameraViewer package/
cp -r qml package/
cp $QT_DIR/lib/libQt6*.so* package/lib/
cp Do3ThinkCamera/SDK/libDVPCamera64.so package/lib/

# Create run script
cat > package/run.sh << 'EOF'
#!/bin/bash
export LD_LIBRARY_PATH=$PWD/lib:$LD_LIBRARY_PATH
export QML2_IMPORT_PATH=$PWD/qml:$QML2_IMPORT_PATH
./QMLCameraViewer "$@"
EOF
chmod +x package/run.sh
```

## Runtime Environment Variables

| Variable | Description | Example |
|----------|-------------|---------|
| `QML2_IMPORT_PATH` | QML module search paths | `./qml:/opt/Qt/6.9.1/gcc_64/qml` |
| `QT_QUICK_CONTROLS_STYLE` | UI style theme | `Material`, `Universal`, `Fusion` |
| `QT_LOGGING_RULES` | Debug output control | `*.debug=true` |
| `LD_LIBRARY_PATH` | Library search path (Linux) | `./lib:$LD_LIBRARY_PATH` |

## Troubleshooting

### Common Issues and Solutions

#### Application won't start on Windows
- **Cause**: Missing platform plugin
- **Solution**: Ensure `platforms/qwindows.dll` exists in executable directory

#### QML modules not found
- **Cause**: Missing QML import path
- **Solution**: Set `QML2_IMPORT_PATH` environment variable

#### Camera not detected
- **Cause**: Do3Think SDK not properly loaded
- **Solution**: 
  - Verify DVPCamera64.dll/libDVPCamera64.so is in library path
  - Check SDK initialization in logs

#### Build fails with MOC errors
- **Cause**: Qt MOC processor issues
- **Solution**: 
  - Ensure `CMAKE_AUTOMOC ON` in CMakeLists.txt
  - Clean build directory and rebuild

#### OpenCV not found (optional)
- **Note**: OpenCV is optional for the QML viewer
- **Solution**: Build with `-DBUILD_OPENCV_COMPONENTS=OFF`

### Debug Build

For debugging:
```bash
# Linux/WSL
cmake .. -DCMAKE_BUILD_TYPE=Debug
gdb ./bin/QMLCameraViewer

# Windows
cmake .. -DCMAKE_BUILD_TYPE=Debug
# Use Visual Studio debugger or gdb
```

Enable verbose logging:
```bash
export QT_LOGGING_RULES="*.debug=true"
export QML_IMPORT_TRACE=1
```

## Performance Optimization

### Build Optimizations
- Use Release build for production: `-DCMAKE_BUILD_TYPE=Release`
- Enable Link Time Optimization: `-DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON`
- Use native CPU optimizations: `-DCMAKE_CXX_FLAGS="-march=native"`

### Runtime Optimizations
- Use hardware acceleration: `export QSG_RENDER_LOOP=threaded`
- Enable QML compiler: Built into Qt 6.9+
- Minimize QML import paths

## Integration with ComponentsForest

The QML viewer integrates seamlessly with the ComponentsForest architecture:

1. **Component Integration**: Uses existing camera components via bridge classes
2. **Signal/Slot Decoupling**: Maintains architecture principles
3. **Thread Safety**: Components run in separate threads
4. **Performance**: Optimized image passing via QML image providers

## Development Tips

1. **Hot Reload**: Use Qt Creator's QML preview for rapid development
2. **Profiling**: Use Qt Creator's QML profiler to identify bottlenecks
3. **Testing**: Run with `QML_IMPORT_TRACE=1` to debug import issues
4. **Styling**: Change theme with `QT_QUICK_CONTROLS_STYLE` variable

## Version Compatibility

| ComponentsForest Version | Qt Version | CMake Version |
|-------------------------|------------|---------------|
| 1.0.0                   | 6.9.1+     | 3.16+        |

## Support

For issues or questions:
1. Check the troubleshooting section above
2. Review build logs for specific error messages
3. Ensure all prerequisites are correctly installed
4. Verify Qt installation with `qmake --version`

## License

The QML Camera Viewer follows the same license as the ComponentsForest project.