#ifndef XYZOOMSCROLL_H
#define XYZOOMSCROLL_H

#include <QObject>
#include "qmlchartviewer.h"


class XYZoomScroll : public QObject {
    Q_OBJECT
public:
    XYZoomScroll(QObject *parent = nullptr);
    ~XYZoomScroll();

    // Callable from QML to initialize the QmlChartViewer, draw the chart and
    // update the image map, draw the track cursor, and save the chart image
    Q_INVOKABLE void initChartViewer(QmlChartViewer *viewer, QmlViewPortControl *vpc);  // Initialize the QChartViewer
    Q_INVOKABLE void drawChart(QmlChartViewer *viewer);                                 // Draw chart
    Q_INVOKABLE void updateImageMap(QmlChartViewer *viewer);
    Q_INVOKABLE void drawTrackCursor(QmlChartViewer *viewer, int mouseX, int mouseY);   // Draw Track Cursor
    Q_INVOKABLE void saveChartImage(QmlChartViewer *viewer, QUrl url);

private:
    BaseChart *m_currentChart;                      // Chart in QmlChartViewer
    BaseChart *m_currentVpcChart;                   // Chart in QmlViewPortControl
    void drawFullChart(QmlViewPortControl *vpc);    // Draw full chart
    void crossHair(XYChart *c, int mouseX, int mouseY);   // Draw Track Cursor

    // Data arrays for the scrollable / zoomable chart.
    std::vector<double> dataX0;
    std::vector<double> dataY0;
    std::vector<double> dataX1;
    std::vector<double> dataY1;
    std::vector<double> dataX2;
    std::vector<double> dataY2;

    // Load data into data arrays
    void loadData();
};


#endif // XYZOOMSCROLL_H
