#ifndef CHART_VIEW_H
#define CHART_VIEW_H

#include <QObject>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QCheckBox>
#include <QComboBox>
#include <QPushButton>
#include <QScrollArea>
#include <QMainWindow>
#include <QLineEdit>
#include <QButtonGroup>
#include <QMouseEvent>
#include <vector>
#include <memory>
#include <map>
#include "baseview.h"
#include "../binding/pybinding.h"

// Include ChartDirector headers BEFORE any slots redefinition
#include "../../ChartDirector/qtdemo/qtdemo/qchartviewer.h"
#include "../../ChartDirector/include/chartdir.h"
#include "../../ChartDirector/include/FinanceChart.h"

#include <pybind11/pybind11.h>
#include <pybind11/embed.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>

// Constantes pour les couleurs des marqueurs
const int BUY_MARKER_COLOR = 0x00FF00;
const int SELL_MARKER_COLOR = 0xFF0000;

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
 * @brief Vue pour afficher les graphiques de prix et d'indicateurs
 */
class ChartView : public BaseView
{
    Q_OBJECT

public:
    explicit ChartView(QWidget* parent = nullptr);
    ~ChartView();
    
    // Implémentation des méthodes virtuelles de BaseView
    void updateData(void* data, void* stats) override;
    void clear() override;

protected:
    void setupUI() override;
    void resizeEvent(QResizeEvent* event) override;

public slots:
    void resizeChart(int newWidth);

private slots:
    void onMouseMovePlotArea(QMouseEvent* event);
    void onViewPortChanged();
    void onChartTypeChanged(int index); // Nouveau slot pour le changement de type de bougie

private:
    // Cache des données
    void* m_cachedData;
    void* m_cachedStats;
    bool m_dataExtracted;
    void* m_currentData;
    void* m_currentStats;

    // UI Components 
    QWidget* m_leftPanel;  // Panneau de gauche (settings)
    QWidget* m_rightPanel; // Panneau de droite (graphique)
    QLabel* m_chartPlaceholder;

    // Contrôles dans le panneau de gauche
    QComboBox* m_chartTypeCombo; // Combo box pour le type de bougie
    QLabel* m_settingsTitle;     // Titre du panneau

    // Chart components
    FinanceChart* m_financeChart;
    QChartViewer* m_chartViewer;
    
    // Data storage
    PriceData m_priceData;
    TradeData m_tradeData;
    EquityData m_equityData;
    
    // État du graphique
    QString m_currentChartType;  // Type de bougie actuel
    
    // Private methods
    void extractDataFromPython(void* data, void* stats);
    void extractPriceData(void* data);
    void extractTradeData(void* stats);
    void extractEquityData(void* stats);
    void createChart();
    void drawChartWithViewport();
    void trackFinance(MultiChart* m, int mouseX);
    
    // Méthode pour calculer les données Heikin Ashi
    void calculateHeikinAshi(const std::vector<double>& open, 
                             const std::vector<double>& high,
                             const std::vector<double>& low, 
                             const std::vector<double>& close,
                             std::vector<double>& ha_open, 
                             std::vector<double>& ha_high,
                             std::vector<double>& ha_low,
                             std::vector<double>& ha_close);
    
    DoubleArray vectorToDoubleArray(const std::vector<double>& vec);
    std::vector<double> extractDoubleVector(void* pyObj);
    bool hasValidData() const;
    void showPlaceholder(const QString& message);
    void debugChart();
};

#endif // CHART_VIEW_H