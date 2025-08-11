#pragma once

#include <memory>
#include <vector>
#include <string>
#include <map>
#include "data.hpp"  
#include "stats.hpp"
#include "common.h"

struct DataReference {
    // Métadonnées d'identification
    std::string symbol;         // Symbole traité
    std::string timeframe;      // Intervalle (1h, 1d, etc.)
    std::string period;         // Période analysée (1m, 1y, etc.)
    
    // Caractéristiques d'identification
    be::Date firstDate;         // Première date de la série
    be::Date lastDate;          // Dernière date de la série
    size_t candleCount;         // Nombre de bougies total
    
    // Empreinte numérique (hash) des données
    std::string dataHash;       // Hash SHA-256 des données OHLCV
    
    // Utilitaires
    bool matchesData(const be::Data& data) const;  // Vérifie si ces données correspondent
    static DataReference fromData(const be::Data& data, const std::string& symbol,
                                  const std::string& timeframe, const std::string& period);
};

// Structure for storing indicators used in the strategy 
// This is used to display the indicators in the chart after the backtest ends
struct StrategyIndicator {
    enum Type {
        RSI,
        EMA,
        STOCHASTIC,
        ATR,
        SUPERTREND
    };
    
    Type type;
    std::map<std::string, double> params;  // Indicator parameters
};

// faire un systeme d'enregistrement des BacktestResults
// pour pouvoir les charger dans l'interface graphique et comparer les résultats avec une nouvelle execution du backtest

struct BacktestResults {
    std::shared_ptr<be::Data> data;  // Pointer to backtest data
    be::Stats stats;                  // Backtest statistics
    std::vector<StrategyIndicator> indicators; // Indicators used by the strategy
    StrategyBaseConfig strategyBaseConfig; // Base configuration for the strategy
};