#include "ui/views/statsView.h"
#include "ui/app.h"
#include <QDebug>
#include <QTime>
#include <QPieSeries>
#include <QBarSeries>
#include <QBarSet>
#include <QBarCategoryAxis>
#include <QValueAxis>
#include <QPieSlice>
#include <QScatterSeries>
#include <QLineSeries>


/*
gantt chart for trades
5. Additional features for a complete visualization
Here are some additional ideas that could be integrated:

Equity curve with drawdown bands: Show the evolution of capital with colored areas indicating drawdowns

Analysis of trading days/times: Heatmap showing performance by day of the week/time of day

Performance vs volatility chart: Positioning your strategy against others in a risk/return chart

Improvement indicators: Visual suggestions on aspects to improve in the strategy

Global analysis widget: displays only critical statistics (those that are not good) this allows to show the user the weaknesses of the strategy. this prevents missing a red flag: for example if everything is green but for example an extremely bad sharp ratio: we will show it to the user as a red light on a car to say that there is a problem.
*/


// Implementation of StatsView
StatsView::StatsView(QWidget* parent)
    : BaseView(parent),
      m_app(nullptr)
{
    // Find the parent application
    QWidget* widget = parent;
    while (widget && !m_app) {
        m_app = qobject_cast<App*>(widget);
        widget = widget->parentWidget();
    }
    
    setupUI();
}

StatsView::~StatsView()
{
    // Models and widgets are automatically destroyed by Qt
}

void StatsView::setupUI() {

    // WIDGET CREATION
    // --------------------------------------
    QGridLayout* gridLayout = new QGridLayout();
    gridLayout->setContentsMargins(0, 0, 0, 0);
    gridLayout->setSpacing(0);

    // --------------------------------------
    // Line 0
    // --------------------------------------
    m_timeInfoWidget = new KeyValueListWidget("Temporal Information");
    m_timeInfoWidget->addItem("Start ", "--", Qt::black);
    m_timeInfoWidget->addItem("End   ", "--", Qt::black);
    m_timeInfoWidget->addItem("Duration ", "--", Qt::black);
    m_timeInfoWidget->setMinimumHeight(m_timeInfoWidget->sizeHint().height());
    gridLayout->addWidget(m_timeInfoWidget, 0, 0, 1, 3); // Spreads over 3 columns

    // Add a stretch that spans the 3 columns (e.g. at line 9)
    // TODO replace with the rest of the stats widgets
    // QSpacerItem* horizontalSpacer = new QSpacerItem(20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    // gridLayout->addItem(horizontalSpacer, 0, 4, 8, 3);  // line 0, column 4, height 8, width 3

    m_equityWidget = new EquityWidget("Equity Curve");
    gridLayout->addWidget(m_equityWidget, 0, 4, 3, 3);



    // --------------------------------------
    // Line 1
    // --------------------------------------
    m_netProfitWidget = new SimpleTextWidget("Net Profit");
    m_netProfitWidget->setStatColors(QColor(0, 150, 0));
    m_netProfitWidget->setBackgroundColor(QColor(0, 150, 0).lighter(300));
    m_netProfitWidget->setSuffix(" €");
    m_netProfitWidget->setSuffixBis(" %");
    m_netProfitWidget->setCheckBoxBisString("Percentage");
    gridLayout->addWidget(m_netProfitWidget, 1, 0, 1, 2); // Spreads over 2 columns

    m_pnlGaugeWidget = new VerticalGaugeRenderWidget("PnL Distribution");
    gridLayout->addWidget(m_pnlGaugeWidget, 1, 2, 4, 1);


    // --------------------------------------
    // Line 2
    // --------------------------------------
    m_tradeDistributionWidget = new FlexiblePieWidget("Trade Distribution");
    m_tradeDistributionWidget->setCenterTextColor(QColor(0, 150, 0));
    m_tradeDistributionWidget->setCenterTextSuffix(" %");
    gridLayout->addWidget(m_tradeDistributionWidget, 2, 0);
    
    m_profitFactorWidget = new FlexiblePieWidget("Profit Factor");
    m_profitFactorWidget->setCenterTextColor(QColor(0, 150, 0));
    gridLayout->addWidget(m_profitFactorWidget, 2, 1);


    // --------------------------------------
    // Line 3
    // --------------------------------------
    m_tradeDetailsWidget = new KeyValueListWidget("Trade details");
    m_tradeDetailsWidget->addItem("Total ", "--", QColor(0, 0, 0), "--");
    m_tradeDetailsWidget->addItem("Take Profit ", "-- %", QColor(0, 150, 0), "--");
    m_tradeDetailsWidget->addItem("Break Even ", "-- %", QColor(0, 0, 150), "--");
    m_tradeDetailsWidget->addItem("Stop Loss ", "-- %", QColor(150, 0, 0), "--");
    m_tradeDetailsWidget->addItem("Manual ", "-- %", Qt::black, "--");
    m_tradeDetailsWidget->setMinimumSize(m_tradeDetailsWidget->sizeHint());
    m_tradeDetailsWidget->setCheckBoxBisString("Count");
    gridLayout->addWidget(m_tradeDetailsWidget, 3, 0, 2, 1); // Spans 2 rows

    m_grossProfitWidget = new SimpleTextWidget("Gross Profit");
    m_grossProfitWidget->setStatColors(QColor(0, 150, 0));
    m_grossProfitWidget->setBackgroundColor(QColor(0, 150, 0).lighter(300));
    m_grossProfitWidget->setSuffix(" €");
    gridLayout->addWidget(m_grossProfitWidget, 3, 1);


    m_tradingHeatmapWidget = new TradingHeatmapWidget("Trading Heatmap");
    gridLayout->addWidget(m_tradingHeatmapWidget, 3, 4, 5, 1);


    m_returnsWidget = new KeyValueListWidget("Returns");
    m_returnsWidget->addItem("Annualized ", "--", Qt::black);
    m_returnsWidget->addItem("CAGR ", "--", Qt::black);
    m_returnsWidget->addItem("Alpha ", "--", Qt::black);
    m_returnsWidget->setMinimumHeight(m_returnsWidget->sizeHint().height());
    gridLayout->addWidget(m_returnsWidget, 3, 5, 2, 1);


    // m_equityDetailsWidget = new KeyValueListWidget("Equity Details");
    // m_equityDetailsWidget->addItem("Initial ", "--", Qt::black);
    // m_equityDetailsWidget->addItem("Final ", "--", QColor(0, 150, 0));
    // m_equityDetailsWidget->addItem("Peak ", "--", Qt::black);
    // m_equityDetailsWidget->setMinimumHeight(m_equityDetailsWidget->sizeHint().height());
    // gridLayout->addWidget(m_equityDetailsWidget, 3, 6);

    m_buyHoldWidget = new KeyValueListWidget("Buy & Hold");
    m_buyHoldWidget->addItem("Return ", "--", Qt::black);
    m_buyHoldWidget->addItem("CAGR ", "--", Qt::black);
    m_buyHoldWidget->setMinimumHeight(m_buyHoldWidget->sizeHint().height());
    gridLayout->addWidget(m_buyHoldWidget, 3, 6, 2, 1);


    // --------------------------------------
    // Line 4
    // --------------------------------------
    m_grossLossWidget = new SimpleTextWidget("Gross Loss");
    m_grossLossWidget->setStatColors(QColor(150, 0, 0));
    m_grossLossWidget->setBackgroundColor(QColor(150, 0, 0).lighter(300));
    m_grossLossWidget->setSuffix(" €");
    gridLayout->addWidget(m_grossLossWidget, 4, 1);


    // --------------------------------------
    // Line 5
    // --------------------------------------
    m_maxDDWidget = new SimpleTextWidget("Max Drawdown");
    m_maxDDWidget->setStatColors(QColor(150, 0, 0));
    m_maxDDWidget->setSuffix(" %");
    gridLayout->addWidget(m_maxDDWidget, 5, 0);

    m_maxDDDurationWidget = new SimpleTextWidget("Max Drawdown Duration");
    m_maxDDDurationWidget->setStatColors(Qt::black);
    gridLayout->addWidget(m_maxDDDurationWidget, 5, 1);

    m_maxTradeDurationWidget = new SimpleTextWidget("Max Trade Duration");
    m_maxTradeDurationWidget->setStatColors(Qt::black);
    gridLayout->addWidget(m_maxTradeDurationWidget, 5, 2);

    m_performanceRatiosWidget = new KeyValueListWidget("Performance Ratios");
    m_performanceRatiosWidget->addItem("Sharpe ", "--", Qt::black);
    m_performanceRatiosWidget->addItem("Sortino ", "--", Qt::black);
    m_performanceRatiosWidget->addItem("Calmar ", "--", Qt::black);
    m_performanceRatiosWidget->addItem("Omega ", "--", Qt::black);
    m_performanceRatiosWidget->setMinimumHeight(m_performanceRatiosWidget->sizeHint().height());
    gridLayout->addWidget(m_performanceRatiosWidget, 5, 5, 2, 2); // Spans 2 columns


    // --------------------------------------
    // Line 6
    // --------------------------------------
    m_averageDDWidget = new SimpleTextWidget("Average Drawdown");
    m_averageDDWidget->setStatColors(QColor(150, 0, 0));
    m_averageDDWidget->setSuffix(" %");
    gridLayout->addWidget(m_averageDDWidget, 6, 0);

    m_averageDDDurationWidget = new SimpleTextWidget("Average Drawdown Duration");
    m_averageDDDurationWidget->setStatColors(Qt::black);
    gridLayout->addWidget(m_averageDDDurationWidget, 6, 1);
    
    m_averageTradeDurationWidget = new SimpleTextWidget("Average Trade Duration");
    m_averageTradeDurationWidget->setStatColors(Qt::black);
    gridLayout->addWidget(m_averageTradeDurationWidget, 6, 2);


    // --------------------------------------
    // Line 7
    // --------------------------------------
    m_exposureWidget = new FlexiblePieWidget("Exposure Time");
    m_exposureWidget->setStartAngle(180);
    m_exposureWidget->setAngleSpan(180);
    m_exposureWidget->setCenterTextColor(QColor(0, 0, 0));
    m_exposureWidget->setCenterTextSuffix(" %");
    gridLayout->addWidget(m_exposureWidget, 7, 0);


    m_histogramWidget = new HistogramWidget("Returns Distribution");
    gridLayout->addWidget(m_histogramWidget, 7, 1, 2, 2);

    m_marketExposureWidget = new KeyValueListWidget("Market Exposure");
    m_marketExposureWidget->addItem("Beta ", "--", Qt::black);
    m_marketExposureWidget->addItem("Annual Volatility ", "-- %", Qt::black);
    m_marketExposureWidget->setMinimumHeight(m_marketExposureWidget->sizeHint().height());
    gridLayout->addWidget(m_marketExposureWidget, 7, 5, 1, 2); // Spans 2 columns

    // --------------------------------------
    // Line 8
    // --------------------------------------
    m_averageTradePerDayWidget = new SimpleTextWidget("Average Trade Per Day");
    m_averageTradePerDayWidget->setStatColors(QColor(0, 150, 0));
    m_averageTradePerDayWidget->setMinimumHeight(m_exposureWidget->sizeHint().height());
    m_averageTradePerDayWidget->setBackgroundIcon(QIcon(":/icons/24hClock7.png"));
    m_averageTradePerDayWidget->setBackgroundIconSize(QSize(120, 120));
    m_averageTradePerDayWidget->setBackgroundIconAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
    m_averageTradePerDayWidget->setBackgroundIconOpacity(0.6);
    m_averageTradePerDayWidget->setFontSize(18);
    gridLayout->addWidget(m_averageTradePerDayWidget, 8, 0);


    m_MAEWidget = new KeyValueListWidget("Maximum Adverse Excursion (MAE)");
    m_MAEWidget->addItem("MAE Average ", "--", Qt::black);
    m_MAEWidget->addItem("MAE Max ", "--", Qt::black);
    m_MAEWidget->setMinimumHeight(m_MAEWidget->sizeHint().height());
    gridLayout->addWidget(m_MAEWidget, 8, 4, 1, 1);


    m_systemQualityWidget = new KeyValueListWidget("System Quality Metrics");
    m_systemQualityWidget->addItem("SQN ", "--", Qt::black);
    m_systemQualityWidget->addItem("Kelly Criterion ", "--", Qt::black);
    m_systemQualityWidget->setMinimumHeight(m_systemQualityWidget->sizeHint().height());
    gridLayout->addWidget(m_systemQualityWidget, 8, 5, 1, 1);


    m_skewnessKurtosisWidget = new KeyValueListWidget("Skewness & Kurtosis");
    m_skewnessKurtosisWidget->addItem("Skewness ", "--", Qt::black);
    m_skewnessKurtosisWidget->addItem("Kurtosis ", "--", Qt::black);
    m_skewnessKurtosisWidget->setMinimumHeight(m_skewnessKurtosisWidget->sizeHint().height());
    gridLayout->addWidget(m_skewnessKurtosisWidget, 8, 6, 1, 1);

    // Final layout setup
    // --------------------------------------
    // Add the gridLayout directly to the main layout (no scroll area)
    m_mainLayout->addLayout(gridLayout);
}

void StatsView::updateData(BacktestResults* results)
{    
    // Store the results for later updates
    m_currentResults = results;
    
    if (!m_currentResults) {
        qWarning() << "Null results received";
        clear();
        return;
    }

    m_timeInfoWidget->updateValue("Start ", "  " + QString::fromStdString(m_currentResults->stats.start.toString()));
    m_timeInfoWidget->updateValue("End   ", "  " + QString::fromStdString(m_currentResults->stats.end.toString()));
    m_timeInfoWidget->updateValue("Duration ", "  " + QString::fromStdString(m_currentResults->stats.duration.toString()));



    // Display net profit in euros AND as a percentage (relative to equityInitial)
    double netProfit = m_currentResults->stats.equityFinal - m_currentResults->stats.equityInitial;
    QString euroText = SimpleTextWidget::formatWithThousandsSeparator(netProfit);
    QString pctText;
    if (m_currentResults->stats.equityInitial != 0.0) {
        double pct = (netProfit / m_currentResults->stats.equityInitial) * 100.0;
        pctText = QString::number(pct, 'f', 2);
    } else {
        pctText = "--";
    }

    m_netProfitWidget->setStatText(euroText);
    m_netProfitWidget->setStatTextBis(pctText);
    // Maybe integrate this mechanism directly into the widget
    if (m_currentResults->stats.equityFinal - m_currentResults->stats.equityInitial > 0) {
        m_netProfitWidget->setStatColors(QColor(0, 150, 0));
        m_netProfitWidget->setBackgroundColor(QColor(0, 150, 0).lighter(300));
    } else {
        m_netProfitWidget->setStatColors(QColor(150, 0, 0));
        m_netProfitWidget->setBackgroundColor(QColor(150, 0, 0).lighter(300));
    }

    m_profitFactorWidget->setCenterText(QString::number(m_currentResults->stats.profitFactor, 'f', 2));
    m_profitFactorWidget->clearSegments();
    m_profitFactorWidget->addSegment(
        std::clamp((m_currentResults->stats.grossProfit / (m_currentResults->stats.grossLoss + m_currentResults->stats.grossProfit)), 0.0, 1.0),
        QColor(0, 200, 0)
    );
    m_profitFactorWidget->addSegment(
        std::clamp((m_currentResults->stats.grossLoss / (m_currentResults->stats.grossLoss + m_currentResults->stats.grossProfit)), 0.0, 1.0),
        QColor(200, 0, 0)
    );

    m_exposureWidget->setCenterText(QString::number(std::clamp(m_currentResults->stats.exposureTimePct, 0.0, 100.0), 'f', 1));
    m_exposureWidget->clearSegments();
    m_exposureWidget->addSegment(std::clamp(m_currentResults->stats.exposureTimePct / 100.0, 0.0, 1.0), QColor(128, 179, 255)); // QColor(128, 179, 255), QColor(209, 212, 230)
    m_exposureWidget->addSegment(std::clamp(1.0 - m_currentResults->stats.exposureTimePct / 100.0, 0.0, 1.0), QColor(209, 212, 230));

    unsigned int totalTrades = m_currentResults->stats.numTrades;
    m_tradeDistributionWidget->clearSegments();
    if (totalTrades > 0) {
        double TP = (static_cast<double>(m_currentResults->stats.numTPTrades) / totalTrades);
        double SL = (static_cast<double>(m_currentResults->stats.numSLTrades) / totalTrades);
        double BE = (static_cast<double>(m_currentResults->stats.numBETrades) / totalTrades);
        double manual = (static_cast<double>(m_currentResults->stats.numManualTrades) / totalTrades);
        double unknown = (static_cast<double>(m_currentResults->stats.numUnknownTrades) / totalTrades);
        
        m_tradeDistributionWidget->addSegment(TP, QColor(0, 200, 0));
        m_tradeDistributionWidget->addSegment(SL, QColor(200, 0, 0));
        m_tradeDistributionWidget->addSegment(BE, QColor(10, 100, 200));
        m_tradeDistributionWidget->addSegment(manual, QColor(127, 140, 141));
        m_tradeDistributionWidget->addSegment(unknown, QColor(44, 62, 80));

        m_tradeDistributionWidget->setCenterText(QString::number(TP * 100.0, 'f', 1));
    }
    else {
        m_tradeDistributionWidget->setCenterText("--");
    }


    m_grossProfitWidget->setStatText(SimpleTextWidget::formatWithThousandsSeparator(m_currentResults->stats.grossProfit));
    m_grossLossWidget->setStatText(SimpleTextWidget::formatWithThousandsSeparator(-m_currentResults->stats.grossLoss));

    // Update trade details: percentage (primary value) and absolute number (valueBis)
    m_tradeDetailsWidget->updateValues("Total ", 
        QString::number(m_currentResults->stats.numTrades),
        QString::number(m_currentResults->stats.numTrades));
    
    m_tradeDetailsWidget->updateValues("Take Profit ", 
        QString::number(m_currentResults->stats.pctTPTrades, 'f', 2) + " %",
        QString::number(m_currentResults->stats.numTPTrades));
    
    m_tradeDetailsWidget->updateValues("Break Even ", 
        QString::number(m_currentResults->stats.pctBETrades, 'f', 2) + " %",
        QString::number(m_currentResults->stats.numBETrades));
    
    m_tradeDetailsWidget->updateValues("Stop Loss ", 
        QString::number(m_currentResults->stats.pctSLTrades, 'f', 2) + " %",
        QString::number(m_currentResults->stats.numSLTrades));
    
    m_tradeDetailsWidget->updateValues("Manual ", 
        QString::number(m_currentResults->stats.pctManualTrades, 'f', 2) + " %",
        QString::number(m_currentResults->stats.numManualTrades));


    m_pnlGaugeWidget->updateContent(m_currentResults->stats);

    m_maxDDWidget->setStatText(QString::number(m_currentResults->stats.maxDrawdownPct, 'f', 2));
    m_averageDDWidget->setStatText(QString::number(m_currentResults->stats.avgDrawdownPct, 'f', 2));
    m_maxDDDurationWidget->setStatText(QString::fromStdString(m_currentResults->stats.maxDrawdownDuration.toString()));
    m_averageDDDurationWidget->setStatText(QString::fromStdString(m_currentResults->stats.avgDrawdownDuration.toString()));
    m_maxTradeDurationWidget->setStatText(QString::fromStdString(m_currentResults->stats.maxTradeDuration.toString()));
    m_averageTradeDurationWidget->setStatText(QString::fromStdString(m_currentResults->stats.avgTradeDuration.toString()));


    m_averageTradePerDayWidget->setStatText(QString::number(m_currentResults->stats.tradesPerDay, 'f', 2));

    m_histogramWidget->updateData(m_currentResults);

    std::vector<be::Date> dates(m_currentResults->candles.size());
    for (size_t i = 0; i < m_currentResults->candles.size(); ++i) {
        dates[i] = m_currentResults->candles[i].date;
    }

    m_equityWidget->setPoints(dates, m_currentResults->stats.equityCurve);

    m_tradingHeatmapWidget->updateContent(m_currentResults->stats.trades);



    m_returnsWidget->updateValue("Annualized ", QString::number(m_currentResults->stats.returnAnnPct, 'f', 1) + " %");
    m_returnsWidget->updateValue("CAGR ", QString::number(m_currentResults->stats.cagrPct, 'f', 1) + " %");
    m_returnsWidget->updateValue("Alpha ", QString::number(m_currentResults->stats.alphaPct, 'f', 1) + " %");


    // m_equityDetailsWidget->updateValue("Initial ", SimpleTextWidget::formatWithThousandsSeparator(m_currentResults->stats.equityInitial, 0) + " €");
    // m_equityDetailsWidget->updateValue("Final ", SimpleTextWidget::formatWithThousandsSeparator(m_currentResults->stats.equityFinal, 0) + " €");
    // m_equityDetailsWidget->updateValue("Peak ", SimpleTextWidget::formatWithThousandsSeparator(m_currentResults->stats.equityPeak, 0) + " €");


    m_buyHoldWidget->updateValue("Return ", QString::number(m_currentResults->stats.buyHoldReturnPct, 'f', 1) + " %");
    m_buyHoldWidget->updateValue("CAGR ", QString::number(m_currentResults->stats.buyHoldCagrPct, 'f', 1) + " %");


    m_performanceRatiosWidget->updateValue("Sharpe ", QString::number(m_currentResults->stats.sharpeRatio, 'f', 1));
    m_performanceRatiosWidget->updateValue("Sortino ", QString::number(m_currentResults->stats.sortinoRatio, 'f', 1));
    m_performanceRatiosWidget->updateValue("Calmar ", QString::number(m_currentResults->stats.calmarRatio, 'f', 1));
    m_performanceRatiosWidget->updateValue("Omega ", QString::number(m_currentResults->stats.omegaRatio, 'f', 1));


    m_marketExposureWidget->updateValue("Beta ", QString::number(m_currentResults->stats.beta, 'f', 1));
    m_marketExposureWidget->updateValue("Annual Volatility ", QString::number(m_currentResults->stats.volatilityAnnPct, 'f', 1) + " %");


    m_MAEWidget->updateValue("MAE Average ", QString::number(m_currentResults->stats.avgMAE, 'f', 2));
    m_MAEWidget->updateValue("MAE Max ", QString::number(m_currentResults->stats.maxMAE, 'f', 2));


    m_systemQualityWidget->updateValue("SQN ", QString::number(m_currentResults->stats.sqn, 'f', 2));
    m_systemQualityWidget->updateValue("Kelly Criterion ", QString::number(m_currentResults->stats.kellyCriterion, 'f', 2));


    m_skewnessKurtosisWidget->updateValue("Skewness ", QString::number(m_currentResults->stats.skewness, 'f', 2));
    m_skewnessKurtosisWidget->updateValue("Kurtosis ", QString::number(m_currentResults->stats.kurtosis, 'f', 2));
}

void StatsView::clear() {
    qDebug() << "StatsView::clear() called";
    
    m_currentResults = nullptr;

    m_profitFactorWidget->setCenterText("--");
    m_profitFactorWidget->clearSegments();
    
    m_exposureWidget->setCenterText("--");
    m_exposureWidget->clearSegments();
    
    m_tradeDistributionWidget->setCenterText("--");
    m_tradeDistributionWidget->clearSegments();

    m_grossProfitWidget->setStatText("--");
    m_grossLossWidget->setStatText("--");

    m_pnlGaugeWidget->clear();
}