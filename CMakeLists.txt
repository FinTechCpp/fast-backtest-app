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
    
    # Utiliser les flags appropriés selon le compilateur
    if(MSVC)
        add_compile_options(/Od /Zi)
    else()
        add_compile_options(-g -O0)
    endif()
else()
    set(CMAKE_BUILD_TYPE Release)
    
    # Utiliser les flags appropriés selon le compilateur
    if(MSVC)
        add_compile_options(/O2)
    else()
        add_compile_options(-O3)
    endif()
endif()

# Détection de compilation croisée MXE
if(CMAKE_CROSSCOMPILING)
    message(STATUS "Compilation croisée détectée")
    message(STATUS "Système cible: ${CMAKE_SYSTEM_NAME}")
    message(STATUS "Architecture cible: ${CMAKE_SYSTEM_PROCESSOR}")
    
    # Pour MXE, Qt6 est automatiquement trouvé
    if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
        message(STATUS "Configuration MXE pour Windows détectée")
    endif()
else()
    # Configuration normale pour développement local
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
            "$ENV{QT_ROOT_DIR}"               # Variable utilisée par GitHub Actions
            "D:/a/*/Qt/*/msvc2022_64"         # Chemin GitHub Actions
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
            # After the QT_CANDIDATES check, replace the existing warning with:
            if(DEFINED ENV{Qt6_DIR})
                list(APPEND CMAKE_PREFIX_PATH "$ENV{Qt6_DIR}")
                message(STATUS "Utilisation de Qt6_DIR depuis l'environnement: $ENV{Qt6_DIR}")
            elseif(DEFINED ENV{QT_ROOT_DIR})
                list(APPEND CMAKE_PREFIX_PATH "$ENV{QT_ROOT_DIR}")
                message(STATUS "Utilisation de QT_ROOT_DIR depuis l'environnement: $ENV{QT_ROOT_DIR}")
            else()
                message(WARNING "Qt6 non trouvé automatiquement. Assurez-vous que Qt6 est installé dans C:/Qt/ ou définissez CMAKE_PREFIX_PATH")
            endif()
        endif()
    elseif(UNIX AND NOT APPLE)
        # Configuration pour Linux
        set(QT_SEARCH_PATHS
            "$ENV{HOME}/Qt/*/gcc_64"          # Installation utilisateur
            "/usr/lib/qt6"                    # Installation système
            # "/usr/lib/x86_64-linux-gnu/qt6"   # Autre emplacement système
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
            # Essayer de récupérer depuis la variable d'environnement
            if(DEFINED ENV{CMAKE_PREFIX_PATH})
                list(APPEND CMAKE_PREFIX_PATH "$ENV{CMAKE_PREFIX_PATH}")
                message(STATUS "Utilisation de CMAKE_PREFIX_PATH depuis l'environnement: $ENV{CMAKE_PREFIX_PATH}")
            else()
                message(WARNING "Qt6 non trouvé automatiquement. Utilisez -DCMAKE_PREFIX_PATH=~/Qt/6.8.3/gcc_64")
            endif()
        endif()
    endif()
endif()

# Vérifier que Qt est disponible (obligatoire)
find_package(Qt6 COMPONENTS Core Widgets Charts Network REQUIRED)

# Ajouter les sous-projets (bibliothèques)
add_subdirectory(backtestEngine)
add_subdirectory(cpp_strategies)
add_subdirectory(backtestAdapter)
add_subdirectory(ChartDirector)
add_subdirectory(backtestApp)

# Afficher un résumé de la configuration
message(STATUS "=========================================")
message(STATUS "Configuration pour ${PROJECT_NAME} v${PROJECT_VERSION}")
message(STATUS "Mode de build: ${CMAKE_BUILD_TYPE}")
message(STATUS "Standard C++: ${CMAKE_CXX_STANDARD}")
message(STATUS "Symboles de débogage: ${BUILD_WITH_DEBUG}")
if(CMAKE_CROSSCOMPILING)
    message(STATUS "Target: ${CMAKE_SYSTEM_NAME} ${CMAKE_SYSTEM_PROCESSOR}")
endif()
message(STATUS "=========================================")