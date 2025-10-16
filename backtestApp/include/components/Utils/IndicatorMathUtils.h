#pragma once

#include <vector>
#include <cmath>
#include <algorithm>
#include <map>
#include <tuple>
#include <unordered_map>
#include <set>
#include "beTypes.h"
#include "ui/chart/chartTypes.h"


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

    /**
     * @brief Calculates the CCI (Commodity Channel Index) indicator
     *
     * @param highData The high prices
     * @param lowData The low prices
     * @param closeData The closing prices
     * @param period The period for CCI calculation
     * @return std::vector<double> The calculated CCI values
     */
    static std::vector<double> calculateCCI(
        const std::vector<double>& highData,
        const std::vector<double>& lowData,
        const std::vector<double>& closeData,
        int period
    );

    /**
     * @brief Calculates MACD (Moving Average Convergence Divergence) indicator
     *
     * Returns a tuple of three vectors: (macd_line, signal_line, histogram)
     *
     * Parameters mirror the MACD implementation in ThirdParty/Strategies/include/Indicators/macd.hpp:
     *  - fastPeriod: fast EMA/SMA length
     *  - slowPeriod: slow EMA/SMA length
     *  - signalPeriod: signal line period
     *  - source: "open"/"high"/"low"/"close" (default "close")
     *  - oscMAType: "EMA" or "SMA" for oscillator moving averages (default "EMA")
     *  - signalMAType: "EMA" or "SMA" for signal line moving average (default "EMA")
     *  - signalSmoothing: optional smoothing length for signal (if 0 -> use signalPeriod)
     */
    static std::tuple<std::vector<double>, std::vector<double>, std::vector<double>> calculateMACD(
        const std::vector<double>& open,
        const std::vector<double>& high,
        const std::vector<double>& low,
        const std::vector<double>& close,
        int fastPeriod,
        int slowPeriod,
        int signalPeriod,
        const std::string& source = "close",
        const std::string& oscMAType = "EMA",
        const std::string& signalMAType = "EMA",
        int signalSmoothing = 0
    );

    /**
     * @brief Calculates Pivot Points
     */
    static std::vector<indicators::PivotPointsInstance::PivotPeriod> calculatePivotPoints(
        const std::vector<double>& openData,
        const std::vector<double>& highData,
        const std::vector<double>& lowData,
        const std::vector<double>& closeData,
        const std::vector<be::Date>& timestamps,
        indicators::PivotPeriodType periodType,
        indicators::PivotCalculationMethod calcMethod
    );
};