#include "simplezoomscroll.h"
#include <chartdir.h>
#include <math.h>


SimpleZoomScroll::SimpleZoomScroll(QObject *parent) : QObject(parent)
{
    m_currentChart = 0;

    // Load the data
    loadData();
}

SimpleZoomScroll::~SimpleZoomScroll()
{
    delete m_ranSeries;
    delete m_currentChart;
}

//
// Load the data
//
void SimpleZoomScroll::loadData()
{
    // In this example, we just use random numbers as data.
    m_ranSeries = new RanSeries(127);
    m_timeStamps = m_ranSeries->getDateSeries(1827, Chart::chartTime(2007, 1, 1), 86400);
    m_dataSeriesA = m_ranSeries->getSeries(1827, 150, -10, 10);
    m_dataSeriesB = m_ranSeries->getSeries(1827, 200, -10, 10);
    m_dataSeriesC = m_ranSeries->getSeries(1827, 250, -8, 8);
}

//
// Initialize the QChartViewer
//
void SimpleZoomScroll::initChartViewer(QmlChartViewer *viewer)
{
    // Set the full x range to be the duration of the data
    viewer->setFullRange("x", m_timeStamps[0], m_timeStamps[m_timeStamps.len - 1]);

    // Initialize the view port to show the latest 20% of the time range
    viewer->setViewPortWidth(0.2);
    viewer->setViewPortLeft(1 - viewer->getViewPortWidth());

    // Set the maximum zoom to 10 points
    viewer->setZoomInWidthLimit(10.0 / m_timeStamps.len);
}

//
// Draw the chart and display it in the given viewer
//
void SimpleZoomScroll::drawChart(QmlChartViewer *viewer)
{
    // Get the start date and end date that are visible on the chart.
    double viewPortStartDate = viewer->getValueAtViewPort("x", viewer->getViewPortLeft());
    double viewPortEndDate = viewer->getValueAtViewPort("x", viewer->getViewPortLeft() +
        viewer->getViewPortWidth());

    // Get the array indexes that corresponds to the visible start and end dates
    int startIndex = (int)floor(Chart::bSearch(m_timeStamps, viewPortStartDate));
    int endIndex = (int)ceil(Chart::bSearch(m_timeStamps, viewPortEndDate));
    int noOfPoints = endIndex - startIndex + 1;

    // Extract the part of the data array that are visible.
    DoubleArray viewPortTimeStamps = DoubleArray(m_timeStamps.data + startIndex, noOfPoints);
    DoubleArray viewPortDataSeriesA = DoubleArray(m_dataSeriesA.data + startIndex, noOfPoints);
    DoubleArray viewPortDataSeriesB = DoubleArray(m_dataSeriesB.data + startIndex, noOfPoints);
    DoubleArray viewPortDataSeriesC = DoubleArray(m_dataSeriesC.data + startIndex, noOfPoints);

    //
    // At this stage, we have extracted the visible data. We can use those data to plot the chart.
    //

    ///////////////////////////////////////////////////////////////////////////////////////
    // Configure overall chart appearance.
    ///////////////////////////////////////////////////////////////////////////////////////

    // Create an XYChart object 600 x 300 pixels in size, with pale blue (0xf0f0ff) background,
    // black (000000) rounded border, 1 pixel raised effect.
    XYChart *c = new XYChart(600, 300, 0xf0f0ff, 0x000000, 1);
    c->setRoundedFrame(Chart::Transparent);

    // Set the plotarea at (52, 60) and of size 520 x 205 pixels. Use white (ffffff) background.
    // Enable both horizontal and vertical grids by setting their colors to grey (cccccc). Set
    // clipping mode to clip the data lines to the plot area.
    c->setPlotArea(52, 60, 520, 205, 0xffffff, -1, -1, 0xcccccc, 0xcccccc);

    // As the data can lie outside the plotarea in a zoomed chart, we need to enable clipping.
    c->setClipping();

    // Add a top title to the chart using 15 pts Times New Roman Bold Italic font, with a light blue
    // (ccccff) background, black (000000) border, and a glass like raised effect.
    c->addTitle("Simple Zooming and Scrolling", "Times New Roman Bold Italic", 15
        )->setBackground(0xccccff, 0x0, Chart::glassEffect());

    // Add a legend box at the top of the plot area with 9pts Arial Bold font with flow layout.
    c->addLegend(50, 33, false, "Arial Bold", 9)->setBackground(Chart::Transparent, Chart::Transparent);

    // Set axes width to 2 pixels
    c->yAxis()->setWidth(2);
    c->xAxis()->setWidth(2);

    // Add a title to the y-axis
    c->yAxis()->setTitle("Price (USD)", "Arial Bold", 9);

    ///////////////////////////////////////////////////////////////////////////////////////
    // Add data to chart
    ///////////////////////////////////////////////////////////////////////////////////////

    //
    // In this example, we represent the data by lines. You may modify the code below to use other
    // representations (areas, scatter plot, etc).
    //

    // Add a line layer for the lines, using a line width of 2 pixels
    LineLayer *layer = c->addLineLayer();
    layer->setLineWidth(2);

    // In this demo, we do not have too many data points. In real code, the chart may contain a lot
    // of data points when fully zoomed out - much more than the number of horizontal pixels in this
    // plot area. So it is a good idea to use fast line mode.
    layer->setFastLineMode();

    // Now we add the 3 data series to a line layer, using the color red (ff0000), green
    // (00cc00) and blue (0000ff)
    layer->setXData(viewPortTimeStamps);
    layer->addDataSet(viewPortDataSeriesA, 0xff0000, "Alpha");
    layer->addDataSet(viewPortDataSeriesB, 0x00cc00, "Beta");
    layer->addDataSet(viewPortDataSeriesC, 0x0000ff, "Gamma");

    ///////////////////////////////////////////////////////////////////////////////////////
    // Configure axis scale and labelling
    ///////////////////////////////////////////////////////////////////////////////////////

    // Set the x-axis as a date/time axis with the scale according to the view port x range.
    viewer->syncDateAxisWithViewPort("x", c->xAxis());

    // In this demo, we rely on ChartDirector to auto-label the axis. We ask ChartDirector to ensure
    // the x-axis labels are at least 75 pixels apart to avoid too many labels.
    c->xAxis()->setTickDensity(75);

    ///////////////////////////////////////////////////////////////////////////////////////
    // Output the chart
    ///////////////////////////////////////////////////////////////////////////////////////

    delete viewer->getChart();
    viewer->setChart(m_currentChart = c);
}

//
// Update the image map
//
void SimpleZoomScroll::updateImageMap(QmlChartViewer *viewer)
{
    // Include tool tip for the chart
    if (0 == viewer->getImageMapHandler())
    {
        viewer->setImageMap(viewer->getChart()->getHTMLImageMap("", "",
            "title='[{dataSetName}]\n{x|mmm dd, yyyy}: USD {value|2}'"));
    }
}



