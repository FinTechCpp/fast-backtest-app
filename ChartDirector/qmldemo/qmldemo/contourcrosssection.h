#ifndef CONTOURCROSSSECTION_H
#define CONTOURCROSSSECTION_H

#include <QObject>
#include "qmlchartviewer.h"


class ContourCrossSection : public QObject {
    Q_OBJECT
public:
    ContourCrossSection(QObject *parent = nullptr);
    ~ContourCrossSection();

    // Callable from QML to draw chart, cross section  and crosshair
    Q_INVOKABLE void drawChart(QmlChartViewer *viewer);
    Q_INVOKABLE void drawCrossHair(QmlChartViewer *viewer, int x, int y);
    Q_INVOKABLE void drawCrossSectionX(QmlChartViewer *viewer, int x);
    Q_INVOKABLE void drawCrossSectionY(QmlChartViewer *viewer, int y);

private:
    // The chart objects and main contour in this demo
    XYChart *m_currentChart;
    XYChart *m_crossSectionX;
    XYChart *m_crossSectionY;
    ContourLayer* m_ContourLayer;

    // Code to actually draw the cross sections
    void drawCrossSectionX(QmlChartViewer *viewer, XYChart* mainChart, ContourLayer* layer, int x);
    void drawCrossSectionY(QmlChartViewer *viewer, XYChart* mainChart, ContourLayer* layer, int y);
};


#endif // CONTOURCROSSSECTION_H
