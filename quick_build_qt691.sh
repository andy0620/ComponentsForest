#!/bin/bash

# Quick Build Script for Qt 6.9.1 on Linux/WSL2
# ==============================================

echo "=========================================="
echo "Do3Think Camera Viewer - Qt 6.9.1 Build"
echo "=========================================="
echo

# Function to find Qt
find_qt() {
    # Check if Qt path is provided
    if [ ! -z "$1" ]; then
        if [ -f "$1/bin/qmake" ]; then
            QT_PATH="$1"
            return 0
        fi
    fi
    
    # Try common paths
    QT_PATHS=(
        "$HOME/Qt/6.9.1/gcc_64"
        "/opt/Qt/6.9.1/gcc_64"
        "/usr/local/Qt/6.9.1/gcc_64"
        "/mnt/c/Qt/6.9.1/gcc_64"
        "/mnt/c/Qt/6.9.1/mingw_64"
        "/mnt/c/Qt/6.9.1/msvc2019_64"
    )
    
    for path in "${QT_PATHS[@]}"; do
        if [ -f "$path/bin/qmake" ]; then
            QT_PATH="$path"
            return 0
        fi
    done
    
    return 1
}

# Find Qt installation
if find_qt "$1"; then
    echo "Found Qt 6.9.1 at: $QT_PATH"
else
    echo "ERROR: Qt 6.9.1 not found!"
    echo
    echo "Please provide Qt path as argument:"
    echo "  ./quick_build_qt691.sh /path/to/Qt/6.9.1/gcc_64"
    echo
    echo "Common installation paths:"
    echo "  Linux: ~/Qt/6.9.1/gcc_64"
    echo "  WSL2:  /mnt/c/Qt/6.9.1/gcc_64"
    exit 1
fi

# Check for CMake
if ! command -v cmake &> /dev/null; then
    echo "ERROR: CMake is not installed"
    echo "Install with: sudo apt-get install cmake"
    exit 1
fi

# Clean and create build directory
echo
echo "Creating build directory..."
rm -rf build_viewer
mkdir build_viewer
cd build_viewer

# Configure with CMake
echo "Configuring with CMake..."
cmake .. \
    -DCMAKE_PREFIX_PATH="$QT_PATH" \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_VIEWERS=ON

if [ $? -ne 0 ]; then
    echo
    echo "ERROR: CMake configuration failed"
    exit 1
fi

# Build the project
echo
echo "Building the project..."
make -j$(nproc)

if [ $? -ne 0 ]; then
    echo
    echo "ERROR: Build failed"
    exit 1
fi

echo
echo "=========================================="
echo "Build completed successfully!"
echo "=========================================="
echo
echo "Executable location:"
echo "  build_viewer/viewers/do3think_camera_viewer/Do3ThinkCameraViewer"
echo
echo "To run the application:"
echo "  cd build_viewer/viewers/do3think_camera_viewer"
echo "  ./Do3ThinkCameraViewer"
echo

# Create run script
cat > run_viewer.sh << 'EOF'
#!/bin/bash
cd viewers/do3think_camera_viewer
export LD_LIBRARY_PATH=../../:$LD_LIBRARY_PATH
./Do3ThinkCameraViewer
EOF

chmod +x run_viewer.sh
echo "Created run_viewer.sh script for easy execution"

cd ..