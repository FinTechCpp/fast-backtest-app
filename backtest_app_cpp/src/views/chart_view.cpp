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
    , m_currentData(nullptr)
    , m_currentStats(nullptr)
{
    // CORRECTION: Ne pas appeler setupIndicatorsList() dans le constructeur
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
    
    qDebug() << "ChartView::update() appelé avec data:" << data << "stats:" << stats;
    
    if (!stats) {
        clear();
        return;
    }
    
    // TODO: Implémenter la mise à jour du graphique
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
void ChartView::extractDataFromPython(void* data, void* stats) { Q_UNUSED(data); Q_UNUSED(stats); }
void ChartView::extractPriceData(void* data) { Q_UNUSED(data); }
void ChartView::extractTradeData(void* stats) { Q_UNUSED(stats); }
void ChartView::extractEquityData(void* stats) { Q_UNUSED(stats); }
void ChartView::createChart() {}
void ChartView::addMainChart() {}
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
    Q_UNUSED(vec);
    return DoubleArray();
}

std::vector<double> ChartView::extractDoubleVector(void* pyObj)
{
    Q_UNUSED(pyObj);
    return std::vector<double>();
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
    return m_currentData != nullptr && m_currentStats != nullptr;
}

void ChartView::showPlaceholder(const QString& message)
{
    if (m_chartPlaceholder) {
        m_chartPlaceholder->setText(message);
        m_chartPlaceholder->setVisible(true);
    }
}
