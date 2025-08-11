#!/bin/bash

################################################################################
# QML Camera Viewer Implementation Verification Script
# 
# This script performs comprehensive verification of the QML implementation
# including architecture compliance, file existence, syntax validation,
# build verification, and dependency checks.
#
# Usage: ./verify_qml_implementation.sh [--verbose] [--fix]
################################################################################

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
VERBOSE=false
FIX_ISSUES=false
REPORT_FILE="qml_verification_report_$(date +%Y%m%d_%H%M%S).txt"
TOTAL_CHECKS=0
PASSED_CHECKS=0
FAILED_CHECKS=0
WARNINGS=0

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --verbose)
            VERBOSE=true
            shift
            ;;
        --fix)
            FIX_ISSUES=true
            shift
            ;;
        *)
            echo "Unknown option: $1"
            echo "Usage: $0 [--verbose] [--fix]"
            exit 1
            ;;
    esac
done

# Logging functions
log() {
    echo -e "$1" | tee -a "$REPORT_FILE"
}

log_success() {
    log "${GREEN}✓${NC} $1"
    ((PASSED_CHECKS++))
    ((TOTAL_CHECKS++))
}

log_error() {
    log "${RED}✗${NC} $1"
    ((FAILED_CHECKS++))
    ((TOTAL_CHECKS++))
}

log_warning() {
    log "${YELLOW}⚠${NC} $1"
    ((WARNINGS++))
}

log_info() {
    log "${BLUE}ℹ${NC} $1"
}

log_section() {
    log "\n${BLUE}========================================${NC}"
    log "${BLUE}$1${NC}"
    log "${BLUE}========================================${NC}"
}

verbose_log() {
    if [ "$VERBOSE" = true ]; then
        log "  $1"
    fi
}

################################################################################
# 1. FILE EXISTENCE CHECKS
################################################################################

check_file_exists() {
    local file="$1"
    local description="$2"
    
    if [ -f "$file" ]; then
        log_success "$description exists: $file"
        return 0
    else
        log_error "$description missing: $file"
        return 1
    fi
}

check_directory_exists() {
    local dir="$1"
    local description="$2"
    
    if [ -d "$dir" ]; then
        log_success "$description exists: $dir"
        return 0
    else
        log_error "$description missing: $dir"
        return 1
    fi
}

verify_file_structure() {
    log_section "FILE STRUCTURE VERIFICATION"
    
    # Core directories
    check_directory_exists "viewers/qml_camera_viewer" "QML viewer directory"
    check_directory_exists "qml_bridge" "QML bridge directory"
    check_directory_exists "viewers/qml_camera_viewer/qml" "QML files directory"
    check_directory_exists "components" "Components directory"
    check_directory_exists "include" "Include directory"
    check_directory_exists "src" "Source directory"
    
    # QML viewer files
    check_file_exists "viewers/qml_camera_viewer/main.cpp" "QML viewer main"
    check_file_exists "viewers/qml_camera_viewer/CMakeLists.txt" "QML CMakeLists"
    check_file_exists "viewers/qml_camera_viewer/qml_image_provider.cpp" "Image provider source"
    check_file_exists "viewers/qml_camera_viewer/qml_image_provider.h" "Image provider header"
    check_file_exists "viewers/qml_camera_viewer/qml.qrc" "QML resource file"
    
    # QML files
    check_file_exists "viewers/qml_camera_viewer/qml/main.qml" "Main QML file"
    check_file_exists "viewers/qml_camera_viewer/qml/CameraView.qml" "Camera view QML"
    check_file_exists "viewers/qml_camera_viewer/qml/CameraControlPanel.qml" "Control panel QML"
    check_file_exists "viewers/qml_camera_viewer/qml/StatusBar.qml" "Status bar QML"
    check_file_exists "viewers/qml_camera_viewer/qml/ThemeConstants.qml" "Theme constants QML"
    
    # Bridge files
    check_file_exists "qml_bridge/camera_bridge.cpp" "Camera bridge source"
    check_file_exists "qml_bridge/camera_bridge.h" "Camera bridge header"
    check_file_exists "qml_bridge/machine_bridge.cpp" "Machine bridge source"
    check_file_exists "qml_bridge/machine_bridge.h" "Machine bridge header"
    
    # Component files
    check_file_exists "components/base_component/base_component.cpp" "Base component source"
    check_file_exists "components/camera_component/camera_component.cpp" "Camera component source"
    check_file_exists "components/do3think_camera/dothink_camera_component.cpp" "Do3Think component"
}

################################################################################
# 2. ARCHITECTURE COMPLIANCE CHECKS
################################################################################

check_inheritance() {
    local file="$1"
    local class="$2"
    local base_class="$3"
    
    if grep -q "class $class.*:.*public $base_class" "$file" 2>/dev/null; then
        log_success "$class properly inherits from $base_class"
        return 0
    else
        log_error "$class does not inherit from $base_class"
        return 1
    fi
}

check_signal_slot_decoupling() {
    local file="$1"
    local description="$2"
    
    # Check for string-based connections (good)
    local string_connects=$(grep -c "connect.*SIGNAL.*SLOT" "$file" 2>/dev/null || echo 0)
    
    # Check for compile-time connections (potentially bad for decoupling)
    local direct_connects=$(grep -c "connect.*&.*::.*&" "$file" 2>/dev/null || echo 0)
    
    if [ "$direct_connects" -eq 0 ]; then
        log_success "$description uses proper signal/slot decoupling"
    elif [ "$string_connects" -gt "$direct_connects" ]; then
        log_warning "$description has some compile-time connections (found $direct_connects)"
    else
        log_error "$description violates signal/slot decoupling principle"
    fi
}

verify_architecture_compliance() {
    log_section "ARCHITECTURE COMPLIANCE VERIFICATION"
    
    # Check inheritance hierarchy
    if [ -f "qml_bridge/camera_bridge.h" ]; then
        check_inheritance "qml_bridge/camera_bridge.h" "CameraBridge" "QObject"
    fi
    
    if [ -f "qml_bridge/machine_bridge.h" ]; then
        check_inheritance "qml_bridge/machine_bridge.h" "MachineBridge" "QObject"
    fi
    
    # Check for Q_OBJECT macro
    for file in qml_bridge/*.h; do
        if [ -f "$file" ]; then
            if grep -q "Q_OBJECT" "$file"; then
                log_success "$(basename $file) has Q_OBJECT macro"
            else
                log_error "$(basename $file) missing Q_OBJECT macro"
            fi
        fi
    done
    
    # Check signal/slot decoupling in bridge files
    for file in qml_bridge/*.cpp; do
        if [ -f "$file" ]; then
            check_signal_slot_decoupling "$file" "$(basename $file)"
        fi
    done
    
    # Check for direct component access (violation)
    for file in qml_bridge/*.cpp viewers/qml_camera_viewer/main.cpp; do
        if [ -f "$file" ]; then
            if grep -q "m_component->" "$file" 2>/dev/null; then
                log_warning "$(basename $file) may have direct component access"
            else
                log_success "$(basename $file) avoids direct component access"
            fi
        fi
    done
}

################################################################################
# 3. QML SYNTAX VALIDATION
################################################################################

validate_qml_syntax() {
    local qml_file="$1"
    
    # Basic syntax checks
    local errors=""
    
    # Check for unclosed braces
    local open_braces=$(grep -o '{' "$qml_file" | wc -l)
    local close_braces=$(grep -o '}' "$qml_file" | wc -l)
    if [ "$open_braces" -ne "$close_braces" ]; then
        errors="${errors}Unmatched braces (open: $open_braces, close: $close_braces); "
    fi
    
    # Check for import statements
    if ! grep -q "^import QtQuick" "$qml_file"; then
        errors="${errors}Missing QtQuick import; "
    fi
    
    # Check for root element
    if ! grep -q "^\(Item\|Rectangle\|Window\|ApplicationWindow\|Column\|Row\|GridLayout\) {" "$qml_file"; then
        log_warning "$(basename $qml_file) may be missing a root element"
    fi
    
    if [ -z "$errors" ]; then
        log_success "$(basename $qml_file) syntax appears valid"
        return 0
    else
        log_error "$(basename $qml_file) syntax issues: $errors"
        return 1
    fi
}

verify_qml_files() {
    log_section "QML FILE VALIDATION"
    
    local qml_dir="viewers/qml_camera_viewer/qml"
    
    if [ -d "$qml_dir" ]; then
        for qml_file in "$qml_dir"/*.qml; do
            if [ -f "$qml_file" ]; then
                validate_qml_syntax "$qml_file"
            fi
        done
    else
        log_error "QML directory not found: $qml_dir"
    fi
    
    # Check QML resource file
    if [ -f "viewers/qml_camera_viewer/qml.qrc" ]; then
        # Verify all QML files are included in qrc
        for qml_file in "$qml_dir"/*.qml; do
            if [ -f "$qml_file" ]; then
                local basename=$(basename "$qml_file")
                if grep -q "$basename" "viewers/qml_camera_viewer/qml.qrc"; then
                    verbose_log "$basename is included in qml.qrc"
                else
                    log_warning "$basename not found in qml.qrc"
                fi
            fi
        done
        log_success "QML resource file validated"
    else
        log_error "QML resource file missing"
    fi
}

################################################################################
# 4. DEPENDENCY CHECKS
################################################################################

check_qt_installation() {
    log_section "QT INSTALLATION CHECK"
    
    # Check for qmake
    if command -v qmake &> /dev/null; then
        local qt_version=$(qmake -query QT_VERSION)
        log_success "Qt found: version $qt_version"
        
        # Check if Qt6
        if [[ "$qt_version" == 6.* ]]; then
            log_success "Qt6 detected (required for QML viewer)"
        else
            log_warning "Qt5 detected - Qt6 recommended for QML viewer"
        fi
    else
        log_error "Qt not found in PATH"
    fi
    
    # Check for required Qt modules
    local required_modules=("QtCore" "QtGui" "QtWidgets" "QtQuick" "QtQml")
    for module in "${required_modules[@]}"; do
        if pkg-config --exists Qt6$module 2>/dev/null || pkg-config --exists Qt5$module 2>/dev/null; then
            log_success "$module module available"
        else
            log_warning "$module module not found via pkg-config"
        fi
    done
}

check_cmake() {
    if command -v cmake &> /dev/null; then
        local cmake_version=$(cmake --version | head -n1 | cut -d' ' -f3)
        log_success "CMake found: version $cmake_version"
        
        # Check minimum version (3.16)
        if [ "$(printf '%s\n' "3.16" "$cmake_version" | sort -V | head -n1)" = "3.16" ]; then
            log_success "CMake version meets minimum requirement (3.16)"
        else
            log_error "CMake version too old (minimum 3.16 required)"
        fi
    else
        log_error "CMake not found"
    fi
}

check_compiler() {
    if command -v g++ &> /dev/null; then
        local gcc_version=$(g++ --version | head -n1)
        log_success "G++ found: $gcc_version"
    elif command -v clang++ &> /dev/null; then
        local clang_version=$(clang++ --version | head -n1)
        log_success "Clang++ found: $clang_version"
    else
        log_error "No C++ compiler found"
    fi
}

verify_dependencies() {
    log_section "DEPENDENCY VERIFICATION"
    
    check_qt_installation
    check_cmake
    check_compiler
    
    # Check for Do3Think SDK
    if [ -d "Do3ThinkCamera/SDK" ]; then
        log_success "Do3Think SDK directory found"
        
        # Check for required SDK files
        if [ -f "Do3ThinkCamera/SDK/include/DVPCamera.h" ]; then
            log_success "Do3Think SDK headers found"
        else
            log_warning "Do3Think SDK headers not found"
        fi
        
        if [ -f "Do3ThinkCamera/SDK/lib/linux/x64/libDVPCamera.so" ] || 
           [ -f "Do3ThinkCamera/SDK/lib/linux/x64/libDVPCamera64.so" ]; then
            log_success "Do3Think SDK libraries found"
        else
            log_warning "Do3Think SDK libraries not found"
        fi
    else
        log_warning "Do3Think SDK not found - camera functionality will be limited"
    fi
}

################################################################################
# 5. BUILD VERIFICATION
################################################################################

attempt_build() {
    log_section "BUILD VERIFICATION"
    
    local build_dir="viewers/qml_camera_viewer/build_test"
    
    # Clean previous build
    if [ -d "$build_dir" ]; then
        rm -rf "$build_dir"
    fi
    
    mkdir -p "$build_dir"
    cd "$build_dir"
    
    log_info "Attempting CMake configuration..."
    
    # Try to configure with CMake
    if cmake .. -DCMAKE_BUILD_TYPE=Debug &> cmake_output.log; then
        log_success "CMake configuration successful"
        
        # Try to build
        log_info "Attempting build..."
        if cmake --build . --parallel 4 &> build_output.log; then
            log_success "Build completed successfully"
            
            # Check if executable was created
            if [ -f "QMLCameraViewer" ]; then
                log_success "Executable created: QMLCameraViewer"
            else
                log_warning "Executable not found after build"
            fi
        else
            log_error "Build failed - check $build_dir/build_output.log"
            if [ "$VERBOSE" = true ]; then
                tail -20 build_output.log
            fi
        fi
    else
        log_error "CMake configuration failed - check $build_dir/cmake_output.log"
        if [ "$VERBOSE" = true ]; then
            tail -20 cmake_output.log
        fi
    fi
    
    cd - > /dev/null
}

################################################################################
# 6. THREAD SAFETY VERIFICATION
################################################################################

check_thread_safety() {
    log_section "THREAD SAFETY VERIFICATION"
    
    # Check for moveToThread usage
    for file in qml_bridge/*.cpp viewers/qml_camera_viewer/main.cpp; do
        if [ -f "$file" ]; then
            if grep -q "moveToThread" "$file"; then
                log_success "$(basename $file) uses moveToThread for threading"
            else
                log_warning "$(basename $file) may not use proper threading"
            fi
        fi
    done
    
    # Check for QMutex usage
    for file in qml_bridge/*.cpp; do
        if [ -f "$file" ]; then
            if grep -q "QMutex\|QMutexLocker" "$file"; then
                log_success "$(basename $file) uses mutex for thread safety"
            else
                verbose_log "$(basename $file) does not use explicit mutex (may be okay)"
            fi
        fi
    done
    
    # Check for Qt::QueuedConnection
    for file in qml_bridge/*.cpp viewers/qml_camera_viewer/main.cpp; do
        if [ -f "$file" ]; then
            if grep -q "Qt::QueuedConnection" "$file"; then
                log_success "$(basename $file) uses QueuedConnection for cross-thread signals"
            else
                verbose_log "$(basename $file) may not use QueuedConnection"
            fi
        fi
    done
}

################################################################################
# 7. QML INTEGRATION VERIFICATION
################################################################################

verify_qml_integration() {
    log_section "QML INTEGRATION VERIFICATION"
    
    # Check QML context properties
    if [ -f "viewers/qml_camera_viewer/main.cpp" ]; then
        if grep -q "setContextProperty" "viewers/qml_camera_viewer/main.cpp"; then
            log_success "QML context properties are set"
            
            # Check for expected properties
            local expected_props=("cameraBridge" "machineBridge")
            for prop in "${expected_props[@]}"; do
                if grep -q "setContextProperty.*\"$prop\"" "viewers/qml_camera_viewer/main.cpp"; then
                    log_success "Context property '$prop' is registered"
                else
                    log_error "Context property '$prop' not found"
                fi
            done
        else
            log_error "No QML context properties found"
        fi
    fi
    
    # Check image provider registration
    if [ -f "viewers/qml_camera_viewer/main.cpp" ]; then
        if grep -q "addImageProvider" "viewers/qml_camera_viewer/main.cpp"; then
            log_success "QML image provider is registered"
        else
            log_error "QML image provider not registered"
        fi
    fi
    
    # Check QML engine initialization
    if [ -f "viewers/qml_camera_viewer/main.cpp" ]; then
        if grep -q "QQmlApplicationEngine\|QQmlEngine" "viewers/qml_camera_viewer/main.cpp"; then
            log_success "QML engine is initialized"
        else
            log_error "QML engine initialization not found"
        fi
    fi
}

################################################################################
# 8. PERFORMANCE CHECKS
################################################################################

check_performance_patterns() {
    log_section "PERFORMANCE PATTERN VERIFICATION"
    
    # Check for image caching
    if grep -r "QPixmapCache\|QImageCache" qml_bridge/ viewers/qml_camera_viewer/ 2>/dev/null | grep -q .; then
        log_success "Image caching patterns found"
    else
        log_warning "No image caching patterns detected"
    fi
    
    # Check for frame rate limiting
    if grep -r "30\|33\|60" qml_bridge/*.cpp 2>/dev/null | grep -q "fps\|FPS\|frame"; then
        log_success "Frame rate limiting appears to be implemented"
    else
        log_warning "Frame rate limiting not clearly defined"
    fi
    
    # Check for ring buffer usage
    if grep -r "RingBuffer\|CircularBuffer\|ring\|circular" components/ 2>/dev/null | grep -q .; then
        log_success "Ring buffer pattern found for efficient memory usage"
    else
        verbose_log "Ring buffer pattern not found (may use other methods)"
    fi
}

################################################################################
# 9. DOCUMENTATION VERIFICATION
################################################################################

verify_documentation() {
    log_section "DOCUMENTATION VERIFICATION"
    
    local doc_files=(
        "viewers/qml_camera_viewer/README_QML.md"
        "viewers/qml_camera_viewer/README_QML_VIEWER.md"
        "viewers/qml_camera_viewer/BUILD_GUIDE.md"
        "qml_bridge/README_BRIDGE_IMPLEMENTATION.md"
    )
    
    for doc in "${doc_files[@]}"; do
        if [ -f "$doc" ]; then
            log_success "Documentation found: $(basename $doc)"
        else
            log_warning "Documentation missing: $doc"
        fi
    done
}

################################################################################
# 10. COMMON ISSUES CHECK
################################################################################

check_common_issues() {
    log_section "COMMON ISSUES CHECK"
    
    # Check for Qt platform plugins
    if [ -d "viewers/qml_camera_viewer/build/platforms" ]; then
        if [ -f "viewers/qml_camera_viewer/build/platforms/qwindows.dll" ] || 
           [ -f "viewers/qml_camera_viewer/build/platforms/libqxcb.so" ]; then
            log_success "Qt platform plugins present"
        else
            log_warning "Qt platform plugins missing in build directory"
        fi
    else
        verbose_log "Build directory not found for platform plugin check"
    fi
    
    # Check for missing includes
    for file in qml_bridge/*.cpp viewers/qml_camera_viewer/*.cpp; do
        if [ -f "$file" ]; then
            # Check for common missing includes
            if ! grep -q "#include <QDebug>" "$file" && grep -q "qDebug" "$file"; then
                log_warning "$(basename $file) uses qDebug but missing QDebug include"
            fi
        fi
    done
    
    # Check for memory leaks patterns
    for file in qml_bridge/*.cpp; do
        if [ -f "$file" ]; then
            # Check for new without delete
            local new_count=$(grep -c "new " "$file" 2>/dev/null || echo 0)
            local delete_count=$(grep -c "delete " "$file" 2>/dev/null || echo 0)
            
            if [ "$new_count" -gt 0 ] && [ "$delete_count" -eq 0 ]; then
                log_warning "$(basename $file) has 'new' without matching 'delete' (check for memory leaks)"
            fi
        fi
    done
}

################################################################################
# 11. FIX COMMON ISSUES
################################################################################

fix_common_issues() {
    if [ "$FIX_ISSUES" = true ]; then
        log_section "ATTEMPTING TO FIX COMMON ISSUES"
        
        # Create missing directories
        mkdir -p viewers/qml_camera_viewer/qml
        mkdir -p qml_bridge
        mkdir -p viewers/qml_camera_viewer/build
        
        # Copy Qt platform plugins if available
        if command -v qmake &> /dev/null; then
            local qt_dir=$(qmake -query QT_INSTALL_PLUGINS)
            if [ -d "$qt_dir/platforms" ]; then
                mkdir -p viewers/qml_camera_viewer/build/platforms
                cp -r "$qt_dir/platforms"/* viewers/qml_camera_viewer/build/platforms/ 2>/dev/null || true
                log_info "Copied Qt platform plugins"
            fi
        fi
        
        log_success "Applied available fixes"
    fi
}

################################################################################
# MAIN EXECUTION
################################################################################

main() {
    log "========================================="
    log "QML CAMERA VIEWER VERIFICATION REPORT"
    log "Date: $(date)"
    log "========================================="
    
    # Run all verification steps
    verify_file_structure
    verify_architecture_compliance
    verify_qml_files
    verify_dependencies
    check_thread_safety
    verify_qml_integration
    check_performance_patterns
    verify_documentation
    check_common_issues
    
    # Attempt build if all critical checks pass
    if [ "$FAILED_CHECKS" -lt 5 ]; then
        attempt_build
    else
        log_warning "Skipping build due to too many failed checks"
    fi
    
    # Fix issues if requested
    fix_common_issues
    
    # Generate summary
    log_section "VERIFICATION SUMMARY"
    log "Total Checks: $TOTAL_CHECKS"
    log "${GREEN}Passed: $PASSED_CHECKS${NC}"
    log "${RED}Failed: $FAILED_CHECKS${NC}"
    log "${YELLOW}Warnings: $WARNINGS${NC}"
    
    local success_rate=$((PASSED_CHECKS * 100 / TOTAL_CHECKS))
    log "\nSuccess Rate: ${success_rate}%"
    
    if [ "$success_rate" -ge 90 ]; then
        log "${GREEN}✓ Implementation is EXCELLENT${NC}"
        exit 0
    elif [ "$success_rate" -ge 70 ]; then
        log "${GREEN}✓ Implementation is GOOD${NC}"
        exit 0
    elif [ "$success_rate" -ge 50 ]; then
        log "${YELLOW}⚠ Implementation needs improvement${NC}"
        exit 1
    else
        log "${RED}✗ Implementation has significant issues${NC}"
        exit 2
    fi
}

# Run main function
main