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

// je sais aps trop mais a voir avec claude
// template<> inline IndicatorType RSIInstance::getStaticType() { return IndicatorType::RSI; }
// template<> inline IndicatorType EMAInstance::getStaticType() { return IndicatorType::EMA; }
// // etc.


struct IndicatorCache {
    std::map<int, std::vector<double>> rsi;  // Clé: période, Valeur: données RSI
    std::map<int, std::vector<double>> ema;  // Clé: période, Valeur: données EMA
    std::map<int, std::vector<double>> supertrend; // Clé: période, Valeur: données SuperTrend
    std::map<std::tuple<int,int,int>, std::pair<std::vector<double>, std::vector<double>>> stochastic;
    std::map<int, std::vector<double>> atr;  // Clé: période, Valeur: données ATR
    bool isValid = false;
};

// ces declaration devrait peut etre etre dans un fichier de declaration
enum class IndicatorType {
    RSI,
    EMA,
    STOCHASTIC,
    ATR,
    SUPERTREND,
    // autres types futurs
};

struct IndicatorBase {
    IndicatorBase(IndicatorType type) : type_(type) {}
    int id = -1; // Identifiant unique de l'indicateur
    IndicatorType type_; // Type d'indicateur
    bool visible = true; // Si l'indicateur est visible

    virtual bool needsRecalculation(const IndicatorBase& other) const = 0;

    bool operator==(const IndicatorBase& other) const {
        return id == other.id && type_ == other.type_;
    }

    bool operator!=(const IndicatorBase& other) const {
        return !(*this == other);
    }
};

// Structures pour les indicateurs techniques (importées depuis ChartWidget)
struct RSIInstance : public IndicatorBase {
    RSIInstance() : IndicatorBase(IndicatorType::RSI) {}
    int period;             // Période du RSI
    int height = 120;       // Hauteur du panneau
    int color = 0x800080;   // Couleur de la ligne principale (violet par défaut)
    double range = 20;      // Plage pour les niveaux de survente/surachat (70/30)
    int upperColor = 0xff6666; // Couleur pour la zone de surachat
    int lowerColor = 0x6666ff; // Couleur pour la zone de survente

    bool needsRecalculation(const IndicatorBase& other) const override {
        const RSIInstance* otherRSI = dynamic_cast<const RSIInstance*>(&other);
        if (!otherRSI) return true;
        return period != otherRSI->period;
    }
};

struct EMAInstance : public IndicatorBase {
    EMAInstance() : IndicatorBase(IndicatorType::EMA) {}
    int period;            // Période de l'EMA
    int color = 0x0000FF;  // Couleur de la ligne (bleu par défaut)

    bool needsRecalculation(const IndicatorBase& other) const override {
        const EMAInstance* otherEMA = dynamic_cast<const EMAInstance*>(&other);
        if (!otherEMA) return true;
        return period != otherEMA->period;
    }
};

struct SuperTrendInstance : public IndicatorBase {
    SuperTrendInstance() : IndicatorBase(IndicatorType::SUPERTREND) {}
    int period;            // Période pour le SuperTrend
    double multiplier;     // Multiplicateur pour le SuperTrend
    int upColor = 0x00AA00;  // Couleur de la ligne (vert par défaut)
    int downColor = 0xFF0000; // Couleur de la ligne (rouge par défaut)

    bool needsRecalculation(const IndicatorBase& other) const override {
        const SuperTrendInstance* otherST = dynamic_cast<const SuperTrendInstance*>(&other);
        if (!otherST) return true;
        return period != otherST->period || multiplier != otherST->multiplier;
    }
};

struct StochasticInstance : public IndicatorBase {
    StochasticInstance() : IndicatorBase(IndicatorType::STOCHASTIC) {}
    int fastKPeriod;        // Période pour calculer le %K brut
    int slowKPeriod;        // Période de lissage pour %K
    int slowDPeriod;        // Période pour calculer %D
    int height = 120;       // Hauteur du panneau
    int kColor = 0x0000FF;  // Couleur de la ligne %K (bleu par défaut)
    int dColor = 0xFF0000;  // Couleur de la ligne %D (rouge par défaut)
    int overboughtLevel = 80; // Niveau de surachat
    int oversoldLevel = 20;   // Niveau de survente

    bool needsRecalculation(const IndicatorBase& other) const override {
        const StochasticInstance* otherStochastic = dynamic_cast<const StochasticInstance*>(&other);
        if (!otherStochastic) return true;
        return fastKPeriod != otherStochastic->fastKPeriod ||
               slowKPeriod != otherStochastic->slowKPeriod ||
               slowDPeriod != otherStochastic->slowDPeriod;
    }
};

struct ATRInstance : public IndicatorBase {
    ATRInstance() : IndicatorBase(IndicatorType::ATR) {}
    int period;            // Période de l'ATR
    int height = 120;      // Hauteur du panneau
    int color = 0x006400;  // Couleur de la ligne (vert foncé par défaut)
    bool useLogScale = false; // Indique si l'échelle logarithmique est utilisée

    bool needsRecalculation(const IndicatorBase& other) const override {
        const ATRInstance* otherATR = dynamic_cast<const ATRInstance*>(&other);
        if (!otherATR) return true;
        return period != otherATR->period || useLogScale != otherATR->useLogScale;
    }
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
    
    // Structure pour stocker les indicateurs actifs
    struct ActiveIndicators {
        std::map<int, std::vector<double>> rsiValues;
        std::map<int, std::vector<double>> emaValues;
        std::map<int, std::pair<std::vector<double>, std::vector<int>>> supertrendValues; // Valeurs + directions
        std::map<int, std::pair<std::vector<double>, std::vector<double>>> stochasticValues;
        std::map<int, std::vector<double>> atrValues;
        
        void clear() {
            rsiValues.clear();
            emaValues.clear();
            supertrendValues.clear();
            stochasticValues.clear();
            atrValues.clear();
        }
    };

    struct AggregatedIndicators {
        // Pour chaque type d'indicateur, stocker les IDs qui ont été agrégés
        std::set<int> validRsiIds;
        std::set<int> validEmaIds;
        std::set<int> validSupertrendIds;
        std::set<int> validStochasticIds;
        std::set<int> validAtrIds;

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
    void calculateIndicator(const IndicatorBase& config);
    
    // Accesseurs
    const std::vector<double>& getTimestamps() const { return m_timestampsCache; }
    const HeikinAshiCache& getHeikinAshiCache() const { return m_heikinAshiCache; }
    const EquityData& getEquityData() const { return m_equityData; }
    std::shared_ptr<const be::Data> getBacktestData() const { return m_backtestData; }
    const AggregatedOHLCV& getAggregatedData(AggregationLevel level) const;
    const std::vector<std::shared_ptr<be::Trade>>& getTrades() const { return m_trades; }
    const ActiveIndicators& getActiveIndicators() const { return m_activeIndicators; }
    const AggregatedIndicators& getAggregatedIndicators(AggregationLevel level) const;
    bool hasValidData() const;

    // methode utilitaires peut etre a deplacer
    static std::string aggregationLevelToString(AggregationLevel level);
    static std::string chartTypeToString(ChartType type);
    static ChartType stringToChartType(const std::string& typeStr);
    static DoubleArray vectorToDoubleArray(const std::vector<double>& vec);

    int getMaxDisplayPoints() const { return m_maxDisplayPoints; }
    void setMaxDisplayPoints(int value);

    
    void removeAllIndicators();


    // nouvelle API : 
    template<typename T, typename = std::enable_if_t<std::is_base_of_v<IndicatorBase, T>>>
    int addIndicator(const T &config) {
        std::unique_ptr<IndicatorBase> indicator = std::make_unique<T>(config);
        indicator->id = m_nextIndicatorId++;

        int id = indicator->id;
        m_indicators.push_back(std::move(indicator));

        calculateIndicator(*m_indicators.back());

        return id;
    }

    template<typename T, typename = std::enable_if_t<std::is_base_of_v<IndicatorBase, T>>>
    T* findIndicator(int id) {
        for (auto& indicator : m_indicators) {
            if (indicator->id == id && indicator->type_ == T().type_)
                return static_cast<T*>(indicator.get());
        }
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

    template<typename T, typename = std::enable_if_t<std::is_base_of_v<IndicatorBase, T>>>
    bool removeIndicator(int id) {
        auto it = std::find_if(m_indicators.begin(), m_indicators.end(),
                         [id](const std::unique_ptr<IndicatorBase>& item) { return item->id == id && item->type_ == T().type_; });

        if (it == m_indicators.end()) return false;

        m_indicators.erase(it);

        return true;
    }

    template<typename T, typename = std::enable_if_t<std::is_base_of_v<IndicatorBase, T>>>
    std::vector<const T*> getIndicatorsOfType() const {
        std::vector<const T*> result;
        for (const auto& indicator : m_indicators) {
            if (indicator->type_ == T().type_) {
                result.push_back(static_cast<const T*>(indicator.get()));
            }
        }
        return result;
    }
    
private:
    struct ChartTypeInfo {
        ChartDataManager::ChartType type;
        const char* name;
    };

    void prepareTimestampsCache();
    void aggregateOHLCV(AggregationLevel level);
    void aggregateIndicators(AggregationLevel level);
    std::vector<double> aggregateVector(const std::vector<double>& data, AggregationLevel level, int aggregateMethod) const;

    void calculateRSI(int id, int period);
    void calculateEMA(int id, int period);
    void calculateSupertrend(int id, int period, double multiplier);
    void calculateStochastic(int id, int fastKPeriod, int slowKPeriod, int slowDPeriod);
    void calculateATR(int id, int period, bool useLogScale = false);


    
    // template<typename T>
    // int addIndicatorImpl(const T& config, std::vector<T>& container);

    // template<typename T>
    // T* findIndicator(int id, std::vector<T>& instances);
    
    // template<typename T>
    // bool setIndicatorConfigImpl(const T& config, std::vector<T>& container);
    
    // template<typename T>
    // bool removeIndicatorImpl(int id, std::vector<T>& container);

    // Utilitaires internes
    double dateToChartTimestamp(const be::Date& date) const;
    size_t findClosestIndex(const std::vector<double>& values, double target, bool searchForward = false) const;
    AggregationLevel determineStartingAggregationLevel(const DoubleArray& timestamps) const;
    
    // Données
    std::shared_ptr<const be::Data> m_backtestData;
    std::vector<double> m_timestampsCache;
    std::unordered_map<AggregationLevel, AggregatedOHLCV> m_aggregatedOHLCVCache;
    std::unordered_map<AggregationLevel, AggregatedIndicators> m_aggregatedIndicatorsCache;
    HeikinAshiCache m_heikinAshiCache;
    std::vector<std::shared_ptr<be::Trade>> m_trades;
    EquityData m_equityData;
    ActiveIndicators m_activeIndicators;

    // ID unique global pour tous les types d'indicateurs
    int m_nextIndicatorId = 1;
    // std::vector<RSIInstance> m_rsiInstances;  ///< Instances de RSI actives
    // std::vector<EMAInstance> m_emaInstances;  ///< Instances d'EMA actives
    // std::vector<SuperTrendInstance> m_superTrendInstances; ///< Instances de SuperTrend actives
    // std::vector<StochasticInstance> m_stochasticInstances; ///< Instances de Stochastique actives
    // std::vector<ATRInstance> m_atrInstances;  ///< Instances d'ATR actives


    std::vector<std::unique_ptr<IndicatorBase>> m_indicators; ///< Toutes les instances d'indicateurs actives

    // Constantes
    int m_maxDisplayPoints = 30000;

    static const std::array<ChartTypeInfo, static_cast<size_t>(ChartDataManager::ChartType::Count)> s_chartTypeData;
};