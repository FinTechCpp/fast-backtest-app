#ifndef FINANCEDEMO_H
#define FINANCEDEMO_H

#include <QObject>
#include "qmlchartviewer.h"
#include <vector>


class FinanceDemo : public QObject {
    Q_OBJECT
public:
    FinanceDemo(QObject *parent = nullptr);
    ~FinanceDemo();

    // Properties of the financial chart that can be set from QML
    Q_PROPERTY(QString tickerSymbol MEMBER m_tickerSymbol)
    Q_PROPERTY(QString compareWith MEMBER m_compareWith)
    Q_PROPERTY(int timeRange MEMBER m_timeRange)
    Q_PROPERTY(bool volumeBars MEMBER m_volumeBars)
    Q_PROPERTY(bool parabolicSAR MEMBER m_parabolicSAR)
    Q_PROPERTY(bool logScale MEMBER m_logScale)
    Q_PROPERTY(bool percentageScale MEMBER m_percentageScale)
    Q_PROPERTY(QString chartType MEMBER m_chartType)
    Q_PROPERTY(QString priceBand MEMBER m_priceBand)
    Q_PROPERTY(QString avgType1 MEMBER m_avgType1)
    Q_PROPERTY(QString avgType2 MEMBER m_avgType2)
    Q_PROPERTY(QString movAvg1 MEMBER m_movAvg1)
    Q_PROPERTY(QString movAvg2 MEMBER m_movAvg2)
    Q_PROPERTY(QString indicator1 MEMBER m_indicator1)
    Q_PROPERTY(QString indicator2 MEMBER m_indicator2)

    // Callable from QML to draw the chart and the track cursor
    Q_INVOKABLE void drawChart(QmlChartViewer *viewer);
    Q_INVOKABLE void drawTrackCursor(QmlChartViewer *viewer, int mouseX);

private:
    BaseChart *m_currentChart;

    // Properties of the financial chart
    QString m_tickerSymbol;
    QString m_compareWith;
    int m_timeRange = 90;
    bool m_volumeBars = true;
    bool m_parabolicSAR = false;
    bool m_logScale = false;
    bool m_percentageScale = false;
    QString m_chartType = "CandleStick";
    QString m_priceBand = "BB";
    QString m_avgType1 = "SMA";
    QString m_avgType2 = "SMA";
    QString m_movAvg1 = "10";
    QString m_movAvg2 = "20";
    QString m_indicator1 = "RSI";
    QString m_indicator2 = "MACD";

    // The current ticker key and the compare to key.
    QString m_currentTicker;
    QString m_currentCompare;
    int m_currentTimeRange;

    struct PriceData
    {
        std::vector<double> timeStamps;
        std::vector<double> highData;
        std::vector<double> lowData;
        std::vector<double> openData;
        std::vector<double> closeData;
        std::vector<double> volData;

        // An extra field for price comparison
        std::vector<double> compareData;
    };

    // In this example, we can use daily, weekly or monthly data depending on the time range
    PriceData m_dailyPrice;
    PriceData m_weeklyPrice;
    PriceData m_monthlyPrice;

    // Convert daily data to weekly, monthly and yearly
    void aggregateData(const ArrayMath& aggregator, PriceData& p);

    // Load data into the data arrays
    void loadData(QmlChartViewer *viewer, const QString& ticker, const QString& compare);

    // Draw track cursor
    void trackFinance(MultiChart* m, int mouseX);
};


#endif // FINANCEDEMO_H
