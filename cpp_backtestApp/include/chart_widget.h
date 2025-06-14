#pragma once

#include <QWidget>
#include <QLabel>
#include <vector>
#include <memory>
#include <array>
#include <unordered_map>
#include <iostream>

#include "data.hpp"
#include "trade.hpp"
#include "components/technical_indicators.h"
#include "components/backtest_results.h"
#include "chart_data_manager.h"
#include "chart_renderer.h"

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
    
    // ======== Constructeurs et destructeur ========
    explicit ChartWidget(QWidget* parent = nullptr);
    ~ChartWidget() override;
    
    // ======== API Publique ========
    // Méthodes d'initialisation des données
    void setBacktestResults(const BacktestResults* results);

    // Configuration et contrôle du graphique
    void setChartType(ChartDataManager::ChartType chartType);
    ChartDataManager::ChartType getChartType() const { return m_config.chartType; }
    const std::vector<RSIInstance>& getRSIInstances() const { return m_rsiInstances; }
    const std::vector<EMAInstance>& getEMAInstances() const { return m_emaInstances; }
    const std::vector<StochasticInstance>& getStochasticInstances() const { return m_stochasticInstances; }
    const std::vector<ATRInstance>& getATRInstances() const { return m_atrInstances; }
    RSIInstance* findRSI(int id);
    EMAInstance* findEMA(int id);
    StochasticInstance* findStochastic(int id);
    ATRInstance* findATR(int id);
    
    // Actions sur le graphique

    void clearChart();     ///< Efface le graphique et les données
    void resetZoom();      ///< Réinitialise le zoom à l'état initial
    
    // État du graphique
    bool hasValidData() const;
    bool isChartCreated() const;
    
    // Conversion de ChartType 
    static QString chartTypeToString(ChartDataManager::ChartType type);
    static ChartDataManager::ChartType stringToChartType(const QString& typeStr);

    // Resize et gestion de la vue
    void setResizing(bool isResizing) { m_isResizing = isResizing; }
    bool isResizing() const { return m_isResizing; }
    
    // Méthode pour activer/désactiver l'outil règle
    void setRulerToolEnabled(bool enabled);
    
signals:
    void chartCreated();
    void viewPortChanged();
    void mouseOverPoint(double timestamp, double price);

    // Signaux pour le RSI
    void rsiAdded(int id, int period);
    void rsiChanged(int id, int period);
    void rsiRemoved(int id);

    // Signaux pour l'EMA
    void emaAdded(int id, int period);
    void emaChanged(int id, int period);
    void emaRemoved(int id);

    // Signaux pour le Stochastique
    void stochasticAdded(int id, int fastKPeriod, int slowKPeriod, int slowDPeriod);
    void stochasticChanged(int id, int fastKPeriod, int slowKPeriod, int slowDPeriod);
    void stochasticRemoved(int id);

    // Signaux pour l'ATR
    void atrAdded(int id, int period);
    void atrChanged(int id, int period);
    void atrRemoved(int id);
    
protected:
    void resizeEvent(QResizeEvent* event) override;
    
private slots:
    void onViewPortChanged();
    void onMouseMovePlotArea(QMouseEvent* event);
    void onMouseClickPlotArea(QMouseEvent* event); // Nouvelle méthode
    
public slots:
    // Pour le RSI
    int addRSI(int period = 14);
    bool setRSIConfig(int id, const RSIInstance& config);
    bool removeRSI(int id);

    // Pour l'EMA
    int addEMA(int period = 20);
    bool setEMAConfig(int id, const EMAInstance& config);
    bool removeEMA(int id);

    // Pour le Stochastique
    int addStochastic(int fastKPeriod = 14, int slowKPeriod = 3, int slowDPeriod = 3);
    bool setStochasticConfig(int id, const StochasticInstance& config);
    bool removeStochastic(int id);

    // Pour l'ATR
    int addATR(int period = 14);
    bool setATRConfig(int id, const ATRInstance& config);
    bool removeATR(int id);

    // Pour gérer le redimensionnement du graphique
    // void onWindowResized(QSize newSize);

private:
    // ======== Structures de données internes ========

    ChartDataManager m_dataManager;
    ChartRenderer m_renderer;


    ChartDataManager::AggregationInfo m_currentAggregation;


    // ======== Méthodes privées ========
    // 1. Traitement et conversion des données
    double dateToChartTimestamp(const be::Date& date);
    void prepareTimestampsCache();
    DoubleArray vectorToDoubleArray(const std::vector<double>& vec);
    void updateIndicatorCache();

    /**
     * @brief Crée ou met à jour le graphique avec les données actuelles
     * 
     * @param useViewport Si true, utiliser le viewport actuel; sinon, afficher toutes les données
     * @param preserveViewport Si true, préserver la position du viewport actuel après mise à jour
     * @return True si le graphique a été créé/mis à jour avec succès
     */
    bool updateChartDisplay(bool useViewport = true, bool preserveViewport = true);

    
    // ======== Membres de données ========
    // 1. Configuration
    ChartConfiguration m_config;
    
    // 2. Données
    std::vector<std::shared_ptr<be::Trade>> m_trades;
    
    IndicatorCache m_indicatorCache;
    std::vector<RSIInstance> m_rsiInstances;  ///< Instances de RSI actives
    std::vector<EMAInstance> m_emaInstances;  ///< Instances d'EMA actives
    std::vector<StochasticInstance> m_stochasticInstances; ///< Instances de Stochastique actives
    std::vector<ATRInstance> m_atrInstances;  ///< Instances d'ATR actives

    int m_nextRSIId = 1;                     ///< Prochain ID disponible pour RSI
    int m_nextEMAId = 1;                     ///< Prochain ID disponible pour EMA
    int m_nextStochasticId = 1;              ///< Prochain ID disponible pour Stochastique
    int m_nextATRId = 1;                      ///< Prochain ID disponible pour ATR

    // Méthodes privées pour le RSI
    // void addRSIToChart(FinanceChart* chart, const RSIInstance& rsi, int startIndex, int pointsToShow);
    void ensureRSICached(int period);

    // Méthodes privées pour l'EMA
    // void addEMAToChart(FinanceChart* chart, const EMAInstance& ema, int startIndex, int pointsToShow);
    void ensureEMACached(int period);

    // Méthodes privées pour le Stochastic
    // void addStochasticToChart(FinanceChart* chart, const StochasticInstance& stochastic, int startIndex, int pointsToShow);
    void ensureStochasticCached(int fastKPeriod, int slowKPeriod, int slowDPeriod);

    // Méthodes privées pour l'ATR
    // void addATRToChart(std::unique_ptr<FinanceChart>& chart, const ATRInstance& atr, int startIndex, int pointsToShow);
    void ensureATRCached(int period);

    // 3. Composants d'interface
    QChartViewer* m_chartViewer = nullptr;
    // std::unique_ptr<FinanceChart> m_financeChart = nullptr;
    

    bool m_isResizing = false;  ///< Indique si le widget est en cours de redimensionnement
    QSize m_pendingResize;  ///< Taille en attente de redimensionnement

    // Variables pour l'outil règle
    bool m_rulerToolEnabled;           // Si l'outil règle est activé
    bool m_rulerFirstPointSelected;    // Si le premier point a été sélectionné
    double m_rulerStartX;              // Coordonnée X du point de départ
    double m_rulerStartY;              // Coordonnée Y du point de départ
};