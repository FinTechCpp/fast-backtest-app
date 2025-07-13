#pragma once

#include <vector>
#include <cmath>
#include <algorithm>
#include <map>
#include "date.hpp"
// super pas top, je fais pour avoir PivotPointsInstance::PeriodType pour les points pivots
#include "ui/chart/indicatorInstances.h"

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
    static void calculateRSI(const std::vector<double>& closeData, int period, std::vector<double>& rsiValues);

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
    static void calculateHeikinAshi(
        const std::vector<double>& open,
        const std::vector<double>& high,
        const std::vector<double>& low,
        const std::vector<double>& close,
        std::vector<double>& ha_open,
        std::vector<double>& ha_high,
        std::vector<double>& ha_low,
        std::vector<double>& ha_close
    );

    /**
     * @brief Calculates the Exponential Moving Average (EMA)
     * 
     * @param closeData The closing prices
     * @param period The period for EMA calculation
     * @param emaValues Output vector that will contain the calculated EMA values
     */
    static void calculateEMA(const std::vector<double>& closeData, int period, std::vector<double>& emaValues);

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
    static void calculateSupertrend(
        const std::vector<double>& highData,
        const std::vector<double>& lowData,
        const std::vector<double>& closeData,
        int period,
        double multiplier,
        std::vector<double>& supertrendValues,
        std::vector<int>& trendDirections
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
    static void calculateStochastic(
        const std::vector<double>& highData,
        const std::vector<double>& lowData,
        const std::vector<double>& closeData,
        int fastKPeriod,
        int slowKPeriod,
        int slowDPeriod,
        std::vector<double>& kValues,
        std::vector<double>& dValues);

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

    static void calculateATR(
        const std::vector<double>& highData,
        const std::vector<double>& lowData,
        const std::vector<double>& closeData,
        int period,
        std::vector<double>& atrValues,
        bool useLogScale = false
    );

    static void calculatePivotPoints(
        const std::vector<double>& highData,
        const std::vector<double>& lowData,
        const std::vector<double>& closeData,
        const std::vector<be::Date>& timestamps,
        PivotPointsInstance::PeriodType periodType,
        std::map<int, std::vector<double>>& levelValues
    );
};