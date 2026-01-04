#include "ui/dialogs/indicators/macdDialog.h"

MACDDialog::MACDDialog(QWidget* parent, ChartWidget* chartWidget, const indicators::MACDInstance& macd)
    : IndicatorDialog<indicators::MACDInstance>(parent, "MACD", chartWidget, macd)
{
    initialize();
}

MACDDialog::~MACDDialog()
{
}

void MACDDialog::setupUI()
{
    // Fast period
    m_fastPeriodSpinBox = new QSpinBox();
    m_fastPeriodSpinBox->setRange(1, 1000);
    m_formLayout->addRow("Fast Period:", m_fastPeriodSpinBox);

    // Slow period
    m_slowPeriodSpinBox = new QSpinBox();
    m_slowPeriodSpinBox->setRange(2, 1000);
    m_formLayout->addRow("Slow Period:", m_slowPeriodSpinBox);

    // Signal period
    m_signalPeriodSpinBox = new QSpinBox();
    m_signalPeriodSpinBox->setRange(1, 1000);
    m_formLayout->addRow("Signal Period:", m_signalPeriodSpinBox);

    // Signal smoothing (0 means "use signal period")
    m_signalSmoothingSpinBox = new QSpinBox();
    m_signalSmoothingSpinBox->setRange(0, 1000);
    m_formLayout->addRow("Signal Smoothing (0 = use signal):", m_signalSmoothingSpinBox);

    // Source field
    m_sourceComboBox = new QComboBox();
    m_sourceComboBox->addItems({ "Open", "High", "Low", "Close" });
    m_formLayout->addRow("Source:", m_sourceComboBox);

    // MA types
    m_oscMATypeComboBox = new QComboBox();
    m_oscMATypeComboBox->addItems({ "EMA", "SMA" });
    m_formLayout->addRow("Oscillator MA Type:", m_oscMATypeComboBox);

    m_signalMATypeComboBox = new QComboBox();
    m_signalMATypeComboBox->addItems({ "EMA", "SMA" });
    m_formLayout->addRow("Signal MA Type:", m_signalMATypeComboBox);

    // Checkbox for resetting on new day
    m_resetOnNewDayCheckBox = new QCheckBox();
    m_formLayout->addRow("Reset on New Day:", m_resetOnNewDayCheckBox);

    // Color buttons
    m_macdColorButton = new QPushButton();
    m_formLayout->addRow("MACD Line Color:", m_macdColorButton);

    m_signalColorButton = new QPushButton();
    m_formLayout->addRow("Signal Line Color:", m_signalColorButton);

    m_histColorButton = new QPushButton();
    m_formLayout->addRow("Histogram Color:", m_histColorButton);
}

void MACDDialog::connectSignals()
{
    connect(m_fastPeriodSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &MACDDialog::onFastPeriodChanged);
    connect(m_slowPeriodSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &MACDDialog::onSlowPeriodChanged);
    connect(m_signalPeriodSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &MACDDialog::onSignalPeriodChanged);
    connect(m_signalSmoothingSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &MACDDialog::onSignalSmoothingChanged);

    connect(m_sourceComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MACDDialog::onSourceChanged);
    connect(m_oscMATypeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MACDDialog::onOscMATypeChanged);
    connect(m_signalMATypeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MACDDialog::onSignalMATypeChanged);

    connect(m_resetOnNewDayCheckBox, &QCheckBox::checkStateChanged, this, &MACDDialog::onResetOnNewDayChanged);

    connect(m_macdColorButton, &QPushButton::clicked, this, &MACDDialog::onMacdColorButtonClicked);
    connect(m_signalColorButton, &QPushButton::clicked, this, &MACDDialog::onSignalColorButtonClicked);
    connect(m_histColorButton, &QPushButton::clicked, this, &MACDDialog::onHistColorButtonClicked);
}

void MACDDialog::updateUIFromInstance()
{
    m_fastPeriodSpinBox->setValue(m_currentIndicator.fastPeriod);
    m_slowPeriodSpinBox->setValue(m_currentIndicator.slowPeriod);
    m_signalPeriodSpinBox->setValue(m_currentIndicator.signalPeriod);
    m_signalSmoothingSpinBox->setValue(m_currentIndicator.signal_smoothing);
    m_resetOnNewDayCheckBox->setCheckState(m_currentIndicator.resetOnNewDay ? Qt::Checked : Qt::Unchecked);

    switch (m_currentIndicator.source) {
        case filter::PriceType::OPEN: m_sourceComboBox->setCurrentIndex(0); break;
        case filter::PriceType::HIGH: m_sourceComboBox->setCurrentIndex(1); break;
        case filter::PriceType::LOW: m_sourceComboBox->setCurrentIndex(2); break;
        case filter::PriceType::CLOSE: m_sourceComboBox->setCurrentIndex(3); break;
        default: m_sourceComboBox->setCurrentIndex(3); break; // Default to Close
    }

    m_oscMATypeComboBox->setCurrentIndex(m_currentIndicator.osc_ma_type == filter::MAType::SMA ? 1 : 0); // EMA=0, SMA=1
    m_signalMATypeComboBox->setCurrentIndex(m_currentIndicator.signal_ma_type == filter::MAType::SMA ? 1 : 0); // EMA=0, SMA=1

    updateColorButtonStyle(m_macdColorButton, m_currentIndicator.macdColor);
    updateColorButtonStyle(m_signalColorButton, m_currentIndicator.signalColor);
    updateColorButtonStyle(m_histColorButton, m_currentIndicator.histogramColor);
}

void MACDDialog::onFastPeriodChanged(int period)
{
    m_currentIndicator.fastPeriod = period;
    applyChanges();
}

void MACDDialog::onSlowPeriodChanged(int period)
{
    m_currentIndicator.slowPeriod = period;
    applyChanges();
}

void MACDDialog::onSignalPeriodChanged(int period)
{
    m_currentIndicator.signalPeriod = period;
    applyChanges();
}

void MACDDialog::onSignalSmoothingChanged(int period)
{
    m_currentIndicator.signal_smoothing = period;
    applyChanges();
}

void MACDDialog::onSourceChanged(int index)
{
    switch (index) {
        case 0: m_currentIndicator.source = filter::PriceType::OPEN; break;
        case 1: m_currentIndicator.source = filter::PriceType::HIGH; break;
        case 2: m_currentIndicator.source = filter::PriceType::LOW; break;
        case 3: m_currentIndicator.source = filter::PriceType::CLOSE; break;
        default: m_currentIndicator.source = filter::PriceType::CLOSE; break;
    }
    applyChanges();
}

void MACDDialog::onOscMATypeChanged(int index)
{
    m_currentIndicator.osc_ma_type = (index == 0) ? filter::MAType::EMA : filter::MAType::SMA;
    applyChanges();
}

void MACDDialog::onSignalMATypeChanged(int index)
{
    m_currentIndicator.signal_ma_type = (index == 0) ? filter::MAType::EMA : filter::MAType::SMA;
    applyChanges();
}

void MACDDialog::onResetOnNewDayChanged(int state)
{
    m_currentIndicator.resetOnNewDay = (state == Qt::Checked);
    applyChanges();
}

void MACDDialog::onMacdColorButtonClicked()
{
    QColor color = openColorDialog(m_currentIndicator.macdColor, "Select MACD Line Color");
    if (color.isValid()) {
        m_currentIndicator.macdColor = colorFromRGB(color.red(), color.green(), color.blue());
        updateColorButtonStyle(m_macdColorButton, m_currentIndicator.macdColor);
        applyChanges();
    }
}

void MACDDialog::onSignalColorButtonClicked()
{
    QColor color = openColorDialog(m_currentIndicator.signalColor, "Select Signal Line Color");
    if (color.isValid()) {
        m_currentIndicator.signalColor = colorFromRGB(color.red(), color.green(), color.blue());
        updateColorButtonStyle(m_signalColorButton, m_currentIndicator.signalColor);
        applyChanges();
    }
}

void MACDDialog::onHistColorButtonClicked()
{
    QColor color = openColorDialog(m_currentIndicator.histogramColor, "Select Histogram Color");
    if (color.isValid()) {
        m_currentIndicator.histogramColor = colorFromRGB(color.red(), color.green(), color.blue());
        updateColorButtonStyle(m_histColorButton, m_currentIndicator.histogramColor);
        applyChanges();
    }
}