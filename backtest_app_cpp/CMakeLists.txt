cmake_minimum_required(VERSION 3.16)

# Project information
project(backtest_app_cpp VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Automatically add the current source and binary directories to the include path
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Enable automatic handling of Qt features
set(CMAKE_AUTOMOC ON)  # For meta-object compiler
set(CMAKE_AUTORCC ON)  # For resource compiler
set(CMAKE_AUTOUIC ON)  # For UI compiler

# Find Qt components
find_package(Qt6 COMPONENTS Core Gui Widgets Charts Concurrent OpenGL OpenGLWidgets PrintSupport REQUIRED)

# Find Python and pybind11
find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
find_package(pybind11 REQUIRED)

# Define the correct path to ChartDirector (one level up from the project)
set(CHARTDIR_PATH "${CMAKE_SOURCE_DIR}/..")
message(STATUS "ChartDirector path: ${CHARTDIR_PATH}/ChartDirector")

# Include directories for all targets
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CHARTDIR_PATH}/ChartDirector/include
    ${Python3_INCLUDE_DIRS}
)

# Handle ChartDirector files differently - find the actual locations
set(CHARTDIR_LOCATIONS
    "${CHARTDIR_PATH}/ChartDirector/qtdemo/qtdemo"
    "${CHARTDIR_PATH}/ChartDirector/qtdemo"
    "${CHARTDIR_PATH}/ChartDirector"
)

# Initialize variables
set(CHARTDIR_VIEWER_SRC "")
set(CHARTDIR_VIEWER_HDR "")
set(FOUND_CHARTDIR FALSE)

# Try to find the files in the possible locations
foreach(DIR ${CHARTDIR_LOCATIONS})
    if(EXISTS "${DIR}/qchartviewer.cpp" AND EXISTS "${DIR}/qchartviewer.h")
        set(CHARTDIR_VIEWER_SRC "${DIR}/qchartviewer.cpp")
        set(CHARTDIR_VIEWER_HDR "${DIR}/qchartviewer.h")
        set(FOUND_CHARTDIR TRUE)
        message(STATUS "Found ChartDirector QtDemo files in: ${DIR}")
        break()
    endif()
endforeach()

# Decide whether to include ChartDirector viewer
if(FOUND_CHARTDIR)
    # Create a symbolic link to chartdir.h in the qchartviewer directory
    get_filename_component(QCHARTVIEWER_DIR ${CHARTDIR_VIEWER_HDR} DIRECTORY)
    file(CREATE_LINK "${CHARTDIR_PATH}/ChartDirector/include/chartdir.h" "${CMAKE_BINARY_DIR}/chartdir.h" SYMBOLIC)
    
    # Add a library for the chart viewer with correct include paths
    add_library(chartviewer STATIC ${CHARTDIR_VIEWER_SRC} ${CHARTDIR_VIEWER_HDR})
    target_include_directories(chartviewer PUBLIC
        ${CHARTDIR_PATH}/ChartDirector/include
        ${CMAKE_BINARY_DIR}  # For the symbolic link to chartdir.h
    )
    
    target_link_libraries(chartviewer PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
    )
    set(CHARTDIR_FOUND TRUE)
else()
    message(WARNING "ChartDirector QtDemo files not found. Chart functionality will be limited.")
    set(CHARTDIR_FOUND FALSE)
endif()

# Specify source files (using file globbing for simplicity)
file(GLOB_RECURSE APP_SOURCES "src/*.cpp")
file(GLOB_RECURSE APP_HEADERS "src/*.h")

# Create the executable target
qt_add_executable(${PROJECT_NAME} ${APP_SOURCES} ${APP_HEADERS})

# Update include directories for the main target to include symbolic link directory
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_BINARY_DIR}  # For the symbolic link to chartdir.h
)

# Link Qt libraries and chartviewer if available
if(CHARTDIR_FOUND)
    target_link_libraries(${PROJECT_NAME} PRIVATE chartviewer)
    target_compile_definitions(${PROJECT_NAME} PRIVATE HAVE_CHARTDIR)
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Charts
    Qt6::Concurrent
    Qt6::OpenGL
    Qt6::OpenGLWidgets
    Qt6::PrintSupport
    Python3::Python
    pybind11::embed
)

# Add ChartDirector library if it exists
if(EXISTS "${CHARTDIR_PATH}/ChartDirector/lib/libchartdir.7.dylib")
    target_link_libraries(${PROJECT_NAME} PRIVATE
        ${CHARTDIR_PATH}/ChartDirector/lib/libchartdir.7.dylib
    )
    
    # Copy ChartDirector library into app bundle
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/../Frameworks"
        COMMAND ${CMAKE_COMMAND} -E copy
            "${CHARTDIR_PATH}/ChartDirector/lib/libchartdir.7.dylib"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/../Frameworks/"
    )
else()
    message(WARNING "ChartDirector library not found at ${CHARTDIR_PATH}/ChartDirector/lib/libchartdir.7.dylib")
endif()

# Set rpath to find the library at runtime
set_target_properties(${PROJECT_NAME} PROPERTIES
    INSTALL_RPATH "@executable_path/../Frameworks"
    BUILD_WITH_INSTALL_RPATH TRUE
    MACOSX_BUNDLE TRUE
)

# Configure compiler definitions from the original project
target_compile_definitions(${PROJECT_NAME} PRIVATE
    QT_CHARTS_LIB
    QT_OPENGLWIDGETS_LIB
    QT_PRINTSUPPORT_LIB
    QT_WIDGETS_LIB
    QT_OPENGL_LIB
    QT_GUI_LIB
    QT_CONCURRENT_LIB
    QT_CORE_LIB
)

# Add post-build command to run macdeployqt
find_program(MACDEPLOYQT_EXECUTABLE macdeployqt HINTS "${CMAKE_PREFIX_PATH}/bin")
if(MACDEPLOYQT_EXECUTABLE)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${MACDEPLOYQT_EXECUTABLE} "$<TARGET_FILE_DIR:${PROJECT_NAME}>/../.."
        COMMENT "Running macdeployqt..."
    )
endif()

# Copy Python library to the app bundle
if(Python3_FOUND)
    get_target_property(PYTHON_LIBRARIES Python3::Python LOCATION)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/../Frameworks"
        COMMAND ${CMAKE_COMMAND} -E copy
            "${PYTHON_LIBRARIES}"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/../Frameworks/"
        COMMENT "Copying Python library to app bundle..."
    )
    
    # Also copy pybind11 if needed (uncomment if required)
    # add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    #     COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/../Frameworks"
    #     COMMAND ${CMAKE_COMMAND} -E copy
    #         "${PYBIND11_DIR}/../../lib/libpybind11.dylib"
    #         "$<TARGET_FILE_DIR:${PROJECT_NAME}>/../Frameworks/"
    #     COMMENT "Copying pybind11 library to app bundle..."
    # )
endif()

# Add a wrapper script to set Python environment
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "#!/bin/bash" > "$<TARGET_FILE_DIR:${PROJECT_NAME}>/../Resources/wrapper.sh"
    COMMAND ${CMAKE_COMMAND} -E echo "export PYTHONHOME=\"\$HOME/miniconda3/envs/trading\"" >> "$<TARGET_FILE_DIR:${PROJECT_NAME}>/../Resources/wrapper.sh"
    COMMAND ${CMAKE_COMMAND} -E echo "export PYTHONPATH=\"\$(dirname \"\$(dirname \"\$(dirname \"\$0\")\")\")/..:\$(dirname \"\$(dirname \"\$(dirname \"\$0\")\")\")/../../igtrader:\$PYTHONPATH\"" >> "$<TARGET_FILE_DIR:${PROJECT_NAME}>/../Resources/wrapper.sh"
    COMMAND ${CMAKE_COMMAND} -E echo "export PYTHONDONTWRITEBYTECODE=1" >> "$<TARGET_FILE_DIR:${PROJECT_NAME}>/../Resources/wrapper.sh"
    COMMAND ${CMAKE_COMMAND} -E echo "exec \"\$0.bin\" \"$@\"" >> "$<TARGET_FILE_DIR:${PROJECT_NAME}>/../Resources/wrapper.sh"
    COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:${PROJECT_NAME}>" "$<TARGET_FILE:${PROJECT_NAME}>.bin"
    COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE_DIR:${PROJECT_NAME}>/../Resources/wrapper.sh" "$<TARGET_FILE:${PROJECT_NAME}>"
    COMMAND chmod +x "$<TARGET_FILE:${PROJECT_NAME}>"
    COMMENT "Creating wrapper script to set Python environment"
)

