#pragma once

#include <cstddef>   // for size_t
#include <vector>
#include <set>
#include <map>
#include <array>
#include <string>
#include <QString>
#include "common.h"
#include <utility>  // for std::pair
#include <cereal/types/polymorphic.hpp>

// Force dynamic initialization
CEREAL_FORCE_DYNAMIC_INIT(chart_types)

namespace chart {
    enum class ChartType {
        CandleStick,  ///< Japanese candlestick chart
        HeikinAshi,   ///< Heikin Ashi candles (averaged)
        OHLC,         ///< OHLC bars (Open-High-Low-Close)
        Close,        ///< Close price line only
        Count         ///< Total number of chart types
    };
    extern const std::array<std::pair<ChartType, const char*>, static_cast<size_t>(ChartType::Count)> chartTypeNames;
    std::string chartTypeToString(ChartType type);
    ChartType stringToChartType(const std::string& typeStr);

    enum class AggregationLevel {
        Raw,         // Raw data
        OneMinute,   // 1 minute
        OneHour,     // 1 hour
        OneDay,      // 1 day
        Count        // Total number of aggregation levels
    };
    extern const std::array<const char*, static_cast<size_t>(AggregationLevel::Count)> aggregationLevelNames;
    std::string aggregationLevelToString(AggregationLevel level);

    // Structure for chart configuration
    struct ChartConfiguration {
        ChartType chartType = ChartType::CandleStick;
        int chartWidth = 1200; // should be adaptive
        int chartHeight = 1000;
        int equityHeight = 180; // included in the main chart height
        // int volumeHeight = 100;
        bool showTrades = true;
        bool showEquity = true;

        // Properties for fixed Y-scale mode
        bool fixedYScale = false;     // Indicates whether using a fixed Y-scale
        double yScaleMin = 0.0;       // Minimum value of the Y-scale
        double yScaleMax = 0.0;       // Maximum value of the Y-scale
        double yScaleOffset = 0.0;    // Vertical offset in scale units
    };

    struct AggregationInfo {
        AggregationLevel level;    // The optimal aggregation level
        size_t startIndex;         // The start index in the cached data
        size_t pointCount;         // The number of points to extract
        bool isValid = false;      // Validity flag
    };
    inline bool operator==(const AggregationInfo& a, const AggregationInfo& b) {
        return a.level == b.level && a.startIndex == b.startIndex && a.pointCount == b.pointCount && a.isValid == b.isValid;
    }
    inline bool operator!=(const AggregationInfo& a, const AggregationInfo& b) {
        return !(a == b);
    }

    // use inheritance to store OHLCV isValid
    struct OHLC {
        std::vector<double> open;
        std::vector<double> high;
        std::vector<double> low;
        std::vector<double> close;
        bool isValid = false;
    };

    struct AggregatedOHLCV : OHLC {
        std::vector<double> timestamps, volume;
        std::vector<std::vector<size_t>> rawIndicesMapping;  // For each aggregated index, list of corresponding raw indices
        /*
        Example :
        Raw data indices:    0, 1, 2, 3, 4, 5, 6, 7, 8
        Aggregated (1min):    0, 1, 2
        rawIndicesMapping:  [[0,1,2],[3,4,5],[6,7,8]]
        0 -> [0,1,2]
        1 -> [3,4,5]
        2 -> [6,7,8]
        In aggregated candle 0, we have raw candles 0,1,2 etc.
        */
    };

    // one could imagine reworking this structure to for example just take the indicator type
    // for example for RSI it's std::vector<double> rsiValues;
    // for pivot points it's std::map<int, std::vector<double>> pivotPointsValues;
    struct IndicatorData {
        // For each indicator type, store the IDs that have been aggregated
        std::set<int> validRsiIds;
        std::set<int> validEmaIds;
        std::set<int> validSupertrendIds;
        std::set<int> validStochasticIds;
        std::set<int> validAtrIds;
        std::set<int> validPivotPointsIds;
        std::set<int> validCciIds;
        std::set<int> validMacdIds;
        std::set<int> validBbIds;

        // Indicator data
        std::map<int, std::vector<double>> rsiValues;
        std::map<int, std::vector<double>> emaValues;
        std::map<int, std::pair<std::vector<double>, std::vector<int>>> supertrendValues; // Values + directions
        std::map<int, std::pair<std::vector<double>, std::vector<double>>> stochasticValues;
        std::map<int, std::vector<double>> atrValues;
        std::map<int, std::vector<double>> cciValues;
        std::map<int, std::tuple<std::vector<double>, std::vector<double>, std::vector<double>>> macdValues; // macd_line, signal_line, histogram
        std::map<int, std::tuple<std::vector<double>, std::vector<double>, std::vector<double>>> bbValues; // middle_band, upper_band, lower_band
        AggregationLevel level;

        // Utility methods to check if a specific indicator is valid
        bool isRsiValid(int id) const { return validRsiIds.find(id) != validRsiIds.end(); }
        bool isEmaValid(int id) const { return validEmaIds.find(id) != validEmaIds.end(); }
        bool isSupertrendValid(int id) const { return validSupertrendIds.find(id) != validSupertrendIds.end(); }
        bool isStochasticValid(int id) const { return validStochasticIds.find(id) != validStochasticIds.end(); }
        bool isAtrValid(int id) const { return validAtrIds.find(id) != validAtrIds.end(); }
        bool isPivotPointsValid(int id) const { return validPivotPointsIds.find(id) != validPivotPointsIds.end(); }
        bool isCciValid(int id) const { return validCciIds.find(id) != validCciIds.end(); }
        bool isMacdValid(int id) const { return validMacdIds.find(id) != validMacdIds.end(); }
        bool isBbValid(int id) const { return validBbIds.find(id) != validBbIds.end(); }
    };

    struct EquityData {
        std::vector<double> timestamps;
        std::vector<double> equity_values;
    };

    // Marker types for drawing tools
    enum class MarkerType {
        Check,   // Green check mark
        Error    // Red error mark
    };

    // Structure to store a marker placed on the chart
    struct ChartMarker {
        size_t barIndex;   // Absolute bar index in raw data
        double price;      // Y coordinate (price)
        MarkerType type;   // Type of marker
        int color = -1;    // RGB 0xRRGGBB, -1 to use default color by marker type
        
        bool operator==(const ChartMarker& other) const {
            return barIndex == other.barIndex && 
                   price == other.price && 
                   type == other.type &&
                   color == other.color;
        }
    };
}



namespace indicators {
    enum class Type {
        RSI,
        EMA,
        STOCHASTIC,
        ATR, 
        SUPERTREND,
        PIVOTPOINTS,
        CCI,
        MACD,
        BB
    };

    enum class PivotPeriodType {
        FourHour,   // Pivot points every 4 hours
        Daily,      // Daily pivot points
        Weekly,     // Weekly pivot points
        Monthly     // Monthly pivot points
    };

    enum class PivotCalculationMethod {
        HLC,     // High, Low, Close (standard method)
        OHLC,    // Open, High, Low, Close
        HLO      // High, Low, Open
    };

    namespace params {
        struct RSI {
            int period = 14;

            bool operator==(const RSI& other) const = default;
            bool operator!=(const RSI& other) const = default;
        };
        
        struct EMA {
            int period = 20;

            bool operator==(const EMA& other) const = default;
            bool operator!=(const EMA& other) const = default;
        };
        
        struct Stochastic {
            int fastKPeriod = 14;
            int slowKPeriod = 3;
            int slowDPeriod = 3;

            bool operator==(const Stochastic& other) const = default;
            bool operator!=(const Stochastic& other) const = default;
        };
        
        struct ATR {
            int period = 14;
            bool useLogScale = false;

            bool operator==(const ATR& other) const = default;
            bool operator!=(const ATR& other) const = default;
        };
        
        struct SuperTrend {
            int period = 10;
            double multiplier = 3.0;

            bool operator==(const SuperTrend& other) const = default;
            bool operator!=(const SuperTrend& other) const = default;
        };
        
        struct PivotPoints {
            PivotPeriodType periodType = PivotPeriodType::Daily;
            PivotCalculationMethod calculationMethod = PivotCalculationMethod::HLC;

            bool operator==(const PivotPoints& other) const = default;
            bool operator!=(const PivotPoints& other) const = default;
        };
        
        struct CCI {
            int period = 20;

            bool operator==(const CCI& other) const = default;
            bool operator!=(const CCI& other) const = default;
        };

        struct MACD {
            int fastPeriod = 12;
            int slowPeriod = 26;
            int signalPeriod = 9;
            filter::PriceType source = filter::PriceType::CLOSE; // Data source (open, high, low, close, hl2, hlc3, ohlc4)
            filter::MAType osc_ma_type = filter::MAType::EMA; // Moving average type for the oscillator (SMA, EMA, WMA)
            filter::MAType signal_ma_type = filter::MAType::EMA; // Moving average type for the signal line (SMA, EMA, WMA)
            int signal_smoothing = 1; // Additional smoothing for the signal line

            bool operator==(const MACD& other) const = default;
            bool operator!=(const MACD& other) const = default;
        };
        struct BB {
            int period = 20;
            double stddev_multiplier = 2.0;
            filter::PriceType source = filter::PriceType::CLOSE; // Data source (open, high, low, close, hl2, hlc3, ohlc4)
            filter::MAType ma_type = filter::MAType::SMA; // Moving average type (SMA, EMA)

            bool operator==(const BB& other) const = default;
            bool operator!=(const BB& other) const = default;
        };
    }

    struct IndicatorSignal {        
        Type type;
        
        // Union of all possible parameter types
        union ParamsUnion {
            params::RSI rsi;
            params::EMA ema;
            params::Stochastic stochastic;
            params::ATR atr;
            params::SuperTrend supertrend;
            params::PivotPoints pivotpoints;
            params::CCI cci;
            params::MACD macd;
            params::BB bb;
            
            ParamsUnion() {} // Union requires a default constructor
            ~ParamsUnion() {} // And a destructor
        } params;
    };

    struct IndicatorBase {
        // IndicatorBase(Type type) : type_(type) {}
        IndicatorBase() = default;
        int id = -1; // Unique identifier of the indicator
        // Type type_; // Indicator type
        bool visible = true; // Whether the indicator is visible
        bool resetOnNewDay = true; // Reset indicator calculations at each new trading day

        virtual bool isCalculationParamsEqual(const IndicatorBase& other) const = 0;
        
        // New method to get the display name of the indicator
        virtual QString getDisplayName() const = 0;
        virtual std::unique_ptr<IndicatorBase> clone() const = 0;

        virtual void setDefaults() = 0;

        virtual ~IndicatorBase() = default;

        // bool operator==(const IndicatorBase& other) const {
        //     return id == other.id && type_ == other.type_;
        // }

        // bool operator!=(const IndicatorBase& other) const {
        //     return !(*this == other);
        // }
    };

    struct RSIInstance : public IndicatorBase {
        RSIInstance() : IndicatorBase() {
            setDefaults();
        }

        RSIInstance(params::RSI p) : IndicatorBase() {
            setDefaults();
            period = p.period;
        }

        // static constexpr Type staticType = Type::RSI;

        int period;             // RSI period
        int height;             // Panel height
        int color;              // Main line color (default purple)
        int overboughtLevel;    // Overbought level
        int oversoldLevel;      // Oversold level
        int upperColor;         // Color for overbought area
        int lowerColor;         // Color for oversold area

        bool isCalculationParamsEqual(const IndicatorBase& other) const override {
            const RSIInstance* otherRSI = dynamic_cast<const RSIInstance*>(&other);
            if (!otherRSI) return false;
            return period == otherRSI->period &&
                   resetOnNewDay == otherRSI->resetOnNewDay;
        }

        std::unique_ptr<IndicatorBase> clone() const override {
            return std::make_unique<RSIInstance>(*this);
        }
        
        QString getDisplayName() const override {
            return QString("RSI (%1)").arg(period);
        }

        void setDefaults() override {
            period = 14;
            height = 120;
            color = 0x800080; // Purple
            overboughtLevel = 80;
            oversoldLevel = 20;
            upperColor = 0xff6666; // Light red
            lowerColor = 0x6666ff; // Light blue
        }
    };

    struct EMAInstance : public IndicatorBase {
        EMAInstance() : IndicatorBase() {
            setDefaults();
        }
        int period;            // EMA period
        int color;             // Line color (default blue)

        bool isCalculationParamsEqual(const IndicatorBase& other) const override {
            const EMAInstance* otherEMA = dynamic_cast<const EMAInstance*>(&other);
            if (!otherEMA) return false;
            return period == otherEMA->period &&
                   resetOnNewDay == otherEMA->resetOnNewDay;
        }

        std::unique_ptr<IndicatorBase> clone() const override {
            return std::make_unique<EMAInstance>(*this);
        }
        
        QString getDisplayName() const override {
            return QString("EMA (%1)").arg(period);
        }

        void setDefaults() override {
            period = 20;
            color = 0x0000FF; // Default blue
        }
    };

    struct SuperTrendInstance : public IndicatorBase {
        SuperTrendInstance() : IndicatorBase() {
            setDefaults();
        }
        int period;            // Period for SuperTrend
        double multiplier;     // Multiplier for SuperTrend
        int upColor;           // Line color (default green)
        int downColor;         // Line color (default red)

        bool isCalculationParamsEqual(const IndicatorBase& other) const override {
            const SuperTrendInstance* otherST = dynamic_cast<const SuperTrendInstance*>(&other);
            if (!otherST) return false;
            return period == otherST->period && 
                   multiplier == otherST->multiplier &&
                   resetOnNewDay == otherST->resetOnNewDay;
        }

        std::unique_ptr<IndicatorBase> clone() const override {
            return std::make_unique<SuperTrendInstance>(*this);
        }
        
        QString getDisplayName() const override {
            return QString("Supertrend (%1, %2)").arg(period).arg(multiplier, 0, 'f', 1);
        }

        void setDefaults() override {
            period = 10;
            multiplier = 3.0;
            upColor = 0xFFA500; // Orange
            downColor = 0x8B008B; // Very dark magenta
        }
    };

    struct StochasticInstance : public IndicatorBase {
        StochasticInstance() : IndicatorBase() {
            setDefaults();
        }
        int fastKPeriod;        // Period to compute raw %K
        int slowKPeriod;        // Smoothing period for %K
        int slowDPeriod;        // Period to compute %D
        int height;             // Panel height
        int kColor;             // %K line color (default blue)
        int dColor;             // %D line color (default red)
        int overboughtLevel;    // Overbought level
        int oversoldLevel;      // Oversold level

        bool isCalculationParamsEqual(const IndicatorBase& other) const override {
            const StochasticInstance* otherStochastic = dynamic_cast<const StochasticInstance*>(&other);
            if (!otherStochastic) return false;
            return fastKPeriod == otherStochastic->fastKPeriod &&
                slowKPeriod == otherStochastic->slowKPeriod &&
                slowDPeriod == otherStochastic->slowDPeriod &&
                resetOnNewDay == otherStochastic->resetOnNewDay;
        }

        std::unique_ptr<IndicatorBase> clone() const override {
            return std::make_unique<StochasticInstance>(*this);
        }
        
        QString getDisplayName() const override {
            return QString("Stochastic (%1,%2,%3)").arg(fastKPeriod).arg(slowKPeriod).arg(slowDPeriod);
        }

        void setDefaults() override {
            fastKPeriod = 14;
            slowKPeriod = 3;
            slowDPeriod = 3;
            height = 120;
            kColor = 0x0000FF;
            dColor = 0xFF0000;
            overboughtLevel = 80;
            oversoldLevel = 20;
        }
    };

    struct ATRInstance : public IndicatorBase {
        ATRInstance() : IndicatorBase() {
            setDefaults();
        }
        int period;            // ATR period
        int height;            // Panel height
        int color;             // Line color (default dark green)
        bool useLogScale;      // Indicates whether logarithmic scale is used

        bool isCalculationParamsEqual(const IndicatorBase& other) const override {
            const ATRInstance* otherATR = dynamic_cast<const ATRInstance*>(&other);
            if (!otherATR) return false;
            return period == otherATR->period && 
                   useLogScale == otherATR->useLogScale &&
                   resetOnNewDay == otherATR->resetOnNewDay;
        }

        std::unique_ptr<IndicatorBase> clone() const override {
            return std::make_unique<ATRInstance>(*this);
        }
        
        QString getDisplayName() const override {
            return QString("ATR (%1)").arg(period);
        }

        void setDefaults() override {
            period = 14;
            height = 120;
            color = 0x006400;
            useLogScale = false;
        }
    };

    struct PivotPointsInstance : public IndicatorBase {
        enum class LevelType {
            R3,         // Resistance 3
            R2,         // Resistance 2
            R1,         // Resistance 1
            Pivot,      // Main pivot point (PP)
            S1,         // Support 1
            S2,         // Support 2
            S3,         // Support 3
            M_R2R3,     // Mid between R2 and R3
            M_R1R2,     // Mid between R1 and R2
            M_PR1,      // Mid between PP and R1
            M_PS1,      // Mid between PP and S1
            M_S1S2,     // Mid between S1 and S2
            M_S2S3,     // Mid between S2 and S3

            Count   // Total number of levels
        };

        enum class LineStyle {
            Solid,
            Dash,
            Dot,
            DotDash,
            AltDash
        };

        struct LevelStyle {
            int color = 0x000000;     // Line color
            int thickness = 2;    // Thickness (1-3)
            LineStyle lineStyle = LineStyle::Solid; // Style (solid, dash, dot, etc.)
            bool visible = false;     // Level visibility

            QString labelFormat = QString(); // Optional display format (e.g., "PP: %.2f")
        };

        struct PivotPeriod {
            // Mapping to aggregated indices for different aggregation levels
            std::array<std::pair<size_t, size_t>, static_cast<size_t>(chart::AggregationLevel::Count)> indices; // Raw indices for each aggregation level
            
            // Values for all pivot levels for this period
            std::array<double, static_cast<size_t>(LevelType::Count)> levelValues;
        };

        PivotPointsInstance() : IndicatorBase() {
            setDefaults();
        }

        PivotPeriodType periodType;                          // Period type (4H, daily, weekly, monthly)
        PivotCalculationMethod calculationMethod;            // Calculation method for pivot points
        std::array<LevelStyle, static_cast<size_t>(LevelType::Count)> levelStyles; // Default styles for each level
        bool showLabels = true;                         // Show level labels


        bool isCalculationParamsEqual(const IndicatorBase& other) const override {
            const PivotPointsInstance* otherPP = dynamic_cast<const PivotPointsInstance*>(&other);
            if (!otherPP) return true;
            return periodType == otherPP->periodType && calculationMethod == otherPP->calculationMethod;
        }

        std::unique_ptr<IndicatorBase> clone() const override {
            return std::make_unique<PivotPointsInstance>(*this);
        }
        
        QString getDisplayName() const override {
            QString periodStr;
            switch (periodType) {
                case PivotPeriodType::FourHour: periodStr = "4H"; break;
                case PivotPeriodType::Daily: periodStr = "Daily"; break;
                case PivotPeriodType::Weekly: periodStr = "Weekly"; break;
                case PivotPeriodType::Monthly: periodStr = "Monthly"; break;
            }
            return QString("Pivot Points (%1)").arg(periodStr);
        }

        bool isLevelVisible(LevelType level) const {
            return levelStyles[static_cast<size_t>(level)].visible;
        }

        void setDefaults() override {
            // Central pivot (black, solid line, visible)
            LevelStyle pivotStyle;
            pivotStyle.color = 0x000000;  // Black
            pivotStyle.thickness = 2;
            pivotStyle.lineStyle = LineStyle::Solid;
            pivotStyle.visible = true;
            pivotStyle.labelFormat = "PP %1";
            levelStyles[static_cast<size_t>(LevelType::Pivot)] = pivotStyle;

            // Resistances (red, solid line, visible)
            LevelStyle resistanceStyle;
            resistanceStyle.color = 0xFF0000;  // Red
            resistanceStyle.thickness = 2;
            resistanceStyle.lineStyle = LineStyle::Solid;
            resistanceStyle.visible = true;
            
            resistanceStyle.labelFormat = "R1 %1";
            levelStyles[static_cast<size_t>(LevelType::R1)] = resistanceStyle;

            resistanceStyle.labelFormat = "R2 %1";
            levelStyles[static_cast<size_t>(LevelType::R2)] = resistanceStyle;

            resistanceStyle.labelFormat = "R3 %1";
            levelStyles[static_cast<size_t>(LevelType::R3)] = resistanceStyle;

            // Supports (green, solid line, visible)
            LevelStyle supportStyle;
            supportStyle.color = 0x008000;  // Green
            supportStyle.thickness = 2;
            supportStyle.lineStyle = LineStyle::Solid;
            supportStyle.visible = true;

            supportStyle.labelFormat = "S1 %1";
            levelStyles[static_cast<size_t>(LevelType::S1)] = supportStyle;

            supportStyle.labelFormat = "S2 %1";
            levelStyles[static_cast<size_t>(LevelType::S2)] = supportStyle;

            supportStyle.labelFormat = "S3 %1";
            levelStyles[static_cast<size_t>(LevelType::S3)] = supportStyle;

            // Mid resistance levels (red, dashed line, not visible by default)
            LevelStyle midResistanceStyle;
            midResistanceStyle.color = 0xFF0000;  // Red
            midResistanceStyle.thickness = 1;
            midResistanceStyle.lineStyle = LineStyle::Dash;
            midResistanceStyle.visible = false;  // Visible if showMidLevels is true
            
            midResistanceStyle.labelFormat = "mR3 %1";
            levelStyles[static_cast<size_t>(LevelType::M_R2R3)] = midResistanceStyle;

            midResistanceStyle.labelFormat = "mR2 %1";
            levelStyles[static_cast<size_t>(LevelType::M_R1R2)] = midResistanceStyle;

            midResistanceStyle.labelFormat = "mR1 %1";
            levelStyles[static_cast<size_t>(LevelType::M_PR1)] = midResistanceStyle;

            // Mid support levels (green, dashed line, not visible by default)
            LevelStyle midSupportStyle;
            midSupportStyle.color = 0x008000;  // Green
            midSupportStyle.thickness = 1;
            midSupportStyle.lineStyle = LineStyle::Dash;
            midSupportStyle.visible = false;  // Visible if showMidLevels is true

            midSupportStyle.labelFormat = "mS1 %1";
            levelStyles[static_cast<size_t>(LevelType::M_PS1)] = midSupportStyle;

            midSupportStyle.labelFormat = "mS2 %1";
            levelStyles[static_cast<size_t>(LevelType::M_S1S2)] = midSupportStyle;

            midSupportStyle.labelFormat = "mS3 %1";
            levelStyles[static_cast<size_t>(LevelType::M_S2S3)] = midSupportStyle;

            // Enable labels by default
            showLabels = true;
            
            // Default period type
            periodType = PivotPeriodType::Daily;

            calculationMethod = PivotCalculationMethod::HLC;
        }
    };

    struct CCIInstance : public IndicatorBase {
        CCIInstance() : IndicatorBase() {
            setDefaults();
        }

        CCIInstance(params::CCI p) : IndicatorBase() {
            setDefaults();
            period = p.period;
        }

        int period;            // CCI period
        int height;            // Panel height
        int color;             // Main line color
        int upperLevel;        // Upper level (typically +100)
        int lowerLevel;        // Lower level (typically -100)
        int upperColor;        // Color for upper area
        int lowerColor;        // Color for lower area

        bool isCalculationParamsEqual(const IndicatorBase& other) const override {
            const CCIInstance* otherCCI = dynamic_cast<const CCIInstance*>(&other);
            if (!otherCCI) return false;
            return period == otherCCI->period &&
                   resetOnNewDay == otherCCI->resetOnNewDay;
        }

        std::unique_ptr<IndicatorBase> clone() const override {
            return std::make_unique<CCIInstance>(*this);
        }
        
        QString getDisplayName() const override {
            return QString("CCI (%1)").arg(period);
        }

        void setDefaults() override {
            period = 20;
            height = 120;
            color = 0xFFA500;      // Orange
            upperLevel = 100;
            lowerLevel = -100;
            upperColor = 0xff6666; // Light red
            lowerColor = 0x6666ff; // Light blue
        }
    };
    struct MACDInstance : public IndicatorBase {
        MACDInstance() : IndicatorBase() {
            setDefaults();
        }

        MACDInstance(params::MACD p) : IndicatorBase() {
            setDefaults();
            fastPeriod = p.fastPeriod;
            slowPeriod = p.slowPeriod;
            signalPeriod = p.signalPeriod;

        }

        int fastPeriod;        // Fast period
        int slowPeriod;        // Slow period
        int signalPeriod;      // Signal line period
        filter::PriceType source;        // Data source (open, high, low, close, hl2, hlc3, ohlc4)
        filter::MAType osc_ma_type;   // Moving average type for the oscillator (SMA, EMA, WMA)
        filter::MAType signal_ma_type;// Moving average type for the signal line (SMA, EMA, WMA)
        int signal_smoothing;  // Additional smoothing for the signal line
        int height;            // Panel height
        int macdColor;         // MACD line color
        int signalColor;       // Signal line color
        int histogramColor;    // Histogram color

        bool isCalculationParamsEqual(const IndicatorBase& other) const override {
            const MACDInstance* otherMACD = dynamic_cast<const MACDInstance*>(&other);
            if (!otherMACD) return false;
            return fastPeriod == otherMACD->fastPeriod &&
                   slowPeriod == otherMACD->slowPeriod &&
                   signalPeriod == otherMACD->signalPeriod &&
                   source == otherMACD->source &&
                   osc_ma_type == otherMACD->osc_ma_type &&
                   signal_ma_type == otherMACD->signal_ma_type &&
                   signal_smoothing == otherMACD->signal_smoothing &&
                   resetOnNewDay == otherMACD->resetOnNewDay &&
                   signal_smoothing == otherMACD->signal_smoothing;
        }

        std::unique_ptr<IndicatorBase> clone() const override {
            return std::make_unique<MACDInstance>(*this);
        }
        
        QString getDisplayName() const override {
            return QString("MACD (%1,%2,%3)").arg(fastPeriod).arg(slowPeriod).arg(signalPeriod);
        }

        void setDefaults() override {
            fastPeriod = 12;
            slowPeriod = 26;
            signalPeriod = 9;
            source = filter::PriceType::CLOSE;
            osc_ma_type = filter::MAType::EMA;
            signal_ma_type = filter::MAType::EMA;
            signal_smoothing = 0;
            height = 200;
            macdColor = 0x0000ff;      // Blue
            signalColor = 0xff0000;    // Red
            histogramColor = 0x808080; // Gray
        }
    };
    struct BBInstance : public IndicatorBase {
        BBInstance() : IndicatorBase() {
            setDefaults();
        }

        BBInstance(params::BB p) : IndicatorBase() {
            setDefaults();
            period = p.period;
            stddev_multiplier = p.stddev_multiplier;
            source = p.source;
            ma_type = p.ma_type;
        }

        int period;                // Band period
        double stddev_multiplier;  // Standard deviation multiplier
        filter::PriceType source;      // Data source (open, high, low, close, hl2, hlc3, ohlc4)
        filter::MAType ma_type;       // Moving average type (SMA, EMA)
        int height;                // Panel height
        int middleBandColor;       // Middle band color
        int upperBandColor;        // Upper band color
        int lowerBandColor;        // Lower band color
        int fillColor;             // Fill color between bands

        bool isCalculationParamsEqual(const IndicatorBase& other) const override {
            const BBInstance* otherBB = dynamic_cast<const BBInstance*>(&other);
            if (!otherBB) return false;
            return period == otherBB->period &&
                   stddev_multiplier == otherBB->stddev_multiplier &&
                   source == otherBB->source &&
                   ma_type == otherBB->ma_type &&
                   resetOnNewDay == otherBB->resetOnNewDay;
        }

        std::unique_ptr<IndicatorBase> clone() const override {
            return std::make_unique<BBInstance>(*this);
        }
        
        QString getDisplayName() const override {
            return QString("Bollinger Bands (%1,%2)").arg(period).arg(stddev_multiplier, 0, 'f', 1);
        }

        void setDefaults() override {
            period = 20;
            stddev_multiplier = 2.0;
            source = filter::PriceType::CLOSE;
            ma_type = filter::MAType::SMA;
            height = 200;
            middleBandColor = 0x0000FF; // Blue
            upperBandColor = 0xFF0000;  // Red
            lowerBandColor = 0x00FF00;  // Green
            fillColor = 0xADD8E6;       // Light blue
        }
    };
}