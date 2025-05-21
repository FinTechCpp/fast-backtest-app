#ifndef CROSSHAIR_H
#define CROSSHAIR_H

#include <QObject>
#include "qmlchartviewer.h"


class CrossHair : public QObject {
    Q_OBJECT
public:
    CrossHair(QObject *parent = nullptr);
    ~CrossHair();

    // Callable from QML to draw the chart and the track cursor
    Q_INVOKABLE void drawChart(QmlChartViewer *viewer);
    Q_INVOKABLE void drawTrackCursor(QmlChartViewer *viewer,
                                    int mouseX, int mouseY);
private:
    BaseChart *m_currentChart;
    void crosshair(XYChart *c, int mouseX, int mouseY);
};


#endif // CROSSHAIR_H
