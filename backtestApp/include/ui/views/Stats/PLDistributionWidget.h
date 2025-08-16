#pragma once

#include <QWidget>
#include <QChart>
#include <QChartView>
#include <QBarSet>
#include <QBarSeries>
#include <QValueAxis>
#include <QBarCategoryAxis>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QComboBox>
#include "stats.hpp"


class PLDistributionWidget : public QWidget {
    Q_OBJECT

public:
    explicit PLDistributionWidget(QWidget* parent = nullptr);
    void updateData(const be::Stats& stats);
    void clear();

private:
    void setupUI();
    void computeStatistics(const std::vector<be::TradeData>& trades);
    void updateChart();
    void updateSummary();
    
    enum class DisplayMode {
        AllTrades,
        WinningTrades,
        LosingTrades
    };

    // Données
    std::vector<double> m_plValues;
    std::vector<double> m_positiveValues;
    std::vector<double> m_negativeValues;
    int m_totalTrades = 0;
    double m_avgWin = 0.0;
    double m_avgLoss = 0.0;
    double m_maxWin = 0.0;
    double m_maxLoss = 0.0;
    double m_median = 0.0;
    
    // Widgets UI
    QChartView* m_chartView;
    
    QGroupBox* m_summaryBox;
    QLabel* m_avgWinLabel;
    QLabel* m_avgLossLabel;
    QLabel* m_ratioLabel;
    QLabel* m_medianLabel;
    QLabel* m_distributionLabel;
    QComboBox* m_displayModeCombo;
    
    DisplayMode m_currentMode = DisplayMode::AllTrades;
    
    int m_numBins = 30;
};