#include <iostream>
#include <iomanip>
#include <string>
#include <limits>
#include <chrono>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include "../../igtrader/trading_ig_config.hpp"
#include "../../igtrader/cpp_trading_ig/include/rest.h"

// Fonction pour effacer l'entrée du buffer
void clearInputBuffer() {
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

void setup_logging() {
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_level(spdlog::level::debug);
    console_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
    
    auto logger = std::make_shared<spdlog::logger>("ig_trader", console_sink);
    logger->set_level(spdlog::level::debug);
    spdlog::set_default_logger(logger);
}

// Fonction pour récupérer les prix historiques
void fetchHistoricalPrices(ig::IGService& service) {
    std::string epic;
    std::string resolution;
    std::string start_date;
    std::string end_date;
    int numpoints = 0;
    int pagesize = 20;

    spdlog::info("Récupération des prix historiques...");

    epic = "IX.D.NASDAQ.IFE.IP";
    spdlog::info("Épic : {}", epic);

    resolution = "D";
    spdlog::info("Résolution : {}", resolution);
    
    start_date = "100";
    spdlog::info("Nombre de points à récupérer : {}", start_date);

    pagesize = std::stoi("20");
    spdlog::info("Taille de page : {}", pagesize);

    spdlog::info("Récupération des données...");

    // Force session refresh before making the request
    spdlog::info("Vérification de la session...");
    try {
        int refreshStatus = service.refresh_session();
        
        if (refreshStatus >= 200 && refreshStatus < 300) {
            spdlog::info("Session rafraîchie avec succès.");
        } else {
            spdlog::warn("Échec du rafraîchissement de session (code {}), recréation...", refreshStatus);
            service.create_session();
            spdlog::info("Session recréée avec succès.");
        }
    } catch (const ig::TokenInvalidException& e) {
        spdlog::warn("Token invalide, recréation de la session...");
        service.create_session();
        spdlog::info("Session recréée avec succès.");
    } catch (const std::exception& e) {
        spdlog::warn("Erreur lors du rafraîchissement de session: {}", e.what());
        spdlog::info("Tentative de recréation de la session...");
        service.create_session();
        spdlog::info("Session recréée avec succès.");
    }

    auto start = std::chrono::high_resolution_clock::now();
    
    // Récupération des prix historiques
    nlohmann::json result = service.fetch_historical_prices_by_epic(
        epic, resolution, start_date, end_date, numpoints, pagesize);
    
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    
    // Affichage des résultats
    spdlog::info("Données récupérées en {} secondes", elapsed.count());

    // Afficher les 5 premiers éléments du tableau des prix
    if (result.contains("prices") && result["prices"].is_array() && !result["prices"].empty()) {
        spdlog::info("Aperçu des prix (5 premiers éléments):");
        int count = 0;
        for (const auto& price : result["prices"]) {
            if (count >= 5) break;
            
            std::string timestamp;
            if (price.contains("snapshotTimeUTC")) {
                timestamp = price["snapshotTimeUTC"].get<std::string>();
            } else if (price.contains("snapshotTime")) {
                timestamp = price["snapshotTime"].get<std::string>();
            }
            
            double bidClose = 0.0;
            double askClose = 0.0;
            
            if (price.contains("closePrice")) {
                if (price["closePrice"].contains("bid")) {
                    bidClose = price["closePrice"]["bid"].get<double>();
                }
                if (price["closePrice"].contains("ask")) {
                    askClose = price["closePrice"]["ask"].get<double>();
                }
            }

            spdlog::info("Date: {} | Bid Close: {} | Ask Close: {}", timestamp, bidClose, askClose);
            count++;
        }

        spdlog::info("Nombre total de points: {}", result["prices"].size());
    } else {
        spdlog::warn("Aucun prix trouvé ou format de réponse inattendu.");
    }
        

}

// Menu principal
void displayMenu() {
    std::cout << "\n=== Menu IG Trading API Test ===\n";
    std::cout << "1. Récupérer les prix historiques\n";
    std::cout << "2. Ouvrir une position\n";    
    std::cout << "0. Quitter\n";
    std::cout << "Choix: ";
}

int main() {
    setup_logging(); // Initialiser le logging avec spdlog

    // Add near the beginning of main():
    spdlog::info("=== Test de l'API IG Trading en C++ ===\n");

    // Charger la configuration
    Config config;
    
    // Afficher les valeurs chargées
    spdlog::info("{:<20} {}", "Username:", config.username.empty() ? "[NON DÉFINI]" : config.username);
    spdlog::info("{:<20} {}", "Password:", config.password.empty() ? "[NON DÉFINI]" : "***********");
    spdlog::info("{:<20} {}", "API Key:", config.api_key.empty() ? "[NON DÉFINI]" : config.api_key.substr(0, 8) + "...");
    spdlog::info("{:<20} {}", "Account Type:", config.acc_type.empty() ? "[NON DÉFINI]" : config.acc_type);
    spdlog::info("{:<20} {}", "Account Number:", config.acc_number.empty() ? "[NON DÉFINI]" : config.acc_number);

    // Vérifier si la configuration est complète
    bool isComplete = !config.username.empty() && 
                        !config.password.empty() && 
                        !config.api_key.empty() && 
                        !config.acc_type.empty() && 
                        !config.acc_number.empty();
    
    if (!isComplete) {
        spdlog::error("⚠️  Configuration incomplète. Vérifiez votre fichier .env");
        return 1;
    }

    spdlog::info("✅ Configuration chargée avec succès!");
    spdlog::info("📊 Type de compte: {}", config.acc_type);

    try {
        // Initialiser le service IG
        spdlog::info("Initialisation du service IG...");
        ig::IGService igService(
            config.username,
            config.password,
            config.api_key,
            config.acc_type,
            config.acc_number,
            false // rate limiter désactivé pour simplifier
        );
        
        // Créer une session
        spdlog::info("Création de la session...");
        auto sessionData = igService.create_session();

        spdlog::info("✅ Connexion réussie!");

        // Boucle du menu
        int choice = -1;
        while (choice != 0) {
            displayMenu();
            std::cin >> choice;
            clearInputBuffer();
            
            switch (choice) {
                case 1:
                    fetchHistoricalPrices(igService);
                    break;
                case 2: {
                    spdlog::info("Ouverture d'une position...");
                    nlohmann::json result = igService.create_open_position(
                        "EUR", // Type de position
                        "BUY", // Direction de la position
                        "IX.D.NASDAQ.IFE.IP", // Exemple d'épic
                        "-",
                        true, //force_open
                        false, //guaranteed_stop
                        0.0, // Level (not applicable for MARKET orders)
                        0.0, // limit_distance
                        0.0, // limit level                        
                        "MARKET", // Type d'ordre
                        "", // quote_id (string parameter)
                        1.0,
                        0.0,
                        0.0,
                        false,
                        0.0,
                        "" // time_in_force parameter
                    );
                    // Display result
                    if (result.contains("dealId")) {
                        spdlog::info("Position ouverte avec succès! Deal ID: {}", result["dealId"].get<std::string>());
                    } else {
                        spdlog::info("Position créée. Référence: {}", result["dealReference"].get<std::string>());
                    }
                    break;
                }
                case 0:
                    spdlog::info("Au revoir!");
                    break;
                default:
                    spdlog::warn("Option invalide. Veuillez réessayer.");
            }
        }
        
        // Déconnexion
        spdlog::info("Déconnexion...");
        igService.logout();
        
    } catch (const ig::IGException& e) {
        spdlog::error("Erreur IG: {}", e.what());
        return 1;
    } catch (const std::exception& e) {
        spdlog::error("Erreur: {}", e.what());
        return 1;
    }
    
    return 0;
}