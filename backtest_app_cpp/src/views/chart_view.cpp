#include "chart_view.h"
#include <QDebug>
#include <QTime>
#include <QMouseEvent>
#include <QSplitter>
#include <algorithm>
#include <cmath>

ChartView::ChartView(QWidget* parent)
    : QObject(parent), BaseView(parent)
    , m_chartContainer(nullptr)
    , m_chartLayout(nullptr)
    , m_chartPlaceholder(nullptr)
    , m_controlsWidget(nullptr)
    , m_controlsLayout(nullptr)
    , m_heikinAshiCheckbox(nullptr)
    , m_volumeCheckbox(nullptr)
    , m_equityCheckbox(nullptr)
    , m_indicatorsCombo(nullptr)
    , m_addIndicatorBtn(nullptr)
    , m_chartViewer(nullptr)
    , m_financeChart(nullptr)
    , m_currentData(nullptr)
    , m_currentStats(nullptr)
{
    setupIndicatorsList();
}

ChartView::~ChartView()
{
    if (m_financeChart) {
        delete m_financeChart;
    }
}

QWidget* ChartView::create()
{
    m_chartContainer = new QWidget(m_parent);
    m_chartLayout = new QVBoxLayout(m_chartContainer);
    
    setupControls();
    
    // Placeholder initial
    m_chartPlaceholder = new QLabel("Exécutez le backtest pour afficher les graphiques");
    m_chartPlaceholder->setAlignment(Qt::AlignCenter);
    m_chartLayout->addWidget(m_chartPlaceholder);
    
    return m_chartContainer;
}

void ChartView::setupControls()
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
    m_controlsLayout->addWidget(m_indicatorsCombo);
    
    // Bouton ajouter indicateur
    m_addIndicatorBtn = new QPushButton("Ajouter indicateur");
    connect(m_addIndicatorBtn, &QPushButton::clicked, this, &ChartView::onAddIndicatorClicked);
    m_controlsLayout->addWidget(m_addIndicatorBtn);
    
    m_controlsLayout->addStretch();
    m_chartLayout->addWidget(m_controlsWidget);
}

void ChartView::setupIndicatorsList()
{
    m_indicatorConfigs["EMA_20"] = QVariantMap{{"type", "EMA"}, {"period", 20}, {"color", 0x0000FF}};
    m_indicatorConfigs["EMA_50"] = QVariantMap{{"type", "EMA"}, {"period", 50}, {"color", 0xFF0000}};
    m_indicatorConfigs["RSI_14"] = QVariantMap{{"type", "RSI"}, {"period", 14}};
    m_indicatorConfigs["STOCH_14"] = QVariantMap{{"type", "STOCH"}, {"fastK", 14}, {"slowK", 3}, {"slowD", 3}};
    m_indicatorConfigs["ATR_14"] = QVariantMap{{"type", "ATR"}, {"period", 14}};
}

void ChartView::update(void* data, void* stats)
{
    if (!data) {
        showPlaceholder("Aucune donnée disponible");
        return;
    }
    
    m_currentData = data;
    m_currentStats = stats;
    
    extractDataFromPython(data, stats);
    
    if (!hasValidData()) {
        showPlaceholder("Données invalides");
        return;
    }
    
    createChart();
}

void ChartView::extractDataFromPython(void* data, void* stats)
{
    // Extraire les données de prix
    extractPriceData(data);
    
    // Extraire les données des trades si disponibles
    if (stats) {
        extractTradeData(stats);
        extractEquityData(stats);
    }
}

void ChartView::extractPriceData(void* data)
{
    Q_UNUSED(data)
    // TODO: Implémenter l'extraction des données Python
    // Pour l'instant, données factices pour tester
    m_priceData.timestamps = {1.0, 2.0, 3.0, 4.0, 5.0};
    m_priceData.open = {100.0, 101.0, 102.0, 103.0, 104.0};
    m_priceData.high = {102.0, 103.0, 104.0, 105.0, 106.0};
    m_priceData.low = {99.0, 100.0, 101.0, 102.0, 103.0};
    m_priceData.close = {101.0, 102.0, 103.0, 104.0, 105.0};
    m_priceData.volume = {1000.0, 1100.0, 1200.0, 1300.0, 1400.0};
}

void ChartView::extractTradeData(void* stats)
{
    Q_UNUSED(stats)
    // TODO: Implémenter l'extraction des trades
}

void ChartView::extractEquityData(void* stats)
{
    Q_UNUSED(stats)
    // TODO: Implémenter l'extraction de l'equity
}

void ChartView::createChart()
{
    if (m_chartPlaceholder) {
        m_chartPlaceholder->hide();
        m_chartLayout->removeWidget(m_chartPlaceholder);
    }
    
    // Créer le graphique principal
    addMainChart();
    
    // Ajouter le volume si demandé
    if (m_volumeCheckbox && m_volumeCheckbox->isChecked()) {
        addVolumeChart();
    }
    
    // Ajouter l'equity si demandé
    if (m_equityCheckbox && m_equityCheckbox->isChecked()) {
        addEquityChart();
    }
    
    // Ajouter les indicateurs
    addIndicators();
    
    // Ajouter les marqueurs de trades
    addTradeMarkers();
    
    // Afficher le graphique
    if (m_chartViewer) {
        m_chartLayout->addWidget(m_chartViewer);
    }
}

void ChartView::addMainChart()
{
    // TODO: Implémenter la création du graphique principal avec ChartDirector
    qDebug() << "Création du graphique principal";
}

void ChartView::addVolumeChart()
{
    // TODO: Implémenter le graphique de volume
    qDebug() << "Ajout du graphique de volume";
}

void ChartView::addEquityChart()
{
    // TODO: Implémenter le graphique d'equity
    qDebug() << "Ajout du graphique d'equity";
}

void ChartView::addTradeMarkers()
{
    // TODO: Implémenter les marqueurs de trades
    qDebug() << "Ajout des marqueurs de trades";
}

void ChartView::addIndicators()
{
    for (const QString& indicator : m_activeIndicators) {
        auto it = m_indicatorConfigs.find(indicator);
        if (it == m_indicatorConfigs.end()) continue;
        
        QVariantMap config = m_indicatorConfigs[indicator];
        QString type = config["type"].toString();
        
        if (type == "EMA") {
            int period = config["period"].toInt();
            int color = config["color"].toInt();
            addEMAIndicator(period, color);
        } else if (type == "RSI") {
            int period = config["period"].toInt();
            addRSIIndicator(period);
        } else if (type == "STOCH") {
            int fastK = config["fastK"].toInt();
            int slowK = config["slowK"].toInt();
            int slowD = config["slowD"].toInt();
            addStochasticIndicator(fastK, slowK, slowD);
        } else if (type == "ATR") {
            int period = config["period"].toInt();
            addATRIndicator(period);
        }
    }
}

void ChartView::addEMAIndicator(int period, int color)
{
    Q_UNUSED(period)
    Q_UNUSED(color)
    qDebug() << "Ajout EMA" << period;
}

void ChartView::addRSIIndicator(int period)
{
    Q_UNUSED(period)
    qDebug() << "Ajout RSI" << period;
}

void ChartView::addStochasticIndicator(int fastK, int slowK, int slowD)
{
    Q_UNUSED(fastK)
    Q_UNUSED(slowK)
    Q_UNUSED(slowD)
    qDebug() << "Ajout Stochastic";
}

void ChartView::addATRIndicator(int period)
{
    Q_UNUSED(period)
    qDebug() << "Ajout ATR" << period;
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
    if (open.empty()) return;
    
    size_t size = open.size();
    ha_open.resize(size);
    ha_high.resize(size);
    ha_low.resize(size);
    ha_close.resize(size);
    
    // Premier point
    ha_open[0] = (open[0] + close[0]) / 2.0;
    ha_close[0] = (open[0] + high[0] + low[0] + close[0]) / 4.0;
    ha_high[0] = std::max({open[0], high[0], ha_open[0], ha_close[0]});
    ha_low[0] = std::min({open[0], low[0], ha_open[0], ha_close[0]});
    
    // Points suivants
    for (size_t i = 1; i < size; ++i) {
        ha_open[i] = (ha_open[i-1] + ha_close[i-1]) / 2.0;
        ha_close[i] = (open[i] + high[i] + low[i] + close[i]) / 4.0;
        ha_high[i] = std::max({high[i], ha_open[i], ha_close[i]});
        ha_low[i] = std::min({low[i], ha_open[i], ha_close[i]});
    }
}

DoubleArray ChartView::vectorToDoubleArray(const std::vector<double>& vec)
{
    return DoubleArray(vec.data(), static_cast<int>(vec.size()));
}

std::vector<double> ChartView::extractDoubleVector(void* pyObj)
{
    Q_UNUSED(pyObj)
    // TODO: Implémenter l'extraction depuis Python
    return {};
}

std::vector<QString> ChartView::extractStringVector(void* pyObj)
{
    Q_UNUSED(pyObj)
    // TODO: Implémenter l'extraction depuis Python
    return {};
}

QVariant ChartView::extractPythonValue(void* pyObj)
{
    Q_UNUSED(pyObj)
    // TODO: Implémenter l'extraction depuis Python
    return QVariant();
}

bool ChartView::hasValidData() const
{
    return !m_priceData.timestamps.empty() && 
           m_priceData.timestamps.size() == m_priceData.open.size() &&
           m_priceData.timestamps.size() == m_priceData.high.size() &&
           m_priceData.timestamps.size() == m_priceData.low.size() &&
           m_priceData.timestamps.size() == m_priceData.close.size();
}

void ChartView::showPlaceholder(const QString& message)
{
    if (!m_chartPlaceholder) {
        m_chartPlaceholder = new QLabel();
        m_chartPlaceholder->setAlignment(Qt::AlignCenter);
    }
    
    m_chartPlaceholder->setText(message);
    m_chartPlaceholder->show();
    
    // Cacher le viewer s'il existe
    if (m_chartViewer) {
        m_chartViewer->hide();
    }
}

void ChartView::clear()
{
    m_priceData = PriceData{};
    m_tradeData = TradeData{};
    m_equityData = EquityData{};
    m_activeIndicators.clear();
    
    showPlaceholder("Exécutez le backtest pour afficher les graphiques");
}

// Slots
void ChartView::onHeikinAshiToggled(bool checked)
{
    Q_UNUSED(checked)
    updateChart();
}

void ChartView::onVolumeToggled(bool checked)
{
    Q_UNUSED(checked)
    updateChart();
}

void ChartView::onEquityToggled(bool checked)
{
    Q_UNUSED(checked)
    updateChart();
}

void ChartView::onAddIndicatorClicked()
{
    QString indicator = m_indicatorsCombo->currentText();
    
    if (!indicator.isEmpty()) {
        auto it = std::find(m_activeIndicators.begin(), m_activeIndicators.end(), indicator);
        if (it == m_activeIndicators.end()) {
            m_activeIndicators.push_back(indicator);
            updateChart();
        }
    }
}

void ChartView::onMouseMovePlotArea(QMouseEvent* event)
{
    Q_UNUSED(event)
    // TODO: Implémenter la gestion des événements souris
}

void ChartView::updateChart()
{
    if (m_currentData) {
        createChart();
    }
}
