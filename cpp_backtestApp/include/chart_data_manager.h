#pragma once

#include <vector>
#include <memory>
#include <unordered_map>
#include <map>
#include <algorithm>
#include <cmath>

#include "data.hpp"
#include "chartdir.h"


struct IndicatorCache {
    std::map<int, std::vector<double>> rsi;  // Clé: période, Valeur: données RSI
    std::map<int, std::vector<double>> ema;  // Clé: période, Valeur: données EMA
    std::map<std::tuple<int,int,int>, std::pair<std::vector<double>, std::vector<double>>> stochastic;
    std::map<int, std::vector<double>> atr;  // Clé: période, Valeur: données ATR
    bool isValid = false;
};

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
        // ajouter 10 minutes mais pas si evident
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
    void setEquityCurve(const std::vector<double>& equityCurve);
    void updateHeikinAshiCache();
    AggregationInfo getOptimalAggregationInfo(const DoubleArray& timestamps);
    
    // Accesseurs
    const std::vector<double>& getTimestamps() const { return m_timestampsCache; }
    const AggregatedOHLCV& getAggregatedData(AggregationLevel level) const;
    const HeikinAshiCache& getHeikinAshiCache() const { return m_heikinAshiCache; }
    const EquityData& getEquityData() const { return m_equityData; }
    std::shared_ptr<const be::Data> getBacktestData() const { return m_backtestData; }
    bool hasValidData() const;

    // methode utilitaires peut etre a deplacer
    static std::string aggregationLevelToString(AggregationLevel level);
    static std::string chartTypeToString(ChartType type);
    static ChartType stringToChartType(const std::string& typeStr);
    static DoubleArray vectorToDoubleArray(const std::vector<double>& vec);

    
private:

    struct ChartTypeInfo {
        ChartDataManager::ChartType type;
        const char* name;
    };

    void prepareTimestampsCache();
    void aggregateData(AggregationLevel level);


    
    // Utilitaires internes
    double dateToChartTimestamp(const be::Date& date) const;
    size_t findClosestIndex(const std::vector<double>& values, double target, bool searchForward = false) const;
    AggregationLevel determineStartingAggregationLevel(const DoubleArray& timestamps) const;
    
    // Données
    std::shared_ptr<const be::Data> m_backtestData;
    std::vector<double> m_timestampsCache;
    std::unordered_map<AggregationLevel, AggregatedOHLCV> m_aggregationCache;
    HeikinAshiCache m_heikinAshiCache;
    EquityData m_equityData;
    
    // Constantes
    const int MAX_DISPLAY_POINTS = 10000;


    static const std::array<ChartTypeInfo, static_cast<size_t>(ChartDataManager::ChartType::Count)> s_chartTypeData;

};