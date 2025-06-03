#include "chart_widget.h"
#include <QVBoxLayout>
#include <QDebug>
#include <QMouseEvent>
#include <algorithm>
#include <cmath>

ChartWidget::ChartWidget(QWidget* parent)
    : QWidget(parent)
    , m_chartViewer(nullptr)
    , m_financeChart(nullptr)
    , m_chartType(ChartType::CandleStick)
    , m_chartWidth(1200)
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
    
    // Connecter les signaux
    connect(m_chartViewer, &QChartViewer::viewPortChanged, 
            this, &ChartWidget::onViewPortChanged);
    connect(m_chartViewer, &QChartViewer::mouseMovePlotArea, 
            this, &ChartWidget::onMouseMovePlotArea);
    
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

void ChartWidget::setBacktestData(const std::shared_ptr<be::Data>& data) {
    if (!data) {
        qWarning() << "Tentative de définir des données de backtest nulles";
        return;
    }
    
    // Convertir les données en format interne
    convertBacktestData(data);
    
    // Mettre à jour le graphique si nous avons des données valides
    if (hasValidData()) {
        updateChart();
    }
}

void ChartWidget::setBacktestTrades(const std::vector<std::shared_ptr<be::Trade>>& trades) {
    if (trades.empty()) {
        qWarning() << "Tentative de définir une liste de trades vide";
        return;
    }
    
    m_trades = trades;
    
    // Mettre à jour le graphique uniquement si nous avons déjà des données de prix valides
    if (hasValidData()) {
        updateChart();
    }
}

void ChartWidget::setEquityCurve(const std::vector<double>& equityCurve) {
    if (equityCurve.empty()) {
        qWarning() << "Tentative de définir une courbe d'équité vide";
        return;
    }
    
    // On a besoin des données de prix pour aligner correctement la courbe d'équité
    if (m_priceData.timestamps.empty()) {
        qWarning() << "Impossible de définir la courbe d'équité sans données de prix";
        return;
    }
    
    // Pour la conversion, nous avons besoin des données be::Data originales
    // L'idéal serait de stocker une référence aux données originales lors de setBacktestData
    // mais pour cet exemple, nous allons reconstruire un objet Data à partir de m_priceData
    
    // Dans un cas réel, il serait préférable d'avoir une référence aux données originales
    std::vector<be::Date> dateVector;
    for (double timestamp : m_priceData.timestamps) {
        // Cette conversion inverse est approximative et devrait être améliorée
        time_t time = static_cast<time_t>(timestamp);
        std::tm* tm = std::localtime(&time);
        be::Date date(tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, 
                     tm->tm_hour, tm->tm_min, tm->tm_sec);
        dateVector.push_back(date);
    }
    
    // Créer un objet Data temporaire
    std::shared_ptr<be::Data> tempData = std::make_shared<be::Data>(
        dateVector, 
        m_priceData.open, 
        m_priceData.high, 
        m_priceData.low, 
        m_priceData.close, 
        m_priceData.volume
    );
    
    // Convertir la courbe d'équité
    convertEquityCurve(equityCurve, tempData);
    
    // Mettre à jour le graphique
    if (hasValidData()) {
        updateChart();
    }
}

void ChartWidget::setChartType(ChartType chartType)
{
    if (m_chartType != chartType) {
        m_chartType = chartType;
        // Si nous avons déjà des données, mettre à jour le graphique
        if (hasValidData()) {
            updateChart();
        }
    }
}

void ChartWidget::createChart()
{
    if (!hasValidData()) {
        qWarning() << "Tentative de création d'un graphique sans données valides";
        return;
    }

    qDebug() << "Création du graphique...";
    
    try {
        // Convertir les données en DoubleArray
        DoubleArray timeStamps = vectorToDoubleArray(m_priceData.timestamps);
        DoubleArray openData = vectorToDoubleArray(m_priceData.open);
        DoubleArray highData = vectorToDoubleArray(m_priceData.high);
        DoubleArray lowData = vectorToDoubleArray(m_priceData.low);
        DoubleArray closeData = vectorToDoubleArray(m_priceData.close);
        DoubleArray volumeData = vectorToDoubleArray(m_priceData.volume);
        
        // Variables pour les données Heikin Ashi (si nécessaire)
        std::vector<double> ha_open, ha_high, ha_low, ha_close;
        DoubleArray haOpenArray, haHighArray, haLowArray, haCloseArray;
        
        // Si le type est HeikinAshi, calculer les valeurs Heikin Ashi
        if (m_chartType == ChartType::HeikinAshi) {
            calculateHeikinAshi(m_priceData.open, m_priceData.high, 
                               m_priceData.low, m_priceData.close,
                               ha_open, ha_high, ha_low, ha_close);
            
            // Convertir en DoubleArray
            haOpenArray = vectorToDoubleArray(ha_open);
            haHighArray = vectorToDoubleArray(ha_high);
            haLowArray = vectorToDoubleArray(ha_low);
            haCloseArray = vectorToDoubleArray(ha_close);
        }
        
        // Configurer le range complet pour le viewport
        m_chartViewer->setFullRange("x", 0, timeStamps.len - 1);
        
        // Créer le graphique
        if (m_chartType == ChartType::HeikinAshi) {
            m_financeChart = drawChart(timeStamps, haHighArray, haLowArray, 
                                     haOpenArray, haCloseArray, volumeData, m_chartWidth);
        } else {
            m_financeChart = drawChart(timeStamps, highData, lowData, 
                                     openData, closeData, volumeData, m_chartWidth);
        }
        
        // Configurer le viewport initial
        int totalPoints = timeStamps.len;
        if (totalPoints > 100) {
            // Afficher les 100 derniers points par défaut
            double visiblePortion = 100.0 / totalPoints;
            m_chartViewer->setViewPortWidth(visiblePortion);
            m_chartViewer->setViewPortLeft(1.0 - visiblePortion);
        } else {
            // Afficher toutes les données
            m_chartViewer->setViewPortWidth(1.0);
            m_chartViewer->setViewPortLeft(0);
        }
        
        // Mettre à jour l'affichage
        m_chartViewer->updateViewPort(true, false);
        
        // Émettre un signal pour indiquer que le graphique a été créé
        emit chartCreated();
        
    } catch (const std::exception& e) {
        qCritical() << "Erreur lors de la création du graphique:" << e.what();
    }
}

void ChartWidget::updateChart()
{
    // Cette méthode recréera le graphique
    if (m_financeChart) {
        delete m_financeChart;
        m_financeChart = nullptr;
    }
    
    // Sauvegarder l'état actuel du viewport
    double currentLeft = 0;
    double currentWidth = 1.0;
    
    if (m_chartViewer) {
        currentLeft = m_chartViewer->getViewPortLeft();
        currentWidth = m_chartViewer->getViewPortWidth();
    }
    
    // Recréer le graphique
    createChart();
    
    // Restaurer le viewport
    if (m_chartViewer) {
        m_chartViewer->setViewPortLeft(currentLeft);
        m_chartViewer->setViewPortWidth(currentWidth);
        m_chartViewer->updateViewPort(true, false);
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
    m_priceData = PriceData();
    // m_tradeData = TradeData();
    m_equityData = EquityData();
}

bool ChartWidget::hasValidData() const
{
    return !m_priceData.timestamps.empty() && 
           !m_priceData.open.empty() && 
           !m_priceData.high.empty() && 
           !m_priceData.low.empty() && 
           !m_priceData.close.empty();
}

void ChartWidget::onViewPortChanged()
{
    // Redessiner le graphique avec le nouveau viewport
    drawChartWithViewport();
    
    // Émettre un signal pour indiquer que le viewport a changé
    emit viewPortChanged();
}

void ChartWidget::onMouseMovePlotArea(QMouseEvent* event)
{
    if (!m_financeChart || !m_chartViewer) {
        return;
    }
    
    // Appliquer le tracking
    int mouseX = m_chartViewer->getPlotAreaMouseX();
    trackFinance(m_financeChart, mouseX);
    
    // Récupérer les informations sur le point
    if (m_financeChart->getChartCount() > 1) {
        XYChart* mainChart = (XYChart*)m_financeChart->getChart(1);
        double xValue = mainChart->getNearestXValue(mouseX);
        
        // Trouver l'indice correspondant
        int dataIndex = -1;
        for (int i = 0; i < (int)m_priceData.timestamps.size(); ++i) {
            if (fabs(m_priceData.timestamps[i] - xValue) < 1e-6) {
                dataIndex = i;
                break;
            }
        }
        
        if (dataIndex >= 0 && dataIndex < (int)m_priceData.close.size()) {
            // Émettre un signal avec les informations du point
            emit mouseOverPoint(m_priceData.timestamps[dataIndex], m_priceData.close[dataIndex]);
        }
    }
    
    // Mettre à jour l'affichage
    m_chartViewer->updateDisplay();
}

void ChartWidget::convertBacktestData(const std::shared_ptr<be::Data>& data) {
    if (!data || data->size() == 0) {
        qWarning() << "Données de backtest vides ou invalides";
        return;
    }

    // Réinitialiser les données de prix
    m_priceData = PriceData();
    
    // Réserver la capacité pour éviter les réallocations
    size_t dataSize = data->size();
    m_priceData.timestamps.reserve(dataSize);
    m_priceData.open.reserve(dataSize);
    m_priceData.high.reserve(dataSize);
    m_priceData.low.reserve(dataSize);
    m_priceData.close.reserve(dataSize);
    m_priceData.volume.reserve(dataSize);

    // Convertir chaque bougie
    for (size_t i = 0; i < dataSize; ++i) {
        const be::Candle& candle = data->at(i);
        
        // Convertir la date en timestamp ChartDirector
        double timestamp = dateToChartTimestamp(candle.date);
        
        m_priceData.timestamps.push_back(timestamp);
        m_priceData.open.push_back(candle.open);
        m_priceData.high.push_back(candle.high);
        m_priceData.low.push_back(candle.low);
        m_priceData.close.push_back(candle.close);
        m_priceData.volume.push_back(candle.volume);
    }
    
    qDebug() << "Données de prix converties:" << dataSize << "bougies";
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
    
    // Cas où les données sont alignées (même nombre de points)
    if (numPoints == numBars) {
        m_equityData.timestamps.reserve(numBars);
        m_equityData.equity_values.reserve(numBars);
        
        // Convertir les dates en timestamps et copier les valeurs d'équité
        for (size_t i = 0; i < numBars; ++i) {
            double timestamp = dateToChartTimestamp(data->at(i).date);
            m_equityData.timestamps.push_back(timestamp);
            m_equityData.equity_values.push_back(equityCurve[i]);
        }

        std::cout << "Équité alignée avec les bougies" << std::endl;
    }
    else if (numPoints < numBars) {
        m_equityData.timestamps.reserve(numBars);
        m_equityData.equity_values.resize(numBars);
        
        // Convertir les timestamps
        for (size_t i = 0; i < numBars; ++i) {
            double timestamp = dateToChartTimestamp(data->at(i).date);
            m_equityData.timestamps.push_back(timestamp);
        }
        
        // La courbe d'équité commence généralement au premier indice, donc aligner le début
        // On suppose que equityCurve[0] correspond à la première bougie
        double initialEquity = equityCurve[0];
        
        // Replier l'équité depuis le début
        for (size_t i = 0; i < numBars; ++i) {
            if (i < numPoints) {
                // Utiliser directement les valeurs disponibles
                m_equityData.equity_values[i] = equityCurve[i];
            } else {
                // Utiliser la dernière valeur disponible pour les bougies supplémentaires
                m_equityData.equity_values[i] = equityCurve[numPoints - 1];
            }
        }

        std::cout << "Équité étendue pour couvrir toutes les bougies" << std::endl;
    }
    // Cas où il y a plus de points d'équité que de barres
    else {
        m_equityData.timestamps.reserve(numBars);
        m_equityData.equity_values.resize(numBars);
        
        // On va supposer que l'equity curve est générée à chaque bougie,
        // donc on prend simplement les points correspondants
        for (size_t i = 0; i < numBars; ++i) {
            double timestamp = dateToChartTimestamp(data->at(i).date);
            m_equityData.timestamps.push_back(timestamp);
            
            // Prendre directement les équivalents (au lieu de sous-échantillonner)
            m_equityData.equity_values[i] = equityCurve[i];
        }

        std::cout << "Équité tronquée pour correspondre aux bougies" << std::endl;
    }
    
    // Calculer le drawdown
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

    std::cout << "Courbe d'équité convertie:" << m_equityData.timestamps.size() << "points" << std::endl;
}

double ChartWidget::dateToChartTimestamp(const be::Date& date) {
    // ChartDirector attend des timestamps en secondes depuis l'époque Unix (1/1/1970)
    std::tm time_struct = {};
    time_struct.tm_year = date.getYear() - 1900; // Les années dans tm commencent à 1900
    time_struct.tm_mon = date.getMonth() - 1;    // Les mois dans tm vont de 0 à 11
    time_struct.tm_mday = date.getDay();
    time_struct.tm_hour = date.getHour();
    time_struct.tm_min = date.getMinute();
    time_struct.tm_sec = date.getSecond();

    // Convertir en timestamp Unix
    std::time_t timestamp = std::mktime(&time_struct);
    return static_cast<double>(timestamp);
}
void ChartWidget::drawChartWithViewport()
{
    if (!hasValidData() || !m_chartViewer) {
        return;
    }
    
    try {
        // Calculer les indices de début et fin basés sur le viewport
        int totalPoints = m_priceData.timestamps.size();
        
        double viewPortLeft = m_chartViewer->getViewPortLeft();
        double viewPortWidth = m_chartViewer->getViewPortWidth();
        
        int startIndex = (int)floor(viewPortLeft * totalPoints);
        int endIndex = (int)ceil((viewPortLeft + viewPortWidth) * totalPoints) - 1;
        
        // S'assurer que les indices sont dans les limites
        startIndex = std::max(0, std::min(startIndex, totalPoints - 1));
        endIndex = std::max(startIndex, std::min(endIndex, totalPoints - 1));
        
        int pointsToShow = endIndex - startIndex + 1;
        
        // Extraire les données visibles pour les prix
        DoubleArray timeStamps = DoubleArray(&m_priceData.timestamps[startIndex], pointsToShow);
        DoubleArray openData = DoubleArray(&m_priceData.open[startIndex], pointsToShow);
        DoubleArray highData = DoubleArray(&m_priceData.high[startIndex], pointsToShow);
        DoubleArray lowData = DoubleArray(&m_priceData.low[startIndex], pointsToShow);
        DoubleArray closeData = DoubleArray(&m_priceData.close[startIndex], pointsToShow);
        DoubleArray volumeData = DoubleArray(&m_priceData.volume[startIndex], pointsToShow);
        
        // Variables pour les données Heikin Ashi (si nécessaire)
        std::vector<double> ha_open, ha_high, ha_low, ha_close;
        DoubleArray haOpenArray, haHighArray, haLowArray, haCloseArray;
        
        // Si le type est HeikinAshi, calculer les valeurs Heikin Ashi
        if (m_chartType == ChartType::HeikinAshi) {
            // Créer des sous-vecteurs pour les données visibles
            std::vector<double> visible_open(m_priceData.open.begin() + startIndex, 
                                           m_priceData.open.begin() + endIndex + 1);
            std::vector<double> visible_high(m_priceData.high.begin() + startIndex, 
                                           m_priceData.high.begin() + endIndex + 1);
            std::vector<double> visible_low(m_priceData.low.begin() + startIndex, 
                                          m_priceData.low.begin() + endIndex + 1);
            std::vector<double> visible_close(m_priceData.close.begin() + startIndex, 
                                            m_priceData.close.begin() + endIndex + 1);
            
            // Calculer les valeurs Heikin Ashi pour la plage visible
            calculateHeikinAshi(visible_open, visible_high, 
                               visible_low, visible_close,
                               ha_open, ha_high, ha_low, ha_close);
                                
            // Convertir en DoubleArray
            haOpenArray = vectorToDoubleArray(ha_open);
            haHighArray = vectorToDoubleArray(ha_high);
            haLowArray = vectorToDoubleArray(ha_low);
            haCloseArray = vectorToDoubleArray(ha_close);
            
            // Créer le graphique
            m_financeChart = drawChart(timeStamps, haHighArray, haLowArray, 
                                     haOpenArray, haCloseArray, volumeData, m_chartWidth);
        } else {
            // Créer le graphique
            m_financeChart = drawChart(timeStamps, highData, lowData, 
                                     openData, closeData, volumeData, m_chartWidth);
        }
        
        // Mettre à jour l'affichage
        m_chartViewer->updateDisplay();
        
    } catch (const std::exception& e) {
        qCritical() << "Erreur dans drawChartWithViewport:" << e.what();
    }
}

DoubleArray ChartWidget::vectorToDoubleArray(const std::vector<double>& vec)
{
    if (vec.empty()) {
        return DoubleArray(nullptr, 0);
    }
    // Créer une copie des données pour éviter les problèmes de durée de vie
    double* data = new double[vec.size()];
    std::copy(vec.begin(), vec.end(), data);
    return DoubleArray(data, static_cast<int>(vec.size()));
    // Note: ChartDirector libère la mémoire des DoubleArray qu'il consomme
}

// possiblement faisable en simd ??? 
void ChartWidget::calculateHeikinAshi(
    const std::vector<double>& open,
    const std::vector<double>& high,
    const std::vector<double>& low, 
    const std::vector<double>& close,
    std::vector<double>& ha_open, 
    std::vector<double>& ha_high,
    std::vector<double>& ha_low, 
    std::vector<double>& ha_close) 
{
    size_t size = open.size();
    if (size == 0) return;
    
    ha_open.resize(size);
    ha_high.resize(size);
    ha_low.resize(size);
    ha_close.resize(size);
    
    // Première bougie
    ha_open[0] = open[0];
    ha_close[0] = (open[0] + high[0] + low[0] + close[0]) / 4.0;
    ha_high[0] = high[0];
    ha_low[0] = low[0];
    
    // Calcul des autres bougies
    for (size_t i = 1; i < size; ++i) {
        ha_close[i] = (open[i] + high[i] + low[i] + close[i]) / 4.0;
        ha_open[i] = (ha_open[i-1] + ha_close[i-1]) / 2.0;
        ha_high[i] = std::max(std::max(high[i], ha_open[i]), ha_close[i]);
        ha_low[i] = std::min(std::min(low[i], ha_open[i]), ha_close[i]);
    }
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
    
    // Configurer les données
    c->setData(timestamps, highData, lowData, openData, closeData, volumeData, 0);
    
    // Ajouter le titre du graphique
    std::string chartTypeStr = chartTypeToString(m_chartType).toStdString();
    std::string title = "Graphique de trading (" + chartTypeStr + ") - " + 
                       std::to_string(timestamps.len) + " points";
    c->addTitle(title.c_str());
    
    // Hauteurs pour les différentes parties du graphique
    int equityHeight = 150;     // Hauteur du graphique d'équité
    int mainChartHeight = 400;  // Hauteur du graphique principal
    int volumeHeight = 100;     // Hauteur du graphique de volume

    // Déterminer l'index de début et de fin des données actuellement affichées
    // timestamps contient uniquement les bougies visibles
    int startIndex = 0;  // L'index de début des données visibles par rapport au dataset complet
    
    // Si nous sommes en mode viewport (zoom/déplacement), déterminer l'index de début
    if (timestamps.len < (int)m_priceData.timestamps.size()) {
        double firstVisibleTimestamp = timestamps[0];
        
        // Trouver l'index correspondant dans le dataset complet
        for (size_t i = 0; i < m_priceData.timestamps.size(); ++i) {
            if (std::abs(m_priceData.timestamps[i] - firstVisibleTimestamp) < 0.001) {
                startIndex = i;
                break;
            }
        }
    }
    
    // 1. Ajouter la courbe d'équité en haut si disponible
    if (!m_equityData.equity_values.empty() && timestamps.len > 0) {
        // Trouver les indices correspondant à la fenêtre visible
        int equityStartIndex = startIndex;  // Utiliser le même index de début que pour les bougies
        int equityEndIndex = std::min(equityStartIndex + timestamps.len, (int)m_equityData.equity_values.size());
        
        if (equityStartIndex < (int)m_equityData.equity_values.size()) {
            // Créer un sous-tableau pour les valeurs d'equity visibles
            std::vector<double> visibleEquity(m_equityData.equity_values.begin() + equityStartIndex,
                                            m_equityData.equity_values.begin() + equityEndIndex);
                                            
            // Si nécessaire, compléter pour avoir la même taille que le nombre de bougies visibles
            while (visibleEquity.size() < (size_t)timestamps.len) {
                visibleEquity.push_back(visibleEquity.back());
            }
            
            // Convertir en DoubleArray
            DoubleArray equityValues = vectorToDoubleArray(visibleEquity);
            
            // Ajouter l'indicateur pour l'equity curve
            XYChart* equityChart = c->addIndicator(equityHeight);
            
            // Configuration du titre et des libellés
            equityChart->yAxis()->setTitle("Capital");
            equityChart->xAxis()->setColors(Chart::Transparent); // Masquer l'axe X
            
            // Ajouter la ligne principale d'équité
            LineLayer* equityLayer = equityChart->addLineLayer();
            equityLayer->addDataSet(equityValues, 0x008800, "Equity");
            equityLayer->setLineWidth(2);
            
            // Optionnel: ajouter un point à la fin de la courbe pour marquer la valeur actuelle
            if (!visibleEquity.empty()) {
                std::vector<double> lastPointX = {(double)(visibleEquity.size() - 1)};
                std::vector<double> lastPointY = {visibleEquity.back()};
                
                DoubleArray xPoint = vectorToDoubleArray(lastPointX);
                DoubleArray yPoint = vectorToDoubleArray(lastPointY);
                
                ScatterLayer* endPoint = equityChart->addScatterLayer(xPoint, yPoint, 
                                                                    "Current", Chart::CircleShape, 7, 
                                                                    0x008800, 0x008800);
                endPoint->moveFront();
            }
        }
    }
    
    // 2. Ajouter le graphique principal
    c->addMainChart(mainChartHeight);
    
    // Ajouter le type de graphique approprié selon le type actuel
    if (m_chartType == ChartType::CandleStick || m_chartType == ChartType::HeikinAshi) {
        c->addCandleStick(0x00CC00, 0xFF3333); // Vert/Rouge pour les bougies
    } else if (m_chartType == ChartType::OHLC) {
        c->addHLOC(0x00CC00, 0xFF3333); // Vert/Rouge pour les barres OHLC
    } else if (m_chartType == ChartType::Close) {
        c->addCloseLine(0x000088); // Ligne bleue pour le prix de clôture
    }
    
    // 4. Ajouter les trades si disponibles
    if (!m_trades.empty()) {
        // Ajouter les marqueurs au graphique principal
        XYChart* mainChart = (XYChart*)c->getChart(1);

        if (!mainChart) {
            return c;
        }
        
        // Filtrer les trades qui sont visibles dans la fenêtre actuelle
        std::vector<double> visibleIndices;
        std::vector<double> visiblePrices;

        // Pour chaque trade, vérifier s'il est visible dans la fenêtre actuelle
        for (const auto& trade : m_trades) {
            int tradeIndex = trade->entryBar();
            
            // Vérifier si le trade est dans la plage visible
            if (tradeIndex >= startIndex && tradeIndex < startIndex + timestamps.len) {
                // Calculer l'index relatif dans la fenêtre visible
                double relativeIndex = tradeIndex - startIndex;
                visibleIndices.push_back(relativeIndex);
                visiblePrices.push_back(trade->entryPrice());
            }
        }

        // Vérifier si nous avons des trades visibles
        if (!visibleIndices.empty()) {
            // Convertir en DoubleArray
            DoubleArray xIndices = vectorToDoubleArray(visibleIndices);
            DoubleArray yPrices = vectorToDoubleArray(visiblePrices);

            // Marqueurs pour les entrées (triangles verts)
            ScatterLayer* entryLayer = mainChart->addScatterLayer(
                xIndices, yPrices,
                "Entries",                   // Nom
                Chart::TriangleSymbol,       // Symbole 
                9,                           // Taille
                0x00AA00,                    // Couleur de remplissage (vert)
                0x000000                     // Couleur de contour
            );
            
            // S'assurer que les marqueurs sont au premier plan
            entryLayer->moveFront();

            std::vector<double> visibleExitIndices;
            std::vector<double> visibleExitPrices;

            // Pour chaque trade, vérifier s'il est fermé et visible dans la fenêtre actuelle
            for (const auto& trade : m_trades) {
                // Ne traiter que les trades fermés
                if (trade->isClosed()) {
                    int exitIndex = trade->exitBar();
                    
                    // Vérifier si la sortie est dans la plage visible
                    if (exitIndex >= startIndex && exitIndex < startIndex + timestamps.len) {
                        // Calculer l'index relatif dans la fenêtre visible
                        double relativeIndex = exitIndex - startIndex;
                        visibleExitIndices.push_back(relativeIndex);
                        visibleExitPrices.push_back(trade->exitPrice());
                    }
                }
            }

            // Vérifier si nous avons des sorties visibles
            if (!visibleExitIndices.empty()) {
                // Convertir en DoubleArray
                DoubleArray xExitIndices = vectorToDoubleArray(visibleExitIndices);
                DoubleArray yExitPrices = vectorToDoubleArray(visibleExitPrices);
                
                // Marqueurs pour les sorties (triangles inversés rouges)
                ScatterLayer* exitLayer = mainChart->addScatterLayer(
                    xExitIndices, yExitPrices,
                    "Exits",                          // Nom
                    Chart::InvertedTriangleSymbol,    // Symbole triangulaire inversé
                    9,                                // Taille
                    0xFF0000,                         // Couleur de remplissage (rouge)
                    0x000000                          // Couleur de contour
                );
                
                // S'assurer que les marqueurs sont au premier plan
                exitLayer->moveFront();
            }
        }
    }
    
    // Mettre à jour le graphique dans le viewer
    m_chartViewer->setChart(c);
    
    return c;
}

void ChartWidget::trackFinance(MultiChart* m, int mouseX)
{
    // Nettoyer la couche dynamique actuelle
    DrawArea* d = m->initDynamicLayer();
    
    // Vérifier que le graphique n'est pas vide
    if (m->getChartCount() == 0)
        return;
    
    // Obtenir la valeur x la plus proche de la souris
    int xValue = (int)(((XYChart*)m->getChart(0))->getNearestXValue(mouseX));

    std::cout << "Tracking mouse at xValue: " << xValue << std::endl;
    
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
                    double lastCloseValue = layer->getDataSet(3)->getValue(xIndex - 1);
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
        
        // La légende commence par l'étiquette de date, puis la légende ohlc (le cas échéant), et ensuite les entrées pour les indicateurs
        std::ostringstream legendText;
        legendText << "<*block,valign=top,maxWidth=" << (plotArea->getWidth() - 5)
            << "*><*font=Arial Bold*>[" << c->xAxis()->getFormattedLabel(xValue, "mmm dd, yyyy")
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
        t->draw(plotAreaLeftX + 5, plotAreaTopY + 15, 0x000000, Chart::TopLeft);
        t->destroy();
    }
}

QString ChartWidget::chartTypeToString(ChartType type)
{
    switch (type) {
        case ChartType::CandleStick: return "CandleStick";
        case ChartType::HeikinAshi: return "HeikinAshi";
        case ChartType::OHLC: return "OHLC";
        case ChartType::Close: return "Close";
        default: return "CandleStick";
    }
}

ChartType ChartWidget::stringToChartType(const QString& typeStr)
{
    if (typeStr == "HeikinAshi") return ChartType::HeikinAshi;
    if (typeStr == "OHLC") return ChartType::OHLC;
    if (typeStr == "Close") return ChartType::Close;
    return ChartType::CandleStick; // Valeur par défaut
}