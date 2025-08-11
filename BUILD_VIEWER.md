# Building Do3Think Camera Viewer Application

## Prerequisites

1. **Qt 6.9.0 or later** - Required for UI and component framework
2. **CMake 3.16 or later** - Build system
3. **C++17 compatible compiler** (MSVC 2019, GCC 9+, Clang 10+)
4. **OpenCV** (optional, for image processing)
5. **Do3Think DVP2 SDK** (for camera hardware support)

## File Structure Verification

Ensure the following files exist:

### Viewer Application Files
- `viewers/do3think_camera_viewer/main.cpp` ✓
- `viewers/do3think_camera_viewer/main_ui.h` ✓
- `viewers/do3think_camera_viewer/main_ui.cpp` ✓
- `viewers/do3think_camera_viewer/machine.h` ✓
- `viewers/do3think_camera_viewer/machine.cpp` ✓
- `viewers/do3think_camera_viewer/CMakeLists.txt` ✓

### Component Library Files
- `components/base_component.h` ✓
- `components/base_component.cpp` ✓
- `components/camera_component.h` ✓
- `components/camera_component.cpp` ✓
- `components/camera_control_panel.h` ✓
- `components/camera_control_panel.cpp` ✓

### Do3Think Camera Files
- `Do3ThinkCamera/dothink_camera.h` ✓
- `Do3ThinkCamera/dothink_camera.cpp` ✓
- `Do3ThinkCamera/dothink_camera_control_panel.h` ✓
- `Do3ThinkCamera/dothink_camera_control_panel.cpp` ✓
- `Do3ThinkCamera/dothink_camera_control_panel_methods.cpp` ✓

## Build Instructions

### Windows (Visual Studio)

```powershell
# 1. Open Developer Command Prompt for VS 2019/2022
# 2. Navigate to project root
cd ComponentsForest

# 3. Create build directory
mkdir build
cd build

# 4. Configure with CMake (adjust Qt path as needed)
cmake .. -G "Visual Studio 17 2022" -A x64 ^
  -DCMAKE_PREFIX_PATH="C:\Qt\6.9.0\msvc2019_64" ^
  -DCMAKE_BUILD_TYPE=Release

# 5. Build the project
cmake --build . --config Release

# 6. The executable will be in:
# build\viewers\do3think_camera_viewer\Release\Do3ThinkCameraViewer.exe
```

### Windows (MinGW)

```bash
# 1. Open Git Bash or MinGW terminal
# 2. Navigate to project root
cd ComponentsForest

# 3. Create build directory
mkdir build && cd build

# 4. Configure with CMake
cmake .. -G "MinGW Makefiles" \
  -DCMAKE_PREFIX_PATH="/c/Qt/6.9.0/mingw_64" \
  -DCMAKE_BUILD_TYPE=Release

# 5. Build
mingw32-make -j4

# 6. Executable location:
# build/viewers/do3think_camera_viewer/Do3ThinkCameraViewer.exe
```

### Linux

```bash
# 1. Install Qt6 if not already installed
sudo apt-get install qt6-base-dev qt6-charts-dev

# 2. Navigate to project root
cd ComponentsForest

# 3. Create build directory
mkdir build && cd build

# 4. Configure with CMake
cmake .. -DCMAKE_PREFIX_PATH=/usr/lib/x86_64-linux-gnu/cmake/Qt6 \
         -DCMAKE_BUILD_TYPE=Release

# 5. Build
make -j$(nproc)

# 6. Executable location:
# build/viewers/do3think_camera_viewer/Do3ThinkCameraViewer
```

## Troubleshooting

### Qt Not Found
- Ensure Qt6 is installed and CMAKE_PREFIX_PATH points to the Qt installation
- On Windows: `C:\Qt\6.9.0\msvc2019_64` or similar
- On Linux: `/usr/lib/x86_64-linux-gnu/cmake/Qt6` or `/opt/Qt/6.9.0/gcc_64`

### Missing DVPCamera64.dll
- Copy `DVPCamera64.dll` from Do3Think SDK to the executable directory
- Or install Do3Think SDK system-wide

### Compilation Errors
1. **Q_OBJECT macro errors**: Run CMake again, it should auto-generate MOC files
2. **Link errors**: Ensure all libraries are built in the same configuration (Debug/Release)
3. **Missing symbols**: Check that all .cpp files are included in CMakeLists.txt

### Runtime Issues
1. **Camera not detected**: 
   - Ensure Do3Think SDK is properly installed
   - Check USB connection and drivers
   - Run as administrator if needed

2. **UI not showing**:
   - Check Qt platform plugins are available
   - Set `QT_PLUGIN_PATH` environment variable if needed

## Implementation Status

### Completed ✓
- Machine class for component management
- MainUI with proper Signal/Slot architecture
- Control Panel as primary camera interface
- CMake build configuration
- Thread-safe component pattern

### Architecture Highlights
1. **Complete Signal/Slot Decoupling**: No direct method calls between UI and components
2. **Control Panel Primary Interface**: MainUI never controls cameras directly
3. **Thread Safety**: Each component runs in its own QThread
4. **PIMPL Pattern**: Private implementation for better encapsulation

## Running the Application

After successful build:

```bash
# Windows
build\viewers\do3think_camera_viewer\Release\Do3ThinkCameraViewer.exe

# Linux
./build/viewers/do3think_camera_viewer/Do3ThinkCameraViewer
```

## Expected Behavior

1. Application starts with empty camera list
2. Click "Add Camera" to discover and add Do3Think cameras
3. Control panels appear for each camera
4. Use control panels to start/stop acquisition
5. View real-time images from cameras
6. Monitor performance metrics in status panels

## Next Steps

1. Install Qt 6.9.0 and CMake on your development machine
2. Clone or copy the ComponentsForest project
3. Follow the build instructions above
4. Report any issues with specific error messages

The implementation follows the architecture defined in CLAUDE.md with complete Signal/Slot decoupling and proper thread safety.