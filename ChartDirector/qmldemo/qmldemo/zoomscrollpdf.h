#ifndef ZOOMSCROLLPDF_H
#define ZOOMSCROLLPDF_H

#include <QObject>
#include "qmlchartviewer.h"


class ZoomScrollPdf : public QObject {
    Q_OBJECT
public:
    ZoomScrollPdf(QObject *parent = nullptr);
    ~ZoomScrollPdf();

    // Callable from QML to initialize the QmlChartViewer, draw the chart and
    // track cursor, and save the chart image and create PDF report.
    Q_INVOKABLE void initChartViewer(QmlChartViewer *viewer);
    Q_INVOKABLE void drawChart(QmlChartViewer *viewer);                     // Draw chart
    Q_INVOKABLE void drawTrackCursor(QmlChartViewer *viewer, int mouseX);   // Draw Track Cursor
    Q_INVOKABLE void saveChartImage(QmlChartViewer *viewer, QUrl url);
    Q_INVOKABLE void createPdfReport(QUrl url);

private:
    BaseChart *m_currentChart;                          // The current chart
    void trackLineLabel(XYChart *c, int mouseX);        // Draw Track Cursor
    XYChart* drawXYChart(double startX, double endX);   // Draw chart in the given x range

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



#endif // ZOOMSCROLLPDF_H
