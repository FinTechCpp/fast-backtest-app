#include "ui/dialogs/MLFeatureEditDialog.h"
#include <QMessageBox>
#include <QTabWidget>
#include <QFormLayout>

MLFeatureEditDialog::MLFeatureEditDialog(QWidget* parent)
    : QDialog(parent),
      m_featureModeCombo(nullptr),
      m_indicatorTypeCombo(nullptr),
      m_transformCombo(nullptr),
      m_parametersGroup(nullptr),
      m_parametersLayout(nullptr),
      m_compositeGroup(nullptr),
      m_compositeTypeCombo(nullptr),
      m_leftIndicatorCombo(nullptr),
      m_rightIndicatorCombo(nullptr),
      m_leftParamsLayout(nullptr),
      m_rightParamsLayout(nullptr),
      m_previewLabel(nullptr),
      m_periodSpinBox(nullptr),
      m_multiplierSpinBox(nullptr),
      m_kPeriodSpinBox(nullptr),
      m_dPeriodSpinBox(nullptr),
      m_smoothSpinBox(nullptr),
      m_fastPeriodSpinBox(nullptr),
      m_slowPeriodSpinBox(nullptr),
      m_signalPeriodSpinBox(nullptr),
      m_leftPeriodSpinBox(nullptr),
      m_leftMultiplierSpinBox(nullptr),
      m_leftKPeriodSpinBox(nullptr),
      m_leftDPeriodSpinBox(nullptr),
      m_leftSmoothSpinBox(nullptr),
      m_leftFastPeriodSpinBox(nullptr),
      m_leftSlowPeriodSpinBox(nullptr),
      m_leftSignalPeriodSpinBox(nullptr),
      m_rightPeriodSpinBox(nullptr),
      m_rightMultiplierSpinBox(nullptr),
      m_rightKPeriodSpinBox(nullptr),
      m_rightDPeriodSpinBox(nullptr),
      m_rightSmoothSpinBox(nullptr),
      m_rightFastPeriodSpinBox(nullptr),
      m_rightSlowPeriodSpinBox(nullptr),
      m_rightSignalPeriodSpinBox(nullptr),
      m_customNameEdit(nullptr)
{
    setWindowTitle("Configuration de la Feature ML");
    setMinimumSize(700, 500);
    
    // Initialize feature with default values
    m_feature.type = filter::IndicatorType::EMA;
    m_feature.transform = filter::TransformType::NONE;
    m_feature.params.period = 14;
    m_feature.is_composite = false;
    
    setupUI();
}

void MLFeatureEditDialog::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Mode selection
    QGroupBox* modeGroup = new QGroupBox("Mode de feature", this);
    QVBoxLayout* modeLayout = new QVBoxLayout();
    
    m_featureModeCombo = new QComboBox(this);
    m_featureModeCombo->addItem("Indicateur simple (avec transformation optionnelle)", 0);
    m_featureModeCombo->addItem("Opération composite (ex: distance, ratio)", 1);

    connect(m_featureModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int index) {
        bool isSimple = (index == 0);
        m_parametersGroup->setVisible(isSimple);
        m_compositeGroup->setVisible(!isSimple);
        updatePreview();
    });
    
    modeLayout->addWidget(m_featureModeCombo);
    modeGroup->setLayout(modeLayout);
    mainLayout->addWidget(modeGroup);
    
    // ========== SIMPLE MODE ==========
    m_parametersGroup = new QGroupBox("Configuration indicateur simple", this);
    QVBoxLayout* simpleLayout = new QVBoxLayout();
    
    // Indicator type selection
    QFormLayout* indicatorLayout = new QFormLayout();
    m_indicatorTypeCombo = new QComboBox(this);
    m_indicatorTypeCombo->addItem("EMA - Moyenne Mobile Exponentielle", static_cast<int>(filter::IndicatorType::EMA));
    m_indicatorTypeCombo->addItem(" RSI - Relative Strength Index", static_cast<int>(filter::IndicatorType::RSI));
    m_indicatorTypeCombo->addItem(" ATR - Average True Range", static_cast<int>(filter::IndicatorType::ATR));
    m_indicatorTypeCombo->addItem(" Stochastic K", static_cast<int>(filter::IndicatorType::STOCHASTIC_K));
    m_indicatorTypeCombo->addItem(" Stochastic D", static_cast<int>(filter::IndicatorType::STOCHASTIC_D));
    m_indicatorTypeCombo->addItem(" SuperTrend Value", static_cast<int>(filter::IndicatorType::SUPERTREND_VALUE));
    m_indicatorTypeCombo->addItem(" SuperTrend Direction", static_cast<int>(filter::IndicatorType::SUPERTREND_DIRECTION));
    m_indicatorTypeCombo->addItem(" CCI - Commodity Channel Index", static_cast<int>(filter::IndicatorType::CCI));
    m_indicatorTypeCombo->addItem(" MACD Histogram", static_cast<int>(filter::IndicatorType::MACD_HISTOGRAM));
    m_indicatorTypeCombo->addItem(" MACD Line", static_cast<int>(filter::IndicatorType::MACD_LINE));
    m_indicatorTypeCombo->addItem(" MACD Signal", static_cast<int>(filter::IndicatorType::MACD_SIGNAL));
    m_indicatorTypeCombo->addItem(" Bollinger Bands Upper", static_cast<int>(filter::IndicatorType::BB_UPPER));
    m_indicatorTypeCombo->addItem(" Bollinger Bands Lower", static_cast<int>(filter::IndicatorType::BB_LOWER));
    m_indicatorTypeCombo->addItem(" Bollinger %B", static_cast<int>(filter::IndicatorType::BB_PERCENT_B));
    
    connect(m_indicatorTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MLFeatureEditDialog::onIndicatorTypeChanged);
    
    indicatorLayout->addRow("Type d'indicateur:", m_indicatorTypeCombo);
    
    // Transform selection
    m_transformCombo = new QComboBox(this);
    m_transformCombo->addItem("Aucune", static_cast<int>(filter::TransformType::NONE));
    m_transformCombo->addItem(" Dérivée (Pente/Slope)", static_cast<int>(filter::TransformType::DERIVATIVE));
    m_transformCombo->addItem(" Logarithme", static_cast<int>(filter::TransformType::LOG));
    m_transformCombo->addItem(" Exponentielle", static_cast<int>(filter::TransformType::EXP));
    
    connect(m_transformCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MLFeatureEditDialog::onTransformChanged);
    
    indicatorLayout->addRow("Transformation:", m_transformCombo);
    simpleLayout->addLayout(indicatorLayout);
    
    // Parameters (dynamic)
    m_parametersLayout = new QGridLayout();
    simpleLayout->addLayout(m_parametersLayout);
    
    m_parametersGroup->setLayout(simpleLayout);
    mainLayout->addWidget(m_parametersGroup);
    
    // ========== COMPOSITE MODE ==========
    m_compositeGroup = new QGroupBox("Configuration opération composite", this);
    QVBoxLayout* compositeLayout = new QVBoxLayout();
    
    // Operation type
    QFormLayout* opLayout = new QFormLayout();
    m_compositeTypeCombo = new QComboBox(this);
    m_compositeTypeCombo->addItem("Distance (|A - B|)", static_cast<int>(filter::ComparisonOperator::DISTANCE_LESS)); // We use the operator enum for storage
    m_compositeTypeCombo->addItem(" Ratio (A / B)", 100); // Custom value
    m_compositeTypeCombo->addItem(" Addition (A + B)", 101);
    m_compositeTypeCombo->addItem(" Soustraction (A - B)", 102);
    
    connect(m_compositeTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MLFeatureEditDialog::onCompositeTypeChanged);
    
    opLayout->addRow("Type d'opération:", m_compositeTypeCombo);
    compositeLayout->addLayout(opLayout);
    
    // Left and Right indicator configuration in horizontal layout
    QHBoxLayout* indicatorsLayout = new QHBoxLayout();
    
    // Left side
    QGroupBox* leftGroup = new QGroupBox("Indicateur A (gauche)", this);
    QVBoxLayout* leftLayout = new QVBoxLayout();
    m_leftIndicatorCombo = new QComboBox(this);
    // Same items as m_indicatorTypeCombo
    m_leftIndicatorCombo->addItem(" EMA", static_cast<int>(filter::IndicatorType::EMA));
    m_leftIndicatorCombo->addItem(" RSI", static_cast<int>(filter::IndicatorType::RSI));
    m_leftIndicatorCombo->addItem(" ATR", static_cast<int>(filter::IndicatorType::ATR));
    m_leftIndicatorCombo->addItem(" Stochastic K", static_cast<int>(filter::IndicatorType::STOCHASTIC_K));
    m_leftIndicatorCombo->addItem(" Stochastic D", static_cast<int>(filter::IndicatorType::STOCHASTIC_D));
    m_leftIndicatorCombo->addItem(" SuperTrend Value", static_cast<int>(filter::IndicatorType::SUPERTREND_VALUE));
    m_leftIndicatorCombo->addItem(" SuperTrend Direction", static_cast<int>(filter::IndicatorType::SUPERTREND_DIRECTION));
    m_leftIndicatorCombo->addItem(" CCI", static_cast<int>(filter::IndicatorType::CCI));
    m_leftIndicatorCombo->addItem(" MACD Histogram", static_cast<int>(filter::IndicatorType::MACD_HISTOGRAM));
    m_leftIndicatorCombo->addItem(" MACD Line", static_cast<int>(filter::IndicatorType::MACD_LINE));
    m_leftIndicatorCombo->addItem(" MACD Signal", static_cast<int>(filter::IndicatorType::MACD_SIGNAL));
    m_leftIndicatorCombo->addItem(" BB Upper", static_cast<int>(filter::IndicatorType::BB_UPPER));
    m_leftIndicatorCombo->addItem(" BB Lower", static_cast<int>(filter::IndicatorType::BB_LOWER));
    m_leftIndicatorCombo->addItem(" BB %B", static_cast<int>(filter::IndicatorType::BB_PERCENT_B));
    
    connect(m_leftIndicatorCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this]() {
        // TODO: setup left parameters
        updatePreview();
    });
    
    leftLayout->addWidget(m_leftIndicatorCombo);
    m_leftParamsLayout = new QGridLayout();
    leftLayout->addLayout(m_leftParamsLayout);
    leftGroup->setLayout(leftLayout);
    indicatorsLayout->addWidget(leftGroup);
    
    // Right side
    QGroupBox* rightGroup = new QGroupBox("Indicateur B (droite)", this);
    QVBoxLayout* rightLayout = new QVBoxLayout();
    m_rightIndicatorCombo = new QComboBox(this);
    // Same items
    m_rightIndicatorCombo->addItem(" EMA", static_cast<int>(filter::IndicatorType::EMA));
    m_rightIndicatorCombo->addItem(" RSI", static_cast<int>(filter::IndicatorType::RSI));
    m_rightIndicatorCombo->addItem(" ATR", static_cast<int>(filter::IndicatorType::ATR));
    m_rightIndicatorCombo->addItem(" Stochastic K", static_cast<int>(filter::IndicatorType::STOCHASTIC_K));
    m_rightIndicatorCombo->addItem(" Stochastic D", static_cast<int>(filter::IndicatorType::STOCHASTIC_D));
    m_rightIndicatorCombo->addItem(" SuperTrend Value", static_cast<int>(filter::IndicatorType::SUPERTREND_VALUE));
    m_rightIndicatorCombo->addItem(" SuperTrend Direction", static_cast<int>(filter::IndicatorType::SUPERTREND_DIRECTION));
    m_rightIndicatorCombo->addItem(" CCI", static_cast<int>(filter::IndicatorType::CCI));
    m_rightIndicatorCombo->addItem(" MACD Histogram", static_cast<int>(filter::IndicatorType::MACD_HISTOGRAM));
    m_rightIndicatorCombo->addItem(" MACD Line", static_cast<int>(filter::IndicatorType::MACD_LINE));
    m_rightIndicatorCombo->addItem(" MACD Signal", static_cast<int>(filter::IndicatorType::MACD_SIGNAL));
    m_rightIndicatorCombo->addItem(" BB Upper", static_cast<int>(filter::IndicatorType::BB_UPPER));
    m_rightIndicatorCombo->addItem(" BB Lower", static_cast<int>(filter::IndicatorType::BB_LOWER));
    m_rightIndicatorCombo->addItem(" BB %B", static_cast<int>(filter::IndicatorType::BB_PERCENT_B));
    
    connect(m_rightIndicatorCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this]() {
        // TODO: setup right parameters
        updatePreview();
    });
    
    rightLayout->addWidget(m_rightIndicatorCombo);
    m_rightParamsLayout = new QGridLayout();
    rightLayout->addLayout(m_rightParamsLayout);
    rightGroup->setLayout(rightLayout);
    indicatorsLayout->addWidget(rightGroup);
    
    compositeLayout->addLayout(indicatorsLayout);
    m_compositeGroup->setLayout(compositeLayout);
    m_compositeGroup->setVisible(false); // Hidden by default
    mainLayout->addWidget(m_compositeGroup);
    
    // ========== COMMON SECTIONS ==========
    
    // Custom name (common to both modes)
    QGroupBox* nameGroup = new QGroupBox("Nom personnalisé (optionnel)", this);
    QVBoxLayout* nameLayout = new QVBoxLayout();
    m_customNameEdit = new QLineEdit(this);
    m_customNameEdit->setPlaceholderText("Ex: ema_slope_1m, bb_width_1m, rsi14, etc.");
    connect(m_customNameEdit, &QLineEdit::textChanged, this, &MLFeatureEditDialog::updatePreview);
    nameLayout->addWidget(m_customNameEdit);
    nameGroup->setLayout(nameLayout);
    mainLayout->addWidget(nameGroup);
    
    // Preview
    QGroupBox* previewGroup = new QGroupBox("Aperçu", this);
    QVBoxLayout* previewLayout = new QVBoxLayout();
    
    m_previewLabel = new QLabel("Aucune feature configurée", this);
    m_previewLabel->setWordWrap(true);
    m_previewLabel->setStyleSheet(
        "QLabel {"
        "   background-color: #f0f0f0;"
        "   padding: 10px;"
        "   border-radius: 4px;"
        "   font-family: monospace;"
        "}"
    );
    previewLayout->addWidget(m_previewLabel);
    previewGroup->setLayout(previewLayout);
    mainLayout->addWidget(previewGroup);
    
    mainLayout->addStretch();
    
    // Buttons
    QDialogButtonBox* buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
        this
    );
    
    connect(buttonBox, &QDialogButtonBox::accepted, this, [this]() {
        // Build the feature configuration from UI state using proper C++ struct
        int indicatorIndex = m_indicatorTypeCombo->currentData().toInt();
        m_feature.type = static_cast<filter::IndicatorType>(indicatorIndex);
        
        // Set transform
        int transformIndex = m_transformCombo->currentData().toInt();
        m_feature.transform = static_cast<filter::TransformType>(transformIndex);
        
        // Set custom name
        m_feature.custom_name = m_customNameEdit ? m_customNameEdit->text().trimmed().toStdString() : "";
        
        // Set indicator-specific parameters based on type
        switch (m_feature.type) {
            case filter::IndicatorType::EMA:
            case filter::IndicatorType::RSI:
            case filter::IndicatorType::ATR:
            case filter::IndicatorType::CCI:
                if (m_periodSpinBox) {
                    m_feature.params.period = m_periodSpinBox->value();
                }
                break;
                
            case filter::IndicatorType::STOCHASTIC_K:
            case filter::IndicatorType::STOCHASTIC_D:
                if (m_kPeriodSpinBox && m_dPeriodSpinBox && m_smoothSpinBox) {
                    m_feature.params.k_period = m_kPeriodSpinBox->value();
                    m_feature.params.d_period = m_dPeriodSpinBox->value();
                    m_feature.params.smooth = m_smoothSpinBox->value();
                }
                break;
                
            case filter::IndicatorType::SUPERTREND_VALUE:
            case filter::IndicatorType::SUPERTREND_DIRECTION:
                if (m_periodSpinBox && m_multiplierSpinBox) {
                    m_feature.params.period = m_periodSpinBox->value();
                    m_feature.params.multiplier = m_multiplierSpinBox->value();
                }
                break;
                
            case filter::IndicatorType::MACD_HISTOGRAM:
            case filter::IndicatorType::MACD_LINE:
            case filter::IndicatorType::MACD_SIGNAL:
                if (m_fastPeriodSpinBox && m_slowPeriodSpinBox && m_signalPeriodSpinBox) {
                    m_feature.params.fast_period = m_fastPeriodSpinBox->value();
                    m_feature.params.slow_period = m_slowPeriodSpinBox->value();
                    m_feature.params.signal_period = m_signalPeriodSpinBox->value();
                }
                break;
                
            case filter::IndicatorType::BB_UPPER:
            case filter::IndicatorType::BB_LOWER:
            case filter::IndicatorType::BB_PERCENT_B:
                if (m_periodSpinBox && m_multiplierSpinBox) {
                    m_feature.params.period = m_periodSpinBox->value();
                    m_feature.params.multiplier = m_multiplierSpinBox->value();
                }
                break;
                
            default:
                break;
        }
        
        accept();
    });
    
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    
    mainLayout->addWidget(buttonBox);
    
    // Trigger initial setup
    onIndicatorTypeChanged(0);
}

void MLFeatureEditDialog::setFeature(const StrategyConfig::MLFeatureConfig& feature) {
    m_feature = feature;
    
    // Select simple mode (composite mode TODO)
    m_featureModeCombo->setCurrentIndex(0);
    
    // Select the indicator type
    for (int i = 0; i < m_indicatorTypeCombo->count(); ++i) {
        if (static_cast<filter::IndicatorType>(m_indicatorTypeCombo->itemData(i).toInt()) == feature.type) {
            m_indicatorTypeCombo->setCurrentIndex(i);
            break;
        }
    }
    
    // Select the transform type
    for (int i = 0; i < m_transformCombo->count(); ++i) {
        if (static_cast<filter::TransformType>(m_transformCombo->itemData(i).toInt()) == feature.transform) {
            m_transformCombo->setCurrentIndex(i);
            break;
        }
    }
    
    // Setup parameter widgets for this indicator
    setupParameterWidgets();
    
    // Populate parameters from the feature struct
    populateParametersFromFeature(feature);
    
    // Set custom name
    if (m_customNameEdit && !feature.custom_name.empty()) {
        m_customNameEdit->setText(QString::fromStdString(feature.custom_name));
    }
    
    updatePreview();
}

void MLFeatureEditDialog::populateParametersFromFeature(const StrategyConfig::MLFeatureConfig& feature) {
    // Set parameter values based on indicator type
    switch (feature.type) {
        case filter::IndicatorType::EMA:
        case filter::IndicatorType::RSI:
        case filter::IndicatorType::ATR:
        case filter::IndicatorType::CCI:
            if (m_periodSpinBox) {
                m_periodSpinBox->setValue(feature.params.period);
            }
            break;
            
        case filter::IndicatorType::STOCHASTIC_K:
        case filter::IndicatorType::STOCHASTIC_D:
            if (m_kPeriodSpinBox) m_kPeriodSpinBox->setValue(feature.params.k_period);
            if (m_dPeriodSpinBox) m_dPeriodSpinBox->setValue(feature.params.d_period);
            if (m_smoothSpinBox) m_smoothSpinBox->setValue(feature.params.smooth);
            break;
            
        case filter::IndicatorType::SUPERTREND_VALUE:
        case filter::IndicatorType::SUPERTREND_DIRECTION:
            if (m_periodSpinBox) m_periodSpinBox->setValue(feature.params.period);
            if (m_multiplierSpinBox) m_multiplierSpinBox->setValue(feature.params.multiplier);
            break;
            
        case filter::IndicatorType::MACD_HISTOGRAM:
        case filter::IndicatorType::MACD_LINE:
        case filter::IndicatorType::MACD_SIGNAL:
            if (m_fastPeriodSpinBox) m_fastPeriodSpinBox->setValue(feature.params.fast_period);
            if (m_slowPeriodSpinBox) m_slowPeriodSpinBox->setValue(feature.params.slow_period);
            if (m_signalPeriodSpinBox) m_signalPeriodSpinBox->setValue(feature.params.signal_period);
            break;
            
        case filter::IndicatorType::BB_UPPER:
        case filter::IndicatorType::BB_LOWER:
        case filter::IndicatorType::BB_PERCENT_B:
            if (m_periodSpinBox) m_periodSpinBox->setValue(feature.params.period);
            if (m_multiplierSpinBox) m_multiplierSpinBox->setValue(feature.params.multiplier);
            break;
            
        default:
            break;
    }
}

// Deprecated helper removed: parsing string parameters is no longer used. Parameter population
// is handled via populateParametersFromFeature(const StrategyConfig::MLFeatureConfig&).

StrategyConfig::MLFeatureConfig MLFeatureEditDialog::getFeature() const {
    return m_feature;
}

void MLFeatureEditDialog::setupParameterWidgets() {
    clearParameterWidgets();
    
    bool isSimpleMode = (m_featureModeCombo->currentIndex() == 0);
    
    if (!isSimpleMode) {
        // TODO: Setup composite mode parameters
        return;
    }
    
    // Simple mode: setup parameters based on indicator type
    int indicatorIndex = m_indicatorTypeCombo->currentData().toInt();
    filter::IndicatorType type = static_cast<filter::IndicatorType>(indicatorIndex);
    
    int row = 0;
    
    switch (type) {
        case filter::IndicatorType::EMA:
        case filter::IndicatorType::RSI:
        case filter::IndicatorType::ATR:
        case filter::IndicatorType::CCI: {
            QLabel* periodLabel = new QLabel("Période :", this);
            m_periodSpinBox = new QSpinBox(this);
            m_periodSpinBox->setRange(1, 500);
            m_periodSpinBox->setValue(14);
            connect(m_periodSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
                    this, &MLFeatureEditDialog::updatePreview);
            m_parametersLayout->addWidget(periodLabel, row, 0);
            m_parametersLayout->addWidget(m_periodSpinBox, row, 1);
            row++;
            break;
        }
        
        case filter::IndicatorType::STOCHASTIC_K:
        case filter::IndicatorType::STOCHASTIC_D: {
            QLabel* kLabel = new QLabel("%K Période :", this);
            m_kPeriodSpinBox = new QSpinBox(this);
            m_kPeriodSpinBox->setRange(1, 100);
            m_kPeriodSpinBox->setValue(14);
            connect(m_kPeriodSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
                    this, &MLFeatureEditDialog::updatePreview);
            m_parametersLayout->addWidget(kLabel, row, 0);
            m_parametersLayout->addWidget(m_kPeriodSpinBox, row, 1);
            row++;
            
            QLabel* dLabel = new QLabel("%D Période :", this);
            m_dPeriodSpinBox = new QSpinBox(this);
            m_dPeriodSpinBox->setRange(1, 100);
            m_dPeriodSpinBox->setValue(3);
            connect(m_dPeriodSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
                    this, &MLFeatureEditDialog::updatePreview);
            m_parametersLayout->addWidget(dLabel, row, 0);
            m_parametersLayout->addWidget(m_dPeriodSpinBox, row, 1);
            row++;
            
            QLabel* smoothLabel = new QLabel("Lissage :", this);
            m_smoothSpinBox = new QSpinBox(this);
            m_smoothSpinBox->setRange(1, 100);
            m_smoothSpinBox->setValue(3);
            connect(m_smoothSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
                    this, &MLFeatureEditDialog::updatePreview);
            m_parametersLayout->addWidget(smoothLabel, row, 0);
            m_parametersLayout->addWidget(m_smoothSpinBox, row, 1);
            row++;
            break;
        }
        
        case filter::IndicatorType::SUPERTREND_VALUE:
        case filter::IndicatorType::SUPERTREND_DIRECTION: {
            QLabel* periodLabel = new QLabel("Période ATR :", this);
            m_periodSpinBox = new QSpinBox(this);
            m_periodSpinBox->setRange(1, 100);
            m_periodSpinBox->setValue(10);
            connect(m_periodSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
                    this, &MLFeatureEditDialog::updatePreview);
            m_parametersLayout->addWidget(periodLabel, row, 0);
            m_parametersLayout->addWidget(m_periodSpinBox, row, 1);
            row++;
            
            QLabel* multLabel = new QLabel("Multiplicateur :", this);
            m_multiplierSpinBox = new QDoubleSpinBox(this);
            m_multiplierSpinBox->setRange(0.1, 10.0);
            m_multiplierSpinBox->setSingleStep(0.1);
            m_multiplierSpinBox->setValue(3.0);
            m_multiplierSpinBox->setDecimals(1);
            connect(m_multiplierSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                    this, &MLFeatureEditDialog::updatePreview);
            m_parametersLayout->addWidget(multLabel, row, 0);
            m_parametersLayout->addWidget(m_multiplierSpinBox, row, 1);
            row++;
            break;
        }
        
        case filter::IndicatorType::MACD_HISTOGRAM:
        case filter::IndicatorType::MACD_LINE:
        case filter::IndicatorType::MACD_SIGNAL: {
            QLabel* fastLabel = new QLabel("Période rapide :", this);
            m_fastPeriodSpinBox = new QSpinBox(this);
            m_fastPeriodSpinBox->setRange(1, 100);
            m_fastPeriodSpinBox->setValue(12);
            connect(m_fastPeriodSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
                    this, &MLFeatureEditDialog::updatePreview);
            m_parametersLayout->addWidget(fastLabel, row, 0);
            m_parametersLayout->addWidget(m_fastPeriodSpinBox, row, 1);
            row++;
            
            QLabel* slowLabel = new QLabel("Période lente :", this);
            m_slowPeriodSpinBox = new QSpinBox(this);
            m_slowPeriodSpinBox->setRange(1, 200);
            m_slowPeriodSpinBox->setValue(26);
            connect(m_slowPeriodSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
                    this, &MLFeatureEditDialog::updatePreview);
            m_parametersLayout->addWidget(slowLabel, row, 0);
            m_parametersLayout->addWidget(m_slowPeriodSpinBox, row, 1);
            row++;
            
            QLabel* signalLabel = new QLabel("Période signal :", this);
            m_signalPeriodSpinBox = new QSpinBox(this);
            m_signalPeriodSpinBox->setRange(1, 100);
            m_signalPeriodSpinBox->setValue(9);
            connect(m_signalPeriodSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
                    this, &MLFeatureEditDialog::updatePreview);
            m_parametersLayout->addWidget(signalLabel, row, 0);
            m_parametersLayout->addWidget(m_signalPeriodSpinBox, row, 1);
            row++;
            break;
        }
        
        case filter::IndicatorType::BB_UPPER:
        case filter::IndicatorType::BB_LOWER:
        case filter::IndicatorType::BB_PERCENT_B: {
            QLabel* periodLabel = new QLabel("Période :", this);
            m_periodSpinBox = new QSpinBox(this);
            m_periodSpinBox->setRange(1, 200);
            m_periodSpinBox->setValue(20);
            connect(m_periodSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
                    this, &MLFeatureEditDialog::updatePreview);
            m_parametersLayout->addWidget(periodLabel, row, 0);
            m_parametersLayout->addWidget(m_periodSpinBox, row, 1);
            row++;
            
            QLabel* multLabel = new QLabel("Écart-type (σ) :", this);
            m_multiplierSpinBox = new QDoubleSpinBox(this);
            m_multiplierSpinBox->setRange(0.1, 5.0);
            m_multiplierSpinBox->setSingleStep(0.1);
            m_multiplierSpinBox->setValue(2.0);
            m_multiplierSpinBox->setDecimals(1);
            connect(m_multiplierSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                    this, &MLFeatureEditDialog::updatePreview);
            m_parametersLayout->addWidget(multLabel, row, 0);
            m_parametersLayout->addWidget(m_multiplierSpinBox, row, 1);
            row++;
            break;
        }
        
        default:
            break;
    }
}

void MLFeatureEditDialog::clearParameterWidgets() {
    // Remove all widgets from layout
    QLayoutItem* item;
    while ((item = m_parametersLayout->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }
    
    // Reset pointers
    m_periodSpinBox = nullptr;
    m_multiplierSpinBox = nullptr;
    m_kPeriodSpinBox = nullptr;
    m_dPeriodSpinBox = nullptr;
    m_smoothSpinBox = nullptr;
    m_fastPeriodSpinBox = nullptr;
    m_slowPeriodSpinBox = nullptr;
    m_signalPeriodSpinBox = nullptr;
}

void MLFeatureEditDialog::onIndicatorTypeChanged(int index) {
    Q_UNUSED(index);
    setupParameterWidgets();
    updatePreview();
}

void MLFeatureEditDialog::onTransformChanged(int index) {
    Q_UNUSED(index);
    updatePreview();
}

void MLFeatureEditDialog::onCompositeTypeChanged(int index) {
    Q_UNUSED(index);
    updatePreview();
}

void MLFeatureEditDialog::updatePreview() {
    bool isSimpleMode = (m_featureModeCombo->currentIndex() == 0);
    
    QString preview;
    
    if (isSimpleMode) {
        // Simple mode preview
        preview = "Mode: Indicateur simple\n\n";
        preview += m_indicatorTypeCombo->currentText() + "\n";
        
        // Show parameters
        int indicatorIndex = m_indicatorTypeCombo->currentData().toInt();
        filter::IndicatorType type = static_cast<filter::IndicatorType>(indicatorIndex);
        
        switch (type) {
            case filter::IndicatorType::EMA:
            case filter::IndicatorType::RSI:
            case filter::IndicatorType::ATR:
            case filter::IndicatorType::CCI:
                if (m_periodSpinBox) {
                    preview += QString("Période: %1\n").arg(m_periodSpinBox->value());
                }
                break;
            case filter::IndicatorType::STOCHASTIC_K:
            case filter::IndicatorType::STOCHASTIC_D:
                if (m_kPeriodSpinBox && m_dPeriodSpinBox && m_smoothSpinBox) {
                    preview += QString("K: %1, D: %2, Lissage: %3\n")
                        .arg(m_kPeriodSpinBox->value())
                        .arg(m_dPeriodSpinBox->value())
                        .arg(m_smoothSpinBox->value());
                }
                break;
            case filter::IndicatorType::SUPERTREND_VALUE:
            case filter::IndicatorType::SUPERTREND_DIRECTION:
                if (m_periodSpinBox && m_multiplierSpinBox) {
                    preview += QString("Période: %1, Mult: %2\n")
                        .arg(m_periodSpinBox->value())
                        .arg(m_multiplierSpinBox->value());
                }
                break;
            case filter::IndicatorType::MACD_HISTOGRAM:
            case filter::IndicatorType::MACD_LINE:
            case filter::IndicatorType::MACD_SIGNAL:
                if (m_fastPeriodSpinBox && m_slowPeriodSpinBox && m_signalPeriodSpinBox) {
                    preview += QString("Rapide: %1, Lente: %2, Signal: %3\n")
                        .arg(m_fastPeriodSpinBox->value())
                        .arg(m_slowPeriodSpinBox->value())
                        .arg(m_signalPeriodSpinBox->value());
                }
                break;
            case filter::IndicatorType::BB_UPPER:
            case filter::IndicatorType::BB_LOWER:
            case filter::IndicatorType::BB_PERCENT_B:
                if (m_periodSpinBox && m_multiplierSpinBox) {
                    preview += QString("Période: %1, σ: %2\n")
                        .arg(m_periodSpinBox->value())
                        .arg(m_multiplierSpinBox->value());
                }
                break;
            default:
                break;
        }
        
        // Show transform
        int transformIndex = m_transformCombo->currentData().toInt();
        filter::TransformType transform = static_cast<filter::TransformType>(transformIndex);
        if (transform != filter::TransformType::NONE) {
            preview += QString("\n✨ Transformation: %1").arg(m_transformCombo->currentText());
        }
    } else {
        // Composite mode preview
        preview = "Mode: Opération composite\n\n";
        preview += QString("Opération: %1\n\n").arg(m_compositeTypeCombo->currentText());
        preview += QString("A = %1\n").arg(m_leftIndicatorCombo->currentText());
        preview += QString("B = %1\n").arg(m_rightIndicatorCombo->currentText());
    }
    
    if (m_customNameEdit && !m_customNameEdit->text().trimmed().isEmpty()) {
        preview += QString("\n\n📝 Nom: %1").arg(m_customNameEdit->text().trimmed());
    }
    
    m_previewLabel->setText(preview);
}

filter::ValueSource MLFeatureEditDialog::getConfiguredValueSource() const {
    // TODO: Implement building ValueSource from UI state
    filter::ValueSource source;
    source.category = filter::ValueCategory::INDICATOR;
    
    int indicatorIndex = m_indicatorTypeCombo->currentData().toInt();
    source.indicatorType = static_cast<filter::IndicatorType>(indicatorIndex);
    
    // Set transform
    int transformIndex = m_transformCombo->currentData().toInt();
    source.transform = static_cast<filter::TransformType>(transformIndex);
    
    // Set parameters based on indicator type
    switch (source.indicatorType) {
        case filter::IndicatorType::EMA:
            if (m_periodSpinBox) {
                source.emaParams = filter::EMAParams(m_periodSpinBox->value());
            }
            break;
        case filter::IndicatorType::RSI:
            if (m_periodSpinBox) {
                source.rsiParams = filter::RSIParams(m_periodSpinBox->value());
            }
            break;
        case filter::IndicatorType::ATR:
            if (m_periodSpinBox) {
                source.atrParams = filter::ATRParams(m_periodSpinBox->value());
            }
            break;
        case filter::IndicatorType::STOCHASTIC_K:
        case filter::IndicatorType::STOCHASTIC_D:
            if (m_kPeriodSpinBox && m_dPeriodSpinBox && m_smoothSpinBox) {
                source.stochParams = filter::StochasticParams(
                    m_kPeriodSpinBox->value(),
                    m_dPeriodSpinBox->value(),
                    m_smoothSpinBox->value()
                );
            }
            break;
        case filter::IndicatorType::SUPERTREND_VALUE:
        case filter::IndicatorType::SUPERTREND_DIRECTION:
            if (m_periodSpinBox && m_multiplierSpinBox) {
                source.supertrendParams = filter::SuperTrendParams(
                    m_periodSpinBox->value(),
                    m_multiplierSpinBox->value()
                );
            }
            break;
        case filter::IndicatorType::CCI:
            if (m_periodSpinBox) {
                source.cciParams = filter::CCIParams(m_periodSpinBox->value());
            }
            break;
        case filter::IndicatorType::MACD_HISTOGRAM:
        case filter::IndicatorType::MACD_LINE:
        case filter::IndicatorType::MACD_SIGNAL:
            if (m_fastPeriodSpinBox && m_slowPeriodSpinBox && m_signalPeriodSpinBox) {
                source.macdParams = filter::MACDParams(
                    m_fastPeriodSpinBox->value(),
                    m_slowPeriodSpinBox->value(),
                    m_signalPeriodSpinBox->value()
                );
            }
            break;
        case filter::IndicatorType::BB_UPPER:
        case filter::IndicatorType::BB_LOWER:
        case filter::IndicatorType::BB_PERCENT_B:
            if (m_periodSpinBox && m_multiplierSpinBox) {
                source.bbParams = filter::BBParams(
                    m_periodSpinBox->value(),
                    m_multiplierSpinBox->value()
                );
            }
            break;
        default:
            break;
    }
    
    return source;
}
