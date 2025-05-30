#include "backtest.hpp"
#include "stats.hpp"
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <functional>
#include <limits>
#include <cmath>
#include <chrono>

class OutOfMoneyError : public std::runtime_error {
public:
    OutOfMoneyError() : std::runtime_error("Out of money") {}
};

Backtest::Backtest(std::shared_ptr<Data> data,
                  std::function<std::shared_ptr<Strategy>(std::shared_ptr<Broker>, std::shared_ptr<Data>)> strategyFactory,
                  double cash,
                  double spread,
                  double commission,
                  double margin,
                  bool tradeOnClose,
                  bool hedging,
                  bool exclusiveOrders,
                  bool finalizeTrades)
    : _data(data),
      _strategyFactory(strategyFactory),
      _cash(cash),
      _spread(spread),
      _commission(commission),
      _margin(margin),
      _tradeOnClose(tradeOnClose),
      _hedging(hedging),
      _exclusiveOrders(exclusiveOrders),
      _finalizeTrades(finalizeTrades) {
    
    // Validation des entrées
    if (!data) {
        throw std::invalid_argument("Data cannot be null");
    }
    if (!strategyFactory) {
        throw std::invalid_argument("Strategy factory cannot be null");
    }
    if (cash <= 0) {
        throw std::invalid_argument("Cash must be positive");
    }
    if (margin <= 0 || margin > 1) {
        throw std::invalid_argument("Margin must be between 0 and 1");
    }
    
    // Vérifier que les données OHLC ne contiennent pas de valeurs manquantes
    for (size_t i = 0; i < data->size(); ++i) {
        if (std::isnan(data->Open(i)) || std::isnan(data->High(i)) || 
            std::isnan(data->Low(i)) || std::isnan(data->Close(i))) {
            throw std::invalid_argument("OHLC data contains NaN values. Please clean the data first.");
        }
    }
    
    // Vérifier si certains prix sont supérieurs au capital initial
    bool largePrices = false;
    for (size_t i = 0; i < data->size(); ++i) {
        if (data->Close(i) > cash) {
            largePrices = true;
            break;
        }
    }
    
    if (largePrices) {
        std::cerr << "WARNING: Some prices are larger than initial cash value. "
                  << "Note that fractional trading is not supported.\n";
    }
}

Stats Backtest::run() {
    // Créer le broker et la stratégie
    _broker = std::make_shared<Broker>(_data, _cash, _spread, _commission, 
                                         _margin, _tradeOnClose, _hedging, 
                                         _exclusiveOrders);
    
    // Créer la stratégie en utilisant la factory
    std::shared_ptr<Strategy> strategy = _strategyFactory(_broker, _data);
    
    if (!strategy) {
        throw std::runtime_error("Strategy factory returned null strategy");
    }
    
    // Initialiser la stratégie
    strategy->init();
    
    // Taille des données
    size_t dataSize = _data->size();
    
    // Sauter les premières barres pour l'échauffement des indicateurs
    // Dans une implémentation réelle, nous calculerions cela en fonction des indicateurs
    size_t start = 1;  // Minimum pour avoir au moins deux points de données
    
    // Exécuter le backtest barre par barre
    try {
        for (size_t i = start; i < dataSize; ++i) {
            // Définir la longueur des données à la barre actuelle
            _data->setLength(i + 1);
            
            // Traiter les ordres et mettre à jour l'état du broker
            try {
                _broker->next();
            } catch (const OutOfMoneyError& e) {
                std::cerr << "Out of money at bar " << i << ". Stopping backtest.\n";
                break;
            } catch (const std::exception& e) {
                std::cerr << "Broker error at bar " << i << ": " << e.what() << "\n";
                break;
            }
            
            // Exécuter la logique de la stratégie pour la barre actuelle
            strategy->next();
        }
        
        // Si finalizeTrades est activé, fermer tous les trades ouverts
        if (_finalizeTrades) {
            for (auto& trade : _broker->trades()) {
                trade->close();
            }
            
            // Exécuter le broker une dernière fois pour traiter les ordres de clôture
            try {
                _broker->next();
            } catch (const std::exception& e) {
                std::cerr << "Error in final broker update: " << e.what() << "\n";
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error during backtest: " << e.what() << "\n";
    }
    
    // Restaurer la longueur complète des données
    _data->setLength(dataSize);
    
    // Récupérer la courbe d'équité depuis le broker
    const std::vector<double>& equityCurve = _broker->getEquityCurve();

    // Calculer les statistiques
    _lastResults = computeStats(_broker->closedTrades(), equityCurve, *_data);

    return _lastResults;
}

std::pair<std::map<std::string, double>, Stats> 
Backtest::optimize(const std::map<std::string, std::vector<double>>& params,
                   const std::string& maximize) {
    if (params.empty()) {
        throw std::invalid_argument("No parameters to optimize");
    }
    
    // Vérifier si la clé maximize est valide
    auto statsMap = dummyStats().toMap();
    if (statsMap.find(maximize) == statsMap.end()) {
        throw std::invalid_argument("Invalid maximize key: " + maximize);
    }
    
    // Meilleurs paramètres et stats
    std::map<std::string, double> bestParams;
    Stats bestStats = dummyStats();  // Utilisation de dummyStats() au lieu de dummyStatsMap

    // Map pour stocker tous les résultats (pour heatmap)
    std::map<std::string, double> heatmap;
    
    // Calculer le nombre total de combinaisons
    size_t combinations = 1;
    for (const auto& [param, values] : params) {
        combinations *= values.size();
    }
        
    // Suivre le meilleur score
    double bestScore = std::numeric_limits<double>::lowest();
    
    // Créer des tableaux de clés et valeurs de paramètres pour l'itération
    std::vector<std::string> paramKeys;
    std::vector<std::vector<double>> paramValues;
    
    for (const auto& [param, values] : params) {
        paramKeys.push_back(param);
        paramValues.push_back(values);
    }
    
    // Fonction pour explorer récursivement les combinaisons de paramètres
    std::function<void(size_t, std::map<std::string, double>&)> searchParams;
    
    searchParams = [&](size_t level, std::map<std::string, double>& currentParams) {
        if (level >= paramKeys.size()) {
            // Nous avons une combinaison complète de paramètres
            // Créer une factory de stratégie temporaire qui applique ces paramètres
            auto tempStrategyFactory = [this, &currentParams](std::shared_ptr<Broker> broker, std::shared_ptr<Data> data) -> std::shared_ptr<Strategy> {
                auto strategy = _strategyFactory(broker, data);
                
                // Appliquer les paramètres à la stratégie
                // Cela dépendrait de l'implémentation de votre stratégie
                // Par exemple: strategy->setParams(currentParams);
                
                return strategy;
            };
            
            // Créer un backtest temporaire
            Backtest tempBacktest(_data, tempStrategyFactory, _cash, _spread, _commission, 
                                 _margin, _tradeOnClose, _hedging, _exclusiveOrders, 
                                 _finalizeTrades);
            
            // Exécuter le backtest avec les paramètres actuels
            Stats stats = tempBacktest.run();  // Maintenant stats est de type Stats, pas une map
            
            // Convertir les statistiques en map pour accéder à la métrique maximize
            auto statsMap = stats.toMap();
            
            // Vérifier si c'est le meilleur score jusqu'à présent
            if (statsMap.find(maximize) != statsMap.end()) {
                double score = statsMap[maximize];
                if (score > bestScore) {
                    bestScore = score;
                    bestParams = currentParams;
                    bestStats = stats;  // Stocke directement l'objet Stats
                }
                
                // Stocker pour la heatmap
                std::string paramKey = "";
                for (const auto& [param, value] : currentParams) {
                    paramKey += param + "=" + std::to_string(value) + ";";
                }
                heatmap[paramKey] = score;
            }
            
            return;
        }
        
        // Essayer chaque valeur pour le paramètre actuel
        const std::string& param = paramKeys[level];
        for (double value : paramValues[level]) {
            currentParams[param] = value;
            searchParams(level + 1, currentParams);
        }
    };
    
    // Commencer la recherche récursive
    std::map<std::string, double> currentParams;
    searchParams(0, currentParams);
    
    return {bestParams, bestStats};  // Retourne les meilleurs paramètres et les meilleures stats
}

