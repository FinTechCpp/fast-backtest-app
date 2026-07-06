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

// Forward declarations for filter namespace types used in method signatures.
// These are declared with a fixed underlying type to allow forward declaration
// without requiring the full definition in this header; include the real
// definition in translation units that need the enum values.
namespace filter {
    enum class PriceType : int;
    enum class MAType : int;
}


/**
 * @brief Utility class for calculating technical indicators
 * 
 * This class provides static methods to calculate various technical indicators
 * used in financial analysis.
 */
class IndicatorMathUtils {
public:
    /**
     * @brief Detects day boundaries in timestamp data
     * 
     * @param timestamps The timestamps in ChartDirector format (seconds since epoch)
     * @return std::vector<size_t> Indices where a new day starts
     */
    static std::vector<size_t> detectDayBoundaries(const std::vector<double>& timestamps);

    /**
     * @brief Calculates the RSI (Relative Strength Index) indicator
     * 
     * @param closeData The closing prices
     * @param period The period for RSI calculation
     * @param timestamps Optional timestamps for day boundary detection
     * @param resetOnNewDay If true, reset indicator calculation at each new day
     * @return std::vector<double> The calculated RSI values
     */
    static std::vector<double> calculateRSI(
        const std::vector<double>& closeData, 
        int period,
        const std::vector<double>& timestamps = {},
        bool resetOnNewDay = false
    );

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
     * @param timestamps Optional timestamps for day boundary detection
     * @param resetOnNewDay If true, reset indicator calculation at each new day
     * @return std::vector<double> The calculated EMA values
     */
    static std::vector<double> calculateEMA(
        const std::vector<double>& closeData, 
        int period,
        const std::vector<double>& timestamps = {},
        bool resetOnNewDay = false
    );

    /**
     * @brief Calculates the SuperTrend indicator
     * 
     * @param highData The high prices
     * @param lowData The low prices
     * @param closeData The closing prices
     * @param period The period for SuperTrend calculation
     * @param multiplier The multiplier for SuperTrend
     * @param timestamps Optional timestamps for day boundary detection
     * @param resetOnNewDay If true, reset indicator calculation at each new day
     * @return tuple of supertrendValues and trendDirections
     */
    static std::tuple<std::vector<double>, std::vector<int>> calculateSupertrend(
        const std::vector<double>& highData,
        const std::vector<double>& lowData,
        const std::vector<double>& closeData,
        int period,
        double multiplier,
        const std::vector<double>& timestamps = {},
        bool resetOnNewDay = false
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
     * @param timestamps Optional timestamps for day boundary detection
     * @param resetOnNewDay If true, reset indicator calculation at each new day
     * @return tuple of kValues and dValues
     */
    static std::tuple<std::vector<double>, std::vector<double>> calculateStochastic(
        const std::vector<double>& highData,
        const std::vector<double>& lowData,
        const std::vector<double>& closeData,
        int fastKPeriod,
        int slowKPeriod,
        int slowDPeriod,
        const std::vector<double>& timestamps = {},
        bool resetOnNewDay = false
    );

    /**
     * @brief Calculates the Average True Range (ATR)
     *
     * @param highData The high prices
     * @param lowData The low prices
     * @param closeData The closing prices
     * @param period The period for ATR calculation
     * @param useLogScale Indicates whether to use logarithmic scale for ATR
     * @param timestamps Optional timestamps for day boundary detection
     * @param resetOnNewDay If true, reset indicator calculation at each new day
     * @return std::vector<double> The calculated ATR values
     */
    static std::vector<double> calculateATR(
        const std::vector<double>& highData,
        const std::vector<double>& lowData,
        const std::vector<double>& closeData,
        int period,
        bool useLogScale = false,
        const std::vector<double>& timestamps = {},
        bool resetOnNewDay = false
    );

    /**
     * @brief Calculates the CCI (Commodity Channel Index) indicator
     *
     * @param highData The high prices
     * @param lowData The low prices
     * @param closeData The closing prices
     * @param period The period for CCI calculation
     * @param timestamps Optional timestamps for day boundary detection
     * @param resetOnNewDay If true, reset indicator calculation at each new day
     * @return std::vector<double> The calculated CCI values
     */
    static std::vector<double> calculateCCI(
        const std::vector<double>& highData,
        const std::vector<double>& lowData,
        const std::vector<double>& closeData,
        int period,
        const std::vector<double>& timestamps = {},
        bool resetOnNewDay = false
    );

    /**
     * @brief Calculates MACD (Moving Average Convergence Divergence) indicator
     *
     * Returns a tuple of three vectors: (macd_line, signal_line, histogram)
     *
     * @param timestamps Optional timestamps for day boundary detection
     * @param resetOnNewDay If true, reset indicator calculation at each new day
     */
    static std::tuple<std::vector<double>, std::vector<double>, std::vector<double>> calculateMACD(
        const std::vector<double>& open,
        const std::vector<double>& high,
        const std::vector<double>& low,
        const std::vector<double>& close,
        int fastPeriod,
        int slowPeriod,
        int signalPeriod,
        filter::PriceType source,
        filter::MAType oscMAType,
        filter::MAType signalMAType,
        int signalSmoothing,
        const std::vector<double>& timestamps = {},
        bool resetOnNewDay = false
    );

    /**
     * @brief Calculates Bollinger Bands (BB) indicator
     * Returns a tuple of three vectors: (middle_band, upper_band, lower_band)
     * @param timestamps Optional timestamps for day boundary detection
     * @param resetOnNewDay If true, reset indicator calculation at each new day
     */
    static std::tuple<std::vector<double>, std::vector<double>, std::vector<double>> calculateBollingerBands(
        const std::vector<double>& open,
        const std::vector<double>& high,
        const std::vector<double>& low,
        const std::vector<double>& close,
        int period,
        double stdDevMultiplier,
        filter::PriceType source,
        filter::MAType oscMAType,
        const std::vector<double>& timestamps = {},
        bool resetOnNewDay = false
    );

    /**
     * @brief Calculates the Swing Structure Trend indicator
     *
     * A swing high is confirmed when the price rose by at least `highMove` over a leg of
     * `minPeriods`-`maxPeriods` bars before the peak, then fell by at least `highMove` over
     * a leg of the same duration range after it, and the peak is the highest high within
     * +/- maxPeriods bars. A swing low mirrors this using `lowMove` on candle lows/closes.
     * The trend is up when the latest swing high/low both exceed the previous ones, down
     * when both are lower, and uncertain (0) otherwise.
     *
     * @return tuple of (swingHighValues, swingLowValues, trendValues), each full-length and
     * carried forward from the last confirmed swing/trend (Chart::NoValue / 0 before the first)
     */
    static std::tuple<std::vector<double>, std::vector<double>, std::vector<int>> calculateSwingStructure(
        const std::vector<double>& highData,
        const std::vector<double>& lowData,
        const std::vector<double>& closeData,
        double highMove,
        double lowMove,
        int minPeriods,
        int maxPeriods,
        const std::vector<double>& timestamps = {},
        bool resetOnNewDay = false
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