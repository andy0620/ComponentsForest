@echo off
REM Local build script for QML Camera Viewer
REM Can be run directly from the viewer directory

setlocal

REM Get the viewer directory (where this script is located)
set VIEWER_DIR=%~dp0
REM Remove trailing backslash
set VIEWER_DIR=%VIEWER_DIR:~0,-1%

REM Get the project root (two levels up)
cd /d "%VIEWER_DIR%\..\.."
set PROJECT_ROOT=%CD%

REM Run the main build script from project root
if exist "build_qml_viewer.bat" (
    call build_qml_viewer.bat %*
) else (
    echo Error: Main build script not found at project root
    echo Expected location: %PROJECT_ROOT%\build_qml_viewer.bat
    exit /b 1
)