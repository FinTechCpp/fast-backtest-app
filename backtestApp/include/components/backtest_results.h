#pragma once

#include <memory>
#include <vector>
#include <string>
#include <map>
#include "data.hpp"  // Inclure la définition de be::Data
#include "stats.hpp" // Inclure la définition de be::Stats

// Structure pour stocker les informations sur les indicateurs à afficher
struct StrategyIndicator {
    enum Type {
        RSI,
        EMA,
        STOCHASTIC,
        ATR
    };
    
    Type type;
    std::map<std::string, double> params;  // Paramètres de l'indicateur
};

struct BacktestResults {
    std::shared_ptr<be::Data> data;  // Pointeur vers les données du backtest
    be::Stats stats;                  // Statistiques du backtest
    std::vector<StrategyIndicator> indicators; // Indicateurs utilisés par la stratégie
};