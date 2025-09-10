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
#include "components/Utils/IndicatorMathUtils.h"


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

    // Structures de données
    struct AggregationInfo {
        AggregationLevel level;    // Le niveau d'agrégation optimal
        size_t startIndex;         // L'indice de début dans les données mises en cache
        size_t pointCount;         // Le nombre de points à extraire
        bool isValid = false;      // Indicateur de validité
    };

    // faire de l'heritage pour stocker ohlcv isvalid
    struct OHLC {
        std::vector<double> open;
        std::vector<double> high;
        std::vector<double> low;
        std::vector<double> close;
        bool isValid = false;
    };

    struct AggregatedOHLCV : OHLC {
        std::vector<double> timestamps, volume;
        std::vector<std::vector<size_t>> rawIndicesMapping;  // Pour chaque indice agrégé, liste des indices raw correspondants
        /*
        Exemple :
        Raw data indices:    0, 1, 2, 3, 4, 5, 6, 7, 8
        Aggregated (1min):    0, 1, 2
        rawIndicesMapping:  [[0,1,2],[3,4,5],[6,7,8]]
        0 -> [0,1,2]
        1 -> [3,4,5]
        2 -> [6,7,8]
        Dans la bougie agrégée 0, on a les bougies raw 0,1,2 etc.
        */
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

        AggregationLevel level;
        
        // Méthodes utilitaires pour vérifier si un indicateur spécifique est valide
        bool isRsiValid(int id) const { return validRsiIds.find(id) != validRsiIds.end(); }
        bool isEmaValid(int id) const { return validEmaIds.find(id) != validEmaIds.end(); }
        bool isSupertrendValid(int id) const { return validSupertrendIds.find(id) != validSupertrendIds.end(); }
        bool isStochasticValid(int id) const { return validStochasticIds.find(id) != validStochasticIds.end(); }
        bool isAtrValid(int id) const { return validAtrIds.find(id) != validAtrIds.end(); }
        bool isPivotPointsValid(int id) const { return validPivotPointsIds.find(id) != validPivotPointsIds.end(); }
    };

    struct EquityData {
        std::vector<double> timestamps;
        std::vector<double> equity_values;
    };

    ChartDataManager();
    ~ChartDataManager();

    // Méthodes pour la gestion des données
    void setData(const std::shared_ptr<const be::Data>& data, const std::vector<be::TradeData>& trades, const std::vector<double>& equityCurve);
    AggregationInfo getOptimalAggregationInfo(const DoubleArray& timestamps);
    
    // Accesseurs
    const AggregatedOHLCV& getAggregatedData(AggregationLevel level) const { return m_aggregatedOHLCVCache[static_cast<size_t>(level)];}
    const std::vector<be::TradeData>& getTrades() const { return m_trades; }
    const EquityData& getEquityData() const { return m_equityData; }
    const OHLC& getHeikinAshiCache() const { return m_heikinAshiCache; }
    const std::vector<double>& getTimestamps() const { return m_aggregatedOHLCVCache[static_cast<size_t>(AggregationLevel::Raw)].timestamps; }
    const std::vector<std::unique_ptr<IndicatorBase>>& getIndicators() const { return m_indicators; }
    const IndicatorData& getAggregatedIndicators(AggregationLevel level) const { return m_aggregatedIndicatorsCache[static_cast<size_t>(level)]; }
    const std::map<int, std::vector<PivotPeriod>>& getPivotPeriods() const { return m_pivotPeriods; }
    std::optional<std::pair<size_t, size_t>> getTradeAggregatedIndices(size_t tradeIndex, AggregationLevel level) const;
    bool hasRawData() const { return m_aggregatedOHLCVCache[static_cast<size_t>(AggregationLevel::Raw)].isValid; }

    // methode utilitaires peut etre a deplacer
    static std::string aggregationLevelToString(AggregationLevel level);
    static std::string chartTypeToString(ChartType type);
    static ChartType stringToChartType(const std::string& typeStr);
    static DoubleArray vectorToDoubleArray(const std::vector<double>& vec);

    int getMaxDisplayPoints() const { return m_maxDisplayPoints; }
    void setMaxDisplayPoints(int value);

    /**
     * @brief Convertit un indice brut en indice agrégé
     * @param level Niveau d'agrégation
     * @param rawIndex Indice dans les données brutes
     * @return Indice correspondant dans les données agrégées, ou -1 si non trouvé
     */
    std::optional<size_t> rawToAggregatedIndex(AggregationLevel level, size_t rawIndex) const;
    
    /**
     * @brief Obtient tous les indices bruts pour un indice agrégé donné
     * @param level Niveau d'agrégation
     * @param aggregatedIndex Indice dans les données agrégées
     * @return Vecteur des indices bruts correspondants
     */
    const std::vector<size_t>& getAggregatedToRawIndices(AggregationLevel level, size_t aggregatedIndex) const;
    
    /**
     * @brief Obtient le premier indice brut pour un indice agrégé donné
     * @param level Niveau d'agrégation
     * @param aggregatedIndex Indice dans les données agrégées
     * @return Premier indice brut correspondant, ou -1 si non trouvé
     */
    int aggregatedToFirstRawIndex(AggregationLevel level, int aggregatedIndex) const;

    
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

    bool configureAggregationSelector(ArrayMath& math, AggregationLevel level) const;
    void aggregateOHLCV(AggregationLevel level);
    void aggregateIndicators(AggregationLevel level);
    std::vector<double> aggregateVector(const std::vector<double>& data, AggregationLevel level, int aggregateMethod) const;
    std::vector<int> aggregateVector(const std::vector<int>& data, AggregationLevel level, int aggregateMethod) const;
    void updateHeikinAshiCache();

    void calculateIndicator(const IndicatorBase& config);
    void calculateRSI(int id, int period);
    void calculateEMA(int id, int period);
    void calculateSupertrend(int id, int period, double multiplier);
    void calculateStochastic(int id, int fastKPeriod, int slowKPeriod, int slowDPeriod);
    void calculateATR(int id, int period, bool useLogScale = false);
    // On a peux etre pas besoin de donner l'instance complete mais pk pas, mais si on fait ca on, le fait pour tous les indicateurs
    void calculatePivotPoints(const PivotPointsInstance& config);

    // deux methode qui utilise la meme logique c'est a factoriser
    void precalculatePivotIndices(std::vector<PivotPeriod>& periods, AggregationLevel level);
    void precalculateTradeIndices(AggregationLevel level);


    // Utilitaires internes
    double dateToChartTimestamp(const be::Date& date) const;
    size_t findClosestIndex(const std::vector<double>& values, double target, bool searchForward = false) const;
    AggregationLevel determineStartingAggregationLevel(const DoubleArray& timestamps) const;
    
    // Données
    // std::shared_ptr<const be::Data> m_backtestData; // a supprimer c'est a mettre dans AggregatedOHLCV
    std::vector<be::Date> m_datesCache; // idem a mettre dans AggregatedOHLCV Temporaire
    std::array<AggregatedOHLCV, static_cast<size_t>(AggregationLevel::Count)> m_aggregatedOHLCVCache;
    std::array<IndicatorData, static_cast<size_t>(AggregationLevel::Count)> m_aggregatedIndicatorsCache;
    // les points pivots ne s'aggrègent pas comme les autres indicateurs, ils supportent nativement l'aggregation
    std::map<int, std::vector<PivotPeriod>> m_pivotPeriods;
    OHLC m_heikinAshiCache;
    std::vector<be::TradeData> m_trades;
    std::vector<std::array<std::pair<size_t, size_t>, static_cast<size_t>(AggregationLevel::Count)>> m_tradeIndices; // Indices pour chaque trade
    EquityData m_equityData;

    // ID unique global pour tous les types d'indicateurs
    int m_nextIndicatorId = 1;
    std::vector<std::unique_ptr<IndicatorBase>> m_indicators; ///< Toutes les instances d'indicateurs actives

    // Constantes
    int m_maxDisplayPoints = 30000;

    static const std::array<ChartTypeInfo, static_cast<size_t>(ChartDataManager::ChartType::Count)> s_chartTypeData;
};