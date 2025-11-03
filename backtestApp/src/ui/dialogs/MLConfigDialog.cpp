#include "ui/dialogs/MLConfigDialog.h"
#include "ui/dialogs/MLFeatureEditDialog.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QScrollArea>

MLConfigDialog::MLConfigDialog(QWidget* parent)
    : QDialog(parent),
      m_useMlEntry(false),
      m_modelPath("./models/entry_signals.onnx"),
      m_lookbackPeriods(50),
      m_threshold(0.5f),
      m_normalize(true)
{
    setWindowTitle("Machine Learning Configuration");
    setMinimumSize(800, 600);
    setupUI();
}

void MLConfigDialog::setConfig(const StrategyConfig& config) {
    m_useMlEntry = config.use_ml_entry;
    m_modelPath = config.ml_entry_model_path;
    m_lookbackPeriods = config.ml_entry_lookback_periods;
    m_threshold = config.ml_entry_threshold;
    m_normalize = config.ml_entry_normalize;
    m_features = config.ml_entry_features;
    
    // Update UI
    m_useMlEntryCheck->setChecked(m_useMlEntry);
    m_modelPathEdit->setText(QString::fromStdString(m_modelPath));
    m_lookbackPeriodsSpinBox->setValue(m_lookbackPeriods);
    m_thresholdSpinBox->setValue(m_threshold);
    m_normalizeCheck->setChecked(m_normalize);
    
    // Explicitly enable/disable the config group based on checkbox state
    onUseMlEntryChanged(m_useMlEntry ? Qt::Checked : Qt::Unchecked);
    
    refreshFeatureList();
}

void MLConfigDialog::updateConfig(StrategyConfig& config) {
    config.use_ml_entry = m_useMlEntry;
    config.ml_entry_model_path = m_modelPath;
    config.ml_entry_lookback_periods = m_lookbackPeriods;
    config.ml_entry_threshold = m_threshold;
    config.ml_entry_normalize = m_normalize;
    config.ml_entry_features = m_features;
}

void MLConfigDialog::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Checkbox to enable/disable ML
    m_useMlEntryCheck = new QCheckBox("Use an AI model for entry signals", this);
    m_useMlEntryCheck->setStyleSheet(
        "QCheckBox {"
        "   font-size: 14px;"
        "   font-weight: bold;"
        "   padding: 10px;"
        "}"
    );
    connect(m_useMlEntryCheck, &QCheckBox::checkStateChanged, this, &MLConfigDialog::onUseMlEntryChanged);
    mainLayout->addWidget(m_useMlEntryCheck);
    
    // ML configuration group
    m_configGroup = new QGroupBox("ML Model Configuration", this);
    QVBoxLayout* configLayout = new QVBoxLayout();
    
    // Section: Model path
    QGroupBox* modelPathGroup = new QGroupBox("ONNX Model", this);
    QHBoxLayout* modelPathLayout = new QHBoxLayout();
    
    QLabel* modelPathLabel = new QLabel("Model path:", this);
    m_modelPathEdit = new QLineEdit(this);
    m_modelPathEdit->setPlaceholderText("./models/entry_signals.onnx");
    
    m_browseButton = new QPushButton("Browse...", this);
    m_browseButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #2196F3;"
        "   color: white;"
        "   font-weight: bold;"
        "   padding: 5px 15px;"
        "   border-radius: 3px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #0b7dda;"
        "}"
    );
    connect(m_browseButton, &QPushButton::clicked, this, &MLConfigDialog::onBrowseModelPath);
    
    modelPathLayout->addWidget(modelPathLabel);
    modelPathLayout->addWidget(m_modelPathEdit, 1);
    modelPathLayout->addWidget(m_browseButton);
    modelPathGroup->setLayout(modelPathLayout);
    configLayout->addWidget(modelPathGroup);
    
    // Section: Model parameters
    QGroupBox* paramsGroup = new QGroupBox("Parameters", this);
    QGridLayout* paramsLayout = new QGridLayout();
    
    // Lookback periods
    QLabel* lookbackLabel = new QLabel("Number of historical candles:", this);
    m_lookbackPeriodsSpinBox = new QSpinBox(this);
    m_lookbackPeriodsSpinBox->setRange(1, 500);
    m_lookbackPeriodsSpinBox->setValue(50);
    m_lookbackPeriodsSpinBox->setToolTip("Number of past candles used as features for the model");
    paramsLayout->addWidget(lookbackLabel, 0, 0);
    paramsLayout->addWidget(m_lookbackPeriodsSpinBox, 0, 1);
    
    // Threshold
    QLabel* thresholdLabel = new QLabel("Prediction threshold (±):", this);
    m_thresholdSpinBox = new QDoubleSpinBox(this);
    m_thresholdSpinBox->setRange(0.0, 1.0);
    m_thresholdSpinBox->setSingleStep(0.05);
    m_thresholdSpinBox->setValue(0.5);
    m_thresholdSpinBox->setDecimals(2);
    m_thresholdSpinBox->setToolTip("Threshold to generate a signal (BUY if > threshold, SELL if < -threshold)");
    paramsLayout->addWidget(thresholdLabel, 1, 0);
    paramsLayout->addWidget(m_thresholdSpinBox, 1, 1);
    
    // Normalize
    m_normalizeCheck = new QCheckBox("Normalize features (Z-score)", this);
    m_normalizeCheck->setChecked(true);
    m_normalizeCheck->setToolTip("Apply Z-score normalization to features before inference");
    paramsLayout->addWidget(m_normalizeCheck, 2, 0, 1, 2);
    
    paramsGroup->setLayout(paramsLayout);
    configLayout->addWidget(paramsGroup);
    
    // Section: Features configuration (indicators)
    QGroupBox* featuresGroup = new QGroupBox("Features (Indicators) used by the model", this);
    QVBoxLayout* featuresLayout = new QVBoxLayout();
    
    QLabel* infoLabel = new QLabel(
        "<b>Important :</b> The order and type of features must exactly match "
        "those used during the training of the ONNX model.<br>"
        "By default, the OHLC data of the last N candles are used.",
        this
    );
    infoLabel->setWordWrap(true);
    infoLabel->setStyleSheet(
        "QLabel {"
        "   background-color: #fff3cd;"
        "   border-left: 4px solid #ffc107;"
        "   padding: 10px;"
        "   border-radius: 3px;"
        "   font-size: 11px;"
        "}"
    );
    featuresLayout->addWidget(infoLabel);
    
    // Feature list with control buttons
    QHBoxLayout* featureControlLayout = new QHBoxLayout();
    
    m_featureList = new QListWidget(this);
    m_featureList->setMinimumHeight(200);
    m_featureList->setStyleSheet(
        "QListWidget {"
        "   border: 2px solid #d0d0d0;"
        "   border-radius: 4px;"
        "   background-color: #ffffff;"
        "}"
    );
    connect(m_featureList, &QListWidget::itemSelectionChanged, this, &MLConfigDialog::updateButtonStates);
    connect(m_featureList, &QListWidget::itemDoubleClicked, this, &MLConfigDialog::onEditFeature);
    featureControlLayout->addWidget(m_featureList, 1);
    
    // Control buttons
    QVBoxLayout* buttonLayout = new QVBoxLayout();
    
    m_addFeatureButton = new QPushButton("➕ Add", this);
    m_addFeatureButton->setToolTip("Add a new feature");
    m_addFeatureButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #4CAF50;"
        "   color: white;"
        "   font-weight: bold;"
        "   padding: 8px;"
        "   border-radius: 3px;"
        "}"
        "QPushButton:hover { background-color: #45a049; }"
    );
    connect(m_addFeatureButton, &QPushButton::clicked, this, &MLConfigDialog::onAddFeature);
    buttonLayout->addWidget(m_addFeatureButton);
    
    m_editFeatureButton = new QPushButton("✏️ Edit", this);
    m_editFeatureButton->setToolTip("Edit the selected feature");
    m_editFeatureButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #FF9800;"
        "   color: white;"
        "   font-weight: bold;"
        "   padding: 8px;"
        "   border-radius: 3px;"
        "}"
        "QPushButton:hover { background-color: #F57C00; }"
        "QPushButton:disabled { background-color: #cccccc; color: #888888; }"
    );
    connect(m_editFeatureButton, &QPushButton::clicked, this, [this]() {
        QListWidgetItem* currentItem = m_featureList->currentItem();
        if (currentItem) {
            onEditFeature(currentItem);
        }
    });
    buttonLayout->addWidget(m_editFeatureButton);
    
    m_removeFeatureButton = new QPushButton("🗑️ Remove", this);
    m_removeFeatureButton->setToolTip("Remove the selected feature");
    m_removeFeatureButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #f44336;"
        "   color: white;"
        "   font-weight: bold;"
        "   padding: 8px;"
        "   border-radius: 3px;"
        "}"
        "QPushButton:hover { background-color: #da190b; }"
        "QPushButton:disabled { background-color: #cccccc; color: #888888; }"
    );
    connect(m_removeFeatureButton, &QPushButton::clicked, this, &MLConfigDialog::onRemoveFeature);
    buttonLayout->addWidget(m_removeFeatureButton);
    
    buttonLayout->addSpacing(10);
    
    m_moveUpButton = new QPushButton("⬆ Move Up", this);
    m_moveUpButton->setToolTip("Move up");
    m_moveUpButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #2196F3;"
        "   color: white;"
        "   font-weight: bold;"
        "   padding: 8px;"
        "   border-radius: 3px;"
        "}"
        "QPushButton:hover { background-color: #0b7dda; }"
        "QPushButton:disabled { background-color: #cccccc; color: #888888; }"
    );
    connect(m_moveUpButton, &QPushButton::clicked, this, &MLConfigDialog::onMoveFeatureUp);
    buttonLayout->addWidget(m_moveUpButton);
    
    m_moveDownButton = new QPushButton("⬇ Move Down", this);
    m_moveDownButton->setToolTip("Move down");
    m_moveDownButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #2196F3;"
        "   color: white;"
        "   font-weight: bold;"
        "   padding: 8px;"
        "   border-radius: 3px;"
        "}"
        "QPushButton:hover { background-color: #0b7dda; }"
        "QPushButton:disabled { background-color: #cccccc; color: #888888; }"
    );
    connect(m_moveDownButton, &QPushButton::clicked, this, &MLConfigDialog::onMoveFeatureDown);
    buttonLayout->addWidget(m_moveDownButton);
    
    buttonLayout->addStretch();
    
    featureControlLayout->addLayout(buttonLayout);
    featuresLayout->addLayout(featureControlLayout);
    
    // Informational label
    QLabel* featureInfoLabel = new QLabel("<b>Tip:</b> Double-click a feature to edit it", this);
    featureInfoLabel->setWordWrap(true);
    featureInfoLabel->setStyleSheet(
        "QLabel {"
        "   color: #555555;"
        "   font-size: 11px;"
        "   font-style: italic;"
        "   padding: 6px;"
        "   background-color: #f0f8ff;"
        "   border-left: 3px solid #0078d4;"
        "   border-radius: 3px;"
        "}"
    );
    featuresLayout->addWidget(featureInfoLabel);
    
    featuresGroup->setLayout(featuresLayout);
    configLayout->addWidget(featuresGroup);
    
    m_configGroup->setLayout(configLayout);
    mainLayout->addWidget(m_configGroup);
    
    // OK/Cancel buttons
    QDialogButtonBox* buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
        this
    );
    
    connect(buttonBox, &QDialogButtonBox::accepted, this, [this]() {
        // Save values
        m_useMlEntry = m_useMlEntryCheck->isChecked();
        m_modelPath = m_modelPathEdit->text().toStdString();
        m_lookbackPeriods = m_lookbackPeriodsSpinBox->value();
        m_threshold = static_cast<float>(m_thresholdSpinBox->value());
        m_normalize = m_normalizeCheck->isChecked();
        
        accept();
    });
    
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    
    mainLayout->addWidget(buttonBox);
    
    // Initial state
    onUseMlEntryChanged(m_useMlEntryCheck->isChecked() ? Qt::Checked : Qt::Unchecked);
    updateButtonStates();
}

void MLConfigDialog::onUseMlEntryChanged(int state) {
    bool enabled = (state == Qt::Checked);
    m_configGroup->setEnabled(enabled);
}

void MLConfigDialog::onBrowseModelPath() {
    QString fileName = QFileDialog::getOpenFileName(
        this,
        "Select an ONNX model",
        QString::fromStdString(m_modelPath),
        "ONNX Models (*.onnx);;All files (*.*)"
    );
    
    if (!fileName.isEmpty()) {
        m_modelPathEdit->setText(fileName);
    }
}

void MLConfigDialog::onAddFeature() {
    // Open dialog to configure a new feature
    MLFeatureEditDialog dialog(this);
    
    if (dialog.exec() == QDialog::Accepted) {
        StrategyConfig::MLFeatureConfig newFeature = dialog.getFeature();
        
        // Check if feature already exists using proper C++ struct comparison
        auto it = std::find(m_features.begin(), m_features.end(), newFeature);
        
        if (it != m_features.end()) {
            QMessageBox::warning(this, "Existing feature", 
                               "This feature with these parameters is already in the list.");
            return;
        }
        
        m_features.push_back(newFeature);
        refreshFeatureList();
    }
}

void MLConfigDialog::onEditFeature(QListWidgetItem* item) {
    if (!item) return;
    
    int row = m_featureList->row(item);
    if (row < 0 || row >= static_cast<int>(m_features.size())) {
        return;
    }
    
    // Open dialog with the existing feature
    MLFeatureEditDialog dialog(this);
    dialog.setFeature(m_features[row]);
    
    if (dialog.exec() == QDialog::Accepted) {
        StrategyConfig::MLFeatureConfig updatedFeature = dialog.getFeature();
        
        // Check if this configuration already exists (excluding current item) using proper C++ struct comparison
        for (size_t i = 0; i < m_features.size(); ++i) {
            if (i != static_cast<size_t>(row) && m_features[i] == updatedFeature) {
                QMessageBox::warning(this, "Existing feature", 
                                   "This feature with these parameters is already in the list.");
                return;
            }
        }
        
        m_features[row] = updatedFeature;
        refreshFeatureList();
        m_featureList->setCurrentRow(row);
    }
}

void MLConfigDialog::onRemoveFeature() {
    int currentRow = m_featureList->currentRow();
    if (currentRow < 0 || currentRow >= static_cast<int>(m_features.size())) {
        return;
    }
    
    m_features.erase(m_features.begin() + currentRow);
    refreshFeatureList();
}

void MLConfigDialog::onMoveFeatureUp() {
    int currentRow = m_featureList->currentRow();
    if (currentRow <= 0 || currentRow >= static_cast<int>(m_features.size())) {
        return;
    }
    
    std::swap(m_features[currentRow], m_features[currentRow - 1]);
    refreshFeatureList();
    m_featureList->setCurrentRow(currentRow - 1);
}

void MLConfigDialog::onMoveFeatureDown() {
    int currentRow = m_featureList->currentRow();
    if (currentRow < 0 || currentRow >= static_cast<int>(m_features.size()) - 1) {
        return;
    }
    
    std::swap(m_features[currentRow], m_features[currentRow + 1]);
    refreshFeatureList();
    m_featureList->setCurrentRow(currentRow + 1);
}

void MLConfigDialog::updateButtonStates() {
    bool hasSelection = m_featureList->currentRow() >= 0;
    int currentRow = m_featureList->currentRow();
    
    m_editFeatureButton->setEnabled(hasSelection);
    m_removeFeatureButton->setEnabled(hasSelection);
    m_moveUpButton->setEnabled(hasSelection && currentRow > 0);
    m_moveDownButton->setEnabled(hasSelection && currentRow < static_cast<int>(m_features.size()) - 1);
}

void MLConfigDialog::refreshFeatureList() {
    m_featureList->clear();
    
    for (size_t i = 0; i < m_features.size(); ++i) {
        QString displayName = QString("%1. %2").arg(i + 1)
                                               .arg(getFeatureDisplayName(m_features[i]));
        m_featureList->addItem(displayName);
    }
    
    updateButtonStates();
}

// (Removed obsolete overload getFeatureDisplayName(IndicatorType))
// The class header only declares getFeatureDisplayName(const StrategyConfig::MLFeatureConfig&),
// so we keep implementations matching the header.

QString MLConfigDialog::getFeatureDisplayName(const StrategyConfig::MLFeatureConfig& feature) const {
    // Use custom name if provided
    if (!feature.custom_name.empty()) {
        return QString::fromStdString(feature.custom_name);
    }
    
    QString name;
    
    // Map indicator type to display name
    switch (feature.type) {
        case filter::IndicatorType::EMA:
            name = "EMA";
            break;
        case filter::IndicatorType::RSI:
            name = "RSI";
            break;
        case filter::IndicatorType::ATR:
            name = "ATR";
            break;
        case filter::IndicatorType::STOCHASTIC_K:
            name = "Stochastic K";
            break;
        case filter::IndicatorType::STOCHASTIC_D:
            name = "Stochastic D";
            break;
        case filter::IndicatorType::SUPERTREND_VALUE:
            name = "SuperTrend Value";
            break;
        case filter::IndicatorType::SUPERTREND_DIRECTION:
            name = "SuperTrend Direction";
            break;
        case filter::IndicatorType::CCI:
            name = "CCI";
            break;
        case filter::IndicatorType::MACD_HISTOGRAM:
            name = "MACD Histogram";
            break;
        case filter::IndicatorType::MACD_LINE:
            name = "MACD Line";
            break;
        case filter::IndicatorType::MACD_SIGNAL:
            name = "MACD Signal";
            break;
        case filter::IndicatorType::BB_UPPER:
            name = "BB Upper";
            break;
        case filter::IndicatorType::BB_LOWER:
            name = "BB Lower";
            break;
        case filter::IndicatorType::BB_PERCENT_B:
            name = "BB %B";
            break;
        case filter::IndicatorType::TIME_SIN:
            name = "Time Cyclic (Sin)";
            break;
        case filter::IndicatorType::TIME_COS:
            name = "Time Cyclic (Cos)";
            break;
        default:
            name = "Unknown";
    }
    
    // Add parameter details using proper C++ struct fields
    QString paramsStr;
    
    switch (feature.type) {
        case filter::IndicatorType::EMA:
        case filter::IndicatorType::RSI:
        case filter::IndicatorType::ATR:
        case filter::IndicatorType::CCI:
            paramsStr = QString("period=%1").arg(feature.params.period);
            break;
            
        case filter::IndicatorType::STOCHASTIC_K:
        case filter::IndicatorType::STOCHASTIC_D:
            paramsStr = QString("k=%1,d=%2,s=%3")
                .arg(feature.params.k_period)
                .arg(feature.params.d_period)
                .arg(feature.params.smooth);
            break;
            
        case filter::IndicatorType::SUPERTREND_VALUE:
        case filter::IndicatorType::SUPERTREND_DIRECTION:
            paramsStr = QString("period=%1,mult=%2")
                .arg(feature.params.period)
                .arg(feature.params.multiplier);
            break;
            
        case filter::IndicatorType::MACD_HISTOGRAM:
        case filter::IndicatorType::MACD_LINE:
        case filter::IndicatorType::MACD_SIGNAL:
            paramsStr = QString("fast=%1,slow=%2,sig=%3")
                .arg(feature.params.fast_period)
                .arg(feature.params.slow_period)
                .arg(feature.params.signal_period);
            break;
            
        case filter::IndicatorType::BB_UPPER:
        case filter::IndicatorType::BB_LOWER:
        case filter::IndicatorType::BB_PERCENT_B:
            paramsStr = QString("period=%1,σ=%2")
                .arg(feature.params.period)
                .arg(feature.params.multiplier);
            break;
            
        case filter::IndicatorType::TIME_SIN:
        case filter::IndicatorType::TIME_COS:
            // No parameters for TimeCyclic
            break;
            
        default:
            break;
    }
    
    // Add transform if not NONE
    if (feature.transform != filter::TransformType::NONE) {
        QString transformStr;
        switch (feature.transform) {
            case filter::TransformType::DERIVATIVE:
                transformStr = "DERIVATIVE";
                break;
            case filter::TransformType::LOG:
                transformStr = "LOG";
                break;
            case filter::TransformType::EXP:
                transformStr = "EXP";
                break;
            default:
                break;
        }
        if (!transformStr.isEmpty()) {
            if (!paramsStr.isEmpty()) paramsStr += ",";
            paramsStr += transformStr;
        }
    }
    
    if (!paramsStr.isEmpty()) {
        name += QString(" [%1]").arg(paramsStr);
    }
    
    return name;
}
