#include "ui/chart/chartControlPanel.h"
#include <QFrame>
#include <QDebug>

#include "ui/dialogs/indicators/rsiDialog.h"
#include "ui/dialogs/indicators/emaDialog.h"
#include "ui/dialogs/indicators/supertrendDialog.h"
#include "ui/dialogs/indicators/stochasticDialog.h"
#include "ui/dialogs/indicators/atrDialog.h"
#include "ui/dialogs/indicators/pivotPointsDialog.h"
#include "ui/dialogs/indicators/cciDialog.h"
#include "ui/dialogs/indicators/macdDialog.h"
#include "ui/dialogs/indicators/bbDialog.h"

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
    setFixedWidth(230); // Fixed width for the panel

    QVBoxLayout* leftPanelLayout = new QVBoxLayout(this);
    leftPanelLayout->setContentsMargins(5, 8, 5, 8);
    leftPanelLayout->setSpacing(10);

    // Panel title
    m_settingsTitle = new QLabel("Settings");
    m_settingsTitle->setAlignment(Qt::AlignCenter);
    m_settingsTitle->setStyleSheet("font-weight: bold; font-size: 16px;");
    leftPanelLayout->addWidget(m_settingsTitle);

    // Chart type selector
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

    // Aggregation threshold control
    QLabel* aggregationTitle = new QLabel("Aggregation Threshold");
    aggregationTitle->setStyleSheet("font-weight: bold;");
    leftPanelLayout->addWidget(aggregationTitle);

    // Layout for the slider and label
    QHBoxLayout* sliderLayout = new QHBoxLayout();

    // Aggregation slider
    int aggregation_default_value = 30000;
    m_aggregationSlider = new QSlider(Qt::Horizontal);
    m_aggregationSlider->setMinimum(1000);
    m_aggregationSlider->setMaximum(100000);
    m_aggregationSlider->setValue(aggregation_default_value);
    m_aggregationSlider->setTickInterval(10000);
    m_aggregationSlider->setTickPosition(QSlider::TicksBelow);

    // Value label
    m_aggregationLabel = new QLabel(QString::number(aggregation_default_value));
    m_aggregationLabel->setMinimumWidth(50);

    sliderLayout->addWidget(m_aggregationSlider);
    sliderLayout->addWidget(m_aggregationLabel);
    leftPanelLayout->addLayout(sliderLayout);
    
    // Aggregation description
    QLabel* aggregationDesc = new QLabel("Adjusts the maximum number of points to display before aggregation");
    aggregationDesc->setWordWrap(true);
    aggregationDesc->setStyleSheet("font-size: 9px; color: #666;");
    leftPanelLayout->addWidget(aggregationDesc);

    connect(m_aggregationSlider, &QSlider::valueChanged, this, &ChartControlPanel::onAggregationSliderChanged);
    connect(m_chartTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ChartControlPanel::onChartTypeChanged);

    // Ruler tool button
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

    // Buttons for drawing tools
    QLabel* drawToolsLabel = new QLabel("Drawing Tools");
    drawToolsLabel->setStyleSheet("font-weight: bold; margin-top: 10px;");
    leftPanelLayout->addWidget(drawToolsLabel);

    QHBoxLayout* drawToolsLayout = new QHBoxLayout();
    drawToolsLayout->setSpacing(5);
    
    // Button for Check marker
    m_checkMarkerButton = new QToolButton();
    m_checkMarkerButton->setIcon(QIcon(":/icons/check.png"));
    m_checkMarkerButton->setIconSize(QSize(32, 32));
    m_checkMarkerButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    m_checkMarkerButton->setCheckable(true);
    m_checkMarkerButton->setToolTip("Place validation markers");
    m_checkMarkerButton->setStyleSheet(
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
    
    // Button for Error marker
    m_errorMarkerButton = new QToolButton();
    m_errorMarkerButton->setIcon(QIcon(":/icons/error.png"));
    m_errorMarkerButton->setIconSize(QSize(32, 32));
    m_errorMarkerButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    m_errorMarkerButton->setCheckable(true);
    m_errorMarkerButton->setToolTip("Place error markers");
    m_errorMarkerButton->setStyleSheet(
        "QToolButton {"
        "    padding: 4px;"
        "    border: 1px solid #999;"
        "    border-radius: 4px;"
        "    background-color: white;"
        "}"
        "QToolButton:checked {"
        "    background-color:rgb(200, 0, 0);"
        "}"
    );
    
    // Button to clear all markers
    m_clearMarkersButton = new QPushButton("Clear");
    m_clearMarkersButton->setIcon(QIcon::fromTheme("edit-clear"));
    m_clearMarkersButton->setToolTip("Clear all markers");
    m_clearMarkersButton->setStyleSheet(
        "QPushButton {"
        "    padding: 4px;"
        "    border: 1px solid #999;"
        "    border-radius: 4px;"
        "    background-color: white;"
        "}"
    );
    
    drawToolsLayout->addWidget(m_checkMarkerButton);
    drawToolsLayout->addWidget(m_errorMarkerButton);
    drawToolsLayout->addWidget(m_clearMarkersButton);
    leftPanelLayout->addLayout(drawToolsLayout);
    
    connect(m_checkMarkerButton, &QToolButton::toggled, this, &ChartControlPanel::onCheckMarkerToggled);
    connect(m_errorMarkerButton, &QToolButton::toggled, this, &ChartControlPanel::onErrorMarkerToggled);
    connect(m_clearMarkersButton, &QPushButton::clicked, this, &ChartControlPanel::onClearMarkersClicked);

    // Horizontal layout for comparison buttons
    m_comparisonButtonsLayout = new QHBoxLayout();
    m_comparisonButtonsLayout->setSpacing(5);
    
    // Button to transfer data to the comparison chart
    m_transferDataButton = new QPushButton("Copy");
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
    
    // Button to exit comparison mode
    m_exitComparisonButton = new QPushButton("");
    m_exitComparisonButton->setIcon(QIcon::fromTheme("window-close"));
    m_exitComparisonButton->setFixedWidth(30);
    m_exitComparisonButton->setToolTip("Exit comparison mode");
    m_exitComparisonButton->setStyleSheet(
        "QPushButton {"
        "    padding: 4px;"
        "    border: 1px solid #999;"
        "    border-radius: 4px;"
        "    background-color: white;"
        "    color: red;"
        "}"
    );
    m_exitComparisonButton->setVisible(false); // Initially hidden
    m_comparisonButtonsLayout->addWidget(m_exitComparisonButton, 0); // Stretch ratio = 0

    leftPanelLayout->addLayout(m_comparisonButtonsLayout);
    
    connect(m_transferDataButton, &QPushButton::clicked, this, &ChartControlPanel::onTransferDataClicked);
    connect(m_exitComparisonButton, &QPushButton::clicked, this, &ChartControlPanel::onExitComparisonClicked);

    // Section for technical indicators
    QLabel* indicatorsLabel = new QLabel("Technical Indicators");
    indicatorsLabel->setStyleSheet("font-weight: bold; margin-top: 10px;");
    leftPanelLayout->addWidget(indicatorsLabel);

    setupIndicatorControls();
    leftPanelLayout->addWidget(m_indicatorsGroup);
    
    // Add the suggestions section
    QLabel* suggestionsLabel = new QLabel("Suggestions (Strategy)");
    suggestionsLabel->setStyleSheet("font-weight: bold; margin-top: 10px;");
    leftPanelLayout->addWidget(suggestionsLabel);
    
    // Create the suggestions group
    m_suggestionsGroup = new QGroupBox("Suggested Indicators");
    QVBoxLayout* suggestionsGroupLayout = new QVBoxLayout(m_suggestionsGroup);
    
    // Layout for suggestions
    m_suggestionsLayout = new QVBoxLayout();
    suggestionsGroupLayout->addLayout(m_suggestionsLayout);
    
    // Buttons layout (Clear and Add All)
    QHBoxLayout* suggestionsButtonsLayout = new QHBoxLayout();
    
    // Button to add all suggestions
    m_addAllSuggestionsButton = new QPushButton("Add All");
    m_addAllSuggestionsButton->setIcon(QIcon::fromTheme("list-add"));
    m_addAllSuggestionsButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #4CAF50;"
        "    color: white;"
        "    border: none;"
        "    padding: 5px;"
        "    border-radius: 3px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #45a049;"
        "}"
    );
    connect(m_addAllSuggestionsButton, &QPushButton::clicked, this, &ChartControlPanel::onAddAllSuggestions);
    suggestionsButtonsLayout->addWidget(m_addAllSuggestionsButton);
    
    // Button to clear all suggestions
    m_clearSuggestionsButton = new QPushButton("Clear");
    m_clearSuggestionsButton->setIcon(QIcon::fromTheme("edit-clear"));
    m_clearSuggestionsButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #f44336;"
        "    color: white;"
        "    border: none;"
        "    padding: 5px;"
        "    border-radius: 3px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #a72525ff;"
        "}"
    );

    connect(m_clearSuggestionsButton, &QPushButton::clicked, this, &ChartControlPanel::onClearAllSuggestions);
    suggestionsButtonsLayout->addWidget(m_clearSuggestionsButton);
    
    suggestionsGroupLayout->addLayout(suggestionsButtonsLayout);
    
    leftPanelLayout->addWidget(m_suggestionsGroup);
    m_suggestionsGroup->setVisible(false); // Initially hidden

    leftPanelLayout->addStretch();
}

void ChartControlPanel::suggestIndicatorsFromStrategy(const std::vector<std::unique_ptr<indicators::IndicatorBase>>& indicators) {
    // Cleanly clear old suggestions
    onClearAllSuggestions();
    
    // If no suggestions, hide the section and return
    if (indicators.empty()) {
        m_suggestionsGroup->setVisible(false);
        return;
    }
    
    // For each suggested indicator
    for (const auto& indicator : indicators) {
        // Check if a similar indicator already exists
        if (!hasSimilarIndicator(indicator.get())) {
            // Create a copy of the indicator
            std::unique_ptr<indicators::IndicatorBase> clonedIndicator = indicator->clone();
            
            // Create a suggestion widget and add it to the list
            createSuggestionWidget(std::move(clonedIndicator));
        }
    }
    
    // Show the suggestions section if any
    m_suggestionsGroup->setVisible(!m_suggestions.empty());
}

void ChartControlPanel::createSuggestionWidget(std::unique_ptr<indicators::IndicatorBase> indicator) {
    // Create the widget and its layout
    QWidget* suggestionWidget = new QWidget();
    QHBoxLayout* layout = new QHBoxLayout(suggestionWidget);
    layout->setContentsMargins(0, 2, 0, 2);
    
    // Label with indicator name
    QLabel* nameLabel = new QLabel(indicator->getDisplayName());
    nameLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    nameLabel->setStyleSheet("color: #0066cc;");
    
    // Create the suggestion item and store the index
    SuggestionItem item;
    item.widget = suggestionWidget;
    item.label = nameLabel;
    item.indicator = std::move(indicator);
    int newIndex = static_cast<int>(m_suggestions.size());
    
    // Button to add the indicator
    QPushButton* addButton = new QPushButton("→");
    addButton->setFixedSize(QSize(25, 25));
    addButton->setToolTip("Add to chart");
    connect(addButton, &QPushButton::clicked, [this, newIndex]() {
        if (newIndex < static_cast<int>(m_suggestions.size())) {
            this->onAddSuggestedIndicator(newIndex);
        }
    });
    item.addButton = addButton;
    
    // Button to reject the suggestion
    QPushButton* rejectButton = new QPushButton("×");
    rejectButton->setFixedSize(QSize(25, 25));
    rejectButton->setToolTip("Ignore this suggestion");
    connect(rejectButton, &QPushButton::clicked, [this, newIndex]() {
        if (newIndex < static_cast<int>(m_suggestions.size())) {
            this->onRejectSuggestion(newIndex);
        }
    });
    item.rejectButton = rejectButton;
    
    // Assemble the widget
    layout->addWidget(nameLabel);
    layout->addWidget(addButton);
    layout->addWidget(rejectButton);
    
    // Add the widget to the layout
    m_suggestionsLayout->addWidget(suggestionWidget);
    
    // Add the item to our list
    m_suggestions.push_back(std::move(item));
}


void ChartControlPanel::onAddSuggestedIndicator(int index) {
    // Safety check
    if (index < 0 || index >= static_cast<int>(m_suggestions.size()) || !m_chartWidget)
        return;
    
    const indicators::IndicatorBase* indicator = m_suggestions[index].indicator.get();
    if (!indicator)
        return;
    
    // Add the indicator to the chart based on its type
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
    } else if (const indicators::MACDInstance* macd = dynamic_cast<const indicators::MACDInstance*>(indicator)) {
        m_chartWidget->addIndicator(*macd);
    } else if (const indicators::BBInstance* bb = dynamic_cast<const indicators::BBInstance*>(indicator)) {
        m_chartWidget->addIndicator(*bb);
    }
    
    // Reject the suggestion after adding it
    onRejectSuggestion(index);
}

void ChartControlPanel::onRejectSuggestion(int index) {
    // Safety check
    if (index < 0 || index >= static_cast<int>(m_suggestions.size()))
        return;
    
    // Get the item to remove
    QWidget* widgetToRemove = m_suggestions[index].widget;
    
    // Remove from layout
    if (widgetToRemove) {
        m_suggestionsLayout->removeWidget(widgetToRemove);
        widgetToRemove->disconnect(); // Disconnect all signals
        widgetToRemove->deleteLater(); // Use deleteLater instead of direct delete
    }
    
    // Remove the item from the list
    m_suggestions.erase(m_suggestions.begin() + index);
    
    // Update the indices in the lambdas for remaining items
    for (int i = index; i < static_cast<int>(m_suggestions.size()); ++i) {
        // Disconnect old connections
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
    
    // Hide the group if there are no suggestions left
    if (m_suggestions.empty()) {
        m_suggestionsGroup->setVisible(false);
    }
}

void ChartControlPanel::onClearAllSuggestions() {
    // Use the safe method to remove all widgets
    while (!m_suggestions.empty()) {
        // Always remove the first item until the list is empty
        onRejectSuggestion(0);
    }
    
    // Double-check to ensure everything is cleaned up
    QLayoutItem* child;
    while ((child = m_suggestionsLayout->takeAt(0)) != nullptr) {
        if (child->widget()) {
            child->widget()->disconnect();
            child->widget()->deleteLater();
        }
        delete child;
    }
    
    // Ensure the list is really empty
    m_suggestions.clear();
    
    // Hide the group
    m_suggestionsGroup->setVisible(false);
}

void ChartControlPanel::onAddAllSuggestions() {
    // Iterate backwards to avoid index issues when removing elements
    for (int i = static_cast<int>(m_suggestions.size()) - 1; i >= 0; --i) 
        onAddSuggestedIndicator(i);
}

bool ChartControlPanel::hasSimilarIndicator(const indicators::IndicatorBase* indicator) const {
    if (!m_chartWidget) return false;
    
    const std::vector<std::unique_ptr<indicators::IndicatorBase>>& existingIndicators = m_chartWidget->getIndicators();
    
    // Check each existing indicator
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
    m_indicatorTypeCombo->addItem("MACD", static_cast<int>(indicators::Type::MACD));
    m_indicatorTypeCombo->addItem("Bollinger Bands", static_cast<int>(indicators::Type::BB));

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
    } else if (indicatorType == static_cast<int>(indicators::Type::MACD)) {
        indicators::MACDInstance macd;
        m_chartWidget->addIndicator(std::move(macd));
    } else if (indicatorType == static_cast<int>(indicators::Type::BB)) {
        indicators::BBInstance bb;
        m_chartWidget->addIndicator(std::move(bb));
    }
}

void ChartControlPanel::createIndicatorWidgets(int id, const QString &name, bool isVisible) {
    QWidget* indicatorWidget = new QWidget();
    QHBoxLayout* layout = new QHBoxLayout(indicatorWidget);
    layout->setContentsMargins(0, 2, 0, 2);

    QLabel* nameLabel = new QLabel(name);
    nameLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    QPushButton* hideButton = new QPushButton(isVisible ? "Hide" : "Show");
    hideButton->setFixedWidth(40);
    hideButton->setCheckable(true);
    hideButton->setChecked(!isVisible);
    connect(hideButton, &QPushButton::toggled, [this, id, hideButton](bool checked) {
        hideButton->setText(checked ? "Show" : "Hide");
        this->onHideIndicator(id);
    });

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
    layout->addWidget(hideButton);
    layout->addWidget(editButton);
    layout->addWidget(removeButton);

    m_indicatorLabels[id] = nameLabel;
    m_hideButtons[id] = hideButton;
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
    if (tryOpenDialog<indicators::MACDInstance, MACDDialog>(id)) return;
    if (tryOpenDialog<indicators::BBInstance, bbDialog>(id)) return;
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
        createIndicatorWidgets(indicator->id, indicator->getDisplayName(), indicator->visible);
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
    if (checked) {
        // Disable marker drawing tools
        m_checkMarkerButton->setChecked(false);
        m_errorMarkerButton->setChecked(false);
    }
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
    createIndicatorWidgets(id, name, true);
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
        } else if (const indicators::MACDInstance* macd = dynamic_cast<const indicators::MACDInstance*>(indicator.get())) {
            m_chartWidget->addIndicator(*macd);
        } else if (const indicators::BBInstance* bb = dynamic_cast<const indicators::BBInstance*>(indicator.get())) {
            m_chartWidget->addIndicator(*bb);
        }
    }
}

void ChartControlPanel::setComparisonMode(bool enabled) {
    m_comparisonActive = enabled;
    m_exitComparisonButton->setVisible(enabled);
    
    if (enabled) {
        m_transferDataButton->setText("Refresh");
    } else {
        m_transferDataButton->setText("Copy");
    }
}

void ChartControlPanel::onCheckMarkerToggled(bool checked) {
    if (checked) {
        // Disable the other marker button and the ruler
        m_errorMarkerButton->setChecked(false);
        m_rulerToolButton->setChecked(false);
        
        if (m_chartWidget) {
            m_chartWidget->setMarkerDrawingEnabled(true, chart::MarkerType::Check);
        }
    } else {
        if (m_chartWidget) {
            m_chartWidget->setMarkerDrawingEnabled(false);
        }
    }
}

void ChartControlPanel::onErrorMarkerToggled(bool checked) {
    if (checked) {
        // Disable the other marker button and the ruler
        m_checkMarkerButton->setChecked(false);
        m_rulerToolButton->setChecked(false);
        
        if (m_chartWidget) {
            m_chartWidget->setMarkerDrawingEnabled(true, chart::MarkerType::Error);
        }
    } else {
        if (m_chartWidget) {
            m_chartWidget->setMarkerDrawingEnabled(false);
        }
    }
}

void ChartControlPanel::onClearMarkersClicked() {
    if (m_chartWidget) {
        m_chartWidget->clearAllMarkers();
    }
}

void ChartControlPanel::onHideIndicator(int id) {
    if (m_chartWidget) {
        m_chartWidget->toggleIndicatorVisibility(id);
    }
}