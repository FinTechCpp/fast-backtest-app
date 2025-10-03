#include "ui/chart/chartControlPanel.h"
#include <QFrame>
#include <QDebug>

#include "ui/dialogs/rsiDialog.h"
#include "ui/dialogs/emaDialog.h"
#include "ui/dialogs/supertrendDialog.h"
#include "ui/dialogs/stochasticDialog.h"
#include "ui/dialogs/atrDialog.h"
#include "ui/dialogs/pivotPointsDialog.h"
#include "ui/dialogs/cciDialog.h"

ChartControlPanel::ChartControlPanel(QWidget* parent)
    : QWidget(parent)
    , m_settingsTitle(nullptr)
    , m_chartTypeCombo(nullptr)
    , m_rulerToolButton(nullptr)
    , m_chartWidget(nullptr)
{
    setupUI();
}

ChartControlPanel::~ChartControlPanel()
{
}

void ChartControlPanel::setChartWidget(ChartWidget* chartWidget)
{
    m_chartWidget = chartWidget;

    if (m_chartWidget) {
        connect(m_chartWidget, &ChartWidget::indicatorAdded, this, &ChartControlPanel::onIndicatorAdded);
        connect(m_chartWidget, &ChartWidget::indicatorChanged, this, &ChartControlPanel::onIndicatorChanged);
        connect(m_chartWidget, &ChartWidget::indicatorRemoved, this, &ChartControlPanel::onIndicatorRemoved);
        connect(m_chartWidget, &ChartWidget::maxDisplayPointsChanged, this, [this](int value) {
            if (m_aggregationSlider->value() != value) {
                m_aggregationSlider->setValue(value);
                m_aggregationLabel->setText(QString::number(value));
            }
        });
    }
}

void ChartControlPanel::setupUI()
{
    setObjectName("leftPanel");
    setStyleSheet("#leftPanel { background-color: #BADDFF; }");
    setFixedWidth(230); // Largeur fixe pour le panneau

    QVBoxLayout* leftPanelLayout = new QVBoxLayout(this);
    leftPanelLayout->setContentsMargins(5, 8, 5, 8);
    leftPanelLayout->setSpacing(10);

    // Titre du panneau
    m_settingsTitle = new QLabel("Settings");
    m_settingsTitle->setAlignment(Qt::AlignCenter);
    m_settingsTitle->setStyleSheet("font-weight: bold; font-size: 16px;");
    leftPanelLayout->addWidget(m_settingsTitle);

    // Sélecteur de type de graphique
    QLabel* chartTypeLabel = new QLabel("Chart Type");
    chartTypeLabel->setStyleSheet("font-weight: bold;");
    leftPanelLayout->addWidget(chartTypeLabel);
    
    m_chartTypeCombo = new QComboBox();
    m_chartTypeCombo->addItem("CandleStick", "CandleStick");
    m_chartTypeCombo->addItem("Heikin Ashi", "HeikinAshi");
    m_chartTypeCombo->addItem("Line", "Close");
    m_chartTypeCombo->addItem("OHLC", "OHLC");
    leftPanelLayout->addWidget(m_chartTypeCombo);

    QFrame* separator = new QFrame();
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Sunken);
    leftPanelLayout->addWidget(separator);

    // Contrôle du seuil d'agrégation
    QLabel* aggregationTitle = new QLabel("Seuil d'Agrégation");
    aggregationTitle->setStyleSheet("font-weight: bold;");
    leftPanelLayout->addWidget(aggregationTitle);

    // Layout pour le slider et l'étiquette
    QHBoxLayout* sliderLayout = new QHBoxLayout();

    // Slider d'agrégation
    int aggregation_default_value = 30000;
    m_aggregationSlider = new QSlider(Qt::Horizontal);
    m_aggregationSlider->setMinimum(1000);
    m_aggregationSlider->setMaximum(100000);
    m_aggregationSlider->setValue(aggregation_default_value);
    m_aggregationSlider->setTickInterval(10000);
    m_aggregationSlider->setTickPosition(QSlider::TicksBelow);

    // Étiquette de valeur
    m_aggregationLabel = new QLabel(QString::number(aggregation_default_value));
    m_aggregationLabel->setMinimumWidth(50);

    sliderLayout->addWidget(m_aggregationSlider);
    sliderLayout->addWidget(m_aggregationLabel);
    leftPanelLayout->addLayout(sliderLayout);
    
    // Description de l'agrégation
    QLabel* aggregationDesc = new QLabel("Ajuste le nombre maximum de points à afficher avant agrégation");
    aggregationDesc->setWordWrap(true);
    aggregationDesc->setStyleSheet("font-size: 9px; color: #666;");
    leftPanelLayout->addWidget(aggregationDesc);

    connect(m_aggregationSlider, &QSlider::valueChanged, this, &ChartControlPanel::onAggregationSliderChanged);
    connect(m_chartTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ChartControlPanel::onChartTypeChanged);

    // Bouton outil règle
    m_rulerToolButton = new QToolButton();
    m_rulerToolButton->setIcon(QIcon(":/icons/ruler_unchecked.png"));
    m_rulerToolButton->setIconSize(QSize(32, 32));
    m_rulerToolButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_rulerToolButton->setCheckable(true);
    m_rulerToolButton->setStyleSheet(
        "QToolButton {"
        "    padding: 4px;"
        "    border: 1px solid #999;"
        "    border-radius: 4px;"
        "    background-color: white;"
        "}"
        "QToolButton:checked {"
        "    background-color:rgb(0, 141, 0);"
        "}"
    );

    connect(m_rulerToolButton, &QToolButton::toggled, this, &ChartControlPanel::onRulerToolToggled);
    connect(m_rulerToolButton, &QToolButton::toggled, [this](bool checked) {
        if (checked) {
            m_rulerToolButton->setIcon(QIcon(":/icons/ruler_checked.png"));
        } else {
            m_rulerToolButton->setIcon(QIcon(":/icons/ruler_unchecked.png"));
        }
    });
    
    leftPanelLayout->addWidget(m_rulerToolButton);

    // Layout horizontal pour les boutons de comparaison
    m_comparisonButtonsLayout = new QHBoxLayout();
    m_comparisonButtonsLayout->setSpacing(5);
    
    // Bouton pour transférer les données vers le graphique de comparaison
    m_transferDataButton = new QPushButton("Copier");
    m_transferDataButton->setIcon(QIcon::fromTheme("edit-copy"));
    m_transferDataButton->setStyleSheet(
        "QPushButton {"
        "    padding: 6px;"
        "    border: 1px solid #999;"
        "    border-radius: 4px;"
        "    background-color: white;"
        "}"
    );
    m_comparisonButtonsLayout->addWidget(m_transferDataButton, 1); // Stretch ratio = 1
    
    // Bouton pour quitter le mode comparaison
    m_exitComparisonButton = new QPushButton("");
    m_exitComparisonButton->setIcon(QIcon::fromTheme("window-close"));
    m_exitComparisonButton->setFixedWidth(30);
    m_exitComparisonButton->setToolTip("Quitter le mode comparaison");
    m_exitComparisonButton->setStyleSheet(
        "QPushButton {"
        "    padding: 4px;"
        "    border: 1px solid #999;"
        "    border-radius: 4px;"
        "    background-color: white;"
        "    color: red;"
        "}"
    );
    m_exitComparisonButton->setVisible(false); // Initialement caché
    m_comparisonButtonsLayout->addWidget(m_exitComparisonButton, 0); // Stretch ratio = 0

    leftPanelLayout->addLayout(m_comparisonButtonsLayout);
    
    connect(m_transferDataButton, &QPushButton::clicked, this, &ChartControlPanel::onTransferDataClicked);
    connect(m_exitComparisonButton, &QPushButton::clicked, this, &ChartControlPanel::onExitComparisonClicked);

    // Section des indicateurs techniques
    QLabel* indicatorsLabel = new QLabel("Technical Indicators");
    indicatorsLabel->setStyleSheet("font-weight: bold; margin-top: 10px;");
    leftPanelLayout->addWidget(indicatorsLabel);

    setupIndicatorControls();
    leftPanelLayout->addWidget(m_indicatorsGroup);
    
    // Ajouter la section des suggestions
    QLabel* suggestionsLabel = new QLabel("Suggestions (Stratégie)");
    suggestionsLabel->setStyleSheet("font-weight: bold; margin-top: 10px;");
    leftPanelLayout->addWidget(suggestionsLabel);
    
    // Créer le groupe de suggestions
    m_suggestionsGroup = new QGroupBox("Indicateurs suggérés");
    QVBoxLayout* suggestionsGroupLayout = new QVBoxLayout(m_suggestionsGroup);
    
    // Layout pour les suggestions
    m_suggestionsLayout = new QVBoxLayout();
    suggestionsGroupLayout->addLayout(m_suggestionsLayout);
    
    // Bouton pour effacer toutes les suggestions
    m_clearSuggestionsButton = new QPushButton("Clear");
    // m_clearSuggestionsButton->setStyleSheet("font-size: 10px;");
    connect(m_clearSuggestionsButton, &QPushButton::clicked, this, &ChartControlPanel::onClearAllSuggestions);
    suggestionsGroupLayout->addWidget(m_clearSuggestionsButton);
    
    leftPanelLayout->addWidget(m_suggestionsGroup);
    m_suggestionsGroup->setVisible(false); // Initialement caché

    leftPanelLayout->addStretch();
}

void ChartControlPanel::suggestIndicatorsFromStrategy(const std::vector<std::unique_ptr<indicators::IndicatorBase>>& indicators) {
    // Effacer proprement les anciennes suggestions
    onClearAllSuggestions();
    
    // Si aucune suggestion, masquer la section et sortir
    if (indicators.empty()) {
        m_suggestionsGroup->setVisible(false);
        return;
    }
    
    // Pour chaque indicateur suggéré
    for (const auto& indicator : indicators) {
        // Vérifier si un indicateur similaire existe déjà
        if (!hasSimilarIndicator(indicator.get())) {
            // Créer une copie de l'indicateur
            std::unique_ptr<indicators::IndicatorBase> clonedIndicator = indicator->clone();
            
            // Créer un widget de suggestion et l'ajouter à la liste
            createSuggestionWidget(std::move(clonedIndicator));
        }
    }
    
    // Afficher la section des suggestions s'il y en a
    m_suggestionsGroup->setVisible(!m_suggestions.empty());
}

void ChartControlPanel::createSuggestionWidget(std::unique_ptr<indicators::IndicatorBase> indicator) {
    // Créer le widget et son layout
    QWidget* suggestionWidget = new QWidget();
    QHBoxLayout* layout = new QHBoxLayout(suggestionWidget);
    layout->setContentsMargins(0, 2, 0, 2);
    
    // Label avec nom de l'indicateur
    QLabel* nameLabel = new QLabel(indicator->getDisplayName());
    nameLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    nameLabel->setStyleSheet("color: #0066cc;");
    
    // Créer l'élément de suggestion et stocker l'index
    SuggestionItem item;
    item.widget = suggestionWidget;
    item.label = nameLabel;
    item.indicator = std::move(indicator);
    int newIndex = static_cast<int>(m_suggestions.size());
    
    // Bouton pour ajouter l'indicateur
    QPushButton* addButton = new QPushButton("→");
    addButton->setFixedSize(QSize(25, 25));
    addButton->setToolTip("Ajouter au graphique");
    connect(addButton, &QPushButton::clicked, [this, newIndex]() {
        if (newIndex < static_cast<int>(m_suggestions.size())) {
            this->onAddSuggestedIndicator(newIndex);
        }
    });
    item.addButton = addButton;
    
    // Bouton pour rejeter la suggestion
    QPushButton* rejectButton = new QPushButton("×");
    rejectButton->setFixedSize(QSize(25, 25));
    rejectButton->setToolTip("Ignorer cette suggestion");
    connect(rejectButton, &QPushButton::clicked, [this, newIndex]() {
        if (newIndex < static_cast<int>(m_suggestions.size())) {
            this->onRejectSuggestion(newIndex);
        }
    });
    item.rejectButton = rejectButton;
    
    // Assembler le widget
    layout->addWidget(nameLabel);
    layout->addWidget(addButton);
    layout->addWidget(rejectButton);
    
    // Ajouter le widget au layout
    m_suggestionsLayout->addWidget(suggestionWidget);
    
    // Ajouter l'élément à notre liste
    m_suggestions.push_back(std::move(item));
}


void ChartControlPanel::onAddSuggestedIndicator(int index) {
    // Vérification de sécurité
    if (index < 0 || index >= static_cast<int>(m_suggestions.size()) || !m_chartWidget)
        return;
    
    const indicators::IndicatorBase* indicator = m_suggestions[index].indicator.get();
    if (!indicator)
        return;
    
    // Ajouter l'indicateur au graphique selon son type
    if (const indicators::RSIInstance* rsi = dynamic_cast<const indicators::RSIInstance*>(indicator)) {
        m_chartWidget->addIndicator(*rsi);
    } else if (const indicators::EMAInstance* ema = dynamic_cast<const indicators::EMAInstance*>(indicator)) {
        m_chartWidget->addIndicator(*ema);
    } else if (const indicators::StochasticInstance* stoch = dynamic_cast<const indicators::StochasticInstance*>(indicator)) {
        m_chartWidget->addIndicator(*stoch);
    } else if (const indicators::ATRInstance* atr = dynamic_cast<const indicators::ATRInstance*>(indicator)) {
        m_chartWidget->addIndicator(*atr);
    } else if (const indicators::SuperTrendInstance* supertrend = dynamic_cast<const indicators::SuperTrendInstance*>(indicator)) {
        m_chartWidget->addIndicator(*supertrend);
    } else if (const indicators::PivotPointsInstance* pivotPoints = dynamic_cast<const indicators::PivotPointsInstance*>(indicator)) {
        m_chartWidget->addIndicator(*pivotPoints);
    } else if (const indicators::CCIInstance* cci = dynamic_cast<const indicators::CCIInstance*>(indicator)) {
        m_chartWidget->addIndicator(*cci);
    }
    
    // Rejeter la suggestion après l'avoir ajoutée
    onRejectSuggestion(index);
}

void ChartControlPanel::onRejectSuggestion(int index) {
    // Vérification de sécurité
    if (index < 0 || index >= static_cast<int>(m_suggestions.size()))
        return;
    
    // Récupérer l'élément à supprimer
    QWidget* widgetToRemove = m_suggestions[index].widget;
    
    // Supprimer du layout
    if (widgetToRemove) {
        m_suggestionsLayout->removeWidget(widgetToRemove);
        widgetToRemove->disconnect(); // Déconnecter tous les signaux
        widgetToRemove->deleteLater(); // Utiliser deleteLater au lieu de delete direct
    }
    
    // Supprimer l'élément de la liste
    m_suggestions.erase(m_suggestions.begin() + index);
    
    // Mettre à jour les indices dans les lambdas pour les éléments restants
    for (int i = index; i < static_cast<int>(m_suggestions.size()); ++i) {
        // Déconnecter les anciennes connexions
        if (m_suggestions[static_cast<size_t>(i)].addButton) {
            m_suggestions[static_cast<size_t>(i)].addButton->disconnect();
            int captureIndex = i;
            connect(m_suggestions[static_cast<size_t>(i)].addButton, &QPushButton::clicked, [this, captureIndex]() {
                this->onAddSuggestedIndicator(captureIndex);
            });
        }

        if (m_suggestions[static_cast<size_t>(i)].rejectButton) {
            m_suggestions[static_cast<size_t>(i)].rejectButton->disconnect();
            int captureIndex = i;
            connect(m_suggestions[static_cast<size_t>(i)].rejectButton, &QPushButton::clicked, [this, captureIndex]() {
                this->onRejectSuggestion(captureIndex);
            });
        }
    }
    
    // Cacher le groupe s'il n'y a plus de suggestions
    if (m_suggestions.empty()) {
        m_suggestionsGroup->setVisible(false);
    }
}

void ChartControlPanel::onClearAllSuggestions() {
    // Utiliser la méthode sécurisée pour supprimer tous les widgets
    while (!m_suggestions.empty()) {
        // Toujours supprimer le premier élément jusqu'à ce que la liste soit vide
        onRejectSuggestion(0);
    }
    
    // Double vérification pour assurer que tout est nettoyé
    QLayoutItem* child;
    while ((child = m_suggestionsLayout->takeAt(0)) != nullptr) {
        if (child->widget()) {
            child->widget()->disconnect();
            child->widget()->deleteLater();
        }
        delete child;
    }
    
    // S'assurer que la liste est bien vide
    m_suggestions.clear();
    
    // Cacher le groupe
    m_suggestionsGroup->setVisible(false);
}

bool ChartControlPanel::hasSimilarIndicator(const indicators::IndicatorBase* indicator) const {
    if (!m_chartWidget) return false;
    
    const std::vector<std::unique_ptr<indicators::IndicatorBase>>& existingIndicators = m_chartWidget->getIndicators();
    
    // Vérifier chaque indicateur existant
    for (const auto& existingIndicator : existingIndicators) {
        if (existingIndicator->isCalculationParamsEqual(*indicator)) {
            return true;
        }
    }
    
    return false;
}


void ChartControlPanel::setupIndicatorControls() {
    m_indicatorsGroup = new QGroupBox("Active Indicators");
    QVBoxLayout* groupLayout = new QVBoxLayout(m_indicatorsGroup);

    m_indicatorsLayout = new QVBoxLayout();
    groupLayout->addLayout(m_indicatorsLayout);

    QHBoxLayout* addIndicatorLayout = new QHBoxLayout();
    
    m_addIndicatorButton = new QPushButton("Add");
    m_addIndicatorButton->setFixedWidth(50);
    
    m_indicatorTypeCombo = new QComboBox();
    m_indicatorTypeCombo->addItem("RSI", static_cast<int>(indicators::Type::RSI));
    m_indicatorTypeCombo->addItem("EMA", static_cast<int>(indicators::Type::EMA));
    m_indicatorTypeCombo->addItem("Supertrend", static_cast<int>(indicators::Type::SUPERTREND));
    m_indicatorTypeCombo->addItem("Stochastic", static_cast<int>(indicators::Type::STOCHASTIC));
    m_indicatorTypeCombo->addItem("ATR", static_cast<int>(indicators::Type::ATR));
    m_indicatorTypeCombo->addItem("Points Pivots", static_cast<int>(indicators::Type::PIVOTPOINTS));
    m_indicatorTypeCombo->addItem("CCI", static_cast<int>(indicators::Type::CCI));

    addIndicatorLayout->addWidget(m_indicatorTypeCombo);
    addIndicatorLayout->addWidget(m_addIndicatorButton);
    
    groupLayout->addLayout(addIndicatorLayout);

    connect(m_addIndicatorButton, &QPushButton::clicked, this, &ChartControlPanel::onAddIndicatorClicked);
    connect(m_indicatorTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &ChartControlPanel::onIndicatorTypeSelected);
}

void ChartControlPanel::onIndicatorTypeSelected(int index) {
    Q_UNUSED(index);
}

void ChartControlPanel::onAddIndicatorClicked() {
    if (!m_chartWidget) return;
    
    int indicatorType = m_indicatorTypeCombo->currentData().toInt();

    if (indicatorType == static_cast<int>(indicators::Type::RSI)) {
        indicators::RSIInstance rsi;
        m_chartWidget->addIndicator(std::move(rsi));
    }
    else if (indicatorType == static_cast<int>(indicators::Type::EMA)) {
        indicators::EMAInstance ema;
        m_chartWidget->addIndicator(std::move(ema));
    }
    else if (indicatorType == static_cast<int>(indicators::Type::SUPERTREND)) {
        indicators::SuperTrendInstance supertrend;
        m_chartWidget->addIndicator(std::move(supertrend));
    }
    else if (indicatorType == static_cast<int>(indicators::Type::STOCHASTIC)) {
        indicators::StochasticInstance stochastic;
        m_chartWidget->addIndicator(std::move(stochastic));
    }
    else if (indicatorType == static_cast<int>(indicators::Type::ATR)) {
        indicators::ATRInstance atr;
        m_chartWidget->addIndicator(std::move(atr));
    }
    else if (indicatorType == static_cast<int>(indicators::Type::PIVOTPOINTS)) {
        indicators::PivotPointsInstance pivotPoints;
        m_chartWidget->addIndicator(std::move(pivotPoints));
    } else if (indicatorType == static_cast<int>(indicators::Type::CCI)) {
        indicators::CCIInstance cci;
        m_chartWidget->addIndicator(std::move(cci));
    }
}

void ChartControlPanel::createIndicatorWidgets(int id, const QString &name) {
    QWidget* indicatorWidget = new QWidget();
    QHBoxLayout* layout = new QHBoxLayout(indicatorWidget);
    layout->setContentsMargins(0, 2, 0, 2);

    QLabel* nameLabel = new QLabel(name);
    nameLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    QPushButton* editButton = new QPushButton("Edit");
    editButton->setFixedWidth(40);
    connect(editButton, &QPushButton::clicked, [this, id]() {
        this->onEditIndicator(id);
    });
    
    QPushButton* removeButton = new QPushButton("X");
    removeButton->setFixedWidth(20);
    connect(removeButton, &QPushButton::clicked, [this, id]() {
        this->onRemoveIndicator(id);
    });

    layout->addWidget(nameLabel);
    layout->addWidget(editButton);
    layout->addWidget(removeButton);

    m_indicatorLabels[id] = nameLabel;
    m_editButtons[id] = editButton;
    m_removeButtons[id] = removeButton;

    m_indicatorsLayout->addWidget(indicatorWidget);
}

void ChartControlPanel::onEditIndicator(int id) {
    if (!m_chartWidget) return;

    if (tryOpenDialog<indicators::RSIInstance, RSIDialog>(id)) return;
    if (tryOpenDialog<indicators::EMAInstance, EMADialog>(id)) return;
    if (tryOpenDialog<indicators::SuperTrendInstance, SupertrendDialog>(id)) return;
    if (tryOpenDialog<indicators::StochasticInstance, StochasticDialog>(id)) return;
    if (tryOpenDialog<indicators::ATRInstance, ATRDialog>(id)) return;
    if (tryOpenDialog<indicators::PivotPointsInstance, PivotPointsDialog>(id)) return;
    if (tryOpenDialog<indicators::CCIInstance, CCIDialog>(id)) return;
}

void ChartControlPanel::onRemoveIndicator(int id) {
    if (m_chartWidget) {
        m_chartWidget->removeIndicator(id);
    }
}

void ChartControlPanel::refreshIndicatorsList() {
    if (!m_chartWidget) return;

    QLayoutItem* child;
    while ((child = m_indicatorsLayout->takeAt(0)) != nullptr) {
        if (child->widget())
            delete child->widget();
        delete child;
    }

    m_indicatorLabels.clear();
    m_editButtons.clear();
    m_removeButtons.clear();

    const std::vector<std::unique_ptr<indicators::IndicatorBase>>& allIndicators = m_chartWidget->getIndicators();
    for (const auto& indicator : allIndicators) {
        if (indicator->visible) {
            createIndicatorWidgets(indicator->id, indicator->getDisplayName());
        }
    }
}

void ChartControlPanel::onChartTypeChanged(int index) {
    if (!m_chartTypeCombo) return;

    QVariant data = m_chartTypeCombo->itemData(index);
    if (data.isValid()) {
        QString chartType = data.toString();
        emit chartTypeChanged(chartType);
    }
}

void ChartControlPanel::onRulerToolToggled(bool checked) {
    emit rulerToolToggled(checked);
}

void ChartControlPanel::onTransferDataClicked() {
    emit transferDataForComparison();
}

void ChartControlPanel::onExitComparisonClicked() {
    emit exitComparisonMode();
}

void ChartControlPanel::onAggregationSliderChanged(int value) {
    m_aggregationLabel->setText(QString::number(value));
    emit aggregationValueChanged(value);
}

void ChartControlPanel::onIndicatorAdded(int id, const QString &name) {
    createIndicatorWidgets(id, name);
}

void ChartControlPanel::onIndicatorChanged(int id, const QString &name) {
    if (m_indicatorLabels.contains(id)) {
        m_indicatorLabels[id]->setText(name);
    }
}

void ChartControlPanel::onIndicatorRemoved(int id) {
    refreshIndicatorsList();
}

void ChartControlPanel::configureIndicatorInstances(const std::vector<std::unique_ptr<indicators::IndicatorBase>>& indicators) {
    if (!m_chartWidget) return;

    m_chartWidget->removeAllIndicators();

    for (const auto& indicator : indicators) {
        if (const indicators::RSIInstance* rsi = dynamic_cast<const indicators::RSIInstance*>(indicator.get())) {
            m_chartWidget->addIndicator(*rsi);
        } else if (const indicators::EMAInstance* ema = dynamic_cast<const indicators::EMAInstance*>(indicator.get())) {
            m_chartWidget->addIndicator(*ema);
        } else if (const indicators::StochasticInstance* stoch = dynamic_cast<const indicators::StochasticInstance*>(indicator.get())) {
            m_chartWidget->addIndicator(*stoch);
        } else if (const indicators::ATRInstance* atr = dynamic_cast<const indicators::ATRInstance*>(indicator.get())) {
            m_chartWidget->addIndicator(*atr);
        } else if (const indicators::SuperTrendInstance* supertrend = dynamic_cast<const indicators::SuperTrendInstance*>(indicator.get())) {
            m_chartWidget->addIndicator(*supertrend);
        } else if (const indicators::PivotPointsInstance* pivotPoints = dynamic_cast<const indicators::PivotPointsInstance*>(indicator.get())) {
            m_chartWidget->addIndicator(*pivotPoints);
        } else if (const indicators::CCIInstance* cci = dynamic_cast<const indicators::CCIInstance*>(indicator.get())) {
            m_chartWidget->addIndicator(*cci);
        }
    }
}

void ChartControlPanel::setComparisonMode(bool enabled) {
    m_comparisonActive = enabled;
    m_exitComparisonButton->setVisible(enabled);
    
    if (enabled) {
        m_transferDataButton->setText("Actualiser");
    } else {
        m_transferDataButton->setText("Copier");
    }
}