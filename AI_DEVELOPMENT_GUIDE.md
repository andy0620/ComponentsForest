# AI Agent Development Guide for ComponentsForest

## Quick Decision Tree for AI Agents

### When User Asks to Add Features
```
Is it a Component?
├─ YES → Must inherit from BaseComponent
│   ├─ Implement all lifecycle methods (onInitialize, onStart, etc.)
│   ├─ Use PIMPL pattern for thread safety
│   └─ Communicate via Signal/Slot ONLY
│
└─ NO → Is it UI related?
    ├─ YES → Is it a Control Panel?
    │   ├─ YES → Inherit from base ControlPanel class
    │   │   ├─ Use STRING-BASED signal connections
    │   │   └─ Never directly access components
    │   └─ NO → Add to existing panel/UI
    │
    └─ NO → Standard Qt/C++ patterns apply
```

### When User Reports "Not Working"
```
Program won't start?
├─ Check platforms/qwindows.dll exists
├─ Remove any AllocConsole() calls
└─ Verify Qt DLLs are present

UI not showing changes?
├─ Did you actually modify source files?
├─ Not just create new theme files
└─ Check if show(), raise(), activateWindow() called

Camera not detected?
├─ Check dvpInit() and dvpRefresh() called
├─ Remove any mock/fake camera data
└─ Verify USB3.0 connection

Signals not working?
├─ Component moved to thread BEFORE connecting?
├─ Using Qt::QueuedConnection for cross-thread?
└─ Q_OBJECT macro present in class?
```

## Critical Rules for Code Generation

### 1. ALWAYS Check Architecture Compliance
```cpp
// Before writing ANY component class:
// ✓ Must inherit from BaseComponent
// ✓ Must have Q_OBJECT macro
// ✓ Must implement all virtual lifecycle methods
// ✓ Must use Signal/Slot for ALL communication

class NewComponent : public BaseComponent {  // ALWAYS
    Q_OBJECT  // ALWAYS
    
    // Required lifecycle methods
    bool onInitialize() override;
    bool onStart() override;
    bool onStop() override;
    bool onReset() override;
    void onDestroy() override;
};
```

### 2. Control Panel Communication Pattern
```cpp
// NEVER do this:
void ControlPanel::someMethod() {
    m_camera->startCapture();  // WRONG - direct call
}

// ALWAYS do this:
void ControlPanel::someMethod() {
    emit requestStartCapture();  // Correct - signal emission
}

// Connection MUST be string-based:
connect(panel, SIGNAL(requestStartCapture()),
        camera, SLOT(startCapture()));
```

### 3. Thread Safety Pattern
```cpp
// For ANY shared data access:
void Component::updateData() {
    QMutexLocker locker(&m_mutex);  // RAII lock
    // Modify data
}  // Auto-unlock

// For state changes:
void Component::changeState(State newState) {
    State oldState;
    {
        QMutexLocker locker(&m_stateMutex);
        oldState = m_state;
        m_state = newState;
    }  // Unlock BEFORE emit
    emit stateChanged(oldState, newState);
}
```

## Common Task Templates

### Adding New Camera Type
```cpp
// 1. Create component inheriting from CameraComponent
class NewCameraComponent : public CameraComponent {
    Q_OBJECT
    // Implement pure virtual methods from CameraComponent
};

// 2. Create control panel inheriting from CameraControlPanel
class NewCameraControlPanel : public CameraControlPanel {
    Q_OBJECT
    // Add vendor-specific UI elements
};

// 3. Update Machine to support new type
void Machine::createCamera(const QString& type) {
    if (type == "NewCamera") {
        component = new NewCameraComponent();
        panel = new NewCameraControlPanel();
    }
}
```

### Adding Image Processing
```cpp
// MUST inherit from BaseComponent (not just QObject)
class ImageProcessor : public BaseComponent {
    Q_OBJECT
    
signals:
    void imageProcessed(const QImage& result);
    
public slots:
    void processImage(const QImage& input);
    
protected:
    bool onInitialize() override {
        // Setup processing pipeline
        return true;
    }
};
```

## Compilation Quick Fixes

### Missing Includes (Qt 6.9.1)
```cpp
// Common missing includes in control panels:
#include <QComboBox>
#include <QSpinBox>
#include <QTextEdit>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QSlider>
#include <QPainterPath>
```

### Qt API Changes
```cpp
// Qt 6.9+ changes:
textEdit->document()->setMaximumBlockCount(100);  // NOT textEdit->setMaximumBlockCount()
void enterEvent(QEnterEvent* event);  // NOT QEvent*
windowState() & Qt::WindowFullScreen;  // NOT windowState() == Qt::WindowFullScreen
```

### CMake Configuration
```cmake
# Essential for Qt projects:
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

# Avoid DLL issues on Windows:
add_library(MyLib STATIC ...)  # Use STATIC not SHARED
```

## Deployment Checklist

### Windows Deployment
```batch
# Required structure:
Release/
├── MyApp.exe
├── Qt6Core.dll
├── Qt6Gui.dll
├── Qt6Widgets.dll
├── platforms/
│   └── qwindows.dll  # CRITICAL
├── styles/
│   └── qwindowsvistastyle.dll
└── DVPCamera64.dll  # If using Do3Think
```

### Debug Steps When App Won't Start
1. Check `platforms/qwindows.dll` exists
2. Remove any `AllocConsole()` calls
3. Run with `--minimal` flag for testing
4. Check all Qt DLLs match Qt version
5. Verify no debug/release DLL mixing

## Performance Optimization Points

### Camera Acquisition
- Use callback mode for >30fps: `dvpRegisterStreamCallback()`
- Implement ring buffer to avoid allocations
- Process in separate thread from UI

### UI Updates
- Limit UI refresh to 30fps maximum
- Use timer-based updates, not per-frame
- Batch UI updates when possible

### Memory Management
- Use object pools for frequently allocated objects
- Implement zero-copy where possible
- Clear unused image buffers promptly

## Testing Patterns

### Component Testing
```cpp
// Test lifecycle transitions
component->initialize(config);
ASSERT(component->state() == ComponentState::Initialized);

component->start();
ASSERT(component->state() == ComponentState::Running);

// Test signal emissions
QSignalSpy spy(component, SIGNAL(frameReady(QImage)));
// Trigger frame
ASSERT(spy.count() == 1);
```

### Thread Safety Testing
```cpp
// Test concurrent access
std::vector<std::thread> threads;
for (int i = 0; i < 10; ++i) {
    threads.emplace_back([component]() {
        component->processData(data);
    });
}
// Should not crash or corrupt data
```

## Decision Matrix for Common Scenarios

| User Request | First Action | Key Considerations |
|-------------|--------------|-------------------|
| "Add new camera" | Check SDK documentation | Must inherit CameraComponent |
| "Make UI prettier" | Review current theme | Avoid animations in industrial UI |
| "Program crashes" | Check thread safety | Look for mutex issues |
| "Not compiling" | Check includes | Qt 6.9 API changes |
| "Won't start" | Check deployment | platforms/qwindows.dll |
| "Too slow" | Profile first | Check UI update frequency |

## Red Flags to Avoid

1. **Direct component access from UI** - Always use signals
2. **QObject without Q_OBJECT** - Causes mysterious failures  
3. **Forgetting moveToThread()** - Signals won't work
4. **Using compile-time signal syntax** - Breaks decoupling
5. **Heavy transparency in UI** - Reduces readability
6. **Continuous animations** - Distracting in industrial apps
7. **Missing platform plugins** - App won't start
8. **Mixing debug/release DLLs** - Runtime crashes

## Success Patterns

1. **Follow three-tier architecture** religiously
2. **Test on clean system** without dev tools
3. **Use string-based signals** for decoupling
4. **Protect all shared data** with mutexes
5. **Document API changes** for Qt versions
6. **Build static libraries** on Windows
7. **Include all Qt headers** explicitly
8. **Test each lifecycle transition**

---

## Quick Command Reference

```bash
# Build from scratch
rm -rf build && mkdir build && cd build
cmake .. -DCMAKE_PREFIX_PATH=C:/Qt/6.9.1/msvc2022_64
cmake --build . --config Release

# Deploy for Windows
mkdir Release\platforms
copy %QT_DIR%\plugins\platforms\qwindows.dll Release\platforms\

# Debug startup issues
Do3ThinkCameraViewer.exe --minimal

# Check dependencies
dumpbin /dependents Do3ThinkCameraViewer.exe
```

This guide helps AI agents make correct architectural decisions and avoid common pitfalls when working with the ComponentsForest codebase.