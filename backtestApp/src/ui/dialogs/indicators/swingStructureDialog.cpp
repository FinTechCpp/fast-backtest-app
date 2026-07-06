#include "ui/dialogs/indicators/swingStructureDialog.h"

SwingStructureDialog::SwingStructureDialog(QWidget* parent, ChartWidget* chartWidget, const indicators::SwingStructureInstance& swingStructure)
    : IndicatorDialog<indicators::SwingStructureInstance>(parent, "Swing Structure Trend", chartWidget, swingStructure)
{
    initialize();
}

SwingStructureDialog::~SwingStructureDialog()
{
}

void SwingStructureDialog::setupUI()
{
    // High move
    m_highMoveSpinBox = new QDoubleSpinBox();
    m_highMoveSpinBox->setRange(0.00001, 100000.0);
    m_highMoveSpinBox->setDecimals(5);
    m_highMoveSpinBox->setSingleStep(0.0001);
    m_formLayout->addRow("High move (price):", m_highMoveSpinBox);

    // Low move
    m_lowMoveSpinBox = new QDoubleSpinBox();
    m_lowMoveSpinBox->setRange(0.00001, 100000.0);
    m_lowMoveSpinBox->setDecimals(5);
    m_lowMoveSpinBox->setSingleStep(0.0001);
    m_formLayout->addRow("Low move (price):", m_lowMoveSpinBox);

    // Min periods
    m_minPeriodsSpinBox = new QSpinBox();
    m_minPeriodsSpinBox->setRange(1, 1000);
    m_formLayout->addRow("Min periods:", m_minPeriodsSpinBox);

    // Max periods
    m_maxPeriodsSpinBox = new QSpinBox();
    m_maxPeriodsSpinBox->setRange(1, 1000);
    m_formLayout->addRow("Max periods:", m_maxPeriodsSpinBox);

    // Checkbox for resetting on new day
    m_resetOnNewDayCheckBox = new QCheckBox();
    m_formLayout->addRow("Reset on New Day:", m_resetOnNewDayCheckBox);

    // Swing High Color
    m_swingHighColorButton = new QPushButton();
    m_formLayout->addRow("Swing High Color:", m_swingHighColorButton);

    // Swing Low Color
    m_swingLowColorButton = new QPushButton();
    m_formLayout->addRow("Swing Low Color:", m_swingLowColorButton);

    // Up Trend Color
    m_upColorButton = new QPushButton();
    m_formLayout->addRow("Up Trend Color:", m_upColorButton);

    // Down Trend Color
    m_downColorButton = new QPushButton();
    m_formLayout->addRow("Down Trend Color:", m_downColorButton);

    // Uncertain Trend Color
    m_uncertainColorButton = new QPushButton();
    m_formLayout->addRow("Uncertain Trend Color:", m_uncertainColorButton);
}

void SwingStructureDialog::connectSignals()
{
    connect(m_highMoveSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &SwingStructureDialog::onHighMoveChanged);
    connect(m_lowMoveSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &SwingStructureDialog::onLowMoveChanged);
    connect(m_minPeriodsSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &SwingStructureDialog::onMinPeriodsChanged);
    connect(m_maxPeriodsSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &SwingStructureDialog::onMaxPeriodsChanged);
    connect(m_resetOnNewDayCheckBox, &QCheckBox::checkStateChanged, this, &SwingStructureDialog::onResetOnNewDayChanged);
    connect(m_swingHighColorButton, &QPushButton::clicked, this, &SwingStructureDialog::onSwingHighColorButtonClicked);
    connect(m_swingLowColorButton, &QPushButton::clicked, this, &SwingStructureDialog::onSwingLowColorButtonClicked);
    connect(m_upColorButton, &QPushButton::clicked, this, &SwingStructureDialog::onUpColorButtonClicked);
    connect(m_downColorButton, &QPushButton::clicked, this, &SwingStructureDialog::onDownColorButtonClicked);
    connect(m_uncertainColorButton, &QPushButton::clicked, this, &SwingStructureDialog::onUncertainColorButtonClicked);
}

void SwingStructureDialog::updateUIFromInstance()
{
    m_highMoveSpinBox->setValue(m_currentIndicator.highMove);
    m_lowMoveSpinBox->setValue(m_currentIndicator.lowMove);
    m_minPeriodsSpinBox->setValue(m_currentIndicator.minPeriods);
    m_maxPeriodsSpinBox->setValue(m_currentIndicator.maxPeriods);
    m_resetOnNewDayCheckBox->setCheckState(m_currentIndicator.resetOnNewDay ? Qt::Checked : Qt::Unchecked);
    updateColorButtonStyle(m_swingHighColorButton, m_currentIndicator.swingHighColor);
    updateColorButtonStyle(m_swingLowColorButton, m_currentIndicator.swingLowColor);
    updateColorButtonStyle(m_upColorButton, m_currentIndicator.upColor);
    updateColorButtonStyle(m_downColorButton, m_currentIndicator.downColor);
    updateColorButtonStyle(m_uncertainColorButton, m_currentIndicator.uncertainColor);
}

void SwingStructureDialog::onHighMoveChanged(double highMove)
{
    m_currentIndicator.highMove = highMove;
    applyChanges();
}

void SwingStructureDialog::onLowMoveChanged(double lowMove)
{
    m_currentIndicator.lowMove = lowMove;
    applyChanges();
}

void SwingStructureDialog::onMinPeriodsChanged(int minPeriods)
{
    m_currentIndicator.minPeriods = minPeriods;
    applyChanges();
}

void SwingStructureDialog::onMaxPeriodsChanged(int maxPeriods)
{
    m_currentIndicator.maxPeriods = maxPeriods;
    applyChanges();
}

void SwingStructureDialog::onResetOnNewDayChanged(int state)
{
    m_currentIndicator.resetOnNewDay = (state == Qt::Checked);
    applyChanges();
}

void SwingStructureDialog::onSwingHighColorButtonClicked()
{
    QColor color = openColorDialog(m_currentIndicator.swingHighColor, "Select Swing High Color");

    if (color.isValid()) {
        m_currentIndicator.swingHighColor = colorFromRGB(color.red(), color.green(), color.blue());
        updateColorButtonStyle(m_swingHighColorButton, m_currentIndicator.swingHighColor);
        applyChanges();
    }
}

void SwingStructureDialog::onSwingLowColorButtonClicked()
{
    QColor color = openColorDialog(m_currentIndicator.swingLowColor, "Select Swing Low Color");

    if (color.isValid()) {
        m_currentIndicator.swingLowColor = colorFromRGB(color.red(), color.green(), color.blue());
        updateColorButtonStyle(m_swingLowColorButton, m_currentIndicator.swingLowColor);
        applyChanges();
    }
}

void SwingStructureDialog::onUpColorButtonClicked()
{
    QColor color = openColorDialog(m_currentIndicator.upColor, "Select Up Trend Color");

    if (color.isValid()) {
        m_currentIndicator.upColor = colorFromRGB(color.red(), color.green(), color.blue());
        updateColorButtonStyle(m_upColorButton, m_currentIndicator.upColor);
        applyChanges();
    }
}

void SwingStructureDialog::onDownColorButtonClicked()
{
    QColor color = openColorDialog(m_currentIndicator.downColor, "Select Down Trend Color");

    if (color.isValid()) {
        m_currentIndicator.downColor = colorFromRGB(color.red(), color.green(), color.blue());
        updateColorButtonStyle(m_downColorButton, m_currentIndicator.downColor);
        applyChanges();
    }
}

void SwingStructureDialog::onUncertainColorButtonClicked()
{
    QColor color = openColorDialog(m_currentIndicator.uncertainColor, "Select Uncertain Trend Color");

    if (color.isValid()) {
        m_currentIndicator.uncertainColor = colorFromRGB(color.red(), color.green(), color.blue());
        updateColorButtonStyle(m_uncertainColorButton, m_currentIndicator.uncertainColor);
        applyChanges();
    }
}
