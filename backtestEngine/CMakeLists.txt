# Ajouter les sources de la bibliothèque
file(GLOB SOURCES "src/*.cpp")

# Créer la bibliothèque
add_library(backtestEngine STATIC ${SOURCES})

# Configurer les chemins d'include
target_include_directories(backtestEngine 
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)