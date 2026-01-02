#include "ui/dialogs/FilterEditDialog.h"
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QGroupBox>
#include <QLineEdit>

FilterEditDialog::FilterEditDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Filter Edit");
    resize(800, 500);  // Larger for horizontal layout

    setupUI();

    // Force initial refresh to display widgets correctly
    onLeftValueCategoryChanged(m_leftCategoryCombo->currentIndex());
    onLeftIndicatorTypeChanged(m_leftIndicatorTypeCombo->currentIndex());
    onRightValueCategoryChanged(m_rightCategoryCombo->currentIndex());
    onRightIndicatorTypeChanged(m_rightIndicatorTypeCombo->currentIndex());
    updateDistanceVisibility();

    updatePreview();
}

void FilterEditDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Main horizontal layout: Left | Operator | Right
    QHBoxLayout* horizontalLayout = new QHBoxLayout();

    // Group for the left part
    QGroupBox* leftGroup = new QGroupBox("Left Value", this);
    setupLeftValueUI(leftGroup);
    horizontalLayout->addWidget(leftGroup, 3);  // Ratio 3

    // Group for the operator (center)
    QGroupBox* operatorGroup = new QGroupBox("Operator", this);
    setupOperatorUI(operatorGroup);
    horizontalLayout->addWidget(operatorGroup, 1);  // Ratio 1

    // Group for the right part
    m_rightGroup = new QGroupBox("Right Value", this);
    setupRightValueUI(m_rightGroup);
    horizontalLayout->addWidget(m_rightGroup, 3);  // Ratio 3

    // Add an invisible placeholder in place of the right group
    m_rightPlaceholder = new QWidget(this);
    m_rightPlaceholder->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_rightPlaceholder->setVisible(false);
    horizontalLayout->addWidget(m_rightPlaceholder, 3); // Same ratio as rightGroup

    mainLayout->addLayout(horizontalLayout);

    // Group for temporal logic (bottom)
    QGroupBox* temporalGroup = new QGroupBox("Temporal Logic", this);
    setupTemporalLogicUI(temporalGroup);
    mainLayout->addWidget(temporalGroup);

    // Filter preview
    QGroupBox* previewGroup = new QGroupBox("Filter Preview", this);
    QVBoxLayout* previewLayout = new QVBoxLayout(previewGroup);
    m_previewLabel = new QLabel(previewGroup);
    m_previewLabel->setWordWrap(true);
    m_previewLabel->setStyleSheet("font-weight: bold;");
    previewLayout->addWidget(m_previewLabel);
    mainLayout->addWidget(previewGroup);

    // OK/Cancel buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    QPushButton* cancelButton = new QPushButton("Cancel", this);
    QPushButton* okButton = new QPushButton("OK", this);
    okButton->setDefault(true);
    buttonLayout->addStretch();
    buttonLayout->addWidget(cancelButton);
    buttonLayout->addWidget(okButton);
    mainLayout->addLayout(buttonLayout);

    connect(okButton, &QPushButton::clicked, this, &FilterEditDialog::onOkButtonClicked);
    connect(cancelButton, &QPushButton::clicked, this, &FilterEditDialog::onCancelButtonClicked);

    // Connections to update preview
    connect(m_leftCategoryCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &FilterEditDialog::onLeftValueCategoryChanged);
    connect(m_leftIndicatorTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &FilterEditDialog::onLeftIndicatorTypeChanged);
    connect(m_rightCategoryCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &FilterEditDialog::onRightValueCategoryChanged);
    connect(m_rightIndicatorTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &FilterEditDialog::onRightIndicatorTypeChanged);
    connect(m_operatorCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &FilterEditDialog::onComparisonOpChanged);
    connect(m_lookbackPeriodsSpin, QOverload<int>::of(&QSpinBox::valueChanged),
        this, &FilterEditDialog::onLookbackPeriodsChanged);


    // Connect all widgets to updatePreview()
    connect(m_leftPriceTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &FilterEditDialog::updatePreview);
    connect(m_leftCandlePropertyCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &FilterEditDialog::updatePreview);
    connect(m_leftHistoricalOffsetSpin, QOverload<int>::of(&QSpinBox::valueChanged), 
            this, &FilterEditDialog::updatePreview);
    connect(m_leftEMAPeriodSpin, QOverload<int>::of(&QSpinBox::valueChanged), 
            this, &FilterEditDialog::updatePreview);
    connect(m_leftRSIPeriodSpin, QOverload<int>::of(&QSpinBox::valueChanged), 
            this, &FilterEditDialog::updatePreview);
    connect(m_leftStochFastKSpin, QOverload<int>::of(&QSpinBox::valueChanged), 
            this, &FilterEditDialog::updatePreview);
    connect(m_leftStochSlowKSpin, QOverload<int>::of(&QSpinBox::valueChanged), 
            this, &FilterEditDialog::updatePreview);
    connect(m_leftStochSlowDSpin, QOverload<int>::of(&QSpinBox::valueChanged), 
            this, &FilterEditDialog::updatePreview);
    connect(m_leftATRPeriodSpin, QOverload<int>::of(&QSpinBox::valueChanged), 
            this, &FilterEditDialog::updatePreview);
    connect(m_leftATRUseLogCheck, &QCheckBox::toggled, 
            this, &FilterEditDialog::updatePreview);
    connect(m_leftSuperTrendPeriodSpin, QOverload<int>::of(&QSpinBox::valueChanged), 
            this, &FilterEditDialog::updatePreview);
    connect(m_leftSuperTrendMultiplierSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), 
            this, &FilterEditDialog::updatePreview);
    connect(m_leftCCIPeriodSpin, QOverload<int>::of(&QSpinBox::valueChanged), 
            this, &FilterEditDialog::updatePreview);
    connect(m_leftCCIOverboughtSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), 
            this, &FilterEditDialog::updatePreview);
    connect(m_leftCCIOversoldSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), 
            this, &FilterEditDialog::updatePreview);
    connect(m_leftMACDFastPeriodSpin, QOverload<int>::of(&QSpinBox::valueChanged), 
            this, &FilterEditDialog::updatePreview);
    connect(m_leftMACDSlowPeriodSpin, QOverload<int>::of(&QSpinBox::valueChanged), 
            this, &FilterEditDialog::updatePreview);
    connect(m_leftMACDSignalPeriodSpin, QOverload<int>::of(&QSpinBox::valueChanged), 
            this, &FilterEditDialog::updatePreview);
    connect(m_leftBBPeriodSpin, QOverload<int>::of(&QSpinBox::valueChanged), 
            this, &FilterEditDialog::updatePreview);
    connect(m_leftBBMATypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &FilterEditDialog::updatePreview);
    connect(m_leftBBSourceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &FilterEditDialog::updatePreview);
    connect(m_leftBBStdDevMultiplierSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), 
            this, &FilterEditDialog::updatePreview);
    // Left transform widgets
    if (m_leftTransformCombo)
        connect(m_leftTransformCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &FilterEditDialog::updatePreview);

    connect(m_rightPriceTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &FilterEditDialog::updatePreview);
    connect(m_rightCandlePropertyCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &FilterEditDialog::updatePreview);
    connect(m_rightConstantValueSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), 
            this, &FilterEditDialog::updatePreview);
    connect(m_rightHistoricalOffsetSpin, QOverload<int>::of(&QSpinBox::valueChanged), 
            this, &FilterEditDialog::updatePreview);
    connect(m_rightEMAPeriodSpin, QOverload<int>::of(&QSpinBox::valueChanged), 
            this, &FilterEditDialog::updatePreview);
    connect(m_rightRSIPeriodSpin, QOverload<int>::of(&QSpinBox::valueChanged), 
            this, &FilterEditDialog::updatePreview);
    connect(m_rightStochFastKSpin, QOverload<int>::of(&QSpinBox::valueChanged), 
            this, &FilterEditDialog::updatePreview);
    connect(m_rightStochSlowKSpin, QOverload<int>::of(&QSpinBox::valueChanged), 
            this, &FilterEditDialog::updatePreview);
    connect(m_rightStochSlowDSpin, QOverload<int>::of(&QSpinBox::valueChanged), 
            this, &FilterEditDialog::updatePreview);
    connect(m_rightATRPeriodSpin, QOverload<int>::of(&QSpinBox::valueChanged), 
            this, &FilterEditDialog::updatePreview);
    connect(m_rightATRUseLogCheck, &QCheckBox::toggled, 
            this, &FilterEditDialog::updatePreview);
    connect(m_rightSuperTrendPeriodSpin, QOverload<int>::of(&QSpinBox::valueChanged), 
            this, &FilterEditDialog::updatePreview);
    connect(m_rightSuperTrendMultiplierSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), 
            this, &FilterEditDialog::updatePreview);
    connect(m_rightCCIPeriodSpin, QOverload<int>::of(&QSpinBox::valueChanged), 
            this, &FilterEditDialog::updatePreview);
    connect(m_rightCCIOverboughtSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), 
            this, &FilterEditDialog::updatePreview);
    connect(m_rightCCIOversoldSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), 
            this, &FilterEditDialog::updatePreview);
    connect(m_rightMACDFastPeriodSpin, QOverload<int>::of(&QSpinBox::valueChanged), 
            this, &FilterEditDialog::updatePreview);
    connect(m_rightMACDSlowPeriodSpin, QOverload<int>::of(&QSpinBox::valueChanged), 
            this, &FilterEditDialog::updatePreview);
    connect(m_rightMACDSignalPeriodSpin, QOverload<int>::of(&QSpinBox::valueChanged), 
            this, &FilterEditDialog::updatePreview); 
    connect(m_rightBBPeriodSpin, QOverload<int>::of(&QSpinBox::valueChanged), 
            this, &FilterEditDialog::updatePreview);
    connect(m_rightBBMATypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &FilterEditDialog::updatePreview);
    connect(m_rightBBSourceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &FilterEditDialog::updatePreview);
    connect(m_rightBBStdDevMultiplierSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), 
            this, &FilterEditDialog::updatePreview);

    // Distance spin connection
    connect(m_distanceSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &FilterEditDialog::updatePreview);

    connect(m_operatorCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &FilterEditDialog::updatePreview);
    // Right transform widgets
    if (m_rightTransformCombo)
        connect(m_rightTransformCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &FilterEditDialog::updatePreview);
    connect(m_temporalLogicCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &FilterEditDialog::updatePreview);
    connect(m_lookbackPeriodsSpin, QOverload<int>::of(&QSpinBox::valueChanged), 
            this, &FilterEditDialog::updatePreview);
}

void FilterEditDialog::setupLeftValueUI(QWidget* parent)
{
    QVBoxLayout* leftLayout = new QVBoxLayout(parent);

    // Category type (Price, Indicator, Candle property, etc.)
    QFormLayout* categoryLayout = new QFormLayout();
    m_leftCategoryCombo = new QComboBox(parent);
    m_leftCategoryCombo->addItem("Price", static_cast<int>(filter::ValueCategory::PRICE));
    m_leftCategoryCombo->addItem("Indicator", static_cast<int>(filter::ValueCategory::INDICATOR));
    m_leftCategoryCombo->addItem("Candle property", static_cast<int>(filter::ValueCategory::CANDLE_PROPERTY));
    categoryLayout->addRow("Category:", m_leftCategoryCombo);

    // Historical offset common to all categories
    m_leftHistoricalOffsetSpin = new QSpinBox(parent);
    m_leftHistoricalOffsetSpin->setRange(0, 200);
    m_leftHistoricalOffsetSpin->setValue(0);
    m_leftHistoricalOffsetSpin->setSuffix(" bars");
    categoryLayout->addRow("Historical offset:", m_leftHistoricalOffsetSpin);

    leftLayout->addLayout(categoryLayout);

    // Widgets specific to each category type

    // 1. Price
    m_leftPriceWidget = new QWidget(parent);
    QFormLayout* priceLayout = new QFormLayout(m_leftPriceWidget);
    m_leftPriceTypeCombo = new QComboBox(m_leftPriceWidget);
    m_leftPriceTypeCombo->addItem("Close", static_cast<int>(filter::PriceType::CLOSE));
    m_leftPriceTypeCombo->addItem("Open", static_cast<int>(filter::PriceType::OPEN));
    m_leftPriceTypeCombo->addItem("High", static_cast<int>(filter::PriceType::HIGH));
    m_leftPriceTypeCombo->addItem("Low", static_cast<int>(filter::PriceType::LOW));
    m_leftPriceTypeCombo->addItem("Typical", static_cast<int>(filter::PriceType::TYPICAL));
    m_leftPriceTypeCombo->addItem("Median", static_cast<int>(filter::PriceType::MEDIAN));
    priceLayout->addRow("Price type:", m_leftPriceTypeCombo);
    m_leftPriceWidget->setVisible(false);
    leftLayout->addWidget(m_leftPriceWidget);

    // 2. Indicator
    m_leftIndicatorWidget = new QWidget(parent);
    QVBoxLayout* indicatorLayout = new QVBoxLayout(m_leftIndicatorWidget);

    // Indicator type
    QFormLayout* indicatorTypeLayout = new QFormLayout();
    m_leftIndicatorTypeCombo = new QComboBox(m_leftIndicatorWidget);
    m_leftIndicatorTypeCombo->addItem("EMA", static_cast<int>(filter::IndicatorType::EMA));
    m_leftIndicatorTypeCombo->addItem("RSI", static_cast<int>(filter::IndicatorType::RSI));
    m_leftIndicatorTypeCombo->addItem("Stochastic K", static_cast<int>(filter::IndicatorType::STOCHASTIC_K));
    m_leftIndicatorTypeCombo->addItem("Stochastic D", static_cast<int>(filter::IndicatorType::STOCHASTIC_D));
    m_leftIndicatorTypeCombo->addItem("ATR", static_cast<int>(filter::IndicatorType::ATR));
    m_leftIndicatorTypeCombo->addItem("SuperTrend Value", static_cast<int>(filter::IndicatorType::SUPERTREND_VALUE));
    m_leftIndicatorTypeCombo->addItem("SuperTrend Direction", static_cast<int>(filter::IndicatorType::SUPERTREND_DIRECTION));
    m_leftIndicatorTypeCombo->addItem("CCI", static_cast<int>(filter::IndicatorType::CCI));
    m_leftIndicatorTypeCombo->addItem("MACD Histogram", static_cast<int>(filter::IndicatorType::MACD_HISTOGRAM));
    m_leftIndicatorTypeCombo->addItem("MACD Line (MACD)", static_cast<int>(filter::IndicatorType::MACD_LINE));
    m_leftIndicatorTypeCombo->addItem("MACD Line (Signal)", static_cast<int>(filter::IndicatorType::MACD_SIGNAL));
    m_leftIndicatorTypeCombo->addItem("Bollinger Bands - Upper Band", static_cast<int>(filter::IndicatorType::BB_UPPER));
    m_leftIndicatorTypeCombo->addItem("Bollinger Bands - Lower Band", static_cast<int>(filter::IndicatorType::BB_LOWER));
    m_leftIndicatorTypeCombo->addItem("Bollinger Bands - Percent B", static_cast<int>(filter::IndicatorType::BB_PERCENT_B));
    indicatorTypeLayout->addRow("Indicator type:", m_leftIndicatorTypeCombo);
    indicatorLayout->addLayout(indicatorTypeLayout);

    // Transform selection for left indicator
    QFormLayout* transformLayout = new QFormLayout();
    m_leftTransformCombo = new QComboBox(m_leftIndicatorWidget);
    m_leftTransformCombo->addItem("None", static_cast<int>(filter::TransformType::NONE));
    m_leftTransformCombo->addItem("Logarithm", static_cast<int>(filter::TransformType::LOG));
    m_leftTransformCombo->addItem("Exponential", static_cast<int>(filter::TransformType::EXP));
    m_leftTransformCombo->addItem("Derivative (diff)", static_cast<int>(filter::TransformType::DERIVATIVE));
    transformLayout->addRow("Transform:", m_leftTransformCombo);
    indicatorLayout->addLayout(transformLayout);

    // Indicator-specific parameters

    // EMA
    m_leftEMAWidget = new QWidget(m_leftIndicatorWidget);
    QFormLayout* emaLayout = new QFormLayout(m_leftEMAWidget);
    m_leftEMAPeriodSpin = new QSpinBox(m_leftEMAWidget);
    m_leftEMAPeriodSpin->setRange(1, 3000);
    m_leftEMAPeriodSpin->setValue(20);
    emaLayout->addRow("Period:", m_leftEMAPeriodSpin);
    m_leftEMAWidget->setVisible(false);
    indicatorLayout->addWidget(m_leftEMAWidget);

    // RSI
    m_leftRSIWidget = new QWidget(m_leftIndicatorWidget);
    QFormLayout* rsiLayout = new QFormLayout(m_leftRSIWidget);
    m_leftRSIPeriodSpin = new QSpinBox(m_leftRSIWidget);
    m_leftRSIPeriodSpin->setRange(1, 1000);
    m_leftRSIPeriodSpin->setValue(14);
    rsiLayout->addRow("Period:", m_leftRSIPeriodSpin);
    m_leftRSIWidget->setVisible(false);
    indicatorLayout->addWidget(m_leftRSIWidget);

    // Stochastic
    m_leftStochasticWidget = new QWidget(m_leftIndicatorWidget);
    QFormLayout* stochLayout = new QFormLayout(m_leftStochasticWidget);
    m_leftStochFastKSpin = new QSpinBox(m_leftStochasticWidget);
    m_leftStochFastKSpin->setRange(1, 1000);
    m_leftStochFastKSpin->setValue(14);
    m_leftStochSlowKSpin = new QSpinBox(m_leftStochasticWidget);
    m_leftStochSlowKSpin->setRange(1, 1000);
    m_leftStochSlowKSpin->setValue(3);
    m_leftStochSlowDSpin = new QSpinBox(m_leftStochasticWidget);
    m_leftStochSlowDSpin->setRange(1, 1000);
    m_leftStochSlowDSpin->setValue(3);
    stochLayout->addRow("FastK period:", m_leftStochFastKSpin);
    stochLayout->addRow("SlowK period:", m_leftStochSlowKSpin);
    stochLayout->addRow("SlowD period:", m_leftStochSlowDSpin);
    m_leftStochasticWidget->setVisible(false);
    indicatorLayout->addWidget(m_leftStochasticWidget);

    // ATR
    m_leftATRWidget = new QWidget(m_leftIndicatorWidget);
    QFormLayout* atrLayout = new QFormLayout(m_leftATRWidget);
    m_leftATRPeriodSpin = new QSpinBox(m_leftATRWidget);
    m_leftATRPeriodSpin->setRange(1, 1000);
    m_leftATRPeriodSpin->setValue(14);
    m_leftATRUseLogCheck = new QCheckBox("Use logarithmic ATR", m_leftATRWidget);
    atrLayout->addRow("Period:", m_leftATRPeriodSpin);
    atrLayout->addRow(m_leftATRUseLogCheck);
    m_leftATRWidget->setVisible(false);
    indicatorLayout->addWidget(m_leftATRWidget);

    // SuperTrend
    m_leftSuperTrendWidget = new QWidget(m_leftIndicatorWidget);
    QFormLayout* stLayout = new QFormLayout(m_leftSuperTrendWidget);
    m_leftSuperTrendPeriodSpin = new QSpinBox(m_leftSuperTrendWidget);
    m_leftSuperTrendPeriodSpin->setRange(1, 1000);
    m_leftSuperTrendPeriodSpin->setValue(10);
    m_leftSuperTrendMultiplierSpin = new QDoubleSpinBox(m_leftSuperTrendWidget);
    m_leftSuperTrendMultiplierSpin->setRange(0.1, 1000.0);
    m_leftSuperTrendMultiplierSpin->setSingleStep(1);
    m_leftSuperTrendMultiplierSpin->setValue(3.0);
    stLayout->addRow("ATR Period:", m_leftSuperTrendPeriodSpin);
    stLayout->addRow("Multiplier:", m_leftSuperTrendMultiplierSpin);
    m_leftSuperTrendWidget->setVisible(false);
    indicatorLayout->addWidget(m_leftSuperTrendWidget);

    // CCI
    m_leftCCIWidget = new QWidget(m_leftIndicatorWidget);
    QFormLayout* cciLeftLayout = new QFormLayout(m_leftCCIWidget);
    m_leftCCIPeriodSpin = new QSpinBox(m_leftCCIWidget);
    m_leftCCIPeriodSpin->setRange(1, 1000);
    m_leftCCIPeriodSpin->setValue(20);
    cciLeftLayout->addRow("Period:", m_leftCCIPeriodSpin);
    // Overbought/oversold values for CCI (used in preview and settings)
    m_leftCCIOverboughtSpin = new QDoubleSpinBox(m_leftCCIWidget);
    m_leftCCIOverboughtSpin->setRange(-10000.0, 10000.0);
    m_leftCCIOverboughtSpin->setDecimals(2);
    m_leftCCIOverboughtSpin->setValue(100.0);
    m_leftCCIOversoldSpin = new QDoubleSpinBox(m_leftCCIWidget);
    m_leftCCIOversoldSpin->setRange(-10000.0, 10000.0);
    m_leftCCIOversoldSpin->setDecimals(2);
    m_leftCCIOversoldSpin->setValue(-100.0);
    cciLeftLayout->addRow("Overbought:", m_leftCCIOverboughtSpin);
    cciLeftLayout->addRow("Oversold:", m_leftCCIOversoldSpin);
    m_leftCCIWidget->setVisible(false);
    indicatorLayout->addWidget(m_leftCCIWidget);

    m_leftIndicatorWidget->setVisible(false);
    leftLayout->addWidget(m_leftIndicatorWidget);

    // MACD
    m_leftMACDWidget = new QWidget(m_leftIndicatorWidget);
    QFormLayout* macdLayout = new QFormLayout(m_leftMACDWidget);
    m_leftMACDFastPeriodSpin = new QSpinBox(m_leftMACDWidget);
    m_leftMACDFastPeriodSpin->setRange(1, 1000);
    m_leftMACDFastPeriodSpin->setValue(12);
    m_leftMACDSlowPeriodSpin = new QSpinBox(m_leftMACDWidget);
    m_leftMACDSlowPeriodSpin->setRange(1, 1000);
    m_leftMACDSlowPeriodSpin->setValue(26);
    m_leftMACDSignalPeriodSpin = new QSpinBox(m_leftMACDWidget);
    m_leftMACDSignalPeriodSpin->setRange(1, 1000);
    m_leftMACDSignalPeriodSpin->setRange(1, 1000);
    m_leftMACDSignalPeriodSpin->setValue(9);
    macdLayout->addRow("Fast period:", m_leftMACDFastPeriodSpin);
    macdLayout->addRow("Slow period:", m_leftMACDSlowPeriodSpin);
    macdLayout->addRow("Signal period:", m_leftMACDSignalPeriodSpin);
    m_leftMACDWidget->setVisible(false);
    indicatorLayout->addWidget(m_leftMACDWidget);

    // Bollinger Bands
    m_leftBBWidget = new QWidget(m_leftIndicatorWidget);
    QFormLayout* bbLayout = new QFormLayout(m_leftBBWidget);
    m_leftBBPeriodSpin = new QSpinBox(m_leftBBWidget);
    m_leftBBPeriodSpin->setRange(1, 1000);
    m_leftBBPeriodSpin->setValue(20);
    m_leftBBMATypeCombo = new QComboBox(m_leftBBWidget);
    m_leftBBMATypeCombo->addItem("SMA", static_cast<int>(filter::MAType::SMA));
    m_leftBBMATypeCombo->addItem("EMA", static_cast<int>(filter::MAType::EMA));
    m_leftBBSourceCombo = new QComboBox(m_leftBBWidget);
    m_leftBBSourceCombo->addItem("Close", static_cast<int>(filter::PriceType::CLOSE));
    m_leftBBSourceCombo->addItem("Open", static_cast<int>(filter::PriceType::OPEN));
    m_leftBBSourceCombo->addItem("High", static_cast<int>(filter::PriceType::HIGH));
    m_leftBBSourceCombo->addItem("Low", static_cast<int>(filter::PriceType::LOW));
    m_leftBBSourceCombo->addItem("Typical", static_cast<int>(filter::PriceType::TYPICAL));
    m_leftBBSourceCombo->addItem("Median", static_cast<int>(filter::PriceType::MEDIAN));
    m_leftBBStdDevMultiplierSpin = new QDoubleSpinBox(m_leftBBWidget);
    m_leftBBStdDevMultiplierSpin->setRange(0.1, 10.0);
    m_leftBBStdDevMultiplierSpin->setSingleStep(0.1);
    m_leftBBStdDevMultiplierSpin->setValue(2.0);
    bbLayout->addRow("Period:", m_leftBBPeriodSpin);
    bbLayout->addRow("MA Type:", m_leftBBMATypeCombo);
    bbLayout->addRow("Source:", m_leftBBSourceCombo);
    bbLayout->addRow("Stddev multiplier:", m_leftBBStdDevMultiplierSpin);
    m_leftBBWidget->setVisible(false);
    indicatorLayout->addWidget(m_leftBBWidget); 

    // 3. Candle property
    m_leftCandlePropertyWidget = new QWidget(parent);
    QFormLayout* candleLayout = new QFormLayout(m_leftCandlePropertyWidget);
    m_leftCandlePropertyCombo = new QComboBox(m_leftCandlePropertyWidget);
    m_leftCandlePropertyCombo->addItem("Green candle (Heikin Ashi)", static_cast<int>(filter::CandlePropertyType::HEIKIN_ASHI_IS_GREEN));
    m_leftCandlePropertyCombo->addItem("Red candle (Heikin Ashi)", static_cast<int>(filter::CandlePropertyType::HEIKIN_ASHI_IS_RED));
    m_leftCandlePropertyCombo->addItem("Green candle", static_cast<int>(filter::CandlePropertyType::IS_GREEN));
    m_leftCandlePropertyCombo->addItem("Red candle", static_cast<int>(filter::CandlePropertyType::IS_RED));
    m_leftCandlePropertyCombo->addItem("Body size", static_cast<int>(filter::CandlePropertyType::BODY_SIZE));
    m_leftCandlePropertyCombo->addItem("Upper shadow size", static_cast<int>(filter::CandlePropertyType::UPPER_SHADOW_SIZE));
    m_leftCandlePropertyCombo->addItem("Lower shadow size", static_cast<int>(filter::CandlePropertyType::LOWER_SHADOW_SIZE));
    m_leftCandlePropertyCombo->addItem("Range", static_cast<int>(filter::CandlePropertyType::RANGE));
    candleLayout->addRow("Property:", m_leftCandlePropertyCombo);
    m_leftCandlePropertyWidget->setVisible(false);
    leftLayout->addWidget(m_leftCandlePropertyWidget);
}

void FilterEditDialog::setupRightValueUI(QWidget* parent)
{
    QVBoxLayout* rightLayout = new QVBoxLayout(parent);

    // Category type (Price, Indicator, Constant, Candle property, etc.)
    QFormLayout* categoryLayout = new QFormLayout();
    m_rightCategoryCombo = new QComboBox(parent);
    m_rightCategoryCombo->addItem("Price", static_cast<int>(filter::ValueCategory::PRICE));
    m_rightCategoryCombo->addItem("Indicator", static_cast<int>(filter::ValueCategory::INDICATOR));
    m_rightCategoryCombo->addItem("Constant", static_cast<int>(filter::ValueCategory::CONSTANT));
    m_rightCategoryCombo->addItem("Candle property", static_cast<int>(filter::ValueCategory::CANDLE_PROPERTY));
    categoryLayout->addRow("Category:", m_rightCategoryCombo);

    // Historical offset common to all categories except Constant
    m_rightHistoricalOffsetSpin = new QSpinBox(parent);
    m_rightHistoricalOffsetSpin->setRange(0, 100);
    m_rightHistoricalOffsetSpin->setValue(0);
    m_rightHistoricalOffsetSpin->setSuffix(" bars");
    categoryLayout->addRow("Historical offset:", m_rightHistoricalOffsetSpin);

    rightLayout->addLayout(categoryLayout);

    // Widgets specific to each category type

    // 1. Price
    m_rightPriceWidget = new QWidget(parent);
    QFormLayout* priceLayout = new QFormLayout(m_rightPriceWidget);
    m_rightPriceTypeCombo = new QComboBox(m_rightPriceWidget);
    m_rightPriceTypeCombo->addItem("Close", static_cast<int>(filter::PriceType::CLOSE));
    m_rightPriceTypeCombo->addItem("Open", static_cast<int>(filter::PriceType::OPEN));
    m_rightPriceTypeCombo->addItem("High", static_cast<int>(filter::PriceType::HIGH));
    m_rightPriceTypeCombo->addItem("Low", static_cast<int>(filter::PriceType::LOW));
    m_rightPriceTypeCombo->addItem("Typical", static_cast<int>(filter::PriceType::TYPICAL));
    m_rightPriceTypeCombo->addItem("Median", static_cast<int>(filter::PriceType::MEDIAN));
    priceLayout->addRow("Price type:", m_rightPriceTypeCombo);
    m_rightPriceWidget->setVisible(false);
    rightLayout->addWidget(m_rightPriceWidget);

    // 2. Indicator (similar to left side)
    m_rightIndicatorWidget = new QWidget(parent);
    QVBoxLayout* indicatorLayout = new QVBoxLayout(m_rightIndicatorWidget);

    QFormLayout* indicatorTypeLayout = new QFormLayout();
    m_rightIndicatorTypeCombo = new QComboBox(m_rightIndicatorWidget);
    m_rightIndicatorTypeCombo->addItem("EMA", static_cast<int>(filter::IndicatorType::EMA));
    m_rightIndicatorTypeCombo->addItem("RSI", static_cast<int>(filter::IndicatorType::RSI));
    m_rightIndicatorTypeCombo->addItem("Stochastic K", static_cast<int>(filter::IndicatorType::STOCHASTIC_K));
    m_rightIndicatorTypeCombo->addItem("Stochastic D", static_cast<int>(filter::IndicatorType::STOCHASTIC_D));
    m_rightIndicatorTypeCombo->addItem("ATR", static_cast<int>(filter::IndicatorType::ATR));
    m_rightIndicatorTypeCombo->addItem("SuperTrend Value", static_cast<int>(filter::IndicatorType::SUPERTREND_VALUE));
    m_rightIndicatorTypeCombo->addItem("SuperTrend Direction", static_cast<int>(filter::IndicatorType::SUPERTREND_DIRECTION));
    m_rightIndicatorTypeCombo->addItem("CCI", static_cast<int>(filter::IndicatorType::CCI));
    m_rightIndicatorTypeCombo->addItem("MACD Histogram", static_cast<int>(filter::IndicatorType::MACD_HISTOGRAM));
    m_rightIndicatorTypeCombo->addItem("MACD Line (MACD)", static_cast<int>(filter::IndicatorType::MACD_LINE));
    m_rightIndicatorTypeCombo->addItem("MACD Line (Signal)", static_cast<int>(filter::IndicatorType::MACD_SIGNAL));
    m_rightIndicatorTypeCombo->addItem("Bollinger Bands - Upper Band", static_cast<int>(filter::IndicatorType::BB_UPPER));
    m_rightIndicatorTypeCombo->addItem("Bollinger Bands - Lower Band", static_cast<int>(filter::IndicatorType::BB_LOWER));
    m_rightIndicatorTypeCombo->addItem("Bollinger Bands - Percent B", static_cast<int>(filter::IndicatorType::BB_PERCENT_B));
    indicatorTypeLayout->addRow("Indicator type:", m_rightIndicatorTypeCombo);
    indicatorLayout->addLayout(indicatorTypeLayout);

    // Transform selection for right indicator
    QFormLayout* transformLayoutR = new QFormLayout();
    m_rightTransformCombo = new QComboBox(m_rightIndicatorWidget);
    m_rightTransformCombo->addItem("None", static_cast<int>(filter::TransformType::NONE));
    m_rightTransformCombo->addItem("Logarithm", static_cast<int>(filter::TransformType::LOG));
    m_rightTransformCombo->addItem("Exponential", static_cast<int>(filter::TransformType::EXP));
    m_rightTransformCombo->addItem("Derivative (diff)", static_cast<int>(filter::TransformType::DERIVATIVE));
    transformLayoutR->addRow("Transform:", m_rightTransformCombo);
    indicatorLayout->addLayout(transformLayoutR);

    // Indicator-specific parameters (similar to left side)
    // EMA
    m_rightEMAWidget = new QWidget(m_rightIndicatorWidget);
    QFormLayout* emaLayout = new QFormLayout(m_rightEMAWidget);
    m_rightEMAPeriodSpin = new QSpinBox(m_rightEMAWidget);
    m_rightEMAPeriodSpin->setRange(1, 1000);
    m_rightEMAPeriodSpin->setValue(20);
    emaLayout->addRow("Period:", m_rightEMAPeriodSpin);
    m_rightEMAWidget->setVisible(false);
    indicatorLayout->addWidget(m_rightEMAWidget);

    // RSI
    m_rightRSIWidget = new QWidget(m_rightIndicatorWidget);
    QFormLayout* rsiLayout = new QFormLayout(m_rightRSIWidget);
    m_rightRSIPeriodSpin = new QSpinBox(m_rightRSIWidget);
    m_rightRSIPeriodSpin->setRange(1, 1000);
    m_rightRSIPeriodSpin->setValue(14);
    rsiLayout->addRow("Period:", m_rightRSIPeriodSpin);
    m_rightRSIWidget->setVisible(false);
    indicatorLayout->addWidget(m_rightRSIWidget);

    // Stochastic
    m_rightStochasticWidget = new QWidget(m_rightIndicatorWidget);
    QFormLayout* stochLayout = new QFormLayout(m_rightStochasticWidget);
    m_rightStochFastKSpin = new QSpinBox(m_rightStochasticWidget);
    m_rightStochFastKSpin->setRange(1, 1000);
    m_rightStochFastKSpin->setValue(14);
    m_rightStochSlowKSpin = new QSpinBox(m_rightStochasticWidget);
    m_rightStochSlowKSpin->setRange(1, 1000);
    m_rightStochSlowKSpin->setValue(3);
    m_rightStochSlowDSpin = new QSpinBox(m_rightStochasticWidget);
    m_rightStochSlowDSpin->setRange(1, 1000);
    m_rightStochSlowDSpin->setValue(3);
    stochLayout->addRow("FastK period:", m_rightStochFastKSpin);
    stochLayout->addRow("SlowK period:", m_rightStochSlowKSpin);
    stochLayout->addRow("SlowD period:", m_rightStochSlowDSpin);
    m_rightStochasticWidget->setVisible(false);
    indicatorLayout->addWidget(m_rightStochasticWidget);

    // ATR
    m_rightATRWidget = new QWidget(m_rightIndicatorWidget);
    QFormLayout* atrLayout = new QFormLayout(m_rightATRWidget);
    m_rightATRPeriodSpin = new QSpinBox(m_rightATRWidget);
    m_rightATRPeriodSpin->setRange(1, 1000);
    m_rightATRPeriodSpin->setValue(14);
    m_rightATRUseLogCheck = new QCheckBox("Use logarithmic ATR", m_rightATRWidget);
    atrLayout->addRow("Period:", m_rightATRPeriodSpin);
    atrLayout->addRow(m_rightATRUseLogCheck);
    m_rightATRWidget->setVisible(false);
    indicatorLayout->addWidget(m_rightATRWidget);

    // SuperTrend
    m_rightSuperTrendWidget = new QWidget(m_rightIndicatorWidget);
    QFormLayout* stLayout = new QFormLayout(m_rightSuperTrendWidget);
    m_rightSuperTrendPeriodSpin = new QSpinBox(m_rightSuperTrendWidget);
    m_rightSuperTrendPeriodSpin->setRange(1, 1000);
    m_rightSuperTrendPeriodSpin->setValue(10);
    m_rightSuperTrendMultiplierSpin = new QDoubleSpinBox(m_rightSuperTrendWidget);
    m_rightSuperTrendMultiplierSpin->setRange(0.1, 1000.0);
    m_rightSuperTrendMultiplierSpin->setSingleStep(1);
    m_rightSuperTrendMultiplierSpin->setValue(3.0);
    stLayout->addRow("ATR Period:", m_rightSuperTrendPeriodSpin);
    stLayout->addRow("Multiplier:", m_rightSuperTrendMultiplierSpin);
    m_rightSuperTrendWidget->setVisible(false);
    indicatorLayout->addWidget(m_rightSuperTrendWidget);

    // CCI
    m_rightCCIWidget = new QWidget(m_rightIndicatorWidget);
    QFormLayout* cciRightLayout = new QFormLayout(m_rightCCIWidget);
    m_rightCCIPeriodSpin = new QSpinBox(m_rightCCIWidget);
    m_rightCCIPeriodSpin->setRange(1, 1000);
    m_rightCCIPeriodSpin->setValue(20);
    cciRightLayout->addRow("Period:", m_rightCCIPeriodSpin);
    // Overbought/oversold values for CCI on right side
    m_rightCCIOverboughtSpin = new QDoubleSpinBox(m_rightCCIWidget);
    m_rightCCIOverboughtSpin->setRange(-10000.0, 10000.0);
    m_rightCCIOverboughtSpin->setDecimals(2);
    m_rightCCIOverboughtSpin->setValue(100.0);
    m_rightCCIOversoldSpin = new QDoubleSpinBox(m_rightCCIWidget);
    m_rightCCIOversoldSpin->setRange(-10000.0, 10000.0);
    m_rightCCIOversoldSpin->setDecimals(2);
    m_rightCCIOversoldSpin->setValue(-100.0);
    cciRightLayout->addRow("Overbought:", m_rightCCIOverboughtSpin);
    cciRightLayout->addRow("Oversold:", m_rightCCIOversoldSpin);
    m_rightCCIWidget->setVisible(false);
    indicatorLayout->addWidget(m_rightCCIWidget);

    m_rightIndicatorWidget->setVisible(false);
    rightLayout->addWidget(m_rightIndicatorWidget);

    // MACD
    m_rightMACDWidget = new QWidget(m_rightIndicatorWidget);
    QFormLayout* macdLayout = new QFormLayout(m_rightMACDWidget);
    m_rightMACDFastPeriodSpin = new QSpinBox(m_rightMACDWidget);
    m_rightMACDFastPeriodSpin->setRange(1, 1000);   
    m_rightMACDFastPeriodSpin->setValue(12);
    m_rightMACDSlowPeriodSpin = new QSpinBox(m_rightMACDWidget);
    m_rightMACDSlowPeriodSpin->setRange(1, 1000);
    m_rightMACDSlowPeriodSpin->setValue(26);
    m_rightMACDSignalPeriodSpin = new QSpinBox(m_rightMACDWidget);
    m_rightMACDSignalPeriodSpin->setRange(1, 1000);
    m_rightMACDSignalPeriodSpin->setValue(9);
    macdLayout->addRow("Fast period:", m_rightMACDFastPeriodSpin);
    macdLayout->addRow("Slow period:", m_rightMACDSlowPeriodSpin);
    macdLayout->addRow("Signal period:", m_rightMACDSignalPeriodSpin);
    m_rightMACDWidget->setVisible(false);
    indicatorLayout->addWidget(m_rightMACDWidget);
    // Bollinger Bands
    m_rightBBWidget = new QWidget(m_rightIndicatorWidget);
    QFormLayout* bbLayout = new QFormLayout(m_rightBBWidget);
    m_rightBBPeriodSpin = new QSpinBox(m_rightBBWidget);
    m_rightBBPeriodSpin->setRange(1, 1000);
    m_rightBBPeriodSpin->setValue(20);
    m_rightBBMATypeCombo = new QComboBox(m_rightBBWidget);  
    m_rightBBMATypeCombo->addItem("SMA", static_cast<int>(filter::MAType::SMA));
    m_rightBBMATypeCombo->addItem("EMA", static_cast<int>(filter::MAType::EMA));
    m_rightBBSourceCombo = new QComboBox(m_rightBBWidget);
    m_rightBBSourceCombo->addItem("Close", static_cast<int>(filter::PriceType::CLOSE));
    m_rightBBSourceCombo->addItem("Open", static_cast<int>(filter::PriceType::OPEN));
    m_rightBBSourceCombo->addItem("High", static_cast<int>(filter::PriceType::HIGH));
    m_rightBBSourceCombo->addItem("Low", static_cast<int>(filter::PriceType::LOW));
    m_rightBBSourceCombo->addItem("Typical", static_cast<int>(filter::PriceType::TYPICAL));
    m_rightBBSourceCombo->addItem("Median", static_cast<int>(filter::PriceType::MEDIAN));
    m_rightBBStdDevMultiplierSpin   = new QDoubleSpinBox(m_rightBBWidget);
    m_rightBBStdDevMultiplierSpin->setRange(0.1, 10.0);
    m_rightBBStdDevMultiplierSpin->setSingleStep(0.1);
    m_rightBBStdDevMultiplierSpin->setValue(2.0);
    bbLayout->addRow("Period:", m_rightBBPeriodSpin);
    bbLayout->addRow("MA Type:", m_rightBBMATypeCombo);
    bbLayout->addRow("Source:", m_rightBBSourceCombo);
    bbLayout->addRow("Stddev multiplier:", m_rightBBStdDevMultiplierSpin);
    m_rightBBWidget->setVisible(false);
    indicatorLayout->addWidget(m_rightBBWidget);    

    // 3. Constant
    m_rightConstantWidget = new QWidget(parent);
    QFormLayout* constantLayout = new QFormLayout(m_rightConstantWidget);
    m_rightConstantValueSpin = new QDoubleSpinBox(m_rightConstantWidget);
    m_rightConstantValueSpin->setRange(-100000, 100000);
    m_rightConstantValueSpin->setDecimals(2);
    m_rightConstantValueSpin->setValue(0);
    constantLayout->addRow("Value:", m_rightConstantValueSpin);
    m_rightConstantWidget->setVisible(false);
    rightLayout->addWidget(m_rightConstantWidget);

    // 4. Candle property
    m_rightCandlePropertyWidget = new QWidget(parent);
    QFormLayout* candleLayout = new QFormLayout(m_rightCandlePropertyWidget);
    m_rightCandlePropertyCombo = new QComboBox(m_rightCandlePropertyWidget);
    m_rightCandlePropertyCombo->addItem("Green candle (Heikin Ashi)", static_cast<int>(filter::CandlePropertyType::HEIKIN_ASHI_IS_GREEN));
    m_rightCandlePropertyCombo->addItem("Red candle (Heikin Ashi)", static_cast<int>(filter::CandlePropertyType::HEIKIN_ASHI_IS_RED));
    m_rightCandlePropertyCombo->addItem("Green candle", static_cast<int>(filter::CandlePropertyType::IS_GREEN));
    m_rightCandlePropertyCombo->addItem("Red candle", static_cast<int>(filter::CandlePropertyType::IS_RED));
    m_rightCandlePropertyCombo->addItem("Body size", static_cast<int>(filter::CandlePropertyType::BODY_SIZE));
    m_rightCandlePropertyCombo->addItem("Upper shadow size", static_cast<int>(filter::CandlePropertyType::UPPER_SHADOW_SIZE));
    m_rightCandlePropertyCombo->addItem("Lower shadow size", static_cast<int>(filter::CandlePropertyType::LOWER_SHADOW_SIZE));
    m_rightCandlePropertyCombo->addItem("Range", static_cast<int>(filter::CandlePropertyType::RANGE));
    candleLayout->addRow("Property:", m_rightCandlePropertyCombo);
    m_rightCandlePropertyWidget->setVisible(false);
    rightLayout->addWidget(m_rightCandlePropertyWidget);
}

void FilterEditDialog::setupOperatorUI(QWidget* parent)
{
    QFormLayout* operatorLayout = new QFormLayout(parent);

    m_operatorCombo = new QComboBox(parent);
    m_operatorCombo->addItem(">", static_cast<int>(filter::ComparisonOperator::GREATER_THAN));
    m_operatorCombo->addItem("<", static_cast<int>(filter::ComparisonOperator::LESS_THAN));
    m_operatorCombo->addItem(">=", static_cast<int>(filter::ComparisonOperator::GREATER_OR_EQUAL));
    m_operatorCombo->addItem("<=", static_cast<int>(filter::ComparisonOperator::LESS_OR_EQUAL));
    m_operatorCombo->addItem("==", static_cast<int>(filter::ComparisonOperator::EQUAL));
    m_operatorCombo->addItem("!=", static_cast<int>(filter::ComparisonOperator::NOT_EQUAL));
    m_operatorCombo->addItem("Crosses above", static_cast<int>(filter::ComparisonOperator::CROSSES_ABOVE));
    m_operatorCombo->addItem("Crosses below", static_cast<int>(filter::ComparisonOperator::CROSSES_BELOW));
    m_operatorCombo->addItem("Is True", static_cast<int>(filter::ComparisonOperator::TRUE));
    m_operatorCombo->addItem("Is False", static_cast<int>(filter::ComparisonOperator::FALSE));

    operatorLayout->addRow("Operator:", m_operatorCombo);

    // Offset (threshold) between left and right values
    m_distanceSpin = new QDoubleSpinBox(parent);
    m_distanceSpin->setRange(0.0, 1e12);
    m_distanceSpin->setDecimals(5);
    m_distanceSpin->setSingleStep(0.1);
    m_distanceSpin->setValue(0.0);
    m_distanceSpin->setToolTip("Minimum offset between the two values for the condition to be true (0 = no constraint)");

    // Create a label for the offset
    m_distanceLabel = new QLabel("Offset:", parent);

    operatorLayout->addRow(m_distanceLabel, m_distanceSpin);

    // Connect operator change to handle visibility
    connect(m_operatorCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &FilterEditDialog::updateDistanceVisibility);

    // Connect distance change to update preview
    connect(m_distanceSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &FilterEditDialog::updatePreview);
}

void FilterEditDialog::setupTemporalLogicUI(QWidget* parent)
{
    QFormLayout* temporalLayout = new QFormLayout(parent);

    m_lookbackPeriodsSpin = new QSpinBox(parent);
    m_lookbackPeriodsSpin->setRange(1, 200);
    m_lookbackPeriodsSpin->setValue(1);
    temporalLayout->addRow("Bars to check:", m_lookbackPeriodsSpin);

    m_temporalLogicCombo = new QComboBox(parent);
    m_temporalLogicCombo->addItem("All of the last N periods", static_cast<int>(filter::TemporalLogic::ALL_OF));
    m_temporalLogicCombo->addItem("At least once in the last N periods", static_cast<int>(filter::TemporalLogic::ANY_OF));


    m_temporalLogicLabel = new QLabel("Temporal logic:", parent);
    temporalLayout->addRow(m_temporalLogicLabel, m_temporalLogicCombo);

    onLookbackPeriodsChanged(m_lookbackPeriodsSpin->value());
}

void FilterEditDialog::updateIndicatorParamsVisibility(QWidget* container, filter::IndicatorType type)
{
    if (container == m_leftIndicatorWidget) {
        m_leftEMAWidget->setVisible(type == filter::IndicatorType::EMA);
        m_leftRSIWidget->setVisible(type == filter::IndicatorType::RSI);
        m_leftStochasticWidget->setVisible(type == filter::IndicatorType::STOCHASTIC_K || type == filter::IndicatorType::STOCHASTIC_D);
        m_leftATRWidget->setVisible(type == filter::IndicatorType::ATR);
        m_leftSuperTrendWidget->setVisible(type == filter::IndicatorType::SUPERTREND_VALUE || type == filter::IndicatorType::SUPERTREND_DIRECTION);
        m_leftCCIWidget->setVisible(type == filter::IndicatorType::CCI);
        m_leftMACDWidget->setVisible(type == filter::IndicatorType::MACD_HISTOGRAM || type == filter::IndicatorType::MACD_LINE || type == filter::IndicatorType::MACD_SIGNAL);
        m_leftBBWidget->setVisible(type == filter::IndicatorType::BB_UPPER || type == filter::IndicatorType::BB_LOWER || type == filter::IndicatorType::BB_PERCENT_B);
    } else if (container == m_rightIndicatorWidget) {
        m_rightEMAWidget->setVisible(type == filter::IndicatorType::EMA);
        m_rightRSIWidget->setVisible(type == filter::IndicatorType::RSI);
        m_rightStochasticWidget->setVisible(type == filter::IndicatorType::STOCHASTIC_K || type == filter::IndicatorType::STOCHASTIC_D);
        m_rightATRWidget->setVisible(type == filter::IndicatorType::ATR);
        m_rightSuperTrendWidget->setVisible(type == filter::IndicatorType::SUPERTREND_VALUE || type == filter::IndicatorType::SUPERTREND_DIRECTION);
        m_rightCCIWidget->setVisible(type == filter::IndicatorType::CCI);
        m_rightMACDWidget->setVisible(type == filter::IndicatorType::MACD_HISTOGRAM || type == filter::IndicatorType::MACD_LINE || type == filter::IndicatorType::MACD_SIGNAL);
        m_rightBBWidget->setVisible(type == filter::IndicatorType::BB_UPPER || type == filter::IndicatorType::BB_LOWER || type == filter::IndicatorType::BB_PERCENT_B);
    }
}

filter::ValueSource FilterEditDialog::getLeftValueSource() const
{
    filter::ValueSource source;
    source.historicalOffset = m_leftHistoricalOffsetSpin->value();
    source.category = static_cast<filter::ValueCategory>(m_leftCategoryCombo->currentData().toInt());

    switch (source.category) {
        case filter::ValueCategory::PRICE:
            source.priceType = static_cast<filter::PriceType>(m_leftPriceTypeCombo->currentData().toInt());
            break;
        case filter::ValueCategory::INDICATOR:
            source.indicatorType = static_cast<filter::IndicatorType>(m_leftIndicatorTypeCombo->currentData().toInt());

            switch (source.indicatorType) {
                case filter::IndicatorType::EMA:
                    source.emaParams = filter::EMAParams(m_leftEMAPeriodSpin->value());
                    break;
                case filter::IndicatorType::RSI:
                    source.rsiParams = filter::RSIParams(m_leftRSIPeriodSpin->value());
                    break;
                case filter::IndicatorType::STOCHASTIC_K:
                case filter::IndicatorType::STOCHASTIC_D:
                    source.stochParams = filter::StochasticParams(
                        m_leftStochFastKSpin->value(),
                        m_leftStochSlowKSpin->value(),
                        m_leftStochSlowDSpin->value()
                    );
                    break;
                case filter::IndicatorType::ATR:
                    source.atrParams = filter::ATRParams(
                        m_leftATRPeriodSpin->value(),
                        m_leftATRUseLogCheck->isChecked()
                    );
                    break;
                case filter::IndicatorType::SUPERTREND_VALUE:
                case filter::IndicatorType::SUPERTREND_DIRECTION:
                    source.supertrendParams = filter::SuperTrendParams(
                        m_leftSuperTrendPeriodSpin->value(),
                        m_leftSuperTrendMultiplierSpin->value()
                    );
                    break;
                case filter::IndicatorType::CCI:
                    source.cciParams = filter::CCIParams(m_leftCCIPeriodSpin->value());
                    break;
                case filter::IndicatorType::MACD_HISTOGRAM:
                case filter::IndicatorType::MACD_LINE:
                case filter::IndicatorType::MACD_SIGNAL:
                    source.macdParams = filter::MACDParams(
                        m_leftMACDFastPeriodSpin->value(),
                        m_leftMACDSlowPeriodSpin->value(),
                        m_leftMACDSignalPeriodSpin->value()
                    );
                    break;
                case filter::IndicatorType::BB_UPPER:
                case filter::IndicatorType::BB_LOWER:
                case filter::IndicatorType::BB_PERCENT_B:
                    // BBParams constructor is (int period, double stddev_multiplier, int offset)
                    source.bbParams = filter::BBParams(
                        m_leftBBPeriodSpin->value(),
                        m_leftBBStdDevMultiplierSpin->value(),
                        0
                    );
                    // Set optional fields (source and ma_type) which are integers in the struct
                    source.bbParams.ma_type = static_cast<int>(m_leftBBMATypeCombo->currentData().toInt());
                    source.bbParams.source = static_cast<int>(m_leftBBSourceCombo->currentData().toInt());
                    break;
                default:
                    break;
            }

            // Transform settings (applied on the left)
            source.transform = static_cast<filter::TransformType>(m_leftTransformCombo->currentData().toInt());
            break;
        case filter::ValueCategory::CANDLE_PROPERTY:
            source.candlePropertyType = static_cast<filter::CandlePropertyType>(m_leftCandlePropertyCombo->currentData().toInt());
            break;
        default:
            break;
    }

    return source;
}

filter::ValueSource FilterEditDialog::getRightValueSource() const
{
    filter::ValueSource source;
    source.historicalOffset = m_rightHistoricalOffsetSpin->value();
    source.category = static_cast<filter::ValueCategory>(m_rightCategoryCombo->currentData().toInt());

    switch (source.category) {
        case filter::ValueCategory::PRICE:
            source.priceType = static_cast<filter::PriceType>(m_rightPriceTypeCombo->currentData().toInt());
            break;
        case filter::ValueCategory::INDICATOR:
            source.indicatorType = static_cast<filter::IndicatorType>(m_rightIndicatorTypeCombo->currentData().toInt());

            switch (source.indicatorType) {
                case filter::IndicatorType::EMA:
                    source.emaParams = filter::EMAParams(m_rightEMAPeriodSpin->value());
                    break;
                case filter::IndicatorType::RSI:
                    source.rsiParams = filter::RSIParams(m_rightRSIPeriodSpin->value());
                    break;
                case filter::IndicatorType::STOCHASTIC_K:
                case filter::IndicatorType::STOCHASTIC_D:
                    source.stochParams = filter::StochasticParams(
                        m_rightStochFastKSpin->value(),
                        m_rightStochSlowKSpin->value(),
                        m_rightStochSlowDSpin->value()
                    );
                    break;
                case filter::IndicatorType::ATR:
                    source.atrParams = filter::ATRParams(
                        m_rightATRPeriodSpin->value(),
                        m_rightATRUseLogCheck->isChecked()
                    );
                    break;
                case filter::IndicatorType::SUPERTREND_VALUE:
                case filter::IndicatorType::SUPERTREND_DIRECTION:
                    source.supertrendParams = filter::SuperTrendParams(
                        m_rightSuperTrendPeriodSpin->value(),
                        m_rightSuperTrendMultiplierSpin->value()
                    );
                    break;
                case filter::IndicatorType::CCI:
                    source.cciParams = filter::CCIParams(m_rightCCIPeriodSpin->value());
                    break;
                case filter::IndicatorType::MACD_HISTOGRAM:
                case filter::IndicatorType::MACD_LINE:
                case filter::IndicatorType::MACD_SIGNAL:
                    source.macdParams = filter::MACDParams(
                        m_rightMACDFastPeriodSpin->value(),
                        m_rightMACDSlowPeriodSpin->value(),
                        m_rightMACDSignalPeriodSpin->value()
                    );
                    break;
                case filter::IndicatorType::BB_UPPER:
                case filter::IndicatorType::BB_LOWER:
                case filter::IndicatorType::BB_PERCENT_B:
                    // BBParams constructor is (int period, double stddev_multiplier, int offset)
                    source.bbParams = filter::BBParams(
                        m_rightBBPeriodSpin->value(),
                        m_rightBBStdDevMultiplierSpin->value(),
                        0
                    );
                    // Set optional fields (source and ma_type)
                    source.bbParams.ma_type = static_cast<int>(m_rightBBMATypeCombo->currentData().toInt());
                    source.bbParams.source = static_cast<int>(m_rightBBSourceCombo->currentData().toInt());
                    break;
                default:
                    break;
            }
            break;
        case filter::ValueCategory::CONSTANT:
            source.constantValue = m_rightConstantValueSpin->value();
            break;
        case filter::ValueCategory::CANDLE_PROPERTY:
            source.candlePropertyType = static_cast<filter::CandlePropertyType>(m_rightCandlePropertyCombo->currentData().toInt());
            break;
        default:
            break;
    }

    return source;
}

void FilterEditDialog::onLeftValueCategoryChanged(int index)
{
    filter::ValueCategory category = static_cast<filter::ValueCategory>(m_leftCategoryCombo->itemData(index).toInt());

    m_leftPriceWidget->setVisible(category == filter::ValueCategory::PRICE);
    m_leftIndicatorWidget->setVisible(category == filter::ValueCategory::INDICATOR);
    m_leftCandlePropertyWidget->setVisible(category == filter::ValueCategory::CANDLE_PROPERTY);

    // If it's an indicator, update parameter visibility immediately
    if (category == filter::ValueCategory::INDICATOR) {
        filter::IndicatorType type = static_cast<filter::IndicatorType>(m_leftIndicatorTypeCombo->currentData().toInt());
        updateIndicatorParamsVisibility(m_leftIndicatorWidget, type);
    }

    updatePreview();
}

void FilterEditDialog::onRightValueCategoryChanged(int index)
{
    filter::ValueCategory category = static_cast<filter::ValueCategory>(m_rightCategoryCombo->itemData(index).toInt());

    m_rightPriceWidget->setVisible(category == filter::ValueCategory::PRICE);
    m_rightIndicatorWidget->setVisible(category == filter::ValueCategory::INDICATOR);
    m_rightConstantWidget->setVisible(category == filter::ValueCategory::CONSTANT);
    m_rightCandlePropertyWidget->setVisible(category == filter::ValueCategory::CANDLE_PROPERTY);

    // If it's an indicator, update parameter visibility immediately
    if (category == filter::ValueCategory::INDICATOR) {
        filter::IndicatorType type = static_cast<filter::IndicatorType>(m_rightIndicatorTypeCombo->currentData().toInt());
        updateIndicatorParamsVisibility(m_rightIndicatorWidget, type);
    }

    // Hide/Show historical offset (not applicable for constants)
    m_rightHistoricalOffsetSpin->setEnabled(category != filter::ValueCategory::CONSTANT);

    updatePreview();
}

void FilterEditDialog::onLeftIndicatorTypeChanged(int index)
{
    filter::IndicatorType type = static_cast<filter::IndicatorType>(m_leftIndicatorTypeCombo->itemData(index).toInt());
    updateIndicatorParamsVisibility(m_leftIndicatorWidget, type);
    updatePreview();
}

void FilterEditDialog::onRightIndicatorTypeChanged(int index)
{
    filter::IndicatorType type = static_cast<filter::IndicatorType>(m_rightIndicatorTypeCombo->itemData(index).toInt());
    updateIndicatorParamsVisibility(m_rightIndicatorWidget, type);
    updatePreview();
}

void FilterEditDialog::onComparisonOpChanged(int index) {
    filter::ComparisonOperator op = static_cast<filter::ComparisonOperator>(m_operatorCombo->itemData(index).toInt());

    bool isBoolOp = (op == filter::ComparisonOperator::TRUE || op == filter::ComparisonOperator::FALSE);

    // Show or hide the right group and the placeholder
    m_rightGroup->setVisible(!isBoolOp);
    m_rightPlaceholder->setVisible(isBoolOp);

    // Update distance visibility
    updateDistanceVisibility();

    updatePreview();
}

void FilterEditDialog::updateDistanceVisibility()
{
    filter::ComparisonOperator op = static_cast<filter::ComparisonOperator>(m_operatorCombo->currentData().toInt());

    // Show distance only for numeric comparison operators
    bool showDistance = (op == filter::ComparisonOperator::GREATER_THAN ||
                        op == filter::ComparisonOperator::LESS_THAN ||
                        op == filter::ComparisonOperator::GREATER_OR_EQUAL ||
                        op == filter::ComparisonOperator::LESS_OR_EQUAL);

    m_distanceSpin->setVisible(showDistance);
    m_distanceLabel->setVisible(showDistance);
}


void FilterEditDialog::onLookbackPeriodsChanged(int value)
{
    bool showTemporalCombo = (value > 1);
    m_temporalLogicCombo->setVisible(showTemporalCombo);
    m_temporalLogicLabel->setVisible(showTemporalCombo);

    if (showTemporalCombo) {
        // Dynamically update the combo items text
        m_temporalLogicCombo->setItemText(
            0, QString("All of the last %1 periods").arg(value)
        );
        m_temporalLogicCombo->setItemText(
            1, QString("At least once in the last %1 periods").arg(value)
        );
    }
}

void FilterEditDialog::setFilter(const filter::GenericFilter& filter)
{
    m_filter = filter;

    // Configure the left side
    int leftCategoryIndex = m_leftCategoryCombo->findData(static_cast<int>(filter.leftValue.category));
    m_leftCategoryCombo->setCurrentIndex(leftCategoryIndex);
    m_leftHistoricalOffsetSpin->setValue(filter.leftValue.historicalOffset);

    switch (filter.leftValue.category) {
        case filter::ValueCategory::PRICE:
            m_leftPriceTypeCombo->setCurrentIndex(m_leftPriceTypeCombo->findData(static_cast<int>(filter.leftValue.priceType)));
            break;
        case filter::ValueCategory::INDICATOR:
            m_leftIndicatorTypeCombo->setCurrentIndex(m_leftIndicatorTypeCombo->findData(static_cast<int>(filter.leftValue.indicatorType)));

            switch (filter.leftValue.indicatorType) {
                case filter::IndicatorType::EMA:
                    m_leftEMAPeriodSpin->setValue(filter.leftValue.emaParams.period);
                    break;
                case filter::IndicatorType::RSI:
                    m_leftRSIPeriodSpin->setValue(filter.leftValue.rsiParams.period);
                    break;
                case filter::IndicatorType::STOCHASTIC_K:
                case filter::IndicatorType::STOCHASTIC_D:
                    m_leftStochFastKSpin->setValue(filter.leftValue.stochParams.fastK);
                    m_leftStochSlowKSpin->setValue(filter.leftValue.stochParams.slowK);
                    m_leftStochSlowDSpin->setValue(filter.leftValue.stochParams.slowD);
                    break;
                case filter::IndicatorType::ATR:
                    m_leftATRPeriodSpin->setValue(filter.leftValue.atrParams.period);
                    m_leftATRUseLogCheck->setChecked(filter.leftValue.atrParams.useLog);
                    break;
                case filter::IndicatorType::SUPERTREND_VALUE:
                case filter::IndicatorType::SUPERTREND_DIRECTION:
                    m_leftSuperTrendPeriodSpin->setValue(filter.leftValue.supertrendParams.atrPeriod);
                    m_leftSuperTrendMultiplierSpin->setValue(filter.leftValue.supertrendParams.multiplier);
                    break;
                case filter::IndicatorType::CCI:
                    m_leftCCIPeriodSpin->setValue(filter.leftValue.cciParams.period);
                    break;
                case filter::IndicatorType::MACD_HISTOGRAM:
                case filter::IndicatorType::MACD_LINE:
                case filter::IndicatorType::MACD_SIGNAL:
                    m_leftMACDFastPeriodSpin->setValue(filter.leftValue.macdParams.fast);
                    m_leftMACDSlowPeriodSpin->setValue(filter.leftValue.macdParams.slow);
                    m_leftMACDSignalPeriodSpin->setValue(filter.leftValue.macdParams.signal);
                    break;
                case filter::IndicatorType::BB_UPPER:
                case filter::IndicatorType::BB_LOWER:
                case filter::IndicatorType::BB_PERCENT_B:
                    m_leftBBPeriodSpin->setValue(filter.leftValue.bbParams.period);
                    m_leftBBMATypeCombo->setCurrentIndex(m_leftBBMATypeCombo->findData(static_cast<int>(filter.leftValue.bbParams.ma_type)));
                    m_leftBBSourceCombo->setCurrentIndex(m_leftBBSourceCombo->findData(static_cast<int>(filter.leftValue.bbParams.source)));
                    m_leftBBStdDevMultiplierSpin->setValue(filter.leftValue.bbParams.stddev_multiplier);
                    break;
                default:
                    break;
            }

            // Update indicator parameter visibility
            updateIndicatorParamsVisibility(m_leftIndicatorWidget, filter.leftValue.indicatorType);
            // Restore transform settings
            m_leftTransformCombo->setCurrentIndex(m_leftTransformCombo->findData(static_cast<int>(filter.leftValue.transform)));
            m_rightTransformCombo->setCurrentIndex(m_rightTransformCombo->findData(static_cast<int>(filter.rightValue.transform)));
            break;
        case filter::ValueCategory::CANDLE_PROPERTY:
            m_leftCandlePropertyCombo->setCurrentIndex(m_leftCandlePropertyCombo->findData(static_cast<int>(filter.leftValue.candlePropertyType)));
            break;
        default:
            break;
    }

    // Configure the operator
    m_operatorCombo->setCurrentIndex(m_operatorCombo->findData(static_cast<int>(filter.op)));

    // Configure the right side
    int rightCategoryIndex = m_rightCategoryCombo->findData(static_cast<int>(filter.rightValue.category));
    m_rightCategoryCombo->setCurrentIndex(rightCategoryIndex);
    m_rightHistoricalOffsetSpin->setValue(filter.rightValue.historicalOffset);

    switch (filter.rightValue.category) {
        case filter::ValueCategory::PRICE:
            m_rightPriceTypeCombo->setCurrentIndex(m_rightPriceTypeCombo->findData(static_cast<int>(filter.rightValue.priceType)));
            break;
        case filter::ValueCategory::INDICATOR:
            m_rightIndicatorTypeCombo->setCurrentIndex(m_rightIndicatorTypeCombo->findData(static_cast<int>(filter.rightValue.indicatorType)));

            switch (filter.rightValue.indicatorType) {
                case filter::IndicatorType::EMA:
                    m_rightEMAPeriodSpin->setValue(filter.rightValue.emaParams.period);
                    break;
                case filter::IndicatorType::RSI:
                    m_rightRSIPeriodSpin->setValue(filter.rightValue.rsiParams.period);
                    break;
                case filter::IndicatorType::STOCHASTIC_K:
                case filter::IndicatorType::STOCHASTIC_D:
                    m_rightStochFastKSpin->setValue(filter.rightValue.stochParams.fastK);
                    m_rightStochSlowKSpin->setValue(filter.rightValue.stochParams.slowK);
                    m_rightStochSlowDSpin->setValue(filter.rightValue.stochParams.slowD);
                    break;
                case filter::IndicatorType::ATR:
                    m_rightATRPeriodSpin->setValue(filter.rightValue.atrParams.period);
                    m_rightATRUseLogCheck->setChecked(filter.rightValue.atrParams.useLog);
                    break;
                case filter::IndicatorType::SUPERTREND_VALUE:
                case filter::IndicatorType::SUPERTREND_DIRECTION:
                    m_rightSuperTrendPeriodSpin->setValue(filter.rightValue.supertrendParams.atrPeriod);
                    m_rightSuperTrendMultiplierSpin->setValue(filter.rightValue.supertrendParams.multiplier);
                    break;
                case filter::IndicatorType::CCI:
                    m_rightCCIPeriodSpin->setValue(filter.rightValue.cciParams.period);
                    break;
                case filter::IndicatorType::MACD_HISTOGRAM:
                case filter::IndicatorType::MACD_LINE:
                case filter::IndicatorType::MACD_SIGNAL:
                    m_rightMACDFastPeriodSpin->setValue(filter.rightValue.macdParams.fast);
                    m_rightMACDSlowPeriodSpin->setValue(filter.rightValue.macdParams.slow);
                    m_rightMACDSignalPeriodSpin->setValue(filter.rightValue.macdParams.signal);
                    break;
                case filter::IndicatorType::BB_UPPER:
                case filter::IndicatorType::BB_LOWER:
                case filter::IndicatorType::BB_PERCENT_B:
                    m_rightBBPeriodSpin->setValue(filter.rightValue.bbParams.period);
                    m_rightBBMATypeCombo->setCurrentIndex(m_rightBBMATypeCombo->findData(static_cast<int>(filter.rightValue.bbParams.ma_type)));
                    m_rightBBSourceCombo->setCurrentIndex(m_rightBBSourceCombo->findData(static_cast<int>(filter.rightValue.bbParams.source)));
                    m_rightBBStdDevMultiplierSpin->setValue(filter.rightValue.bbParams.stddev_multiplier);
                    break;
                default:
                    break;
            }

            // Update indicator parameter visibility
            updateIndicatorParamsVisibility(m_rightIndicatorWidget, filter.rightValue.indicatorType);
            break;
        case filter::ValueCategory::CONSTANT:
            m_rightConstantValueSpin->setValue(filter.rightValue.constantValue);
            break;
        case filter::ValueCategory::CANDLE_PROPERTY:
            m_rightCandlePropertyCombo->setCurrentIndex(m_rightCandlePropertyCombo->findData(static_cast<int>(filter.rightValue.candlePropertyType)));
            break;
        default:
            break;
    }

    // Configure temporal logic
    m_temporalLogicCombo->setCurrentIndex(m_temporalLogicCombo->findData(static_cast<int>(filter.temporalLogic)));
    m_lookbackPeriodsSpin->setValue(filter.lookbackPeriods);

    // Load offset
    m_distanceSpin->setValue(filter.offset);

    // Update visibility
    onLeftValueCategoryChanged(leftCategoryIndex);
    onRightValueCategoryChanged(rightCategoryIndex);
    onLookbackPeriodsChanged(filter.lookbackPeriods);
    updateDistanceVisibility();

    updatePreview();
}

void FilterEditDialog::updatePreview()
{    
    // Left part
    filter::ValueSource leftSource = getLeftValueSource();
    filter::ComparisonOperator op = static_cast<filter::ComparisonOperator>(m_operatorCombo->currentData().toInt());
    filter::ValueSource rightSource = getRightValueSource();
    filter::TemporalLogic tempLogic = static_cast<filter::TemporalLogic>(m_temporalLogicCombo->currentData().toInt());
    int lookbackPeriods = m_lookbackPeriodsSpin->value();
    double offset = m_distanceSpin->value();

    filter::GenericFilter tempFilter(leftSource, op, rightSource, tempLogic, lookbackPeriods);
    tempFilter.offset = offset;

    m_previewLabel->setText(tempFilter.description.c_str());
}

void FilterEditDialog::onOkButtonClicked()
{
    // Build the filter from current widget values
    m_filter.leftValue = getLeftValueSource();
    m_filter.rightValue = getRightValueSource();
    m_filter.op = static_cast<filter::ComparisonOperator>(m_operatorCombo->currentData().toInt());
    m_filter.temporalLogic = static_cast<filter::TemporalLogic>(m_temporalLogicCombo->currentData().toInt());
    m_filter.lookbackPeriods = m_lookbackPeriodsSpin->value();
    m_filter.offset = m_distanceSpin->value();
    m_filter.enabled = true;
    m_filter.description = m_filter.autoGenerateDescription();

    accept();
}

void FilterEditDialog::onCancelButtonClicked()
{
    reject();
}

filter::GenericFilter FilterEditDialog::getFilter() const
{
    return m_filter;
}
