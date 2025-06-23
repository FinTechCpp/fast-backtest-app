#pragma once

#include <QWidget>
#include <QLabel>
#include <vector>
#include <memory>
#include <array>
#include <unordered_map>
#include <iostream>

#include "data.hpp"
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
    void setChartType(ChartDataManager::ChartType chartType); // remplacer par un slot
    ChartDataManager::ChartType getChartType() const { return m_config.chartType; }
    const std::vector<std::unique_ptr<IndicatorBase>>& getIndicators() const { return m_dataManager.getIndicators(); }
    std::vector<const RSIInstance*> getRSIInstances() const { return m_dataManager.getIndicatorsOfType<RSIInstance>(); }
    std::vector<const EMAInstance*> getEMAInstances() const { return m_dataManager.getIndicatorsOfType<EMAInstance>(); }
    std::vector<const SuperTrendInstance*> getSuperTrendInstances() const { return m_dataManager.getIndicatorsOfType<SuperTrendInstance>(); }
    std::vector<const StochasticInstance*> getStochasticInstances() const { return m_dataManager.getIndicatorsOfType<StochasticInstance>(); }
    std::vector<const ATRInstance*> getATRInstances() const { return m_dataManager.getIndicatorsOfType<ATRInstance>(); }
    RSIInstance* findRSI(int id) const { return m_dataManager.findIndicator<RSIInstance>(id); }
    EMAInstance* findEMA(int id) const { return m_dataManager.findIndicator<EMAInstance>(id); }
    SuperTrendInstance* findSuperTrend(int id) const { return m_dataManager.findIndicator<SuperTrendInstance>(id); }
    StochasticInstance* findStochastic(int id) const { return m_dataManager.findIndicator<StochasticInstance>(id); }
    ATRInstance* findATR(int id) const { return m_dataManager.findIndicator<ATRInstance>(id); }

    // État du graphique
    bool hasValidData() const; // ne devrait pas etre un probleme les class exterieur s'enfoutent de si les données sont valides
    bool isChartCreated() const;
    void removeAllIndicators();
    
    // Conversion de ChartType 
    static QString chartTypeToString(ChartDataManager::ChartType type);
    static ChartDataManager::ChartType stringToChartType(const QString& typeStr);

    // Resize et gestion de la vue
    void setResizing(bool isResizing) { m_isResizing = isResizing; }
    bool isResizing() const { return m_isResizing; }
    
    // Méthode pour activer/désactiver l'outil règle
    void setRulerToolEnabled(bool enabled); // remplacer par un slot

    void setMaxDisplayPoints(int value);
    int getMaxDisplayPoints() const;
    
signals:
    void chartCreated();
    void viewPortChanged();
    void mouseOverPoint(double timestamp, double price);

    void indicatorAdded(int id, const QString& displayName);
    void indicatorChanged(int id, const QString& displayName);
    void indicatorRemoved(int id);

    void maxDisplayPointsChanged(int value);
    
protected:
    void resizeEvent(QResizeEvent* event) override;
    
private slots:
    void onViewPortChanged();
    void onMouseMovePlotArea(QMouseEvent* event);
    void onMouseClickPlotArea(QMouseEvent* event);
    
public slots:
// pour la validation on pourrait la rendre generique avec un lambda dans la 
// struct qui definit la condition de validityé de l'indicateur
// on pourrait aussi mettre la condition de recalcule pouir evité de la faire dans chaque
// methode et passer un booléen pour savoir si on doit recalculer ou pas
    // Pour le RSI
    int addRSI(RSIInstance&& config);
    int addRSI(const RSIInstance& config) { return addRSI(std::move(RSIInstance(config))); }
    bool setRSIConfig(const RSIInstance& config);
    bool removeRSI(int id);

    // Pour l'EMA
    int addEMA(EMAInstance&& config);
    int addEMA(const EMAInstance& config) { return addEMA(std::move(EMAInstance(config))); }
    bool setEMAConfig(const EMAInstance& config);
    bool removeEMA(int id);

    // Pour le SuperTrend
    int addSuperTrend(SuperTrendInstance&& config);
    int addSuperTrend(const SuperTrendInstance& config) { return addSuperTrend(std::move(SuperTrendInstance(config))); }
    bool setSuperTrendConfig(const SuperTrendInstance& config);
    bool removeSuperTrend(int id);

    // Pour le Stochastique
    int addStochastic(StochasticInstance&& config);
    int addStochastic(const StochasticInstance& config) { return addStochastic(std::move(StochasticInstance(config))); }
    bool setStochasticConfig(const StochasticInstance& config);
    bool removeStochastic(int id);

    // Pour l'ATR
    int addATR(ATRInstance&& config);
    int addATR(const ATRInstance& config) { return addATR(std::move(ATRInstance(config))); }
    bool setATRConfig(const ATRInstance& config);
    bool removeATR(int id);

    // Pour gérer le redimensionnement du graphique
    // void onWindowResized(QSize newSize);

private:
    enum class ViewPortMode {
        FULL_CHART,      // Afficher toutes les données
        USE_CURRENT      // Utiliser le viewport actuel
    };

    ChartDataManager m_dataManager;
    ChartRenderer m_renderer;
    ChartDataManager::AggregationInfo m_currentAggregation;
    ChartConfiguration m_config;

    // ======== Méthodes privées ========
    // 1. Traitement et conversion des données
    double dateToChartTimestamp(const be::Date& date);
    void prepareTimestampsCache();
    DoubleArray vectorToDoubleArray(const std::vector<double>& vec);


    bool updateChartDisplay(ViewPortMode mode = ViewPortMode::FULL_CHART);

    // 3. Composants d'interface
    QChartViewer* m_chartViewer = nullptr;
    

    bool m_isResizing = false;  ///< Indique si le widget est en cours de redimensionnement
    QSize m_pendingResize;  ///< Taille en attente de redimensionnement

    // Variables pour l'outil règle
    bool m_rulerToolEnabled;           // Si l'outil règle est activé
    bool m_rulerFirstPointSelected;    // Si le premier point a été sélectionné
    double m_rulerStartX;              // Coordonnée X du point de départ
    double m_rulerStartY;              // Coordonnée Y du point de départ
};