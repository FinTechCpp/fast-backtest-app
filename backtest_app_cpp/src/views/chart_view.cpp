#include "chart_view.h"
#include <QDebug>
#include <QTimer>
#include <QResizeEvent>
#include <QApplication>  // AJOUTER CETTE LIGNE
#include <QButtonGroup>  // Si pas déjà inclus

ChartView::ChartView(QWidget* parent)
    : BaseView(parent)
    , m_cachedData(nullptr)
    , m_cachedStats(nullptr)
    , m_dataExtracted(false)
    , m_currentData(nullptr)
    , m_currentStats(nullptr)
    , m_chartPlaceholder(nullptr)
    , m_financeChart(nullptr)
    , m_chartViewer(nullptr)
{
    qDebug() << "ChartView créée avec parent:" << parent;
    
    // Initialiser les structures de données
    m_priceData = PriceData();
    m_tradeData = TradeData();
    m_equityData = EquityData();
    
    // Construire l'interface dans le constructeur
    setupUI();
}

ChartView::~ChartView()
{
    if (m_financeChart) {
        delete m_financeChart;
        m_financeChart = nullptr;
    }
}

void ChartView::setupUI()
{
    QTime start = QTime::currentTime();
    
    qDebug() << "ChartView::setupUI() - Configuration du layout principal";
    
    // Configurer le layout principal pour occuper tout l'espace
    m_mainLayout->setContentsMargins(0, 0, 0, 0);  // Pas de marges
    m_mainLayout->setSpacing(0);                     // Pas d'espacement
    
    // Créer le placeholder initial qui occupe tout l'espace
    m_chartPlaceholder = new QLabel("Exécutez le backtest pour afficher les graphiques");
    m_chartPlaceholder->setAlignment(Qt::AlignCenter);
    m_chartPlaceholder->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_chartPlaceholder->setStyleSheet(
        "QLabel { "
        "background-color: #f5f5f5; "
        "border: 1px dashed #cccccc; "
        "color: #666666; "
        "font-size: 14px; "
        "}"
    );
    
    // Ajouter le placeholder au layout principal
    m_mainLayout->addWidget(m_chartPlaceholder);
    
    // Configurer le widget pour s'étendre
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    
    int elapsed = start.msecsTo(QTime::currentTime());
    qInfo() << "ChartView::setupUI() took" << elapsed << "ms";
}

void ChartView::resizeEvent(QResizeEvent* event)
{
    BaseView::resizeEvent(event);
    
    qDebug() << "ChartView redimensionnée vers:" << event->size();
    
    // Si on a un graphique, le redimensionner
    if (m_financeChart && m_chartViewer) {
        QTimer::singleShot(100, [this]() {
            resizeChart(this->width());
        });
    }
}

void ChartView::updateData(void* data, void* stats)
{
    QTime start = QTime::currentTime();
    
    m_currentData = data;
    m_currentStats = stats;
    
    qDebug() << "=== DÉBUT ChartView::updateData() ===";
    qDebug() << "Data pointer:" << data << "Stats pointer:" << stats;

    // Vérifier si les données ont déjà été extraites pour ces pointeurs
    if (m_dataExtracted && m_cachedData == data && m_cachedStats == stats) {
        qDebug() << "Données déjà en cache, pas de ré-extraction nécessaire";
        if (hasValidData()) {
            createChart();
        }
        qDebug() << "=== FIN ChartView::updateData() (depuis cache) ===";
        return;
    }
    
    // Mettre en cache les nouveaux pointeurs
    m_cachedData = data;
    m_cachedStats = stats;
    
    if (!data || !stats) {
        qDebug() << "Données nulles détectées";
        clear();
        showPlaceholder("Aucune donnée disponible");
        return;
    }
    
    try {
        qDebug() << "Acquisition du GIL...";
        py::gil_scoped_acquire acquire;
        qDebug() << "GIL acquis avec succès";
        
        qDebug() << "Début extraction des données Python...";
        extractDataFromPython(data, stats);
        qDebug() << "Extraction terminée";
        
        qDebug() << "Vérification des données...";
        if (!hasValidData()) {
            qDebug() << "Données invalides détectées";
            showPlaceholder("Données invalides");
            return;
        }
        qDebug() << "Données validées";
        
        qDebug() << "Masquage du placeholder...";
        if (m_chartPlaceholder) {
            m_chartPlaceholder->setVisible(false);
        }
        qDebug() << "Placeholder masqué";
        
        qDebug() << "Création du graphique...";
        createChart();
        qDebug() << "Graphique créé";
        
        m_dataExtracted = true;
        
    } catch (const py::error_already_set& e) {
        qCritical() << "Erreur Python:" << e.what();
        showPlaceholder("Erreur Python lors de la création du graphique");
    } catch (const std::exception& e) {
        qCritical() << "Erreur C++:" << e.what();
        showPlaceholder("Erreur lors de la création du graphique");
    } catch (...) {
        qCritical() << "Erreur inconnue dans ChartView::updateData()";
        showPlaceholder("Erreur inconnue");
    }

    int elapsed = start.msecsTo(QTime::currentTime());
    qInfo() << "ChartView::updateData() took" << elapsed << "ms";
    qDebug() << "=== FIN ChartView::updateData() ===";
}

void ChartView::clear()
{
    qDebug() << "ChartView::clear() appelé";
    
    m_currentData = nullptr;
    m_currentStats = nullptr;
    m_cachedData = nullptr;
    m_cachedStats = nullptr;
    m_dataExtracted = false;
    
    // Nettoyer les données
    m_priceData = PriceData();
    m_tradeData = TradeData();
    m_equityData = EquityData();
    
    // Supprimer le graphique existant
    if (m_financeChart) {
        delete m_financeChart;
        m_financeChart = nullptr;
    }
    
    if (m_chartViewer) {
        m_mainLayout->removeWidget(m_chartViewer);
        delete m_chartViewer;
        m_chartViewer = nullptr;
    }
    
    // Réafficher le placeholder
    if (m_chartPlaceholder) {
        m_chartPlaceholder->setText("Exécutez un backtest pour afficher les graphiques");
        m_chartPlaceholder->setVisible(true);
    }
    
    qDebug() << "ChartView nettoyée";
}

void ChartView::createChart()
{
    qDebug() << "=== DÉBUT createChart ===";
    qDebug() << "Taille des données timestamps:" << m_priceData.timestamps.size();
    
    if (m_priceData.timestamps.empty()) {
        qWarning() << "Aucune donnée de prix disponible";
        return;
    }
    
    qDebug() << "Données disponibles - création du graphique FinanceChart...";
    
    try {
        // Nettoyer le graphique précédent
        if (m_financeChart) {
            delete m_financeChart;
            m_financeChart = nullptr;
        }
        
        if (m_chartViewer) {
            m_mainLayout->removeWidget(m_chartViewer);
            delete m_chartViewer;
            m_chartViewer = nullptr;
        }
        
        qDebug() << "Conversion des données en DoubleArray...";
        DoubleArray timeStamps = vectorToDoubleArray(m_priceData.timestamps);
        DoubleArray openData = vectorToDoubleArray(m_priceData.open);
        DoubleArray highData = vectorToDoubleArray(m_priceData.high);
        DoubleArray lowData = vectorToDoubleArray(m_priceData.low);
        DoubleArray closeData = vectorToDoubleArray(m_priceData.close);
        DoubleArray volumeData = vectorToDoubleArray(m_priceData.volume);
        
        qDebug() << "Données converties - timeStamps:" << timeStamps.len << "points";
        
        // Calculer la largeur optimale pour le graphique
        int widgetWidth = this->width();
        int widgetHeight = this->height();
        
        qDebug() << "Taille actuelle du widget:" << widgetWidth << "x" << widgetHeight;
        
        // Utiliser toute la largeur disponible (minimum 800 pour lisibilité)
        int chartWidth = std::max(800, widgetWidth - 20);  // -20 pour petites marges
        int chartHeight = std::max(400, widgetHeight - 20); // Hauteur adaptative
        
        qDebug() << "Taille calculée du graphique:" << chartWidth << "x" << chartHeight;
        
        // Créer FinanceChart avec la largeur adaptative
        m_financeChart = new FinanceChart(chartWidth);
        
        // Configurer les données
        m_financeChart->setData(timeStamps, highData, lowData, openData, closeData, volumeData, 0);
        
        // Ajouter le titre du graphique
        std::string title = "Graphique de trading - " + std::to_string(timeStamps.len) + " points";
        m_financeChart->addTitle(title.c_str());
        
        // Calculer la hauteur du graphique principal proportionnellement
        int mainChartHeight = std::max(250, chartHeight - 150); // Réserver espace pour axes/titre
        m_financeChart->addMainChart(mainChartHeight);
        
        // Ajouter les chandelles japonaises
        m_financeChart->addCandleStick(0x00CC00, 0xFF3333); // Vert/Rouge
        
        // Ajouter le volume (plus petit)
        int volumeHeight = std::max(60, chartHeight / 8); // 1/8 de la hauteur totale
        m_financeChart->addVolBars(volumeHeight, 0x99ff99, 0xff9999, 0x808080);
        
        qDebug() << "FinanceChart configuré, création du QChartViewer...";
        
        // Créer le QChartViewer qui occupe tout l'espace
        m_chartViewer = new QChartViewer(this);
        
        // Configurer le viewer pour occuper tout l'espace disponible
        m_chartViewer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        m_chartViewer->setMouseUsage(Chart::MouseUsageScroll);
        m_chartViewer->setMouseTracking(true);
        m_chartViewer->setMouseWheelZoomRatio(1.1);
        m_chartViewer->setScrollDirection(Chart::DirectionHorizontal);
        m_chartViewer->setZoomDirection(Chart::DirectionHorizontal);
        
        // Configurer le range complet
        m_chartViewer->setFullRange("x", 0, timeStamps.len - 1);
        
        // Assigner le graphique au viewer
        m_chartViewer->setChart(m_financeChart);
        
        // Configurer le viewport pour afficher les dernières données
        int totalPoints = timeStamps.len;
        if (totalPoints > 100) {
            // Afficher les 100 derniers points par défaut
            double visiblePortion = 100.0 / totalPoints;
            m_chartViewer->setViewPortWidth(visiblePortion);
            m_chartViewer->setViewPortLeft(1.0 - visiblePortion);
        } else {
            // Afficher toutes les données si moins de 100 points
            m_chartViewer->setViewPortWidth(1.0);
            m_chartViewer->setViewPortLeft(0);
        }
        
        // Ajouter au layout principal (occupe tout l'espace)
        m_mainLayout->addWidget(m_chartViewer);
        
        // Connecter les signaux essentiels
        connect(m_chartViewer, &QChartViewer::viewPortChanged, 
                this, &ChartView::onViewPortChanged);
        connect(m_chartViewer, &QChartViewer::mouseMovePlotArea, 
                this, &ChartView::onMouseMovePlotArea);
        
        // Forcer la mise à jour initiale
        m_chartViewer->updateViewPort(true, false);
        
        qDebug() << "Graphique FinanceChart créé avec succès !";
        
        // Debug des données
        if (!m_priceData.close.empty()) {
            double minPrice = *std::min_element(m_priceData.close.begin(), m_priceData.close.end());
            double maxPrice = *std::max_element(m_priceData.close.begin(), m_priceData.close.end());
            qDebug() << "Range de prix:" << minPrice << "à" << maxPrice;
            qDebug() << "Premier prix:" << m_priceData.close[0] << "Dernier prix:" << m_priceData.close.back();
        }
        
    } catch (const std::exception& e) {
        qCritical() << "Erreur lors de la création du FinanceChart:" << e.what();
        showPlaceholder(QString("Erreur graphique: %1").arg(e.what()));
    }
    
    debugChart();
    qDebug() << "=== FIN createChart ===";
}

void ChartView::resizeChart(int newWidth)
{
    if (!m_financeChart || newWidth <= 0) {
        return;
    }
    
    qDebug() << "Redimensionnement du graphique vers:" << newWidth;
    
    // Sauvegarder l'état du viewport actuel
    double currentLeft = m_chartViewer ? m_chartViewer->getViewPortLeft() : 0;
    double currentWidth = m_chartViewer ? m_chartViewer->getViewPortWidth() : 1.0;
    
    // Forcer la recréation du graphique avec la nouvelle largeur
    if (m_priceData.timestamps.empty()) {
        return;
    }
    
    try {
        // Recréer le graphique avec la nouvelle largeur
        DoubleArray timeStamps = vectorToDoubleArray(m_priceData.timestamps);
        DoubleArray openData = vectorToDoubleArray(m_priceData.open);
        DoubleArray highData = vectorToDoubleArray(m_priceData.high);
        DoubleArray lowData = vectorToDoubleArray(m_priceData.low);
        DoubleArray closeData = vectorToDoubleArray(m_priceData.close);
        DoubleArray volumeData = vectorToDoubleArray(m_priceData.volume);
        
        delete m_financeChart;
        
        // Utiliser la nouvelle largeur (minimum 800)
        int chartWidth = std::max(800, newWidth - 20);
        m_financeChart = new FinanceChart(chartWidth);
        m_financeChart->setData(timeStamps, highData, lowData, openData, closeData, volumeData, 0);
        
        std::string title = "Graphique de trading - " + std::to_string(timeStamps.len) + " points";
        m_financeChart->addTitle(title.c_str());
        
        // Adapter les hauteurs
        int widgetHeight = this->height();
        int mainChartHeight = std::max(250, widgetHeight - 150);
        int volumeHeight = std::max(60, widgetHeight / 8);
        
        m_financeChart->addMainChart(mainChartHeight);
        m_financeChart->addCandleStick(0x00CC00, 0xFF3333);
        m_financeChart->addVolBars(volumeHeight, 0x99ff99, 0xff9999, 0x808080);
        
        // Réassigner le graphique et restaurer le viewport
        if (m_chartViewer) {
            m_chartViewer->setChart(m_financeChart);
            m_chartViewer->setViewPortLeft(currentLeft);
            m_chartViewer->setViewPortWidth(currentWidth);
            m_chartViewer->updateViewPort(true, false);
        }
        
        qDebug() << "Graphique redimensionné avec succès";
        
    } catch (const std::exception& e) {
        qCritical() << "Erreur lors du redimensionnement:" << e.what();
    }
}

void ChartView::onViewPortChanged()
{
    if (!m_financeChart || !m_chartViewer) {
        return;
    }
    
    // Redessiner le graphique avec les données visibles
    drawChartWithViewport();
}

void ChartView::onMouseMovePlotArea(QMouseEvent *event)
{
    Q_UNUSED(event);
    
    if (!m_financeChart || !m_chartViewer) {
        return;
    }
    
    // Ajouter le tracking avec ligne verticale
    trackFinance(m_financeChart, m_chartViewer->getPlotAreaMouseX());
    m_chartViewer->updateDisplay();
}

void ChartView::drawChartWithViewport()
{
    if (m_priceData.timestamps.empty()) {
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
        
        // Extraire les données visibles
        DoubleArray timeStamps = DoubleArray(&m_priceData.timestamps[startIndex], pointsToShow);
        DoubleArray openData = DoubleArray(&m_priceData.open[startIndex], pointsToShow);
        DoubleArray highData = DoubleArray(&m_priceData.high[startIndex], pointsToShow);
        DoubleArray lowData = DoubleArray(&m_priceData.low[startIndex], pointsToShow);
        DoubleArray closeData = DoubleArray(&m_priceData.close[startIndex], pointsToShow);
        DoubleArray volumeData = DoubleArray(&m_priceData.volume[startIndex], pointsToShow);
        
        // Utiliser la largeur actuelle du FinanceChart
        int chartWidth = m_financeChart ? m_financeChart->getWidth() : std::max(800, this->width() - 20);
        
        // Créer un nouveau FinanceChart
        if (m_financeChart) {
            delete m_financeChart;
        }
        
        m_financeChart = new FinanceChart(chartWidth);
        m_financeChart->setData(timeStamps, highData, lowData, openData, closeData, volumeData, 0);
        
        // Reconfigurer le graphique
        std::string title = "Trading - Points " + std::to_string(startIndex) + 
                           " à " + std::to_string(endIndex);
        m_financeChart->addTitle(title.c_str());
        
        int widgetHeight = this->height();
        int mainChartHeight = std::max(250, widgetHeight - 150);
        int volumeHeight = std::max(60, widgetHeight / 8);
        
        m_financeChart->addMainChart(mainChartHeight);
        m_financeChart->addCandleStick(0x00CC00, 0xFF3333);
        m_financeChart->addVolBars(volumeHeight, 0x99ff99, 0xff9999, 0x808080);
        
        // Assigner le nouveau graphique
        m_chartViewer->setChart(m_financeChart);
        
    } catch (const std::exception& e) {
        qCritical() << "Erreur dans drawChartWithViewport:" << e.what();
    }
}

void ChartView::trackFinance(MultiChart* m, int mouseX)
{
    // Implémentation du tracking de souris (gardée identique)
    DrawArea *d = m->initDynamicLayer();
    
    if (m->getChartCount() == 0)
        return;
    
    int xValue = (int)(((XYChart *)m->getChart(0))->getNearestXValue(mouseX));
    
    XYChart *c = 0;
    for (int i = 0; i < m->getChartCount(); ++i) {
        c = (XYChart *)m->getChart(i);
        
        std::ostringstream ohlcLegend;
        std::vector<std::string> legendEntries;
        
        for (int j = 0; j < c->getLayerCount(); ++j) {
            Layer *layer = c->getLayerByZ(j);
            
            if (!layer->getDataSetCount())
                continue;
            
            DataSet *dataSet = layer->getDataSetByZ(0);
            
            int ohlcLayer = 0;
            const char *name = dataSet->getDataName();
            if (name && ((std::string)name == "Open" || (std::string)name == "High" ||
                (std::string)name == "Low" || (std::string)name == "Close")) {
                ohlcLayer = 1;
            }
            
            int xIndex = layer->getXIndexOf(xValue);
            if (xIndex < 0)
                continue;
            
            if (ohlcLayer) {
                double openValue = layer->getDataSetByZ(0)->getValue(xIndex);
                double highValue = layer->getDataSetByZ(1)->getValue(xIndex);
                double lowValue = layer->getDataSetByZ(2)->getValue(xIndex);
                double closeValue = layer->getDataSetByZ(3)->getValue(xIndex);
                
                if (openValue != Chart::NoValue) {
                    ohlcLegend << "      <*block*>";
                    ohlcLegend << "O: " << c->formatValue(openValue, "{value|P4}");
                    ohlcLegend << ", H: " << c->formatValue(highValue, "{value|P4}");
                    ohlcLegend << ", L: " << c->formatValue(lowValue, "{value|P4}");
                    ohlcLegend << ", C: " << c->formatValue(closeValue, "{value|P4}");
                    
                    if (xIndex > 0) {
                        double lastCloseValue = layer->getDataSetByZ(3)->getValue(xIndex - 1);
                        if (lastCloseValue != Chart::NoValue) {
                            double change = closeValue - lastCloseValue;
                            double percent = change * 100 / lastCloseValue;
                            
                            std::string symbol = (change >= 0) ?
                                "<*font,color=008800*>▲" : "<*font,color=CC0000*>▼";
                            
                            ohlcLegend << "  " << symbol << " " << c->formatValue(change, "{value|P4}");
                            ohlcLegend << " (" << c->formatValue(percent, "{value|2}") << "%)<*/font*>";
                        }
                    }
                    
                    ohlcLegend << "<*/*>";
                }
            }
        }
        
        PlotArea *plotArea = c->getPlotArea();
        int plotAreaLeftX = plotArea->getLeftX() + c->getAbsOffsetX();
        int plotAreaTopY = plotArea->getTopY() + c->getAbsOffsetY();
        int plotAreaWidth = plotArea->getWidth();
        int plotAreaHeight = plotArea->getHeight();
        
        std::ostringstream legendText;
        legendText << "<*block,valign=top,maxWidth=" << (plotAreaWidth - 5)
            << "*><*font=Arial Bold*>[" << c->xAxis()->getFormattedLabel(xValue, "yyyy-MM-dd hh:nn:ss")
            << "]<*/font*>" << ohlcLegend.str();
        for (int i = ((int)legendEntries.size()) - 1; i >= 0; --i) {
            legendText << "      " << legendEntries[i];
        }
        legendText << "<*/*>";
        
        int xCoor = c->getXCoor(xValue) + c->getAbsOffsetX();
        d->vline(plotAreaTopY, plotAreaTopY + plotAreaHeight, xCoor, d->dashLineColor(0x000000, 0x0101));
        
        TTFText *t = d->text(legendText.str().c_str(), "Arial", 8);
        if (mouseX > plotAreaLeftX + plotAreaWidth / 2) {
            t->draw(plotAreaLeftX + 5, plotAreaTopY + 25, 0x000000, Chart::TopLeft);
        } else {
            t->draw(plotAreaLeftX + plotAreaWidth - 5, plotAreaTopY + 25, 0x000000, Chart::TopRight);
        }
        t->destroy();
    }
}

void ChartView::extractDataFromPython(void* data, void* stats)
{
    qDebug() << "=== DÉBUT extractDataFromPython ===";
    
    if (!data || !stats) {
        qDebug() << "Pointeurs data ou stats nuls";
        return;
    }
    
    try {
        qDebug() << "Acquisition du GIL local...";
        py::gil_scoped_acquire acquire;
        qDebug() << "GIL local acquis";
        
        qDebug() << "Cast des pointeurs Python...";
        py::object* dataObj = static_cast<py::object*>(data);
        py::object* statsObj = static_cast<py::object*>(stats);
        qDebug() << "Cast terminé - dataObj:" << dataObj << "statsObj:" << statsObj;
        
        if (!dataObj || !statsObj) {
            throw std::runtime_error("Objets Python invalides après cast");
        }
        
        qDebug() << "Début extraction des données de prix...";
        extractPriceData(dataObj);
        qDebug() << "Prix extraits, taille:" << m_priceData.close.size();
        
        qDebug() << "Début extraction des données de trades...";
        extractTradeData(statsObj);
        qDebug() << "Trades extraits, taille:" << m_tradeData.entry_times.size();
        
        qDebug() << "Début extraction des données d'équité...";
        extractEquityData(statsObj);
        qDebug() << "Équité extraite, taille:" << m_equityData.equity_values.size();
        
        qDebug() << "=== FIN extractDataFromPython ===";
                 
    } catch (const py::error_already_set& e) {
        qCritical() << "Erreur Python dans extractDataFromPython:" << e.what();
        throw std::runtime_error(QString("Erreur Python: %1").arg(e.what()).toStdString());
    } catch (const std::exception& e) {
        qCritical() << "Erreur C++ dans extractDataFromPython:" << e.what();
        throw;
    }
}

void ChartView::extractPriceData(void* data)
{
    qDebug() << "=== DÉBUT extractPriceData ===";
    
    try {
        qDebug() << "Cast de l'objet data...";
        py::object* dataObj = static_cast<py::object*>(data);
        if (!dataObj) {
            throw std::runtime_error("Objet dataObj est null");
        }
        qDebug() << "Cast réussi";
        
        qDebug() << "Déréférencement de l'objet...";
        py::object dataFrame = *dataObj;
        qDebug() << "Déréférencement réussi";
        
        // Tests de validation
        qDebug() << "Validation de l'objet...";
        if (dataFrame.ptr() == nullptr || dataFrame.is_none()) {
            throw std::runtime_error("DataFrame invalide");
        }
        
        if (!py::hasattr(dataFrame, "columns") || !py::hasattr(dataFrame, "index")) {
            throw std::runtime_error("DataFrame manque attributs essentiels");
        }
        
        qDebug() << "DataFrame validé - extraction des données réelles...";
        
        // Extraction des données
        try {
            // Extraire les informations sur le DataFrame
            py::object columns = dataFrame.attr("columns");
            py::list columns_list = py::list(columns);
            qDebug() << "Nombre de colonnes:" << py::len(columns_list);
            
            // Vérifier la taille du DataFrame
            size_t df_size = py::len(dataFrame);
            qDebug() << "Taille du DataFrame:" << df_size;
            
            if (df_size == 0) {
                throw std::runtime_error("DataFrame vide");
            }
            
            // Extraire les colonnes OHLCV
            qDebug() << "Extraction des colonnes OHLCV...";
            
            py::object open_col = dataFrame[py::str("Open")];
            py::object high_col = dataFrame[py::str("High")];
            py::object low_col = dataFrame[py::str("Low")];
            py::object close_col = dataFrame[py::str("Close")];
            py::object volume_col = dataFrame[py::str("Volume")];
            
            qDebug() << "Colonnes extraites, conversion en vecteurs C++...";
            
            // Convertir en vecteurs C++
            m_priceData.open = extractDoubleVector(&open_col);
            m_priceData.high = extractDoubleVector(&high_col);
            m_priceData.low = extractDoubleVector(&low_col);
            m_priceData.close = extractDoubleVector(&close_col);
            m_priceData.volume = extractDoubleVector(&volume_col);
            
            qDebug() << "Données OHLCV extraites:";
            qDebug() << "- Open:" << m_priceData.open.size() << "éléments";
            qDebug() << "- High:" << m_priceData.high.size() << "éléments";
            qDebug() << "- Low:" << m_priceData.low.size() << "éléments";
            qDebug() << "- Close:" << m_priceData.close.size() << "éléments";
            qDebug() << "- Volume:" << m_priceData.volume.size() << "éléments";
            
            // Extraire l'index (timestamps)
            qDebug() << "Extraction de l'index...";
            py::object index = dataFrame.attr("index");
            size_t indexSize = py::len(index);
            qDebug() << "Taille de l'index:" << indexSize;
            
            m_priceData.timestamps.clear();
            m_priceData.timestamps.reserve(indexSize);
            
            // Conversion des timestamps
            for (size_t i = 0; i < indexSize; i++) {
                try {
                    py::object timestamp = index[py::int_(i)];
                    
                    if (py::hasattr(timestamp, "timestamp")) {
                        py::object timestamp_seconds = timestamp.attr("timestamp")();
                        double epoch_time = timestamp_seconds.cast<double>();
                        m_priceData.timestamps.push_back(epoch_time);
                    } else {
                        m_priceData.timestamps.push_back(static_cast<double>(i));
                    }
                } catch (const std::exception& e) {
                    qWarning() << "Erreur conversion timestamp" << i << "- utilisation index:" << e.what();
                    m_priceData.timestamps.push_back(static_cast<double>(i));
                }
            }
            
            qDebug() << "Timestamps extraits:" << m_priceData.timestamps.size() << "éléments";
            
            // Validation finale
            if (m_priceData.timestamps.size() != m_priceData.close.size()) {
                throw std::runtime_error("Incohérence dans les tailles des données");
            }
            
            qDebug() << "=== EXTRACTION RÉELLE TERMINÉE AVEC SUCCÈS ===";
            
        } catch (const std::exception& e) {
            qCritical() << "Erreur lors de l'extraction réelle:" << e.what();
            throw;
        }
        
    } catch (const py::error_already_set& e) {
        qCritical() << "Erreur Python dans extractPriceData:" << e.what();
        throw std::runtime_error(QString("Erreur Python prix: %1").arg(e.what()).toStdString());
    } catch (const std::exception& e) {
        qCritical() << "Erreur C++ dans extractPriceData:" << e.what();
        throw;
    }
}

void ChartView::extractTradeData(void* stats) 
{ 
    qDebug() << "extractTradeData appelé (stub)";
    Q_UNUSED(stats); 
}

void ChartView::extractEquityData(void* stats) 
{ 
    qDebug() << "extractEquityData appelé (stub)";
    Q_UNUSED(stats); 
}

DoubleArray ChartView::vectorToDoubleArray(const std::vector<double>& vec)
{
    if (vec.empty()) {
        return DoubleArray();
    }
    
    return DoubleArray(&vec[0], static_cast<int>(vec.size()));
}


std::vector<double> ChartView::extractDoubleVector(void* pyObj)
{
    std::vector<double> result;
    
    try {
        py::object* obj = static_cast<py::object*>(pyObj);
        if (!obj) {
            return result;
        }
        
        py::object pyVector = *obj;
        if (!pyVector) {
            return result;
        }
        
        py::list pyList = py::list(pyVector);
        size_t listSize = py::len(pyList);
        
        if (listSize == 0) {
            return result;
        }
        
        result.reserve(listSize);
        
        for (size_t i = 0; i < listSize; i++) {
            try {
                double value = pyList[i].cast<double>();
                result.push_back(value);
            } catch (const std::exception& e) {
                qWarning() << "Erreur conversion élément" << i << ":" << e.what();
                result.push_back(0.0);
            }
        }
        
    } catch (const std::exception& e) {
        qCritical() << "Erreur dans extractDoubleVector:" << e.what();
    }
    
    return result;
}

bool ChartView::hasValidData() const
{
    return !m_priceData.timestamps.empty() && 
           m_priceData.timestamps.size() == m_priceData.close.size() &&
           m_priceData.timestamps.size() == m_priceData.open.size() &&
           m_priceData.timestamps.size() == m_priceData.high.size() &&
           m_priceData.timestamps.size() == m_priceData.low.size();
}

void ChartView::showPlaceholder(const QString& message)
{
    if (m_chartPlaceholder) {
        m_chartPlaceholder->setText(message);
        m_chartPlaceholder->setVisible(true);
    }
}

void ChartView::debugChart()
{
    qDebug() << "=== DEBUG CHART ===";
    qDebug() << "m_financeChart:" << m_financeChart;
    qDebug() << "m_chartViewer:" << m_chartViewer;
    
    if (m_financeChart) {
        qDebug() << "FinanceChart largeur:" << m_financeChart->getWidth();
    }
    
    if (m_chartViewer) {
        qDebug() << "ChartViewer taille:" << m_chartViewer->size();
        qDebug() << "ViewPort Width:" << m_chartViewer->getViewPortWidth();
        qDebug() << "ViewPort Left:" << m_chartViewer->getViewPortLeft();
    }
    
    qDebug() << "Widget taille:" << this->size();
    qDebug() << "Données - Timestamps:" << m_priceData.timestamps.size();
    qDebug() << "Données - Close:" << m_priceData.close.size();
    qDebug() << "=== FIN DEBUG CHART ===";
}