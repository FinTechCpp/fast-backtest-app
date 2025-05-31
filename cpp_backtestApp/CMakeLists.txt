# Trouver les packages Qt nécessaires
find_package(Qt6 COMPONENTS Core Widgets Charts REQUIRED)

# Activer l'outil de méta-objets Qt (moc)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Recueillir les fichiers source et d'en-tête
file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE HEADERS "include/*.h")

# Si pour une raison quelconque qchartviewer.cpp n'est pas correctement pris en compte 
# dans la bibliothèque chartdirector, nous pouvons l'ajouter directement ici
set(QCHARTVIEWER_CPP "${CMAKE_SOURCE_DIR}/ChartDirector/qtdemo/helloworld/qchartviewer.cpp")
if(EXISTS "${QCHARTVIEWER_CPP}" AND NOT TARGET chartdirector)
    list(APPEND SOURCES "${QCHARTVIEWER_CPP}")
    message(STATUS "Ajout direct de qchartviewer.cpp aux sources")
endif()

# Créer l'exécutable
add_executable(backtestapp 
    ${SOURCES}
    ${HEADERS}
)

# Configurer les chemins d'inclusion
target_include_directories(backtestapp
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/cpp_backtestEngine/include
        ${CMAKE_SOURCE_DIR}/cpp_strategies/include
        ${CMAKE_SOURCE_DIR}/cpp_adaptator/include
        ${CMAKE_SOURCE_DIR}/ChartDirector/include
        ${CMAKE_SOURCE_DIR}/ChartDirector/qtdemo/helloworld  # Ajouté pour qchartviewer.h
)

# Lier avec Qt et les bibliothèques de backtest
target_link_libraries(backtestapp
    PRIVATE
        Qt6::Core
        Qt6::Widgets
        Qt6::Charts
        cpp_backtestEngine
        cpp_strategies
        chartdirector
)

# Installer l'exécutable et ses dépendances
install(TARGETS backtestapp
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)