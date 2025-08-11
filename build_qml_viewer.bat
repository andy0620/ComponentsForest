@echo off
setlocal enabledelayedexpansion

echo ========================================
echo ComponentsForest QML Camera Viewer Build
echo ========================================
echo.

REM Auto-detect Qt installation or use provided path
if "%QT_DIR%"=="" (
    REM Try common Qt installation paths
    if exist "C:\Qt\6.9.1\msvc2022_64" (
        set QT_DIR=C:\Qt\6.9.1\msvc2022_64
    ) else if exist "C:\Qt\6.9.0\msvc2022_64" (
        set QT_DIR=C:\Qt\6.9.0\msvc2022_64
    ) else if exist "D:\Qt\6.9.1\msvc2022_64" (
        set QT_DIR=D:\Qt\6.9.1\msvc2022_64
    ) else (
        echo Error: Qt installation not found. Please set QT_DIR environment variable.
        echo Example: set QT_DIR=C:\Qt\6.9.1\msvc2022_64
        exit /b 1
    )
)

echo Using Qt from: %QT_DIR%
set PATH=%QT_DIR%\bin;%PATH%

REM Parse command line arguments
set BUILD_TYPE=Release
set CLEAN_BUILD=1
set DEPLOY_QT=1
set BUILD_STANDALONE=ON

:parse_args
if "%1"=="" goto end_parse
if /i "%1"=="--debug" (
    set BUILD_TYPE=Debug
    shift
    goto parse_args
)
if /i "%1"=="--no-clean" (
    set CLEAN_BUILD=0
    shift
    goto parse_args
)
if /i "%1"=="--no-deploy" (
    set DEPLOY_QT=0
    shift
    goto parse_args
)
if /i "%1"=="--integrated" (
    set BUILD_STANDALONE=OFF
    shift
    goto parse_args
)
if /i "%1"=="--help" (
    echo Usage: %0 [options]
    echo Options:
    echo   --debug        Build in Debug mode
    echo   --no-clean     Don't clean previous build
    echo   --no-deploy    Skip Qt deployment
    echo   --integrated   Build as part of ComponentsForest
    echo   --help         Show this help
    exit /b 0
)
shift
goto parse_args
:end_parse

REM Set build directory
set BUILD_DIR=build_qml_%BUILD_TYPE%

REM Clean previous build if requested
if %CLEAN_BUILD%==1 (
    if exist %BUILD_DIR% (
        echo Cleaning previous build...
        rmdir /s /q %BUILD_DIR%
    )
)

REM Create build directory
if not exist %BUILD_DIR% mkdir %BUILD_DIR%
cd %BUILD_DIR%

REM Detect Visual Studio version
set VS_GENERATOR="Visual Studio 17 2022"
if exist "%ProgramFiles%\Microsoft Visual Studio\2019" (
    set VS_GENERATOR="Visual Studio 16 2019"
)

REM Configure with CMake
echo.
echo Configuring project with:
echo   Generator: %VS_GENERATOR%
echo   Build Type: %BUILD_TYPE%
echo   Standalone: %BUILD_STANDALONE%
echo.

cmake ../viewers/qml_camera_viewer -G %VS_GENERATOR% -A x64 ^
    -DCMAKE_PREFIX_PATH=%QT_DIR% ^
    -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
    -DBUILD_STANDALONE=%BUILD_STANDALONE% ^
    -DDEPLOY_QT_RUNTIME=%DEPLOY_QT%

if %errorlevel% neq 0 (
    echo Configuration failed!
    pause
    exit /b 1
)

REM Build the project
echo.
echo Building project...
echo.

REM Get number of CPU cores for parallel build
for /f "tokens=2 delims==" %%i in ('wmic cpu get NumberOfCores /value ^| findstr NumberOfCores') do set CORES=%%i

cmake --build . --config %BUILD_TYPE% --parallel %CORES%

if %errorlevel% neq 0 (
    echo Build failed!
    pause
    exit /b 1
)

REM Deploy Qt dependencies if requested
if %DEPLOY_QT%==1 (
    echo.
    echo Deploying Qt dependencies...
    echo.
    
    set OUTPUT_DIR=%BUILD_TYPE%
    if not exist !OUTPUT_DIR!\QMLCameraViewer.exe (
        set OUTPUT_DIR=bin\%BUILD_TYPE%
    )
    if not exist !OUTPUT_DIR!\QMLCameraViewer.exe (
        set OUTPUT_DIR=bin
    )
    
    if exist !OUTPUT_DIR!\QMLCameraViewer.exe (
        %QT_DIR%\bin\windeployqt.exe ^
            --qmldir ..\..\viewers\qml_camera_viewer\qml ^
            --quick ^
            --no-translations ^
            --no-system-d3d-compiler ^
            --no-opengl-sw ^
            !OUTPUT_DIR!\QMLCameraViewer.exe
    ) else (
        echo Warning: Could not find executable for deployment
    )
)

REM Copy Do3Think DLL if it exists
if exist "..\..\Do3ThinkCamera\SDK\DVPCamera64.dll" (
    echo.
    echo Copying Do3Think SDK...
    
    set OUTPUT_DIR=%BUILD_TYPE%
    if not exist !OUTPUT_DIR! set OUTPUT_DIR=bin\%BUILD_TYPE%
    if not exist !OUTPUT_DIR! set OUTPUT_DIR=bin
    
    if exist !OUTPUT_DIR! (
        copy /Y "..\..\Do3ThinkCamera\SDK\DVPCamera64.dll" !OUTPUT_DIR!\
    )
) else (
    echo Info: Do3Think SDK not found, will use dynamic loading if available
)

echo.
echo ========================================
echo Build completed successfully!
echo.

REM Find the actual executable location
set EXE_FOUND=0
for %%D in (%BUILD_TYPE% bin\%BUILD_TYPE% bin) do (
    if exist %%D\QMLCameraViewer.exe (
        echo Executable: %BUILD_DIR%\%%D\QMLCameraViewer.exe
        set EXE_FOUND=1
        goto exe_found
    )
)
:exe_found

if %EXE_FOUND%==0 (
    echo Warning: Could not locate the built executable
)

echo.
echo To run the application:
echo   cd %BUILD_DIR%
echo   %BUILD_TYPE%\QMLCameraViewer.exe
echo.
echo Build options used:
echo   Build Type: %BUILD_TYPE%
echo   Standalone: %BUILD_STANDALONE%
echo   Qt Deployed: %DEPLOY_QT%
echo ========================================

cd ..

REM Offer to run the application
if %EXE_FOUND%==1 (
    echo.
    set /p RUN_APP="Run the application now? (Y/N): "
    if /i "!RUN_APP!"=="Y" (
        cd %BUILD_DIR%
        for %%D in (%BUILD_TYPE% bin\%BUILD_TYPE% bin) do (
            if exist %%D\QMLCameraViewer.exe (
                start "" %%D\QMLCameraViewer.exe
                goto end
            )
        )
    )
)

:end
pause