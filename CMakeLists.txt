cmake_minimum_required(VERSION 3.14)
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

# Configurer le chemin Qt6 selon le système d'exploitation
if(APPLE)
    # Essayer de trouver Qt automatiquement dans les emplacements standards
    set(QT_SEARCH_PATHS
        "$ENV{HOME}/Qt/*/macos"           # Installation utilisateur standard
        "/opt/Qt/*/macos"                 # Installation système alternative
        "/usr/local/opt/qt@6"             # Homebrew Qt6
        "/opt/homebrew/opt/qt@6"          # Homebrew sur Apple Silicon
    )
    
    # Chercher la version la plus récente de Qt6
    file(GLOB QT_CANDIDATES ${QT_SEARCH_PATHS})
    if(QT_CANDIDATES)
        # Trier pour avoir la version la plus récente en premier
        list(SORT QT_CANDIDATES)
        list(REVERSE QT_CANDIDATES)
        list(GET QT_CANDIDATES 0 QT_PATH)
        list(APPEND CMAKE_PREFIX_PATH "${QT_PATH}")
        message(STATUS "Qt6 détecté automatiquement: ${QT_PATH}")
    else()
        message(WARNING "Qt6 non trouvé automatiquement. Utilisez -DCMAKE_PREFIX_PATH=<chemin_vers_qt> ou installez Qt via Homebrew: brew install qt@6")
    endif()
elseif(WIN32)
    # Configuration pour Windows - chercher Qt dans les emplacements standards
    set(QT_SEARCH_PATHS
        "C:/Qt/*/msvc2022_64"             # Installation Qt standard avec MSVC 2022
        "C:/Qt/*/msvc2019_64"             # Installation Qt standard avec MSVC 2019
        "$ENV{USERPROFILE}/Qt/*/msvc2022_64"  # Installation utilisateur
        "$ENV{QTDIR}"                     # Variable d'environnement QTDIR
    )
    
    # Chercher la version la plus récente de Qt6
    file(GLOB QT_CANDIDATES ${QT_SEARCH_PATHS})
    if(QT_CANDIDATES)
        # Trier pour avoir la version la plus récente en premier
        list(SORT QT_CANDIDATES)
        list(REVERSE QT_CANDIDATES)
        list(GET QT_CANDIDATES 0 QT_PATH)
        list(APPEND CMAKE_PREFIX_PATH "${QT_PATH}")
        message(STATUS "Qt6 détecté automatiquement: ${QT_PATH}")
    else()
        message(WARNING "Qt6 non trouvé automatiquement. Assurez-vous que Qt6 est installé dans C:/Qt/ ou définissez CMAKE_PREFIX_PATH")
    endif()
endif()

# Vérifier que Qt est disponible (obligatoire)
find_package(Qt6 COMPONENTS Core Widgets REQUIRED)

# Ajouter les sous-projets (bibliothèques)
add_subdirectory(cpp_backtestEngine)
add_subdirectory(cpp_strategies)
add_subdirectory(cpp_adaptator)
add_subdirectory(ChartDirector)
add_subdirectory(cpp_backtestApp)

# Afficher un résumé de la configuration
message(STATUS "=========================================")
message(STATUS "Configuration pour ${PROJECT_NAME} v${PROJECT_VERSION}")
message(STATUS "Mode de build: ${CMAKE_BUILD_TYPE}")
message(STATUS "Standard C++: ${CMAKE_CXX_STANDARD}")
message(STATUS "Symboles de débogage: ${BUILD_WITH_DEBUG}")
message(STATUS "=========================================")