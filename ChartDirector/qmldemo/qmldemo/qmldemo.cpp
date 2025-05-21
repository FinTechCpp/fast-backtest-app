#include "qmldemo.h"
#include "democharts.h"

QmlDemo::QmlDemo()
{
}

QmlDemo::~QmlDemo()
{
    // Delete charts that has not yet been deleted.
    for(std::set<BaseChart *>::iterator i = toBeDeleted.begin(); i != toBeDeleted.end(); ++i)
        delete *i;
}

// Convert the demo catalog to a form that can be used as a model in QML ListView.
QVariant QmlDemo::getDemoCatalog()
{
    // A list of items
    QVariantList list;
    for (int i = 0; demoCharts[i].name; ++i)
    {
        DemoChart &d = demoCharts[i];
        QVariantMap map;
        map["name"] = d.name;
        map["chartCount"] = d.noOfCharts;
        if (d.noOfCharts == 0)
        {
            // The item is a folder
            map["isFolder"] = true;
            map["iconSource"] = d.path;
        }
        else
        {
            // The item is a demo. The demo can draw and display the chart(s) directly,
            // or load a QML file.
            if (d.createChart)
                map["demoChart"] = i;
            else
               map["demoQML"] = d.path;
        }

        list << map;
    }

    return list;
}

// Draw the specified chart in the specified demo and put it in the QmlChartViewer
void QmlDemo::drawChart(QmlChartViewer *viewer, int demoIndex, int chartIndex)
{
    // Delete the existing chart in the QmlChartViewer. We check against the
    // toBeDeleted set first to make sure the chart is created by this module.
    std::set<BaseChart *>::iterator i = toBeDeleted.find(viewer->getChart());
    if (i != toBeDeleted.end())
    {
        delete *i;
        toBeDeleted.erase(i);
        viewer->setChart(0);
    }

    demoCharts[demoIndex].createChart(viewer, chartIndex);

    // Add the chart to the toBeDeleted list so it can be deleted in the future.
    if (viewer->getChart())
        toBeDeleted.insert(viewer->getChart());
}
