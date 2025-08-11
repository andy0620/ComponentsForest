cmake_minimum_required(VERSION 3.16)
project(OpenCVComponents VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Find required packages
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Concurrent)
find_package(OpenCV REQUIRED)

# Find optional CUDA support
find_package(CUDA QUIET)
if(CUDA_FOUND)
    add_definitions(-DHAS_CUDA)
    message(STATUS "CUDA support enabled for ROI processing")
endif()

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../components
    ${OpenCV_INCLUDE_DIRS}
)

# ROI System Library
add_library(ROISystem SHARED
    # Core ROI classes
    roi_types.h
    roi_types.cpp
    roi_manager.h
    roi_manager.cpp
    
    # Interactive widget
    roi_selector_widget.h
    roi_selector_widget.cpp
    
    # Enhanced control panel (these files will be created if needed)
    # dothink_roi_control_panel.h
    # dothink_roi_control_panel.cpp
    
    # Preprocessor base classes (now properly inheriting from BaseComponent)
    preprocessor_base.h
    preprocessor_base.cpp
    roi_preprocessor_base.h
    roi_preprocessor_base.cpp
    preprocessing_pipeline.h
    preprocessing_pipeline.cpp
    
    # New Algorithm base class
    algorithm_base.h
    algorithm_base.cpp

    # Example preprocessors
    blur_preprocessor.h
    blur_preprocessor.cpp
    edge_preprocessor.h
    edge_preprocessor.cpp
    denoise_preprocessor.h
    denoise_preprocessor.cpp
    threshold_preprocessor.h
    threshold_preprocessor.cpp

    # Example algorithms
    contour_area_algorithm.h
    contour_area_algorithm.cpp
    histogram_algorithm.h
    histogram_algorithm.cpp

    # More Preprocessors
    morphology_preprocessor.h
    morphology_preprocessor.cpp
)

# Link libraries
target_link_libraries(ROISystem
    Qt6::Core
    Qt6::Widgets
    Qt6::Concurrent
    ${OpenCV_LIBS}
    ComponentsForestCore  # Link to base components
    Do3ThinkCameraComponent  # Link to camera component
)

if(CUDA_FOUND)
    target_link_libraries(ROISystem ${CUDA_LIBRARIES})
endif()

# Set properties
set_target_properties(ROISystem PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    WINDOWS_EXPORT_ALL_SYMBOLS ON
)

# Installation rules
install(TARGETS ROISystem
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
    ARCHIVE DESTINATION lib
)

# Install headers
install(FILES
    roi_types.h
    roi_manager.h
    roi_selector_widget.h
    preprocessor_base.h
    roi_preprocessor_base.h
    preprocessing_pipeline.h
    algorithm_base.h
    threshold_preprocessor.h
    contour_area_algorithm.h
    morphology_preprocessor.h
    histogram_algorithm.h
    DESTINATION include/ComponentsForest/OpenCV
)

# Example application (optional)
option(BUILD_ROI_EXAMPLES "Build ROI example applications" ON)
if(BUILD_ROI_EXAMPLES)
    add_executable(ROIExample
        examples/roi_example.cpp
    )
    
    target_link_libraries(ROIExample
        ROISystem
        Qt6::Core
        Qt6::Widgets
        ${OpenCV_LIBS}
    )
    
    # Test for new preprocessor architecture (pending full integration)
    # add_executable(PreprocessorTest
    #     test_new_preprocessor.cpp
    # )
    
    # target_link_libraries(PreprocessorTest
    #     ROISystem
    #     ComponentsForestCore
    #     Qt6::Core
    #     Qt6::Widgets
    #     ${OpenCV_LIBS}
    # )
endif()

# Display configuration summary
message(STATUS "")
message(STATUS "OpenCV ROI System Configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  OpenCV Version: ${OpenCV_VERSION}")
message(STATUS "  CUDA Support: ${CUDA_FOUND}")
message(STATUS "  Build Examples: ${BUILD_ROI_EXAMPLES}")
message(STATUS "")