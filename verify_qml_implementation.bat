@echo off
setlocal enabledelayedexpansion

REM ################################################################################
REM # QML Camera Viewer Implementation Verification Script (Windows)
REM # 
REM # This script performs comprehensive verification of the QML implementation
REM # including architecture compliance, file existence, syntax validation,
REM # build verification, and dependency checks.
REM #
REM # Usage: verify_qml_implementation.bat [--verbose] [--fix]
REM ################################################################################

REM Configuration
set VERBOSE=0
set FIX_ISSUES=0
set REPORT_FILE=qml_verification_report_%date:~-4%%date:~4,2%%date:~7,2%_%time:~0,2%%time:~3,2%%time:~6,2%.txt
set REPORT_FILE=%REPORT_FILE: =0%
set TOTAL_CHECKS=0
set PASSED_CHECKS=0
set FAILED_CHECKS=0
set WARNINGS=0

REM Parse arguments
:parse_args
if "%1"=="" goto :start_verification
if "%1"=="--verbose" (
    set VERBOSE=1
    shift
    goto :parse_args
)
if "%1"=="--fix" (
    set FIX_ISSUES=1
    shift
    goto :parse_args
)
echo Unknown option: %1
echo Usage: %0 [--verbose] [--fix]
exit /b 1

:start_verification
echo ========================================= > "%REPORT_FILE%"
echo QML CAMERA VIEWER VERIFICATION REPORT >> "%REPORT_FILE%"
echo Date: %date% %time% >> "%REPORT_FILE%"
echo ========================================= >> "%REPORT_FILE%"

echo QML Camera Viewer Verification Starting...
echo.

REM ################################################################################
REM # 1. FILE EXISTENCE CHECKS
REM ################################################################################

:verify_file_structure
echo ========================================= >> "%REPORT_FILE%"
echo FILE STRUCTURE VERIFICATION >> "%REPORT_FILE%"
echo ========================================= >> "%REPORT_FILE%"
echo.
echo [1/10] Verifying File Structure...

REM Check core directories
call :check_directory "viewers\qml_camera_viewer" "QML viewer directory"
call :check_directory "qml_bridge" "QML bridge directory"
call :check_directory "viewers\qml_camera_viewer\qml" "QML files directory"
call :check_directory "components" "Components directory"
call :check_directory "include" "Include directory"
call :check_directory "src" "Source directory"

REM Check QML viewer files
call :check_file "viewers\qml_camera_viewer\main.cpp" "QML viewer main"
call :check_file "viewers\qml_camera_viewer\CMakeLists.txt" "QML CMakeLists"
call :check_file "viewers\qml_camera_viewer\qml_image_provider.cpp" "Image provider source"
call :check_file "viewers\qml_camera_viewer\qml_image_provider.h" "Image provider header"
call :check_file "viewers\qml_camera_viewer\qml.qrc" "QML resource file"

REM Check QML files
call :check_file "viewers\qml_camera_viewer\qml\main.qml" "Main QML file"
call :check_file "viewers\qml_camera_viewer\qml\CameraView.qml" "Camera view QML"
call :check_file "viewers\qml_camera_viewer\qml\CameraControlPanel.qml" "Control panel QML"
call :check_file "viewers\qml_camera_viewer\qml\StatusBar.qml" "Status bar QML"
call :check_file "viewers\qml_camera_viewer\qml\ThemeConstants.qml" "Theme constants QML"

REM Check bridge files
call :check_file "qml_bridge\camera_bridge.cpp" "Camera bridge source"
call :check_file "qml_bridge\camera_bridge.h" "Camera bridge header"
call :check_file "qml_bridge\machine_bridge.cpp" "Machine bridge source"
call :check_file "qml_bridge\machine_bridge.h" "Machine bridge header"

REM Check component files
call :check_file "components\base_component\base_component.cpp" "Base component source"
call :check_file "components\camera_component\camera_component.cpp" "Camera component source"
call :check_file "components\do3think_camera\dothink_camera_component.cpp" "Do3Think component"

REM ################################################################################
REM # 2. ARCHITECTURE COMPLIANCE CHECKS
REM ################################################################################

:verify_architecture
echo.
echo [2/10] Verifying Architecture Compliance...
echo ========================================= >> "%REPORT_FILE%"
echo ARCHITECTURE COMPLIANCE VERIFICATION >> "%REPORT_FILE%"
echo ========================================= >> "%REPORT_FILE%"

REM Check inheritance hierarchy
if exist "qml_bridge\camera_bridge.h" (
    findstr /C:"class CameraBridge" /C:"public QObject" "qml_bridge\camera_bridge.h" >nul 2>&1
    if !errorlevel! equ 0 (
        call :log_success "CameraBridge properly inherits from QObject"
    ) else (
        call :log_error "CameraBridge does not inherit from QObject"
    )
)

if exist "qml_bridge\machine_bridge.h" (
    findstr /C:"class MachineBridge" /C:"public QObject" "qml_bridge\machine_bridge.h" >nul 2>&1
    if !errorlevel! equ 0 (
        call :log_success "MachineBridge properly inherits from QObject"
    ) else (
        call :log_error "MachineBridge does not inherit from QObject"
    )
)

REM Check for Q_OBJECT macro
for %%f in (qml_bridge\*.h) do (
    findstr /C:"Q_OBJECT" "%%f" >nul 2>&1
    if !errorlevel! equ 0 (
        call :log_success "%%~nxf has Q_OBJECT macro"
    ) else (
        call :log_error "%%~nxf missing Q_OBJECT macro"
    )
)

REM Check signal/slot decoupling
for %%f in (qml_bridge\*.cpp) do (
    call :check_signal_slot_decoupling "%%f" "%%~nxf"
)

REM ################################################################################
REM # 3. QML SYNTAX VALIDATION
REM ################################################################################

:verify_qml_files
echo.
echo [3/10] Validating QML Files...
echo ========================================= >> "%REPORT_FILE%"
echo QML FILE VALIDATION >> "%REPORT_FILE%"
echo ========================================= >> "%REPORT_FILE%"

set QML_DIR=viewers\qml_camera_viewer\qml

if exist "%QML_DIR%" (
    for %%f in (%QML_DIR%\*.qml) do (
        call :validate_qml_syntax "%%f"
    )
) else (
    call :log_error "QML directory not found: %QML_DIR%"
)

REM Check QML resource file
if exist "viewers\qml_camera_viewer\qml.qrc" (
    call :log_success "QML resource file exists"
) else (
    call :log_error "QML resource file missing"
)

REM ################################################################################
REM # 4. DEPENDENCY CHECKS
REM ################################################################################

:verify_dependencies
echo.
echo [4/10] Checking Dependencies...
echo ========================================= >> "%REPORT_FILE%"
echo DEPENDENCY VERIFICATION >> "%REPORT_FILE%"
echo ========================================= >> "%REPORT_FILE%"

REM Check Qt installation
where qmake >nul 2>&1
if !errorlevel! equ 0 (
    for /f "tokens=*" %%i in ('qmake -query QT_VERSION') do set QT_VERSION=%%i
    call :log_success "Qt found: version !QT_VERSION!"
    
    echo !QT_VERSION! | findstr /C:"6." >nul
    if !errorlevel! equ 0 (
        call :log_success "Qt6 detected (required for QML viewer)"
    ) else (
        call :log_warning "Qt5 detected - Qt6 recommended for QML viewer"
    )
) else (
    call :log_error "Qt not found in PATH"
)

REM Check CMake
where cmake >nul 2>&1
if !errorlevel! equ 0 (
    for /f "tokens=3" %%i in ('cmake --version ^| findstr /C:"cmake version"') do set CMAKE_VERSION=%%i
    call :log_success "CMake found: version !CMAKE_VERSION!"
) else (
    call :log_error "CMake not found"
)

REM Check compiler
where cl >nul 2>&1
if !errorlevel! equ 0 (
    call :log_success "MSVC compiler found"
) else (
    where g++ >nul 2>&1
    if !errorlevel! equ 0 (
        call :log_success "G++ compiler found"
    ) else (
        call :log_error "No C++ compiler found"
    )
)

REM Check Do3Think SDK
if exist "Do3ThinkCamera\SDK" (
    call :log_success "Do3Think SDK directory found"
    
    if exist "Do3ThinkCamera\SDK\include\DVPCamera.h" (
        call :log_success "Do3Think SDK headers found"
    ) else (
        call :log_warning "Do3Think SDK headers not found"
    )
    
    if exist "Do3ThinkCamera\SDK\lib\win\x64\DVPCamera64.lib" (
        call :log_success "Do3Think SDK libraries found"
    ) else (
        call :log_warning "Do3Think SDK libraries not found"
    )
) else (
    call :log_warning "Do3Think SDK not found - camera functionality will be limited"
)

REM ################################################################################
REM # 5. BUILD VERIFICATION
REM ################################################################################

:attempt_build
echo.
echo [5/10] Attempting Build Verification...
echo ========================================= >> "%REPORT_FILE%"
echo BUILD VERIFICATION >> "%REPORT_FILE%"
echo ========================================= >> "%REPORT_FILE%"

set BUILD_DIR=viewers\qml_camera_viewer\build_test

REM Clean previous build
if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%"
mkdir "%BUILD_DIR%"
cd "%BUILD_DIR%"

echo Attempting CMake configuration...
cmake .. -G "Visual Studio 17 2022" -A x64 >cmake_output.log 2>&1
if !errorlevel! equ 0 (
    call :log_success "CMake configuration successful"
    
    echo Attempting build...
    cmake --build . --config Release >build_output.log 2>&1
    if !errorlevel! equ 0 (
        call :log_success "Build completed successfully"
        
        if exist "Release\QMLCameraViewer.exe" (
            call :log_success "Executable created: QMLCameraViewer.exe"
        ) else (
            call :log_warning "Executable not found after build"
        )
    ) else (
        call :log_error "Build failed - check build_output.log"
        if %VERBOSE%==1 (
            echo Last 20 lines of build output:
            powershell -Command "Get-Content build_output.log | Select-Object -Last 20"
        )
    )
) else (
    call :log_error "CMake configuration failed - check cmake_output.log"
    if %VERBOSE%==1 (
        echo Last 20 lines of cmake output:
        powershell -Command "Get-Content cmake_output.log | Select-Object -Last 20"
    )
)

cd ..\..\..

REM ################################################################################
REM # 6. THREAD SAFETY VERIFICATION
REM ################################################################################

:verify_thread_safety
echo.
echo [6/10] Verifying Thread Safety...
echo ========================================= >> "%REPORT_FILE%"
echo THREAD SAFETY VERIFICATION >> "%REPORT_FILE%"
echo ========================================= >> "%REPORT_FILE%"

REM Check for moveToThread usage
for %%f in (qml_bridge\*.cpp viewers\qml_camera_viewer\main.cpp) do (
    if exist "%%f" (
        findstr /C:"moveToThread" "%%f" >nul 2>&1
        if !errorlevel! equ 0 (
            call :log_success "%%~nxf uses moveToThread for threading"
        ) else (
            call :log_warning "%%~nxf may not use proper threading"
        )
    )
)

REM Check for QMutex usage
for %%f in (qml_bridge\*.cpp) do (
    if exist "%%f" (
        findstr /C:"QMutex" /C:"QMutexLocker" "%%f" >nul 2>&1
        if !errorlevel! equ 0 (
            call :log_success "%%~nxf uses mutex for thread safety"
        ) else (
            if %VERBOSE%==1 echo %%~nxf does not use explicit mutex (may be okay)
        )
    )
)

REM ################################################################################
REM # 7. QML INTEGRATION VERIFICATION
REM ################################################################################

:verify_qml_integration
echo.
echo [7/10] Verifying QML Integration...
echo ========================================= >> "%REPORT_FILE%"
echo QML INTEGRATION VERIFICATION >> "%REPORT_FILE%"
echo ========================================= >> "%REPORT_FILE%"

if exist "viewers\qml_camera_viewer\main.cpp" (
    findstr /C:"setContextProperty" "viewers\qml_camera_viewer\main.cpp" >nul 2>&1
    if !errorlevel! equ 0 (
        call :log_success "QML context properties are set"
        
        findstr /C:"cameraBridge" "viewers\qml_camera_viewer\main.cpp" >nul 2>&1
        if !errorlevel! equ 0 (
            call :log_success "Context property 'cameraBridge' is registered"
        ) else (
            call :log_error "Context property 'cameraBridge' not found"
        )
        
        findstr /C:"machineBridge" "viewers\qml_camera_viewer\main.cpp" >nul 2>&1
        if !errorlevel! equ 0 (
            call :log_success "Context property 'machineBridge' is registered"
        ) else (
            call :log_error "Context property 'machineBridge' not found"
        )
    ) else (
        call :log_error "No QML context properties found"
    )
    
    findstr /C:"addImageProvider" "viewers\qml_camera_viewer\main.cpp" >nul 2>&1
    if !errorlevel! equ 0 (
        call :log_success "QML image provider is registered"
    ) else (
        call :log_error "QML image provider not registered"
    )
)

REM ################################################################################
REM # 8. PERFORMANCE CHECKS
REM ################################################################################

:verify_performance
echo.
echo [8/10] Checking Performance Patterns...
echo ========================================= >> "%REPORT_FILE%"
echo PERFORMANCE PATTERN VERIFICATION >> "%REPORT_FILE%"
echo ========================================= >> "%REPORT_FILE%"

findstr /s /C:"QPixmapCache" /C:"QImageCache" qml_bridge\*.cpp viewers\qml_camera_viewer\*.cpp >nul 2>&1
if !errorlevel! equ 0 (
    call :log_success "Image caching patterns found"
) else (
    call :log_warning "No image caching patterns detected"
)

findstr /s /C:"30" /C:"33" /C:"60" qml_bridge\*.cpp | findstr /C:"fps" /C:"FPS" /C:"frame" >nul 2>&1
if !errorlevel! equ 0 (
    call :log_success "Frame rate limiting appears to be implemented"
) else (
    call :log_warning "Frame rate limiting not clearly defined"
)

REM ################################################################################
REM # 9. DOCUMENTATION VERIFICATION
REM ################################################################################

:verify_documentation
echo.
echo [9/10] Verifying Documentation...
echo ========================================= >> "%REPORT_FILE%"
echo DOCUMENTATION VERIFICATION >> "%REPORT_FILE%"
echo ========================================= >> "%REPORT_FILE%"

call :check_file "viewers\qml_camera_viewer\README_QML.md" "README_QML.md"
call :check_file "viewers\qml_camera_viewer\README_QML_VIEWER.md" "README_QML_VIEWER.md"
call :check_file "viewers\qml_camera_viewer\BUILD_GUIDE.md" "BUILD_GUIDE.md"
call :check_file "qml_bridge\README_BRIDGE_IMPLEMENTATION.md" "Bridge implementation docs"

REM ################################################################################
REM # 10. COMMON ISSUES CHECK
REM ################################################################################

:check_common_issues
echo.
echo [10/10] Checking for Common Issues...
echo ========================================= >> "%REPORT_FILE%"
echo COMMON ISSUES CHECK >> "%REPORT_FILE%"
echo ========================================= >> "%REPORT_FILE%"

REM Check for Qt platform plugins
if exist "viewers\qml_camera_viewer\build\platforms\qwindows.dll" (
    call :log_success "Qt platform plugins present"
) else (
    call :log_warning "Qt platform plugins missing in build directory"
)

REM ################################################################################
REM # FIX COMMON ISSUES
REM ################################################################################

:fix_issues
if %FIX_ISSUES%==1 (
    echo.
    echo Attempting to fix common issues...
    echo ========================================= >> "%REPORT_FILE%"
    echo ATTEMPTING TO FIX COMMON ISSUES >> "%REPORT_FILE%"
    echo ========================================= >> "%REPORT_FILE%"
    
    REM Create missing directories
    if not exist "viewers\qml_camera_viewer\qml" mkdir "viewers\qml_camera_viewer\qml"
    if not exist "qml_bridge" mkdir "qml_bridge"
    if not exist "viewers\qml_camera_viewer\build" mkdir "viewers\qml_camera_viewer\build"
    
    REM Copy Qt platform plugins if available
    where qmake >nul 2>&1
    if !errorlevel! equ 0 (
        for /f "tokens=*" %%i in ('qmake -query QT_INSTALL_PLUGINS') do set QT_PLUGINS_DIR=%%i
        if exist "!QT_PLUGINS_DIR!\platforms" (
            if not exist "viewers\qml_camera_viewer\build\platforms" mkdir "viewers\qml_camera_viewer\build\platforms"
            xcopy /Y /Q "!QT_PLUGINS_DIR!\platforms\*" "viewers\qml_camera_viewer\build\platforms\" >nul 2>&1
            echo Copied Qt platform plugins
        )
    )
    
    call :log_success "Applied available fixes"
)

REM ################################################################################
REM # GENERATE SUMMARY
REM ################################################################################

:generate_summary
echo.
echo ========================================= >> "%REPORT_FILE%"
echo VERIFICATION SUMMARY >> "%REPORT_FILE%"
echo ========================================= >> "%REPORT_FILE%"

echo.
echo =========================================
echo VERIFICATION SUMMARY
echo =========================================
echo Total Checks: %TOTAL_CHECKS%
echo Passed: %PASSED_CHECKS%
echo Failed: %FAILED_CHECKS%
echo Warnings: %WARNINGS%

echo Total Checks: %TOTAL_CHECKS% >> "%REPORT_FILE%"
echo Passed: %PASSED_CHECKS% >> "%REPORT_FILE%"
echo Failed: %FAILED_CHECKS% >> "%REPORT_FILE%"
echo Warnings: %WARNINGS% >> "%REPORT_FILE%"

REM Calculate success rate
if %TOTAL_CHECKS% gtr 0 (
    set /a SUCCESS_RATE=%PASSED_CHECKS%*100/%TOTAL_CHECKS%
) else (
    set SUCCESS_RATE=0
)

echo.
echo Success Rate: %SUCCESS_RATE%%%
echo Success Rate: %SUCCESS_RATE%%% >> "%REPORT_FILE%"

if %SUCCESS_RATE% geq 90 (
    echo.
    echo RESULT: Implementation is EXCELLENT
    echo RESULT: Implementation is EXCELLENT >> "%REPORT_FILE%"
    set EXIT_CODE=0
) else if %SUCCESS_RATE% geq 70 (
    echo.
    echo RESULT: Implementation is GOOD
    echo RESULT: Implementation is GOOD >> "%REPORT_FILE%"
    set EXIT_CODE=0
) else if %SUCCESS_RATE% geq 50 (
    echo.
    echo RESULT: Implementation needs improvement
    echo RESULT: Implementation needs improvement >> "%REPORT_FILE%"
    set EXIT_CODE=1
) else (
    echo.
    echo RESULT: Implementation has significant issues
    echo RESULT: Implementation has significant issues >> "%REPORT_FILE%"
    set EXIT_CODE=2
)

echo.
echo Full report saved to: %REPORT_FILE%
echo.

exit /b %EXIT_CODE%

REM ################################################################################
REM # HELPER FUNCTIONS
REM ################################################################################

:check_file
if exist "%~1" (
    call :log_success "%~2 exists"
) else (
    call :log_error "%~2 missing: %~1"
)
goto :eof

:check_directory
if exist "%~1\" (
    call :log_success "%~2 exists"
) else (
    call :log_error "%~2 missing: %~1"
)
goto :eof

:log_success
set /a PASSED_CHECKS+=1
set /a TOTAL_CHECKS+=1
echo [PASS] %~1
echo [PASS] %~1 >> "%REPORT_FILE%"
goto :eof

:log_error
set /a FAILED_CHECKS+=1
set /a TOTAL_CHECKS+=1
echo [FAIL] %~1
echo [FAIL] %~1 >> "%REPORT_FILE%"
goto :eof

:log_warning
set /a WARNINGS+=1
echo [WARN] %~1
echo [WARN] %~1 >> "%REPORT_FILE%"
goto :eof

:check_signal_slot_decoupling
set FILE=%~1
set BASENAME=%~2

REM Count string-based connections (good for decoupling)
for /f %%i in ('findstr /C:"connect.*SIGNAL.*SLOT" "%FILE%" 2^>nul ^| find /c /v ""') do set STRING_CONNECTS=%%i

REM Count compile-time connections (potentially bad for decoupling)
for /f %%i in ('findstr /R "connect.*&.*::.*&" "%FILE%" 2^>nul ^| find /c /v ""') do set DIRECT_CONNECTS=%%i

if %DIRECT_CONNECTS%==0 (
    call :log_success "%BASENAME% uses proper signal/slot decoupling"
) else if %STRING_CONNECTS% gtr %DIRECT_CONNECTS% (
    call :log_warning "%BASENAME% has some compile-time connections"
) else (
    call :log_error "%BASENAME% violates signal/slot decoupling principle"
)
goto :eof

:validate_qml_syntax
set QML_FILE=%~1
set BASENAME=%~nx1

REM Basic syntax checks
set ERRORS=

REM Check for import statements
findstr /B "import QtQuick" "%QML_FILE%" >nul 2>&1
if !errorlevel! neq 0 (
    set ERRORS=!ERRORS!Missing QtQuick import; 
)

REM Check for braces (simplified check)
for /f %%i in ('findstr /C:"{" "%QML_FILE%" 2^>nul ^| find /c /v ""') do set OPEN_BRACES=%%i
for /f %%i in ('findstr /C:"}" "%QML_FILE%" 2^>nul ^| find /c /v ""') do set CLOSE_BRACES=%%i

if !OPEN_BRACES! neq !CLOSE_BRACES! (
    set ERRORS=!ERRORS!Unmatched braces; 
)

if "!ERRORS!"=="" (
    call :log_success "%BASENAME% syntax appears valid"
) else (
    call :log_error "%BASENAME% syntax issues: !ERRORS!"
)
goto :eof