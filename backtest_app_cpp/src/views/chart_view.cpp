#include "chart_view.h"
#include <QDebug>
#include <QTimer>
#include <QResizeEvent>
#include <QApplication>
#include <QButtonGroup>
#include <QFrame>
#include <QTime>
#include <QLabel>

ChartView::ChartView(QWidget* parent)
    : BaseView(parent)
    , m_cachedData(nullptr)
    , m_cachedStats(nullptr)
    , m_dataExtracted(false)
    , m_currentData(nullptr)
    , m_currentStats(nullptr)
    , m_chartPlaceholder(nullptr)
    , m_chartTypeCombo(nullptr)
    , m_settingsTitle(nullptr)
    , m_financeChart(nullptr)
    , m_chartViewer(nullptr)
    , m_leftPanel(nullptr)
    , m_rightPanel(nullptr)
    , m_currentChartType("CandleStick")
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
    qDebug() << "ChartView détruite";
}

void ChartView::setupUI()
{
    QTime start = QTime::currentTime();
    
    qDebug() << "ChartView::setupUI() - Configuration du layout principal";
    
    // Configurer le layout principal pour occuper tout l'espace
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);
    
    // Créer un layout horizontal pour les panneaux gauche et droit
    QHBoxLayout* horizontalLayout = new QHBoxLayout();
    horizontalLayout->setContentsMargins(0, 0, 0, 0);
    horizontalLayout->setSpacing(0);
    m_mainLayout->addLayout(horizontalLayout);
    
    // Créer le panneau gauche avec une largeur fixe de 155 pixels
    m_leftPanel = new QWidget();
    m_leftPanel->setObjectName("leftPanel");
    m_leftPanel->setStyleSheet("#leftPanel { background-color: #BADDFF; }");
    m_leftPanel->setFixedWidth(155);
    
    // Ajouter un layout vertical au panneau gauche
    QVBoxLayout* leftPanelLayout = new QVBoxLayout(m_leftPanel);
    leftPanelLayout->setContentsMargins(8, 8, 8, 8);
    leftPanelLayout->setSpacing(10);
    
    // Ajouter un titre au panneau gauche
    m_settingsTitle = new QLabel("Settings");
    m_settingsTitle->setAlignment(Qt::AlignCenter);
    m_settingsTitle->setStyleSheet("font-weight: bold; font-size: 16px;");
    leftPanelLayout->addWidget(m_settingsTitle);
    
    // Ajouter le sélecteur de type de graphique
    QLabel* chartTypeLabel = new QLabel("Chart Type");
    chartTypeLabel->setStyleSheet("font-weight: bold;");
    leftPanelLayout->addWidget(chartTypeLabel);
    
    m_chartTypeCombo = new QComboBox();
    m_chartTypeCombo->addItem("CandleStick", "CandleStick");
    m_chartTypeCombo->addItem("Heikin Ashi", "HeikinAshi");
    m_chartTypeCombo->addItem("Line", "Close");
    m_chartTypeCombo->addItem("OHLC", "OHLC");
    leftPanelLayout->addWidget(m_chartTypeCombo);
    
    // Connecter le signal de changement à notre slot
    connect(m_chartTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ChartView::onChartTypeChanged);
    
    // Type de graphique par défaut
    m_currentChartType = "CandleStick";
    
    // Ajouter un espace extensible en bas
    leftPanelLayout->addStretch();
    
    // Créer un séparateur vertical
    QFrame* separator = new QFrame();
    separator->setFrameStyle(QFrame::VLine | QFrame::Plain);
    separator->setStyleSheet("color: #CCCCCC;"); // Couleur de la ligne
    
    // Créer le panneau droit qui contiendra le graphique
    m_rightPanel = new QWidget();
    m_rightPanel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    
    // Layout pour le panneau droit
    QVBoxLayout* rightPanelLayout = new QVBoxLayout(m_rightPanel);
    rightPanelLayout->setContentsMargins(0, 0, 0, 0);
    rightPanelLayout->setSpacing(0);
    
    // Créer le placeholder initial qui occupe tout l'espace du panneau droit
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
    
    // Ajouter le placeholder au layout du panneau droit
    rightPanelLayout->addWidget(m_chartPlaceholder);
    
    // Ajouter les composants au layout horizontal
    horizontalLayout->addWidget(m_leftPanel);
    horizontalLayout->addWidget(separator);
    horizontalLayout->addWidget(m_rightPanel);
    
    // Configurer le widget pour s'étendre
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    
    int elapsed = start.msecsTo(QTime::currentTime());
    qInfo() << "ChartView::setupUI() took" << elapsed << "ms";
}

void ChartView::resizeEvent(QResizeEvent* event)
{
    BaseView::resizeEvent(event);
    
    qDebug() << "ChartView redimensionnée vers:" << event->size();
    
    // Si on a un graphique, utiliser une taille calculée dynamiquement
    // if (m_financeChart && m_chartViewer && m_rightPanel) {
    //     QTimer::singleShot(100, [this]() {
    //         int newWidth = std::max(800, m_rightPanel->width() - 20);
    //         resizeChart(newWidth);
    //     });
    // }
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
        if (m_chartViewer->parentWidget()) {
            m_chartViewer->parentWidget()->layout()->removeWidget(m_chartViewer);
        }
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
        
        // Nettoyer le viewer précédent
        if (m_chartViewer) {
            if (m_chartViewer->parentWidget()) {
                m_chartViewer->parentWidget()->layout()->removeWidget(m_chartViewer);
            }
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
        
        // Variables pour les données Heikin Ashi (si nécessaire)
        std::vector<double> ha_open, ha_high, ha_low, ha_close;
        DoubleArray haOpenArray, haHighArray, haLowArray, haCloseArray;
        
        // Si le type est HeikinAshi, calculer les valeurs Heikin Ashi
        bool isHeikinAshi = (m_currentChartType == "HeikinAshi");
        if (isHeikinAshi) {
            // Calculer les valeurs Heikin Ashi
            calculateHeikinAshi(m_priceData.open, m_priceData.high, 
                               m_priceData.low, m_priceData.close,
                               ha_open, ha_high, ha_low, ha_close);
            
            // Convertir en DoubleArray pour ChartDirector
            haOpenArray = vectorToDoubleArray(ha_open);
            haHighArray = vectorToDoubleArray(ha_high);
            haLowArray = vectorToDoubleArray(ha_low);
            haCloseArray = vectorToDoubleArray(ha_close);
        }
        
        qDebug() << "Données converties - timeStamps:" << timeStamps.len << "points";
        
        // Calculer la largeur optimale pour le graphique
        int chartWidth = 1200;  // Largeur fixe
        
        // Masquer le placeholder
        if (m_chartPlaceholder) {
            m_chartPlaceholder->setVisible(false);
        }
        
        // Créer le QChartViewer qui occupe tout l'espace du panneau droit
        m_chartViewer = new QChartViewer(m_rightPanel);
        
        // Configurer le viewer pour occuper tout l'espace disponible
        m_chartViewer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        m_chartViewer->setMouseUsage(Chart::MouseUsageScroll);
        m_chartViewer->setMouseTracking(true);
        m_chartViewer->setMouseWheelZoomRatio(1.1);
        m_chartViewer->setScrollDirection(Chart::DirectionHorizontal);
        m_chartViewer->setZoomDirection(Chart::DirectionHorizontal);
        
        // Configurer le range complet pour le viewport
        m_chartViewer->setFullRange("x", 0, timeStamps.len - 1);
        
        // Utiliser la méthode centralisée pour créer le graphique
        if (isHeikinAshi) {
            m_financeChart = drawChart(m_chartViewer, timeStamps, haHighArray, haLowArray, 
                                      haOpenArray, haCloseArray, volumeData, chartWidth);
        } else {
            m_financeChart = drawChart(m_chartViewer, timeStamps, highData, lowData, 
                                     openData, closeData, volumeData, chartWidth);
        }
        
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
        
        // Ajouter au layout du panneau droit
        QLayout* rightLayout = m_rightPanel->layout();
        rightLayout->addWidget(m_chartViewer);
        
        // Connecter les signaux essentiels
        connect(m_chartViewer, &QChartViewer::viewPortChanged, 
                this, &ChartView::onViewPortChanged);
        connect(m_chartViewer, &QChartViewer::mouseMovePlotArea, 
                this, &ChartView::onMouseMovePlotArea);
        
        // Forcer la mise à jour initiale
        m_chartViewer->updateViewPort(true, false);
        
        qDebug() << "Graphique FinanceChart créé avec succès !";
        
    } catch (const std::exception& e) {
        qCritical() << "Erreur lors de la création du FinanceChart:" << e.what();
        showPlaceholder(QString("Erreur graphique: %1").arg(e.what()));
    }
    
    qDebug() << "=== FIN createChart ===";
}

void ChartView::resizeChart(int newWidth)
{
    if (!m_financeChart || !m_chartViewer) {
        return;
    }
    
    qDebug() << "Redimensionnement du graphique vers:" << newWidth;
    
    // Sauvegarder l'état du viewport actuel
    double currentLeft = m_chartViewer->getViewPortLeft();
    double currentWidth = m_chartViewer->getViewPortWidth();
    
    if (m_priceData.timestamps.empty()) {
        return;
    }
    
    try {
        // Convertir les données
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
        bool isHeikinAshi = (m_currentChartType == "HeikinAshi");
        if (isHeikinAshi) {
            calculateHeikinAshi(m_priceData.open, m_priceData.high, 
                               m_priceData.low, m_priceData.close,
                               ha_open, ha_high, ha_low, ha_close);
            
            haOpenArray = vectorToDoubleArray(ha_open);
            haHighArray = vectorToDoubleArray(ha_high);
            haLowArray = vectorToDoubleArray(ha_low);
            haCloseArray = vectorToDoubleArray(ha_close);
            
            // Utiliser la méthode centralisée pour créer/mettre à jour le graphique
            m_financeChart = drawChart(m_chartViewer, timeStamps, haHighArray, haLowArray, 
                                     haOpenArray, haCloseArray, volumeData, newWidth);
        } else {
            // Utiliser la méthode centralisée pour créer/mettre à jour le graphique
            m_financeChart = drawChart(m_chartViewer, timeStamps, highData, lowData, 
                                     openData, closeData, volumeData, newWidth);
        }
        
        // Restaurer le viewport
        m_chartViewer->setViewPortLeft(currentLeft);
        m_chartViewer->setViewPortWidth(currentWidth);
        m_chartViewer->updateViewPort(true, false);
        
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

void ChartView::onChartTypeChanged(int index)
{
    if (m_chartTypeCombo) {
        QVariant data = m_chartTypeCombo->itemData(index);
        if (data.isValid()) {
            QString chartType = data.toString();
            qDebug() << "Type de graphique changé pour:" << chartType;
            
            // Ne recréez le graphique que si le type a réellement changé
            if (m_currentChartType != chartType) {
                m_currentChartType = chartType;
                
                // Si nous avons déjà des données valides, recréer le graphique
                if (hasValidData() && m_chartViewer) {
                    // La méthode createChart() va recréer entièrement le graphique
                    // avec le nouveau type
                    createChart();
                    
                    // Force la mise à jour immédiate
                    m_chartViewer->updateViewPort(true, true);
                }
            }
        }
    }
}

void ChartView::drawChartWithViewport()
{
    if (m_priceData.timestamps.empty() || !m_chartViewer) {
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
        
        // Variables pour les données Heikin Ashi (si nécessaire)
        std::vector<double> ha_open, ha_high, ha_low, ha_close;
        DoubleArray haOpenArray, haHighArray, haLowArray, haCloseArray;
        
        // Taille fixe standard pour le graphique
        int chartWidth = 1200;
        
        // Si le type est HeikinAshi, calculer les valeurs Heikin Ashi
        bool isHeikinAshi = (m_currentChartType == "HeikinAshi");
        if (isHeikinAshi) {
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
            
            // Utiliser la méthode centralisée pour créer le graphique
            m_financeChart = drawChart(m_chartViewer, timeStamps, haHighArray, haLowArray, 
                                     haOpenArray, haCloseArray, volumeData, chartWidth);
        } else {
            // Utiliser la méthode centralisée pour créer le graphique
            m_financeChart = drawChart(m_chartViewer, timeStamps, highData, lowData, 
                                     openData, closeData, volumeData, chartWidth);
        }
        
    } catch (const std::exception& e) {
        qCritical() << "Erreur dans drawChartWithViewport:" << e.what();
    }
}

void ChartView::trackFinance(MultiChart *m, int mouseX)
{
    // Clear the current dynamic layer and get the DrawArea object to draw on it.
    DrawArea *d = m->initDynamicLayer();

    // It is possible for a FinanceChart to be empty, so we need to check for it.
    if (m->getChartCount() == 0)
        return ;

    // Get the data x-value that is nearest to the mouse
    int xValue = (int)(((XYChart *)m->getChart(0))->getNearestXValue(mouseX));

    // Iterate the XY charts (main price chart and indicator charts) in the FinanceChart
    XYChart *c = 0;
    for(int i = 0; i < m->getChartCount(); ++i) {
        c = (XYChart *)m->getChart(i);

        // Variables to hold the legend entries
        std::ostringstream ohlcLegend;
        std::vector<std::string> legendEntries;

        // Iterate through all layers to find the highest data point
        for(int j = 0; j < c->getLayerCount(); ++j) {
            Layer *layer = c->getLayerByZ(j);
            int xIndex = layer->getXIndexOf(xValue);
            int dataSetCount = layer->getDataSetCount();

            // In a FinanceChart, only layers showing OHLC data can have 4 data sets
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

                    // We also draw an upward or downward triangle for up and down days and the %
                    // change
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
                // Iterate through all the data sets in the layer
                for(int k = 0; k < layer->getDataSetCount(); ++k) {
                    DataSet *dataSet = layer->getDataSetByZ(k);

                    std::string name = dataSet->getDataName();
                    double value = dataSet->getValue(xIndex);
                    if ((0 != name.size()) && (value != Chart::NoValue)) {

                        // In a FinanceChart, the data set name consists of the indicator name and its
                        // latest value. It is like "Vol: 123M" or "RSI (14): 55.34". As we are
                        // generating the values dynamically, we need to extract the indictor name
                        // out, and also the volume unit (if any).

						// The volume unit
                        std::string unitChar;

                        // The indicator name is the part of the name up to the colon character.
						int delimiterPosition = (int)name.find(':');
                        if ((int)name.npos != delimiterPosition) {
							
							// The unit, if any, is the trailing non-digit character(s).
							int lastDigitPos = (int)name.find_last_of("0123456789");
                            if (((int)name.npos != lastDigitPos) && (lastDigitPos + 1 < (int)name.size())
                                && (lastDigitPos > delimiterPosition))
								unitChar = name.substr(lastDigitPos + 1);

							name.resize(delimiterPosition);
                        }

                        // In a FinanceChart, if there are two data sets, it must be representing a
                        // range.
                        if (dataSetCount == 2) {
                            // We show both values in the range in a single legend entry
                            value = layer->getDataSet(0)->getValue(xIndex);
                            double value2 = layer->getDataSet(1)->getValue(xIndex);
                            name = name + ": " + c->formatValue((std::min)(value, value2), "{value|P3}");
                            name = name + " - " + c->formatValue((std::max)(value, value2), "{value|P3}");
                        } else {
                            // In a FinanceChart, only the layer for volume bars has 3 data sets for
                            // up/down/flat days
                            if (dataSetCount == 3) {
                                // The actual volume is the sum of the 3 data sets.
                                value = layer->getDataSet(0)->getValue(xIndex) + layer->getDataSet(1
                                    )->getValue(xIndex) + layer->getDataSet(2)->getValue(xIndex);
                            }

                            // Create the legend entry
                            name = name + ": " + c->formatValue(value, "{value|P3}") + unitChar;
                        }

                        // Build the legend entry, consist of a colored square box and the name (with
                        // the data value in it).
                        std::ostringstream legendEntry;
						legendEntry << "<*block*><*img=@square,width=8,edgeColor=000000,color=" 
                            << std::hex << dataSet->getDataColor() << "*> " << name << "<*/*>";
                        legendEntries.push_back(legendEntry.str());
                    }
                }
            }
        }

        // Get the plot area position relative to the entire FinanceChart
        PlotArea *plotArea = c->getPlotArea();
        int plotAreaLeftX = plotArea->getLeftX() + c->getAbsOffsetX();
        int plotAreaTopY = plotArea->getTopY() + c->getAbsOffsetY();

		// The legend begins with the date label, then the ohlcLegend (if any), and then the
		// entries for the indicators.
        std::ostringstream legendText;
		legendText << "<*block,valign=top,maxWidth=" << (plotArea->getWidth() - 5) 
			<< "*><*font=Arial Bold*>[" << c->xAxis()->getFormattedLabel(xValue, "mmm dd, yyyy")
			<< "]<*/font*>" << ohlcLegend.str();
		for (int i = ((int)legendEntries.size()) - 1; i >= 0; --i) {
			legendText << "      " << legendEntries[i];
		}
		legendText << "<*/*>";

        // Draw a vertical track line at the x-position
        d->vline(plotAreaTopY, plotAreaTopY + plotArea->getHeight(), c->getXCoor(xValue) +
            c->getAbsOffsetX(), d->dashLineColor(0x000000, 0x0101));

        // Display the legend on the top of the plot area
        TTFText *t = d->text(legendText.str().c_str(), "Arial", 8);
        t->draw(plotAreaLeftX + 5, plotAreaTopY + 3, 0x000000, Chart::TopLeft);
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

void ChartView::calculateHeikinAshi(const std::vector<double>& open, 
                                    const std::vector<double>& high,
                                    const std::vector<double>& low, 
                                    const std::vector<double>& close,
                                    std::vector<double>& ha_open, 
                                    std::vector<double>& ha_high,
                                    std::vector<double>& ha_low,
                                    std::vector<double>& ha_close)
{
    int size = open.size();
    ha_open.resize(size);
    ha_high.resize(size);
    ha_low.resize(size);
    ha_close.resize(size);
    
    // Calculer les valeurs HA_Close: (Open + High + Low + Close) / 4
    for (int i = 0; i < size; ++i) {
        ha_close[i] = (open[i] + high[i] + low[i] + close[i]) / 4.0;
    }
    
    // Calculer les valeurs HA_Open: (HA_Open_previous + HA_Close_previous) / 2
    ha_open[0] = open[0]; // Pour la première bougie, HA_Open = Open
    for (int i = 1; i < size; ++i) {
        ha_open[i] = (ha_open[i-1] + ha_close[i-1]) / 2.0;
    }
    
    // Calculer les valeurs HA_High et HA_Low
    for (int i = 0; i < size; ++i) {
        ha_high[i] = std::max(high[i], std::max(ha_open[i], ha_close[i]));
        ha_low[i] = std::min(low[i], std::min(ha_open[i], ha_close[i]));
    }
}

/**
 * Méthode centrale pour dessiner un graphique financier
 * @param viewer Le QChartViewer sur lequel afficher le graphique
 * @param timestamps Les timestamps
 * @param highData Les prix hauts
 * @param lowData Les prix bas
 * @param openData Les prix d'ouverture
 * @param closeData Les prix de fermeture
 * @param volumeData Les volumes
 * @param chartWidth Largeur du graphique (en pixels)
 */
FinanceChart* ChartView::drawChart(QChartViewer* viewer, 
                               const DoubleArray& timestamps, 
                               const DoubleArray& highData, 
                               const DoubleArray& lowData, 
                               const DoubleArray& openData, 
                               const DoubleArray& closeData,
                               const DoubleArray& volumeData,
                               int chartWidth)
{
    // Nettoyage du graphique précédent si nécessaire
    if (m_financeChart) {
        delete m_financeChart;
        m_financeChart = nullptr;
    }
    
    // Créer un nouveau FinanceChart avec la largeur spécifiée
    FinanceChart* c = new FinanceChart(chartWidth);
    
    // Configurer les données
    c->setData(timestamps, highData, lowData, openData, closeData, volumeData, 0);
    
    // Ajouter le titre du graphique avec indication du type
    std::string chartTypeStr = m_currentChartType.toStdString();
    std::string title = "Graphique de trading (" + chartTypeStr + ") - " + 
                       std::to_string(timestamps.len) + " points";
    c->addTitle(title.c_str());
    
    // Hauteurs pour les différentes parties du graphique (proportionnelles ou fixes)
    int mainChartHeight = 500;  // Hauteur du graphique principal
    int volumeHeight = 100;     // Hauteur du graphique de volume
    
    // Ajouter le graphique principal
    c->addMainChart(mainChartHeight);
    
    // Ajouter le type de graphique approprié selon le type actuel
    if (m_currentChartType == "CandleStick" || m_currentChartType == "HeikinAshi") {
        c->addCandleStick(0x00CC00, 0xFF3333); // Vert/Rouge pour les bougies
    } else if (m_currentChartType == "OHLC") {
        c->addHLOC(0x00CC00, 0xFF3333); // Vert/Rouge pour les barres OHLC
    } else if (m_currentChartType == "Close") {
        c->addCloseLine(0x000088); // Ligne bleue pour le prix de clôture
    }
    
    // Ajouter le graphique de volume
    c->addVolBars(volumeHeight, 0x99ff99, 0xff9999, 0x808080);
    
    // Assigner le graphique au viewer
    if (viewer) {
        viewer->setChart(c);
    }
    
    // Retourner le graphique créé
    return c;
}