#include "ui/dialogs/bbDialog.h"
#include <QColorDialog>

bbDialog::bbDialog(QWidget* parent, ChartWidget* chartWidget, const indicators::BBInstance& bollingerBands)
    : IndicatorDialog<indicators::BBInstance>(parent, "Bollinger Bands", chartWidget, bollingerBands)
{
    initialize();
}

bbDialog::~bbDialog()
{
}

void bbDialog::setupUI()
{
// Période
    m_periodSpinBox = new QSpinBox();
    m_periodSpinBox->setRange(2, 3000);
    m_formLayout->addRow("Period:", m_periodSpinBox);
    
    // Multiplicateur d'écart-type
    m_stdDevMultiplierSpinBox = new QDoubleSpinBox();
    m_stdDevMultiplierSpinBox->setRange(0.1, 10.0);
    m_stdDevMultiplierSpinBox->setSingleStep(0.1);
    m_stdDevMultiplierSpinBox->setDecimals(2);
    m_formLayout->addRow("Std Dev Multiplier:", m_stdDevMultiplierSpinBox);
    
    // Source de données
    m_sourceCombo = new QComboBox();
    m_sourceCombo->addItem("Close", static_cast<int>(filter::PriceType::CLOSE));
    m_sourceCombo->addItem("Open", static_cast<int>(filter::PriceType::OPEN));
    m_sourceCombo->addItem("High", static_cast<int>(filter::PriceType::HIGH));
    m_sourceCombo->addItem("Low", static_cast<int>(filter::PriceType::LOW));
    m_sourceCombo->addItem("Typical (H+L+C)/3", static_cast<int>(filter::PriceType::TYPICAL));
    m_sourceCombo->addItem("Median (H+L)/2", static_cast<int>(filter::PriceType::MEDIAN));
    m_formLayout->addRow("Source:", m_sourceCombo);
    
    // Type de moyenne mobile
    m_maTypeCombo = new QComboBox();
    m_maTypeCombo->addItem("SMA", static_cast<int>(filter::MAType::SMA));
    m_maTypeCombo->addItem("EMA", static_cast<int>(filter::MAType::EMA));
    m_formLayout->addRow("MA Type:", m_maTypeCombo);
    
    // Couleur de la bande médiane
    m_middleBandColorButton = new QPushButton();
    m_formLayout->addRow("Middle Band Color:", m_middleBandColorButton);
    
    // Couleur de la bande supérieure
    m_upperBandColorButton = new QPushButton();
    m_formLayout->addRow("Upper Band Color:", m_upperBandColorButton);
    
    // Couleur de la bande inférieure
    m_lowerBandColorButton = new QPushButton();
    m_formLayout->addRow("Lower Band Color:", m_lowerBandColorButton);
}

void bbDialog::connectSignals()
{
    connect(m_periodSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &bbDialog::onPeriodChanged);
    connect(m_stdDevMultiplierSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &bbDialog::onStdDevMultiplierChanged);
    connect(m_sourceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &bbDialog::onSourceChanged);
    connect(m_maTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &bbDialog::onMATypeChanged);
    connect(m_middleBandColorButton, &QPushButton::clicked, this, &bbDialog::onMiddleBandColorClicked);
    connect(m_upperBandColorButton, &QPushButton::clicked, this, &bbDialog::onUpperBandColorClicked);
    connect(m_lowerBandColorButton, &QPushButton::clicked, this, &bbDialog::onLowerBandColorClicked);
}

void bbDialog::updateUIFromInstance()
{
    m_periodSpinBox->setValue(m_currentIndicator.period);
    m_stdDevMultiplierSpinBox->setValue(m_currentIndicator.stddev_multiplier);
    m_sourceCombo->setCurrentIndex(m_sourceCombo->findData(static_cast<int>(m_currentIndicator.source)));
    m_maTypeCombo->setCurrentIndex(m_maTypeCombo->findData(static_cast<int>(m_currentIndicator.ma_type)));
    updateColorButtonStyle(m_middleBandColorButton, m_currentIndicator.middleBandColor);
    updateColorButtonStyle(m_upperBandColorButton, m_currentIndicator.upperBandColor);
    updateColorButtonStyle(m_lowerBandColorButton, m_currentIndicator.lowerBandColor);
}

void bbDialog::onPeriodChanged(int period)
{
    m_currentIndicator.period = period;
    applyChanges(); // Appliquer immédiatement les changements
}

void bbDialog::onColorButtonClicked(QPushButton* button, int& colorField)
{
    QColor color = openColorDialog(colorField, "Select Color");
    
    if (color.isValid()) {
        colorField = colorFromRGB(color.red(), color.green(), color.blue());
        updateColorButtonStyle(button, colorField);
        applyChanges(); // Appliquer immédiatement les changements
    }
}

void bbDialog::onStdDevMultiplierChanged(double multiplier)
{
    m_currentIndicator.stddev_multiplier = multiplier;
    applyChanges();
}

void bbDialog::onSourceChanged(int index)
{
    int data = m_sourceCombo->itemData(index).toInt();
    m_currentIndicator.source = static_cast<filter::PriceType>(data);
    applyChanges();
}

void bbDialog::onMATypeChanged(int index)
{
    int data = m_maTypeCombo->itemData(index).toInt();
    m_currentIndicator.ma_type = static_cast<filter::MAType>(data);
    applyChanges();
}

void bbDialog::onMiddleBandColorClicked()
{
    onColorButtonClicked(m_middleBandColorButton, m_currentIndicator.middleBandColor);
}

void bbDialog::onUpperBandColorClicked()
{
    onColorButtonClicked(m_upperBandColorButton, m_currentIndicator.upperBandColor);
}

void bbDialog::onLowerBandColorClicked()
{
    onColorButtonClicked(m_lowerBandColorButton, m_currentIndicator.lowerBandColor);
}
