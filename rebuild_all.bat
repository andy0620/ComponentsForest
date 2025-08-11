@echo off
REM Complete rebuild script for ComponentsForest with Do3Think components

echo ========================================
echo ComponentsForest Complete Build Script
echo ========================================
echo.

REM Set up Visual Studio environment
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64

REM Clean build directory
echo Cleaning build directory...
if exist build rmdir /s /q build
mkdir build
cd build

REM Configure with CMake
echo.
echo Configuring with CMake...
cmake .. -G "Visual Studio 17 2022" -A x64 -DBUILD_EXAMPLES=OFF -DCMAKE_BUILD_TYPE=Release

if %ERRORLEVEL% neq 0 (
    echo ERROR: CMake configuration failed!
    cd ..
    pause
    exit /b 1
)

REM Build the solution
echo.
echo Building solution...
cmake --build . --config Release --parallel

if %ERRORLEVEL% neq 0 (
    echo ERROR: Build failed!
    cd ..
    pause
    exit /b 1
)

REM Package the DLLs
echo.
echo Packaging DLLs...
cd ..
if not exist package\Release mkdir package\Release

copy /Y build\Release\ComponentsForestCore.dll package\Release\
copy /Y build\Release\ComponentsForestCore.lib package\Release\
copy /Y build\Release\Do3ThinkCameraComponent.dll package\Release\
copy /Y build\Release\Do3ThinkCameraComponent.lib package\Release\

REM Copy DVPCamera64.dll if it exists
if exist "C:\Program Files (x86)\Camera\DVP2 x64\DVPCamera64.dll" (
    copy /Y "C:\Program Files (x86)\Camera\DVP2 x64\DVPCamera64.dll" package\Release\
)

echo.
echo ========================================
echo BUILD SUCCESSFUL!
echo ========================================
echo.
echo Built DLLs are in: package\Release\
echo.
dir package\Release\*.dll

echo.
pause