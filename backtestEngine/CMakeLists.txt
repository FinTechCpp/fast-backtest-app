# Add src files to the library
file(GLOB SOURCES "src/*.cpp")

# Create the library
add_library(backtestEngine STATIC ${SOURCES})

# Configure the include paths
target_include_directories(backtestEngine 
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Add spdlog for headers that use it
target_link_libraries(backtestEngine PUBLIC spdlog::spdlog)