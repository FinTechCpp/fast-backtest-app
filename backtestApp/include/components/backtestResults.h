#pragma once

#include <memory>
#include <vector>
#include <string>
#include <map>
#include "data.hpp"  
#include "stats.hpp" 

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

struct BacktestResults {
    std::shared_ptr<be::Data> data;  // Pointer to backtest data
    be::Stats stats;                  // Backtest statistics
    std::vector<StrategyIndicator> indicators; // Indicators used by the strategy
};