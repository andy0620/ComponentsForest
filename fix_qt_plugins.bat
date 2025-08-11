@echo off
REM Fix Qt platform plugins for Do3ThinkCameraViewerStandalone.exe

echo ========================================
echo Fixing Qt Platform Plugins
echo ========================================

set TARGET_DIR=build\viewers\do3think_camera_viewer\Release
set QT_DIR=C:\Qt\6.9.1\msvc2022_64

if not exist "%QT_DIR%" (
    echo ERROR: Qt installation not found at %QT_DIR%
    echo Please update the QT_DIR variable in this script to point to your Qt installation
    exit /b 1
)

echo Qt found at: %QT_DIR%

REM Create platforms directory
if not exist "%TARGET_DIR%\platforms" (
    echo Creating platforms directory...
    mkdir "%TARGET_DIR%\platforms"
)

REM Copy platform plugins
echo Copying platform plugins...
xcopy /Y /E "%QT_DIR%\plugins\platforms\*.dll" "%TARGET_DIR%\platforms\"

REM Create styles directory
if not exist "%TARGET_DIR%\styles" (
    echo Creating styles directory...
    mkdir "%TARGET_DIR%\styles"
)

REM Copy style plugins
echo Copying style plugins...
xcopy /Y /E "%QT_DIR%\plugins\styles\*.dll" "%TARGET_DIR%\styles\"

REM Create imageformats directory
if not exist "%TARGET_DIR%\imageformats" (
    echo Creating imageformats directory...
    mkdir "%TARGET_DIR%\imageformats"
)

REM Copy imageformats plugins
echo Copying imageformats plugins...
xcopy /Y /E "%QT_DIR%\plugins\imageformats\*.dll" "%TARGET_DIR%\imageformats\"

REM Create iconengines directory
if not exist "%TARGET_DIR%\iconengines" (
    echo Creating iconengines directory...
    mkdir "%TARGET_DIR%\iconengines"
)

REM Copy icon engine plugins
echo Copying icon engine plugins...
xcopy /Y /E "%QT_DIR%\plugins\iconengines\*.dll" "%TARGET_DIR%\iconengines\"

REM List what was copied
echo.
echo ========================================
echo Copied plugins:
echo ========================================
dir /B "%TARGET_DIR%\platforms"
echo.

REM Test the application
echo ========================================
echo Testing application with --minimal flag
echo ========================================
cd %TARGET_DIR%
Do3ThinkCameraViewerStandalone.exe --minimal

pause