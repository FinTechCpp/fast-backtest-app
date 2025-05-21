#ifndef MEGAZOOMSCROLL_H
#define MEGAZOOMSCROLL_H

#include <QObject>
#include "qmlchartviewer.h"


class MegaZoomScroll : public QObject {
    Q_OBJECT
public:
    MegaZoomScroll(QObject *parent = nullptr);
    ~MegaZoomScroll();

    // Callable from QML to start the plotting, draw the chart and track cursor
    Q_INVOKABLE void plotChart(QmlChartViewer *viewer);
    Q_INVOKABLE void drawChart(QmlChartViewer *viewer);
    Q_INVOKABLE void drawTrackCursor(QmlChartViewer *viewer, int mouseX);

private:
    BaseChart *m_currentChart;                          // The current chart
    void initChartViewer(QmlChartViewer *viewer);       // Initialize the chart viewer
    XYChart* drawXYChart(double startX, double endX);   // Draw chart in the given x range
    void trackLineLabel(XYChart *c, int mouseX);        // Draw Track Cursor

    // Data arrays
    std::vector<double> m_dataSeriesA;
    std::vector<double> m_dataSeriesB;
    std::vector<double> m_dataSeriesC;
    std::vector<double> m_timeStamps;

    // Flag to indicate the chart has been drawn so the zoom/scroll and track cursor
    // event handlers can run.
    bool m_hasFinishedInitialization;

    // Load data into data arrays
    void loadData();

    // Data Accelerator for handle large data sets
    DataAccelerator* m_fastData;
};


#endif // MEGAZOOMSCROLL_H
