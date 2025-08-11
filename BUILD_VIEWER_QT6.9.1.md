# Do3Think Camera Viewer Build Instructions for Qt 6.9.1

## Overview
This guide provides step-by-step instructions for building the Do3Think Camera Viewer application with Qt 6.9.1.

## Prerequisites

### Required Software
- **Qt 6.9.1** (with the following modules):
  - Core
  - Widgets
  - Concurrent
  - Network
  - Charts
- **CMake** 3.16 or higher
- **Compiler**:
  - Windows: Visual Studio 2019/2022 or MinGW
  - Linux: GCC 9+ or Clang 10+

### Optional Software
- **OpenCV** 4.x (for advanced image processing)
- **Git** (for version control)

## Quick Start

### Windows (PowerShell)
```powershell
# Run the automated build script
.\build_viewer_qt6.9.1.ps1

# Or specify Qt path explicitly
.\build_viewer_qt6.9.1.ps1 -QtPath "C:\Qt\6.9.1\msvc2019_64"

# Clean build with debug configuration
.\build_viewer_qt6.9.1.ps1 -Clean -BuildType Debug
```

### Linux/WSL2 (Bash)
```bash
# Make script executable
chmod +x build_viewer_qt6.9.1.sh

# Run the automated build script
./build_viewer_qt6.9.1.sh

# Or specify Qt path explicitly
./build_viewer_qt6.9.1.sh --qt-path ~/Qt/6.9.1/gcc_64

# Clean build
./build_viewer_qt6.9.1.sh --clean
```

## Manual Build Instructions

### Step 1: Set Qt Environment

#### Windows
```batch
set QT_PATH=C:\Qt\6.9.1\msvc2019_64
set PATH=%QT_PATH%\bin;%PATH%
set CMAKE_PREFIX_PATH=%QT_PATH%;%CMAKE_PREFIX_PATH%
```

#### Linux
```bash
export QT_PATH=~/Qt/6.9.1/gcc_64
export PATH=$QT_PATH/bin:$PATH
export LD_LIBRARY_PATH=$QT_PATH/lib:$LD_LIBRARY_PATH
export CMAKE_PREFIX_PATH=$QT_PATH:$CMAKE_PREFIX_PATH
```

### Step 2: Build Core Components

```bash
# Create build directory
mkdir build && cd build

# Configure with CMake
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH=$QT_PATH \
    -DQt6_DIR=$QT_PATH/lib/cmake/Qt6 \
    -DBUILD_SHARED_LIBS=ON

# Build core library
cmake --build . --target ComponentsForestCore --parallel

# Build Do3Think component
cmake --build . --target Do3ThinkCameraComponent --parallel
```

### Step 3: Build Viewer Application

#### Option A: Using Main Build System
```bash
# From the build directory
cmake --build . --target Do3ThinkCameraViewer --parallel
```

#### Option B: Standalone Build
```bash
# Create separate viewer build directory
mkdir build_viewer && cd build_viewer

# Use standalone CMakeLists
cmake ../viewers/do3think_camera_viewer \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH=$QT_PATH

# Build
cmake --build . --parallel
```

### Step 4: Deploy Runtime Dependencies

#### Windows
```batch
cd build_viewer\Release

REM Deploy Qt dependencies
%QT_PATH%\bin\windeployqt.exe Do3ThinkCameraViewer.exe

REM Copy Do3Think SDK
copy ..\..\Do3ThinkCamera\SDK\DVPCamera64.dll .

REM Copy component libraries
copy ..\..\build\Release\ComponentsForestCore.dll .
copy ..\..\build\Release\Do3ThinkCameraComponent.dll .
```

#### Linux
```bash
cd build_viewer

# Copy libraries
cp ../build/libComponentsForestCore.so .
cp ../build/libDo3ThinkCameraComponent.so .
cp ../Do3ThinkCamera/SDK/libDVPCamera64.so .

# Set library path
export LD_LIBRARY_PATH=$(pwd):$LD_LIBRARY_PATH
```

## Platform-Specific Notes

### Windows with Visual Studio
1. Open "x64 Native Tools Command Prompt for VS 2019/2022"
2. Navigate to project directory
3. Run PowerShell build script or manual commands

### Windows with MinGW
1. Ensure MinGW bin directory is in PATH
2. Use `-G "MinGW Makefiles"` with CMake
3. Use `mingw32-make` instead of `nmake`

### Linux (Ubuntu/Debian)
```bash
# Install build dependencies
sudo apt-get update
sudo apt-get install build-essential cmake qt6-base-dev \
    qt6-charts-dev libqt6concurrent6 libqt6network6

# Optional: Install OpenCV
sudo apt-get install libopencv-dev
```

### WSL2 Considerations
- Ensure X11 forwarding is configured for GUI applications
- Install VcXsrv or similar X server on Windows
- Set DISPLAY environment variable:
  ```bash
  export DISPLAY=$(cat /etc/resolv.conf | grep nameserver | awk '{print $2}'):0
  ```

## Qt 6.9.1 Specific Requirements

### Module Dependencies
The application requires these Qt 6.9.1 modules:
- `Qt6Core` - Core functionality
- `Qt6Widgets` - UI components
- `Qt6Concurrent` - Threading support
- `Qt6Network` - Network operations
- `Qt6Charts` - Data visualization

### C++ Standard
Qt 6.9.1 requires C++17 or later. The project is configured for C++17.

### CMake Version
Qt 6.9.1 requires CMake 3.16 or later.

## Troubleshooting

### Qt Not Found
```
Error: Could not find a package configuration file provided by "Qt6"
```
**Solution**: Specify Qt path explicitly:
```bash
cmake .. -DCMAKE_PREFIX_PATH=/path/to/Qt/6.9.1/gcc_64
```

### Missing Qt Modules
```
Error: Could not find a package configuration file provided by "Qt6Charts"
```
**Solution**: Install Qt Charts module through Qt Maintenance Tool

### DVPCamera Library Not Found
```
Warning: DVPCamera library not found - using dynamic loading
```
**Solution**: The application will attempt to load the library at runtime. Ensure DVPCamera64.dll/.so is in the same directory as the executable.

### Permission Denied (Linux)
```
Error: Permission denied accessing /dev/video0
```
**Solution**: Add user to video group:
```bash
sudo usermod -a -G video $USER
# Logout and login again
```

### Visual Studio Version Mismatch
```
Error: The C++ compiler is not able to compile a simple test program
```
**Solution**: Ensure Qt was built with the same Visual Studio version you're using.

## Running the Application

### Windows
```batch
cd build_viewer\Release
.\Do3ThinkCameraViewer.exe

REM Or use the run script
.\run_viewer.bat
```

### Linux
```bash
cd build_viewer
./Do3ThinkCameraViewer

# Or use the run script
./run_viewer.sh
```

## Build Outputs

After successful build, you'll find:

### Core Libraries
- Windows: `ComponentsForestCore.dll`, `Do3ThinkCameraComponent.dll`
- Linux: `libComponentsForestCore.so`, `libDo3ThinkCameraComponent.so`

### Viewer Application
- Windows: `Do3ThinkCameraViewer.exe`
- Linux: `Do3ThinkCameraViewer`

### Directory Structure
```
build/
├── ComponentsForestCore.dll/so
├── Do3ThinkCameraComponent.dll/so
└── viewers/
    └── do3think_camera_viewer/
        └── Do3ThinkCameraViewer.exe

build_viewer/  (if built separately)
├── Do3ThinkCameraViewer.exe
├── DVPCamera64.dll/so
├── ComponentsForestCore.dll/so
├── Do3ThinkCameraComponent.dll/so
└── [Qt runtime libraries]
```

## Advanced Configuration

### Debug Build
```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug
```

### Release with Debug Info
```bash
cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo
```

### Custom Install Prefix
```bash
cmake .. -DCMAKE_INSTALL_PREFIX=/opt/do3think_viewer
make install
```

### Cross-Compilation
For cross-compilation, create a toolchain file and use:
```bash
cmake .. -DCMAKE_TOOLCHAIN_FILE=toolchain.cmake
```

## Support

For issues specific to:
- **Qt 6.9.1**: Check Qt documentation at https://doc.qt.io/qt-6.9/
- **Do3Think SDK**: Refer to `Do3ThinkCamera/SDK/DVP2 SDK使用指南.pdf`
- **Build System**: See `CLAUDE.md` for project-specific guidelines