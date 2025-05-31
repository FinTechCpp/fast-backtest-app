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
    , m_cachedResults(nullptr)
    , m_dataExtracted(false)
    , m_currentResults(nullptr)
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

void ChartView::updateData(BacktestResults* results)
{
    QTime start = QTime::currentTime();
    
    m_currentResults = results;
    
    qDebug() << "=== DÉBUT ChartView::updateData() ===";
    qDebug() << "BacktestResults pointer:" << results;

    // Vérifier si les données ont déjà été extraites pour ces pointeurs
    if (m_dataExtracted && m_cachedResults == results) {
        qDebug() << "Données déjà en cache, pas de ré-extraction nécessaire";
        if (hasValidData()) {
            createChart();
        }
        qDebug() << "=== FIN ChartView::updateData() (depuis cache) ===";
        return;
    }
    
    // Mettre en cache les nouveaux pointeurs
    m_cachedResults = results;
    
    if (!results || !results->data) {
        qDebug() << "Données nulles détectées";
        clear();
        showPlaceholder("Aucune donnée disponible");
        return;
    }
    
    try {
        qDebug() << "Début extraction des données C++...";
        extractDataFromCpp(results);
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
    
    m_currentResults = nullptr;
    m_cachedResults = nullptr;
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


void ChartView::extractDataFromCpp(BacktestResults* results)
{
    qDebug() << "=== DÉBUT extractDataFromCpp ===";
    
    if (!results || !results->data) {
        qDebug() << "Pointeurs résultats ou data nuls";
        return;
    }
    
    try {
        qDebug() << "Début extraction des données de prix...";
        extractPriceData(results);
        qDebug() << "Prix extraits, taille:" << m_priceData.close.size();
        
        qDebug() << "Début extraction des données de trades...";
        extractTradeData(results);
        qDebug() << "Trades extraits, taille:" << m_tradeData.entry_times.size();
        
        qDebug() << "Début extraction des données d'équité...";
        extractEquityData(results);
        qDebug() << "Équité extraite, taille:" << m_equityData.equity_values.size();
        
        qDebug() << "=== FIN extractDataFromCpp ===";
                 
    } catch (const std::exception& e) {
        qCritical() << "Erreur C++ dans extractDataFromCpp:" << e.what();
        throw;
    }
}

void ChartView::extractPriceData(BacktestResults* results)
{
    qDebug() << "=== DÉBUT extractPriceData ===";
    
    try {
        auto& beData = results->data;
        if (!beData) {
            throw std::runtime_error("Objet be::Data est null");
        }
        
        // Nettoyer les anciennes données
        m_priceData = PriceData();
        
        // Obtenir la taille des données
        size_t dataSize = beData->size();
        qDebug() << "Taille des données OHLC:" << dataSize;
        
        if (dataSize == 0) {
            throw std::runtime_error("Données OHLC vides");
        }
        
        // Réserver la capacité pour les vecteurs
        m_priceData.timestamps.reserve(dataSize);
        m_priceData.open.reserve(dataSize);
        m_priceData.high.reserve(dataSize);
        m_priceData.low.reserve(dataSize);
        m_priceData.close.reserve(dataSize);
        m_priceData.volume.reserve(dataSize);
        
        // Extraire les données OHLCV pour chaque barre
        for (size_t i = 0; i < dataSize; ++i) {
            // Convertir la date en timestamp pour le graphique
            const be::Date& date = beData->getDate(i);
            double timestamp = date.toTimestamp();
            
            // Ajouter les données aux vecteurs
            m_priceData.timestamps.push_back(timestamp);
            m_priceData.open.push_back(beData->Open(i));
            m_priceData.high.push_back(beData->High(i));
            m_priceData.low.push_back(beData->Low(i));
            m_priceData.close.push_back(beData->Close(i));
            m_priceData.volume.push_back(beData->Volume(i));
        }
        
        qDebug() << "Données OHLCV extraites:";
        qDebug() << "- Open:" << m_priceData.open.size() << "éléments";
        qDebug() << "- High:" << m_priceData.high.size() << "éléments";
        qDebug() << "- Low:" << m_priceData.low.size() << "éléments";
        qDebug() << "- Close:" << m_priceData.close.size() << "éléments";
        qDebug() << "- Volume:" << m_priceData.volume.size() << "éléments";
        
    } catch (const std::exception& e) {
        qCritical() << "Erreur dans extractPriceData:" << e.what();
        throw std::runtime_error(QString("Erreur prix: %1").arg(e.what()).toStdString());
    }
}

void ChartView::extractTradeData(BacktestResults* results)
{
    qDebug() << "=== DÉBUT extractTradeData ===";
    
    try {
        const auto& trades = results->stats.trades;
        
        // Nettoyer les anciennes données
        m_tradeData = TradeData();
        
        // Si aucun trade, retourner
        if (trades.empty()) {
            qDebug() << "Aucun trade à extraire";
            return;
        }
        
        // Réserver la capacité
        size_t numTrades = trades.size();
        m_tradeData.entry_times.reserve(numTrades);
        m_tradeData.exit_times.reserve(numTrades);
        m_tradeData.entry_prices.reserve(numTrades);
        m_tradeData.exit_prices.reserve(numTrades);
        m_tradeData.types.reserve(numTrades);
        m_tradeData.pnl.reserve(numTrades);
        
        // Extraire chaque trade
        for (const auto& trade : trades) {
            // Convertir dates en timestamps
            double entryTime = trade->entryDate().toTimestamp();
            double exitTime = trade->exitDate().toTimestamp();

            // Déterminer le type (LONG/SHORT)
            QString type = trade->size() > 0 ? "LONG" : "SHORT";

            // Ajouter les données aux vecteurs
            m_tradeData.entry_times.push_back(entryTime);
            m_tradeData.exit_times.push_back(exitTime);
            m_tradeData.entry_prices.push_back(trade->entryPrice());
            m_tradeData.exit_prices.push_back(trade->exitPrice());
            m_tradeData.types.push_back(type);
            m_tradeData.pnl.push_back(trade->pl());
        }
        
        qDebug() << "Trades extraits:" << m_tradeData.entry_times.size();
        
    } catch (const std::exception& e) {
        qCritical() << "Erreur dans extractTradeData:" << e.what();
        // Ne pas faire échouer toute l'extraction si les trades échouent
    }
}

void ChartView::extractEquityData(BacktestResults* results)
{
    qDebug() << "=== DÉBUT extractEquityData ===";
    
    try {
        const auto& equityCurve = results->stats.equityCurve;
        
        // Nettoyer les anciennes données
        m_equityData = EquityData();
        
        // Si pas de courbe d'équité, retourner
        if (equityCurve.empty()) {
            qDebug() << "Aucune donnée d'équité à extraire";
            return;
        }
        
        // Réserver la capacité
        size_t numPoints = equityCurve.size();
        m_equityData.equity_values.reserve(numPoints);
        m_equityData.timestamps.reserve(numPoints);
        
        // Dans cette implémentation, nous utilisons les mêmes timestamps que pour les prix
        // car la courbe d'équité est typiquement alignée avec les barres de prix
        if (!m_priceData.timestamps.empty() && m_priceData.timestamps.size() >= numPoints) {
            // Copier les timestamps des prix
            m_equityData.timestamps = m_priceData.timestamps;
            // Limiter à la taille de la courbe d'équité si nécessaire
            if (m_equityData.timestamps.size() > numPoints) {
                m_equityData.timestamps.resize(numPoints);
            }
        } else {
            // Fallback: générer des timestamps séquentiels si les prix ne sont pas disponibles
            for (size_t i = 0; i < numPoints; ++i) {
                m_equityData.timestamps.push_back(static_cast<double>(i));
            }
        }
        
        // Copier les valeurs d'équité
        m_equityData.equity_values = equityCurve;
        
        // Calculer le drawdown (optionnel)
        double peak = equityCurve[0];
        for (size_t i = 0; i < numPoints; ++i) {
            if (equityCurve[i] > peak) {
                peak = equityCurve[i];
            }
            double dd = (peak - equityCurve[i]) / peak * 100.0; // En pourcentage
            m_equityData.drawdown.push_back(dd);
        }
        
        qDebug() << "Données d'équité extraites:" << m_equityData.equity_values.size() << "points";
        
    } catch (const std::exception& e) {
        qCritical() << "Erreur dans extractEquityData:" << e.what();
        // Ne pas faire échouer toute l'extraction si l'équité échoue
    }
}

// Conversion de vector<double> à DoubleArray pour ChartDirector
DoubleArray ChartView::vectorToDoubleArray(const std::vector<double>& vec) {
    if (vec.empty()) {
        return DoubleArray(nullptr, 0);
    }
    return DoubleArray(vec.data(), static_cast<int>(vec.size()));
}

// Vérification des données
bool ChartView::hasValidData() const {
    return !m_priceData.timestamps.empty() && 
           !m_priceData.open.empty() && 
           !m_priceData.high.empty() && 
           !m_priceData.low.empty() && 
           !m_priceData.close.empty();
}

// Affichage d'un placeholder
void ChartView::showPlaceholder(const QString& message) {
    if (m_chartPlaceholder) {
        m_chartPlaceholder->setText(message);
        m_chartPlaceholder->setVisible(true);
    }
    
    if (m_chartViewer) {
        m_chartViewer->setVisible(false);
    }
}

// Calcul des bougies Heikin Ashi
void ChartView::calculateHeikinAshi(
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