# ComponentsForest Ubuntu Installation Guide

## Table of Contents
1. [System Requirements](#system-requirements)
2. [Prerequisites Installation](#prerequisites-installation)
3. [Qt 6.9.1 Installation](#qt-691-installation)
4. [OpenCV Installation](#opencv-installation)
5. [Do3Think SDK Setup](#do3think-sdk-setup)
6. [Environment Configuration](#environment-configuration)
7. [Building ComponentsForest](#building-componentsforest)
8. [Verification Steps](#verification-steps)
9. [Troubleshooting](#troubleshooting)
10. [Optional Development Tools](#optional-development-tools)

---

## System Requirements

### Minimum Hardware Requirements
- **CPU**: x86_64 processor (Intel Core i5 or AMD Ryzen 5 or better)
- **RAM**: 8GB minimum, 16GB recommended
- **Storage**: 20GB free space (including Qt installation)
- **GPU**: OpenGL 3.3+ support for QML viewer

### Supported Ubuntu Versions
- **Ubuntu 20.04 LTS (Focal Fossa)** - Recommended for stability
- **Ubuntu 22.04 LTS (Jammy Jellyfish)** - Latest LTS version
- **Ubuntu 24.04 LTS (Noble Numbat)** - Newest version (may require adjustments)

### Network Requirements
- Internet connection for package installation
- Access to Qt online installer or offline packages
- Optional: Access to Do3Think SDK download server

---

## Prerequisites Installation

### Step 1: Update System Packages

```bash
# Update package list and upgrade existing packages
sudo apt update && sudo apt upgrade -y
```

### Step 2: Install Essential Build Tools

```bash
# Install GCC, G++, Make, and other build essentials
sudo apt install -y \
    build-essential \
    gcc-10 \
    g++-10 \
    make \
    cmake \
    ninja-build \
    git \
    wget \
    curl \
    pkg-config
```

**Ubuntu 20.04 specific:**
```bash
# Set GCC 10 as default on Ubuntu 20.04
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-10 100
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-10 100
```

**Ubuntu 22.04/24.04 specific:**
```bash
# GCC 11+ is default, no changes needed
gcc --version  # Should show 11.x or higher
```

### Step 3: Install Development Libraries

```bash
# X11 and OpenGL development libraries (required for Qt)
sudo apt install -y \
    libgl1-mesa-dev \
    libglu1-mesa-dev \
    libxcb-xinerama0 \
    libxcb-icccm4 \
    libxcb-image0 \
    libxcb-keysyms1 \
    libxcb-randr0 \
    libxcb-render-util0 \
    libxcb-shape0 \
    libxcb-sync1 \
    libxcb-xfixes0 \
    libxcb-xkb1 \
    libxkbcommon-x11-0 \
    libxkbcommon-dev \
    libx11-dev \
    libx11-xcb-dev \
    libxext-dev \
    libxfixes-dev \
    libxi-dev \
    libxrender-dev \
    libxcb1-dev \
    libxcb-glx0-dev \
    libxcb-util-dev \
    libxcb-cursor-dev

# Additional multimedia and network libraries
sudo apt install -y \
    libasound2-dev \
    libpulse-dev \
    libcups2-dev \
    libssl-dev \
    libfontconfig1-dev \
    libfreetype6-dev \
    libdbus-1-dev \
    libwayland-dev \
    libgtk-3-dev
```

### Step 4: Verify CMake Version

```bash
# Check CMake version (minimum 3.16 required)
cmake --version

# If CMake version is less than 3.16, install newer version:
# For Ubuntu 20.04 (has 3.16 by default, skip if satisfied)
# For older versions or to get latest:
wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | \
    gpg --dearmor - | sudo tee /usr/share/keyrings/kitware-archive-keyring.gpg >/dev/null

echo 'deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] https://apt.kitware.com/ubuntu/ focal main' | \
    sudo tee /etc/apt/sources.list.d/kitware.list >/dev/null

sudo apt update
sudo apt install -y cmake
```

**Expected output:**
```
cmake version 3.16.3 or higher
```

---

## Qt 6.9.1 Installation

### Option A: Online Installer (Recommended)

```bash
# Create Qt installation directory
mkdir -p ~/Qt
cd ~/Downloads

# Download Qt online installer
wget https://download.qt.io/official_releases/online_installers/qt-unified-linux-x64-online.run

# Make installer executable
chmod +x qt-unified-linux-x64-online.run

# Run installer
./qt-unified-linux-x64-online.run
```

**Installation steps:**
1. Log in with Qt account (create one if needed)
2. Select "Custom installation"
3. Choose Qt 6.9.1 (or latest 6.9.x)
4. Select components:
   - Desktop gcc 64-bit
   - Sources
   - Qt Charts
   - Qt Quick 3D (for QML)
   - Qt Quick Controls
   - Qt Multimedia
5. Installation path: `/home/$USER/Qt` (default)
6. Complete installation (approximately 4-6 GB)

### Option B: Build Qt from Source

```bash
# Install additional dependencies for Qt build
sudo apt install -y \
    libatspi2.0-dev \
    libmtdev-dev \
    libinput-dev \
    libxcb-xinerama0-dev \
    libxcb-xinput-dev \
    perl \
    python3

# Create build directory
mkdir -p ~/qt-build
cd ~/qt-build

# Download Qt 6.9.1 source
wget https://download.qt.io/official_releases/qt/6.9/6.9.1/single/qt-everywhere-src-6.9.1.tar.xz

# Extract source
tar xf qt-everywhere-src-6.9.1.tar.xz
cd qt-everywhere-src-6.9.1

# Configure Qt build (minimal configuration for ComponentsForest)
./configure \
    -prefix /usr/local/Qt-6.9.1 \
    -opensource \
    -confirm-license \
    -release \
    -shared \
    -platform linux-g++ \
    -nomake examples \
    -nomake tests \
    -skip qtwebengine \
    -skip qt3d \
    -skip qtlocation \
    -qt-libjpeg \
    -qt-libpng \
    -qt-zlib \
    -qt-pcre \
    -qt-harfbuzz \
    -opengl desktop \
    -feature-widgets \
    -feature-quickcontrols2-material

# Build Qt (this will take 1-3 hours)
cmake --build . --parallel $(nproc)

# Install Qt
sudo cmake --install .
```

### Option C: Use Pre-built Packages (If Available)

```bash
# For Ubuntu 22.04+ (Qt 6.2 available in repos, not 6.9)
# This is NOT recommended as ComponentsForest requires Qt 6.9+
# Only use if you want to test basic functionality

sudo apt install -y \
    qt6-base-dev \
    qt6-charts-dev \
    qt6-declarative-dev \
    qt6-quickcontrols2-dev \
    qt6-tools-dev \
    qt6-tools-dev-tools \
    qml6-module-qtquick-controls \
    qml6-module-qtquick-layouts \
    qml6-module-qtquick-window \
    qml6-module-qtqml-workerscript
```

### Verify Qt Installation

```bash
# If installed via online installer or from source
export PATH=~/Qt/6.9.1/gcc_64/bin:$PATH  # Adjust path as needed
qmake --version

# Expected output:
# QMake version 3.1
# Using Qt version 6.9.1 in /home/username/Qt/6.9.1/gcc_64/lib
```

---

## OpenCV Installation

### Option A: Install from Ubuntu Repository (Simpler)

```bash
# For Ubuntu 20.04
sudo apt install -y \
    libopencv-dev \
    python3-opencv \
    opencv-data

# For Ubuntu 22.04/24.04
sudo apt install -y \
    libopencv-dev \
    python3-opencv \
    opencv-data \
    libopencv-contrib-dev

# Verify installation
pkg-config --modversion opencv4

# Expected output: 4.2.0 (Ubuntu 20.04) or 4.5.4+ (Ubuntu 22.04)
```

### Option B: Build OpenCV from Source (Latest Version)

```bash
# Install OpenCV build dependencies
sudo apt install -y \
    libjpeg-dev \
    libpng-dev \
    libtiff-dev \
    libavcodec-dev \
    libavformat-dev \
    libswscale-dev \
    libv4l-dev \
    libxvidcore-dev \
    libx264-dev \
    libgtk-3-dev \
    libatlas-base-dev \
    gfortran \
    python3-dev

# Create build directory
mkdir -p ~/opencv-build
cd ~/opencv-build

# Clone OpenCV and contrib repositories
git clone https://github.com/opencv/opencv.git
git clone https://github.com/opencv/opencv_contrib.git

# Checkout stable version (4.9.0 as of 2024)
cd opencv
git checkout 4.9.0
cd ../opencv_contrib
git checkout 4.9.0
cd ..

# Create build directory
mkdir opencv/build
cd opencv/build

# Configure OpenCV build
cmake -D CMAKE_BUILD_TYPE=RELEASE \
    -D CMAKE_INSTALL_PREFIX=/usr/local \
    -D OPENCV_EXTRA_MODULES_PATH=../../opencv_contrib/modules \
    -D WITH_QT=ON \
    -D WITH_OPENGL=ON \
    -D WITH_TBB=ON \
    -D BUILD_EXAMPLES=OFF \
    -D BUILD_TESTS=OFF \
    -D BUILD_PERF_TESTS=OFF \
    ..

# Build OpenCV
make -j$(nproc)

# Install OpenCV
sudo make install

# Update library cache
sudo ldconfig

# Verify installation
pkg-config --modversion opencv4
# Expected output: 4.9.0
```

---

## Do3Think SDK Setup

The Do3Think SDK is a proprietary camera SDK. Since it's primarily Windows-focused, Linux support requires special handling.

### Step 1: Check for Linux SDK Library

```bash
# The project includes a Linux library file
ls -la ~/Desktop/ComponentsForest/Do3ThinkCamera/SDK/libDVPCamera64.so

# If the file exists, copy it to system library path
sudo cp ~/Desktop/ComponentsForest/Do3ThinkCamera/SDK/libDVPCamera64.so /usr/local/lib/
sudo ldconfig
```

### Step 2: Set Up Dynamic Loading (If No Linux Library)

```bash
# The project supports dynamic loading if the SDK is not available
# This is handled automatically by the CMake configuration
# The USE_DYNAMIC_LOADING flag will be set if the library is not found
```

### Step 3: USB Permissions for Camera Access

```bash
# Create udev rule for Do3Think cameras
sudo tee /etc/udev/rules.d/99-do3think.rules << 'EOF'
# Do3Think Camera USB permissions
SUBSYSTEM=="usb", ATTRS{idVendor}=="04b4", MODE="0666", GROUP="plugdev"
SUBSYSTEM=="usb", ATTRS{idVendor}=="0547", MODE="0666", GROUP="plugdev"
EOF

# Add current user to plugdev group
sudo usermod -a -G plugdev $USER

# Reload udev rules
sudo udevadm control --reload-rules
sudo udevadm trigger

# Note: You may need to logout and login again for group changes to take effect
```

---

## Environment Configuration

### Step 1: Create Environment Setup Script

```bash
# Create environment configuration file
cat > ~/Desktop/ComponentsForest/setup_env.sh << 'EOF'
#!/bin/bash
# ComponentsForest Environment Setup

# Qt 6.9.1 Path (adjust based on your installation)
export QT_DIR="$HOME/Qt/6.9.1/gcc_64"
export PATH="$QT_DIR/bin:$PATH"
export LD_LIBRARY_PATH="$QT_DIR/lib:$LD_LIBRARY_PATH"

# CMake configuration
export CMAKE_PREFIX_PATH="$QT_DIR:$CMAKE_PREFIX_PATH"

# QML paths for QML viewer
export QML2_IMPORT_PATH="$PWD/viewers/qml_camera_viewer/qml:$QML2_IMPORT_PATH"
export QML_IMPORT_PATH="$PWD/viewers/qml_camera_viewer/qml:$QML_IMPORT_PATH"

# Qt Quick settings
export QT_QUICK_CONTROLS_STYLE=Material
export QSG_RENDER_LOOP=threaded

# Do3Think SDK library path
export LD_LIBRARY_PATH="$PWD/Do3ThinkCamera/SDK:$LD_LIBRARY_PATH"

# OpenCV (if built from source)
export PKG_CONFIG_PATH="/usr/local/lib/pkgconfig:$PKG_CONFIG_PATH"

# Logging
export QT_LOGGING_RULES="ComponentsForest.*=true"

echo "ComponentsForest environment configured."
echo "Qt directory: $QT_DIR"
echo "OpenCV version: $(pkg-config --modversion opencv4 2>/dev/null || echo 'Not found')"
EOF

chmod +x ~/Desktop/ComponentsForest/setup_env.sh
```

### Step 2: Load Environment

```bash
# Navigate to project directory
cd ~/Desktop/ComponentsForest

# Source the environment
source setup_env.sh
```

### Step 3: Make Environment Persistent (Optional)

```bash
# Add to .bashrc for automatic loading
echo "[ -f ~/Desktop/ComponentsForest/setup_env.sh ] && source ~/Desktop/ComponentsForest/setup_env.sh" >> ~/.bashrc
```

---

## Building ComponentsForest

### Building QWidget Version

```bash
# Navigate to project root
cd ~/Desktop/ComponentsForest

# Source environment
source setup_env.sh

# Use the quick build script
./quick_build_linux.sh $QT_DIR

# Or build manually
mkdir -p build_widget
cd build_widget
cmake .. \
    -DCMAKE_PREFIX_PATH="$QT_DIR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_VIEWERS=ON \
    -DBUILD_OPENCV_COMPONENTS=ON \
    -DBUILD_EXAMPLES=OFF

# Build with parallel jobs
make -j$(nproc)

# Expected output structure:
# build_widget/
# ├── viewers/
# │   └── do3think_camera_viewer/
# │       └── Do3ThinkCameraViewer (executable)
# ├── libComponentsForestCore.a
# └── libDo3ThinkCameraComponent.a
```

### Building QML Version

```bash
# Navigate to project root
cd ~/Desktop/ComponentsForest

# Source environment
source setup_env.sh

# Use the QML build script
./build_qml_viewer.sh --qt-dir $QT_DIR

# Or build manually
mkdir -p build_qml
cd build_qml
cmake ../viewers/qml_camera_viewer \
    -DCMAKE_PREFIX_PATH="$QT_DIR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_STANDALONE=ON \
    -DBUILD_WITH_OPENCV=ON

# Build
make -j$(nproc)

# Run the QML viewer
./run_qml_viewer.sh
```

### Building Everything (Recommended)

```bash
# Complete build script
cat > ~/Desktop/ComponentsForest/build_all.sh << 'EOF'
#!/bin/bash
set -e

echo "Building ComponentsForest - Complete Build"
echo "=========================================="

# Source environment
source setup_env.sh

# Clean previous builds
rm -rf build_complete

# Create build directory
mkdir build_complete
cd build_complete

# Configure with all features
cmake .. \
    -DCMAKE_PREFIX_PATH="$QT_DIR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_VIEWERS=ON \
    -DBUILD_QML_VIEWER=ON \
    -DBUILD_OPENCV_COMPONENTS=ON \
    -DBUILD_EXAMPLES=ON \
    -DBUILD_TESTS=OFF

# Build
echo "Building with $(nproc) parallel jobs..."
make -j$(nproc)

echo "Build completed successfully!"
echo ""
echo "Executables:"
echo "  QWidget Viewer: build_complete/viewers/do3think_camera_viewer/Do3ThinkCameraViewer"
echo "  QML Viewer: build_complete/viewers/qml_camera_viewer/QMLCameraViewer"

cd ..
EOF

chmod +x ~/Desktop/ComponentsForest/build_all.sh
./build_all.sh
```

---

## Verification Steps

### Step 1: Verify Build Outputs

```bash
# Check if executables exist
ls -la build_complete/viewers/do3think_camera_viewer/Do3ThinkCameraViewer
ls -la build_complete/viewers/qml_camera_viewer/QMLCameraViewer

# Check library files
ls -la build_complete/*.a
```

### Step 2: Run Library Dependencies Check

```bash
# Check QWidget viewer dependencies
ldd build_complete/viewers/do3think_camera_viewer/Do3ThinkCameraViewer | grep -E "(Qt|opencv|DVP)"

# Check QML viewer dependencies
ldd build_complete/viewers/qml_camera_viewer/QMLCameraViewer | grep -E "(Qt|opencv|DVP)"

# Expected: Should show Qt6 libraries and optionally OpenCV/DVPCamera
```

### Step 3: Test QWidget Viewer

```bash
# Run QWidget viewer
cd build_complete/viewers/do3think_camera_viewer
./Do3ThinkCameraViewer

# Expected behavior:
# - Application window opens
# - Shows "Do3Think Camera Viewer" title
# - Camera discovery button is visible
# - No crash or missing library errors
```

### Step 4: Test QML Viewer

```bash
# Run QML viewer
cd build_complete/viewers/qml_camera_viewer
./run_qml_viewer.sh

# Expected behavior:
# - Modern QML interface opens
# - Material Design theme is applied
# - Camera controls are visible
# - Console shows "QML Camera Viewer started"
```

### Step 5: Camera Detection Test

```bash
# Create camera test script
cat > test_camera.cpp << 'EOF'
#include <iostream>
#include <dlfcn.h>

int main() {
    void* handle = dlopen("libDVPCamera64.so", RTLD_LAZY);
    if (handle) {
        std::cout << "Do3Think SDK loaded successfully" << std::endl;
        dlclose(handle);
        return 0;
    } else {
        std::cout << "Do3Think SDK not found (will use dynamic loading)" << std::endl;
        return 1;
    }
}
EOF

# Compile and run test
g++ test_camera.cpp -o test_camera -ldl
./test_camera
```

---

## Troubleshooting

### Common Issues and Solutions

#### Issue 1: Qt Not Found
```bash
# Error: Could not find Qt6Config.cmake
# Solution: Ensure Qt path is correct
export CMAKE_PREFIX_PATH="$HOME/Qt/6.9.1/gcc_64:$CMAKE_PREFIX_PATH"
```

#### Issue 2: OpenCV Linking Errors
```bash
# Error: undefined reference to cv::...
# Solution: Install OpenCV or disable OpenCV components
cmake .. -DBUILD_OPENCV_COMPONENTS=OFF
```

#### Issue 3: Do3Think SDK Not Loading
```bash
# Error: Cannot load DVPCamera64
# Solution: Copy SDK library to system path
sudo cp Do3ThinkCamera/SDK/libDVPCamera64.so /usr/local/lib/
sudo ldconfig
```

#### Issue 4: QML Module Not Found
```bash
# Error: QML module not found
# Solution: Set QML import paths
export QML2_IMPORT_PATH="$PWD/viewers/qml_camera_viewer/qml:$QML2_IMPORT_PATH"
```

#### Issue 5: Permission Denied for USB Camera
```bash
# Error: Cannot access USB device
# Solution: Add user to plugdev group and setup udev rules
sudo usermod -a -G plugdev $USER
# Then logout and login again
```

#### Issue 6: Missing XCB Libraries
```bash
# Error: qt.qpa.plugin: Could not load the Qt platform plugin "xcb"
# Solution: Install missing libraries
sudo apt install -y libxcb-xinerama0 libxcb-cursor0
```

### Debug Build for Troubleshooting

```bash
# Build with debug symbols
mkdir build_debug
cd build_debug
cmake .. \
    -DCMAKE_PREFIX_PATH="$QT_DIR" \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_VERBOSE_MAKEFILE=ON

make VERBOSE=1

# Run with debugging output
export QT_DEBUG_PLUGINS=1
export QT_LOGGING_RULES="*.debug=true"
./viewers/do3think_camera_viewer/Do3ThinkCameraViewer
```

---

## Optional Development Tools

### Install Qt Creator IDE

```bash
# Via Qt Installer (recommended)
# Select Qt Creator during Qt installation

# Or install separately
wget https://download.qt.io/qtcreator/qtcreator-14.0.0-linux-x86_64.run
chmod +x qtcreator-14.0.0-linux-x86_64.run
./qtcreator-14.0.0-linux-x86_64.run
```

### Install Development Utilities

```bash
# Code analysis and debugging tools
sudo apt install -y \
    gdb \
    valgrind \
    clang-format \
    clang-tidy \
    cppcheck \
    doxygen \
    graphviz

# Performance profiling
sudo apt install -y \
    linux-tools-common \
    linux-tools-generic \
    linux-tools-$(uname -r)

# Git GUI tools
sudo apt install -y \
    gitk \
    git-gui \
    meld
```

### VS Code Setup (Alternative IDE)

```bash
# Install VS Code
wget -qO- https://packages.microsoft.com/keys/microsoft.asc | gpg --dearmor > packages.microsoft.gpg
sudo install -o root -g root -m 644 packages.microsoft.gpg /etc/apt/trusted.gpg.d/
sudo sh -c 'echo "deb [arch=amd64,arm64,armhf signed-by=/etc/apt/trusted.gpg.d/packages.microsoft.gpg] https://packages.microsoft.com/repos/code stable main" > /etc/apt/sources.list.d/vscode.list'
sudo apt update
sudo apt install -y code

# Install C++ extensions
code --install-extension ms-vscode.cpptools
code --install-extension ms-vscode.cmake-tools
code --install-extension tonka3000.qtvsctools
```

### Configure Git for Development

```bash
# Set up Git identity
git config --global user.name "Your Name"
git config --global user.email "your.email@example.com"

# Configure Git for better diff output
git config --global core.pager "less -FRX"
git config --global color.ui true
git config --global diff.tool meld
git config --global merge.tool meld
```

---

## Performance Optimization Tips

### Compiler Optimizations

```bash
# For maximum performance builds
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_FLAGS="-O3 -march=native -mtune=native -flto" \
    -DCMAKE_EXE_LINKER_FLAGS="-flto"
```

### System Tuning

```bash
# Increase file descriptor limits
echo "* soft nofile 65536" | sudo tee -a /etc/security/limits.conf
echo "* hard nofile 65536" | sudo tee -a /etc/security/limits.conf

# Disable CPU frequency scaling for testing
sudo cpupower frequency-set -g performance
```

---

## Quick Reference Commands

```bash
# Environment setup
source ~/Desktop/ComponentsForest/setup_env.sh

# Quick build QWidget version
./quick_build_linux.sh $QT_DIR

# Quick build QML version
./build_qml_viewer.sh --qt-dir $QT_DIR

# Run QWidget viewer
./build_widget/viewers/do3think_camera_viewer/Do3ThinkCameraViewer

# Run QML viewer
./build_qml/run_qml_viewer.sh

# Clean all builds
rm -rf build_* CMakeCache.txt CMakeFiles/

# Check dependencies
ldd ./build_widget/viewers/do3think_camera_viewer/Do3ThinkCameraViewer

# Monitor build output
make -j$(nproc) 2>&1 | tee build.log
```

---

## Support and Resources

### Project Documentation
- Main documentation: See project README files
- Architecture guide: `ARCHITECTURE.md`
- Component development: `07_Standard_Components_Development.md`

### External Resources
- Qt Documentation: https://doc.qt.io/qt-6/
- OpenCV Documentation: https://docs.opencv.org/4.x/
- CMake Documentation: https://cmake.org/documentation/
- Ubuntu Forums: https://ubuntuforums.org/

### Getting Help
1. Check the troubleshooting section above
2. Review build logs: `make VERBOSE=1 2>&1 | tee build.log`
3. Enable Qt debugging: `export QT_LOGGING_RULES="*.debug=true"`
4. Use `gdb` for debugging crashes
5. Check system logs: `journalctl -xe`

---

## Conclusion

This guide provides comprehensive instructions for setting up ComponentsForest on Ubuntu systems. Following these steps will give you:

1. A complete Qt 6.9.1 development environment
2. OpenCV for image processing capabilities
3. Do3Think camera SDK integration
4. Both QWidget and QML viewer applications
5. Full development toolchain for further customization

Remember to:
- Always source the environment setup script before building
- Check that all paths match your actual installation directories
- Run verification steps after each major installation
- Keep build logs for troubleshooting

For production deployments, consider creating Docker containers or snap packages to simplify distribution.

---

*Document Version: 1.0*  
*Last Updated: 2025-08-11*  
*ComponentsForest Project*