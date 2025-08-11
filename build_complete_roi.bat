@echo off
REM Complete build script for ComponentsForest with ROI System
REM ===========================================================

echo ==========================================
echo Building ComponentsForest with ROI System
echo ==========================================
echo.

REM Set Qt path (adjust if needed)
set QT_PATH=C:\Qt\6.9.1\msvc2022_64
set CMAKE_PREFIX_PATH=%QT_PATH%

REM Check if build directory exists
if exist build (
    echo Cleaning previous build...
    rmdir /s /q build
)

mkdir build
cd build

REM Configure with CMake
echo.
echo Configuring project with CMake...
cmake .. -G "Visual Studio 17 2022" -A x64 ^
         -DCMAKE_PREFIX_PATH="%CMAKE_PREFIX_PATH%" ^
         -DCMAKE_BUILD_TYPE=Release ^
         -DBUILD_OPENCV_COMPONENTS=ON ^
         -DBUILD_ROI_EXAMPLES=ON ^
         -DBUILD_VIEWERS=ON ^
         -DBUILD_EXAMPLES=OFF

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ERROR: CMake configuration failed
    echo Please check:
    echo   1. CMake is installed and in PATH
    echo   2. Qt 6.9.1 is installed at %QT_PATH%
    echo   3. OpenCV is installed and findable
    echo   4. Visual Studio 2022 is installed
    cd ..
    pause
    exit /b 1
)

REM Build core components first
echo.
echo Building core components...
cmake --build . --config Release --target ComponentsForestCore

if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Core components build failed
    cd ..
    pause
    exit /b 1
)

REM Build Do3Think camera component
echo.
echo Building Do3Think camera component...
cmake --build . --config Release --target Do3ThinkCameraComponent

if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Do3Think camera component build failed
    cd ..
    pause
    exit /b 1
)

REM Build ROI System
echo.
echo Building ROI System...
cmake --build . --config Release --target ROISystem

if %ERRORLEVEL% NEQ 0 (
    echo ERROR: ROI System build failed
    cd ..
    pause
    exit /b 1
)

REM Build enhanced viewer with ROI support
echo.
echo Building Enhanced Camera Viewer...
cmake --build . --config Release --target Do3ThinkCameraViewerStandalone

if %ERRORLEVEL% NEQ 0 (
    echo WARNING: Viewer build failed (optional)
)

REM Build ROI example
echo.
echo Building ROI Example...
cmake --build . --config Release --target ROIExample

if %ERRORLEVEL% NEQ 0 (
    echo WARNING: ROI Example build failed (optional)
)

echo.
echo ==========================================
echo Build completed successfully!
echo ==========================================
echo.
echo ComponentsForest with ROI System Features:
echo   - Interactive ROI selection with mouse
echo   - Multiple ROI types (Rectangle, Circle, Polygon)
echo   - Real-time ROI statistics and analysis
echo   - ROI-based image preprocessing
echo   - Integration with Do3Think camera control panel
echo   - Save/Load ROI configurations
echo   - Export ROI masks
echo.
echo Output files:
echo   - Core Library: build\Release\ComponentsForestCore.dll
echo   - Camera Component: build\Release\Do3ThinkCameraComponent.dll
echo   - ROI System: build\OpenCV\Release\ROISystem.dll
echo   - Viewer: build\viewers\do3think_camera_viewer\Release\Do3ThinkCameraViewerStandalone.exe
echo   - ROI Example: build\OpenCV\Release\ROIExample.exe
echo.
echo To test the ROI system:
echo   1. Run the viewer executable
echo   2. Connect a Do3Think camera
echo   3. Use the ROI toolbar to enable ROI mode
echo   4. Click and drag on the camera view to create ROIs
echo   5. Select different ROI shapes from the toolbar
echo   6. View real-time statistics for each ROI
echo.

cd ..
pause