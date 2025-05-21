#include "helloworld.h"


HelloWorld::HelloWorld(QObject *parent) : QObject(parent)
{
    m_currentChart = 0;
}

HelloWorld::~HelloWorld()
{
    delete m_currentChart;
}

void HelloWorld::drawChart(QmlChartViewer *viewer)
{
    // The data for the bar chart
    double data[] = {85, 156, 179.5, 211, 123};

    // The labels for the bar chart
    const char *labels[] = {"Mon", "Tue", "Wed", "Thu", "Fri"};

    // Create a XYChart object of size 250 x 250 pixels
    XYChart *c = new XYChart(250, 250);

    // Set the plotarea at (30, 20) and of size 200 x 200 pixels
    c->setPlotArea(30, 20, 200, 200);

    // Add a bar chart layer using the given data
    c->addBarLayer(DoubleArray(data, sizeof(data)/sizeof(data[0])));

    // Set the labels on the x axis.
    c->xAxis()->setLabels(StringArray(labels, sizeof(labels)/sizeof(labels[0])));

    // Output the chart. Save the chart in m_currentChart so it can be deleted later.
    viewer->setChart(m_currentChart = c);

    // Include tool tip for the chart
    viewer->setImageMap(c->getHTMLImageMap("", "", "title='{xLabel}: US${value}K'"));
}
