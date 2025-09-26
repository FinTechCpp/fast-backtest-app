#include "ui/dialogs/StrategyCreationDialog.h"
#include <QApplication>

// FilterConfigWidget Implementation
FilterConfigWidget::FilterConfigWidget(QWidget* parent)
    : QWidget(parent)
    , m_leftParamWidget(nullptr)
    , m_rightParamWidget(nullptr)
{
    setupUI();
}

FilterConfigWidget::FilterConfigWidget(const GenericFilter& filter, QWidget* parent)
    : QWidget(parent)
    , m_leftParamWidget(nullptr)
    , m_rightParamWidget(nullptr)
{
    setupUI();
    setFilter(filter);
}

void FilterConfigWidget::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Description
    QFormLayout* descLayout = new QFormLayout();
    m_descriptionEdit = new QLineEdit(this);
    m_descriptionEdit->setPlaceholderText("Description du filtre (optionnel)");
    descLayout->addRow("Description:", m_descriptionEdit);
    mainLayout->addLayout(descLayout);

    // Left Value Source
    QGroupBox* leftGroup = new QGroupBox("Valeur de gauche", this);
    QFormLayout* leftLayout = new QFormLayout(leftGroup);
    setupValueSourceWidgets(leftLayout, "left", m_leftCategoryCombo, m_leftTypeCombo,
                           m_leftParamWidget, m_leftOffsetSpin, m_leftConstantSpin);
    mainLayout->addWidget(leftGroup);

    // Comparison Operator
    QGroupBox* compGroup = new QGroupBox("Comparaison", this);
    QFormLayout* compLayout = new QFormLayout(compGroup);
    
    m_comparisonCombo = new QComboBox(this);
    m_comparisonCombo->addItems({
        "Supérieur à (>)",
        "Inférieur à (<)", 
        "Supérieur ou égal (>=)",
        "Inférieur ou égal (<=)",
        "Égal à (=)",
        "Différent de (≠)",
        "Croise au-dessus",
        "Croise en-dessous"
    });
    compLayout->addRow("Opérateur:", m_comparisonCombo);

    m_temporalLogicCombo = new QComboBox(this);
    m_temporalLogicCombo->addItems({
        "Actuel seulement",
        "Au moins une fois dans les dernières périodes", 
        "Toutes les dernières périodes"
    });
    compLayout->addRow("Logique temporelle:", m_temporalLogicCombo);

    m_lookbackPeriodsSpin = new QSpinBox(this);
    m_lookbackPeriodsSpin->setRange(1, 100);
    m_lookbackPeriodsSpin->setValue(1);
    m_lookbackPeriodsSpin->setEnabled(false);
    compLayout->addRow("Nombre de périodes:", m_lookbackPeriodsSpin);

    // Enable lookback when temporal logic changes
    connect(m_temporalLogicCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            [this](int index) {
                m_lookbackPeriodsSpin->setEnabled(index > 0);
            });

    mainLayout->addWidget(compGroup);

    // Right Value Source  
    QGroupBox* rightGroup = new QGroupBox("Valeur de droite", this);
    QFormLayout* rightLayout = new QFormLayout(rightGroup);
    setupValueSourceWidgets(rightLayout, "right", m_rightCategoryCombo, m_rightTypeCombo,
                           m_rightParamWidget, m_rightOffsetSpin, m_rightConstantSpin);
    mainLayout->addWidget(rightGroup);

    // Enabled checkbox
    m_enabledCheck = new QCheckBox("Filtre activé", this);
    m_enabledCheck->setChecked(true);
    mainLayout->addWidget(m_enabledCheck);

    // Connect signals
    connect(m_leftCategoryCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &FilterConfigWidget::onLeftValueCategoryChanged);
    connect(m_rightCategoryCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &FilterConfigWidget::onRightValueCategoryChanged);
    connect(m_leftTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &FilterConfigWidget::onLeftIndicatorTypeChanged);
    connect(m_rightTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &FilterConfigWidget::onRightIndicatorTypeChanged);
}

void FilterConfigWidget::setupValueSourceWidgets(QFormLayout* layout, const QString& prefix,
                                                 QComboBox*& categoryCombo,
                                                 QComboBox*& typeCombo,
                                                 QWidget*& paramWidget,
                                                 QSpinBox*& offsetSpin,
                                                 QDoubleSpinBox*& constantSpin)
{
    // Category selection
    categoryCombo = new QComboBox(this);
    categoryCombo->addItems({"Prix", "Indicateur", "Constante", "Propriété de bougie"});
    layout->addRow("Type de valeur:", categoryCombo);

    // Type selection (will be populated based on category)
    typeCombo = new QComboBox(this);
    layout->addRow("Sous-type:", typeCombo);

    // Parameter widget placeholder
    paramWidget = new QWidget(this);
    layout->addRow("Paramètres:", paramWidget);

    // Historical offset
    offsetSpin = new QSpinBox(this);
    offsetSpin->setRange(0, 100);
    offsetSpin->setValue(0);
    offsetSpin->setToolTip("0 = valeur actuelle, 1 = valeur précédente, etc.");
    layout->addRow("Décalage historique:", offsetSpin);

    // Constant value (only visible when category is CONSTANT)
    constantSpin = new QDoubleSpinBox(this);
    constantSpin->setRange(-1000000, 1000000);
    constantSpin->setDecimals(4);
    constantSpin->setValue(0.0);
    constantSpin->setVisible(false);
    layout->addRow("Valeur constante:", constantSpin);
}

void FilterConfigWidget::onLeftValueCategoryChanged()
{
    updateParameterWidgets(m_leftCategoryCombo, m_leftTypeCombo, m_leftParamWidget, true);
    m_leftConstantSpin->setVisible(m_leftCategoryCombo->currentIndex() == 2); // CONSTANT
}

void FilterConfigWidget::onRightValueCategoryChanged() 
{
    updateParameterWidgets(m_rightCategoryCombo, m_rightTypeCombo, m_rightParamWidget, false);
    m_rightConstantSpin->setVisible(m_rightCategoryCombo->currentIndex() == 2); // CONSTANT
}

void FilterConfigWidget::onLeftIndicatorTypeChanged()
{
    updateIndicatorParameterWidgets(m_leftTypeCombo, m_leftParamWidget, true);
}

void FilterConfigWidget::onRightIndicatorTypeChanged()
{
    updateIndicatorParameterWidgets(m_rightTypeCombo, m_rightParamWidget, false);
}

void FilterConfigWidget::updateParameterWidgets(QComboBox* categoryCombo, QComboBox* typeCombo,
                                               QWidget*& paramWidget, bool isLeft)
{
    // Temporarily disconnect signals to avoid recursion
    typeCombo->blockSignals(true);
    
    // Clear existing parameter widget
    if (paramWidget) {
        // Clear all child widgets first
        QLayout* oldLayout = paramWidget->layout();
        if (oldLayout) {
            QLayoutItem* item;
            while ((item = oldLayout->takeAt(0)) != nullptr) {
                if (item->widget()) {
                    item->widget()->deleteLater();
                }
                delete item;
            }
            delete oldLayout;
        }
    } else {
        paramWidget = new QWidget(this);
    }

    QFormLayout* paramLayout = new QFormLayout(paramWidget);
    paramLayout->setContentsMargins(0, 0, 0, 0);

    // Update type combo based on category
    typeCombo->clear();
    
    int category = categoryCombo->currentIndex();
    switch (category) {
        case 0: // PRICE
            typeCombo->addItems({"Close", "Open", "High", "Low", "Typical", "Median"});
            break;
            
        case 1: // INDICATOR
            typeCombo->addItems({
                "EMA", "RSI", "Stochastic K", "Stochastic D", 
                "ATR", "SuperTrend Value", "SuperTrend Direction", "Pivot Point"
            });
            
            // Add parameter fields - we'll do this after setting the current index
            break;
            
        case 2: // CONSTANT
            // No type selection needed for constants
            typeCombo->addItems({"Valeur constante"});
            break;
            
        case 3: // CANDLE_PROPERTY
            typeCombo->addItems({
                "Heikin Ashi vert", "Heikin Ashi rouge", "Bougie verte", "Bougie rouge",
                "Taille du corps", "Taille ombre haute", "Taille ombre basse", "Range"
            });
            break;
    }
    
    // Re-enable signals
    typeCombo->blockSignals(false);
    
    // Now add parameter fields for indicators
    if (category == 1 && typeCombo->count() > 0) { // INDICATOR
        int indicatorType = typeCombo->currentIndex();
        
        if (indicatorType == 0) { // EMA
            QSpinBox* periodSpin = new QSpinBox(paramWidget);
            periodSpin->setRange(1, 500);
            periodSpin->setValue(20);
            periodSpin->setObjectName(isLeft ? "leftEmaPeriod" : "rightEmaPeriod");
            paramLayout->addRow("Période:", periodSpin);
        }
        else if (indicatorType == 1) { // RSI
            QSpinBox* periodSpin = new QSpinBox(paramWidget);
            periodSpin->setRange(2, 100);
            periodSpin->setValue(14);
            periodSpin->setObjectName(isLeft ? "leftRsiPeriod" : "rightRsiPeriod");
            paramLayout->addRow("Période:", periodSpin);
        }
        else if (indicatorType == 2 || indicatorType == 3) { // Stochastic
            QSpinBox* fastKSpin = new QSpinBox(paramWidget);
            fastKSpin->setRange(1, 100);
            fastKSpin->setValue(14);
            fastKSpin->setObjectName(isLeft ? "leftStochFastK" : "rightStochFastK");
            paramLayout->addRow("Fast K:", fastKSpin);
            
            QSpinBox* slowKSpin = new QSpinBox(paramWidget);
            slowKSpin->setRange(1, 100);
            slowKSpin->setValue(3);
            slowKSpin->setObjectName(isLeft ? "leftStochSlowK" : "rightStochSlowK");
            paramLayout->addRow("Slow K:", slowKSpin);
            
            QSpinBox* slowDSpin = new QSpinBox(paramWidget);
            slowDSpin->setRange(1, 100);
            slowDSpin->setValue(3);
            slowDSpin->setObjectName(isLeft ? "leftStochSlowD" : "rightStochSlowD");
            paramLayout->addRow("Slow D:", slowDSpin);
        }
        else if (indicatorType == 4) { // ATR
            QSpinBox* periodSpin = new QSpinBox(paramWidget);
            periodSpin->setRange(1, 100);
            periodSpin->setValue(14);
            periodSpin->setObjectName(isLeft ? "leftAtrPeriod" : "rightAtrPeriod");
            paramLayout->addRow("Période:", periodSpin);
            
            QCheckBox* useLogCheck = new QCheckBox("Utiliser log", paramWidget);
            useLogCheck->setObjectName(isLeft ? "leftAtrUseLog" : "rightAtrUseLog");
            paramLayout->addRow("Options:", useLogCheck);
        }
        else if (indicatorType == 5 || indicatorType == 6) { // SuperTrend
            QSpinBox* atrPeriodSpin = new QSpinBox(paramWidget);
            atrPeriodSpin->setRange(1, 100);
            atrPeriodSpin->setValue(10);
            atrPeriodSpin->setObjectName(isLeft ? "leftSupertrendAtrPeriod" : "rightSupertrendAtrPeriod");
            paramLayout->addRow("Période ATR:", atrPeriodSpin);
            
            QDoubleSpinBox* multiplierSpin = new QDoubleSpinBox(paramWidget);
            multiplierSpin->setRange(0.1, 10.0);
            multiplierSpin->setDecimals(1);
            multiplierSpin->setValue(3.0);
            multiplierSpin->setObjectName(isLeft ? "leftSupertrendMultiplier" : "rightSupertrendMultiplier");
            paramLayout->addRow("Multiplicateur:", multiplierSpin);
        }
    }
}

void FilterConfigWidget::updateIndicatorParameterWidgets(QComboBox* typeCombo, QWidget*& paramWidget, bool isLeft)
{
    // Clear existing parameter widget
    if (paramWidget) {
        // Clear all child widgets first
        QLayout* oldLayout = paramWidget->layout();
        if (oldLayout) {
            QLayoutItem* item;
            while ((item = oldLayout->takeAt(0)) != nullptr) {
                if (item->widget()) {
                    item->widget()->deleteLater();
                }
                delete item;
            }
            delete oldLayout;
        }
    } else {
        paramWidget = new QWidget(this);
    }

    QFormLayout* paramLayout = new QFormLayout(paramWidget);
    paramLayout->setContentsMargins(0, 0, 0, 0);

    // Add parameter fields based on current indicator type
    int indicatorType = typeCombo->currentIndex();
    
    if (indicatorType == 0) { // EMA
        QSpinBox* periodSpin = new QSpinBox(paramWidget);
        periodSpin->setRange(1, 500);
        periodSpin->setValue(20);
        periodSpin->setObjectName(isLeft ? "leftEmaPeriod" : "rightEmaPeriod");
        paramLayout->addRow("Période:", periodSpin);
    }
    else if (indicatorType == 1) { // RSI
        QSpinBox* periodSpin = new QSpinBox(paramWidget);
        periodSpin->setRange(2, 100);
        periodSpin->setValue(14);
        periodSpin->setObjectName(isLeft ? "leftRsiPeriod" : "rightRsiPeriod");
        paramLayout->addRow("Période:", periodSpin);
    }
    else if (indicatorType == 2 || indicatorType == 3) { // Stochastic
        QSpinBox* fastKSpin = new QSpinBox(paramWidget);
        fastKSpin->setRange(1, 100);
        fastKSpin->setValue(14);
        fastKSpin->setObjectName(isLeft ? "leftStochFastK" : "rightStochFastK");
        paramLayout->addRow("Fast K:", fastKSpin);
        
        QSpinBox* slowKSpin = new QSpinBox(paramWidget);
        slowKSpin->setRange(1, 100);
        slowKSpin->setValue(3);
        slowKSpin->setObjectName(isLeft ? "leftStochSlowK" : "rightStochSlowK");
        paramLayout->addRow("Slow K:", slowKSpin);
        
        QSpinBox* slowDSpin = new QSpinBox(paramWidget);
        slowDSpin->setRange(1, 100);
        slowDSpin->setValue(3);
        slowDSpin->setObjectName(isLeft ? "leftStochSlowD" : "rightStochSlowD");
        paramLayout->addRow("Slow D:", slowDSpin);
    }
    else if (indicatorType == 4) { // ATR
        QSpinBox* periodSpin = new QSpinBox(paramWidget);
        periodSpin->setRange(1, 100);
        periodSpin->setValue(14);
        periodSpin->setObjectName(isLeft ? "leftAtrPeriod" : "rightAtrPeriod");
        paramLayout->addRow("Période:", periodSpin);
        
        QCheckBox* useLogCheck = new QCheckBox("Utiliser log", paramWidget);
        useLogCheck->setObjectName(isLeft ? "leftAtrUseLog" : "rightAtrUseLog");
        paramLayout->addRow("Options:", useLogCheck);
    }
    else if (indicatorType == 5 || indicatorType == 6) { // SuperTrend
        QSpinBox* atrPeriodSpin = new QSpinBox(paramWidget);
        atrPeriodSpin->setRange(1, 100);
        atrPeriodSpin->setValue(10);
        atrPeriodSpin->setObjectName(isLeft ? "leftSupertrendAtrPeriod" : "rightSupertrendAtrPeriod");
        paramLayout->addRow("Période ATR:", atrPeriodSpin);
        
        QDoubleSpinBox* multiplierSpin = new QDoubleSpinBox(paramWidget);
        multiplierSpin->setRange(0.1, 10.0);
        multiplierSpin->setDecimals(1);
        multiplierSpin->setValue(3.0);
        multiplierSpin->setObjectName(isLeft ? "leftSupertrendMultiplier" : "rightSupertrendMultiplier");
        paramLayout->addRow("Multiplicateur:", multiplierSpin);
    }
    else if (indicatorType == 7) { // Pivot Point
        // Pivot Point doesn't need additional parameters
        QLabel* infoLabel = new QLabel("Aucun paramètre requis", paramWidget);
        infoLabel->setStyleSheet("color: #888; font-style: italic;");
        paramLayout->addRow(infoLabel);
    }
}

GenericFilter FilterConfigWidget::getFilter() const
{
    GenericFilter filter;
    
    // Get left and right values
    filter.leftValue = getValueSourceFromWidgets(m_leftCategoryCombo, m_leftTypeCombo,
                                                m_leftParamWidget, m_leftOffsetSpin, m_leftConstantSpin);
    filter.rightValue = getValueSourceFromWidgets(m_rightCategoryCombo, m_rightTypeCombo,
                                                 m_rightParamWidget, m_rightOffsetSpin, m_rightConstantSpin);
    
    // Get comparison operator
    filter.op = static_cast<ComparisonOperator>(m_comparisonCombo->currentIndex());
    
    // Get temporal logic
    filter.temporalLogic = static_cast<TemporalLogic>(m_temporalLogicCombo->currentIndex());
    filter.lookbackPeriods = m_lookbackPeriodsSpin->value();
    
    // Other properties
    filter.enabled = m_enabledCheck->isChecked();
    filter.description = m_descriptionEdit->text().toStdString();
    
    // Auto-generate description if empty
    if (filter.description.empty()) {
        filter.description = filter.autoGenerateDescription();
    }
    
    return filter;
}

ValueSource FilterConfigWidget::getValueSourceFromWidgets(QComboBox* categoryCombo, QComboBox* typeCombo,
                                                         QWidget* paramWidget, QSpinBox* offsetSpin,
                                                         QDoubleSpinBox* constantSpin) const
{
    ValueSource source;
    source.category = static_cast<ValueCategory>(categoryCombo->currentIndex());
    source.historicalOffset = offsetSpin->value();
    
    switch (source.category) {
        case ValueCategory::PRICE:
            source.priceType = static_cast<PriceType>(typeCombo->currentIndex());
            break;
            
        case ValueCategory::INDICATOR: {
            source.indicatorType = static_cast<IndicatorType>(typeCombo->currentIndex());
            
            // Get parameters from paramWidget
            if (source.indicatorType == IndicatorType::EMA) {
                QSpinBox* periodSpin = paramWidget->findChild<QSpinBox*>("*EmaPeriod");
                if (periodSpin) {
                    source.emaParams = EMAParams{periodSpin->value()};
                }
            }
            else if (source.indicatorType == IndicatorType::RSI) {
                QSpinBox* periodSpin = paramWidget->findChild<QSpinBox*>("*RsiPeriod");
                if (periodSpin) {
                    source.rsiParams = RSIParams{periodSpin->value()};
                }
            }
            else if (source.indicatorType == IndicatorType::STOCHASTIC_K || 
                     source.indicatorType == IndicatorType::STOCHASTIC_D) {
                QSpinBox* fastKSpin = paramWidget->findChild<QSpinBox*>("*StochFastK");
                QSpinBox* slowKSpin = paramWidget->findChild<QSpinBox*>("*StochSlowK");
                QSpinBox* slowDSpin = paramWidget->findChild<QSpinBox*>("*StochSlowD");
                if (fastKSpin && slowKSpin && slowDSpin) {
                    source.stochParams = StochasticParams{fastKSpin->value(), slowKSpin->value(), slowDSpin->value()};
                }
            }
            else if (source.indicatorType == IndicatorType::ATR) {
                QSpinBox* periodSpin = paramWidget->findChild<QSpinBox*>("*AtrPeriod");
                QCheckBox* useLogCheck = paramWidget->findChild<QCheckBox*>("*AtrUseLog");
                if (periodSpin) {
                    source.atrParams = ATRParams{periodSpin->value(), useLogCheck ? useLogCheck->isChecked() : false};
                }
            }
            else if (source.indicatorType == IndicatorType::SUPERTREND_VALUE ||
                     source.indicatorType == IndicatorType::SUPERTREND_DIRECTION) {
                QSpinBox* atrPeriodSpin = paramWidget->findChild<QSpinBox*>("*SupertrendAtrPeriod");
                QDoubleSpinBox* multiplierSpin = paramWidget->findChild<QDoubleSpinBox*>("*SupertrendMultiplier");
                if (atrPeriodSpin && multiplierSpin) {
                    source.supertrendParams = SuperTrendParams{atrPeriodSpin->value(), multiplierSpin->value()};
                }
            }
            break;
        }
        
        case ValueCategory::CONSTANT:
            source.constantValue = constantSpin->value();
            break;
            
        case ValueCategory::CANDLE_PROPERTY:
            source.candlePropertyType = static_cast<CandlePropertyType>(typeCombo->currentIndex());
            break;
    }
    
    return source;
}

void FilterConfigWidget::setFilter(const GenericFilter& filter)
{
    // Set description
    m_descriptionEdit->setText(QString::fromStdString(filter.description));
    
    // Set comparison operator
    m_comparisonCombo->setCurrentIndex(static_cast<int>(filter.op));
    
    // Set temporal logic
    m_temporalLogicCombo->setCurrentIndex(static_cast<int>(filter.temporalLogic));
    m_lookbackPeriodsSpin->setValue(filter.lookbackPeriods);
    
    // Set enabled
    m_enabledCheck->setChecked(filter.enabled);
    
    // Set value sources
    setValueSourceToWidgets(filter.leftValue, m_leftCategoryCombo, m_leftTypeCombo,
                           m_leftParamWidget, m_leftOffsetSpin, m_leftConstantSpin);
    setValueSourceToWidgets(filter.rightValue, m_rightCategoryCombo, m_rightTypeCombo,
                           m_rightParamWidget, m_rightOffsetSpin, m_rightConstantSpin);
}

void FilterConfigWidget::setValueSourceToWidgets(const ValueSource& source, QComboBox* categoryCombo,
                                                 QComboBox* typeCombo, QWidget*& paramWidget,
                                                 QSpinBox* offsetSpin, QDoubleSpinBox* constantSpin)
{
    // Set category
    categoryCombo->setCurrentIndex(static_cast<int>(source.category));
    
    // Set historical offset
    offsetSpin->setValue(source.historicalOffset);
    
    // Set constant value if applicable
    constantSpin->setValue(source.constantValue);
    constantSpin->setVisible(source.category == ValueCategory::CONSTANT);
    
    // Update parameter widgets first
    updateParameterWidgets(categoryCombo, typeCombo, paramWidget, 
                          categoryCombo == m_leftCategoryCombo);
    
    // Set type based on category
    switch (source.category) {
        case ValueCategory::PRICE:
            typeCombo->setCurrentIndex(static_cast<int>(source.priceType));
            break;
            
        case ValueCategory::INDICATOR:
            typeCombo->setCurrentIndex(static_cast<int>(source.indicatorType));
            // Set parameters based on indicator type
            if (source.indicatorType == IndicatorType::EMA) {
                QSpinBox* periodSpin = paramWidget->findChild<QSpinBox*>("*EmaPeriod");
                if (periodSpin) periodSpin->setValue(source.emaParams.period);
            }
            // Add similar code for other indicators...
            break;
            
        case ValueCategory::CANDLE_PROPERTY:
            typeCombo->setCurrentIndex(static_cast<int>(source.candlePropertyType));
            break;
            
        case ValueCategory::CONSTANT:
            typeCombo->setCurrentIndex(0);
            break;
    }
}

// StrategyCreationDialog Implementation
StrategyCreationDialog::StrategyCreationDialog(QWidget* parent)
    : QDialog(parent)
    , m_currentFilterWidget(nullptr)
    , m_editMode(false)
    , m_currentFilterIndex(-1)
{
    setWindowTitle("Créer une nouvelle stratégie");
    setModal(true);
    resize(800, 600);
    setupUI();
}

StrategyCreationDialog::StrategyCreationDialog(const GenericStrategyConfig& existingConfig, QWidget* parent)
    : QDialog(parent)
    , m_currentFilterWidget(nullptr)
    , m_editMode(true)
    , m_currentFilterIndex(-1)
{
    setWindowTitle("Modifier la stratégie");
    setModal(true);
    resize(800, 600);
    setupUI();
    
    // Load existing configuration
    m_strategyNameEdit->setText(QString::fromStdString(existingConfig.name));
    if (existingConfig.go_direction.has_value()) {
        m_directionCombo->setCurrentIndex(existingConfig.go_direction.value() ? 0 : 1);
    } else {
        m_directionCombo->setCurrentIndex(2);
    }
    
    m_filters = existingConfig.filters;
    updateFilterList();
}

void StrategyCreationDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Strategy basic info
    QGroupBox* basicInfoGroup = new QGroupBox("Informations de base", this);
    QFormLayout* basicLayout = new QFormLayout(basicInfoGroup);
    
    m_strategyNameEdit = new QLineEdit(this);
    m_strategyNameEdit->setPlaceholderText("Nom de la stratégie");
    basicLayout->addRow("Nom:", m_strategyNameEdit);
    
    m_directionCombo = new QComboBox(this);
    m_directionCombo->addItems({"Long seulement", "Short seulement", "Long et Short"});
    m_directionCombo->setCurrentIndex(2);
    basicLayout->addRow("Direction:", m_directionCombo);
    
    mainLayout->addWidget(basicInfoGroup);
    
    // Filters section
    QGroupBox* filtersGroup = new QGroupBox("Filtres de la stratégie", this);
    QHBoxLayout* filtersLayout = new QHBoxLayout(filtersGroup);
    
    // Left side - filters list
    QVBoxLayout* listLayout = new QVBoxLayout();
    
    QLabel* listLabel = new QLabel("Liste des filtres:", this);
    listLayout->addWidget(listLabel);
    
    m_filtersList = new QListWidget(this);
    m_filtersList->setMaximumWidth(300);
    listLayout->addWidget(m_filtersList);
    
    // Buttons for filter management
    QHBoxLayout* filterButtonsLayout = new QHBoxLayout();
    m_addFilterBtn = new QPushButton("Ajouter", this);
    m_removeFilterBtn = new QPushButton("Supprimer", this);
    m_removeFilterBtn->setEnabled(false);
    filterButtonsLayout->addWidget(m_addFilterBtn);
    filterButtonsLayout->addWidget(m_removeFilterBtn);
    listLayout->addLayout(filterButtonsLayout);
    
    filtersLayout->addLayout(listLayout);
    
    // Right side - filter details
    QVBoxLayout* detailsLayout = new QVBoxLayout();
    
    QLabel* detailsLabel = new QLabel("Configuration du filtre:", this);
    detailsLayout->addWidget(detailsLabel);
    
    m_filterDetailsArea = new QScrollArea(this);
    m_filterDetailsArea->setWidgetResizable(true);
    m_filterDetailsArea->setMinimumHeight(400);
    detailsLayout->addWidget(m_filterDetailsArea);
    
    filtersLayout->addLayout(detailsLayout);
    
    mainLayout->addWidget(filtersGroup);
    
    // Dialog buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    
    m_cancelBtn = new QPushButton("Annuler", this);
    m_okBtn = new QPushButton("OK", this);
    m_okBtn->setDefault(true);
    
    buttonLayout->addWidget(m_cancelBtn);
    buttonLayout->addWidget(m_okBtn);
    mainLayout->addLayout(buttonLayout);
    
    // Connect signals
    connect(m_addFilterBtn, &QPushButton::clicked, this, &StrategyCreationDialog::onAddFilter);
    connect(m_removeFilterBtn, &QPushButton::clicked, this, &StrategyCreationDialog::onRemoveFilter);
    connect(m_filtersList, &QListWidget::currentRowChanged, this, &StrategyCreationDialog::onFilterSelectionChanged);
    connect(m_okBtn, &QPushButton::clicked, this, &StrategyCreationDialog::accept);
    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

void StrategyCreationDialog::onAddFilter()
{
    // Create a new filter with default values
    GenericFilter newFilter;
    newFilter.leftValue = ValueSource::Price(PriceType::CLOSE);
    newFilter.rightValue = ValueSource::Constant(50.0);
    newFilter.op = ComparisonOperator::GREATER_THAN;
    newFilter.temporalLogic = TemporalLogic::CURRENT;
    newFilter.lookbackPeriods = 1;
    newFilter.enabled = true;
    newFilter.description = "Nouveau filtre";
    
    m_filters.push_back(newFilter);
    updateFilterList();
    
    // Select the newly added filter
    m_filtersList->setCurrentRow(m_filters.size() - 1);
}

void StrategyCreationDialog::onRemoveFilter()
{
    int currentRow = m_filtersList->currentRow();
    if (currentRow >= 0 && currentRow < static_cast<int>(m_filters.size())) {
        m_filters.erase(m_filters.begin() + currentRow);
        updateFilterList();
        
        // Clear filter details if no filters left
        if (m_filters.empty()) {
            m_filterDetailsArea->setWidget(nullptr);
            if (m_currentFilterWidget) {
                m_currentFilterWidget->deleteLater();
                m_currentFilterWidget = nullptr;
            }
        }
    }
}

void StrategyCreationDialog::onFilterSelectionChanged()
{
    int currentRow = m_filtersList->currentRow();
    m_removeFilterBtn->setEnabled(currentRow >= 0);
    
    // Save previous filter if exists
    if (m_currentFilterWidget && m_currentFilterIndex >= 0 && 
        m_currentFilterIndex < static_cast<int>(m_filters.size())) {
        m_filters[m_currentFilterIndex] = m_currentFilterWidget->getFilter();
    }
    
    if (currentRow >= 0 && currentRow < static_cast<int>(m_filters.size())) {
        // Create new filter widget for selected filter
        if (m_currentFilterWidget) {
            m_currentFilterWidget->deleteLater();
        }
        
        m_currentFilterWidget = new FilterConfigWidget(m_filters[currentRow], this);
        m_filterDetailsArea->setWidget(m_currentFilterWidget);
        m_currentFilterIndex = currentRow;
    } else {
        // No valid selection
        if (m_currentFilterWidget) {
            m_currentFilterWidget->deleteLater();
            m_currentFilterWidget = nullptr;
        }
        m_filterDetailsArea->setWidget(nullptr);
        m_currentFilterIndex = -1;
    }
}

void StrategyCreationDialog::updateFilterList()
{
    m_filtersList->clear();
    
    for (size_t i = 0; i < m_filters.size(); ++i) {
        QString itemText = QString("Filtre %1: %2")
                             .arg(i + 1)
                             .arg(QString::fromStdString(m_filters[i].description));
        if (!m_filters[i].enabled) {
            itemText += " (désactivé)";
        }
        
        QListWidgetItem* item = new QListWidgetItem(itemText);
        if (!m_filters[i].enabled) {
            item->setForeground(QColor::fromRgb(128, 128, 128));
        }
        
        m_filtersList->addItem(item);
    }
}

void StrategyCreationDialog::accept()
{
    // Validate strategy name
    if (m_strategyNameEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "Erreur", "Veuillez saisir un nom pour la stratégie.");
        m_strategyNameEdit->setFocus();
        return;
    }
    
    // Save current filter if exists
    if (m_currentFilterWidget && m_currentFilterIndex >= 0 && 
        m_currentFilterIndex < static_cast<int>(m_filters.size())) {
        m_filters[m_currentFilterIndex] = m_currentFilterWidget->getFilter();
    }
    
    QDialog::accept();
}

GenericStrategyConfig StrategyCreationDialog::getStrategyConfig() const
{
    GenericStrategyConfig config;
    
    config.name = m_strategyNameEdit->text().toStdString();
    
    // Set direction
    int directionIndex = m_directionCombo->currentIndex();
    if (directionIndex == 0) {
        config.go_direction = true;  // Long only
    } else if (directionIndex == 1) {
        config.go_direction = false; // Short only
    } else {
        config.go_direction = std::nullopt; // Both
    }
    
    // Set filters
    config.filters = m_filters;
    
    return config;
}