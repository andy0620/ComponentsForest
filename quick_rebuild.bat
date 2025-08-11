@echo off
echo Rebuilding with Modern UI...
cd build
cmake --build . --config Release --target Do3ThinkCameraViewerStandalone
echo.
echo Build complete! Run the application to see the modern UI:
echo   cd viewers\do3think_camera_viewer\Release
echo   Do3ThinkCameraViewerStandalone.exe
cd ..
pause