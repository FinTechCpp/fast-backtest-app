#pragma once

#include <memory>
#include <vector>
#include <string>
#include <utility>

#include "qchartviewer.h"
#include "chartdir.h"
#include "FinanceChart.h"
#include "chart_data_manager.h"
#include "trade.hpp"

// Prédéclarations de classes
class ChartDataManager;
class ChartIndicatorManager;

// Structure pour la configuration du graphique
struct ChartConfiguration {
    ChartDataManager::ChartType chartType = ChartDataManager::ChartType::CandleStick;
    int chartWidth = 1200; // il faut que ce soit adaptatif
    int mainChartHeight = 400;
    int equityHeight = 150;
    int volumeHeight = 100;
    bool showTrades = true;
    bool showVolume = true;
    bool showEquity = true;
};

// Structure pour les segments TP/SL des trades
struct TPSLSegment {
    double startX;        // Index du point d'entrée
    double endX;          // Index du point de sortie
    double level;         // Niveau de prix (TP ou SL)
    bool isTakeProfit;    // true = TP, false = SL
    int color;            // Couleur basée sur le résultat du trade
};

// Structures pour les indicateurs techniques (importées depuis ChartWidget)
struct RSIInstance {
    int id;                 // Identifiant unique 
    int period;             // Période du RSI
    bool visible = true;    // Si l'indicateur est visible
    int height = 120;       // Hauteur du panneau
    int color = 0x800080;   // Couleur de la ligne principale (violet par défaut)
    double range = 20;      // Plage pour les niveaux de survente/surachat (70/30)
    int upperColor = 0xff6666; // Couleur pour la zone de surachat
    int lowerColor = 0x6666ff; // Couleur pour la zone de survente
    
    bool operator==(const RSIInstance& other) const {
        return id == other.id;
    }
};

struct EMAInstance {
    int id;                // Identifiant unique 
    int period;            // Période de l'EMA
    bool visible = true;   // Si l'indicateur est visible
    int color = 0x0000FF;  // Couleur de la ligne (bleu par défaut)
    
    bool operator==(const EMAInstance& other) const {
        return id == other.id;
    }
};

struct StochasticInstance {
    int id;                 // Identifiant unique 
    int fastKPeriod;        // Période pour calculer le %K brut
    int slowKPeriod;        // Période de lissage pour %K
    int slowDPeriod;        // Période pour calculer %D
    bool visible = true;    // Si l'indicateur est visible
    int height = 120;       // Hauteur du panneau
    int kColor = 0x0000FF;  // Couleur de la ligne %K (bleu par défaut)
    int dColor = 0xFF0000;  // Couleur de la ligne %D (rouge par défaut)
    int overboughtLevel = 80; // Niveau de surachat
    int oversoldLevel = 20;   // Niveau de survente
    
    bool operator==(const StochasticInstance& other) const {
        return id == other.id;
    }
};

struct ATRInstance {
    int id;                // Identifiant unique 
    int period;            // Période de l'ATR
    bool visible = true;   // Si l'indicateur est visible
    int height = 120;      // Hauteur du panneau
    int color = 0x006400;  // Couleur de la ligne (vert foncé par défaut)
    
    bool operator==(const ATRInstance& other) const {
        return id == other.id;
    }
};



class ChartRenderer {
public:
    ChartRenderer();
    ~ChartRenderer();

    // Méthode principale pour créer ou mettre à jour le graphique
    void createOrUpdateChart(
        QChartViewer* viewer,
        const ChartDataManager& dataManager,
        const ChartConfiguration& config,
        const std::vector<std::shared_ptr<be::Trade>>& trades,
        const ChartDataManager::AggregationInfo& aggregationInfo,
        const std::vector<RSIInstance>& rsiInstances,
        const std::vector<EMAInstance>& emaInstances,
        const std::vector<StochasticInstance>& stochasticInstances,
        const std::vector<ATRInstance>& atrInstances,
        const IndicatorCache& indicatorCache
    );

    void updateDynamicLayer(QChartViewer* viewer, 
                        bool rulerEnabled, 
                        bool rulerFirstPointSelected,
                        int rulerStartX, int rulerStartY, 
                        const ChartDataManager& dataManager);
    
private:
    // Méthode pour dessiner la règle
    void drawRuler(MultiChart* m, int startX, int startY, int endX, int endY, DrawArea* d);
    
    // Méthode pour le tracking de la souris
    void trackFinance(MultiChart* m, int mouseX, int mouseY, DrawArea* d);
    
    // Méthodes de rendu des sections
    void addEquityCurveSection(FinanceChart *chart, 
                               const ChartDataManager& dataManager, 
                               const DoubleArray &timestamps, 
                               int startIndex);
    
    void addTradeMarkers(FinanceChart *chart, 
                         const DoubleArray &timestamps,
                         int startIndex,
                         const std::vector<std::shared_ptr<be::Trade>>& trades,
                         const ChartDataManager& dataManager,
                         const ChartDataManager::AggregationInfo& aggregationInfo);
    
    // Méthodes pour l'ajout d'indicateurs
    void addRSIToChart(FinanceChart* chart, 
                      const RSIInstance& rsi, 
                      const IndicatorCache& cache, 
                      int startIndex, 
                      int pointsToShow);
    
    void addEMAToChart(FinanceChart* chart, 
                      const EMAInstance& ema, 
                      const IndicatorCache& cache, 
                      int startIndex, 
                      int pointsToShow);
    
    void addStochasticToChart(FinanceChart* chart, 
                             const StochasticInstance& stochastic, 
                             const IndicatorCache& cache, 
                             int startIndex, 
                             int pointsToShow);
    
    void addATRToChart(FinanceChart* chart, 
                      const ATRInstance& atr, 
                      const IndicatorCache& cache, 
                      int startIndex, 
                      int pointsToShow);
    
    // Utilitaires
    void addMarkers(XYChart* chart, 
                   const std::vector<std::pair<double, double>>& markers, 
                   const char* name, 
                   int symbolType, 
                   int symbolSize, 
                   int color);
    
    void addTPSLSegments(XYChart* chart, 
                        const std::vector<TPSLSegment>& segments);
        
    // Données membres
    std::unique_ptr<FinanceChart> m_financeChart;
};