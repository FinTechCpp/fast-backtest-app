# Bibliothèque IG Trading C++
cmake_minimum_required(VERSION 3.14)

# Définir la bibliothèque
add_library(cpp_trading_ig
    src/rest.cpp
    src/lightstreamer.cpp
    src/stream.cpp
    src/igBroker.cpp
)

# Headers publics
target_include_directories(cpp_trading_ig
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Dépendances requises
find_package(PkgConfig REQUIRED)

# nlohmann/json pour le parsing JSON
find_package(nlohmann_json REQUIRED)
target_link_libraries(cpp_trading_ig PRIVATE nlohmann_json::nlohmann_json)

# spdlog pour les logs
find_package(spdlog REQUIRED)
target_link_libraries(cpp_trading_ig PRIVATE spdlog::spdlog)

# Crypto++ pour le chiffrement
find_path(CRYPTOPP_INCLUDE_DIR cryptopp/cryptlib.h)
find_library(CRYPTOPP_LIBRARY NAMES cryptopp libcryptopp)
if(NOT CRYPTOPP_INCLUDE_DIR OR NOT CRYPTOPP_LIBRARY)
    message(FATAL_ERROR "Crypto++ library not found. Please install cryptopp.")
endif()
target_include_directories(cpp_trading_ig PRIVATE ${CRYPTOPP_INCLUDE_DIR})
target_link_libraries(cpp_trading_ig PRIVATE ${CRYPTOPP_LIBRARY})

# CPR pour les requêtes HTTP - Installation automatique si non trouvé
find_package(cpr QUIET)
if(NOT cpr_FOUND)
    message(STATUS "CPR non trouvé, installation automatique...")
    
    # Utiliser FetchContent pour télécharger et compiler CPR
    include(FetchContent)
    FetchContent_Declare(cpr
        GIT_REPOSITORY https://github.com/libcpr/cpr.git
        GIT_TAG 1.10.5) # Version stable
    FetchContent_MakeAvailable(cpr)
    
    target_link_libraries(cpp_trading_ig PRIVATE cpr::cpr)
else()
    target_link_libraries(cpp_trading_ig PRIVATE cpr::cpr)
endif()

# Standard C++17
target_compile_features(cpp_trading_ig PRIVATE cxx_std_17)

# Options de compilation selon le mode debug/release
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(cpp_trading_ig PRIVATE DEBUG_MODE=1)
endif()
