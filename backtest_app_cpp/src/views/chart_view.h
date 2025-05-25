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
    ChartView(QObject* parent = nullptr);  
    ~ChartView();
    
    QWidget* create(QWidget* parentWidget = nullptr) override;
    void update(void* data, void* stats) override;
    void clear() override;

private slots:
    void onHeikinAshiToggled(bool checked);
    void onVolumeToggled(bool checked);
    void onEquityToggled(bool checked);
    void onAddIndicatorClicked();
    void onMouseMovePlotArea(QMouseEvent* event);
    void updateChart();
    void onViewPortChanged();

private:
    void* m_cachedData;                // Pointeur vers les données du backtest
    void* m_cachedStats;               // Pointeur vers les statistiques du backtest
    bool m_dataExtracted;            // Indique si les données ont été extraites

    // UI Components
    QWidget* m_chartContainer;
    QVBoxLayout* m_chartLayout;
    QLabel* m_chartPlaceholder;
    
    // Controls
    QWidget* m_controlsWidget;           // Widget conteneur des contrôles
    QHBoxLayout* m_controlsLayout;       // Layout des contrôles
    QCheckBox* m_heikinAshiCheckbox;
    QCheckBox* m_volumeCheckbox;
    QCheckBox* m_equityCheckbox;
    QPushButton* m_addIndicatorBtn;
    QComboBox* m_indicatorsCombo;        // ComboBox pour sélectionner les indicateurs

    // Chart components
    FinanceChart* m_financeChart;
    QChartViewer* m_chartViewer;
    
    // Data storage
    PriceData m_priceData;
    TradeData m_tradeData;
    EquityData m_equityData;
    std::map<QString, QVariant> m_indicatorsList;

    // AJOUT DES MEMBRES MANQUANTS
    void* m_currentData;                                    // Données courantes
    void* m_currentStats;                                   // Stats courantes
    QMap<QString, QVariant> m_indicatorConfigs;            // Configurations des indicateurs
    QStringList m_activeIndicators;                         // Liste des indicateurs actifs
    
    // Private methods
    void setupControls();
    void setupIndicatorsList();
    void extractDataFromPython(void* data, void* stats);
    void extractPriceData(void* data);
    void extractTradeData(void* stats);
    void extractEquityData(void* stats);
    void createChart();
    void addMainChart();
    void addVolumeChart();
    void addEquityChart();
    void addTradeMarkers();
    void addIndicators();
    void addEMAIndicator(int period, int color);
    void addRSIIndicator(int period);
    void addStochasticIndicator(int fastK, int slowK, int slowD);
    void addATRIndicator(int period);
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
    std::vector<QString> extractStringVector(void* pyObj);
    QVariant extractPythonValue(void* pyObj);
    bool hasValidData() const;
    void showPlaceholder(const QString& message);
    void debugChart();

private:
    void drawChartWithViewport();
    void setupMouseControls();
    void setupNavigationControls();

};

#endif // CHART_VIEW_H