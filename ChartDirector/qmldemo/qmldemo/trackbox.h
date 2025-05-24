#ifndef TRACKBOX_H
#define TRACKBOX_H

#include <QObject>
#include "qmlchartviewer.h"


class TrackBox : public QObject {
    Q_OBJECT
public:
    TrackBox(QObject *parent = nullptr);
    ~TrackBox();

    Q_INVOKABLE void drawChart(QmlChartViewer *viewer);                                 // Draw chart
    Q_INVOKABLE void drawTrackCursor(QmlChartViewer *viewer, int mouseX, int mouseY);   // Draw Track Cursor

private:
    BaseChart *m_currentChart;
    Q_INVOKABLE void trackBoxLegend(XYChart *c, int mouseX, int mouseY);
};


#endif // TRACKBOX_H
