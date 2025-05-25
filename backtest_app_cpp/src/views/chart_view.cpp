#include "chart_view.h"
#include <QDebug>
#include <QTime>

ChartView::ChartView(QObject* parent)  
    : BaseView(parent)
    , m_chartContainer(nullptr)
    , m_chartLayout(nullptr)
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
    , m_cachedData(nullptr)          // Ceci doit venir avant m_currentStats
    , m_currentStats(nullptr)        // dans l'ordre de déclaration du header
    , m_cachedStats(nullptr)
    , m_dataExtracted(false)
{
    qDebug() << "ChartView créée avec parent:" << parent;
    
    // Initialiser les structures de données
    m_priceData = PriceData();
    m_tradeData = TradeData();
    m_equityData = EquityData();
}
ChartView::~ChartView()
{
    if (m_financeChart) {
        delete m_financeChart;
        m_financeChart = nullptr;
    }
}

QWidget* ChartView::create(QWidget* parentWidget)
{   
    m_parentWidget = parentWidget;
    m_chartContainer = new QWidget(parentWidget);
    m_chartLayout = new QVBoxLayout(m_chartContainer);
    
    // CORRECTION: Appeler setupIndicatorsList() ici
    setupIndicatorsList();
    setupControls();
    
    // Placeholder initial
    m_chartPlaceholder = new QLabel("Exécutez le backtest pour afficher les graphiques");
    m_chartPlaceholder->setAlignment(Qt::AlignCenter);
    m_chartLayout->addWidget(m_chartPlaceholder);
    
    return m_chartContainer;
}

void ChartView::setupIndicatorsList()
{
    // Utiliser m_indicatorConfigs correctement déclaré
    m_indicatorConfigs["EMA_20"] = QVariantMap{{"type", "EMA"}, {"period", 20}, {"color", 0x0000FF}};
    m_indicatorConfigs["EMA_50"] = QVariantMap{{"type", "EMA"}, {"period", 50}, {"color", 0xFF0000}};
    m_indicatorConfigs["RSI_14"] = QVariantMap{{"type", "RSI"}, {"period", 14}};
}

void ChartView::setupControls()
{
    m_controlsWidget = new QWidget();
    m_controlsLayout = new QHBoxLayout(m_controlsWidget);
    
    // Checkbox Heikin-Ashi
    m_heikinAshiCheckbox = new QCheckBox("Heikin-Ashi");
    QObject::connect(m_heikinAshiCheckbox, &QCheckBox::toggled, this, &ChartView::onHeikinAshiToggled);
    m_controlsLayout->addWidget(m_heikinAshiCheckbox);
    
    // Checkbox Volume
    m_volumeCheckbox = new QCheckBox("Volume");
    QObject::connect(m_volumeCheckbox, &QCheckBox::toggled, this, &ChartView::onVolumeToggled);
    m_controlsLayout->addWidget(m_volumeCheckbox);
    
    // Checkbox Equity
    m_equityCheckbox = new QCheckBox("Equity");
    QObject::connect(m_equityCheckbox, &QCheckBox::toggled, this, &ChartView::onEquityToggled);
    m_controlsLayout->addWidget(m_equityCheckbox);
    
    // Combo indicateurs
    m_indicatorsCombo = new QComboBox();
    for (auto it = m_indicatorConfigs.begin(); it != m_indicatorConfigs.end(); ++it) {
        m_indicatorsCombo->addItem(it.key(), it.key());
    }
    m_controlsLayout->addWidget(m_indicatorsCombo);
    
    // Bouton ajouter indicateur
    m_addIndicatorBtn = new QPushButton("Ajouter indicateur");
    QObject::connect(m_addIndicatorBtn, &QPushButton::clicked, this, &ChartView::onAddIndicatorClicked);
    m_controlsLayout->addWidget(m_addIndicatorBtn);
    
    m_controlsLayout->addStretch();
    m_chartLayout->addWidget(m_controlsWidget);
}

// Toutes les autres méthodes restent inchangées avec des implémentations TODO
void ChartView::update(void* data, void* stats)
{
    m_currentData = data;
    m_currentStats = stats;
    
    qDebug() << "=== DÉBUT ChartView::update() ===";
    qDebug() << "Data pointer:" << data << "Stats pointer:" << stats;

    // Vérifier si les données ont déjà été extraites pour ces pointeurs
    if (m_dataExtracted && m_cachedData == data && m_cachedStats == stats) {
        qDebug() << "Données déjà en cache, pas de ré-extraction nécessaire";
        updateChart();
        qDebug() << "=== FIN ChartView::update() (depuis cache) ===";
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
        
        qDebug() << "=== FIN ChartView::update() ===";
        
    } catch (const py::error_already_set& e) {
        qCritical() << "Erreur Python:" << e.what();
        showPlaceholder("Erreur Python lors de la création du graphique");
    } catch (const std::exception& e) {
        qCritical() << "Erreur C++:" << e.what();
        showPlaceholder("Erreur lors de la création du graphique");
    } catch (...) {
        qCritical() << "Erreur inconnue dans ChartView::update()";
        showPlaceholder("Erreur inconnue");
    }

    m_dataExtracted = true; // Marquer les données comme extraites
    qDebug() << "=== FIN ChartView::update() ===";
}

void ChartView::clear()
{
    m_activeIndicators.clear();
    m_currentData = nullptr;
    m_currentStats = nullptr;
    
    if (m_chartPlaceholder) {
        m_chartPlaceholder->setVisible(true);
    }
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
    
    qDebug() << "ViewPort changed - redrawing chart";
    
    // Ne pas recréer le graphique, juste mettre à jour l'affichage
    // ChartDirector gère automatiquement le zoom/scroll
    m_chartViewer->updateDisplay();
}

void ChartView::onMouseMovePlotArea(QMouseEvent *event)
{
    if (!m_financeChart || !m_chartViewer) {
        return;
    }
    
    // Ajouter une ligne de suivi comme dans la démo
    // Pour l'instant, on peut laisser vide ou ajouter un simple debug
    qDebug() << "Mouse at plot area:" << event->x() << event->y();
}

void ChartView::updateChart()
{
    if (m_currentData) {
        // TODO: Implémenter la mise à jour du graphique
        qDebug() << "Mise à jour du graphique demandée";
    }
}

// Toutes les autres méthodes avec des implémentations TODO...
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
            m_chartLayout->removeWidget(m_chartViewer);
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
        
        // CORRECTION 1 : Créer FinanceChart avec une largeur fixe
        m_financeChart = new FinanceChart(800);
        
        // CORRECTION 2 : Configurer les données AVANT d'ajouter des éléments
        m_financeChart->setData(timeStamps, highData, lowData, openData, closeData, volumeData, 0);
        
        // CORRECTION 3 : Ajouter le titre du graphique
        std::string title = "Données de backtest - " + std::to_string(timeStamps.len) + " points";
        m_financeChart->addTitle(title.c_str());
        
        // CORRECTION 4 : Ajouter le graphique principal avec hauteur appropriée
        m_financeChart->addMainChart(300);  // Hauteur en pixels
        
        // CORRECTION 5 : Ajouter les chandelles APRÈS avoir ajouté le graphique principal
        m_financeChart->addCandleStick(0x00AA00, 0xFF3333);
        
        // CORRECTION 6 : Ajouter le volume si demandé
        if (m_volumeCheckbox && m_volumeCheckbox->isChecked()) {
            m_financeChart->addVolBars(80, 0x99ff99, 0xff9999, 0x808080);
        }
        
        qDebug() << "FinanceChart configuré, création du QChartViewer...";
        
        // CORRECTION 7 : Créer le QChartViewer avec configuration complète
        m_chartViewer = new QChartViewer(m_chartContainer);
        
        // Configuration du viewer comme dans la démo
        m_chartViewer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        m_chartViewer->setMouseUsage(Chart::MouseUsageScroll);
        m_chartViewer->setMouseWheelZoomRatio(1.1);
        m_chartViewer->setScrollDirection(Chart::DirectionHorizontal);
        m_chartViewer->setZoomDirection(Chart::DirectionHorizontal);
        
        // CORRECTION 8 : Configurer le range complet AVANT setChart
        m_chartViewer->setFullRange("x", 0, timeStamps.len - 1);
        
        // CORRECTION 9 : Assigner le graphique au viewer
        m_chartViewer->setChart(m_financeChart);
        
        // CORRECTION 10 : Configurer le viewport pour afficher les dernières données
        int totalPoints = timeStamps.len;
        if (totalPoints > 100) {
            double visiblePortion = 100.0 / totalPoints;
            m_chartViewer->setViewPortWidth(visiblePortion);
            m_chartViewer->setViewPortLeft(1.0 - visiblePortion);
        } else {
            m_chartViewer->setViewPortWidth(1.0);
            m_chartViewer->setViewPortLeft(0);
        }
        
        // Connecter les signaux
        connect(m_chartViewer, &QChartViewer::viewPortChanged, 
                this, &ChartView::onViewPortChanged);
        connect(m_chartViewer, &QChartViewer::mouseMovePlotArea, 
                this, &ChartView::onMouseMovePlotArea);
        
        // CORRECTION 11 : Forcer la mise à jour initiale
        m_chartViewer->updateViewPort(true, false);
        
        // Ajouter au layout
        m_chartLayout->addWidget(m_chartViewer);
        
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