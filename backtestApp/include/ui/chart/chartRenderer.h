#pragma once

#include <memory>
#include <vector>
#include <string>
#include <utility>

#include "qchartviewer.h"
#include "chartdir.h"
#include "FinanceChart.h"
#include "chartDataManager.h"

#include "ui/chart/chartTypes.h"

// Forward declarations of classes
class ChartDataManager;


class ChartRenderer {
public:
    ChartRenderer();
    ~ChartRenderer();

    // Main method to create or update the chart
    void createOrUpdateChart(
        QChartViewer* viewer,
        const ChartDataManager& dataManager,
        const chart::ChartConfiguration& config,
        const chart::AggregationInfo& aggregationInfo
    );

    std::optional<std::pair<int, int>> updateDynamicLayer(QChartViewer* viewer, 
                        bool rulerEnabled, 
                        bool rulerFirstPointSelected,
                        int rulerStartX, int rulerStartY, 
                        const ChartDataManager& dataManager,
                        const chart::AggregationInfo& aggregationInfo);

    void updateTrackFinance(QChartViewer* viewer, std::pair<int, int> forcedMousePosition);
    
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
        TakeProfit,  // Take Profit level
        StopLoss,    // Stop Loss level
        BreakEven    // Break Even level
    };

    // Structure for trade TP/SL segments
    struct TPSLBESegment {
        double startX;        // Entry point index
        double endX;          // Exit point index
        double level;         // Price level (TP or SL)
        TPSLBEType type;      // Level type (TP, SL, BE)
        int color;            // Color based on the trade result
    };

    // Method to draw the ruler
    void drawRuler(MultiChart* m, int startX, int startY, int endX, int endY, DrawArea* d,
                   const ChartDataManager& dataManager,
                   const chart::AggregationInfo& aggregationInfo);
    
    // Method for mouse tracking
    void trackFinance(MultiChart* m, int mouseX, int mouseY, DrawArea* d);
    
    // Rendering methods for sections
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

    
    // Methods to add indicators
    void addRSIToChart(FinanceChart* chart, 
        const indicators::RSIInstance& rsi, 
        const ChartDataManager& dataManager, 
        const chart::AggregationInfo& aggregationInfo);

    void addEMAToChart(FinanceChart* chart, 
        const indicators::EMAInstance& ema, 
        const ChartDataManager& dataManager, 
        const chart::AggregationInfo& aggregationInfo);

        // Add to the ChartRenderer class:
    void addSupertrendToChart(FinanceChart* chart, 
        const indicators::SuperTrendInstance& supertrend, 
        const ChartDataManager& dataManager, 
        const chart::AggregationInfo& aggregationInfo);

    void addStochasticToChart(FinanceChart* chart, 
        const indicators::StochasticInstance& stochastic, 
        const ChartDataManager& dataManager, 
        const chart::AggregationInfo& aggregationInfo);

    void addATRToChart(FinanceChart* chart, 
        const indicators::ATRInstance& atr, 
        const ChartDataManager& dataManager, 
        const chart::AggregationInfo& aggregationInfo);

    void addCCIToChart(FinanceChart* chart, 
        const indicators::CCIInstance& cci, 
        const ChartDataManager& dataManager, 
        const chart::AggregationInfo& aggregationInfo);
    
    void addMACDToChart(FinanceChart* chart, 
        const indicators::MACDInstance& macd, 
        const ChartDataManager& dataManager, 
        const chart::AggregationInfo& aggregationInfo);
    
    void addBBToChart(FinanceChart* chart, 
        const indicators::BBInstance& bb, 
        const ChartDataManager& dataManager, 
        const chart::AggregationInfo& aggregationInfo);

    void addPivotPointsToChart(XYChart* mainChart,
        const indicators::PivotPointsInstance& pivotPoints,
        const ChartDataManager& dataManager,
        const chart::AggregationInfo& aggregationInfo);

    void addSwingStructureToChart(FinanceChart* chart,
        const indicators::SwingStructureInstance& swingStructure,
        const ChartDataManager& dataManager,
        const chart::AggregationInfo& aggregationInfo);

    // Method to add user-drawn markers
    void addUserMarkers(XYChart* mainChart,
                       const std::vector<chart::ChartMarker>& markers,
                       const chart::AggregationInfo& aggregationInfo);

    // Utilities
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
        
    // Member data
    std::unique_ptr<FinanceChart> m_financeChart;

    double m_lastYMin = 0.0; // Last minimum value of the Y-axis scale
    double m_lastYMax = 0.0; // Last maximum value of the Y-axis scale
    double m_plotAreaHeight = 0.0; // Height of the plot area
};