#include "dialog/atr_dialog.h"

ATRDialog::ATRDialog(QWidget* parent, ChartWidget* chartWidget, int atrId, const ATRInstance& atr)
    : QDialog(parent)
    , m_chartWidget(chartWidget)
    , m_atrId(atrId)
    , m_originalAtr(atr)
    , m_currentAtr(atr)
{
    // Configuration du dialogue
    setWindowTitle("ATR Settings");
    setMinimumWidth(300);
    setModal(true);
    
    // Layout principal
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Formulaire de paramètres
    QFormLayout* formLayout = new QFormLayout();
    
    // Période
    m_periodSpinBox = new QSpinBox();
    m_periodSpinBox->setRange(2, 100);
    m_periodSpinBox->setValue(atr.period);
    formLayout->addRow("Period:", m_periodSpinBox);
    
    // Hauteur
    m_heightSpinBox = new QSpinBox();
    m_heightSpinBox->setRange(50, 300);
    m_heightSpinBox->setSingleStep(10);
    m_heightSpinBox->setValue(atr.height);
    formLayout->addRow("Height:", m_heightSpinBox);
    
    // Couleur de la ligne
    m_colorButton = new QPushButton();
    updateColorButtonStyle(m_colorButton, atr.color);
    formLayout->addRow("Line Color:", m_colorButton);
    
    // Ajouter le formulaire au layout
    mainLayout->addLayout(formLayout);
    
    // Boutons Annuler/Appliquer
    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    mainLayout->addWidget(m_buttonBox);
    
    // Connecter les signaux
    connect(m_periodSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &ATRDialog::onPeriodChanged);
    connect(m_heightSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &ATRDialog::onHeightChanged);
    connect(m_colorButton, &QPushButton::clicked, this, &ATRDialog::onColorButtonClicked);
    
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &ATRDialog::onApply);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &ATRDialog::onCancel);
}

ATRDialog::~ATRDialog()
{
}

void ATRDialog::updateColorButtonStyle(QPushButton* button, int color)
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

void ATRDialog::onPeriodChanged(int period)
{
    m_currentAtr.period = period;
    // updateATR();
}

void ATRDialog::onHeightChanged(int height)
{
    m_currentAtr.height = height;
    // updateATR();
}

void ATRDialog::onColorButtonClicked()
{
    int r = (m_currentAtr.color >> 16) & 0xFF;
    int g = (m_currentAtr.color >> 8) & 0xFF;
    int b = m_currentAtr.color & 0xFF;
    
    QColor initialColor(r, g, b);
    QColor color = QColorDialog::getColor(initialColor, this, "Sélectionner une couleur");
    
    if (color.isValid()) {
        m_currentAtr.color = (color.red() << 16) | (color.green() << 8) | color.blue();
        updateColorButtonStyle(m_colorButton, m_currentAtr.color);
        // updateATR();
    }
}

void ATRDialog::updateATR()
{
    // Mettre à jour les paramètres de l'ATR en temps réel
    // m_chartWidget->setATRPeriod(m_atrId, m_currentAtr.period);
    // m_chartWidget->setATRHeight(m_atrId, m_currentAtr.height);
    // m_chartWidget->setATRColor(m_atrId, m_currentAtr.color);
    
    // Note: ChartWidget devrait avoir des méthodes pour ces paramètres
    // si ce n'est pas le cas, il faudra les ajouter
}

void ATRDialog::onApply()
{
    m_chartWidget->setATRConfig(m_currentAtr);
    accept();
}

void ATRDialog::onCancel()
{
    // Restaurer les paramètres d'origine
    m_chartWidget->setATRConfig(m_originalAtr);
    reject();
}