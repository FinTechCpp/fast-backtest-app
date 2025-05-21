#ifndef TRACKFINANCE_H
#define TRACKFINANCE_H

#include <QObject>
#include "qmlchartviewer.h"


class TrackFinance : public QObject {
    Q_OBJECT
public:
    TrackFinance(QObject *parent = nullptr);
    ~TrackFinance();

    Q_INVOKABLE void drawChart(QmlChartViewer *viewer);                     // Draw chart
    Q_INVOKABLE void drawTrackCursor(QmlChartViewer *viewer, int mouseX);   // Draw Track Cursor

private:
    BaseChart *m_currentChart;
    Q_INVOKABLE void trackFinance(MultiChart *c, int mouseX);
};


#endif // TRACKFINANCE_H
