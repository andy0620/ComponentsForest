@echo off
REM Script to create import library from DVPCamera64.dll
REM This uses dumpbin and lib tools from Visual Studio

echo Creating import library for DVPCamera64.dll...

REM Check if Visual Studio tools are available
where dumpbin >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo Error: dumpbin not found. Please run this from Visual Studio Developer Command Prompt.
    exit /b 1
)

where lib >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo Error: lib not found. Please run this from Visual Studio Developer Command Prompt.
    exit /b 1
)

REM Find the DLL
set DLL_PATH=
if exist "C:\Program Files (x86)\Camera\DVP2 x64\DVPCamera64.dll" (
    set "DLL_PATH=C:\Program Files (x86)\Camera\DVP2 x64\DVPCamera64.dll"
) else if exist "C:\Program Files\Teledyne DALSA\Sherlockx64\Bin\DVPCamera64.dll" (
    set "DLL_PATH=C:\Program Files\Teledyne DALSA\Sherlockx64\Bin\DVPCamera64.dll"
) else (
    echo Error: DVPCamera64.dll not found in known locations.
    exit /b 1
)

echo Found DLL at: %DLL_PATH%

REM Export the functions
echo Exporting functions from DLL...
dumpbin /EXPORTS "%DLL_PATH%" > DVPCamera64.exports

REM Create DEF file
echo Creating DEF file...
echo LIBRARY DVPCamera64 > DVPCamera64.def
echo EXPORTS >> DVPCamera64.def

REM Parse exports and add to DEF file
for /f "skip=19 tokens=4" %%a in (DVPCamera64.exports) do (
    echo %%a | findstr /r "^dvp" >nul
    if not errorlevel 1 echo    %%a >> DVPCamera64.def
)

REM Create import library
echo Creating import library...
lib /DEF:DVPCamera64.def /OUT:DVPCamera64.lib /MACHINE:X64

if exist DVPCamera64.lib (
    echo Success! DVPCamera64.lib created.
    
    REM Create lib directory if it doesn't exist
    if not exist lib mkdir lib
    move DVPCamera64.lib lib\
    
    echo Library moved to lib\DVPCamera64.lib
) else (
    echo Error: Failed to create import library.
    exit /b 1
)

REM Clean up temporary files
del DVPCamera64.exports 2>nul
del DVPCamera64.def 2>nul
del DVPCamera64.exp 2>nul

echo Done!