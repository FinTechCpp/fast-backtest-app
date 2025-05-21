#ifndef TRACKAXIS_H
#define TRACKAXIS_H

#include <QObject>
#include "qmlchartviewer.h"


class TrackAxis : public QObject {
    Q_OBJECT
public:
    TrackAxis(QObject *parent = nullptr);
    ~TrackAxis();

    Q_INVOKABLE void drawChart(QmlChartViewer *viewer);                     // Draw chart
    Q_INVOKABLE void drawTrackCursor(QmlChartViewer *viewer, int mouseX);   // Draw Track Cursor

private:
    BaseChart *m_currentChart;
    void trackLineAxis(XYChart *c, int mouseX);
};


#endif // TRACKAXIS_H

