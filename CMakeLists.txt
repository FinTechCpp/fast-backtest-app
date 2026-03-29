cmake_minimum_required(VERSION 3.14)
project(fast-backtest-app VERSION 1.0)

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

    # In Debug mode, logging is ENABLED (no DISABLE_LOGGING definition)
    message(STATUS "Debug mode: Logging ENABLED")
else()
    set(CMAKE_BUILD_TYPE Release)
    
    # Use appropriate flags based on the compiler
    if(MSVC)
        # Maximum speed optimization: /O2 (maximize speed) + /fp:fast (fast floating-point)
        # /arch:AVX2 enables SIMD auto-vectorization (requires CPU from 2013+: Intel Haswell, AMD Excavator)
        # /Zi for debug symbols (crash diagnostics), /GL+/LTCG for whole program optimization
        set(CMAKE_CXX_FLAGS_RELEASE "/O2 /fp:fast /arch:AVX2 /GL /Zi /DNDEBUG" CACHE STRING "MSVC Release flags" FORCE)
        set(CMAKE_EXE_LINKER_FLAGS_RELEASE "/LTCG /DEBUG" CACHE STRING "MSVC Linker Release flags" FORCE)
        message(STATUS "MSVC Release flags: ${CMAKE_CXX_FLAGS_RELEASE}")
        message(STATUS "MSVC Linker Release flags: ${CMAKE_EXE_LINKER_FLAGS_RELEASE}")
    else()
        # Maximum speed optimization: -Ofast (aggressive optimizations) + LTO
        add_compile_options(-Ofast -march=native -flto)
        add_link_options(-flto)
        message(STATUS "GCC/Clang Release flags: -Ofast -march=native -flto")
    endif()

    # In Release mode, logging is DISABLED (DISABLE_LOGGING defined)
    add_compile_definitions(DISABLE_LOGGING)
    message(STATUS "Release mode: Logging DISABLED")
endif()

# Cross-compilation detection for MXE
if(CMAKE_CROSSCOMPILING)
    message(STATUS "Cross-compilation detected")
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

# On modern macOS, the AGL framework is deprecated and removed.
# We need to prevent Qt's FindWrapOpenGL from trying to link AGL.
# By setting WrapOpenGL_AGL to NOTFOUND before Qt6 is found, and creating
# the WrapOpenGL::WrapOpenGL target ourselves, we avoid the issue.
if(APPLE)
    # Find OpenGL without AGL
    find_package(OpenGL)
    if(OpenGL_FOUND AND NOT TARGET WrapOpenGL::WrapOpenGL)
        add_library(WrapOpenGL::WrapOpenGL INTERFACE IMPORTED)
        # Only link OpenGL framework, not the deprecated AGL
        target_link_libraries(WrapOpenGL::WrapOpenGL INTERFACE "-framework OpenGL")
        set(WrapOpenGL_FOUND ON CACHE BOOL "WrapOpenGL found" FORCE)
    endif()
endif()

# Check if Qt6 is found (Mandatory)
find_package(Qt6 COMPONENTS Core Widgets Charts Network REQUIRED)

# Embedded Lua runtime (no external installation required on end-user machines)
include(FetchContent)

FetchContent_Declare(
    lua_source
    GIT_REPOSITORY https://github.com/lua/lua.git
    GIT_TAG v5.4.7
)

FetchContent_GetProperties(lua_source)
if(NOT lua_source_POPULATED)
    FetchContent_Populate(lua_source)
endif()

if(NOT TARGET lua_vendor)
    add_library(lua_vendor STATIC
        ${lua_source_SOURCE_DIR}/lapi.c
        ${lua_source_SOURCE_DIR}/lauxlib.c
        ${lua_source_SOURCE_DIR}/lbaselib.c
        ${lua_source_SOURCE_DIR}/lcode.c
        ${lua_source_SOURCE_DIR}/lcorolib.c
        ${lua_source_SOURCE_DIR}/lctype.c
        ${lua_source_SOURCE_DIR}/ldblib.c
        ${lua_source_SOURCE_DIR}/ldebug.c
        ${lua_source_SOURCE_DIR}/ldo.c
        ${lua_source_SOURCE_DIR}/ldump.c
        ${lua_source_SOURCE_DIR}/lfunc.c
        ${lua_source_SOURCE_DIR}/lgc.c
        ${lua_source_SOURCE_DIR}/linit.c
        ${lua_source_SOURCE_DIR}/liolib.c
        ${lua_source_SOURCE_DIR}/llex.c
        ${lua_source_SOURCE_DIR}/lmathlib.c
        ${lua_source_SOURCE_DIR}/lmem.c
        ${lua_source_SOURCE_DIR}/loadlib.c
        ${lua_source_SOURCE_DIR}/lobject.c
        ${lua_source_SOURCE_DIR}/lopcodes.c
        ${lua_source_SOURCE_DIR}/loslib.c
        ${lua_source_SOURCE_DIR}/lparser.c
        ${lua_source_SOURCE_DIR}/lstate.c
        ${lua_source_SOURCE_DIR}/lstring.c
        ${lua_source_SOURCE_DIR}/lstrlib.c
        ${lua_source_SOURCE_DIR}/ltable.c
        ${lua_source_SOURCE_DIR}/ltablib.c
        ${lua_source_SOURCE_DIR}/ltm.c
        ${lua_source_SOURCE_DIR}/lundump.c
        ${lua_source_SOURCE_DIR}/lutf8lib.c
        ${lua_source_SOURCE_DIR}/lvm.c
        ${lua_source_SOURCE_DIR}/lzio.c
    )

    target_include_directories(lua_vendor PUBLIC ${lua_source_SOURCE_DIR})

    if(WIN32)
        target_compile_definitions(lua_vendor PRIVATE LUA_USE_WINDOWS _CRT_SECURE_NO_WARNINGS)
    elseif(APPLE)
        target_compile_definitions(lua_vendor PRIVATE LUA_USE_MACOSX)
    elseif(UNIX)
        target_compile_definitions(lua_vendor PRIVATE LUA_USE_LINUX)
        target_link_libraries(lua_vendor PUBLIC ${CMAKE_DL_LIBS} m)
    endif()
endif()

# git submodules 
add_subdirectory(ThirdParty/spdlog)
# include_directories(SYSTEM ${CMAKE_SOURCE_DIR}/ThirdParty/cereal/include)
add_subdirectory(ThirdParty/Strategies)
add_subdirectory(ThirdParty/ChartDirector)

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