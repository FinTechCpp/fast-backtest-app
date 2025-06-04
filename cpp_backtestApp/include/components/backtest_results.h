#pragma once

#include <memory>
#include "data.hpp"  // Inclure la définition de be::Data
#include "stats.hpp" // Inclure la définition de be::Stats

struct BacktestResults {
    std::shared_ptr<be::Data> data;  // Pointeur vers les données du backtest
    be::Stats stats;                  // Statistiques du backtest
};