@echo off
REM Build script for QML Camera Viewer on Windows
REM Requires Qt6 and Visual Studio or MinGW

setlocal enabledelayedexpansion

REM Set Qt path (adjust as needed)
set QT_DIR=C:\Qt\6.9.0\msvc2019_64
set QT_BIN=%QT_DIR%\bin
set QT_LIB=%QT_DIR%\lib
set QT_INCLUDE=%QT_DIR%\include

REM Add Qt to PATH
set PATH=%QT_BIN%;%PATH%

REM Check if Qt tools exist
if not exist "%QT_BIN%\moc.exe" (
    echo Error: Qt MOC not found at %QT_BIN%\moc.exe
    echo Please adjust QT_DIR in this script
    exit /b 1
)

echo ========================================
echo Building QML Camera Viewer
echo Qt Directory: %QT_DIR%
echo ========================================

REM Create build directory
if not exist build mkdir build
cd build

REM Generate MOC files
echo Generating MOC files...
%QT_BIN%\moc.exe ..\qml_image_provider.h -o moc_qml_image_provider.cpp
%QT_BIN%\moc.exe ..\..\..\qml_bridge\camera_bridge.h -o moc_camera_bridge.cpp
%QT_BIN%\moc.exe ..\..\..\qml_bridge\machine_bridge.h -o moc_machine_bridge.cpp
%QT_BIN%\moc.exe ..\..\..\components\base_component.h -o moc_base_component.cpp
%QT_BIN%\moc.exe ..\..\..\components\camera_component.h -o moc_camera_component.cpp
%QT_BIN%\moc.exe ..\..\..\components\camera_control_panel.h -o moc_camera_control_panel.cpp
%QT_BIN%\moc.exe ..\..\..\Do3ThinkCamera\dothink_camera.h -o moc_dothink_camera.cpp
%QT_BIN%\moc.exe ..\..\..\Do3ThinkCamera\dothink_camera_control_panel.h -o moc_dothink_camera_control_panel.cpp
%QT_BIN%\moc.exe ..\..\do3think_camera_viewer\machine.h -o moc_machine.cpp

REM Generate resource file
echo Generating resource file...
echo ^<!DOCTYPE RCC^> > resources.qrc
echo ^<RCC version="1.0"^> >> resources.qrc
echo   ^<qresource prefix="/"^> >> resources.qrc
echo     ^<file^>../qml/main.qml^</file^> >> resources.qrc
echo     ^<file^>../qml/components/CameraControlPanel.qml^</file^> >> resources.qrc
echo     ^<file^>../qml/components/CameraListDelegate.qml^</file^> >> resources.qrc
echo     ^<file^>../qml/components/CameraParameters.qml^</file^> >> resources.qrc
echo     ^<file^>../qml/components/ConnectionIndicator.qml^</file^> >> resources.qrc
echo     ^<file^>../qml/components/SettingsPanel.qml^</file^> >> resources.qrc
echo     ^<file^>../qml/components/StatisticsPanel.qml^</file^> >> resources.qrc
echo     ^<file^>../qml/controls/ConnectionControls.qml^</file^> >> resources.qrc
echo     ^<file^>../qml/controls/AcquisitionControls.qml^</file^> >> resources.qrc
echo   ^</qresource^> >> resources.qrc
echo ^</RCC^> >> resources.qrc

%QT_BIN%\rcc.exe resources.qrc -o qrc_resources.cpp

REM Compile with Visual Studio compiler
echo Compiling source files...

REM Check for Visual Studio environment
if not defined VCINSTALLDIR (
    echo Setting up Visual Studio environment...
    call "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
    if errorlevel 1 (
        call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
        if errorlevel 1 (
            call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
            if errorlevel 1 (
                call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
                if errorlevel 1 (
                    echo Error: Visual Studio environment not found
                    echo Please install Visual Studio or adjust this script
                    exit /b 1
                )
            )
        )
    )
)

REM Set compiler flags
set INCLUDES=/I"%QT_INCLUDE%" /I"%QT_INCLUDE%\QtCore" /I"%QT_INCLUDE%\QtGui" /I"%QT_INCLUDE%\QtQuick" /I"%QT_INCLUDE%\QtQml" /I"%QT_INCLUDE%\QtQuickControls2" /I"%QT_INCLUDE%\QtWidgets" /I"..\..\.." /I"..\..\..\components" /I"..\..\..\Do3ThinkCamera" /I"..\..\..\qml_bridge" /I"..\..\do3think_camera_viewer" /I".."

set DEFINES=/DQT_NO_DEBUG /DQT_QUICK_LIB /DQT_GUI_LIB /DQT_QML_LIB /DQT_CORE_LIB /DCOMPONENTSFORESTCORE_STATIC_DEFINE /DWIN32 /D_WINDOWS

set CXXFLAGS=/std:c++17 /O2 /MD /EHsc %DEFINES% %INCLUDES%

REM Compile all source files
cl %CXXFLAGS% /c ..\main.cpp /Fomain.obj
cl %CXXFLAGS% /c ..\qml_image_provider.cpp /Foqml_image_provider.obj
cl %CXXFLAGS% /c ..\..\..\qml_bridge\camera_bridge.cpp /Focamera_bridge.obj
cl %CXXFLAGS% /c ..\..\..\qml_bridge\machine_bridge.cpp /Fomachine_bridge.obj
cl %CXXFLAGS% /c ..\..\..\components\base_component.cpp /Fobase_component.obj
cl %CXXFLAGS% /c ..\..\..\components\camera_component.cpp /Focamera_component.obj
cl %CXXFLAGS% /c ..\..\..\components\camera_control_panel.cpp /Focamera_control_panel.obj
cl %CXXFLAGS% /c ..\..\..\Do3ThinkCamera\dothink_camera.cpp /Fodothink_camera.obj
cl %CXXFLAGS% /c ..\..\..\Do3ThinkCamera\dothink_camera_control_panel.cpp /Fodothink_camera_control_panel.obj
cl %CXXFLAGS% /c ..\..\..\Do3ThinkCamera\dothink_camera_control_panel_methods.cpp /Fodothink_camera_control_panel_methods.obj
cl %CXXFLAGS% /c ..\..\..\Do3ThinkCamera\dvp_wrapper.cpp /Fodvp_wrapper.obj
cl %CXXFLAGS% /c ..\..\do3think_camera_viewer\machine.cpp /Fomachine.obj

REM Compile MOC files
cl %CXXFLAGS% /c moc_qml_image_provider.cpp /Fomoc_qml_image_provider.obj
cl %CXXFLAGS% /c moc_camera_bridge.cpp /Fomoc_camera_bridge.obj
cl %CXXFLAGS% /c moc_machine_bridge.cpp /Fomoc_machine_bridge.obj
cl %CXXFLAGS% /c moc_base_component.cpp /Fomoc_base_component.obj
cl %CXXFLAGS% /c moc_camera_component.cpp /Fomoc_camera_component.obj
cl %CXXFLAGS% /c moc_camera_control_panel.cpp /Fomoc_camera_control_panel.obj
cl %CXXFLAGS% /c moc_dothink_camera.cpp /Fomoc_dothink_camera.obj
cl %CXXFLAGS% /c moc_dothink_camera_control_panel.cpp /Fomoc_dothink_camera_control_panel.obj
cl %CXXFLAGS% /c moc_machine.cpp /Fomoc_machine.obj

REM Compile resource file
cl %CXXFLAGS% /c qrc_resources.cpp /Foqrc_resources.obj

REM Link the executable
echo Linking executable...
set LIBS=Qt6Core.lib Qt6Gui.lib Qt6Quick.lib Qt6QuickControls2.lib Qt6Qml.lib Qt6Widgets.lib Qt6Concurrent.lib kernel32.lib user32.lib shell32.lib uuid.lib ole32.lib advapi32.lib ws2_32.lib winmm.lib psapi.lib

link /OUT:QMLCameraViewer.exe /SUBSYSTEM:WINDOWS *.obj /LIBPATH:"%QT_LIB%" /LIBPATH:"..\..\..\Do3ThinkCamera\SDK" %LIBS% DVPCamera64.lib

if errorlevel 1 (
    echo Build failed!
    exit /b 1
)

REM Copy required DLLs
echo Copying required DLLs...
copy "%QT_BIN%\Qt6Core.dll" . >nul
copy "%QT_BIN%\Qt6Gui.dll" . >nul
copy "%QT_BIN%\Qt6Quick.dll" . >nul
copy "%QT_BIN%\Qt6QuickControls2.dll" . >nul
copy "%QT_BIN%\Qt6Qml.dll" . >nul
copy "%QT_BIN%\Qt6Widgets.dll" . >nul
copy "%QT_BIN%\Qt6Concurrent.dll" . >nul
copy "%QT_BIN%\Qt6Network.dll" . >nul
copy "%QT_BIN%\Qt6OpenGL.dll" . >nul

REM Copy Qt platform plugins (CRITICAL!)
if not exist platforms mkdir platforms
copy "%QT_DIR%\plugins\platforms\qwindows.dll" platforms\ >nul

REM Copy Qt style plugins
if not exist styles mkdir styles
copy "%QT_DIR%\plugins\styles\qwindowsvistastyle.dll" styles\ >nul 2>nul

REM Copy Qt Quick plugins
xcopy /E /I /Y "%QT_DIR%\plugins\qmltooling" qmltooling >nul 2>nul
xcopy /E /I /Y "%QT_DIR%\qml\QtQuick" QtQuick >nul 2>nul
xcopy /E /I /Y "%QT_DIR%\qml\QtQuick.2" QtQuick.2 >nul 2>nul
xcopy /E /I /Y "%QT_DIR%\qml\QtQml" QtQml >nul 2>nul

REM Copy Do3Think SDK DLL
copy "..\..\..\Do3ThinkCamera\SDK\DVPCamera64.dll" . >nul

echo ========================================
echo Build complete!
echo Executable: build\QMLCameraViewer.exe
echo ========================================

REM Return to original directory
cd ..

REM Create run script
echo @echo off > run_qml_viewer.bat
echo cd build >> run_qml_viewer.bat
echo set QML2_IMPORT_PATH=..\qml;%%QML2_IMPORT_PATH%% >> run_qml_viewer.bat
echo set QT_QUICK_CONTROLS_STYLE=Material >> run_qml_viewer.bat
echo QMLCameraViewer.exe %%* >> run_qml_viewer.bat
echo cd .. >> run_qml_viewer.bat

echo.
echo To run the application, use: run_qml_viewer.bat
echo.

endlocal