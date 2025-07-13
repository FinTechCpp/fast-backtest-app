#include "ui/dialogs/atrDialog.h"

ATRDialog::ATRDialog(QWidget* parent, ChartWidget* chartWidget, int atrId, const ATRInstance& atr)
    : BaseDialog(parent, chartWidget, "ATR Settings")
    , m_atrId(atrId)
    , m_originalAtr(atr)
    , m_currentAtr(atr)
{
    setupUI();
    connectSignals();
}

ATRDialog::~ATRDialog()
{
}

void ATRDialog::setupUI() {
    // Période
    m_periodSpinBox = new QSpinBox();
    m_periodSpinBox->setRange(2, 100);
    m_periodSpinBox->setValue(m_currentAtr.period);
    m_formLayout->addRow("Period:", m_periodSpinBox);
    
    // Hauteur
    m_heightSpinBox = new QSpinBox();
    m_heightSpinBox->setRange(50, 300);
    m_heightSpinBox->setSingleStep(10);
    m_heightSpinBox->setValue(m_currentAtr.height);
    m_formLayout->addRow("Height:", m_heightSpinBox);
    
    // Case à cocher pour l'échelle logarithmique
    m_useLogScaleCheckBox = new QCheckBox();
    m_useLogScaleCheckBox->setChecked(m_currentAtr.useLogScale);
    m_formLayout->addRow("Use Logarithmic Scale:", m_useLogScaleCheckBox);
    
    // Couleur de la ligne
    m_colorButton = new QPushButton();
    updateColorButtonStyle(m_colorButton, m_currentAtr.color);
    m_formLayout->addRow("Line Color:", m_colorButton);
}

void ATRDialog::connectSignals() {
    connect(m_periodSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &ATRDialog::onPeriodChanged);
    connect(m_heightSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &ATRDialog::onHeightChanged);
    connect(m_useLogScaleCheckBox, &QCheckBox::checkStateChanged, this, &ATRDialog::onLogScaleChanged);
    connect(m_colorButton, &QPushButton::clicked, this, &ATRDialog::onColorButtonClicked);
}

void ATRDialog::onPeriodChanged(int period) {
    m_currentAtr.period = period;
    applyChanges(); // Appliquer immédiatement les changements
}

void ATRDialog::onHeightChanged(int height) {
    m_currentAtr.height = height;
    applyChanges(); // Appliquer immédiatement les changements
}

void ATRDialog::onLogScaleChanged(int state) {
    m_currentAtr.useLogScale = (state == Qt::Checked);
    applyChanges(); // Appliquer immédiatement les changements
}

void ATRDialog::onColorButtonClicked() {
    QColor color = openColorDialog(m_currentAtr.color, "Select Line Color");
    
    if (color.isValid()) {
        m_currentAtr.color = colorFromRGB(color.red(), color.green(), color.blue());
        updateColorButtonStyle(m_colorButton, m_currentAtr.color);
        applyChanges(); // Appliquer immédiatement les changements
    }
}

void ATRDialog::applyChanges() {
    m_chartWidget->updateIndicator(m_currentAtr);
}

void ATRDialog::cancelChanges() {
    m_chartWidget->updateIndicator(m_originalAtr);
}