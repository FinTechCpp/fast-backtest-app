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
    std::vector<StrategyConfig> strategyConfigs; // Strategy configurations used in the backtest
    std::vector<be::Candle> candles;
    be::Stats stats;                  // Backtest statistics
    std::vector<chart::ChartMarker> userMarkers; // User-placed markers on the chart
};