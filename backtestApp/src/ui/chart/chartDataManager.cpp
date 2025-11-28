#include "ui/chart/chartDataManager.h"
#include "common.h"
#include <QDebug>
#include <iostream>

ChartDataManager::ChartDataManager() {
}

ChartDataManager::~ChartDataManager() {
}

void ChartDataManager::setData(const std::vector<be::Candle>& candles, const std::vector<be::TradeData>& trades, const std::vector<be::EquityPoint>& equityCurve) {
    if (candles.empty()) return;

    // Check if the data is different from the current data
    bool dataChanged = true;
    
    // If we have cached data, check if it has changed
    if (m_aggregatedOHLCVCache[static_cast<size_t>(chart::AggregationLevel::Raw)].isValid) {
        // Check if the size has changed
        if (m_aggregatedOHLCVCache[static_cast<size_t>(chart::AggregationLevel::Raw)].close.size() == candles.size()) {
            // Check a few points to see if the data is identical
            // We check the first, last, and a middle point
            const std::vector<double>& cachedClose = m_aggregatedOHLCVCache[static_cast<size_t>(chart::AggregationLevel::Raw)].close;
            
            size_t size = cachedClose.size();
            dataChanged = false;
            
            // Check the first point
            if (std::abs(cachedClose[0] - candles[0].close) > 1e-10) {
                dataChanged = true;
            } 
            // Check the last point
            else if (std::abs(cachedClose[size-1] - candles[size-1].close) > 1e-10) {
                dataChanged = true;
            }
            // Check a middle point
            else if (std::abs(cachedClose[size/2] - candles[size/2].close) > 1e-10) {
                dataChanged = true;
            }
        }
    }

    // If the data has changed, update the OHLCV cache and recalculate indicators
    if (dataChanged) {
        m_datesCache.resize(candles.size());
        std::vector<double> m_openCache(candles.size());
        std::vector<double> m_highCache(candles.size());
        std::vector<double> m_lowCache(candles.size());
        std::vector<double> m_closeCache(candles.size());
        std::vector<double> m_volumeCache(candles.size());

        for (size_t i = 0; i < candles.size(); ++i) {
            m_datesCache[i] = candles[i].date;
            m_openCache[i] = candles[i].open;
            m_highCache[i] = candles[i].high;
            m_lowCache[i] = candles[i].low;
            m_closeCache[i] = candles[i].close;
            m_volumeCache[i] = candles[i].volume;
        }


        m_aggregatedOHLCVCache.fill(chart::AggregatedOHLCV());
        {
            std::vector<double>& rawTimestamps = m_aggregatedOHLCVCache[static_cast<size_t>(chart::AggregationLevel::Raw)].timestamps;
            std::vector<std::vector<size_t>> oneToOneMapping = m_aggregatedOHLCVCache[static_cast<size_t>(chart::AggregationLevel::Raw)].rawIndicesMapping;

            rawTimestamps.reserve(m_datesCache.size());
            oneToOneMapping.resize(m_datesCache.size());

            for (size_t i = 0; i < m_datesCache.size(); ++i) {
                rawTimestamps.push_back(dateToChartTimestamp(m_datesCache[i]));
                oneToOneMapping[i] = {i};
            }
        }
        m_aggregatedOHLCVCache[static_cast<size_t>(chart::AggregationLevel::Raw)].open = std::move(m_openCache);
        m_aggregatedOHLCVCache[static_cast<size_t>(chart::AggregationLevel::Raw)].high = std::move(m_highCache);
        m_aggregatedOHLCVCache[static_cast<size_t>(chart::AggregationLevel::Raw)].low = std::move(m_lowCache);
        m_aggregatedOHLCVCache[static_cast<size_t>(chart::AggregationLevel::Raw)].close = std::move(m_closeCache);
        m_aggregatedOHLCVCache[static_cast<size_t>(chart::AggregationLevel::Raw)].volume = std::move(m_volumeCache);
        m_aggregatedOHLCVCache[static_cast<size_t>(chart::AggregationLevel::Raw)].isValid = true; // Raw data is always valid

        updateHeikinAshiCache();
        
        m_aggregatedIndicatorsCache.fill(chart::IndicatorData());
        
        
        // Recalculate all existing indicators
        for (const auto& indicator : m_indicators) {
            calculateIndicator(*indicator);
        }
    }



    m_trades = trades;
    m_tradeIndices.clear();
    
    // Preallocate the indices vector
    m_tradeIndices.resize(m_trades.size());

    // Initialize raw indices for all trades
    for (size_t i = 0; i < m_trades.size(); ++i) {
        size_t entryBar = m_trades[i].entryBar;
        size_t exitBar = m_trades[i].hasBeenClosed() ? m_trades[i].exitBar : entryBar;

        // Store raw indices
        m_tradeIndices[i][static_cast<size_t>(chart::AggregationLevel::Raw)] = {entryBar, exitBar};
    }

    // If the aggregation level has already been calculated, recalculate trade indices
    for (size_t i = 1; i < static_cast<size_t>(chart::AggregationLevel::Count); ++i) {
        if (m_aggregatedOHLCVCache[i].isValid) {
            precalculateTradeIndices(static_cast<chart::AggregationLevel>(i));
        }
    }
    
    
    if (equityCurve.empty()) return;
    
    size_t numPoints = equityCurve.size();
    size_t numBars = candles.size();
    
    // Reset equity data
    m_equityData = chart::EquityData();
    
    // Preallocate for the worst case
    m_equityData.timestamps.reserve(numPoints);
    m_equityData.equity_values.reserve(numPoints);
    
    // Iterate through the remaining points
    for (size_t i = 0; i < numPoints; ++i) {
        be::EquityPoint currentValue = equityCurve[i];

        // Ensure the index is within bounds
        if (currentValue.index >= numBars) {
            break;
        }

        m_equityData.timestamps.push_back(dateToChartTimestamp(candles[currentValue.index].date));
        m_equityData.equity_values.push_back(currentValue.value);
    }
}

double ChartDataManager::dateToChartTimestamp(const be::Date& date) const {
    return Chart::chartTime(date.year, date.month, date.day, date.hour, date.minute, date.second);
}

void ChartDataManager::updateHeikinAshiCache() {
    if (!m_aggregatedOHLCVCache[static_cast<size_t>(chart::AggregationLevel::Raw)].isValid) {
        m_heikinAshiCache.isValid = false;
        return;
    }

    std::tie(m_heikinAshiCache.open, m_heikinAshiCache.high, m_heikinAshiCache.low, m_heikinAshiCache.close) = IndicatorMathUtils::calculateHeikinAshi(
        m_aggregatedOHLCVCache[static_cast<size_t>(chart::AggregationLevel::Raw)].open,
        m_aggregatedOHLCVCache[static_cast<size_t>(chart::AggregationLevel::Raw)].high,
        m_aggregatedOHLCVCache[static_cast<size_t>(chart::AggregationLevel::Raw)].low,
        m_aggregatedOHLCVCache[static_cast<size_t>(chart::AggregationLevel::Raw)].close
    );
    
    m_heikinAshiCache.isValid = true;
}

// It seems that we make two copies in this method, one to work on it 
// and avoid modifying the original data, and one to store it in the cache
void ChartDataManager::aggregateOHLCV(chart::AggregationLevel level) {
    // If already cached and valid, do nothing
    if (m_aggregatedOHLCVCache[static_cast<size_t>(level)].isValid)
        return;
        
    // Create a new record in the cache
    chart::AggregatedOHLCV& aggregatedData = m_aggregatedOHLCVCache[static_cast<size_t>(level)];
    
    // Check that we have data to aggregate
    if (!m_aggregatedOHLCVCache[static_cast<size_t>(chart::AggregationLevel::Raw)].isValid) {
        aggregatedData.isValid = false;
        return;
    }

    // This needs to be reworked, I don't like this logic
    // Find the optimal source aggregation level (the highest available below the requested level)
    chart::AggregationLevel sourceLevel = chart::AggregationLevel::Raw;
    
    // Hierarchical order of aggregation levels
    // std::vector<AggregationLevel> levelHierarchy = {
    //     chart::AggregationLevel::Raw,
    //     chart::AggregationLevel::OneMinute,
    //     chart::AggregationLevel::OneHour,
    //     chart::AggregationLevel::OneDay
    // };
    
    // // Find the highest available level below the requested level
    // for (auto it = levelHierarchy.rbegin(); it != levelHierarchy.rend(); ++it) {
    //     if (*it < level && 
    //         m_aggregatedOHLCVCache.find(*it) != m_aggregatedOHLCVCache.end() && 
    //         m_aggregatedOHLCVCache[*it].isValid) {
    //         sourceLevel = *it;
    //         break;
    //     }
    // }
    
    // If no lower level is available, use raw data
    if (sourceLevel == chart::AggregationLevel::Raw && !m_aggregatedOHLCVCache[0].isValid) {
        // Calculate the Raw level if not already calculated
        aggregateOHLCV(chart::AggregationLevel::Raw);
    }
    
    // Get the source data
    const chart::AggregatedOHLCV& sourceData = m_aggregatedOHLCVCache[0];
    
    // First copy to work on the data
    // Make copies of the source data for aggregation
    std::vector<double> timestampsCopy = sourceData.timestamps;
    std::vector<double> openCopy = sourceData.open;
    std::vector<double> highCopy = sourceData.high;
    std::vector<double> lowCopy = sourceData.low;
    std::vector<double> closeCopy = sourceData.close;
    std::vector<double> volumeCopy = sourceData.volume;
    
    // Create the ArrayMath for aggregation
    ArrayMath timestampsMath(vectorToDoubleArray(timestampsCopy));

    if (!configureAggregationSelector(timestampsMath, level)) {
        aggregatedData.isValid = false;
        return;
    }

    timestampsMath.trim(1); // Remove the first NoValue element
    timestampsMath.insert(DoubleArray(&sourceData.timestamps[0], 1), 0);

    // Get the indices after selection
    DoubleArray indices = timestampsMath.result();

    aggregatedData.rawIndicesMapping.clear();
    int currentGroup = -1;
    for (int i = 0; i < indices.len; i++) {
        if (indices[i] != Chart::NoValue) {
            currentGroup++;
            aggregatedData.rawIndicesMapping.push_back({});
        }
        if (currentGroup >= 0) {
            // Add this raw index to the current group
            aggregatedData.rawIndicesMapping[currentGroup].push_back(i);
        }
    }
    
    if (indices.len <= 0) {
        qDebug() << "Aggregation failed - no points selected";
        aggregatedData.isValid = false;
        return;
    }
    
    // Aggregate OHLCV data with appropriate strategies
    DoubleArray times = timestampsMath.aggregate(
        vectorToDoubleArray(timestampsCopy), 
        Chart::AggregateFirst);
    
    DoubleArray open = timestampsMath.aggregate(
        vectorToDoubleArray(openCopy), 
        Chart::AggregateFirst);
    
    DoubleArray high = timestampsMath.aggregate(
        vectorToDoubleArray(highCopy), 
        Chart::AggregateMax);
    
    DoubleArray low = timestampsMath.aggregate(
        vectorToDoubleArray(lowCopy), 
        Chart::AggregateMin);
    
    DoubleArray close = timestampsMath.aggregate(
        vectorToDoubleArray(closeCopy), 
        Chart::AggregateLast);
    
    DoubleArray volume = timestampsMath.aggregate(
        vectorToDoubleArray(volumeCopy), 
        Chart::AggregateSum);

    // Check that all aggregated arrays have the same size
    if (times.len != open.len || times.len != high.len || times.len != low.len || 
        times.len != close.len || times.len != volume.len) {
        aggregatedData.isValid = false;
        return;
    }
    
    // Second copy to store the aggregated data
    // Store the results in the vectors
    aggregatedData.timestamps.resize(times.len);
    aggregatedData.open.resize(open.len);
    aggregatedData.high.resize(high.len);
    aggregatedData.low.resize(low.len);
    aggregatedData.close.resize(close.len);
    aggregatedData.volume.resize(volume.len);

    // Copy the data
    for (int i = 0; i < times.len; i++) {
        aggregatedData.timestamps[i] = times[i];
        aggregatedData.open[i] = open[i];
        aggregatedData.high[i] = high[i];
        aggregatedData.low[i] = low[i];
        aggregatedData.close[i] = close[i];
        aggregatedData.volume[i] = volume[i];
    }
    
    // aggregatedData.level = level;
    aggregatedData.isValid = true;

    // Precalculate trade indices for this new aggregation level
    if (!m_trades.empty() && level != chart::AggregationLevel::Raw) {
        precalculateTradeIndices(level);

        // CORRECTION: Iterate through all pivot points for this level
        for (auto& [pivotId, periods] : m_pivotPeriods) {
            precalculatePivotIndices(periods, level);
        }
    }
}

void ChartDataManager::aggregateIndicators(chart::AggregationLevel level) {
    if (level == chart::AggregationLevel::Raw) return;

    // maybe need to do this also in aggregateOHLCV

    chart::IndicatorData& aggregated = m_aggregatedIndicatorsCache[static_cast<size_t>(level)];

    // need to find the source aggregation level like for OHLCV
    // for now only the Raw level is handled


    for (const auto& [id, values] : m_aggregatedIndicatorsCache[static_cast<size_t>(chart::AggregationLevel::Raw)].rsiValues) {
        if (!aggregated.isRsiValid(id)) {
            // Aggregate the RSI
            std::vector<double> rsiData = aggregateVector(values, level, Chart::AggregateLast);
            if (!rsiData.empty()) {
                aggregated.rsiValues[id] = std::move(rsiData);
                aggregated.validRsiIds.insert(id);
            }
        }
    }

    // EMA
    for (const auto& [id, values] : m_aggregatedIndicatorsCache[static_cast<size_t>(chart::AggregationLevel::Raw)].emaValues) {
        if (!aggregated.isEmaValid(id)) {
            std::vector<double> emaData = aggregateVector(values, level, Chart::AggregateLast);
            if (!emaData.empty()) {
                aggregated.emaValues[id] = std::move(emaData);
                aggregated.validEmaIds.insert(id);
            }
        }
    }

    // Supertrend
    for (const auto& [id, valuesPair] : m_aggregatedIndicatorsCache[static_cast<size_t>(chart::AggregationLevel::Raw)].supertrendValues) {
        if (!aggregated.isSupertrendValid(id)) {
            const auto& [supertrendValues, trendDirections] = valuesPair;
            std::vector<double> supertrendData = aggregateVector(supertrendValues, level, Chart::AggregateLast);
            std::vector<int> directionsData = aggregateVector(trendDirections, level, Chart::AggregateLast);

            if (!supertrendData.empty() && !directionsData.empty() && supertrendData.size() == directionsData.size()) {
                aggregated.supertrendValues[id] = std::make_pair(
                    std::move(supertrendData),
                    std::move(directionsData)
                );
                aggregated.validSupertrendIds.insert(id);
            }
        }
    }

    // Stochastic
    for (const auto& [id, values] : m_aggregatedIndicatorsCache[static_cast<size_t>(chart::AggregationLevel::Raw)].stochasticValues) {
        if (!aggregated.isStochasticValid(id)) {
            const auto& [kValues, dValues] = values;
            std::vector<double> kData = aggregateVector(kValues, level, Chart::AggregateLast);
            std::vector<double> dData = aggregateVector(dValues, level, Chart::AggregateLast);

            if (!kData.empty() && !dData.empty() && kData.size() == dData.size()) {
                aggregated.stochasticValues[id] = std::make_pair(
                    std::move(kData),
                    std::move(dData)
                );
                aggregated.validStochasticIds.insert(id);
            }
        }
    }

    // ATR
    for (const auto& [id, values] : m_aggregatedIndicatorsCache[static_cast<size_t>(chart::AggregationLevel::Raw)].atrValues) {
        if (!aggregated.isAtrValid(id)) {
            std::vector<double> atrData = aggregateVector(values, level, Chart::AggregateLast);
            if (!atrData.empty()) {
                aggregated.atrValues[id] = std::move(atrData);
                aggregated.validAtrIds.insert(id);
            }
        }
    }

    // CCI
    for (const auto& [id, values] : m_aggregatedIndicatorsCache[static_cast<size_t>(chart::AggregationLevel::Raw)].cciValues) {
        if (!aggregated.isCciValid(id)) {
            std::vector<double> cciData = aggregateVector(values, level, Chart::AggregateLast);
            if (!cciData.empty()) {
                aggregated.cciValues[id] = std::move(cciData);
                aggregated.validCciIds.insert(id);
            }
        }
    }

    // MACD
    for (const auto& [id, values] : m_aggregatedIndicatorsCache[static_cast<size_t>(chart::AggregationLevel::Raw)].macdValues) {
        if (!aggregated.isMacdValid(id)) {
            const auto& [macdLine, signalLine, histogram] = values;
            std::vector<double> macdData = aggregateVector(macdLine, level, Chart::AggregateLast);
            std::vector<double> signalData = aggregateVector(signalLine, level, Chart::AggregateLast);
            std::vector<double> histData = aggregateVector(histogram, level, Chart::AggregateLast);
            if (!macdData.empty() && !signalData.empty() && !histData.empty() &&
                macdData.size() == signalData.size() && macdData.size() == histData.size()) {
                aggregated.macdValues[id] = std::make_tuple(
                    std::move(macdData),
                    std::move(signalData),
                    std::move(histData)
                );
                aggregated.validMacdIds.insert(id);
            }
        }

    }

    // Bollinger Bands
    for (const auto& [id, values] : m_aggregatedIndicatorsCache[static_cast<size_t>(chart::AggregationLevel::Raw)].bbValues) {
        if (!aggregated.isBbValid(id)) {
            const auto& [middleBand, upperBand, lowerBand] = values;
            std::vector<double> middleData = aggregateVector(middleBand, level, Chart::AggregateLast);
            std::vector<double> upperData = aggregateVector(upperBand, level, Chart::AggregateLast);
            std::vector<double> lowerData = aggregateVector(lowerBand, level, Chart::AggregateLast);
            if (!middleData.empty() && !upperData.empty() && !lowerData.empty() &&
                middleData.size() == upperData.size() && middleData.size() == lowerData.size()) {
                aggregated.bbValues[id] = std::make_tuple(
                    std::move(middleData),
                    std::move(upperData),
                    std::move(lowerData)
                );
                aggregated.validBbIds.insert(id);
            }
        }
    }
}

std::vector<double> ChartDataManager::aggregateVector(const std::vector<double> &data, chart::AggregationLevel level, int aggregateMethod) const {
    if (level == chart::AggregationLevel::Raw)
        return data;

    std::vector<double> timestamps = m_aggregatedOHLCVCache[static_cast<size_t>(chart::AggregationLevel::Raw)].timestamps;
    std::vector<double> dataCopy = data;

    ArrayMath timestampsMath(DoubleArray(timestamps.data(), static_cast<int>(timestamps.size())));

    if (!configureAggregationSelector(timestampsMath, level))
        return std::vector<double>();

    DoubleArray indices = timestampsMath.result();
    if (indices.len <= 0) {
        qDebug() << "Aggregation failed - no points selected";
        return std::vector<double>();
    }

    DoubleArray result = timestampsMath.aggregate(
        DoubleArray(dataCopy.data(), static_cast<int>(dataCopy.size())), 
        aggregateMethod);
    return std::vector<double>(result.data, result.data + result.len);
}

std::vector<int> ChartDataManager::aggregateVector(const std::vector<int>& data, chart::AggregationLevel level, int aggregateMethod) const {
    std::vector<double> dataCopy(data.begin(), data.end());
    std::vector<double> dataAggregated = aggregateVector(dataCopy, level, aggregateMethod);
    return std::vector<int>(dataAggregated.begin(), dataAggregated.end());
}


void ChartDataManager::calculateRSI(int id, int period) {
    // Verify necessary data is available
    if (!m_aggregatedOHLCVCache[static_cast<size_t>(chart::AggregationLevel::Raw)].isValid || period < 2) return;

    // Get closing prices
    const std::vector<double>& closePrices = m_aggregatedOHLCVCache[static_cast<size_t>(chart::AggregationLevel::Raw)].close;

    std::vector<double> rsiValues = IndicatorMathUtils::calculateRSI(closePrices, period);

    // Update active indicators cache
    m_aggregatedIndicatorsCache[static_cast<size_t>(chart::AggregationLevel::Raw)].rsiValues[id] = std::move(rsiValues);
}

void ChartDataManager::calculateEMA(int id, int period) {
    // Verify necessary data is available
    if (!m_aggregatedOHLCVCache[static_cast<size_t>(chart::AggregationLevel::Raw)].isValid || period < 2) return;

    // Get closing prices
    const std::vector<double>& closePrices = m_aggregatedOHLCVCache[static_cast<size_t>(chart::AggregationLevel::Raw)].close;

    std::vector<double> emaValues = IndicatorMathUtils::calculateEMA(closePrices, period);

    // Update active indicators cache
    m_aggregatedIndicatorsCache[static_cast<size_t>(chart::AggregationLevel::Raw)].emaValues[id] = std::move(emaValues);
}

void ChartDataManager::calculateSupertrend(int id, int period, double multiplier) {
    // Verify necessary data is available
    if (!m_aggregatedOHLCVCache[static_cast<size_t>(chart::AggregationLevel::Raw)].isValid || period < 2) return;

    // Get prices
    const std::vector<double>& highPrices = m_aggregatedOHLCVCache[static_cast<size_t>(chart::AggregationLevel::Raw)].high;
    const std::vector<double>& lowPrices = m_aggregatedOHLCVCache[static_cast<size_t>(chart::AggregationLevel::Raw)].low;
    const std::vector<double>& closePrices = m_aggregatedOHLCVCache[static_cast<size_t>(chart::AggregationLevel::Raw)].close;

    std::vector<double> supertrendValues;
    std::vector<int> trendDirections;
    std::tie(supertrendValues, trendDirections) = IndicatorMathUtils::calculateSupertrend(highPrices, lowPrices, closePrices, period, multiplier);

    // Update active indicators cache
    m_aggregatedIndicatorsCache[static_cast<size_t>(chart::AggregationLevel::Raw)].supertrendValues[id] = std::make_pair(std::move(supertrendValues), std::move(trendDirections));
}

void ChartDataManager::calculateStochastic(int id, int fastKPeriod, int slowKPeriod, int slowDPeriod) {
    // Verify necessary data is available
    if (!m_aggregatedOHLCVCache[static_cast<size_t>(chart::AggregationLevel::Raw)].isValid || fastKPeriod < 2 || slowKPeriod < 2 || slowDPeriod < 2) return;

    // Get prices
    const std::vector<double>& highPrices = m_aggregatedOHLCVCache[static_cast<size_t>(chart::AggregationLevel::Raw)].high;
    const std::vector<double>& lowPrices = m_aggregatedOHLCVCache[static_cast<size_t>(chart::AggregationLevel::Raw)].low;
    const std::vector<double>& closePrices = m_aggregatedOHLCVCache[static_cast<size_t>(chart::AggregationLevel::Raw)].close;

    auto [stochasticKValues, stochasticDValues] = IndicatorMathUtils::calculateStochastic(
        highPrices, lowPrices, closePrices, fastKPeriod, slowKPeriod, slowDPeriod
    );

    // Update active indicators cache
    m_aggregatedIndicatorsCache[static_cast<size_t>(chart::AggregationLevel::Raw)].stochasticValues[id] = {
        std::move(stochasticKValues), std::move(stochasticDValues)
    };
}

void ChartDataManager::calculateATR(int id, int period, bool useLogScale) {
    // Verify necessary data is available
    if (!m_aggregatedOHLCVCache[static_cast<size_t>(chart::AggregationLevel::Raw)].isValid || period < 2) return;

    // Get prices
    const std::vector<double>& highPrices = m_aggregatedOHLCVCache[static_cast<size_t>(chart::AggregationLevel::Raw)].high;
    const std::vector<double>& lowPrices = m_aggregatedOHLCVCache[static_cast<size_t>(chart::AggregationLevel::Raw)].low;
    const std::vector<double>& closePrices = m_aggregatedOHLCVCache[static_cast<size_t>(chart::AggregationLevel::Raw)].close;

    std::vector<double> atrValues = IndicatorMathUtils::calculateATR(highPrices, lowPrices, closePrices, period, useLogScale);

    // Update active indicators cache
    m_aggregatedIndicatorsCache[static_cast<size_t>(chart::AggregationLevel::Raw)].atrValues[id] = std::move(atrValues);
}

void ChartDataManager::calculateCCI(int id, int period) {
    // Verify necessary data is available
    if (!m_aggregatedOHLCVCache[static_cast<size_t>(chart::AggregationLevel::Raw)].isValid || period < 2) return;

    // Get prices
    const std::vector<double>& highPrices = m_aggregatedOHLCVCache[static_cast<size_t>(chart::AggregationLevel::Raw)].high;
    const std::vector<double>& lowPrices = m_aggregatedOHLCVCache[static_cast<size_t>(chart::AggregationLevel::Raw)].low;
    const std::vector<double>& closePrices = m_aggregatedOHLCVCache[static_cast<size_t>(chart::AggregationLevel::Raw)].close;

    std::vector<double> cciValues = IndicatorMathUtils::calculateCCI(highPrices, lowPrices, closePrices, period);

    // Update active indicators cache
    m_aggregatedIndicatorsCache[static_cast<size_t>(chart::AggregationLevel::Raw)].cciValues[id] = std::move(cciValues);
}

void ChartDataManager::calculateMACD(int id, int fastPeriod, int slowPeriod, int signalPeriod, filter::PriceType source, filter::MAType oscMA, filter::MAType signalMA, int signal_smoothing) {
    // Verify necessary data is available
    if (!m_aggregatedOHLCVCache[static_cast<size_t>(chart::AggregationLevel::Raw)].isValid || fastPeriod < 2 || slowPeriod < 2 || signalPeriod < 2) return;

    // Get prices according to source
    const std::vector<double>& openPrices = m_aggregatedOHLCVCache[static_cast<size_t>(chart::AggregationLevel::Raw)].open;
    const std::vector<double>& highPrices = m_aggregatedOHLCVCache[static_cast<size_t>(chart::AggregationLevel::Raw)].high;
    const std::vector<double>& lowPrices = m_aggregatedOHLCVCache[static_cast<size_t>(chart::AggregationLevel::Raw)].low;
    const std::vector<double>& closePrices = m_aggregatedOHLCVCache[static_cast<size_t>(chart::AggregationLevel::Raw)].close;

    std::vector<double> macdLine;
    std::vector<double> signalLine;
    std::vector<double> histogram;
    std::tie(macdLine, signalLine, histogram) = IndicatorMathUtils::calculateMACD(
        openPrices, highPrices, lowPrices, closePrices,
        fastPeriod, slowPeriod, signalPeriod,
        source, oscMA, signalMA, signal_smoothing
    );

    // Update active indicators cache
    m_aggregatedIndicatorsCache[static_cast<size_t>(chart::AggregationLevel::Raw)].macdValues[id] = std::make_tuple(std::move(macdLine), std::move(signalLine), std::move(histogram));
}

void ChartDataManager::calculateBB(int id, int period, double stdDevMultiplier, filter::PriceType source, filter::MAType maType) {
    // Verify necessary data is available
    if (!m_aggregatedOHLCVCache[static_cast<size_t>(chart::AggregationLevel::Raw)].isValid || period < 2) return;

    // Get prices according to source
    const std::vector<double>& openPrices = m_aggregatedOHLCVCache[static_cast<size_t>(chart::AggregationLevel::Raw)].open;
    const std::vector<double>& highPrices = m_aggregatedOHLCVCache[static_cast<size_t>(chart::AggregationLevel::Raw)].high;
    const std::vector<double>& lowPrices = m_aggregatedOHLCVCache[static_cast<size_t>(chart::AggregationLevel::Raw)].low;
    const std::vector<double>& closePrices = m_aggregatedOHLCVCache[static_cast<size_t>(chart::AggregationLevel::Raw)].close;

    std::vector<double> middleBand;
    std::vector<double> upperBand;
    std::vector<double> lowerBand;
    std::tie(middleBand, upperBand, lowerBand) = IndicatorMathUtils::calculateBollingerBands(
        openPrices, highPrices, lowPrices, closePrices,
        period, stdDevMultiplier, source, maType
    );

    // Update active indicators cache
    m_aggregatedIndicatorsCache[static_cast<size_t>(chart::AggregationLevel::Raw)].bbValues[id] = std::make_tuple(std::move(middleBand), std::move(upperBand), std::move(lowerBand));
}

void ChartDataManager::calculatePivotPoints(const indicators::PivotPointsInstance& config) {
    // Verify necessary data is available
    if (!m_aggregatedOHLCVCache[static_cast<size_t>(chart::AggregationLevel::Raw)].isValid) return;
    
    // Get prices
    std::vector<be::Date> dates = m_datesCache;
    std::vector<double> openPrices = m_aggregatedOHLCVCache[static_cast<size_t>(chart::AggregationLevel::Raw)].open;
    std::vector<double> highPrices = m_aggregatedOHLCVCache[static_cast<size_t>(chart::AggregationLevel::Raw)].high;
    std::vector<double> lowPrices = m_aggregatedOHLCVCache[static_cast<size_t>(chart::AggregationLevel::Raw)].low;
    std::vector<double> closePrices = m_aggregatedOHLCVCache[static_cast<size_t>(chart::AggregationLevel::Raw)].close;
    
    // Structure to store calculated levels
    std::vector<indicators::PivotPointsInstance::PivotPeriod> levelSegments = IndicatorMathUtils::calculatePivotPoints(
        openPrices, highPrices, lowPrices, closePrices, dates, 
        config.periodType, config.calculationMethod
    );

    m_pivotPeriods[config.id] = std::move(levelSegments);

    // Precompute indices for all existing aggregation levels
    for (size_t i = 0; i < m_aggregatedOHLCVCache.size(); ++i) {
        if (i == static_cast<size_t>(chart::AggregationLevel::Raw) || !m_aggregatedOHLCVCache[i].isValid) 
            continue;

        precalculatePivotIndices(m_pivotPeriods[config.id], static_cast<chart::AggregationLevel>(i));
    }
}

void ChartDataManager::precalculatePivotIndices(std::vector<indicators::PivotPointsInstance::PivotPeriod>& periods, chart::AggregationLevel level) {
    
    // Check that the level exists in the cache
    const chart::AggregatedOHLCV& aggData = m_aggregatedOHLCVCache[static_cast<size_t>(level)];
    if (!aggData.isValid)
        return;

    const std::vector<std::vector<size_t>>& mapping = aggData.rawIndicesMapping;
    if (mapping.empty() || periods.empty())
        return;

    // Sort periods by increasing rawStartIndex
    // std::sort(periods.begin(), periods.end(), 
    //     [](const PivotPeriod& a, const PivotPeriod& b) {
    //         return a.rawStartIndex < b.rawStartIndex;
    //     });

    // Cursor to traverse the mapping a single time
    size_t mappingIdx = 0;
    
    for (indicators::PivotPointsInstance::PivotPeriod& period : periods) {
        size_t aggStartIndex;
        size_t aggEndIndex;

        const std::pair<size_t, size_t>& rawIndices = period.indices[static_cast<size_t>(chart::AggregationLevel::Raw)];

        // Find the start index
        while (mappingIdx < mapping.size()) {
            if (mapping[mappingIdx].empty()) {
                mappingIdx++;
                continue;
            }

            size_t lastIdx = mapping[mappingIdx].back();
            if (rawIndices.first <= lastIdx) {
                aggStartIndex = mappingIdx;
                break;
            }

            mappingIdx++;
        }
        
        // If start index was not found, skip to the next period
        if (aggStartIndex < 0) continue;
                
        // Find the end index starting from the last found point
        while (mappingIdx < mapping.size()) {
            if (mapping[mappingIdx].empty()) {
                mappingIdx++;
                continue;
            }

            size_t lastIdx = mapping[mappingIdx].back();
            if (rawIndices.second <= lastIdx) {
                aggEndIndex = mappingIdx;
                break;
            }

            mappingIdx++;
        }
        
        // If both indices are valid, store them
        if (aggStartIndex >= 0 && aggEndIndex >= 0) {
            period.indices[static_cast<size_t>(level)] = {aggStartIndex, aggEndIndex};
        }
    }
}

void ChartDataManager::precalculateTradeIndices(chart::AggregationLevel level) {
    // Check that the level exists in the cache
    const chart::AggregatedOHLCV& aggData = m_aggregatedOHLCVCache[static_cast<size_t>(level)];
    if (!aggData.isValid)
        return;

    const std::vector<std::vector<size_t>>& mapping = aggData.rawIndicesMapping;
    if (mapping.empty() || m_trades.empty())
        return;
    
    // Sort trades by increasing entry bar to optimize the search
    std::vector<size_t> sortedTradeIndices;
    sortedTradeIndices.reserve(m_trades.size());
    
    for (size_t i = 0; i < m_trades.size(); ++i) {
        sortedTradeIndices.push_back(i);
    }
    
    std::sort(sortedTradeIndices.begin(), sortedTradeIndices.end(),
        [this](size_t a, size_t b) {
            return m_trades[a].entryBar < m_trades[b].entryBar;
        });
    
    // Cursor to traverse the mapping once
    size_t mappingIdx = 0;
    
    for (size_t idx : sortedTradeIndices) {
        const auto& trade = m_trades[idx];

        int rawEntryBar = static_cast<int>(trade.entryBar);
        int rawExitBar = trade.hasBeenClosed() ? static_cast<int>(trade.exitBar) : rawEntryBar;

        int aggEntryIndex = -1;
        int aggExitIndex = -1;
        
        // Find the entry index
        while (mappingIdx < mapping.size()) {
            bool found = false;
            for (size_t rawIdx : mapping[mappingIdx]) {
                if (static_cast<int>(rawIdx) == rawEntryBar) {
                    aggEntryIndex = static_cast<int>(mappingIdx);
                    found = true;
                    break;
                }
            }
            
            if (found) break;
            mappingIdx++;
        }
        
        // If entry index was not found, skip to the next trade
        if (aggEntryIndex < 0) continue;
        
        // Find the exit index starting from the entry point
        size_t exitMappingIdx = mappingIdx;
        while (exitMappingIdx < mapping.size()) {
            bool found = false;
            for (size_t rawIdx : mapping[exitMappingIdx]) {
                if (static_cast<int>(rawIdx) == rawExitBar) {
                    aggExitIndex = static_cast<int>(exitMappingIdx);
                    found = true;
                    break;
                }
            } 

            // Here we do not increment the exit cursor in case the next trade has its exit earlier
            
            if (found) break;
            exitMappingIdx++;
        }
        
        // If exit index not found, use the entry index
        if (aggExitIndex < 0) {
            aggExitIndex = aggEntryIndex;
        }
        
        // Store indices for this trade and aggregation level
        m_tradeIndices[idx][static_cast<size_t>(level)] = {aggEntryIndex, aggExitIndex};
    }
}

// Utility method to configure the aggregation selector
bool ChartDataManager::configureAggregationSelector(ArrayMath& math, chart::AggregationLevel level) const {
    switch (level) {
    case chart::AggregationLevel::OneMinute:
        math.selectStartOfMinute();
        break;
    case chart::AggregationLevel::OneHour:
        math.selectStartOfHour();
        break;
    case chart::AggregationLevel::OneDay:
        math.selectStartOfDay();
        break;
    default:
        // Do nothing for Raw
        return false;
        break;
    }
    return true;
}

chart::AggregationInfo ChartDataManager::getOptimalAggregationInfo(const DoubleArray& timestamps) {
    chart::AggregationInfo result;
    result.level = chart::AggregationLevel::Raw;
    result.startIndex = findClosestIndex(m_aggregatedOHLCVCache[static_cast<size_t>(chart::AggregationLevel::Raw)].timestamps, timestamps[0]);
    result.pointCount = timestamps.len;
    result.isValid = true;

    if (timestamps.len <= m_maxDisplayPoints) return result;

    // Determine the starting aggregation level based on the data period
    chart::AggregationLevel startLevel = determineStartingAggregationLevel(timestamps);

    // Find the index of the starting level in our list
    size_t startIdx = 0;
    for (size_t i = 0; i < static_cast<size_t>(chart::AggregationLevel::Count); i++) {
        if (i == static_cast<size_t>(startLevel)) {
            startIdx = i;
            break;
        }
    }

    // Iterate through aggregation levels starting from the start level
    for (size_t i = startIdx + 1; i < static_cast<size_t>(chart::AggregationLevel::Count); i++) {
        chart::AggregationLevel level = static_cast<chart::AggregationLevel>(i);

        if (!m_aggregatedOHLCVCache[i].isValid) {
            aggregateOHLCV(level);
        }

        aggregateIndicators(level);

        const chart::AggregatedOHLCV& aggregatedData = m_aggregatedOHLCVCache[i];
        if (!aggregatedData.isValid) continue;

        // Extract start and end timestamps of the display interval
        double startTime = timestamps[0];
        double endTime = timestamps[timestamps.len - 1];
            
        // Find corresponding indices in the aggregated data
        size_t aggStartIdx = findClosestIndex(aggregatedData.timestamps, startTime);
        size_t aggEndIdx = findClosestIndex(aggregatedData.timestamps, endTime, true);

        // Compute how many aggregated points would be visible in this range
        int visibleAggPoints = (aggEndIdx >= aggStartIdx) ? static_cast<int>(aggEndIdx - aggStartIdx + 1) : 0;

        // If aggregation is valid and reduces the data enough
        if (visibleAggPoints > 0 && visibleAggPoints <= m_maxDisplayPoints) {
            result.level = level;
            result.startIndex = aggStartIdx;
            result.pointCount = visibleAggPoints;
            return result;
        }
    }

    // If no level fits, use the highest available
    size_t highestLevel = static_cast<size_t>(static_cast<int>(chart::AggregationLevel::Count) - 1);
    if (m_aggregatedOHLCVCache[highestLevel].isValid) {
        result.level = static_cast<chart::AggregationLevel>(highestLevel);
        result.startIndex = 0;
        result.pointCount = m_aggregatedOHLCVCache[highestLevel].timestamps.size();
    }

    return result;
}

void ChartDataManager::setMaxDisplayPoints(int value) {
    if (value < 100) value = 100; // Minimum value to avoid issues
    if (value > 100000) value = 100000; // Limit to the specified maximum
    
    if (m_maxDisplayPoints == value) 
        return; // No change
        
    m_maxDisplayPoints = value;

}

std::optional<size_t> ChartDataManager::rawToAggregatedIndex(chart::AggregationLevel level, size_t rawIndex) const {
    // If no aggregation, the index is identical
    if (level == chart::AggregationLevel::Raw)
        return rawIndex;
        
    // Check that the level exists in the cache
    const chart::AggregatedOHLCV& aggData = m_aggregatedOHLCVCache[static_cast<size_t>(level)];
    if (aggData.rawIndicesMapping.empty() || !aggData.isValid)
        return std::nullopt;

    const auto& mapping = aggData.rawIndicesMapping;
    for (size_t i = 0; i < mapping.size(); ++i) {
        if (mapping[i].empty())
            continue;
        // Look at the last index of the group
        size_t lastIdx = mapping[i].back();
        if (rawIndex <= lastIdx)
            return i;
    }
    return std::nullopt;
}

const std::vector<size_t>& ChartDataManager::getAggregatedToRawIndices(chart::AggregationLevel level, size_t aggregatedIndex) const {
    static const std::vector<size_t> empty;
    
    // Special case for Raw: return singleton with the index itself
    if (level == chart::AggregationLevel::Raw) {
        static std::vector<size_t> singleIndex;
        singleIndex = {aggregatedIndex};
        return singleIndex;
    }
    
    // Check that the level and index are valid
    const chart::AggregatedOHLCV& aggData = m_aggregatedOHLCVCache[static_cast<size_t>(level)];
    if (!aggData.isValid || aggregatedIndex >= static_cast<size_t>(aggData.rawIndicesMapping.size()))
        return empty;
        
    return aggData.rawIndicesMapping[aggregatedIndex];
}

int ChartDataManager::aggregatedToFirstRawIndex(chart::AggregationLevel level, int aggregatedIndex) const {
    // If no aggregation, the index is identical
    if (level == chart::AggregationLevel::Raw)
        return aggregatedIndex;
        
    // Get all corresponding raw indices
    const auto& rawIndices = getAggregatedToRawIndices(level, aggregatedIndex);
    
    // Return the first if it exists
    return rawIndices.empty() ? -1 : static_cast<int>(rawIndices.front());
}

void ChartDataManager::calculateIndicator(const indicators::IndicatorBase &config)
{
    if (const indicators::RSIInstance* rsiConfig = dynamic_cast<const indicators::RSIInstance*>(&config)) {
        calculateRSI(rsiConfig->id, rsiConfig->period);
        for (auto& aggregated : m_aggregatedIndicatorsCache)
            aggregated.validRsiIds.erase(rsiConfig->id);
        return;
    }
    if (const indicators::EMAInstance* emaConfig = dynamic_cast<const indicators::EMAInstance*>(&config)) {
        calculateEMA(emaConfig->id, emaConfig->period);
        for (auto& aggregated : m_aggregatedIndicatorsCache)
            aggregated.validEmaIds.erase(emaConfig->id);
        return;
    }
    if (const indicators::SuperTrendInstance* supertrendConfig = dynamic_cast<const indicators::SuperTrendInstance*>(&config)) {
        calculateSupertrend(supertrendConfig->id, supertrendConfig->period, supertrendConfig->multiplier);
        for (auto& aggregated : m_aggregatedIndicatorsCache)
            aggregated.validSupertrendIds.erase(supertrendConfig->id);
        return;
    }
    if (const indicators::StochasticInstance* stochasticConfig = dynamic_cast<const indicators::StochasticInstance*>(&config)) {
        calculateStochastic(stochasticConfig->id, stochasticConfig->fastKPeriod, stochasticConfig->slowKPeriod, stochasticConfig->slowDPeriod);
        for (auto& aggregated : m_aggregatedIndicatorsCache)
            aggregated.validStochasticIds.erase(stochasticConfig->id);
        return;
    }
    if (const indicators::ATRInstance* atrConfig = dynamic_cast<const indicators::ATRInstance*>(&config)) {
        calculateATR(atrConfig->id, atrConfig->period, atrConfig->useLogScale);
        for (auto& aggregated : m_aggregatedIndicatorsCache)
            aggregated.validAtrIds.erase(atrConfig->id);
        return;
    }
    if (const indicators::CCIInstance* cciConfig = dynamic_cast<const indicators::CCIInstance*>(&config)) {
        calculateCCI(cciConfig->id, cciConfig->period);
        for (auto& aggregated : m_aggregatedIndicatorsCache)
            aggregated.validCciIds.erase(cciConfig->id);
        return;
    }
    if (const indicators::MACDInstance* macdConfig = dynamic_cast<const indicators::MACDInstance*>(&config)) {
        calculateMACD(macdConfig->id, macdConfig->fastPeriod, macdConfig->slowPeriod, macdConfig->signalPeriod,
                      macdConfig->source, macdConfig->osc_ma_type,
                      macdConfig->signal_ma_type, macdConfig->signal_smoothing);
        for (auto& aggregated : m_aggregatedIndicatorsCache)
            aggregated.validMacdIds.erase(macdConfig->id);
        return;
    }
    if (const indicators::BBInstance* bbConfig = dynamic_cast<const indicators::BBInstance*>(&config)) {
        calculateBB(bbConfig->id, bbConfig->period, bbConfig->stddev_multiplier,
                    bbConfig->source, bbConfig->ma_type);
        for (auto& aggregated : m_aggregatedIndicatorsCache)
            aggregated.validBbIds.erase(bbConfig->id);
        return;
    }
    if (const indicators::PivotPointsInstance* pivotConfig = dynamic_cast<const indicators::PivotPointsInstance*>(&config)) {
        calculatePivotPoints(*pivotConfig);
        for (auto& aggregated : m_aggregatedIndicatorsCache)
            aggregated.validPivotPointsIds.erase(pivotConfig->id);
        return;
    }
}

chart::AggregationLevel ChartDataManager::determineStartingAggregationLevel(const DoubleArray& timestamps) const {
    // If fewer than two timestamps, impossible to determine the period
    if (timestamps.len < 2) {
        return chart::AggregationLevel::Raw;
    }

    // The period is constant: take the difference between the first two points
    double period = timestamps[1] - timestamps[0];

    // Determine the starting level based on the period
    if (period >= 86400) {   // More than a day
        return chart::AggregationLevel::OneDay;
    } else if (period >= 3600) {    // More than an hour
        return chart::AggregationLevel::OneHour;
    } else if (period >= 60) {      // More than a minute
        return chart::AggregationLevel::OneMinute;
    } else {
        return chart::AggregationLevel::Raw;  // Less than a minute
    }
}

size_t ChartDataManager::findClosestIndex(const std::vector<double>& values, double target, bool searchForward) const {
    // If searchForward is true, find the first element >= target
    // Otherwise, find the last element <= target

    if (values.empty()) return 0;

    if (searchForward) {
        // Search forward (first element >= target)
        for (size_t i = 0; i < values.size(); i++)
            if (values[i] >= target)
                return i;
        return values.size() - 1;
    } else {
        // Search backward (last element <= target)
        for (int i = static_cast<int>(values.size()) - 1; i >= 0; i--)
            if (values[i] <= target)
                return i;
        return 0;
    }
}

std::optional<std::pair<size_t, size_t>> ChartDataManager::getTradeAggregatedIndices(size_t tradeIndex, chart::AggregationLevel level) const {
    if (tradeIndex >= m_tradeIndices.size())
        return std::nullopt;

    return m_tradeIndices[tradeIndex][static_cast<size_t>(level)];
}

DoubleArray ChartDataManager::vectorToDoubleArray(const std::vector<double>& vec) {
    if (vec.empty())
        return DoubleArray(nullptr, 0);
    return DoubleArray(vec.data(), static_cast<int>(vec.size()));
}

void ChartDataManager::removeAllIndicators() {
    m_indicators.clear();
}

void ChartDataManager::addMarker(const chart::ChartMarker& marker) {
    m_markers.push_back(marker);
}

void ChartDataManager::removeMarker(size_t index) {
    if (index < m_markers.size()) {
        m_markers.erase(m_markers.begin() + index);
    }
}

void ChartDataManager::clearAllMarkers() {
    m_markers.clear();
}

bool ChartDataManager::toggleIndicatorVisibility(int id) {
    auto it = std::find_if(m_indicators.begin(), m_indicators.end(), [id](const std::unique_ptr<indicators::IndicatorBase>& item) {  
        return item->id == id; 
    });

    if (it == m_indicators.end()) 
        return false;

    (*it)->visible = !(*it)->visible;
    return true;
}
