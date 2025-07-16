#pragma once

#include <vector>
#include <cmath>
#include <algorithm>
#include <map>
#include <tuple>
#include "date.hpp"
// super pas top, je fais pour avoir PivotPointsInstance::PeriodType pour les points pivots
#include "ui/chart/indicatorInstances.h"

struct PivotSegment {
    size_t startIndex;  // Indice de début du segment
    size_t endIndex;    // Indice de fin du segment (inclus)
    double value;       // Valeur du niveau pour ce segment
    
    PivotSegment(size_t start, size_t end, double val) 
        : startIndex(start), endIndex(end), value(val) {}
};

/**
 * @brief Utility class for calculating technical indicators
 * 
 * This class provides static methods to calculate various technical indicators
 * used in financial analysis.
 */
class TechnicalIndicators {
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

    static std::map<int, std::vector<PivotSegment>> calculatePivotPoints(
        const std::vector<double>& openData,
        const std::vector<double>& highData,
        const std::vector<double>& lowData,
        const std::vector<double>& closeData,
        const std::vector<be::Date>& timestamps,
        PivotPointsInstance::PeriodType periodType,
        PivotPointsInstance::CalculationMethod calcMethod
    );
};