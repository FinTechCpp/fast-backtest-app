#ifndef ZOOMSCROLLTRACK_H
#define ZOOMSCROLLTRACK_H

#include <QObject>
#include "qmlchartviewer.h"


class ZoomScrollTrack : public QObject {
    Q_OBJECT
public:
    ZoomScrollTrack(QObject *parent = nullptr);
    ~ZoomScrollTrack();

    // Callable from QML to initialize the QmlChartViewer, draw the chart and track cursor
    Q_INVOKABLE void initChartViewer(QmlChartViewer *viewer);
    Q_INVOKABLE void drawChart(QmlChartViewer *viewer);
    Q_INVOKABLE void drawTrackCursor(QmlChartViewer *viewer, int mouseX);

private:
    BaseChart *m_currentChart;                      // The current chart
    void trackLineLegend(XYChart *c, int mouseX);   // Draw Track Cursor

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


#endif // ZOOMSCROLLTRACK_H
