cmake_minimum_required(VERSION 3.14)
project(chartdirector)

# Afficher des informations de diagnostic
message(STATUS "Configuration de ChartDirector...")

# Sur Windows, il est plus simple d'utiliser une bibliothèque STATIC
# Sur les autres plateformes, on peut utiliser SHARED
if(WIN32)
    add_library(chartdirector STATIC 
        "${CMAKE_CURRENT_SOURCE_DIR}/qtdemo/helloworld/qchartviewer.cpp"
    )
else()
    add_library(chartdirector SHARED 
        "${CMAKE_CURRENT_SOURCE_DIR}/qtdemo/helloworld/qchartviewer.cpp"
    )
endif()

# Configurer les chemins d'inclusion
target_include_directories(chartdirector 
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/qtdemo/qtdemo
    ${CMAKE_CURRENT_SOURCE_DIR}/qtdemo/helloworld
)

# Détection adaptative selon le système d'exploitation
if(APPLE)
    set(CHARTDIR_LIB "${CMAKE_CURRENT_SOURCE_DIR}/lib/libchartdir.7.dylib")
    set(CHARTDIR_QTDEMO_LIB "${CMAKE_CURRENT_SOURCE_DIR}/qtdemo/libchartdir.dylib")
elseif(WIN32)
    set(CHARTDIR_LIB "${CMAKE_CURRENT_SOURCE_DIR}/lib/chartdir70.lib")
    set(CHARTDIR_QTDEMO_LIB "${CMAKE_CURRENT_SOURCE_DIR}/qtdemo/chartdir70.lib")
    # Sur Windows, on a aussi besoin de la DLL à l'exécution
    set(CHARTDIR_DLL "${CMAKE_CURRENT_SOURCE_DIR}/lib/chartdir70.dll")
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
    PUBLIC
    ${CHARTDIR_LIB} 
    Qt6::Core 
    Qt6::Widgets
)

# Sur Windows, copier la DLL ChartDirector dans le dossier de sortie
if(WIN32 AND EXISTS "${CHARTDIR_DLL}")
    # Copier la DLL dans tous les dossiers de sortie possibles
    add_custom_command(TARGET chartdirector POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CHARTDIR_DLL}"
        "$<TARGET_FILE_DIR:chartdirector>"
        COMMENT "Copie de chartdir70.dll"
    )
    
    # Installer la DLL avec l'application
    install(FILES "${CHARTDIR_DLL}" 
            DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

# Définir une variable pour indiquer que ChartDirector est configuré
set(CHARTDIRECTOR_FOUND TRUE CACHE BOOL "ChartDirector trouvé et configuré" FORCE)