#ifndef TRACKLEGEND_H
#define TRACKLEGEND_H

#include <QObject>
#include "qmlchartviewer.h"


class TrackLegend : public QObject {
    Q_OBJECT
public:
    TrackLegend(QObject *parent = nullptr);
    ~TrackLegend();

    Q_INVOKABLE void drawChart(QmlChartViewer *viewer);                     // Draw chart
    Q_INVOKABLE void drawTrackCursor(QmlChartViewer *viewer, int mouseX);   // Draw Track Cursor

private:
    BaseChart *m_currentChart;
    Q_INVOKABLE void trackLineLegend(XYChart *c, int mouseX);
};


#endif // TRACKLEGEND_H
