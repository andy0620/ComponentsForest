@echo off
echo ==========================================
echo FIXING Qt Platform Plugin Issue
echo ==========================================
echo.

REM Create platforms directory
echo Creating platforms directory...
mkdir "build\viewers\do3think_camera_viewer\Release\platforms" 2>nul

REM Try different Qt locations
echo Searching for Qt installation...

if exist "C:\Qt\6.9.1\msvc2022_64\plugins\platforms\qwindows.dll" (
    echo Found Qt at C:\Qt\6.9.1\msvc2022_64
    copy "C:\Qt\6.9.1\msvc2022_64\plugins\platforms\qwindows.dll" "build\viewers\do3think_camera_viewer\Release\platforms\" >nul
    copy "C:\Qt\6.9.1\msvc2022_64\plugins\platforms\qoffscreen.dll" "build\viewers\do3think_camera_viewer\Release\platforms\" >nul 2>&1
    copy "C:\Qt\6.9.1\msvc2022_64\plugins\platforms\qminimal.dll" "build\viewers\do3think_camera_viewer\Release\platforms\" >nul 2>&1
    echo Platform plugins copied successfully!
    goto :test
)

if exist "C:\Qt\6.9.0\msvc2022_64\plugins\platforms\qwindows.dll" (
    echo Found Qt at C:\Qt\6.9.0\msvc2022_64
    copy "C:\Qt\6.9.0\msvc2022_64\plugins\platforms\qwindows.dll" "build\viewers\do3think_camera_viewer\Release\platforms\" >nul
    echo Platform plugins copied successfully!
    goto :test
)

if exist "D:\Qt\6.9.1\msvc2022_64\plugins\platforms\qwindows.dll" (
    echo Found Qt at D:\Qt\6.9.1\msvc2022_64
    copy "D:\Qt\6.9.1\msvc2022_64\plugins\platforms\qwindows.dll" "build\viewers\do3think_camera_viewer\Release\platforms\" >nul
    echo Platform plugins copied successfully!
    goto :test
)

echo ERROR: Could not find Qt installation!
echo.
echo Please manually copy qwindows.dll from:
echo   [Your Qt Path]\plugins\platforms\qwindows.dll
echo To:
echo   %CD%\build\viewers\do3think_camera_viewer\Release\platforms\
echo.
pause
exit /b 1

:test
echo.
echo Testing application with --minimal flag...
cd build\viewers\do3think_camera_viewer\Release
echo.
echo Starting application...
start Do3ThinkCameraViewerStandalone.exe --minimal

echo.
echo ==========================================
echo FIX APPLIED SUCCESSFULLY!
echo ==========================================
echo.
echo The application should now start!
echo.
echo If you see a test message box, the fix worked.
echo Close the message box to see the main window.
echo.
pause