#!/bin/bash

# Build script for QML Camera Viewer on Linux/WSL
# Supports both standalone and integrated builds

set -e  # Exit on error

echo "========================================"
echo "Building ComponentsForest QML Viewer"
echo "========================================"

# Parse command line arguments
BUILD_TYPE=${1:-Release}
QT_VERSION=${2:-6.9.1}
BUILD_DIR=${3:-build_qml}

# Detect if we're in WSL
if grep -q Microsoft /proc/version 2>/dev/null; then
    IS_WSL=true
    echo "Detected WSL environment"
else
    IS_WSL=false
fi

# Set Qt path (adjust as needed)
if [ -z "$QT_DIR" ]; then
    # Try common Qt installation paths
    QT_SEARCH_PATHS=(
        "/opt/Qt/${QT_VERSION}/gcc_64"
        "$HOME/Qt/${QT_VERSION}/gcc_64"
        "/usr/local/Qt/${QT_VERSION}/gcc_64"
        "/usr/lib/qt6"
        "/usr"
    )
    
    for path in "${QT_SEARCH_PATHS[@]}"; do
        if [ -d "$path" ] && [ -f "$path/bin/qmake" -o -f "$path/bin/qmake6" ]; then
            QT_DIR="$path"
            echo "Found Qt at: $QT_DIR"
            break
        fi
    done
    
    if [ -z "$QT_DIR" ]; then
        echo "Error: Qt ${QT_VERSION} not found. Please set QT_DIR environment variable."
        echo "Example: export QT_DIR=/opt/Qt/${QT_VERSION}/gcc_64"
        exit 1
    fi
fi

# Add Qt to PATH
export PATH=$QT_DIR/bin:$PATH

# Check for required tools
command -v cmake >/dev/null 2>&1 || { echo "Error: cmake is not installed. Please install cmake."; exit 1; }
command -v make >/dev/null 2>&1 || { echo "Error: make is not installed. Please install build-essential."; exit 1; }

# Get the script directory (where the viewer CMakeLists.txt is)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

echo "Project root: $PROJECT_ROOT"
echo "Qt directory: $QT_DIR"
echo "Build type: $BUILD_TYPE"
echo "Build directory: $BUILD_DIR"

# Clean previous build if requested
if [ "$1" == "clean" ]; then
    echo "Cleaning previous build..."
    rm -rf "$BUILD_DIR"
    shift
fi

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Detect if we should build standalone or as part of main project
if [ -f "$PROJECT_ROOT/CMakeLists.txt" ]; then
    echo "Building as part of ComponentsForest project"
    CMAKE_SOURCE_DIR="$PROJECT_ROOT"
    CMAKE_OPTIONS="-DBUILD_VIEWERS=ON -DBUILD_OPENCV_COMPONENTS=OFF"
else
    echo "Building standalone QML viewer"
    CMAKE_SOURCE_DIR="$SCRIPT_DIR"
    CMAKE_OPTIONS=""
fi

# Configure with CMake
echo ""
echo "Configuring project..."
cmake "$CMAKE_SOURCE_DIR" \
    -DCMAKE_PREFIX_PATH="$QT_DIR" \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    $CMAKE_OPTIONS

if [ $? -ne 0 ]; then
    echo "Configuration failed!"
    exit 1
fi

# Build the project
echo ""
echo "Building project..."
if [ -f "Makefile" ]; then
    make -j$(nproc) QMLCameraViewer
elif [ -f "build.ninja" ]; then
    ninja QMLCameraViewer
else
    cmake --build . --target QMLCameraViewer --parallel
fi

if [ $? -ne 0 ]; then
    echo "Build failed!"
    exit 1
fi

# Find the built executable
if [ -f "bin/QMLCameraViewer" ]; then
    EXECUTABLE="bin/QMLCameraViewer"
elif [ -f "viewers/qml_camera_viewer/QMLCameraViewer" ]; then
    EXECUTABLE="viewers/qml_camera_viewer/QMLCameraViewer"
elif [ -f "QMLCameraViewer" ]; then
    EXECUTABLE="QMLCameraViewer"
else
    echo "Warning: Could not find built executable"
    EXECUTABLE="QMLCameraViewer"
fi

# Create run script
cat > run_qml_viewer.sh << EOF
#!/bin/bash
# Run script for QML Camera Viewer

# Set Qt plugin path
export QT_PLUGIN_PATH=$QT_DIR/plugins:\$QT_PLUGIN_PATH

# Set QML import paths
export QML2_IMPORT_PATH=$SCRIPT_DIR/qml:$QT_DIR/qml:\$QML2_IMPORT_PATH
export QML_IMPORT_PATH=$SCRIPT_DIR/qml:$QT_DIR/qml:\$QML_IMPORT_PATH

# Set Qt Quick Controls style
export QT_QUICK_CONTROLS_STYLE=Material

# Set library paths for Do3Think SDK
export LD_LIBRARY_PATH=$PROJECT_ROOT/Do3ThinkCamera/SDK:\$LD_LIBRARY_PATH

# Enable Qt logging (optional, comment out for production)
export QT_LOGGING_RULES="*.debug=false;qt.qml.debug=true"

# Set platform if in WSL (optional)
if grep -q Microsoft /proc/version 2>/dev/null; then
    # WSL-specific settings
    export QT_QPA_PLATFORM=xcb
    export DISPLAY=:0
fi

# Run the application
exec ./$EXECUTABLE "\$@"
EOF

chmod +x run_qml_viewer.sh

# Create debug script
cat > debug_qml_viewer.sh << EOF
#!/bin/bash
# Debug script for QML Camera Viewer

# Enable all Qt logging
export QT_LOGGING_RULES="*.debug=true"
export QML_IMPORT_TRACE=1

# Run with gdb
gdb -ex run --args ./$EXECUTABLE
EOF

chmod +x debug_qml_viewer.sh

# Copy Do3Think SDK library if needed
if [ -f "$PROJECT_ROOT/Do3ThinkCamera/SDK/libDVPCamera64.so" ]; then
    echo ""
    echo "Copying Do3Think SDK library..."
    cp "$PROJECT_ROOT/Do3ThinkCamera/SDK/libDVPCamera64.so" . 2>/dev/null || true
fi

# Success message
echo ""
echo "========================================"
echo "Build completed successfully!"
echo "========================================"
echo "Executable: $BUILD_DIR/$EXECUTABLE"
echo ""
echo "To run the application:"
echo "  cd $BUILD_DIR"
echo "  ./run_qml_viewer.sh"
echo ""
echo "To debug:"
echo "  cd $BUILD_DIR"
echo "  ./debug_qml_viewer.sh"
echo ""
echo "To rebuild from clean:"
echo "  $0 clean $BUILD_TYPE $QT_VERSION"
echo "========================================"

# Return to original directory
cd - > /dev/null