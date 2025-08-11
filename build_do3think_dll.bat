@echo off
REM Build script for Do3ThinkCameraComponent.dll

echo Building Do3ThinkCameraComponent.dll...

REM Set up Visual Studio environment
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64

REM Set Qt paths
set QT_PATH=C:\Qt\6.9.1\msvc2022_64
set PATH=%QT_PATH%\bin;%PATH%

REM Create output directory
if not exist "build\Release" mkdir "build\Release"

REM Compile the stub library first
echo Compiling DVPCamera stub library...
cl /c /EHsc /MD /std:c++17 /Zc:__cplusplus /I"Do3ThinkCamera\SDK" ^
   /Fo"build\Release\DVPCamera_stub.obj" ^
   "Do3ThinkCamera\SDK\DVPCamera_stub.cpp"

lib /OUT:"build\Release\DVPCamera64.lib" "build\Release\DVPCamera_stub.obj"

REM Compile Do3ThinkCameraComponent
echo Compiling Do3ThinkCameraComponent...
cl /LD /EHsc /MD /std:c++17 /Zc:__cplusplus ^
   /I"." ^
   /I"components" ^
   /I"Do3ThinkCamera" ^
   /I"Do3ThinkCamera\SDK" ^
   /I"%QT_PATH%\include" ^
   /I"%QT_PATH%\include\QtCore" ^
   /I"%QT_PATH%\include\QtGui" ^
   /I"%QT_PATH%\include\QtWidgets" ^
   /I"%QT_PATH%\include\QtCharts" ^
   /DQT_CORE_LIB /DQT_GUI_LIB /DQT_WIDGETS_LIB /DQT_CHARTS_LIB /DWIN32 /D_WINDOWS /DUSE_DYNAMIC_LOADING ^
   /Fe"build\Release\Do3ThinkCameraComponent.dll" ^
   "Do3ThinkCamera\dothink_camera.cpp" ^
   "Do3ThinkCamera\dothink_camera_control_panel.cpp" ^
   "components\base_component.cpp" ^
   "components\camera_component.cpp" ^
   "components\camera_control_panel.cpp" ^
   /link ^
   /LIBPATH:"%QT_PATH%\lib" ^
   /LIBPATH:"build\Release" ^
   Qt6Core.lib Qt6Gui.lib Qt6Widgets.lib Qt6Charts.lib DVPCamera64.lib

if %ERRORLEVEL% == 0 (
    echo.
    echo SUCCESS: Do3ThinkCameraComponent.dll has been built successfully!
    echo Location: build\Release\Do3ThinkCameraComponent.dll
) else (
    echo.
    echo ERROR: Build failed!
)

pause