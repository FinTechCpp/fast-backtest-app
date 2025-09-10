#pragma once

#include <vector>
#include <cmath>
#include <algorithm>
#include <map>
#include <tuple>
#include <unordered_map>
#include <set>
#include "date.hpp"
// super pas top, je fais pour avoir PivotPointsInstance::PeriodType pour les points pivots
#include "ui/chart/indicatorInstances.h"

enum class AggregationLevel {
    Raw,         // Données brutes
    OneMinute,   // 1 minute
    OneHour,     // 1 heure
    OneDay,      // 1 jour
    Count        // Nombre total de niveaux d'agrégation
};

struct PivotPeriod {
    // Mapping vers les indices agrégés pour différents niveaux d'agrégation
    std::array<std::pair<size_t, size_t>, static_cast<size_t>(AggregationLevel::Count)> indices; // Indices bruts pour chaque niveau d'agrégation
    
    // Valeurs de tous les niveaux de pivot pour cette période
    std::array<double, static_cast<size_t>(PivotPointsInstance::LevelType::NumLevels)> levelValues;
};

/**
 * @brief Utility class for calculating technical indicators
 * 
 * This class provides static methods to calculate various technical indicators
 * used in financial analysis.
 */
class IndicatorMathUtils {
public:
    /**
     * @brief Calculates the RSI (Relative Strength Index) indicator
     * 
     * @param closeData The closing prices
     * @param period The period for RSI calculation
     * @param rsiValues Output vector that will contain the calculated RSI values
     */
    static std::vector<double> calculateRSI(const std::vector<double>& closeData, int period);

    /**
     * @brief Calculates Heikin-Ashi candles
     * 
     * @param open Opening prices
     * @param high High prices
     * @param low Low prices
     * @param close Closing prices
     * @param ha_open Heikin-Ashi opening prices (output)
     * @param ha_high Heikin-Ashi high prices (output)
     * @param ha_low Heikin-Ashi low prices (output)
     * @param ha_close Heikin-Ashi closing prices (output)
     */
    static std::tuple<std::vector<double>, std::vector<double>, std::vector<double>, std::vector<double>> calculateHeikinAshi(
        const std::vector<double>& open,
        const std::vector<double>& high,
        const std::vector<double>& low,
        const std::vector<double>& close
    );

    /**
     * @brief Calculates the Exponential Moving Average (EMA)
     * 
     * @param closeData The closing prices
     * @param period The period for EMA calculation
     * @param emaValues Output vector that will contain the calculated EMA values
     */
    static std::vector<double> calculateEMA(const std::vector<double>& closeData, int period);

    /**
     * @brief Calculates the SuperTrend indicator
     * 
     * @param highData The high prices
     * @param lowData The low prices
     * @param closeData The closing prices
     * @param period The period for SuperTrend calculation
     * @param multiplier The multiplier for SuperTrend
     * @param supertrendValues Output vector that will contain the calculated SuperTrend values
     * @param trendDirections Output vector that will contain the trend directions (1 for bullish trend, -1 for bearish trend)
     */
    static std::tuple<std::vector<double>, std::vector<int>> calculateSupertrend(
        const std::vector<double>& highData,
        const std::vector<double>& lowData,
        const std::vector<double>& closeData,
        int period,
        double multiplier
    );

    /**
     * @brief Calculates the Stochastic indicator
     * 
     * @param highData The high prices
     * @param lowData The low prices
     * @param closeData The closing prices
     * @param fastKPeriod The period for calculating raw %K
     * @param slowKPeriod The smoothing period for %K
     * @param slowDPeriod The period for calculating %D
     * @param kValues Output vector that will contain the smoothed %K values
     * @param dValues Output vector that will contain the %D values
     */
    static std::tuple<std::vector<double>, std::vector<double>> calculateStochastic(
        const std::vector<double>& highData,
        const std::vector<double>& lowData,
        const std::vector<double>& closeData,
        int fastKPeriod,
        int slowKPeriod,
        int slowDPeriod
    );

    /**
     * @brief Calculates the Average True Range (ATR)
     *
     * @param highData The high prices
     * @param lowData The low prices
     * @param closeData The closing prices
     * @param period The period for ATR calculation
     * @param atrValues Output vector that will contain the calculated ATR values
     * @param useLogScale Indicates whether to use logarithmic scale for ATR
     */

    static std::vector<double> calculateATR(
        const std::vector<double>& highData,
        const std::vector<double>& lowData,
        const std::vector<double>& closeData,
        int period,
        bool useLogScale = false
    );

    static std::vector<PivotPeriod> calculatePivotPoints(
        const std::vector<double>& openData,
        const std::vector<double>& highData,
        const std::vector<double>& lowData,
        const std::vector<double>& closeData,
        const std::vector<be::Date>& timestamps,
        PivotPointsInstance::PeriodType periodType,
        PivotPointsInstance::CalculationMethod calcMethod
    );
};