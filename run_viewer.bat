@echo off
echo ==========================================
echo Running Do3Think Camera Viewer
echo ==========================================
echo.

REM Check if executable exists
if exist "build\viewers\do3think_camera_viewer\Release\Do3ThinkCameraViewerStandalone.exe" (
    echo Starting application...
    cd build\viewers\do3think_camera_viewer\Release
    start Do3ThinkCameraViewerStandalone.exe
    echo Application launched!
) else (
    echo ERROR: Executable not found!
    echo.
    echo Please compile the application first:
    echo   1. Open Visual Studio or use cmake
    echo   2. Build the Do3ThinkCameraViewerStandalone target
    echo   3. Run this script again
    echo.
    echo Expected location:
    echo   build\viewers\do3think_camera_viewer\Release\Do3ThinkCameraViewerStandalone.exe
)

echo.
pause