#!/bin/bash

#############################################################################
# Do3Think Camera Viewer Build Script for Qt 6.9.1
# Author: ComponentsForest Build System
# Date: 2025-08-09
#############################################################################

set -e  # Exit on error

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}Do3Think Camera Viewer Build Script${NC}"
echo -e "${BLUE}Qt 6.9.1 Configuration${NC}"
echo -e "${BLUE}========================================${NC}"

# Function to find Qt installation
find_qt() {
    echo -e "${YELLOW}Searching for Qt 6.9.1 installation...${NC}"
    
    # Common Qt installation paths
    QT_SEARCH_PATHS=(
        "$HOME/Qt/6.9.1/gcc_64"
        "$HOME/Qt6/6.9.1/gcc_64"
        "/opt/Qt/6.9.1/gcc_64"
        "/usr/local/Qt/6.9.1/gcc_64"
        "/usr/lib/qt6"
        "/usr/local/lib/qt6"
        "$HOME/Qt/6.9.1/mingw_64"  # For MinGW on Windows
        "/mnt/c/Qt/6.9.1/gcc_64"    # WSL2 accessing Windows Qt
        "/mnt/c/Qt/6.9.1/msvc2019_64"  # MSVC version
    )
    
    # Check if Qt path is provided as argument
    if [ ! -z "$1" ]; then
        QT_SEARCH_PATHS=("$1" "${QT_SEARCH_PATHS[@]}")
    fi
    
    # Search for Qt installation
    for path in "${QT_SEARCH_PATHS[@]}"; do
        if [ -f "$path/bin/qmake" ]; then
            QT_PATH="$path"
            echo -e "${GREEN}Found Qt 6.9.1 at: $QT_PATH${NC}"
            return 0
        fi
    done
    
    # If not found, ask user
    echo -e "${YELLOW}Qt 6.9.1 not found in standard locations.${NC}"
    echo -e "${YELLOW}Please enter the full path to your Qt 6.9.1 installation:${NC}"
    echo -e "${YELLOW}(e.g., /home/user/Qt/6.9.1/gcc_64)${NC}"
    read -r user_path
    
    if [ -f "$user_path/bin/qmake" ]; then
        QT_PATH="$user_path"
        echo -e "${GREEN}Using Qt installation at: $QT_PATH${NC}"
        return 0
    else
        echo -e "${RED}Error: Qt not found at $user_path${NC}"
        return 1
    fi
}

# Function to check dependencies
check_dependencies() {
    echo -e "${YELLOW}Checking build dependencies...${NC}"
    
    local missing_deps=()
    
    # Check for CMake
    if ! command -v cmake &> /dev/null; then
        missing_deps+=("cmake")
    else
        cmake_version=$(cmake --version | head -n1)
        echo -e "${GREEN}✓ CMake found: $cmake_version${NC}"
    fi
    
    # Check for Make
    if ! command -v make &> /dev/null; then
        missing_deps+=("make")
    else
        echo -e "${GREEN}✓ Make found${NC}"
    fi
    
    # Check for GCC/G++
    if ! command -v g++ &> /dev/null; then
        missing_deps+=("g++")
    else
        gcc_version=$(g++ --version | head -n1)
        echo -e "${GREEN}✓ G++ found: $gcc_version${NC}"
    fi
    
    # Check for OpenCV (optional but recommended)
    if pkg-config --exists opencv4 2>/dev/null; then
        opencv_version=$(pkg-config --modversion opencv4)
        echo -e "${GREEN}✓ OpenCV found: $opencv_version${NC}"
    else
        echo -e "${YELLOW}⚠ OpenCV not found (optional)${NC}"
    fi
    
    if [ ${#missing_deps[@]} -ne 0 ]; then
        echo -e "${RED}Missing dependencies: ${missing_deps[*]}${NC}"
        echo -e "${YELLOW}Install with: sudo apt-get install ${missing_deps[*]}${NC}"
        return 1
    fi
    
    return 0
}

# Function to build core components
build_core_components() {
    echo -e "${YELLOW}Building ComponentsForest Core library...${NC}"
    
    # Create build directory
    BUILD_DIR="build_core"
    rm -rf "$BUILD_DIR"
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    
    # Configure with CMake
    cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_PREFIX_PATH="$QT_PATH" \
        -DQt6_DIR="$QT_PATH/lib/cmake/Qt6" \
        -DBUILD_SHARED_LIBS=ON \
        -DBUILD_EXAMPLES=OFF
    
    # Build core library
    make -j$(nproc) ComponentsForestCore
    
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}✓ ComponentsForest Core built successfully${NC}"
    else
        echo -e "${RED}✗ Failed to build ComponentsForest Core${NC}"
        return 1
    fi
    
    cd ..
    return 0
}

# Function to build Do3Think component
build_do3think_component() {
    echo -e "${YELLOW}Building Do3Think Camera Component...${NC}"
    
    cd build_core
    
    # Build Do3Think component
    make -j$(nproc) Do3ThinkCameraComponent
    
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}✓ Do3Think Camera Component built successfully${NC}"
    else
        echo -e "${RED}✗ Failed to build Do3Think Camera Component${NC}"
        return 1
    fi
    
    cd ..
    return 0
}

# Function to build viewer application
build_viewer() {
    echo -e "${YELLOW}Building Do3Think Camera Viewer application...${NC}"
    
    # Create viewer build directory
    VIEWER_BUILD_DIR="build_viewer"
    rm -rf "$VIEWER_BUILD_DIR"
    mkdir -p "$VIEWER_BUILD_DIR"
    cd "$VIEWER_BUILD_DIR"
    
    # Configure viewer with CMake
    cmake ../viewers/do3think_camera_viewer \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_PREFIX_PATH="$QT_PATH" \
        -DQt6_DIR="$QT_PATH/lib/cmake/Qt6" \
        -DComponentsForestCore_DIR="../build_core" \
        -DDo3ThinkCameraComponent_DIR="../build_core"
    
    # Build viewer
    make -j$(nproc)
    
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}✓ Do3Think Camera Viewer built successfully${NC}"
        echo -e "${GREEN}Executable location: $(pwd)/Do3ThinkCameraViewer${NC}"
    else
        echo -e "${RED}✗ Failed to build Do3Think Camera Viewer${NC}"
        return 1
    fi
    
    cd ..
    return 0
}

# Function to copy runtime dependencies
copy_runtime_deps() {
    echo -e "${YELLOW}Copying runtime dependencies...${NC}"
    
    VIEWER_BIN_DIR="build_viewer"
    
    # Copy core libraries
    if [ -f "build_core/libComponentsForestCore.so" ]; then
        cp build_core/libComponentsForestCore.so "$VIEWER_BIN_DIR/"
        echo -e "${GREEN}✓ Copied ComponentsForestCore library${NC}"
    fi
    
    if [ -f "build_core/libDo3ThinkCameraComponent.so" ]; then
        cp build_core/libDo3ThinkCameraComponent.so "$VIEWER_BIN_DIR/"
        echo -e "${GREEN}✓ Copied Do3ThinkCameraComponent library${NC}"
    fi
    
    # Copy DVPCamera library if exists
    if [ -f "Do3ThinkCamera/SDK/libDVPCamera64.so" ]; then
        cp Do3ThinkCamera/SDK/libDVPCamera64.so "$VIEWER_BIN_DIR/"
        echo -e "${GREEN}✓ Copied DVPCamera SDK library${NC}"
    elif [ -f "Do3ThinkCamera/SDK/DVPCamera64.dll" ]; then
        cp Do3ThinkCamera/SDK/DVPCamera64.dll "$VIEWER_BIN_DIR/"
        echo -e "${GREEN}✓ Copied DVPCamera SDK library (Windows DLL)${NC}"
    else
        echo -e "${YELLOW}⚠ DVPCamera SDK library not found (camera may not work)${NC}"
    fi
    
    # Create run script
    cat > "$VIEWER_BIN_DIR/run_viewer.sh" << 'EOF'
#!/bin/bash
# Do3Think Camera Viewer run script
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
export LD_LIBRARY_PATH="$SCRIPT_DIR:$LD_LIBRARY_PATH"

# Set Qt plugin path if needed
if [ ! -z "$QT_PATH" ]; then
    export QT_PLUGIN_PATH="$QT_PATH/plugins"
fi

# Run the viewer
"$SCRIPT_DIR/Do3ThinkCameraViewer" "$@"
EOF
    
    chmod +x "$VIEWER_BIN_DIR/run_viewer.sh"
    echo -e "${GREEN}✓ Created run script: $VIEWER_BIN_DIR/run_viewer.sh${NC}"
}

# Function to create development environment setup
create_dev_setup() {
    echo -e "${YELLOW}Creating development environment setup...${NC}"
    
    cat > "setup_qt_env.sh" << EOF
#!/bin/bash
# Qt 6.9.1 Environment Setup
export QT_PATH="$QT_PATH"
export PATH="\$QT_PATH/bin:\$PATH"
export LD_LIBRARY_PATH="\$QT_PATH/lib:\$LD_LIBRARY_PATH"
export PKG_CONFIG_PATH="\$QT_PATH/lib/pkgconfig:\$PKG_CONFIG_PATH"
export CMAKE_PREFIX_PATH="\$QT_PATH:\$CMAKE_PREFIX_PATH"

echo "Qt 6.9.1 environment configured"
echo "Qt Path: \$QT_PATH"
echo "You can now build Qt applications"
EOF
    
    chmod +x setup_qt_env.sh
    echo -e "${GREEN}✓ Created environment setup script: setup_qt_env.sh${NC}"
}

# Main build process
main() {
    echo -e "${BLUE}Starting build process...${NC}"
    echo ""
    
    # Parse command line arguments
    QT_CUSTOM_PATH=""
    CLEAN_BUILD=false
    
    while [[ $# -gt 0 ]]; do
        case $1 in
            --qt-path)
                QT_CUSTOM_PATH="$2"
                shift 2
                ;;
            --clean)
                CLEAN_BUILD=true
                shift
                ;;
            --help)
                echo "Usage: $0 [OPTIONS]"
                echo "Options:"
                echo "  --qt-path PATH    Specify Qt 6.9.1 installation path"
                echo "  --clean           Clean build (remove existing build directories)"
                echo "  --help            Show this help message"
                exit 0
                ;;
            *)
                echo -e "${RED}Unknown option: $1${NC}"
                exit 1
                ;;
        esac
    done
    
    # Clean existing builds if requested
    if [ "$CLEAN_BUILD" = true ]; then
        echo -e "${YELLOW}Cleaning existing build directories...${NC}"
        rm -rf build_core build_viewer build
        echo -e "${GREEN}✓ Build directories cleaned${NC}"
    fi
    
    # Step 1: Find Qt
    if ! find_qt "$QT_CUSTOM_PATH"; then
        echo -e "${RED}Failed to find Qt 6.9.1 installation${NC}"
        exit 1
    fi
    
    # Step 2: Check dependencies
    if ! check_dependencies; then
        echo -e "${RED}Missing required dependencies${NC}"
        exit 1
    fi
    
    # Step 3: Build core components
    if ! build_core_components; then
        echo -e "${RED}Failed to build core components${NC}"
        exit 1
    fi
    
    # Step 4: Build Do3Think component
    if ! build_do3think_component; then
        echo -e "${RED}Failed to build Do3Think component${NC}"
        exit 1
    fi
    
    # Step 5: Build viewer application
    if ! build_viewer; then
        echo -e "${RED}Failed to build viewer application${NC}"
        exit 1
    fi
    
    # Step 6: Copy runtime dependencies
    copy_runtime_deps
    
    # Step 7: Create development setup
    create_dev_setup
    
    echo ""
    echo -e "${GREEN}========================================${NC}"
    echo -e "${GREEN}Build completed successfully!${NC}"
    echo -e "${GREEN}========================================${NC}"
    echo ""
    echo -e "${BLUE}To run the viewer:${NC}"
    echo -e "  cd build_viewer"
    echo -e "  ./run_viewer.sh"
    echo ""
    echo -e "${BLUE}To set up Qt environment for development:${NC}"
    echo -e "  source ./setup_qt_env.sh"
    echo ""
    echo -e "${BLUE}Build artifacts:${NC}"
    echo -e "  Core library: build_core/libComponentsForestCore.so"
    echo -e "  Do3Think component: build_core/libDo3ThinkCameraComponent.so"
    echo -e "  Viewer application: build_viewer/Do3ThinkCameraViewer"
    echo ""
}

# Run main function
main "$@"