#include "chart_widget.h"
#include <QVBoxLayout>
#include <QDebug>
#include <QMouseEvent>
#include <algorithm>
#include <cmath>
#include <set>

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

bool ChartWidget::hasValidData() const {
    return m_dataManager.hasValidData();
}


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

RSIInstance *ChartWidget::findRSI(int id)
{
    return findIndicator<RSIInstance>(id, m_rsiInstances);
}

EMAInstance *ChartWidget::findEMA(int id)
{
    return findIndicator<EMAInstance>(id, m_emaInstances);
}

SuperTrendInstance *ChartWidget::findSuperTrend(int id)
{
    return findIndicator<SuperTrendInstance>(id, m_superTrendInstances);
}

StochasticInstance *ChartWidget::findStochastic(int id)
{
    return findIndicator<StochasticInstance>(id, m_stochasticInstances);
}

ATRInstance *ChartWidget::findATR(int id)
{
    return findIndicator<ATRInstance>(id, m_atrInstances);
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
    m_renderer.createOrUpdateChart(m_chartViewer, m_dataManager, m_config, m_currentAggregation, 
                                   m_rsiInstances, m_emaInstances, m_superTrendInstances, 
                                   m_stochasticInstances, m_atrInstances);

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


//-----Indicators Implementation-----
int ChartWidget::addRSI(const RSIInstance &config)
{
    RSIInstance validatedConfig = config;

    if (validatedConfig.period < 2) validatedConfig.period = 2;  // Validation de base

    int id = addIndicatorImpl(validatedConfig, m_rsiInstances);

    emit rsiAdded(id, validatedConfig.period);

    return id;
}

bool ChartWidget::setRSIConfig(const RSIInstance &config)
{
    RSIInstance* oldConfig = findIndicator(config.id, m_rsiInstances);
    if (!oldConfig) return false;

    bool needsRecalculation = (oldConfig->period != config.period);

    if (!setIndicatorConfigImpl(config, m_rsiInstances, needsRecalculation)) return false;

    emit rsiChanged(config.id, config.period);

    return true;
}

// si l'id est a -1 on supprime tous les RSI
bool ChartWidget::removeRSI(int id)
{
    if (!removeIndicatorImpl<RSIInstance>(id, m_rsiInstances))
        return false;

    emit rsiRemoved(id);
    
    return true;
}

int ChartWidget::addEMA(const EMAInstance& config)
{
    EMAInstance validatedConfig = config;

    if (validatedConfig.period < 2) validatedConfig.period = 2;  // Validation de base

    int id = addIndicatorImpl(validatedConfig, m_emaInstances);

    emit emaAdded(id, validatedConfig.period);

    return id;
}

bool ChartWidget::setEMAConfig(const EMAInstance &config)
{
    EMAInstance* oldConfig = findIndicator(config.id, m_emaInstances);
    if (!oldConfig) return false;

    bool needsRecalculation = (oldConfig->period != config.period);

    if (!setIndicatorConfigImpl(config, m_emaInstances, needsRecalculation)) return false;

    emit emaChanged(config.id, config.period);

    return true;
}

bool ChartWidget::removeEMA(int id)
{
    if (!removeIndicatorImpl<EMAInstance>(id, m_emaInstances))
        return false;
    
    emit emaRemoved(id);
    
    return true;
}

int ChartWidget::addSuperTrend(const SuperTrendInstance &config)
{
    SuperTrendInstance validatedConfig = config;

    if (validatedConfig.period < 2) validatedConfig.period = 2;
    if (validatedConfig.multiplier <= 0) validatedConfig.multiplier = 3.0;

    int id = addIndicatorImpl(validatedConfig, m_superTrendInstances);

    emit superTrendAdded(id, validatedConfig.period, validatedConfig.multiplier);

    return id;
}

bool ChartWidget::setSuperTrendConfig(const SuperTrendInstance &config)
{
    SuperTrendInstance* oldConfig = findIndicator(config.id, m_superTrendInstances);
    if (!oldConfig) return false;

    bool needsRecalculation = (oldConfig->period != config.period || 
                               oldConfig->multiplier != config.multiplier);

    if (!setIndicatorConfigImpl(config, m_superTrendInstances, needsRecalculation)) return false;

    emit superTrendChanged(config.id, config.period, config.multiplier);

    return true;
}

bool ChartWidget::removeSuperTrend(int id)
{
    if (!removeIndicatorImpl<SuperTrendInstance>(id, m_superTrendInstances))
        return false;
    
    emit superTrendRemoved(id);
    
    return true;
}

int ChartWidget::addStochastic(const StochasticInstance& config)
{
    StochasticInstance validatedConfig = config;

    if (validatedConfig.fastKPeriod < 2) validatedConfig.fastKPeriod = 2;
    if (validatedConfig.slowKPeriod < 2) validatedConfig.slowKPeriod = 2;
    if (validatedConfig.slowDPeriod < 2) validatedConfig.slowDPeriod = 2;

    int id = addIndicatorImpl(validatedConfig, m_stochasticInstances);

    emit stochasticAdded(id, validatedConfig.fastKPeriod, validatedConfig.slowKPeriod, validatedConfig.slowDPeriod);

    return id;
}

bool ChartWidget::setStochasticConfig(const StochasticInstance& config)
{
    StochasticInstance* oldConfig = findIndicator(config.id, m_stochasticInstances);
    if (!oldConfig) return false;


    bool needsRecalculation = (oldConfig->fastKPeriod != config.fastKPeriod ||
                          oldConfig->slowKPeriod != config.slowKPeriod ||
                          oldConfig->slowDPeriod != config.slowDPeriod);
    if (!setIndicatorConfigImpl(config, m_stochasticInstances, needsRecalculation)) return false;

    emit stochasticChanged(config.id, config.fastKPeriod, config.slowKPeriod, config.slowDPeriod);

    return true;
}

bool ChartWidget::removeStochastic(int id)
{
    if (!removeIndicatorImpl<StochasticInstance>(id, m_stochasticInstances))
        return false;
    
    emit stochasticRemoved(id);
    
    return true;
}

int ChartWidget::addATR(const ATRInstance& config)
{
    ATRInstance validatedConfig = config;

    if (validatedConfig.period < 2) validatedConfig.period = 2;  // Validation de base

    int id = addIndicatorImpl(validatedConfig, m_atrInstances);

    emit atrAdded(id, validatedConfig.period);

    return id;
}

bool ChartWidget::setATRConfig(const ATRInstance &config)
{
    ATRInstance* oldConfig = findIndicator(config.id, m_atrInstances);
    if (!oldConfig) return false;

    bool needsRecalculation = (oldConfig->period != config.period || 
                              oldConfig->useLogScale != config.useLogScale);

    if (!setIndicatorConfigImpl(config, m_atrInstances, needsRecalculation)) return false;

    emit atrChanged(config.id, config.period);

    return true;
}

bool ChartWidget::removeATR(int id)
{
    if (!removeIndicatorImpl<ATRInstance>(id, m_atrInstances))
        return false;
    
    emit atrRemoved(id);
    
    return true;
}

template <typename T>
T *ChartWidget::findIndicator(int id, std::vector<T> &instances)
{
    auto it = std::find_if(instances.begin(), instances.end(),
                         [id](const T& instance) { return instance.id == id; });

    if (it == instances.end())
        return nullptr;

    return &(*it);
}

template <typename T, typename Container>
int ChartWidget::addIndicatorImpl(const T &configIn, Container &container)
{
    // Créer une copie pour pouvoir modifier l'ID
    T config = configIn;
    config.id = m_nextIndicatorId++;
    
    // Ajouter aux instances
    container.push_back(config);

    // Methode a implementer dans ChartDataManager
    m_dataManager.calculateIndicator(config);
    
    // Mettre à jour le graphique si nécessaire
    if (m_dataManager.hasValidData() && m_chartViewer)
        updateChartDisplay(ViewPortMode::USE_CURRENT);
    
    return config.id;
}

template<typename T, typename Container>
bool ChartWidget::setIndicatorConfigImpl(const T& config, Container& container, bool needsRecalculation)
{
    auto it = std::find_if(container.begin(), container.end(),
                         [config](const T& item) { return item.id == config.id; });

    if (it == container.end()) {
        return false;
    }
    
    // Mettre à jour la configuration mais préserver l'ID
    *it = config;

    if (needsRecalculation) {
        // S'assurer que les données sont recalculées
        m_dataManager.calculateIndicator(config);
    }

    // Mettre à jour le graphique
    if (m_dataManager.hasValidData())
        updateChartDisplay(ViewPortMode::USE_CURRENT);
    
    return true;
}

template<typename T, typename Container>
bool ChartWidget::removeIndicatorImpl(int id, Container& container)
{
    auto it = std::find_if(container.begin(), container.end(),
                         [id](const T& item) { return item.id == id; });
    
    if (it == container.end()) {
        return false;
    }
    
    // Supprimer l'instance
    container.erase(it);
    
    // Mettre à jour le graphique
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
    // Copier les listes pour éviter les problèmes lors de la suppression
    std::vector<int> rsiIds, emaIds, stochIds, atrIds;
    
    for (const auto& rsi : m_rsiInstances) {
        rsiIds.push_back(rsi.id);
    }
    
    for (const auto& ema : m_emaInstances) {
        emaIds.push_back(ema.id);
    }
    
    for (const auto& stoch : m_stochasticInstances) {
        stochIds.push_back(stoch.id);
    }
    
    for (const auto& atr : m_atrInstances) {
        atrIds.push_back(atr.id);
    }
    
    // Supprimer tous les indicateurs
    for (int id : rsiIds) removeRSI(id);
    for (int id : emaIds) removeEMA(id);
    for (int id : stochIds) removeStochastic(id);
    for (int id : atrIds) removeATR(id);
}
