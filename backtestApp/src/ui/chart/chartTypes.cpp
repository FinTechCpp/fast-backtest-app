#include "ui/chart/chartTypes.h"

namespace chart {

const std::array<std::pair<ChartType, const char*>, static_cast<size_t>(ChartType::Count)> chartTypeNames = {{
    { ChartType::CandleStick, "CandleStick" },
    { ChartType::HeikinAshi, "HeikinAshi" },
    { ChartType::OHLC, "OHLC" },
    { ChartType::Close, "Close" }
}};

std::string chartTypeToString(ChartType type) {
    for (const auto& info : chartTypeNames)
        if (info.first == type)
            return info.second;
    return "Unknown";
}

ChartType stringToChartType(const std::string& typeStr) {
    for (const auto& info : chartTypeNames)
        if (typeStr == info.second)
            return info.first;
    return ChartType::CandleStick; // Valeur par défaut
}

const std::array<const char*, static_cast<size_t>(AggregationLevel::Count)> aggregationLevelNames = {
    "Raw", "1 Minute", "1 Hour", "1 Day"
};

std::string aggregationLevelToString(AggregationLevel level) {
    size_t idx = static_cast<size_t>(level);
    if (idx < aggregationLevelNames.size())
        return aggregationLevelNames[idx];
    return "Unknown";
}

}