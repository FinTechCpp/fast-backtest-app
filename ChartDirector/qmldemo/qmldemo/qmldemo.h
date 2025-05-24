#ifndef QMLDEMO_H
#define QMLDEMO_H

#include <QObject>
#include <QVariant>
#include "qmlchartviewer.h"
#include <set>

class QmlDemo : public QObject {
    Q_OBJECT
public:
    QmlDemo();
    ~QmlDemo();

    // Convert the demo catalog to a form that can be used as a model in QML ListView.
    Q_INVOKABLE QVariant getDemoCatalog();

    // Draw the specified chart in the specified demo and put it in the QmlChartViewer
    Q_INVOKABLE void drawChart(QmlChartViewer *viewer, int demoIndex, int chartIndex);

private:
    // To avoid memory leak, we keep track of the charts created but not yet deleted,
    // so we can delete them when program ends.
    std::set<BaseChart *> toBeDeleted;
};

#endif // QMLDEMO_H
