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

void ChartView::onMouseMovePlotArea(QMouseEvent* event)
{
    Q_UNUSED(event);
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
    
    qDebug() << "Données disponibles - création du graphique...";
    
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
        
        // Convertir les données en DoubleArray pour ChartDirector
        qDebug() << "Conversion des données en DoubleArray...";
        DoubleArray timeStamps = vectorToDoubleArray(m_priceData.timestamps);
        DoubleArray openData = vectorToDoubleArray(m_priceData.open);
        DoubleArray highData = vectorToDoubleArray(m_priceData.high);
        DoubleArray lowData = vectorToDoubleArray(m_priceData.low);
        DoubleArray closeData = vectorToDoubleArray(m_priceData.close);
        DoubleArray volumeData = vectorToDoubleArray(m_priceData.volume);
        
        qDebug() << "Création du FinanceChart...";
        m_financeChart = new FinanceChart(800);
        
        qDebug() << "Configuration des données...";
        m_financeChart->setData(timeStamps, highData, lowData, openData, closeData, volumeData, 0);
        
        qDebug() << "Ajout du graphique principal...";
        addMainChart();
        
        qDebug() << "Création du QChartViewer...";
        m_chartViewer = new QChartViewer(m_chartContainer);
        
        // Utiliser MultiChart ou getChart selon ce qui est disponible
        BaseChart* chart = static_cast<BaseChart*>(m_financeChart);
        m_chartViewer->setChart(chart);
        
        // Ajouter au layout
        m_chartLayout->addWidget(m_chartViewer);
        
        qDebug() << "Graphique créé avec succès !";
        
    } catch (const std::exception& e) {
        qCritical() << "Erreur lors de la création du graphique:" << e.what();
        showPlaceholder("Erreur lors de la création du graphique");
    }
    
    qDebug() << "=== FIN createChart ===";
}
// void ChartView::createChart()
// {
//     if (m_priceData.timestamps.empty()) {
//         qWarning() << "Aucune donnée de prix disponible";
//         return;
//     }
    
//     try {
//         // Nettoyer le graphique précédent
//         if (m_financeChart) {
//             delete m_financeChart;
//             m_financeChart = nullptr;
//         }
        
//         if (m_chartViewer) {
//             m_chartLayout->removeWidget(m_chartViewer);
//             delete m_chartViewer;
//             m_chartViewer = nullptr;
//         }
        
//         // Convertir les données en DoubleArray pour ChartDirector
//         DoubleArray timeStamps = vectorToDoubleArray(m_priceData.timestamps);
//         DoubleArray openData = vectorToDoubleArray(m_priceData.open);
//         DoubleArray highData = vectorToDoubleArray(m_priceData.high);
//         DoubleArray lowData = vectorToDoubleArray(m_priceData.low);
//         DoubleArray closeData = vectorToDoubleArray(m_priceData.close);
//         DoubleArray volumeData = vectorToDoubleArray(m_priceData.volume);
        
//         // Créer le graphique financier
//         m_financeChart = new FinanceChart(800);
        
//         // setData avec 7 arguments (ajouter extraDays = 0)
//         m_financeChart->setData(timeStamps, highData, lowData, openData, closeData, volumeData, 0);
        
//         // Ajouter le graphique principal
//         addMainChart();
        
//         // Ajouter les graphiques optionnels
//         if (m_volumeCheckbox && m_volumeCheckbox->isChecked()) {
//             addVolumeChart();
//         }
        
//         if (m_equityCheckbox && m_equityCheckbox->isChecked()) {
//             addEquityChart();
//         }
        
//         // Ajouter les indicateurs
//         addIndicators();
        
//         // Ajouter les marqueurs de trades
//         addTradeMarkers();
        
//         // Créer le QChartViewer
//         m_chartViewer = new QChartViewer(m_chartContainer);
        
//         // CORRECTION: Utiliser getChart() qui retourne BaseChart*
//         BaseChart* chart = m_financeChart->getChart();
//         if (chart) {
//             m_chartViewer->setChart(chart);
//         } else {
//             qWarning() << "Impossible de récupérer le graphique BaseChart";
//             showPlaceholder("Erreur lors de la récupération du graphique");
//             return;
//         }
        
//         // Connecter les événements de souris
//         QObject::connect(m_chartViewer, &QChartViewer::mouseMovePlotArea, 
//                         this, &ChartView::onMouseMovePlotArea);
        
//         // Ajouter au layout
//         m_chartLayout->addWidget(m_chartViewer);
        
//         qDebug() << "Graphique créé avec succès";
        
//     } catch (const std::exception& e) {
//         qCritical() << "Erreur lors de la création du graphique:" << e.what();
//         showPlaceholder("Erreur lors de la création du graphique");
//     }
// }

void ChartView::addMainChart()
{
    if (!m_financeChart) {
        return;
    }
    
    try {
        // Configurer le graphique principal
        XYChart* mainChart = m_financeChart->addMainChart(300);
        
        // Utiliser Heikin-Ashi si demandé
        if (m_heikinAshiCheckbox && m_heikinAshiCheckbox->isChecked()) {
            std::vector<double> ha_open, ha_high, ha_low, ha_close;
            calculateHeikinAshi(m_priceData.open, m_priceData.high, 
                               m_priceData.low, m_priceData.close,
                               ha_open, ha_high, ha_low, ha_close);
            
            DoubleArray ha_openData = vectorToDoubleArray(ha_open);
            DoubleArray ha_highData = vectorToDoubleArray(ha_high);
            DoubleArray ha_lowData = vectorToDoubleArray(ha_low);
            DoubleArray ha_closeData = vectorToDoubleArray(ha_close);
            
            // Ajouter le layer Heikin-Ashi
            CandleStickLayer* layer = mainChart->addCandleStickLayer(
                ha_highData, ha_lowData, ha_openData, ha_closeData);
            layer->setDataLabelFormat("{value|P}");
        } else {
            // CORRECTION: addCandleStick avec seulement 2 arguments (couleurs)
            m_financeChart->addCandleStick(0x008000, 0xFF0000);
        }
        
        // Configurer les axes
        if (mainChart) {
            mainChart->yAxis()->setTitle("Prix");
            mainChart->xAxis()->setTitle("Temps");
        }
        
        qDebug() << "Graphique principal ajouté";
        
    } catch (const std::exception& e) {
        qCritical() << "Erreur lors de l'ajout du graphique principal:" << e.what();
    }
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
    
    // CORRECTION: Créer un tableau temporaire puis le passer au constructeur
    double* tempArray = new double[vec.size()];
    for (size_t i = 0; i < vec.size(); ++i) {
        tempArray[i] = vec[i];
    }
    
    // Créer le DoubleArray avec le pointeur et la taille
    DoubleArray result(tempArray, static_cast<int>(vec.size()));
    
    // Note: ChartDirector copie les données, donc on peut libérer le tableau temporaire
    delete[] tempArray;
    
    return result;
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
