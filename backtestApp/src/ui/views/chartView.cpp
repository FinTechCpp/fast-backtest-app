#include "ui/views/chartView.h"
#include "ui/app.h"
#include <QDebug>
#include <QTime>
#include <QLabel>
#include <QFrame>
#include <QResizeEvent>

ChartView::ChartView(QWidget* parent)
    : BaseView(parent)
    , m_comparisonMode(false)
    , m_chartPlaceholder(nullptr)
    , m_leftPanel(nullptr)
    , m_rightPanel(nullptr)
    , m_chartWidget(nullptr)
    , m_chartWidget2(nullptr)
{
    setupUI();
}

ChartView::~ChartView()
{
}

void ChartView::setupUI()
{
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    // Layout horizontal for the left and right panels
    QHBoxLayout* horizontalLayout = new QHBoxLayout();
    horizontalLayout->setContentsMargins(0, 0, 0, 0);
    horizontalLayout->setSpacing(0);
    m_mainLayout->addLayout(horizontalLayout);

    // Create the left control panel
    m_leftPanel = new ChartControlPanel(this);

    // Vertical separator
    QFrame* verticalSeparator = new QFrame();
    verticalSeparator->setFrameStyle(QFrame::VLine | QFrame::Plain);
    verticalSeparator->setStyleSheet("color: #CCCCCC;");

    // Right panel containing the chart
    m_rightPanel = new QWidget();
    m_rightPanel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // Layout for the right panel
    m_rightPanelLayout = new QStackedLayout(m_rightPanel);
    m_rightPanelLayout->setContentsMargins(0, 0, 0, 0);
    m_rightPanelLayout->setSpacing(0);

    // Initial placeholder
    m_chartPlaceholder = new QLabel("Run the backtest to display the charts");
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
    m_chartContainer = new QWidget();
    QHBoxLayout* chartLayout = new QHBoxLayout(m_chartContainer);
    chartLayout->setContentsMargins(0, 0, 0, 0);
    chartLayout->setSpacing(0);

    // Chart widget
    m_chartWidget = new ChartWidget();
    m_chartWidget2 = new ChartWidget(); // For vertical comparison
    m_chartWidget->setVisible(true);
    m_chartWidget2->setVisible(false);

    // Associate the chart widget with the control panel
    m_leftPanel->setChartWidget(m_chartWidget);

    // Connect control panel signals
    connect(m_leftPanel, &ChartControlPanel::chartTypeChanged, [this](const QString& chartType) {
        m_chartWidget->setChartType(chart::stringToChartType(chartType.toStdString()));
        if (m_comparisonMode && m_chartWidget2) {
            m_chartWidget2->setChartType(chart::stringToChartType(chartType.toStdString()));
        }
    });
    
    connect(m_leftPanel, &ChartControlPanel::rulerToolToggled, [this](bool checked) {
        m_chartWidget->setRulerToolEnabled(checked);
        if (m_comparisonMode && m_chartWidget2) {
            m_chartWidget2->setRulerToolEnabled(checked);
        }
    });

    connect(m_leftPanel, &ChartControlPanel::transferDataForComparison, this, [this]() {
        if (!m_chartWidget || !m_chartWidget2 || !m_currentResults) return;

        // Connect signals for synchronization
        connect(m_chartWidget, &ChartWidget::aggregationChanged,
                m_chartWidget2, &ChartWidget::setCurrentAggregation);
        connect(m_chartWidget2, &ChartWidget::aggregationChanged, 
                m_chartWidget, &ChartWidget::setCurrentAggregation);
        connect(m_chartWidget, &ChartWidget::viewportChanged, 
                m_chartWidget2, &ChartWidget::setViewport);
        connect(m_chartWidget2, &ChartWidget::viewportChanged, 
                m_chartWidget, &ChartWidget::setViewport);
        connect(m_chartWidget, &ChartWidget::trackFinanceUpdated,
                m_chartWidget2, &ChartWidget::forceUpdateTrackFinance);
        connect(m_chartWidget2, &ChartWidget::trackFinanceUpdated,
                m_chartWidget, &ChartWidget::forceUpdateTrackFinance);

        m_chartWidget->setSyncPartner(m_chartWidget2);
        m_chartWidget2->setSyncPartner(m_chartWidget);

        m_chartWidget2->setVisible(true);

        m_chartWidget2->setBacktestResults(m_currentResults);

        m_comparisonMode = true;
        m_leftPanel->setComparisonMode(true);
    });
    
    connect(m_leftPanel, &ChartControlPanel::exitComparisonMode, this, [this]() {
        if (!m_chartWidget || !m_chartWidget2)
            return;

        // DDisconnect synchronization signals
        disconnect(m_chartWidget, &ChartWidget::aggregationChanged,
                   m_chartWidget2, &ChartWidget::setCurrentAggregation);
        disconnect(m_chartWidget2, &ChartWidget::aggregationChanged, 
                    m_chartWidget, &ChartWidget::setCurrentAggregation);
        disconnect(m_chartWidget, &ChartWidget::viewportChanged, 
                    m_chartWidget2, &ChartWidget::setViewport);
        disconnect(m_chartWidget2, &ChartWidget::viewportChanged, 
                    m_chartWidget, &ChartWidget::setViewport);
        disconnect(m_chartWidget, &ChartWidget::trackFinanceUpdated,
                    m_chartWidget2, &ChartWidget::forceUpdateTrackFinance);
        disconnect(m_chartWidget2, &ChartWidget::trackFinanceUpdated,
                    m_chartWidget, &ChartWidget::forceUpdateTrackFinance);
        
        m_chartWidget->setSyncPartner(nullptr);
        m_chartWidget2->setSyncPartner(nullptr);

        m_chartWidget2->setVisible(false);
        
        m_comparisonMode = false;
        m_leftPanel->setComparisonMode(false);
    });
    
    connect(m_leftPanel, &ChartControlPanel::aggregationValueChanged, [this](int value) {
        m_chartWidget->setMaxDisplayPoints(value);
        if (m_comparisonMode && m_chartWidget2) {
            m_chartWidget2->setMaxDisplayPoints(value);
        }
    });

    chartLayout->addWidget(m_chartWidget, /*stretch=*/1);
    chartLayout->addWidget(m_chartWidget2, /*stretch=*/1);

    // Add widgets to the right panel layout
    m_rightPanelLayout->addWidget(m_chartPlaceholder);  
    m_rightPanelLayout->addWidget(m_chartContainer);

    // Add components to the horizontal layout
    horizontalLayout->addWidget(m_leftPanel);
    horizontalLayout->addWidget(verticalSeparator);
    horizontalLayout->addWidget(m_rightPanel);

    // Configure the widget to stretch
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void ChartView::updateData(BacktestResults* results) {
    // BUG FIX: Don't access m_currentResults - it may be a dangling pointer
    // when a new backtest destroys the previous BacktestResults.
    // Instead, get markers from the chart widget and save them in the NEW results.
    
    m_currentResults = results;

    if (!results || results->candles.empty()) {
        clear();
        showPlaceholder("No data available");
        return;
    }

    m_chartWidget->setBacktestResults(results);

    // Restore the saved markers for this backtest
    if (!results->userMarkers.empty()) 
        m_chartWidget->setMarkers(results->userMarkers);

    // Show the chart widget
    showChartWidget();



    m_leftPanel->suggestIndicatorsFromStrategy(
        extractIndicatorsFromFilters(results->strategyConfigs)
    );

    // Update the indicators list
    m_leftPanel->refreshIndicatorsList();
}

void ChartView::showChartWidget() {
    m_rightPanelLayout->setCurrentWidget(m_chartContainer);
}

void ChartView::showPlaceholder(const QString& message) {
    m_chartPlaceholder->setText(message);
    m_rightPanelLayout->setCurrentWidget(m_chartPlaceholder);
}

void ChartView::clear() {
    m_leftPanel->refreshIndicatorsList();
    showPlaceholder("Run a backtest to display charts");
}

void ChartView::zoomToTrade(const be::TradeData& trade) {
    qDebug() << "ChartView::zoomToTrade called for trade with entry:" << trade.entryDate.toString().c_str();

    // Ensure the chart widget is visible
    showChartWidget();

    // Delegate zooming to the ChartWidget
    m_chartWidget->zoomToTrade(trade);
}

void ChartView::zoomToPeriod(const QDateTime& startDate, const QDateTime& endDate) {
    qDebug() << "ChartView::zoomToPeriod called for period from" 
             << startDate.toString("dd/MM/yyyy hh:mm:ss")
             << "to" << endDate.toString("dd/MM/yyyy hh:mm:ss");

    // Ensure the chart widget is visible
    showChartWidget();

    // Delegate zooming to the ChartWidget
    m_chartWidget->zoomToPeriod(startDate, endDate);
}

std::vector<std::unique_ptr<indicators::IndicatorBase>> ChartView::extractIndicatorsFromFilters(const std::vector<StrategyConfig>& strategyConfigs) {
    std::vector<std::unique_ptr<indicators::IndicatorBase>> indicatorInstances;
        

    // Utility function to add if not already present
    auto addIfNotPresent = [&](std::unique_ptr<indicators::IndicatorBase> candidate) {
        for (const auto& existing : indicatorInstances) {
            if (candidate && existing && candidate->isCalculationParamsEqual(*existing)) {
                return; // already present, do not add
            }
        }
        indicatorInstances.push_back(std::move(candidate));
    };

    // Helper function to extract indicators from ValueSource
    auto extractIndicator = [&](const filter::ValueSource& source) {
        if (source.category != filter::ValueCategory::INDICATOR) {
            return;
        }
        
        switch (source.indicatorType) {
            case filter::IndicatorType::EMA: {
                auto ema = std::make_unique<indicators::EMAInstance>();
                ema->period = source.emaParams.period;
                addIfNotPresent(std::move(ema));
                break;
            }
            case filter::IndicatorType::RSI: {
                auto rsi = std::make_unique<indicators::RSIInstance>();
                rsi->period = source.rsiParams.period;
                addIfNotPresent(std::move(rsi));
                break;
            }
            case filter::IndicatorType::ATR: {
                auto atr = std::make_unique<indicators::ATRInstance>();
                atr->period = source.atrParams.period;
                atr->useLogScale = source.atrParams.useLog;
                addIfNotPresent(std::move(atr));
                break;
            }
            case filter::IndicatorType::STOCHASTIC_K:
            case filter::IndicatorType::STOCHASTIC_D: {
                auto stoch = std::make_unique<indicators::StochasticInstance>();
                stoch->fastKPeriod = source.stochParams.fastK;
                stoch->slowKPeriod = source.stochParams.slowK;
                stoch->slowDPeriod = source.stochParams.slowD;
                addIfNotPresent(std::move(stoch));
                break;
            }
            case filter::IndicatorType::SUPERTREND_VALUE:
            case filter::IndicatorType::SUPERTREND_DIRECTION: {
                auto supertrend = std::make_unique<indicators::SuperTrendInstance>();
                supertrend->period = source.supertrendParams.atrPeriod;
                supertrend->multiplier = source.supertrendParams.multiplier;
                addIfNotPresent(std::move(supertrend));
                break;
            }
            case filter::IndicatorType::CCI: {
                auto cci = std::make_unique<indicators::CCIInstance>();
                cci->period = source.cciParams.period;
                addIfNotPresent(std::move(cci));
                break;
            }
            case filter::IndicatorType::MACD_HISTOGRAM:
            case filter::IndicatorType::MACD_LINE:
            case filter::IndicatorType::MACD_SIGNAL: {
                auto macd = std::make_unique<indicators::MACDInstance>();
                macd->fastPeriod = source.macdParams.fast;
                macd->slowPeriod = source.macdParams.slow;
                macd->signalPeriod = source.macdParams.signal;
                addIfNotPresent(std::move(macd));
                break;
            }
            case filter::IndicatorType::BB_UPPER:
            case filter::IndicatorType::BB_LOWER:
            case filter::IndicatorType::BB_PERCENT_B: {
                auto bb = std::make_unique<indicators::BBInstance>();
                bb->period = source.bbParams.period;
                // bbParams stores ma_type and source as ints; cast to the enum types expected by BBInstance
                bb->ma_type = static_cast<filter::MAType>(source.bbParams.ma_type);
                bb->source = static_cast<filter::PriceType>(source.bbParams.source);
                bb->stddev_multiplier = source.bbParams.stddev_multiplier;
                addIfNotPresent(std::move(bb));
                break;
            }
            default:
                break;
        }
    };
    
    for (const auto& strategyConfig : strategyConfigs) {
        // Iterate through all filters and extract indicators
        for (const auto& filter : strategyConfig.buyFilters) {
            extractIndicator(filter.leftValue);
            extractIndicator(filter.rightValue);
        }

        for (const auto& filter : strategyConfig.sellFilters) {
            extractIndicator(filter.leftValue);
            extractIndicator(filter.rightValue);
        }

        for (const auto& filter : strategyConfig.rebuyFilters) {
            extractIndicator(filter.leftValue);
            extractIndicator(filter.rightValue);
        }

        for (const auto& filter : strategyConfig.resaleFilters) {
            extractIndicator(filter.leftValue);
            extractIndicator(filter.rightValue);
        }
        // Also add indicators used for SL/TP
        if (strategyConfig.sl_method == StopLossMethod::ATR || 
            strategyConfig.tp_method == TakeProfitMethod::ATR) {
            auto atr = std::make_unique<indicators::ATRInstance>();
            atr->period = strategyConfig.atr_period;
            atr->useLogScale = true;
            addIfNotPresent(std::move(atr));
        }
    }
    
    return indicatorInstances;
}