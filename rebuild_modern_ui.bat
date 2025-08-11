@echo off
REM Rebuild Do3Think Camera Viewer with Modern UI
REM =============================================

echo ==========================================
echo Rebuilding with Modern UI Design
echo ==========================================
echo.

cd build

REM Clean previous build of viewer only
echo Cleaning previous viewer build...
if exist viewers\do3think_camera_viewer\Release\Do3ThinkCameraViewerStandalone.exe (
    del viewers\do3think_camera_viewer\Release\Do3ThinkCameraViewerStandalone.exe
)

REM Rebuild the viewer with modern UI
echo Building viewer with modern UI...
cmake --build . --config Release --target Do3ThinkCameraViewerStandalone

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ERROR: Build failed
    echo Please check the error messages above
    pause
    exit /b 1
)

echo.
echo ==========================================
echo Modern UI Build Completed Successfully!
echo ==========================================
echo.
echo The application has been updated with:
echo  - Modern dark theme with gradients
echo  - Glass-morphism effects
echo  - Animated toggle switches
echo  - Card-based layouts
echo  - Floating action buttons
echo  - Enhanced status indicators
echo.
echo To run the modern UI version:
echo   cd viewers\do3think_camera_viewer\Release
echo   Do3ThinkCameraViewerStandalone.exe
echo.
pause