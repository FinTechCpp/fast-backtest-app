#pragma once

#include <cstddef>   // pour size_t
#include <vector>
#include <set>
#include <map>
#include <array>
#include <string>
#include <QString>
#include <utility>  // pour std::pair

namespace chart {
    enum class ChartType {
        CandleStick,  ///< Graphique en chandeliers japonais
        HeikinAshi,   ///< Chandeliers Heikin Ashi (moyenne)
        OHLC,         ///< Barres OHLC (Open-High-Low-Close)
        Close,        ///< Ligne de prix de clôture uniquement
        Count         ///< Nombre total de types de graphiques
    };
    extern const std::array<std::pair<ChartType, const char*>, static_cast<size_t>(ChartType::Count)> chartTypeNames;
    std::string chartTypeToString(ChartType type);
    ChartType stringToChartType(const std::string& typeStr);

    enum class AggregationLevel {
        Raw,         // Données brutes
        OneMinute,   // 1 minute
        OneHour,     // 1 heure
        OneDay,      // 1 jour
        Count        // Nombre total de niveaux d'agrégation
    };
    extern const std::array<const char*, static_cast<size_t>(AggregationLevel::Count)> aggregationLevelNames;
    std::string aggregationLevelToString(AggregationLevel level);

    // Structure pour la configuration du graphique
    struct ChartConfiguration {
        ChartType chartType = ChartType::CandleStick;
        int chartWidth = 1200; // il faut que ce soit adaptatif
        int chartHeight = 1000;
        int equityHeight = 180; // inclue dans la taille du graphique principal
        // int volumeHeight = 100;
        bool showTrades = true;
        bool showEquity = true;

        // Propriétés pour le mode d'échelle Y fixe
        bool fixedYScale = false;     // Indique si on utilise une échelle Y fixe
        double yScaleMin = 0.0;       // Valeur minimale de l'échelle Y
        double yScaleMax = 0.0;       // Valeur maximale de l'échelle Y
        double yScaleOffset = 0.0;    // Décalage vertical en unités de l'échelle
    };

    struct AggregationInfo {
        AggregationLevel level;    // Le niveau d'agrégation optimal
        size_t startIndex;         // L'indice de début dans les données mises en cache
        size_t pointCount;         // Le nombre de points à extraire
        bool isValid = false;      // Indicateur de validité
    };
    inline bool operator==(const AggregationInfo& a, const AggregationInfo& b) {
        return a.level == b.level && a.startIndex == b.startIndex && a.pointCount == b.pointCount && a.isValid == b.isValid;
    }
    inline bool operator!=(const AggregationInfo& a, const AggregationInfo& b) {
        return !(a == b);
    }

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
}



namespace indicators {
    enum class Type {
        RSI,
        EMA,
        STOCHASTIC,
        ATR, 
        SUPERTREND,
        PIVOTPOINTS
    };

    // Il faudrait faire des structure pour contenir uniquement les info techenique qui vont servir a CALCULER l'indicateur
    // comme ca scela allege les methodes de calcule ET on utilise cela pour communiquer entre la strategy et le front plutot que :
    /*
    struct StrategyIndicator {
        enum Type {
            RSI,
            EMA,
            STOCHASTIC,
            ATR,
            SUPERTREND
        };
        
        Type type;
        std::map<std::string, double> params;  // Indicator parameters
    };
    */

    enum class PivotPeriodType {
        FourHour,   // Points pivots toutes les 4 heures
        Daily,      // Points pivots quotidiens
        Weekly,     // Points pivots hebdomadaires
        Monthly     // Points pivots mensuels
    };

    enum class PivotCalculationMethod {
        HLC,     // High, Low, Close (méthode standard)
        OHLC,    // Open, High, Low, Close
        HLO      // High, Low, Open
    };

    namespace params {
        struct RSI {
            int period = 14;

            bool operator==(const RSI& other) const = default;
            bool operator!=(const RSI& other) const = default;
        };
        
        struct EMA {
            int period = 20;

            bool operator==(const EMA& other) const = default;
            bool operator!=(const EMA& other) const = default;
        };
        
        struct Stochastic {
            int fastKPeriod = 14;
            int slowKPeriod = 3;
            int slowDPeriod = 3;

            bool operator==(const Stochastic& other) const = default;
            bool operator!=(const Stochastic& other) const = default;
        };
        
        struct ATR {
            int period = 14;
            bool useLogScale = false;

            bool operator==(const ATR& other) const = default;
            bool operator!=(const ATR& other) const = default;
        };
        
        struct SuperTrend {
            int period = 10;
            double multiplier = 3.0;

            bool operator==(const SuperTrend& other) const = default;
            bool operator!=(const SuperTrend& other) const = default;
        };
        
        struct PivotPoints {
            PivotPeriodType periodType = PivotPeriodType::Daily;
            PivotCalculationMethod calculationMethod = PivotCalculationMethod::HLC;

            bool operator==(const PivotPoints& other) const = default;
            bool operator!=(const PivotPoints& other) const = default;
        };
    }

    struct IndicatorSignal {        
        Type type;
        
        // Unions de tous les types possibles de paramètres
        union ParamsUnion {
            params::RSI rsi;
            params::EMA ema;
            params::Stochastic stochastic;
            params::ATR atr;
            params::SuperTrend supertrend;
            params::PivotPoints pivotpoints;
            
            ParamsUnion() {} // Union nécessite un constructeur par défaut
            ~ParamsUnion() {} // Et un destructeur
        } params;
    };

    struct IndicatorBase {
        // IndicatorBase(Type type) : type_(type) {}
        IndicatorBase() = default;
        int id = -1; // Identifiant unique de l'indicateur
        // Type type_; // Type d'indicateur
        bool visible = true; // Si l'indicateur est visible

        virtual bool needsRecalculation(const IndicatorBase& other) const = 0;
        
        // Nouvelle méthode pour obtenir le nom d'affichage de l'indicateur
        virtual QString getDisplayName() const = 0;

        virtual void setDefaults() = 0;

        virtual ~IndicatorBase() = default;

        // bool operator==(const IndicatorBase& other) const {
        //     return id == other.id && type_ == other.type_;
        // }

        // bool operator!=(const IndicatorBase& other) const {
        //     return !(*this == other);
        // }
    };

    template <typename T>
    struct IndicatorCRTP : public IndicatorBase {
        bool needsRecalculation(const IndicatorBase& other) const override {
            const T* otherIndicator = static_cast<const T*>(&other);
            return *static_cast<const T*>(this) != *otherIndicator;
        }
    };

    struct RSIInstance : public IndicatorBase {
        RSIInstance() : IndicatorBase() {
            setDefaults();
        }

        RSIInstance(params::RSI p) : IndicatorBase() {
            setDefaults();
            period = p.period;
        }

        // static constexpr Type staticType = Type::RSI;

        int period;             // Période du RSI
        int height;             // Hauteur du panneau
        int color;              // Couleur de la ligne principale (violet par défaut)
        int overboughtLevel;    // Niveau de surachat
        int oversoldLevel;      // Niveau de survente
        int upperColor;         // Couleur pour la zone de surachat
        int lowerColor;         // Couleur pour la zone de survente

        bool needsRecalculation(const IndicatorBase& other) const override {
            const RSIInstance* otherRSI = dynamic_cast<const RSIInstance*>(&other);
            if (!otherRSI) return true;
            return period != otherRSI->period;
        }
        
        QString getDisplayName() const override {
            return QString("RSI (%1)").arg(period);
        }

        void setDefaults() override {
            period = 14;
            height = 120;
            color = 0x800080; // Violet
            overboughtLevel = 80;
            oversoldLevel = 20;
            upperColor = 0xff6666; // Rouge clair
            lowerColor = 0x6666ff; // Bleu clair
        }
    };

    struct EMAInstance : public IndicatorBase {
        EMAInstance() : IndicatorBase() {
            setDefaults();
        }
        int period;            // Période de l'EMA
        int color;             // Couleur de la ligne (bleu par défaut)

        bool needsRecalculation(const IndicatorBase& other) const override {
            const EMAInstance* otherEMA = dynamic_cast<const EMAInstance*>(&other);
            if (!otherEMA) return true;
            return period != otherEMA->period;
        }
        
        QString getDisplayName() const override {
            return QString("EMA (%1)").arg(period);
        }

        void setDefaults() override {
            period = 20;
            color = 0x0000FF; // Bleu par défaut
        }
    };

    struct SuperTrendInstance : public IndicatorBase {
        SuperTrendInstance() : IndicatorBase() {
            setDefaults();
        }
        int period;            // Période pour le SuperTrend
        double multiplier;     // Multiplicateur pour le SuperTrend
        int upColor;           // Couleur de la ligne (vert par défaut)
        int downColor;         // Couleur de la ligne (rouge par défaut)

        bool needsRecalculation(const IndicatorBase& other) const override {
            const SuperTrendInstance* otherST = dynamic_cast<const SuperTrendInstance*>(&other);
            if (!otherST) return true;
            return period != otherST->period || multiplier != otherST->multiplier;
        }
        
        QString getDisplayName() const override {
            return QString("Supertrend (%1, %2)").arg(period).arg(multiplier, 0, 'f', 1);
        }

        void setDefaults() override {
            period = 10;
            multiplier = 3.0;
            upColor = 0xFFA500; // Orange doré
            downColor = 0x8B008B; // Rose extrêmememnt foncé
        }
    };

    struct StochasticInstance : public IndicatorBase {
        StochasticInstance() : IndicatorBase() {
            setDefaults();
        }
        int fastKPeriod;        // Période pour calculer le %K brut
        int slowKPeriod;        // Période de lissage pour %K
        int slowDPeriod;        // Période pour calculer %D
        int height;             // Hauteur du panneau
        int kColor;             // Couleur de la ligne %K (bleu par défaut)
        int dColor;             // Couleur de la ligne %D (rouge par défaut)
        int overboughtLevel;    // Niveau de surachat
        int oversoldLevel;      // Niveau de survente

        bool needsRecalculation(const IndicatorBase& other) const override {
            const StochasticInstance* otherStochastic = dynamic_cast<const StochasticInstance*>(&other);
            if (!otherStochastic) return true;
            return fastKPeriod != otherStochastic->fastKPeriod ||
                slowKPeriod != otherStochastic->slowKPeriod ||
                slowDPeriod != otherStochastic->slowDPeriod;
        }
        
        QString getDisplayName() const override {
            return QString("Stochastic (%1,%2,%3)").arg(fastKPeriod).arg(slowKPeriod).arg(slowDPeriod);
        }

        void setDefaults() override {
            fastKPeriod = 14;
            slowKPeriod = 3;
            slowDPeriod = 3;
            height = 120;
            kColor = 0x0000FF;
            dColor = 0xFF0000;
            overboughtLevel = 80;
            oversoldLevel = 20;
        }
    };

    struct ATRInstance : public IndicatorBase {
        ATRInstance() : IndicatorBase() {
            setDefaults();
        }
        int period;            // Période de l'ATR
        int height;            // Hauteur du panneau
        int color;             // Couleur de la ligne (vert foncé par défaut)
        bool useLogScale;      // Indique si l'échelle logarithmique est utilisée

        bool needsRecalculation(const IndicatorBase& other) const override {
            const ATRInstance* otherATR = dynamic_cast<const ATRInstance*>(&other);
            if (!otherATR) return true;
            return period != otherATR->period || useLogScale != otherATR->useLogScale;
        }
        
        QString getDisplayName() const override {
            return QString("ATR (%1)").arg(period);
        }

        void setDefaults() override {
            period = 14;
            height = 120;
            color = 0x006400;
            useLogScale = false;
        }
    };

    struct PivotPointsInstance : public IndicatorBase {
        enum class LevelType {
            R3,         // Résistance 3
            R2,         // Résistance 2
            R1,         // Résistance 1
            Pivot,      // Point pivot principal (PP)
            S1,         // Support 1
            S2,         // Support 2
            S3,         // Support 3
            M_R2R3,     // Milieu entre R2 et R3
            M_R1R2,     // Milieu entre R1 et R2
            M_PR1,      // Milieu entre PP et R1
            M_PS1,      // Milieu entre PP et S1
            M_S1S2,     // Milieu entre S1 et S2
            M_S2S3,     // Milieu entre S2 et S3

            Count   // Nombre total de niveaux
        };

        enum class LineStyle {
            Solid,
            Dash,
            Dot,
            DotDash,
            AltDash
        };

        struct LevelStyle {
            int color = 0x000000;     // Couleur de la ligne
            int thickness = 2;    // Épaisseur (1-3)
            LineStyle lineStyle = LineStyle::Solid; // Style (solid, dash, dot, etc.)
            bool visible = false;     // Visibilité du niveau

            QString labelFormat = QString(); // Format d'affichage optionnel (ex: "PP: %.2f")
        };

        struct PivotPeriod {
            // Mapping vers les indices agrégés pour différents niveaux d'agrégation
            std::array<std::pair<size_t, size_t>, static_cast<size_t>(chart::AggregationLevel::Count)> indices; // Indices bruts pour chaque niveau d'agrégation
            
            // Valeurs de tous les niveaux de pivot pour cette période
            std::array<double, static_cast<size_t>(LevelType::Count)> levelValues;
        };

        PivotPointsInstance() : IndicatorBase() {
            setDefaults();
        }

        PivotPeriodType periodType;                          // Type de période (4H, journalier, hebdomadaire, mensuel)
        PivotCalculationMethod calculationMethod;            // Méthode de calcul des points pivots
        std::array<LevelStyle, static_cast<size_t>(LevelType::Count)> levelStyles; // Styles par défaut pour chaque niveau
        bool showLabels = true;                         // Afficher les étiquettes des niveaux


        bool needsRecalculation(const IndicatorBase& other) const override {
            const PivotPointsInstance* otherPP = dynamic_cast<const PivotPointsInstance*>(&other);
            if (!otherPP) return true;
            return periodType != otherPP->periodType || calculationMethod != otherPP->calculationMethod;
        }
        
        QString getDisplayName() const override {
            QString periodStr;
            switch (periodType) {
                case PivotPeriodType::FourHour: periodStr = "4H"; break;
                case PivotPeriodType::Daily: periodStr = "Jour"; break;
                case PivotPeriodType::Weekly: periodStr = "Hebdomadaire"; break;
                case PivotPeriodType::Monthly: periodStr = "Mensuel"; break;
            }
            return QString("Pivot Points (%1)").arg(periodStr);
        }

        bool isLevelVisible(LevelType level) const {
            return levelStyles[static_cast<size_t>(level)].visible;
        }

        void setDefaults() override {
            // Point pivot central (noir, trait plein, visible)
            LevelStyle pivotStyle;
            pivotStyle.color = 0x000000;  // Noir
            pivotStyle.thickness = 2;
            pivotStyle.lineStyle = LineStyle::Solid;
            pivotStyle.visible = true;
            pivotStyle.labelFormat = "Piv %1";
            levelStyles[static_cast<size_t>(LevelType::Pivot)] = pivotStyle;

            // Résistances (rouge, trait plein, visibles)
            LevelStyle resistanceStyle;
            resistanceStyle.color = 0xFF0000;  // Rouge
            resistanceStyle.thickness = 2;
            resistanceStyle.lineStyle = LineStyle::Solid;
            resistanceStyle.visible = true;
            
            resistanceStyle.labelFormat = "R1 %1";
            levelStyles[static_cast<size_t>(LevelType::R1)] = resistanceStyle;

            resistanceStyle.labelFormat = "R2 %1";
            levelStyles[static_cast<size_t>(LevelType::R2)] = resistanceStyle;

            resistanceStyle.labelFormat = "R3 %1";
            levelStyles[static_cast<size_t>(LevelType::R3)] = resistanceStyle;

            // Supports (vert, trait plein, visibles)
            LevelStyle supportStyle;
            supportStyle.color = 0x008000;  // Vert
            supportStyle.thickness = 2;
            supportStyle.lineStyle = LineStyle::Solid;
            supportStyle.visible = true;

            supportStyle.labelFormat = "S1 %1";
            levelStyles[static_cast<size_t>(LevelType::S1)] = supportStyle;

            supportStyle.labelFormat = "S2 %1";
            levelStyles[static_cast<size_t>(LevelType::S2)] = supportStyle;

            supportStyle.labelFormat = "S3 %1";
            levelStyles[static_cast<size_t>(LevelType::S3)] = supportStyle;

            // Niveaux milieux résistance (rouge, trait pointillé, non visibles par défaut)
            LevelStyle midResistanceStyle;
            midResistanceStyle.color = 0xFF0000;  // Rouge
            midResistanceStyle.thickness = 1;
            midResistanceStyle.lineStyle = LineStyle::Dash;
            midResistanceStyle.visible = false;  // Visible si showMidLevels est true
            
            midResistanceStyle.labelFormat = "mR3 %1";
            levelStyles[static_cast<size_t>(LevelType::M_R2R3)] = midResistanceStyle;

            midResistanceStyle.labelFormat = "mR2 %1";
            levelStyles[static_cast<size_t>(LevelType::M_R1R2)] = midResistanceStyle;

            midResistanceStyle.labelFormat = "mR1 %1";
            levelStyles[static_cast<size_t>(LevelType::M_PR1)] = midResistanceStyle;

            // Niveaux milieux support (vert, trait pointillé, non visibles par défaut)
            LevelStyle midSupportStyle;
            midSupportStyle.color = 0x008000;  // Vert
            midSupportStyle.thickness = 1;
            midSupportStyle.lineStyle = LineStyle::Dash;
            midSupportStyle.visible = false;  // Visible si showMidLevels est true

            midSupportStyle.labelFormat = "mS1 %1";
            levelStyles[static_cast<size_t>(LevelType::M_PS1)] = midSupportStyle;

            midSupportStyle.labelFormat = "mS2 %1";
            levelStyles[static_cast<size_t>(LevelType::M_S1S2)] = midSupportStyle;

            midSupportStyle.labelFormat = "mS3 %1";
            levelStyles[static_cast<size_t>(LevelType::M_S2S3)] = midSupportStyle;

            // Activer les étiquettes par défaut
            showLabels = true;
            
            // Type de période par défaut
            periodType = PivotPeriodType::Daily;

            calculationMethod = PivotCalculationMethod::HLC;
        }
    };
}