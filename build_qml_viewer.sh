#!/bin/bash

# ComponentsForest QML Camera Viewer Build Script
# Supports both standalone and integrated builds
# Compatible with Linux and WSL environments

set -e

echo "========================================"
echo "ComponentsForest QML Camera Viewer Build"
echo "========================================"
echo

# Get the script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Default configuration
BUILD_TYPE="Release"
CLEAN_BUILD=1
BUILD_STANDALONE="ON"
BUILD_OPENCV="OFF"
JOBS=$(nproc 2>/dev/null || echo 4)
QT_DIR=""

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        --no-clean)
            CLEAN_BUILD=0
            shift
            ;;
        --integrated)
            BUILD_STANDALONE="OFF"
            shift
            ;;
        --with-opencv)
            BUILD_OPENCV="ON"
            shift
            ;;
        --qt-dir)
            QT_DIR="$2"
            shift 2
            ;;
        -j|--jobs)
            JOBS="$2"
            shift 2
            ;;
        --help)
            echo "Usage: $0 [options]"
            echo "Options:"
            echo "  --debug           Build in Debug mode"
            echo "  --no-clean        Don't clean previous build"
            echo "  --integrated      Build as part of ComponentsForest"
            echo "  --with-opencv     Enable OpenCV support"
            echo "  --qt-dir PATH     Specify Qt installation directory"
            echo "  -j, --jobs N      Number of parallel build jobs (default: $(nproc))"
            echo "  --help            Show this help"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

# Auto-detect Qt if not specified
if [ -z "$QT_DIR" ]; then
    # Try to find Qt using common paths
    QT_SEARCH_PATHS=(
        "/usr/lib/qt6"
        "/usr/local/Qt-6.9.1"
        "/opt/Qt/6.9.1/gcc_64"
        "/opt/Qt/6.9.0/gcc_64"
        "$HOME/Qt/6.9.1/gcc_64"
        "$HOME/Qt/6.9.0/gcc_64"
        "/usr"
    )
    
    for path in "${QT_SEARCH_PATHS[@]}"; do
        if [ -f "$path/lib/cmake/Qt6/Qt6Config.cmake" ] || [ -f "$path/cmake/Qt6/Qt6Config.cmake" ]; then
            QT_DIR="$path"
            echo "Found Qt at: $QT_DIR"
            break
        fi
    done
    
    if [ -z "$QT_DIR" ]; then
        # Try using system Qt
        if command -v qmake6 &> /dev/null; then
            QT_DIR="$(qmake6 -query QT_INSTALL_PREFIX)"
            echo "Using system Qt from: $QT_DIR"
        elif command -v qmake &> /dev/null; then
            QT_VERSION=$(qmake -query QT_VERSION)
            if [[ "$QT_VERSION" == 6.* ]]; then
                QT_DIR="$(qmake -query QT_INSTALL_PREFIX)"
                echo "Using system Qt from: $QT_DIR"
            fi
        fi
    fi
fi

# Verify Qt installation
if [ -z "$QT_DIR" ]; then
    echo "Error: Qt 6.9+ installation not found."
    echo "Please install Qt or specify the path with --qt-dir"
    exit 1
fi

# Set build directory
BUILD_DIR="build_qml_${BUILD_TYPE,,}"

# Clean previous build if requested
if [ $CLEAN_BUILD -eq 1 ]; then
    if [ -d "$BUILD_DIR" ]; then
        echo "Cleaning previous build..."
        rm -rf "$BUILD_DIR"
    fi
fi

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure with CMake
echo
echo "Configuring project with:"
echo "  Build Type: $BUILD_TYPE"
echo "  Standalone: $BUILD_STANDALONE"
echo "  OpenCV: $BUILD_OPENCV"
echo "  Qt Directory: $QT_DIR"
echo "  Parallel Jobs: $JOBS"
echo

cmake ../viewers/qml_camera_viewer \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DCMAKE_PREFIX_PATH="$QT_DIR" \
    -DBUILD_STANDALONE="$BUILD_STANDALONE" \
    -DBUILD_WITH_OPENCV="$BUILD_OPENCV" \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

if [ $? -ne 0 ]; then
    echo "Configuration failed!"
    exit 1
fi

# Build the project
echo
echo "Building project with $JOBS parallel jobs..."
echo

cmake --build . --parallel "$JOBS"

if [ $? -ne 0 ]; then
    echo "Build failed!"
    exit 1
fi

# Copy Do3Think SDK library if available
if [ -f "../Do3ThinkCamera/SDK/libDVPCamera64.so" ]; then
    echo
    echo "Copying Do3Think SDK library..."
    
    OUTPUT_DIR="bin"
    [ -d "$OUTPUT_DIR" ] || OUTPUT_DIR="."
    
    cp -f "../Do3ThinkCamera/SDK/libDVPCamera64.so" "$OUTPUT_DIR/"
    
    # Set RPATH for the executable
    if command -v patchelf &> /dev/null && [ -f "$OUTPUT_DIR/QMLCameraViewer" ]; then
        patchelf --set-rpath '\$ORIGIN' "$OUTPUT_DIR/QMLCameraViewer"
    fi
else
    echo "Info: Do3Think SDK not found, will use dynamic loading if available"
fi

# Create run script
RUN_SCRIPT="run_qml_viewer.sh"
cat > "$RUN_SCRIPT" << 'EOF'
#!/bin/bash

# Set environment for QML
export QML2_IMPORT_PATH="../viewers/qml_camera_viewer/qml:$QML2_IMPORT_PATH"
export QT_QUICK_CONTROLS_STYLE=Material
export QSG_RENDER_LOOP=threaded
export QT_LOGGING_RULES="ComponentsForest.*=true"

# Set library path for Do3Think SDK
export LD_LIBRARY_PATH=".:$LD_LIBRARY_PATH"

# Run the application
if [ -f "bin/QMLCameraViewer" ]; then
    exec bin/QMLCameraViewer "$@"
elif [ -f "QMLCameraViewer" ]; then
    exec ./QMLCameraViewer "$@"
else
    echo "Error: QMLCameraViewer executable not found"
    exit 1
fi
EOF

chmod +x "$RUN_SCRIPT"

echo
echo "========================================"
echo "Build completed successfully!"
echo

# Find the executable
EXE_PATH=""
for dir in bin . ; do
    if [ -f "$dir/QMLCameraViewer" ]; then
        EXE_PATH="$BUILD_DIR/$dir/QMLCameraViewer"
        echo "Executable: $EXE_PATH"
        break
    fi
done

if [ -z "$EXE_PATH" ]; then
    echo "Warning: Could not locate the built executable"
fi

echo
echo "To run the application:"
echo "  cd $BUILD_DIR"
echo "  ./$RUN_SCRIPT"
echo
echo "Build options used:"
echo "  Build Type: $BUILD_TYPE"
echo "  Standalone: $BUILD_STANDALONE"
echo "  OpenCV: $BUILD_OPENCV"
echo "========================================"

cd ..

# Offer to run the application
if [ -n "$EXE_PATH" ]; then
    echo
    read -p "Run the application now? (y/N): " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        cd "$BUILD_DIR"
        ./$RUN_SCRIPT
    fi
fi