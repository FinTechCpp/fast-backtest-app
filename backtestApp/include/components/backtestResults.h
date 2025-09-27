#pragma once

#include <memory>
#include <vector>
#include <string>
#include <map>
#include "data.hpp"
#include "common.h"
#include "ui/chart/chartTypes.h"
#include "ui/panels/generalParamsPanel.h"

struct BacktestResults {
    GeneralParamsConfig generalConfig; // General configuration for the backtest
    StrategyConfig strategyConfig; // Strategy configuration used in the backtest
    std::shared_ptr<be::Data> data;  // Pointer to backtest data
    be::Stats stats;                  // Backtest statistics
};