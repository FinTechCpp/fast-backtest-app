#ifndef REALTIMEDEMO_H
#define REALTIMEDEMO_H

#include <QObject>
#include <QDateTime>
#include "qmlchartviewer.h"


class RealTimeDemo : public QObject {
    Q_OBJECT
public:
    RealTimeDemo(QObject *parent = nullptr);
    ~RealTimeDemo();

    // Latest data values accessible to QML to provide some feedback to user
    Q_PROPERTY(double ValueA MEMBER m_ValueA)
    Q_PROPERTY(double ValueB MEMBER m_ValueB)
    Q_PROPERTY(double ValueC MEMBER m_ValueC)

    // Callable from QML to get realtime data and to draw the charts.
    Q_INVOKABLE void getData();
    Q_INVOKABLE void drawChart(QmlChartViewer *viewer);

private:
    BaseChart *m_currentChart;          // Chart in QmlChartViewer

    std::vector<double> m_timeStamps;	// The timestamps for the data series
    std::vector<double> m_dataSeriesA;	// The values for the data series A
    std::vector<double> m_dataSeriesB;	// The values for the data series B
    std::vector<double> m_dataSeriesC;	// The values for the data series C

    QDateTime m_nextDataTime;   // Used by the random number generator to generate realtime data.

    // Latest data values
    double m_ValueA;
    double m_ValueB;
    double m_ValueC;
};


#endif // REALTIMEDEMO_H
