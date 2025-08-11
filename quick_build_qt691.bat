@echo off
REM Quick Build Script for Qt 6.9.1 on Windows
REM ===========================================

echo ==========================================
echo Do3Think Camera Viewer - Qt 6.9.1 Build
echo ==========================================
echo.

REM Check if Qt path is provided
if "%1"=="" (
    echo Usage: quick_build_qt691.bat [Qt_Path]
    echo Example: quick_build_qt691.bat C:\Qt\6.9.1\msvc2019_64
    echo.
    echo Trying common Qt paths...
    
    REM Try common Qt installation paths
    if exist "C:\Qt\6.9.1\msvc2019_64\bin\qmake.exe" (
        set QT_PATH=C:\Qt\6.9.1\msvc2019_64
    ) else if exist "C:\Qt\6.9.1\mingw_64\bin\qmake.exe" (
        set QT_PATH=C:\Qt\6.9.1\mingw_64
    ) else if exist "D:\Qt\6.9.1\msvc2019_64\bin\qmake.exe" (
        set QT_PATH=D:\Qt\6.9.1\msvc2019_64
    ) else (
        echo ERROR: Qt 6.9.1 not found in common locations
        echo Please provide Qt path as argument
        exit /b 1
    )
) else (
    set QT_PATH=%1
)

echo Using Qt at: %QT_PATH%
echo.

REM Clean previous build
if exist build_viewer rmdir /s /q build_viewer
mkdir build_viewer
cd build_viewer

REM Configure with CMake
echo Configuring with CMake...
cmake .. -DCMAKE_PREFIX_PATH="%QT_PATH%" -DCMAKE_BUILD_TYPE=Release -DBUILD_VIEWERS=ON

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ERROR: CMake configuration failed
    echo Please ensure:
    echo  1. CMake is installed and in PATH
    echo  2. Qt 6.9.1 is properly installed at %QT_PATH%
    echo  3. Visual Studio or MinGW is installed
    exit /b 1
)

REM Build the project
echo.
echo Building the project...
cmake --build . --config Release --parallel

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ERROR: Build failed
    echo Check the error messages above
    exit /b 1
)

echo.
echo ==========================================
echo Build completed successfully!
echo ==========================================
echo.
echo Executable location:
echo   build_viewer\viewers\do3think_camera_viewer\Release\Do3ThinkCameraViewer.exe
echo.
echo To run the application:
echo   cd build_viewer\viewers\do3think_camera_viewer\Release
echo   Do3ThinkCameraViewer.exe
echo.

cd ..