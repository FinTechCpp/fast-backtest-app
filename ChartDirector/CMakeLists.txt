cmake_minimum_required(VERSION 3.14)
project(chartdirector)

# Afficher des informations de diagnostic
message(STATUS "Configuration de ChartDirector...")

# Utiliser une bibliothèque normale au lieu d'une interface pour pouvoir y ajouter qchartviewer.cpp
# Modification : changé INTERFACE à SHARED
add_library(chartdirector SHARED 
    "${CMAKE_CURRENT_SOURCE_DIR}/qtdemo/helloworld/qchartviewer.cpp"
)

# Configurer les chemins d'inclusion
target_include_directories(chartdirector 
    # Modification : PUBLIC au lieu de INTERFACE pour que backtestapp puisse les voir
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/qtdemo/qtdemo
    ${CMAKE_CURRENT_SOURCE_DIR}/qtdemo/helloworld  # Ajout du dossier contenant qchartviewer.h
)

# Les bibliothèques ont été trouvées à ces emplacements spécifiques
# Détection adaptative selon le système d'exploitation
if(APPLE)
    set(CHARTDIR_LIB "${CMAKE_CURRENT_SOURCE_DIR}/lib/libchartdir.7.dylib")
    set(CHARTDIR_QTDEMO_LIB "${CMAKE_CURRENT_SOURCE_DIR}/qtdemo/libchartdir.dylib")
elseif(WIN32)
    set(CHARTDIR_LIB "${CMAKE_CURRENT_SOURCE_DIR}/lib/chartdir70.lib")
    set(CHARTDIR_QTDEMO_LIB "${CMAKE_CURRENT_SOURCE_DIR}/qtdemo/chartdir70.lib")
else()
    set(CHARTDIR_LIB "${CMAKE_CURRENT_SOURCE_DIR}/lib/libchartdir.so")
    set(CHARTDIR_QTDEMO_LIB "${CMAKE_CURRENT_SOURCE_DIR}/qtdemo/libchartdir.so")
endif()

# Vérifier que la bibliothèque principale existe
if(NOT EXISTS "${CHARTDIR_LIB}")
    message(WARNING "Bibliothèque principale non trouvée à ${CHARTDIR_LIB}, tentative avec l'alternative")
    
    if(EXISTS "${CHARTDIR_QTDEMO_LIB}")
        set(CHARTDIR_LIB "${CHARTDIR_QTDEMO_LIB}")
        message(STATUS "Utilisation de la bibliothèque alternative: ${CHARTDIR_LIB}")
    else()
        message(FATAL_ERROR "Aucune bibliothèque ChartDirector trouvée!")
    endif()
else()
    message(STATUS "Bibliothèque ChartDirector trouvée: ${CHARTDIR_LIB}")
endif()

# Configuration pour Qt
find_package(Qt6 COMPONENTS Core Widgets REQUIRED)

# Activer le système de MOC pour qchartviewer.cpp
set_target_properties(chartdirector PROPERTIES 
    AUTOMOC ON
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# Lier la bibliothèque trouvée
target_link_libraries(chartdirector 
    PUBLIC  # Modifié de INTERFACE à PUBLIC
    ${CHARTDIR_LIB} 
    Qt6::Core 
    Qt6::Widgets
)

# Définir une variable pour indiquer que ChartDirector est configuré
set(CHARTDIRECTOR_FOUND TRUE CACHE BOOL "ChartDirector trouvé et configuré" FORCE)