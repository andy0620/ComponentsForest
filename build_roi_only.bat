@echo off
REM Quick build script for ROI System only
REM ========================================

echo Building ROI System...
echo.

cd build

REM Build the ROI system library
cmake --build . --config Release --target ROISystem

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ERROR: ROI System build failed
    echo Trying to reconfigure...
    cd ..
    rmdir /s /q build\OpenCV
    mkdir build\OpenCV
    cd build
    cmake .. -DBUILD_OPENCV_COMPONENTS=ON
    cmake --build . --config Release --target ROISystem
    cd ..
    pause
    exit /b 1
)

REM Build the ROI example
echo.
echo Building ROI Example...
cmake --build . --config Release --target ROIExample

echo.
echo ==========================================
echo ROI System build completed!
echo ==========================================
echo.

cd ..
pause