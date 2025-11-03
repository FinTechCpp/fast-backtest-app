#include "components/Utils/IndicatorMathUtils.h"
#include "common.h"
#include <algorithm>

std::vector<double> IndicatorMathUtils::calculateRSI(const std::vector<double>& closeData, int period)
{
    size_t dataSize = closeData.size();
    std::vector<double> rsiValues(dataSize, 0.0);

    if (dataSize <= period) {
        std::fill(rsiValues.begin(), rsiValues.end(), 50.0);  // Default neutral value
        return rsiValues;
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

    // Default values for the first periods where RSI is undefined
    for (int i = 0; i < period; ++i) {
        rsiValues[i] = 50.0;  // Neutral value
    }

    // Calculate the first average
    double avgGain = 0;
    double avgLoss = 0;
    for (int i = 0; i < period; ++i) {
        avgGain += gains[i];
        avgLoss += losses[i];
    }
    avgGain /= period;
    avgLoss /= period;

    // Calculate the first RSI
    double rs = (avgLoss > 0) ? (avgGain / avgLoss) : 100.0;
    rsiValues[period] = 100.0 - (100.0 / (1.0 + rs));

    // Calculate RSI for the remaining points (Wilder's method)
    for (size_t i = period + 1; i < dataSize; ++i) {
        // Calculate smoothed averages
        avgGain = ((period - 1) * avgGain + gains[i - 1]) / period;
        avgLoss = ((period - 1) * avgLoss + losses[i - 1]) / period;
        
        // Avoid division by zero
        if (avgLoss > 0) {
            rs = avgGain / avgLoss;
            rsiValues[i] = 100.0 - (100.0 / (1.0 + rs));
        } else {
            rsiValues[i] = 100.0;
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

std::vector<double> IndicatorMathUtils::calculateEMA(const std::vector<double>& closeData, int period)
{
    size_t dataSize = closeData.size();
    std::vector<double> emaValues(dataSize, 0.0);

    if (dataSize <= period) {
        std::copy(closeData.begin(), closeData.end(), emaValues.begin());
        return emaValues;
    }
    
    // Calculate the smoothing factor
    double multiplier = 2.0 / (period + 1.0);
    
    // First EMA value = simple average of the first 'period' points
    double sum = 0.0;
    for (int i = 0; i < period; ++i) {
        sum += closeData[i];
        emaValues[i] = closeData[i];  // Use the price itself for the first points
    }
    emaValues[period-1] = sum / period;
    
    // Calculate EMA for the remaining points
    for (size_t i = period; i < dataSize; ++i) {
        emaValues[i] = (closeData[i] - emaValues[i-1]) * multiplier + emaValues[i-1];
    }
    return emaValues;
}

std::tuple<std::vector<double>, std::vector<int>> IndicatorMathUtils::calculateSupertrend(
    const std::vector<double>& highData,
    const std::vector<double>& lowData,
    const std::vector<double>& closeData,
    int period,
    double multiplier)
{
    size_t dataSize = closeData.size();
    std::vector<double> supertrendValues(dataSize);
    std::vector<int> trendDirections(dataSize, 0);

    if (dataSize <= period) {
        std::fill(supertrendValues.begin(), supertrendValues.end(), 0.0);
        return {supertrendValues, trendDirections};
    }
    
    // Calculate the ATR
    std::vector<double> atrValues = calculateATR(highData, lowData, closeData, period, false);
    
    // Calculate base bands (HL2 +/- multiplier * ATR)
    std::vector<double> basicUpperBand(dataSize);
    std::vector<double> basicLowerBand(dataSize);
    std::vector<double> finalUpperBand(dataSize);
    std::vector<double> finalLowerBand(dataSize);
    
    for (size_t i = 0; i < dataSize; ++i) {
        if (i < period) {
            basicUpperBand[i] = 0.0;
            basicLowerBand[i] = 0.0;
            finalUpperBand[i] = 0.0;
            finalLowerBand[i] = 0.0;
            supertrendValues[i] = 0.0;
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

std::tuple<std::vector<double>, std::vector<double>> IndicatorMathUtils::calculateStochastic(
    const std::vector<double>& highData,
    const std::vector<double>& lowData,
    const std::vector<double>& closeData,
    int fastKPeriod,
    int slowKPeriod,
    int slowDPeriod)
{
    // Input data validation
    size_t dataSize = closeData.size();
    if (dataSize == 0 || highData.size() != dataSize || lowData.size() != dataSize)
        return {std::vector<double>(), std::vector<double>()};
    
    // Resize output vectors
    std::vector<double> kValues(dataSize);
    std::vector<double> dValues(dataSize);

    // Default values (50 is a neutral value for the oscillator)
    std::fill(kValues.begin(), kValues.end(), 50.0);
    std::fill(dValues.begin(), dValues.end(), 50.0);
    
    if (dataSize < static_cast<size_t>(fastKPeriod)) 
        return {kValues, dValues};  // Not enough data to compute Stochastic
    
    // Step 1: Calculate raw %K (Fast %K) - The formula is:
    // %K = 100 * (C - L14) / (H14 - L14)
    // where C is the current close, L14 is the lowest low over 14 periods
    // and H14 is the highest high over 14 periods
    std::vector<double> rawK(dataSize);
    
    for (size_t i = fastKPeriod - 1; i < dataSize; ++i) {
        // Find lowest low and highest high over the fastKPeriod
        double lowestLow = std::numeric_limits<double>::max();
        double highestHigh = std::numeric_limits<double>::lowest();
        
        for (size_t j = i - fastKPeriod + 1; j <= i; ++j) {
            lowestLow = std::min(lowestLow, lowData[j]);
            highestHigh = std::max(highestHigh, highData[j]);
        }
        
        // Calculate raw %K
        double range = highestHigh - lowestLow;
        if (range > 0.0) {
            rawK[i] = ((closeData[i] - lowestLow) / range) * 100.0;
        } else {
            rawK[i] = 50.0; // Neutral value if range is zero
        }
    }
    
    // Step 2: Smooth raw %K with a moving average over slowKPeriod to get slow %K
    for (size_t i = 0; i < dataSize; ++i) {
        if (i < fastKPeriod - 1 + slowKPeriod - 1) {
            kValues[i] = 50.0;  // Not enough data, neutral value
            continue;
        }
        
        double sum = 0.0;
        for (size_t j = 0; j < slowKPeriod; ++j) {
            sum += rawK[i - j];
        }
        kValues[i] = sum / slowKPeriod;
    }
    
    // Step 3: Calculate %D as a moving average of %K values over slowDPeriod
    for (size_t i = 0; i < dataSize; ++i) {
        if (i < fastKPeriod - 1 + slowKPeriod - 1 + slowDPeriod - 1) {
            dValues[i] = 50.0;  // Not enough data, neutral value
            continue;
        }
        
        double sum = 0.0;
        for (size_t j = 0; j < slowDPeriod; ++j) {
            sum += kValues[i - j];
        }
        dValues[i] = sum / slowDPeriod;
    }

    return {kValues, dValues};
}

std::vector<double> IndicatorMathUtils::calculateATR(
    const std::vector<double>& highData,
    const std::vector<double>& lowData,
    const std::vector<double>& closeData,
    int period,
    bool useLogScale)
{
    size_t dataSize = closeData.size();
    std::vector<double> atrValues(dataSize, 0.0);

    if (dataSize < static_cast<size_t>(period)) {
        std::fill(atrValues.begin(), atrValues.end(), 0.0);
        return atrValues;
    }
    
    // Calculate price variations
    std::vector<double> tr(dataSize - 1);
    for (size_t i = 1; i < dataSize; ++i) {
        double highLow = highData[i] - lowData[i];
        double highClose = std::abs(highData[i] - closeData[i - 1]);
        double lowClose = std::abs(lowData[i] - closeData[i - 1]);
        tr[i - 1] = std::max({highLow, highClose, lowClose});
    }
    
    // Calculate the first average
    double sum = 0.0;
    for (int i = 0; i < period; ++i) {
        sum += tr[i];
        double atrValue = sum / period;
        
        // Apply logarithm immediately if needed
        atrValues[i] = useLogScale ? std::log(atrValue + 1) : atrValue;
    }
    
    // Calculate ATR for remaining points (Wilder's method)
    for (size_t i = period; i < dataSize; ++i) {
        double atrValue = (atrValues[i - 1] * (period - 1) + tr[i - 1]) / period;
        
        // If using log scale, first convert previous log(atr+1) back to atr before using it
        if (useLogScale) {
            double prevATR = std::exp(atrValues[i - 1]) - 1;  // Recover true ATR value
            atrValue = (prevATR * (period - 1) + tr[i - 1]) / period;
            atrValues[i] = std::log(atrValue + 1);  // Store in logarithm
        } else {
            atrValues[i] = atrValue;  // Store normally
        }
    }

    return atrValues;
}

std::vector<double> IndicatorMathUtils::calculateCCI(
    const std::vector<double>& highData,
    const std::vector<double>& lowData,
    const std::vector<double>& closeData,
    int period)
{
    size_t dataSize = closeData.size();
    std::vector<double> cciValues(dataSize, 0.0);

    if (dataSize < static_cast<size_t>(period)) {
        return cciValues;
    }

    // Calculate Typical Price (TP) = (High + Low + Close) / 3
    std::vector<double> typicalPrice(dataSize);
    for (size_t i = 0; i < dataSize; ++i) {
        typicalPrice[i] = (highData[i] + lowData[i] + closeData[i]) / 3.0;
    }

    // Calculate CCI for each point
    for (size_t i = period - 1; i < dataSize; ++i) {
        // Calculate Simple Moving Average of Typical Price
        double smaTP = 0.0;
        for (int j = 0; j < period; ++j) {
            smaTP += typicalPrice[i - j];
        }
        smaTP /= period;

        // Calculate Mean Deviation
        double meanDeviation = 0.0;
        for (int j = 0; j < period; ++j) {
            meanDeviation += std::abs(typicalPrice[i - j] - smaTP);
        }
        meanDeviation /= period;

        // Calculate CCI
        // CCI = (Typical Price - SMA(TP)) / (0.015 * Mean Deviation)
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
    int signalSmoothing)
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

    // ensure fast < slow by convention (if caller swapped, still works)
    // we compute EMA for both periods independently
    double multFast = 2.0 / (fastPeriod + 1.0);
    double multSlow = 2.0 / (slowPeriod + 1.0);
    double multSignal = 2.0 / (signalPeriod + 1.0);

    std::vector<double> fastEma(n, 0.0);
    std::vector<double> slowEma(n, 0.0);

    // seed fast EMA with SMA when enough points
    if (n >= static_cast<size_t>(fastPeriod)) {
        double sum = 0.0;
        for (int i = 0; i < fastPeriod; ++i) sum += (*sourceData)[i];
        fastEma[fastPeriod - 1] = sum / fastPeriod;
        for (size_t i = fastPeriod; i < n; ++i) fastEma[i] = ((*sourceData)[i] - fastEma[i - 1]) * multFast + fastEma[i - 1];
    }

    // seed slow EMA with SMA when enough points
    if (n >= static_cast<size_t>(slowPeriod)) {
        double sum = 0.0;
        for (int i = 0; i < slowPeriod; ++i) sum += (*sourceData)[i];
        slowEma[slowPeriod - 1] = sum / slowPeriod;
        for (size_t i = slowPeriod; i < n; ++i) slowEma[i] = ((*sourceData)[i] - slowEma[i - 1]) * multSlow + slowEma[i - 1];
    }

    // Build macd line where both EMAs are available
    std::vector<double> macdHistory; macdHistory.reserve(n);
    std::vector<size_t> macdIndexes; macdIndexes.reserve(n);
    for (size_t i = 0; i < n; ++i) {
        bool fastReady = (i >= static_cast<size_t>(fastPeriod - 1));
        bool slowReady = (i >= static_cast<size_t>(slowPeriod - 1));
        if (fastReady && slowReady) {
            macdLine[i] = fastEma[i] - slowEma[i];
            macdHistory.push_back(macdLine[i]);
            macdIndexes.push_back(i);
        } else {
            macdLine[i] = 0.0;
        }
    }

    // Compute signal line as EMA over MACD history
    if (!macdHistory.empty() && macdHistory.size() >= static_cast<size_t>(signalPeriod)) {
        // initial SMA over first signalPeriod MACD values
        double sum = 0.0;
        for (int k = 0; k < signalPeriod; ++k) sum += macdHistory[k];
        double sig = sum / signalPeriod;
        // assign signal value to corresponding global index
        size_t idx = macdIndexes[signalPeriod - 1];
        signalLine[idx] = sig;
        histogram[idx] = macdLine[idx] - sig;

        // continue EMA on remaining macdHistory entries
        for (size_t h = signalPeriod; h < macdHistory.size(); ++h) {
            sig = (macdHistory[h] - sig) * multSignal + sig;
            size_t globalIdx = macdIndexes[h];
            signalLine[globalIdx] = sig;
            histogram[globalIdx] = macdLine[globalIdx] - sig;
        }
    }

    // For indices before signal initialized, signalLine & histogram remain 0.0 (consistent with other helpers)
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
    filter::MAType oscMAType
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

    // Compute middle band (SMA or EMA)
    if (oscMAType == filter::MAType::EMA) {
        middle = calculateEMA(*src, period);
    } else {
        // Simple moving average (SMA)
        double sum = 0.0;
        for (int i = 0; i < period; ++i) sum += (*src)[i];
        middle[period - 1] = sum / period;
        for (size_t i = period; i < n; ++i) {
            sum += (*src)[i];
            sum -= (*src)[i - period];
            middle[i] = sum / period;
        }
        // lower indices remain 0.0
    }

    // Compute rolling standard deviation (population stddev over window)
    for (size_t i = static_cast<size_t>(period - 1); i < n; ++i) {
        // compute mean over the window for stddev calculation
        double mean = 0.0;
        for (size_t j = i - period + 1; j <= i; ++j) mean += (*src)[j];
        mean /= period;

        double sumsq = 0.0;
        for (size_t j = i - period + 1; j <= i; ++j) {
            double d = (*src)[j] - mean;
            sumsq += d * d;
        }
        double sd = std::sqrt(sumsq / period);

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
