# Ubuntu Quick Reference Guide for ComponentsForest

Quick reference cheat sheet for ComponentsForest development on Ubuntu/WSL2. Keep this open while working!

## 1. Essential Commands Cheat Sheet

### Build Commands
```bash
# Quick Build (Auto-detects Qt)
./quick_build_linux.sh                    # QWidget version
./build_qml_viewer.sh                      # QML version

# Build with specific Qt path
./quick_build_linux.sh ~/Qt/6.9.1/gcc_64
./build_qml_viewer.sh --qt-dir ~/Qt/6.9.1/gcc_64

# Standard CMake build
mkdir -p build && cd build
cmake .. -DCMAKE_PREFIX_PATH=~/Qt/6.9.1/gcc_64 -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

# Debug build
./build_qml_viewer.sh --debug

# Build with OpenCV support
./build_qml_viewer.sh --with-opencv

# Clean rebuild
rm -rf build_qml_release build_viewer
./build_qml_viewer.sh
```

### Running Commands
```bash
# Run QWidget viewer
cd build_viewer/viewers/do3think_camera_viewer
./Do3ThinkCameraViewer

# Run QML viewer
cd build_qml_release
./run_qml_viewer.sh

# Run with environment variables
export QT_LOGGING_RULES="ComponentsForest.*=true"
./QMLCameraViewer

# Run tests
./run_qml_tests.sh
./verify_qml_implementation.sh --verbose
```

### Debugging Commands
```bash
# Enable Qt debug output
export QT_LOGGING_RULES="*.debug=true"
export QT_DEBUG_PLUGINS=1

# GDB debugging
gdb ./QMLCameraViewer
(gdb) run
(gdb) bt                    # backtrace on crash

# Valgrind memory check
valgrind --leak-check=full ./QMLCameraViewer

# Strace system calls
strace -e openat ./QMLCameraViewer 2>&1 | grep -E "\.so|plugin"

# Check shared libraries
ldd ./QMLCameraViewer | grep "not found"
```

### Git Workflow
```bash
# Status and diff
git status
git diff --staged
git log --oneline -10

# Branching
git checkout -b feature/my-feature
git push -u origin feature/my-feature

# Stash changes
git stash save "WIP: description"
git stash pop

# Clean workspace
git clean -fdx               # Remove all untracked files
git reset --hard HEAD        # Reset to last commit
```

## 2. Quick Fixes

| Error | Fix |
|-------|-----|
| **"cannot find -lQt6Core"** | `export LD_LIBRARY_PATH=~/Qt/6.9.1/gcc_64/lib:$LD_LIBRARY_PATH` |
| **"qwindows.dll not found"** | Linux: Install `libqt6-qpa-plugins` or copy from Qt/plugins/platforms |
| **"Failed to load platform plugin"** | `export QT_QPA_PLATFORM_PLUGIN_PATH=~/Qt/6.9.1/gcc_64/plugins/platforms` |
| **"DVPCamera.h not found"** | Check Do3ThinkCamera/SDK/include exists |
| **"libDVPCamera64.so not found"** | `export LD_LIBRARY_PATH=$PWD/Do3ThinkCamera/SDK:$LD_LIBRARY_PATH` |
| **"Permission denied"** | `chmod +x script.sh` or use `sudo` for system operations |
| **"CMake version too old"** | Use bundled: `./cmake-3.27.4-linux-x86_64/bin/cmake` |
| **"No module named Qt6"** | Install: `sudo apt install qt6-base-dev qt6-declarative-dev` |
| **"undefined reference to vtable"** | Clean rebuild: `rm -rf build && mkdir build` |
| **"QML module not found"** | `export QML2_IMPORT_PATH=~/Qt/6.9.1/gcc_64/qml:$QML2_IMPORT_PATH` |

### Permission Issues
```bash
# Fix script permissions
chmod +x *.sh

# Fix library permissions
chmod 644 *.so
chmod 755 directories/

# USB camera permissions
sudo usermod -a -G video $USER  # Add user to video group
sudo chmod 666 /dev/video*      # Quick fix (temporary)

# Create udev rule (permanent)
echo 'SUBSYSTEM=="usb", ATTR{idVendor}=="YOUR_VENDOR_ID", MODE="0666"' | \
  sudo tee /etc/udev/rules.d/99-camera.rules
sudo udevadm control --reload-rules
```

## 3. Environment Variables Quick Reference

### Qt Variables
```bash
# Add to ~/.bashrc for persistence
export QTDIR=~/Qt/6.9.1/gcc_64
export PATH=$QTDIR/bin:$PATH
export LD_LIBRARY_PATH=$QTDIR/lib:$LD_LIBRARY_PATH

# QML specific
export QML2_IMPORT_PATH=$QTDIR/qml:$PWD/qml:$QML2_IMPORT_PATH
export QT_QUICK_CONTROLS_STYLE=Material
export QSG_RENDER_LOOP=threaded

# Plugin paths
export QT_QPA_PLATFORM_PLUGIN_PATH=$QTDIR/plugins/platforms
export QT_PLUGIN_PATH=$QTDIR/plugins

# Debug output
export QT_LOGGING_RULES="ComponentsForest.*=true;qt.qml.debug=true"
export QT_DEBUG_PLUGINS=1
export QSG_INFO=1                   # Scene graph info
```

### Do3Think SDK Variables
```bash
export DO3THINK_SDK=$PWD/Do3ThinkCamera/SDK
export LD_LIBRARY_PATH=$DO3THINK_SDK:$LD_LIBRARY_PATH
```

## 4. File Locations Reference

| Item | Location |
|------|----------|
| **Qt Installation** | `~/Qt/6.9.1/gcc_64` or `/opt/Qt/6.9.1/gcc_64` |
| **System Qt** | `/usr/lib/x86_64-linux-gnu/qt6` |
| **Built Binaries** | `build_viewer/viewers/do3think_camera_viewer/` |
| **QML Binary** | `build_qml_release/bin/QMLCameraViewer` |
| **Camera SDK** | `Do3ThinkCamera/SDK/` |
| **Qt Plugins** | `$QTDIR/plugins/` |
| **QML Modules** | `$QTDIR/qml/` |
| **Logs** | `debug.log`, `build/*.log` |
| **CMake Cache** | `build/CMakeCache.txt` |
| **Qt Config** | `~/.config/QtProject/` |

## 5. Network and Camera Commands

### USB Camera Detection
```bash
# List USB devices
lsusb
lsusb -v | grep -E "idVendor|idProduct|iProduct"

# List video devices
ls -la /dev/video*
v4l2-ctl --list-devices

# Camera capabilities
v4l2-ctl -d /dev/video0 --all
v4l2-ctl -d /dev/video0 --list-formats-ext

# Test camera
ffplay /dev/video0                  # Quick preview
cheese                               # GUI camera test
```

### Network Camera Debugging
```bash
# Check network interfaces
ip addr show
ifconfig

# Scan for cameras (GigE Vision)
sudo arp-scan --local | grep -i camera
nmap -sn 192.168.1.0/24            # Scan subnet

# Check multicast
netstat -g                          # Multicast groups
tcpdump -i eth0 multicast          # Monitor multicast

# Test connectivity
ping camera_ip
nc -zv camera_ip 3956              # GigE Vision control port
```

### Do3Think Camera Tools
```bash
# Check SDK library
ldconfig -p | grep DVP
nm -D libDVPCamera64.so | grep dvpInit

# Monitor camera discovery
export DVP_DEBUG=1
strace -e openat,ioctl ./QMLCameraViewer 2>&1 | grep -E "usb|video"
```

## 6. Performance Quick Checks

### CPU Monitoring
```bash
# Real-time CPU usage
top -H -p $(pgrep QMLCameraViewer)
htop                                # Interactive

# Per-thread CPU
ps -eLf | grep QMLCameraViewer
pidstat -t -p $(pgrep QMLCameraViewer) 1
```

### Memory Monitoring
```bash
# Memory usage
free -h
watch -n 1 'ps aux | grep QMLCameraViewer'

# Detailed memory map
pmap -x $(pgrep QMLCameraViewer)
cat /proc/$(pgrep QMLCameraViewer)/status | grep -E "Vm|Rss"

# Memory leaks
valgrind --leak-check=full --show-leak-kinds=all ./QMLCameraViewer
```

### Build Optimization
```bash
# Parallel build (use all cores)
make -j$(nproc)
cmake --build . --parallel $(nproc)

# Release build with optimizations
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DCMAKE_CXX_FLAGS="-O3 -march=native"

# Link time optimization
cmake .. -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON

# ccache for faster rebuilds
sudo apt install ccache
export PATH=/usr/lib/ccache:$PATH
```

### Graphics Performance
```bash
# OpenGL info
glxinfo | grep -E "OpenGL|renderer"

# Qt Scene Graph debugging
export QSG_VISUALIZE=batches       # Visualize batching
export QSG_VISUALIZE=clip          # Visualize clipping
export QSG_VISUALIZE=overdraw      # Show overdraw

# Frame timing
export QSG_RENDER_TIMING=1
```

## 7. Emergency Recovery

### Clean Rebuild
```bash
#!/bin/bash
# Full clean rebuild
rm -rf build* CMakeCache.txt CMakeFiles/
rm -rf ~/.cache/ComponentsForest
find . -name "*.o" -delete
find . -name "moc_*" -delete
find . -name "ui_*" -delete

# Rebuild
mkdir build && cd build
cmake .. -DCMAKE_PREFIX_PATH=~/Qt/6.9.1/gcc_64
make -j$(nproc)
```

### Cache Clearing
```bash
# CMake cache
rm -rf build/CMakeCache.txt build/CMakeFiles/

# Qt cache
rm -rf ~/.cache/QtProject/
rm -rf ~/.cache/qmlcache/

# Shader cache
rm -rf ~/.cache/qtshadercache/

# ccache
ccache -C                           # Clear all
ccache -z                           # Zero statistics
```

### Dependency Reset
```bash
# Update package lists
sudo apt update

# Reinstall Qt dependencies
sudo apt install --reinstall \
  qt6-base-dev \
  qt6-declarative-dev \
  qt6-tools-dev \
  qt6-multimedia-dev \
  libgl1-mesa-dev \
  libglu1-mesa-dev

# Fix broken packages
sudo apt --fix-broken install
sudo dpkg --configure -a

# Rebuild Qt MOC files
find . -name "moc_*.cpp" -delete
find . -name "*.moc" -delete
qmake -spec linux-g++ CONFIG+=release
```

### Library Issues
```bash
# Regenerate library cache
sudo ldconfig

# Find missing libraries
ldd ./QMLCameraViewer | grep "not found"

# Set library path permanently
echo "export LD_LIBRARY_PATH=~/Qt/6.9.1/gcc_64/lib:$LD_LIBRARY_PATH" >> ~/.bashrc
source ~/.bashrc

# Fix RPATH
patchelf --set-rpath '$ORIGIN:$ORIGIN/../lib' ./QMLCameraViewer
```

## Quick Command Aliases

Add to `~/.bashrc`:
```bash
# ComponentsForest aliases
alias cfbuild='./quick_build_linux.sh'
alias cfqml='./build_qml_viewer.sh'
alias cfrun='cd build_qml_release && ./run_qml_viewer.sh'
alias cfclean='rm -rf build* && find . -name "*.o" -delete'
alias cftest='./verify_qml_implementation.sh --verbose'
alias cflog='tail -f debug.log'
alias cfcam='v4l2-ctl --list-devices'
```

## Common Build Patterns

```bash
# Development build (with debug symbols)
cmake .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

# Production build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-O3"

# Build specific target
cmake --build . --target QMLCameraViewer

# Verbose build (show commands)
make VERBOSE=1

# Continue on error
make -k

# Dry run (show what would be built)
make -n
```

## Troubleshooting Checklist

When something doesn't work:

1. ✓ Check Qt is in PATH: `which qmake`
2. ✓ Check library path: `echo $LD_LIBRARY_PATH`
3. ✓ Check permissions: `ls -la executable`
4. ✓ Check dependencies: `ldd executable`
5. ✓ Clean rebuild: `rm -rf build && mkdir build`
6. ✓ Check logs: `tail -100 debug.log`
7. ✓ Verify Qt version: `qmake --version`
8. ✓ Check CMake cache: `grep PREFIX build/CMakeCache.txt`

---
*Keep this guide handy during development. Updated for ComponentsForest Qt 6.9.1*