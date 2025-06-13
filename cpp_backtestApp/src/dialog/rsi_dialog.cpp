#include "dialog/rsi_dialog.h"

RSIDialog::RSIDialog(QWidget* parent, ChartWidget* chartWidget, int rsiId, const RSIInstance& rsi)
    : QDialog(parent)
    , m_chartWidget(chartWidget)
    , m_rsiId(rsiId)
    , m_originalRsi(rsi)
    , m_currentRsi(rsi)
{
    // Configuration du dialogue
    setWindowTitle("RSI Settings");
    setMinimumWidth(300);
    setModal(true);
    
    // Layout principal
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Formulaire de paramètres
    QFormLayout* formLayout = new QFormLayout();
    
    // Période
    m_periodSpinBox = new QSpinBox();
    m_periodSpinBox->setRange(2, 100);
    m_periodSpinBox->setValue(rsi.period);
    formLayout->addRow("Period:", m_periodSpinBox);
    
    // Hauteur
    m_heightSpinBox = new QSpinBox();
    m_heightSpinBox->setRange(50, 300);
    m_heightSpinBox->setSingleStep(10);
    m_heightSpinBox->setValue(rsi.height);
    formLayout->addRow("Height:", m_heightSpinBox);
    
    // Range
    m_rangeSpinBox = new QDoubleSpinBox();
    m_rangeSpinBox->setRange(5, 40);
    m_rangeSpinBox->setSingleStep(1);
    m_rangeSpinBox->setValue(rsi.range);
    formLayout->addRow("Range:", m_rangeSpinBox);
    
    // Couleur principale
    m_colorButton = new QPushButton();
    updateColorButtonStyle(m_colorButton, rsi.color);
    formLayout->addRow("Line Color:", m_colorButton);
    
    // Couleur zone supérieure
    m_upperColorButton = new QPushButton();
    updateColorButtonStyle(m_upperColorButton, rsi.upperColor);
    formLayout->addRow("Upper Zone Color:", m_upperColorButton);
    
    // Couleur zone inférieure
    m_lowerColorButton = new QPushButton();
    updateColorButtonStyle(m_lowerColorButton, rsi.lowerColor);
    formLayout->addRow("Lower Zone Color:", m_lowerColorButton);
    
    // Ajouter le formulaire au layout
    mainLayout->addLayout(formLayout);
    
    // Boutons Annuler/Appliquer
    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    mainLayout->addWidget(m_buttonBox);
    
    // Connecter les signaux
    connect(m_periodSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &RSIDialog::onPeriodChanged);
    connect(m_heightSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &RSIDialog::onHeightChanged);
    connect(m_rangeSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &RSIDialog::onRangeChanged);
    connect(m_colorButton, &QPushButton::clicked, this, &RSIDialog::onColorButtonClicked);
    connect(m_upperColorButton, &QPushButton::clicked, this, &RSIDialog::onUpperColorButtonClicked);
    connect(m_lowerColorButton, &QPushButton::clicked, this, &RSIDialog::onLowerColorButtonClicked);
    
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &RSIDialog::onApply);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &RSIDialog::onCancel);
}

RSIDialog::~RSIDialog()
{
}

void RSIDialog::updateColorButtonStyle(QPushButton* button, int color)
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

void RSIDialog::onPeriodChanged(int period)
{
    m_currentRsi.period = period;
    // updateRSI();
}

void RSIDialog::onHeightChanged(int height)
{
    m_currentRsi.height = height;
    // updateRSI();
}

void RSIDialog::onRangeChanged(double range)
{
    m_currentRsi.range = range;
    // updateRSI();
}

void RSIDialog::onColorButtonClicked()
{
    int r = (m_currentRsi.color >> 16) & 0xFF;
    int g = (m_currentRsi.color >> 8) & 0xFF;
    int b = m_currentRsi.color & 0xFF;
    
    QColor initialColor(r, g, b);
    QColor color = QColorDialog::getColor(initialColor, this, "Sélectionner une couleur");
    
    if (color.isValid()) {
        m_currentRsi.color = (color.red() << 16) | (color.green() << 8) | color.blue();
        updateColorButtonStyle(m_colorButton, m_currentRsi.color);
        // updateRSI();
    }
}

void RSIDialog::onUpperColorButtonClicked()
{
    int r = (m_currentRsi.upperColor >> 16) & 0xFF;
    int g = (m_currentRsi.upperColor >> 8) & 0xFF;
    int b = m_currentRsi.upperColor & 0xFF;
    
    QColor initialColor(r, g, b);
    QColor color = QColorDialog::getColor(initialColor, this, "Sélectionner une couleur");
    
    if (color.isValid()) {
        m_currentRsi.upperColor = (color.red() << 16) | (color.green() << 8) | color.blue();
        updateColorButtonStyle(m_upperColorButton, m_currentRsi.upperColor);
        // updateRSI();
    }
}

void RSIDialog::onLowerColorButtonClicked()
{
    int r = (m_currentRsi.lowerColor >> 16) & 0xFF;
    int g = (m_currentRsi.lowerColor >> 8) & 0xFF;
    int b = m_currentRsi.lowerColor & 0xFF;
    
    QColor initialColor(r, g, b);
    QColor color = QColorDialog::getColor(initialColor, this, "Sélectionner une couleur");
    
    if (color.isValid()) {
        m_currentRsi.lowerColor = (color.red() << 16) | (color.green() << 8) | color.blue();
        updateColorButtonStyle(m_lowerColorButton, m_currentRsi.lowerColor);
        // updateRSI();
    }
}

void RSIDialog::updateRSI()
{
    // Mettre à jour les paramètres du RSI en temps réel
    // m_chartWidget->setRSIPeriod(m_rsiId, m_currentRsi.period);
    // m_chartWidget->setRSIHeight(m_rsiId, m_currentRsi.height);
    // m_chartWidget->setRSIRange(m_rsiId, m_currentRsi.range);
    // m_chartWidget->setRSIColor(m_rsiId, m_currentRsi.color);
    
    // Note: ChartWidget devrait avoir des méthodes pour ces paramètres
    // si ce n'est pas le cas, il faudra les ajouter
}

void RSIDialog::onApply()
{
    m_chartWidget->setRSIConfig(m_rsiId, m_currentRsi);
    accept();
}

void RSIDialog::onCancel()
{
    // Restaurer les paramètres d'origine
    m_chartWidget->setRSIConfig(m_rsiId, m_originalRsi);
    reject();
}

