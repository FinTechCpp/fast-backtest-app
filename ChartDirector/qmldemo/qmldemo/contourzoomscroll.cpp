#include "contourzoomscroll.h"
#include <math.h>


//
// Utlity to convert std::vector to a DoubleArray
//
static DoubleArray vectorToArray(std::vector<double> &v)
{
    return (v.size() == 0) ? DoubleArray() : DoubleArray(&(v[0]), v.size());
}


ContourZoomScroll::ContourZoomScroll(QObject *parent) : QObject(parent)
{
    //
    // Initialize member variables
    //
    m_currentChart = 0;
    m_currentVpcChart = 0;
    m_firstChart = 0;
    m_firstColorAxis = 0;

    // Load the data
    loadData();
}

ContourZoomScroll::~ContourZoomScroll()
{
    // Delete the old chart in m_ChartViewer unless it is the first chart
    if (m_currentChart != m_firstChart)
        delete m_currentChart;

    // Delete the first chart
    delete m_firstChart;

    // Delete the chart in the m_ViewPortControl
    delete m_currentVpcChart;
}

//
// Load the data
//
void ContourZoomScroll::loadData()
{
    //
    // For simplicity, in this demo, we just use hard coded data.
    //
    // The x and y coordinates of the grid
    double x[] = {-10, -9, -8, -7, -6, -5, -4, -3, -2, -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    double y[] = {-10, -9, -8, -7, -6, -5, -4, -3, -2, -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    dataX.assign(x, x + sizeof(x) / sizeof(*x));
    dataY.assign(y, y + sizeof(y) / sizeof(*y));

    // The values at the grid points. In this example, we will compute the values using the
    // formula z = x * sin(y) + y * sin(x).
    dataZ.resize(dataX.size() * dataY.size());
    for (int yIndex = 0; yIndex < (int)dataY.size(); ++yIndex)
    {
        double y = dataY[yIndex];
        for (int xIndex = 0; xIndex < (int)dataX.size(); ++xIndex)
        {
            double x = dataX[xIndex];
            dataZ[yIndex * dataX.size() + xIndex] = x * sin(y) + y * sin(x);
        }
    }
}

void ContourZoomScroll::initChartViewer(QmlChartViewer *viewer, QmlViewPortControl *vpCtrl)
{
    // Trigger the ViewPortChanged event to draw the chart
    viewer->updateViewPort(true, true);

    // Draw and display the full chart in the CViewPortControl
    drawFullChart(vpCtrl);
}

//
// Draw the chart and display it in the given viewer
//
void ContourZoomScroll::drawChart(QmlChartViewer *viewer)
{
    // Create an XYChart object 500 x 480 pixels in size, with the same background color
    // as the container
    XYChart *c = new XYChart(540, 490);

    // Set the plotarea at (75, 40) and of size 400 x 400 pixels. Use semi-transparent black
    // (80000000) dotted lines for both horizontal and vertical grid lines
    c->setPlotArea(50, 40, 400, 400, -1, -1, -1, c->dashLineColor(0x80000000, Chart::DotLine), -1);

    // Enable clipping mode to clip the part of the data that is outside the plot area.
    c->setClipping();

    // Add a title to the chart using 15 points Arial Bold Italic font
    c->addTitle("z = x * sin(y) + y * sin(x)      ", "Arial Bold Italic", 15);

    // Set x-axis and y-axis title using 12 points Arial Bold Italic font
    c->xAxis()->setTitle("X-Axis Title Place Holder", "Arial Bold Italic", 12);
    c->yAxis()->setTitle("Y-Axis Title Place Holder", "Arial Bold Italic", 12);

    // Set x-axis and y-axis labels to use Arial Bold font
    c->xAxis()->setLabelStyle("Arial Bold");
    c->yAxis()->setLabelStyle("Arial Bold");

    // When auto-scaling, use tick spacing of 40 pixels as a guideline
    c->yAxis()->setTickDensity(40);
    c->xAxis()->setTickDensity(40);

    // Add a contour layer using the given data
    ContourLayer* layer = c->addContourLayer(vectorToArray(dataX), vectorToArray(dataY),
        vectorToArray(dataZ));

    // Move the grid lines in front of the contour layer
    c->getPlotArea()->moveGridBefore(layer);

    // Add a color axis (the legend) in which the top left corner is anchored at (505, 40). Set the
    // length to 400 pixels and the labels on the right side.
    ColorAxis* cAxis = layer->setColorAxis(470, 40, Chart::TopLeft, 400, Chart::Right);

    // Add a title to the color axis using 12 points Arial Bold Italic font
    cAxis->setTitle("Color Legend Title Place Holder", "Arial Bold Italic", 12)
        ->setAlignment(Chart::Right);

    // Set color axis labels to use Arial Bold font
    cAxis->setLabelStyle("Arial Bold");

    // In this example, we have not explicitly configured the full x and y range. In this case,
    // the first time syncLinearAxisWithViewPort is called, ChartDirector will auto-scale the axis
    // and assume the resulting range is the full range. In subsequent calls, ChartDirector will
    // set the axis range based on the view port and the full range.
    viewer->syncLinearAxisWithViewPort("x", c->xAxis());
    viewer->syncLinearAxisWithViewPort("y", c->yAxis());

    // To ensure the color is consistent, we must keep the color axis the same scale when the
    // user zooms or scorlls the chart. To do this, we save the color axis of the first chart.
    // and synchronize all future charts with this color axis.
    if (0 == m_firstChart)
    {
        // We need to save the first chart as well as the first color axis. Otherwise the first
        // chart may get deleted, and this will delete the first color axis as well.
        m_firstChart = c;
        m_firstColorAxis = cAxis;
    }
    else
        cAxis->syncAxis(m_firstColorAxis);

    // Delete the old chart unless it is the first chart
    if (viewer->getChart() != m_firstChart)
        delete viewer->getChart();

    // Display chart
    viewer->setChart(m_currentChart = c);

    // Tooltip for the contour chart
    viewer->setImageMap(c->getHTMLImageMap("", "", "title='<*cdml*>X={x|2}\nY={y|2}\nZ={z|2}'"));
}

//
// Draw the full thumbnail chart and display it in the given CViewPortControl
//
void ContourZoomScroll::drawFullChart(QmlViewPortControl *vpc)
{
    // Create an XYChart object 110 x 110 pixels in size
    XYChart* c = new XYChart(110, 110);

    // Set the plotarea to cover the entire chart. Disable grid lines by setting their colors
    // to transparent.
    c->setPlotArea(0, 0, c->getWidth() - 1, c->getHeight() - 1, -1, -1, -1, Chart::Transparent,
        Chart::Transparent);

    // Add a contour layer using the given data
    ContourLayer* layer = c->addContourLayer(vectorToArray(dataX), vectorToArray(dataY),
        vectorToArray(dataZ));
    layer->setContourColor(0xcf000000);

    if (m_firstColorAxis)
        layer->colorAxis()->syncAxis(m_firstColorAxis);

    // The x and y axis scales reflect the full range of the view port
    QmlChartViewer *viewer = vpc->getViewer();
    if (viewer)
    {
        c->xAxis()->setLinearScale(viewer->getValueAtViewPort("x", 0), viewer->getValueAtViewPort("x", 1),
            Chart::NoValue);
        c->yAxis()->setLinearScale(viewer->getValueAtViewPort("y", 0), viewer->getValueAtViewPort("y", 1),
            Chart::NoValue);
    }

    // Set the chart image to the WinChartViewer
    delete vpc->getChart();
    vpc->setChart(m_currentVpcChart = c);
}

//
// The Save button is pressed
//
void ContourZoomScroll::saveChartImage(QmlChartViewer *viewer, QUrl url)
{
    QString path = url.toLocalFile();
    if (!path.isEmpty())
    {
        // Save the chart
        BaseChart *c = viewer->getChart();
        if (0 != c)
            c->makeChart(path.toUtf8().constData());
    }
}
