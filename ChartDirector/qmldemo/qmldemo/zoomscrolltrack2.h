#ifndef ZOOMSCROLLTRACK2_H
#define ZOOMSCROLLTRACK2_H

#include <QObject>
#include "qmlchartviewer.h"


class ZoomScrollTrack2 : public QObject {
    Q_OBJECT
    // QML accessible flags to disable/enable plotting of data series
    Q_PROPERTY(bool plotAlpha MEMBER m_plotAlpha)
    Q_PROPERTY(bool plotBeta MEMBER m_plotBeta)
    Q_PROPERTY(bool plotGamma MEMBER m_plotGamma)
public:
    ZoomScrollTrack2(QObject *parent = nullptr);
    ~ZoomScrollTrack2();

    // Callable from QML to initialize the QmlChartViewer, draw the chart and track cursor
    Q_INVOKABLE void initChartViewer(QmlChartViewer *viewer);
    Q_INVOKABLE void drawChart(QmlChartViewer *viewer);
    Q_INVOKABLE void drawTrackCursor(QmlChartViewer *viewer, int mouseX);

private:
    BaseChart *m_currentChart;                     // The current chart
    void trackLineLabel(XYChart *c, int mouseX);   // Draw Track Cursor

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

    // Flags to determine which series to plot
    bool m_plotAlpha = true;
    bool m_plotBeta = true;
    bool m_plotGamma = true;
};


#endif // ZOOMSCROLLTRACK2_H
