# CMakeLists.txt
cmake_minimum_required(VERSION 3.10)
project(cpp_strategies)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Chercher pybind11
find_package(pybind11 REQUIRED)

# Ajouter les répertoires d'en-têtes
include_directories(include)

# Ajouter les sources de la bibliothèque
set(SOURCES 
    src/Strategy.cpp
    src/BuyHeikinGreen.cpp
    src/Indicators.cpp
)

# Créer la bibliothèque
add_library(cpp_strategies_lib STATIC ${SOURCES})

# Créer le module Python
pybind11_add_module(cpp_strategies bindings/bindings.cpp)
target_link_libraries(cpp_strategies PRIVATE cpp_strategies_lib)