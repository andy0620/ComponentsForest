# Ubuntu Development Workflow Guide for ComponentsForest

## Table of Contents
1. [Development Environment Setup](#1-development-environment-setup)
2. [Build System Optimization](#2-build-system-optimization)
3. [Development Workflow](#3-development-workflow)
4. [Camera Development](#4-camera-development)
5. [Performance Optimization](#5-performance-optimization)
6. [Docker Development Environment](#6-docker-development-environment)
7. [CI/CD Pipeline](#7-cicd-pipeline)
8. [Shell Scripts and Automation](#8-shell-scripts-and-automation)

---

## 1. Development Environment Setup

### 1.1 IDE Configuration - Qt Creator

```bash
# Install Qt Creator
sudo apt update
sudo apt install qtcreator qtbase6-dev qt6-tools-dev

# Configure Qt Creator for ComponentsForest
mkdir -p ~/.config/QtProject/qtcreator/templates/wizards/componentsforest
```

Create custom project template: `~/.config/QtProject/qtcreator/templates/wizards/componentsforest/component_template.xml`:

```xml
<?xml version="1.0" encoding="UTF-8"?>
<wizard version="1" kind="project" category="ComponentsForest">
    <description>Create a new ComponentsForest component</description>
    <displayname>ComponentsForest Component</displayname>
    <files>
        <file source="component.h" target="%{ComponentName}.h"/>
        <file source="component.cpp" target="%{ComponentName}.cpp"/>
        <file source="control_panel.h" target="%{ComponentName}ControlPanel.h"/>
        <file source="control_panel.cpp" target="%{ComponentName}ControlPanel.cpp"/>
    </files>
</wizard>
```

### 1.2 VS Code Configuration

Create `.vscode/settings.json`:

```json
{
    "cmake.configureSettings": {
        "CMAKE_PREFIX_PATH": "/opt/Qt/6.9.1/gcc_64",
        "CMAKE_BUILD_TYPE": "Debug",
        "CMAKE_EXPORT_COMPILE_COMMANDS": true
    },
    "C_Cpp.default.configurationProvider": "ms-vscode.cmake-tools",
    "C_Cpp.clang_format_style": "file",
    "files.associations": {
        "*.qml": "qml",
        "*.qss": "css",
        "*.ui": "xml"
    },
    "cmake.buildDirectory": "${workspaceFolder}/build_vscode",
    "cmake.generator": "Ninja",
    "cmake.parallelJobs": 8,
    "qt.qtdir": "/opt/Qt/6.9.1/gcc_64",
    "qt.qmlFormatCommand": "/opt/Qt/6.9.1/gcc_64/bin/qmlformat",
    "editor.formatOnSave": true,
    "editor.rulers": [100],
    "files.trimTrailingWhitespace": true
}
```

Create `.vscode/launch.json` for debugging:

```json
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "Debug Camera Viewer",
            "type": "cppdbg",
            "request": "launch",
            "program": "${workspaceFolder}/build_vscode/viewers/do3think_camera_viewer/Do3ThinkCameraViewer",
            "args": [],
            "stopAtEntry": false,
            "cwd": "${workspaceFolder}/build_vscode/viewers/do3think_camera_viewer",
            "environment": [
                {
                    "name": "LD_LIBRARY_PATH",
                    "value": "${workspaceFolder}/build_vscode:${env:LD_LIBRARY_PATH}"
                },
                {
                    "name": "QT_LOGGING_RULES",
                    "value": "ComponentsForest.*=true;qt.qml.debug=true"
                }
            ],
            "externalConsole": false,
            "MIMode": "gdb",
            "setupCommands": [
                {
                    "description": "Enable pretty-printing for Qt",
                    "text": "python exec(open('/opt/Qt/6.9.1/gcc_64/bin/gdbbridge.py').read())",
                    "ignoreFailures": true
                }
            ],
            "preLaunchTask": "cmake build"
        },
        {
            "name": "Debug QML Viewer",
            "type": "cppdbg",
            "request": "launch",
            "program": "${workspaceFolder}/build_qml_debug/QMLCameraViewer",
            "args": ["--qmljsdebugger=port:3768,block"],
            "environment": [
                {
                    "name": "QML_DEBUG_ARGS",
                    "value": "port:3768,block"
                }
            ]
        }
    ]
}
```

### 1.3 CLion Configuration

Create `.idea/cmake.xml`:

```xml
<?xml version="1.0" encoding="UTF-8"?>
<project version="4">
  <component name="CMakeSharedSettings">
    <configurations>
      <configuration PROFILE_NAME="Debug" ENABLED="true" 
                     CONFIG_NAME="Debug" 
                     GENERATION_OPTIONS="-DCMAKE_PREFIX_PATH=/opt/Qt/6.9.1/gcc_64">
        <envs>
          <env name="QT_LOGGING_RULES" value="ComponentsForest.*=true"/>
        </envs>
      </configuration>
      <configuration PROFILE_NAME="Release" ENABLED="true" 
                     CONFIG_NAME="Release" 
                     GENERATION_OPTIONS="-DCMAKE_PREFIX_PATH=/opt/Qt/6.9.1/gcc_64"/>
    </configurations>
  </component>
</project>
```

### 1.4 Debugger Setup for Qt Applications

Install Qt debugging helpers:

```bash
# Install debugging tools
sudo apt install gdb valgrind qtbase6-dev-tools

# Setup Qt pretty printers for GDB
cat >> ~/.gdbinit << 'EOF'
python
import sys
sys.path.insert(0, '/opt/Qt/6.9.1/gcc_64/lib')
from qt6printers import register_qt6_printers
register_qt6_printers(None)
end
EOF
```

---

## 2. Build System Optimization

### 2.1 ccache Setup for Faster Rebuilds

```bash
# Install ccache
sudo apt install ccache

# Configure ccache
ccache --max-size=10G
ccache --set-config=compression=true
ccache --set-config=compression_level=6

# Add to ~/.bashrc
export PATH="/usr/lib/ccache:$PATH"
export CCACHE_DIR="$HOME/.ccache"
export CCACHE_COMPRESS=1
export CCACHE_COMPRESSLEVEL=6
export CCACHE_MAXSIZE=10G

# Use with CMake
cmake -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
      -DCMAKE_C_COMPILER_LAUNCHER=ccache \
      ..
```

### 2.2 Distributed Compilation with distcc

```bash
# Install distcc on all machines
sudo apt install distcc distcc-pump

# Configure distcc hosts (edit /etc/distcc/hosts)
echo "192.168.1.100/4 192.168.1.101/4 localhost/2" > ~/.distcc/hosts

# Setup distcc daemon on build servers
sudo systemctl enable distcc
sudo systemctl start distcc

# Use with CMake
export DISTCC_HOSTS="192.168.1.100/4 192.168.1.101/4 localhost/2"
cmake -DCMAKE_CXX_COMPILER_LAUNCHER="ccache distcc" \
      -DCMAKE_C_COMPILER_LAUNCHER="ccache distcc" \
      ..
```

### 2.3 Build Directory Management

Create `build_manager.sh`:

```bash
#!/bin/bash

# ComponentsForest Build Manager
# Manages multiple build configurations efficiently

BUILD_ROOT="${HOME}/builds/ComponentsForest"
SOURCE_DIR="$(pwd)"

function create_build() {
    local config=$1
    local qt_path=$2
    local build_dir="${BUILD_ROOT}/${config}"
    
    mkdir -p "$build_dir"
    cd "$build_dir"
    
    cmake "$SOURCE_DIR" \
        -GNinja \
        -DCMAKE_BUILD_TYPE="$config" \
        -DCMAKE_PREFIX_PATH="$qt_path" \
        -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
        -DBUILD_TESTING=ON
    
    # Link compile_commands.json to source dir for IDE support
    ln -sf "$build_dir/compile_commands.json" "$SOURCE_DIR/"
}

function incremental_build() {
    local build_dir="$1"
    
    if [ ! -d "$build_dir" ]; then
        echo "Build directory not found: $build_dir"
        return 1
    fi
    
    # Only rebuild changed targets
    ninja -C "$build_dir" -t targets | grep -E "^[^:]+:" | cut -d: -f1 | \
    while read target; do
        if ninja -C "$build_dir" -n "$target" 2>&1 | grep -q "no work"; then
            continue
        fi
        echo "Building $target..."
        ninja -C "$build_dir" "$target"
    done
}

# Usage examples
create_build Debug /opt/Qt/6.9.1/gcc_64
create_build Release /opt/Qt/6.9.1/gcc_64
create_build RelWithDebInfo /opt/Qt/6.9.1/gcc_64
```

### 2.4 Incremental Build Strategies

Create `smart_build.sh`:

```bash
#!/bin/bash

# Smart incremental build system
# Only rebuilds what's necessary based on git changes

LAST_BUILD_COMMIT_FILE=".last_build_commit"
BUILD_DIR="build"

# Get the last successful build commit
if [ -f "$LAST_BUILD_COMMIT_FILE" ]; then
    LAST_COMMIT=$(cat "$LAST_BUILD_COMMIT_FILE")
else
    LAST_COMMIT=""
fi

# Get changed files since last build
if [ -n "$LAST_COMMIT" ]; then
    CHANGED_FILES=$(git diff --name-only "$LAST_COMMIT" HEAD)
else
    CHANGED_FILES=$(git ls-files)
fi

# Determine what needs rebuilding
REBUILD_CORE=false
REBUILD_VIEWERS=false
REBUILD_TESTS=false

for file in $CHANGED_FILES; do
    case "$file" in
        components/*)
            REBUILD_CORE=true
            ;;
        viewers/*)
            REBUILD_VIEWERS=true
            ;;
        tests/*)
            REBUILD_TESTS=true
            ;;
        CMakeLists.txt|*.cmake)
            # Full rebuild needed
            REBUILD_CORE=true
            REBUILD_VIEWERS=true
            REBUILD_TESTS=true
            ;;
    esac
done

# Perform targeted builds
cd "$BUILD_DIR" || exit 1

if [ "$REBUILD_CORE" = true ]; then
    echo "Rebuilding core components..."
    ninja ComponentsForestCore
fi

if [ "$REBUILD_VIEWERS" = true ]; then
    echo "Rebuilding viewers..."
    ninja Do3ThinkCameraViewer QMLCameraViewer
fi

if [ "$REBUILD_TESTS" = true ]; then
    echo "Rebuilding tests..."
    ninja all_tests
fi

# Update last build commit
git rev-parse HEAD > "../$LAST_BUILD_COMMIT_FILE"
```

---

## 3. Development Workflow

### 3.1 Component Development Cycle

Create `dev_cycle.sh`:

```bash
#!/bin/bash

# Component Development Cycle Helper
# Automates the typical development workflow

COMPONENT_NAME=$1
BUILD_DIR="build_dev"

function create_component() {
    local name=$1
    
    # Generate component files from template
    cat > "components/${name}_component.h" << EOF
#pragma once
#include "base_component.h"

class ${name}Component : public BaseComponent {
    Q_OBJECT
public:
    explicit ${name}Component(QObject* parent = nullptr);
    
protected:
    void doStart() override;
    void doStop() override;
    
signals:
    void dataReady(const QVariant& data);
    
private slots:
    void processData();
};
EOF
    
    # Generate implementation
    cat > "components/${name}_component.cpp" << EOF
#include "${name}_component.h"

${name}Component::${name}Component(QObject* parent)
    : BaseComponent("${name}", parent) {
}

void ${name}Component::doStart() {
    // Implementation
}

void ${name}Component::doStop() {
    // Implementation
}

void ${name}Component::processData() {
    // Process and emit data
    emit dataReady(QVariant());
}
EOF
    
    echo "Created component: ${name}"
}

function build_component() {
    ninja -C "$BUILD_DIR" "${1}Component"
}

function test_component() {
    "$BUILD_DIR/tests/test_${1}_component"
}

function watch_component() {
    # Watch for changes and auto-rebuild
    while inotifywait -e modify "components/${1}_component.cpp" "components/${1}_component.h"; do
        clear
        build_component "$1"
        test_component "$1"
    done
}

# Main workflow
create_component "$COMPONENT_NAME"
build_component "$COMPONENT_NAME"
test_component "$COMPONENT_NAME"
watch_component "$COMPONENT_NAME"
```

### 3.2 Testing Procedures

Create `test_runner.sh`:

```bash
#!/bin/bash

# Comprehensive test runner for ComponentsForest

BUILD_DIR="build"
RESULTS_DIR="test_results"
mkdir -p "$RESULTS_DIR"

# Unit tests
function run_unit_tests() {
    echo "Running unit tests..."
    ctest --test-dir "$BUILD_DIR" \
          --output-on-failure \
          --parallel $(nproc) \
          --output-junit "$RESULTS_DIR/unit_tests.xml"
}

# Integration tests
function run_integration_tests() {
    echo "Running integration tests..."
    
    # Start mock camera server
    python3 tests/mock_camera_server.py &
    MOCK_PID=$!
    
    # Run integration tests
    "$BUILD_DIR/tests/integration_tests" \
        --gtest_output="xml:$RESULTS_DIR/integration_tests.xml"
    
    # Cleanup
    kill $MOCK_PID
}

# Performance tests
function run_performance_tests() {
    echo "Running performance benchmarks..."
    
    # Run with perf
    perf record -g "$BUILD_DIR/tests/performance_tests"
    perf report --stdio > "$RESULTS_DIR/perf_report.txt"
}

# Memory tests
function run_memory_tests() {
    echo "Running memory leak detection..."
    
    valgrind --leak-check=full \
             --show-leak-kinds=all \
             --track-origins=yes \
             --xml=yes \
             --xml-file="$RESULTS_DIR/valgrind.xml" \
             "$BUILD_DIR/viewers/do3think_camera_viewer/Do3ThinkCameraViewer"
}

# Coverage report
function generate_coverage() {
    echo "Generating coverage report..."
    
    # Build with coverage flags
    cmake -B build_coverage \
          -DCMAKE_BUILD_TYPE=Debug \
          -DCMAKE_CXX_FLAGS="--coverage" \
          -DCMAKE_EXE_LINKER_FLAGS="--coverage"
    
    cmake --build build_coverage
    
    # Run tests
    ctest --test-dir build_coverage
    
    # Generate report
    lcov --capture --directory build_coverage --output-file coverage.info
    lcov --remove coverage.info '/usr/*' --output-file coverage.info
    genhtml coverage.info --output-directory "$RESULTS_DIR/coverage"
}

# Run all tests
run_unit_tests
run_integration_tests
run_performance_tests
run_memory_tests
generate_coverage

echo "Test results available in $RESULTS_DIR"
```

### 3.3 Hot Reload for QML Development

Create `qml_hot_reload.sh`:

```bash
#!/bin/bash

# QML Hot Reload Development Server
# Watches QML files and auto-reloads the application

QML_DIR="viewers/qml_camera_viewer/qml"
BUILD_DIR="build_qml_debug"
APP="$BUILD_DIR/QMLCameraViewer"

# Build in debug mode with QML debugging
cmake -B "$BUILD_DIR" \
      -DCMAKE_BUILD_TYPE=Debug \
      -DQML_DEBUG=ON \
      viewers/qml_camera_viewer

cmake --build "$BUILD_DIR"

# Start the application with QML debugging
QML_DISABLE_DISK_CACHE=1 \
QML_XHR_DUMP=1 \
QT_LOGGING_RULES="qt.qml.debug=true" \
"$APP" &

APP_PID=$!

# Watch for QML changes
inotifywait -m -r -e modify "$QML_DIR" --format '%w%f' | while read file; do
    if [[ "$file" == *.qml ]]; then
        echo "QML file changed: $file"
        
        # Send reload signal to application
        kill -USR1 $APP_PID 2>/dev/null || {
            echo "Restarting application..."
            QML_DISABLE_DISK_CACHE=1 "$APP" &
            APP_PID=$!
        }
    fi
done

# Cleanup on exit
trap "kill $APP_PID 2>/dev/null" EXIT
```

### 3.4 Signal/Slot Debugging Techniques

Create `signal_debugger.cpp`:

```cpp
// Signal/Slot debugging helper for ComponentsForest

#include <QObject>
#include <QMetaMethod>
#include <QDebug>

class SignalSpy : public QObject {
public:
    static void installOn(QObject* target) {
        auto spy = new SignalSpy(target);
        
        const QMetaObject* meta = target->metaObject();
        for (int i = 0; i < meta->methodCount(); ++i) {
            QMetaMethod method = meta->method(i);
            
            if (method.methodType() == QMetaMethod::Signal) {
                QObject::connect(target, method, spy, 
                    [=]() {
                        qDebug() << "Signal emitted:" 
                                 << target->objectName() 
                                 << method.name();
                    });
            }
        }
    }
    
private:
    SignalSpy(QObject* parent) : QObject(parent) {}
};

// Usage in debugging:
// SignalSpy::installOn(cameraComponent);
```

---

## 4. Camera Development

### 4.1 Do3Think SDK Debugging on Linux

Create `camera_debug.sh`:

```bash
#!/bin/bash

# Do3Think Camera Debugging Helper

# Check USB devices
function list_cameras() {
    echo "=== USB Camera Devices ==="
    lsusb | grep -i "camera\|imaging\|do3think"
    
    echo -e "\n=== V4L2 Devices ==="
    v4l2-ctl --list-devices
    
    echo -e "\n=== Device Permissions ==="
    ls -la /dev/video* 2>/dev/null
}

# Monitor USB events
function monitor_usb() {
    echo "Monitoring USB events (press Ctrl+C to stop)..."
    udevadm monitor --udev --subsystem-match=usb
}

# Test camera access
function test_camera() {
    local device=${1:-/dev/video0}
    
    echo "Testing camera: $device"
    v4l2-ctl --device="$device" --all
    
    # Try to capture a frame
    v4l2-ctl --device="$device" \
             --set-fmt-video=width=640,height=480,pixelformat=MJPG \
             --stream-mmap --stream-count=1 --stream-to=test.jpg
    
    if [ -f test.jpg ]; then
        echo "Successfully captured frame to test.jpg"
    else
        echo "Failed to capture frame"
    fi
}

# Debug Do3Think SDK
function debug_sdk() {
    # Set SDK debug environment
    export DVP_DEBUG=1
    export DVP_LOG_LEVEL=TRACE
    
    # Run with strace to see system calls
    strace -e trace=open,ioctl,read,write -o sdk_trace.log \
           build/tests/test_do3think_camera
    
    echo "SDK trace saved to sdk_trace.log"
}

# Main menu
case "$1" in
    list)
        list_cameras
        ;;
    monitor)
        monitor_usb
        ;;
    test)
        test_camera "$2"
        ;;
    debug)
        debug_sdk
        ;;
    *)
        echo "Usage: $0 {list|monitor|test|debug}"
        ;;
esac
```

### 4.2 USB Device Permissions Management

Create `/etc/udev/rules.d/99-do3think-cameras.rules`:

```bash
# Do3Think Camera USB Rules
# Provides access to Do3Think cameras for development

# Do3Think cameras (adjust VID/PID as needed)
SUBSYSTEM=="usb", ATTRS{idVendor}=="1234", ATTRS{idProduct}=="5678", MODE="0666", GROUP="plugdev"

# Generic USB cameras for testing
SUBSYSTEM=="video4linux", MODE="0666", GROUP="plugdev"

# Reload rules: sudo udevadm control --reload-rules && sudo udevadm trigger
```

### 4.3 Virtual Camera Setup for Testing

Create `setup_virtual_camera.sh`:

```bash
#!/bin/bash

# Setup virtual camera for testing without hardware

# Install v4l2loopback
sudo apt install v4l2loopback-dkms v4l2loopback-utils

# Load the module
sudo modprobe v4l2loopback devices=2 \
    video_nr=10,11 \
    card_label="Virtual Camera 1","Virtual Camera 2" \
    exclusive_caps=1

# Create test pattern generator
function generate_test_pattern() {
    ffmpeg -f lavfi -i testsrc2=size=1920x1080:rate=30 \
           -f v4l2 -pix_fmt yuv420p /dev/video10 &
    
    ffmpeg -f lavfi -i mandelbrot=size=1280x720:rate=25 \
           -f v4l2 -pix_fmt yuv420p /dev/video11 &
}

# Simulate camera with video file
function simulate_from_file() {
    local video_file=$1
    local device=${2:-/dev/video10}
    
    ffmpeg -re -i "$video_file" -f v4l2 -pix_fmt yuv420p "$device"
}

# Create mock Do3Think camera wrapper
cat > mock_do3think_camera.cpp << 'EOF'
#include "DVPCamera.h"
#include <cstring>
#include <vector>

// Mock implementation that uses V4L2 devices
class MockDo3ThinkCamera {
    static std::vector<std::string> virtualDevices;
    
public:
    static dvpStatus dvpInit() {
        virtualDevices = {"/dev/video10", "/dev/video11"};
        return DVP_STATUS_OK;
    }
    
    static dvpStatus dvpEnum(dvpUint32* pCount, dvpCameraInfo* pInfo) {
        *pCount = virtualDevices.size();
        if (pInfo) {
            for (size_t i = 0; i < virtualDevices.size(); ++i) {
                sprintf(pInfo[i].FriendlyName, "Virtual Camera %zu", i);
                strcpy(pInfo[i].PortType, "USB");
            }
        }
        return DVP_STATUS_OK;
    }
};
EOF

echo "Virtual cameras created at /dev/video10 and /dev/video11"
generate_test_pattern
```

### 4.4 Camera Simulation for CI/CD

Create `ci_camera_mock.py`:

```python
#!/usr/bin/env python3

"""
Mock camera server for CI/CD testing
Simulates Do3Think camera behavior over network
"""

import socket
import json
import threading
import numpy as np
from PIL import Image
import io

class MockCameraServer:
    def __init__(self, port=9999):
        self.port = port
        self.cameras = {
            "CAM001": {"width": 1920, "height": 1080, "fps": 30},
            "CAM002": {"width": 1280, "height": 720, "fps": 60}
        }
        self.running = False
        
    def generate_frame(self, camera_id):
        """Generate synthetic frame data"""
        cam = self.cameras[camera_id]
        # Create test pattern
        frame = np.random.randint(0, 255, 
            (cam["height"], cam["width"], 3), dtype=np.uint8)
        
        # Add timestamp overlay
        img = Image.fromarray(frame)
        return img.tobytes()
    
    def handle_client(self, client_socket):
        """Handle client requests"""
        while self.running:
            try:
                data = client_socket.recv(1024).decode()
                if not data:
                    break
                    
                request = json.loads(data)
                
                if request["cmd"] == "list_cameras":
                    response = {
                        "cameras": list(self.cameras.keys())
                    }
                elif request["cmd"] == "get_frame":
                    frame_data = self.generate_frame(request["camera_id"])
                    response = {
                        "frame_size": len(frame_data)
                    }
                    client_socket.send(json.dumps(response).encode())
                    client_socket.send(frame_data)
                    continue
                    
                client_socket.send(json.dumps(response).encode())
                
            except Exception as e:
                print(f"Error: {e}")
                break
                
        client_socket.close()
    
    def start(self):
        """Start mock server"""
        self.running = True
        server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        server_socket.bind(('localhost', self.port))
        server_socket.listen(5)
        
        print(f"Mock camera server listening on port {self.port}")
        
        while self.running:
            client, addr = server_socket.accept()
            thread = threading.Thread(target=self.handle_client, args=(client,))
            thread.start()

if __name__ == "__main__":
    server = MockCameraServer()
    server.start()
```

---

## 5. Performance Optimization

### 5.1 Profiling Qt Applications with perf

Create `profile_qt.sh`:

```bash
#!/bin/bash

# Qt Application Performance Profiling

APP="build/viewers/do3think_camera_viewer/Do3ThinkCameraViewer"
OUTPUT_DIR="profiling_results"
mkdir -p "$OUTPUT_DIR"

# CPU profiling
function profile_cpu() {
    echo "Starting CPU profiling..."
    
    # Record performance data
    perf record -g -F 99 --call-graph dwarf -o "$OUTPUT_DIR/perf.data" "$APP" &
    APP_PID=$!
    
    # Let it run for 30 seconds
    sleep 30
    kill $APP_PID
    
    # Generate reports
    perf report --stdio > "$OUTPUT_DIR/cpu_report.txt"
    perf script | c++filt > "$OUTPUT_DIR/cpu_trace.txt"
    
    # Generate flame graph
    perf script | stackcollapse-perf.pl | flamegraph.pl > "$OUTPUT_DIR/flamegraph.svg"
}

# Qt specific profiling
function profile_qt_events() {
    # Enable Qt performance logging
    QT_LOGGING_RULES="qt.qpa.events=true;qt.widgets.performance=true" \
    QML_PROFILE=1 \
    "$APP" 2>&1 | tee "$OUTPUT_DIR/qt_events.log"
}

# GPU profiling for QML
function profile_gpu() {
    # Intel GPU
    if [ -d /sys/class/drm/card0 ]; then
        intel_gpu_top -o "$OUTPUT_DIR/gpu_usage.txt" &
        GPU_PID=$!
    fi
    
    # Run with scene graph debugging
    QSG_VISUALIZE=overdraw \
    QSG_RENDER_TIMING=1 \
    "$APP"
    
    [ -n "$GPU_PID" ] && kill $GPU_PID
}

profile_cpu
profile_qt_events
profile_gpu

echo "Profiling results saved to $OUTPUT_DIR"
```

### 5.2 Memory Leak Detection with Valgrind

Create `memory_check.sh`:

```bash
#!/bin/bash

# Comprehensive memory checking for ComponentsForest

function check_memory_leaks() {
    valgrind \
        --leak-check=full \
        --show-leak-kinds=all \
        --track-origins=yes \
        --verbose \
        --log-file=valgrind_report.txt \
        --gen-suppressions=all \
        --suppressions=qt6.supp \
        build/viewers/do3think_camera_viewer/Do3ThinkCameraViewer
}

# Create Qt6 suppressions file
cat > qt6.supp << 'EOF'
{
   Qt6_Initialization
   Memcheck:Leak
   ...
   fun:*Qt*
}

{
   Qt6_MetaObject
   Memcheck:Leak
   ...
   fun:*QMetaObject*
}

{
   X11_Display
   Memcheck:Leak
   ...
   obj:*/libX11.so*
}
EOF

# AddressSanitizer (faster than valgrind)
function check_with_asan() {
    # Rebuild with AddressSanitizer
    cmake -B build_asan \
          -DCMAKE_BUILD_TYPE=Debug \
          -DCMAKE_CXX_FLAGS="-fsanitize=address -fno-omit-frame-pointer" \
          -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address"
    
    cmake --build build_asan
    
    # Run with ASAN
    ASAN_OPTIONS=verbosity=1:halt_on_error=0:print_stats=1 \
    build_asan/viewers/do3think_camera_viewer/Do3ThinkCameraViewer
}

# ThreadSanitizer for race conditions
function check_thread_safety() {
    cmake -B build_tsan \
          -DCMAKE_BUILD_TYPE=Debug \
          -DCMAKE_CXX_FLAGS="-fsanitize=thread -fno-omit-frame-pointer" \
          -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=thread"
    
    cmake --build build_tsan
    
    TSAN_OPTIONS=halt_on_error=0:history_size=7 \
    build_tsan/viewers/do3think_camera_viewer/Do3ThinkCameraViewer
}

check_memory_leaks
check_with_asan
check_thread_safety
```

### 5.3 Qt Performance Profiler Setup

Create `qt_profiler.cpp`:

```cpp
// Qt Performance Profiler Integration

#include <QElapsedTimer>
#include <QDebug>
#include <chrono>

class PerformanceProfiler {
public:
    class ScopedTimer {
        QString m_name;
        QElapsedTimer m_timer;
        
    public:
        ScopedTimer(const QString& name) : m_name(name) {
            m_timer.start();
        }
        
        ~ScopedTimer() {
            qDebug() << m_name << "took" << m_timer.elapsed() << "ms";
        }
    };
    
    static void profileFunction(const QString& name, std::function<void()> func) {
        ScopedTimer timer(name);
        func();
    }
    
    static void enableQmlProfiling() {
        qputenv("QML_PROFILE", "1");
        qputenv("QML_PROFILE_VERBOSE", "1");
    }
};

// Usage:
// PerformanceProfiler::ScopedTimer timer("Camera initialization");
// or
// PerformanceProfiler::profileFunction("Process frame", [&]() {
//     processFrame();
// });
```

### 5.4 Build Optimization Flags

Create `optimized_build.cmake`:

```cmake
# Optimization flags for ComponentsForest

# CPU-specific optimizations
include(CheckCXXCompilerFlag)

function(add_optimization_flags target)
    # Base optimizations
    target_compile_options(${target} PRIVATE
        $<$<CONFIG:Release>:-O3>
        $<$<CONFIG:Release>:-march=native>
        $<$<CONFIG:Release>:-mtune=native>
        $<$<CONFIG:Release>:-flto>
        $<$<CONFIG:Release>:-fomit-frame-pointer>
    )
    
    # Link-time optimization
    target_link_options(${target} PRIVATE
        $<$<CONFIG:Release>:-flto>
        $<$<CONFIG:Release>:-s>
    )
    
    # Profile-guided optimization
    if(ENABLE_PGO)
        # First pass: generate profile
        target_compile_options(${target} PRIVATE -fprofile-generate)
        target_link_options(${target} PRIVATE -fprofile-generate)
        
        # After running: cmake -DUSE_PGO_DATA=ON
        if(USE_PGO_DATA)
            target_compile_options(${target} PRIVATE -fprofile-use)
            target_link_options(${target} PRIVATE -fprofile-use)
        endif()
    endif()
    
    # Qt-specific optimizations
    target_compile_definitions(${target} PRIVATE
        $<$<CONFIG:Release>:QT_NO_DEBUG>
        $<$<CONFIG:Release>:QT_NO_DEBUG_OUTPUT>
    )
endfunction()

# Use in CMakeLists.txt:
# add_optimization_flags(Do3ThinkCameraViewer)
```

---

## 6. Docker Development Environment

### 6.1 Dockerfile for Consistent Environment

Create `Dockerfile.dev`:

```dockerfile
# ComponentsForest Development Environment
FROM ubuntu:22.04

# Prevent interactive prompts
ENV DEBIAN_FRONTEND=noninteractive
ENV QT_VERSION=6.9.1

# Install base dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    ninja-build \
    git \
    wget \
    curl \
    python3-pip \
    # Qt dependencies
    libgl1-mesa-dev \
    libglu1-mesa-dev \
    libxcb-xinerama0 \
    libxcb-cursor0 \
    libxkbcommon-dev \
    libxkbcommon-x11-dev \
    # Development tools
    gdb \
    valgrind \
    ccache \
    clang-format \
    clang-tidy \
    # USB/Camera support
    libusb-1.0-0-dev \
    v4l-utils \
    && rm -rf /var/lib/apt/lists/*

# Install Qt (using aqtinstall for automated installation)
RUN pip3 install aqtinstall && \
    aqt install-qt linux desktop ${QT_VERSION} gcc_64 \
        -O /opt/Qt \
        --modules qtcharts qtquick3d

# Set Qt environment
ENV PATH=/opt/Qt/${QT_VERSION}/gcc_64/bin:$PATH
ENV LD_LIBRARY_PATH=/opt/Qt/${QT_VERSION}/gcc_64/lib:$LD_LIBRARY_PATH
ENV QT_DIR=/opt/Qt/${QT_VERSION}/gcc_64

# Install Do3Think SDK (if available)
COPY Do3ThinkCamera/SDK /opt/Do3Think/SDK
ENV LD_LIBRARY_PATH=/opt/Do3Think/SDK:$LD_LIBRARY_PATH

# Setup ccache
RUN ccache --max-size=5G && \
    ln -s /usr/bin/ccache /usr/local/bin/gcc && \
    ln -s /usr/bin/ccache /usr/local/bin/g++ && \
    ln -s /usr/bin/ccache /usr/local/bin/cc && \
    ln -s /usr/bin/ccache /usr/local/bin/c++

# Create development user
RUN useradd -m -s /bin/bash developer && \
    usermod -aG video,plugdev developer

USER developer
WORKDIR /home/developer/ComponentsForest

# Set build alias
RUN echo 'alias build="cmake --build build --parallel"' >> ~/.bashrc && \
    echo 'alias test="ctest --test-dir build --output-on-failure"' >> ~/.bashrc

CMD ["/bin/bash"]
```

### 6.2 Docker Compose for Multi-Component Testing

Create `docker-compose.yml`:

```yaml
version: '3.8'

services:
  # Main development container
  dev:
    build:
      context: .
      dockerfile: Dockerfile.dev
    volumes:
      - .:/home/developer/ComponentsForest
      - build-cache:/home/developer/ComponentsForest/build
      - ccache-data:/home/developer/.ccache
      # X11 socket for GUI
      - /tmp/.X11-unix:/tmp/.X11-unix:rw
    environment:
      - DISPLAY=${DISPLAY}
      - QT_X11_NO_MITSHM=1
      - CCACHE_DIR=/home/developer/.ccache
    devices:
      # USB devices for cameras
      - /dev/bus/usb:/dev/bus/usb
      - /dev/video0:/dev/video0
    network_mode: host
    stdin_open: true
    tty: true
    
  # Mock camera service for testing
  mock-camera:
    image: python:3.10-slim
    volumes:
      - ./tests:/tests
    command: python /tests/mock_camera_server.py
    ports:
      - "9999:9999"
    
  # Build service for CI
  builder:
    build:
      context: .
      dockerfile: Dockerfile.dev
    volumes:
      - .:/home/developer/ComponentsForest
      - build-output:/home/developer/ComponentsForest/build
    command: |
      bash -c "
        cmake -B build -S . \
          -DCMAKE_BUILD_TYPE=Release \
          -GNinja && \
        cmake --build build --parallel
      "
    
  # Test runner service
  tester:
    build:
      context: .
      dockerfile: Dockerfile.dev
    volumes:
      - .:/home/developer/ComponentsForest
      - build-output:/home/developer/ComponentsForest/build
    depends_on:
      - builder
      - mock-camera
    command: |
      bash -c "
        cd build && \
        ctest --output-on-failure --parallel
      "

volumes:
  build-cache:
  build-output:
  ccache-data:
```

### 6.3 X11 Forwarding for GUI Applications

Create `docker_gui.sh`:

```bash
#!/bin/bash

# Run ComponentsForest GUI in Docker with X11 forwarding

# Allow X11 connections from Docker
xhost +local:docker

# Run container with GUI support
docker run -it --rm \
    --name componentsforest-dev \
    -e DISPLAY=$DISPLAY \
    -e QT_X11_NO_MITSHM=1 \
    -v /tmp/.X11-unix:/tmp/.X11-unix:rw \
    -v $(pwd):/home/developer/ComponentsForest \
    --device /dev/dri \
    --device /dev/bus/usb \
    --network host \
    componentsforest:dev \
    bash -c "
        cd /home/developer/ComponentsForest
        ./build_qml_viewer.sh
        build_qml_release/QMLCameraViewer
    "

# Revoke X11 permission when done
xhost -local:docker
```

### 6.4 Volume Mounting Strategies

Create `docker_volumes.sh`:

```bash
#!/bin/bash

# Optimized volume mounting for Docker development

# Use named volumes for better performance
docker volume create componentsforest-build
docker volume create componentsforest-ccache
docker volume create componentsforest-conan

# Development container with optimized volumes
docker run -it --rm \
    --name cf-dev \
    -v $(pwd):/workspace:cached \
    -v componentsforest-build:/workspace/build:delegated \
    -v componentsforest-ccache:/home/developer/.ccache:delegated \
    -v componentsforest-conan:/home/developer/.conan:delegated \
    -e CCACHE_DIR=/home/developer/.ccache \
    componentsforest:dev

# Sync changes back to host (if needed)
docker cp cf-dev:/workspace/build ./build_docker
```

---

## 7. CI/CD Pipeline

### 7.1 GitHub Actions Configuration

Create `.github/workflows/ubuntu_ci.yml`:

```yaml
name: Ubuntu CI/CD Pipeline

on:
  push:
    branches: [ main, develop ]
  pull_request:
    branches: [ main ]

env:
  QT_VERSION: '6.9.1'
  BUILD_TYPE: Release
  CCACHE_DIR: ${{ github.workspace }}/.ccache

jobs:
  build-and-test:
    runs-on: ubuntu-22.04
    
    strategy:
      matrix:
        compiler: [gcc-11, gcc-12, clang-14]
        build_type: [Debug, Release]
    
    steps:
    - uses: actions/checkout@v3
      with:
        submodules: recursive
    
    - name: Cache ccache
      uses: actions/cache@v3
      with:
        path: .ccache
        key: ${{ runner.os }}-${{ matrix.compiler }}-ccache-${{ hashFiles('**/*.cpp', '**/*.h') }}
        restore-keys: |
          ${{ runner.os }}-${{ matrix.compiler }}-ccache-
    
    - name: Install Qt
      uses: jurplel/install-qt-action@v3
      with:
        version: ${{ env.QT_VERSION }}
        cache: true
        modules: qtcharts qtquick3d
    
    - name: Install dependencies
      run: |
        sudo apt-get update
        sudo apt-get install -y \
          ninja-build ccache valgrind \
          libgl1-mesa-dev libxkbcommon-dev
        
        # Setup compiler
        if [[ "${{ matrix.compiler }}" == clang* ]]; then
          sudo apt-get install -y ${{ matrix.compiler }}
          echo "CC=${{ matrix.compiler }}" >> $GITHUB_ENV
          echo "CXX=${{ matrix.compiler }}++" >> $GITHUB_ENV
        else
          sudo apt-get install -y ${{ matrix.compiler }}
          echo "CC=${{ matrix.compiler }}" >> $GITHUB_ENV
          echo "CXX=g++-${{{ matrix.compiler }}#gcc-}" >> $GITHUB_ENV
        fi
    
    - name: Configure
      run: |
        cmake -B build \
          -DCMAKE_BUILD_TYPE=${{ matrix.build_type }} \
          -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
          -GNinja
    
    - name: Build
      run: cmake --build build --parallel
    
    - name: Test
      run: |
        cd build
        ctest --output-on-failure --parallel
    
    - name: Memory check
      if: matrix.build_type == 'Debug'
      run: |
        cd build
        valgrind --leak-check=full --error-exitcode=1 \
          ./tests/test_base_component
    
    - name: Coverage
      if: matrix.compiler == 'gcc-11' && matrix.build_type == 'Debug'
      run: |
        sudo apt-get install -y lcov
        lcov --capture --directory build --output-file coverage.info
        lcov --remove coverage.info '/usr/*' --output-file coverage.info
        bash <(curl -s https://codecov.io/bash)
    
    - name: Package
      if: matrix.build_type == 'Release'
      run: |
        cd build
        cpack -G DEB
        cpack -G TGZ
    
    - name: Upload artifacts
      if: matrix.build_type == 'Release'
      uses: actions/upload-artifact@v3
      with:
        name: packages-${{ matrix.compiler }}
        path: |
          build/*.deb
          build/*.tar.gz

  performance-test:
    runs-on: ubuntu-22.04
    needs: build-and-test
    
    steps:
    - uses: actions/checkout@v3
    
    - name: Install Qt
      uses: jurplel/install-qt-action@v3
      with:
        version: ${{ env.QT_VERSION }}
    
    - name: Build with profiling
      run: |
        cmake -B build_perf \
          -DCMAKE_BUILD_TYPE=RelWithDebInfo \
          -DCMAKE_CXX_FLAGS="-fno-omit-frame-pointer" \
          -GNinja
        cmake --build build_perf
    
    - name: Run benchmarks
      run: |
        cd build_perf
        ./tests/benchmark_components --benchmark_format=json > benchmark_results.json
    
    - name: Store benchmark result
      uses: benchmark-action/github-action-benchmark@v1
      with:
        tool: 'googletest'
        output-file-path: build_perf/benchmark_results.json
        github-token: ${{ secrets.GITHUB_TOKEN }}
        auto-push: true
```

### 7.2 GitLab CI Setup

Create `.gitlab-ci.yml`:

```yaml
stages:
  - build
  - test
  - deploy

variables:
  QT_VERSION: "6.9.1"
  GIT_SUBMODULE_STRATEGY: recursive
  CCACHE_DIR: "$CI_PROJECT_DIR/.ccache"

cache:
  key: "$CI_COMMIT_REF_SLUG"
  paths:
    - .ccache/
    - build/

before_script:
  - apt-get update -qq
  - apt-get install -y -qq build-essential cmake ninja-build ccache
  - apt-get install -y -qq libgl1-mesa-dev libxkbcommon-dev
  # Install Qt
  - pip3 install aqtinstall
  - aqt install-qt linux desktop ${QT_VERSION} gcc_64 -O /opt/Qt
  - export PATH=/opt/Qt/${QT_VERSION}/gcc_64/bin:$PATH
  - export LD_LIBRARY_PATH=/opt/Qt/${QT_VERSION}/gcc_64/lib:$LD_LIBRARY_PATH

build:debug:
  stage: build
  script:
    - cmake -B build_debug -DCMAKE_BUILD_TYPE=Debug -GNinja
    - cmake --build build_debug --parallel
  artifacts:
    paths:
      - build_debug/

build:release:
  stage: build
  script:
    - cmake -B build_release -DCMAKE_BUILD_TYPE=Release -GNinja
    - cmake --build build_release --parallel
  artifacts:
    paths:
      - build_release/

test:unit:
  stage: test
  dependencies:
    - build:debug
  script:
    - cd build_debug
    - ctest --output-on-failure --parallel
  artifacts:
    reports:
      junit: build_debug/test_results.xml

test:integration:
  stage: test
  dependencies:
    - build:debug
  services:
    - name: python:3.10
      alias: mock-camera
      command: ["python", "/tests/mock_camera_server.py"]
  script:
    - cd build_debug
    - ./tests/integration_tests

test:memory:
  stage: test
  dependencies:
    - build:debug
  script:
    - apt-get install -y valgrind
    - cd build_debug
    - valgrind --leak-check=full --error-exitcode=1 ./tests/test_base_component

deploy:package:
  stage: deploy
  dependencies:
    - build:release
  script:
    - cd build_release
    - cpack -G DEB
    - cpack -G TGZ
  artifacts:
    paths:
      - build_release/*.deb
      - build_release/*.tar.gz
  only:
    - tags
```

### 7.3 Automated Testing on Linux

Create `ci_test_suite.sh`:

```bash
#!/bin/bash

# Comprehensive CI test suite for ComponentsForest

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

function log_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

function log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

function log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

# Build the project
function ci_build() {
    log_info "Building ComponentsForest..."
    
    cmake -B build_ci \
          -DCMAKE_BUILD_TYPE=Debug \
          -DBUILD_TESTING=ON \
          -DCMAKE_CXX_FLAGS="--coverage" \
          -GNinja
    
    cmake --build build_ci --parallel
}

# Run unit tests
function ci_unit_tests() {
    log_info "Running unit tests..."
    
    cd build_ci
    ctest --output-on-failure \
          --parallel $(nproc) \
          --output-junit unit_tests.xml
    cd ..
}

# Run integration tests
function ci_integration_tests() {
    log_info "Running integration tests..."
    
    # Start mock services
    python3 tests/mock_camera_server.py &
    MOCK_PID=$!
    
    sleep 2  # Wait for server to start
    
    # Run tests
    build_ci/tests/integration_tests \
        --gtest_output=xml:integration_tests.xml
    
    # Cleanup
    kill $MOCK_PID 2>/dev/null || true
}

# Static analysis
function ci_static_analysis() {
    log_info "Running static analysis..."
    
    # clang-tidy
    find components viewers -name "*.cpp" -o -name "*.h" | \
    xargs clang-tidy -p build_ci \
        --checks='-*,bugprone-*,performance-*,readability-*' \
        > static_analysis.log 2>&1 || true
    
    # cppcheck
    cppcheck --enable=all \
             --suppress=missingIncludeSystem \
             --xml \
             --xml-version=2 \
             components/ viewers/ 2> cppcheck.xml
}

# Code coverage
function ci_coverage() {
    log_info "Generating code coverage..."
    
    lcov --capture --directory build_ci --output-file coverage.info
    lcov --remove coverage.info '/usr/*' '*/tests/*' --output-file coverage.info
    
    # Generate HTML report
    genhtml coverage.info --output-directory coverage_report
    
    # Check coverage threshold
    COVERAGE=$(lcov --summary coverage.info 2>&1 | grep lines | sed 's/.*: \([0-9.]*\)%.*/\1/')
    if (( $(echo "$COVERAGE < 70" | bc -l) )); then
        log_warning "Code coverage is below 70%: ${COVERAGE}%"
    else
        log_info "Code coverage: ${COVERAGE}%"
    fi
}

# Memory sanitization
function ci_memory_check() {
    log_info "Running memory checks..."
    
    # Rebuild with sanitizers
    cmake -B build_asan \
          -DCMAKE_BUILD_TYPE=Debug \
          -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined" \
          -GNinja
    
    cmake --build build_asan --parallel
    
    # Run with AddressSanitizer
    ASAN_OPTIONS=halt_on_error=0:print_stats=1 \
    build_asan/tests/test_base_component 2> asan_report.log
}

# Performance regression test
function ci_performance_test() {
    log_info "Running performance tests..."
    
    build_ci/tests/benchmark_components \
        --benchmark_out=benchmark_results.json \
        --benchmark_out_format=json
    
    # Compare with baseline (if exists)
    if [ -f benchmark_baseline.json ]; then
        python3 tests/compare_benchmarks.py \
            benchmark_baseline.json \
            benchmark_results.json
    fi
}

# Main CI pipeline
function main() {
    log_info "Starting CI pipeline..."
    
    ci_build
    ci_unit_tests
    ci_integration_tests
    ci_static_analysis
    ci_coverage
    ci_memory_check
    ci_performance_test
    
    log_info "CI pipeline completed successfully!"
}

main "$@"
```

### 7.4 Deployment Packaging

Create `package_release.sh`:

```bash
#!/bin/bash

# ComponentsForest Release Packaging Script

VERSION=${1:-1.0.0}
ARCH=$(dpkg --print-architecture)
RELEASE_DIR="release_${VERSION}_${ARCH}"

# Build release version
cmake -B build_release \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_INSTALL_PREFIX=/usr \
      -GNinja

cmake --build build_release --parallel

# Create package structure
mkdir -p "${RELEASE_DIR}/DEBIAN"
mkdir -p "${RELEASE_DIR}/usr/bin"
mkdir -p "${RELEASE_DIR}/usr/lib/componentsforest"
mkdir -p "${RELEASE_DIR}/usr/share/applications"
mkdir -p "${RELEASE_DIR}/usr/share/icons/hicolor/256x256/apps"

# Copy binaries
cp build_release/viewers/do3think_camera_viewer/Do3ThinkCameraViewer \
   "${RELEASE_DIR}/usr/bin/"
cp build_release/viewers/qml_camera_viewer/QMLCameraViewer \
   "${RELEASE_DIR}/usr/bin/"

# Copy libraries
cp build_release/*.so* "${RELEASE_DIR}/usr/lib/componentsforest/"

# Create desktop entry
cat > "${RELEASE_DIR}/usr/share/applications/componentsforest.desktop" << EOF
[Desktop Entry]
Type=Application
Name=ComponentsForest Camera Viewer
Comment=Industrial AOI Camera Viewer
Exec=/usr/bin/Do3ThinkCameraViewer
Icon=componentsforest
Categories=Development;Engineering;
Terminal=false
EOF

# Create control file
cat > "${RELEASE_DIR}/DEBIAN/control" << EOF
Package: componentsforest
Version: ${VERSION}
Section: devel
Priority: optional
Architecture: ${ARCH}
Depends: libqt6core6 (>= 6.9.0), libqt6widgets6 (>= 6.9.0), libqt6quick6 (>= 6.9.0)
Maintainer: ComponentsForest Team <team@componentsforest.org>
Description: Industrial AOI Equipment Component Ecosystem
 Qt6.9-based component framework for automated optical inspection
 equipment with complete Signal/Slot decoupling.
EOF

# Create postinst script
cat > "${RELEASE_DIR}/DEBIAN/postinst" << 'EOF'
#!/bin/bash
ldconfig
update-desktop-database
EOF
chmod 755 "${RELEASE_DIR}/DEBIAN/postinst"

# Build DEB package
dpkg-deb --build "${RELEASE_DIR}" "componentsforest_${VERSION}_${ARCH}.deb"

# Create AppImage
wget -c https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage
wget -c https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage

chmod +x linuxdeploy*.AppImage

./linuxdeploy-x86_64.AppImage \
    --appdir AppDir \
    --executable build_release/viewers/do3think_camera_viewer/Do3ThinkCameraViewer \
    --desktop-file "${RELEASE_DIR}/usr/share/applications/componentsforest.desktop" \
    --plugin qt \
    --output appimage

mv ComponentsForest*.AppImage "ComponentsForest-${VERSION}-${ARCH}.AppImage"

echo "Packages created:"
echo "  - componentsforest_${VERSION}_${ARCH}.deb"
echo "  - ComponentsForest-${VERSION}-${ARCH}.AppImage"
```

---

## 8. Shell Scripts and Automation

### 8.1 Useful Bash Aliases

Add to `~/.bashrc`:

```bash
# ComponentsForest Development Aliases

# Quick navigation
alias cf='cd ~/ComponentsForest'
alias cfb='cd ~/ComponentsForest/build'
alias cfc='cd ~/ComponentsForest/components'
alias cfv='cd ~/ComponentsForest/viewers'

# Build commands
alias cfbuild='cmake --build build --parallel'
alias cfclean='rm -rf build && mkdir build'
alias cfrebuild='cfclean && cmake -B build && cfbuild'
alias cftest='ctest --test-dir build --output-on-failure'

# Qt specific
alias qmlscene6='/opt/Qt/6.9.1/gcc_64/bin/qmlscene'
alias qmlformat='/opt/Qt/6.9.1/gcc_64/bin/qmlformat'
alias qmllint='/opt/Qt/6.9.1/gcc_64/bin/qmllint'

# Debugging
alias cfgdb='gdb build/viewers/do3think_camera_viewer/Do3ThinkCameraViewer'
alias cfvalgrind='valgrind --leak-check=full build/viewers/do3think_camera_viewer/Do3ThinkCameraViewer'
alias cfasan='ASAN_OPTIONS=symbolize=1 build_asan/viewers/do3think_camera_viewer/Do3ThinkCameraViewer'

# Git shortcuts
alias gst='git status'
alias gco='git checkout'
alias gcm='git commit -m'
alias gp='git push'
alias gl='git log --oneline --graph --decorate'

# Component generation
cfnew() {
    local component_name=$1
    if [ -z "$component_name" ]; then
        echo "Usage: cfnew <component_name>"
        return 1
    fi
    
    python3 scripts/generate_component.py "$component_name"
    echo "Created component: $component_name"
}

# Quick run with logging
cfrun() {
    QT_LOGGING_RULES="ComponentsForest.*=true" \
    build/viewers/do3think_camera_viewer/Do3ThinkCameraViewer "$@" 2>&1 | \
    tee "run_$(date +%Y%m%d_%H%M%S).log"
}

# Search project
cfsearch() {
    rg "$1" --type cpp --type h --type qml
}

# Format code
cfformat() {
    find components viewers -name "*.cpp" -o -name "*.h" | \
    xargs clang-format -i
    
    find viewers -name "*.qml" | \
    xargs /opt/Qt/6.9.1/gcc_64/bin/qmlformat -i
}
```

### 8.2 Development Helper Scripts

Create `scripts/dev_tools.sh`:

```bash
#!/bin/bash

# ComponentsForest Development Tools

# Quick setup for new developers
function setup_dev_env() {
    echo "Setting up ComponentsForest development environment..."
    
    # Install dependencies
    sudo apt update
    sudo apt install -y \
        build-essential cmake ninja-build git \
        libgl1-mesa-dev libxkbcommon-dev \
        gdb valgrind ccache clang-format \
        python3-pip inotify-tools
    
    # Install Qt
    if [ ! -d "/opt/Qt/6.9.1" ]; then
        pip3 install aqtinstall
        sudo aqt install-qt linux desktop 6.9.1 gcc_64 -O /opt/Qt
    fi
    
    # Setup git hooks
    setup_git_hooks
    
    # Create initial build
    cmake -B build -GNinja
    cmake --build build --parallel
    
    echo "Development environment ready!"
}

# Setup git hooks
function setup_git_hooks() {
    # Pre-commit hook for formatting
    cat > .git/hooks/pre-commit << 'EOF'
#!/bin/bash
# Format staged C++ files
for file in $(git diff --cached --name-only --diff-filter=d | grep -E '\.(cpp|h)$'); do
    clang-format -i "$file"
    git add "$file"
done

# Check QML files
for file in $(git diff --cached --name-only --diff-filter=d | grep -E '\.qml$'); do
    /opt/Qt/6.9.1/gcc_64/bin/qmllint "$file"
    if [ $? -ne 0 ]; then
        echo "QML lint failed for $file"
        exit 1
    fi
done
EOF
    chmod +x .git/hooks/pre-commit
    
    # Pre-push hook for tests
    cat > .git/hooks/pre-push << 'EOF'
#!/bin/bash
# Run tests before push
cmake --build build --target test
if [ $? -ne 0 ]; then
    echo "Tests failed, push aborted"
    exit 1
fi
EOF
    chmod +x .git/hooks/pre-push
}

# Watch and rebuild on changes
function watch_and_build() {
    echo "Watching for changes..."
    
    while true; do
        inotifywait -r -e modify,create,delete \
            --exclude '(build|\.git|\.swp)' \
            components/ viewers/ tests/ CMakeLists.txt
        
        clear
        echo "Changes detected, rebuilding..."
        cmake --build build --parallel
        
        if [ $? -eq 0 ]; then
            echo -e "\033[32mBuild successful!\033[0m"
            
            # Run tests if build succeeds
            ctest --test-dir build --output-on-failure
        else
            echo -e "\033[31mBuild failed!\033[0m"
        fi
    done
}

# Generate component documentation
function generate_docs() {
    doxygen Doxyfile
    
    # Generate QML documentation
    /opt/Qt/6.9.1/gcc_64/bin/qdoc componentsforest.qdocconf
    
    echo "Documentation generated in docs/html/"
}

# Profile build times
function profile_build() {
    # Clean build with timing
    rm -rf build_profile
    
    # Time each target
    cmake -B build_profile -GNinja \
          -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
    
    ninja -C build_profile -t targets | grep -E "^[^:]+:" | cut -d: -f1 | \
    while read target; do
        echo -n "$target: "
        time ninja -C build_profile "$target" 2>&1 | tail -1
    done > build_times.txt
    
    echo "Build times saved to build_times.txt"
}

# Interactive menu
function main_menu() {
    PS3='Select operation: '
    options=(
        "Setup development environment"
        "Watch and build"
        "Generate documentation"
        "Profile build"
        "Run tests"
        "Format code"
        "Exit"
    )
    
    select opt in "${options[@]}"; do
        case $opt in
            "Setup development environment")
                setup_dev_env
                ;;
            "Watch and build")
                watch_and_build
                ;;
            "Generate documentation")
                generate_docs
                ;;
            "Profile build")
                profile_build
                ;;
            "Run tests")
                ctest --test-dir build --output-on-failure
                ;;
            "Format code")
                cfformat
                ;;
            "Exit")
                break
                ;;
            *)
                echo "Invalid option"
                ;;
        esac
    done
}

# Run if executed directly
if [ "${BASH_SOURCE[0]}" == "${0}" ]; then
    main_menu
fi
```

### 8.3 Build Automation

Create `scripts/auto_build.sh`:

```bash
#!/bin/bash

# Automated build system with multiple configurations

CONFIGS="Debug Release RelWithDebInfo MinSizeRel"
QT_PATH="/opt/Qt/6.9.1/gcc_64"
BUILD_ROOT="builds"

# Build all configurations
function build_all_configs() {
    for config in $CONFIGS; do
        echo "Building $config configuration..."
        
        build_dir="${BUILD_ROOT}/${config}"
        
        cmake -B "$build_dir" \
              -DCMAKE_BUILD_TYPE="$config" \
              -DCMAKE_PREFIX_PATH="$QT_PATH" \
              -GNinja
        
        cmake --build "$build_dir" --parallel
        
        # Run tests for each configuration
        ctest --test-dir "$build_dir" \
              --output-on-failure \
              --output-junit "${build_dir}/test_results.xml"
    done
}

# Parallel builds with job server
function parallel_builds() {
    export MAKEFLAGS="-j$(nproc)"
    
    # Use GNU parallel for multiple configurations
    parallel --jobs 2 --tag \
        "cmake -B ${BUILD_ROOT}/{} -DCMAKE_BUILD_TYPE={} && \
         cmake --build ${BUILD_ROOT}/{}" \
        ::: $CONFIGS
}

# Incremental nightly build
function nightly_build() {
    # Update code
    git pull origin main
    
    # Clean old builds
    find "$BUILD_ROOT" -type d -mtime +7 -exec rm -rf {} \; 2>/dev/null
    
    # Build and test
    build_all_configs
    
    # Generate reports
    generate_reports
    
    # Send notification
    send_build_notification
}

# Generate build reports
function generate_reports() {
    report_file="build_report_$(date +%Y%m%d).html"
    
    cat > "$report_file" << EOF
<!DOCTYPE html>
<html>
<head><title>Build Report</title></head>
<body>
<h1>ComponentsForest Build Report</h1>
<p>Date: $(date)</p>
<table border="1">
<tr><th>Configuration</th><th>Status</th><th>Tests</th><th>Time</th></tr>
EOF
    
    for config in $CONFIGS; do
        build_dir="${BUILD_ROOT}/${config}"
        
        if [ -f "${build_dir}/CMakeCache.txt" ]; then
            status="✓ Success"
        else
            status="✗ Failed"
        fi
        
        test_count=$(grep -c "<testcase" "${build_dir}/test_results.xml" 2>/dev/null || echo "0")
        
        echo "<tr><td>$config</td><td>$status</td><td>$test_count</td><td>N/A</td></tr>" >> "$report_file"
    done
    
    echo "</table></body></html>" >> "$report_file"
}

# Send build notification
function send_build_notification() {
    # Desktop notification
    notify-send "ComponentsForest Build" "Build completed at $(date)"
    
    # Email notification (if configured)
    if [ -n "$BUILD_EMAIL" ]; then
        mail -s "ComponentsForest Build Report" "$BUILD_EMAIL" < build_report_*.html
    fi
}

build_all_configs
```

### 8.4 Log Management

Create `scripts/log_manager.sh`:

```bash
#!/bin/bash

# ComponentsForest Log Management System

LOG_DIR="logs"
MAX_LOG_AGE=30  # days
MAX_LOG_SIZE="100M"

# Centralized logging setup
function setup_logging() {
    mkdir -p "$LOG_DIR"
    
    # Create logging configuration
    cat > logging.conf << EOF
[loggers]
keys=root,componentsforest

[handlers]
keys=console,file,syslog

[formatters]
keys=detailed

[logger_root]
level=INFO
handlers=console

[logger_componentsforest]
level=DEBUG
handlers=file,syslog
qualname=ComponentsForest

[handler_console]
class=StreamHandler
level=INFO
formatter=detailed
args=(sys.stdout,)

[handler_file]
class=handlers.RotatingFileHandler
level=DEBUG
formatter=detailed
args=('${LOG_DIR}/componentsforest.log', 'a', 10485760, 5)

[handler_syslog]
class=handlers.SysLogHandler
level=ERROR
formatter=detailed
args=('/dev/log',)

[formatter_detailed]
format=%(asctime)s - %(name)s - %(levelname)s - %(message)s
datefmt=%Y-%m-%d %H:%M:%S
EOF
}

# Rotate logs
function rotate_logs() {
    # Use logrotate configuration
    cat > /etc/logrotate.d/componentsforest << EOF
${LOG_DIR}/*.log {
    daily
    rotate 7
    compress
    delaycompress
    missingok
    notifempty
    create 644 $USER $USER
    sharedscripts
    postrotate
        systemctl reload componentsforest 2>/dev/null || true
    endscript
}
EOF
}

# Parse and analyze logs
function analyze_logs() {
    local log_file=${1:-"${LOG_DIR}/componentsforest.log"}
    
    echo "=== Log Analysis for $log_file ==="
    echo
    
    # Error summary
    echo "Error Summary:"
    grep -i error "$log_file" | cut -d' ' -f5- | sort | uniq -c | sort -rn | head -10
    
    echo
    echo "Warning Summary:"
    grep -i warning "$log_file" | cut -d' ' -f5- | sort | uniq -c | sort -rn | head -10
    
    echo
    echo "Performance Issues:"
    grep -E "took [0-9]{4,}ms" "$log_file" | tail -10
    
    echo
    echo "Component Statistics:"
    grep "Component:" "$log_file" | cut -d' ' -f4 | sort | uniq -c
}

# Real-time log monitoring
function monitor_logs() {
    # Use multitail for multiple log files
    if command -v multitail &> /dev/null; then
        multitail \
            -i "${LOG_DIR}/componentsforest.log" \
            -i "${LOG_DIR}/camera.log" \
            -i "${LOG_DIR}/performance.log"
    else
        # Fallback to tail
        tail -f "${LOG_DIR}"/*.log
    fi
}

# Archive old logs
function archive_logs() {
    local archive_dir="${LOG_DIR}/archive"
    mkdir -p "$archive_dir"
    
    # Find and compress old logs
    find "$LOG_DIR" -name "*.log" -mtime +${MAX_LOG_AGE} -exec gzip {} \;
    
    # Move to archive
    find "$LOG_DIR" -name "*.log.gz" -exec mv {} "$archive_dir/" \;
    
    echo "Archived $(find "$archive_dir" -name "*.log.gz" | wc -l) log files"
}

# Clean up old logs
function cleanup_logs() {
    # Remove very old archives
    find "${LOG_DIR}/archive" -name "*.log.gz" -mtime +90 -delete
    
    # Remove empty log files
    find "$LOG_DIR" -name "*.log" -empty -delete
    
    # Truncate large active logs
    find "$LOG_DIR" -name "*.log" -size +${MAX_LOG_SIZE} -exec truncate -s 0 {} \;
}

# Main menu
case "$1" in
    setup)
        setup_logging
        ;;
    rotate)
        rotate_logs
        ;;
    analyze)
        analyze_logs "$2"
        ;;
    monitor)
        monitor_logs
        ;;
    archive)
        archive_logs
        ;;
    cleanup)
        cleanup_logs
        ;;
    *)
        echo "Usage: $0 {setup|rotate|analyze|monitor|archive|cleanup}"
        ;;
esac
```

---

## Summary

This comprehensive Ubuntu development workflow guide provides:

1. **Complete IDE setup** for Qt Creator, VS Code, and CLion with project-specific configurations
2. **Optimized build system** using ccache, distcc, and incremental build strategies
3. **Structured development workflow** with component templates and automated testing
4. **Camera development tools** including USB debugging, virtual cameras, and CI mocking
5. **Performance optimization** with profiling, memory checking, and build optimization
6. **Docker environment** for consistent development and testing across teams
7. **Full CI/CD pipelines** for GitHub Actions and GitLab CI with comprehensive testing
8. **Extensive shell automation** with aliases, helper scripts, and log management

The guide focuses on practical, real-world workflows specific to the ComponentsForest project, providing ready-to-use scripts and configurations that can be immediately applied to improve development productivity on Ubuntu systems.