cmake_minimum_required(VERSION 3.16)
project(ComponentsForest VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Find required packages
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Concurrent Network Charts)

# OpenCV is optional - only required for OpenCV components
if(BUILD_OPENCV_COMPONENTS)
    find_package(OpenCV REQUIRED)
else()
    find_package(OpenCV QUIET)
endif()

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/components
    ${CMAKE_CURRENT_SOURCE_DIR}/Do3ThinkCamera
    ${CMAKE_CURRENT_SOURCE_DIR}/Do3ThinkCamera/SDK
)

# Do3Think SDK setup
# Try to find DVPCamera library in common locations
if(WIN32)
    # Common installation paths for Do3Think SDK on Windows
    set(DO3THINK_SEARCH_PATHS
        "C:/Program Files (x86)/Camera/DVP2 x64"
        "C:/Program Files (x86)/Camera/DVP2"
        "C:/Program Files/Do3Think/DVP2 SDK/lib"
        "C:/Program Files (x86)/Do3Think/DVP2 SDK/lib"
        "C:/Do3Think/DVP2 SDK/lib"
        "D:/Do3Think/DVP2 SDK/lib"
        "${CMAKE_CURRENT_SOURCE_DIR}/Do3ThinkCamera/SDK/lib"
        "${CMAKE_CURRENT_SOURCE_DIR}/Do3ThinkCamera/library/Visual C++/lib"
    )
    
    # First try to find the .lib file
    find_library(DVPCAMERA_LIB
        NAMES DVPCamera64 DVPCamera
        PATHS ${DO3THINK_SEARCH_PATHS}
        PATH_SUFFIXES x64 x64/Release x64/Debug lib
    )
    
    # Decide whether to use dynamic loading or static linking
    if(NOT DVPCAMERA_LIB)
        find_file(DVPCAMERA_DLL
            NAMES DVPCamera64.dll DVPCamera.dll
            PATHS ${DO3THINK_SEARCH_PATHS}
            PATH_SUFFIXES "" bin
        )
        
        if(DVPCAMERA_DLL)
            message(STATUS "Found Do3Think SDK DLL: ${DVPCAMERA_DLL}")
            message(STATUS "Will use dynamic loading since no import library was found")
        else()
            message(WARNING "DVPCamera64.dll not found. Dynamic loading will be required at runtime.")
        endif()
        
        # For now, always use dynamic loading since the stub library approach has issues
        # with dllimport/dllexport on Windows
        message(STATUS "Using dynamic loading for Do3Think SDK (no import library found)")
        set(USE_DYNAMIC_LOADING TRUE)
    else()
        message(STATUS "Found Do3Think SDK library: ${DVPCAMERA_LIB}")
        set(USE_DYNAMIC_LOADING FALSE)
    endif()
endif()

# Define base component library
add_library(ComponentsForestCore STATIC
    components/base_component.h
    components/base_component.cpp
    components/camera_component.h
    components/camera_component.cpp
    components/camera_control_panel.h
    components/camera_control_panel.cpp
)

# Generate export header for proper DLL export/import
include(GenerateExportHeader)
generate_export_header(ComponentsForestCore
    EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/componentsforestcore_export.h
    EXPORT_MACRO_NAME COMPONENTSFORESTCORE_EXPORT
    STATIC_DEFINE COMPONENTSFORESTCORE_STATIC_DEFINE
)
target_include_directories(ComponentsForestCore PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(ComponentsForestCore
    Qt6::Core
    Qt6::Widgets
    Qt6::Charts  # For performance charts in control panels
)

# Define Do3Think Camera Component library
add_library(Do3ThinkCameraComponent STATIC
    Do3ThinkCamera/dothink_camera.h
    Do3ThinkCamera/dothink_camera.cpp
    Do3ThinkCamera/dothink_camera_control_panel.h
    Do3ThinkCamera/dothink_camera_control_panel.cpp
    Do3ThinkCamera/dothink_camera_control_panel_methods.cpp
    # Do3ThinkCamera/dothink_camera_roi_integration.h  # Temporarily disabled - API mismatches need fixing
    # Do3ThinkCamera/dothink_camera_roi_integration.cpp
    Do3ThinkCamera/dvp_wrapper.h
    Do3ThinkCamera/dvp_wrapper.cpp
)

# Generate export header for Do3ThinkCameraComponent
generate_export_header(Do3ThinkCameraComponent
    EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/do3thinkcameracomponent_export.h
    EXPORT_MACRO_NAME DO3THINKCAMERACOMPONENT_EXPORT
    STATIC_DEFINE DO3THINKCAMERACOMPONENT_STATIC_DEFINE
)
target_include_directories(Do3ThinkCameraComponent PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(Do3ThinkCameraComponent
    ComponentsForestCore
    Qt6::Core
    Qt6::Widgets
    Qt6::Concurrent
    Qt6::Charts
)

# Link OpenCV only if found
if(OpenCV_FOUND)
    target_link_libraries(Do3ThinkCameraComponent ${OpenCV_LIBS})
    target_compile_definitions(Do3ThinkCameraComponent PRIVATE HAS_OPENCV)
endif()

if(USE_DYNAMIC_LOADING)
    # Add compile definition for dynamic loading
    # When using dynamic loading, we don't link against any library
    target_compile_definitions(Do3ThinkCameraComponent PRIVATE USE_DYNAMIC_LOADING)
    
    if(DVPCAMERA_DLL)
        # Copy the DLL to the output directory for runtime
        add_custom_command(TARGET Do3ThinkCameraComponent POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${DVPCAMERA_DLL}"
            $<TARGET_FILE_DIR:Do3ThinkCameraComponent>)
    endif()
else()
    # Link Do3Think SDK library (either real SDK or stub)
    if(DVPCAMERA_LIB)
        target_link_libraries(Do3ThinkCameraComponent ${DVPCAMERA_LIB})
        
        # If using the stub library, define DVP_USE_STUB_LIBRARY
        if(USING_STUB_LIBRARY)
            target_compile_definitions(Do3ThinkCameraComponent PRIVATE DVP_USE_STUB_LIBRARY)
            message(STATUS "Using stub library - defining DVP_USE_STUB_LIBRARY")
        endif()
    endif()
endif()

# Set properties
set_target_properties(ComponentsForestCore PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    PUBLIC_HEADER "components/base_component.h"
)

set_target_properties(Do3ThinkCameraComponent PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    PUBLIC_HEADER "Do3ThinkCamera/dothink_camera.h;Do3ThinkCamera/dothink_camera_control_panel.h"
)

# Define static library symbols
target_compile_definitions(ComponentsForestCore PUBLIC COMPONENTSFORESTCORE_STATIC_DEFINE)
target_compile_definitions(Do3ThinkCameraComponent PUBLIC DO3THINKCAMERACOMPONENT_STATIC_DEFINE)

# Installation rules
install(TARGETS ComponentsForestCore Do3ThinkCameraComponent
    LIBRARY DESTINATION lib
    PUBLIC_HEADER DESTINATION include/ComponentsForest
)

# Example application (optional)
option(BUILD_EXAMPLES "Build example applications" OFF)
if(BUILD_EXAMPLES)
    add_executable(CameraExample
        examples/camera_example.cpp
    )
    
    target_link_libraries(CameraExample
        Do3ThinkCameraComponent
        Qt6::Core
        Qt6::Widgets
    )
endif()

# OpenCV ROI System
option(BUILD_OPENCV_COMPONENTS "Build OpenCV components" ON)
if(BUILD_OPENCV_COMPONENTS AND OpenCV_FOUND)
    add_subdirectory(OpenCV)
    message(STATUS "OpenCV components will be built")
elseif(BUILD_OPENCV_COMPONENTS AND NOT OpenCV_FOUND)
    message(STATUS "OpenCV components disabled - OpenCV not found")
    set(BUILD_OPENCV_COMPONENTS OFF CACHE BOOL "Build OpenCV components" FORCE)
endif()

# Viewer applications
option(BUILD_VIEWERS "Build viewer applications" ON)
option(BUILD_QML_VIEWER "Build QML-based camera viewer" ON)

if(BUILD_VIEWERS)
    add_subdirectory(viewers/do3think_camera_viewer)
    
    if(BUILD_QML_VIEWER)
        # Check if Qt Quick is available
        find_package(Qt6 QUIET COMPONENTS Quick QuickControls2 Qml)
        if(Qt6Quick_FOUND AND Qt6QuickControls2_FOUND AND Qt6Qml_FOUND)
            add_subdirectory(viewers/qml_camera_viewer)
            message(STATUS "QML Camera Viewer will be built")
        else()
            message(STATUS "QML Camera Viewer skipped - Qt Quick components not found")
        endif()
    endif()
endif()

# Testing
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG release-1.12.1
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

option(BUILD_TESTS "Build test suite" ON)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Package configuration
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/ComponentsForestConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

# Display configuration summary
message(STATUS "")
message(STATUS "ComponentsForest Configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Qt Version: ${Qt6_VERSION}")
if(OpenCV_FOUND)
    message(STATUS "  OpenCV Version: ${OpenCV_VERSION}")
else()
    message(STATUS "  OpenCV: Not found (OpenCV components disabled)")
endif()
message(STATUS "  Build Examples: ${BUILD_EXAMPLES}")
message(STATUS "  Build OpenCV Components: ${BUILD_OPENCV_COMPONENTS}")
message(STATUS "  Build Viewers: ${BUILD_VIEWERS}")
message(STATUS "  Build QML Viewer: ${BUILD_QML_VIEWER}")
message(STATUS "  Build Tests: ${BUILD_TESTS}")
message(STATUS "")