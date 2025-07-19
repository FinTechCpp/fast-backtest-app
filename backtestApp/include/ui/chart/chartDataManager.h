#pragma once

#include <vector>
#include <memory>
#include <unordered_map>
#include <set>
#include <map>
#include <algorithm>
#include <cmath>

#include "data.hpp"
#include "trade.hpp"
#include "chartdir.h"
#include "ui/chart/indicatorInstances.h"
#include "components/technicalIndicators.h"


// je sais aps trop mais a voir avec claude
// template<> inline IndicatorType RSIInstance::getStaticType() { return IndicatorType::RSI; }
// template<> inline IndicatorType EMAInstance::getStaticType() { return IndicatorType::EMA; }
// etc.



class ChartDataManager {
public:
    // Enumérations
    enum class ChartType {
        CandleStick,  ///< Graphique en chandeliers japonais
        HeikinAshi,   ///< Chandeliers Heikin Ashi (moyenne)
        OHLC,         ///< Barres OHLC (Open-High-Low-Close)
        Close,        ///< Ligne de prix de clôture uniquement
        Count         ///< Nombre total de types de graphiques
    };

    enum class AggregationLevel {
        Raw,         // Données brutes
        OneMinute,   // 1 minute
        OneHour,     // 1 heure
        OneDay,      // 1 jour
    };

    // Structures de données
    struct AggregationInfo {
        AggregationLevel level;    // Le niveau d'agrégation optimal
        size_t startIndex;         // L'indice de début dans les données mises en cache
        int pointCount;            // Le nombre de points à extraire
        bool isValid = false;      // Indicateur de validité
    };

    struct AggregatedOHLCV {
        std::vector<double> timestamps;
        std::vector<double> open;
        std::vector<double> high;
        std::vector<double> low;
        std::vector<double> close;
        std::vector<double> volume;
        AggregationLevel level;
        bool isValid = false;
    };

    // on pourrait imaginer de refaire cette structure pour par exemple juste prendre le type de l'indicateur
    // par exmeple pour le rsi c'est std::vector<double> rsiValues;
    // pour les points pivots c'est std::map<int, std::vector<double>> pivotPointsValues;
    struct IndicatorData {
        // Pour chaque type d'indicateur, stocker les IDs qui ont été agrégés
        std::set<int> validRsiIds;
        std::set<int> validEmaIds;
        std::set<int> validSupertrendIds;
        std::set<int> validStochasticIds;
        std::set<int> validAtrIds;
        std::set<int> validPivotPointsIds;

        // Données des indicateurs
        std::map<int, std::vector<double>> rsiValues;
        std::map<int, std::vector<double>> emaValues;
        std::map<int, std::pair<std::vector<double>, std::vector<int>>> supertrendValues; // Valeurs + directions
        std::map<int, std::pair<std::vector<double>, std::vector<double>>> stochasticValues;
        std::map<int, std::vector<double>> atrValues;
        std::map<int, std::map<int, std::vector<PivotSegment>>> pivotPointsSegments;

        AggregationLevel level;
        
        // Méthodes utilitaires pour vérifier si un indicateur spécifique est valide
        bool isRsiValid(int id) const { return validRsiIds.find(id) != validRsiIds.end(); }
        bool isEmaValid(int id) const { return validEmaIds.find(id) != validEmaIds.end(); }
        bool isSupertrendValid(int id) const { return validSupertrendIds.find(id) != validSupertrendIds.end(); }
        bool isStochasticValid(int id) const { return validStochasticIds.find(id) != validStochasticIds.end(); }
        bool isAtrValid(int id) const { return validAtrIds.find(id) != validAtrIds.end(); }
        bool isPivotPointsValid(int id) const { return validPivotPointsIds.find(id) != validPivotPointsIds.end(); }
    };

    struct HeikinAshiCache {
        std::vector<double> open;
        std::vector<double> high;
        std::vector<double> low;
        std::vector<double> close;
        bool isValid = false;
    };

    struct EquityData {
        std::vector<double> timestamps;
        std::vector<double> equity_values;
    };

    ChartDataManager();
    ~ChartDataManager();

    // Méthodes pour la gestion des données
    void setBacktestData(const std::shared_ptr<const be::Data>& data);
    void setTrades(const std::vector<std::shared_ptr<be::Trade>>& trades);
    void setEquityCurve(const std::vector<double>& equityCurve);
    void updateHeikinAshiCache();
    AggregationInfo getOptimalAggregationInfo(const DoubleArray& timestamps);
    
    // Accesseurs
    const std::vector<double>& getTimestamps() const { return m_timestampsCache; }
    const HeikinAshiCache& getHeikinAshiCache() const { return m_heikinAshiCache; }
    const EquityData& getEquityData() const { return m_equityData; }
    std::shared_ptr<const be::Data> getBacktestData() const { return m_backtestData; }
    const AggregatedOHLCV& getAggregatedData(AggregationLevel level) const;
    const std::vector<std::shared_ptr<be::Trade>>& getTrades() const { return m_trades; }
    const std::vector<std::unique_ptr<IndicatorBase>>& getIndicators() const { return m_indicators; }
    const IndicatorData& getActiveIndicators() const { return getAggregatedIndicators(AggregationLevel::Raw); }
    const IndicatorData& getAggregatedIndicators(AggregationLevel level) const;
    bool hasValidData() const;

    // methode utilitaires peut etre a deplacer
    static std::string aggregationLevelToString(AggregationLevel level);
    static std::string chartTypeToString(ChartType type);
    static ChartType stringToChartType(const std::string& typeStr);
    static DoubleArray vectorToDoubleArray(const std::vector<double>& vec);

    int getMaxDisplayPoints() const { return m_maxDisplayPoints; }
    void setMaxDisplayPoints(int value);

    
    void removeAllIndicators();


    // Ici il faut implementer le vole de données avec move
    template<typename T, typename = std::enable_if_t<std::is_base_of_v<IndicatorBase, T>>>
    int addIndicator(T&& config) {
        config.id = m_nextIndicatorId++;
        int id = config.id;

        std::unique_ptr<IndicatorBase> indicator = std::make_unique<T>(std::move(config));

        m_indicators.push_back(std::move(indicator));

        calculateIndicator(*m_indicators.back());

        return id;
    }

    template<typename T, typename = std::enable_if_t<std::is_base_of_v<IndicatorBase, T>>>
    T* findIndicator(int id) const {
        for (auto& indicator : m_indicators)
            if (indicator->id == id && indicator->type_ == T().type_)
                return static_cast<T*>(indicator.get());
        return nullptr;
    }

    template<typename T, typename = std::enable_if_t<std::is_base_of_v<IndicatorBase, T>>>
    bool updateIndicator(const T& config) {
        T* indicator = findIndicator<T>(config.id);

        if (!indicator) return false;

        bool needsRecalculation = indicator->needsRecalculation(config);

        *indicator = config; // Met à jour la configuration de l'indicateur

        if (needsRecalculation)
            calculateIndicator(config);

        return true;
    }

    bool removeIndicator(int id) {
        auto it = std::find_if(m_indicators.begin(), m_indicators.end(), [id](const std::unique_ptr<IndicatorBase>& item) {  
            return item->id == id; 
        });

        if (it == m_indicators.end()) 
            return false;

        m_indicators.erase(it);

        return true;
    }

    template<typename T, typename = std::enable_if_t<std::is_base_of_v<IndicatorBase, T>>>
    std::vector<const T*> getIndicatorsOfType() const {
        std::vector<const T*> result;
        for (const auto& indicator : m_indicators)
            if (indicator->type_ == T().type_)
                result.push_back(static_cast<const T*>(indicator.get()));
        return result;
    }
    
private:
    struct ChartTypeInfo {
        ChartDataManager::ChartType type;
        const char* name;
    };

    void prepareTimestampsCache();
    bool configureAggregationSelector(ArrayMath& math, AggregationLevel level) const;
    void aggregateOHLCV(AggregationLevel level);
    void aggregateIndicators(AggregationLevel level);
    std::vector<double> aggregateVector(const std::vector<double>& data, AggregationLevel level, int aggregateMethod) const;
    std::vector<int> aggregateVector(const std::vector<int>& data, AggregationLevel level, int aggregateMethod) const;

    void calculateIndicator(const IndicatorBase& config);
    void calculateRSI(int id, int period);
    void calculateEMA(int id, int period);
    void calculateSupertrend(int id, int period, double multiplier);
    void calculateStochastic(int id, int fastKPeriod, int slowKPeriod, int slowDPeriod);
    void calculateATR(int id, int period, bool useLogScale = false);
    // On a peux etre pas besoin de donner l'instance complete mais pk pas, mais si on fait ca on, le fait pour tous les indicateurs
    void calculatePivotPoints(int id, const PivotPointsInstance& config);

    // Utilitaires internes
    double dateToChartTimestamp(const be::Date& date) const;
    size_t findClosestIndex(const std::vector<double>& values, double target, bool searchForward = false) const;
    AggregationLevel determineStartingAggregationLevel(const DoubleArray& timestamps) const;
    
    // Données
    std::shared_ptr<const be::Data> m_backtestData;
    std::vector<double> m_timestampsCache;
    std::unordered_map<AggregationLevel, AggregatedOHLCV> m_aggregatedOHLCVCache;
    std::unordered_map<AggregationLevel, IndicatorData> m_aggregatedIndicatorsCache;
    HeikinAshiCache m_heikinAshiCache;
    std::vector<std::shared_ptr<be::Trade>> m_trades;
    EquityData m_equityData;

    // ID unique global pour tous les types d'indicateurs
    int m_nextIndicatorId = 1;
    std::vector<std::unique_ptr<IndicatorBase>> m_indicators; ///< Toutes les instances d'indicateurs actives

    // Constantes
    int m_maxDisplayPoints = 30000;

    static const std::array<ChartTypeInfo, static_cast<size_t>(ChartDataManager::ChartType::Count)> s_chartTypeData;
};