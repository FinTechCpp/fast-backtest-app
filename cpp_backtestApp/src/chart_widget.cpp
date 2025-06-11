#include "chart_widget.h"
#include <QVBoxLayout>
#include <QDebug>
#include <QMouseEvent>
#include <algorithm>
#include <cmath>
#include <set>

const std::array<ChartWidget::ChartTypeInfo, static_cast<size_t>(ChartWidget::ChartType::Count)> ChartWidget::s_chartTypeData = {{
    { ChartWidget::ChartType::CandleStick, "CandleStick" },
    { ChartWidget::ChartType::HeikinAshi, "HeikinAshi" },
    { ChartWidget::ChartType::OHLC, "OHLC" },
    { ChartWidget::ChartType::Close, "Close" }
}};

ChartWidget::ChartWidget(QWidget* parent)
    : QWidget(parent)
    , m_chartViewer(nullptr)
    , m_financeChart(nullptr)
    , m_rulerToolEnabled(false)
    , m_rulerFirstPointSelected(false)
    , m_rulerStartX(0)
    , m_rulerStartY(0)
    , m_rulerEndX(0)
    , m_rulerEndY(0)
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

ChartWidget::~ChartWidget()
{
    // Nettoyer le graphique
    if (m_financeChart) {
        delete m_financeChart;
        m_financeChart = nullptr;
    }
    
    qDebug() << "ChartWidget détruit";
}

void ChartWidget::setBacktestResults(const BacktestResults* results) {
    if (!results) {
        qWarning() << "Tentative de définir des résultats de backtest nuls";
        return;
    }

    m_backtestData = results->data;
    m_trades = results->stats.trades;

    prepareTimestampsCache();
    updateHeikinAshiCache();
    updateIndicatorCache();
    convertEquityCurve(results->stats.equityCurve, m_backtestData);

    // Mettre à jour le graphique si nous avons des données valides
    if (hasValidData()) {
        updateChartDisplay(false, false);
    }
}

void ChartWidget::setChartType(ChartType chartType)
{
    if (m_config.chartType == chartType)
        return; // Pas de changement, rien à faire

    m_config.chartType = chartType;

    // Si nous passons en mode HeikinAshi et que le cache n'est pas valide, le recalculer
    if (chartType == ChartType::HeikinAshi && !m_heikinAshiCache.isValid && hasValidData()) {
        updateHeikinAshiCache();
    }

    // Si nous avons déjà des données, mettre à jour le graphique
    if (hasValidData())
        updateChartDisplay(true, true);
}

bool ChartWidget::updateChartDisplay(bool useViewport, bool preserveViewport) {
    if (!hasValidData() || !m_chartViewer) {
        return false;
    }
    
    try {
        // Sauvegarder l'état actuel du viewport si nécessaire
        double currentLeft = 0.0;
        double currentWidth = 1.0;
        
        if (preserveViewport && m_chartViewer) {
            currentLeft = m_chartViewer->getViewPortLeft();
            currentWidth = m_chartViewer->getViewPortWidth();
        }
        
        // Nettoyer le graphique précédent
        if (m_financeChart) {
            delete m_financeChart;
            m_financeChart = nullptr;
        }
        
        // Déterminer les indices de début et fin basés sur le viewport
        int startIndex = 0;
        int pointsToShow = static_cast<int>(m_timestampsCache.size());
        
        if (useViewport) {
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
        
        // Extraire les données à afficher
        DoubleArray timeStamps;
        DoubleArray openData, highData, lowData, closeData, volumeData;
        
        if (startIndex < static_cast<int>(m_timestampsCache.size())) {
            timeStamps = DoubleArray(&m_timestampsCache[startIndex], pointsToShow);
            volumeData = DoubleArray(&m_backtestData->getVolume()[startIndex], pointsToShow);
            
            // Déterminer quel type de données afficher (standard ou Heikin-Ashi)
            if (m_config.chartType == ChartType::HeikinAshi) {
                // Vérifier si le cache est valide
                if (!m_heikinAshiCache.isValid) {
                    updateHeikinAshiCache();
                }
                
                // Utiliser les données Heikin-Ashi
                openData = DoubleArray(&m_heikinAshiCache.open[startIndex], pointsToShow);
                highData = DoubleArray(&m_heikinAshiCache.high[startIndex], pointsToShow);
                lowData = DoubleArray(&m_heikinAshiCache.low[startIndex], pointsToShow);
                closeData = DoubleArray(&m_heikinAshiCache.close[startIndex], pointsToShow);
            } else {
                // Utiliser les données OHLC standards
                openData = DoubleArray(&m_backtestData->getOpen()[startIndex], pointsToShow);
                highData = DoubleArray(&m_backtestData->getHigh()[startIndex], pointsToShow);
                lowData = DoubleArray(&m_backtestData->getLow()[startIndex], pointsToShow);
                closeData = DoubleArray(&m_backtestData->getClose()[startIndex], pointsToShow);
            }
        } else {
            // Pas de données à afficher
            return false;
        }
        
        // Créer le graphique
        m_financeChart = drawChart(timeStamps, highData, lowData, openData, closeData, 
                                  volumeData, m_config.chartWidth);
        
        // Configurer le viewport
        if (!useViewport) {
            // Afficher toutes les données
            m_chartViewer->setViewPortWidth(1.0);
            m_chartViewer->setViewPortLeft(0);
        } else if (preserveViewport) {
            // Restaurer le viewport précédent
            m_chartViewer->setViewPortLeft(currentLeft);
            m_chartViewer->setViewPortWidth(currentWidth);
        }
        
        // Mettre à jour l'affichage
        m_chartViewer->updateViewPort(true, false);
        
        // Émettre un signal si c'est une création initiale
        if (!useViewport) {
            emit chartCreated();
        }
        
        return true;
    } catch (const std::exception& e) {
        qCritical() << "Erreur lors de la création/mise à jour du graphique:" << e.what();
        return false;
    }
}

void ChartWidget::clearChart()
{
    // Nettoyer le graphique
    if (m_financeChart) {
        delete m_financeChart;
        m_financeChart = nullptr;
    }
    
    // Réinitialiser le viewer
    if (m_chartViewer) {
        m_chartViewer->setChart(nullptr);
    }
    
    // Effacer les données
    // m_priceData = PriceData();
    // m_tradeData = TradeData();
    m_equityData = EquityData();
}

bool ChartWidget::hasValidData() const {
    if (!m_backtestData)
        return false;

    const auto& open = m_backtestData->getOpen();
    const auto& high = m_backtestData->getHigh();
    const auto& low = m_backtestData->getLow();
    const auto& close = m_backtestData->getClose();

    size_t size = open.size();
    return size > 0 &&
           high.size() == size &&
           low.size() == size &&
           close.size() == size &&
           m_timestampsCache.size() == size;
}

void ChartWidget::onViewPortChanged()
{
    // Redessiner le graphique avec le nouveau viewport
    updateChartDisplay(true, true);

    // Émettre un signal pour indiquer que le viewport a changé
    // emit viewPortChanged();
}

void ChartWidget::onMouseMovePlotArea(QMouseEvent* event)
{
    if (!m_financeChart || !m_chartViewer) {
        return;
    }
    
    int mouseX = m_chartViewer->getPlotAreaMouseX();
    int mouseY = m_chartViewer->getPlotAreaMouseY();

    // Si l'outil règle est activé et que le premier point a été sélectionné
    if (m_rulerToolEnabled && m_rulerFirstPointSelected) {
        // Mettre à jour le point final avec la position actuelle de la souris
        m_rulerEndX = m_chartViewer->getPlotAreaMouseX();
        m_rulerEndY = m_chartViewer->getPlotAreaMouseY();
    }
    
    // Comportement normal de suivi du graphique
    trackFinance(m_financeChart, mouseX);
    
    // Récupérer les informations sur le point
    if (m_financeChart->getChartCount() > 1) {
        XYChart* mainChart = (XYChart*)m_financeChart->getChart(1);
        double xValue = mainChart->getNearestXValue(mouseX);
        
        // Trouver l'indice correspondant
        int dataIndex = -1;
        for (int i = 0; i < (int)m_timestampsCache.size(); ++i) {
            if (fabs(m_timestampsCache[i] - xValue) < 1e-6) {
                dataIndex = i;
                break;
            }
        }

        if (dataIndex >= 0 && dataIndex < (int)m_backtestData->getClose().size()) {
            // Émettre un signal avec les informations du point
            emit mouseOverPoint(m_timestampsCache[dataIndex], m_backtestData->getClose()[dataIndex]);
        }
    }
    
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
            
            // Initialiser aussi le point final pour éviter des valeurs incorrectes lors du dessin
            m_rulerEndX = m_rulerStartX;
            m_rulerEndY = m_rulerStartY;
        } else {
            // Si c'est le deuxième clic, enregistrer le point final et réinitialiser
            m_rulerEndX = m_chartViewer->getPlotAreaMouseX();
            m_rulerEndY = m_chartViewer->getPlotAreaMouseY();
            m_rulerFirstPointSelected = false;
        }
        
        // Mettre à jour l'affichage
        if (m_chartViewer->getChart()) {
            m_chartViewer->updateDisplay();
        }
    }
}


//-----Ruler Tool Implementation-----
void ChartWidget::drawRuler(MultiChart* m, int mouseX, int mouseY, DrawArea* d)
{   
    // Vérifier que le chart est valide et qu'il y a au moins un graphique
    if (!m || m->getChartCount() == 0) return;
    
    // Obtenir le premier graphique XY (graphique principal)
    XYChart* c = (XYChart*)m->getChart(1);
    if (!c) return;

    // Obtenir les indices correspondant aux positions du curseur
    double xValueStartIndex = c->getNearestXValue(m_rulerStartX);
    double xValueEndIndex = c->getNearestXValue(m_rulerEndX);
    double yValueStart = c->getYValue(m_rulerStartY);
    double yValueEnd = c->getYValue(m_rulerEndY);

    // Récupérer les timestamps formattés pour l'affichage
    const char* startTimeStr = c->xAxis()->getFormattedLabel(xValueStartIndex, "yyyy-mm-dd hh:nn:ss");
    const char* endTimeStr = c->xAxis()->getFormattedLabel(xValueEndIndex, "yyyy-mm-dd hh:nn:ss");

    // Convertir les indices en timestamps réels
    double startTimestamp = 0;
    double endTimestamp = 0;
    
    // Trouver les timestamps correspondants aux indices
    int startIdx = static_cast<int>(std::round(xValueStartIndex));
    int endIdx = static_cast<int>(std::round(xValueEndIndex));
    
    // Assurer que les indices sont dans les limites du tableau
    if (startIdx >= 0 && startIdx < static_cast<int>(m_timestampsCache.size()) &&
        endIdx >= 0 && endIdx < static_cast<int>(m_timestampsCache.size())) {
        startTimestamp = m_timestampsCache[startIdx];
        endTimestamp = m_timestampsCache[endIdx];
    } else {
        // Indices hors limites - utiliser une valeur par défaut
        std::cout << "Indices hors limites : " << startIdx << ", " << endIdx << std::endl;
        return;
    }
    
    // Calculer la différence de temps en secondes
    double deltaX = fabs(startTimestamp - endTimestamp);
    double deltaY = yValueEnd - yValueStart;

    // Définir la couleur du rectangle en fonction de deltaY
    int deltaColor = (deltaY < 0) ? 0xFF0000 : 0x008800;
    int alpha = 0xCC; // Niveau de transparence (assez particulier avec chartdirector, j'ai pas tout compris)
    int finalColor =  (alpha << 24) | deltaColor; // Couleur avec alpha (résultat en ARGB : 0xCCFF0000 ou 0xCC008800)

    // Texte pour deltaX (au-dessus du rectangle)
    char bufferX[50];
    
    // Calculer la durée réelle en secondes
    int totalSeconds = static_cast<int>(deltaX);
    int hours = totalSeconds / 3600;
    int minutes = (totalSeconds % 3600) / 60;
    int seconds = totalSeconds % 60;
    
    // Formater avec le signe approprié et adapter le format selon la durée
    if (hours > 0) {
        sprintf(bufferX, "%02dh%02dm%02ds", hours, minutes, seconds);
    } else if (minutes > 0) {
        sprintf(bufferX, "%02dm%02ds", minutes, seconds);
    } else {
        sprintf(bufferX, "%02ds", seconds);
    }
        
    // Texte pour deltaY (à droite du rectangle)
    char bufferY[50];
    sprintf(bufferY, "%.5f %s", deltaY, "$");
    
    // Ajouter le % de variation pour le deltaY si applicable
    if (yValueStart != 0) {  // Éviter division par zéro
        double percentChange = (deltaY / yValueStart) * 100.0;
        sprintf(bufferY, "%+.2f \n(%.2f%%)", deltaY, percentChange);
    }

    // Dessiner le rectangle entre les deux points
    d->rect(m_rulerStartX, m_rulerStartY, m_rulerEndX, m_rulerEndY, deltaColor, finalColor);

    // Position pour le texte deltaX (au-dessus du rectangle)
    int textXPosX = (m_rulerStartX + m_rulerEndX) / 2; // Centre du rectangle
    int textXPosY = std::min(m_rulerStartY, m_rulerEndY) - 15; // Au-dessus
    
    // Position pour le texte deltaY (à droite du rectangle)
    int textYPosX = std::max(m_rulerStartX, m_rulerEndX) + 10; // À droite
    int textYPosY = (m_rulerStartY + m_rulerEndY) / 2; // Milieu vertical
    
    // Créer et afficher le texte pour deltaX
    TTFText* tForXDelta = d->text(bufferX, "Arial", 12);
    tForXDelta->draw(textXPosX, textXPosY, deltaColor, Chart::Bottom);
    tForXDelta->destroy();
    
    // Créer et afficher le texte pour deltaY
    TTFText* tForYDelta = d->text(bufferY, "Arial", 12);
    tForYDelta->draw(textYPosX, textYPosY, deltaColor, Chart::Left);
    tForYDelta->destroy();
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
//-----End Ruler Tool Implementation-----


//-----Indicators Implementation-----
int ChartWidget::addRSI(int period)
{
    if (period < 2) period = 2;  // Validation de base
    
    // Créer une nouvelle instance RSI
    RSIInstance rsi;
    rsi.id = m_nextRSIId++;
    rsi.period = period;
    
    // S'assurer que les données RSI sont en cache
    ensureRSICached(period);
    
    // Ajouter aux instances actives
    m_rsiInstances.push_back(rsi);
    
    // Émettre le signal
    emit rsiAdded(rsi.id, period);
    
    // Mettre à jour le graphique
    if (hasValidData())
        updateChartDisplay(true, true);
    
    return rsi.id;
}

bool ChartWidget::setRSIConfig(int id, const RSIInstance &config)
{
    RSIInstance* rsi = findRSI(id);
    if (!rsi) return false;

    // Mettre à jour la configuration
    *rsi = config;

    // S'assurer que les nouvelles données RSI sont en cache
    ensureRSICached(config.period);

    // Émettre le signal
    emit rsiChanged(id, config.period);

    // Mettre à jour le graphique
    if (hasValidData())
        updateChartDisplay(true, true);

    return true;
}

bool ChartWidget::removeRSI(int id)
{
    auto it = std::find_if(m_rsiInstances.begin(), m_rsiInstances.end(),
                         [id](const RSIInstance& rsi) { return rsi.id == id; });
    
    if (it == m_rsiInstances.end()) {
        return false;
    }
    
    // Supprimer l'instance
    m_rsiInstances.erase(it);
    
    // Émettre le signal
    emit rsiRemoved(id);
    
    // Mettre à jour le graphique
    if (hasValidData() && m_financeChart) {
        updateChartDisplay(true, true);
    }
    
    return true;
}

int ChartWidget::addEMA(int period)
{
    if (period < 2) period = 2;  // Validation de base
    
    // Créer une nouvelle instance EMA
    EMAInstance ema;
    ema.id = m_nextEMAId++;
    ema.period = period;
    
    // S'assurer que les données EMA sont en cache
    ensureEMACached(period);
    
    // Ajouter aux instances actives
    m_emaInstances.push_back(ema);
    
    // Émettre le signal
    emit emaAdded(ema.id, period);
    
    // Mettre à jour le graphique
    if (hasValidData()) {
        updateChartDisplay(true, true);
    }
    
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
    if (hasValidData())
        updateChartDisplay(true, true);

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
    if (hasValidData() && m_financeChart) {
        updateChartDisplay(true, true);
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
    stochastic.id = m_nextStochasticId++;
    stochastic.fastKPeriod = fastKPeriod;
    stochastic.slowKPeriod = slowKPeriod;
    stochastic.slowDPeriod = slowDPeriod;
    
    // S'assurer que les données Stochastique sont en cache
    ensureStochasticCached(fastKPeriod, slowKPeriod, slowDPeriod);
    
    // Ajouter aux instances actives
    m_stochasticInstances.push_back(stochastic);
    
    // Émettre le signal
    emit stochasticAdded(stochastic.id, fastKPeriod, slowKPeriod, slowDPeriod);
    
    // Mettre à jour le graphique
    if (hasValidData()) {
        updateChartDisplay(true, true);
    }
    
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
    if (hasValidData())
        updateChartDisplay(true, true);

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
    if (hasValidData() && m_financeChart) {
        updateChartDisplay(true, true);
    }
    
    return true;
}

int ChartWidget::addATR(int period)
{
    if (period < 2) period = 2;  // Validation de base
    
    // Créer une nouvelle instance ATR
    ATRInstance atr;
    atr.id = m_nextATRId++;
    atr.period = period;
    
    // S'assurer que les données ATR sont en cache
    ensureATRCached(period);
    
    // Ajouter aux instances actives
    m_atrInstances.push_back(atr);
    
    // Émettre le signal
    emit atrAdded(atr.id, period);
    
    // Mettre à jour le graphique
    if (hasValidData()) {
        updateChartDisplay(true, true);
    }
    
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
    if (hasValidData())
        updateChartDisplay(true, true);

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
    if (hasValidData() && m_financeChart) {
        updateChartDisplay(true, true);
    }
    
    return true;
}

ChartWidget::RSIInstance* ChartWidget::findRSI(int id)
{
    auto it = std::find_if(m_rsiInstances.begin(), m_rsiInstances.end(),
                         [id](const RSIInstance& rsi) { return rsi.id == id; });
    
    if (it == m_rsiInstances.end()) {
        return nullptr;
    }
    
    return &(*it);
}

ChartWidget::EMAInstance* ChartWidget::findEMA(int id)
{
    auto it = std::find_if(m_emaInstances.begin(), m_emaInstances.end(),
                         [id](const EMAInstance& ema) { return ema.id == id; });
    
    if (it == m_emaInstances.end()) {
        return nullptr;
    }
    
    return &(*it);
}

ChartWidget::StochasticInstance* ChartWidget::findStochastic(int id)
{
    auto it = std::find_if(m_stochasticInstances.begin(), m_stochasticInstances.end(),
                         [id](const StochasticInstance& stochastic) { return stochastic.id == id; });
    
    if (it == m_stochasticInstances.end()) {
        return nullptr;
    }
    
    return &(*it);
}

ChartWidget::ATRInstance* ChartWidget::findATR(int id)
{
    auto it = std::find_if(m_atrInstances.begin(), m_atrInstances.end(),
                         [id](const ATRInstance& atr) { return atr.id == id; });
    
    if (it == m_atrInstances.end()) {
        return nullptr;
    }
    
    return &(*it);
}

void ChartWidget::ensureRSICached(int period)
{
    // Vérifier que la période RSI est en cache
    if (m_indicatorCache.isValid && !m_backtestData->getClose().empty()) {
        if (m_indicatorCache.rsi.find(period) == m_indicatorCache.rsi.end()) {
            // Calculer le RSI pour cette période
            std::vector<double>& rsiCache = m_indicatorCache.rsi[period];
            TechnicalIndicators::calculateRSI(m_backtestData->getClose(), period, rsiCache);
            qDebug() << "Calculé RSI avec période" << period;
        }
    }
}

void ChartWidget::ensureEMACached(int period)
{
    // Vérifier que la période EMA est en cache
    if (m_indicatorCache.isValid && !m_backtestData->getClose().empty()) {
        if (m_indicatorCache.ema.find(period) == m_indicatorCache.ema.end()) {
            // Calculer l'EMA pour cette période
            std::vector<double>& emaCache = m_indicatorCache.ema[period];
            TechnicalIndicators::calculateEMA(m_backtestData->getClose(), period, emaCache);
            qDebug() << "Calculé EMA avec période" << period;
        }
    }
}

void ChartWidget::ensureStochasticCached(int fastKPeriod, int slowKPeriod, int slowDPeriod)
{
    // Vérifier que les périodes Stochastic sont en cache
    if (m_indicatorCache.isValid && !m_backtestData->getClose().empty() && !m_backtestData->getHigh().empty() && !m_backtestData->getLow().empty()) {
        std::tuple<int, int, int> key = std::make_tuple(fastKPeriod, slowKPeriod, slowDPeriod);
        
        if (m_indicatorCache.stochastic.find(key) == m_indicatorCache.stochastic.end()) {
            // Calculer le Stochastic pour cette combinaison de périodes
            auto& cacheEntry = m_indicatorCache.stochastic[key];
            std::vector<double>& kValues = cacheEntry.first;
            std::vector<double>& dValues = cacheEntry.second;
            
            TechnicalIndicators::calculateStochastic(
                m_backtestData->getHigh(), m_backtestData->getLow(), m_backtestData->getClose(),
                fastKPeriod, slowKPeriod, slowDPeriod, kValues, dValues);
                
            qDebug() << "Calculé Stochastic avec périodes fastK:" << fastKPeriod 
                     << "slowK:" << slowKPeriod << "slowD:" << slowDPeriod;
        }
    }
}

void ChartWidget::ensureATRCached(int period)
{
    // Vérifier que la période ATR est en cache
    if (m_indicatorCache.isValid && !m_backtestData->getClose().empty() && !m_backtestData->getHigh().empty() && !m_backtestData->getLow().empty()) {
        if (m_indicatorCache.atr.find(period) == m_indicatorCache.atr.end()) {
            // Calculer l'ATR pour cette période
            std::vector<double>& atrCache = m_indicatorCache.atr[period];
            TechnicalIndicators::calculateATR(m_backtestData->getHigh(), m_backtestData->getLow(), m_backtestData->getClose(), period, atrCache);
            qDebug() << "Calculé ATR avec période" << period;
        }
    }
}

void ChartWidget::updateHeikinAshiCache()
{
    if (!hasValidData()) {
        qWarning() << "Tentative de mise à jour du cache Heikin-Ashi avec des données vides";
        m_heikinAshiCache.isValid = false;
        return;
    }

    TechnicalIndicators::calculateHeikinAshi(
        m_backtestData->getOpen(), 
        m_backtestData->getHigh(), 
        m_backtestData->getLow(),
        m_backtestData->getClose(),
        m_heikinAshiCache.open,
        m_heikinAshiCache.high,
        m_heikinAshiCache.low,
        m_heikinAshiCache.close
    );
    
    m_heikinAshiCache.isValid = true;
}

void ChartWidget::updateIndicatorCache()
{
    if (!hasValidData()) {
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
        TechnicalIndicators::calculateRSI(m_backtestData->getClose(), period, rsiCache);
    }

    // Calculer tous les EMA nécessaires
    for (int period : emaPeriods) {
        std::vector<double>& emaCache = m_indicatorCache.ema[period];
        TechnicalIndicators::calculateEMA(m_backtestData->getClose(), period, emaCache);
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
            m_backtestData->getHigh(), m_backtestData->getLow(), m_backtestData->getClose(),
            fastKPeriod, slowKPeriod, slowDPeriod, kValues, dValues);
    }

    // Calculer tous les ATR nécessaires
    for (int period : atrPeriods) {
        std::vector<double>& atrCache = m_indicatorCache.atr[period];
        TechnicalIndicators::calculateATR(m_backtestData->getHigh(), m_backtestData->getLow(), m_backtestData->getClose(), period, atrCache);
    }

    m_indicatorCache.isValid = true;
    qDebug() << "Cache d'indicateurs mis à jour avec" << m_backtestData->getClose().size() << "points";
}

void ChartWidget::convertEquityCurve(const std::vector<double>& equityCurve, 
                                    const std::shared_ptr<be::Data>& data) {
    if (equityCurve.empty() || !data) {
        qWarning() << "Courbe d'équité vide ou données de prix invalides";
        return;
    }
    
    size_t numPoints = equityCurve.size();
    size_t numBars = data->size();
    
    // Réinitialiser les données d'équité
    m_equityData = EquityData();
    
    // Valider les tailles
    if (numPoints != numBars) {
        qWarning() << "Tailles incompatibles: equityCurve:" << numPoints << "data:" << numBars;
        return;
    }
    
    // Préallouer pour le pire cas
    m_equityData.timestamps.reserve(numPoints);
    m_equityData.equity_values.reserve(numPoints);
    
    // Compresser les données en ne gardant que les points où l'équité change
    double lastValue = equityCurve[0];
    
    // Toujours ajouter le premier point
    m_equityData.timestamps.push_back(dateToChartTimestamp(data->at(0).date));
    m_equityData.equity_values.push_back(lastValue);
    
    // Parcourir le reste des points
    for (size_t i = 1; i < numPoints; ++i) {
        double currentValue = equityCurve[i];
        
        // Si la valeur a changé ou si c'est le dernier point, l'ajouter
        if (std::abs(currentValue - lastValue) > 1e-10 || i == numPoints - 1) {
            m_equityData.timestamps.push_back(dateToChartTimestamp(data->at(i).date));
            m_equityData.equity_values.push_back(currentValue);
            lastValue = currentValue;
        }
    }
    
    // Calculer le drawdown (inchangé)
    if (!m_equityData.equity_values.empty()) {
        m_equityData.drawdown.resize(m_equityData.equity_values.size());
        double peak = m_equityData.equity_values[0];
        
        for (size_t i = 0; i < m_equityData.equity_values.size(); ++i) {
            if (m_equityData.equity_values[i] > peak) {
                peak = m_equityData.equity_values[i];
            }
            double dd = (peak - m_equityData.equity_values[i]) / peak * 100.0;
            m_equityData.drawdown[i] = dd;
        }
    }
    
    qInfo() << "Courbe d'équité compressée:" << numPoints << "points réduits à" 
            << m_equityData.timestamps.size() << "points significatifs";
}

double ChartWidget::dateToChartTimestamp(const be::Date& date) {
    return Chart::chartTime(date.getYear(), date.getMonth(), date.getDay(), date.getHour(), date.getMinute(), date.getSecond());
}

void ChartWidget::prepareTimestampsCache() {
    if (!m_backtestData || m_backtestData->size() == 0) {
        m_timestampsCache.clear();
        return;
    }
    
    const auto& dates = m_backtestData->getDates();
    size_t dataSize = dates.size();
    
    // Réserver la capacité et convertir toutes les dates en timestamps
    m_timestampsCache.clear();
    m_timestampsCache.reserve(dataSize);
    
    for (const auto& date : dates) {
        double timestamp = dateToChartTimestamp(date);
        m_timestampsCache.push_back(timestamp);
    }
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
//         if (hasValidData() && m_chartViewer) {
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
    if (newSize.width() > 10 && std::abs(newSize.width() - m_config.chartWidth) > 5) {
        m_config.chartWidth = newSize.width() - 10;
        
        // Update only if we have valid data and the chart exists
        if (hasValidData() && m_chartViewer && m_financeChart) {
            updateChartDisplay(true, true);
        }
    }
}

DoubleArray ChartWidget::vectorToDoubleArray(const std::vector<double>& vec) {
    if (vec.empty())
        return DoubleArray(nullptr, 0);
    return DoubleArray(vec.data(), static_cast<int>(vec.size()));
}

FinanceChart* ChartWidget::drawChart(
    const DoubleArray& timestamps, 
    const DoubleArray& highData, 
    const DoubleArray& lowData, 
    const DoubleArray& openData, 
    const DoubleArray& closeData,
    const DoubleArray& volumeData,
    int chartWidth)
{
    // Nettoyer le graphique précédent
    if (m_financeChart) {
        delete m_financeChart;
        m_financeChart = nullptr;
    }
    
    // Créer un nouveau graphique
    FinanceChart* c = new FinanceChart(chartWidth);

    c->setPlotAreaStyle(0xE2F4FF, 0xCC999999, 0xCC999999, 0xCC999999, 0xCC999999);
    c->setDateLabelFormat(
        "{value|yyyy}", 
        "{value|yyyy-mm-dd}", 
        "{value|mm-dd}", 
        "{value|yyyy-mm-dd}", 
        "{value|mm-dd}", 
        "{value|yyyy-mm-dd hh:nn:ss}", 
        "{value|hh:nn:ss}"
    );    c->setDateLabelSpacing(50); // Espacement des étiquettes de date

    // Configurer les données
    c->setData(timestamps, highData, lowData, openData, closeData, volumeData, 0);

    // METHODE DE GITAN : Cacher la légende par défaut de ChartDirector en la rendant transparente 
    c->setLegendStyle("normal", 8, Chart::Transparent, Chart::Transparent);
    
    // Ajouter le titre du graphique
    std::string chartTypeStr = chartTypeToString(m_config.chartType).toStdString();
    std::string title = "Graphique de trading (" + chartTypeStr + ") - " + 
                       std::to_string(timestamps.len) + " points";
    c->addTitle(title.c_str());
    
    // Hauteurs pour les différentes parties du graphique
    int mainChartHeight = 400;  // Hauteur du graphique principal
    int volumeHeight = 100;     // Hauteur du graphique de volume

    // Déterminer l'index de début et de fin des données actuellement affichées
    // timestamps contient uniquement les bougies visibles
    int startIndex = 0;  // L'index de début des données visibles par rapport au dataset complet
    
    // Si nous sommes en mode viewport (zoom/déplacement), déterminer l'index de début
    if (timestamps.len < (int)m_timestampsCache.size()) {
        double firstVisibleTimestamp = timestamps[0];
        
        // Trouver l'index correspondant dans le dataset complet
        auto it = std::lower_bound(
            m_timestampsCache.begin(),
            m_timestampsCache.end(),
            firstVisibleTimestamp,
            [](double a, double b) { return a < b - 0.001; }
        );
        startIndex = std::distance(m_timestampsCache.begin(), it);
    }
    
    // 1. Ajouter la courbe d'équité en haut si disponible
    addEquityCurveSection(c, timestamps, startIndex);
    
    // 2. Ajouter le graphique principal
    c->addMainChart(mainChartHeight);

    // Ajouter le type de graphique approprié selon le type actuel
    if (m_config.chartType == ChartType::CandleStick || m_config.chartType == ChartType::HeikinAshi) {
        c->addCandleStick(0x00CC00, 0xFF3333); // Vert/Rouge pour les bougies
    } else if (m_config.chartType == ChartType::OHLC) {
        c->addHLOC(0x00CC00, 0xFF3333); // Vert/Rouge pour les barres OHLC
    } else if (m_config.chartType == ChartType::Close) {
        c->addCloseLine(0x000088); // Ligne bleue pour le prix de clôture
    }

    // Ajouter tous les RSI actifs
    for (const auto& rsi : m_rsiInstances) {
        if (rsi.visible) {
            addRSIToChart(c, rsi, startIndex, timestamps.len);
        }
    }

    // Ajouter tous les EMA actifs
    for (const auto& ema : m_emaInstances) {
        if (ema.visible) {
            addEMAToChart(c, ema, startIndex, timestamps.len);
        }
    }

    // Ajouter tous les Stochastiques actifs
    for (const auto& stochastic : m_stochasticInstances) {
        if (stochastic.visible) {
            addStochasticToChart(c, stochastic, startIndex, timestamps.len);
        }
    }

    // Ajouter tous les ATR actifs
    for (const auto& atr : m_atrInstances) {
        if (atr.visible) {
            addATRToChart(c, atr, startIndex, timestamps.len);
        }
    }

    // 4. Ajouter les trades si disponibles
    addTradeMarkers(c, timestamps, startIndex);
    
    // Mettre à jour le graphique dans le viewer
    m_chartViewer->setChart(c);
    // m_chartViewer->setFullRange("x", 0, timestamps.len - 1);

    
    return c;
}

FinanceChart *ChartWidget::initializeChart(int chartWidth)
{
    return nullptr;
}

void ChartWidget::addEquityCurveSection(FinanceChart *chart, const DoubleArray &timestamps, int startIndex)
{
    if (m_equityData.equity_values.empty() || timestamps.len == 0)
        return; // Pas de données d'équité ou pas de bougies visibles

    int equityHeight = 120;     // Hauteur du graphique d'équité

    // Obtenir la plage de temps visible
    double visibleStartTime = timestamps[0];
    double visibleEndTime = timestamps[timestamps.len - 1];
    
    // Créer les vecteurs pour les données d'équité interpolées
    std::vector<double> interpolatedTimes;
    std::vector<double> interpolatedValues;
    std::vector<double> colorValues;  // Pour stocker les valeurs de couleur
    
    interpolatedTimes.reserve(timestamps.len);
    interpolatedValues.reserve(timestamps.len);
    colorValues.reserve(timestamps.len - 1);  // Un segment de moins que de points
    
    // Trouver le premier point d'équité qui précède ou correspond à visibleStartTime
    size_t equityIndex = 0;
    while (equityIndex + 1 < m_equityData.timestamps.size() && 
           m_equityData.timestamps[equityIndex + 1] < visibleStartTime) {
        equityIndex++;
    }
    
    // Valeur d'équité au début de la fenêtre visible
    double currentEquityValue = m_equityData.equity_values[equityIndex];
    
    // Pour chaque timestamp visible, interpoler la valeur d'équité
    for (int i = 0; i < timestamps.len; ++i) {
        double currentTime = timestamps[i];
        
        // Avancer dans les données d'équité si nécessaire
        while (equityIndex + 1 < m_equityData.timestamps.size() && 
               m_equityData.timestamps[equityIndex + 1] <= currentTime) {
            equityIndex++;
            currentEquityValue = m_equityData.equity_values[equityIndex];
        }
        
        // Ajouter le point interpolé
        interpolatedTimes.push_back(i); // Utiliser l'index comme position X
        interpolatedValues.push_back(currentEquityValue);
        
        // Déterminer la couleur du segment (pour tous sauf le premier point)
        if (i > 0) {
            double prev = interpolatedValues[i-1];
            double curr = currentEquityValue;
            double diff = curr - prev;
            
            // Définir la valeur de couleur basée sur la direction
            if (std::abs(diff) < 1e-10) {
                // Constant (gris)
                colorValues.push_back(0);  
            } else if (diff > 0) {
                // Ascendant (vert)
                colorValues.push_back(1);
            } else {
                // Descendant (rouge)
                colorValues.push_back(2);
            }
        }
    }
    
    // Convertir en DoubleArray
    DoubleArray equityTimes = vectorToDoubleArray(interpolatedTimes);
    DoubleArray equityValues = vectorToDoubleArray(interpolatedValues);
    
    // Ajouter l'indicateur pour l'equity curve
    XYChart* equityChart = chart->addIndicator(equityHeight);
    
    // Configuration du titre et des libellés
    equityChart->yAxis()->setTitle("Capital");
    equityChart->xAxis()->setColors(Chart::Transparent); // Masquer l'axe X
    
    // Définir les couleurs pour les segments
    int constColor = 0x999999;  // gris pour constant
    int upColor = 0x53DD00;     // Vert pour ascendant
    int downColor = 0xFF0000;   // Rouge pour descendant
    
    // Créer trois couches de stepline séparées, une pour chaque couleur
    StepLineLayer* equityLineLayer = equityChart->addStepLineLayer(equityValues, Chart::Transparent, "Equity");
    StepLineLayer* constantLayer = equityChart->addStepLineLayer();
    StepLineLayer* upLayer = equityChart->addStepLineLayer();
    StepLineLayer* downLayer = equityChart->addStepLineLayer(); 
    
    constantLayer->setFastLineMode(true);
    upLayer->setFastLineMode(true);
    downLayer->setFastLineMode(true);

    // Créer une ligne horizontale pour le cash initial
    double initialCash = m_equityData.equity_values.front();

    Mark* mark = equityChart->yAxis()->addMark(m_equityData.equity_values.front(), 0x000000,"Initial Cash");
    mark->setLineWidth(2);
    mark->setMarkColor(equityChart->dashLineColor(0x000000), 0xffffff);
    mark->setAlignment(Chart::Left);
    mark->setBackground(0x000000, 0xffffff, 1);
    equityChart->addInterLineLayer(equityLineLayer->getLine(), mark->getLine(), 0xCC53DD00, 0xCCff0000);

    // Configurer l'alignement des steplines (début de chaque période)
    constantLayer->setAlignment(Chart::Left);
    upLayer->setAlignment(Chart::Left);
    downLayer->setAlignment(Chart::Left);
    
    // Création des ensembles de données pour chaque type de segment
    std::vector<std::vector<double>> segmentX(3);
    std::vector<std::vector<double>> segmentY(3);
    
    // Parcourir les points et créer des segments colorés
    for (size_t i = 1; i < interpolatedValues.size(); ++i) {
        int colorIndex = static_cast<int>(colorValues[i-1]);
        
        // Ajouter le point de début et de fin pour ce segment
        segmentX[colorIndex].push_back(interpolatedTimes[i-1]);
        segmentY[colorIndex].push_back(interpolatedValues[i-1]);
        
        segmentX[colorIndex].push_back(interpolatedTimes[i]);
        segmentY[colorIndex].push_back(interpolatedValues[i]);
        
        // Ajouter un NoValue pour séparer les segments non-contigus de même couleur
        segmentX[colorIndex].push_back(Chart::NoValue);
        segmentY[colorIndex].push_back(Chart::NoValue);
    }
    
    // Ajouter les segments à leurs couches respectives
    if (!segmentX[0].empty()) {
        // Get arrays for x and y values
        DoubleArray x = vectorToDoubleArray(segmentX[0]);
        DoubleArray y = vectorToDoubleArray(segmentY[0]);
        
        // Create a step line layer for constant segments
        constantLayer->setXData(x);
        DataSet* constDataSet = constantLayer->addDataSet(y, constColor, "Constant");
        constantLayer->setLineWidth(2);
    }
    
    if (!segmentX[1].empty()) {
        // Get arrays for x and y values
        DoubleArray x = vectorToDoubleArray(segmentX[1]);
        DoubleArray y = vectorToDoubleArray(segmentY[1]);
        
        // Create a step line layer for up segments
        upLayer->setXData(x);
        DataSet* upDataSet = upLayer->addDataSet(y, upColor, "Up");
        upLayer->setLineWidth(5);
    }
    
    if (!segmentX[2].empty()) {
        // Get arrays for x and y values
        DoubleArray x = vectorToDoubleArray(segmentX[2]);
        DoubleArray y = vectorToDoubleArray(segmentY[2]);
        
        // Create a step line layer for down segments
        downLayer->setXData(x);
        DataSet* downDataSet = downLayer->addDataSet(y, downColor, "Down");
        downLayer->setLineWidth(5);
    }
    
    // Ajouter un point à la fin de la courbe pour marquer la valeur actuelle
    if (!interpolatedValues.empty()) {
        std::vector<double> lastPointX = {(double)(interpolatedTimes.size() - 1)};
        std::vector<double> lastPointY = {interpolatedValues.back()};
        
        DoubleArray xPoint = vectorToDoubleArray(lastPointX);
        DoubleArray yPoint = vectorToDoubleArray(lastPointY);
        
        ScatterLayer* endPoint = equityChart->addScatterLayer(xPoint, yPoint, 
                                                            "Current", Chart::CircleShape, 7, 
                                                            0x000000, 0x000000);
        endPoint->moveFront();
    }
}

void ChartWidget::addMainChartSection(FinanceChart *chart, int chartHeight)
{
}

void ChartWidget::addTradeMarkers(FinanceChart *chart, const DoubleArray &timestamps, int startIndex)
{
    if (m_trades.empty() || !m_backtestData)
        return;

    // Ajouter les marqueurs au graphique principal
    XYChart* mainChart = (XYChart*)chart->getChart(1);
    if (!mainChart)
        return;

    // Structure pour organiser les marqueurs par type
    enum TradeResult { WINNING = 0, LOSING = 1, NEUTRAL = 2, RESULT_COUNT = 3 };
    const int COLORS[RESULT_COUNT] = { 0x00AA00, 0xCC0000, 0x000000 }; // Vert, Rouge, Noir

    // Tous nos containers de marqueurs
    std::vector<std::pair<double, double>> entryMarkers;
    std::vector<std::pair<double, double>> exitMarkers;
    std::vector<std::pair<double, double>> entryArrows[RESULT_COUNT]; // Winning, Losing, Neutral
    std::vector<std::pair<double, double>> exitArrows[RESULT_COUNT];  // Winning, Losing, Neutral

    std::vector<TPSLSegment> tpslSegments;
    
    // Préallocation
    size_t estimatedMarkers = std::min(size_t(100), m_trades.size() * 2);
    entryMarkers.reserve(estimatedMarkers);
    exitMarkers.reserve(estimatedMarkers);
    tpslSegments.reserve(estimatedMarkers * 2);
    for (int i = 0; i < RESULT_COUNT; i++) {
        entryArrows[i].reserve(estimatedMarkers);
        exitArrows[i].reserve(estimatedMarkers);
    }

    // Pour chaque trade, vérifier s'il est visible dans la fenêtre actuelle
    for (const auto& trade : m_trades) {
        // Déterminer la catégorie du résultat
        int resultIndex = NEUTRAL; // Par défaut
        
        if (trade->isClosed()) {
            double pnl = trade->pl();
            if (pnl > 0) 
                resultIndex = WINNING;
            else if (pnl < 0) 
                resultIndex = LOSING;
        }

        // Traiter le point d'entrée
        size_t entryBarIndex = trade->entryBar();
        if (entryBarIndex >= static_cast<size_t>(startIndex) && entryBarIndex < static_cast<size_t>(startIndex + timestamps.len)) {
            double relativeIndex = static_cast<double>(entryBarIndex - startIndex);
            
            // Marqueur carré pour la position d'entrée
            entryMarkers.push_back({relativeIndex, trade->entryPrice()});
            
            // Flèche d'entrée
            if (entryBarIndex < static_cast<int>(m_backtestData->size())) {
                const be::Candle& entryCandle = m_backtestData->at(entryBarIndex);
                double arrowY = entryCandle.high * 1.0005; // Légèrement au-dessus du high
                entryArrows[resultIndex].push_back({relativeIndex, arrowY});
            }

            // Si le trade est fermé, on peut ajouter les segments TP/SL
            if (trade->isClosed()) {
                size_t exitBarIndex = trade->exitBar();
                if (exitBarIndex >= static_cast<size_t>(startIndex) && exitBarIndex < static_cast<size_t>(startIndex + timestamps.len)) {
                    double relativeExitIndex = static_cast<double>(exitBarIndex - startIndex);
                    
                    // Récupérer les valeurs de TP et SL si elles existent
                    double tpValue = trade->tp();
                    if (tpValue > 0) {
                        tpslSegments.push_back({
                            relativeIndex, relativeExitIndex, 
                            tpValue, true, // true = TP
                            COLORS[resultIndex]
                        });
                    }
                    
                    double slValue = trade->sl();
                    if (slValue > 0) {
                        tpslSegments.push_back({
                            relativeIndex, relativeExitIndex,
                            slValue, false, // false = SL
                            COLORS[resultIndex]
                        });
                    }
                }
            }
        }
        
        // Traiter le point de sortie (seulement pour les trades fermés)
        if (trade->isClosed()) {
            size_t exitBarIndex = trade->exitBar();
            if (exitBarIndex >= static_cast<size_t>(startIndex) && exitBarIndex < static_cast<size_t>(startIndex + timestamps.len)) {
                double relativeExitIndex = static_cast<double>(exitBarIndex - startIndex);
                
                // Marqueur carré pour la position de sortie
                exitMarkers.push_back({relativeExitIndex, trade->exitPrice()});
                
                // Flèche de sortie
                if (exitBarIndex < static_cast<int>(m_backtestData->size())) {
                    const be::Candle& exitCandle = m_backtestData->at(exitBarIndex);
                    double arrowY = exitCandle.low * 0.9995; // Légèrement en-dessous du low
                    exitArrows[resultIndex].push_back({relativeExitIndex, arrowY});
                }
            }
        }
    }

    // Ajouter les marqueurs carrés pour les entrées et sorties
    addMarkers(mainChart, entryMarkers, "Entries", Chart::SquareSymbol, 7, 0x000000);
    addMarkers(mainChart, exitMarkers, "Exits", Chart::SquareSymbol, 7, 0x000000);
    
    // Ajouter les flèches
    const char* resultNames[RESULT_COUNT] = { "Win", "Loss", "Flat" };
    
    for (int i = 0; i < RESULT_COUNT; i++) {
        if (!entryArrows[i].empty()) {
            std::string name = std::string(resultNames[i]) + " Entry";
            addMarkers(mainChart, entryArrows[i], name.c_str(), Chart::InvertedTriangleSymbol, 15, COLORS[i]);
        }
        
        if (!exitArrows[i].empty()) {
            std::string name = std::string(resultNames[i]) + " Exit";
            addMarkers(mainChart, exitArrows[i], name.c_str(), Chart::TriangleSymbol, 15, COLORS[i]);
        }
    }

    // Ajouter les segments TP/SL
    addTPSLSegments(mainChart, tpslSegments);
}

void ChartWidget::addTPSLSegments(XYChart* chart, const std::vector<TPSLSegment>& segments) {
    if (segments.empty()) return;
    
    // Créer des vecteurs séparés pour les segments TP et SL
    std::vector<double> tpXData, tpYData;
    std::vector<double> slXData, slYData;
    
    // Parcourir tous les segments et les séparer par type
    for (const auto& segment : segments) {
        std::vector<double>& xData = segment.isTakeProfit ? tpXData : slXData;
        std::vector<double>& yData = segment.isTakeProfit ? tpYData : slYData;
        
        // Ajouter le point de départ du segment horizontal
        xData.push_back(segment.startX);
        yData.push_back(segment.level);
        
        // Ajouter le point de fin du segment horizontal
        xData.push_back(segment.endX);
        yData.push_back(segment.level);
        
        // Ajouter un point NoValue pour créer une discontinuité
        xData.push_back(Chart::NoValue);
        yData.push_back(Chart::NoValue);
    }
    
    // Ajouter les segments de Take Profit
    if (!tpXData.empty()) {
        LineLayer* tpLayer = chart->addLineLayer();
        tpLayer->setLineWidth(1);  // Ligne fine
        tpLayer->setFastLineMode(true);  // Mode rapide pour les lignes droites
        
        // Convertir en DoubleArray pour ChartDirector
        DoubleArray tpX = vectorToDoubleArray(tpXData);
        DoubleArray tpY = vectorToDoubleArray(tpYData);
        
        // Définir les données X et Y séparément
        tpLayer->setXData(tpX);
        tpLayer->addDataSet(tpY, 0x00AA00, "Take Profit");
        tpLayer->moveFront();  // Mettre au premier plan
    }
    
    // Ajouter les segments de Stop Loss
    if (!slXData.empty()) {
        LineLayer* slLayer = chart->addLineLayer();
        slLayer->setLineWidth(1);  // Ligne fine
        slLayer->setFastLineMode(true);  // Mode rapide pour les lignes droites
        
        // Convertir en DoubleArray pour ChartDirector
        DoubleArray slX = vectorToDoubleArray(slXData);
        DoubleArray slY = vectorToDoubleArray(slYData);
        
        // Définir les données X et Y séparément
        slLayer->setXData(slX);
        slLayer->addDataSet(slY, 0xCC0000, "Stop Loss");
        slLayer->moveFront();  // Mettre au premier plan
    }
}

FinanceChart *ChartWidget::finalizeChart(FinanceChart *chart)
{
    return nullptr;
}

void ChartWidget::addMarkers(XYChart* chart, const std::vector<std::pair<double, double>>& arrows, 
                            const char* name, int symbolType, int symbolSize, int color) {
    if (arrows.empty()) return;

    std::vector<double> xValues;
    std::vector<double> yValues;
    xValues.reserve(arrows.size());
    yValues.reserve(arrows.size());
    
    for (const auto& pair : arrows) {
        xValues.push_back(pair.first);
        yValues.push_back(pair.second);
    }

    // Convertir en DoubleArray
    DoubleArray xArray = vectorToDoubleArray(xValues);
    DoubleArray yArray = vectorToDoubleArray(yValues);

    ScatterLayer* layer = chart->addScatterLayer(xArray, yArray, name, 
                                              symbolType, symbolSize, color);
    layer->moveFront();
}

void ChartWidget::addRSIToChart(FinanceChart* chart, const RSIInstance& rsi, int startIndex, int pointsToShow)
{
    if (!m_indicatorCache.isValid) return;

    auto it = m_indicatorCache.rsi.find(rsi.period);
    if (it == m_indicatorCache.rsi.end()) {
        ensureRSICached(rsi.period);
        it = m_indicatorCache.rsi.find(rsi.period);
        if (it == m_indicatorCache.rsi.end()) {
            return; // Toujours pas disponible
        }
    }

    const std::vector<double>& rsiData = it->second;
    
    if (startIndex >= (int)rsiData.size()) return;

    // Limiter le nombre de points à afficher
    int endIndex = std::min(startIndex + pointsToShow, (int)rsiData.size());
    int actualPoints = endIndex - startIndex;

    if (actualPoints <= 0) return;

    // Extraire les données RSI visibles du cache
    DoubleArray rsiArray(&rsiData[startIndex], actualPoints);
    
    // Ajouter le graphique d'indicateur
    XYChart* c = chart->addIndicator(rsi.height);
    
    // Configurer et ajouter le RSI
    char buffer[1024];
    snprintf(buffer, sizeof(buffer), "RSI (%d)", rsi.period);
    LineLayer* layer = chart->addLineIndicator2(c, rsiArray, rsi.color, buffer);
    layer->setFastLineMode(true);

    // Ajouter les seuils
    chart->addThreshold(c, layer, 50 + rsi.range, rsi.upperColor, 50 - rsi.range, rsi.lowerColor);
    
    // Configurer l'échelle de l'axe Y
    c->yAxis()->setLinearScale(0, 100);
}

void ChartWidget::addEMAToChart(FinanceChart* chart, const EMAInstance& ema, int startIndex, int pointsToShow)
{
    if (!m_indicatorCache.isValid || !chart || !chart->getChart(1)) {
        return;
    }

    auto it = m_indicatorCache.ema.find(ema.period);
    if (it == m_indicatorCache.ema.end()) {
        ensureEMACached(ema.period);
        it = m_indicatorCache.ema.find(ema.period);
        if (it == m_indicatorCache.ema.end()) {
            return; // Toujours pas disponible
        }
    }

    const std::vector<double>& emaData = it->second;
    
    if (startIndex >= (int)emaData.size()) {
        return;
    }

    // Limiter le nombre de points à afficher
    int endIndex = std::min(startIndex + pointsToShow, (int)emaData.size());
    int actualPoints = endIndex - startIndex;

    if (actualPoints <= 0) {
        return;
    }

    // Extraire les données EMA visibles du cache
    DoubleArray emaArray(&emaData[startIndex], actualPoints);
    
    // Configurer et ajouter l'EMA directement sur le graphique principal
    char buffer[1024];
    snprintf(buffer, sizeof(buffer), "EMA (%d)", ema.period);
    LineLayer* layer = chart->addLineIndicator2((XYChart*)chart->getChart(1), emaArray, ema.color, buffer);
    layer->setFastLineMode(true);
}

void ChartWidget::addStochasticToChart(FinanceChart* chart, const StochasticInstance& stochastic, int startIndex, int pointsToShow)
{
    if (!m_indicatorCache.isValid) return;

    // Clé pour retrouver les données en cache
    std::tuple<int, int, int> key = std::make_tuple(stochastic.fastKPeriod, stochastic.slowKPeriod, stochastic.slowDPeriod);
    
    auto it = m_indicatorCache.stochastic.find(key);
    if (it == m_indicatorCache.stochastic.end()) {
        ensureStochasticCached(stochastic.fastKPeriod, stochastic.slowKPeriod, stochastic.slowDPeriod);
        it = m_indicatorCache.stochastic.find(key);
        if (it == m_indicatorCache.stochastic.end()) {
            return; // Toujours pas disponible
        }
    }

    const std::vector<double>& kValues = it->second.first;
    const std::vector<double>& dValues = it->second.second;
    
    if (startIndex >= (int)kValues.size() || startIndex >= (int)dValues.size()) {
        return;
    }

    // Limiter le nombre de points à afficher
    int endIndex = std::min(startIndex + pointsToShow, (int)kValues.size());
    int actualPoints = endIndex - startIndex;

    if (actualPoints <= 0) {
        return;
    }

    // Extraire les données Stochastic visibles du cache
    DoubleArray kArray(&kValues[startIndex], actualPoints);
    DoubleArray dArray(&dValues[startIndex], actualPoints);
    
    // Ajouter le graphique d'indicateur
    XYChart* c = chart->addIndicator(stochastic.height);
    
    // Configurer et ajouter les lignes %K et %D
    char buffer[1024];
    snprintf(buffer, sizeof(buffer), "Stochastic %%K (%d, %d, %d)", 
             stochastic.fastKPeriod, stochastic.slowKPeriod, stochastic.slowDPeriod);

    LineLayer* kLayer = chart->addLineIndicator2(c, kArray, stochastic.kColor, buffer);
    kLayer->setFastLineMode(true);

    snprintf(buffer, sizeof(buffer), "%%D (%d)", stochastic.slowDPeriod);
    LineLayer* dLayer = chart->addLineIndicator2(c, dArray, stochastic.dColor, buffer);
    dLayer->setFastLineMode(true);

    // Configurer l'échelle de l'axe Y
    c->yAxis()->setLinearScale(0, 100);

    // Ajouter les seuils pour les niveaux de surachat et de survente
    c->yAxis()->addMark(stochastic.overboughtLevel, 0xff6666, std::to_string(stochastic.overboughtLevel).c_str());
    c->yAxis()->addMark(stochastic.oversoldLevel, 0x6666ff, std::to_string(stochastic.oversoldLevel).c_str());
}

void ChartWidget::addATRToChart(FinanceChart* chart, const ATRInstance& atr, int startIndex, int pointsToShow)
{
    if (!m_indicatorCache.isValid) return;

    auto it = m_indicatorCache.atr.find(atr.period);
    if (it == m_indicatorCache.atr.end()) {
        ensureATRCached(atr.period);
        it = m_indicatorCache.atr.find(atr.period);
        if (it == m_indicatorCache.atr.end()) {
            return; // Toujours pas disponible
        }
    }

    const std::vector<double>& atrData = it->second;
    
    if (startIndex >= (int)atrData.size()) return;

    // Limiter le nombre de points à afficher
    int endIndex = std::min(startIndex + pointsToShow, (int)atrData.size());
    int actualPoints = endIndex - startIndex;

    if (actualPoints <= 0) return;

    // Extraire les données ATR visibles du cache
    DoubleArray atrArray(&atrData[startIndex], actualPoints);
    
    // Ajouter le graphique d'indicateur
    XYChart* c = chart->addIndicator(atr.height);
    
    // Configurer et ajouter l'ATR
    char buffer[1024];
    snprintf(buffer, sizeof(buffer), "ATR (%d)", atr.period);
    LineLayer* layer = chart->addLineIndicator2(c, atrArray, atr.color, buffer);
    layer->setFastLineMode(true);

    // Configurer l'échelle de l'axe Y
    c->yAxis()->setLinearScale(0, *std::max_element(atrData.begin() + startIndex, atrData.begin() + endIndex));
}


void ChartWidget::trackFinance(MultiChart* m, int mouseX)
{
    
    // Nettoyer la couche dynamique actuelle
    DrawArea* d = m->initDynamicLayer();
    
    // Vérifier que le graphique n'est pas vide
    if (m->getChartCount() == 0)
        return;

    if (m_rulerToolEnabled && m_rulerFirstPointSelected)
        drawRuler(m, mouseX, m_chartViewer->getPlotAreaMouseY(), d);
    
    // Obtenir la valeur x la plus proche de la souris
    int xValue = (int)(((XYChart*)m->getChart(0))->getNearestXValue(mouseX));
    
    // Itérer sur tous les graphiques XY dans le FinanceChart
    XYChart *c = 0;
    
    for (int i = 0; i < m->getChartCount(); ++i) {
        c = (XYChart*)m->getChart(i);
        
        // Variables pour les entrées de légende
        std::ostringstream ohlcLegend;
        std::vector<std::string> legendEntries;
        
        // Itérer sur toutes les couches pour trouver le point de données le plus élevé
        for (int j = 0; j < c->getLayerCount(); ++j) {
            Layer* layer = c->getLayerByZ(j);
            int xIndex = layer->getXIndexOf(xValue);
            int dataSetCount = layer->getDataSetCount();
            
            // Dans un FinanceChart, seules les couches montrant des données OHLC peuvent avoir 4 ensembles de données
            if (dataSetCount == 4) {
                double highValue = layer->getDataSet(0)->getValue(xIndex);
                double lowValue = layer->getDataSet(1)->getValue(xIndex);
                double openValue = layer->getDataSet(2)->getValue(xIndex);
                double closeValue = layer->getDataSet(3)->getValue(xIndex);
                
                if (closeValue != Chart::NoValue) {
                    // Build the OHLC legend
					ohlcLegend << "      <*block*>";
					ohlcLegend << "Open: " << c->formatValue(openValue, "{value|P4}");
					ohlcLegend << ", High: " << c->formatValue(highValue, "{value|P4}"); 
					ohlcLegend << ", Low: " << c->formatValue(lowValue, "{value|P4}"); 
					ohlcLegend << ", Close: " << c->formatValue(closeValue, "{value|P4}");
                    
                    // Aussi dessiner un triangle vers le haut ou vers le bas pour les jours de hausse et de baisse et le % de variation
                    double lastCloseValue = (xIndex > 0) ? 
                        layer->getDataSet(3)->getValue(xIndex - 1) : 
                        Chart::NoValue;

                    if (lastCloseValue != Chart::NoValue) {
                        double change = closeValue - lastCloseValue;
                        double percent = change * 100 / closeValue;
                        std::string symbol = (change >= 0) ?
                            "<*font,color=008800*><*img=@triangle,width=8,color=008800*>" :
                            "<*font,color=CC0000*><*img=@invertedtriangle,width=8,color=CC0000*>";

                        ohlcLegend << "  " << symbol << " " << c->formatValue(change, "{value|P4}");
						ohlcLegend << " (" << c->formatValue(percent, "{value|2}") << "%)<*/font*>";
                    }

					ohlcLegend << "<*/*>";
                }
            } else {
                // Itérer sur tous les ensembles de données de la couche
                for (int k = 0; k < layer->getDataSetCount(); ++k) {
                    DataSet* dataSet = layer->getDataSetByZ(k);
                    
                    std::string name = dataSet->getDataName();
                    double value = dataSet->getValue(xIndex);
                    if ((0 != name.size()) && (value != Chart::NoValue)) {
                        
                        // Dans un FinanceChart, le nom de l'ensemble de données consiste en le nom de l'indicateur et sa valeur la plus récente
                        
                        // Le caractère d'unité
                        std::string unitChar;
                        
                        // Le nom de l'indicateur est la partie du nom jusqu'au caractère deux-points
                        int delimiterPosition = (int)name.find(':');
                        if ((int)name.npos != delimiterPosition) {
                            
                            // L'unité, le cas échéant, est le(s) caractère(s) non-chiffre(s) final(s)
                            int lastDigitPos = (int)name.find_last_of("0123456789");
                            if (((int)name.npos != lastDigitPos) && (lastDigitPos + 1 < (int)name.size())
                                && (lastDigitPos > delimiterPosition))
                                unitChar = name.substr(lastDigitPos + 1);
                            
                            name.resize(delimiterPosition);
                        }
                        
                        // Dans un FinanceChart, s'il y a deux ensembles de données, cela doit représenter une plage
                        if (dataSetCount == 2) {
                            // Nous montrons les deux valeurs dans la plage dans une seule entrée de légende
                            value = layer->getDataSet(0)->getValue(xIndex);
                            double value2 = layer->getDataSet(1)->getValue(xIndex);
                            name = name + ": " + c->formatValue((std::min)(value, value2), "{value|P3}");
                            name = name + " - " + c->formatValue((std::max)(value, value2), "{value|P3}");
                        } else {
                            // Dans un FinanceChart, seule la couche pour les barres de volume a 3 ensembles de données pour les jours de hausse/baisse/plat
                            if (dataSetCount == 3) {
                                // Le volume réel est la somme des 3 ensembles de données
                                value = layer->getDataSet(0)->getValue(xIndex) + layer->getDataSet(1
                                    )->getValue(xIndex) + layer->getDataSet(2)->getValue(xIndex);
                            }
                            
                            // Créer l'entrée de légende
                            name = name + ": " + c->formatValue(value, "{value|P3}") + unitChar;
                        }
                        
                        // Construire l'entrée de légende, composée d'une boîte carrée colorée et du nom (avec la valeur des données dedans)
                        std::ostringstream legendEntry;
                        legendEntry << "<*block*><*img=@square,width=8,edgeColor=000000,color="
                            << std::hex << dataSet->getDataColor() << "*> " << name << "<*/*>";
                        legendEntries.push_back(legendEntry.str());
                    }
                }
            }
        }
        
        // Obtenir la position de la zone de tracé par rapport à l'ensemble du FinanceChart
        PlotArea* plotArea = c->getPlotArea();
        int plotAreaLeftX = plotArea->getLeftX() + c->getAbsOffsetX();
        int plotAreaTopY = plotArea->getTopY() + c->getAbsOffsetY();
        int plotAreaBottomY = plotAreaTopY + plotArea->getHeight();

        // Calculer la position Y de la souris et la valeur correspondante sur l'axe Y
        int mouseY = m_chartViewer->getPlotAreaMouseY() - c->getAbsOffsetY();
        double yValue = c->getYValue(mouseY);

        // Afficher le tooltip de l'axe Y sur le côté droit
        if (mouseY >= plotArea->getTopY() && mouseY <= plotArea->getTopY() + plotArea->getHeight()) {
            // Position du tooltip sur l'axe Y (côté droit de la zone de tracé)
            int yAxisTooltipX = plotAreaLeftX + plotArea->getWidth() + 5;
            int yAxisTooltipY = mouseY + c->getAbsOffsetY();
            
            // Créer le texte du tooltip avec la valeur Y formatée
            std::string yTooltipText = c->formatValue(yValue, "{value|P4}");
            
            // Dessiner un rectangle de fond pour le tooltip Y
            int tooltipWidth = 60;
            int tooltipHeight = 20;
            d->rect(yAxisTooltipX - 2, yAxisTooltipY - tooltipHeight/2 - 2, 
                   yAxisTooltipX + tooltipWidth + 2, yAxisTooltipY + tooltipHeight/2 + 2, 
                   0x000000, 0xffffcc);
            
            // Afficher le texte du tooltip Y
            TTFText* yTooltip = d->text(yTooltipText.c_str(), "Arial", 8);
            yTooltip->draw(yAxisTooltipX, yAxisTooltipY, 0x000000, Chart::Left);
            yTooltip->destroy();
            
            // Dessiner une ligne horizontale pour le crosshair Y
            d->hline(plotAreaLeftX, plotAreaLeftX + plotArea->getWidth(), 
                    yAxisTooltipY, d->dashLineColor(0x000000, 0x0101));
        }
        
        // La légende commence par l'étiquette de date, puis la légende ohlc (le cas échéant), et ensuite les entrées pour les indicateurs
        std::ostringstream legendText;
        legendText << "<*block,valign=top,maxWidth=" << (plotArea->getWidth() - 5)
            << "*><*font=Arial Bold*>[" << c->xAxis()->getFormattedLabel(xValue, "yyyy-mm-dd hh:nn:ss")
            << "]<*/font*>" << ohlcLegend.str();
        for (int i = ((int)legendEntries.size()) - 1; i >= 0; --i) {
            legendText << "      " << legendEntries[i];
        }
        legendText << "<*/*>";
        
        // Dessiner une ligne de suivi verticale à la position x
        d->vline(plotAreaTopY, plotAreaTopY + plotArea->getHeight(), c->getXCoor(xValue) +
            c->getAbsOffsetX(), d->dashLineColor(0x000000, 0x0101));
        
        // Afficher la légende en haut de la zone de tracé
        TTFText* t = d->text(legendText.str().c_str(), "Arial", 8);
        t->draw(plotAreaLeftX + 5, plotAreaTopY + 5, 0x000000, Chart::TopLeft);
        t->destroy();

        // Seulement pour le dernier graphique (celui du bas avec l'axe X visible)
        if (i == m->getChartCount() - 1) {
            // Obtenir le texte formaté du timestamp
            std::string timeStampText = c->xAxis()->getFormattedLabel(xValue, "yyyy-mm-dd hh:nn:ss");
            
            // Créer un fond rectangulaire pour le texte
            int textHeight = 16;
            int textWidth = 150;  // Ajuster selon la longueur du texte
            int xLabelPos = c->getXCoor(xValue) ;
            int yLabelPos = plotAreaBottomY + 15;  // Position juste en dessous de l'axe X
            
            // Dessiner le fond du texte
            d->rect(xLabelPos - textWidth/2, yLabelPos - textHeight/2,
                    xLabelPos + textWidth/2, yLabelPos + textHeight/2,
                    0x000000, 0xffffcc);
            
            // Créer et dessiner le texte
            TTFText* timeLabel = d->text(timeStampText.c_str(), "Arial", 8);
            timeLabel->draw(xLabelPos, yLabelPos, 0x000000, Chart::Center);
            timeLabel->destroy();
            
            // Dessiner une petite marque verticale sur l'axe X
            d->vline(plotAreaBottomY, plotAreaBottomY + 5, c->getXCoor(xValue), 0x000000);
        }
    }
}

QString ChartWidget::chartTypeToString(ChartType type) {
    for (const auto& info : s_chartTypeData)
        if (info.type == type)
            return QString(info.name);
    return QString("Unknown");
}

ChartWidget::ChartType ChartWidget::stringToChartType(const QString& typeStr) {
    for (const auto& info : s_chartTypeData)
        if (typeStr == info.name)
            return info.type;
    return ChartType::CandleStick; // Valeur par défaut
}