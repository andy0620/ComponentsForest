@echo off
setlocal enabledelayedexpansion

echo =========================================
echo Running Do3Think Camera Viewer with Preprocessor Panel
echo =========================================

:: Navigate to the executable directory
cd build\viewers\do3think_camera_viewer\Release

:: Check if executable exists
if not exist Do3ThinkCameraViewerStandalone.exe (
    echo ERROR: Do3ThinkCameraViewerStandalone.exe not found!
    echo Please build the project first using build_viewer_with_preprocessor.bat
    cd ..\..\..\..
    exit /b 1
)

:: Set Qt plugin path
set QT_PLUGIN_PATH=C:\Qt\6.9.1\msvc2022_64\plugins
if not exist "%QT_PLUGIN_PATH%" (
    set QT_PLUGIN_PATH=C:\Qt\6.9.0\msvc2019_64\plugins
)

:: Set PATH to include Qt and OpenCV binaries
set PATH=C:\Qt\6.9.1\msvc2022_64\bin;C:\Program Files\opencv\build\x64\vc16\bin;%PATH%

:: Copy required DLLs if not present
if not exist Qt6Core.dll (
    echo Copying Qt DLLs...
    copy "C:\Qt\6.9.1\msvc2022_64\bin\Qt6Core.dll" . >nul 2>&1
    copy "C:\Qt\6.9.1\msvc2022_64\bin\Qt6Gui.dll" . >nul 2>&1
    copy "C:\Qt\6.9.1\msvc2022_64\bin\Qt6Widgets.dll" . >nul 2>&1
    copy "C:\Qt\6.9.1\msvc2022_64\bin\Qt6Network.dll" . >nul 2>&1
    copy "C:\Qt\6.9.1\msvc2022_64\bin\Qt6Concurrent.dll" . >nul 2>&1
    copy "C:\Qt\6.9.1\msvc2022_64\bin\Qt6Charts.dll" . >nul 2>&1
)

:: Copy platform plugin if not present
if not exist platforms\qwindows.dll (
    echo Copying Qt platform plugin...
    if not exist platforms mkdir platforms
    copy "C:\Qt\6.9.1\msvc2022_64\plugins\platforms\qwindows.dll" platforms\ >nul 2>&1
)

:: Copy OpenCV DLL if not present
if not exist opencv_world4110.dll (
    echo Copying OpenCV DLL...
    copy "C:\Program Files\opencv\build\x64\vc16\bin\opencv_world4110.dll" . >nul 2>&1
)

:: Copy DVPCamera DLL if not present
if not exist DVPCamera64.dll (
    echo Copying DVPCamera64.dll...
    if exist "..\..\..\..\Do3ThinkCamera\SDK\DVPCamera64.dll" (
        copy "..\..\..\..\Do3ThinkCamera\SDK\DVPCamera64.dll" . >nul 2>&1
    ) else if exist "C:\Program Files (x86)\Camera\DVP2 x64\DVPCamera64.dll" (
        copy "C:\Program Files (x86)\Camera\DVP2 x64\DVPCamera64.dll" . >nul 2>&1
    )
)

echo.
echo Starting Do3ThinkCameraViewerStandalone...
echo.
echo The application now includes:
echo   - Camera Control Panels
echo   - Preprocessor Control Panel
echo   - Edge Detection Processor
echo.

:: Run the application
Do3ThinkCameraViewerStandalone.exe

cd ..\..\..\..

endlocal