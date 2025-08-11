# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

ComponentsForest is a Qt6.9-based industrial automation AOI (Automated Optical Inspection) equipment component ecosystem implementing a three-tier architecture with complete Signal/Slot decoupling between components and UI.

## Build Commands

```bash
# QWidget Version (Original)
quick_build_qt691.bat C:\Qt\6.9.1\msvc2019_64  # Windows
./quick_build_linux.sh ~/Qt/6.9.1/gcc_64       # Linux

# QML Version (New)
build_qml_viewer.bat     # Windows
./build_qml_viewer.sh    # Linux

# Standard CMake build
mkdir build && cd build
cmake .. -DCMAKE_PREFIX_PATH=/path/to/Qt/6.9.0 -DCMAKE_BUILD_TYPE=Release
cmake --build . --parallel

# Full rebuild all components (Windows)
rebuild_all.bat
```

## Architecture - Three-Tier Design

```
Foundation Layer: BaseComponent (base class for all components)
     ↓
Abstraction Layer: CameraComponent, CameraControlPanel (common interfaces)
     ↓  
Implementation Layer: Do3ThinkCameraComponent, Do3ThinkCameraControlPanel (specific implementations)
```

### Critical Architecture Rules
1. **Components and Control Panels communicate ONLY through Signal/Slot** - no direct dependencies
2. **Each component runs in its own QThread** - never access components directly from UI thread
3. **Control Panels are the SOLE interface for camera control from UI** - MainUI never controls cameras directly

## Common Development Tasks

### Thread-Safe Component Pattern
```cpp
// Components MUST be moved to thread BEFORE connecting signals
m_component->moveToThread(m_thread);
connect(panel, SIGNAL(requestStart()), component, SLOT(start()), Qt::QueuedConnection);
```

### Do3Think SDK Integration
```cpp
dvpInit();                        // Global init (once per app)
dvpRefresh();                     // Refresh device list
dvpEnum(&count, devices);         // Enumerate devices
dvpOpenByName(name, &handle);     // Open specific device
```

## Qt 6.9 Compilation Requirements

### Required Includes for UI
```cpp
#include <QComboBox>
#include <QSpinBox>
#include <QTextEdit>
#include <QLabel>
#include <QPainterPath>
```

### Qt 6 API Changes
```cpp
// Use document() for QTextEdit operations
textEdit->document()->setMaximumBlockCount(maxLines);

// Qt 6 event signature
void enterEvent(QEnterEvent* event) override;
```

## Deployment Requirements

### Windows Platform Plugin (CRITICAL)
```batch
# App won't start without this structure:
Release/
├── Do3ThinkCameraViewer.exe
└── platforms/
    └── qwindows.dll  # MANDATORY
```

## Quick Debugging

| Problem | Solution |
|---------|----------|
| App won't start | Copy platforms/qwindows.dll |
| MOC errors | Add Q_OBJECT macro to QObject classes |
| Signals not working | moveToThread before connect |
| Camera not found | Call dvpInit() and dvpRefresh() |

## Key Files

### Core Components (Shared by both versions)
- **base_component.cpp**: Foundation for all components, state machine
- **camera_component.cpp**: Abstract camera interface
- **dothink_camera.cpp**: Do3Think SDK implementation

### QWidget Version
- **main_ui.cpp**: QWidget-based UI, manages control panels
- **machine.cpp**: Component lifecycle management

### QML Version
- **qml_bridge/camera_bridge.cpp**: QML-Component bridge layer
- **qml/main.qml**: QML-based main window
- **qml/components/CameraControlPanel.qml**: QML camera controls

## UI Versions

Two UI implementations available:
1. **QWidget** (Original): Traditional C++ widgets, direct Signal/Slot
2. **QML** (New): Modern declarative UI, uses bridge layer

See `QML_VS_QWIDGET_COMPARISON.md` for detailed comparison.