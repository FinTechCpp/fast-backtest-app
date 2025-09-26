#pragma once

#include <memory>
#include <vector>
#include <string>
#include <map>
#include "data.hpp"
#include "common.h"
#include "ui/chart/chartTypes.h"

struct BacktestResults {
    std::shared_ptr<be::Data> data;  // Pointer to backtest data
    be::Stats stats;                  // Backtest statistics
    std::vector<std::unique_ptr<indicators::IndicatorBase>> indicatorInstances; // Instances of indicators calculated during backtest
    StrategyConfig strategyConfig; // Base configuration for the strategy
};