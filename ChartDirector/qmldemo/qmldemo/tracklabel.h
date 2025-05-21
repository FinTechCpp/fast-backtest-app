#ifndef TRACKLABEL_H
#define TRACKLABEL_H

#include <QObject>
#include "qmlchartviewer.h"


class TrackLabel : public QObject {
    Q_OBJECT
public:
    TrackLabel(QObject *parent = nullptr);
    ~TrackLabel();

    Q_INVOKABLE void drawChart(QmlChartViewer *viewer);                     // Draw chart
    Q_INVOKABLE void drawTrackCursor(QmlChartViewer *viewer, int mouseX);   // Draw Track Cursor

private:
    BaseChart *m_currentChart;
    Q_INVOKABLE void trackLineLabel(XYChart *c, int mouseX);
};


#endif // TRACKLABEL_H
