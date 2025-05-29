#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <random>
#include <algorithm>
#include <iomanip>
#include <chrono>
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
        
        // Afficher des informations de débogage
        if (i % 1 == 0) {  // Afficher uniquement toutes les barres pour éviter trop de sorties
            std::cout << "Bar " << i << ": Price=" << currentPrice 
                      << ", FastSMA=" << _currentFastSMA 
                      << ", SlowSMA=" << _currentSlowSMA << std::endl;
        }
        
        // Position actuelle
        Position position = getPosition();
        
        // Logique de trading
        if (crossover) {
            std::cout << "Crossover detected at bar " << i << ", attempting to buy..." << std::endl; 
            // Acheter si croisement vers le haut et pas de position longue
            if (!position) {
                std::cout << "No position, executing buy..." << std::endl;
                buy(1.0, 0, 0, 0, 0, 0.05, 0.10, "Crossover");  // SL à 5%, TP à 10% en points
                std::cout << "BUY signal at bar " << i << ", price: " << currentPrice << std::endl;
            }
            else if (position.size() <= 0) {
                std::cout << "Closing short position..." << std::endl;
                position.close();  // Fermer la position courte existante
                std::cout << "Going long..." << std::endl;
                buy(1.0, 0, 0, 0, 0, 0.05, 0.10, "Crossover");  // SL à 5%, TP à 10% en points
                std::cout << "BUY signal at bar " << i << ", price: " << currentPrice << std::endl;
            }
        }
        else if (crossunder) {
            std::cout << "Crossunder detected at bar " << i << ", attempting to sell..." << std::endl;
            // Vendre si croisement vers le bas et pas de position courte
            if (!position) {
                std::cout << "No position, executing sell..." << std::endl;
                sell(1.0, 0, 0, 0, 0, 0.05, 0.10, "Crossunder");  // SL à 5%, TP à 10% en points
                std::cout << "SELL signal at bar " << i << ", price: " << currentPrice << std::endl;
            }
            else if (position.size() >= 0) {
                std::cout << "Closing long position..." << std::endl;
                position.close();  // Fermer la position longue existante
                std::cout << "Going short..." << std::endl;
                sell(1.0, 0, 0, 0, 0, 0.05, 0.10, "Crossunder");  // SL à 5%, TP à 10% en points
                std::cout << "SELL signal at bar " << i << ", price: " << currentPrice << std::endl;
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

// Fonction pour générer des données synthétiques
std::shared_ptr<Data> generateSyntheticData(int bars, double initialPrice = 100.0, 
                                           double volatility = 0.01, double trend = 0.0001) {
    std::vector<std::string> dates;
    std::vector<double> open, high, low, close, volume;
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<> d(0, volatility);
    
    double price = initialPrice;
    auto now = std::chrono::system_clock::now();
    auto now_t = std::chrono::system_clock::to_time_t(now);
    
    for (int i = 0; i < bars; ++i) {
        // Générer une date (un jour de moins à chaque barre)
        auto bar_time = now_t - (bars - i - 1) * 24 * 60 * 60;
        char buffer[20];
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%d", std::localtime(&bar_time));
        dates.push_back(std::string(buffer));
        
        // Ajouter un bruit aléatoire au prix
        double change = d(gen) + trend;
        price *= (1 + change);
        
        // Générer les valeurs OHLC
        double day_open = price * (1 + d(gen) * 0.5);
        double day_close = price * (1 + d(gen) * 0.5);
        double day_high = std::max(day_open, day_close) * (1 + std::abs(d(gen)) * 0.5);
        double day_low = std::min(day_open, day_close) * (1 - std::abs(d(gen)) * 0.5);
        
        // Générer un volume aléatoire
        double day_volume = 1000 + std::abs(d(gen)) * 500;
        
        open.push_back(day_open);
        high.push_back(day_high);
        low.push_back(day_low);
        close.push_back(day_close);
        volume.push_back(day_volume);
    }
    
    return std::make_shared<Data>(dates, open, high, low, close, volume);
}

// Fonction pour formater et afficher les statistiques
void displayStats(const std::map<std::string, double>& stats) {
    std::cout << "\n============== BACKTEST RESULTS ==============\n";
    
    // Format des nombres
    std::cout << std::fixed << std::setprecision(2);
    
    // Statistiques de performance principales
    std::cout << "Equity Final [$]: " << stats.at("Equity Final [$]") << std::endl;
    std::cout << "Equity Peak [$]: " << stats.at("Equity Peak [$]") << std::endl;
    std::cout << "Return [%]: " << stats.at("Return [%]") << std::endl;
    std::cout << "Buy & Hold Return [%]: " << stats.at("Buy & Hold Return [%]") << std::endl;
    
    // Statistiques de risque
    std::cout << "\n-------------- RISK METRICS --------------\n";
    std::cout << "Max. Drawdown [%]: " << stats.at("Max. Drawdown [%]") << std::endl;
    std::cout << "Sharpe Ratio: " << stats.at("Sharpe Ratio") << std::endl;
    std::cout << "Sortino Ratio: " << stats.at("Sortino Ratio") << std::endl;
    std::cout << "Calmar Ratio: " << stats.at("Calmar Ratio") << std::endl;
    
    // Statistiques des trades
    std::cout << "\n-------------- TRADE STATISTICS --------------\n";
    std::cout << "# Trades: " << stats.at("# Trades") << std::endl;
    std::cout << "Win Rate [%]: " << stats.at("Win Rate [%]") << std::endl;
    std::cout << "Best Trade [%]: " << stats.at("Best Trade [%]") << std::endl;
    std::cout << "Worst Trade [%]: " << stats.at("Worst Trade [%]") << std::endl;
    std::cout << "Avg. Trade [%]: " << stats.at("Avg. Trade [%]") << std::endl;
    std::cout << "Profit Factor: " << stats.at("Profit Factor") << std::endl;
    std::cout << "SQN: " << stats.at("SQN") << std::endl;
    
    std::cout << "==============================================\n";
}

// Programme principal
int main() {
    try {
        std::cout << "Generating synthetic data..." << std::endl;
        // Créer des données synthétiques: 500 jours, prix initial 100, légère tendance haussière
        auto data = generateSyntheticData(500, 100.0, 0.015, 0.0002);
        std::cout << "Data generated with " << data->size() << " bars" << std::endl;
        
        // Créer une factory pour la stratégie
        auto strategyFactory = [](std::shared_ptr<Broker> broker, std::shared_ptr<Data> data) {
            // Utiliser la stratégie SmaCrossStrategy avec une moyenne rapide de 10 jours et une lente de 30 jours
            return std::make_shared<SmaCrossStrategy>(broker, data, 10, 30);
        };
        
        std::cout << "Creating and running backtest..." << std::endl;
        // Créer et exécuter le backtest
        Backtest backtest(data, strategyFactory, 10000.0, 0.0, 0.001, 1.0, false, false, false, true);
        auto results = backtest.run();
        
        // Afficher les résultats
        displayStats(results);
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}