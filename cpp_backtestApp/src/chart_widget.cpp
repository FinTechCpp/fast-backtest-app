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
    m_chartViewer->setMouseWheelZoomRatio(1.1);
    m_chartViewer->setScrollDirection(Chart::DirectionHorizontal);
    m_chartViewer->setZoomDirection(Chart::DirectionHorizontal);
    m_chartViewer->setZoomInWidthLimit(0.00001); // Limite de zoom pour éviter les zooms trop fins
    // m_chartViewer->set
    
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


    m_trades = results->stats.trades;


    updateIndicatorCache();

    

    // Mettre à jour le graphique si nous avons des données valides
    if (m_dataManager.hasValidData()) {
        updateChartDisplay(ViewPortMode::FULL_CHART);
    }
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
    
    m_renderer.createOrUpdateChart(m_chartViewer, m_dataManager,m_config, m_trades, m_currentAggregation, m_rsiInstances, m_emaInstances, m_stochasticInstances, m_atrInstances, m_indicatorCache);


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
        m_dataManager
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
    RSIInstance* oldConfig = findRSI(config.id);
    if (!oldConfig) return false;

    bool periodChanged = (oldConfig->period != config.period);

    if (!setIndicatorConfigImpl(config, m_rsiInstances, periodChanged)) return false;

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

int ChartWidget::addEMA(int period)
{
    if (period < 2) period = 2;  // Validation de base
    
    // Créer une nouvelle instance EMA
    EMAInstance ema;
    ema.id = m_nextIndicatorId++;
    ema.period = period;
    
    // S'assurer que les données EMA sont en cache
    ensureEMACached(period);
    
    // Ajouter aux instances actives
    m_emaInstances.push_back(ema);

    // Si nous avons déjà un graphique et des données valides, ajouter directement l'indicateur
    if (m_dataManager.hasValidData() && m_chartViewer) {
        // Déterminer l'index de début et le nombre de points visibles actuellement
        int startIndex = m_chartViewer->getViewPortLeft();
        int pointsToShow = m_chartViewer->getViewPortWidth();

        // Ajouter directement le RSI au graphique existant
        // addEMAToChart((FinanceChart*)m_chartViewer->getChart(), ema, startIndex, pointsToShow);
        
        m_chartViewer->updateViewPort(false, false);
    }
    else if (m_dataManager.hasValidData()) {
        // Si pas de graphique mais des données valides, créer le graphique complet
        updateChartDisplay(ViewPortMode::USE_CURRENT);
    }

    // Émettre le signal
    emit emaAdded(ema.id, ema.period);
    
    return ema.id;
}

bool ChartWidget::setEMAConfig(int id, const EMAInstance &config)
{
    EMAInstance* ema = findEMA(id);
    if (!ema) return false;

    // Mettre à jour la configuration
    *ema = config;

    // S'assurer que les nouvelles données EMA sont en cache
    ensureEMACached(config.period);

    // Émettre le signal
    emit emaChanged(id, config.period);

    // Mettre à jour le graphique
    if (m_dataManager.hasValidData())
        updateChartDisplay(ViewPortMode::USE_CURRENT);

    return true;
}

bool ChartWidget::removeEMA(int id)
{
    auto it = std::find_if(m_emaInstances.begin(), m_emaInstances.end(),
                         [id](const EMAInstance& ema) { return ema.id == id; });
    
    if (it == m_emaInstances.end()) {
        return false;
    }
    
    // Supprimer l'instance
    m_emaInstances.erase(it);
    
    // Émettre le signal
    emit emaRemoved(id);
    
    // Mettre à jour le graphique
    if (m_dataManager.hasValidData()) {
        updateChartDisplay(ViewPortMode::USE_CURRENT);
    }
    
    return true;
}

int ChartWidget::addStochastic(int fastKPeriod, int slowKPeriod, int slowDPeriod)
{
    if (fastKPeriod < 2) fastKPeriod = 2;  // Validation de base
    if (slowKPeriod < 2) slowKPeriod = 2;
    if (slowDPeriod < 2) slowDPeriod = 2;
    
    // Créer une nouvelle instance Stochastique
    StochasticInstance stochastic;
    stochastic.id = m_nextIndicatorId++;
    stochastic.fastKPeriod = fastKPeriod;
    stochastic.slowKPeriod = slowKPeriod;
    stochastic.slowDPeriod = slowDPeriod;
    
    // S'assurer que les données Stochastique sont en cache
    ensureStochasticCached(fastKPeriod, slowKPeriod, slowDPeriod);
    
    // Ajouter aux instances actives
    m_stochasticInstances.push_back(stochastic);


    // Si nous avons déjà un graphique et des données valides, ajouter directement l'indicateur
    if (m_dataManager.hasValidData() && m_chartViewer) {
        // Déterminer l'index de début et le nombre de points visibles actuellement
        int startIndex = m_chartViewer->getViewPortLeft();
        int pointsToShow = m_chartViewer->getViewPortWidth();

        // Ajouter directement le Stochastique au graphique existant
        // addStochasticToChart((FinanceChart*)m_chartViewer->getChart(), stochastic, startIndex, pointsToShow);

        // m_chartViewer->updateViewPort(false, false);
        
    }
    else if (m_dataManager.hasValidData()) {
        // Si pas de graphique mais des données valides, créer le graphique complet
    }
    updateChartDisplay(ViewPortMode::USE_CURRENT);

    // Émettre le signal
    emit stochasticAdded(stochastic.id, stochastic.fastKPeriod, stochastic.slowKPeriod, stochastic.slowDPeriod);
    
    return stochastic.id;
}

bool ChartWidget::setStochasticConfig(int id, const StochasticInstance& config)
{
    StochasticInstance* stochastic = findStochastic(id);
    if (!stochastic) return false;

    // Mettre à jour la configuration
    *stochastic = config;

    // S'assurer que les nouvelles données Stochastiques sont en cache
    ensureStochasticCached(config.fastKPeriod, config.slowKPeriod, config.slowDPeriod);

    // Émettre le signal
    emit stochasticChanged(id, config.fastKPeriod, config.slowKPeriod, config.slowDPeriod);

    // Mettre à jour le graphique
    if (m_dataManager.hasValidData())
        updateChartDisplay(ViewPortMode::USE_CURRENT);

    return true;
}

bool ChartWidget::removeStochastic(int id)
{
    auto it = std::find_if(m_stochasticInstances.begin(), m_stochasticInstances.end(),
                         [id](const StochasticInstance& stochastic) { return stochastic.id == id; });
    
    if (it == m_stochasticInstances.end()) {
        return false;
    }
    
    // Supprimer l'instance
    m_stochasticInstances.erase(it);
    
    // Émettre le signal
    emit stochasticRemoved(id);
    
    // Mettre à jour le graphique
    if (m_dataManager.hasValidData()) {
        updateChartDisplay(ViewPortMode::USE_CURRENT);
    }
    
    return true;
}

int ChartWidget::addATR(int period)
{
    if (period < 2) period = 2;  // Validation de base
    
    // Créer une nouvelle instance ATR
    ATRInstance atr;
    atr.id = m_nextIndicatorId++;
    atr.period = period;
    
    // S'assurer que les données ATR sont en cache
    ensureATRCached(period);
    
    // Ajouter aux instances actives
    m_atrInstances.push_back(atr);
    
    // Si nous avons déjà un graphique et des données valides, ajouter directement l'indicateur
    if (m_dataManager.hasValidData() && m_chartViewer) {
        // Déterminer l'index de début et le nombre de points visibles actuellement
        int startIndex = m_chartViewer->getViewPortLeft();
        int pointsToShow = m_chartViewer->getViewPortWidth();

        // Ajouter directement le Stochastique au graphique existant
        // m_renderer.addATRToChart(m_financeChart, atr, startIndex, pointsToShow);
        // std::cout << "On va appeler updateViewPort" << std::endl;

        m_chartViewer->updateViewPort(false, true);
    }
    else if (m_dataManager.hasValidData()) {
        // Si pas de graphique mais des données valides, créer le graphique complet
        updateChartDisplay(ViewPortMode::USE_CURRENT);
    }

    // Émettre le signal
    emit atrAdded(atr.id, atr.period);
    
    return atr.id;
}

bool ChartWidget::setATRConfig(int id, const ATRInstance &config)
{
    ATRInstance* atr = findATR(id);
    if (!atr) return false;

    // Mettre à jour la configuration
    *atr = config;

    // S'assurer que les nouvelles données ATR sont en cache
    ensureATRCached(config.period);

    // Émettre le signal
    emit atrChanged(id, config.period);

    // Mettre à jour le graphique
    if (m_dataManager.hasValidData())
        updateChartDisplay(ViewPortMode::USE_CURRENT);

    return true;
}

bool ChartWidget::removeATR(int id)
{
    auto it = std::find_if(m_atrInstances.begin(), m_atrInstances.end(),
                         [id](const ATRInstance& atr) { return atr.id == id; });
    
    if (it == m_atrInstances.end()) {
        return false;
    }
    
    // Supprimer l'instance
    m_atrInstances.erase(it);
    
    // Émettre le signal
    emit atrRemoved(id);
    
    // Mettre à jour le graphique
    if (m_dataManager.hasValidData()) {
        updateChartDisplay(ViewPortMode::USE_CURRENT);
    }
    
    return true;
}

RSIInstance* ChartWidget::findRSI(int id)
{
    auto it = std::find_if(m_rsiInstances.begin(), m_rsiInstances.end(),
                         [id](const RSIInstance& rsi) { return rsi.id == id; });
    
    if (it == m_rsiInstances.end()) {
        return nullptr;
    }
    
    return &(*it);
}

EMAInstance* ChartWidget::findEMA(int id)
{
    auto it = std::find_if(m_emaInstances.begin(), m_emaInstances.end(),
                         [id](const EMAInstance& ema) { return ema.id == id; });
    
    if (it == m_emaInstances.end()) {
        return nullptr;
    }
    
    return &(*it);
}

StochasticInstance* ChartWidget::findStochastic(int id)
{
    auto it = std::find_if(m_stochasticInstances.begin(), m_stochasticInstances.end(),
                         [id](const StochasticInstance& stochastic) { return stochastic.id == id; });
    
    if (it == m_stochasticInstances.end()) {
        return nullptr;
    }
    
    return &(*it);
}

ATRInstance* ChartWidget::findATR(int id)
{
    auto it = std::find_if(m_atrInstances.begin(), m_atrInstances.end(),
                         [id](const ATRInstance& atr) { return atr.id == id; });
    
    if (it == m_atrInstances.end()) {
        return nullptr;
    }
    
    return &(*it);
}

template<typename T, typename Container>
int ChartWidget::addIndicatorImpl(const T& configIn, Container& container)
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

void ChartWidget::ensureRSICached(int period)
{
    // Vérifier que la période RSI est en cache
    if (m_indicatorCache.isValid && !m_dataManager.getBacktestData()->getClose().empty()) {
        if (m_indicatorCache.rsi.find(period) == m_indicatorCache.rsi.end()) {
            // Calculer le RSI pour cette période
            std::vector<double>& rsiCache = m_indicatorCache.rsi[period];
            TechnicalIndicators::calculateRSI(m_dataManager.getBacktestData()->getClose(), period, rsiCache);
            qDebug() << "Calculé RSI avec période" << period;
        }
    }
}

void ChartWidget::ensureEMACached(int period)
{
    // Vérifier que la période EMA est en cache
    if (m_indicatorCache.isValid && !m_dataManager.getBacktestData()->getClose().empty()) {
        if (m_indicatorCache.ema.find(period) == m_indicatorCache.ema.end()) {
            // Calculer l'EMA pour cette période
            std::vector<double>& emaCache = m_indicatorCache.ema[period];
            TechnicalIndicators::calculateEMA(m_dataManager.getBacktestData()->getClose(), period, emaCache);
            qDebug() << "Calculé EMA avec période" << period;
        }
    }
}

void ChartWidget::ensureStochasticCached(int fastKPeriod, int slowKPeriod, int slowDPeriod)
{
    // Vérifier que les périodes Stochastic sont en cache
    if (m_indicatorCache.isValid && !m_dataManager.getBacktestData()->getClose().empty() && !m_dataManager.getBacktestData()->getHigh().empty() && !m_dataManager.getBacktestData()->getLow().empty()) {
        std::tuple<int, int, int> key = std::make_tuple(fastKPeriod, slowKPeriod, slowDPeriod);
        
        if (m_indicatorCache.stochastic.find(key) == m_indicatorCache.stochastic.end()) {
            // Calculer le Stochastic pour cette combinaison de périodes
            auto& cacheEntry = m_indicatorCache.stochastic[key];
            std::vector<double>& kValues = cacheEntry.first;
            std::vector<double>& dValues = cacheEntry.second;
            
            TechnicalIndicators::calculateStochastic(
                m_dataManager.getBacktestData()->getHigh(), m_dataManager.getBacktestData()->getLow(), m_dataManager.getBacktestData()->getClose(),
                fastKPeriod, slowKPeriod, slowDPeriod, kValues, dValues);
        }
    }
}

void ChartWidget::ensureATRCached(int period)
{
    // Vérifier que la période ATR est en cache
    if (m_indicatorCache.isValid && !m_dataManager.getBacktestData()->getClose().empty() && !m_dataManager.getBacktestData()->getHigh().empty() && !m_dataManager.getBacktestData()->getLow().empty()) {
        if (m_indicatorCache.atr.find(period) == m_indicatorCache.atr.end()) {
            std::vector<double>& atrCache = m_indicatorCache.atr[period];
            TechnicalIndicators::calculateATR(m_dataManager.getBacktestData()->getHigh(), m_dataManager.getBacktestData()->getLow(), m_dataManager.getBacktestData()->getClose(), period, atrCache);
        }
    }
}

void ChartWidget::updateIndicatorCache()
{
    if (!m_dataManager.hasValidData()) {
        qWarning() << "Tentative de mise à jour du cache d'indicateurs avec des données invalides";
        m_indicatorCache.isValid = false;
        return;
    }

    // Vider le cache existant
    m_indicatorCache.rsi.clear();
    m_indicatorCache.ema.clear();
    m_indicatorCache.stochastic.clear();
    m_indicatorCache.atr.clear();

    // Recueillir toutes les périodes RSI nécessaires
    std::set<int> rsiPeriods;
    std::set<int> emaPeriods;
    std::set<std::tuple<int, int, int>> stochasticParams;
    std::set<int> atrPeriods;

    // Ajouter les périodes de toutes les instances RSI actives
    for (const auto& rsi : m_rsiInstances) {
        rsiPeriods.insert(rsi.period);
    }

    // Ajouter les périodes de toutes les instances EMA actives
    for (const auto& ema : m_emaInstances) {
        emaPeriods.insert(ema.period);
    }

    // Ajouter les paramètres de toutes les instances Stochastique actives
    for (const auto& stoch : m_stochasticInstances) {
        stochasticParams.insert(std::make_tuple(stoch.fastKPeriod, stoch.slowKPeriod, stoch.slowDPeriod));
    }

    // Ajouter les périodes de toutes les instances ATR actives
    for (const auto& atr : m_atrInstances) {
        atrPeriods.insert(atr.period);
    }


    // Calculer tous les RSI nécessaires
    for (int period : rsiPeriods) {
        std::vector<double>& rsiCache = m_indicatorCache.rsi[period];
        TechnicalIndicators::calculateRSI(m_dataManager.getBacktestData()->getClose(), period, rsiCache);
    }

    // Calculer tous les EMA nécessaires
    for (int period : emaPeriods) {
        std::vector<double>& emaCache = m_indicatorCache.ema[period];
        TechnicalIndicators::calculateEMA(m_dataManager.getBacktestData()->getClose(), period, emaCache);
    }

    // Calculer tous les Stochastiques nécessaires
    for (const auto& params : stochasticParams) {
        int fastKPeriod = std::get<0>(params);
        int slowKPeriod = std::get<1>(params);
        int slowDPeriod = std::get<2>(params);
        
        auto& stochCache = m_indicatorCache.stochastic[params];
        std::vector<double>& kValues = stochCache.first;
        std::vector<double>& dValues = stochCache.second;
        
        TechnicalIndicators::calculateStochastic(
            m_dataManager.getBacktestData()->getHigh(), m_dataManager.getBacktestData()->getLow(), m_dataManager.getBacktestData()->getClose(),
            fastKPeriod, slowKPeriod, slowDPeriod, kValues, dValues);
    }

    // Calculer tous les ATR nécessaires
    for (int period : atrPeriods) {
        std::vector<double>& atrCache = m_indicatorCache.atr[period];
        TechnicalIndicators::calculateATR(m_dataManager.getBacktestData()->getHigh(), m_dataManager.getBacktestData()->getLow(), m_dataManager.getBacktestData()->getClose(), period, atrCache);
    }

    m_indicatorCache.isValid = true;
    qDebug() << "Cache d'indicateurs mis à jour avec" << m_dataManager.getBacktestData()->getClose().size() << "points";
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
    
    // Don't update chart while resizing - wait until the resize is finished
    if (m_isResizing) {
        // Just store the new size for later
        m_pendingResize = newSize;
        return;
    }
    
    // Update chart width if it's significant
    if (newSize.width() > 10 && std::abs(newSize.width() - m_config.chartWidth) > 50) {
        m_config.chartWidth = newSize.width() - 10;
        
        // Update only if we have valid data and the chart exists
        if (m_dataManager.hasValidData() && m_chartViewer) {
            m_chartViewer->updateViewPort(false, false);
        }
    }
}
