cmake_minimum_required(VERSION 3.10)
project(cpp_backtestEngine VERSION 1.0)

# Spécifier le standard C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Ajouter les fichiers source
file(GLOB SOURCES "src/*.cpp")

# Créer une bibliothèque statique
add_library(cpp_backtestEngine STATIC ${SOURCES})

# Spécifier les chemins d'inclusion
target_include_directories(cpp_backtestEngine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Ajouter l'exécutable de démo
add_executable(backtest_demo demo.cpp)
target_link_libraries(backtest_demo PRIVATE cpp_backtestEngine)