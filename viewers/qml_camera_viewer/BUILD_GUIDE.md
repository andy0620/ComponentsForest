# QML Camera Viewer Build Guide

## Quick Start

### Windows
```batch
# From project root
build_qml_viewer.bat

# Or from viewer directory
cd viewers/qml_camera_viewer
build.bat
```

### Linux/WSL
```bash
# From project root
./build_qml_viewer.sh

# Or from viewer directory
cd viewers/qml_camera_viewer
./build.sh
```

## Build Requirements

### Required Software
- **CMake** 3.16 or higher
- **Qt** 6.9.0 or higher with:
  - Qt Core
  - Qt Quick
  - Qt QuickControls2
  - Qt Concurrent
  - Qt Charts
  - Qt Widgets (for control panels)
- **C++ Compiler**:
  - Windows: Visual Studio 2019/2022 with C++17 support
  - Linux: GCC 9+ or Clang 10+

### Optional Dependencies
- **OpenCV** 4.x (for preprocessor support)
- **Do3Think SDK** (for camera hardware support)

## Build Options

### Windows Build Script Options
```batch
build_qml_viewer.bat [options]
  --debug        Build in Debug mode (default: Release)
  --no-clean     Don't clean previous build
  --no-deploy    Skip Qt runtime deployment
  --integrated   Build as part of ComponentsForest
  --help         Show help message
```

### Linux Build Script Options
```bash
./build_qml_viewer.sh [options]
  --debug           Build in Debug mode (default: Release)
  --no-clean        Don't clean previous build
  --integrated      Build as part of ComponentsForest
  --with-opencv     Enable OpenCV support
  --qt-dir PATH     Specify Qt installation directory
  -j, --jobs N      Number of parallel build jobs
  --help            Show help message
```

## Build Modes

### Standalone Build (Default)
Builds the QML viewer as an independent application with all required components compiled in:
```bash
./build_qml_viewer.sh  # Linux
build_qml_viewer.bat   # Windows
```

### Integrated Build
Builds as part of the main ComponentsForest project, linking to shared libraries:
```bash
./build_qml_viewer.sh --integrated  # Linux
build_qml_viewer.bat --integrated   # Windows
```

### Debug Build
Includes debug symbols and QML debugging support:
```bash
./build_qml_viewer.sh --debug  # Linux
build_qml_viewer.bat --debug   # Windows
```

## CMake Direct Build

For advanced users who prefer direct CMake control:

### Configure
```bash
mkdir build
cd build
cmake ../viewers/qml_camera_viewer \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH=/path/to/Qt/6.9.1/gcc_64 \
    -DBUILD_STANDALONE=ON \
    -DBUILD_WITH_OPENCV=OFF
```

### Build
```bash
cmake --build . --parallel
```

### CMake Options
- `BUILD_STANDALONE`: Build as standalone app (ON) or integrated (OFF)
- `BUILD_WITH_OPENCV`: Enable OpenCV support
- `DEPLOY_QT_RUNTIME`: Deploy Qt runtime libraries (Windows)
- `CMAKE_BUILD_TYPE`: Release or Debug
- `CMAKE_PREFIX_PATH`: Path to Qt installation

## Directory Structure

After successful build:
```
build_qml_release/           # Linux build directory
build_qml_Release/           # Windows build directory
├── bin/                     # Executable output
│   ├── QMLCameraViewer      # Linux executable
│   ├── QMLCameraViewer.exe  # Windows executable
│   ├── platforms/           # Qt platform plugins (Windows)
│   ├── qml/                 # QML modules
│   └── *.dll/.so            # Runtime libraries
├── CMakeCache.txt           # CMake configuration
└── run_qml_viewer.sh        # Linux run script
```

## Running the Application

### Windows
```batch
cd build_qml_Release
Release\QMLCameraViewer.exe
```

### Linux
```bash
cd build_qml_release
./run_qml_viewer.sh
```

## Deployment

### Windows Deployment
The build script automatically runs `windeployqt` to copy required Qt libraries. For manual deployment:
```batch
%QT_DIR%\bin\windeployqt.exe ^
    --qmldir viewers\qml_camera_viewer\qml ^
    --quick ^
    --no-translations ^
    QMLCameraViewer.exe
```

### Linux Deployment
Create an AppImage or package with required libraries:
```bash
# Copy Qt libraries
ldd QMLCameraViewer | grep Qt | awk '{print $3}' | xargs -I{} cp {} ./lib/

# Set RPATH
patchelf --set-rpath '$ORIGIN/lib' QMLCameraViewer
```

## Troubleshooting

### Qt Not Found
- Set `QT_DIR` environment variable:
  ```batch
  set QT_DIR=C:\Qt\6.9.1\msvc2022_64  # Windows
  export QT_DIR=/opt/Qt/6.9.1/gcc_64  # Linux
  ```

### Missing QML Modules
- Ensure Qt Quick and QuickControls2 are installed
- Check QML2_IMPORT_PATH environment variable

### Camera Not Detected
- Verify Do3Think SDK is installed
- Check DVPCamera64.dll/.so is in the output directory
- Enable debug logging: `export QT_LOGGING_RULES="ComponentsForest.*=true"`

### Build Fails with MOC Errors
- Clean build directory and rebuild
- Verify all headers with Q_OBJECT are listed in CMakeLists.txt

### Application Won't Start (Windows)
- Check platforms/qwindows.dll exists
- Run with QT_DEBUG_PLUGINS=1 for diagnostics

## Performance Optimization

### Compiler Optimizations
The build system automatically applies:
- Windows: `/O2 /GL /LTCG` (whole program optimization)
- Linux: `-O3 -march=native -ffast-math`

### Runtime Performance
- Set `QSG_RENDER_LOOP=threaded` for better rendering
- Use Release builds for production
- Enable GPU acceleration: `QSG_RHI_BACKEND=opengl`

## Development Tips

### Enable QML Debugging
```bash
export QML_DISABLE_DISK_CACHE=1
export QT_QML_DEBUG=1
```

### Hot Reload QML
The application supports QML hot reload in Debug mode. Press F5 in the running application to reload QML files.

### Custom Qt Location
Specify a custom Qt installation:
```bash
./build_qml_viewer.sh --qt-dir /custom/path/to/Qt
```

## CI/CD Integration

### GitHub Actions Example
```yaml
- name: Build QML Viewer
  run: |
    ./build_qml_viewer.sh --no-clean --jobs 4
    
- name: Run Tests
  run: |
    cd build_qml_release
    ./QMLCameraViewer --test-mode
```

### Docker Build
```dockerfile
FROM ubuntu:22.04
RUN apt-get update && apt-get install -y \
    qt6-base-dev qt6-declarative-dev \
    qt6-quickcontrols2-dev cmake g++
COPY . /app
WORKDIR /app
RUN ./build_qml_viewer.sh --jobs 4
```

## Support

For issues or questions:
1. Check the [TROUBLESHOOTING_AND_BEST_PRACTICES_GUIDE.md](../../TROUBLESHOOTING_AND_BEST_PRACTICES_GUIDE.md)
2. Review build logs in `build_qml_*/CMakeFiles/CMakeOutput.log`
3. Enable verbose CMake output: `cmake --build . --verbose`