// #pragma once

// class ChartRenderer {
// public:
//     void createOrUpdateChart(
//         QChartViewer* viewer,
//         const ChartDataManager& dataManager,
//         const ChartIndicatorManager& indicatorManager,
//         const ChartConfiguration& config,
//         const std::vector<std::shared_ptr<be::Trade>>& trades,
//         AggregationLevel level,
//         int startIndex,
//         int pointCount
//     );
    
//     void drawRuler(MultiChart* m, int startX, int startY, int endX, int endY, DrawArea* d);
//     void trackFinance(MultiChart* m, int mouseX);
    
// private:
//     // Méthodes de rendu des sections
//     void addEquityCurveSection(FinanceChart *chart, const DataManager& dataManager, const DoubleArray &timestamps, int startIndex);
//     void addMainChartSection(FinanceChart *chart, int chartHeight);
//     void addTradeMarkers(FinanceChart *chart, const DoubleArray &timestamps, int startIndex);
    
//     // Méthodes pour l'ajout d'indicateurs
//     void addRSIToChart(FinanceChart* chart, const RSIInstance& rsi, const IndicatorCache& cache, int startIndex, int pointsToShow);
//     void addEMAToChart(FinanceChart* chart, const EMAInstance& ema, const IndicatorCache& cache, int startIndex, int pointsToShow);
//     void addStochasticToChart(FinanceChart* chart, const StochasticInstance& stochastic, const IndicatorCache& cache, int startIndex, int pointsToShow);
//     void addATRToChart(FinanceChart* chart, const ATRInstance& atr, const IndicatorCache& cache, int startIndex, int pointsToShow);
    
//     // Utilitaires
//     void addMarkers(XYChart* chart, const std::vector<std::pair<double, double>>& arrows, const char* name, int symbolType, int symbolSize, int color);
//     void addTPSLSegments(XYChart* chart, const std::vector<TPSLSegment>& segments);
//     DoubleArray vectorToDoubleArray(const std::vector<double>& vec);
    
//     std::unique_ptr<FinanceChart> m_financeChart;
// };