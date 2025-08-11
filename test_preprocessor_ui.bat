@echo off
echo ========================================
echo PREPROCESSOR UI TEST VERIFICATION
echo ========================================
echo.
echo This script will help you verify the preprocessor UI changes.
echo.
echo STARTING THE APPLICATION...
echo.

REM Check if the executable exists
if not exist "build\viewers\do3think_camera_viewer\Release\Do3ThinkCameraViewerStandalone.exe" (
    echo ERROR: Executable not found!
    echo Please build the project first.
    exit /b 1
)

echo ========================================
echo TEST CHECKLIST - Please verify:
echo ========================================
echo.
echo 1. INITIAL STATE:
echo    [ ] The preprocessor dock is NOT visible when the app starts
echo    [ ] The main window shows camera panels only
echo.
echo 2. PREPROCESS BUTTON:
echo    [ ] Look for "Preprocess" in the Processing menu
echo    [ ] Look for "Preprocess" button in the toolbar
echo    [ ] The button/menu item shows as unchecked
echo    [ ] Hover over button - tooltip shows "Ctrl+P"
echo.
echo 3. SHOW THE DOCK:
echo    [ ] Click the "Preprocess" button
echo    [ ] A floating "Image Processing" window appears
echo    [ ] The window is positioned at (100, 100) from top-left
echo    [ ] The button now shows as checked/pressed
echo.
echo 4. DOCK CONTENT:
echo    [ ] The dock contains an "Edge Detection" tab
echo    [ ] The tab shows preprocessor controls
echo    [ ] You can adjust threshold, kernel size, etc.
echo.
echo 5. HIDE THE DOCK:
echo    [ ] Click "Preprocess" button again
echo    [ ] The dock disappears
echo    [ ] The button returns to unchecked state
echo.
echo 6. KEYBOARD SHORTCUT:
echo    [ ] Press Ctrl+P - the dock should appear
echo    [ ] Press Ctrl+P again - the dock should hide
echo.
echo 7. DOCK BEHAVIOR:
echo    [ ] Drag the floating dock to dock it to the right side
echo    [ ] Close and reopen - it remembers the position
echo    [ ] Click the X button on the dock - button unchecks
echo.
echo ========================================
echo Starting application now...
echo ========================================
echo.

REM Start the application
start "" "build\viewers\do3think_camera_viewer\Release\Do3ThinkCameraViewerStandalone.exe"

echo Application started. Please perform the verification steps above.
echo.
echo Press any key to exit this test script...
pause > nul