# Sources pour l'adaptateur
file(GLOB SOURCES "src/*.cpp")

# Créer l'exécutable principal
add_executable(main ${SOURCES})

# Lier avec les bibliothèques requises
target_link_libraries(main
    PRIVATE
        backtestEngine
        cpp_strategies
)

# Configurer les chemins d'inclusion
target_include_directories(main
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/backtestEngine/include
        ${CMAKE_SOURCE_DIR}/cpp_strategies/include
)

# Copier les fichiers de données s'ils existent
if(EXISTS ${CMAKE_SOURCE_DIR}/data)
    file(COPY ${CMAKE_SOURCE_DIR}/data
         DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
         PATTERN "*.csv")
endif()