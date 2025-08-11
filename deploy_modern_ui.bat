@echo off
REM Deployment script for Modern UI version
REM ========================================

echo ==========================================
echo Deploying Modern UI Application
echo ==========================================
echo.

REM Set paths
set SOURCE_DIR=build\viewers\do3think_camera_viewer\Release
set DEPLOY_DIR=Deploy\ModernUI
set QT_DIR=C:\Qt\6.9.1\msvc2022_64

REM Create deployment directory
echo Creating deployment directory...
if not exist %DEPLOY_DIR% mkdir %DEPLOY_DIR%

REM Copy executable
echo Copying executable...
copy %SOURCE_DIR%\Do3ThinkCameraViewerStandalone.exe %DEPLOY_DIR%\

REM Deploy Qt dependencies
echo Deploying Qt dependencies...
%QT_DIR%\bin\windeployqt.exe ^
    --release ^
    --no-translations ^
    --no-system-d3d-compiler ^
    --no-opengl-sw ^
    %DEPLOY_DIR%\Do3ThinkCameraViewerStandalone.exe

REM Copy Do3Think SDK DLLs if needed
echo Copying Do3Think SDK libraries...
if exist Do3ThinkCamera\SDK\lib\x64\DVPCamera64.dll (
    copy Do3ThinkCamera\SDK\lib\x64\DVPCamera64.dll %DEPLOY_DIR%\
)

REM Create info file
echo Creating deployment info...
echo Modern UI Build > %DEPLOY_DIR%\version.txt
echo Build Date: %date% %time% >> %DEPLOY_DIR%\version.txt
echo. >> %DEPLOY_DIR%\version.txt
echo UI Features: >> %DEPLOY_DIR%\version.txt
echo - JetBrains Mono font >> %DEPLOY_DIR%\version.txt
echo - Compact buttons (32-36px) >> %DEPLOY_DIR%\version.txt
echo - Tech colors (cyan/green accents) >> %DEPLOY_DIR%\version.txt
echo - Glass-morphism effects >> %DEPLOY_DIR%\version.txt
echo - Material Design 3 inspired >> %DEPLOY_DIR%\version.txt

REM Create run script
echo @echo off > %DEPLOY_DIR%\run.bat
echo echo Starting Modern UI Camera Viewer... >> %DEPLOY_DIR%\run.bat
echo start "" Do3ThinkCameraViewerStandalone.exe >> %DEPLOY_DIR%\run.bat

echo.
echo ==========================================
echo Deployment Complete!
echo ==========================================
echo.
echo Application deployed to: %DEPLOY_DIR%
echo Run with: %DEPLOY_DIR%\run.bat
echo.
pause