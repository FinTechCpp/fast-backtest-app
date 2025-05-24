#ifndef REALTIMEZOOMSCROLL_H
#define REALTIMEZOOMSCROLL_H

#include <QObject>
#include <QDateTime>
#include "qmlchartviewer.h"


class RealTimeZoomScroll: public QObject {
    Q_OBJECT
public:
    RealTimeZoomScroll(QObject *parent = nullptr);
    ~RealTimeZoomScroll();

    // Latest data values accessible to QML to provide some feedback to user
    Q_PROPERTY(double ValueA MEMBER m_ValueA)
    Q_PROPERTY(double ValueB MEMBER m_ValueB)
    Q_PROPERTY(double ValueC MEMBER m_ValueC)

    // Callable from QML to get realtime data values, to update the chart due
    // to new data, to redraw chart due to viewport update, and to draw the track
    // cursor and save the chart image.
    Q_INVOKABLE void getData();
    Q_INVOKABLE void updateChart(QmlChartViewer *viewer);       // Get new data values
    Q_INVOKABLE void drawChart(QmlChartViewer *viewer);         // Draw the chart.
    Q_INVOKABLE void drawTrackCursor(QmlChartViewer *viewer, int mouseX);
    Q_INVOKABLE void saveChartImage(QmlChartViewer *viewer, QUrl url);

private:
    BaseChart *m_currentChart;                      // Chart in QmlChartViewer
    void trackLineLabel(XYChart *c, int mouseX);    // Code to actually draw the track line

    // The number of samples per data series used in this demo
    static const int sampleSize = 10000;

    // The initial full range is set to 60 seconds of data.
    static const int initialFullRange = 60;

    // The maximum zoom in is 10 seconds.
    static const int zoomInLimit = 10;

    double m_timeStamps[sampleSize];	// The timestamps for the data series
    double m_dataSeriesA[sampleSize];	// The values for the data series A
    double m_dataSeriesB[sampleSize];	// The values for the data series B
    double m_dataSeriesC[sampleSize];	// The values for the data series C
    int m_currentIndex;                 // Index of the array position to which new values are added.

    QDateTime m_nextDataTime;           // Used by the random number generator to generate realtime data.

    // Latest data values
    double m_ValueA;
    double m_ValueB;
    double m_ValueC;
};


#endif // REALTIMEZOOMSCROLL_H
