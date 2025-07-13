#pragma once

#include <memory>
#include <vector>
#include <string>
#include <utility>

#include "qchartviewer.h"
#include "chartdir.h"
#include "FinanceChart.h"
#include "chartDataManager.h"
#include "trade.hpp"

// Prédéclarations de classes
class ChartDataManager;



// Structure pour la configuration du graphique
struct ChartConfiguration {
    ChartDataManager::ChartType chartType = ChartDataManager::ChartType::CandleStick;
    int chartWidth = 1200; // il faut que ce soit adaptatif
    int chartHeight = 500;
    int equityHeight = 150; // inclue dans la taille du graphique principal
    // int volumeHeight = 100;
    bool showTrades = true;
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


class ChartRenderer {
public:
    ChartRenderer();
    ~ChartRenderer();

    // Méthode principale pour créer ou mettre à jour le graphique
    void createOrUpdateChart(
        QChartViewer* viewer,
        const ChartDataManager& dataManager,
        const ChartConfiguration& config,
        const ChartDataManager::AggregationInfo& aggregationInfo
    );

    void updateDynamicLayer(QChartViewer* viewer, 
                        bool rulerEnabled, 
                        bool rulerFirstPointSelected,
                        int rulerStartX, int rulerStartY, 
                        const ChartDataManager& dataManager,
                        const ChartDataManager::AggregationInfo& aggregationInfo);
    
private:
    // Méthode pour dessiner la règle
    void drawRuler(MultiChart* m, int startX, int startY, int endX, int endY, DrawArea* d,
                   const ChartDataManager& dataManager,
                   const ChartDataManager::AggregationInfo& aggregationInfo);
    
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
        const ChartDataManager& dataManager, 
        const ChartDataManager::AggregationInfo& aggregationInfo);

    void addEMAToChart(FinanceChart* chart, 
        const EMAInstance& ema, 
        const ChartDataManager& dataManager, 
        const ChartDataManager::AggregationInfo& aggregationInfo);

        // Ajouter dans la classe ChartRenderer:
    void addSupertrendToChart(FinanceChart* chart, 
        const SuperTrendInstance& supertrend, 
        const ChartDataManager& dataManager, 
        const ChartDataManager::AggregationInfo& aggregationInfo);

    void addStochasticToChart(FinanceChart* chart, 
        const StochasticInstance& stochastic, 
        const ChartDataManager& dataManager, 
        const ChartDataManager::AggregationInfo& aggregationInfo);

    void addATRToChart(FinanceChart* chart, 
        const ATRInstance& atr, 
        const ChartDataManager& dataManager, 
        const ChartDataManager::AggregationInfo& aggregationInfo);

    void addPivotPointsToChart(FinanceChart* chart,
        const PivotPointsInstance& pivotPoints,
        const ChartDataManager& dataManager,
        const ChartDataManager::AggregationInfo& aggregationInfo);

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