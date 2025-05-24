#ifndef THREEDCHARTROTATION_H
#define THREEDCHARTROTATION_H

#include <QObject>
#include "qmlchartviewer.h"


class ThreeDChartRotation : public QObject {
    Q_OBJECT
public:
    ThreeDChartRotation(QObject *parent = nullptr);
    ~ThreeDChartRotation();

    // Properties accessible from QML to change the view angles and the draw frame mode
    Q_PROPERTY(double elevationAngle MEMBER m_elevationAngle)
    Q_PROPERTY(double rotationAngle MEMBER m_rotationAngle)
    Q_PROPERTY(double drawFrameOnly MEMBER m_drawFrameOnly)

    // Callable from QML to draw the chart
    Q_INVOKABLE void drawChart(QmlChartViewer *viewer);

private:
    BaseChart *m_currentChart;

    // 3D view angles
    double m_elevationAngle;
    double m_rotationAngle;

    // Draw frame only
    bool m_drawFrameOnly;
};


#endif // THREEDCHARTROTATION_H
