# Application IG Command Line
cmake_minimum_required(VERSION 3.14)

# Trouver les packages requis
find_package(fmt REQUIRED)

# Fonction pour configurer un exécutable IG
function(configure_ig_executable name source_file)
    # Créer l'exécutable
    add_executable(${name} ${source_file})
    
    # Configurer les headers
    target_include_directories(${name}
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include
            ${CMAKE_CURRENT_SOURCE_DIR}/../igtrader
            ${CMAKE_CURRENT_SOURCE_DIR}/../strategies/include
    )
    
    # Lier avec les bibliothèques (arguments supplémentaires optionnels)
    target_link_libraries(${name} PRIVATE cpp_trading_ig fmt::fmt ${ARGN})
    
    # Standard C++17
    target_compile_features(${name} PRIVATE cxx_std_17)
    
    # Répertoire de sortie
    set_target_properties(${name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    )
endfunction()

# Configurer les exécutables
configure_ig_executable(ig_cmd_app src/cmd_app.cpp)
configure_ig_executable(ig_live_app src/liveIG.cpp strategies)
