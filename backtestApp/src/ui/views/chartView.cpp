#include "ui/views/chartView.h"
#include "ui/app.h"
#include <QDebug>
#include <QTime>
#include <QLabel>
#include <QFrame>
#include <QResizeEvent>
#include <QToolButton>

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
    // Find the parent App instance
    QWidget* widget = parent;
    while (widget && !m_app) {
        m_app = qobject_cast<App*>(widget);
        widget = widget->parentWidget();
    }

    // Build the UI
    setupUI();
}

ChartView::~ChartView()
{
}

void ChartView::setupUI()
{
    // Configure the main layout to take up all available space
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    // Create a horizontal layout for the left and right panels
    QHBoxLayout* horizontalLayout = new QHBoxLayout();
    horizontalLayout->setContentsMargins(0, 0, 0, 0);
    horizontalLayout->setSpacing(0);
    m_mainLayout->addLayout(horizontalLayout);
    
    // Create the left panel with a fixed width
    m_leftPanel = new QWidget();
    m_leftPanel->setObjectName("leftPanel");
    m_leftPanel->setStyleSheet("#leftPanel { background-color: #BADDFF; }");
    m_leftPanel->setFixedWidth(230); // Increase width for indicator controls

    // Add a vertical layout to the left panel
    QVBoxLayout* leftPanelLayout = new QVBoxLayout(m_leftPanel);
    leftPanelLayout->setContentsMargins(5, 8, 5, 8);
    leftPanelLayout->setSpacing(10);

    // Add a title to the left panel
    m_settingsTitle = new QLabel("Settings");
    m_settingsTitle->setAlignment(Qt::AlignCenter);
    m_settingsTitle->setStyleSheet("font-weight: bold; font-size: 16px;");
    leftPanelLayout->addWidget(m_settingsTitle);

    // Add the chart type selector
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

    // Add the aggregation threshold control
    QLabel* aggregationTitle = new QLabel("Seuil d'Agrégation");
    aggregationTitle->setStyleSheet("font-weight: bold;");
    leftPanelLayout->addWidget(aggregationTitle);

    // Layout for the slider and value label
    QHBoxLayout* sliderLayout = new QHBoxLayout();

    // Create the slider
    int aggregation_default_value = 30000;
    m_aggregationSlider = new QSlider(Qt::Horizontal);
    m_aggregationSlider->setMinimum(1000);   // Minimum
    m_aggregationSlider->setMaximum(100000); // Maximum
    m_aggregationSlider->setValue(aggregation_default_value); // Default value
    m_aggregationSlider->setTickInterval(10000);
    m_aggregationSlider->setTickPosition(QSlider::TicksBelow);

    // Create the value label
    m_aggregationLabel = new QLabel(QString::number(aggregation_default_value));
    m_aggregationLabel->setMinimumWidth(50);

    // Add widgets to the layout
    sliderLayout->addWidget(m_aggregationSlider);
    sliderLayout->addWidget(m_aggregationLabel);

    // Add the layout to the left panel
    leftPanelLayout->addLayout(sliderLayout);
    
    // Add a description
    QLabel* aggregationDesc = new QLabel("Ajuste le nombre maximum de points à afficher avant agrégation");
    aggregationDesc->setWordWrap(true);
    aggregationDesc->setStyleSheet("font-size: 9px; color: #666;");
    leftPanelLayout->addWidget(aggregationDesc);

    connect(m_aggregationSlider, &QSlider::valueChanged, this, &ChartView::onAggregationSliderChanged);

    // Connect the change signal to our slot
    connect(m_chartTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ChartView::onChartTypeChanged);

    // Replace the checkbox with a styled QToolButton
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

    // Connect the button's toggle signal
    connect(rulerToolButton, &QToolButton::toggled, this, &ChartView::onRulerToolToggled);

    // Connect the signal to change the icon when the state changes
    connect(rulerToolButton, &QToolButton::toggled, [rulerToolButton](bool checked) {
        if (checked) {
            rulerToolButton->setIcon(QIcon(":/icons/ruler_checked.png"));
        } else {
            rulerToolButton->setIcon(QIcon(":/icons/ruler_unchecked.png"));
        }
    });
    
    leftPanelLayout->addWidget(rulerToolButton);

    // Technical Indicators Section
    QLabel* indicatorsLabel = new QLabel("Technical Indicators");
    indicatorsLabel->setStyleSheet("font-weight: bold; margin-top: 10px;");
    leftPanelLayout->addWidget(indicatorsLabel);

    // Setup the indicator controls
    setupIndicatorControls();
    leftPanelLayout->addWidget(m_indicatorsGroup);

    // Add a stretchable space at the bottom
    leftPanelLayout->addStretch();

    // Create a vertical separator
    QFrame* verticalSeparator = new QFrame();
    verticalSeparator->setFrameStyle(QFrame::VLine | QFrame::Plain);
    verticalSeparator->setStyleSheet("color: #CCCCCC;"); // Line color

    // Create the right panel that will contain the chart
    m_rightPanel = new QWidget();
    m_rightPanel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // Layout for the right panel
    QVBoxLayout* rightPanelLayout = new QVBoxLayout(m_rightPanel);
    rightPanelLayout->setContentsMargins(0, 0, 0, 0);
    rightPanelLayout->setSpacing(0);

    // Create the initial placeholder
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

    // Create the chart widget
    m_chartWidget = new ChartWidget();
    m_chartWidget->setVisible(false); // Initially hidden

    connect(m_chartWidget, &ChartWidget::indicatorAdded, this, &ChartView::onIndicatorAdded);
    connect(m_chartWidget, &ChartWidget::indicatorChanged, this, &ChartView::onIndicatorChanged);
    connect(m_chartWidget, &ChartWidget::indicatorRemoved, this, &ChartView::onIndicatorRemoved);
    connect(m_chartWidget, &ChartWidget::maxDisplayPointsChanged, this, &ChartView::onMaxDisplayPointsChanged);

    // Add the widgets to the right panel layout
    rightPanelLayout->addWidget(m_chartPlaceholder);
    rightPanelLayout->addWidget(m_chartWidget);

    // Add the components to the horizontal layout
    horizontalLayout->addWidget(m_leftPanel);
    horizontalLayout->addWidget(verticalSeparator);
    horizontalLayout->addWidget(m_rightPanel);

    // Configure the widget to stretch
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    if (m_app) 
        connect(m_app, &App::windowResizeStarted, m_chartWidget, [this]() {
            m_chartWidget->setResizing(true);
        }); 

    QPushButton* zoomModeButton = new QPushButton("Zoom Y");
    zoomModeButton->setCheckable(true);
    zoomModeButton->setStyleSheet(
        "QPushButton {"
        "    padding: 4px;"
        "    border: 1px solid #999;"
        "    border-radius: 4px;"
        "    background-color: white;"
        "}"
        "QPushButton:checked {"
        "    background-color: rgb(0, 141, 0);"
        "}"
    );
    connect(zoomModeButton, &QPushButton::toggled, m_chartWidget, &ChartWidget::toggleVerticalMoveMode);
    leftPanelLayout->addWidget(zoomModeButton);
}

void ChartView::setupIndicatorControls() {
    // Create the group for the indicator controls
    m_indicatorsGroup = new QGroupBox("Active Indicators");
    QVBoxLayout* groupLayout = new QVBoxLayout(m_indicatorsGroup);

    // Create the layout for the indicator list
    m_indicatorsLayout = new QVBoxLayout();
    groupLayout->addLayout(m_indicatorsLayout);

    // Add a button and a combobox for the indicator type
    QHBoxLayout* addIndicatorLayout = new QHBoxLayout();
    
    m_addIndicatorButton = new QPushButton("Add");
    m_addIndicatorButton->setFixedWidth(50);
    
    m_indicatorTypeCombo = new QComboBox();
    m_indicatorTypeCombo->addItem("RSI", "RSI");
    m_indicatorTypeCombo->addItem("EMA", "EMA");
    m_indicatorTypeCombo->addItem("Supertrend", "SUPERTREND");
    m_indicatorTypeCombo->addItem("Stochastic", "STOCH");
    m_indicatorTypeCombo->addItem("ATR", "ATR");
    m_indicatorTypeCombo->addItem("Points Pivots", "PivotPoints");
    // Add other indicator types here as needed

    addIndicatorLayout->addWidget(m_indicatorTypeCombo);
    addIndicatorLayout->addWidget(m_addIndicatorButton);
    
    groupLayout->addLayout(addIndicatorLayout);

    // Connect the signals
    connect(m_addIndicatorButton, &QPushButton::clicked, this, &ChartView::onAddIndicatorClicked);
    connect(m_indicatorTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &ChartView::onIndicatorTypeSelected);
}

void ChartView::onIndicatorTypeSelected(int index) {
    // This method can be used to add specific behavior
    // when the indicator type is changed in the dropdown
    Q_UNUSED(index);
}

void ChartView::onAddIndicatorClicked() {    
    QString indicatorType = m_indicatorTypeCombo->currentData().toString();

    // It would be better to open the dialog window directly rather than setting default values
    // on pourrait faire une methode appeler a la construction de l'indicateur pour les paramettre par défaut
    // factorisation du code
    if (indicatorType == "RSI") {
        RSIInstance rsi;
        m_chartWidget->addIndicator(std::move(rsi));
    }
    else if (indicatorType == "EMA") {
        EMAInstance ema;
        m_chartWidget->addIndicator(std::move(ema));
    }
    else if (indicatorType == "SUPERTREND") {
        SuperTrendInstance supertrend;
        m_chartWidget->addIndicator(std::move(supertrend));
    }
    else if (indicatorType == "STOCH") {
        StochasticInstance stochastic;
        m_chartWidget->addIndicator(std::move(stochastic));
    }
    else if (indicatorType == "ATR") {
        ATRInstance atr;
        m_chartWidget->addIndicator(std::move(atr));
    }
    else if (indicatorType == "PivotPoints") {
        PivotPointsInstance pivotPoints;
        m_chartWidget->addIndicator(std::move(pivotPoints));
    }
    // Add other indicator types here as needed
}

void ChartView::createIndicatorWidgets(int id, const QString &name) {
    // Create a horizontal widget for this indicator
    QWidget* indicatorWidget = new QWidget();
    QHBoxLayout* layout = new QHBoxLayout(indicatorWidget);
    layout->setContentsMargins(0, 2, 0, 2);

    // Add a label with the indicator name
    QLabel* nameLabel = new QLabel(name);
    nameLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    // Add edit and remove buttons
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

    // Add the widgets to the layout
    layout->addWidget(nameLabel);
    layout->addWidget(editButton);
    layout->addWidget(removeButton);

    // Store the references
    m_indicatorLabels[id] = nameLabel;
    m_editButtons[id] = editButton;
    m_removeButtons[id] = removeButton;

    // Add to the main indicators layout
    m_indicatorsLayout->addWidget(indicatorWidget);
}

void ChartView::onEditIndicator(int id) {
    if (tryOpenDialog<RSIInstance, RSIDialog>(id)) return;
    if (tryOpenDialog<EMAInstance, EMADialog>(id)) return;
    if (tryOpenDialog<SuperTrendInstance, SupertrendDialog>(id)) return;
    if (tryOpenDialog<StochasticInstance, StochasticDialog>(id)) return;
    if (tryOpenDialog<ATRInstance, ATRDialog>(id)) return;
    if (tryOpenDialog<PivotPointsInstance, PivotPointsDialog>(id)) return;
}

// Remove all indicators individually by their ID
void ChartView::onRemoveIndicator(int id) {
    m_chartWidget->removeIndicator(id);
}


void ChartView::refreshIndicatorsList() {
    // Remove all existing indicator widgets
    QLayoutItem* child;
    while ((child = m_indicatorsLayout->takeAt(0)) != nullptr) {
        if (child->widget())
            delete child->widget();
        delete child;
    }

    // Clear the maps
    m_indicatorLabels.clear();
    m_editButtons.clear();
    m_removeButtons.clear();

    // Generic approach for all indicators
    const std::vector<std::unique_ptr<IndicatorBase>>& allIndicators = m_chartWidget->getIndicators();
    for (const auto& indicator : allIndicators) {
        if (indicator->visible) {
            createIndicatorWidgets(indicator->id, indicator->getDisplayName());
        }
    }
}

void ChartView::updateData(BacktestResults* results) {
    QTime start = QTime::currentTime();

    // Retrieve results from the App
    BacktestResults* appResults = m_app ? m_app->getBacktestResults() : nullptr;

    // Update local references
    m_currentResults = appResults;

    // Cache the new pointers
    m_cachedResults = appResults;
    
    if (!appResults || !appResults->data) {
        clear();
        showPlaceholder("Aucune donnée disponible");
        return;
    }

    m_chartWidget->setBacktestResults(results);

    m_dataExtracted = true;
    
    // Define the chart type
    QString chartType = m_chartTypeCombo->currentData().toString();
    m_chartWidget->setChartType(m_chartWidget->stringToChartType(chartType));

    // Show the chart widget and hide the placeholder
    showChartWidget();

    // Always configure strategy indicators (even if empty to clean)
    configureStrategyIndicators(results->indicators);

    // Refresh the indicators list AFTER configuring strategy indicators to ensure all indicators are cleared
    refreshIndicatorsList();
    
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

        // Update the chart type in the widget
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

    // Clear the indicators list
    refreshIndicatorsList();

    // Show the placeholder
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
    refreshIndicatorsList(); //TODO: This method is a bit heavy, it would be better to remove the indicator directly
    // and delete the associated widgets directly

}

void ChartView::onAggregationSliderChanged(int value) {
    // Update the label
    m_aggregationLabel->setText(QString::number(value));

    // Update the ChartWidget if available
    if (m_chartWidget) {
        m_chartWidget->setMaxDisplayPoints(value);
    }
}

void ChartView::onMaxDisplayPointsChanged(int value) {
    // Update the slider and label if the value changes from the ChartWidget
    if (m_aggregationSlider->value() != value) {
        m_aggregationSlider->setValue(value);
        m_aggregationLabel->setText(QString::number(value));
    }
}

void ChartView::configureStrategyIndicators(const std::vector<StrategyIndicator>& indicators) {
    // Remove all existing indicators from the chart widget
    if (m_chartWidget) {
        m_chartWidget->removeAllIndicators();

        // For each strategy indicator
        for (const auto& indicator : indicators) {
            switch (indicator.type) {
                case StrategyIndicator::RSI: {
                    RSIInstance rsi;
                    rsi.period = static_cast<int>(indicator.params.at("period"));
                    rsi.height = 90;  // Standard height
                    rsi.color = 0x800080;  // Default color (purple)
                    m_chartWidget->addIndicator(std::move(rsi));
                    break;
                }
                case StrategyIndicator::EMA: {
                    EMAInstance ema;
                    ema.period = static_cast<int>(indicator.params.at("period"));
                    ema.visible = true;

                    // Assign a different color based on the period
                    if (ema.period < 50)
                        ema.color = 0x0000FF;  // Blue for short EMA
                    else if (ema.period < 100)
                        ema.color = 0xff00b6;  // Pink for medium EMA   
                    else
                        ema.color = 0xFFA500;  // Orange for long EMA

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
                    stoch.height = 90;  // Standard height
                    stoch.kColor = 0x0000FF;  // Blue for K
                    stoch.dColor = 0xFF0000;  // Red for D
                    m_chartWidget->addIndicator(std::move(stoch));
                    break;
                }
                case StrategyIndicator::ATR: {
                    ATRInstance atr;
                    atr.period = static_cast<int>(indicator.params.at("period"));
                    atr.useLogScale = indicator.params.at("useLogScale") > 0.5;
                    atr.height = 90;  // Standard height
                    atr.color = 0x008800;  // Green
                    m_chartWidget->addIndicator(std::move(atr));
                    break;
                }
                case StrategyIndicator::SUPERTREND: {
                    SuperTrendInstance supertrend;
                    supertrend.period = static_cast<int>(indicator.params.at("period"));
                    supertrend.multiplier = indicator.params.at("multiplier");
                    m_chartWidget->addIndicator(std::move(supertrend));
                    break;
                }
                // ajouter ici les point pivots quand ils seront dans la stratégie
                // case StrategyIndicator::PIVOT_POINTS: {
                //     PivotPointsInstance pivotPoints;
                //     pivotPoints.periodType = PivotPointsInstance::PeriodType::Daily;
                //     pivotPoints.levelStyles[PivotPointsInstance::LevelType::Pivot] = { 0xFF0000, 2, Qt::SolidLine };
                //     m_chartWidget->addIndicator(std::move(pivotPoints));
                //     break;
                // }
            }
        }
    }
}
