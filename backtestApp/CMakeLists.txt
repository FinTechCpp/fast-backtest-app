# Find necessary packages for Qt6
find_package(Qt6 COMPONENTS Core Widgets Charts Network REQUIRED)

# Enable the Qt meta-object compiler (moc)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Gather source and header files
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.cpp")
file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS "include/*.h")

# Create the executable
add_executable(backtestapp 
    ${SOURCES}
    ${HEADERS}
    ${CMAKE_CURRENT_SOURCE_DIR}/qresources.qrc
)

# Configure the main include paths - Modified section
target_include_directories(backtestapp
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/ThirdParty/fast-backtest-engine/include
        ${CMAKE_SOURCE_DIR}/ThirdParty/Strategies/include 
        ${CMAKE_SOURCE_DIR}/ThirdParty/cereal/include 
        ${CMAKE_SOURCE_DIR}/local_data_server/shared
        ${CMAKE_SOURCE_DIR}/ChartDirector/include
        ${CMAKE_SOURCE_DIR}/ChartDirector/src
        ${CMAKE_CURRENT_BINARY_DIR}
)

# Additional include path for internal components
target_include_directories(backtestapp
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

# Read the version from the VERSION file or environment variable
if(DEFINED ENV{VERSION})
    set(APP_VERSION $ENV{VERSION})
else()
    file(READ "${CMAKE_SOURCE_DIR}/VERSION" APP_VERSION)
    string(STRIP "${APP_VERSION}" APP_VERSION)
endif()

# Create a configuration file
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/src/version.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/version.h"
)

# Add the build directory to the includes
target_include_directories(backtestapp PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

# Link with Qt and backtest libraries
target_link_libraries(backtestapp
    PRIVATE
        Qt6::Core
        Qt6::Widgets
        Qt6::Charts
        Qt6::Network
        backtestEngine
        strategies
        chartdirector
)

# For Windows compilation (native or cross-compilation)
if(WIN32 OR CMAKE_CROSSCOMPILING)
    # No windeployqt in cross-compilation, libs are static with MXE
    if(NOT CMAKE_CROSSCOMPILING)
        # Find the windeployqt tool
        get_target_property(QT6_QMAKE_EXECUTABLE Qt6::qmake IMPORTED_LOCATION)
        get_filename_component(QT6_WINDEPLOYQT_EXECUTABLE ${QT6_QMAKE_EXECUTABLE} PATH)
        set(QT6_WINDEPLOYQT_EXECUTABLE "${QT6_WINDEPLOYQT_EXECUTABLE}/windeployqt.exe")

        # Check if windeployqt exists
        if(EXISTS ${QT6_WINDEPLOYQT_EXECUTABLE})
            # Command to deploy Qt after compilation
            # Use $<CONFIG> to automatically select --release or --debug based on build type
            add_custom_command(TARGET backtestapp POST_BUILD
                COMMAND ${QT6_WINDEPLOYQT_EXECUTABLE} 
                    --qmldir ${CMAKE_SOURCE_DIR}
                    $<$<CONFIG:Debug>:--debug>
                    $<$<CONFIG:Release>:--release>
                    $<$<CONFIG:RelWithDebInfo>:--release>
                    $<$<CONFIG:MinSizeRel>:--release>
                    $<TARGET_FILE:backtestapp>
                COMMENT "Deploying Qt DLLs with windeployqt")
        else()
            message(WARNING "windeployqt not found. Qt DLLs will need to be copied manually.")

            # Alternative: copy Qt main DLLs manually
            set(QT_RUNTIME_PATH "${Qt6_DIR}/../../../bin")
            set(QT_DLLS
                Qt6Core.dll
                Qt6Gui.dll
                Qt6Widgets.dll
                Qt6Charts.dll
            )
            
            foreach(DLL ${QT_DLLS})
                if(EXISTS "${QT_RUNTIME_PATH}/${DLL}")
                    add_custom_command(TARGET backtestapp POST_BUILD
                        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${QT_RUNTIME_PATH}/${DLL}"
                        "$<TARGET_FILE_DIR:backtestapp>"
                        COMMENT "Copying ${DLL}"
                    )
                endif()
            endforeach()
        endif()
    endif()
    
    # Copy the ChartDirector DLL if it exists (fixed for the correct path)
    set(CHARTDIR_DLL "${CMAKE_SOURCE_DIR}/ThirdParty/ChartDirector/lib/chartdir70.dll")
    if(EXISTS "${CHARTDIR_DLL}")
        add_custom_command(TARGET backtestapp POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CHARTDIR_DLL}"
            "$<TARGET_FILE_DIR:backtestapp>"
            COMMENT "Copy chartdir70.dll to the executable directory"
        )
    endif()
endif()

# Install the executable and its dependencies
install(TARGETS backtestapp
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
)