cmake_minimum_required(VERSION 3.14)
project(ig_trading_bot VERSION 1.0)

# Global configuration
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build options
option(BUILD_WITH_DEBUG "Compiling with debug symbols" OFF)

if(BUILD_WITH_DEBUG)
    set(CMAKE_BUILD_TYPE Debug)
    
    # Use appropriate flags based on the compiler
    if(MSVC)
        add_compile_options(/Od /Zi)
    else()
        add_compile_options(-g -O0)
    endif()
else()
    set(CMAKE_BUILD_TYPE Release)
    
    # Use appropriate flags based on the compiler
    if(MSVC)
        add_compile_options(/O2)
    else()
        add_compile_options(-O3)
        # add_compile_options(-march=native)
    endif()
endif()

# Cross-compilation detection for MXE
if(CMAKE_CROSSCOMPILING)
    message(STATUS "ross-compilation detected")
    message(STATUS "Target system: ${CMAKE_SYSTEM_NAME}")
    message(STATUS "Target architecture: ${CMAKE_SYSTEM_PROCESSOR}")

    # For MXE, Qt6 is automatically found
    if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
        message(STATUS "MXE configuration detected for Windows")
    endif()
else()
    # Normal configuration for local builds
    # Configure Qt6 path based on the operating system
    if(APPLE)
        # Try to find Qt automatically in standard locations
        set(QT_SEARCH_PATHS
            "$ENV{HOME}/Qt/*/macos"           # Standard user installation
            "/opt/Qt/*/macos"                 # Alternative system installation
            "/usr/local/opt/qt@6"             # Homebrew Qt6
            "/opt/homebrew/opt/qt@6"          # Homebrew on Apple Silicon
        )
        
        # Look for the most recent version of Qt6
        file(GLOB QT_CANDIDATES ${QT_SEARCH_PATHS})
        if(QT_CANDIDATES)
            # Sort to have the most recent version first
            list(SORT QT_CANDIDATES)
            list(REVERSE QT_CANDIDATES)
            list(GET QT_CANDIDATES 0 QT_PATH)
            list(APPEND CMAKE_PREFIX_PATH "${QT_PATH}")
            message(STATUS "Qt6 detected: ${QT_PATH}")
        else()
            message(WARNING "Qt6 not found automatically. Use -DCMAKE_PREFIX_PATH=<path_to_qt> or install Qt via Homebrew: brew install qt@6")
        endif()
    elseif(WIN32)
        # Windows configuration - try to find Qt automatically in standard locations
        # TODO : Improve search methods for Qt6 on Windows
        set(QT_SEARCH_PATHS
            "C:/Qt/*/msvc2022_64"             # Standard Qt installation with MSVC 2022
            "C:/Qt/*/msvc2019_64"             # Standard Qt installation with MSVC 2019
            "$ENV{USERPROFILE}/Qt/*/msvc2022_64"  # User installation
            "$ENV{QTDIR}"                     # QTDIR environment variable
            "$ENV{QT_ROOT_DIR}"               # Variable used by GitHub Actions
            "D:/a/*/Qt/*/msvc2022_64"         # GitHub Actions path
        )
        # Look for the most recent version of Qt6
        file(GLOB QT_CANDIDATES ${QT_SEARCH_PATHS})
        if(QT_CANDIDATES)
            # Sort to have the most recent version first
            list(SORT QT_CANDIDATES)
            list(REVERSE QT_CANDIDATES)
            list(GET QT_CANDIDATES 0 QT_PATH)
            list(APPEND CMAKE_PREFIX_PATH "${QT_PATH}")
            message(STATUS "Qt6 detected: ${QT_PATH}")
        else()
            # After the QT_CANDIDATES check, replace the existing warning with:
            if(DEFINED ENV{Qt6_DIR})
                list(APPEND CMAKE_PREFIX_PATH "$ENV{Qt6_DIR}")
                message(STATUS "Using Qt6_DIR from environment: $ENV{Qt6_DIR}")
            elseif(DEFINED ENV{QT_ROOT_DIR})
                list(APPEND CMAKE_PREFIX_PATH "$ENV{QT_ROOT_DIR}")
                message(STATUS "Using QT_ROOT_DIR from environment: $ENV{QT_ROOT_DIR}")
            else()
                message(WARNING "Qt6 not found automatically. Ensure Qt6 is installed in C:/Qt/ or set CMAKE_PREFIX_PATH")
            endif()
        endif()
    elseif(UNIX AND NOT APPLE)
        # Linux configuration - try to find Qt automatically in standard locations
        set(QT_SEARCH_PATHS
            "$ENV{HOME}/Qt/*/gcc_64"          # User installation
            "/usr/lib/qt6"                    # System installation
            # "/usr/lib/x86_64-linux-gnu/qt6"   # Other system location
        )

        # Look for the most recent version of Qt6
        file(GLOB QT_CANDIDATES ${QT_SEARCH_PATHS})
        if(QT_CANDIDATES)
            # Sort to have the most recent version first
            list(SORT QT_CANDIDATES)
            list(REVERSE QT_CANDIDATES)
            list(GET QT_CANDIDATES 0 QT_PATH)
            list(APPEND CMAKE_PREFIX_PATH "${QT_PATH}")
            message(STATUS "Qt6 detected: ${QT_PATH}")
        else()
            # Try to retrieve from the environment variable
            if(DEFINED ENV{CMAKE_PREFIX_PATH})
                list(APPEND CMAKE_PREFIX_PATH "$ENV{CMAKE_PREFIX_PATH}")
                message(STATUS "Using CMAKE_PREFIX_PATH from environment: $ENV{CMAKE_PREFIX_PATH}")
            else()
                message(WARNING "Qt6 not found automatically. Use -DCMAKE_PREFIX_PATH=~/Qt/6.8.3/gcc_64")
            endif()
        endif()
    endif()
endif()

# Check if Qt6 is found (Mandatory)
find_package(Qt6 COMPONENTS Core Widgets Charts Network REQUIRED)

# git submodules 
add_subdirectory(ThirdParty/spdlog)
# include_directories(SYSTEM ${CMAKE_SOURCE_DIR}/ThirdParty/cereal/include)
add_subdirectory(ThirdParty/Strategies)
add_subdirectory(ThirdParty/ChartDirector)

# Configure CPR options before adding subdirectory
set(CPR_USE_SYSTEM_CURL OFF CACHE BOOL "" FORCE)
set(CPR_CURL_USE_LIBPSL OFF CACHE BOOL "" FORCE)
set(CPR_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(CPR_BUILD_TESTS_SSL OFF CACHE BOOL "" FORCE)
set(CPR_ENABLE_SSL ON CACHE BOOL "" FORCE)
add_subdirectory(ThirdParty/cpr)

# Add sub-projects (libraries)
add_subdirectory(ThirdParty/fast-backtest-engine)
add_subdirectory(backtestApp)

# Show a summary of the configuration
message(STATUS "=========================================")
message(STATUS "Configuration for ${PROJECT_NAME} v${PROJECT_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Debug symbols: ${BUILD_WITH_DEBUG}")
if(CMAKE_CROSSCOMPILING)
    message(STATUS "Target: ${CMAKE_SYSTEM_NAME} ${CMAKE_SYSTEM_PROCESSOR}")
endif()
message(STATUS "=========================================")