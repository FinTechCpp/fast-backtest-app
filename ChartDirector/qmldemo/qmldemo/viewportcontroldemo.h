#ifndef VIEWPORTCONTROLDEMO_H
#define VIEWPORTCONTROLDEMO_H

#include <QObject>
#include "qmlchartviewer.h"


class ViewPortControlDemo : public QObject {
    Q_OBJECT
public:
    ViewPortControlDemo(QObject *parent = nullptr);
    ~ViewPortControlDemo();

    // Callable from QML to initialize the QmlChartViewer, draw the chart and
    // the track cursor, and save the chart image
    Q_INVOKABLE void initChartViewer(QmlChartViewer *viewer, QmlViewPortControl *vpc);
    Q_INVOKABLE void drawChart(QmlChartViewer *viewer);
    Q_INVOKABLE void drawTrackCursor(QmlChartViewer *viewer, int mouseX);
    Q_INVOKABLE void saveChartImage(QmlChartViewer *viewer, QUrl url);

private:
    BaseChart *m_currentChart;                      // Chart in QmlChartViewer
    BaseChart *m_currentVpcChart;                   // Chart in QmlViewPortControl
    void drawFullChart(QmlViewPortControl *vpc);    // Draw full chart
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


#endif // VIEWPORTCONTROLDEMO_H
