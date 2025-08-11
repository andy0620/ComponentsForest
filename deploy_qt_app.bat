@echo off
REM Complete Qt deployment script for Do3ThinkCameraViewerStandalone

echo ============================================
echo Qt Application Deployment Tool
echo ============================================

set TARGET_DIR=build\viewers\do3think_camera_viewer\Release
set QT_DIR=C:\Qt\6.9.1\msvc2022_64
set APP_NAME=Do3ThinkCameraViewerStandalone.exe

REM Check if Qt is installed
if not exist "%QT_DIR%" (
    echo ERROR: Qt not found at %QT_DIR%
    echo.
    echo Trying alternative locations...
    
    if exist "C:\Qt\6.9.0\msvc2022_64" (
        set QT_DIR=C:\Qt\6.9.0\msvc2022_64
        echo Found Qt at: %QT_DIR%
    ) else if exist "D:\Qt\6.9.1\msvc2022_64" (
        set QT_DIR=D:\Qt\6.9.1\msvc2022_64
        echo Found Qt at: %QT_DIR%
    ) else if exist "D:\Qt\6.9.0\msvc2022_64" (
        set QT_DIR=D:\Qt\6.9.0\msvc2022_64
        echo Found Qt at: %QT_DIR%
    ) else (
        echo ERROR: Could not find Qt installation
        echo Please install Qt or update the QT_DIR variable in this script
        pause
        exit /b 1
    )
)

echo Using Qt from: %QT_DIR%
echo.

REM Use windeployqt if available
if exist "%QT_DIR%\bin\windeployqt.exe" (
    echo ============================================
    echo Using windeployqt for automatic deployment
    echo ============================================
    
    cd %TARGET_DIR%
    echo Running windeployqt...
    "%QT_DIR%\bin\windeployqt.exe" --release --no-translations --no-system-d3d-compiler --no-opengl-sw %APP_NAME%
    
    echo.
    echo Deployment complete!
    
) else (
    echo ============================================
    echo Manual deployment (windeployqt not found)
    echo ============================================
    
    REM Manual copy of essential plugins
    
    REM Platforms (REQUIRED)
    echo Copying platform plugins...
    if not exist "%TARGET_DIR%\platforms" mkdir "%TARGET_DIR%\platforms"
    copy /Y "%QT_DIR%\plugins\platforms\qwindows.dll" "%TARGET_DIR%\platforms\"
    copy /Y "%QT_DIR%\plugins\platforms\qoffscreen.dll" "%TARGET_DIR%\platforms\" 2>nul
    copy /Y "%QT_DIR%\plugins\platforms\qminimal.dll" "%TARGET_DIR%\platforms\" 2>nul
    
    REM Styles
    echo Copying style plugins...
    if not exist "%TARGET_DIR%\styles" mkdir "%TARGET_DIR%\styles"
    copy /Y "%QT_DIR%\plugins\styles\qwindowsvistastyle.dll" "%TARGET_DIR%\styles\" 2>nul
    
    REM Image formats
    echo Copying image format plugins...
    if not exist "%TARGET_DIR%\imageformats" mkdir "%TARGET_DIR%\imageformats"
    copy /Y "%QT_DIR%\plugins\imageformats\qico.dll" "%TARGET_DIR%\imageformats\" 2>nul
    copy /Y "%QT_DIR%\plugins\imageformats\qjpeg.dll" "%TARGET_DIR%\imageformats\" 2>nul
    copy /Y "%QT_DIR%\plugins\imageformats\qpng.dll" "%TARGET_DIR%\imageformats\" 2>nul
    
    REM Icon engines
    echo Copying icon engine plugins...
    if not exist "%TARGET_DIR%\iconengines" mkdir "%TARGET_DIR%\iconengines"
    copy /Y "%QT_DIR%\plugins\iconengines\qsvgicon.dll" "%TARGET_DIR%\iconengines\" 2>nul
)

REM Verify deployment
echo.
echo ============================================
echo Verifying deployment...
echo ============================================

if exist "%TARGET_DIR%\platforms\qwindows.dll" (
    echo [OK] Platform plugin qwindows.dll found
) else (
    echo [ERROR] Platform plugin qwindows.dll NOT FOUND!
    echo The application will not start without this file.
)

REM List all DLLs in the target directory
echo.
echo Files in target directory:
dir /B "%TARGET_DIR%\*.dll" "%TARGET_DIR%\*.exe"

echo.
echo Platform plugins:
if exist "%TARGET_DIR%\platforms" (
    dir /B "%TARGET_DIR%\platforms\*.dll" 2>nul || echo   No platform plugins found!
) else (
    echo   platforms directory not found!
)

REM Create a qt.conf file to help Qt find plugins
echo.
echo Creating qt.conf file...
echo [Paths] > "%TARGET_DIR%\qt.conf"
echo Plugins = . >> "%TARGET_DIR%\qt.conf"
echo Libraries = . >> "%TARGET_DIR%\qt.conf"
echo.
echo qt.conf created.

REM Test the application
echo.
echo ============================================
echo Testing application...
echo ============================================
cd %TARGET_DIR%

REM First test with minimal flag
echo Testing with --minimal flag...
%APP_NAME% --minimal

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ============================================
    echo SUCCESS: Application started successfully!
    echo ============================================
    echo.
    echo You can now run the full application:
    echo   %TARGET_DIR%\%APP_NAME%
) else (
    echo.
    echo ============================================
    echo ERROR: Application failed to start
    echo ============================================
    echo.
    echo Check debug.log for details.
    echo Common issues:
    echo   1. Missing Visual C++ Redistributables
    echo   2. Missing Qt dependencies
    echo   3. Incompatible Qt version
)

pause