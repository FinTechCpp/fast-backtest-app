#pragma once

#include <QObject>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTableView>
#include <QGroupBox>
#include <QPushButton>
#include <QComboBox>
#include <QSplitter>
#include <QStackedWidget>
#include <QHeaderView>
#include <QChartView>
#include <QVariant>
#include <memory>
#include "ui/views/baseView.h"
#include "ui/metricWidget.h"
#include "ui/views/Stats/TradingHeatmapWidget.h"

#include "ui/views/Stats/FlexiblePieWidget.h"
#include "ui/views/Stats/SimpleTextWidget.h"
#include "ui/views/Stats/KeyValueListWidget.h"
#include "ui/views/Stats/VerticalGaugeRenderWidget.h"
#include "ui/views/Stats/HistogramWidget.h"
#include "ui/views/Stats/EquityWidget.h"

#include <cmath>

class App;

/**
 * @brief View to display backtest statistics.
 * Organizes metrics into categories.
 */
class StatsView : public BaseView {
    Q_OBJECT

public:
    /**
     * @brief Constructor for the statistics view
     * @param parent The parent widget
     */
    StatsView(QWidget* parent = nullptr);
    
    /**
     * @brief Destructor
     */
    ~StatsView();

    /**
     * @brief Updates the display with new backtest results
     * @param results The results to display
     */
    void updateData(BacktestResults* results) override;
    
    /**
     * @brief Clears all data and resets the display
     */
    void clear() override;

protected:
    /**
     * @brief Configures the user interface
     */
    void setupUI() override;

private:
    // ==================== Private Members ====================
    TradingHeatmapWidget* m_tradingHeatmapWidget = nullptr;

    EquityWidget* m_equityWidget = nullptr;
    
    FlexiblePieWidget* m_profitFactorWidget = nullptr;
    FlexiblePieWidget* m_exposureWidget = nullptr;
    FlexiblePieWidget* m_tradeDistributionWidget = nullptr;
    
    SimpleTextWidget* m_netProfitWidget = nullptr;
    SimpleTextWidget* m_grossProfitWidget = nullptr;
    SimpleTextWidget* m_grossLossWidget = nullptr;

    SimpleTextWidget* m_maxDDWidget = nullptr;
    SimpleTextWidget* m_averageDDWidget = nullptr;
    SimpleTextWidget* m_maxDDDurationWidget = nullptr;
    SimpleTextWidget* m_averageDDDurationWidget = nullptr;
    SimpleTextWidget* m_maxTradeDurationWidget = nullptr;
    SimpleTextWidget* m_averageTradeDurationWidget = nullptr;

    SimpleTextWidget* m_averageTradePerDayWidget = nullptr;
    
    KeyValueListWidget* m_tradeDetailsWidget = nullptr;
    KeyValueListWidget* m_timeInfoWidget = nullptr;
    KeyValueListWidget* m_returnsWidget = nullptr;
    KeyValueListWidget* m_buyHoldWidget = nullptr;
    KeyValueListWidget* m_performanceRatiosWidget = nullptr;
    KeyValueListWidget* m_marketExposureWidget = nullptr;
    KeyValueListWidget* m_MAEWidget = nullptr;
    KeyValueListWidget* m_systemQualityWidget = nullptr;
    KeyValueListWidget* m_skewnessKurtosisWidget = nullptr;

    VerticalGaugeRenderWidget* m_pnlGaugeWidget = nullptr;

    HistogramWidget* m_histogramWidget = nullptr;

    // --- State ---
    App* m_app;  // Reference to the main application
};
