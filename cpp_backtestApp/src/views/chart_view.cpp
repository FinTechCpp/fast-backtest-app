#include "views/chart_view.h"
#include "app.h"
#include <QDebug>
#include <QTime>
#include <QLabel>
#include <QFrame>
#include <QResizeEvent>

ChartView::ChartView(QWidget* parent)
    : BaseView(parent)
    , m_cachedResults(nullptr)
    , m_dataExtracted(false)
    , m_currentResults(nullptr)
    , m_chartPlaceholder(nullptr)
    , m_chartTypeCombo(nullptr)
    , m_settingsTitle(nullptr)
    , m_chartWidget(nullptr)
    , m_leftPanel(nullptr)
    , m_app(nullptr)
{
    // Trouver l'application parente
    QWidget* widget = parent;
    while (widget && !m_app) {
        m_app = qobject_cast<App*>(widget);
        widget = widget->parentWidget();
    }
        
    // Initialiser les structures de données
    m_priceData = PriceData();
    m_tradeData = TradeData();
    m_equityData = EquityData();
    
    // Construire l'interface
    setupUI();
}

ChartView::~ChartView()
{
}

void ChartView::setupUI()
{
    // Configurer le layout principal pour occuper tout l'espace
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);
    
    // Créer un layout horizontal pour les panneaux gauche et droit
    QHBoxLayout* horizontalLayout = new QHBoxLayout();
    horizontalLayout->setContentsMargins(0, 0, 0, 0);
    horizontalLayout->setSpacing(0);
    m_mainLayout->addLayout(horizontalLayout);
    
    // Créer le panneau gauche avec une largeur fixe
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
    
    // Créer le placeholder initial
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
    
    // Créer le widget de graphique
    m_chartWidget = new ChartWidget();
    m_chartWidget->setVisible(false); // Cacher initialement
    
    // Ajouter les widgets au layout du panneau droit
    rightPanelLayout->addWidget(m_chartPlaceholder);
    rightPanelLayout->addWidget(m_chartWidget);
    
    // Ajouter les composants au layout horizontal
    horizontalLayout->addWidget(m_leftPanel);
    horizontalLayout->addWidget(separator);
    horizontalLayout->addWidget(m_rightPanel);
    
    // Configurer le widget pour s'étendre
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void ChartView::updateData(BacktestResults* results)
{
    QTime start = QTime::currentTime();

    Q_UNUSED(results);
            
    // Récupérer les résultats depuis l'App
    BacktestResults* appResults = m_app ? m_app->getBacktestResults() : nullptr;
    
    // Mettre à jour les références locales
    m_currentResults = appResults;

    // Vérifier si les données ont déjà été extraites pour ces pointeurs
    if (m_dataExtracted && m_cachedResults == appResults) {
        if (hasValidData()) {
            showChartWidget();
        }
        return;
    }
    
    // Mettre en cache les nouveaux pointeurs
    m_cachedResults = appResults;
    
    if (!appResults || !appResults->data) {
        clear();
        showPlaceholder("Aucune donnée disponible");
        return;
    }
    
    try {
        extractDataFromCpp(appResults);
        
        if (!hasValidData()) {
            showPlaceholder("Données invalides");
            return;
        }
        
        // Transférer les données au widget de graphique
        m_chartWidget->setPriceData(m_priceData);
        m_chartWidget->setTradeData(m_tradeData);
        m_chartWidget->setEquityData(m_equityData);
        
        // Définir le type de graphique
        QString chartType = m_chartTypeCombo->currentData().toString();
        m_chartWidget->setChartType(chartType);
        
        // Afficher le widget de graphique et masquer le placeholder
        showChartWidget();
        
        // Créer le graphique
        m_chartWidget->createChart();
        
        m_dataExtracted = true;
        
    } catch (const std::exception& e) {
        qCritical() << "Erreur C++:" << e.what();
        showPlaceholder("Erreur lors de la création du graphique");
    } catch (...) {
        qCritical() << "Erreur inconnue dans ChartView::updateData()";
        showPlaceholder("Erreur inconnue");
    }

    int elapsed = start.msecsTo(QTime::currentTime());
}

void ChartView::clear()
{    
    m_currentResults = nullptr;
    m_cachedResults = nullptr;
    m_dataExtracted = false;
    
    // Nettoyer les données
    m_priceData = PriceData();
    m_tradeData = TradeData();
    m_equityData = EquityData();
    
    // Nettoyer le widget de graphique
    if (m_chartWidget) {
        m_chartWidget->clearChart();
    }
    
    // Réafficher le placeholder
    showPlaceholder("Exécutez un backtest pour afficher les graphiques");
}

void ChartView::onChartTypeChanged(int index)
{
    if (m_chartTypeCombo) {
        QVariant data = m_chartTypeCombo->itemData(index);
        if (data.isValid()) {
            QString chartType = data.toString();
            
            // Mettre à jour le type de graphique dans le widget
            if (m_chartWidget) {
                m_chartWidget->setChartType(chartType);
                
                // Si nous avons déjà des données valides, mettre à jour le graphique
                if (hasValidData() && m_chartWidget->isVisible()) {
                    m_chartWidget->updateChart();
                }
            }
        }
    }
}

void ChartView::showChartWidget()
{
    if (m_chartPlaceholder) {
        m_chartPlaceholder->setVisible(false);
    }
    
    if (m_chartWidget) {
        m_chartWidget->setVisible(true);
    }
}

void ChartView::showPlaceholder(const QString& message)
{
    if (m_chartWidget) {
        m_chartWidget->setVisible(false);
    }
    
    if (m_chartPlaceholder) {
        m_chartPlaceholder->setText(message);
        m_chartPlaceholder->setVisible(true);
    }
}

void ChartView::extractDataFromCpp(BacktestResults* results)
{
    
    if (!results || !results->data) {
        return;
    }
    
    try {
        extractPriceData(results);
        extractTradeData(results);
        extractEquityData(results);
    } catch (const std::exception& e) {
        qCritical() << "Erreur C++ dans extractDataFromCpp:" << e.what();
        throw;
    }
}

void ChartView::extractPriceData(BacktestResults* results)
{   
    try {
        std::shared_ptr<be::Data>& beData = results->data;
        if (!beData) {
            throw std::runtime_error("Objet be::Data est null");
        }
        
        // Nettoyer les anciennes données
        m_priceData = PriceData();
        
        // Obtenir la taille des données
        size_t dataSize = beData->size();
        
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
        
    } catch (const std::exception& e) {
        qCritical() << "Erreur dans extractPriceData:" << e.what();
        throw std::runtime_error(QString("Erreur prix: %1").arg(e.what()).toStdString());
    }
}

void ChartView::extractTradeData(BacktestResults* results)
{    
    try {
        const auto& trades = results->stats.trades;
        
        // Nettoyer les anciennes données
        m_tradeData = TradeData();
        
        // Si aucun trade, retourner
        if (trades.empty()) {
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
                
    } catch (const std::exception& e) {
        qCritical() << "Erreur dans extractTradeData:" << e.what();
        // Ne pas faire échouer toute l'extraction si les trades échouent
    }
}

void ChartView::extractEquityData(BacktestResults* results)
{   
    try {
        const auto& equityCurve = results->stats.equityCurve;
        
        // Nettoyer les anciennes données
        m_equityData = EquityData();
        
        // Si pas de courbe d'équité, retourner
        if (equityCurve.empty()) {
            return;
        }
        
        // Vérifier que nous avons des données de prix pour synchroniser le graphique
        if (m_priceData.timestamps.empty()) {
            return;
        }

        // Réserver la capacité
        size_t numPoints = equityCurve.size();
        size_t numBars = m_priceData.timestamps.size();
        
        // Adapter la taille de la courbe d'équité à celle des barres de prix
        // Plusieurs scénarios possibles:
        
        if (numPoints == numBars) {
            // Cas idéal: même nombre de points, correspondance directe
            m_equityData.timestamps = m_priceData.timestamps;
            m_equityData.equity_values = equityCurve;
        }
        else if (numPoints < numBars) {
            // Moins de points d'equity que de barres: interpoler l'equity
            m_equityData.timestamps = m_priceData.timestamps;
            m_equityData.equity_values.resize(numBars);
            
            // Interpolation simple
            for (size_t i = 0; i < numBars; i++) {
                // Mappage linéaire de i dans [0,numBars-1] à j dans [0,numPoints-1]
                double j_exact = i * (numPoints - 1) / (double)(numBars - 1);
                size_t j_low = (size_t)floor(j_exact);
                size_t j_high = (size_t)ceil(j_exact);
                j_low = std::min(j_low, numPoints - 1);
                j_high = std::min(j_high, numPoints - 1);
                
                if (j_low == j_high) {
                    m_equityData.equity_values[i] = equityCurve[j_low];
                } else {
                    // Interpolation linéaire
                    double weight_high = j_exact - j_low;
                    double weight_low = 1.0 - weight_high;
                    m_equityData.equity_values[i] = weight_low * equityCurve[j_low] + 
                                                   weight_high * equityCurve[j_high];
                }
            }
        }
        else {
            // Plus de points d'equity que de barres: sous-échantillonner l'equity
            m_equityData.timestamps = m_priceData.timestamps;
            m_equityData.equity_values.resize(numBars);
            
            for (size_t i = 0; i < numBars; i++) {
                // Mappage linéaire
                size_t j = (size_t)(i * (numPoints - 1) / (numBars - 1));
                j = std::min(j, numPoints - 1);
                m_equityData.equity_values[i] = equityCurve[j];
            }
        }
        
        // Calculer le drawdown
        if (!m_equityData.equity_values.empty()) {
            m_equityData.drawdown.resize(m_equityData.equity_values.size());
            double peak = m_equityData.equity_values[0];
            
            for (size_t i = 0; i < m_equityData.equity_values.size(); ++i) {
                if (m_equityData.equity_values[i] > peak) {
                    peak = m_equityData.equity_values[i];
                }
                double dd = (peak - m_equityData.equity_values[i]) / peak * 100.0; // En pourcentage
                m_equityData.drawdown[i] = dd;
            }
        }
        
        
    } catch (const std::exception& e) {
        qCritical() << "Erreur dans extractEquityData:" << e.what();
        // Ne pas faire échouer toute l'extraction si l'équité échoue
    }
}

bool ChartView::hasValidData() const {
    return !m_priceData.timestamps.empty() && 
           !m_priceData.open.empty() && 
           !m_priceData.high.empty() && 
           !m_priceData.low.empty() && 
           !m_priceData.close.empty();
}