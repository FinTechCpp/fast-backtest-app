#ifndef REALTIMEDEMO_H
#define REALTIMEDEMO_H

#include <QObject>
#include "qmlchartviewer.h"


class HelloWorld : public QObject {
    Q_OBJECT
public:
    HelloWorld(QObject *parent = nullptr);
    ~HelloWorld();

    // Callable from QML to draw chart
    Q_INVOKABLE void drawChart(QmlChartViewer *viewer);

private:
    // Current chart in QmlChartViewer
    BaseChart *m_currentChart;
};


#endif // REALTIMEDEMO_H
