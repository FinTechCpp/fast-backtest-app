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
    // m_chartViewer->setUpdateInterval(5);
    
    // Configurer le viewer
    m_chartViewer->setMouseUsage(Chart::MouseUsageScroll);
    m_chartViewer->setMouseTracking(true);
    m_chartViewer->setMouseWheelZoomRatio(1.4);
    m_chartViewer->setScrollDirection(Chart::DirectionHorizontalVertical);
    m_chartViewer->setZoomDirection(Chart::DirectionHorizontalVertical);
    m_chartViewer->setZoomInWidthLimit(0.00001); // Limite de zoom pour éviter les zooms trop fins
    
    // Connecter les signaux
    connect(m_chartViewer, &QChartViewer::viewPortChanged, this, &ChartWidget::onViewPortChanged);
    connect(m_chartViewer, &QChartViewer::mousePressed, this, &ChartWidget::onMousePressed);
    connect(m_chartViewer, &QChartViewer::mouseMoveChart, this, &ChartWidget::onMouseMoveChart);
    connect(m_chartViewer, &QChartViewer::mouseMovePlotArea, this, &ChartWidget::onMouseMovePlotArea);
    connect(m_chartViewer, &QChartViewer::mouseReleased, this, &ChartWidget::onMouseReleased);
    connect(m_chartViewer, &QChartViewer::mouseDoubleClicked, this, &ChartWidget::onMouseDoubleClicked);

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

    m_dataManager.setData(results->data, results->stats.trades, results->stats.equityCurve);

    updateChartDisplay(ViewPortMode::USE_CURRENT);
}

void ChartWidget::setChartType(chart::ChartType chartType)
{
    if (m_config.chartType == chartType)
        return; // Pas de changement, rien à faire

    m_config.chartType = chartType;

    updateChartDisplay(ViewPortMode::USE_CURRENT);
}

bool ChartWidget::updateChartDisplay(ViewPortMode mode) {
    if (!m_dataManager.hasRawData() || !m_chartViewer)
        return false;

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

    chart::AggregationInfo newAggregation = m_dataManager.getOptimalAggregationInfo(
        DoubleArray(&m_dataManager.getTimestamps()[startIndex], pointsToShow)
    );

    // Si l'agrégation a changé, mettre à jour et émettre le signal
    if (newAggregation != m_currentAggregation) {
        m_currentAggregation = newAggregation;
        
        // Émettre le signal pour synchroniser l'autre widget si nécessaire
        emit aggregationChanged(m_currentAggregation);
    }

    m_renderer.createOrUpdateChart(m_chartViewer, m_dataManager, m_config, m_currentAggregation);

    if (mode == ViewPortMode::FULL_CHART) {
        m_chartViewer->setViewPortLeft(0);
        m_chartViewer->setViewPortWidth(1.0);
    }
    
    return true;
}

void ChartWidget::onViewPortChanged() {
    if (m_chartViewer) {
        double left = m_chartViewer->getViewPortLeft();
        double width = m_chartViewer->getViewPortWidth();
        
        // Émettre le signal de changement de viewport
        emit viewportChanged(left, width);
    }

    updateChartDisplay(ViewPortMode::USE_CURRENT);
}

void ChartWidget::onMouseMovePlotArea(QMouseEvent* event)
{
    if (!m_chartViewer) return;

    if (m_verticalMoveMode && (event->buttons() & Qt::LeftButton)) {
        // Calculer le delta en pixels
        int deltaY = event->pos().y() - m_lastMousePos.y();
        
        if (deltaY != 0) {
            // Convertir le delta en pixels en unités de l'échelle Y
            double valueShift = deltaY * m_pixelToValueRatio;
            
            // Mettre à jour l'offset dans la configuration
            m_config.yScaleOffset += valueShift; // Inverser le signe pour un déplacement naturel
            
            // Mettre à jour la position de référence
            m_lastMousePos = event->pos();
        }
    }
    


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

}

void ChartWidget::onMousePressed(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_lastMousePos = event->pos();

        // Au lieu de réinitialiser l'offset, mettre à jour les valeurs de référence
        // pour qu'elles reflètent l'échelle actuelle avec l'offset appliqué
        m_config.yScaleMin = m_renderer.getYAxisMin();
        m_config.yScaleMax = m_renderer.getYAxisMax();
        m_config.yScaleOffset = 0.0;  // Maintenant on peut réinitialiser car les min/max sont à jour
        
        // Recalculer le ratio pixel/valeur pour la nouvelle échelle
        double yRange = m_config.yScaleMax - m_config.yScaleMin;
        double plotAreaHeight = m_renderer.getPlotAreaHeight();
        m_pixelToValueRatio = yRange / plotAreaHeight;

        double relativeYPos = (m_lastMousePos.y() - m_config.equityHeight) / plotAreaHeight;
        if (m_lastMousePos.x() > m_config.chartWidth - m_yAxisMarginWidth && relativeYPos >= 0 && relativeYPos <= 1) {
            m_yAxisClickRelativePos = 1.0 - relativeYPos; // Inverser pour que 0 soit en bas et 1 en haut
            
            m_config.fixedYScale = true;

            m_isYAxisDragging = true;
            setCursor(Qt::SizeVerCursor);
        }
    }
}

void ChartWidget::onMouseDoubleClicked(QMouseEvent *event)
{
    if (m_verticalMoveMode) {
        m_config.fixedYScale = false;
        setCursor(Qt::ArrowCursor);

        updateChartDisplay(ViewPortMode::USE_CURRENT);
    }
}

void ChartWidget::onMouseMoveChart(QMouseEvent *event)
{
    // Gestion du zoom vertical sur l'axe Y
    if (m_isYAxisDragging && (event->buttons() & Qt::LeftButton)) {
        // Calculer le delta en pixels
        int deltaY = event->pos().y() - m_lastMousePos.y();
        
        if (deltaY != 0) {
            // Facteur d'amortissement pour contrôler la vitesse du zoom
            double zoomFactor = 1.0 - (deltaY * 0.005); // Ajuster selon la sensibilité souhaitée
            
            // Récupérer l'échelle Y actuelle
            double currentMin = m_config.yScaleMin;
            double currentMax = m_config.yScaleMax;
            double currentRange = currentMax - currentMin;
            
            // Calculer le point pivot (la valeur qui reste fixe pendant le zoom)
            double pivotValue = currentMin + m_yAxisClickRelativePos * currentRange;
            
            // Calculer la nouvelle plage en fonction du facteur de zoom
            double newRange = currentRange / zoomFactor;
            
            // Calculer les nouvelles limites en maintenant le point pivot à sa position relative
            double newMin = pivotValue - (m_yAxisClickRelativePos * newRange);
            double newMax = pivotValue + ((1.0 - m_yAxisClickRelativePos) * newRange);
            
            // Mettre à jour l'échelle Y
            m_config.yScaleMin = newMin;
            m_config.yScaleMax = newMax;
            
            // Mettre à jour la position de référence
            m_lastMousePos = event->pos();
            
            // Recalculer le ratio pour le déplacement vertical
            m_pixelToValueRatio = (newMax - newMin) / m_renderer.getPlotAreaHeight();

            // Indiquer que nous sommes en mode déplacement vertical
            m_verticalMoveMode = true;
            
            // Redessiner le graphique avec la nouvelle configuration
            updateChartDisplay(ViewPortMode::USE_CURRENT);
        }
    }

    std::optional<std::pair<int, int>> mousePosition = m_renderer.updateDynamicLayer(
        m_chartViewer,
        m_rulerToolEnabled,
        m_rulerFirstPointSelected,
        m_rulerStartX, m_rulerStartY,
        m_dataManager,
        m_currentAggregation
    );

    m_chartViewer->updateDisplay();

    if (mousePosition) {
        emit trackFinanceUpdated(mousePosition->first, mousePosition->second);
    }
}

void ChartWidget::onMouseReleased(QMouseEvent *event)
{
    if (!m_chartViewer || !m_chartViewer->getChart())
        return;
    
    // Si le bouton gauche est cliqué et que l'outil règle est activé
    if (m_rulerToolEnabled) {
        if (event->button() == Qt::LeftButton) {
            if (!m_rulerFirstPointSelected) {
                // Use plot area coordinates instead of chart coordinates
                m_rulerStartX = m_chartViewer->getPlotAreaMouseX();
                m_rulerStartY = m_chartViewer->getPlotAreaMouseY();
                m_rulerFirstPointSelected = true;
                
            } else {
                m_rulerFirstPointSelected = false;
            }
            m_chartViewer->updateDisplay();
        }

        return;
    }

    if (m_isYAxisDragging) {
        m_isYAxisDragging = false;
        // Maintenir le curseur en mode vertical puisqu'on reste en mode vertical
        setCursor(Qt::SizeVerCursor);
        event->accept();
    }
}

void ChartWidget::setMaxDisplayPoints(int value) {
    if (value == m_dataManager.getMaxDisplayPoints())
        return;

    m_dataManager.setMaxDisplayPoints(value);

    if (m_dataManager.hasRawData())
        updateChartDisplay(ViewPortMode::USE_CURRENT);

    emit maxDisplayPointsChanged(m_dataManager.getMaxDisplayPoints());
}

void ChartWidget::setRulerToolEnabled(bool enabled)
{
    m_rulerToolEnabled = enabled;
    
    // Si l'outil est désactivé, réinitialiser l'état
    if (enabled) 
        return;

    m_rulerFirstPointSelected = false;
    
    // Mettre à jour le graphique pour supprimer la règle
    if (m_chartViewer && m_chartViewer->getChart())
        m_chartViewer->updateDisplay();
}

bool ChartWidget::removeIndicator(int id) {
    if (!m_dataManager.removeIndicator(id)) return false;

    emit indicatorRemoved(id);

    if (m_dataManager.hasRawData())
        updateChartDisplay(ViewPortMode::USE_CURRENT);

    return true;
}

void ChartWidget::zoomToTrade(const be::TradeData& trade)
{
    if (!m_dataManager.hasRawData() || !m_chartViewer) {
        qDebug() << "Impossible de zoomer: données non valides ou viewer non initialisé";
        return;
    }
    
    // Convertir les dates du trade en timestamps pour le graphique
    double entryTimestamp = dateToChartTimestamp(trade.entryDate);
    double exitTimestamp = dateToChartTimestamp(trade.exitDate);
    
    qDebug() << "Zoom sur trade - Entrée timestamp:" << entryTimestamp 
             << "Sortie timestamp:" << exitTimestamp;
    
    // Obtenir tous les timestamps pour calculer les indices
    const std::vector<double>& timestamps = m_dataManager.getTimestamps();
    
    if (timestamps.empty()) {
        qDebug() << "Aucun timestamp disponible pour le zoom";
        return;
    }
    
    // Trouver les indices correspondant aux timestamps d'entrée et de sortie
    int entryIndex = -1;
    int exitIndex = -1;
    
    for (int i = 0; i < static_cast<int>(timestamps.size()); ++i) {
        if (entryIndex == -1 && timestamps[i] >= entryTimestamp) {
            entryIndex = i;
        }
        if (timestamps[i] >= exitTimestamp) {
            exitIndex = i;
            break;
        }
    }
    
    // Si nous n'avons pas trouvé l'index d'entrée, prendre le premier
    if (entryIndex == -1) entryIndex = 0;
    
    // Si nous n'avons pas trouvé l'index de sortie, prendre le dernier
    if (exitIndex == -1) exitIndex = static_cast<int>(timestamps.size()) - 1;
    
    // Ajouter une marge autour du trade (20% de la durée du trade de chaque côté)
    int tradeDuration = exitIndex - entryIndex;
    int margin = std::max(10, tradeDuration / 5); // Au minimum 10 points de marge
    
    int startIndex = std::max(0, entryIndex - margin);
    int endIndex = std::min(static_cast<int>(timestamps.size()) - 1, exitIndex + margin);
    
    // Calculer les proportions du viewport
    double totalPoints = static_cast<double>(timestamps.size());
    double viewPortLeft = static_cast<double>(startIndex) / totalPoints;
    double viewPortWidth = static_cast<double>(endIndex - startIndex + 1) / totalPoints;
    
    // S'assurer que la largeur du viewport ne dépasse pas 1.0
    if (viewPortLeft + viewPortWidth > 1.0) {
        viewPortWidth = 1.0 - viewPortLeft;
    }
    
    qDebug() << "Zoom calculé - Index entrée:" << entryIndex 
             << "Index sortie:" << exitIndex
             << "ViewPort left:" << viewPortLeft
             << "ViewPort width:" << viewPortWidth;
    
    // Appliquer le zoom
    m_chartViewer->setViewPortLeft(viewPortLeft);
    m_chartViewer->setViewPortWidth(viewPortWidth);
    
    // Mettre à jour l'affichage
    updateChartDisplay(ViewPortMode::USE_CURRENT);
}

void ChartWidget::setCurrentAggregation(const chart::AggregationInfo& aggregation) {
    // Seulement si l'agrégation est différente
    if (m_currentAggregation.level == aggregation.level && 
        m_currentAggregation.startIndex == aggregation.startIndex && 
        m_currentAggregation.pointCount == aggregation.pointCount) {
        return;
    }

    m_currentAggregation = aggregation;
    
    // Ne pas émettre le signal si on vient du partenaire pour éviter les boucles infinies
    if (sender() != m_syncPartner) {
        emit aggregationChanged(m_currentAggregation);
    }
    
    // Mettre à jour le graphique si nécessaire
    if (m_dataManager.hasRawData() && m_chartViewer) {
        m_renderer.createOrUpdateChart(m_chartViewer, m_dataManager, m_config, m_currentAggregation);
    }
}

void ChartWidget::setViewport(double left, double width) {
    if (m_chartViewer && sender() == m_syncPartner) {
        m_chartViewer->setViewPortLeft(left);
        m_chartViewer->setViewPortWidth(width);
    
        // Ne pas appeler updateChartDisplay directement pour éviter la boucle
        // Le signal viewPortChanged du QChartViewer sera émis et connecté à onViewPortChanged
        return;
    }
}

void ChartWidget::forceUpdateTrackFinance(int mouseX, int mouseY) {
    if (m_chartViewer && sender() == m_syncPartner) {
        m_renderer.updateTrackFinance(m_chartViewer, std::make_pair(mouseX, mouseY));

        m_chartViewer->updateDisplay();
        return;
    }
}

double ChartWidget::dateToChartTimestamp(const be::Date &date)
{
    return Chart::chartTime(date.year, date.month, date.day, date.hour, date.minute, date.second);
}

void ChartWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    QSize newSize = event->size();    
    
    // Mettre à jour la largeur du graphique en fonction de la largeur du widget
    if (newSize.width() > 10) {
        m_config.chartWidth = newSize.width();
        m_config.chartHeight = newSize.height() - 20;
        
        // Mettre à jour le graphique seulement si nécessaire
        if (m_dataManager.hasRawData() && m_chartViewer) {
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

    if (m_dataManager.hasRawData())
        updateChartDisplay(ViewPortMode::USE_CURRENT);
}
