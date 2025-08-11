# ComponentsForest QML Camera Viewer Build Configuration
# Supports both standalone and integrated builds
# Requires Qt 6.9+ with Quick and QuickControls2 modules

cmake_minimum_required(VERSION 3.16)
project(QMLCameraViewer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Export compile commands for better IDE support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Option to build as standalone or integrated
option(BUILD_STANDALONE "Build as standalone application" ON)
option(BUILD_WITH_OPENCV "Build with OpenCV support" OFF)
option(DEPLOY_QT_RUNTIME "Deploy Qt runtime libraries" ON)

# Find required Qt6 components - Updated for Qt 6.9.1
find_package(Qt6 6.9 REQUIRED COMPONENTS 
    Core 
    Gui 
    Quick 
    QuickControls2
    Qml
    Concurrent
    Widgets    # Required for camera control panels
    Charts     # Required for performance charts
    Network    # Required for network operations
)

# Add Qt6 QML import path
list(APPEND QML_IMPORT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/qml)
set_property(DIRECTORY APPEND PROPERTY QT_QML_IMPORT_PATH ${QML_IMPORT_PATH})

# Find OpenCV (required for preprocessor components)
find_package(OpenCV QUIET)

# Define sources
set(SOURCES
    main.cpp
    qml_image_provider.cpp
    ../../qml_bridge/camera_bridge.cpp
    ../../qml_bridge/machine_bridge.cpp
)

set(HEADERS
    qml_image_provider.h
    ../../qml_bridge/camera_bridge.h
    ../../qml_bridge/machine_bridge.h
)

# QML files - include all discovered QML files
set(QML_FILES
    qml/main.qml
    qml/components/CameraControlPanel.qml
    qml/components/CameraListDelegate.qml
    qml/components/CameraParameters.qml
    qml/components/ConnectionIndicator.qml
    qml/components/ImageDisplay.qml
    qml/components/ParameterControls.qml
    qml/components/SettingsPanel.qml
    qml/components/StatisticsPanel.qml
    qml/controls/ConnectionControls.qml
    qml/controls/AcquisitionControls.qml
)

# Use the existing qml.qrc resource file
qt6_add_resources(RESOURCES qml.qrc)

# Create executable
qt6_add_executable(${PROJECT_NAME}
    ${SOURCES}
    ${HEADERS}
    ${RESOURCES}
)

# Set up QML module
qt6_add_qml_module(${PROJECT_NAME}
    URI ComponentsForestQML
    VERSION 1.0
    QML_FILES ${QML_FILES}
    SOURCES ${SOURCES} ${HEADERS}
)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../
    ${CMAKE_CURRENT_SOURCE_DIR}/../../components
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Do3ThinkCamera
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Do3ThinkCamera/SDK
    ${CMAKE_CURRENT_SOURCE_DIR}/../../qml_bridge
    ${CMAKE_CURRENT_SOURCE_DIR}/../do3think_camera_viewer
    ${CMAKE_CURRENT_BINARY_DIR}  # For generated export headers
)

# Determine build mode based on target availability or option
if(NOT BUILD_STANDALONE AND TARGET ComponentsForestCore AND TARGET Do3ThinkCameraComponent)
    # Building as part of main project - link to existing libraries
    set(LINK_MODE "LIBRARIES")
    message(STATUS "QML Viewer: Linking to existing ComponentsForest libraries")
else()
    # Building standalone - include sources directly
    set(LINK_MODE "SOURCES")
    message(STATUS "QML Viewer: Building standalone with included sources")
    
    set(COMPONENT_SOURCES
        ../../components/base_component.cpp
        ../../components/camera_component.cpp
        ../../components/camera_control_panel.cpp
        ../../Do3ThinkCamera/dothink_camera.cpp
        ../../Do3ThinkCamera/dothink_camera_control_panel.cpp
        ../../Do3ThinkCamera/dothink_camera_control_panel_methods.cpp
        ../../Do3ThinkCamera/dvp_wrapper.cpp
        ../do3think_camera_viewer/machine.cpp
    )
    
    target_sources(${PROJECT_NAME} PRIVATE ${COMPONENT_SOURCES})
    
    # Define static library symbols when building standalone
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        COMPONENTSFORESTCORE_STATIC_DEFINE
        DO3THINKCAMERACOMPONENT_STATIC_DEFINE
    )
endif()

# Link libraries
if(LINK_MODE STREQUAL "LIBRARIES")
    # Link to existing ComponentsForest libraries
    target_link_libraries(${PROJECT_NAME} PRIVATE
        ComponentsForestCore
        Do3ThinkCameraComponent
    )
endif()

# Link Qt6 libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Qml
    Qt6::Concurrent
    Qt6::Widgets    # Required for camera control panels
    Qt6::Charts     # Required for performance charts
    Qt6::Network    # Required for network operations
)

# Link OpenCV if found
if(OpenCV_FOUND)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${OpenCV_LIBS})
    target_compile_definitions(${PROJECT_NAME} PRIVATE HAS_OPENCV)
endif()

# Set runtime output directory to simplify deployment
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin
)

# Compiler-specific flags for optimization
if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(${PROJECT_NAME} PRIVATE
        /W3           # Warning level 3
        /O2           # Optimize for speed
        /fp:fast      # Fast floating point
        /GL           # Whole program optimization
    )
    target_link_options(${PROJECT_NAME} PRIVATE
        /LTCG         # Link time code generation
    )
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(${PROJECT_NAME} PRIVATE
        -Wall -Wextra
        -O3
        -march=native
        -ffast-math
    )
endif()

# Platform-specific configurations
if(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
    
    # Link Windows libraries for components
    target_link_libraries(${PROJECT_NAME} PRIVATE
        ws2_32
        winmm
        psapi
    )
    
    # Handle Do3Think SDK
    if(LINK_MODE STREQUAL "SOURCES")
        # When building standalone, handle SDK directly
        find_library(DVPCAMERA_LIB
            NAMES DVPCamera64 DVPCamera
            PATHS
                "${CMAKE_CURRENT_SOURCE_DIR}/../../Do3ThinkCamera/SDK"
                "C:/Program Files (x86)/Camera/DVP2 x64"
                "C:/Program Files (x86)/Camera/DVP2"
            PATH_SUFFIXES lib x64
        )
        
        if(DVPCAMERA_LIB)
            target_link_libraries(${PROJECT_NAME} PRIVATE ${DVPCAMERA_LIB})
            message(STATUS "QML Viewer: Found Do3Think SDK library: ${DVPCAMERA_LIB}")
        else()
            # Use dynamic loading
            target_compile_definitions(${PROJECT_NAME} PRIVATE USE_DYNAMIC_LOADING)
            message(STATUS "QML Viewer: Using dynamic loading for Do3Think SDK")
        endif()
    endif()
    
    # Copy Do3Think DLL to output directory
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Do3ThinkCamera/SDK/DVPCamera64.dll")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/../../Do3ThinkCamera/SDK/DVPCamera64.dll"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
        )
    endif()
    
    # Deploy Qt dependencies for Windows (only in standalone mode)
    if(BUILD_STANDALONE AND DEPLOY_QT_RUNTIME)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:Qt6::Core>
        $<TARGET_FILE:Qt6::Gui>
        $<TARGET_FILE:Qt6::Quick>
        $<TARGET_FILE:Qt6::QuickControls2>
        $<TARGET_FILE:Qt6::Qml>
        $<TARGET_FILE:Qt6::Widgets>
        $<TARGET_FILE:Qt6::Concurrent>
        $<TARGET_FILE:Qt6::Charts>
        $<TARGET_FILE:Qt6::Network>
            $<TARGET_FILE_DIR:${PROJECT_NAME}>
        )
    endif()
    
    # Copy Qt platform plugin (CRITICAL for Windows!)
    if(BUILD_STANDALONE AND DEPLOY_QT_RUNTIME)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/platforms"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:Qt6::QWindowsIntegrationPlugin>"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/platforms/"
        )
    endif()
elseif(UNIX AND NOT APPLE)
    # Linux configuration
    target_link_libraries(${PROJECT_NAME} PRIVATE
        pthread
        dl
    )
    
    if(LINK_MODE STREQUAL "SOURCES")
        # Handle Do3Think SDK for Linux
        find_library(DVPCAMERA_LIB
            NAMES DVPCamera64 DVPCamera
            PATHS
                "${CMAKE_CURRENT_SOURCE_DIR}/../../Do3ThinkCamera/SDK"
                "/usr/local/lib"
                "/usr/lib"
            PATH_SUFFIXES "" x86_64-linux-gnu
        )
        
        if(DVPCAMERA_LIB)
            target_link_libraries(${PROJECT_NAME} PRIVATE ${DVPCAMERA_LIB})
            message(STATUS "QML Viewer: Found Do3Think SDK library: ${DVPCAMERA_LIB}")
        else()
            # Use dynamic loading
            target_compile_definitions(${PROJECT_NAME} PRIVATE USE_DYNAMIC_LOADING)
            message(STATUS "QML Viewer: Using dynamic loading for Do3Think SDK")
        endif()
    endif()
    
    # Set RPATH for finding libraries at runtime
    set_target_properties(${PROJECT_NAME} PROPERTIES
        INSTALL_RPATH "$ORIGIN:$ORIGIN/../lib:${CMAKE_CURRENT_SOURCE_DIR}/../../Do3ThinkCamera/SDK"
        BUILD_WITH_INSTALL_RPATH TRUE
    )
endif()

# Configure QML debugging for development builds
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        QT_QML_DEBUG
        QML_DISABLE_DISK_CACHE
    )
endif()

# Set up deployment and QML modules
if(Qt6_FOUND AND BUILD_STANDALONE AND DEPLOY_QT_RUNTIME)
    # Deploy QML modules
    qt6_import_qml_plugins(${PROJECT_NAME})
    qt_finalize_executable(${PROJECT_NAME})
    
    # Use Qt deployment tool for complete deployment
    if(WIN32 AND NOT CMAKE_CROSSCOMPILING)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_PREFIX_PATH}/bin/windeployqt 
                --qmldir ${CMAKE_CURRENT_SOURCE_DIR}/qml 
                --quick 
                --no-translations 
                --no-system-d3d-compiler 
                --no-opengl-sw 
                $<TARGET_FILE:${PROJECT_NAME}>
            COMMENT "Running windeployqt..."
        )
    elseif(APPLE)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_PREFIX_PATH}/bin/macdeployqt 
                $<TARGET_BUNDLE_DIR:${PROJECT_NAME}> 
                -qmldir=${CMAKE_CURRENT_SOURCE_DIR}/qml
            COMMENT "Running macdeployqt..."
        )
    endif()
endif()

# Install rules
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# Install QML files
install(DIRECTORY qml/
    DESTINATION bin/qml
    FILES_MATCHING PATTERN "*.qml"
)

# Create launch script for development
if(UNIX AND NOT APPLE)
    file(WRITE ${CMAKE_BINARY_DIR}/run_qml_viewer.sh
        "#!/bin/bash\n"
        "export QML2_IMPORT_PATH=${CMAKE_CURRENT_SOURCE_DIR}/qml:$QML2_IMPORT_PATH\n"
        "export QT_QUICK_CONTROLS_STYLE=Material\n"
        "./${PROJECT_NAME}\n"
    )
    
    file(CHMOD ${CMAKE_BINARY_DIR}/run_qml_viewer.sh
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
    )
endif()

# Create a configuration file for runtime settings
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/config.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/config.h"
    @ONLY
)

# Add configured header to include path
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
)

# Print configuration summary
message(STATUS "========================================")
message(STATUS "QML Camera Viewer Configuration Summary:")
message(STATUS "========================================")
message(STATUS "Qt6 Version: ${Qt6_VERSION}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Build Mode: ${LINK_MODE}")
message(STATUS "Standalone Build: ${BUILD_STANDALONE}")
message(STATUS "Deploy Qt Runtime: ${DEPLOY_QT_RUNTIME}")
message(STATUS "OpenCV Found: ${OpenCV_FOUND}")
if(OpenCV_FOUND)
    message(STATUS "OpenCV Version: ${OpenCV_VERSION}")
endif()
message(STATUS "Install Prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "QML Import Path: ${QML_IMPORT_PATH}")
message(STATUS "Output Directory: ${CMAKE_BINARY_DIR}/bin")
message(STATUS "========================================")