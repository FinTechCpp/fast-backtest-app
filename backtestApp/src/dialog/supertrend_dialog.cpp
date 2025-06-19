#include "dialog/supertrend_dialog.h"

SupertrendDialog::SupertrendDialog(QWidget* parent, ChartWidget* chartWidget, int supertrendId, const SuperTrendInstance& supertrend)
    : QDialog(parent)
    , m_chartWidget(chartWidget)
    , m_supertrendId(supertrendId)
    , m_originalSupertrend(supertrend)
    , m_currentSupertrend(supertrend)
{
    // Configuration du dialogue
    setWindowTitle("Supertrend Settings");
    setMinimumWidth(300);
    setModal(true);
    
    // Layout principal
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Formulaire de paramètres
    QFormLayout* formLayout = new QFormLayout();
    
    // Période
    m_periodSpinBox = new QSpinBox();
    m_periodSpinBox->setRange(2, 100);
    m_periodSpinBox->setValue(supertrend.period);
    formLayout->addRow("ATR Period:", m_periodSpinBox);
    
    // Multiplicateur
    m_multiplierSpinBox = new QDoubleSpinBox();
    m_multiplierSpinBox->setRange(0.1, 10.0);
    m_multiplierSpinBox->setSingleStep(0.1);
    m_multiplierSpinBox->setDecimals(1);
    m_multiplierSpinBox->setValue(supertrend.multiplier);
    formLayout->addRow("Multiplier:", m_multiplierSpinBox);
    
    // Couleur tendance haussière
    m_upColorButton = new QPushButton();
    updateColorButtonStyle(m_upColorButton, supertrend.upColor);
    formLayout->addRow("Up Trend Color:", m_upColorButton);
    
    // Couleur tendance baissière
    m_downColorButton = new QPushButton();
    updateColorButtonStyle(m_downColorButton, supertrend.downColor);
    formLayout->addRow("Down Trend Color:", m_downColorButton);
    
    // Ajouter le formulaire au layout
    mainLayout->addLayout(formLayout);
    
    // Boutons Annuler/Appliquer
    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    mainLayout->addWidget(m_buttonBox);
    
    // Connecter les signaux
    connect(m_periodSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &SupertrendDialog::onPeriodChanged);
    connect(m_multiplierSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &SupertrendDialog::onMultiplierChanged);
    connect(m_upColorButton, &QPushButton::clicked, this, &SupertrendDialog::onUpColorButtonClicked);
    connect(m_downColorButton, &QPushButton::clicked, this, &SupertrendDialog::onDownColorButtonClicked);
    
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &SupertrendDialog::onApply);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &SupertrendDialog::onCancel);
}

SupertrendDialog::~SupertrendDialog()
{
}

void SupertrendDialog::updateColorButtonStyle(QPushButton* button, int color)
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

void SupertrendDialog::onPeriodChanged(int period)
{
    m_currentSupertrend.period = period;
}

void SupertrendDialog::onMultiplierChanged(double multiplier)
{
    m_currentSupertrend.multiplier = multiplier;
}

void SupertrendDialog::onUpColorButtonClicked()
{
    int r = (m_currentSupertrend.upColor >> 16) & 0xFF;
    int g = (m_currentSupertrend.upColor >> 8) & 0xFF;
    int b = m_currentSupertrend.upColor & 0xFF;
    
    QColor initialColor(r, g, b);
    QColor color = QColorDialog::getColor(initialColor, this, "Sélectionner une couleur");
    
    if (color.isValid()) {
        m_currentSupertrend.upColor = (color.red() << 16) | (color.green() << 8) | color.blue();
        updateColorButtonStyle(m_upColorButton, m_currentSupertrend.upColor);
    }
}

void SupertrendDialog::onDownColorButtonClicked()
{
    int r = (m_currentSupertrend.downColor >> 16) & 0xFF;
    int g = (m_currentSupertrend.downColor >> 8) & 0xFF;
    int b = m_currentSupertrend.downColor & 0xFF;
    
    QColor initialColor(r, g, b);
    QColor color = QColorDialog::getColor(initialColor, this, "Sélectionner une couleur");
    
    if (color.isValid()) {
        m_currentSupertrend.downColor = (color.red() << 16) | (color.green() << 8) | color.blue();
        updateColorButtonStyle(m_downColorButton, m_currentSupertrend.downColor);
    }
}

void SupertrendDialog::onApply()
{
    m_chartWidget->setSuperTrendConfig(m_currentSupertrend);
    accept();
}

void SupertrendDialog::onCancel()
{
    // Restaurer les paramètres d'origine
    m_chartWidget->setSuperTrendConfig(m_originalSupertrend);
    reject();
}