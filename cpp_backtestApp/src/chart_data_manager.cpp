#include "chart_data_manager.h"
#include "components/technical_indicators.h"
#include <QDebug>


const std::array<ChartDataManager::ChartTypeInfo, static_cast<size_t>(ChartDataManager::ChartType::Count)> ChartDataManager::s_chartTypeData = {{
    { ChartDataManager::ChartType::CandleStick, "CandleStick" },
    { ChartDataManager::ChartType::HeikinAshi, "HeikinAshi" },
    { ChartDataManager::ChartType::OHLC, "OHLC" },
    { ChartDataManager::ChartType::Close, "Close" }
}};

ChartDataManager::ChartDataManager() {
}

ChartDataManager::~ChartDataManager() {
}

void ChartDataManager::setBacktestData(const std::shared_ptr<const be::Data>& data) {
    m_backtestData = data;
    
    if (data) {
        prepareTimestampsCache();
        updateHeikinAshiCache();
        
        m_aggregationCache.clear();
        m_activeIndicators.clear();
    }
}

void ChartDataManager::setEquityCurve(const std::vector<double>& equityCurve) {
    if (equityCurve.empty() || !m_backtestData) {
        qWarning() << "Courbe d'équité vide ou données de prix invalides";
        return;
    }
    
    size_t numPoints = equityCurve.size();
    size_t numBars = m_backtestData->size();
    
    // Réinitialiser les données d'équité
    m_equityData = EquityData();
    
    // Valider les tailles
    if (numPoints != numBars) {
        qWarning() << "Tailles incompatibles: equityCurve:" << numPoints << "data:" << numBars;
        return;
    }
    
    // Préallouer pour le pire cas
    m_equityData.timestamps.reserve(numPoints);
    m_equityData.equity_values.reserve(numPoints);
    
    // Compresser les données en ne gardant que les points où l'équité change
    double lastValue = equityCurve[0];
    
    // Toujours ajouter le premier point
    m_equityData.timestamps.push_back(dateToChartTimestamp(m_backtestData->at(0).date));
    m_equityData.equity_values.push_back(lastValue);
    
    // Parcourir le reste des points
    for (size_t i = 1; i < numPoints; ++i) {
        double currentValue = equityCurve[i];
        
        // Si la valeur a changé ou si c'est le dernier point, l'ajouter
        if (std::abs(currentValue - lastValue) > 1e-10 || i == numPoints - 1) {
            m_equityData.timestamps.push_back(dateToChartTimestamp(m_backtestData->at(i).date));
            m_equityData.equity_values.push_back(currentValue);
            lastValue = currentValue;
        }
    }
}

void ChartDataManager::prepareTimestampsCache() {
    if (!m_backtestData || m_backtestData->size() == 0) {
        m_timestampsCache.clear();
        return;
    }
    
    const auto& dates = m_backtestData->getDates();
    size_t dataSize = dates.size();
    
    // Réserver la capacité et convertir toutes les dates en timestamps
    m_timestampsCache.clear();
    m_timestampsCache.reserve(dataSize);
    
    for (const auto& date : dates) {
        double timestamp = dateToChartTimestamp(date);
        m_timestampsCache.push_back(timestamp);
    }
}

double ChartDataManager::dateToChartTimestamp(const be::Date& date) const {
    return Chart::chartTime(date.getYear(), date.getMonth(), date.getDay(), date.getHour(), date.getMinute(), date.getSecond());
}

void ChartDataManager::updateHeikinAshiCache() {
    if (!hasValidData()) {
        qWarning() << "Tentative de mise à jour du cache Heikin-Ashi avec des données vides";
        m_heikinAshiCache.isValid = false;
        return;
    }

    TechnicalIndicators::calculateHeikinAshi(
        m_backtestData->getOpen(), 
        m_backtestData->getHigh(), 
        m_backtestData->getLow(),
        m_backtestData->getClose(),
        m_heikinAshiCache.open,
        m_heikinAshiCache.high,
        m_heikinAshiCache.low,
        m_heikinAshiCache.close
    );
    
    m_heikinAshiCache.isValid = true;
}

void ChartDataManager::aggregateData(AggregationLevel level) {
    // Si déjà en cache et valide, ne rien faire
    if (m_aggregationCache.find(level) != m_aggregationCache.end() && 
        m_aggregationCache[level].isValid) {
        return;
    }
        
    // Créer un nouvel enregistrement dans le cache
    AggregatedOHLCV& aggregatedData = m_aggregationCache[level];
    
    // Vérifier qu'on a des données à agréger
    if (m_timestampsCache.empty() || !m_backtestData) {
        aggregatedData.isValid = false;
        return;
    }

    // Pour le cas Raw, copier les données originales
    if (level == AggregationLevel::Raw) {
        // Créer des copies des données brutes dans les vecteurs
        aggregatedData.timestamps = m_timestampsCache;
        aggregatedData.open = m_backtestData->getOpen();
        aggregatedData.high = m_backtestData->getHigh();
        aggregatedData.low = m_backtestData->getLow();
        aggregatedData.close = m_backtestData->getClose();
        aggregatedData.volume = m_backtestData->getVolume();
        aggregatedData.level = AggregationLevel::Raw;
        aggregatedData.isValid = true;
        return;
    }

    // Trouver le niveau d'agrégation source optimal (le plus élevé disponible inférieur au niveau demandé)
    AggregationLevel sourceLevel = AggregationLevel::Raw;
    
    // Ordre hiérarchique des niveaux d'agrégation
    std::vector<AggregationLevel> levelHierarchy = {
        AggregationLevel::Raw,
        AggregationLevel::OneMinute,
        AggregationLevel::OneHour,
        AggregationLevel::OneDay
    };
    
    // Trouver le niveau le plus élevé disponible qui est inférieur au niveau demandé
    for (auto it = levelHierarchy.rbegin(); it != levelHierarchy.rend(); ++it) {
        if (*it < level && 
            m_aggregationCache.find(*it) != m_aggregationCache.end() && 
            m_aggregationCache[*it].isValid) {
            sourceLevel = *it;
            break;
        }
    }
    
    // Si aucun niveau inférieur n'est disponible, on utilise les données brutes
    if (sourceLevel == AggregationLevel::Raw && !m_aggregationCache[sourceLevel].isValid) {
        // Calculer le niveau Raw s'il n'est pas déjà calculé
        aggregateData(AggregationLevel::Raw);
    }
    
    // Obtenir les données source
    const AggregatedOHLCV& sourceData = m_aggregationCache[sourceLevel];
    
    // Faire des copies des données source pour l'agrégation
    std::vector<double> timestampsCopy = sourceData.timestamps;
    std::vector<double> openCopy = sourceData.open;
    std::vector<double> highCopy = sourceData.high;
    std::vector<double> lowCopy = sourceData.low;
    std::vector<double> closeCopy = sourceData.close;
    std::vector<double> volumeCopy = sourceData.volume;
    
    // Créer l'ArrayMath pour l'agrégation
    ArrayMath timestampsMath(DoubleArray(timestampsCopy.data(), timestampsCopy.size()));
    
    // Appliquer le sélecteur approprié selon le niveau d'agrégation demandé
    switch (level) {
        case AggregationLevel::OneMinute:
            timestampsMath.selectStartOfMinute();
            break;
        case AggregationLevel::OneHour:
            timestampsMath.selectStartOfHour();
            break;
        case AggregationLevel::OneDay:
            timestampsMath.selectStartOfDay();
            break;
        default:
            // Ne devrait pas arriver car Raw est géré plus haut
            aggregatedData.isValid = false;
            return;
    }
    
    // Obtenir les indices après sélection
    DoubleArray indices = timestampsMath.result();
    
    if (indices.len <= 0) {
        qDebug() << "Agrégation échouée - aucun point sélectionné";
        aggregatedData.isValid = false;
        return;
    }
    
    // Agréger les données OHLCV avec les stratégies appropriées
    DoubleArray times = timestampsMath.aggregate(
        DoubleArray(timestampsCopy.data(), timestampsCopy.size()), 
        Chart::AggregateFirst);
    
    DoubleArray open = timestampsMath.aggregate(
        DoubleArray(openCopy.data(), openCopy.size()), 
        Chart::AggregateFirst);
    
    DoubleArray high = timestampsMath.aggregate(
        DoubleArray(highCopy.data(), highCopy.size()), 
        Chart::AggregateMax);
    
    DoubleArray low = timestampsMath.aggregate(
        DoubleArray(lowCopy.data(), lowCopy.size()), 
        Chart::AggregateMin);
    
    DoubleArray close = timestampsMath.aggregate(
        DoubleArray(closeCopy.data(), closeCopy.size()), 
        Chart::AggregateLast);
    
    DoubleArray volume = timestampsMath.aggregate(
        DoubleArray(volumeCopy.data(), volumeCopy.size()), 
        Chart::AggregateSum);

    // Vérifier que tous les tableaux agrégés ont la même taille
    if (times.len != open.len || times.len != high.len || times.len != low.len || 
        times.len != close.len || times.len != volume.len) {
        aggregatedData.isValid = false;
        return;
    }
    
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
    
    aggregatedData.level = level;
    aggregatedData.isValid = true;
}

void ChartDataManager::calculateRSI(int id, int period)
{
    // Vérifier si les données nécessaires sont disponibles
    if (!hasValidData() || period < 2) return;

    // Obtenir les prix de clôture
    const std::vector<double>& closePrices = m_backtestData->getClose();
    
    std::vector<double> rsiValues;
    TechnicalIndicators::calculateRSI(closePrices, period, rsiValues);

    // Mettre à jour le cache des indicateurs actifs
    m_activeIndicators.rsiValues[id] = std::move(rsiValues);
}

void ChartDataManager::calculateEMA(int id, int period)
{
    // Vérifier si les données nécessaires sont disponibles
    if (!hasValidData() || period < 2) return;

    // Obtenir les prix de clôture
    const std::vector<double>& closePrices = m_backtestData->getClose();
    
    std::vector<double> emaValues;
    TechnicalIndicators::calculateEMA(closePrices, period, emaValues);

    // Mettre à jour le cache des indicateurs actifs
    m_activeIndicators.emaValues[id] = std::move(emaValues);
}

void ChartDataManager::calculateStochastic(int id, int fastKPeriod, int slowKPeriod, int slowDPeriod)
{
    // Vérifier si les données nécessaires sont disponibles
    if (!hasValidData() || fastKPeriod < 2 || slowKPeriod < 2 || slowDPeriod < 2) return;

    // Obtenir les prix
    const std::vector<double>& highPrices = m_backtestData->getHigh();
    const std::vector<double>& lowPrices = m_backtestData->getLow();
    const std::vector<double>& closePrices = m_backtestData->getClose();

    std::vector<double> stochasticKValues;
    std::vector<double> stochasticDValues;
    TechnicalIndicators::calculateStochastic(highPrices, lowPrices, closePrices, fastKPeriod, slowKPeriod, slowDPeriod, stochasticKValues, stochasticDValues);

    // Mettre à jour le cache des indicateurs actifs
    m_activeIndicators.stochasticValues[id] = std::make_pair(std::move(stochasticKValues), std::move(stochasticDValues));
}

void ChartDataManager::calculateATR(int id, int period)
{
    // Vérifier si les données nécessaires sont disponibles
    if (!hasValidData() || period < 2) return;

    // Obtenir les prix
    const std::vector<double>& highPrices = m_backtestData->getHigh();
    const std::vector<double>& lowPrices = m_backtestData->getLow();
    const std::vector<double>& closePrices = m_backtestData->getClose();

    std::vector<double> atrValues;
    TechnicalIndicators::calculateATR(highPrices, lowPrices, closePrices, period, atrValues);

    // Mettre à jour le cache des indicateurs actifs
    m_activeIndicators.atrValues[id] = std::move(atrValues);
}

ChartDataManager::AggregationInfo ChartDataManager::getOptimalAggregationInfo(const DoubleArray& timestamps) {
    AggregationInfo result;
    result.level = AggregationLevel::Raw;
    result.startIndex = findClosestIndex(m_timestampsCache, timestamps[0]);
    result.pointCount = timestamps.len;
    result.isValid = true;

    if (timestamps.len <= MAX_DISPLAY_POINTS) return result;

    // Déterminer le niveau d'agrégation de départ en fonction de la période des données
    AggregationLevel startLevel = determineStartingAggregationLevel(timestamps);

    // Liste ordonnée des niveaux d'agrégation disponibles
    std::vector<AggregationLevel> aggregationLevels = {
        AggregationLevel::Raw,
        AggregationLevel::OneMinute,
        AggregationLevel::OneHour,
        AggregationLevel::OneDay,
    };

    // Trouver l'indice du niveau de départ dans notre liste
    size_t startIdx = 0;
    for (size_t i = 0; i < aggregationLevels.size(); i++) {
        if (aggregationLevels[i] == startLevel) {
            startIdx = i;
            break;
        }
    }

    // Parcourir les niveaux d'agrégation à partir du niveau de départ
    for (size_t i = startIdx + 1; i < aggregationLevels.size(); i++) {
        AggregationLevel level = aggregationLevels[i];

        auto it = m_aggregationCache.find(level);
        if (it == m_aggregationCache.end() || !it->second.isValid) {
            aggregateData(level);
        }

        const AggregatedOHLCV& aggregatedData = m_aggregationCache[level];
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
        if (visibleAggPoints > 0 && visibleAggPoints <= MAX_DISPLAY_POINTS) {
            result.level = level;
            result.startIndex = aggStartIdx;
            result.pointCount = visibleAggPoints;
            return result;
        }
    }

    // Si aucun niveau ne convient, utiliser le plus élevé disponible
    AggregationLevel highestLevel = AggregationLevel::OneDay;
    auto it = m_aggregationCache.find(highestLevel);
    if (it != m_aggregationCache.end() && it->second.isValid) {
        result.level = highestLevel;
        result.startIndex = 0;
        result.pointCount = it->second.timestamps.size();
    }

    return result;
}

ChartDataManager::AggregationLevel ChartDataManager::determineStartingAggregationLevel(const DoubleArray& timestamps) const {
    // Si moins de deux timestamps, impossible de déterminer la période
    if (timestamps.len < 2) {
        return AggregationLevel::Raw;
    }

    // La période est constante : prendre la différence entre les deux premiers points
    double period = timestamps[1] - timestamps[0];

    // Déterminer le niveau de départ en fonction de la période
    if (period >= 86400) {   // Plus d'un jour
        return AggregationLevel::OneDay;
    } else if (period >= 3600) {    // Plus d'une heure
        return AggregationLevel::OneHour;
    } else if (period >= 60) {      // Plus d'une minute
        return AggregationLevel::OneMinute;
    } else {
        return AggregationLevel::Raw;  // Moins d'une minute
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

bool ChartDataManager::hasValidData() const {
    if (!m_backtestData)
        return false;

    const auto& open = m_backtestData->getOpen();
    const auto& high = m_backtestData->getHigh();
    const auto& low = m_backtestData->getLow();
    const auto& close = m_backtestData->getClose();

    size_t size = open.size();
    return size > 0 &&
           high.size() == size &&
           low.size() == size &&
           close.size() == size &&
           m_timestampsCache.size() == size;
}

std::string ChartDataManager::aggregationLevelToString(AggregationLevel level) {
    switch (level) {
        case AggregationLevel::Raw:
            return "Raw";
        case AggregationLevel::OneMinute:
            return "1 Min";
        case AggregationLevel::OneHour:
            return "1 Hour";
        case AggregationLevel::OneDay:
            return "1 Day";
        default:
            return "Unknown";
    }
}

std::string ChartDataManager::chartTypeToString(ChartType type) {
    for (const auto& info : s_chartTypeData)
        if (info.type == type)
            return QString(info.name).toStdString();
    return QString("Unknown").toStdString();
}

ChartDataManager::ChartType ChartDataManager::stringToChartType(const std::string& typeStr){
    for (const auto& info : s_chartTypeData)
        if (typeStr == info.name)
            return info.type;
    return ChartDataManager::ChartType::CandleStick; // Valeur par défaut
}

DoubleArray ChartDataManager::vectorToDoubleArray(const std::vector<double>& vec) {
    if (vec.empty())
        return DoubleArray(nullptr, 0);
    return DoubleArray(vec.data(), static_cast<int>(vec.size()));
}


const ChartDataManager::AggregatedOHLCV& ChartDataManager::getAggregatedData(AggregationLevel level) const {
    static AggregatedOHLCV emptyData;
    auto it = m_aggregationCache.find(level);
    if (it != m_aggregationCache.end()) {
        return it->second;
    }
    return emptyData; // Retourner une référence à une structure vide si non trouvée
}