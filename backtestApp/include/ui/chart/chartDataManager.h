#pragma once

#include <vector>
#include <memory>
#include <unordered_map>
#include <set>
#include <map>
#include <algorithm>
#include <cmath>

#include "data.hpp"
#include "beTypes.h"
#include "chartdir.h"
#include "ui/chart/chartTypes.h"
#include "components/Utils/IndicatorMathUtils.h"


class ChartDataManager {
public:
    ChartDataManager();
    ~ChartDataManager();

    // Methods for data management
    void setData(const std::vector<be::Candle>& data, const std::vector<be::TradeData>& trades, const std::vector<be::EquityPoint>& equityCurve);
    chart::AggregationInfo getOptimalAggregationInfo(const DoubleArray& timestamps);
    
    // Accessors
    const chart::AggregatedOHLCV& getAggregatedData(chart::AggregationLevel level) const { return m_aggregatedOHLCVCache[static_cast<size_t>(level)];}
    const std::vector<be::TradeData>& getTrades() const { return m_trades; }
    const chart::EquityData& getEquityData() const { return m_equityData; }
    const chart::OHLC& getHeikinAshiCache() const { return m_heikinAshiCache; }
    const std::vector<double>& getTimestamps() const { return m_aggregatedOHLCVCache[static_cast<size_t>(chart::AggregationLevel::Raw)].timestamps; }
    const std::vector<std::unique_ptr<indicators::IndicatorBase>>& getIndicators() const { return m_indicators; }
    const chart::IndicatorData& getAggregatedIndicators(chart::AggregationLevel level) const { return m_aggregatedIndicatorsCache[static_cast<size_t>(level)]; }
    const std::map<int, std::vector<indicators::PivotPointsInstance::PivotPeriod>>& getPivotPeriods() const { return m_pivotPeriods; }
    std::optional<std::pair<size_t, size_t>> getTradeAggregatedIndices(size_t tradeIndex, chart::AggregationLevel level) const;
    bool hasRawData() const { return m_aggregatedOHLCVCache[static_cast<size_t>(chart::AggregationLevel::Raw)].isValid; }

    static DoubleArray vectorToDoubleArray(const std::vector<double>& vec);

    int getMaxDisplayPoints() const { return m_maxDisplayPoints; }
    void setMaxDisplayPoints(int value);

    /**
     * @brief Convert a raw index to an aggregated index
     * @param level Aggregation level
     * @param rawIndex Index in the raw data
     * @return Corresponding index in the aggregated data, or nullopt if not found
     */
    std::optional<size_t> rawToAggregatedIndex(chart::AggregationLevel level, size_t rawIndex) const;
    
    /**
     * @brief Get all raw indices for a given aggregated index
     * @param level Aggregation level
     * @param aggregatedIndex Index in the aggregated data
     * @return Vector of corresponding raw indices
     */
    const std::vector<size_t>& getAggregatedToRawIndices(chart::AggregationLevel level, size_t aggregatedIndex) const;
    
    /**
     * @brief Get the first raw index for a given aggregated index
     * @param level Aggregation level
     * @param aggregatedIndex Index in the aggregated data
     * @return First corresponding raw index, or -1 if not found
     */
    int aggregatedToFirstRawIndex(chart::AggregationLevel level, int aggregatedIndex) const;

    
    void removeAllIndicators();

    // Marker management
    void addMarker(const chart::ChartMarker& marker);
    void removeMarker(size_t index);
    void clearAllMarkers();
    const std::vector<chart::ChartMarker>& getMarkers() const { return m_markers; }
    void setMarkers(const std::vector<chart::ChartMarker>& markers) { m_markers = markers; }


    // Here data transfer with move semantics needs to be implemented
    template<typename T, typename = std::enable_if_t<std::is_base_of_v<indicators::IndicatorBase, T>>>
    int addIndicator(const T& config) {
        T copy = config;
        copy.id = m_nextIndicatorId++;
        int id = copy.id;

        std::unique_ptr<indicators::IndicatorBase> indicator = std::make_unique<T>(copy);

        m_indicators.push_back(std::move(indicator));

        calculateIndicator(*m_indicators.back());

        return id;
    }

    template<typename T, typename = std::enable_if_t<std::is_base_of_v<indicators::IndicatorBase, T>>>
    T* findIndicator(int id) const {
        for (auto& indicator : m_indicators)
            if (indicator->id == id)
                return dynamic_cast<T*>(indicator.get());
        return nullptr;
    }

    template<typename T, typename = std::enable_if_t<std::is_base_of_v<indicators::IndicatorBase, T>>>
    bool updateIndicator(const T& config) {
        T* indicator = findIndicator<T>(config.id);

        if (!indicator) return false;

        bool isCalculationParamsEqual = indicator->isCalculationParamsEqual(config);

        *indicator = config; // Update the indicator configuration

        if (!isCalculationParamsEqual)
            calculateIndicator(config);

        return true;
    }

    bool removeIndicator(int id) {
        auto it = std::find_if(m_indicators.begin(), m_indicators.end(), [id](const std::unique_ptr<indicators::IndicatorBase>& item) {  
            return item->id == id; 
        });

        if (it == m_indicators.end()) 
            return false;

        m_indicators.erase(it);

        return true;
    }

    template<typename T, typename = std::enable_if_t<std::is_base_of_v<indicators::IndicatorBase, T>>>
    std::vector<const T*> getIndicatorsOfType() const {
        std::vector<const T*> result;
        for (const auto& indicator : m_indicators) {
            if (const T* typedIndicator = dynamic_cast<const T*>(indicator.get())) {
                result.push_back(typedIndicator);
            }
        }
        return result;
    }
    
private:
    bool configureAggregationSelector(ArrayMath& math, chart::AggregationLevel level) const;
    void aggregateOHLCV(chart::AggregationLevel level);
    void aggregateIndicators(chart::AggregationLevel level);
    std::vector<double> aggregateVector(const std::vector<double>& data, chart::AggregationLevel level, int aggregateMethod) const;
    std::vector<int> aggregateVector(const std::vector<int>& data, chart::AggregationLevel level, int aggregateMethod) const;
    void updateHeikinAshiCache();

    void calculateIndicator(const indicators::IndicatorBase& config);
    // We should not pass ids but return the calculated values to put them into the cache afterwards; it should not be done inside this function
    // so there's no need for the id
    void calculateRSI(int id, int period);
    void calculateEMA(int id, int period);
    void calculateSupertrend(int id, int period, double multiplier);
    void calculateStochastic(int id, int fastKPeriod, int slowKPeriod, int slowDPeriod);
    void calculateATR(int id, int period, bool useLogScale = false);
    void calculateCCI(int id, int period);
    void calculateMACD(int id,int fastPeriod, int slowPeriod, int signalPeriod, filter::PriceType source, filter::MAType osc_ma_type, filter::MAType signal_ma_type, int signal_smoothing);
    void calculateBB(int id, int period, double stdDevMultiplier, filter::PriceType source, filter::MAType osc_ma_type);
    // We may not need to pass the complete instance but why not; if we do that, we do it for all indicators
    void calculatePivotPoints(const indicators::PivotPointsInstance& config);

    // two methods that use the same logic should be refactored
    void precalculatePivotIndices(std::vector<indicators::PivotPointsInstance::PivotPeriod>& periods, chart::AggregationLevel level);
    void precalculateTradeIndices(chart::AggregationLevel level);


    // Internal utilities
    double dateToChartTimestamp(const be::Date& date) const;
    size_t findClosestIndex(const std::vector<double>& values, double target, bool searchForward = false) const;
    chart::AggregationLevel determineStartingAggregationLevel(const DoubleArray& timestamps) const;
    
    // Data
    std::vector<be::Date> m_datesCache; // same, should be moved into AggregatedOHLCV (temporary)
    std::array<chart::AggregatedOHLCV, static_cast<size_t>(chart::AggregationLevel::Count)> m_aggregatedOHLCVCache;
    std::array<chart::IndicatorData, static_cast<size_t>(chart::AggregationLevel::Count)> m_aggregatedIndicatorsCache;
    // pivot points do not aggregate like other indicators; they natively support aggregation
    std::map<int, std::vector<indicators::PivotPointsInstance::PivotPeriod>> m_pivotPeriods;
    chart::OHLC m_heikinAshiCache;
    std::vector<be::TradeData> m_trades;
    std::vector<std::array<std::pair<size_t, size_t>, static_cast<size_t>(chart::AggregationLevel::Count)>> m_tradeIndices; // Indices for each trade
    chart::EquityData m_equityData;
    std::vector<chart::ChartMarker> m_markers; // User-placed markers on the chart

    // Global unique ID for all indicator types
    int m_nextIndicatorId = 1;
    std::vector<std::unique_ptr<indicators::IndicatorBase>> m_indicators; ///< All active indicator instances

    // Constants
    int m_maxDisplayPoints = 30000;
};