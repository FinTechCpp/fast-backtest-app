#include "ui/chart/chartDataManager.h"
#include <QDebug>
#include <iostream>

ChartDataManager::ChartDataManager() {
}

ChartDataManager::~ChartDataManager() {
}

void ChartDataManager::setData(const std::vector<be::Candle>& candles, const std::vector<be::TradeData>& trades, const std::vector<be::EquityPoint>& equityCurve) {
    if (candles.empty()) return;

    // Vérifier si les données sont différentes des données actuelles
    bool dataChanged = true;
    
    // Si nous avons des données en cache, vérifier si elles ont changé
    if (m_aggregatedOHLCVCache[static_cast<size_t>(chart::AggregationLevel::Raw)].isValid) {
        // Vérifier si la taille a changé
        if (m_aggregatedOHLCVCache[static_cast<size_t>(chart::AggregationLevel::Raw)].close.size() == candles.size()) {
            // Vérifier quelques points pour voir si les données sont identiques
            // Nous vérifions le premier, le dernier et un point au milieu
            const std::vector<double>& cachedClose = m_aggregatedOHLCVCache[static_cast<size_t>(chart::AggregationLevel::Raw)].close;
            
            size_t size = cachedClose.size();
            dataChanged = false;
            
            // Vérifier le premier point
            if (std::abs(cachedClose[0] - candles[0].close) > 1e-10) {
                dataChanged = true;
            } 
            // Vérifier le dernier point
            else if (std::abs(cachedClose[size-1] - candles[size-1].close) > 1e-10) {
                dataChanged = true;
            }
            // Vérifier un point au milieu
            else if (std::abs(cachedClose[size/2] - candles[size/2].close) > 1e-10) {
                dataChanged = true;
            }
        }
    }

    // Si les données ont changé, mettre à jour le cache OHLCV et recalculer les indicateurs
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
        m_aggregatedOHLCVCache[static_cast<size_t>(chart::AggregationLevel::Raw)].isValid = true; // Les données brutes sont toujours valides

        updateHeikinAshiCache();
        
        m_aggregatedIndicatorsCache.fill(chart::IndicatorData());
        
        
        // Recalculer tous les indicateurs existants
        for (const auto& indicator : m_indicators) {
            calculateIndicator(*indicator);
        }
    }



    m_trades = trades;
    m_tradeIndices.clear();
    
    // Préallouer le vecteur d'indices
    m_tradeIndices.resize(m_trades.size());

    // Initialiser les indices raw pour tous les trades
    for (size_t i = 0; i < m_trades.size(); ++i) {
        size_t entryBar = m_trades[i].entryBar;
        size_t exitBar = m_trades[i].hasBeenClosed() ? m_trades[i].exitBar : entryBar;

        // Stocker les indices raw
        m_tradeIndices[i][static_cast<size_t>(chart::AggregationLevel::Raw)] = {entryBar, exitBar};
    }

    // Si le niveau d'agrégation a déjà été calculé, recalculer les indices des trades
    for (size_t i = 1; i < static_cast<size_t>(chart::AggregationLevel::Count); ++i) {
        if (m_aggregatedOHLCVCache[i].isValid) {
            precalculateTradeIndices(static_cast<chart::AggregationLevel>(i));
        }
    }
    
    
    if (equityCurve.empty()) return;
    
    size_t numPoints = equityCurve.size();
    size_t numBars = candles.size();
    
    // Réinitialiser les données d'équité
    m_equityData = chart::EquityData();
    
    // Préallouer pour le pire cas
    m_equityData.timestamps.reserve(numPoints);
    m_equityData.equity_values.reserve(numPoints);
    
    // Parcourir le reste des points
    for (size_t i = 0; i < numPoints; ++i) {
        be::EquityPoint currentValue = equityCurve[i];

        // S'assurer que l'index est dans les limites
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

// il me semble que l'on fait deux copy dans cette methode, une pour travailler dessus 
// et eviter de modifier les données originales, et une pour les stocker dans le cache
void ChartDataManager::aggregateOHLCV(chart::AggregationLevel level) {
    // Si déjà en cache et valide, ne rien faire
    if (m_aggregatedOHLCVCache[static_cast<size_t>(level)].isValid)
        return;
        
    // Créer un nouvel enregistrement dans le cache
    chart::AggregatedOHLCV& aggregatedData = m_aggregatedOHLCVCache[static_cast<size_t>(level)];
    
    // Vérifier qu'on a des données à agréger
    if (!m_aggregatedOHLCVCache[static_cast<size_t>(chart::AggregationLevel::Raw)].isValid) {
        aggregatedData.isValid = false;
        return;
    }

    // cela est a retravailler je n'aime pas cette logique
    // Trouver le niveau d'agrégation source optimal (le plus élevé disponible inférieur au niveau demandé)
    chart::AggregationLevel sourceLevel = chart::AggregationLevel::Raw;
    
    // Ordre hiérarchique des niveaux d'agrégation
    // std::vector<AggregationLevel> levelHierarchy = {
    //     chart::AggregationLevel::Raw,
    //     chart::AggregationLevel::OneMinute,
    //     chart::AggregationLevel::OneHour,
    //     chart::AggregationLevel::OneDay
    // };
    
    // // Trouver le niveau le plus élevé disponible qui est inférieur au niveau demandé
    // for (auto it = levelHierarchy.rbegin(); it != levelHierarchy.rend(); ++it) {
    //     if (*it < level && 
    //         m_aggregatedOHLCVCache.find(*it) != m_aggregatedOHLCVCache.end() && 
    //         m_aggregatedOHLCVCache[*it].isValid) {
    //         sourceLevel = *it;
    //         break;
    //     }
    // }
    
    // Si aucun niveau inférieur n'est disponible, on utilise les données brutes
    if (sourceLevel == chart::AggregationLevel::Raw && !m_aggregatedOHLCVCache[0].isValid) {
        // Calculer le niveau Raw s'il n'est pas déjà calculé
        aggregateOHLCV(chart::AggregationLevel::Raw);
    }
    
    // Obtenir les données source
    const chart::AggregatedOHLCV& sourceData = m_aggregatedOHLCVCache[0];
    
    // premier copy pour travailler sur les données
    // Faire des copies des données source pour l'agrégation
    std::vector<double> timestampsCopy = sourceData.timestamps;
    std::vector<double> openCopy = sourceData.open;
    std::vector<double> highCopy = sourceData.high;
    std::vector<double> lowCopy = sourceData.low;
    std::vector<double> closeCopy = sourceData.close;
    std::vector<double> volumeCopy = sourceData.volume;
    
    // Créer l'ArrayMath pour l'agrégation
    ArrayMath timestampsMath(vectorToDoubleArray(timestampsCopy));

    if (!configureAggregationSelector(timestampsMath, level)) {
        aggregatedData.isValid = false;
        return;
    }

    timestampsMath.trim(1); // Retirer le premier élément NoValue
    timestampsMath.insert(DoubleArray(&sourceData.timestamps[0], 1), 0);

    // Obtenir les indices après sélection
    DoubleArray indices = timestampsMath.result();

    aggregatedData.rawIndicesMapping.clear();
    int currentGroup = -1;
    for (int i = 0; i < indices.len; i++) {
        if (indices[i] != Chart::NoValue) {
            currentGroup++;
            aggregatedData.rawIndicesMapping.push_back({});
        }
        if (currentGroup >= 0) {
            // Ajouter cet indice brut au groupe courant
            aggregatedData.rawIndicesMapping[currentGroup].push_back(i);
        }
    }
    
    if (indices.len <= 0) {
        qDebug() << "Agrégation échouée - aucun point sélectionné";
        aggregatedData.isValid = false;
        return;
    }
    
    // Agréger les données OHLCV avec les stratégies appropriées
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

    // Vérifier que tous les tableaux agrégés ont la même taille
    if (times.len != open.len || times.len != high.len || times.len != low.len || 
        times.len != close.len || times.len != volume.len) {
        aggregatedData.isValid = false;
        return;
    }
    
    // deuxieme copy pour stocker les données agrégées
    // Stocker les résultats dans les vecteurs
    aggregatedData.timestamps.resize(times.len);
    aggregatedData.open.resize(open.len);
    aggregatedData.high.resize(high.len);
    aggregatedData.low.resize(low.len);
    aggregatedData.close.resize(close.len);
    aggregatedData.volume.resize(volume.len);

    // Copier les données
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

    // Précalculer les indices des trades pour ce nouveau niveau d'agrégation
    if (!m_trades.empty() && level != chart::AggregationLevel::Raw) {
        precalculateTradeIndices(level);

        // CORRECTION: Parcourir tous les points pivots pour ce niveau
        for (auto& [pivotId, periods] : m_pivotPeriods) {
            precalculatePivotIndices(periods, level);
        }
    }
}

void ChartDataManager::aggregateIndicators(chart::AggregationLevel level) {
    if (level == chart::AggregationLevel::Raw) return;

    // il faut peut etre faire ca aussi dans aggregateOHLCV

    chart::IndicatorData& aggregated = m_aggregatedIndicatorsCache[static_cast<size_t>(level)];

    // il faut trouvé le niveau d'agrégation source comme pour les OHLCV
    // pour l'instant on ne gère que le niveau Raw


    for (const auto& [id, values] : m_aggregatedIndicatorsCache[static_cast<size_t>(chart::AggregationLevel::Raw)].rsiValues) {
        if (!aggregated.isRsiValid(id)) {
            // Agréger les RSI
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
    for (const auto& [id, values] : m_aggregatedIndicatorsCache[static_cast<size_t>(chart::AggregationLevel::Raw)].cciValues) {
        if (!aggregated.isCciValid(id)) {
            std::vector<double> cciData = aggregateVector(values, level, Chart::AggregateLast);
            if (!cciData.empty()) {
                aggregated.cciValues[id] = std::move(cciData);
                aggregated.validCciIds.insert(id);
            }
        }
    }
}

std::vector<double> ChartDataManager::aggregateVector(const std::vector<double> &data, chart::AggregationLevel level, int aggregateMethod) const {
    if (level == chart::AggregationLevel::Raw)
        return data;

    std::vector<double> timestamps = m_aggregatedOHLCVCache[static_cast<size_t>(chart::AggregationLevel::Raw)].timestamps;
    std::vector<double> dataCopy = data;

    ArrayMath timestampsMath(DoubleArray(timestamps.data(), timestamps.size()));

    if (!configureAggregationSelector(timestampsMath, level))
        return std::vector<double>();

    DoubleArray indices = timestampsMath.result();
    if (indices.len <= 0) {
        qDebug() << "Agrégation échouée - aucun point sélectionné";
        return std::vector<double>();
    }

    DoubleArray result = timestampsMath.aggregate(
        DoubleArray(dataCopy.data(), dataCopy.size()), 
        aggregateMethod);
    return std::vector<double>(result.data, result.data + result.len);
}

std::vector<int> ChartDataManager::aggregateVector(const std::vector<int>& data, chart::AggregationLevel level, int aggregateMethod) const {
    std::vector<double> dataCopy(data.begin(), data.end());
    std::vector<double> dataAggregated = aggregateVector(dataCopy, level, aggregateMethod);
    return std::vector<int>(dataAggregated.begin(), dataAggregated.end());
}


void ChartDataManager::calculateRSI(int id, int period) {
    // Vérifier si les données nécessaires sont disponibles
    if (!m_aggregatedOHLCVCache[static_cast<size_t>(chart::AggregationLevel::Raw)].isValid || period < 2) return;

    // Obtenir les prix de clôture
    const std::vector<double>& closePrices = m_aggregatedOHLCVCache[static_cast<size_t>(chart::AggregationLevel::Raw)].close;

    std::vector<double> rsiValues = IndicatorMathUtils::calculateRSI(closePrices, period);

    // Mettre à jour le cache des indicateurs actifs
    m_aggregatedIndicatorsCache[static_cast<size_t>(chart::AggregationLevel::Raw)].rsiValues[id] = std::move(rsiValues);
}

void ChartDataManager::calculateEMA(int id, int period) {
    // Vérifier si les données nécessaires sont disponibles
    if (!m_aggregatedOHLCVCache[static_cast<size_t>(chart::AggregationLevel::Raw)].isValid || period < 2) return;

    // Obtenir les prix de clôture
    const std::vector<double>& closePrices = m_aggregatedOHLCVCache[static_cast<size_t>(chart::AggregationLevel::Raw)].close;

    std::vector<double> emaValues = IndicatorMathUtils::calculateEMA(closePrices, period);

    // Mettre à jour le cache des indicateurs actifs
    m_aggregatedIndicatorsCache[static_cast<size_t>(chart::AggregationLevel::Raw)].emaValues[id] = std::move(emaValues);
}

void ChartDataManager::calculateSupertrend(int id, int period, double multiplier) {
    // Vérifier si les données nécessaires sont disponibles
    if (!m_aggregatedOHLCVCache[static_cast<size_t>(chart::AggregationLevel::Raw)].isValid || period < 2) return;

    // Obtenir les prix
    const std::vector<double>& highPrices = m_aggregatedOHLCVCache[static_cast<size_t>(chart::AggregationLevel::Raw)].high;
    const std::vector<double>& lowPrices = m_aggregatedOHLCVCache[static_cast<size_t>(chart::AggregationLevel::Raw)].low;
    const std::vector<double>& closePrices = m_aggregatedOHLCVCache[static_cast<size_t>(chart::AggregationLevel::Raw)].close;

    std::vector<double> supertrendValues;
    std::vector<int> trendDirections;
    std::tie(supertrendValues, trendDirections) = IndicatorMathUtils::calculateSupertrend(highPrices, lowPrices, closePrices, period, multiplier);

    // Mettre à jour le cache des indicateurs actifs
    m_aggregatedIndicatorsCache[static_cast<size_t>(chart::AggregationLevel::Raw)].supertrendValues[id] = std::make_pair(std::move(supertrendValues), std::move(trendDirections));
}

void ChartDataManager::calculateStochastic(int id, int fastKPeriod, int slowKPeriod, int slowDPeriod) {
    // Vérifier si les données nécessaires sont disponibles
    if (!m_aggregatedOHLCVCache[static_cast<size_t>(chart::AggregationLevel::Raw)].isValid || fastKPeriod < 2 || slowKPeriod < 2 || slowDPeriod < 2) return;

    // Obtenir les prix
    const std::vector<double>& highPrices = m_aggregatedOHLCVCache[static_cast<size_t>(chart::AggregationLevel::Raw)].high;
    const std::vector<double>& lowPrices = m_aggregatedOHLCVCache[static_cast<size_t>(chart::AggregationLevel::Raw)].low;
    const std::vector<double>& closePrices = m_aggregatedOHLCVCache[static_cast<size_t>(chart::AggregationLevel::Raw)].close;

    auto [stochasticKValues, stochasticDValues] = IndicatorMathUtils::calculateStochastic(
        highPrices, lowPrices, closePrices, fastKPeriod, slowKPeriod, slowDPeriod
    );

    // Mettre à jour le cache des indicateurs actifs
    m_aggregatedIndicatorsCache[static_cast<size_t>(chart::AggregationLevel::Raw)].stochasticValues[id] = {
        std::move(stochasticKValues), std::move(stochasticDValues)
    };
}

void ChartDataManager::calculateATR(int id, int period, bool useLogScale) {
    // Vérifier si les données nécessaires sont disponibles
    if (!m_aggregatedOHLCVCache[static_cast<size_t>(chart::AggregationLevel::Raw)].isValid || period < 2) return;

    // Obtenir les prix
    const std::vector<double>& highPrices = m_aggregatedOHLCVCache[static_cast<size_t>(chart::AggregationLevel::Raw)].high;
    const std::vector<double>& lowPrices = m_aggregatedOHLCVCache[static_cast<size_t>(chart::AggregationLevel::Raw)].low;
    const std::vector<double>& closePrices = m_aggregatedOHLCVCache[static_cast<size_t>(chart::AggregationLevel::Raw)].close;

    std::vector<double> atrValues = IndicatorMathUtils::calculateATR(highPrices, lowPrices, closePrices, period, useLogScale);

    // Mettre à jour le cache des indicateurs actifs
    m_aggregatedIndicatorsCache[static_cast<size_t>(chart::AggregationLevel::Raw)].atrValues[id] = std::move(atrValues);
}

void ChartDataManager::calculateCCI(int id, int period) {
    // Vérifier si les données nécessaires sont disponibles
    if (!m_aggregatedOHLCVCache[static_cast<size_t>(chart::AggregationLevel::Raw)].isValid || period < 2) return;

    // Obtenir les prix
    const std::vector<double>& highPrices = m_aggregatedOHLCVCache[static_cast<size_t>(chart::AggregationLevel::Raw)].high;
    const std::vector<double>& lowPrices = m_aggregatedOHLCVCache[static_cast<size_t>(chart::AggregationLevel::Raw)].low;
    const std::vector<double>& closePrices = m_aggregatedOHLCVCache[static_cast<size_t>(chart::AggregationLevel::Raw)].close;

    std::vector<double> cciValues = IndicatorMathUtils::calculateCCI(highPrices, lowPrices, closePrices, period);

    // Mettre à jour le cache des indicateurs actifs
    m_aggregatedIndicatorsCache[static_cast<size_t>(chart::AggregationLevel::Raw)].cciValues[id] = std::move(cciValues);
}

void ChartDataManager::calculatePivotPoints(const indicators::PivotPointsInstance& config) {
    // Vérifier si les données nécessaires sont disponibles
    if (!m_aggregatedOHLCVCache[static_cast<size_t>(chart::AggregationLevel::Raw)].isValid) return;
    
    // Obtenir les prix
    std::vector<be::Date> dates = m_datesCache;
    std::vector<double> openPrices = m_aggregatedOHLCVCache[static_cast<size_t>(chart::AggregationLevel::Raw)].open;
    std::vector<double> highPrices = m_aggregatedOHLCVCache[static_cast<size_t>(chart::AggregationLevel::Raw)].high;
    std::vector<double> lowPrices = m_aggregatedOHLCVCache[static_cast<size_t>(chart::AggregationLevel::Raw)].low;
    std::vector<double> closePrices = m_aggregatedOHLCVCache[static_cast<size_t>(chart::AggregationLevel::Raw)].close;
    
    // Structure pour stocker les niveaux calculés
    std::vector<indicators::PivotPointsInstance::PivotPeriod> levelSegments = IndicatorMathUtils::calculatePivotPoints(
        openPrices, highPrices, lowPrices, closePrices, dates, 
        config.periodType, config.calculationMethod
    );

    m_pivotPeriods[config.id] = std::move(levelSegments);

    // Pré-calculer les indices pour tous les niveaux d'agrégation existants
    for (size_t i = 0; i < m_aggregatedOHLCVCache.size(); ++i) {
        if (i == static_cast<size_t>(chart::AggregationLevel::Raw) || !m_aggregatedOHLCVCache[i].isValid) 
            continue;

        precalculatePivotIndices(m_pivotPeriods[config.id], static_cast<chart::AggregationLevel>(i));
    }
}

void ChartDataManager::precalculatePivotIndices(std::vector<indicators::PivotPointsInstance::PivotPeriod>& periods, chart::AggregationLevel level) {
    
    // Vérifier que le niveau existe dans le cache
    const chart::AggregatedOHLCV& aggData = m_aggregatedOHLCVCache[static_cast<size_t>(level)];
    if (!aggData.isValid)
        return;

    const std::vector<std::vector<size_t>>& mapping = aggData.rawIndicesMapping;
    if (mapping.empty() || periods.empty())
        return;

    // Trier les périodes par ordre croissant de leur rawStartIndex
    // std::sort(periods.begin(), periods.end(), 
    //     [](const PivotPeriod& a, const PivotPeriod& b) {
    //         return a.rawStartIndex < b.rawStartIndex;
    //     });

    // Curseurs pour parcourir le mapping une seule fois
    size_t mappingIdx = 0;
    
    for (indicators::PivotPointsInstance::PivotPeriod& period : periods) {
        size_t aggStartIndex;
        size_t aggEndIndex;

        const std::pair<size_t, size_t>& rawIndices = period.indices[static_cast<size_t>(chart::AggregationLevel::Raw)];

        // Rechercher l'indice de début
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
        
        // Si on n'a pas trouvé l'indice de début, passer à la période suivante
        if (aggStartIndex < 0) continue;
                
        // Rechercher l'indice de fin à partir du dernier point trouvé
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
        
        // Si les deux indices sont valides, les stocker
        if (aggStartIndex >= 0 && aggEndIndex >= 0) {
            period.indices[static_cast<size_t>(level)] = {aggStartIndex, aggEndIndex};
        }
    }
}

void ChartDataManager::precalculateTradeIndices(chart::AggregationLevel level) {
    // Vérifier que le niveau existe dans le cache
    const chart::AggregatedOHLCV& aggData = m_aggregatedOHLCVCache[static_cast<size_t>(level)];
    if (!aggData.isValid)
        return;

    const std::vector<std::vector<size_t>>& mapping = aggData.rawIndicesMapping;
    if (mapping.empty() || m_trades.empty())
        return;
    
    // Trier les trades par ordre croissant de leur barre d'entrée pour optimiser la recherche
    std::vector<size_t> sortedTradeIndices;
    sortedTradeIndices.reserve(m_trades.size());
    
    for (size_t i = 0; i < m_trades.size(); ++i) {
        sortedTradeIndices.push_back(i);
    }
    
    std::sort(sortedTradeIndices.begin(), sortedTradeIndices.end(),
        [this](size_t a, size_t b) {
            return m_trades[a].entryBar < m_trades[b].entryBar;
        });
    
    // Curseur pour parcourir le mapping une seule fois
    size_t mappingIdx = 0;
    
    for (size_t idx : sortedTradeIndices) {
        const auto& trade = m_trades[idx];

        int rawEntryBar = static_cast<int>(trade.entryBar);
        int rawExitBar = trade.hasBeenClosed() ? static_cast<int>(trade.exitBar) : rawEntryBar;

        int aggEntryIndex = -1;
        int aggExitIndex = -1;
        
        // Rechercher l'indice d'entrée
        while (mappingIdx < mapping.size()) {
            bool found = false;
            for (int rawIdx : mapping[mappingIdx]) {
                if (rawIdx == rawEntryBar) {
                    aggEntryIndex = static_cast<int>(mappingIdx);
                    found = true;
                    break;
                }
            }
            
            if (found) break;
            mappingIdx++;
        }
        
        // Si on n'a pas trouvé l'indice d'entrée, passer au trade suivant
        if (aggEntryIndex < 0) continue;
        
        // Rechercher l'indice de sortie à partir du point d'entrée
        size_t exitMappingIdx = mappingIdx;
        while (exitMappingIdx < mapping.size()) {
            bool found = false;
            for (int rawIdx : mapping[exitMappingIdx]) {
                if (rawIdx == rawExitBar) {
                    aggExitIndex = static_cast<int>(exitMappingIdx);
                    found = true;
                    break;
                }
            } 

            // Ici on n'incremente pas l'indice sur la sortie au cas ou le trade d'après a sa sortie avant
            
            if (found) break;
            exitMappingIdx++;
        }
        
        // Si on n'a pas trouvé l'indice de sortie, utiliser l'indice d'entrée
        if (aggExitIndex < 0) {
            aggExitIndex = aggEntryIndex;
        }
        
        // Stocker les indices pour ce trade et ce niveau d'agrégation
        m_tradeIndices[idx][static_cast<size_t>(level)] = {aggEntryIndex, aggExitIndex};
    }
}

// Méthode utilitaire pour configurer le sélecteur d'agrégation
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
        // Ne rien faire pour Raw
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

    // Déterminer le niveau d'agrégation de départ en fonction de la période des données
    chart::AggregationLevel startLevel = determineStartingAggregationLevel(timestamps);

    // Trouver l'indice du niveau de départ dans notre liste
    size_t startIdx = 0;
    for (size_t i = 0; i < static_cast<size_t>(chart::AggregationLevel::Count); i++) {
        if (i == static_cast<size_t>(startLevel)) {
            startIdx = i;
            break;
        }
    }

    // Parcourir les niveaux d'agrégation à partir du niveau de départ
    for (size_t i = startIdx + 1; i < static_cast<size_t>(chart::AggregationLevel::Count); i++) {
        chart::AggregationLevel level = static_cast<chart::AggregationLevel>(i);

        if (!m_aggregatedOHLCVCache[i].isValid) {
            aggregateOHLCV(level);
        }

        aggregateIndicators(level);

        const chart::AggregatedOHLCV& aggregatedData = m_aggregatedOHLCVCache[i];
        if (!aggregatedData.isValid) continue;

        // Extraire les timestamps de début et de fin de l'intervalle à afficher
        double startTime = timestamps[0];
        double endTime = timestamps[timestamps.len - 1];
            
        // Trouver les indices correspondants dans les données agrégées
        size_t aggStartIdx = findClosestIndex(aggregatedData.timestamps, startTime);
        size_t aggEndIdx = findClosestIndex(aggregatedData.timestamps, endTime, true);

        // Calculer combien de points agrégés seraient visibles dans cette plage
        int visibleAggPoints = (aggEndIdx >= aggStartIdx) ? (aggEndIdx - aggStartIdx + 1) : 0;

        // Si l'agrégation est valide et réduit suffisamment les données
        if (visibleAggPoints > 0 && visibleAggPoints <= m_maxDisplayPoints) {
            result.level = level;
            result.startIndex = aggStartIdx;
            result.pointCount = visibleAggPoints;
            return result;
        }
    }

    // Si aucun niveau ne convient, utiliser le plus élevé disponible
    size_t highestLevel = static_cast<size_t>(static_cast<int>(chart::AggregationLevel::Count) - 1);
    if (m_aggregatedOHLCVCache[highestLevel].isValid) {
        result.level = static_cast<chart::AggregationLevel>(highestLevel);
        result.startIndex = 0;
        result.pointCount = m_aggregatedOHLCVCache[highestLevel].timestamps.size();
    }

    return result;
}

void ChartDataManager::setMaxDisplayPoints(int value) {
    if (value < 100) value = 100; // Valeur minimale pour éviter les problèmes
    if (value > 100000) value = 100000; // Limiter à la valeur maximale spécifiée
    
    if (m_maxDisplayPoints == value) 
        return; // Pas de changement
        
    m_maxDisplayPoints = value;

}

std::optional<size_t> ChartDataManager::rawToAggregatedIndex(chart::AggregationLevel level, size_t rawIndex) const {
    // Si pas d'agrégation, l'indice est identique
    if (level == chart::AggregationLevel::Raw)
        return rawIndex;
        
    // Vérifier que le niveau existe dans le cache
    const chart::AggregatedOHLCV& aggData = m_aggregatedOHLCVCache[static_cast<size_t>(level)];
    if (aggData.rawIndicesMapping.empty() || !aggData.isValid)
        return std::nullopt;

    const auto& mapping = aggData.rawIndicesMapping;
    for (size_t i = 0; i < mapping.size(); ++i) {
        if (mapping[i].empty())
            continue;
        // On regarde le dernier indice du groupe
        size_t lastIdx = mapping[i].back();
        if (rawIndex <= lastIdx)
            return i;
    }
    return std::nullopt;
}

const std::vector<size_t>& ChartDataManager::getAggregatedToRawIndices(chart::AggregationLevel level, size_t aggregatedIndex) const {
    static const std::vector<size_t> empty;
    
    // Cas spécial pour Raw: retourner singleton avec l'indice lui-même
    if (level == chart::AggregationLevel::Raw) {
        static std::vector<size_t> singleIndex;
        singleIndex = {aggregatedIndex};
        return singleIndex;
    }
    
    // Vérifier que le niveau et l'indice sont valides
    const chart::AggregatedOHLCV& aggData = m_aggregatedOHLCVCache[static_cast<size_t>(level)];
    if (!aggData.isValid || aggregatedIndex >= static_cast<size_t>(aggData.rawIndicesMapping.size()))
        return empty;
        
    return aggData.rawIndicesMapping[aggregatedIndex];
}

int ChartDataManager::aggregatedToFirstRawIndex(chart::AggregationLevel level, int aggregatedIndex) const {
    // Si pas d'agrégation, l'indice est identique
    if (level == chart::AggregationLevel::Raw)
        return aggregatedIndex;
        
    // Obtenir tous les indices raw correspondants
    const auto& rawIndices = getAggregatedToRawIndices(level, aggregatedIndex);
    
    // Retourner le premier s'il existe
    return rawIndices.empty() ? -1 : rawIndices.front();
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
    if (const indicators::PivotPointsInstance* pivotConfig = dynamic_cast<const indicators::PivotPointsInstance*>(&config)) {
        calculatePivotPoints(*pivotConfig);
        for (auto& aggregated : m_aggregatedIndicatorsCache)
            aggregated.validPivotPointsIds.erase(pivotConfig->id);
        return;
    }
}

chart::AggregationLevel ChartDataManager::determineStartingAggregationLevel(const DoubleArray& timestamps) const {
    // Si moins de deux timestamps, impossible de déterminer la période
    if (timestamps.len < 2) {
        return chart::AggregationLevel::Raw;
    }

    // La période est constante : prendre la différence entre les deux premiers points
    double period = timestamps[1] - timestamps[0];

    // Déterminer le niveau de départ en fonction de la période
    if (period >= 86400) {   // Plus d'un jour
        return chart::AggregationLevel::OneDay;
    } else if (period >= 3600) {    // Plus d'une heure
        return chart::AggregationLevel::OneHour;
    } else if (period >= 60) {      // Plus d'une minute
        return chart::AggregationLevel::OneMinute;
    } else {
        return chart::AggregationLevel::Raw;  // Moins d'une minute
    }
}

size_t ChartDataManager::findClosestIndex(const std::vector<double>& values, double target, bool searchForward) const {
    // Si searchForward est true, on cherche le premier élément >= target
    // Sinon, on cherche le dernier élément <= target

    if (values.empty()) return 0;

    if (searchForward) {
        // Rechercher vers l'avant (premier élément >= target)
        for (size_t i = 0; i < values.size(); i++)
            if (values[i] >= target)
                return i;
        return values.size() - 1;
    } else {
        // Rechercher vers l'arrière (dernier élément <= target)
        for (int i = values.size() - 1; i >= 0; i--)
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
