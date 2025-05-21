#ifndef MEGAREALTIMEZOOMSCROLL_H
#define MEGAREALTIMEZOOMSCROLL_H

#include <QObject>
#include "qmlchartviewer.h"


class MegaRealTimeZoomScroll : public QObject {
    Q_OBJECT
public:
    MegaRealTimeZoomScroll(QObject *parent = nullptr);
    ~MegaRealTimeZoomScroll();

    Q_PROPERTY(int pointCount MEMBER m_currentIndex)

    // Callable from QML to start the plotting, draw and update the chart
    // and draw the track cursor
    Q_INVOKABLE void plotChart(QmlChartViewer *viewer);
    Q_INVOKABLE void drawChart(QmlChartViewer *viewer);
    Q_INVOKABLE void updateChart(QmlChartViewer *viewer);
    Q_INVOKABLE void drawTrackCursor(QmlChartViewer *viewer, int mouseX);

private:
    BaseChart *m_currentChart;                          // The current chart
    void initChartViewer(QmlChartViewer *viewer);       // Initialize the chart viewer
    void trackLineLabel(XYChart *c, int mouseX);        // Draw Track Cursor
    XYChart* drawXYChart(double startX, double endX);   // Draw chart in the given x range

    // Data arrays
    std::vector<double> m_dataSeriesA;
    std::vector<double> m_dataSeriesB;
    std::vector<double> m_dataSeriesC;
    std::vector<double> m_timeStamps;

    // The index position that new data will be added.
    int m_currentIndex;

    // Data Accelerator for handle large data sets
    DataAccelerator* m_fastData;

    // Flag to indicate the chart has been drawn so the zoom/scroll and track cursor
    // event handlers can run.
    bool m_hasFinishedInitialization;

    // Load data into data arrays
    void loadData();

    // Our real-time random number generator
    RanSeries m_realTimeData;
    QElapsedTimer m_lastUpdateTime;
    bool getRealTimeData();

    // Update the axis scale to accomodate new data
    void updateAxisScale(QmlChartViewer* viewer);
};


#endif // MEGAREALTIMEZOOMSCROLL_H
