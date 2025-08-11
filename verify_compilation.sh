#!/bin/bash

echo "=== Verifying C++ source files compilation ==="
echo ""

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Counter for errors
ERRORS=0
WARNINGS=0
SUCCESS=0

# Function to check a cpp file
check_file() {
    local file=$1
    echo -n "Checking $file... "
    
    # Try to compile with syntax checking only (no linking)
    output=$(g++ -std=c++17 -c -fsyntax-only -fPIC \
        -I. \
        -I./components \
        -I./Do3ThinkCamera \
        -I./Do3ThinkCamera/SDK \
        -I./viewers/do3think_camera_viewer \
        "$file" 2>&1)
    
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}OK${NC}"
        ((SUCCESS++))
    else
        echo -e "${RED}FAILED${NC}"
        echo "  Error details:"
        echo "$output" | head -20
        ((ERRORS++))
    fi
}

# Check viewer application files
echo "Checking viewer application files:"
echo "=================================="

if [ -f "viewers/do3think_camera_viewer/machine.cpp" ]; then
    check_file "viewers/do3think_camera_viewer/machine.cpp"
fi

if [ -f "viewers/do3think_camera_viewer/main_ui.cpp" ]; then
    check_file "viewers/do3think_camera_viewer/main_ui.cpp"
fi

if [ -f "viewers/do3think_camera_viewer/main.cpp" ]; then
    check_file "viewers/do3think_camera_viewer/main.cpp"
fi

echo ""
echo "Checking component files:"
echo "========================"

if [ -f "components/base_component.cpp" ]; then
    check_file "components/base_component.cpp"
fi

if [ -f "components/camera_component.cpp" ]; then
    check_file "components/camera_component.cpp"
fi

if [ -f "components/camera_control_panel.cpp" ]; then
    check_file "components/camera_control_panel.cpp"
fi

echo ""
echo "Checking Do3Think camera files:"
echo "==============================="

if [ -f "Do3ThinkCamera/dothink_camera.cpp" ]; then
    check_file "Do3ThinkCamera/dothink_camera.cpp"
fi

if [ -f "Do3ThinkCamera/dothink_camera_control_panel.cpp" ]; then
    check_file "Do3ThinkCamera/dothink_camera_control_panel.cpp"
fi

if [ -f "Do3ThinkCamera/dothink_camera_control_panel_methods.cpp" ]; then
    check_file "Do3ThinkCamera/dothink_camera_control_panel_methods.cpp"
fi

echo ""
echo "=================================="
echo "Summary:"
echo -e "  ${GREEN}Success: $SUCCESS files${NC}"
echo -e "  ${YELLOW}Warnings: $WARNINGS${NC}"
echo -e "  ${RED}Errors: $ERRORS files${NC}"
echo ""

if [ $ERRORS -eq 0 ]; then
    echo -e "${GREEN}All files passed basic syntax check!${NC}"
    echo ""
    echo "Note: This only verifies C++ syntax."
    echo "Full compilation requires Qt6 MOC processing and linking."
    echo ""
    echo "To build the complete application, use:"
    echo "  mkdir build && cd build"
    echo "  cmake .. -DCMAKE_PREFIX_PATH=/path/to/Qt/6.9.0"
    echo "  cmake --build ."
    exit 0
else
    echo -e "${RED}Some files have compilation errors.${NC}"
    echo "Please fix the errors above before building."
    exit 1
fi