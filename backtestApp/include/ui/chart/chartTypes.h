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

    namespace pivotpoints {
        enum class PeriodType {
            FourHour,   // Points pivots toutes les 4 heures
            Daily,      // Points pivots quotidiens
            Weekly,     // Points pivots hebdomadaires
            Monthly,    // Points pivots mensuels
        };

        enum class CalculationMethod {
            HLC,     // High, Low, Close (méthode standard)
            OHLC,    // Open, High, Low, Close
            HL0       // High, Low, Open
        };

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
            std::array<double, static_cast<size_t>(chart::pivotpoints::LevelType::Count)> levelValues;
        };
    }
}