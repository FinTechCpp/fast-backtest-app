#include "xyzoomscroll.h"
#include <sstream>

//
// Utlity to convert std::vector to a DoubleArray
//
static DoubleArray vectorToArray(std::vector<double> &v)
{
    return (v.size() == 0) ? DoubleArray() : DoubleArray(&(v[0]), v.size());
}


XYZoomScroll::XYZoomScroll(QObject *parent) : QObject(parent)
{
    m_currentChart = 0;
    m_currentVpcChart = 0;

    // Load the data
    loadData();
}

XYZoomScroll::~XYZoomScroll()
{
    delete m_currentChart;
    delete m_currentVpcChart;
}

//
// Load the data
//
void XYZoomScroll::loadData()
{
    //
    // For simplicity, in this demo, we just use hard coded data.
    //
    double X0[] = {10, 15, 6, -12, 14, -8, 13, -3, 16, 12, 10.5, -7, 3, -10, -5, 2, 5};
    double Y0[] = {130, 150, 80, 110, -110, -105, -130, -15, -170, 125,  125, 60, 25, 150,
        150, 15, 120};
    double X1[] = {6, 7, -4, 3.5, 7, 8, -9, -10, -12, 11, 8, -3, -2, 8, 4, -15, 15};
    double Y1[] = {65, -40, -40, 45, -70, -80, 80, 10, -100, 105, 60, 50, 20, 170, -25,
        50, 75};
    double X2[] = {-10, -12, 11, 8, 6, 12, -4, 3.5, 7, 8, -9, 3, -13, 16, -7.5, -10, -15};
    double Y2[] = {65, -80, -40, 45, -70, -80, 80, 90, -100, 105, 60, -75, -150, -40, 120,
        -50, -30};

    dataX0.assign(X0, X0 + sizeof(X0) / sizeof(*X0));
    dataY0.assign(Y0, Y0 + sizeof(Y0) / sizeof(*Y0));
    dataX1.assign(X1, X1 + sizeof(X1) / sizeof(*X1));
    dataY1.assign(Y1, Y1 + sizeof(Y1) / sizeof(*Y1));
    dataX2.assign(X2, X2 + sizeof(X2) / sizeof(*X2));
    dataY2.assign(Y2, Y2 + sizeof(Y2) / sizeof(*Y2));
}

void XYZoomScroll::initChartViewer(QmlChartViewer *viewer, QmlViewPortControl *vpCtrl)
{
    // Trigger the ViewPortChanged event to draw the chart
    viewer->updateViewPort(true, true);

    // Draw and display the full chart in the CViewPortControl
    drawFullChart(vpCtrl);
}

//
// Draw the chart and display it in the given viewer
//
void XYZoomScroll::drawChart(QmlChartViewer *viewer)
{
    // Create an XYChart object 500 x 480 pixels in size, with the same background color
    // as the container
    XYChart *c = new XYChart(500, 480, Chart::Transparent);

    // Set the plotarea at (50, 40) and of size 400 x 400 pixels. Use light grey (c0c0c0)
    // horizontal and vertical grid lines. Set 4 quadrant coloring, where the colors of
    // the quadrants alternate between lighter and deeper grey (dddddd/eeeeee).
    c->setPlotArea(50, 40, 400, 400, -1, -1, -1, 0xc0c0c0, 0xc0c0c0
        )->set4QBgColor(0xdddddd, 0xeeeeee, 0xdddddd, 0xeeeeee, 0x000000);

    // Enable clipping mode to clip the part of the data that is outside the plot area.
    c->setClipping();

    // Set 4 quadrant mode, with both x and y axes symetrical around the origin
    c->setAxisAtOrigin(Chart::XYAxisAtOrigin, Chart::XAxisSymmetric + Chart::YAxisSymmetric);

    // Add a legend box at (450, 40) (top right corner of the chart) with vertical layout
    // and 8 pts Arial Bold font. Set the background color to semi-transparent grey.
    LegendBox *legendBox = c->addLegend(450, 40, true, "Arial Bold", 8);
    legendBox->setAlignment(Chart::TopRight);
    legendBox->setBackground(0x40dddddd);

    // Add a titles to axes
    c->xAxis()->setTitle("Alpha Index");
    c->yAxis()->setTitle("Beta Index");

    // Set axes width to 2 pixels
    c->xAxis()->setWidth(2);
    c->yAxis()->setWidth(2);

    // The default ChartDirector settings has a denser y-axis grid spacing and less-dense
    // x-axis grid spacing. In this demo, we want the tick spacing to be symmetrical.
    // We use around 50 pixels between major ticks and 25 pixels between minor ticks.
    c->xAxis()->setTickDensity(50, 25);
    c->yAxis()->setTickDensity(50, 25);

    //
    // In this example, we represent the data by scatter points. If you want to represent
    // the data by somethings else (lines, bars, areas, floating boxes, etc), just modify
    // the code below to use the layer type of your choice.
    //

    // Add scatter layer, using 11 pixels red (ff33333) X shape symbols
    c->addScatterLayer(vectorToArray(dataX0), vectorToArray(dataY0), "Group A",
        Chart::Cross2Shape(), 11, 0xff3333);

    // Add scatter layer, using 11 pixels green (33ff33) circle symbols
    c->addScatterLayer(vectorToArray(dataX1), vectorToArray(dataY1), "Group B",
        Chart::CircleShape, 11,  0x33ff33);

    // Add scatter layer, using 11 pixels blue (3333ff) triangle symbols
    c->addScatterLayer(vectorToArray(dataX2), vectorToArray(dataY2), "Group C",
        Chart::TriangleSymbol, 11, 0x3333ff);

    //
    // In this example, we have not explicitly configured the full x and y range. In this case,
    // the first time syncLinearAxisWithViewPort is called, ChartDirector will auto-scale the axis
    // and assume the resulting range is the full range. In subsequent calls, ChartDirector will
    // set the axis range based on the view port and the full range.
    //
    viewer->syncLinearAxisWithViewPort("x", c->xAxis());
    viewer->syncLinearAxisWithViewPort("y", c->yAxis());

    // We need to update the track line too. If the mouse is moving on the chart (eg. if
    // the user drags the mouse on the chart to scroll it), the track line will be updated
    // in the MouseMovePlotArea event. Otherwise, we need to update the track line here.
    if ((!viewer->isInMouseMoveEvent()) && viewer->isMouseOnPlotArea())
        crossHair(c, viewer->getPlotAreaMouseX(), viewer->getPlotAreaMouseY());

    // Set the chart image to the QChartViewer
    delete viewer->getChart();
    viewer->setChart(m_currentChart = c);
}

//
// Draw the full thumbnail chart and display it in the given CViewPortControl
//
void XYZoomScroll::drawFullChart(QmlViewPortControl *vpc)
{
    // Create an XYChart object 110 x 110 pixels in size
    XYChart *c = new XYChart(110, 110);

    // Set the plotarea to cover the entire chart. Disable grid lines by setting their colors
    // to transparent. Set 4 quadrant coloring, where the colors of the quadrants alternate
    // between lighter and deeper grey (dddddd/eeeeee).
    c->setPlotArea(0, 0, c->getWidth() - 1, c->getHeight() - 1, -1, -1, -1, Chart::Transparent,
        Chart::Transparent)->set4QBgColor(0xdddddd, 0xeeeeee, 0xdddddd, 0xeeeeee, 0x000000);

    // Set 4 quadrant mode, with both x and y axes symetrical around the origin
    c->setAxisAtOrigin(Chart::XYAxisAtOrigin, Chart::XAxisSymmetric + Chart::YAxisSymmetric);

    // The x and y axis scales reflect the full range of the view port
    QmlChartViewer *viewer = vpc->getViewer();
    if (viewer)
    {
        c->xAxis()->setLinearScale(viewer->getValueAtViewPort("x", 0), viewer->getValueAtViewPort(
            "x", 1), Chart::NoValue);
        c->yAxis()->setLinearScale(viewer->getValueAtViewPort("y", 0), viewer->getValueAtViewPort(
            "y", 1), Chart::NoValue);
    }

    // Add scatter layer, using 11 pixels red (ff33333) X shape symbols
    c->addScatterLayer(vectorToArray(dataX0), vectorToArray(dataY0), "", Chart::Cross2Shape(),
        3, 0xff3333, 0xff3333);

    // Add scatter layer, using 11 pixels green (33ff33) circle symbols
    c->addScatterLayer(vectorToArray(dataX1), vectorToArray(dataY1), "", Chart::CircleShape,
        3, 0x33ff33, 0x33ff33);

    // Add scatter layer, using 11 pixels blue (3333ff) triangle symbols
    c->addScatterLayer(vectorToArray(dataX2), vectorToArray(dataY2), "", Chart::TriangleSymbol,
        3, 0x3333ff, 0x3333ff);

    // Set the chart image to the WinChartViewer
    vpc->setChart(m_currentVpcChart = c);
}

//
// Update the image map
//
void XYZoomScroll::updateImageMap(QmlChartViewer *viewer)
{
    if (0 == viewer->getImageMapHandler())
    {
        // no existing image map - creates a new one
        viewer->setImageMap(viewer->getChart()->getHTMLImageMap("", "",
            "title='[{dataSetName}] Alpha = {x}, Beta = {value}'"));
    }
}

//
// The Save button is pressed
//
void XYZoomScroll::saveChartImage(QmlChartViewer *viewer, QUrl url)
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

//
// Draw track cursor when mouse is moving over plotarea, and update image map if necessary
//
void XYZoomScroll::drawTrackCursor(QmlChartViewer *viewer, int mouseX, int mouseY)
{
    // Draw crosshair track cursor
    crossHair((XYChart *)viewer->getChart(), mouseX, mouseY);
    viewer->updateDisplay();

    // Hide the track cursor when the mouse leaves the plot area
    viewer->removeDynamicLayer("mouseLeavePlotArea");

    // Update image map if necessary. If the mouse is still dragging, the chart is still
    // updating and not confirmed, so there is no need to set up the image map.
    if (!viewer->isMouseDragging())
        updateImageMap(viewer);
}

//
// Draw cross hair cursor with axis labels
//
void XYZoomScroll::crossHair(XYChart *c, int mouseX, int mouseY)
{
    // Clear the current dynamic layer and get the DrawArea object to draw on it.
    DrawArea *d = c->initDynamicLayer();

    // The plot area object
    PlotArea *plotArea = c->getPlotArea();

    // Draw a vertical line and a horizontal line as the cross hair
    d->vline(plotArea->getTopY(), plotArea->getBottomY(), mouseX, d->dashLineColor(0x000000, 0x0101));
    d->hline(plotArea->getLeftX(), plotArea->getRightX(), mouseY, d->dashLineColor(0x000000, 0x0101));

    // Draw y-axis label
    std::ostringstream ylabel;
    ylabel << "<*block,bgColor=FFFFDD,margin=3,edgeColor=000000*>" << c->formatValue(c->getYValue(
        mouseY, c->yAxis()), "{value|P4}") << "<*/*>";
    TTFText *t = d->text(ylabel.str().c_str(), "Arial Bold", 8);
    t->draw(plotArea->getLeftX() - 5, mouseY, 0x000000, Chart::Right);
    t->destroy();

    // Draw x-axis label
    std::ostringstream xlabel;
    xlabel << "<*block,bgColor=FFFFDD,margin=3,edgeColor=000000*>" << c->formatValue(c->getXValue(
        mouseX), "{value|P4}") << "<*/*>";
    t = d->text(xlabel.str().c_str(), "Arial Bold", 8);
    t->draw(mouseX, plotArea->getBottomY() + 5, 0x000000, Chart::Top);
    t->destroy();
}


