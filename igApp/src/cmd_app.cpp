#include <iostream>
#include <iomanip>
#include <string>
#include <limits>
#include <chrono>
#include <thread>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include "trading_ig_config.hpp"
#include "rest.h"
#include "lightstreamer.h"
#include "stream.h"

const std::string EPIC = "IX.D.NASDAQ.IFE.IP";
const std::string RESOLUTION = "D";
const std::string START_DATE = "100";
const std::string END_DATE = "";
const int NUM_POINTS = std::stoi("100");
const int PAGE_SIZE = std::stoi("20");

// Fonction pour effacer l'entrée du buffer
void clearInputBuffer() {
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

void setup_logging() {
    std::shared_ptr<spdlog::sinks::stdout_color_sink_mt> console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_level(spdlog::level::debug);
    console_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
    
    std::shared_ptr<spdlog::logger> logger = std::make_shared<spdlog::logger>("ig_trader", console_sink);
    logger->set_level(spdlog::level::debug);
    spdlog::set_default_logger(logger);
}

// Fonction pour récupérer les prix historiques
void fetchHistoricalPrices(ig::IGService& service) {

    spdlog::info("Récupération des prix historiques...");

    spdlog::info("Using epic: {}, resolution: {}, start_date: {}, pagesize: {}", 
                 EPIC, RESOLUTION, START_DATE, PAGE_SIZE);

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
        EPIC, RESOLUTION, START_DATE, END_DATE, NUM_POINTS, PAGE_SIZE);

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
    std::cout << "3. Fermer une position\n";
    std::cout << "4. Streaming de ticks\n";
    std::cout << "0. Quitter\n";
    std::cout << "Choix: ";
}

int main() {
    setup_logging(); // Initialiser le logging avec spdlog

    // Add near the beginning of main():
    spdlog::info("=== Test de l'API IG Trading en C++ ===\n");
    spdlog::debug("Test du logging debug avec spdlog\n");

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
                    // Créer et initialiser une structure PositionCreateParams
                    ig::PositionCreateParams params;
                    params.currencyCode = "EUR";
                    params.direction = "BUY";
                    params.epic = EPIC;
                    params.expiry = "-";
                    params.forceOpen = true;
                    params.guaranteedStop = false;
                    params.level = 0.0;
                    params.limitDistance = 0.0;
                    params.limitLevel = 0.0;
                    params.orderType = "MARKET";
                    params.quoteId = "";
                    params.size = 1.0;
                    params.stopDistance = 0.0;
                    params.stopLevel = 0.0;
                    params.trailingStop = false;
                    params.trailingStopIncrement = 0.0;
                    params.timeInForce = "";
                    
                    // Appeler la fonction avec la structure
                    nlohmann::json result = igService.create_open_position(params);
                    // Display result
                    if (result.contains("dealId")) {
                        spdlog::info("Position ouverte avec succès! Deal ID: {}", result["dealId"].get<std::string>());
                    } else {
                        spdlog::info("Position créée. Référence: {}", result["dealReference"].get<std::string>());
                    }
                    break;
                }
                case 3: 
                    spdlog::info("Non implémenté, veuillez réessayer plus tard.");
                    break;
                case 4: {
                    spdlog::info("Démarrage du streaming de ticks...");
                    
                    try {
                        // Utiliser directement le service IG existant pour le streaming
                        auto stream_service = std::make_shared<ig::IGStreamService>(
                            std::shared_ptr<ig::IGService>(&igService, [](ig::IGService*){}));
                        
                        // Créer la session de streaming
                        stream_service->create_session();
                        
                        // Créer le gestionnaire de streaming
                        auto streaming_manager = std::make_shared<ig::StreamingManager>(stream_service);
                        
                        // Démarrer la subscription de ticks pour l'epic par défaut
                        streaming_manager->start_tick_subscription(EPIC);
                        
                        spdlog::info("Streaming démarré pour l'epic: {}", EPIC);
                        spdlog::info("Appuyez sur Entrée pour arrêter le streaming...");
                        
                        // Afficher les données en temps réel pendant 30 secondes ou jusqu'à ce que l'utilisateur appuie sur Entrée
                        auto start_time = std::chrono::steady_clock::now();
                        auto timeout = std::chrono::seconds(30);
                        
                        while (std::chrono::steady_clock::now() - start_time < timeout) {
                            // Vérifier si l'utilisateur a appuyé sur Entrée
                            if (std::cin.peek() != EOF) {
                                std::string dummy;
                                std::getline(std::cin, dummy);
                                break;
                            }
                            
                            // Obtenir et afficher les données de tick
                            try {
                                auto ticker = streaming_manager->get_ticker(EPIC, 1);
                                if (!ticker.last_update_time.empty()) {
                                    spdlog::info("Tick {} - BID: {:.5f}, OFR: {:.5f}, LTP: {:.5f}, Volume: {}, Time: {}",
                                                EPIC, ticker.bid, ticker.offer, ticker.last_traded_price,
                                                ticker.total_traded_volume, ticker.last_update_time);
                                }
                            } catch (const std::exception& e) {
                                spdlog::debug("Pas encore de données de tick: {}", e.what());
                            }
                            
                            std::this_thread::sleep_for(std::chrono::seconds(2));
                        }
                        
                        // Arrêter le streaming
                        streaming_manager->stop_all_subscriptions();
                        stream_service->disconnect();
                        
                        spdlog::info("Streaming arrêté.");
                        
                    } catch (const std::exception& e) {
                        spdlog::error("Erreur lors du streaming: {}", e.what());
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