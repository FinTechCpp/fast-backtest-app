#ifndef CONTOURZOOMSCROLL_H
#define CONTOURZOOMSCROLL_H

#include <QObject>
#include "qmlchartviewer.h"


class ContourZoomScroll : public QObject {
    Q_OBJECT
public:
    ContourZoomScroll(QObject *parent = nullptr);
    ~ContourZoomScroll();

    // Callable from QML to initialize the chart viewer, draw chart, and save chart image
    Q_INVOKABLE void initChartViewer(QmlChartViewer *viewer, QmlViewPortControl *vpc);
    Q_INVOKABLE void drawChart(QmlChartViewer *viewer);
    Q_INVOKABLE void saveChartImage(QmlChartViewer *viewer, QUrl url);

private:
    BaseChart *m_currentChart;      // Chart in QmlChartViewer
    BaseChart *m_currentVpcChart;   // Chart in QmlViewPortControl

    // To ensure color consistency, we need to save the first color axis and
    // synchronize the color axis of the zoom-in charts with the this axis.
    XYChart* m_firstChart;
    ColorAxis* m_firstColorAxis;

    void drawFullChart(QmlViewPortControl *vpc);        // Draw full chart
    void crossHair(XYChart *c, int mouseX, int mouseY); // Draw Track Cursor

    // Data arrays for the scrollable / zoomable chart.
    std::vector<double> dataX;
    std::vector<double> dataY;
    std::vector<double> dataZ;

    // Load data into data arrays
    void loadData();
};


#endif // CONTOURZOOMSCROLL_H
