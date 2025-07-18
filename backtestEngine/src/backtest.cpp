#include "backtest.hpp"
#include "stats.hpp"
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <functional>
#include <limits>
#include <cmath>
#include <chrono>

namespace be {

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
    if (!data) throw std::invalid_argument("Data cannot be null");
    if (!strategyFactory) throw std::invalid_argument("Strategy factory cannot be null");
    if (cash <= 0) throw std::invalid_argument("Cash must be positive");
    if (margin <= 0 || margin > 1) throw std::invalid_argument("Margin must be between 0 and 1");
    
    
    // Vérifier que les données OHLC ne contiennent pas de valeurs manquantes
    // Utilise maintenant la méthode at() de notre nouvelle interface
    for (size_t i = 0; i < data->size(); ++i) {
        const Candle& candle = data->at(i);
        if (std::isnan(candle.open) || std::isnan(candle.high) || 
            std::isnan(candle.low) || std::isnan(candle.close)) {
            throw std::invalid_argument("OHLC data contains NaN values. Please clean the data first.");
        }
    }
}

Stats Backtest::run() {
    // Réinitialiser l'itérateur de données
    _data->reset();
    
    // Créer le broker avec la nouvelle interface
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
    
    // Exécuter le backtest barre par barre - nouvelle approche avec itérateur
    try {
        // Réinitialiser l'itérateur et avancer à la première position
        _data->reset();
        _data->moveNext();
        
        // Continuer tant qu'il y a des données valides
        while (_data->position() < _data->size() - 1) {  // S'assurer qu'on ne dépasse pas
            // Traiter les ordres et mettre à jour l'état du broker
            try {
                _broker->next();
            } catch (const OutOfMoneyError&) {
                std::cerr << "Out of money at position " << _data->position() << ". Stopping backtest.\n";
                break;
            } catch (const std::exception& e) {
                std::cerr << "Broker error at position " << _data->position() << ": " << e.what() << "\n";
                break;
            }
            
            // Exécuter la logique de la stratégie pour la barre actuelle
            strategy->next();
            
            // Vérifier s'il y a un trou après la bougie courante
            if (_data->hasGapAfterCurrent() && !_broker->trades().empty()) {
                // Fermer toutes les positions ouvertes
                for (auto& trade : _broker->trades()) {
                    trade->close();
                }
                
                // Traiter les ordres pour s'assurer que les positions sont fermées correctement
                _broker->next();
            }
            
            // Rapport de progression
            if (_progressCallback && (_data->position() % 100 == 0 || _data->position() == dataSize - 1)) { 
                _progressCallback(_data->position() + 1, dataSize);
            }
            
            // Avancer à la prochaine barre (mais vérifier qu'on peut)
            if (!_data->moveNext()) {
                break;  // Sortir proprement si moveNext échoue
            }
        }
        
        // Si finalizeTrades est activé, fermer tous les trades ouverts
        if (_finalizeTrades) {
            for (auto& trade : _broker->trades()) {
                trade->close();
            }
            
            // Utiliser la méthode spéciale pour finaliser les ordres
            _broker->finalizeOrders();

        }
    } catch (const std::exception& e) {
        std::cerr << "Error during backtest: " << e.what() << "\n";
        return dummyStats(); // ou une autre valeur par défaut appropriée
    }
    
    // Récupérer la courbe d'équité depuis le broker
    const std::vector<double>& equityCurve = _broker->getEquityCurve();

    // Calculer les statistiques avec nos interfaces mises à jour
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
    Stats bestStats = dummyStats();

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
            Stats stats = tempBacktest.run();
            
            // Convertir les statistiques en map pour accéder à la métrique maximize
            auto statsMap = stats.toMap();
            
            // Vérifier si c'est le meilleur score jusqu'à présent
            if (statsMap.find(maximize) != statsMap.end()) {
                double score = statsMap[maximize];
                if (score > bestScore) {
                    bestScore = score;
                    bestParams = currentParams;
                    bestStats = stats;
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
    
    return {bestParams, bestStats};
}

} // namespace be