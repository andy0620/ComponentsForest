@echo off
REM Diagnostic script for Qt application issues

echo ================================================
echo Qt Application Diagnostic Tool
echo ================================================
echo.

set TARGET_DIR=build\viewers\do3think_camera_viewer\Release
set APP_NAME=Do3ThinkCameraViewerStandalone.exe

cd %TARGET_DIR%

echo Current directory: %CD%
echo.

echo ================================================
echo 1. Checking for application executable
echo ================================================
if exist %APP_NAME% (
    echo [OK] %APP_NAME% found
    for %%F in (%APP_NAME%) do echo      Size: %%~zF bytes
) else (
    echo [ERROR] %APP_NAME% NOT FOUND!
    goto :end
)
echo.

echo ================================================
echo 2. Checking for Qt Core DLLs
echo ================================================
if exist Qt6Core.dll (
    echo [OK] Qt6Core.dll found
) else (
    echo [ERROR] Qt6Core.dll NOT FOUND!
)

if exist Qt6Gui.dll (
    echo [OK] Qt6Gui.dll found
) else (
    echo [ERROR] Qt6Gui.dll NOT FOUND!
)

if exist Qt6Widgets.dll (
    echo [OK] Qt6Widgets.dll found
) else (
    echo [ERROR] Qt6Widgets.dll NOT FOUND!
)
echo.

echo ================================================
echo 3. Checking for platform plugins (CRITICAL)
echo ================================================
if exist platforms\qwindows.dll (
    echo [OK] platforms\qwindows.dll found
    for %%F in (platforms\qwindows.dll) do echo      Size: %%~zF bytes
) else (
    echo [ERROR] platforms\qwindows.dll NOT FOUND!
    echo.
    echo THIS IS THE MOST COMMON CAUSE OF STARTUP FAILURE!
    echo The application cannot start without platform plugins.
    echo.
    echo Solution: Run deploy_qt_app.bat to copy the necessary files.
)
echo.

echo ================================================
echo 4. Checking for other plugins
echo ================================================
if exist styles (
    echo [INFO] styles directory exists
    dir /B styles\*.dll 2>nul || echo        No style plugins found
) else (
    echo [INFO] styles directory not found (optional)
)

if exist imageformats (
    echo [INFO] imageformats directory exists
    dir /B imageformats\*.dll 2>nul || echo        No image format plugins found
) else (
    echo [INFO] imageformats directory not found (optional)
)
echo.

echo ================================================
echo 5. Checking environment variables
echo ================================================
echo QT_PLUGIN_PATH: %QT_PLUGIN_PATH%
if "%QT_PLUGIN_PATH%"=="" echo     (not set - this is usually OK)

echo QT_QPA_PLATFORM_PLUGIN_PATH: %QT_QPA_PLATFORM_PLUGIN_PATH%
if "%QT_QPA_PLATFORM_PLUGIN_PATH%"=="" echo     (not set - this is usually OK)

echo PATH contains Qt: 
echo %PATH% | findstr /i "qt" >nul
if %ERRORLEVEL% EQU 0 (
    echo     Yes - Qt found in PATH
) else (
    echo     No - Qt not in PATH (this is OK if plugins are deployed)
)
echo.

echo ================================================
echo 6. Checking qt.conf
echo ================================================
if exist qt.conf (
    echo [OK] qt.conf found:
    type qt.conf
) else (
    echo [INFO] qt.conf not found (optional)
)
echo.

echo ================================================
echo 7. Dependency Walker Check (if available)
echo ================================================
where dumpbin >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo Using dumpbin to check dependencies...
    dumpbin /dependents %APP_NAME% | findstr /i "dll"
) else (
    echo dumpbin not found (Visual Studio tool)
)
echo.

echo ================================================
echo 8. Testing with different platform settings
echo ================================================

echo Test 1: Default platform
set QT_QPA_PLATFORM=
echo QT_QPA_PLATFORM not set (using default)
%APP_NAME% --minimal 2>test_default.err
if %ERRORLEVEL% NEQ 0 (
    echo [FAILED] Default platform test failed
    if exist test_default.err (
        echo Error output:
        type test_default.err
    )
) else (
    echo [OK] Default platform works
)
echo.

echo Test 2: Explicitly set windows platform
set QT_QPA_PLATFORM=windows
echo QT_QPA_PLATFORM=windows
%APP_NAME% --minimal 2>test_windows.err
if %ERRORLEVEL% NEQ 0 (
    echo [FAILED] Windows platform test failed
    if exist test_windows.err (
        echo Error output:
        type test_windows.err
    )
) else (
    echo [OK] Windows platform works
)
echo.

echo Test 3: Offscreen platform (for testing)
set QT_QPA_PLATFORM=offscreen
echo QT_QPA_PLATFORM=offscreen
%APP_NAME% --minimal 2>test_offscreen.err
if %ERRORLEVEL% NEQ 0 (
    echo [FAILED] Offscreen platform test failed
    if exist test_offscreen.err (
        echo Error output:
        type test_offscreen.err
    )
) else (
    echo [OK] Offscreen platform works (no GUI)
)
echo.

REM Clean up test files
del test_*.err 2>nul

echo ================================================
echo 9. Debug log analysis
echo ================================================
if exist debug.log (
    echo Last 20 lines of debug.log:
    echo --------------------------------
    powershell -Command "Get-Content debug.log -Tail 20"
) else (
    echo No debug.log found
)
echo.

echo ================================================
echo DIAGNOSIS COMPLETE
echo ================================================
echo.
echo Common solutions:
echo 1. Run deploy_qt_app.bat to copy Qt plugins
echo 2. Install Visual C++ Redistributables
echo 3. Check that Qt version matches the build
echo.

:end
pause