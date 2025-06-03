#pragma once

#include <QWidget>
#include <QLabel>
#include <vector>
#include <memory>
#include <array>
#include <unordered_map>

#include "data.hpp"
#include "trade.hpp"

// Include ChartDirector headers
#include "qchartviewer.h"
#include "chartdir.h"
#include "FinanceChart.h"

/**
 * @brief Widget qui encapsule un graphique financier ChartDirector
 */
class ChartWidget : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief Enum pour les différents types de graphiques financiers
     */
    enum class ChartType {
        CandleStick,  ///< Graphique en chandeliers japonais
        HeikinAshi,   ///< Chandeliers Heikin Ashi (moyenne)
        OHLC,         ///< Barres OHLC (Open-High-Low-Close)
        Close,        ///< Ligne de prix de clôture uniquement
        
        Count         ///< Nombre total de types de graphiques
    };
    
    // ======== Constructeurs et destructeur ========
    explicit ChartWidget(QWidget* parent = nullptr);
    ~ChartWidget() override;
    
    // ======== API Publique ========
    // Méthodes d'initialisation des données
    void setBacktestData(const std::shared_ptr<be::Data>& data);
    void setBacktestTrades(const std::vector<std::shared_ptr<be::Trade>>& trades);
    void setEquityCurve(const std::vector<double>& equityCurve);
    
    // Configuration et contrôle du graphique
    void setChartType(ChartType chartType);
    ChartType getChartType() const { return m_config.chartType; }
    void setChartWidth(int width);
    void setShowVolume(bool show);
    void setShowTrades(bool show);
    void setShowEquity(bool show);
    
    // Actions sur le graphique
    void createChart();    ///< Crée le graphique complet avec les données actuelles
    void updateChart();    ///< Met à jour le graphique avec les nouvelles données
    void clearChart();     ///< Efface le graphique et les données
    void resetZoom();      ///< Réinitialise le zoom à l'état initial
    
    // État du graphique
    bool hasValidData() const;
    bool isChartCreated() const;
    
    // Conversion de ChartType 
    static QString chartTypeToString(ChartType type);
    static ChartType stringToChartType(const QString& typeStr);
    
signals:
    void chartCreated();
    void viewPortChanged();
    void mouseOverPoint(double timestamp, double price);
    
protected:
    // void resizeEvent(QResizeEvent* event) override;
    
private slots:
    void onViewPortChanged();
    void onMouseMovePlotArea(QMouseEvent* event);

private:
    // ======== Structures de données internes ========
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
     * @brief Structure pour stocker les données d'équité
     */
    struct EquityData {
        std::vector<double> timestamps;
        std::vector<double> equity_values;
        std::vector<double> drawdown;
    };

    /**
     * @brief Structure pour la configuration du graphique
     */
    struct ChartConfig {
        ChartType chartType = ChartType::CandleStick;
        int chartWidth = 1200;
        int mainChartHeight = 400;
        int equityHeight = 150;
        int volumeHeight = 100;
        bool showTrades = true;
        bool showVolume = true;
        bool showEquity = true;
    };
    
    /**
     * @brief Structure de métadonnées pour chaque type de graphique
     */
    struct ChartTypeInfo {
        ChartType type;
        const char* name;
    };

    // ======== Méthodes privées ========
    // 1. Traitement et conversion des données
    void convertBacktestData(const std::shared_ptr<be::Data>& data);
    void convertEquityCurve(const std::vector<double>& equityCurve, 
                          const std::shared_ptr<be::Data>& data);
    double dateToChartTimestamp(const be::Date& date);
    DoubleArray vectorToDoubleArray(const std::vector<double>& vec);
    void calculateHeikinAshi(const std::vector<double>& open, 
                           const std::vector<double>& high,
                           const std::vector<double>& low, 
                           const std::vector<double>& close,
                           std::vector<double>& ha_open, 
                           std::vector<double>& ha_high,
                           std::vector<double>& ha_low,
                           std::vector<double>& ha_close);
    
    // 2. Fonctions utilitaires pour les données
    template<typename T>
    std::vector<T> getVisibleDataRange(const std::vector<T>& data, int startIndex, int count);
    int calculateStartIndex(const DoubleArray& timestamps) const;
    void prepareVisibleOHLCVData(int startIndex, int endIndex, 
                               DoubleArray& timestamps, DoubleArray& open,
                               DoubleArray& high, DoubleArray& low,
                               DoubleArray& close, DoubleArray& volume);
    
    // 3. Méthodes d'initialisation et de configuration
    void setupChart();
    void setupChartViewer();
    
    // 4. Méthodes de rendu du graphique
    void drawChartWithViewport();
    FinanceChart* drawChart(const DoubleArray& timestamps, 
                          const DoubleArray& highData, 
                          const DoubleArray& lowData, 
                          const DoubleArray& openData, 
                          const DoubleArray& closeData,
                          const DoubleArray& volumeData,
                          int chartWidth);
    
    // 5. Composants du graphique
    FinanceChart* initializeChart(int chartWidth);
    void addEquityCurveSection(FinanceChart* chart, const DoubleArray& timestamps, int startIndex);
    void addMainChartSection(FinanceChart* chart, int chartHeight);
    void addTradeMarkers(FinanceChart* chart, const DoubleArray& timestamps, int startIndex);
    FinanceChart* finalizeChart(FinanceChart* chart);
    void addMarkers(XYChart* chart, const std::vector<std::pair<double, double>>& arrows, const char* name,
                  int symbolType, int symbolSize = 5, int color = -1);
    void addDirectionalArrow(XYChart* chart, double x, double y, double height, int color = -1);

    // 6. Gestion des interactions utilisateur
    void trackFinance(MultiChart* m, int mouseX);
    
    // 7. Fonctions utilitaires pour le tracking
    void setupTrackingLayer(DrawArea* d, XYChart* c);
    std::string getOHLCLegend(XYChart* c, Layer* layer, int xIndex);
    std::vector<std::string> getIndicatorLegends(XYChart* c, Layer* layer, int xIndex);
    void drawTrackingLine(DrawArea* d, XYChart* c, int mouseX, int xValue);
    
    // ======== Membres de données ========
    // 1. Configuration
    ChartConfig m_config;
    
    // 2. Données
    PriceData m_priceData;
    std::shared_ptr<be::Data> m_backtestData; 
    std::vector<std::shared_ptr<be::Trade>> m_trades;
    EquityData m_equityData;
    std::map<be::Date, double> m_timestampCache; // Cache pour dateToChartTimestamp
    
    // 3. Composants d'interface
    QChartViewer* m_chartViewer = nullptr;
    FinanceChart* m_financeChart = nullptr;
    
    // 4. Constantes statiques
    static const std::array<ChartTypeInfo, static_cast<size_t>(ChartType::Count)> s_chartTypeData;
};