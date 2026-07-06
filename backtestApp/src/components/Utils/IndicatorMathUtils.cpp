#include "components/Utils/IndicatorMathUtils.h"
#include "common.h"
#include <algorithm>
#include <set>
#include <QDebug>
#include <chartdir.h>

// Helper function to detect day boundaries in timestamp data
std::vector<size_t> IndicatorMathUtils::detectDayBoundaries(const std::vector<double>& timestamps) {
    std::vector<size_t> boundaries;
    if (timestamps.empty()) {
        qDebug() << "detectDayBoundaries: Empty timestamps vector";
        return boundaries;
    }
    
    boundaries.push_back(0);  // First index is always a boundary
    
    int prevYMD = -1;
    
    for (size_t i = 0; i < timestamps.size(); ++i) {
        // Use ChartDirector's getChartYMD to decode timestamp
        // Returns YYYYMMDD as an integer (e.g., 20251123 for Nov 23, 2025)
        int currentYMD = Chart::getChartYMD(timestamps[i]);
        
        // Debug: log first few timestamps to see conversion
        if (i < 3 || i == timestamps.size() - 1) {
            qDebug() << "  [" << i << "] timestamp:" << timestamps[i] << "-> YMD:" << currentYMD;
        }
        
        // Check if we've moved to a new calendar day
        if (prevYMD != -1 && currentYMD != prevYMD) {
            boundaries.push_back(i);
            
            // Debug: log first few boundaries
            if (boundaries.size() <= 5) {
                qDebug() << "detectDayBoundaries: Boundary" << boundaries.size() - 1 
                         << "at index" << i << "YMD:" << currentYMD;
            }
        }
        
        prevYMD = currentYMD;
    }
    
    qDebug() << "detectDayBoundaries: Found" << boundaries.size() << "boundaries in" << timestamps.size() << "timestamps";
    
    return boundaries;
}

// Helper function to check if index is at a day boundary
static bool isAtDayBoundary(size_t index, const std::set<size_t>& boundaries) {
    return boundaries.find(index) != boundaries.end();
}

std::vector<double> IndicatorMathUtils::calculateRSI(
    const std::vector<double>& closeData, 
    int period,
    const std::vector<double>& timestamps,
    bool resetOnNewDay)
{
    size_t dataSize = closeData.size();
    std::vector<double> rsiValues(dataSize, 0.0);

    if (dataSize <= static_cast<size_t>(period)) {
        std::fill(rsiValues.begin(), rsiValues.end(), 50.0);  // Default neutral value
        return rsiValues;
    }

    // Detect day boundaries if reset is enabled
    std::set<size_t> dayBoundaries;
    if (resetOnNewDay && !timestamps.empty()) {
        auto boundaries = detectDayBoundaries(timestamps);
        dayBoundaries.insert(boundaries.begin(), boundaries.end());
    }

    // Calculate price changes (delta)
    std::vector<double> deltas(dataSize - 1);
    for (size_t i = 1; i < dataSize; ++i) {
        deltas[i - 1] = closeData[i] - closeData[i - 1];
    }

    // Separate positive and negative changes
    std::vector<double> gains(dataSize - 1);
    std::vector<double> losses(dataSize - 1);
    for (size_t i = 0; i < deltas.size(); ++i) {
        gains[i] = (deltas[i] > 0) ? deltas[i] : 0;
        losses[i] = (deltas[i] < 0) ? -deltas[i] : 0;
    }

    double avgGain = 0;
    double avgLoss = 0;
    int warmupCount = 0;  // Track how many periods we've accumulated since last reset

    for (size_t i = 0; i < dataSize; ++i) {
        // Check for day boundary reset
        if (resetOnNewDay && isAtDayBoundary(i, dayBoundaries)) {
            avgGain = 0;
            avgLoss = 0;
            warmupCount = 0;
        }

        if (i == 0) {
            rsiValues[i] = 0.0;  // No value for first candle
            continue;
        }

        warmupCount++;

        if (warmupCount <= period) {
            // Still in warmup period - accumulate values
            avgGain += gains[i - 1];
            avgLoss += losses[i - 1];
            
            if (warmupCount == period) {
                // End of warmup - calculate first average
                avgGain /= period;
                avgLoss /= period;
                double rs = (avgLoss > 0) ? (avgGain / avgLoss) : 100.0;
                rsiValues[i] = 100.0 - (100.0 / (1.0 + rs));
            } else {
                rsiValues[i] = 0.0;  // No value during warmup
            }
        } else {
            // Normal Wilder's smoothing
            avgGain = ((period - 1) * avgGain + gains[i - 1]) / period;
            avgLoss = ((period - 1) * avgLoss + losses[i - 1]) / period;
            
            if (avgLoss > 0) {
                double rs = avgGain / avgLoss;
                rsiValues[i] = 100.0 - (100.0 / (1.0 + rs));
            } else {
                rsiValues[i] = 100.0;
            }
        }
    }

    return rsiValues;
}

std::tuple<std::vector<double>, std::vector<double>, std::vector<double>, std::vector<double>> IndicatorMathUtils::calculateHeikinAshi(
    const std::vector<double>& open,
    const std::vector<double>& high,
    const std::vector<double>& low,
    const std::vector<double>& close)
{
    size_t size = open.size();
    if (size == 0) return {};

    std::vector<double> ha_open(size);
    std::vector<double> ha_high(size);
    std::vector<double> ha_low(size);
    std::vector<double> ha_close(size);

    // First candle
    ha_open[0] = open[0];
    ha_close[0] = (open[0] + high[0] + low[0] + close[0]) / 4.0;
    ha_high[0] = high[0];
    ha_low[0] = low[0];
    
    // Calculate other candles
    for (size_t i = 1; i < size; ++i) {
        ha_close[i] = (open[i] + high[i] + low[i] + close[i]) / 4.0;
        ha_open[i] = (ha_open[i-1] + ha_close[i-1]) / 2.0;
        ha_high[i] = std::max(std::max(high[i], ha_open[i]), ha_close[i]);
        ha_low[i] = std::min(std::min(low[i], ha_open[i]), ha_close[i]);
    }

    return {ha_open, ha_high, ha_low, ha_close};
}

std::vector<double> IndicatorMathUtils::calculateEMA(
    const std::vector<double>& closeData, 
    int period,
    const std::vector<double>& timestamps,
    bool resetOnNewDay)
{
    size_t dataSize = closeData.size();
    std::vector<double> emaValues(dataSize, 0.0);

    if (dataSize <= static_cast<size_t>(period)) {
        std::copy(closeData.begin(), closeData.end(), emaValues.begin());
        return emaValues;
    }
    
    // Detect day boundaries if reset is enabled
    std::set<size_t> dayBoundaries;
    if (resetOnNewDay && !timestamps.empty()) {
        auto boundaries = detectDayBoundaries(timestamps);
        dayBoundaries.insert(boundaries.begin(), boundaries.end());
    }
    
    // Calculate the smoothing factor
    double multiplier = 2.0 / (period + 1.0);
    
    double sum = 0.0;
    int warmupCount = 0;
    double currentEma = 0.0;
    
    for (size_t i = 0; i < dataSize; ++i) {
        // Check for day boundary reset
        if (resetOnNewDay && isAtDayBoundary(i, dayBoundaries)) {
            sum = 0.0;
            warmupCount = 0;
            currentEma = 0.0;
        }
        
        warmupCount++;
        
        if (warmupCount < period) {
            sum += closeData[i];
            emaValues[i] = Chart::NoValue;  // No value during warmup
        } else if (warmupCount == period) {
            sum += closeData[i];
            currentEma = sum / period;
            emaValues[i] = currentEma;
        } else {
            currentEma = (closeData[i] - currentEma) * multiplier + currentEma;
            emaValues[i] = currentEma;
        }
    }
    return emaValues;
}

std::tuple<std::vector<double>, std::vector<int>> IndicatorMathUtils::calculateSupertrend(
    const std::vector<double>& highData,
    const std::vector<double>& lowData,
    const std::vector<double>& closeData,
    int period,
    double multiplier,
    const std::vector<double>& timestamps,
    bool resetOnNewDay)
{
    size_t dataSize = closeData.size();
    std::vector<double> supertrendValues(dataSize);
    std::vector<int> trendDirections(dataSize, 0);

    if (dataSize <= static_cast<size_t>(period)) {
        std::fill(supertrendValues.begin(), supertrendValues.end(), Chart::NoValue);
        return {supertrendValues, trendDirections};
    }
    
    // Calculate the ATR (with reset support)
    std::vector<double> atrValues = calculateATR(highData, lowData, closeData, period, false, timestamps, resetOnNewDay);
    
    // Detect day boundaries if reset is enabled
    std::set<size_t> dayBoundaries;
    if (resetOnNewDay && !timestamps.empty()) {
        auto boundaries = detectDayBoundaries(timestamps);
        dayBoundaries.insert(boundaries.begin(), boundaries.end());
    }
    
    // Calculate base bands (HL2 +/- multiplier * ATR)
    std::vector<double> basicUpperBand(dataSize);
    std::vector<double> basicLowerBand(dataSize);
    std::vector<double> finalUpperBand(dataSize);
    std::vector<double> finalLowerBand(dataSize);
    
    for (size_t i = 0; i < dataSize; ++i) {
        if (i < period) {
            basicUpperBand[i] = Chart::NoValue;
            basicLowerBand[i] = Chart::NoValue;
            finalUpperBand[i] = Chart::NoValue;
            finalLowerBand[i] = Chart::NoValue;
            supertrendValues[i] = Chart::NoValue;
            trendDirections[i] = 0;
            continue;
        }
        
        double hl2 = (highData[i] + lowData[i]) / 2.0;
        double atr = atrValues[i];
        
        // Calculate base bands
        basicUpperBand[i] = hl2 + (multiplier * atr);
        basicLowerBand[i] = hl2 - (multiplier * atr);
        
        // Calculate final bands (with hold logic)
        if (i == period) {
            // First value
            finalUpperBand[i] = basicUpperBand[i];
            finalLowerBand[i] = basicLowerBand[i];
        } else {
            // Final upper band: only decreases if previous close was above
            finalUpperBand[i] = (basicUpperBand[i] < finalUpperBand[i-1] || closeData[i-1] > finalUpperBand[i-1]) 
                               ? basicUpperBand[i] 
                               : finalUpperBand[i-1];
            
            // Final lower band: only increases if previous close was below
            finalLowerBand[i] = (basicLowerBand[i] > finalLowerBand[i-1] || closeData[i-1] < finalLowerBand[i-1]) 
                               ? basicLowerBand[i] 
                               : finalLowerBand[i-1];
        }
    }
    
    // Calculate final Supertrend and direction
    for (size_t i = period; i < dataSize; ++i) {
        if (i == period) {
            // First value - determine initial trend
            if (closeData[i] <= finalUpperBand[i]) {
                supertrendValues[i] = finalUpperBand[i];
                trendDirections[i] = -1; // Bearish trend
            } else {
                supertrendValues[i] = finalLowerBand[i];
                trendDirections[i] = 1;  // Bullish trend
            }
        } else {
            // Trend change logic
            int prevTrend = trendDirections[i-1];
            double prevSupertrend = supertrendValues[i-1];
            
            if (prevTrend == 1) { // Previous uptrend
                if (closeData[i] < finalLowerBand[i]) {
                    // Change to downtrend
                    supertrendValues[i] = finalUpperBand[i];
                    trendDirections[i] = -1;
                } else {
                    // Maintain uptrend
                    supertrendValues[i] = finalLowerBand[i];
                    trendDirections[i] = 1;
                }
            } else { // Previous downtrend
                if (closeData[i] > finalUpperBand[i]) {
                    // Change to uptrend
                    supertrendValues[i] = finalLowerBand[i];
                    trendDirections[i] = 1;
                } else {
                    // Maintain downtrend
                    supertrendValues[i] = finalUpperBand[i];
                    trendDirections[i] = -1;
                }
            }
        }
    }

    return {supertrendValues, trendDirections};
}

std::tuple<std::vector<double>, std::vector<double>, std::vector<int>> IndicatorMathUtils::calculateSwingStructure(
    const std::vector<double>& highData,
    const std::vector<double>& lowData,
    const std::vector<double>& closeData,
    double highMove,
    double lowMove,
    int minPeriods,
    int maxPeriods,
    const std::vector<double>& timestamps,
    bool resetOnNewDay)
{
    size_t dataSize = closeData.size();
    std::vector<double> swingHighValues(dataSize, Chart::NoValue);
    std::vector<double> swingLowValues(dataSize, Chart::NoValue);
    std::vector<int> trendValues(dataSize, 0);

    if (minPeriods < 1 || maxPeriods < minPeriods)
        return {swingHighValues, swingLowValues, trendValues};

    size_t windowSize = static_cast<size_t>(2 * maxPeriods + 1);
    if (dataSize < windowSize)
        return {swingHighValues, swingLowValues, trendValues};

    // Split into per-day segments if reset is enabled, otherwise treat the whole series as one segment
    std::set<size_t> segmentBoundaries;
    segmentBoundaries.insert(0);
    if (resetOnNewDay && !timestamps.empty()) {
        auto boundaries = detectDayBoundaries(timestamps);
        segmentBoundaries.insert(boundaries.begin(), boundaries.end());
    }
    segmentBoundaries.insert(dataSize);

    std::vector<size_t> segments(segmentBoundaries.begin(), segmentBoundaries.end());

    for (size_t s = 0; s + 1 < segments.size(); ++s) {
        size_t segStart = segments[s];
        size_t segEnd = segments[s + 1]; // exclusive
        if (segEnd - segStart < windowSize) continue;

        std::vector<double> shList, slList; // keep at most the last 2 confirmed swings
        double lastHigh = Chart::NoValue;
        double lastLow = Chart::NoValue;
        int trend = 0;

        for (size_t i = segStart; i < segEnd; ++i) {
            if (i >= segStart + static_cast<size_t>(2 * maxPeriods)) {
                // Candidate is the middle of a 2*maxPeriods+1 window: maxPeriods bars are
                // available both before and after it, giving the widest validation window.
                size_t candidateIdx = i - static_cast<size_t>(maxPeriods);

                // Swing high check
                double peak = highData[candidateIdx];
                bool rose = false;
                for (int k = minPeriods; k <= maxPeriods; ++k) {
                    if (peak - closeData[candidateIdx - k] >= highMove) { rose = true; break; }
                }
                if (rose) {
                    bool fell = false;
                    for (int j = minPeriods; j <= maxPeriods; ++j) {
                        if (peak - closeData[candidateIdx + j] >= highMove) { fell = true; break; }
                    }
                    if (fell) {
                        bool isLocalMax = true;
                        for (size_t k2 = candidateIdx - maxPeriods; k2 <= candidateIdx + static_cast<size_t>(maxPeriods); ++k2) {
                            if (k2 != candidateIdx && highData[k2] > peak) { isLocalMax = false; break; }
                        }
                        if (isLocalMax) {
                            shList.push_back(peak);
                            if (shList.size() > 2) shList.erase(shList.begin());
                            lastHigh = peak;
                        }
                    }
                }

                // Swing low check
                double trough = lowData[candidateIdx];
                bool fellBefore = false;
                for (int k = minPeriods; k <= maxPeriods; ++k) {
                    if (closeData[candidateIdx - k] - trough >= lowMove) { fellBefore = true; break; }
                }
                if (fellBefore) {
                    bool roseAfter = false;
                    for (int j = minPeriods; j <= maxPeriods; ++j) {
                        if (closeData[candidateIdx + j] - trough >= lowMove) { roseAfter = true; break; }
                    }
                    if (roseAfter) {
                        bool isLocalMin = true;
                        for (size_t k2 = candidateIdx - maxPeriods; k2 <= candidateIdx + static_cast<size_t>(maxPeriods); ++k2) {
                            if (k2 != candidateIdx && lowData[k2] < trough) { isLocalMin = false; break; }
                        }
                        if (isLocalMin) {
                            slList.push_back(trough);
                            if (slList.size() > 2) slList.erase(slList.begin());
                            lastLow = trough;
                        }
                    }
                }

                if (shList.size() >= 2 && slList.size() >= 2) {
                    double hh = shList[1], ph = shList[0];
                    double hl = slList[1], pl = slList[0];
                    if (hh > ph && hl > pl) trend = 1;
                    else if (hh < ph && hl < pl) trend = -1;
                    else trend = 0;
                }
            }

            swingHighValues[i] = lastHigh;
            swingLowValues[i] = lastLow;
            trendValues[i] = trend;
        }
    }

    return {swingHighValues, swingLowValues, trendValues};
}

std::tuple<std::vector<double>, std::vector<double>> IndicatorMathUtils::calculateStochastic(
    const std::vector<double>& highData,
    const std::vector<double>& lowData,
    const std::vector<double>& closeData,
    int fastKPeriod,
    int slowKPeriod,
    int slowDPeriod,
    const std::vector<double>& timestamps,
    bool resetOnNewDay)
{
    // Input data validation
    size_t dataSize = closeData.size();
    if (dataSize == 0 || highData.size() != dataSize || lowData.size() != dataSize)
        return {std::vector<double>(), std::vector<double>()};
    
    // Detect day boundaries if reset is enabled
    std::set<size_t> dayBoundaries;
    if (resetOnNewDay && !timestamps.empty()) {
        auto boundaries = detectDayBoundaries(timestamps);
        dayBoundaries.insert(boundaries.begin(), boundaries.end());
    }
    
    // Resize output vectors
    std::vector<double> kValues(dataSize);
    std::vector<double> dValues(dataSize);

    // Default values (0.0 = no value)
    std::fill(kValues.begin(), kValues.end(), 0.0);
    std::fill(dValues.begin(), dValues.end(), 0.0);
    
    if (dataSize < static_cast<size_t>(fastKPeriod)) 
        return {kValues, dValues};  // Not enough data to compute Stochastic
    
    // For reset on new day, we need to track segment boundaries
    std::vector<size_t> segmentStarts;
    if (resetOnNewDay) {
        segmentStarts.assign(dayBoundaries.begin(), dayBoundaries.end());
        std::sort(segmentStarts.begin(), segmentStarts.end());
    }
    
    // Step 1: Calculate raw %K (Fast %K)
    std::vector<double> rawK(dataSize, 0.0);
    
    for (size_t i = 0; i < dataSize; ++i) {
        // Find segment start for this index
        size_t segmentStart = 0;
        if (resetOnNewDay) {
            for (auto it = segmentStarts.rbegin(); it != segmentStarts.rend(); ++it) {
                if (*it <= i) {
                    segmentStart = *it;
                    break;
                }
            }
        }
        
        size_t localIndex = i - segmentStart;
        if (localIndex < static_cast<size_t>(fastKPeriod - 1)) {
            rawK[i] = 50.0;
            continue;
        }
        
        // Find lowest low and highest high over the fastKPeriod (within segment)
        double lowestLow = std::numeric_limits<double>::max();
        double highestHigh = std::numeric_limits<double>::lowest();
        
        size_t lookbackStart = std::max(segmentStart, i - fastKPeriod + 1);
        for (size_t j = lookbackStart; j <= i; ++j) {
            lowestLow = std::min(lowestLow, lowData[j]);
            highestHigh = std::max(highestHigh, highData[j]);
        }
        
        double range = highestHigh - lowestLow;
        if (range > 0.0) {
            rawK[i] = ((closeData[i] - lowestLow) / range) * 100.0;
        } else {
            rawK[i] = 50.0;
        }
    }
    
    // Step 2: Smooth raw %K to get slow %K
    for (size_t i = 0; i < dataSize; ++i) {
        size_t segmentStart = 0;
        if (resetOnNewDay) {
            for (auto it = segmentStarts.rbegin(); it != segmentStarts.rend(); ++it) {
                if (*it <= i) {
                    segmentStart = *it;
                    break;
                }
            }
        }
        
        size_t localIndex = i - segmentStart;
        if (localIndex < static_cast<size_t>(fastKPeriod + slowKPeriod - 2)) {
            kValues[i] = 50.0;
            continue;
        }
        
        double sum = 0.0;
        size_t count = 0;
        for (size_t j = 0; j < static_cast<size_t>(slowKPeriod) && i >= j; ++j) {
            if (i - j >= segmentStart) {
                sum += rawK[i - j];
                count++;
            }
        }
        kValues[i] = (count > 0) ? sum / count : 50.0;
    }
    
    // Step 3: Calculate %D as moving average of %K
    for (size_t i = 0; i < dataSize; ++i) {
        size_t segmentStart = 0;
        if (resetOnNewDay) {
            for (auto it = segmentStarts.rbegin(); it != segmentStarts.rend(); ++it) {
                if (*it <= i) {
                    segmentStart = *it;
                    break;
                }
            }
        }
        
        size_t localIndex = i - segmentStart;
        if (localIndex < static_cast<size_t>(fastKPeriod + slowKPeriod + slowDPeriod - 3)) {
            dValues[i] = 50.0;
            continue;
        }
        
        double sum = 0.0;
        size_t count = 0;
        for (size_t j = 0; j < static_cast<size_t>(slowDPeriod) && i >= j; ++j) {
            if (i - j >= segmentStart) {
                sum += kValues[i - j];
                count++;
            }
        }
        dValues[i] = (count > 0) ? sum / count : 50.0;
    }

    return {kValues, dValues};
}

std::vector<double> IndicatorMathUtils::calculateATR(
    const std::vector<double>& highData,
    const std::vector<double>& lowData,
    const std::vector<double>& closeData,
    int period,
    bool useLogScale,
    const std::vector<double>& timestamps,
    bool resetOnNewDay)
{
    size_t dataSize = closeData.size();
    std::vector<double> atrValues(dataSize, 0.0);

    if (dataSize < static_cast<size_t>(period)) {
        std::fill(atrValues.begin(), atrValues.end(), 0.0);
        return atrValues;
    }
    
    // Detect day boundaries if reset is enabled
    std::set<size_t> dayBoundaries;
    if (resetOnNewDay && !timestamps.empty()) {
        auto boundaries = detectDayBoundaries(timestamps);
        dayBoundaries.insert(boundaries.begin(), boundaries.end());
    }
    
    // Calculate True Range values
    std::vector<double> tr(dataSize);
    tr[0] = highData[0] - lowData[0];  // First TR is just high-low
    for (size_t i = 1; i < dataSize; ++i) {
        double highLow = highData[i] - lowData[i];
        double highClose = std::abs(highData[i] - closeData[i - 1]);
        double lowClose = std::abs(lowData[i] - closeData[i - 1]);
        tr[i] = std::max({highLow, highClose, lowClose});
    }
    
    double sum = 0.0;
    int warmupCount = 0;
    double currentATR = 0.0;
    
    for (size_t i = 0; i < dataSize; ++i) {
        // Check for day boundary reset
        if (resetOnNewDay && isAtDayBoundary(i, dayBoundaries)) {
            sum = 0.0;
            warmupCount = 0;
            currentATR = 0.0;
        }
        
        warmupCount++;
        
        if (warmupCount <= period) {
            sum += tr[i];
            if (warmupCount == period) {
                currentATR = sum / period;
                atrValues[i] = useLogScale ? std::log(currentATR + 1) : currentATR;
            } else {
                atrValues[i] = 0.0;
            }
        } else {
            // Wilder's smoothing
            if (useLogScale) {
                double prevATR = std::exp(atrValues[i - 1]) - 1;
                currentATR = (prevATR * (period - 1) + tr[i]) / period;
                atrValues[i] = std::log(currentATR + 1);
            } else {
                currentATR = (currentATR * (period - 1) + tr[i]) / period;
                atrValues[i] = currentATR;
            }
        }
    }

    return atrValues;
}

std::vector<double> IndicatorMathUtils::calculateCCI(
    const std::vector<double>& highData,
    const std::vector<double>& lowData,
    const std::vector<double>& closeData,
    int period,
    const std::vector<double>& timestamps,
    bool resetOnNewDay)
{
    size_t dataSize = closeData.size();
    std::vector<double> cciValues(dataSize, 0.0);

    if (dataSize < static_cast<size_t>(period)) {
        return cciValues;
    }

    // Detect day boundaries if reset is enabled
    std::set<size_t> dayBoundaries;
    std::vector<size_t> segmentStarts;
    if (resetOnNewDay && !timestamps.empty()) {
        auto boundaries = detectDayBoundaries(timestamps);
        dayBoundaries.insert(boundaries.begin(), boundaries.end());
        segmentStarts.assign(boundaries.begin(), boundaries.end());
        std::sort(segmentStarts.begin(), segmentStarts.end());
    }

    // Calculate Typical Price (TP) = (High + Low + Close) / 3
    std::vector<double> typicalPrice(dataSize);
    for (size_t i = 0; i < dataSize; ++i) {
        typicalPrice[i] = (highData[i] + lowData[i] + closeData[i]) / 3.0;
    }

    // Calculate CCI for each point
    for (size_t i = 0; i < dataSize; ++i) {
        // Find segment start for this index
        size_t segmentStart = 0;
        if (resetOnNewDay) {
            for (auto it = segmentStarts.rbegin(); it != segmentStarts.rend(); ++it) {
                if (*it <= i) {
                    segmentStart = *it;
                    break;
                }
            }
        }
        
        size_t localIndex = i - segmentStart;
        if (localIndex < static_cast<size_t>(period - 1)) {
            cciValues[i] = 0.0;
            continue;
        }

        // Calculate Simple Moving Average of Typical Price (within segment)
        double smaTP = 0.0;
        int count = 0;
        for (int j = 0; j < period && i >= static_cast<size_t>(j); ++j) {
            if (i - j >= segmentStart) {
                smaTP += typicalPrice[i - j];
                count++;
            }
        }
        if (count > 0) smaTP /= count;

        // Calculate Mean Deviation
        double meanDeviation = 0.0;
        count = 0;
        for (int j = 0; j < period && i >= static_cast<size_t>(j); ++j) {
            if (i - j >= segmentStart) {
                meanDeviation += std::abs(typicalPrice[i - j] - smaTP);
                count++;
            }
        }
        if (count > 0) meanDeviation /= count;

        // Calculate CCI
        if (meanDeviation != 0.0) {
            cciValues[i] = (typicalPrice[i] - smaTP) / (0.015 * meanDeviation);
        } else {
            cciValues[i] = 0.0;
        }
    }

    return cciValues;
}

std::tuple<std::vector<double>, std::vector<double>, std::vector<double>> IndicatorMathUtils::calculateMACD(
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
    const std::vector<double>& timestamps,
    bool resetOnNewDay)
{
    // Select the source data based on the source parameter
    const std::vector<double>* sourceData = &close;

    if (source == filter::PriceType::OPEN) sourceData = &open;
    else if (source == filter::PriceType::HIGH) sourceData = &high;
    else if (source == filter::PriceType::LOW) sourceData = &low;
    else sourceData = &close; // default to close

    // For now, we only support EMA-based MACD (ignoring oscMAType and signalMAType)
    // TODO: Implement SMA support if needed
    (void)oscMAType;
    (void)signalMAType;
    (void)signalSmoothing;

    size_t n = sourceData->size();
    std::vector<double> macdLine(n, 0.0);
    std::vector<double> signalLine(n, 0.0);
    std::vector<double> histogram(n, 0.0);

    if (n == 0 || fastPeriod <= 0 || slowPeriod <= 0 || signalPeriod <= 0)
        return {macdLine, signalLine, histogram};

    // Use EMA with reset support
    std::vector<double> fastEma = calculateEMA(*sourceData, fastPeriod, timestamps, resetOnNewDay);
    std::vector<double> slowEma = calculateEMA(*sourceData, slowPeriod, timestamps, resetOnNewDay);

    // Detect day boundaries if reset is enabled
    std::set<size_t> dayBoundaries;
    std::vector<size_t> segmentStarts;
    if (resetOnNewDay && !timestamps.empty()) {
        auto boundaries = detectDayBoundaries(timestamps);
        dayBoundaries.insert(boundaries.begin(), boundaries.end());
        segmentStarts.assign(boundaries.begin(), boundaries.end());
        std::sort(segmentStarts.begin(), segmentStarts.end());
    }

    double multSignal = 2.0 / (signalPeriod + 1.0);

    // Build macd line where both EMAs are available
    for (size_t i = 0; i < n; ++i) {
        size_t segmentStart = 0;
        if (resetOnNewDay) {
            for (auto it = segmentStarts.rbegin(); it != segmentStarts.rend(); ++it) {
                if (*it <= i) {
                    segmentStart = *it;
                    break;
                }
            }
        }
        
        size_t localIndex = i - segmentStart;
        bool fastReady = (localIndex >= static_cast<size_t>(fastPeriod - 1));
        bool slowReady = (localIndex >= static_cast<size_t>(slowPeriod - 1));
        
        if (fastReady && slowReady) {
            macdLine[i] = fastEma[i] - slowEma[i];
        } else {
            macdLine[i] = 0.0;
        }
    }

    // Compute signal line as EMA over MACD
    double sig = 0.0;
    int signalWarmup = 0;
    double signalSum = 0.0;
    
    for (size_t i = 0; i < n; ++i) {
        // Check for day boundary reset
        if (resetOnNewDay && isAtDayBoundary(i, dayBoundaries)) {
            sig = 0.0;
            signalWarmup = 0;
            signalSum = 0.0;
        }
        
        size_t segmentStart = 0;
        if (resetOnNewDay) {
            for (auto it = segmentStarts.rbegin(); it != segmentStarts.rend(); ++it) {
                if (*it <= i) {
                    segmentStart = *it;
                    break;
                }
            }
        }
        
        size_t localIndex = i - segmentStart;
        bool macdReady = (localIndex >= static_cast<size_t>(slowPeriod - 1));
        
        if (!macdReady) {
            signalLine[i] = 0.0;
            histogram[i] = 0.0;
            continue;
        }
        
        signalWarmup++;
        
        if (signalWarmup <= signalPeriod) {
            signalSum += macdLine[i];
            if (signalWarmup == signalPeriod) {
                sig = signalSum / signalPeriod;
                signalLine[i] = sig;
                histogram[i] = macdLine[i] - sig;
            } else {
                signalLine[i] = 0.0;
                histogram[i] = 0.0;
            }
        } else {
            sig = (macdLine[i] - sig) * multSignal + sig;
            signalLine[i] = sig;
            histogram[i] = macdLine[i] - sig;
        }
    }

    return {macdLine, signalLine, histogram};
}

std::tuple<std::vector<double>, std::vector<double>, std::vector<double>> IndicatorMathUtils::calculateBollingerBands(
    const std::vector<double>& open,
    const std::vector<double>& high,
    const std::vector<double>& low,
    const std::vector<double>& close,
    int period,
    double stdDevMultiplier,
    filter::PriceType source,
    filter::MAType oscMAType,
    const std::vector<double>& timestamps,
    bool resetOnNewDay
) {
    // Select source series
    const std::vector<double>* src = &close;
    if (source == filter::PriceType::OPEN)  src = &open;
    else if (source == filter::PriceType::HIGH) src = &high;
    else if (source == filter::PriceType::LOW)  src = &low;
    // else keep close

    size_t n = src->size();
    std::vector<double> middle(n, 0.0);
    std::vector<double> upper(n, 0.0);
    std::vector<double> lower(n, 0.0);

    if (n == 0 || period <= 0) return {middle, upper, lower};
    if (n < static_cast<size_t>(period)) return {middle, upper, lower};

    // Detect day boundaries if reset is enabled
    std::set<size_t> dayBoundaries;
    std::vector<size_t> segmentStarts;
    if (resetOnNewDay && !timestamps.empty()) {
        auto boundaries = detectDayBoundaries(timestamps);
        dayBoundaries.insert(boundaries.begin(), boundaries.end());
        segmentStarts.assign(boundaries.begin(), boundaries.end());
        std::sort(segmentStarts.begin(), segmentStarts.end());
    }

    // Compute middle band (SMA or EMA with reset support)
    if (oscMAType == filter::MAType::EMA) {
        middle = calculateEMA(*src, period, timestamps, resetOnNewDay);
    } else {
        // Simple moving average (SMA) with reset support
        for (size_t i = 0; i < n; ++i) {
            size_t segmentStart = 0;
            if (resetOnNewDay) {
                for (auto it = segmentStarts.rbegin(); it != segmentStarts.rend(); ++it) {
                    if (*it <= i) {
                        segmentStart = *it;
                        break;
                    }
                }
            }
            
            size_t localIndex = i - segmentStart;
            if (localIndex < static_cast<size_t>(period - 1)) {
                middle[i] = Chart::NoValue;
                continue;
            }
            
            double sum = 0.0;
            size_t count = 0;
            for (int j = 0; j < period && i >= static_cast<size_t>(j); ++j) {
                if (i - j >= segmentStart) {
                    sum += (*src)[i - j];
                    count++;
                }
            }
            middle[i] = (count > 0) ? sum / count : 0.0;
        }
    }

    // Compute rolling standard deviation (population stddev over window)
    for (size_t i = 0; i < n; ++i) {
        size_t segmentStart = 0;
        if (resetOnNewDay) {
            for (auto it = segmentStarts.rbegin(); it != segmentStarts.rend(); ++it) {
                if (*it <= i) {
                    segmentStart = *it;
                    break;
                }
            }
        }
        
        size_t localIndex = i - segmentStart;
        if (localIndex < static_cast<size_t>(period - 1)) {
            upper[i] = Chart::NoValue;
            lower[i] = Chart::NoValue;
            continue;
        }

        // compute mean over the window for stddev calculation
        double mean = 0.0;
        size_t count = 0;
        size_t windowStart = std::max(segmentStart, i - period + 1);
        for (size_t j = windowStart; j <= i; ++j) {
            mean += (*src)[j];
            count++;
        }
        if (count > 0) mean /= count;

        double sumsq = 0.0;
        for (size_t j = windowStart; j <= i; ++j) {
            double d = (*src)[j] - mean;
            sumsq += d * d;
        }
        double sd = (count > 0) ? std::sqrt(sumsq / count) : 0.0;

        upper[i] = middle[i] + stdDevMultiplier * sd;
        lower[i] = middle[i] - stdDevMultiplier * sd;
    }

    return {middle, upper, lower};
}

std::vector<indicators::PivotPointsInstance::PivotPeriod> IndicatorMathUtils::calculatePivotPoints(
    const std::vector<double>& openData,
    const std::vector<double>& highData,
    const std::vector<double>& lowData,
    const std::vector<double>& closeData,
    const std::vector<be::Date>& dates,
    indicators::PivotPeriodType periodType,
    indicators::PivotCalculationMethod calcMethod
) {
    if (openData.empty() || highData.empty() || lowData.empty() || closeData.empty() || dates.empty())
        return {};

    std::vector<indicators::PivotPointsInstance::PivotPeriod> pivotPeriods;

    // Initialize all level vectors with zeros
    size_t dataSize = highData.size();
    std::vector<size_t> periodBoundaries;
    periodBoundaries.push_back(0);
    
    // need to use arrayMath to determine indices of new days, months etc
    be::Date currentDate = dates[0];
    
    for (size_t i = 1; i < dataSize; ++i) {
        const be::Date& date = dates[i];
        
        bool newPeriod = false;
        switch (periodType) {
            case indicators::PivotPeriodType::FourHour: {
                // We consider a new period if current hour is in {13, 17, 21, 1}
                // and different from the previous one (to avoid splitting multiple times at the same hour)
                int hour = static_cast<int>(date.hour);
                bool isBoundary = (hour == 13 || hour == 17 || hour == 21 || hour == 1);
                int prevHour = static_cast<int>(currentDate.hour);
                newPeriod = isBoundary && (hour != prevHour);
                // Also force split if day/month/year changes
                newPeriod = newPeriod ||
                            (date.day != currentDate.day) ||
                            (date.month != currentDate.month) ||
                            (date.year != currentDate.year);
                break;
            }
            case indicators::PivotPeriodType::Daily:
                // New day if the day changed
                newPeriod = (date.day != currentDate.day ||
                             date.month != currentDate.month ||
                             date.year != currentDate.year);
                break;
                
            case indicators::PivotPeriodType::Weekly: {
                // New week if day difference > 2 (weekend or holidays)
                int dayDiff = static_cast<int>(date.day - dates[i-1].day);
                bool isMonday = (dayDiff > 2);
                newPeriod = isMonday;
                break;
            }
                
            case indicators::PivotPeriodType::Monthly:
                // New month
                newPeriod = (date.month != currentDate.month ||
                             date.year != currentDate.year);
                break;
        }
        
        if (newPeriod) {
            periodBoundaries.push_back(i);
            currentDate = date;
        }
    }
    periodBoundaries.push_back(dataSize);  // Add the end
    
    // For each period, calculate pivot levels
    for (size_t i = 0; i < periodBoundaries.size() - 1; ++i) {
        size_t start = periodBoundaries[i];
        size_t end = periodBoundaries[i+1] - 1;
        
        // If first period incomplete (except for daily)
        if (i == 0 && periodType != indicators::PivotPeriodType::Daily) {
            continue;
        }
        
        // Get high, low, close for the previous period
        double open = 0.0;
        double high = -std::numeric_limits<double>::max();
        double low = std::numeric_limits<double>::max();
        double close = 0.0;
        
        // If it's the first period, use current data
        size_t calcStart = (i == 0) ? start : periodBoundaries[i-1];
        size_t calcEnd = (i == 0) ? end : start - 1;
        
        for (size_t j = calcStart; j <= calcEnd; ++j) {
            high = std::max(high, highData[j]);
            low = std::min(low, lowData[j]);
        }
        open = openData[calcStart];  // First value of the period
        close = closeData[calcEnd];  // Last value
        
        // The rest of pivot points calculation remains unchanged
        double pivot;
        switch (calcMethod) {
            case indicators::PivotCalculationMethod::OHLC:
                pivot = (high + low + close + open) / 4.0;
                break;
            case indicators::PivotCalculationMethod::HLO:
                pivot = (high + low + open) / 3.0;
                break;
            case indicators::PivotCalculationMethod::HLC:
            default:
                pivot = (high + low + close) / 3.0;
                break;
        }


        double r1 = (2.0 * pivot) - low;
        double s1 = (2.0 * pivot) - high;
        double r2 = pivot + (high - low);
        double s2 = pivot - (high - low);
        double r3 = high + 2.0 * (pivot - low);
        double s3 = low - 2.0 * (high - pivot);
        
        double mpr1 = (pivot + r1) / 2.0;
        double mr1r2 = (r1 + r2) / 2.0;
        double mr2r3 = (r2 + r3) / 2.0;
        double mps1 = (pivot + s1) / 2.0;
        double ms1s2 = (s1 + s2) / 2.0;
        double ms2s3 = (s2 + s3) / 2.0;

        // Create a new pivot period
        indicators::PivotPointsInstance::PivotPeriod period;
        period.indices[static_cast<size_t>(chart::AggregationLevel::Raw)] = {static_cast<int>(start), static_cast<int>(end)};
        
        // Store a single segment for each level during this period
        using LT = indicators::PivotPointsInstance::LevelType;
        period.levelValues[static_cast<int>(LT::Pivot)] = pivot;
        period.levelValues[static_cast<int>(LT::R1)] = r1;
        period.levelValues[static_cast<int>(LT::R2)] = r2;
        period.levelValues[static_cast<int>(LT::R3)] = r3;
        period.levelValues[static_cast<int>(LT::S1)] = s1;
        period.levelValues[static_cast<int>(LT::S2)] = s2;
        period.levelValues[static_cast<int>(LT::S3)] = s3;
        period.levelValues[static_cast<int>(LT::M_PR1)] = mpr1;
        period.levelValues[static_cast<int>(LT::M_R1R2)] = mr1r2;
        period.levelValues[static_cast<int>(LT::M_R2R3)] = mr2r3;
        period.levelValues[static_cast<int>(LT::M_PS1)] = mps1;
        period.levelValues[static_cast<int>(LT::M_S1S2)] = ms1s2;
        period.levelValues[static_cast<int>(LT::M_S2S3)] = ms2s3;
        
        pivotPeriods.push_back(period);
    }

    return pivotPeriods;
}
