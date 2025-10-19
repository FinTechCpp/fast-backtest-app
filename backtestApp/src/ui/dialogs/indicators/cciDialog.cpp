#include "ui/dialogs/indicators/cciDialog.h"

CCIDialog::CCIDialog(QWidget* parent, ChartWidget* chartWidget, const indicators::CCIInstance& cci)
    : IndicatorDialog<indicators::CCIInstance>(parent, "CCI", chartWidget, cci)
{
    initialize();
}

CCIDialog::~CCIDialog() 
{
}

void CCIDialog::setupUI() {
    // Période
    m_periodSpinBox = new QSpinBox();
    m_periodSpinBox->setRange(2, 1000);
    m_formLayout->addRow("Period:", m_periodSpinBox);
    
    // Hauteur
    m_heightSpinBox = new QSpinBox();
    m_heightSpinBox->setRange(50, 300);
    m_heightSpinBox->setSingleStep(10);
    m_formLayout->addRow("Height:", m_heightSpinBox);
    
    // Niveau supérieur (typiquement +100)
    m_upperLevelSpinBox = new QSpinBox();
    m_upperLevelSpinBox->setRange(50, 300);
    m_formLayout->addRow("Upper Level:", m_upperLevelSpinBox);
    
    // Niveau inférieur (typiquement -100)
    m_lowerLevelSpinBox = new QSpinBox();
    m_lowerLevelSpinBox->setRange(-300, -50);
    m_formLayout->addRow("Lower Level:", m_lowerLevelSpinBox);
    
    // Couleur principale
    m_colorButton = new QPushButton();
    m_formLayout->addRow("Line Color:", m_colorButton);
    
    // Couleur zone supérieure
    m_upperColorButton = new QPushButton();
    m_formLayout->addRow("Upper Zone Color:", m_upperColorButton);
    
    // Couleur zone inférieure
    m_lowerColorButton = new QPushButton();
    m_formLayout->addRow("Lower Zone Color:", m_lowerColorButton);
}

void CCIDialog::connectSignals() {
    connect(m_periodSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &CCIDialog::onPeriodChanged);
    connect(m_heightSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &CCIDialog::onHeightChanged);
    connect(m_upperLevelSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &CCIDialog::onUpperLevelChanged);
    connect(m_lowerLevelSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &CCIDialog::onLowerLevelChanged);
    connect(m_colorButton, &QPushButton::clicked, this, &CCIDialog::onColorButtonClicked);
    connect(m_upperColorButton, &QPushButton::clicked, this, &CCIDialog::onUpperColorButtonClicked);
    connect(m_lowerColorButton, &QPushButton::clicked, this, &CCIDialog::onLowerColorButtonClicked);
}

void CCIDialog::updateUIFromInstance()
{
    m_periodSpinBox->setValue(m_currentIndicator.period);
    m_heightSpinBox->setValue(m_currentIndicator.height);
    m_upperLevelSpinBox->setValue(m_currentIndicator.upperLevel);
    m_lowerLevelSpinBox->setValue(m_currentIndicator.lowerLevel);
    updateColorButtonStyle(m_colorButton, m_currentIndicator.color);
    updateColorButtonStyle(m_upperColorButton, m_currentIndicator.upperColor);
    updateColorButtonStyle(m_lowerColorButton, m_currentIndicator.lowerColor);
}

void CCIDialog::onPeriodChanged(int period) {
    m_currentIndicator.period = period;
    applyChanges();
}

void CCIDialog::onHeightChanged(int height) {
    m_currentIndicator.height = height;
    applyChanges();
}

void CCIDialog::onUpperLevelChanged(int level) {
    m_currentIndicator.upperLevel = level;
    applyChanges();
}

void CCIDialog::onLowerLevelChanged(int level) {
    m_currentIndicator.lowerLevel = level;
    applyChanges();
}

void CCIDialog::onColorButtonClicked() {
    QColor color = openColorDialog(m_currentIndicator.color, "Select Line Color");
    
    if (color.isValid()) {
        m_currentIndicator.color = colorFromRGB(color.red(), color.green(), color.blue());
        updateColorButtonStyle(m_colorButton, m_currentIndicator.color);
        applyChanges();
    }
}

void CCIDialog::onUpperColorButtonClicked() {
    QColor color = openColorDialog(m_currentIndicator.upperColor, "Select Upper Zone Color");

    if (color.isValid()) {
        m_currentIndicator.upperColor = colorFromRGB(color.red(), color.green(), color.blue());
        updateColorButtonStyle(m_upperColorButton, m_currentIndicator.upperColor);
        applyChanges();
    }
}

void CCIDialog::onLowerColorButtonClicked() {
    QColor color = openColorDialog(m_currentIndicator.lowerColor, "Select Lower Zone Color");

    if (color.isValid()) {
        m_currentIndicator.lowerColor = colorFromRGB(color.red(), color.green(), color.blue());
        updateColorButtonStyle(m_lowerColorButton, m_currentIndicator.lowerColor);
        applyChanges();
    }
}
