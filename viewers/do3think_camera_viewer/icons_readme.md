# Icons Required for Modern UI

The modern UI theme references the following icons that should be added to the Qt resource file:

## Menu and Toolbar Icons (24x24 px recommended)
- `/icons/camera.png` - Main camera icon for window
- `/icons/camera-large.png` - Large camera icon (128x128 px) for welcome screen
- `/icons/settings.png` - Settings/gear icon
- `/icons/exit.png` - Exit/close application icon
- `/icons/add-camera.png` - Add/plus camera icon
- `/icons/remove-camera.png` - Remove/minus camera icon
- `/icons/play-all.png` - Play/start all icon
- `/icons/stop-all.png` - Stop all icon
- `/icons/refresh.png` - Refresh/reload icon
- `/icons/fullscreen.png` - Fullscreen toggle icon
- `/icons/layout.png` - Layout/grid icon
- `/icons/about.png` - Info/about icon
- `/icons/info.png` - Information icon
- `/icons/stats.png` - Statistics/chart icon
- `/icons/clear.png` - Clear/trash icon
- `/icons/save.png` - Save/disk icon

## Status Icons (16x16 px recommended)
- `/icons/status-ok.png` - Green check or circle for OK status
- `/icons/status-error.png` - Red X or circle for error status
- `/icons/status-warning.png` - Yellow triangle for warning

## UI Control Icons
- `/icons/close.png` - Small close X for dock widgets
- `/icons/close-small.png` - Tiny close X for tabs
- `/icons/float.png` - Float/undock icon
- `/icons/chevron-down.png` - Dropdown arrow
- `/icons/check.png` - Checkmark for checkboxes

## Icon Style Guidelines
- Use flat, monochrome icons that work well on dark backgrounds
- Primary icons should be white or light gray (#f0f6fc)
- Consider using Font Awesome or Material Icons as sources
- SVG format recommended for scalability
- Add colored versions for special states (hover, active)

## Creating a Qt Resource File

Create a `resources.qrc` file in the project:

```xml
<!DOCTYPE RCC>
<RCC version="1.0">
    <qresource prefix="/">
        <file>icons/camera.png</file>
        <file>icons/camera-large.png</file>
        <file>icons/settings.png</file>
        <!-- Add all other icon files here -->
    </qresource>
</RCC>
```

Then add to CMakeLists.txt:
```cmake
qt6_add_resources(RESOURCES resources.qrc)
```

## Alternative: Use Icon Fonts

For a more scalable solution, consider using icon fonts like Font Awesome:

```cpp
// In the code, use Unicode characters for icons
QString cameraIcon = "\uf030";  // Font Awesome camera
QString settingsIcon = "\uf013"; // Font Awesome cog
```

This approach eliminates the need for separate icon files and provides perfect scaling at any size.