@echo off
REM Direct MSBuild script for ROI System
REM =====================================

echo ==========================================
echo Building ROI System with MSBuild
echo ==========================================
echo.

cd build\OpenCV

REM Build ROISystem library
echo Building ROISystem.dll...
msbuild ROISystem.vcxproj /p:Configuration=Release /p:Platform=x64

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ERROR: ROISystem build failed
    echo.
    cd ..\..
    pause
    exit /b 1
)

REM Build ROI Example
echo.
echo Building ROIExample.exe...
msbuild ROIExample.vcxproj /p:Configuration=Release /p:Platform=x64

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo WARNING: ROIExample build failed (this is optional)
    echo.
)

echo.
echo ==========================================
echo Build completed!
echo ==========================================
echo.
echo Output files:
echo   - Library: build\OpenCV\Release\ROISystem.dll
echo   - Example: build\OpenCV\Release\ROIExample.exe
echo.

cd ..\..
pause