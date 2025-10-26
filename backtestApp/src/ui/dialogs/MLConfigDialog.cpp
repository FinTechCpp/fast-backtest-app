#include "ui/dialogs/MLConfigDialog.h"
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
    setWindowTitle("Configuration Machine Learning");
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
    
    // Checkbox pour activer/désactiver ML
    m_useMlEntryCheck = new QCheckBox("🤖 Utiliser un modèle d'Intelligence Artificielle pour les signaux d'entrée", this);
    m_useMlEntryCheck->setStyleSheet(
        "QCheckBox {"
        "   font-size: 14px;"
        "   font-weight: bold;"
        "   padding: 10px;"
        "}"
    );
    connect(m_useMlEntryCheck, &QCheckBox::stateChanged, this, &MLConfigDialog::onUseMlEntryChanged);
    mainLayout->addWidget(m_useMlEntryCheck);
    
    // Groupe de configuration ML
    m_configGroup = new QGroupBox("Configuration du modèle ML", this);
    QVBoxLayout* configLayout = new QVBoxLayout();
    
    // Section: Chemin du modèle
    QGroupBox* modelPathGroup = new QGroupBox("Modèle ONNX", this);
    QHBoxLayout* modelPathLayout = new QHBoxLayout();
    
    QLabel* modelPathLabel = new QLabel("Chemin du modèle :", this);
    m_modelPathEdit = new QLineEdit(this);
    m_modelPathEdit->setPlaceholderText("./models/entry_signals.onnx");
    
    m_browseButton = new QPushButton("📁 Parcourir...", this);
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
    
    // Section: Paramètres du modèle
    QGroupBox* paramsGroup = new QGroupBox("Paramètres", this);
    QGridLayout* paramsLayout = new QGridLayout();
    
    // Lookback periods
    QLabel* lookbackLabel = new QLabel("Nombre de bougies historiques :", this);
    m_lookbackPeriodsSpinBox = new QSpinBox(this);
    m_lookbackPeriodsSpinBox->setRange(1, 500);
    m_lookbackPeriodsSpinBox->setValue(50);
    m_lookbackPeriodsSpinBox->setToolTip("Nombre de bougies passées utilisées comme features pour le modèle");
    paramsLayout->addWidget(lookbackLabel, 0, 0);
    paramsLayout->addWidget(m_lookbackPeriodsSpinBox, 0, 1);
    
    // Threshold
    QLabel* thresholdLabel = new QLabel("Seuil de prédiction (±) :", this);
    m_thresholdSpinBox = new QDoubleSpinBox(this);
    m_thresholdSpinBox->setRange(0.0, 1.0);
    m_thresholdSpinBox->setSingleStep(0.05);
    m_thresholdSpinBox->setValue(0.5);
    m_thresholdSpinBox->setDecimals(2);
    m_thresholdSpinBox->setToolTip("Seuil pour générer un signal (BUY si > seuil, SELL si < -seuil)");
    paramsLayout->addWidget(thresholdLabel, 1, 0);
    paramsLayout->addWidget(m_thresholdSpinBox, 1, 1);
    
    // Normalize
    m_normalizeCheck = new QCheckBox("Normaliser les features (Z-score)", this);
    m_normalizeCheck->setChecked(true);
    m_normalizeCheck->setToolTip("Appliquer une normalisation Z-score aux features avant l'inférence");
    paramsLayout->addWidget(m_normalizeCheck, 2, 0, 1, 2);
    
    paramsGroup->setLayout(paramsLayout);
    configLayout->addWidget(paramsGroup);
    
    // Section: Configuration des features (indicateurs)
    QGroupBox* featuresGroup = new QGroupBox("📊 Features (Indicateurs) utilisées par le modèle", this);
    QVBoxLayout* featuresLayout = new QVBoxLayout();
    
    QLabel* infoLabel = new QLabel(
        "💡 <b>Important :</b> L'ordre et le type des features doivent correspondre exactement "
        "à celles utilisées lors de l'entraînement du modèle ONNX.<br>"
        "Par défaut, les données OHLC des N dernières bougies sont utilisées.",
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
    
    // Liste des features avec boutons de contrôle
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
    featureControlLayout->addWidget(m_featureList, 1);
    
    // Boutons de contrôle
    QVBoxLayout* buttonLayout = new QVBoxLayout();
    
    m_addFeatureButton = new QPushButton("➕ Ajouter", this);
    m_addFeatureButton->setToolTip("Ajouter une nouvelle feature");
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
    
    m_removeFeatureButton = new QPushButton("🗑️ Supprimer", this);
    m_removeFeatureButton->setToolTip("Supprimer la feature sélectionnée");
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
    
    m_moveUpButton = new QPushButton("⬆ Monter", this);
    m_moveUpButton->setToolTip("Déplacer vers le haut");
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
    
    m_moveDownButton = new QPushButton("⬇ Descendre", this);
    m_moveDownButton->setToolTip("Déplacer vers le bas");
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
    
    // Sélecteur d'indicateur pour ajouter des features
    QGroupBox* addFeatureGroup = new QGroupBox("Ajouter un indicateur comme feature", this);
    QVBoxLayout* addFeatureLayout = new QVBoxLayout();
    
    m_indicatorTypeCombo = new QComboBox(this);
    m_indicatorTypeCombo->addItem("📉 EMA - Moyenne Mobile Exponentielle", static_cast<int>(filter::IndicatorType::EMA));
    m_indicatorTypeCombo->addItem("📊 RSI - Relative Strength Index", static_cast<int>(filter::IndicatorType::RSI));
    m_indicatorTypeCombo->addItem("📈 ATR - Average True Range", static_cast<int>(filter::IndicatorType::ATR));
    m_indicatorTypeCombo->addItem("🎯 Stochastic K", static_cast<int>(filter::IndicatorType::STOCHASTIC_K));
    m_indicatorTypeCombo->addItem("🎯 Stochastic D", static_cast<int>(filter::IndicatorType::STOCHASTIC_D));
    m_indicatorTypeCombo->addItem("🔄 SuperTrend Value", static_cast<int>(filter::IndicatorType::SUPERTREND_VALUE));
    m_indicatorTypeCombo->addItem("➡️ SuperTrend Direction", static_cast<int>(filter::IndicatorType::SUPERTREND_DIRECTION));
    m_indicatorTypeCombo->addItem("📐 CCI - Commodity Channel Index", static_cast<int>(filter::IndicatorType::CCI));
    m_indicatorTypeCombo->addItem("📊 MACD Histogram", static_cast<int>(filter::IndicatorType::MACD_HISTOGRAM));
    m_indicatorTypeCombo->addItem("📈 MACD Line", static_cast<int>(filter::IndicatorType::MACD_LINE));
    m_indicatorTypeCombo->addItem("📉 MACD Signal", static_cast<int>(filter::IndicatorType::MACD_SIGNAL));
    m_indicatorTypeCombo->addItem("🔴 Bollinger Bands Upper", static_cast<int>(filter::IndicatorType::BB_UPPER));
    m_indicatorTypeCombo->addItem("🔵 Bollinger Bands Lower", static_cast<int>(filter::IndicatorType::BB_LOWER));
    m_indicatorTypeCombo->addItem("📊 Bollinger %B", static_cast<int>(filter::IndicatorType::BB_PERCENT_B));
    addFeatureLayout->addWidget(m_indicatorTypeCombo);
    
    // Widget pour les paramètres (sera ajouté dynamiquement selon l'indicateur)
    m_parameterWidget = new QWidget(this);
    m_parameterLayout = new QVBoxLayout(m_parameterWidget);
    addFeatureLayout->addWidget(m_parameterWidget);
    
    addFeatureGroup->setLayout(addFeatureLayout);
    featuresLayout->addWidget(addFeatureGroup);
    
    featuresGroup->setLayout(featuresLayout);
    configLayout->addWidget(featuresGroup);
    
    m_configGroup->setLayout(configLayout);
    mainLayout->addWidget(m_configGroup);
    
    // Boutons OK/Cancel
    QDialogButtonBox* buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
        this
    );
    
    connect(buttonBox, &QDialogButtonBox::accepted, this, [this]() {
        // Sauvegarder les valeurs
        m_useMlEntry = m_useMlEntryCheck->isChecked();
        m_modelPath = m_modelPathEdit->text().toStdString();
        m_lookbackPeriods = m_lookbackPeriodsSpinBox->value();
        m_threshold = static_cast<float>(m_thresholdSpinBox->value());
        m_normalize = m_normalizeCheck->isChecked();
        
        accept();
    });
    
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    
    mainLayout->addWidget(buttonBox);
    
    // État initial
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
        "Sélectionner un modèle ONNX",
        QString::fromStdString(m_modelPath),
        "Modèles ONNX (*.onnx);;Tous les fichiers (*.*)"
    );
    
    if (!fileName.isEmpty()) {
        m_modelPathEdit->setText(fileName);
    }
}

void MLConfigDialog::onAddFeature() {
    int indicatorIndex = m_indicatorTypeCombo->currentData().toInt();
    filter::IndicatorType type = static_cast<filter::IndicatorType>(indicatorIndex);
    
    // Pour l'instant, on utilise des paramètres par défaut
    // TODO: Créer un dialog pour configurer les paramètres de chaque indicateur
    std::string params = "default";
    
    StrategyConfig::MLFeatureConfig feature{type, params};
    
    // Vérifier si la feature existe déjà
    auto it = std::find(m_features.begin(), m_features.end(), feature);
    if (it != m_features.end()) {
        QMessageBox::warning(this, "Feature existante", 
                           "Cette feature est déjà dans la liste.");
        return;
    }
    
    m_features.push_back(feature);
    refreshFeatureList();
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
    
    m_removeFeatureButton->setEnabled(hasSelection);
    m_moveUpButton->setEnabled(hasSelection && currentRow > 0);
    m_moveDownButton->setEnabled(hasSelection && currentRow < static_cast<int>(m_features.size()) - 1);
}

void MLConfigDialog::refreshFeatureList() {
    m_featureList->clear();
    
    for (size_t i = 0; i < m_features.size(); ++i) {
        QString displayName = QString("%1. %2").arg(i + 1)
                                               .arg(getFeatureDisplayName(m_features[i].type, m_features[i].parameters));
        m_featureList->addItem(displayName);
    }
    
    updateButtonStates();
}

QString MLConfigDialog::getFeatureDisplayName(const filter::IndicatorType& type, const std::string& params) const {
    QString name;
    
    switch (type) {
        case filter::IndicatorType::EMA:
            name = "📉 EMA";
            break;
        case filter::IndicatorType::RSI:
            name = "📊 RSI";
            break;
        case filter::IndicatorType::ATR:
            name = "📈 ATR";
            break;
        case filter::IndicatorType::STOCHASTIC_K:
            name = "🎯 Stochastic K";
            break;
        case filter::IndicatorType::STOCHASTIC_D:
            name = "🎯 Stochastic D";
            break;
        case filter::IndicatorType::SUPERTREND_VALUE:
            name = "🔄 SuperTrend Value";
            break;
        case filter::IndicatorType::SUPERTREND_DIRECTION:
            name = "➡️ SuperTrend Direction";
            break;
        case filter::IndicatorType::CCI:
            name = "📐 CCI";
            break;
        case filter::IndicatorType::MACD_HISTOGRAM:
            name = "📊 MACD Histogram";
            break;
        case filter::IndicatorType::MACD_LINE:
            name = "📈 MACD Line";
            break;
        case filter::IndicatorType::MACD_SIGNAL:
            name = "📉 MACD Signal";
            break;
        case filter::IndicatorType::BB_UPPER:
            name = "🔴 BB Upper";
            break;
        case filter::IndicatorType::BB_LOWER:
            name = "🔵 BB Lower";
            break;
        case filter::IndicatorType::BB_PERCENT_B:
            name = "📊 BB %B";
            break;
        default:
            name = "❓ Inconnu";
    }
    
    if (params != "default" && !params.empty()) {
        name += QString(" (%1)").arg(QString::fromStdString(params));
    }
    
    return name;
}
