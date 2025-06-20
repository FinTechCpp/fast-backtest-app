# Application IG Command Line
cmake_minimum_required(VERSION 3.14)

# Créer l'exécutable
add_executable(ig_cmd_app
    src/cmd_app.cpp
)

# Headers
target_include_directories(ig_cmd_app
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../igtrader/cpp_trading_ig/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../igtrader
)

find_package(fmt REQUIRED)

# Lier avec la bibliothèque IG trading
target_link_libraries(ig_cmd_app PRIVATE cpp_trading_ig fmt::fmt)

# Standard C++17
target_compile_features(ig_cmd_app PRIVATE cxx_std_17)

# Copier dans le dossier de build pour faciliter l'exécution
set_target_properties(ig_cmd_app PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
