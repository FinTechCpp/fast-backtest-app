#ifndef REALTIMECHARTDEMO_H
#define REALTIMECHARTDEMO_H

#include <QObject>
#include <QImage>
#include <QQuickImageProvider>
#include <QQuickPaintedItem>
#include <QPainter>
#include <QDateTime>
#include "randomwalk.h"
#include "qdoublebufferedqueue.h"
#include "qmlchartviewer.h"


class RealTimeMultiThreadChart : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool plotAlpha MEMBER plotAlpha)
    Q_PROPERTY(bool plotBeta MEMBER plotBeta)
    Q_PROPERTY(bool plotGamma MEMBER plotGamma)

private:
    // Current chart
    BaseChart *currentChart;

    // The number of samples per data series used in this demo
    static const int sampleSize = 10000;

    // The initial full range is set to 60 seconds of data.
    static const int initialFullRange = 60;

    // The maximum zoom in is 5 seconds.
    static const int zoomInLimit = 5;

    // If the track cursor is at the end of the data series, we will automatic move the track
    // line when new data arrives.
    double trackLineEndPos;
    bool trackLineIsAtEnd;

    double m_timeStamps[sampleSize];	// The timestamps for the data series
    double m_dataSeriesA[sampleSize];	// The values for the data series A
    double m_dataSeriesB[sampleSize];	// The values for the data series B
    double m_dataSeriesC[sampleSize];   // The values for the data series C
    int m_currentIndex;                 // Index of the array position to which new values are added.

    QDateTime m_nextDataTime;           // Used by the random number generator to generate realtime data.

public:

    bool plotAlpha = true;
    bool plotBeta = true;
    bool plotGamma = true;

    void setPlotAlpha(bool b)
    {
        plotAlpha = b;
    }
    void setPlotBeta(bool b)
    {
        plotBeta = b;
    }
    void setPlotGamma(bool b)
    {
        plotGamma = b;
    }

    Q_INVOKABLE void onDataTimer();     // Read real time data
    Q_INVOKABLE void onChartUpdateTimer(QmlChartViewer *viewer);    // Update chart

    Q_INVOKABLE virtual void drawChart(QmlChartViewer *viewer);
    Q_INVOKABLE virtual void drawTrackCursor(QmlChartViewer *viewer, int mouseX);

    Q_INVOKABLE virtual void drawFullChart(QmlViewPortControl *viewer);

    virtual double trackLineLabel(XYChart *c, int mouseX);

    RealTimeMultiThreadChart(QObject *parent = nullptr);
    virtual ~RealTimeMultiThreadChart() override;
};

#endif // REALTIMECHARTDEMO_H
