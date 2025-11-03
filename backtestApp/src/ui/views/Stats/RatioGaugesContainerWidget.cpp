#include "ui/views/Stats/RatioGaugesContainerWidget.h"
#include <QGridLayout>
#include <QLabel>
#include <QDebug>

RatioGaugesContainerWidget::RatioGaugesContainerWidget(QWidget* parent)
    : StatsBaseWidget(parent)
{
    // Main layout
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    
    // Content widget
    m_contentWidget = new QWidget();
    m_mainLayout->addWidget(m_contentWidget);
    
    // Grid layout for gauges (2 columns)
    QGridLayout* gridLayout = new QGridLayout(m_contentWidget);
    gridLayout->setSpacing(5);


    QColor DarkGray(140, 140, 140);
    QColor MediumGray(200, 200, 200);
    QColor LightGray(240, 240, 240);
    QColor Red(217, 76, 60);
    QColor Black(0, 0, 0);
    QColor Green(92, 184, 92);

    // exposureTimePct
    QVector<RatioGaugeWidget::GaugeZone> exposureTimeZones = {
        {0.0, 100.0, LightGray, Black, "*"}         // Very light gray
    };
    QString exposureTimeExplanation =
        "The <b>exposure time percentage</b> indicates the proportion of time during which the trading system is active in the market. "
        "A higher percentage can indicate better capital utilization, but can also increase exposure to market fluctuations.";
    m_exposureTimeGauge = new RatioGaugeWidget(exposureTimeZones, "Exposure Time (%)", exposureTimeExplanation);
    m_exposureTimeGauge->setValueFormat(1, true); // Display as percentage with 1 decimal

    // QVector<RatioGaugeWidget::GaugeZone> sharpeZones = {
    //     {-1.0, 0.0, QColor(217, 83, 79), QColor(0, 0, 0), "Mauvais"},                          // Red
    //     { 0.0, 0.5, QColor(240, 173, 78), QColor(0, 0, 0), "Faible"},                         // Orange
    //     { 0.5, 1.0, QColor(240, 240, 80), QColor(0, 0, 0), "Moyen"},                         // Yellow
    //     { 1.0, 1.5, QColor(150, 200, 80), QColor(0, 0, 0), "Bon"},                           // Yellow-green
    //     { 1.5, 2.5, QColor(92, 184, 92), QColor(0, 0, 0), "Très bon"},                       // Green
    //     { 2.5, 4.0, QColor(32, 150, 80), QColor(0, 0, 0), "Excellent"}                      // Dark green
    // };
    QVector<RatioGaugeWidget::GaugeZone> sharpeZones = {
        {-1.0, 0.0, DarkGray, Red, "Poor"},                     // Red
        {0.0, 1.0, MediumGray, Black, "Average"},          // Light gray
        {1.0, 4.0, LightGray, Green, "Good"}         // Very light gray
    };
    QString sharpeExplanation =
        "The <b>Sharpe ratio</b> measures risk-adjusted return. "
        "It indicates how many units of excess return you get per unit of volatility. "
        "A higher ratio indicates better risk-adjusted performance.";
    m_sharpeGauge = new RatioGaugeWidget(sharpeZones, "Sharpe Ratio", sharpeExplanation);


    QVector<RatioGaugeWidget::GaugeZone> sortinoZones = {
        {-1.0, 0.0, QColor(217, 83, 79), QColor(0, 0, 0), "Poor"},                          // Red
        { 0.0, 0.75, QColor(240, 173, 78), QColor(0, 0, 0), "Low"},                        // Orange
        { 0.75, 1.5, QColor(240, 240, 80), QColor(0, 0, 0), "Average"},                        // Yellow
        { 1.5, 2.5, QColor(150, 200, 80), QColor(0, 0, 0), "Good"},                          // Yellow-green
        { 2.5, 3.5, QColor(92, 184, 92), QColor(0, 0, 0), "Very good"},                      // Green
        { 3.5, 4.0, QColor(32, 150, 80), QColor(0, 0, 0), "Excellent"}                     // Dark green
    };
    QString sortinoExplanation = 
        "The <b>Sortino ratio</b> is similar to the Sharpe ratio but penalizes only downside volatility. "
        "It measures excess return per unit of downside risk, which is often more relevant for traders. "
        "A higher ratio indicates better downside risk management.";
    m_sortinoGauge = new RatioGaugeWidget(sortinoZones, "Sortino Ratio", sortinoExplanation);


    QVector<RatioGaugeWidget::GaugeZone> calmarZones = {
        {-1.0, 0.0, QColor(217, 83, 79), QColor(0, 0, 0), "Poor"},                          // Red
        { 0.0, 0.5, QColor(240, 173, 78), QColor(0, 0, 0), "Low"},                         // Orange
        { 0.5, 1.0, QColor(240, 240, 80), QColor(0, 0, 0), "Average"},                         // Yellow
        { 1.0, 2.0, QColor(150, 200, 80), QColor(0, 0, 0), "Good"},                           // Yellow-green
        { 2.0, 3.0, QColor(92, 184, 92), QColor(0, 0, 0), "Very good"},                       // Green
        { 3.0, 4.0, QColor(32, 150, 80), QColor(0, 0, 0), "Excellent"}                     // Dark green
    };
    QString calmarExplanation =
        "The <b>Calmar ratio</b> measures annualized return relative to maximum drawdown. "
        "It indicates the return obtained per unit of drawdown risk. "
        "A Calmar ratio above 1 means the annualized return exceeds the maximum drawdown.";
    m_calmarGauge = new RatioGaugeWidget(calmarZones, "Calmar Ratio", calmarExplanation);


    // QVector<RatioGaugeWidget::GaugeZone> winRateZones = {
    //     {0.0, 0.3, QColor(217, 83, 79), QColor(0, 0, 0), "Very low"},                       // Red
    //     {0.3, 0.4, QColor(240, 173, 78), QColor(0, 0, 0), "Low"},                          // Orange
    //     {0.4, 0.5, QColor(240, 240, 80), QColor(0, 0, 0), "Average"},                          // Yellow
    //     {0.5, 0.6, QColor(150, 200, 80), QColor(0, 0, 0), "Good"},                            // Yellow-green
    //     {0.6, 0.7, QColor(92, 184, 92), QColor(0, 0, 0), "Very good"},                        // Green
    //     {0.7, 1.0, QColor(32, 150, 80), QColor(0, 0, 0), "Excellent"}                       // Dark green
    // };
    QVector<RatioGaugeWidget::GaugeZone> winRateZones = {
        {0.0, 20, DarkGray, Red, "Not profitable"},                     // Red
        {20, 50, MediumGray, Black, "Average profitability"},          // Light gray
        {50, 100, LightGray, Green, "Good profitability"}         // Very light gray
    };

    QString winRateExplanation =
        "The <b>win rate</b> represents the percentage of winning trades. "
        "Although important, it should be evaluated together with the profit/loss ratio, "
        "because a strategy with a low win rate can be profitable if gains are large compared to losses.";
    m_winRateGauge = new RatioGaugeWidget(winRateZones, "Win Rate", winRateExplanation);
    m_winRateGauge->setValueFormat(1, true); // Display as percentage with 1 decimal


    // QVector<RatioGaugeWidget::GaugeZone> profitFactorZones = {
    //     {0.0, 1.0, QColor(217, 83, 79), QColor(0, 0, 0), "Not profitable"},                     // Red
    //     {1.0, 1.25, QColor(240, 173, 78), QColor(0, 0, 0), "Marginal profitability"},          // Orange
    //     {1.25, 1.5, QColor(240, 240, 80), QColor(0, 0, 0), "Acceptable profitability"},        // Yellow
    //     {1.5, 2.0, QColor(150, 200, 80), QColor(0, 0, 0), "Good profitability"},              // Yellow-green
    //     {2.0, 3.0, QColor(92, 184, 92), QColor(0, 0, 0), "Very good profitability"},         // Green
    //     {3.0, 4.0, QColor(32, 150, 80), QColor(0, 0, 0), "Excellent profitability"}         // Dark green
    // };
    QVector<RatioGaugeWidget::GaugeZone> profitFactorZones = {
        {0.0, 1.0, DarkGray, Red, "Not profitable"},                // Dark gray
        {1.0, 1.5, MediumGray, Black, "Average profitability"},      // Light gray
        {1.5, 4.0, LightGray, Green, "Good profitability"}         // Very light gray
    };
    QString profitFactorExplanation = 
        "The <b>profit factor</b> is the ratio between gross profits and gross losses. "
        "A profit factor greater than 1 indicates a profitable strategy. "
        "The higher this ratio, the more robust the strategy is to market fluctuations.";
    m_profitFactorGauge = new RatioGaugeWidget(profitFactorZones, "Profit Factor", profitFactorExplanation);
    // m_profitFactorGauge->setReferenceValue(1.0); // Reference line at 1.0


    QVector<RatioGaugeWidget::GaugeZone> kellyZones = {
        {-1.0, 0.0, QColor(217, 83, 79), QColor(0, 0, 0), "Not viable"},                       // Red
        { 0.0, 0.05, QColor(240, 173, 78), QColor(0, 0, 0), "Minimal size"},                // Orange
        { 0.05, 0.15, QColor(240, 240, 80), QColor(0, 0, 0), "Conservative size"},            // Yellow
        { 0.15, 0.25, QColor(150, 200, 80), QColor(0, 0, 0), "Optimal size"},                // Yellow-green
        { 0.25, 0.4, QColor(92, 184, 92), QColor(0, 0, 0), "Aggressive size"},               // Green
        { 0.4, 1.0, QColor(32, 150, 80), QColor(0, 0, 0), "Very aggressive"}                  // Dark green
    };
    QString kellyExplanation =
        "The <b>Kelly criterion</b> determines the optimal position size to maximize long-term capital growth. "
        "In practice, many traders use a fraction of Kelly (25-50%) to reduce volatility. "
        "A negative criterion indicates the strategy should not be traded.";
    m_kellyGauge = new RatioGaugeWidget(kellyZones, "Kelly Criterion", kellyExplanation);


    // QVector<RatioGaugeWidget::GaugeZone> drawdownZones = {
    //     {100.0, 50.0, QColor(217, 83, 79), QColor(0, 0, 0), "Critical"},                      // Red
    //     {50.0, 30.0, QColor(240, 173, 78), QColor(0, 0, 0), "Severe"},                       // Orange
    //     {30.0, 20.0, QColor(240, 240, 80), QColor(0, 0, 0), "Significant"},                    // Yellow
    //     {20.0, 10.0, QColor(150, 200, 80), QColor(0, 0, 0), "Moderate"},                       // Yellow-green
    //     {10.0, 5.0, QColor(92, 184, 92), QColor(0, 0, 0), "Low"},                        // Green
    //     {5.0, 0.0, QColor(32, 150, 80), QColor(0, 0, 0), "Very low"}                   // Dark green
    // };
    QVector<RatioGaugeWidget::GaugeZone> drawdownZones = {
        {100.0, 40.0, DarkGray, Red, "Critical"},                // Dark gray
        {40.0, 20.0, MediumGray, Black, "Significant"},      // Light gray
        {20.0, 0.0, LightGray, Green, "Low"}         // Very light gray
    };
    QString drawdownExplanation = 
        "The <b>maximum drawdown</b> measures the largest loss suffered between a peak and a trough of equity. "
        "It's a key risk indicator that shows the worst loss a trader could have experienced. "
        "A lower drawdown is preferable and indicates better risk management.";
    m_maxDrawdownGauge = new RatioGaugeWidget(drawdownZones, "Max Drawdown", drawdownExplanation);
    m_maxDrawdownGauge->setValueFormat(1, true); // Display as percentage with 1 decimal
    m_maxDrawdownGauge->setFillDirection(FillDirection::RightToLeft);


    QVector<RatioGaugeWidget::GaugeZone> sqnZones = {
        {-10.0, 1.6, QColor(217, 83, 79), QColor(0, 0, 0), "Poor"},                        // Red
        { 1.6, 2.0, QColor(240, 173, 78), QColor(0, 0, 0), "Average"},                          // Orange
        { 2.0, 2.5, QColor(240, 240, 80), QColor(0, 0, 0), "Good"},                           // Yellow
        { 2.5, 3.0, QColor(150, 200, 80), QColor(0, 0, 0), "Very good"},                      // Yellow-green
        { 3.0, 5.0, QColor(92, 184, 92), QColor(0, 0, 0), "Excellent"},                      // Green
        { 5.0, 10.0, QColor(32, 150, 80), QColor(0, 0, 0), "Extraordinary"}                 // Dark green
    };
    QString sqnExplanation =
        "The <b>System Quality Number (SQN)</b> measures the overall quality of a trading system. "
        "It accounts for average return per trade, the standard deviation of returns, and the number of trades. "
        "A higher SQN indicates a more robust and reliable system.";
    m_sqnGauge = new RatioGaugeWidget(sqnZones, "SQN", sqnExplanation);

    // Add the gauges to the grid layout
    gridLayout->addWidget(m_profitFactorGauge, 0, 0);
    gridLayout->addWidget(m_winRateGauge, 1, 0);
    gridLayout->addWidget(m_maxDrawdownGauge, 2, 0);
    gridLayout->addWidget(m_exposureTimeGauge, 3, 0);
    // gridLayout->addWidget(m_sharpeGauge, 4, 0);
    // gridLayout->addWidget(m_sortinoGauge, 0, 1);
    // gridLayout->addWidget(m_calmarGauge, 1, 1);
    // gridLayout->addWidget(m_sqnGauge, 2, 1);
    // gridLayout->addWidget(m_kellyGauge, 3, 1);

    // Set sizes
    // setMinimumHeight(250);
}

void RatioGaugesContainerWidget::updateContent(const be::Stats& stats)
{
    // Update each gauge with statistics values
    m_exposureTimeGauge->setValue(stats.exposureTimePct);
    m_sharpeGauge->setValue(stats.sharpeRatio);
    m_sortinoGauge->setValue(stats.sortinoRatio);
    m_calmarGauge->setValue(stats.calmarRatio);
    m_winRateGauge->setValue(stats.pctTPTrades); // Convert between 0-1
    m_profitFactorGauge->setValue(stats.profitFactor);
    m_sqnGauge->setValue(stats.sqn);
    m_maxDrawdownGauge->setValue(std::abs(stats.maxDrawdownPct));
    m_kellyGauge->setValue(stats.kellyCriterion * 0.01); // Convert between 0-1
}

void RatioGaugesContainerWidget::clear()
{
    // Reset all gauges to zero
    m_exposureTimeGauge->clear();
    m_sharpeGauge->clear();
    m_sortinoGauge->clear();
    m_calmarGauge->clear();
    m_winRateGauge->clear();
    m_profitFactorGauge->clear();
    m_sqnGauge->clear();
    m_maxDrawdownGauge->clear();
    m_kellyGauge->clear();
}