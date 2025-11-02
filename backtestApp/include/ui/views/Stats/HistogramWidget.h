#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QStackedWidget>
#include <QHBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QBarSeries>
#include <QtCharts/QStackedBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QValueAxis>
#include <QDateTime>
#include <QGraphicsLineItem>
#include <QGraphicsTextItem>
#include <QMouseEvent>
#include <vector>
#include <map>
#include "ui/views/Stats/TitledWidget.h"
#include "components/backtestResults.h"

// Custom class for QChartView with interactive tooltip
class InteractiveChartView : public QChartView
{
    Q_OBJECT

public:
    InteractiveChartView(QChart* chart, QWidget* parent = nullptr);
    
    void setTooltipData(const QStringList& categories, const QList<double>& values, 
                       const QMap<QString, QDateTime>& fullDates);

signals:
    void barClicked(int barIndex);

protected:
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    void showCrosshair(const QPointF& position);
    void hideCrosshair();
    void updateTooltip(const QPointF& position);
    QString createTooltipText(const QString& category, double value);
    void setupCrosshairElements();
    
    // Data for the tooltip
    QStringList m_categories;
    QList<double> m_values;
    QMap<QString, QDateTime> m_fullDates;
    
    // Visual elements of the crosshair
    QGraphicsLineItem* m_horizontalLine;
    QGraphicsLineItem* m_verticalLine;
    QGraphicsTextItem* m_tooltipItem;
    
    bool m_crosshairVisible;
};

/**
 * @brief Widget to display the histogram of profits and losses
 */
class HistogramWidget : public TitledWidget
{
    Q_OBJECT

public:
    explicit HistogramWidget(const QString& title = "PnL Histogram", QWidget* parent = nullptr);
    ~HistogramWidget();

    // Update with backtest results
    void updateData(BacktestResults* results);
    void clear();

signals:
    void periodClicked(const QDateTime& startDate, const QDateTime& endDate);

protected:
    void paintContent(QPainter& painter, const QRect& contentRect) override;
    
private slots:  
    void updateHistogram();

private:
    // Interface widgets
    QStackedWidget* m_stackWidget = nullptr;
    QWidget* m_contentWidget;
    QLabel* m_placeholderLabel = nullptr;
    InteractiveChartView* m_chartView;
    QComboBox* m_timeUnitCombo;
    QChart* m_chart;
    
    // Current data
    BacktestResults* m_currentResults;
    
    // Structure to represent a trade
    struct TradeInfo {
        QDateTime exitTime;
        double pnl;
    };
    
    // Structure to store grouped data
    struct GroupedData {
        QStringList categories;
        QList<double> values;
        QMap<QString, QDateTime> fullDates;
        QMap<QString, QPair<QDateTime, QDateTime>> periodRanges; // Start and end of each period
    };
    
    GroupedData m_currentGroupedData; // Store the current grouped data
    
    // Utility methods
    std::vector<TradeInfo> extractTradesFromResults(BacktestResults* results);
    GroupedData groupDataByTimeUnit(const std::vector<TradeInfo>& trades, const QString& timeUnit);
    void createChart(const GroupedData& data);
    QDateTime getRepresentativeDate(const QDateTime& dateTime, const QString& timeUnit);
    QString generatePeriodKey(const QDateTime& dateTime, const QString& timeUnit);
    QPair<QDateTime, QDateTime> getPeriodRange(const QDateTime& representativeDate, const QString& timeUnit);
    void onBarClicked(int barIndex);
};