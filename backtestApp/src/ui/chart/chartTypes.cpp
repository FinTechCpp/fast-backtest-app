#include "ui/chart/chartTypes.h"


#include <cereal/archives/binary.hpp>
#include <cereal/archives/json.hpp>
#include <cereal/types/polymorphic.hpp>
#include <cereal/types/memory.hpp>

// Enregistrer tous les types dérivés
CEREAL_REGISTER_TYPE(indicators::RSIInstance)
CEREAL_REGISTER_TYPE(indicators::EMAInstance)
CEREAL_REGISTER_TYPE(indicators::StochasticInstance)
CEREAL_REGISTER_TYPE(indicators::ATRInstance)
CEREAL_REGISTER_TYPE(indicators::SuperTrendInstance)
CEREAL_REGISTER_TYPE(indicators::CCIInstance)
CEREAL_REGISTER_TYPE(indicators::MACDInstance)
CEREAL_REGISTER_TYPE(indicators::PivotPointsInstance)

// Déclarer les relations hiérarchiques
CEREAL_REGISTER_POLYMORPHIC_RELATION(indicators::IndicatorBase, indicators::RSIInstance)
CEREAL_REGISTER_POLYMORPHIC_RELATION(indicators::IndicatorBase, indicators::EMAInstance)
CEREAL_REGISTER_POLYMORPHIC_RELATION(indicators::IndicatorBase, indicators::StochasticInstance)
CEREAL_REGISTER_POLYMORPHIC_RELATION(indicators::IndicatorBase, indicators::ATRInstance)
CEREAL_REGISTER_POLYMORPHIC_RELATION(indicators::IndicatorBase, indicators::SuperTrendInstance)
CEREAL_REGISTER_POLYMORPHIC_RELATION(indicators::IndicatorBase, indicators::CCIInstance)
CEREAL_REGISTER_POLYMORPHIC_RELATION(indicators::IndicatorBase, indicators::MACDInstance)
CEREAL_REGISTER_POLYMORPHIC_RELATION(indicators::IndicatorBase, indicators::PivotPointsInstance)

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