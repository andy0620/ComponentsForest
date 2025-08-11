# Ultra Patterns - ComponentsForest Core Wisdom

## The 7 Golden Rules

### 1. Architecture is LAW
```
BaseComponent → YourComponent → ALWAYS
Control Panel → Signal/Slot → Component → ONLY WAY
MainUI → Control Panel → Component → NEVER DIRECT
```

### 2. Platform Plugins = Life
```
No platforms/qwindows.dll = Dead App
Fix: mkdir platforms && copy Qt/plugins/platforms/qwindows.dll
```

### 3. Thread Before Connect
```cpp
component->moveToThread(thread);  // FIRST
connect(..., Qt::QueuedConnection);  // THEN
```

### 4. String Signals = Decoupling
```cpp
connect(SIGNAL(foo()), SLOT(bar()));  // YES
connect(&Obj::foo, &Obj::bar);  // NO - Coupling
```

### 5. Mutex In, Signal Out
```cpp
{
    QMutexLocker lock(&mutex);
    // Change state
}  // Unlock
emit stateChanged();  // Outside lock
```

### 6. Static > DLL on Windows
```cmake
add_library(MyLib STATIC ...)  // Avoid DLL hell
```

### 7. Include Everything Explicit
```cpp
#include <QComboBox>  // Don't assume
#include <QLabel>     // Include all
#include <QSpinBox>   // Be explicit
```

## The Pattern Matrix

| Problem | Pattern | Never Do |
|---------|---------|----------|
| New Component | Inherit BaseComponent | Direct QObject |
| UI Control | Signal through Panel | Direct component call |
| Thread Comm | QueuedConnection | Direct access |
| Build Windows | Static libs | DLL without wrapper |
| App Won't Start | Check platforms/ | AllocConsole() |
| State Change | Mutex then emit | Emit in lock |
| Camera Control | Panel → Component | MainUI → Component |

## The 3-Layer Truth

```
Layer 1: BaseComponent (Foundation)
    ↓ Must Inherit
Layer 2: CameraComponent (Abstraction)  
    ↓ 80% Logic Here
Layer 3: Do3ThinkCamera (Implementation)
    ↓ 20% Vendor Specific
    
Control: ControlPanel ←→ Component (Signals Only)
```

## Qt 6.9.1 Specifics

```cpp
// API Changes
textEdit->document()->setMaximumBlockCount(n);  // ✓
windowState() & Qt::WindowFullScreen;  // ✓
void enterEvent(QEnterEvent*);  // ✓

// Build Essentials
set(CMAKE_AUTOMOC ON)  // ✓
Q_OBJECT  // Always ✓
```

## Deployment DNA

```
Release/
├── app.exe
├── Qt6*.dll
└── platforms/
    └── qwindows.dll  ← YOUR APP DIES WITHOUT THIS
```

## Debug Flowchart

```
App Dead?
└─ platforms/qwindows.dll? → No → Copy it → Fixed

Build Fails?  
└─ Missing includes? → Yes → Add QComboBox etc → Fixed

Signals Dead?
└─ Component in thread? → No → moveToThread first → Fixed

UI Frozen?
└─ SDK call in UI thread? → Yes → Move to component → Fixed
```

## The Callback Pattern

```cpp
// Do3Think Specific but Universal Pattern
struct CallbackData {
    Component* component;
    QMutex* mutex;
    std::atomic<bool> active;
};

static void callback(void* userData) {
    auto* data = static_cast<CallbackData*>(userData);
    QMutexLocker lock(data->mutex);
    if (data->active) {
        // Process safely
    }
}
```

## Zero-Copy Wisdom

```cpp
// Pass pointers not copies
emit frameReady(QImage(ptr, w, h, QImage::Format_RGB888));  // ✓
// Not: emit frameReady(image.copy());  // ✗
```

## Industrial UI Rules

```cpp
// No animations
timer->stop();  // Industrial != Gaming

// Full opacity  
rgba(20,24,36,255)  // ✓ Readable
rgba(20,24,36,128)  // ✗ Transparent

// Control response
< 100ms  // Acceptable
> 100ms  // Feels broken
```

## CMAKE Survival Kit

```cmake
# Windows: Always static
add_library(ComponentsForestCore STATIC ...)
target_compile_definitions(... STATIC_DEFINE)

# Qt: Always MOC
set(CMAKE_AUTOMOC ON)

# Include: Always explicit
target_include_directories(... PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
```

## The Final Truth Table

| Always | Never |
|--------|-------|
| BaseComponent inheritance | Direct QObject for components |
| String-based signals | Compile-time signal syntax |
| Control Panel interface | MainUI direct control |
| moveToThread before connect | Connect before thread |
| Static libs on Windows | DLLs without wrappers |
| Include all headers | Assume includes |
| Mutex outside emit | Emit inside mutex |
| platforms/qwindows.dll | AllocConsole() in GUI |

## One-Line Fixes

```bash
# App won't start
mkdir platforms && copy C:\Qt\6.9.1\msvc2022_64\plugins\platforms\qwindows.dll platforms\

# Build fails  
cmake .. -DCMAKE_PREFIX_PATH=C:/Qt/6.9.1/msvc2022_64 -G "Visual Studio 17 2022"

# Missing MOC
set(CMAKE_AUTOMOC ON)

# DLL issues
add_library(MyLib STATIC ...)  # Change to static

# Signals broken
component->moveToThread(thread);  # Before connecting
```

## The Ultimate Checklist

- [ ] Inherits from BaseComponent?
- [ ] Has Q_OBJECT macro?
- [ ] Moved to thread before connect?
- [ ] Using string-based signals?
- [ ] platforms/qwindows.dll deployed?
- [ ] No AllocConsole() calls?
- [ ] Static libs on Windows?
- [ ] All headers included?
- [ ] Mutex before emit?
- [ ] Control Panel is sole interface?

**If all checked → It will work**

---

*Remember: Architecture is not a suggestion. It's the law.*