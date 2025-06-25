#include "views/chart_view.h"
#include "app.h"
#include <QDebug>
#include <QTime>
#include <QLabel>
#include <QFrame>
#include <QResizeEvent>
#include <QToolButton>
#include "views/chart_view.h"

ChartView::ChartView(QWidget* parent)
    : BaseView(parent)
    , m_cachedResults(nullptr)
    , m_dataExtracted(false)
    , m_currentResults(nullptr)
    , m_chartPlaceholder(nullptr)
    , m_chartTypeCombo(nullptr)
    , m_settingsTitle(nullptr)
    , m_chartWidget(nullptr)
    , m_leftPanel(nullptr)
    , m_app(nullptr)
{
    // Trouver l'application parente
    QWidget* widget = parent;
    while (widget && !m_app) {
        m_app = qobject_cast<App*>(widget);
        widget = widget->parentWidget();
    }
    
    // Construire l'interface
    setupUI();
}

ChartView::~ChartView()
{
}

void ChartView::setupUI()
{
    // Configurer le layout principal pour occuper tout l'espace
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);
    
    // Créer un layout horizontal pour les panneaux gauche et droit
    QHBoxLayout* horizontalLayout = new QHBoxLayout();
    horizontalLayout->setContentsMargins(0, 0, 0, 0);
    horizontalLayout->setSpacing(0);
    m_mainLayout->addLayout(horizontalLayout);
    
    // Créer le panneau gauche avec une largeur fixe
    m_leftPanel = new QWidget();
    m_leftPanel->setObjectName("leftPanel");
    m_leftPanel->setStyleSheet("#leftPanel { background-color: #BADDFF; }");
    m_leftPanel->setFixedWidth(230); // Augmenter la largeur pour les contrôles d'indicateurs
    
    // Ajouter un layout vertical au panneau gauche
    QVBoxLayout* leftPanelLayout = new QVBoxLayout(m_leftPanel);
    leftPanelLayout->setContentsMargins(5, 8, 5, 8);
    leftPanelLayout->setSpacing(10);
    
    // Ajouter un titre au panneau gauche
    m_settingsTitle = new QLabel("Settings");
    m_settingsTitle->setAlignment(Qt::AlignCenter);
    m_settingsTitle->setStyleSheet("font-weight: bold; font-size: 16px;");
    leftPanelLayout->addWidget(m_settingsTitle);
    
    // Ajouter le sélecteur de type de graphique
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
    
    // Ajouter le contrôle de seuil d'agrégation
    QLabel* aggregationTitle = new QLabel("Seuil d'Agrégation");
    aggregationTitle->setStyleSheet("font-weight: bold;");
    leftPanelLayout->addWidget(aggregationTitle);
    
    // Layout pour le slider et l'étiquette de valeur
    QHBoxLayout* sliderLayout = new QHBoxLayout();
    
    // Créer le slider
    int aggregation_default_value = 30000; 
    m_aggregationSlider = new QSlider(Qt::Horizontal);
    m_aggregationSlider->setMinimum(1000);   // Minimum 
    m_aggregationSlider->setMaximum(100000); // Maximum 
    m_aggregationSlider->setValue(aggregation_default_value); // Valeur par défaut 
    m_aggregationSlider->setTickInterval(10000);
    m_aggregationSlider->setTickPosition(QSlider::TicksBelow);
    
    // Créer l'étiquette de valeur
    m_aggregationLabel = new QLabel(QString::number(aggregation_default_value));
    m_aggregationLabel->setMinimumWidth(50);
    
    // Ajouter les widgets au layout
    sliderLayout->addWidget(m_aggregationSlider);
    sliderLayout->addWidget(m_aggregationLabel);
    
    // Ajouter le layout au panneau gauche
    leftPanelLayout->addLayout(sliderLayout);
    
    // Ajouter une description
    QLabel* aggregationDesc = new QLabel("Ajuste le nombre maximum de points à afficher avant agrégation");
    aggregationDesc->setWordWrap(true);
    aggregationDesc->setStyleSheet("font-size: 9px; color: #666;");
    leftPanelLayout->addWidget(aggregationDesc);

    connect(m_aggregationSlider, &QSlider::valueChanged, this, &ChartView::onAggregationSliderChanged);
    
    // Connecter le signal de changement à notre slot
    connect(m_chartTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ChartView::onChartTypeChanged);
    
    // Remplacer la case à cocher par un QToolButton stylisé
    QToolButton* rulerToolButton = new QToolButton();
    rulerToolButton->setIcon(QIcon(":/icons/ruler_unchecked.png"));
    rulerToolButton->setIconSize(QSize(32, 32));
    rulerToolButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    rulerToolButton->setCheckable(true);
    rulerToolButton->setStyleSheet(
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
    
    // Connecter le signal de changement du bouton
    connect(rulerToolButton, &QToolButton::toggled, this, &ChartView::onRulerToolToggled);
    
    // Connecter le signal pour changer l'icône quand l'état change
    connect(rulerToolButton, &QToolButton::toggled, [rulerToolButton](bool checked) {
        if (checked) {
            rulerToolButton->setIcon(QIcon(":/icons/ruler_checked.png"));
        } else {
            rulerToolButton->setIcon(QIcon(":/icons/ruler_unchecked.png"));
        }
    });
    
    leftPanelLayout->addWidget(rulerToolButton);
    
    // Section des indicateurs techniques
    QLabel* indicatorsLabel = new QLabel("Technical Indicators");
    indicatorsLabel->setStyleSheet("font-weight: bold; margin-top: 10px;");
    leftPanelLayout->addWidget(indicatorsLabel);
    
    // Configuration des contrôles d'indicateurs
    setupIndicatorControls();
    leftPanelLayout->addWidget(m_indicatorsGroup);
    
    // Ajouter un espace extensible en bas
    leftPanelLayout->addStretch();
    
    // Créer un séparateur vertical
    QFrame* verticalSeparator = new QFrame();
    verticalSeparator->setFrameStyle(QFrame::VLine | QFrame::Plain);
    verticalSeparator->setStyleSheet("color: #CCCCCC;"); // Couleur de la ligne

    // Créer le panneau droit qui contiendra le graphique
    m_rightPanel = new QWidget();
    m_rightPanel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    
    // Layout pour le panneau droit
    QVBoxLayout* rightPanelLayout = new QVBoxLayout(m_rightPanel);
    rightPanelLayout->setContentsMargins(0, 0, 0, 0);
    rightPanelLayout->setSpacing(0);
    
    // Créer le placeholder initial
    m_chartPlaceholder = new QLabel("Exécutez le backtest pour afficher les graphiques");
    m_chartPlaceholder->setAlignment(Qt::AlignCenter);
    m_chartPlaceholder->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_chartPlaceholder->setStyleSheet(
        "QLabel { "
        "background-color: #f5f5f5; "
        "border: 1px dashed #cccccc; "
        "color: #666666; "
        "font-size: 14px; "
        "}"
    );
    
    // Créer le widget de graphique
    m_chartWidget = new ChartWidget();
    m_chartWidget->setVisible(false); // Cacher initialement

    connect(m_chartWidget, &ChartWidget::indicatorAdded, this, &ChartView::onIndicatorAdded);
    connect(m_chartWidget, &ChartWidget::indicatorChanged, this, &ChartView::onIndicatorChanged);
    connect(m_chartWidget, &ChartWidget::indicatorRemoved, this, &ChartView::onIndicatorRemoved);
    connect(m_chartWidget, &ChartWidget::maxDisplayPointsChanged, this, &ChartView::onMaxDisplayPointsChanged);

    // Ajouter les widgets au layout du panneau droit
    rightPanelLayout->addWidget(m_chartPlaceholder);
    rightPanelLayout->addWidget(m_chartWidget);
    
    // Ajouter les composants au layout horizontal
    horizontalLayout->addWidget(m_leftPanel);
    horizontalLayout->addWidget(verticalSeparator);
    horizontalLayout->addWidget(m_rightPanel);
    
    // Configurer le widget pour s'étendre
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    if (m_app) {
        connect(m_app, &App::windowResizeStarted, m_chartWidget, [this]() {
            m_chartWidget->setResizing(true);
        });
        
        // connect(m_app, &App::windowResizeFinished, m_chartWidget, [this](QSize size) {
        //     m_chartWidget->setResizing(false);
        //     m_chartWidget->onWindowResized(size);
        // });
    }
}

void ChartView::setupIndicatorControls() {
    // Créer le groupe pour les contrôles d'indicateurs
    m_indicatorsGroup = new QGroupBox("Active Indicators");
    QVBoxLayout* groupLayout = new QVBoxLayout(m_indicatorsGroup);
    
    // Créer le layout pour la liste des indicateurs
    m_indicatorsLayout = new QVBoxLayout();
    groupLayout->addLayout(m_indicatorsLayout);
    
    // Ajouter un bouton d'ajout et une combobox pour le type d'indicateur
    QHBoxLayout* addIndicatorLayout = new QHBoxLayout();
    
    m_addIndicatorButton = new QPushButton("Add");
    m_addIndicatorButton->setFixedWidth(50);
    
    m_indicatorTypeCombo = new QComboBox();
    m_indicatorTypeCombo->addItem("RSI", "RSI");
    m_indicatorTypeCombo->addItem("EMA", "EMA");
    m_indicatorTypeCombo->addItem("Supertrend", "SUPERTREND");
    m_indicatorTypeCombo->addItem("Stochastic", "STOCH");  
    m_indicatorTypeCombo->addItem("ATR", "ATR"); 
    // Ajouter d'autres types d'indicateurs ici au besoin
    
    addIndicatorLayout->addWidget(m_indicatorTypeCombo);
    addIndicatorLayout->addWidget(m_addIndicatorButton);
    
    groupLayout->addLayout(addIndicatorLayout);
    
    // Connecter les signaux
    connect(m_addIndicatorButton, &QPushButton::clicked, this, &ChartView::onAddIndicatorClicked);
    connect(m_indicatorTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &ChartView::onIndicatorTypeSelected);
}

void ChartView::onIndicatorTypeSelected(int index) {
    // Cette méthode peut être utilisée pour ajouter un comportement spécifique
    // lorsque le type d'indicateur est modifié dans la liste déroulante
    Q_UNUSED(index);
}

void ChartView::onAddIndicatorClicked() {    
    QString indicatorType = m_indicatorTypeCombo->currentData().toString();
    
    // il serait bien d'ouvrir la fenetre de dialog directement plutot que mettre des valeurs par défaut
    if (indicatorType == "RSI") {
        RSIInstance rsi;
        rsi.period = 14;
        m_chartWidget->addIndicator(std::move(rsi));
    }
    else if (indicatorType == "EMA") {
        EMAInstance ema;
        ema.period = 20;
        ema.color = 0x0000FF;
        m_chartWidget->addIndicator(std::move(ema));
    }
    else if (indicatorType == "SUPERTREND") {
        SuperTrendInstance supertrend;
        supertrend.period = 10;
        supertrend.multiplier = 3.0;
        m_chartWidget->addIndicator(std::move(supertrend));
    }
    else if (indicatorType == "STOCH") {
        StochasticInstance stochastic;
        stochastic.fastKPeriod = 14;
        stochastic.slowKPeriod = 3;
        stochastic.slowDPeriod = 3;
        m_chartWidget->addIndicator(std::move(stochastic));
    }
    else if (indicatorType == "ATR") {
        ATRInstance atr;
        atr.period = 14;
        m_chartWidget->addIndicator(std::move(atr));
    }
    // Ajouter d'autres types d'indicateurs ici
}

void ChartView::createIndicatorWidgets(int id, const QString &name) {
    // Créer un widget horizontal pour cet indicateur
    QWidget* indicatorWidget = new QWidget();
    QHBoxLayout* layout = new QHBoxLayout(indicatorWidget);
    layout->setContentsMargins(0, 2, 0, 2);
    
    // Ajouter un libellé avec le nom de l'indicateur
    QLabel* nameLabel = new QLabel(name);
    nameLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    
    // Ajouter des boutons d'édition et de suppression
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
    
    // Ajouter les widgets au layout
    layout->addWidget(nameLabel);
    layout->addWidget(editButton);
    layout->addWidget(removeButton);
    
    // Stocker les références
    m_indicatorLabels[id] = nameLabel;
    m_editButtons[id] = editButton;
    m_removeButtons[id] = removeButton;
    
    // Ajouter au layout principal des indicateurs
    m_indicatorsLayout->addWidget(indicatorWidget);
}

void ChartView::onEditIndicator(int id) {
    if (tryOpenDialog<RSIInstance, RSIDialog>(id)) return;
    if (tryOpenDialog<EMAInstance, EMADialog>(id)) return;
    if (tryOpenDialog<SuperTrendInstance, SupertrendDialog>(id)) return;
    if (tryOpenDialog<StochasticInstance, StochasticDialog>(id)) return;
    if (tryOpenDialog<ATRInstance, ATRDialog>(id)) return;
}

//Remove tous les indicateurs individuellement par leur ID
void ChartView::onRemoveIndicator(int id) {
    m_chartWidget->removeIndicator(id);
}


void ChartView::refreshIndicatorsList() {
    // Supprimer tous les widgets d'indicateurs existants
    QLayoutItem* child;
    while ((child = m_indicatorsLayout->takeAt(0)) != nullptr) {
        if (child->widget())
            delete child->widget();
        delete child;
    }
    
    // Vider les maps
    m_indicatorLabels.clear();
    m_editButtons.clear();
    m_removeButtons.clear();
    
    // Approche générique pour tous les indicateurs
    const std::vector<std::unique_ptr<IndicatorBase>>& allIndicators = m_chartWidget->getIndicators();
    for (const auto& indicator : allIndicators) {
        if (indicator->visible) {
            createIndicatorWidgets(indicator->id, indicator->getDisplayName());
        }
    }
}

void ChartView::updateData(BacktestResults* results) {
    QTime start = QTime::currentTime();
            
    // Récupérer les résultats depuis l'App
    BacktestResults* appResults = m_app ? m_app->getBacktestResults() : nullptr;
    
    // Mettre à jour les références locales
    m_currentResults = appResults;

    // Mettre en cache les nouveaux pointeurs
    m_cachedResults = appResults;
    
    if (!appResults || !appResults->data) {
        clear();
        showPlaceholder("Aucune donnée disponible");
        return;
    }

    m_chartWidget->setBacktestResults(results);

    m_dataExtracted = true;
    
    // Définir le type de graphique
    QString chartType = m_chartTypeCombo->currentData().toString();
    m_chartWidget->setChartType(m_chartWidget->stringToChartType(chartType));
    
    // Afficher le widget de graphique et masquer le placeholder
    showChartWidget();
    
    // Rafraîchir la liste des indicateurs
    refreshIndicatorsList();

    if(!results->indicators.empty()) {
        // Configurer les indicateurs de la stratégie
        configureStrategyIndicators(results->indicators);
    }
    
    m_dataExtracted = true;

    int elapsed = start.msecsTo(QTime::currentTime());
}

void ChartView::onChartTypeChanged(int index) {
    if (!m_chartTypeCombo || !m_chartWidget) {
        return;
    }

    QVariant data = m_chartTypeCombo->itemData(index);
    if (data.isValid()) {
        QString chartType = data.toString();
        
        // Mettre à jour le type de graphique dans le widget
        m_chartWidget->setChartType(m_chartWidget->stringToChartType(chartType));
    }
}

void ChartView::showChartWidget() {
    if (m_chartPlaceholder) {
        m_chartPlaceholder->setVisible(false);
    }
    
    if (m_chartWidget) {
        m_chartWidget->setVisible(true);
    }
}

void ChartView::showPlaceholder(const QString& message) {
    if (m_chartWidget) {
        m_chartWidget->setVisible(false);
    }
    
    if (m_chartPlaceholder) {
        m_chartPlaceholder->setText(message);
        m_chartPlaceholder->setVisible(true);
    }
}

void ChartView::clear() {
    m_currentResults = nullptr;
    m_cachedResults = nullptr;
    m_dataExtracted = false;
    
    // Nettoyer le widget de graphique
    if (m_chartWidget) {
        // m_chartWidget->clearChart();
    }
    
    // Vider la liste des indicateurs
    refreshIndicatorsList();
    
    // Réafficher le placeholder
    showPlaceholder("Exécutez un backtest pour afficher les graphiques");
}

void ChartView::onRulerToolToggled(bool checked) {
    if (m_chartWidget) {
        m_chartWidget->setRulerToolEnabled(checked);
    }
}

void ChartView::onIndicatorAdded(int id, const QString &name) {
    createIndicatorWidgets(id, name);
}

void ChartView::onIndicatorChanged(int id, const QString &name) {
    if (m_indicatorLabels.contains(id)) {
        m_indicatorLabels[id]->setText(name);
    }
}

void ChartView::onIndicatorRemoved(int id) {
    refreshIndicatorsList(); // methode de merde lourde
    // Il faudrait plutot supprimer l'indicateur directement
    // et supprimer les widgets associés
    // if (m_indicatorLabels.contains(id)) {
    //     delete m_indicatorLabels[id];
    //     m_indicatorLabels.remove(id);
    // }
}

void ChartView::onAggregationSliderChanged(int value) {
    // Mettre à jour l'étiquette
    m_aggregationLabel->setText(QString::number(value));
    
    // Mettre à jour le ChartWidget si disponible
    if (m_chartWidget) {
        m_chartWidget->setMaxDisplayPoints(value);
    }
}

void ChartView::onMaxDisplayPointsChanged(int value) {
    // Mettre à jour le slider et l'étiquette si la valeur change depuis le ChartWidget
    if (m_aggregationSlider->value() != value) {
        m_aggregationSlider->setValue(value);
        m_aggregationLabel->setText(QString::number(value));
    }
}

void ChartView::configureStrategyIndicators(const std::vector<StrategyIndicator>& indicators) {
    // Supprimer les indicateurs existants
    if (m_chartWidget) {
        m_chartWidget->removeAllIndicators();

        // Pour chaque indicateur de la stratégie
        for (const auto& indicator : indicators) {
            switch (indicator.type) {
                case StrategyIndicator::RSI: {
                    RSIInstance rsi;
                    rsi.period = static_cast<int>(indicator.params.at("period"));
                    rsi.height = 90;  // Hauteur standard
                    rsi.color = 0x800080;  // Couleur par défaut (violet)
                    m_chartWidget->addIndicator(std::move(rsi));
                    break;
                }
                case StrategyIndicator::EMA: {
                    EMAInstance ema;
                    ema.period = static_cast<int>(indicator.params.at("period"));
                    ema.visible = true;
                    
                    // Attribuer une couleur différente selon la période
                    if (ema.period < 50) 
                        ema.color = 0x0000FF;  // Bleu pour EMA courte
                    else if (ema.period < 100)
                        ema.color = 0xFF0000;  // Rouge pour EMA moyenne
                    else
                        ema.color = 0x008000;  // Vert pour EMA longue

                    m_chartWidget->addIndicator(std::move(ema));
                    break;
                }
                case StrategyIndicator::STOCHASTIC: {
                    StochasticInstance stoch;
                    stoch.fastKPeriod = static_cast<int>(indicator.params.at("fastKPeriod"));
                    stoch.slowKPeriod = static_cast<int>(indicator.params.at("slowKPeriod"));
                    stoch.slowDPeriod = static_cast<int>(indicator.params.at("slowDPeriod"));
                    stoch.overboughtLevel = static_cast<int>(indicator.params.at("overboughtLevel"));
                    stoch.oversoldLevel = static_cast<int>(indicator.params.at("oversoldLevel"));
                    stoch.height = 90;  // Hauteur standard
                    stoch.kColor = 0x0000FF;  // Bleu pour K
                    stoch.dColor = 0xFF0000;  // Rouge pour D
                    m_chartWidget->addIndicator(std::move(stoch));
                    break;
                }
                case StrategyIndicator::ATR: {
                    ATRInstance atr;
                    atr.period = static_cast<int>(indicator.params.at("period"));
                    atr.useLogScale = indicator.params.at("useLogScale") > 0.5;
                    atr.height = 90;  // Hauteur standard
                    atr.color = 0x008800;  // Vert
                    m_chartWidget->addIndicator(std::move(atr));
                    break;
                }
            }
        }
    }
}
