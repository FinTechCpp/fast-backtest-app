#pragma once

#include <memory>
#include <vector>
#include <string>
#include <map>
#include "data.hpp"  
#include "stats.hpp"
#include "common.h"

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