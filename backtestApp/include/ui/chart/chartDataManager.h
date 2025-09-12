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
#include "ui/chart/indicatorInstances.h"
#include "components/Utils/IndicatorMathUtils.h"


class ChartDataManager {
public:
    ChartDataManager();
    ~ChartDataManager();

    // Méthodes pour la gestion des données
    void setData(const std::shared_ptr<const be::Data>& data, const std::vector<be::TradeData>& trades, const std::vector<double>& equityCurve);
    chart::AggregationInfo getOptimalAggregationInfo(const DoubleArray& timestamps);
    
    // Accesseurs
    const chart::AggregatedOHLCV& getAggregatedData(chart::AggregationLevel level) const { return m_aggregatedOHLCVCache[static_cast<size_t>(level)];}
    const std::vector<be::TradeData>& getTrades() const { return m_trades; }
    const chart::EquityData& getEquityData() const { return m_equityData; }
    const chart::OHLC& getHeikinAshiCache() const { return m_heikinAshiCache; }
    const std::vector<double>& getTimestamps() const { return m_aggregatedOHLCVCache[static_cast<size_t>(chart::AggregationLevel::Raw)].timestamps; }
    const std::vector<std::unique_ptr<IndicatorBase>>& getIndicators() const { return m_indicators; }
    const chart::IndicatorData& getAggregatedIndicators(chart::AggregationLevel level) const { return m_aggregatedIndicatorsCache[static_cast<size_t>(level)]; }
    const std::map<int, std::vector<chart::pivotpoints::PivotPeriod>>& getPivotPeriods() const { return m_pivotPeriods; }
    std::optional<std::pair<size_t, size_t>> getTradeAggregatedIndices(size_t tradeIndex, chart::AggregationLevel level) const;
    bool hasRawData() const { return m_aggregatedOHLCVCache[static_cast<size_t>(chart::AggregationLevel::Raw)].isValid; }

    static DoubleArray vectorToDoubleArray(const std::vector<double>& vec);

    int getMaxDisplayPoints() const { return m_maxDisplayPoints; }
    void setMaxDisplayPoints(int value);

    /**
     * @brief Convertit un indice brut en indice agrégé
     * @param level Niveau d'agrégation
     * @param rawIndex Indice dans les données brutes
     * @return Indice correspondant dans les données agrégées, ou -1 si non trouvé
     */
    std::optional<size_t> rawToAggregatedIndex(chart::AggregationLevel level, size_t rawIndex) const;
    
    /**
     * @brief Obtient tous les indices bruts pour un indice agrégé donné
     * @param level Niveau d'agrégation
     * @param aggregatedIndex Indice dans les données agrégées
     * @return Vecteur des indices bruts correspondants
     */
    const std::vector<size_t>& getAggregatedToRawIndices(chart::AggregationLevel level, size_t aggregatedIndex) const;
    
    /**
     * @brief Obtient le premier indice brut pour un indice agrégé donné
     * @param level Niveau d'agrégation
     * @param aggregatedIndex Indice dans les données agrégées
     * @return Premier indice brut correspondant, ou -1 si non trouvé
     */
    int aggregatedToFirstRawIndex(chart::AggregationLevel level, int aggregatedIndex) const;

    
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
    bool configureAggregationSelector(ArrayMath& math, chart::AggregationLevel level) const;
    void aggregateOHLCV(chart::AggregationLevel level);
    void aggregateIndicators(chart::AggregationLevel level);
    std::vector<double> aggregateVector(const std::vector<double>& data, chart::AggregationLevel level, int aggregateMethod) const;
    std::vector<int> aggregateVector(const std::vector<int>& data, chart::AggregationLevel level, int aggregateMethod) const;
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
    void precalculatePivotIndices(std::vector<chart::pivotpoints::PivotPeriod>& periods, chart::AggregationLevel level);
    void precalculateTradeIndices(chart::AggregationLevel level);


    // Utilitaires internes
    double dateToChartTimestamp(const be::Date& date) const;
    size_t findClosestIndex(const std::vector<double>& values, double target, bool searchForward = false) const;
    chart::AggregationLevel determineStartingAggregationLevel(const DoubleArray& timestamps) const;
    
    // Données
    std::vector<be::Date> m_datesCache; // idem a mettre dans AggregatedOHLCV Temporaire
    std::array<chart::AggregatedOHLCV, static_cast<size_t>(chart::AggregationLevel::Count)> m_aggregatedOHLCVCache;
    std::array<chart::IndicatorData, static_cast<size_t>(chart::AggregationLevel::Count)> m_aggregatedIndicatorsCache;
    // les points pivots ne s'aggrègent pas comme les autres indicateurs, ils supportent nativement l'aggregation
    std::map<int, std::vector<chart::pivotpoints::PivotPeriod>> m_pivotPeriods;
    chart::OHLC m_heikinAshiCache;
    std::vector<be::TradeData> m_trades;
    std::vector<std::array<std::pair<size_t, size_t>, static_cast<size_t>(chart::AggregationLevel::Count)>> m_tradeIndices; // Indices pour chaque trade
    chart::EquityData m_equityData;

    // ID unique global pour tous les types d'indicateurs
    int m_nextIndicatorId = 1;
    std::vector<std::unique_ptr<IndicatorBase>> m_indicators; ///< Toutes les instances d'indicateurs actives

    // Constantes
    int m_maxDisplayPoints = 30000;
};