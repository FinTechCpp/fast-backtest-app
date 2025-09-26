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
    setWindowTitle("Édition de filtre");
    resize(800, 500);  // Plus large pour la disposition horizontale
    
    setupUI();
    
    // Forcer l'actualisation initiale pour afficher correctement les widgets
    onLeftValueCategoryChanged(m_leftCategoryCombo->currentIndex());
    onLeftIndicatorTypeChanged(m_leftIndicatorTypeCombo->currentIndex());
    onRightValueCategoryChanged(m_rightCategoryCombo->currentIndex());
    onRightIndicatorTypeChanged(m_rightIndicatorTypeCombo->currentIndex());
    
    updatePreview();
}

void FilterEditDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Disposition horizontale principale : Gauche | Opérateur | Droite
    QHBoxLayout* horizontalLayout = new QHBoxLayout();
    
    // Groupe pour la partie gauche
    QGroupBox* leftGroup = new QGroupBox("Valeur de gauche", this);
    setupLeftValueUI(leftGroup);
    horizontalLayout->addWidget(leftGroup, 3);  // Ratio 3
    
    // Groupe pour l'opérateur (au milieu)
    QGroupBox* operatorGroup = new QGroupBox("Opérateur", this);
    setupOperatorUI(operatorGroup);
    horizontalLayout->addWidget(operatorGroup, 1);  // Ratio 1
    
    // Groupe pour la partie droite
    QGroupBox* rightGroup = new QGroupBox("Valeur de droite", this);
    setupRightValueUI(rightGroup);
    horizontalLayout->addWidget(rightGroup, 3);  // Ratio 3
    
    mainLayout->addLayout(horizontalLayout);
    
    // Groupe pour la logique temporelle (en bas)
    QGroupBox* temporalGroup = new QGroupBox("Logique temporelle", this);
    setupTemporalLogicUI(temporalGroup);
    mainLayout->addWidget(temporalGroup);
    
    // Aperçu du filtre
    QGroupBox* previewGroup = new QGroupBox("Aperçu du filtre", this);
    QVBoxLayout* previewLayout = new QVBoxLayout(previewGroup);
    m_previewLabel = new QLabel(previewGroup);
    m_previewLabel->setWordWrap(true);
    m_previewLabel->setStyleSheet("font-weight: bold;");
    previewLayout->addWidget(m_previewLabel);
    mainLayout->addWidget(previewGroup);
    
    // Boutons OK/Annuler
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    QPushButton* cancelButton = new QPushButton("Annuler", this);
    QPushButton* okButton = new QPushButton("OK", this);
    okButton->setDefault(true);
    buttonLayout->addStretch();
    buttonLayout->addWidget(cancelButton);
    buttonLayout->addWidget(okButton);
    mainLayout->addLayout(buttonLayout);
    
    connect(okButton, &QPushButton::clicked, this, &FilterEditDialog::onOkButtonClicked);
    connect(cancelButton, &QPushButton::clicked, this, &FilterEditDialog::onCancelButtonClicked);
    
    // Connexions pour mise à jour de l'aperçu
    connect(m_leftCategoryCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &FilterEditDialog::onLeftValueCategoryChanged);
    connect(m_leftIndicatorTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &FilterEditDialog::onLeftIndicatorTypeChanged);
    connect(m_rightCategoryCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &FilterEditDialog::onRightValueCategoryChanged);
    connect(m_rightIndicatorTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &FilterEditDialog::onRightIndicatorTypeChanged);
    
    // Connecter tous les widgets à updatePreview()
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
    
    connect(m_operatorCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &FilterEditDialog::updatePreview);
    connect(m_temporalLogicCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &FilterEditDialog::updatePreview);
    connect(m_lookbackPeriodsSpin, QOverload<int>::of(&QSpinBox::valueChanged), 
            this, &FilterEditDialog::updatePreview);
}

void FilterEditDialog::setupLeftValueUI(QWidget* parent)
{
    QVBoxLayout* leftLayout = new QVBoxLayout(parent);
    
    // Type de catégorie (Prix, Indicateur, etc.)
    QFormLayout* categoryLayout = new QFormLayout();
    m_leftCategoryCombo = new QComboBox(parent);
    m_leftCategoryCombo->addItem("Prix", static_cast<int>(ValueCategory::PRICE));
    m_leftCategoryCombo->addItem("Indicateur", static_cast<int>(ValueCategory::INDICATOR));
    m_leftCategoryCombo->addItem("Propriété bougie", static_cast<int>(ValueCategory::CANDLE_PROPERTY));
    categoryLayout->addRow("Catégorie:", m_leftCategoryCombo);
    
    // Décalage historique commun à toutes les catégories
    m_leftHistoricalOffsetSpin = new QSpinBox(parent);
    m_leftHistoricalOffsetSpin->setRange(0, 100);
    m_leftHistoricalOffsetSpin->setValue(0);
    m_leftHistoricalOffsetSpin->setSuffix(" barres");
    categoryLayout->addRow("Décalage historique:", m_leftHistoricalOffsetSpin);
    
    leftLayout->addLayout(categoryLayout);
    
    // Widgets spécifiques à chaque type de catégorie
    
    // 1. Prix
    m_leftPriceWidget = new QWidget(parent);
    QFormLayout* priceLayout = new QFormLayout(m_leftPriceWidget);
    m_leftPriceTypeCombo = new QComboBox(m_leftPriceWidget);
    m_leftPriceTypeCombo->addItem("Clôture", static_cast<int>(PriceType::CLOSE));
    m_leftPriceTypeCombo->addItem("Ouverture", static_cast<int>(PriceType::OPEN));
    m_leftPriceTypeCombo->addItem("Plus haut", static_cast<int>(PriceType::HIGH));
    m_leftPriceTypeCombo->addItem("Plus bas", static_cast<int>(PriceType::LOW));
    m_leftPriceTypeCombo->addItem("Typique", static_cast<int>(PriceType::TYPICAL));
    m_leftPriceTypeCombo->addItem("Médian", static_cast<int>(PriceType::MEDIAN));
    priceLayout->addRow("Type de prix:", m_leftPriceTypeCombo);
    m_leftPriceWidget->setVisible(false);
    leftLayout->addWidget(m_leftPriceWidget);
    
    // 2. Indicateur
    m_leftIndicatorWidget = new QWidget(parent);
    QVBoxLayout* indicatorLayout = new QVBoxLayout(m_leftIndicatorWidget);
    
    // Type d'indicateur
    QFormLayout* indicatorTypeLayout = new QFormLayout();
    m_leftIndicatorTypeCombo = new QComboBox(m_leftIndicatorWidget);
    m_leftIndicatorTypeCombo->addItem("EMA", static_cast<int>(IndicatorType::EMA));
    m_leftIndicatorTypeCombo->addItem("RSI", static_cast<int>(IndicatorType::RSI));
    m_leftIndicatorTypeCombo->addItem("Stochastique K", static_cast<int>(IndicatorType::STOCHASTIC_K));
    m_leftIndicatorTypeCombo->addItem("Stochastique D", static_cast<int>(IndicatorType::STOCHASTIC_D));
    m_leftIndicatorTypeCombo->addItem("ATR", static_cast<int>(IndicatorType::ATR));
    m_leftIndicatorTypeCombo->addItem("SuperTrend Valeur", static_cast<int>(IndicatorType::SUPERTREND_VALUE));
    m_leftIndicatorTypeCombo->addItem("SuperTrend Direction", static_cast<int>(IndicatorType::SUPERTREND_DIRECTION));
    indicatorTypeLayout->addRow("Type d'indicateur:", m_leftIndicatorTypeCombo);
    indicatorLayout->addLayout(indicatorTypeLayout);
    
    // Paramètres spécifiques à chaque type d'indicateur
    
    // EMA
    m_leftEMAWidget = new QWidget(m_leftIndicatorWidget);
    QFormLayout* emaLayout = new QFormLayout(m_leftEMAWidget);
    m_leftEMAPeriodSpin = new QSpinBox(m_leftEMAWidget);
    m_leftEMAPeriodSpin->setRange(1, 500);
    m_leftEMAPeriodSpin->setValue(20);
    emaLayout->addRow("Période:", m_leftEMAPeriodSpin);
    m_leftEMAWidget->setVisible(false);
    indicatorLayout->addWidget(m_leftEMAWidget);
    
    // RSI
    m_leftRSIWidget = new QWidget(m_leftIndicatorWidget);
    QFormLayout* rsiLayout = new QFormLayout(m_leftRSIWidget);
    m_leftRSIPeriodSpin = new QSpinBox(m_leftRSIWidget);
    m_leftRSIPeriodSpin->setRange(1, 500);
    m_leftRSIPeriodSpin->setValue(14);
    rsiLayout->addRow("Période:", m_leftRSIPeriodSpin);
    m_leftRSIWidget->setVisible(false);
    indicatorLayout->addWidget(m_leftRSIWidget);
    
    // Stochastique
    m_leftStochasticWidget = new QWidget(m_leftIndicatorWidget);
    QFormLayout* stochLayout = new QFormLayout(m_leftStochasticWidget);
    m_leftStochFastKSpin = new QSpinBox(m_leftStochasticWidget);
    m_leftStochFastKSpin->setRange(1, 500);
    m_leftStochFastKSpin->setValue(14);
    m_leftStochSlowKSpin = new QSpinBox(m_leftStochasticWidget);
    m_leftStochSlowKSpin->setRange(1, 500);
    m_leftStochSlowKSpin->setValue(3);
    m_leftStochSlowDSpin = new QSpinBox(m_leftStochasticWidget);
    m_leftStochSlowDSpin->setRange(1, 500);
    m_leftStochSlowDSpin->setValue(3);
    stochLayout->addRow("Période FastK:", m_leftStochFastKSpin);
    stochLayout->addRow("Période SlowK:", m_leftStochSlowKSpin);
    stochLayout->addRow("Période SlowD:", m_leftStochSlowDSpin);
    m_leftStochasticWidget->setVisible(false);
    indicatorLayout->addWidget(m_leftStochasticWidget);
    
    // ATR
    m_leftATRWidget = new QWidget(m_leftIndicatorWidget);
    QFormLayout* atrLayout = new QFormLayout(m_leftATRWidget);
    m_leftATRPeriodSpin = new QSpinBox(m_leftATRWidget);
    m_leftATRPeriodSpin->setRange(1, 500);
    m_leftATRPeriodSpin->setValue(14);
    m_leftATRUseLogCheck = new QCheckBox("Utiliser ATR logarithmique", m_leftATRWidget);
    atrLayout->addRow("Période:", m_leftATRPeriodSpin);
    atrLayout->addRow(m_leftATRUseLogCheck);
    m_leftATRWidget->setVisible(false);
    indicatorLayout->addWidget(m_leftATRWidget);
    
    // SuperTrend
    m_leftSuperTrendWidget = new QWidget(m_leftIndicatorWidget);
    QFormLayout* stLayout = new QFormLayout(m_leftSuperTrendWidget);
    m_leftSuperTrendPeriodSpin = new QSpinBox(m_leftSuperTrendWidget);
    m_leftSuperTrendPeriodSpin->setRange(1, 500);
    m_leftSuperTrendPeriodSpin->setValue(10);
    m_leftSuperTrendMultiplierSpin = new QDoubleSpinBox(m_leftSuperTrendWidget);
    m_leftSuperTrendMultiplierSpin->setRange(0.1, 100.0);
    m_leftSuperTrendMultiplierSpin->setSingleStep(0.1);
    m_leftSuperTrendMultiplierSpin->setValue(3.0);
    stLayout->addRow("Période ATR:", m_leftSuperTrendPeriodSpin);
    stLayout->addRow("Multiplicateur:", m_leftSuperTrendMultiplierSpin);
    m_leftSuperTrendWidget->setVisible(false);
    indicatorLayout->addWidget(m_leftSuperTrendWidget);
    
    m_leftIndicatorWidget->setVisible(false);
    leftLayout->addWidget(m_leftIndicatorWidget);
    
    // 3. Propriété bougie
    m_leftCandlePropertyWidget = new QWidget(parent);
    QFormLayout* candleLayout = new QFormLayout(m_leftCandlePropertyWidget);
    m_leftCandlePropertyCombo = new QComboBox(m_leftCandlePropertyWidget);
    m_leftCandlePropertyCombo->addItem("Bougie verte (Heikin Ashi)", static_cast<int>(CandlePropertyType::HEIKIN_ASHI_IS_GREEN));
    m_leftCandlePropertyCombo->addItem("Bougie rouge (Heikin Ashi)", static_cast<int>(CandlePropertyType::HEIKIN_ASHI_IS_RED));
    m_leftCandlePropertyCombo->addItem("Bougie verte", static_cast<int>(CandlePropertyType::IS_GREEN));
    m_leftCandlePropertyCombo->addItem("Bougie rouge", static_cast<int>(CandlePropertyType::IS_RED));
    m_leftCandlePropertyCombo->addItem("Taille du corps", static_cast<int>(CandlePropertyType::BODY_SIZE));
    m_leftCandlePropertyCombo->addItem("Ombre supérieure", static_cast<int>(CandlePropertyType::UPPER_SHADOW_SIZE));
    m_leftCandlePropertyCombo->addItem("Ombre inférieure", static_cast<int>(CandlePropertyType::LOWER_SHADOW_SIZE));
    m_leftCandlePropertyCombo->addItem("Amplitude", static_cast<int>(CandlePropertyType::RANGE));
    candleLayout->addRow("Propriété:", m_leftCandlePropertyCombo);
    m_leftCandlePropertyWidget->setVisible(false);
    leftLayout->addWidget(m_leftCandlePropertyWidget);
}

void FilterEditDialog::setupRightValueUI(QWidget* parent)
{
    QVBoxLayout* rightLayout = new QVBoxLayout(parent);
    
    // Type de catégorie (Prix, Indicateur, Constante, etc.)
    QFormLayout* categoryLayout = new QFormLayout();
    m_rightCategoryCombo = new QComboBox(parent);
    m_rightCategoryCombo->addItem("Prix", static_cast<int>(ValueCategory::PRICE));
    m_rightCategoryCombo->addItem("Indicateur", static_cast<int>(ValueCategory::INDICATOR));
    m_rightCategoryCombo->addItem("Constante", static_cast<int>(ValueCategory::CONSTANT));
    m_rightCategoryCombo->addItem("Propriété bougie", static_cast<int>(ValueCategory::CANDLE_PROPERTY));
    categoryLayout->addRow("Catégorie:", m_rightCategoryCombo);
    
    // Décalage historique commun à toutes les catégories sauf Constante
    m_rightHistoricalOffsetSpin = new QSpinBox(parent);
    m_rightHistoricalOffsetSpin->setRange(0, 100);
    m_rightHistoricalOffsetSpin->setValue(0);
    m_rightHistoricalOffsetSpin->setSuffix(" barres");
    categoryLayout->addRow("Décalage historique:", m_rightHistoricalOffsetSpin);
    
    rightLayout->addLayout(categoryLayout);
    
    // Widgets spécifiques à chaque type de catégorie
    
    // 1. Prix
    m_rightPriceWidget = new QWidget(parent);
    QFormLayout* priceLayout = new QFormLayout(m_rightPriceWidget);
    m_rightPriceTypeCombo = new QComboBox(m_rightPriceWidget);
    m_rightPriceTypeCombo->addItem("Clôture", static_cast<int>(PriceType::CLOSE));
    m_rightPriceTypeCombo->addItem("Ouverture", static_cast<int>(PriceType::OPEN));
    m_rightPriceTypeCombo->addItem("Plus haut", static_cast<int>(PriceType::HIGH));
    m_rightPriceTypeCombo->addItem("Plus bas", static_cast<int>(PriceType::LOW));
    m_rightPriceTypeCombo->addItem("Typique", static_cast<int>(PriceType::TYPICAL));
    m_rightPriceTypeCombo->addItem("Médian", static_cast<int>(PriceType::MEDIAN));
    priceLayout->addRow("Type de prix:", m_rightPriceTypeCombo);
    m_rightPriceWidget->setVisible(false);
    rightLayout->addWidget(m_rightPriceWidget);
    
    // 2. Indicateur (similaire à la partie gauche)
    m_rightIndicatorWidget = new QWidget(parent);
    QVBoxLayout* indicatorLayout = new QVBoxLayout(m_rightIndicatorWidget);
    
    QFormLayout* indicatorTypeLayout = new QFormLayout();
    m_rightIndicatorTypeCombo = new QComboBox(m_rightIndicatorWidget);
    m_rightIndicatorTypeCombo->addItem("EMA", static_cast<int>(IndicatorType::EMA));
    m_rightIndicatorTypeCombo->addItem("RSI", static_cast<int>(IndicatorType::RSI));
    m_rightIndicatorTypeCombo->addItem("Stochastique K", static_cast<int>(IndicatorType::STOCHASTIC_K));
    m_rightIndicatorTypeCombo->addItem("Stochastique D", static_cast<int>(IndicatorType::STOCHASTIC_D));
    m_rightIndicatorTypeCombo->addItem("ATR", static_cast<int>(IndicatorType::ATR));
    m_rightIndicatorTypeCombo->addItem("SuperTrend Valeur", static_cast<int>(IndicatorType::SUPERTREND_VALUE));
    m_rightIndicatorTypeCombo->addItem("SuperTrend Direction", static_cast<int>(IndicatorType::SUPERTREND_DIRECTION));
    indicatorTypeLayout->addRow("Type d'indicateur:", m_rightIndicatorTypeCombo);
    indicatorLayout->addLayout(indicatorTypeLayout);
    
    // Paramètres spécifiques à chaque indicateur (similaires à la partie gauche)
    // EMA
    m_rightEMAWidget = new QWidget(m_rightIndicatorWidget);
    QFormLayout* emaLayout = new QFormLayout(m_rightEMAWidget);
    m_rightEMAPeriodSpin = new QSpinBox(m_rightEMAWidget);
    m_rightEMAPeriodSpin->setRange(1, 500);
    m_rightEMAPeriodSpin->setValue(20);
    emaLayout->addRow("Période:", m_rightEMAPeriodSpin);
    m_rightEMAWidget->setVisible(false);
    indicatorLayout->addWidget(m_rightEMAWidget);
    
    // RSI
    m_rightRSIWidget = new QWidget(m_rightIndicatorWidget);
    QFormLayout* rsiLayout = new QFormLayout(m_rightRSIWidget);
    m_rightRSIPeriodSpin = new QSpinBox(m_rightRSIWidget);
    m_rightRSIPeriodSpin->setRange(1, 500);
    m_rightRSIPeriodSpin->setValue(14);
    rsiLayout->addRow("Période:", m_rightRSIPeriodSpin);
    m_rightRSIWidget->setVisible(false);
    indicatorLayout->addWidget(m_rightRSIWidget);
    
    // Stochastique
    m_rightStochasticWidget = new QWidget(m_rightIndicatorWidget);
    QFormLayout* stochLayout = new QFormLayout(m_rightStochasticWidget);
    m_rightStochFastKSpin = new QSpinBox(m_rightStochasticWidget);
    m_rightStochFastKSpin->setRange(1, 500);
    m_rightStochFastKSpin->setValue(14);
    m_rightStochSlowKSpin = new QSpinBox(m_rightStochasticWidget);
    m_rightStochSlowKSpin->setRange(1, 500);
    m_rightStochSlowKSpin->setValue(3);
    m_rightStochSlowDSpin = new QSpinBox(m_rightStochasticWidget);
    m_rightStochSlowDSpin->setRange(1, 500);
    m_rightStochSlowDSpin->setValue(3);
    stochLayout->addRow("Période FastK:", m_rightStochFastKSpin);
    stochLayout->addRow("Période SlowK:", m_rightStochSlowKSpin);
    stochLayout->addRow("Période SlowD:", m_rightStochSlowDSpin);
    m_rightStochasticWidget->setVisible(false);
    indicatorLayout->addWidget(m_rightStochasticWidget);
    
    // ATR
    m_rightATRWidget = new QWidget(m_rightIndicatorWidget);
    QFormLayout* atrLayout = new QFormLayout(m_rightATRWidget);
    m_rightATRPeriodSpin = new QSpinBox(m_rightATRWidget);
    m_rightATRPeriodSpin->setRange(1, 500);
    m_rightATRPeriodSpin->setValue(14);
    m_rightATRUseLogCheck = new QCheckBox("Utiliser ATR logarithmique", m_rightATRWidget);
    atrLayout->addRow("Période:", m_rightATRPeriodSpin);
    atrLayout->addRow(m_rightATRUseLogCheck);
    m_rightATRWidget->setVisible(false);
    indicatorLayout->addWidget(m_rightATRWidget);
    
    // SuperTrend
    m_rightSuperTrendWidget = new QWidget(m_rightIndicatorWidget);
    QFormLayout* stLayout = new QFormLayout(m_rightSuperTrendWidget);
    m_rightSuperTrendPeriodSpin = new QSpinBox(m_rightSuperTrendWidget);
    m_rightSuperTrendPeriodSpin->setRange(1, 500);
    m_rightSuperTrendPeriodSpin->setValue(10);
    m_rightSuperTrendMultiplierSpin = new QDoubleSpinBox(m_rightSuperTrendWidget);
    m_rightSuperTrendMultiplierSpin->setRange(0.1, 10.0);
    m_rightSuperTrendMultiplierSpin->setSingleStep(0.1);
    m_rightSuperTrendMultiplierSpin->setValue(3.0);
    stLayout->addRow("Période ATR:", m_rightSuperTrendPeriodSpin);
    stLayout->addRow("Multiplicateur:", m_rightSuperTrendMultiplierSpin);
    m_rightSuperTrendWidget->setVisible(false);
    indicatorLayout->addWidget(m_rightSuperTrendWidget);
    
    m_rightIndicatorWidget->setVisible(false);
    rightLayout->addWidget(m_rightIndicatorWidget);
    
    // 3. Constante
    m_rightConstantWidget = new QWidget(parent);
    QFormLayout* constantLayout = new QFormLayout(m_rightConstantWidget);
    m_rightConstantValueSpin = new QDoubleSpinBox(m_rightConstantWidget);
    m_rightConstantValueSpin->setRange(-100000, 100000);
    m_rightConstantValueSpin->setDecimals(2);
    m_rightConstantValueSpin->setValue(0);
    constantLayout->addRow("Valeur:", m_rightConstantValueSpin);
    m_rightConstantWidget->setVisible(false);
    rightLayout->addWidget(m_rightConstantWidget);
    
    // 4. Propriété bougie
    m_rightCandlePropertyWidget = new QWidget(parent);
    QFormLayout* candleLayout = new QFormLayout(m_rightCandlePropertyWidget);
    m_rightCandlePropertyCombo = new QComboBox(m_rightCandlePropertyWidget);
    m_rightCandlePropertyCombo->addItem("Bougie verte (Heikin Ashi)", static_cast<int>(CandlePropertyType::HEIKIN_ASHI_IS_GREEN));
    m_rightCandlePropertyCombo->addItem("Bougie rouge (Heikin Ashi)", static_cast<int>(CandlePropertyType::HEIKIN_ASHI_IS_RED));
    m_rightCandlePropertyCombo->addItem("Bougie verte", static_cast<int>(CandlePropertyType::IS_GREEN));
    m_rightCandlePropertyCombo->addItem("Bougie rouge", static_cast<int>(CandlePropertyType::IS_RED));
    m_rightCandlePropertyCombo->addItem("Taille du corps", static_cast<int>(CandlePropertyType::BODY_SIZE));
    m_rightCandlePropertyCombo->addItem("Ombre supérieure", static_cast<int>(CandlePropertyType::UPPER_SHADOW_SIZE));
    m_rightCandlePropertyCombo->addItem("Ombre inférieure", static_cast<int>(CandlePropertyType::LOWER_SHADOW_SIZE));
    m_rightCandlePropertyCombo->addItem("Amplitude", static_cast<int>(CandlePropertyType::RANGE));
    candleLayout->addRow("Propriété:", m_rightCandlePropertyCombo);
    m_rightCandlePropertyWidget->setVisible(false);
    rightLayout->addWidget(m_rightCandlePropertyWidget);
}

void FilterEditDialog::setupOperatorUI(QWidget* parent)
{
    QFormLayout* operatorLayout = new QFormLayout(parent);
    
    m_operatorCombo = new QComboBox(parent);
    m_operatorCombo->addItem(">", static_cast<int>(ComparisonOperator::GREATER_THAN));
    m_operatorCombo->addItem("<", static_cast<int>(ComparisonOperator::LESS_THAN));
    m_operatorCombo->addItem(">=", static_cast<int>(ComparisonOperator::GREATER_OR_EQUAL));
    m_operatorCombo->addItem("<=", static_cast<int>(ComparisonOperator::LESS_OR_EQUAL));
    m_operatorCombo->addItem("==", static_cast<int>(ComparisonOperator::EQUAL));
    m_operatorCombo->addItem("!=", static_cast<int>(ComparisonOperator::NOT_EQUAL));
    m_operatorCombo->addItem("Croise au-dessus", static_cast<int>(ComparisonOperator::CROSSES_ABOVE));
    m_operatorCombo->addItem("Croise en-dessous", static_cast<int>(ComparisonOperator::CROSSES_BELOW));
    
    operatorLayout->addRow("Opérateur:", m_operatorCombo);
}

void FilterEditDialog::setupTemporalLogicUI(QWidget* parent)
{
    QFormLayout* temporalLayout = new QFormLayout(parent);
    
    m_temporalLogicCombo = new QComboBox(parent);
    m_temporalLogicCombo->addItem("Actuel", static_cast<int>(TemporalLogic::CURRENT));
    m_temporalLogicCombo->addItem("Au moins une fois dans les N dernières barres", static_cast<int>(TemporalLogic::ANY_OF));
    m_temporalLogicCombo->addItem("Toutes les N dernières barres", static_cast<int>(TemporalLogic::ALL_OF));
    temporalLayout->addRow("Logique temporelle:", m_temporalLogicCombo);
    
    m_lookbackPeriodsSpin = new QSpinBox(parent);
    m_lookbackPeriodsSpin->setRange(1, 100);
    m_lookbackPeriodsSpin->setValue(1);
    temporalLayout->addRow("Barres à vérifier:", m_lookbackPeriodsSpin);
}

void FilterEditDialog::updateIndicatorParamsVisibility(QWidget* container, IndicatorType type)
{
    if (container == m_leftIndicatorWidget) {
        m_leftEMAWidget->setVisible(type == IndicatorType::EMA);
        m_leftRSIWidget->setVisible(type == IndicatorType::RSI);
        m_leftStochasticWidget->setVisible(type == IndicatorType::STOCHASTIC_K || type == IndicatorType::STOCHASTIC_D);
        m_leftATRWidget->setVisible(type == IndicatorType::ATR);
        m_leftSuperTrendWidget->setVisible(type == IndicatorType::SUPERTREND_VALUE || type == IndicatorType::SUPERTREND_DIRECTION);
    } else if (container == m_rightIndicatorWidget) {
        m_rightEMAWidget->setVisible(type == IndicatorType::EMA);
        m_rightRSIWidget->setVisible(type == IndicatorType::RSI);
        m_rightStochasticWidget->setVisible(type == IndicatorType::STOCHASTIC_K || type == IndicatorType::STOCHASTIC_D);
        m_rightATRWidget->setVisible(type == IndicatorType::ATR);
        m_rightSuperTrendWidget->setVisible(type == IndicatorType::SUPERTREND_VALUE || type == IndicatorType::SUPERTREND_DIRECTION);
    }
}

ValueSource FilterEditDialog::getLeftValueSource() const
{
    ValueSource source;
    source.historicalOffset = m_leftHistoricalOffsetSpin->value();
    source.category = static_cast<ValueCategory>(m_leftCategoryCombo->currentData().toInt());
    
    switch (source.category) {
        case ValueCategory::PRICE:
            source.priceType = static_cast<PriceType>(m_leftPriceTypeCombo->currentData().toInt());
            break;
        case ValueCategory::INDICATOR:
            source.indicatorType = static_cast<IndicatorType>(m_leftIndicatorTypeCombo->currentData().toInt());
            
            switch (source.indicatorType) {
                case IndicatorType::EMA:
                    source.emaParams = EMAParams(m_leftEMAPeriodSpin->value());
                    break;
                case IndicatorType::RSI:
                    source.rsiParams = RSIParams(m_leftRSIPeriodSpin->value());
                    break;
                case IndicatorType::STOCHASTIC_K:
                case IndicatorType::STOCHASTIC_D:
                    source.stochParams = StochasticParams(
                        m_leftStochFastKSpin->value(),
                        m_leftStochSlowKSpin->value(),
                        m_leftStochSlowDSpin->value()
                    );
                    break;
                case IndicatorType::ATR:
                    source.atrParams = ATRParams(
                        m_leftATRPeriodSpin->value(),
                        m_leftATRUseLogCheck->isChecked()
                    );
                    break;
                case IndicatorType::SUPERTREND_VALUE:
                case IndicatorType::SUPERTREND_DIRECTION:
                    source.supertrendParams = SuperTrendParams(
                        m_leftSuperTrendPeriodSpin->value(),
                        m_leftSuperTrendMultiplierSpin->value()
                    );
                    break;
                default:
                    break;
            }
            break;
        case ValueCategory::CANDLE_PROPERTY:
            source.candlePropertyType = static_cast<CandlePropertyType>(m_leftCandlePropertyCombo->currentData().toInt());
            break;
        default:
            break;
    }
    
    return source;
}

ValueSource FilterEditDialog::getRightValueSource() const
{
    ValueSource source;
    source.historicalOffset = m_rightHistoricalOffsetSpin->value();
    source.category = static_cast<ValueCategory>(m_rightCategoryCombo->currentData().toInt());
    
    switch (source.category) {
        case ValueCategory::PRICE:
            source.priceType = static_cast<PriceType>(m_rightPriceTypeCombo->currentData().toInt());
            break;
        case ValueCategory::INDICATOR:
            source.indicatorType = static_cast<IndicatorType>(m_rightIndicatorTypeCombo->currentData().toInt());
            
            switch (source.indicatorType) {
                case IndicatorType::EMA:
                    source.emaParams = EMAParams(m_rightEMAPeriodSpin->value());
                    break;
                case IndicatorType::RSI:
                    source.rsiParams = RSIParams(m_rightRSIPeriodSpin->value());
                    break;
                case IndicatorType::STOCHASTIC_K:
                case IndicatorType::STOCHASTIC_D:
                    source.stochParams = StochasticParams(
                        m_rightStochFastKSpin->value(),
                        m_rightStochSlowKSpin->value(),
                        m_rightStochSlowDSpin->value()
                    );
                    break;
                case IndicatorType::ATR:
                    source.atrParams = ATRParams(
                        m_rightATRPeriodSpin->value(),
                        m_rightATRUseLogCheck->isChecked()
                    );
                    break;
                case IndicatorType::SUPERTREND_VALUE:
                case IndicatorType::SUPERTREND_DIRECTION:
                    source.supertrendParams = SuperTrendParams(
                        m_rightSuperTrendPeriodSpin->value(),
                        m_rightSuperTrendMultiplierSpin->value()
                    );
                    break;
                default:
                    break;
            }
            break;
        case ValueCategory::CONSTANT:
            source.constantValue = m_rightConstantValueSpin->value();
            break;
        case ValueCategory::CANDLE_PROPERTY:
            source.candlePropertyType = static_cast<CandlePropertyType>(m_rightCandlePropertyCombo->currentData().toInt());
            break;
        default:
            break;
    }
    
    return source;
}

void FilterEditDialog::onLeftValueCategoryChanged(int index)
{
    ValueCategory category = static_cast<ValueCategory>(m_leftCategoryCombo->itemData(index).toInt());
    
    m_leftPriceWidget->setVisible(category == ValueCategory::PRICE);
    m_leftIndicatorWidget->setVisible(category == ValueCategory::INDICATOR);
    m_leftCandlePropertyWidget->setVisible(category == ValueCategory::CANDLE_PROPERTY);
    
    // Si c'est un indicateur, mettre à jour immédiatement la visibilité des paramètres
    if (category == ValueCategory::INDICATOR) {
        IndicatorType type = static_cast<IndicatorType>(m_leftIndicatorTypeCombo->currentData().toInt());
        updateIndicatorParamsVisibility(m_leftIndicatorWidget, type);
    }

    updatePreview();
}

void FilterEditDialog::onRightValueCategoryChanged(int index)
{
    ValueCategory category = static_cast<ValueCategory>(m_rightCategoryCombo->itemData(index).toInt());
    
    m_rightPriceWidget->setVisible(category == ValueCategory::PRICE);
    m_rightIndicatorWidget->setVisible(category == ValueCategory::INDICATOR);
    m_rightConstantWidget->setVisible(category == ValueCategory::CONSTANT);
    m_rightCandlePropertyWidget->setVisible(category == ValueCategory::CANDLE_PROPERTY);

    // Si c'est un indicateur, mettre à jour immédiatement la visibilité des paramètres
    if (category == ValueCategory::INDICATOR) {
        IndicatorType type = static_cast<IndicatorType>(m_rightIndicatorTypeCombo->currentData().toInt());
        updateIndicatorParamsVisibility(m_rightIndicatorWidget, type);
    }
    
    // Masquer/Afficher le décalage historique (non applicable pour les constantes)
    m_rightHistoricalOffsetSpin->setEnabled(category != ValueCategory::CONSTANT);
    
    updatePreview();
}

void FilterEditDialog::onLeftIndicatorTypeChanged(int index)
{
    IndicatorType type = static_cast<IndicatorType>(m_leftIndicatorTypeCombo->itemData(index).toInt());
    updateIndicatorParamsVisibility(m_leftIndicatorWidget, type);
    updatePreview();
}

void FilterEditDialog::onRightIndicatorTypeChanged(int index)
{
    IndicatorType type = static_cast<IndicatorType>(m_rightIndicatorTypeCombo->itemData(index).toInt());
    updateIndicatorParamsVisibility(m_rightIndicatorWidget, type);
    updatePreview();
}

void FilterEditDialog::setFilter(const GenericFilter& filter)
{
    m_filter = filter;
    
    // Configurer la partie gauche
    int leftCategoryIndex = m_leftCategoryCombo->findData(static_cast<int>(filter.leftValue.category));
    m_leftCategoryCombo->setCurrentIndex(leftCategoryIndex);
    m_leftHistoricalOffsetSpin->setValue(filter.leftValue.historicalOffset);
    
    switch (filter.leftValue.category) {
        case ValueCategory::PRICE:
            m_leftPriceTypeCombo->setCurrentIndex(m_leftPriceTypeCombo->findData(static_cast<int>(filter.leftValue.priceType)));
            break;
        case ValueCategory::INDICATOR:
            m_leftIndicatorTypeCombo->setCurrentIndex(m_leftIndicatorTypeCombo->findData(static_cast<int>(filter.leftValue.indicatorType)));
            
            switch (filter.leftValue.indicatorType) {
                case IndicatorType::EMA:
                    m_leftEMAPeriodSpin->setValue(filter.leftValue.emaParams.period);
                    break;
                case IndicatorType::RSI:
                    m_leftRSIPeriodSpin->setValue(filter.leftValue.rsiParams.period);
                    break;
                case IndicatorType::STOCHASTIC_K:
                case IndicatorType::STOCHASTIC_D:
                    m_leftStochFastKSpin->setValue(filter.leftValue.stochParams.fastK);
                    m_leftStochSlowKSpin->setValue(filter.leftValue.stochParams.slowK);
                    m_leftStochSlowDSpin->setValue(filter.leftValue.stochParams.slowD);
                    break;
                case IndicatorType::ATR:
                    m_leftATRPeriodSpin->setValue(filter.leftValue.atrParams.period);
                    m_leftATRUseLogCheck->setChecked(filter.leftValue.atrParams.useLog);
                    break;
                case IndicatorType::SUPERTREND_VALUE:
                case IndicatorType::SUPERTREND_DIRECTION:
                    m_leftSuperTrendPeriodSpin->setValue(filter.leftValue.supertrendParams.atrPeriod);
                    m_leftSuperTrendMultiplierSpin->setValue(filter.leftValue.supertrendParams.multiplier);
                    break;
                default:
                    break;
            }
            
            // Mettre à jour la visibilité des paramètres d'indicateurs
            updateIndicatorParamsVisibility(m_leftIndicatorWidget, filter.leftValue.indicatorType);
            break;
        case ValueCategory::CANDLE_PROPERTY:
            m_leftCandlePropertyCombo->setCurrentIndex(m_leftCandlePropertyCombo->findData(static_cast<int>(filter.leftValue.candlePropertyType)));
            break;
        default:
            break;
    }
    
    // Configurer l'opérateur
    m_operatorCombo->setCurrentIndex(m_operatorCombo->findData(static_cast<int>(filter.op)));
    
    // Configurer la partie droite
    int rightCategoryIndex = m_rightCategoryCombo->findData(static_cast<int>(filter.rightValue.category));
    m_rightCategoryCombo->setCurrentIndex(rightCategoryIndex);
    m_rightHistoricalOffsetSpin->setValue(filter.rightValue.historicalOffset);
    
    switch (filter.rightValue.category) {
        case ValueCategory::PRICE:
            m_rightPriceTypeCombo->setCurrentIndex(m_rightPriceTypeCombo->findData(static_cast<int>(filter.rightValue.priceType)));
            break;
        case ValueCategory::INDICATOR:
            m_rightIndicatorTypeCombo->setCurrentIndex(m_rightIndicatorTypeCombo->findData(static_cast<int>(filter.rightValue.indicatorType)));
            
            switch (filter.rightValue.indicatorType) {
                case IndicatorType::EMA:
                    m_rightEMAPeriodSpin->setValue(filter.rightValue.emaParams.period);
                    break;
                case IndicatorType::RSI:
                    m_rightRSIPeriodSpin->setValue(filter.rightValue.rsiParams.period);
                    break;
                case IndicatorType::STOCHASTIC_K:
                case IndicatorType::STOCHASTIC_D:
                    m_rightStochFastKSpin->setValue(filter.rightValue.stochParams.fastK);
                    m_rightStochSlowKSpin->setValue(filter.rightValue.stochParams.slowK);
                    m_rightStochSlowDSpin->setValue(filter.rightValue.stochParams.slowD);
                    break;
                case IndicatorType::ATR:
                    m_rightATRPeriodSpin->setValue(filter.rightValue.atrParams.period);
                    m_rightATRUseLogCheck->setChecked(filter.rightValue.atrParams.useLog);
                    break;
                case IndicatorType::SUPERTREND_VALUE:
                case IndicatorType::SUPERTREND_DIRECTION:
                    m_rightSuperTrendPeriodSpin->setValue(filter.rightValue.supertrendParams.atrPeriod);
                    m_rightSuperTrendMultiplierSpin->setValue(filter.rightValue.supertrendParams.multiplier);
                    break;
                default:
                    break;
            }
            
            // Mettre à jour la visibilité des paramètres d'indicateurs
            updateIndicatorParamsVisibility(m_rightIndicatorWidget, filter.rightValue.indicatorType);
            break;
        case ValueCategory::CONSTANT:
            m_rightConstantValueSpin->setValue(filter.rightValue.constantValue);
            break;
        case ValueCategory::CANDLE_PROPERTY:
            m_rightCandlePropertyCombo->setCurrentIndex(m_rightCandlePropertyCombo->findData(static_cast<int>(filter.rightValue.candlePropertyType)));
            break;
        default:
            break;
    }
    
    // Configurer la logique temporelle
    m_temporalLogicCombo->setCurrentIndex(m_temporalLogicCombo->findData(static_cast<int>(filter.temporalLogic)));
    m_lookbackPeriodsSpin->setValue(filter.lookbackPeriods);
    
    // Mettre à jour la visibilité
    onLeftValueCategoryChanged(leftCategoryIndex);
    onRightValueCategoryChanged(rightCategoryIndex);
    
    updatePreview();
}

void FilterEditDialog::updatePreview()
{    
    // Partie gauche
    ValueSource leftSource = getLeftValueSource();
    ComparisonOperator op = static_cast<ComparisonOperator>(m_operatorCombo->currentData().toInt());
    ValueSource rightSource = getRightValueSource();
    TemporalLogic tempLogic = static_cast<TemporalLogic>(m_temporalLogicCombo->currentData().toInt());
    int lookbackPeriods = m_lookbackPeriodsSpin->value();

    GenericFilter tempFilter;
    tempFilter.leftValue = leftSource;
    tempFilter.rightValue = rightSource;
    tempFilter.op = op;
    tempFilter.temporalLogic = tempLogic;
    tempFilter.lookbackPeriods = lookbackPeriods;
    
    m_previewLabel->setText(tempFilter.autoGenerateDescription().c_str());
}

void FilterEditDialog::onOkButtonClicked()
{
    // Construire le filtre à partir des valeurs actuelles des widgets
    m_filter.leftValue = getLeftValueSource();
    m_filter.rightValue = getRightValueSource();
    m_filter.op = static_cast<ComparisonOperator>(m_operatorCombo->currentData().toInt());
    m_filter.temporalLogic = static_cast<TemporalLogic>(m_temporalLogicCombo->currentData().toInt());
    m_filter.lookbackPeriods = m_lookbackPeriodsSpin->value();
    m_filter.enabled = true;
    m_filter.description = m_filter.autoGenerateDescription();
    
    accept();
}

void FilterEditDialog::onCancelButtonClicked()
{
    reject();
}

GenericFilter FilterEditDialog::getFilter() const
{
    return m_filter;
}