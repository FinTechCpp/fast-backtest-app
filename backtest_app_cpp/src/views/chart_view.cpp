#include "chart_view.h"
#include <QDebug>
#include <QTimer>
#include <QResizeEvent>
#include <QApplication>  // AJOUTER CETTE LIGNE
#include <QButtonGroup>  // Si pas déjà inclus

ChartView::ChartView(QWidget* parent)  // Changé de QObject* à QWidget*
    : BaseView(parent)
    , m_cachedData(nullptr)
    , m_cachedStats(nullptr)
    , m_dataExtracted(false)
    , m_currentData(nullptr)
    , m_currentStats(nullptr)
    , m_chartPlaceholder(nullptr)
    , m_controlsWidget(nullptr)
    , m_controlsLayout(nullptr)
    , m_heikinAshiCheckbox(nullptr)
    , m_volumeCheckbox(nullptr)
    , m_equityCheckbox(nullptr)
    , m_addIndicatorBtn(nullptr)
    , m_indicatorsCombo(nullptr)
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
    
    // Créer le placeholder initial
    m_chartPlaceholder = new QLabel("Exécutez le backtest pour afficher les graphiques");
    m_chartPlaceholder->setAlignment(Qt::AlignCenter);
    m_chartPlaceholder->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    
    // Initialiser la configuration des indicateurs
    setupIndicatorsList();
    
    // Créer les contrôles
    createControls();
    
    // Ajouter au layout principal (hérité de BaseView)
    m_mainLayout->addWidget(m_controlsWidget);
    m_mainLayout->addWidget(m_chartPlaceholder);
    
    // Configurer les marges pour maximiser l'espace du graphique
    m_mainLayout->setContentsMargins(5, 5, 5, 5);
    m_mainLayout->setSpacing(5);
    
    int elapsed = start.msecsTo(QTime::currentTime());
    qInfo() << "ChartView::setupUI() took" << elapsed << "ms";
}

void ChartView::setupIndicatorsList()
{
    // Configuration des indicateurs disponibles
    m_indicatorConfigs["EMA_20"] = QVariantMap{{"type", "EMA"}, {"period", 20}, {"color", 0x0000FF}};
    m_indicatorConfigs["EMA_50"] = QVariantMap{{"type", "EMA"}, {"period", 50}, {"color", 0xFF0000}};
    m_indicatorConfigs["RSI_14"] = QVariantMap{{"type", "RSI"}, {"period", 14}};
    m_indicatorConfigs["Stoch_14"] = QVariantMap{{"type", "Stochastic"}, {"fastK", 14}, {"slowK", 3}, {"slowD", 3}};
    m_indicatorConfigs["ATR_14"] = QVariantMap{{"type", "ATR"}, {"period", 14}};
}

void ChartView::createControls()
{
    m_controlsWidget = new QWidget();
    m_controlsLayout = new QHBoxLayout(m_controlsWidget);
    
    // Checkbox Heikin-Ashi
    m_heikinAshiCheckbox = new QCheckBox("Heikin-Ashi");
    connect(m_heikinAshiCheckbox, &QCheckBox::toggled, this, &ChartView::onHeikinAshiToggled);
    m_controlsLayout->addWidget(m_heikinAshiCheckbox);
    
    // Checkbox Volume
    m_volumeCheckbox = new QCheckBox("Volume");
    connect(m_volumeCheckbox, &QCheckBox::toggled, this, &ChartView::onVolumeToggled);
    m_controlsLayout->addWidget(m_volumeCheckbox);
    
    // Checkbox Equity
    m_equityCheckbox = new QCheckBox("Equity");
    connect(m_equityCheckbox, &QCheckBox::toggled, this, &ChartView::onEquityToggled);
    m_controlsLayout->addWidget(m_equityCheckbox);
    
    // Combo indicateurs
    m_indicatorsCombo = new QComboBox();
    for (auto it = m_indicatorConfigs.begin(); it != m_indicatorConfigs.end(); ++it) {
        m_indicatorsCombo->addItem(it.key(), it.key());
    }
    m_controlsLayout->addWidget(m_indicatorsCombo);
    
    // Bouton ajouter indicateur
    m_addIndicatorBtn = new QPushButton("Ajouter indicateur");
    connect(m_addIndicatorBtn, &QPushButton::clicked, this, &ChartView::onAddIndicatorClicked);
    m_controlsLayout->addWidget(m_addIndicatorBtn);
    
    // Ajouter les contrôles de souris et navigation
    setupMouseControls();
    setupNavigationControls();
    
    m_controlsLayout->addStretch();
}

void ChartView::setupMouseControls()
{
    // Créer un groupe de boutons pour les modes de souris
    QWidget* mouseControlsWidget = new QWidget();
    QHBoxLayout* mouseLayout = new QHBoxLayout(mouseControlsWidget);
    
    QPushButton* scrollBtn = new QPushButton("📜 Scroll", mouseControlsWidget);
    QPushButton* zoomInBtn = new QPushButton("🔍 Zoom In", mouseControlsWidget);
    QPushButton* zoomOutBtn = new QPushButton("🔍 Zoom Out", mouseControlsWidget);
    
    scrollBtn->setCheckable(true);
    zoomInBtn->setCheckable(true);
    zoomOutBtn->setCheckable(true);
    
    // Créer un groupe mutuellement exclusif
    QButtonGroup* mouseGroup = new QButtonGroup(this);
    mouseGroup->addButton(scrollBtn, Chart::MouseUsageScroll);
    mouseGroup->addButton(zoomInBtn, Chart::MouseUsageZoomIn);
    mouseGroup->addButton(zoomOutBtn, Chart::MouseUsageZoomOut);
    
    // Connecter le signal pour changer le mode de souris
    connect(mouseGroup, QOverload<QAbstractButton*>::of(&QButtonGroup::buttonPressed),
            [this](QAbstractButton* button) {
                QButtonGroup* group = qobject_cast<QButtonGroup*>(sender());
                int mouseUsage = group->id(button);
                if (m_chartViewer) {
                    m_chartViewer->setMouseUsage(mouseUsage);
                    qDebug() << "Mode souris changé vers:" << mouseUsage;
                }
            });
    
    // Par défaut, mode scroll
    scrollBtn->setChecked(true);
    
    mouseLayout->addWidget(scrollBtn);
    mouseLayout->addWidget(zoomInBtn);
    mouseLayout->addWidget(zoomOutBtn);
    mouseLayout->addStretch();
    
    // Ajouter au layout principal des contrôles
    m_controlsLayout->addWidget(mouseControlsWidget);
}

void ChartView::setupNavigationControls()
{
    QWidget* navWidget = new QWidget();
    QHBoxLayout* navLayout = new QHBoxLayout(navWidget);
    
    QPushButton* homeBtn = new QPushButton("🏠 Tout", navWidget);
    QPushButton* leftBtn = new QPushButton("⬅️", navWidget);
    QPushButton* rightBtn = new QPushButton("➡️", navWidget);
    QPushButton* zoomInBtn = new QPushButton("➕", navWidget);
    QPushButton* zoomOutBtn = new QPushButton("➖", navWidget);
    
    // Connecter les boutons
    connect(homeBtn, &QPushButton::clicked, [this]() {
        if (m_chartViewer) {
            m_chartViewer->setViewPortLeft(0);
            m_chartViewer->setViewPortWidth(1.0);
            m_chartViewer->updateViewPort(true, false);
        }
    });
    
    connect(leftBtn, &QPushButton::clicked, [this]() {
        if (m_chartViewer) {
            double left = m_chartViewer->getViewPortLeft();
            double width = m_chartViewer->getViewPortWidth();
            m_chartViewer->setViewPortLeft(std::max(0.0, left - width * 0.1));
            m_chartViewer->updateViewPort(true, false);
        }
    });
    
    connect(rightBtn, &QPushButton::clicked, [this]() {
        if (m_chartViewer) {
            double left = m_chartViewer->getViewPortLeft();
            double width = m_chartViewer->getViewPortWidth();
            m_chartViewer->setViewPortLeft(std::min(1.0 - width, left + width * 0.1));
            m_chartViewer->updateViewPort(true, false);
        }
    });
    
    connect(zoomInBtn, &QPushButton::clicked, [this]() {
        if (m_chartViewer) {
            double left = m_chartViewer->getViewPortLeft();
            double width = m_chartViewer->getViewPortWidth();
            double newWidth = width * 0.8;
            double newLeft = left + (width - newWidth) / 2;
            m_chartViewer->setViewPortLeft(newLeft);
            m_chartViewer->setViewPortWidth(newWidth);
            m_chartViewer->updateViewPort(true, false);
        }
    });
    
    connect(zoomOutBtn, &QPushButton::clicked, [this]() {
        if (m_chartViewer) {
            double left = m_chartViewer->getViewPortLeft();
            double width = m_chartViewer->getViewPortWidth();
            double newWidth = std::min(1.0, width * 1.25);
            double newLeft = std::max(0.0, left - (newWidth - width) / 2);
            m_chartViewer->setViewPortLeft(newLeft);
            m_chartViewer->setViewPortWidth(newWidth);
            m_chartViewer->updateViewPort(true, false);
        }
    });
    
    navLayout->addWidget(homeBtn);
    navLayout->addWidget(leftBtn);
    navLayout->addWidget(rightBtn);
    navLayout->addWidget(zoomInBtn);
    navLayout->addWidget(zoomOutBtn);
    navLayout->addStretch();
    
    m_controlsLayout->addWidget(navWidget);
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
        updateChart();
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
        
        m_dataExtracted = true; // Marquer les données comme extraites
        
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
    
    m_activeIndicators.clear();
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
        
        // Calculer la largeur du graphique en fonction du widget parent
        int chartWidth = 800; // Valeur par défaut

        // Utiliser la largeur du widget (this) plutôt qu'un conteneur séparé
        this->updateGeometry();
        QApplication::processEvents();
        
        int widgetWidth = this->width();
        qDebug() << "Largeur du widget ChartView:" << widgetWidth;
        
        if (widgetWidth > 100) {
            chartWidth = std::max(1200, widgetWidth - 40); // -40 pour marges, minimum 1200
            qDebug() << "Largeur calculée:" << chartWidth;
        }
        
        qDebug() << "Largeur FINALE du graphique:" << chartWidth;
        
        // Créer FinanceChart avec la largeur dynamique
        m_financeChart = new FinanceChart(chartWidth);
        
        // Configurer les données
        m_financeChart->setData(timeStamps, highData, lowData, openData, closeData, volumeData, 0);
        
        // Ajouter le titre du graphique
        std::string title = "Données de backtest - " + std::to_string(timeStamps.len) + " points";
        m_financeChart->addTitle(title.c_str());
        
        // Ajouter le graphique principal avec hauteur appropriée
        m_financeChart->addMainChart(300);
        
        // Ajouter les chandelles
        m_financeChart->addCandleStick(0x00AA00, 0xFF3333);
        
        // Ajouter le volume si demandé
        if (m_volumeCheckbox && m_volumeCheckbox->isChecked()) {
            m_financeChart->addVolBars(80, 0x99ff99, 0xff9999, 0x808080);
        }
        
        qDebug() << "FinanceChart configuré, création du QChartViewer...";
        
        // Créer le QChartViewer APRÈS le FinanceChart
        m_chartViewer = new QChartViewer(this);
        
        // Configurer le viewer
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
            double visiblePortion = 100.0 / totalPoints;
            m_chartViewer->setViewPortWidth(visiblePortion);
            m_chartViewer->setViewPortLeft(1.0 - visiblePortion);
        } else {
            m_chartViewer->setViewPortWidth(1.0);
            m_chartViewer->setViewPortLeft(0);
        }
        
        // Ajouter au layout principal (hérité de BaseView)
        m_mainLayout->addWidget(m_chartViewer);
        
        // Connecter les signaux
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

// Implémentations des slots
void ChartView::onHeikinAshiToggled(bool checked)
{
    Q_UNUSED(checked);
    updateChart();
}

void ChartView::onVolumeToggled(bool checked)
{
    Q_UNUSED(checked);
    updateChart();
}

void ChartView::onEquityToggled(bool checked)
{
    Q_UNUSED(checked);
    updateChart();
}

void ChartView::onAddIndicatorClicked()
{
    if (!m_indicatorsCombo) {
        return;
    }
    
    QString indicator = m_indicatorsCombo->currentData().toString();
    if (!indicator.isEmpty() && !m_activeIndicators.contains(indicator)) {
        m_activeIndicators.append(indicator);
        updateChart();
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

void ChartView::updateChart()
{
    if (m_currentData) {
        qDebug() << "Mise à jour du graphique demandée";
        // Recréer le graphique si nécessaire
        if (hasValidData()) {
            createChart();
        }
    }
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
    
    // Recréer le graphique avec la nouvelle largeur
    DoubleArray timeStamps = vectorToDoubleArray(m_priceData.timestamps);
    DoubleArray openData = vectorToDoubleArray(m_priceData.open);
    DoubleArray highData = vectorToDoubleArray(m_priceData.high);
    DoubleArray lowData = vectorToDoubleArray(m_priceData.low);
    DoubleArray closeData = vectorToDoubleArray(m_priceData.close);
    DoubleArray volumeData = vectorToDoubleArray(m_priceData.volume);
    
    delete m_financeChart;
    m_financeChart = new FinanceChart(newWidth);
    m_financeChart->setData(timeStamps, highData, lowData, openData, closeData, volumeData, 0);
    
    std::string title = "Données de backtest - " + std::to_string(timeStamps.len) + " points";
    m_financeChart->addTitle(title.c_str());
    m_financeChart->addMainChart(300);
    m_financeChart->addCandleStick(0x00AA00, 0xFF3333);
    
    if (m_volumeCheckbox && m_volumeCheckbox->isChecked()) {
        m_financeChart->addVolBars(80, 0x99ff99, 0xff9999, 0x808080);
    }
    
    // Réassigner le graphique et restaurer le viewport
    if (m_chartViewer) {
        m_chartViewer->setChart(m_financeChart);
        m_chartViewer->setViewPortLeft(currentLeft);
        m_chartViewer->setViewPortWidth(currentWidth);
        m_chartViewer->updateViewPort(true, false);
    }
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
        
        // CORRECTION : NE PAS recalculer la largeur - utiliser la largeur actuelle du FinanceChart
        int chartWidth = m_financeChart ? m_financeChart->getWidth() : 800;
        
        // Créer un nouveau FinanceChart avec la MÊME largeur
        if (m_financeChart) {
            delete m_financeChart;
        }
        
        m_financeChart = new FinanceChart(chartWidth);
        m_financeChart->setData(timeStamps, highData, lowData, openData, closeData, volumeData, 0);
        
        // Reconfigurer le graphique
        std::string title = "Backtest - Points " + std::to_string(startIndex) + 
                           " à " + std::to_string(endIndex);
        m_financeChart->addTitle(title.c_str());
        m_financeChart->addMainChart(300);
        m_financeChart->addCandleStick(0x00AA00, 0xFF3333);
        
        // Ajouter le volume si demandé
        if (m_volumeCheckbox && m_volumeCheckbox->isChecked()) {
            m_financeChart->addVolBars(80, 0x99ff99, 0xff9999, 0x808080);
        }
        
        // Assigner le nouveau graphique
        m_chartViewer->setChart(m_financeChart);
        
    } catch (const std::exception& e) {
        qCritical() << "Erreur dans drawChartWithViewport:" << e.what();
    }
}

void ChartView::trackFinance(MultiChart* m, int mouseX)
{
    // Nettoyer la couche dynamique actuelle et obtenir l'objet DrawArea pour dessiner dessus
    DrawArea *d = m->initDynamicLayer();
    
    // Il est possible qu'un FinanceChart soit vide, donc nous devons le vérifier
    if (m->getChartCount() == 0)
        return;
    
    // Obtenir la valeur x des données la plus proche de la souris
    int xValue = (int)(((XYChart *)m->getChart(0))->getNearestXValue(mouseX));
    
    // Itérer à travers tous les graphiques dans le MultiChart
    XYChart *c = 0;
    for (int i = 0; i < m->getChartCount(); ++i) {
        c = (XYChart *)m->getChart(i);
        
        // Variables pour contenir les entrées de légende
        std::ostringstream ohlcLegend;
        std::vector<std::string> legendEntries;
        
        // Itérer à travers toutes les couches pour trouver les points de données
        for (int j = 0; j < c->getLayerCount(); ++j) {
            Layer *layer = c->getLayerByZ(j);
            
            // Ignorer les couches vides
            if (!layer->getDataSetCount())
                continue;
            
            // Obtenir le jeu de données
            DataSet *dataSet = layer->getDataSetByZ(0);
            
            // Vérifier si c'est une couche OHLC
            int ohlcLayer = 0;
            const char *name = dataSet->getDataName();
            if (name && ((std::string)name == "Open" || (std::string)name == "High" ||
                (std::string)name == "Low" || (std::string)name == "Close")) {
                ohlcLayer = 1;
            }
            
            // Obtenir l'index du tableau correspondant à la valeur x
            int xIndex = layer->getXIndexOf(xValue);
            if (xIndex < 0)
                continue;
            
            // Traitement spécial pour les couches OHLC
            if (ohlcLayer) {
                // Supposer l'ordre OHLC standard : Open(0), High(1), Low(2), Close(3)
                double openValue = layer->getDataSetByZ(0)->getValue(xIndex);
                double highValue = layer->getDataSetByZ(1)->getValue(xIndex);
                double lowValue = layer->getDataSetByZ(2)->getValue(xIndex);
                double closeValue = layer->getDataSetByZ(3)->getValue(xIndex);
                
                if (openValue != Chart::NoValue) {
                    // Construire la légende OHLC - valeurs Open, High, Low, Close
                    ohlcLegend << "      <*block*>";
                    ohlcLegend << "Open: " << c->formatValue(openValue, "{value|P4}");
                    ohlcLegend << ", High: " << c->formatValue(highValue, "{value|P4}");
                    ohlcLegend << ", Low: " << c->formatValue(lowValue, "{value|P4}");
                    ohlcLegend << ", Close: " << c->formatValue(closeValue, "{value|P4}");
                    
                    // Ajouter le changement en pourcentage pour la valeur de clôture
                    if (xIndex > 0) {
                        double lastCloseValue = layer->getDataSetByZ(3)->getValue(xIndex - 1);
                        if (lastCloseValue != Chart::NoValue) {
                            double change = closeValue - lastCloseValue;
                            double percent = change * 100 / lastCloseValue;
                            
                            // Utiliser les couleurs et icônes appropriées pour les changements à la hausse/baisse
                            std::string symbol = (change >= 0) ?
                                "<*font,color=008800*><*img=@triangle,width=8,color=008800*>" :
                                "<*font,color=CC0000*><*img=@invertedtriangle,width=8,color=CC0000*>";
                            
                            ohlcLegend << "  " << symbol << " " << c->formatValue(change, "{value|P4}");
                            ohlcLegend << " (" << c->formatValue(percent, "{value|2}") << "%)<*/font*>";
                        }
                    }
                    
                    ohlcLegend << "<*/*>";
                }
            }
            else {
                // Collecter les valeurs pour tous les jeux de données dans la couche
                for (int k = 0; k < layer->getDataSetCount(); ++k) {
                    DataSet *dataSet = layer->getDataSetByZ(k);
                    
                    // Obtenir le nom et la valeur du jeu de données
                    const char *dataName = dataSet->getDataName();
                    int color = dataSet->getDataColor();
                    double value = dataSet->getValue(xIndex);
                    
                    // Ignorer les jeux de données vides ou le point de données manquant
                    if (!dataName || !*dataName || (color == (int)Chart::Transparent) || 
                        (value == Chart::NoValue))
                        continue;
                    
                    // Dans une légende standard, la ligne ou le symbole est coloré avec la même couleur que les données
                    // Cependant, nous voulons que la légende soit plus jolie, donc nous utilisons un carré coloré à la place
                    std::ostringstream legendEntry;
                    legendEntry << "<*block*><*img=@square,width=8,edgeColor=000000,color="
                        << std::hex << color << "*> " << dataName << ": ";
                    
                    // Formater la valeur (avec unité si disponible)
                    std::string formatString = "{value|P4}";
                    legendEntry << c->formatValue(value, formatString.c_str());
                    legendEntry << "<*/*>";
                    
                    legendEntries.push_back(legendEntry.str());
                }
            }
        }
        
        // Obtenir la position de la zone de tracé relative à l'ensemble du FinanceChart
        PlotArea *plotArea = c->getPlotArea();
        int plotAreaLeftX = plotArea->getLeftX() + c->getAbsOffsetX();
        int plotAreaTopY = plotArea->getTopY() + c->getAbsOffsetY();
        int plotAreaWidth = plotArea->getWidth();
        int plotAreaHeight = plotArea->getHeight();
        
        // Créer le texte de la légende
        std::ostringstream legendText;
        // Par celle-ci pour inclure la date :
        legendText << "<*block,valign=top,maxWidth=" << (plotAreaWidth - 5)
            << "*><*font=Arial Bold*>[" << c->xAxis()->getFormattedLabel(xValue, "yyyy-MM-dd hh:nn:ss")
            << "]<*/font*>" << ohlcLegend.str();
        for (int i = ((int)legendEntries.size()) - 1; i >= 0; --i) {
            legendText << "      " << legendEntries[i];
        }
        legendText << "<*/*>";
        
        // Dessiner une ligne de suivi verticale à la position x
        int xCoor = c->getXCoor(xValue) + c->getAbsOffsetX();
        d->vline(plotAreaTopY, plotAreaTopY + plotAreaHeight, xCoor, d->dashLineColor(0x000000, 0x0101));
        
        // Afficher la date en bas de la zone de tracé
        std::ostringstream dateText;
        dateText << "<*font=Arial Bold*>[" << c->xAxis()->getFormattedLabel(xValue, "hh:nn:ss") << "]<*/font*>";
        TTFText *dateLabel = d->text(dateText.str().c_str(), "Arial", 8);
        dateLabel->draw(xCoor, plotAreaTopY + plotAreaHeight + 2, 0x000000, Chart::Top);
        dateLabel->destroy();
        
        // Positionner l'infobulle en fonction de la position de la souris pour éviter le chevauchement
        // Si la souris est sur la moitié droite du graphique, mettre l'infobulle sur le côté gauche
        TTFText *t = d->text(legendText.str().c_str(), "Arial", 8);
        if (mouseX > plotAreaLeftX + plotAreaWidth / 2) {
            // La souris est sur le côté droit, mettre l'infobulle à gauche
            t->draw(plotAreaLeftX + 5, plotAreaTopY + 25, 0x000000, Chart::TopLeft);
        } else {
            // La souris est sur le côté gauche, mettre l'infobulle à droite
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
        
        // Tests de validation (gardés pour sécurité)
        qDebug() << "Validation de l'objet...";
        if (dataFrame.ptr() == nullptr || dataFrame.is_none()) {
            throw std::runtime_error("DataFrame invalide");
        }
        
        if (!py::hasattr(dataFrame, "columns") || !py::hasattr(dataFrame, "index")) {
            throw std::runtime_error("DataFrame manque attributs essentiels");
        }
        
        qDebug() << "DataFrame validé - extraction des données réelles...";
        
        // NOUVELLE SECTION : EXTRACTION RÉELLE DES DONNÉES
        try {
            // Extraire les informations sur le DataFrame
            py::object columns = dataFrame.attr("columns");
            py::list columns_list = py::list(columns);
            qDebug() << "Nombre de colonnes:" << py::len(columns_list);
            
            // Afficher les noms des colonnes
            for (size_t i = 0; i < py::len(columns_list); i++) {
                std::string col_name = py::str(columns_list[i]).cast<std::string>();
                qDebug() << "Colonne" << i << ":" << QString::fromStdString(col_name);
            }
            
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
            
            // Afficher quelques valeurs pour vérification
            if (!m_priceData.close.empty()) {
                qDebug() << "Premiers prix Close:" << m_priceData.close[0] 
                         << m_priceData.close[1] << m_priceData.close[2];
            }
            
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
                    
                    // Essayer de convertir en timestamp Unix
                    if (py::hasattr(timestamp, "timestamp")) {
                        // C'est un pandas Timestamp
                        py::object timestamp_seconds = timestamp.attr("timestamp")();
                        double epoch_time = timestamp_seconds.cast<double>();
                        m_priceData.timestamps.push_back(epoch_time);
                    } else {
                        // Fallback : utiliser l'index numérique
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

void ChartView::addMainChart()
{
    // Cette méthode n'est plus nécessaire car tout est fait dans createChart()
    qDebug() << "addMainChart() appelé - logique déplacée dans createChart()";
}

void ChartView::addVolumeChart() {}
void ChartView::addEquityChart() {}
void ChartView::addTradeMarkers() {}
void ChartView::addIndicators() {}
void ChartView::addEMAIndicator(int period, int color) { Q_UNUSED(period); Q_UNUSED(color); }
void ChartView::addRSIIndicator(int period) { Q_UNUSED(period); }
void ChartView::addStochasticIndicator(int fastK, int slowK, int slowD) { Q_UNUSED(fastK); Q_UNUSED(slowK); Q_UNUSED(slowD); }
void ChartView::addATRIndicator(int period) { Q_UNUSED(period); }

void ChartView::calculateHeikinAshi(const std::vector<double>& open, 
                                   const std::vector<double>& high,
                                   const std::vector<double>& low, 
                                   const std::vector<double>& close,
                                   std::vector<double>& ha_open, 
                                   std::vector<double>& ha_high,
                                   std::vector<double>& ha_low,
                                   std::vector<double>& ha_close)
{
    Q_UNUSED(open); Q_UNUSED(high); Q_UNUSED(low); Q_UNUSED(close);
    Q_UNUSED(ha_open); Q_UNUSED(ha_high); Q_UNUSED(ha_low); Q_UNUSED(ha_close);
}

DoubleArray ChartView::vectorToDoubleArray(const std::vector<double>& vec)
{
    if (vec.empty()) {
        return DoubleArray();
    }
    
    // CORRECTION : Utiliser directement le pointeur du vector
    // ChartDirector copie les données, donc c'est sécurisé
    return DoubleArray(&vec[0], static_cast<int>(vec.size()));
}

std::vector<double> ChartView::extractDoubleVector(void* pyObj)
{
    std::vector<double> result;
    
    try {
        qDebug() << "=== DÉBUT extractDoubleVector ===";
        py::object* obj = static_cast<py::object*>(pyObj);
        if (!obj) {
            qDebug() << "Objet Python null";
            return result;
        }
        
        qDebug() << "Déréférencement de l'objet Python...";
        py::object pyVector = *obj;
        qDebug() << "Déréférencement réussi";
        
        qDebug() << "Vérification de l'objet...";
        if (!pyVector) {
            qDebug() << "Objet Python invalide";
            return result;
        }
        
        qDebug() << "Conversion en liste Python...";
        py::list pyList = py::list(pyVector);
        qDebug() << "Conversion réussie";
        
        size_t listSize = py::len(pyList);
        qDebug() << "Taille de la liste:" << listSize;
        
        if (listSize == 0) {
            qDebug() << "Liste vide";
            return result;
        }
        
        result.reserve(listSize);
        
        qDebug() << "Début conversion des éléments...";
        for (size_t i = 0; i < listSize; i++) {
            try {
                double value = pyList[i].cast<double>();
                result.push_back(value);
                
                // Log seulement quelques valeurs pour éviter le spam
                if (i < 5 || i == listSize - 1) {
                    qDebug() << "Élément" << i << ":" << value;
                }
            } catch (const std::exception& e) {
                qWarning() << "Erreur conversion élément" << i << ":" << e.what();
                result.push_back(0.0); // Valeur par défaut
            }
        }
        
        qDebug() << "=== FIN extractDoubleVector, taille finale:" << result.size() << "===";
        
    } catch (const std::exception& e) {
        qCritical() << "Erreur dans extractDoubleVector:" << e.what();
    }
    
    return result;
}

std::vector<QString> ChartView::extractStringVector(void* pyObj)
{
    Q_UNUSED(pyObj);
    return std::vector<QString>();
}

QVariant ChartView::extractPythonValue(void* pyObj)
{
    Q_UNUSED(pyObj);
    return QVariant();
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
        qDebug() << "FinanceChart existe";
    }
    
    if (m_chartViewer) {
        qDebug() << "ChartViewer existe";
        qDebug() << "Chart assigné:" << m_chartViewer->getChart();
        qDebug() << "ViewPort Width:" << m_chartViewer->getViewPortWidth();
        qDebug() << "ViewPort Left:" << m_chartViewer->getViewPortLeft();
    }
    
    qDebug() << "Données:";
    qDebug() << "- Timestamps:" << m_priceData.timestamps.size();
    qDebug() << "- Close:" << m_priceData.close.size();
    qDebug() << "=== FIN DEBUG CHART ===";
}
