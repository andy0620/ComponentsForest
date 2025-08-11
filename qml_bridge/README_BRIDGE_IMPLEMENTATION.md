# QML Bridge Layer Implementation

## Overview
The QML bridge layer provides a clean interface between the C++ ComponentsForest architecture and QML UI, maintaining complete signal/slot decoupling as required by the project architecture.

## Implemented Classes

### 1. CameraBridge (`camera_bridge.h/cpp`)
Bridges camera components to QML with the following features:

#### QML Properties (Q_PROPERTY)
- `cameraId` - Unique identifier for the camera
- `deviceName` - Connected device name
- `isConnected` - Connection status
- `isAcquiring` - Acquisition status
- `currentFrame` - Latest captured frame
- `fps` - Current frame rate
- `frameCount` - Total frames captured
- `exposureTime` - Camera exposure time
- `gain` - Camera gain value
- `width/height` - Resolution
- `deviceInfo` - Device information map
- `statistics` - Performance statistics
- `errorMessage` - Last error message
- `debugMode` - Debug logging enabled

#### QML Methods (Q_INVOKABLE)
- `connectToDevice(deviceName)` - Connect to a camera device
- `disconnectFromDevice()` - Disconnect from device
- `startAcquisition()` - Start image acquisition
- `stopAcquisition()` - Stop image acquisition
- `triggerSoftware()` - Send software trigger
- `saveCurrentFrame(filePath)` - Save current frame to file
- `setROI(x, y, width, height)` - Set region of interest
- `clearROI()` - Clear region of interest
- `getAvailableDevices()` - Get list of available devices
- `refreshDeviceList()` - Refresh device list
- `applyConfiguration(config)` - Apply configuration map
- `getConfiguration()` - Get current configuration
- `resetStatistics()` - Reset performance statistics
- `getLogMessages()` - Get debug log messages
- `clearLog()` - Clear debug log

### 2. MachineBridge (`machine_bridge.h/cpp`)
Manages multiple camera components with:

#### QML Properties
- `isRunning` - Machine running status
- `cameraCount` - Number of cameras
- `cameras` - List of camera information
- `selectedCameraId` - Currently selected camera
- `machineStatus` - Overall machine status
- `autoDiscovery` - Auto-discovery enabled
- `discoveryInterval` - Discovery interval in seconds

#### QML Methods
- `startMachine()` - Start the machine
- `stopMachine()` - Stop the machine
- `addCamera(cameraId, config)` - Add a new camera
- `removeCamera(cameraId)` - Remove a camera
- `startCamera(cameraId)` - Start specific camera
- `stopCamera(cameraId)` - Stop specific camera
- `startAllCameras()` - Start all cameras
- `stopAllCameras()` - Stop all cameras
- `discoverDevices()` - Discover available devices
- `connectToDevice(cameraId, deviceName)` - Connect camera to device
- `disconnectDevice(cameraId)` - Disconnect camera from device
- `getCameraInfo(cameraId)` - Get camera information
- `setCameraConfig(cameraId, config)` - Set camera configuration
- `getCameraConfig(cameraId)` - Get camera configuration
- `saveConfiguration(filePath)` - Save configuration to file
- `loadConfiguration(filePath)` - Load configuration from file
- `resetMachine()` - Reset machine to initial state
- `getPerformanceMetrics()` - Get performance metrics

## Architecture Compliance

### Signal/Slot Decoupling ✓
- Uses **string-based connections** (SIGNAL/SLOT macros) instead of compile-time connections
- Maintains complete decoupling between bridge and component layers
- Example:
```cpp
connect(d->component, SIGNAL(frameReady(QImage,FrameMetadata)),
        this, SLOT(onComponentFrameReady(QImage,FrameMetadata)));
```

### Thread Safety ✓
- Mutexes protect shared data (frames, statistics)
- Cross-thread signal/slot communication
- Components run in separate threads, bridge handles synchronization

### Component Independence ✓
- Bridge connects to components through `QObject*` pointers
- No direct compile-time dependency on specific component implementations
- Works with any component that follows the ComponentsForest interface

## Usage Example

### QML Side
```qml
import ComponentsForestQML 1.0

ApplicationWindow {
    MachineBridge {
        id: machine
        autoDiscovery: true
        discoveryInterval: 5
        
        onMachineStarted: console.log("Machine started")
        onCameraAdded: console.log("Camera added:", cameraId)
    }
    
    CameraBridge {
        id: camera
        cameraId: "camera1"
        debugMode: true
        
        onFrameReady: imageView.source = currentFrame
        onFpsChanged: fpsLabel.text = fps.toFixed(1) + " FPS"
    }
    
    Button {
        text: "Connect"
        onClicked: camera.connectToDevice("Do3Think Camera 1")
    }
    
    Button {
        text: "Start"
        onClicked: camera.startAcquisition()
    }
}
```

### C++ Side (main.cpp)
```cpp
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include "qml_bridge/camera_bridge.h"
#include "qml_bridge/machine_bridge.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    
    // Register bridge types for QML
    qmlRegisterType<ComponentsForest::CameraBridge>(
        "ComponentsForestQML", 1, 0, "CameraBridge");
    qmlRegisterType<ComponentsForest::MachineBridge>(
        "ComponentsForestQML", 1, 0, "MachineBridge");
    
    QQmlApplicationEngine engine;
    engine.load(QUrl(QStringLiteral("qrc:/main.qml")));
    
    return app.exec();
}
```

## Key Features

### 1. Real-time Updates
- Statistics updated every 100ms
- Frame display with minimal latency
- Thread-safe frame buffer management

### 2. Configuration Management
- Save/load configurations to JSON files
- Runtime parameter adjustment
- Persistent settings support

### 3. Debug Support
- Debug mode with comprehensive logging
- Log message retrieval for QML display
- Performance metrics tracking

### 4. Device Management
- Auto-discovery with configurable interval
- Multiple camera support
- Hot-plug device detection

## Build Integration
The bridge is integrated into the QML viewer build:
- Located in `/qml_bridge/` directory
- Included in `viewers/qml_camera_viewer/CMakeLists.txt`
- Compiled together with component sources

## Testing
To test the bridge implementation:
1. Build the QML viewer application
2. Run with a QML file that uses the bridge classes
3. Verify signal/slot communication works
4. Check thread safety with multiple cameras
5. Test configuration save/load functionality

## Notes
- Bridge classes do NOT modify original components
- Maintains ComponentsForest three-tier architecture
- Compatible with all existing camera components
- Extensible for future component types