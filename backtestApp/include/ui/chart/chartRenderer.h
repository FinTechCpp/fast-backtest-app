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

#include "ui/chart/chartTypes.h"

// Prédéclarations de classes
class ChartDataManager;


class ChartRenderer {
public:
    ChartRenderer();
    ~ChartRenderer();

    // Méthode principale pour créer ou mettre à jour le graphique
    void createOrUpdateChart(
        QChartViewer* viewer,
        const ChartDataManager& dataManager,
        const chart::ChartConfiguration& config,
        const chart::AggregationInfo& aggregationInfo
    );

    void updateDynamicLayer(QChartViewer* viewer, 
                        bool rulerEnabled, 
                        bool rulerFirstPointSelected,
                        int rulerStartX, int rulerStartY, 
                        const ChartDataManager& dataManager,
                        const chart::AggregationInfo& aggregationInfo);

    std::optional<std::pair<int, int>> updateTrackFinance(QChartViewer* viewer, std::optional<std::pair<int, int>> forcedMousePosition = std::nullopt);

    double getYAxisMin() const {
        return m_lastYMin;
    }

    double getYAxisMax() const {
        return m_lastYMax;
    }

    double getPlotAreaHeight() const {
        return m_plotAreaHeight;
    }
    
private:
    enum class TPSLBEType {
        TakeProfit,  // Niveau de Take Profit
        StopLoss,    // Niveau de Stop Loss
        BreakEven    // Niveau de Break Even
    };

    // Structure pour les segments TP/SL des trades
    struct TPSLBESegment {
        double startX;        // Index du point d'entrée
        double endX;          // Index du point de sortie
        double level;         // Niveau de prix (TP ou SL)
        TPSLBEType type;     // Type de niveau (TP, SL, BE)
        int color;            // Couleur basée sur le résultat du trade
    };

    // Méthode pour dessiner la règle
    void drawRuler(MultiChart* m, int startX, int startY, int endX, int endY, DrawArea* d,
                   const ChartDataManager& dataManager,
                   const chart::AggregationInfo& aggregationInfo);
    
    // Méthode pour le tracking de la souris
    void trackFinance(MultiChart* m, int mouseX, int mouseY, DrawArea* d);
    
    // Méthodes de rendu des sections
    void addEquityCurveSection(FinanceChart *chart, 
                               const ChartDataManager& dataManager, 
                               const DoubleArray &timestamps, 
                               int startIndex,
                               int equityHeight);
    
    void addTradeMarkers(XYChart *mainChart, 
                         const DoubleArray &timestamps,
                         const ChartDataManager& dataManager,
                         const chart::AggregationInfo& aggregationInfo);
                  
    void addRawTradeMarkers(XYChart *mainChart, 
                           const DoubleArray &timestamps,
                           const ChartDataManager& dataManager,
                           const chart::AggregationInfo& aggregationInfo);
                           
    void addAggregatedTradeMarkers(XYChart *mainChart, 
                                  const DoubleArray &timestamps,
                                  const ChartDataManager& dataManager,
                                  const chart::AggregationInfo& aggregationInfo);
    // void addTriggerPriceSegments(XYChart* chart, const std::vector<TriggerPriceSegment>& segments);

    
    // Méthodes pour l'ajout d'indicateurs
    void addRSIToChart(FinanceChart* chart, 
        const RSIInstance& rsi, 
        const ChartDataManager& dataManager, 
        const chart::AggregationInfo& aggregationInfo);

    void addEMAToChart(FinanceChart* chart, 
        const EMAInstance& ema, 
        const ChartDataManager& dataManager, 
        const chart::AggregationInfo& aggregationInfo);

        // Ajouter dans la classe ChartRenderer:
    void addSupertrendToChart(FinanceChart* chart, 
        const SuperTrendInstance& supertrend, 
        const ChartDataManager& dataManager, 
        const chart::AggregationInfo& aggregationInfo);

    void addStochasticToChart(FinanceChart* chart, 
        const StochasticInstance& stochastic, 
        const ChartDataManager& dataManager, 
        const chart::AggregationInfo& aggregationInfo);

    void addATRToChart(FinanceChart* chart, 
        const ATRInstance& atr, 
        const ChartDataManager& dataManager, 
        const chart::AggregationInfo& aggregationInfo);

    void addPivotPointsToChart(XYChart* mainChart,
        const PivotPointsInstance& pivotPoints,
        const ChartDataManager& dataManager,
        const chart::AggregationInfo& aggregationInfo);

    // Utilitaires
    ScatterLayer* addMarkers(XYChart* chart, 
                   const std::vector<std::pair<double, double>>& markers, 
                   const char* name, 
                   int symbolType, 
                   int symbolSize, 
                   int color, 
                   int offsetX = 0, 
                   int offsetY = 0);
    
    void addTPSLSegments(XYChart* chart, 
                        const std::vector<TPSLBESegment>& segments);
        
    // Données membres
    std::unique_ptr<FinanceChart> m_financeChart;

    double m_lastYMin = 0.0; // Dernière valeur minimale de l'échelle Y
    double m_lastYMax = 0.0; // Dernière valeur maximale de l'échelle Y
    double m_plotAreaHeight = 0.0; // Hauteur de la zone de tracé
};