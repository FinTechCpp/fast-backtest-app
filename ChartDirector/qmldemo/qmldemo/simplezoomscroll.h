#ifndef SIMPLEZOOMSCROLL_H
#define SIMPLEZOOMSCROLL_H

#include <QObject>
#include "qmlchartviewer.h"


class SimpleZoomScroll : public QObject {
    Q_OBJECT
public:
    SimpleZoomScroll(QObject *parent = nullptr);
    ~SimpleZoomScroll();

    Q_INVOKABLE void initChartViewer(QmlChartViewer *viewer);   // Initialize the viewport
    Q_INVOKABLE void drawChart(QmlChartViewer *viewer);         // Draw chart
    Q_INVOKABLE void updateImageMap(QmlChartViewer *viewer);    // Update image map

private:
    // The current chart
    BaseChart *m_currentChart;

    //
    // Data arrays for the scrollable / zoomable chart.
    // - In this demo, we just use a RanTable object to generate random data for the chart.
    //
    RanSeries *m_ranSeries;
    DoubleArray m_timeStamps;
    DoubleArray m_dataSeriesA;
    DoubleArray m_dataSeriesB;
    DoubleArray m_dataSeriesC;

    // Load data into data arrays
    void loadData();
};


#endif // SIMPLEZOOMSCROLL_H
