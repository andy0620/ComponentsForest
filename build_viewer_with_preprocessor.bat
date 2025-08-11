@echo off
setlocal enabledelayedexpansion

echo =========================================
echo Building Do3Think Camera Viewer with Preprocessor Panel
echo =========================================

:: Set Qt installation path (update this to your Qt installation)
set QT_DIR=C:\Qt\6.9.1\msvc2022_64
if not exist "%QT_DIR%" (
    set QT_DIR=C:\Qt\6.9.0\msvc2019_64
)
if not exist "%QT_DIR%" (
    echo ERROR: Qt installation not found. Please update QT_DIR in this script.
    exit /b 1
)

:: Set up Visual Studio environment
echo Setting up Visual Studio environment...
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
if errorlevel 1 (
    call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
    if errorlevel 1 (
        call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
        if errorlevel 1 (
            echo ERROR: Visual Studio environment not found
            exit /b 1
        )
    )
)

:: Create build directory
if not exist build mkdir build
cd build

:: Configure with CMake
echo Configuring project...
cmake .. -G "Visual Studio 17 2022" -A x64 ^
    -DCMAKE_PREFIX_PATH="%QT_DIR%" ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DBUILD_EXAMPLES=OFF

if errorlevel 1 (
    echo ERROR: CMake configuration failed
    cd ..
    exit /b 1
)

:: Build the viewer
echo Building Do3ThinkCameraViewerStandalone...
cmake --build . --config Release --target Do3ThinkCameraViewerStandalone --parallel 8

if errorlevel 1 (
    echo ERROR: Build failed
    cd ..
    exit /b 1
)

:: Copy required DLLs
echo Copying required DLLs...
cd viewers\do3think_camera_viewer\Release

:: Copy Qt DLLs
copy "%QT_DIR%\bin\Qt6Core.dll" . >nul 2>&1
copy "%QT_DIR%\bin\Qt6Gui.dll" . >nul 2>&1
copy "%QT_DIR%\bin\Qt6Widgets.dll" . >nul 2>&1
copy "%QT_DIR%\bin\Qt6Network.dll" . >nul 2>&1
copy "%QT_DIR%\bin\Qt6Concurrent.dll" . >nul 2>&1
copy "%QT_DIR%\bin\Qt6Charts.dll" . >nul 2>&1
copy "%QT_DIR%\bin\Qt6OpenGL.dll" . >nul 2>&1
copy "%QT_DIR%\bin\Qt6OpenGLWidgets.dll" . >nul 2>&1

:: Copy Qt platform plugin (CRITICAL)
if not exist platforms mkdir platforms
copy "%QT_DIR%\plugins\platforms\qwindows.dll" platforms\ >nul 2>&1

:: Copy Qt styles
if not exist styles mkdir styles
copy "%QT_DIR%\plugins\styles\*.dll" styles\ >nul 2>&1

:: Copy DVPCamera SDK DLL
if exist "..\..\..\..\..\Do3ThinkCamera\SDK\DVPCamera64.dll" (
    copy "..\..\..\..\..\Do3ThinkCamera\SDK\DVPCamera64.dll" . >nul 2>&1
    echo DVPCamera64.dll copied
)

:: Copy OpenCV DLLs if available
set OPENCV_DIR=C:\opencv\build\x64\vc16\bin
if exist "%OPENCV_DIR%" (
    copy "%OPENCV_DIR%\opencv_world*.dll" . >nul 2>&1
    echo OpenCV DLLs copied
)

echo.
echo =========================================
echo Build completed successfully!
echo =========================================
echo.
echo Executable location: %CD%\Do3ThinkCameraViewerStandalone.exe
echo.
echo To run the application:
echo   cd build\viewers\do3think_camera_viewer\Release
echo   Do3ThinkCameraViewerStandalone.exe
echo.

cd ..\..\..\..

endlocal