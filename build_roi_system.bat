@echo off
REM Build script for ROI System with OpenCV
REM =========================================

echo ==========================================
echo Building ROI System with OpenCV
echo ==========================================
echo.

REM Check if build directory exists
if not exist build (
    mkdir build
)

cd build

REM Configure with CMake
echo Configuring with CMake...
cmake .. -DCMAKE_PREFIX_PATH="C:\Qt\6.9.1\msvc2022_64" ^
         -DCMAKE_BUILD_TYPE=Release ^
         -DBUILD_OPENCV_COMPONENTS=ON ^
         -DBUILD_ROI_EXAMPLES=ON ^
         -DBUILD_VIEWERS=ON

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ERROR: CMake configuration failed
    echo Please check:
    echo   1. Qt 6.9.1 is installed
    echo   2. OpenCV is installed and findable
    echo   3. Visual Studio 2022 is installed
    pause
    exit /b 1
)

REM Build the ROI system
echo.
echo Building ROI System...
cmake --build . --config Release --target ROISystem

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ERROR: ROI System build failed
    pause
    exit /b 1
)

REM Build the enhanced viewer with ROI support
echo.
echo Building Enhanced Viewer...
cmake --build . --config Release --target Do3ThinkCameraViewerStandalone

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ERROR: Viewer build failed
    pause
    exit /b 1
)

REM Build the ROI example
echo.
echo Building ROI Example...
cmake --build . --config Release --target ROIExample

echo.
echo ==========================================
echo Build completed successfully!
echo ==========================================
echo.
echo ROI System features:
echo   - Interactive ROI selection
echo   - Multiple ROI types (Rectangle, Circle, Polygon)
echo   - Mouse drag to create ROIs
echo   - Resize and move ROIs with handles
echo   - ROI statistics and analysis
echo   - Integration with camera control panel
echo.
echo Executables:
echo   - Viewer: build\viewers\do3think_camera_viewer\Release\Do3ThinkCameraViewerStandalone.exe
echo   - ROI Example: build\OpenCV\Release\ROIExample.exe
echo.
echo To test ROI functionality:
echo   1. Run the viewer and connect a camera
echo   2. Use the ROI toolbar to create regions
echo   3. Click and drag on the image to create ROIs
echo   4. View real-time statistics for each ROI
echo.

cd ..
pause