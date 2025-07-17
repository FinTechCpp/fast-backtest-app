#pragma once

#include <QWidget>
#include <QLabel>
#include <vector>
#include <memory>
#include <array>
#include <unordered_map>
#include <iostream>

#include "data.hpp"
#include "components/technicalIndicators.h"
#include "components/backtestResults.h"
#include "ui/chart/chartDataManager.h"
#include "ui/chart/chartRenderer.h"

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

    template<typename T>
    int addIndicator(T&& config) {
        if (!m_dataManager.hasValidData()) return -1; 

        QString displayName = config.getDisplayName();
        int id = m_dataManager.addIndicator(std::move(config));

        emit indicatorAdded(id, displayName);

        if (m_dataManager.hasValidData() && m_chartViewer)
            updateChartDisplay(ViewPortMode::USE_CURRENT);

        return id;
    }

    template<typename T>
    T* findIndicator(int id) const {
        return m_dataManager.findIndicator<T>(id);
    }

    template<typename T>
    bool updateIndicator(const T& config) {
        if (!m_dataManager.updateIndicator(config)) 
            return false;

        emit indicatorChanged(config.id, config.getDisplayName());

        if (m_dataManager.hasValidData() && m_chartViewer)
            updateChartDisplay(ViewPortMode::USE_CURRENT);

        return true;
    }

    bool removeIndicator(int id);
    
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
    void onMousePressed(QMouseEvent* event);
    void onMouseDoubleClicked(QMouseEvent* event);
    void onMouseMoveChart(QMouseEvent* event);
    void onMouseMovePlotArea(QMouseEvent* event);
    void onMouseReleased(QMouseEvent* event);

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


    bool m_yAxisZoomMode = false;
    int m_yAxisZoomStartY = 0;
    bool m_advancedNavigationMode = false;

    bool m_isDraggingVertically = false;
    int m_cumulativeVerticalDelta = 0; // Pour le suivi des déplacements cumulés

    // Propriétés pour le déplacement vertical
    bool m_verticalMoveMode = false;
    QPoint m_lastMousePos;
    double m_pixelToValueRatio = 0.0;

    // Variables pour le zoom vertical sur l'axe Y
    bool m_isYAxisDragging = false;     // Indique si on est en train de faire glisser l'axe Y
    double m_yAxisClickRelativePos = 0.0; // Position relative du clic sur l'axe Y (0 = bas, 1 = haut)
    int m_yAxisMarginWidth = 50;        // Marge en pixels pour détecter les clics sur l'axe Y
};