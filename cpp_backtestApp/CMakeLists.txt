# Trouver les packages Qt nécessaires
find_package(Qt6 COMPONENTS Core Widgets Charts REQUIRED)

# Activer l'outil de méta-objets Qt (moc)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Recueillir les fichiers source et d'en-tête
file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE HEADERS "include/*.h")

# Créer l'exécutable
add_executable(backtestapp 
    ${SOURCES}
    ${HEADERS}
)

# Configurer les chemins d'inclusion principaux
target_include_directories(backtestapp
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/cpp_backtestEngine/include
        ${CMAKE_SOURCE_DIR}/cpp_strategies/include
        ${CMAKE_SOURCE_DIR}/cpp_adaptator/include
        ${CMAKE_SOURCE_DIR}/ChartDirector/include
        ${CMAKE_SOURCE_DIR}/ChartDirector/qtdemo/helloworld
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

# Pour compilation Windows (native ou croisée)
if(WIN32 OR CMAKE_CROSSCOMPILING)
    # Pas de windeployqt en compilation croisée, les libs sont statiques avec MXE
    if(NOT CMAKE_CROSSCOMPILING)
        # Trouver l'outil windeployqt
        get_target_property(QT6_QMAKE_EXECUTABLE Qt6::qmake IMPORTED_LOCATION)
        get_filename_component(QT6_WINDEPLOYQT_EXECUTABLE ${QT6_QMAKE_EXECUTABLE} PATH)
        set(QT6_WINDEPLOYQT_EXECUTABLE "${QT6_WINDEPLOYQT_EXECUTABLE}/windeployqt.exe")

        # Vérifier que windeployqt existe
        if(EXISTS ${QT6_WINDEPLOYQT_EXECUTABLE})
            # Commande pour déployer Qt après la compilation
            add_custom_command(TARGET backtestapp POST_BUILD
                COMMAND ${QT6_WINDEPLOYQT_EXECUTABLE} --qmldir ${CMAKE_SOURCE_DIR} $<TARGET_FILE:backtestapp>
                COMMENT "Déploiement des DLL Qt avec windeployqt")
        else()
            message(WARNING "windeployqt non trouvé. Les DLL Qt devront être copiées manuellement.")
            
            # Alternative: copier manuellement les DLL Qt principales
            set(QT_RUNTIME_PATH "${Qt6_DIR}/../../../bin")
            set(QT_DLLS
                Qt6Core.dll
                Qt6Gui.dll
                Qt6Widgets.dll
                Qt6Charts.dll
            )
            
            foreach(DLL ${QT_DLLS})
                if(EXISTS "${QT_RUNTIME_PATH}/${DLL}")
                    add_custom_command(TARGET backtestapp POST_BUILD
                        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${QT_RUNTIME_PATH}/${DLL}"
                        "$<TARGET_FILE_DIR:backtestapp>"
                        COMMENT "Copie de ${DLL}"
                    )
                endif()
            endforeach()
        endif()
    endif()
    
    # Copier la DLL ChartDirector si elle existe
    set(CHARTDIR_DLL "${CMAKE_SOURCE_DIR}/ChartDirector/lib/chartdir70.dll")
    if(EXISTS "${CHARTDIR_DLL}")
        add_custom_command(TARGET backtestapp POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CHARTDIR_DLL}"
            "$<TARGET_FILE_DIR:backtestapp>"
            COMMENT "Copie de chartdir70.dll vers le dossier de l'exécutable"
        )
    endif()
endif()

# Installer l'exécutable et ses dépendances
install(TARGETS backtestapp
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
)