#pragma once

#include <QWidget>
#include <vector>
#include <memory>

// Include ChartDirector headers
#include "qchartviewer.h"
#include "chartdir.h"
#include "FinanceChart.h"

/**
 * @brief Structure pour stocker les données de prix
 */
struct PriceData {
    std::vector<double> timestamps;
    std::vector<double> open;
    std::vector<double> high;
    std::vector<double> low;
    std::vector<double> close;
    std::vector<double> volume;
};

/**
 * @brief Structure pour stocker les données des trades
 */
struct TradeData {
    std::vector<double> entry_times;
    std::vector<double> exit_times;
    std::vector<double> entry_prices;
    std::vector<double> exit_prices;
    std::vector<QString> types;
    std::vector<double> pnl;
};

/**
 * @brief Structure pour stocker les données d'équité
 */
struct EquityData {
    std::vector<double> timestamps;
    std::vector<double> equity_values;
    std::vector<double> drawdown;
};

/**
 * @brief Widget qui encapsule un graphique financier ChartDirector
 */
class ChartWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ChartWidget(QWidget* parent = nullptr);
    ~ChartWidget();

    // Définir les données du graphique
    void setPriceData(const PriceData& data);
    void setTradeData(const TradeData& data);
    void setEquityData(const EquityData& data);
    
    // Configuration du graphique
    void setChartType(const QString& chartType);
    
    // Création et mise à jour du graphique
    void createChart();
    void updateChart();
    void clearChart();
    
    // Vérification des données
    bool hasValidData() const;

signals:
    void chartCreated();
    void viewPortChanged();
    void mouseOverPoint(double timestamp, double price);

private slots:
    void onViewPortChanged();
    void onMouseMovePlotArea(QMouseEvent* event);

private:
    // Méthodes internes
    void setupChart();
    void drawChartWithViewport();
    void calculateHeikinAshi(const std::vector<double>& open, 
                            const std::vector<double>& high,
                            const std::vector<double>& low, 
                            const std::vector<double>& close,
                            std::vector<double>& ha_open, 
                            std::vector<double>& ha_high,
                            std::vector<double>& ha_low,
                            std::vector<double>& ha_close);
    
    // Traitement des données
    DoubleArray vectorToDoubleArray(const std::vector<double>& vec);
    
    // Méthodes de rendu
    FinanceChart* drawChart(const DoubleArray& timestamps, 
                          const DoubleArray& highData, 
                          const DoubleArray& lowData, 
                          const DoubleArray& openData, 
                          const DoubleArray& closeData,
                          const DoubleArray& volumeData,
                          int chartWidth);
    
    void trackFinance(MultiChart* m, int mouseX);
    
    // Données
    PriceData m_priceData;
    TradeData m_tradeData;
    EquityData m_equityData;
    
    // Éléments du graphique
    QChartViewer* m_chartViewer;
    FinanceChart* m_financeChart;
    
    // Configuration
    QString m_chartType;
    int m_chartWidth;
};