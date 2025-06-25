#include "dialog/stochastic_dialog.h"

StochasticDialog::StochasticDialog(QWidget* parent, ChartWidget* chartWidget, int stochasticId, const StochasticInstance& stochastic)
    : IndicatorDialog(parent, chartWidget, "Stochastic Settings")
    , m_stochasticId(stochasticId)
    , m_originalStochastic(stochastic)
    , m_currentStochastic(stochastic)
{
    setupUI();
    connectSignals();
}

StochasticDialog::~StochasticDialog()
{
}

void StochasticDialog::setupUI() {
    // Période Fast K
    m_fastKPeriodSpinBox = new QSpinBox();
    m_fastKPeriodSpinBox->setRange(2, 100);
    m_fastKPeriodSpinBox->setValue(m_currentStochastic.fastKPeriod);
    m_formLayout->addRow("Fast K Period:", m_fastKPeriodSpinBox);
    
    // Période Slow K
    m_slowKPeriodSpinBox = new QSpinBox();
    m_slowKPeriodSpinBox->setRange(1, 100);
    m_slowKPeriodSpinBox->setValue(m_currentStochastic.slowKPeriod);
    m_formLayout->addRow("Slow K Period:", m_slowKPeriodSpinBox);
    
    // Période Slow D
    m_slowDPeriodSpinBox = new QSpinBox();
    m_slowDPeriodSpinBox->setRange(1, 100);
    m_slowDPeriodSpinBox->setValue(m_currentStochastic.slowDPeriod);
    m_formLayout->addRow("Slow D Period:", m_slowDPeriodSpinBox);
    
    // Hauteur
    m_heightSpinBox = new QSpinBox();
    m_heightSpinBox->setRange(50, 300);
    m_heightSpinBox->setSingleStep(10);
    m_heightSpinBox->setValue(m_currentStochastic.height);
    m_formLayout->addRow("Height:", m_heightSpinBox);
    
    // Niveau de surachat
    m_overboughtLevelSpinBox = new QSpinBox();
    m_overboughtLevelSpinBox->setRange(50, 100);
    m_overboughtLevelSpinBox->setValue(m_currentStochastic.overboughtLevel);
    m_formLayout->addRow("Overbought Level:", m_overboughtLevelSpinBox);
    
    // Niveau de survente
    m_oversoldLevelSpinBox = new QSpinBox();
    m_oversoldLevelSpinBox->setRange(0, 50);
    m_oversoldLevelSpinBox->setValue(m_currentStochastic.oversoldLevel);
    m_formLayout->addRow("Oversold Level:", m_oversoldLevelSpinBox);
    
    // Couleur de la ligne K
    m_kColorButton = new QPushButton();
    updateColorButtonStyle(m_kColorButton, m_currentStochastic.kColor);
    m_formLayout->addRow("%K Line Color:", m_kColorButton);
    
    // Couleur de la ligne D
    m_dColorButton = new QPushButton();
    updateColorButtonStyle(m_dColorButton, m_currentStochastic.dColor);
    m_formLayout->addRow("%D Line Color:", m_dColorButton);
}

void StochasticDialog::connectSignals()
{
    connect(m_fastKPeriodSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &StochasticDialog::onFastKPeriodChanged);
    connect(m_slowKPeriodSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &StochasticDialog::onSlowKPeriodChanged);
    connect(m_slowDPeriodSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &StochasticDialog::onSlowDPeriodChanged);
    connect(m_heightSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &StochasticDialog::onHeightChanged);
    connect(m_overboughtLevelSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &StochasticDialog::onOverboughtLevelChanged);
    connect(m_oversoldLevelSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &StochasticDialog::onOversoldLevelChanged);
    connect(m_kColorButton, &QPushButton::clicked, this, &StochasticDialog::onKColorButtonClicked);
    connect(m_dColorButton, &QPushButton::clicked, this, &StochasticDialog::onDColorButtonClicked);
}

void StochasticDialog::onFastKPeriodChanged(int period) {
    m_currentStochastic.fastKPeriod = period;
    applyChanges();
}

void StochasticDialog::onSlowKPeriodChanged(int period) {
    m_currentStochastic.slowKPeriod = period;
    applyChanges();
}

void StochasticDialog::onSlowDPeriodChanged(int period) {
    m_currentStochastic.slowDPeriod = period;
    applyChanges();
}

void StochasticDialog::onHeightChanged(int height) {
    m_currentStochastic.height = height;
    applyChanges();
}

void StochasticDialog::onOverboughtLevelChanged(int level) {
    m_currentStochastic.overboughtLevel = level;
    applyChanges();
    
    // Assurer que le niveau de surachat est toujours supérieur au niveau de survente
    if (level <= m_oversoldLevelSpinBox->value()) {
        m_oversoldLevelSpinBox->setValue(level - 1);
    }
}

void StochasticDialog::onOversoldLevelChanged(int level) {
    m_currentStochastic.oversoldLevel = level;
    applyChanges();
    
    // Assurer que le niveau de survente est toujours inférieur au niveau de surachat
    if (level >= m_overboughtLevelSpinBox->value()) {
        m_overboughtLevelSpinBox->setValue(level + 1);
    }
}

void StochasticDialog::onKColorButtonClicked() {
    QColor color = openColorDialog(m_currentStochastic.kColor, "Select %K Line Color");
    
    if (color.isValid()) {
        m_currentStochastic.kColor = colorFromRGB(color.red(), color.green(), color.blue());
        updateColorButtonStyle(m_kColorButton, m_currentStochastic.kColor);
        applyChanges();
    }
}

void StochasticDialog::onDColorButtonClicked() {
    QColor color = openColorDialog(m_currentStochastic.dColor, "Select %D Line Color");
    
    if (color.isValid()) {
        m_currentStochastic.dColor = colorFromRGB(color.red(), color.green(), color.blue());
        updateColorButtonStyle(m_dColorButton, m_currentStochastic.dColor);
        applyChanges();
    }
}

void StochasticDialog::applyChanges() {
    m_chartWidget->updateIndicator(m_currentStochastic);
}

void StochasticDialog::cancelChanges() {
    m_chartWidget->updateIndicator(m_originalStochastic);
}