#!/bin/bash

################################################################################
# Quick Test Runner for QML Implementation
################################################################################

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'
BOLD='\033[1m'

echo -e "${BLUE}${BOLD}=== QML Implementation Test Runner ===${NC}"
echo

# Check if we're in the right directory
if [ ! -f "CMakeLists.txt" ]; then
    echo -e "${RED}Error: CMakeLists.txt not found. Run from project root.${NC}"
    exit 1
fi

# Option parsing
BUILD_TYPE="Debug"
RUN_VERIFICATION=false
RUN_UNIT_TESTS=false
RUN_ALL=false

while [[ $# -gt 0 ]]; do
    case $1 in
        --verify)
            RUN_VERIFICATION=true
            shift
            ;;
        --test)
            RUN_UNIT_TESTS=true
            shift
            ;;
        --all)
            RUN_ALL=true
            shift
            ;;
        --release)
            BUILD_TYPE="Release"
            shift
            ;;
        --help)
            echo "Usage: $0 [options]"
            echo "Options:"
            echo "  --verify    Run verification script"
            echo "  --test      Build and run unit tests"
            echo "  --all       Run all tests and verification"
            echo "  --release   Build in Release mode"
            echo "  --help      Show this help"
            exit 0
            ;;
        *)
            echo -e "${RED}Unknown option: $1${NC}"
            exit 1
            ;;
    esac
done

# Default to all if nothing specified
if [ "$RUN_VERIFICATION" = false ] && [ "$RUN_UNIT_TESTS" = false ]; then
    RUN_ALL=true
fi

if [ "$RUN_ALL" = true ]; then
    RUN_VERIFICATION=true
    RUN_UNIT_TESTS=true
fi

################################################################################
# Run Verification Script
################################################################################

if [ "$RUN_VERIFICATION" = true ]; then
    echo -e "${YELLOW}${BOLD}Running verification script...${NC}"
    
    if [ -f "verify_qml_implementation.sh" ]; then
        chmod +x verify_qml_implementation.sh
        ./verify_qml_implementation.sh
        VERIFY_RESULT=$?
        
        if [ $VERIFY_RESULT -eq 0 ]; then
            echo -e "${GREEN}✓ Verification passed${NC}"
        else
            echo -e "${YELLOW}⚠ Verification completed with issues${NC}"
        fi
    else
        echo -e "${RED}✗ verify_qml_implementation.sh not found${NC}"
    fi
    
    echo
fi

################################################################################
# Build and Run Unit Tests
################################################################################

if [ "$RUN_UNIT_TESTS" = true ]; then
    echo -e "${YELLOW}${BOLD}Building and running unit tests...${NC}"
    
    # Create test build directory
    TEST_BUILD_DIR="build_tests"
    
    if [ -d "$TEST_BUILD_DIR" ]; then
        echo "Cleaning existing test build directory..."
        rm -rf "$TEST_BUILD_DIR"
    fi
    
    echo "Creating test build directory..."
    mkdir -p "$TEST_BUILD_DIR"
    cd "$TEST_BUILD_DIR"
    
    # Configure with testing enabled
    echo -e "${BLUE}Configuring CMake with testing...${NC}"
    if cmake .. -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
                -DBUILD_TESTING=ON \
                -DBUILD_QML_VIEWER=ON; then
        echo -e "${GREEN}✓ CMake configuration successful${NC}"
        
        # Build tests
        echo -e "${BLUE}Building tests...${NC}"
        if cmake --build . --target test_qml_viewer --parallel; then
            echo -e "${GREEN}✓ Build successful${NC}"
            
            # Run tests
            echo -e "${BLUE}Running tests...${NC}"
            if [ -f "test_qml_viewer" ]; then
                # Run with Qt platform set to offscreen for CI/headless
                export QT_QPA_PLATFORM=offscreen
                
                if ./test_qml_viewer -v2; then
                    echo -e "${GREEN}${BOLD}✓ All unit tests passed!${NC}"
                    TEST_RESULT=0
                else
                    echo -e "${RED}${BOLD}✗ Some tests failed${NC}"
                    TEST_RESULT=1
                fi
            else
                echo -e "${RED}✗ Test executable not found${NC}"
                TEST_RESULT=1
            fi
        else
            echo -e "${RED}✗ Build failed${NC}"
            echo "Check build output for errors"
            TEST_RESULT=1
        fi
    else
        echo -e "${RED}✗ CMake configuration failed${NC}"
        TEST_RESULT=1
    fi
    
    cd ..
    echo
fi

################################################################################
# Summary
################################################################################

echo -e "${BLUE}${BOLD}=== Test Summary ===${NC}"

if [ "$RUN_VERIFICATION" = true ]; then
    if [ ${VERIFY_RESULT:-1} -eq 0 ]; then
        echo -e "${GREEN}✓ Verification: PASSED${NC}"
    else
        echo -e "${YELLOW}⚠ Verification: ISSUES FOUND${NC}"
    fi
fi

if [ "$RUN_UNIT_TESTS" = true ]; then
    if [ ${TEST_RESULT:-1} -eq 0 ]; then
        echo -e "${GREEN}✓ Unit Tests: PASSED${NC}"
    else
        echo -e "${RED}✗ Unit Tests: FAILED${NC}"
    fi
fi

echo
echo -e "${BOLD}Reports generated:${NC}"
[ -f "qml_verification_summary.txt" ] && echo "  - qml_verification_summary.txt"
[ -f "qml_verification_*.log" ] && echo "  - qml_verification_*.log"
[ -d "build_tests" ] && echo "  - build_tests/ (test build artifacts)"

# Exit with appropriate code
if [ "$RUN_UNIT_TESTS" = true ] && [ ${TEST_RESULT:-1} -ne 0 ]; then
    exit 1
fi

if [ "$RUN_VERIFICATION" = true ] && [ ${VERIFY_RESULT:-1} -ne 0 ]; then
    exit 1  
fi

exit 0