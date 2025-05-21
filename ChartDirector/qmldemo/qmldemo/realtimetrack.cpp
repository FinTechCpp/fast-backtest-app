#include "realtimetrack.h"
#include <sstream>
#include <math.h>


// 100ms per data point, chart contains 1 min of data = 600 data points.
static const int DataInterval = 100;
static const int sampleSize = 60 * 1000 / DataInterval;


RealTimeTrack::RealTimeTrack(QObject *parent) : QObject(parent)
{
    m_currentChart = 0;

    // Allocate memory for the data series and initialize to Chart::NoValue
    m_timeStamps.resize(sampleSize, Chart::NoValue);
    m_dataSeriesA.resize(sampleSize, Chart::NoValue);
    m_dataSeriesB.resize(sampleSize, Chart::NoValue);
    m_dataSeriesC.resize(sampleSize, Chart::NoValue);
    m_currentIndex = 0;

    // Set m_nextDataTime to the current time. It is used by the real time random number
    // generator so it knows what timestamp should be used for the next data point.
    m_nextDataTime = QDateTime::fromMSecsSinceEpoch(0);
}

RealTimeTrack::~RealTimeTrack()
{
    delete m_currentChart;
}

//
// A utility to shift a new data value into a data array
//
static void shiftData(double *data, int len, double newValue)
{
    memmove(data, data + 1, sizeof(*data) * (len - 1));
    data[len - 1] = newValue;
}

//
// Shift new data values into the real time data series
//
void RealTimeTrack::getData()
{
    // The current time
    QDateTime now = QDateTime::currentDateTime();

    // This is our formula for the random number generator
    do
    {
        // We need the currentTime in millisecond resolution
        qint64 t = m_nextDataTime.toMSecsSinceEpoch();
        if (t == 0) {
            // Initialize to "now" on first use
            m_nextDataTime = now;
            t = m_nextDataTime.toMSecsSinceEpoch();
        }

        double currentTime = Chart::chartTime2((int)(t / 1000)) + (t % 1000) / DataInterval
            * DataInterval / 1000.0;

        // Get a data sample
        double p = currentTime * 4;
        double dataA = 20 + cos(p * 129241) * 10 + 1 / (cos(p) * cos(p) + 0.01);
        double dataB = 150 + 100 * sin(p / 27.7) * sin(p / 10.1);
        double dataC = 150 + 100 * cos(p / 6.7) * cos(p / 11.9);

        // After obtaining the new values, we need to update the data arrays.
        if (m_currentIndex < sampleSize)
        {
            // Store the new values in the current index position, and increment the index.
            m_dataSeriesA[m_currentIndex] = dataA;
            m_dataSeriesB[m_currentIndex] = dataB;
            m_dataSeriesC[m_currentIndex] = dataC;
            m_timeStamps[m_currentIndex] = currentTime;
            ++m_currentIndex;
        }
        else
        {
            // The data arrays are full. Shift the arrays and store the values at the end.
            shiftData(&m_dataSeriesA[0], sampleSize, dataA);
            shiftData(&m_dataSeriesB[0], sampleSize, dataB);
            shiftData(&m_dataSeriesC[0], sampleSize, dataC);
            shiftData(&m_timeStamps[0], sampleSize, currentTime);
        }

        m_nextDataTime = m_nextDataTime.addMSecs(DataInterval);
    }
    while (m_nextDataTime < now);

    //
    // We provide some visual feedback to the latest numbers generated, so you can see the
    // data being generated.
    //
    m_ValueA = m_dataSeriesA[m_currentIndex - 1];
    m_ValueB = m_dataSeriesB[m_currentIndex - 1];
    m_ValueC = m_dataSeriesC[m_currentIndex - 1];
}


//
// Draw chart
//
void RealTimeTrack::drawChart(QmlChartViewer *viewer)
{
    // Create an XYChart object 600 x 270 pixels in size, with light grey (f4f4f4)
    // background, black (000000) border, 1 pixel raised effect, and with a rounded frame.
    XYChart *c = new XYChart(600, 270, 0xf4f4f4, 0x000000, 1);
    c->setRoundedFrame(Chart::Transparent);

    // Set the plotarea at (55, 55) and of size 520 x 185 pixels. Use white (ffffff)
    // background. Enable both horizontal and vertical grids by setting their colors to
    // grey (cccccc). Set clipping mode to clip the data lines to the plot area.
    c->setPlotArea(55, 55, 520, 185, 0xffffff, -1, -1, 0xcccccc, 0xcccccc);
    c->setClipping();

    // Add a title to the chart using 15 pts Times New Roman Bold Italic font, with a light
    // grey (dddddd) background, black (000000) border, and a glass like raised effect.
    c->addTitle("Field Intensity at Observation Satellite", "Times New Roman Bold Italic",
        15)->setBackground(0xdddddd, 0x000000, Chart::glassEffect());

    // Set the reference font size of the legend box
    c->getLegend()->setFontSize(8);

    // Configure the y-axis with a 10pts Arial Bold axis title
    c->yAxis()->setTitle("Intensity (V/m)", "Arial Bold", 10);

    // Configure the x-axis to auto-scale with at least 75 pixels between major tick and
    // 15  pixels between minor ticks. This shows more minor grid lines on the chart.
    c->xAxis()->setTickDensity(75, 15);

    // Set the axes width to 2 pixels
    c->xAxis()->setWidth(2);
    c->yAxis()->setWidth(2);

    // Now we add the data to the chart.
    double firstTime  = m_timeStamps[0];
    if (firstTime != Chart::NoValue)
    {
        // Set up the x-axis to show the time range in the data buffer
        c->xAxis()->setDateScale(firstTime, firstTime + DataInterval * sampleSize / 1000.0);

        // Set the x-axis label format
        c->xAxis()->setLabelFormat("{value|hh:nn:ss}");

        // Create a line layer to plot the lines
        LineLayer *layer = c->addLineLayer();

        // The x-coordinates are the timeStamps.
        layer->setXData(DoubleArray(&m_timeStamps[0], sampleSize));

        // The 3 data series are used to draw 3 lines.
        layer->addDataSet(DoubleArray(&m_dataSeriesA[0], sampleSize), 0xff0000, "Alpha");
        layer->addDataSet(DoubleArray(&m_dataSeriesB[0], sampleSize), 0x00cc00, "Beta");
        layer->addDataSet(DoubleArray(&m_dataSeriesC[0], sampleSize), 0x0000ff, "Gamma");
    }

    // Include track line with legend. If the mouse is on the plot area, show the track
    // line with legend at the mouse position; otherwise, show them for the latest data
    // values (that is, at the rightmost position).
    trackLineLegend(c, viewer->isMouseOnPlotArea() ? viewer->getPlotAreaMouseX() :
        c->getPlotArea()->getRightX());

    // Set the chart image to the WinChartViewer
    delete viewer->getChart();
    viewer->setChart(m_currentChart = c);
}

//
// Draw track cursor when mouse is moving over plotarea
//
void RealTimeTrack::drawTrackCursor(QmlChartViewer *viewer, int mouseX)
{
    trackLineLegend((XYChart *)viewer->getChart(), mouseX);
    viewer->updateDisplay();
}

//
// Draw the track line with legend
//
void RealTimeTrack::trackLineLegend(XYChart *c, int mouseX)
{
    // Clear the current dynamic layer and get the DrawArea object to draw on it.
    DrawArea *d = c->initDynamicLayer();

    // The plot area object
    PlotArea *plotArea = c->getPlotArea();

    // Get the data x-value that is nearest to the mouse, and find its pixel coordinate.
    double xValue = c->getNearestXValue(mouseX);
    int xCoor = c->getXCoor(xValue);

    // Draw a vertical track line at the x-position
    d->vline(plotArea->getTopY(), plotArea->getBottomY(), xCoor, d->dashLineColor(0x000000, 0x0101));

    // Container to hold the legend entries
    std::vector<std::string> legendEntries;

    // Iterate through all layers to build the legend array
    for (int i = 0; i < c->getLayerCount(); ++i) {
        Layer *layer = c->getLayerByZ(i);

        // The data array index of the x-value
        int xIndex = layer->getXIndexOf(xValue);

        // Iterate through all the data sets in the layer
        for (int j = 0; j < layer->getDataSetCount(); ++j) {
            DataSet *dataSet = layer->getDataSetByZ(j);

            // We are only interested in visible data sets with names
            const char *dataName = dataSet->getDataName();
            int color = dataSet->getDataColor();
            if (dataName && *dataName && (color != (int)Chart::Transparent)) {
                // Build the legend entry, consist of the legend icon, name and data value.
                double dataValue = dataSet->getValue(xIndex);
                std::ostringstream legendEntry;
                legendEntry << "<*block*>" << dataSet->getLegendIcon() << " " << dataName << ": " <<
                    ((dataValue == Chart::NoValue) ? "N/A" : c->formatValue(dataValue, "{value|P4}"))
                    << "<*/*>";
                legendEntries.push_back(legendEntry.str());

                // Draw a track dot for data points within the plot area
                int yCoor = c->getYCoor(dataSet->getPosition(xIndex), dataSet->getUseYAxis());
                if ((yCoor >= plotArea->getTopY()) && (yCoor <= plotArea->getBottomY())) {
                    d->circle(xCoor, yCoor, 4, 4, color, color);
                }
            }
        }
    }

    // Create the legend by joining the legend entries
    std::ostringstream legendText;
    legendText << "<*block,maxWidth=" << plotArea->getWidth() << "*><*block*><*font=Arial Bold*>["
        << c->xAxis()->getFormattedLabel(xValue, "hh:nn:ss") << "]<*/*>";
    for (int i = ((int)legendEntries.size()) - 1; i >= 0; --i)
        legendText << "        " << legendEntries[i];

    // Display the legend on the top of the plot area
    TTFText *t = d->text(legendText.str().c_str(), "Arial", 8);
    t->draw(plotArea->getLeftX() + 5, plotArea->getTopY() - 3, 0x000000, Chart::BottomLeft);
    t->destroy();
}
