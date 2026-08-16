cmake_minimum_required(VERSION 3.14)

# Standalone tool: converts market-data CSV files into Cereal binary (.bin)
# files consumed by the local REST API / the client application.
# It intentionally has no Qt dependency so it can be built and run
# independently from the desktop application.

add_executable(csv_to_bin_converter
    main.cpp
)

target_include_directories(csv_to_bin_converter
    PRIVATE
        ${CMAKE_SOURCE_DIR}/local_data_server/shared
        ${CMAKE_SOURCE_DIR}/ThirdParty/cereal/include
)

target_compile_features(csv_to_bin_converter PRIVATE cxx_std_17)
