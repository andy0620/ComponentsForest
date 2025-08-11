@echo off
REM Build script for ComponentsForestCore.dll (working component)

echo Building ComponentsForestCore.dll...

REM Navigate to build directory
cd /d "%~dp0\build"

REM Build using MSBuild
echo Using MSBuild to build ComponentsForestCore...
"C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" ComponentsForestCore.vcxproj /p:Configuration=Release /p:Platform=x64 /v:minimal

if %ERRORLEVEL% == 0 (
    echo.
    echo SUCCESS: ComponentsForestCore.dll has been built successfully!
    echo Location: build\Release\ComponentsForestCore.dll
    
    REM Package the DLL
    echo.
    echo Packaging DLL...
    if not exist "..\package\Release" mkdir "..\package\Release"
    copy /Y Release\ComponentsForestCore.* ..\package\Release\
    
    echo.
    echo Package created in: package\Release\
    dir ..\package\Release\*.dll
) else (
    echo.
    echo ERROR: Build failed!
)

cd /d "%~dp0"
pause