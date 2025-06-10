#include "dialog/stochastic_dialog.h"

StochasticDialog::StochasticDialog(QWidget* parent, ChartWidget* chartWidget, int stochasticId, const ChartWidget::StochasticInstance& stochastic)
    : QDialog(parent)
    , m_chartWidget(chartWidget)
    , m_stochasticId(stochasticId)
    , m_originalStochastic(stochastic)
    , m_currentStochastic(stochastic)
{
    // Configuration du dialogue
    setWindowTitle("Stochastic Settings");
    setMinimumWidth(300);
    setModal(true);
    
    // Layout principal
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Formulaire de paramètres
    QFormLayout* formLayout = new QFormLayout();
    
    // Période Fast K
    m_fastKPeriodSpinBox = new QSpinBox();
    m_fastKPeriodSpinBox->setRange(2, 100);
    m_fastKPeriodSpinBox->setValue(stochastic.fastKPeriod);
    formLayout->addRow("Fast K Period:", m_fastKPeriodSpinBox);
    
    // Période Slow K
    m_slowKPeriodSpinBox = new QSpinBox();
    m_slowKPeriodSpinBox->setRange(1, 100);
    m_slowKPeriodSpinBox->setValue(stochastic.slowKPeriod);
    formLayout->addRow("Slow K Period:", m_slowKPeriodSpinBox);
    
    // Période Slow D
    m_slowDPeriodSpinBox = new QSpinBox();
    m_slowDPeriodSpinBox->setRange(1, 100);
    m_slowDPeriodSpinBox->setValue(stochastic.slowDPeriod);
    formLayout->addRow("Slow D Period:", m_slowDPeriodSpinBox);
    
    // Hauteur
    m_heightSpinBox = new QSpinBox();
    m_heightSpinBox->setRange(50, 300);
    m_heightSpinBox->setSingleStep(10);
    m_heightSpinBox->setValue(stochastic.height);
    formLayout->addRow("Height:", m_heightSpinBox);
    
    // Niveau de surachat
    m_overboughtLevelSpinBox = new QSpinBox();
    m_overboughtLevelSpinBox->setRange(50, 100);
    m_overboughtLevelSpinBox->setValue(stochastic.overboughtLevel);
    formLayout->addRow("Overbought Level:", m_overboughtLevelSpinBox);
    
    // Niveau de survente
    m_oversoldLevelSpinBox = new QSpinBox();
    m_oversoldLevelSpinBox->setRange(0, 50);
    m_oversoldLevelSpinBox->setValue(stochastic.oversoldLevel);
    formLayout->addRow("Oversold Level:", m_oversoldLevelSpinBox);
    
    // Couleur de la ligne K
    m_kColorButton = new QPushButton();
    updateColorButtonStyle(m_kColorButton, stochastic.kColor);
    formLayout->addRow("%K Line Color:", m_kColorButton);
    
    // Couleur de la ligne D
    m_dColorButton = new QPushButton();
    updateColorButtonStyle(m_dColorButton, stochastic.dColor);
    formLayout->addRow("%D Line Color:", m_dColorButton);
    
    // Ajouter le formulaire au layout
    mainLayout->addLayout(formLayout);
    
    // Boutons Annuler/Appliquer
    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    mainLayout->addWidget(m_buttonBox);
    
    // Connecter les signaux
    connect(m_fastKPeriodSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &StochasticDialog::onFastKPeriodChanged);
    connect(m_slowKPeriodSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &StochasticDialog::onSlowKPeriodChanged);
    connect(m_slowDPeriodSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &StochasticDialog::onSlowDPeriodChanged);
    connect(m_heightSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &StochasticDialog::onHeightChanged);
    connect(m_overboughtLevelSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &StochasticDialog::onOverboughtLevelChanged);
    connect(m_oversoldLevelSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &StochasticDialog::onOversoldLevelChanged);
    connect(m_kColorButton, &QPushButton::clicked, this, &StochasticDialog::onKColorButtonClicked);
    connect(m_dColorButton, &QPushButton::clicked, this, &StochasticDialog::onDColorButtonClicked);
    
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &StochasticDialog::onApply);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &StochasticDialog::onCancel);
}

StochasticDialog::~StochasticDialog()
{
}

void StochasticDialog::updateColorButtonStyle(QPushButton* button, int color)
{
    int r = (color >> 16) & 0xFF;
    int g = (color >> 8) & 0xFF;
    int b = color & 0xFF;
    
    QString styleSheet = QString("background-color: rgb(%1, %2, %3); ")
                          .arg(r).arg(g).arg(b);
                          
    // Ajuster le texte pour qu'il soit lisible sur la couleur de fond
    int brightness = (r * 299 + g * 587 + b * 114) / 1000;
    if (brightness > 125) {
        styleSheet += "color: black;";
    } else {
        styleSheet += "color: white;";
    }
    
    button->setStyleSheet(styleSheet);
    button->setText(QString("#%1").arg(color, 6, 16, QChar('0')));
}

void StochasticDialog::onFastKPeriodChanged(int period)
{
    m_currentStochastic.fastKPeriod = period;
}

void StochasticDialog::onSlowKPeriodChanged(int period)
{
    m_currentStochastic.slowKPeriod = period;
}

void StochasticDialog::onSlowDPeriodChanged(int period)
{
    m_currentStochastic.slowDPeriod = period;
}

void StochasticDialog::onHeightChanged(int height)
{
    m_currentStochastic.height = height;
}

void StochasticDialog::onOverboughtLevelChanged(int level)
{
    m_currentStochastic.overboughtLevel = level;
    
    // Assurer que le niveau de surachat est toujours supérieur au niveau de survente
    if (level <= m_oversoldLevelSpinBox->value()) {
        m_oversoldLevelSpinBox->setValue(level - 1);
    }
}

void StochasticDialog::onOversoldLevelChanged(int level)
{
    m_currentStochastic.oversoldLevel = level;
    
    // Assurer que le niveau de survente est toujours inférieur au niveau de surachat
    if (level >= m_overboughtLevelSpinBox->value()) {
        m_overboughtLevelSpinBox->setValue(level + 1);
    }
}

void StochasticDialog::onKColorButtonClicked()
{
    int r = (m_currentStochastic.kColor >> 16) & 0xFF;
    int g = (m_currentStochastic.kColor >> 8) & 0xFF;
    int b = m_currentStochastic.kColor & 0xFF;
    
    QColor initialColor(r, g, b);
    QColor color = QColorDialog::getColor(initialColor, this, "Sélectionner une couleur pour %K");
    
    if (color.isValid()) {
        m_currentStochastic.kColor = (color.red() << 16) | (color.green() << 8) | color.blue();
        updateColorButtonStyle(m_kColorButton, m_currentStochastic.kColor);
    }
}

void StochasticDialog::onDColorButtonClicked()
{
    int r = (m_currentStochastic.dColor >> 16) & 0xFF;
    int g = (m_currentStochastic.dColor >> 8) & 0xFF;
    int b = m_currentStochastic.dColor & 0xFF;
    
    QColor initialColor(r, g, b);
    QColor color = QColorDialog::getColor(initialColor, this, "Sélectionner une couleur pour %D");
    
    if (color.isValid()) {
        m_currentStochastic.dColor = (color.red() << 16) | (color.green() << 8) | color.blue();
        updateColorButtonStyle(m_dColorButton, m_currentStochastic.dColor);
    }
}

void StochasticDialog::onApply()
{
    m_chartWidget->setStochasticConfig(m_stochasticId, m_currentStochastic);
    m_chartWidget->updateChartDisplay();
    accept();
}

void StochasticDialog::onCancel()
{
    // Restaurer les paramètres d'origine
    m_chartWidget->setStochasticConfig(m_stochasticId, m_originalStochastic);
    
    // Mettre à jour le graphique
    m_chartWidget->updateChartDisplay();
    
    // Fermer la boîte de dialogue
    reject();
}