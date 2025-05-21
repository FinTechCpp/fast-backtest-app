#include "threedchartrotation.h"
#include <math.h>


ThreeDChartRotation::ThreeDChartRotation(QObject*parent) : QObject(parent)
{
    m_currentChart = 0;

    // 3D view angles
    m_elevationAngle = 30;
    m_rotationAngle = 45;

    // Draw frame only
    m_drawFrameOnly = false;
}

ThreeDChartRotation::~ThreeDChartRotation()
{
    delete m_currentChart;
}

//
// Draw chart
//
void ThreeDChartRotation::drawChart(QmlChartViewer *viewer)
{
    // The x and y coordinates of the grid
    double dataX[] = { -10, -9, -8, -7, -6, -5, -4, -3, -2, -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
    const int dataXLength = (int)(sizeof(dataX) / sizeof(*dataX));

    double dataY[] = { -10, -9, -8, -7, -6, -5, -4, -3, -2, -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
    const int dataYLength = (int)(sizeof(dataY) / sizeof(*dataY));

    // The values at the grid points. In this example, we will compute the values using the
    // formula z = x * sin(y) + y * sin(x).
    double dataZ[dataXLength * dataYLength];
    for (int yIndex = 0; yIndex < dataYLength; ++yIndex)
    {
        double y = dataY[yIndex];
        for (int xIndex = 0; xIndex < dataXLength; ++xIndex)
        {
            double x = dataX[xIndex];
            dataZ[yIndex * dataXLength + xIndex] = x * sin(y) + y * sin(x);
        }
    }

    // Create a SurfaceChart object of size 720 x 600 pixels
    SurfaceChart *c = new SurfaceChart(720, 600);

    // Set the center of the plot region at (330, 290), and set width x depth x height to
    // 360 x 360 x 270 pixels
    c->setPlotRegion(330, 290, 360, 360, 270);

    // Set the data to use to plot the chart
    c->setData(DoubleArray(dataX, dataXLength), DoubleArray(dataY, dataYLength),
        DoubleArray(dataZ, dataXLength * dataYLength));

    // Spline interpolate data to a 80 x 80 grid for a smooth surface
    c->setInterpolation(80, 80);

    // Set the view angles
    c->setViewAngle(m_elevationAngle, m_rotationAngle);

    // Check if draw frame only during rotation
    if (m_drawFrameOnly)
        c->setShadingMode(Chart::RectangularFrame);

    // Add a color axis (the legend) in which the left center is anchored at (660, 270). Set
    // the length to 200 pixels and the labels on the right side.
    c->setColorAxis(650, 270, Chart::Left, 200, Chart::Right);

    // Set the x, y and z axis titles using 10 points Arial Bold font
    c->xAxis()->setTitle("X", "Arial Bold", 15);
    c->yAxis()->setTitle("Y", "Arial Bold", 15);

    // Set axis label font
    c->xAxis()->setLabelStyle("Arial", 10);
    c->yAxis()->setLabelStyle("Arial", 10);
    c->zAxis()->setLabelStyle("Arial", 10);
    c->colorAxis()->setLabelStyle("Arial", 10);

    // Output the chart
    delete viewer->getChart();
    viewer->setChart(m_currentChart = c);

    //include tool tip for the chart
    viewer->setImageMap(c->getHTMLImageMap("", "",
        "title='<*cdml*>X: {x|2}<*br*>Y: {y|2}<*br*>Z: {z|2}'"));
}


