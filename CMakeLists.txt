cmake_minimum_required(VERSION 3.10)
project(ig_trading_bot VERSION 1.0)

# Configuration globale
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Options de build
option(BUILD_WITH_DEBUG "Compiler avec les symboles de débogage" OFF)

if(BUILD_WITH_DEBUG)
    set(CMAKE_BUILD_TYPE Debug)
    add_compile_options(-g -O0)
else()
    set(CMAKE_BUILD_TYPE Release)
    add_compile_options(-O3)
endif()

# Ajouter les sous-projets (bibliothèques)
add_subdirectory(cpp_backtestEngine)
add_subdirectory(cpp_strategies)
add_subdirectory(cpp_adaptator)

# Afficher un résumé de la configuration
message(STATUS "=========================================")
message(STATUS "Configuration pour ${PROJECT_NAME} v${PROJECT_VERSION}")
message(STATUS "Mode de build: ${CMAKE_BUILD_TYPE}")
message(STATUS "Standard C++: ${CMAKE_CXX_STANDARD}")
message(STATUS "Symboles de débogage: ${BUILD_WITH_DEBUG}")
message(STATUS "=========================================")