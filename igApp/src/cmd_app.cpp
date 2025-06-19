#include <iostream>
#include <iomanip>
#include <string>
#include <limits>
#include <chrono>
#include "../../igtrader/trading_ig_config.hpp"
#include "../../igtrader/cpp_trading_ig/include/rest.h"

// Fonction pour effacer l'entrée du buffer
void clearInputBuffer() {
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

// Fonction pour récupérer les prix historiques
void fetchHistoricalPrices(ig::IGService& service) {
    std::string epic;
    std::string resolution;
    std::string start_date;
    std::string end_date;
    int numpoints = 0;
    int pagesize = 20;
    
    std::cout << "\n=== Récupération des prix historiques ===\n";
    
    std::cout << "Epic (ex: IX.D.FTSE.DAILY.IP): ";
    std::cin >> epic;
    clearInputBuffer();
    
    std::cout << "Résolution (ex: D, 1H, 15Min): ";
    std::cin >> resolution;
    clearInputBuffer();
    
    std::cout << "Voulez-vous spécifier une plage de dates (o/n)? ";
    char choice;
    std::cin >> choice;
    clearInputBuffer();
    
    if (choice == 'o' || choice == 'O') {
        std::cout << "Date de début (format YYYY-MM-DD'T'HH:mm:ss, ex: 2023-01-01T00:00:00): ";
        std::getline(std::cin, start_date);
        
        std::cout << "Date de fin (format YYYY-MM-DD'T'HH:mm:ss, ex: 2023-02-01T00:00:00): ";
        std::getline(std::cin, end_date);
    } else {
        std::cout << "Nombre de points à récupérer (ex: 100): ";
        std::cin >> numpoints;
        clearInputBuffer();
    }
    
    std::cout << "Taille de page (défaut: 20): ";
    std::string pageSizeStr;
    std::getline(std::cin, pageSizeStr);
    if (!pageSizeStr.empty()) {
        pagesize = std::stoi(pageSizeStr);
    }
    
    try {
        std::cout << "\nRécupération des données...\n";
        auto start = std::chrono::high_resolution_clock::now();
        
        // Récupération des prix historiques
        nlohmann::json result = service.fetch_historical_prices_by_epic(
            epic, resolution, start_date, end_date, numpoints, pagesize);
        
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end - start;
        
        // Affichage des résultats
        std::cout << "\nDonnées récupérées en " << elapsed.count() << " secondes\n";
        
        // Afficher les 5 premiers éléments du tableau des prix
        if (result.contains("prices") && result["prices"].is_array() && !result["prices"].empty()) {
            std::cout << "\nAperçu des prix (5 premiers éléments):\n";
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
                
                std::cout << "Date: " << timestamp 
                          << " | Bid Close: " << bidClose 
                          << " | Ask Close: " << askClose << std::endl;
                count++;
            }
            
            std::cout << "\nNombre total de points: " << result["prices"].size() << std::endl;
        } else {
            std::cout << "\nAucun prix trouvé ou format de réponse inattendu.\n";
        }
        
    } catch (const std::exception& e) {
        std::cerr << "\nErreur lors de la récupération des prix: " << e.what() << std::endl;
    }
}

// Menu principal
void displayMenu() {
    std::cout << "\n=== Menu IG Trading API Test ===\n";
    std::cout << "1. Récupérer les prix historiques\n";
    std::cout << "0. Quitter\n";
    std::cout << "Choix: ";
}

int main() {
    std::cout << "=== Test de l'API IG Trading en C++ ===\n";
    
    // Charger la configuration
    Config config;
    
    // Afficher les valeurs chargées
    std::cout << std::left << std::setw(20) << "Username:" 
                << (config.username.empty() ? "[NON DÉFINI]" : config.username) << std::endl;
    
    std::cout << std::left << std::setw(20) << "Password:" 
                << (config.password.empty() ? "[NON DÉFINI]" : "***********") << std::endl;
    
    std::cout << std::left << std::setw(20) << "API Key:" 
                << (config.api_key.empty() ? "[NON DÉFINI]" : config.api_key.substr(0, 8) + "...") << std::endl;
    
    std::cout << std::left << std::setw(20) << "Account Type:" 
                << (config.acc_type.empty() ? "[NON DÉFINI]" : config.acc_type) << std::endl;
    
    std::cout << std::left << std::setw(20) << "Account Number:" 
                << (config.acc_number.empty() ? "[NON DÉFINI]" : config.acc_number) << std::endl;
    
    // Vérifier si la configuration est complète
    bool isComplete = !config.username.empty() && 
                        !config.password.empty() && 
                        !config.api_key.empty() && 
                        !config.acc_type.empty() && 
                        !config.acc_number.empty();
    
    if (!isComplete) {
        std::cerr << "\n⚠️  Configuration incomplète. Vérifiez votre fichier .env" << std::endl;
        return 1;
    }

    std::cout << "\n✅ Configuration chargée avec succès!" << std::endl;
    std::cout << "📊 Type de compte: " << config.acc_type << std::endl;
    
    try {
        // Initialiser le service IG
        std::cout << "\nInitialisation du service IG..." << std::endl;
        ig::IGService igService(
            config.username,
            config.password,
            config.api_key,
            config.acc_type,
            config.acc_number,
            false // rate limiter désactivé pour simplifier
        );
        
        // Créer une session
        std::cout << "Création de la session..." << std::endl;
        auto sessionData = igService.create_session(false, "2");
        
        std::cout << "✅ Connexion réussie!" << std::endl;
        
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
                case 0:
                    std::cout << "Au revoir!" << std::endl;
                    break;
                default:
                    std::cout << "Option invalide. Veuillez réessayer." << std::endl;
            }
        }
        
        // Déconnexion
        std::cout << "Déconnexion..." << std::endl;
        igService.logout();
        
    } catch (const ig::IGException& e) {
        std::cerr << "Erreur IG: " << e.what() << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Erreur: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}