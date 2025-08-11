cmake_minimum_required(VERSION 3.16)
project(ComponentsForestTests)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Test)
find_package(GTest REQUIRED)
find_package(OpenCV REQUIRED)

# Enable automoc for Qt
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

# BaseComponent Test
add_executable(test_base_component
    test_base_component.cpp
)

target_link_libraries(test_base_component
    ComponentsForestCore
    Qt6::Core
    Qt6::Test
    gtest_main
)

add_test(NAME BaseComponentTest COMMAND test_base_component)


# Preprocessor UI Test
add_executable(test_preprocessor_ui
    test_preprocessor_ui.cpp
    ../viewers/do3think_camera_viewer/main_ui.cpp
    ../OpenCV/preprocessor_base.cpp
    ../OpenCV/preprocessor_control_panel.cpp
    ../OpenCV/simple_edge_preprocessor.cpp
)

target_link_libraries(test_preprocessor_ui
    ComponentsForestCore
    Qt6::Core
    Qt6::Widgets
    Qt6::Test
    gtest_main
    ${OpenCV_LIBS}
)

# Add test
add_test(NAME PreprocessorUITest COMMAND test_preprocessor_ui)


# Do3ThinkCameraComponent Test
add_executable(test_dothink_camera
    test_dothink_camera.cpp
)

target_link_libraries(test_dothink_camera
    Do3ThinkCameraComponent
    ComponentsForestCore
    Qt6::Core
    Qt6::Test
    gmock_main
)

add_test(NAME Do3ThinkCameraComponentTest COMMAND test_dothink_camera)


# Enable testing
enable_testing()

# Copy Qt platform plugins for test execution
if(WIN32)
    add_custom_command(TARGET test_preprocessor_ui POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory 
            "$<TARGET_FILE_DIR:test_preprocessor_ui>/platforms"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${Qt6_DIR}/../../../plugins/platforms/qwindows$<$<CONFIG:Debug>:d>.dll"
            "$<TARGET_FILE_DIR:test_preprocessor_ui>/platforms/"
    )
endif()