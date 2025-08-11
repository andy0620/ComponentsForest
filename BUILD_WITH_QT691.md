# Building with Qt 6.9.1

## Quick Start

### For Windows Users

1. **Open Command Prompt or PowerShell**
2. **Navigate to ComponentsForest directory**
3. **Run the build script:**

```cmd
# If Qt is installed in default location (C:\Qt\6.9.1)
quick_build_qt691.bat

# Or specify your Qt path
quick_build_qt691.bat D:\Qt\6.9.1\msvc2019_64
```

### For Linux/WSL2 Users

1. **Open Terminal**
2. **Navigate to ComponentsForest directory**
3. **Run the build script:**

```bash
# If Qt is in standard location
./quick_build_qt691.sh

# Or specify your Qt path
./quick_build_qt691.sh /mnt/c/Qt/6.9.1/gcc_64
```

## Common Qt 6.9.1 Installation Paths

### Windows
- `C:\Qt\6.9.1\msvc2019_64` (Visual Studio)
- `C:\Qt\6.9.1\mingw_64` (MinGW)
- `D:\Qt\6.9.1\msvc2019_64`

### Linux
- `~/Qt/6.9.1/gcc_64`
- `/opt/Qt/6.9.1/gcc_64`

### WSL2 (accessing Windows Qt)
- `/mnt/c/Qt/6.9.1/gcc_64`
- `/mnt/c/Qt/6.9.1/msvc2019_64`

## Manual Build (if scripts don't work)

### Step 1: Create build directory
```bash
mkdir build_viewer
cd build_viewer
```

### Step 2: Configure with CMake
```bash
# Linux/WSL2
cmake .. -DCMAKE_PREFIX_PATH=/path/to/Qt/6.9.1/gcc_64 -DCMAKE_BUILD_TYPE=Release

# Windows (Visual Studio)
cmake .. -DCMAKE_PREFIX_PATH="C:\Qt\6.9.1\msvc2019_64" -G "Visual Studio 17 2022" -A x64
```

### Step 3: Build
```bash
# Linux/WSL2
make -j$(nproc)

# Windows
cmake --build . --config Release
```

## Running the Application

### Windows
```cmd
cd build_viewer\viewers\do3think_camera_viewer\Release
Do3ThinkCameraViewer.exe
```

### Linux/WSL2
```bash
cd build_viewer/viewers/do3think_camera_viewer
./Do3ThinkCameraViewer
```

## Troubleshooting

### "Qt not found"
- Verify Qt 6.9.1 is installed
- Check the installation path
- Ensure qmake is in `<Qt_Path>/bin/`

### "CMake not found"
- **Windows**: Download from https://cmake.org/download/
- **Linux**: `sudo apt-get install cmake`
- **WSL2**: `sudo apt-get update && sudo apt-get install cmake`

### "Compiler not found"
- **Windows**: Install Visual Studio 2019/2022 with C++ workload
- **Linux/WSL2**: `sudo apt-get install build-essential`

### "Missing Qt modules"
Ensure these Qt components are installed:
- Qt Core
- Qt Widgets
- Qt Charts
- Qt Concurrent
- Qt Network

### Build Errors
1. Clean the build directory: `rm -rf build_viewer`
2. Check Qt version: Should be 6.9.0 or later
3. Verify all source files exist in `viewers/do3think_camera_viewer/`

## What Gets Built

The build process creates:
1. **ComponentsForestCore.dll/so** - Core component library
2. **Do3ThinkCameraComponent.dll/so** - Camera component library
3. **Do3ThinkCameraViewer.exe** - The viewer application

## Features of the Viewer

- **Multi-camera support** - Manage multiple Do3Think cameras
- **Thread-safe architecture** - Each component runs in its own thread
- **Signal/Slot decoupling** - Complete separation between UI and logic
- **Control Panel interface** - All camera control through panels
- **Multiple view modes** - Tab, Grid, Split, Single views
- **Real-time performance monitoring**
- **Comprehensive logging**

## Next Steps After Building

1. **Connect Do3Think camera** to your computer
2. **Run the viewer application**
3. **Click "Add Camera"** to discover connected cameras
4. **Use control panels** to start/stop acquisition
5. **View real-time images** from the cameras

## Architecture Compliance

The implementation follows strict architectural rules:
- ✅ Control Panels are the SOLE interface for camera control
- ✅ Signal/Slot only communication (no direct method calls)
- ✅ Thread-safe component isolation
- ✅ MainUI never controls cameras directly

## Support

If you encounter issues:
1. Check the build output for specific error messages
2. Verify Qt 6.9.1 is properly installed
3. Ensure all dependencies are available
4. Review CLAUDE.md for architecture details