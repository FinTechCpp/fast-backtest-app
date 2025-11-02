cmake_minimum_required(VERSION 3.14)
project(chartdirector)

message(STATUS "ChartDirector configuration...")

# On windows, we use a static library
# On other platforms, we can use SHARED
if(WIN32)
    add_library(chartdirector STATIC 
        "${CMAKE_CURRENT_SOURCE_DIR}/src/qchartviewer.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/qchartviewer.h"
    )
else()
    add_library(chartdirector SHARED 
        "${CMAKE_CURRENT_SOURCE_DIR}/src/qchartviewer.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/qchartviewer.h"
    )
endif()

# Configure include paths
target_include_directories(chartdirector 
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Adaptative detection based on platform
if(APPLE)
    set(CHARTDIR_LIB "${CMAKE_CURRENT_SOURCE_DIR}/lib/libchartdir.7.dylib")
elseif(WIN32)
    set(CHARTDIR_LIB "${CMAKE_CURRENT_SOURCE_DIR}/lib/chartdir70.lib")
    # On Windows, we also need the DLL at runtime
    set(CHARTDIR_DLL "${CMAKE_CURRENT_SOURCE_DIR}/lib/chartdir70.dll")
else()
    set(CHARTDIR_LIB "${CMAKE_CURRENT_SOURCE_DIR}/lib/libchartdir.so.7.0")
endif()

# Check if the library exists
if(NOT EXISTS "${CHARTDIR_LIB}")
    message(WARNING "ChartDirector library not found at ${CHARTDIR_LIB}")
else()
    message(STATUS "ChartDirector library found: ${CHARTDIR_LIB}")
endif()

# Qt configuration
find_package(Qt6 COMPONENTS Core Widgets REQUIRED)

# Enable MOC for qchartviewer.cpp
set_target_properties(chartdirector PROPERTIES 
    AUTOMOC ON
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# Link the found library
target_link_libraries(chartdirector 
    PUBLIC
    ${CHARTDIR_LIB} 
    Qt6::Core 
    Qt6::Widgets
)

# On Windows, copy the ChartDirector DLL to the output folder
if(WIN32 AND EXISTS "${CHARTDIR_DLL}")
    # Copy the DLL to all possible output folders
    add_custom_command(TARGET chartdirector POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CHARTDIR_DLL}"
        "$<TARGET_FILE_DIR:chartdirector>"
        COMMENT "Copying chartdir70.dll"
    )

    # Install the DLL with the application
    install(FILES "${CHARTDIR_DLL}" 
            DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

# Define a variable to indicate that ChartDirector is configured
set(CHARTDIRECTOR_FOUND TRUE CACHE BOOL "ChartDirector found and configured" FORCE)