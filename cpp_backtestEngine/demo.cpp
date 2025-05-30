#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <random>
#include <algorithm>
#include <iomanip>
#include <chrono>
#include <fstream>
#include <sstream>
#include "include/backtest.hpp"
#include "include/strategy.hpp"
#include "include/data.hpp"

// Fonction pour calculer une moyenne mobile simple (SMA)
std::vector<double> calculateSMA(const std::vector<double>& prices, int period) {
    std::vector<double> sma(prices.size(), 0);
    
    for (size_t i = 0; i < prices.size(); ++i) {
        if (i < static_cast<size_t>(period) - 1) {
            // Pas assez de données pour calculer la moyenne
            sma[i] = std::numeric_limits<double>::quiet_NaN();
        } else {
            double sum = 0;
            for (int j = 0; j < period; ++j) {
                sum += prices[i - j];
            }
            sma[i] = sum / period;
        }
    }
    
    return sma;
}

// Stratégie de croisement de moyennes mobiles autonome
class SmaCrossStrategy : public Strategy {
public:
    SmaCrossStrategy(std::shared_ptr<Broker> broker, std::shared_ptr<Data> data, 
                    int fastPeriod = 10, int slowPeriod = 30)
        : Strategy(broker, data), 
          _fastPeriod(fastPeriod), 
          _slowPeriod(slowPeriod),
          _fastValues(fastPeriod, 0.0),  // Tampon circulaire pour calculs rapides
          _slowValues(slowPeriod, 0.0),  // Tampon circulaire pour calculs lents
          _fastIndex(0),
          _slowIndex(0),
          _lastFastSMA(std::numeric_limits<double>::quiet_NaN()),
          _currentFastSMA(std::numeric_limits<double>::quiet_NaN()),
          _lastSlowSMA(std::numeric_limits<double>::quiet_NaN()),
          _currentSlowSMA(std::numeric_limits<double>::quiet_NaN()),
          _barsProcessed(0) {}
                    
    void init() override {
        std::cout << "Strategy initialized with FastSMA(" << _fastPeriod 
                  << ") and SlowSMA(" << _slowPeriod << ")" << std::endl;
        // Pas de précalcul - tout sera fait "à la volée" dans next()
    }
    
    void next() override {
        size_t i = getData()->size() - 1;
        double currentPrice = getData()->Close(i);
        
        // Mettre à jour les tampons circulaires
        _fastValues[_fastIndex] = currentPrice;
        _slowValues[_slowIndex] = currentPrice;
        
        _fastIndex = (_fastIndex + 1) % _fastPeriod;
        _slowIndex = (_slowIndex + 1) % _slowPeriod;
        
        _barsProcessed++;
        
        // Calculer les SMAs si nous avons assez de données
        _lastFastSMA = _currentFastSMA;  // Sauvegarder la valeur précédente
        _lastSlowSMA = _currentSlowSMA;  // Sauvegarder la valeur précédente
        
        if (_barsProcessed >= _fastPeriod) {
            double sum = 0;
            for (double val : _fastValues) {
                sum += val;
            }
            _currentFastSMA = sum / _fastPeriod;
        }
        
        if (_barsProcessed >= _slowPeriod) {
            double sum = 0;
            for (double val : _slowValues) {
                sum += val;
            }
            _currentSlowSMA = sum / _slowPeriod;
        }
        
        // Attendre que les deux moyennes mobiles soient disponibles
        if (std::isnan(_currentFastSMA) || std::isnan(_currentSlowSMA) || 
            std::isnan(_lastFastSMA) || std::isnan(_lastSlowSMA)) {
            return;
        }
        
        // Vérifier les croisements
        bool crossover = _lastFastSMA <= _lastSlowSMA && _currentFastSMA > _currentSlowSMA;
        bool crossunder = _lastFastSMA >= _lastSlowSMA && _currentFastSMA < _currentSlowSMA;
        
        // Afficher des informations de débogage sur chaque barre comme demandé
        std::cout << "Bar " << i << ": Price=" << std::fixed << std::setprecision(4) << currentPrice 
              << ", FastSMA=" << std::fixed << std::setprecision(4) << _currentFastSMA 
              << ", SlowSMA=" << std::fixed << std::setprecision(4) << _currentSlowSMA << std::endl;
        
        // Position actuelle
        Position position = getPosition();
        
        // Logique de trading avec logs harmonisés
        if (crossover) {
            std::cout << "Crossover détecté à la barre " << i << ", tentative d'achat..." << std::endl; 
            // Acheter si croisement vers le haut et pas de position longue
            if (!position) {
                std::cout << "Pas de position, exécution de l'achat..." << std::endl;
                buy(1.0, 0, 0, 0, 0, 1, 3, "Crossover");  // SL à 1 point, TP à 3 points
                std::cout << "Signal d'achat à la barre " << i << ", prix: " << std::fixed << std::setprecision(4) << currentPrice << std::endl;
            }
            else if (position.size() <= 0) {
                std::cout << "Fermeture de la position courte..." << std::endl;
                position.close();  // Fermer la position courte existante
                std::cout << "Ouverture d'une position longue..." << std::endl;
                buy(1.0, 0, 0, 0, 0, 1, 3, "Crossover");  // SL à 1 point, TP à 3 points
                std::cout << "Signal d'achat à la barre " << i << ", prix: " << std::fixed << std::setprecision(4) << currentPrice << std::endl;
            }
        }
        else if (crossunder) {
            std::cout << "Crossunder détecté à la barre " << i << ", tentative de vente..." << std::endl;
            // Vendre si croisement vers le bas et pas de position courte
            if (!position) {
                std::cout << "Pas de position, exécution de la vente..." << std::endl;
                sell(1.0, 0, 0, 0, 0, 1, 3, "Crossunder");  // SL à 1 point, TP à 3 points
                std::cout << "Signal de vente à la barre " << i << ", prix: " << std::fixed << std::setprecision(4) << currentPrice << std::endl;
            }
            else if (position.size() >= 0) {
                std::cout << "Fermeture de la position longue..." << std::endl;
                position.close();  // Fermer la position longue existante
                std::cout << "Ouverture d'une position courte..." << std::endl;
                sell(1.0, 0, 0, 0, 0, 1, 3, "Crossunder");  // SL à 1 point, TP à 3 points
                std::cout << "Signal de vente à la barre " << i << ", prix: " << std::fixed << std::setprecision(4) << currentPrice << std::endl;
            }
        }
    }
    
private:
    int _fastPeriod;
    int _slowPeriod;
    std::vector<double> _fastValues;  // Tampon circulaire pour SMA rapide
    std::vector<double> _slowValues;  // Tampon circulaire pour SMA lente
    size_t _fastIndex;                // Index courant dans le tampon rapide  
    size_t _slowIndex;                // Index courant dans le tampon lent
    double _lastFastSMA;              // Dernière valeur de SMA rapide calculée
    double _currentFastSMA;           // Valeur actuelle de SMA rapide
    double _lastSlowSMA;              // Dernière valeur de SMA lente calculée
    double _currentSlowSMA;           // Valeur actuelle de SMA lente
    size_t _barsProcessed;            // Nombre de barres traitées
};

class SimpleStrategy : public Strategy {
public:
    SimpleStrategy(std::shared_ptr<Broker> broker, std::shared_ptr<Data> data)
        : Strategy(broker, data) {}
        
    void init() override {
        std::cout << "Simple strategy initialized" << std::endl;
    }
    
    void next() override {
        size_t i = getData()->size() - 1;
        std::cout << "Processing bar " << i << std::endl;
        
        // N'acheter qu'à la barre 100
        if (i == 100) {
            std::cout << "Buying at bar 100" << std::endl;
            buy(1.0, 0, 0, 0, 0, 0, 0, "Test");
            std::cout << "Buy order created" << std::endl;
        }
    }
};

// Fonction pour charger des données à partir d'un fichier CSV
std::shared_ptr<Data> loadDataFromCSV(const std::string& filename) {
    std::vector<Date> dates;
    std::vector<double> open, high, low, close, volume;
    
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Impossible d'ouvrir le fichier: " + filename);
    }
    
    std::string line;
    bool header = true;
    
    while (std::getline(file, line)) {
        // Ignorer la ligne d'en-tête
        if (header) {
            header = false;
            continue;
        }
        
        std::stringstream ss(line);
        std::string cell;
        std::vector<std::string> cells;
        
        // Découper la ligne en cellules
        while (std::getline(ss, cell, ',')) {
            cells.push_back(cell);
        }
        
        // Vérifier que la ligne contient assez de cellules
        if (cells.size() < 6) {
            std::cerr << "Avertissement: ligne ignorée (pas assez de cellules): " << line << std::endl;
            continue;
        }
        
        // Parser la date (format: YYYY-MM-DD)
        std::string dateStr = cells[0];
        int year = 0, month = 0, day = 0;
        
        // Utiliser sscanf pour extraire les composants de la date
        if (sscanf(dateStr.c_str(), "%d-%d-%d", &year, &month, &day) != 3) {
            std::cerr << "Erreur: format de date invalide: " << dateStr << std::endl;
            continue;
        }
        
        // Créer l'objet Date (heure définie à 00:00:00)
        Date date(year, month, day, 0, 0, 0);
        dates.push_back(date);
        
        try {
            open.push_back(std::stod(cells[1]));
            high.push_back(std::stod(cells[2]));
            low.push_back(std::stod(cells[3]));
            close.push_back(std::stod(cells[4]));
            volume.push_back(std::stod(cells[5]));
        } catch (const std::exception& e) {
            std::cerr << "Erreur de conversion pour la ligne: " << line << " - " << e.what() << std::endl;
            // Retirer la dernière date ajoutée puisque les données sont invalides
            dates.pop_back();
            continue;
        }
    }
    
    std::cout << "Chargé " << dates.size() << " barres depuis " << filename << std::endl;
    
    return std::make_shared<Data>(dates, open, high, low, close, volume);
}

// Fonction pour générer des données synthétiques
std::shared_ptr<Data> generateSyntheticData(int numBars, double initialPrice = 100.0, 
                                           double volatility = 0.01, 
                                           double drift = 0.0001) {
    std::vector<Date> dates;
    std::vector<double> openPrices, highPrices, lowPrices, closePrices, volumes;
    
    // Générateur de nombres aléatoires
    std::mt19937 generator(42);  // Seed fixe pour reproductibilité
    std::normal_distribution<double> distribution(0.0, 1.0);
    
    double price = initialPrice;
    
    // Date de départ (1er janvier 2024)
    int year = 2024;
    int month = 1;
    int day = 1;
    
    for (int i = 0; i < numBars; ++i) {
        // Créer un objet Date directement (au lieu d'une chaîne)
        Date currentDate(year, month, day, 0, 0, 0);
        dates.push_back(currentDate);
        
        // Avancer au jour suivant
        day++;
        // Gestion simplifiée des mois (considère tous les mois à 30 jours)
        if (day > 30) {
            day = 1;
            month++;
            if (month > 12) {
                month = 1;
                year++;
            }
        }
        
        // Simuler le mouvement du prix avec un mouvement brownien géométrique
        double dailyReturn = drift + volatility * distribution(generator);
        double todayOpen = price;
        price = price * (1.0 + dailyReturn);
        double todayClose = price;
        
        // Simuler high et low
        double highLowRange = std::abs(todayOpen - todayClose) * 0.5 + 
                             (volatility * price * distribution(generator) + volatility * price);
        double todayHigh = std::max(todayOpen, todayClose) + highLowRange * 0.5;
        double todayLow = std::min(todayOpen, todayClose) - highLowRange * 0.5;
        
        // Simuler le volume (proportionnel à la volatilité)
        double todayVolume = 1000 + 100 * std::abs(distribution(generator));
        
        openPrices.push_back(todayOpen);
        highPrices.push_back(todayHigh);
        lowPrices.push_back(todayLow);
        closePrices.push_back(todayClose);
        volumes.push_back(todayVolume);
    }
    
    // Créer et retourner l'objet Data
    return std::make_shared<Data>(dates, openPrices, highPrices, lowPrices, closePrices, volumes);
}

// Fonction pour formater et afficher les statistiques
void displayStats(const Stats& stats) {
    std::cout << "\n============== BACKTEST RESULTS ==============\n";
    
    // Format des nombres
    std::cout << std::fixed << std::setprecision(2);
    
    // Statistiques de performance principales
    std::cout << "Equity Final [$]: " << stats.equityFinal << std::endl;
    std::cout << "Equity Peak [$]: " << stats.equityPeak << std::endl;
    std::cout << "Return [%]: " << stats.returnPct << std::endl;
    std::cout << "Buy & Hold Return [%]: " << stats.buyHoldReturnPct << std::endl;

    // Statistiques de risque
    std::cout << "\n-------------- RISK METRICS --------------\n";
    std::cout << "Max. Drawdown [%]: " << stats.maxDrawdownPct << std::endl;
    std::cout << "Sharpe Ratio: " << stats.sharpeRatio << std::endl;
    std::cout << "Sortino Ratio: " << stats.sortinoRatio << std::endl;
    std::cout << "Calmar Ratio: " << stats.calmarRatio << std::endl;

    // Statistiques des trades
    std::cout << "\n-------------- TRADE STATISTICS --------------\n";
    std::cout << "# Trades: " << stats.numTrades << std::endl;
    std::cout << "Win Rate [%]: " << stats.winRatePct << std::endl;
    std::cout << "Best Trade [%]: " << stats.bestTradePct << std::endl;
    std::cout << "Worst Trade [%]: " << stats.worstTradePct << std::endl;
    std::cout << "Avg. Trade [%]: " << stats.avgTradePct << std::endl;
    std::cout << "Profit Factor: " << stats.profitFactor << std::endl;
    std::cout << "SQN: " << stats.sqn << std::endl;

    // Stat sur les trades details
    std::cout << "\n-------------- TRADE DETAILS --------------\n";
    
    // Afficher un en-tête de tableau similaire à celui de Python
    std::cout << std::setw(4) << " "
              << std::setw(8) << "Size" 
              << std::setw(10) << "EntryBar" 
              << std::setw(10) << "ExitBar" 
              << std::setw(12) << "EntryPrice" 
              << std::setw(12) << "ExitPrice" 
              << std::setw(12) << "SL" 
              << std::setw(12) << "TP" 
              << std::setw(12) << "PnL" 
              << std::setw(12) << "ReturnPct" 
              << std::setw(22) << "EntryTime" 
              << std::setw(22) << "ExitTime" 
              << std::setw(10) << "Duration" 
              << "  Tag" << std::endl;

    std::vector<std::shared_ptr<Trade>> trades = stats.trades;
    
    // Afficher les détails de chaque trade avec gestion d'erreurs
    try {
        for (size_t i = 0; i < trades.size(); ++i) {
            try {
                const auto& trade = trades[i];
                if (!trade) {
                    std::cerr << "Warning: Trade null à l'index " << i << std::endl;
                    continue;
                }
                
                // Récupérer les indices de barres et vérifier leur validité
                size_t entryBar = trade->entryBar();
                size_t exitBar = trade->exitBar();
                
                // Calculer la durée en jours
                int durationDays = static_cast<int>(exitBar) - static_cast<int>(entryBar);

                Date entryDate = trade->entryDate();
                Date exitDate = trade->exitDate();
                // Afficher les détails du trade
                std::cout << std::setw(4) << i
                          << std::setw(8) << int(trade->size())
                          << std::setw(10) << entryBar
                          << std::setw(10) << exitBar
                          << std::setw(12) << std::fixed << std::setprecision(6) << trade->entryPrice()
                          << std::setw(12) << std::fixed << std::setprecision(6) << trade->exitPrice()
                          << std::setw(12) << std::fixed << std::setprecision(6) << trade->sl()
                          << std::setw(12) << std::fixed << std::setprecision(6) << trade->tp()
                          << std::setw(12) << std::fixed << std::setprecision(6) << trade->pl()
                          << std::setw(12) << std::fixed << std::setprecision(6) << trade->plPercent()
                          << std::setw(22) << entryDate
                          << std::setw(22) << exitDate
                          << std::setw(10) << durationDays << " days"
                          << "  " << trade->tag() << std::endl;
                
            } catch (const std::exception& e) {
                std::cerr << "Erreur lors du traitement du trade " << i << ": " << e.what() << std::endl;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Erreur lors de l'affichage des trades: " << e.what() << std::endl;
    }
    
    std::cout << "==============================================\n";
}

// Programme principal
int main() {
    try {
        std::shared_ptr<Data> data;
        
        // Chemin vers le fichier CSV
        std::string csv_file = "../../data/synthetic_data.csv";
        
        // Essayer de charger à partir du CSV d'abord
        try {
            std::cout << "Tentative de chargement des données depuis " << csv_file << "..." << std::endl;
            data = loadDataFromCSV(csv_file);
        } catch (const std::exception& e) {
            std::cerr << "Erreur lors du chargement du CSV: " << e.what() << std::endl;
            std::cout << "Génération de données synthétiques à la place..." << std::endl;
            // Utiliser les données synthétiques comme fallback
            data = generateSyntheticData(500, 100.0, 0.015, 0.0002);
        }
        
        std::cout << "Données chargées avec " << data->size() << " barres" << std::endl;
        
        // Créer une factory pour la stratégie
        auto strategyFactory = [](std::shared_ptr<Broker> broker, std::shared_ptr<Data> data) {
            // Utiliser la stratégie SmaCrossStrategy avec une moyenne rapide de 10 jours et une lente de 30 jours
            return std::make_shared<SmaCrossStrategy>(broker, data, 10, 30);
        };
        
        std::cout << "Creating and running backtest..." << std::endl;
        // Créer et exécuter le backtest
        Backtest backtest(data, strategyFactory, 10000.0, 0.0, 0.001, 1.0, false, false, false, true);
        Stats results = backtest.run();
        
        // Afficher les résultats en passant les trades et les données
        displayStats(results);
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}