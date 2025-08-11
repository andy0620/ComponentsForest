cmake_minimum_required(VERSION 3.16)
project(Do3ThinkCameraViewer VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Find required Qt6 components
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Concurrent Network Charts)

# OpenCV is optional for the viewer
find_package(OpenCV QUIET)
if(OpenCV_FOUND)
    message(STATUS "Do3Think Viewer: OpenCV found, ROI features enabled")
else()
    message(STATUS "Do3Think Viewer: OpenCV not found, ROI features disabled")
endif()

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_SOURCE_DIR}/../../components
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Do3ThinkCamera
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Do3ThinkCamera/SDK
    ${CMAKE_CURRENT_SOURCE_DIR}/../../OpenCV
    ${CMAKE_CURRENT_SOURCE_DIR}/../../build  # For generated export headers
)

# Compile definitions for dynamic loading
add_compile_definitions(USE_DYNAMIC_LOADING)
add_compile_definitions(COMPONENTSFORESTCORE_STATIC_DEFINE)
add_compile_definitions(DO3THINKCAMERACOMPONENT_STATIC_DEFINE)

# All source files including dependencies
set(ALL_SOURCES
    # Viewer sources
    main.cpp
    main_ui.cpp
    # main_ui_roi.cpp  # Temporarily disabled due to API mismatches
    machine.cpp
    
    # Theme sources
    futuristic_theme.cpp
    futuristic_widgets.cpp
    material_theme.cpp
    
    # Component sources
    ../../components/base_component.cpp
    ../../components/camera_component.cpp
    ../../components/camera_control_panel.cpp
    
    # Do3Think sources
    ../../Do3ThinkCamera/dothink_camera.cpp
    ../../Do3ThinkCamera/dothink_camera_control_panel.cpp
    ../../Do3ThinkCamera/dothink_camera_control_panel_methods.cpp
    # ../../Do3ThinkCamera/dothink_camera_roi_integration.cpp  # Temporarily disabled due to API mismatches
    ../../Do3ThinkCamera/dvp_wrapper.cpp
)

# Add OpenCV-dependent sources only if OpenCV is found
if(OpenCV_FOUND)
    list(APPEND ALL_SOURCES
        # Preprocessor sources
        ../../OpenCV/preprocessor_base.cpp
        ../../OpenCV/preprocessor_control_panel.cpp
        ../../OpenCV/simple_edge_preprocessor.cpp
        # ../../OpenCV/preprocessing_pipeline.cpp
        # ../../OpenCV/blur_preprocessor.cpp
        # ../../OpenCV/edge_preprocessor.cpp
        # ../../OpenCV/denoise_preprocessor.cpp
        
        # ROI System sources - temporarily disabled due to API mismatches
        # ../../OpenCV/roi_types.cpp
        # ../../OpenCV/roi_manager.cpp
        # ../../OpenCV/roi_selector_widget.cpp
        # ../../OpenCV/roi_preprocessor_base.cpp
    )
endif()

# All headers
set(ALL_HEADERS
    main_ui.h
    machine.h
    
    # Theme headers
    futuristic_theme.h
    futuristic_widgets.h
    modern_theme.h
    modern_control_widgets.h
    material_theme.h
    
    ../../components/base_component.h
    ../../components/camera_component.h
    ../../components/camera_control_panel.h
    
    ../../Do3ThinkCamera/dothink_camera.h
    ../../Do3ThinkCamera/dothink_camera_control_panel.h
    # ../../Do3ThinkCamera/dothink_camera_roi_integration.h  # Temporarily disabled due to API mismatches
    ../../Do3ThinkCamera/dvp_wrapper.h
)

# Add OpenCV-dependent headers only if OpenCV is found
if(OpenCV_FOUND)
    list(APPEND ALL_HEADERS
        # Preprocessor headers
        ../../OpenCV/preprocessor_base.h
        ../../OpenCV/preprocessor_control_panel.h
        ../../OpenCV/simple_edge_preprocessor.h
        # ../../OpenCV/preprocessing_pipeline.h
        # ../../OpenCV/blur_preprocessor.h
        # ../../OpenCV/edge_preprocessor.h
        # ../../OpenCV/denoise_preprocessor.h
        
        # ROI System headers - temporarily disabled due to API mismatches
        # ../../OpenCV/roi_types.h
        # ../../OpenCV/roi_manager.h
        # ../../OpenCV/roi_selector_widget.h
        # ../../OpenCV/roi_preprocessor_base.h
    )
endif()

# Create executable with all sources
add_executable(Do3ThinkCameraViewerStandalone
    ${ALL_SOURCES}
    ${ALL_HEADERS}
)

# Link libraries
target_link_libraries(Do3ThinkCameraViewerStandalone
    Qt6::Core
    Qt6::Widgets
    Qt6::Concurrent
    Qt6::Network
    Qt6::Charts
)

# Link OpenCV libraries only if found
if(OpenCV_FOUND)
    target_link_libraries(Do3ThinkCameraViewerStandalone ${OpenCV_LIBS})
    target_compile_definitions(Do3ThinkCameraViewerStandalone PRIVATE HAS_OPENCV)
endif()

# Set executable properties
set_target_properties(Do3ThinkCameraViewerStandalone PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

# Copy DVPCamera64.dll if it exists
if(WIN32)
    # Copy DVPCamera DLL from common locations
    set(DVP_SEARCH_PATHS
        "${CMAKE_CURRENT_SOURCE_DIR}/../../Do3ThinkCamera/SDK"
        "C:/Program Files (x86)/Camera/DVP2 x64"
        "C:/Program Files (x86)/Camera/DVP2"
    )
    
    foreach(path ${DVP_SEARCH_PATHS})
        if(EXISTS "${path}/DVPCamera64.dll")
            add_custom_command(TARGET Do3ThinkCameraViewerStandalone POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${path}/DVPCamera64.dll"
                    $<TARGET_FILE_DIR:Do3ThinkCameraViewerStandalone>
            )
            break()
        endif()
    endforeach()
    
    # Copy Qt DLLs
    add_custom_command(TARGET Do3ThinkCameraViewerStandalone POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:Qt6::Core>
            $<TARGET_FILE:Qt6::Widgets>
            $<TARGET_FILE:Qt6::Gui>
            $<TARGET_FILE:Qt6::Concurrent>
            $<TARGET_FILE:Qt6::Network>
            $<TARGET_FILE:Qt6::Charts>
            $<TARGET_FILE_DIR:Do3ThinkCameraViewerStandalone>
    )
endif()