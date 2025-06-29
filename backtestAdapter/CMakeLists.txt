# Sources for adapter
file(GLOB SOURCES "src/*.cpp")

# Create the main executable
add_executable(main ${SOURCES})

# Link with the required libraries
target_link_libraries(main
    PRIVATE
        backtestEngine
        strategies
        spdlog::spdlog
)

# Configure include paths
target_include_directories(main
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/backtestEngine/include
        ${CMAKE_SOURCE_DIR}/strategies/include
)

# Copy data files if they exist
if(EXISTS ${CMAKE_SOURCE_DIR}/data)
    file(COPY ${CMAKE_SOURCE_DIR}/data
         DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
         PATTERN "*.csv")
endif()