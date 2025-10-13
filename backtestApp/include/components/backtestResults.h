#pragma once

#include <memory>
#include <vector>
#include <string>
#include <map>
#include "data.hpp"
#include "common.h"
#include "ui/chart/chartTypes.h"
#include "ui/panels/generalParamsPanel.h"

// struct Candles {
//     std::vector<be::Date> date;
//     std::vector<double> open;
//     std::vector<double> high;
//     std::vector<double> low;
//     std::vector<double> close;
//     std::vector<double> volume;
// };

struct BacktestResults {
    GeneralParamsConfig generalConfig; // General configuration for the backtest
    StrategyConfig strategyConfig; // Strategy configuration used in the backtest
    std::vector<be::Candle> candles;
    be::Stats stats;                  // Backtest statistics
    std::vector<chart::ChartMarker> userMarkers; // User-placed markers on the chart
};