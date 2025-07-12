#include "ui/chart/chartWidget.h"
#include <QVBoxLayout>
#include <QDebug>
#include <QMouseEvent>
#include <algorithm>
#include <cmath>
#include <set>
#include "ui/chart/chartDataManager.h"

ChartWidget::ChartWidget(QWidget* parent)
    : QWidget(parent)
    , m_chartViewer(nullptr)
    , m_rulerToolEnabled(false)
    , m_rulerFirstPointSelected(false)
    , m_rulerStartX(0)
    , m_rulerStartY(0)
{
    // Configurer le widget
    setObjectName("chartWidget");
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    
    // Créer le layout
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    
    // Créer le QChartViewer
    m_chartViewer = new QChartViewer(this);
    m_chartViewer->setObjectName("chartViewer");
    m_chartViewer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    
    // Configurer le viewer
    m_chartViewer->setMouseUsage(Chart::MouseUsageScroll);
    m_chartViewer->setMouseTracking(true);
    m_chartViewer->setMouseWheelZoomRatio(1.4);
    m_chartViewer->setScrollDirection(Chart::DirectionHorizontal);
    m_chartViewer->setZoomDirection(Chart::DirectionHorizontal);
    m_chartViewer->setZoomInWidthLimit(0.00001); // Limite de zoom pour éviter les zooms trop fins
    
    // Connecter les signaux
    connect(m_chartViewer, &QChartViewer::viewPortChanged, 
            this, &ChartWidget::onViewPortChanged);
    connect(m_chartViewer, &QChartViewer::mouseMovePlotArea, 
            this, &ChartWidget::onMouseMovePlotArea);
    connect(m_chartViewer, &QChartViewer::clicked, 
            this, &ChartWidget::onMouseClickPlotArea);
    
    // Ajouter le viewer au layout
    layout->addWidget(m_chartViewer);
    
    qDebug() << "ChartWidget créé";
}

ChartWidget::~ChartWidget() {

}

void ChartWidget::setBacktestResults(const BacktestResults* results) {
    if (!results) {
        qWarning() << "Tentative de définir des résultats de backtest nuls";
        return;
    }

    m_dataManager.setBacktestData(results->data);
    m_dataManager.setEquityCurve(results->stats.equityCurve);
    m_dataManager.setTrades(results->stats.trades);

    updateChartDisplay(ViewPortMode::FULL_CHART);
}

// bool ChartWidget::hasValidData() const {
//     return m_dataManager.hasValidData();
// }


// mouais vrm pas terrible on pourrait directement utiliser les méthodes de ChartDataManager
QString ChartWidget::chartTypeToString(ChartDataManager::ChartType type)
{
    return QString::fromStdString(ChartDataManager::chartTypeToString(type));
}

ChartDataManager::ChartType ChartWidget::stringToChartType(const QString &typeStr)
{
    return ChartDataManager::stringToChartType(typeStr.toStdString());
}

void ChartWidget::setChartType(ChartDataManager::ChartType chartType)
{
    if (m_config.chartType == chartType)
        return; // Pas de changement, rien à faire

    m_config.chartType = chartType;

    // Si nous passons en mode HeikinAshi et que le cache n'est pas valide, le recalculer
    if (chartType == ChartDataManager::ChartType::HeikinAshi && !m_dataManager.getHeikinAshiCache().isValid && m_dataManager.hasValidData()) {
        m_dataManager.updateHeikinAshiCache();
    }

    // Si nous avons déjà des données, mettre à jour le graphique
    if (m_dataManager.hasValidData())
        updateChartDisplay(ViewPortMode::USE_CURRENT);
}

bool ChartWidget::updateChartDisplay(ViewPortMode mode) {
    if (!m_dataManager.hasValidData() || !m_chartViewer) {
        return false;
    }

    // Déterminer les indices de début et fin basés sur le viewport
    int startIndex = 0;
    int pointsToShow = static_cast<int>(m_dataManager.getTimestamps().size());
    
    if (mode == ViewPortMode::USE_CURRENT) {
        int totalPoints = pointsToShow;
        double viewPortLeft = m_chartViewer->getViewPortLeft();
        double viewPortWidth = m_chartViewer->getViewPortWidth();
        
        startIndex = (int)floor(viewPortLeft * totalPoints);
        int endIndex = (int)ceil((viewPortLeft + viewPortWidth) * totalPoints) - 1;
        
        // S'assurer que les indices sont dans les limites
        startIndex = std::max(0, std::min(startIndex, totalPoints - 1));
        endIndex = std::max(startIndex, std::min(endIndex, totalPoints - 1));
        
        pointsToShow = endIndex - startIndex + 1;
    }

    m_currentAggregation = m_dataManager.getOptimalAggregationInfo(DoubleArray(&m_dataManager.getTimestamps()[startIndex], pointsToShow));
    
    // Corriger l'appel avec tous les paramètres requis
    m_renderer.createOrUpdateChart(m_chartViewer, m_dataManager, m_config, m_currentAggregation);

    if (mode == ViewPortMode::FULL_CHART) {
        m_chartViewer->setViewPortLeft(0);
        m_chartViewer->setViewPortWidth(1.0);
    }
    
    return true;
}

void ChartWidget::onViewPortChanged()
{
    // Redessiner le graphique avec le nouveau viewport
    // if (m_chartViewer->needUpdateChart())
        updateChartDisplay(ViewPortMode::USE_CURRENT);

    // Émettre un signal pour indiquer que le viewport a changé
    // emit viewPortChanged();

    // std::cout << "Viewport changed: "
    //           << "Left: " << m_chartViewer->getViewPortLeft()
    //           << ", Width: " << m_chartViewer->getViewPortWidth() << std::endl;

    // std::cout << "Top: " << m_chartViewer->getViewPortTop()
    //           << ", Height: " << m_chartViewer->getViewPortHeight() << std::endl;
}

void ChartWidget::onMouseMovePlotArea(QMouseEvent* event)
{
    if (!m_chartViewer) return;
    

    m_renderer.updateDynamicLayer(
        m_chartViewer,
        m_rulerToolEnabled,
        m_rulerFirstPointSelected,
        m_rulerStartX, m_rulerStartY,
        m_dataManager,
        m_currentAggregation
    );

    // Récupérer les informations sur le point
    // if (m_financeChart->getChart()->getChartCount() > 1) {
    //     XYChart* mainChart = (XYChart*)m_financeChart->getChart(1);
    //     double xValue = mainChart->getNearestXValue(mouseX);
        
    //     // Trouver l'indice correspondant
    //     int dataIndex = -1;
    //     for (int i = 0; i < (int)m_dataManager.getTimestamps().size(); ++i) {
    //         if (fabs(m_dataManager.getTimestamps()[i] - xValue) < 1e-6) {
    //             dataIndex = i;
    //             break;
    //         }
    //     }

    //     if (dataIndex >= 0 && dataIndex < (int)m_dataManager.getBacktestData()->getClose().size()) {
    //         // Émettre un signal avec les informations du point
    //         emit mouseOverPoint(m_dataManager.getTimestamps()[dataIndex], m_dataManager.getBacktestData()->getClose()[dataIndex]);
    //     }
    // }
    
    // Mettre à jour l'affichage
    m_chartViewer->updateDisplay();
}

void ChartWidget::onMouseClickPlotArea(QMouseEvent* event)
{
    if (!m_rulerToolEnabled || !m_chartViewer || !m_chartViewer->getChart()) {
        return;
    }
    
    // Si le bouton gauche est cliqué et que l'outil règle est activé
    if (event->button() == Qt::LeftButton) {
        // Si c'est le premier clic, enregistrer le point de départ
        if (!m_rulerFirstPointSelected) {
            // Use plot area coordinates instead of chart coordinates
            m_rulerStartX = m_chartViewer->getPlotAreaMouseX();
            m_rulerStartY = m_chartViewer->getPlotAreaMouseY();
            m_rulerFirstPointSelected = true;
            
        } else {
            m_rulerFirstPointSelected = false;
        }
        
        // Mettre à jour l'affichage
        if (m_chartViewer->getChart()) {
            m_chartViewer->updateDisplay();
        }
    }
}

void ChartWidget::setMaxDisplayPoints(int value) {
    int oldValue = m_dataManager.getMaxDisplayPoints();
    m_dataManager.setMaxDisplayPoints(value);
    
    // Si la valeur a changé, mettre à jour le graphique et émettre le signal
    if (oldValue != m_dataManager.getMaxDisplayPoints()) {
        if (m_dataManager.hasValidData()) {
            updateChartDisplay(ViewPortMode::USE_CURRENT);
        }
        emit maxDisplayPointsChanged(m_dataManager.getMaxDisplayPoints());
    }
}

int ChartWidget::getMaxDisplayPoints() const {
    return m_dataManager.getMaxDisplayPoints();
}

void ChartWidget::setRulerToolEnabled(bool enabled)
{
    m_rulerToolEnabled = enabled;
    
    // Si l'outil est désactivé, réinitialiser l'état
    if (!enabled) {
        m_rulerFirstPointSelected = false;
        
        // Mettre à jour le graphique pour supprimer la règle
        if (m_chartViewer && m_chartViewer->getChart()) {
            m_chartViewer->updateDisplay();
        }
    }
}

bool ChartWidget::removeIndicator(int id) {
    if (!m_dataManager.removeIndicator(id)) return false;

    emit indicatorRemoved(id);

    if (m_dataManager.hasValidData())
        updateChartDisplay(ViewPortMode::USE_CURRENT);

    return true;
}

double ChartWidget::dateToChartTimestamp(const be::Date& date) {
    return Chart::chartTime(date.getYear(), date.getMonth(), date.getDay(), date.getHour(), date.getMinute(), date.getSecond());
}

// void ChartWidget::onWindowResized(QSize newSize)
// {
//     // Apply any pending resize
//     if (!m_pendingResize.isNull()) {
//         newSize = m_pendingResize;
//         m_pendingResize = QSize();
//     }
    
//     // Only update if width is valid
//     if (newSize.width() > 10) {
//         m_config.chartWidth = newSize.width() - 10;
        
//         // Update the chart if we have valid data
//         if (m_dataManager.hasValidData() && m_chartViewer) {
//             // Save current viewport state
//             double currentLeft = m_chartViewer->getViewPortLeft();
//             double currentWidth = m_chartViewer->getViewPortWidth();
            
//             // Redraw the chart
//             drawChartWithViewport();
            
//             // Restore viewport state
//             m_chartViewer->setViewPortLeft(currentLeft);
//             m_chartViewer->setViewPortWidth(currentWidth);
//             m_chartViewer->updateViewPort(true, false);
//         }
//     }
// }

void ChartWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    QSize newSize = event->size();
    
    // Ne pas mettre à jour pendant un redimensionnement en cours
    if (m_isResizing) {
        m_pendingResize = newSize;
        return;
    }
    
    // Mettre à jour la largeur du graphique en fonction de la largeur du widget
    if (newSize.width() > 10) {
        m_config.chartWidth = newSize.width();
        
        // Mettre à jour le graphique seulement si nécessaire
        if (m_dataManager.hasValidData() && m_chartViewer) {
            // Sauvegarder l'état actuel du viewport
            double currentLeft = m_chartViewer->getViewPortLeft();
            double currentWidth = m_chartViewer->getViewPortWidth();
            
            // Redessiner le graphique
            updateChartDisplay(ViewPortMode::USE_CURRENT);
            
            // Restaurer l'état du viewport
            m_chartViewer->setViewPortLeft(currentLeft);
            m_chartViewer->setViewPortWidth(currentWidth);
        }
    }
}

void ChartWidget::removeAllIndicators() {
    m_dataManager.removeAllIndicators();

    if (m_dataManager.hasValidData())
        updateChartDisplay(ViewPortMode::USE_CURRENT);
}
