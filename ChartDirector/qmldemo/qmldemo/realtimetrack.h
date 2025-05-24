#ifndef REALTIMETRACK_H
#define REALTIMETRACK_H

#include <QObject>
#include <QDateTime>
#include "qmlchartviewer.h"


class RealTimeTrack: public QObject {
    Q_OBJECT
public:
    RealTimeTrack(QObject *parent = nullptr);
    ~RealTimeTrack();

    // Latest data values accessible to QML to provide some feedback to user
    Q_PROPERTY(double ValueA MEMBER m_ValueA)
    Q_PROPERTY(double ValueB MEMBER m_ValueB)
    Q_PROPERTY(double ValueC MEMBER m_ValueC)

    // Callable from QML to get new realtime data and to draw the chart and track cursor.
    Q_INVOKABLE void getData();
    Q_INVOKABLE void drawChart(QmlChartViewer *viewer);
    Q_INVOKABLE void drawTrackCursor(QmlChartViewer *viewer, int mouseX);

private:
    BaseChart *m_currentChart;                      // Chart in QmlChartViewer
    void trackLineLegend(XYChart *c, int mouseX);   // Code to draw the track cursor

    std::vector<double> m_timeStamps;	// The timestamps for the data series
    std::vector<double> m_dataSeriesA;	// The values for the data series A
    std::vector<double> m_dataSeriesB;	// The values for the data series B
    std::vector<double> m_dataSeriesC;	// The values for the data series C
    int m_currentIndex;                 // Index of the array position to which new values are added.

    QDateTime m_nextDataTime;           // Used by the random number generator to generate realtime data.

    // Latest data values
    double m_ValueA;
    double m_ValueB;
    double m_ValueC;
};


#endif // REALTIMETRACK_H
