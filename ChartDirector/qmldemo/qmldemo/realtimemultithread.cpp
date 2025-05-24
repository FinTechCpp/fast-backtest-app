#include <QApplication>
#include <QButtonGroup>
#include <QIcon>
#include <QPushButton>
#include <QComboBox>
#include <QFileDialog>
#include <QtPrintSupport/QPrinter>
#include <QtPrintSupport/QPrintDialog>
#include <QPainter>
#include "realtimemultithread.h"
#include "chartdir.h"
#include <math.h>
#include <vector>
#include <sstream>

using namespace std;


int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setStyleSheet("* {font-family:arial;font-size:11px}");
    RealTimeMultiThread demo;
    demo.show();
    return app.exec();
}


RealTimeMultiThread::RealTimeMultiThread(QWidget *parent) :
    QDialog(parent)
{
    //
    // Set up the GUI
    //

    setFixedSize(772, 450);
    setWindowTitle("Multithreading Real-Time Chart with Zoom/Scroll and Track Line");

    QFrame *frame = new QFrame(this);
    frame->setGeometry(4, 4, 120, 442);
    frame->setFrameShape(QFrame::StyledPanel);

    // Pointer push button
    QPushButton *pointerPB = new QPushButton(QIcon(":/pointer.png"), "Pointer", frame);
    pointerPB->setGeometry(4, 8, 112, 28);
    pointerPB->setStyleSheet("QPushButton { text-align:left; padding:5px}");
    pointerPB->setCheckable(true);

    // Zoom In push button
    QPushButton *zoomInPB = new QPushButton(QIcon(":/zoomin.png"), "Zoom In", frame);
    zoomInPB->setGeometry(4, 36, 112, 28);
    zoomInPB->setStyleSheet("QPushButton { text-align:left; padding:5px}");
    zoomInPB->setCheckable(true);

    // Zoom Out push button
    QPushButton *zoomOutPB = new QPushButton(QIcon(":/zoomout.png"), "Zoom Out", frame);
    zoomOutPB->setGeometry(4, 64, 112, 28);
    zoomOutPB->setStyleSheet("QPushButton { text-align:left; padding:5px}");
    zoomOutPB->setCheckable(true);

    // Save push button
    QPushButton *savePB = new QPushButton(QIcon(":/save.png"), "Save", frame);
    savePB->setStyleSheet("QPushButton { text-align:left; padding:5px}");
    savePB->setGeometry(4, 120, 112, 28);
    connect(savePB, SIGNAL(clicked(bool)), SLOT(onSave(bool)));

    // The Pointer/Zoom In/Zoom Out buttons form a button group
    QButtonGroup *mouseUsage = new QButtonGroup(frame);
    mouseUsage->addButton(pointerPB, Chart::MouseUsageScroll);
    mouseUsage->addButton(zoomInPB, Chart::MouseUsageZoomIn);
    mouseUsage->addButton(zoomOutPB, Chart::MouseUsageZoomOut);
    connect(mouseUsage, SIGNAL(buttonPressed(int)), SLOT(onMouseUsageChanged(int)));

    // Chart Viewer
    m_ChartViewer = new QChartViewer(this);
    m_ChartViewer->setGeometry(128, 4, 640, 420);
    connect(m_ChartViewer, SIGNAL(viewPortChanged()), SLOT(onViewPortChanged()));
    connect(m_ChartViewer, SIGNAL(mouseMovePlotArea(QMouseEvent*)),
        SLOT(onMouseMovePlotArea(QMouseEvent*)));

    // Horizontal scroll bar
    m_HScrollBar = new QScrollBar(Qt::Horizontal, this);
    m_HScrollBar->setGeometry(128, 428, 640, 17);
    connect(m_HScrollBar, SIGNAL(valueChanged(int)), SLOT(onHScrollBarChanged(int)));

    //
    // Initialize member variables
    //
    m_currentIndex = 0;

    // Initially, auto-move the track line to make it follow the data series
    trackLineEndPos = 0;
    trackLineIsAtEnd = true;

    // Initially set the mouse to drag to scroll mode.
    pointerPB->click();

    // Enable mouse wheel zooming by setting the zoom ratio to 1.1 per wheel event
    m_ChartViewer->setMouseWheelZoomRatio(1.1);

    // Start the random data generator
    dataSource = new RandomWalk(OnData, this);
    dataSource->start();

    // Set up the chart update timer
    m_ChartUpdateTimer = new QTimer(this);
    connect(m_ChartUpdateTimer, SIGNAL(timeout()), SLOT(onChartUpdateTimer()));

    // The chart update rate is set to 100ms
    m_ChartUpdateTimer->start(100);
}

RealTimeMultiThread::~RealTimeMultiThread()
{
    freeChart((MultiChart *)m_ChartViewer->getChart());
    delete dataSource;
}

void RealTimeMultiThread::freeChart(MultiChart *m)
{
    if (0 != m)
    {
        XYChart *c = 0;
        for (int i = 0; 0 != (c = (XYChart *)m->getChart(i)); ++i)
            delete c;
        delete m;
    }
}

//
// The Pointer, Zoom In or Zoom out button is pressed
//
void RealTimeMultiThread::onMouseUsageChanged(int mouseUsage)
{
    m_ChartViewer->setMouseUsage(mouseUsage);
}

//
// The Save button is pressed
//
void RealTimeMultiThread::onSave(bool)
{
    QString fileName = QFileDialog::getSaveFileName(this, "Save", "chartdirector_demo",
        "PNG (*.png);;JPG (*.jpg);;GIF (*.gif);;BMP (*.bmp);;SVG (*.svg);;PDF (*.pdf)");

    if (!fileName.isEmpty())
    {
        // Save the chart
        BaseChart *c = m_ChartViewer->getChart();
        if (0 != c)
            c->makeChart(fileName.toUtf8().constData());
    }
}

//
// Handles realtime data from RandomWalk. The RandomWalk will call this method from its own thread.
// This is a static method.
//
void RealTimeMultiThread::OnData(void *self, double elapsedTime, double series0, double series1)
{
    // Copy the data into a structure and store it in the queue.
    DataPacket packet;
    packet.elapsedTime = elapsedTime;
    packet.series0 = series0;
    packet.series1 = series1;
    packet.series2 = (fmod(elapsedTime + 0.5, 2) > 1) ? 9 : 1;
    packet.series3 = (elapsedTime - (int)elapsedTime) * 8 + 1;

    ((RealTimeMultiThread *)self)->buffer.put(packet);
}

//
// Get data from the queue, update the viewport and update the chart display if necessary.
//
void RealTimeMultiThread::onChartUpdateTimer()
{
    QChartViewer *viewer = m_ChartViewer;

    // Enables auto scroll if the viewport is showing the latest data before the update
    bool autoScroll = (m_currentIndex > 0) && (0.001 + viewer->getValueAtViewPort("x",
        viewer->getViewPortLeft() + viewer->getViewPortWidth()) >= m_timeStamps[m_currentIndex - 1]);

    //
    // Get new data from the queue and append them to the data arrays
    //
    int count;
    DataPacket *packets;
    if ((count = buffer.get(&packets)) <= 0)
        return;

    // if data arrays have insufficient space, we need to remove some old data.
    if (m_currentIndex + count >= sampleSize)
    {
        // For safety, we check if the queue contains too much data than the entire data arrays. If
        // this is the case, we only use the latest data to completely fill the data arrays.
        if (count > sampleSize)
        {
            packets += count - sampleSize;
            count = sampleSize;
        }

        // Remove oldest data to leave space for new data. To avoid frequent removal, we ensure at
        // least 5% empty space available after removal.
        int originalIndex = m_currentIndex;
        m_currentIndex = sampleSize * 95 / 100 - 1;
        if (m_currentIndex > sampleSize - count)
            m_currentIndex = sampleSize - count;

        for (int i = 0; i < m_currentIndex; ++i)
        {
            int srcIndex = i + originalIndex - m_currentIndex;
            m_timeStamps[i] = m_timeStamps[srcIndex];
            m_dataSeriesA[i] = m_dataSeriesA[srcIndex];
            m_dataSeriesB[i] = m_dataSeriesB[srcIndex];
            m_dataSeriesC[i] = m_dataSeriesC[srcIndex];
            m_dataSeriesD[i] = m_dataSeriesD[srcIndex];
        }
    }

    // Append the data from the queue to the data arrays
    for (int n = 0; n < count; ++n)
    {
        m_timeStamps[m_currentIndex] = packets[n].elapsedTime;
        m_dataSeriesA[m_currentIndex] = packets[n].series0;
        m_dataSeriesB[m_currentIndex] = packets[n].series1;
        m_dataSeriesC[m_currentIndex] = packets[n].series2;
        m_dataSeriesD[m_currentIndex] = packets[n].series3;
        ++m_currentIndex;
    }

    //
    // As we added more data, we may need to update the full range of the viewport.
    //

    double startDate = m_timeStamps[0];
    double endDate = m_timeStamps[m_currentIndex - 1];

    // Use the initialFullRange (which is 60 seconds in this demo) if this is sufficient.
    double duration = endDate - startDate;
    if (duration < initialFullRange)
        endDate = startDate + initialFullRange;

    // Update the new full data range to include the latest data
    bool axisScaleHasChanged = viewer->updateFullRangeH("x", startDate, endDate,
        Chart::KeepVisibleRange);

    if (autoScroll)
    {
        // Scroll the viewport if necessary to display the latest data
        double viewPortEndPos = viewer->getViewPortAtValue("x", m_timeStamps[m_currentIndex - 1]);
        if (viewPortEndPos > viewer->getViewPortLeft() + viewer->getViewPortWidth())
        {
            viewer->setViewPortLeft(viewPortEndPos - viewer->getViewPortWidth());
            axisScaleHasChanged = true;
        }
    }

    // Set the zoom in limit as a ratio to the full range
    viewer->setZoomInWidthLimit(zoomInLimit / (viewer->getValueAtViewPort("x", 1) -
        viewer->getValueAtViewPort("x", 0)));

    // Trigger the viewPortChanged event. Updates the chart if the axis scale has changed
    // (scrolling or zooming) or if new data are added to the existing axis scale.
    viewer->updateViewPort(axisScaleHasChanged || (duration < initialFullRange), false);
}

//
// View port changed event
//
void RealTimeMultiThread::onViewPortChanged()
{
    // In addition to updating the chart, we may also need to update other controls that
    // changes based on the view port.
    updateControls(m_ChartViewer);

    // Update the chart if necessary
    if (m_ChartViewer->needUpdateChart())
        drawChart(m_ChartViewer);
}

//
// User clicks on the the horizontal scroll bar
//
void RealTimeMultiThread::onHScrollBarChanged(int value)
{
    if (!m_ChartViewer->isInViewPortChangedEvent())
    {
        // Set the view port based on the scroll bar
        int scrollBarLen = m_HScrollBar->maximum() + m_HScrollBar->pageStep();
        m_ChartViewer->setViewPortLeft(value / (double)scrollBarLen);

        // Update the chart display without updating the image maps. (We can delay updating
        // the image map until scrolling is completed and the chart display is stable.)
        m_ChartViewer->updateViewPort(true, false);
    }
}

//
// Update controls in the user interface when the view port changed
//
void RealTimeMultiThread::updateControls(QChartViewer *viewer)
{
    // The logical length of the scrollbar. It can be any large value. The actual value does
    // not matter.
    const int scrollBarLen = 1000000000;

    // Update the horizontal scroll bar
    m_HScrollBar->setEnabled(viewer->getViewPortWidth() < 1);
    m_HScrollBar->setPageStep((int)ceil(viewer->getViewPortWidth() * scrollBarLen));
    m_HScrollBar->setSingleStep(min(scrollBarLen / 100, m_HScrollBar->pageStep()));
    m_HScrollBar->setRange(0, scrollBarLen - m_HScrollBar->pageStep());
    m_HScrollBar->setValue((int)(0.5 + viewer->getViewPortLeft() * scrollBarLen));
}

//
// Draw chart
//
void RealTimeMultiThread::drawChart(QChartViewer *viewer)
{
    // Get the start date and end date that are visible on the chart.
    double viewPortStartDate = viewer->getValueAtViewPort("x", viewer->getViewPortLeft());
    double viewPortEndDate = viewer->getValueAtViewPort("x", viewer->getViewPortLeft() +
        viewer->getViewPortWidth());

    // Extract the part of the data arrays that are visible.
    DoubleArray viewPortTimeStamps;
    DoubleArray viewPortDataSeriesA;
    DoubleArray viewPortDataSeriesB;
    DoubleArray viewPortDataSeriesC;
    DoubleArray viewPortDataSeriesD;

    if (m_currentIndex > 0)
    {
        // Get the array indexes that corresponds to the visible start and end dates
        int startIndex = (int)floor(Chart::bSearch(DoubleArray(m_timeStamps, m_currentIndex), viewPortStartDate));
        int endIndex = (int)ceil(Chart::bSearch(DoubleArray(m_timeStamps, m_currentIndex), viewPortEndDate));
        int noOfPoints = endIndex - startIndex + 1;

        // Extract the visible data
        viewPortTimeStamps = DoubleArray(m_timeStamps+ startIndex, noOfPoints);
        viewPortDataSeriesA = DoubleArray(m_dataSeriesA + startIndex, noOfPoints);
        viewPortDataSeriesB = DoubleArray(m_dataSeriesB + startIndex, noOfPoints);
        viewPortDataSeriesC = DoubleArray(m_dataSeriesC + startIndex, noOfPoints);
        viewPortDataSeriesD = DoubleArray(m_dataSeriesD + startIndex, noOfPoints);

        // Keep track of the latest available data at chart plotting time
        trackLineEndPos = m_timeStamps[m_currentIndex - 1];
    }

    //
    // All the channel data and other configurable parameters
    //
    DoubleArray allChannels[] = { viewPortDataSeriesA, viewPortDataSeriesB, viewPortDataSeriesC,
                                  viewPortDataSeriesD };
    int colors[] = { 0xff0000, 0x00cc00, 0x0000ff, 0x888800 };
    const char *names[] = { "Alpha", "Beta", "Gamma", "Delta" };

    double lowerLimit[] = { 0, 0, 0, 0 };
    double upperLimit[] = { 100, 100, 10, 10 };

    //================================================================================
    // Configure overall chart appearance.
    //================================================================================

    // First 3 charts are 100 pixels in height. Last chart is 120 pixels to reserve space for the
    // x-axis labels
    MultiChart *m = new MultiChart(640, 100 * 3 + 120);

    for (int i = 0; i < 4; ++i)
    {
        // Create an XYChart object and add it to the MultiChart
        XYChart *c = new XYChart(640, (i < 3) ? 100 : 120);
        m->addChart(0, i * 100, c);

        // Configure the plot area
        c->setPlotArea(55, 10, c->getWidth() - 85, 80, 0x99ccff, -1, Chart::Transparent,
                       0xffffff, 0xffffff);

        // As the data can lie outside the plotarea in a zoomed chart, we need enable clipping.
        c->setClipping();

        // Set the x and y axis stems to transparent and the label font to 8pt Arial
        c->xAxis()->setColors(Chart::Transparent);
        c->yAxis()->setColors(Chart::Transparent);
        c->xAxis()->setLabelStyle("arial.ttf", 8, (i < 3) ? Chart::Transparent : Chart::TextColor);
        c->yAxis()->setLabelStyle("arial.ttf", 8);

        //================================================================================
        // Add data to chart
        //================================================================================

        LineLayer *layer = c->addLineLayer();
        layer->setLineWidth(2);
        layer->setFastLineMode();
        layer->setXData(viewPortTimeStamps);
        layer->addDataSet(allChannels[i], colors[i], names[i]);

        // Use the given y-axis scale
        c->yAxis()->setLinearScale(lowerLimit[i], upperLimit[i]);
        c->yAxis()->setRounding(false, false);

        //================================================================================
        // Configure axis scale and labelling
        //================================================================================

        // Set the x-axis as a date/time axis with the scale according to the view port x range.
        if (m_currentIndex > 0)
            c->xAxis()->setDateScale(viewPortStartDate, viewPortEndDate);

        // For the automatic axis labels, set the minimum spacing to 75/30 pixels for the x/y axis.
        c->xAxis()->setTickDensity(75);

        // We use "hh:nn:ss" as the axis label format.
        c->xAxis()->setLabelFormat("{value|hh:nn:ss}");

        // We make sure the tick increment must be at least 1 second.
        c->xAxis()->setMinTickInc(1);

        c->yAxis()->setTitle(names[i]);
    }

    //================================================================================
    // Output the chart
    //================================================================================

    // We need to update the track line too. If the mouse is moving on the chart (eg. if
    // the user drags the mouse on the chart to scroll it), the track line will be updated
    // in the MouseMovePlotArea event. Otherwise, we need to update the track line here.
    if (!viewer->isInMouseMoveEvent())
        trackLineLabel(m, trackLineIsAtEnd ? m->getWidth() : viewer->getPlotAreaMouseX());

    // Set the chart image to the QChartViewer
    freeChart((MultiChart *)viewer->getChart());
    viewer->setChart(m);
}

//
// Draw track cursor when mouse is moving over plotarea
//
void RealTimeMultiThread::onMouseMovePlotArea(QMouseEvent *)
{
    double trackLinePos = trackLineLabel((MultiChart *)m_ChartViewer->getChart(),
        m_ChartViewer->getPlotAreaMouseX());
    trackLineIsAtEnd = (m_currentIndex <= 0) || (trackLinePos == trackLineEndPos);

    m_ChartViewer->updateDisplay();    
}

//
// Draw the track line with data point labels
//
double RealTimeMultiThread::trackLineLabel(MultiChart *m, int mouseX)
{    
    // Clear the current dynamic layer and get the DrawArea object to draw on it.
    DrawArea *d = m->initDynamicLayer();
    // The x-value nearest to the mouse
    double xValue = 0;

    XYChart *c = 0;
    for(int i = 0; 0 != (c = (XYChart *)m->getChart(i)); ++i)
    {
        // Offset the XYChart relative to the MultiChart
        int offsetX = c->getAbsOffsetX();
        int offsetY = c->getAbsOffsetY();

        // The plot area object
        PlotArea *plotArea = c->getPlotArea();

        // Get the data x-value that is nearest to the mouse, and find its pixel coordinate.
        xValue = c->getNearestXValue(mouseX);
        int xCoor = c->getXCoor(xValue);
        if (xCoor < plotArea->getLeftX())
            continue;

        // Draw a vertical track line at the x-position
        d->vline(plotArea->getTopY() + offsetY, plotArea->getBottomY() + offsetY, xCoor + offsetX, 0x888888);

        // Draw a label on the x-axis to show the track line position.
        ostringstream xlabel;
        xlabel << "<*font,bgColor=000000*> " << c->xAxis()->getFormattedLabel(xValue + 0.00499,
            "hh:nn:ss.ff") << " <*/font*>";
        TTFText *t = d->text(xlabel.str().c_str(), "arialbd.ttf", 10);

        // Restrict the x-pixel position of the label to make sure it stays inside the chart image.
        int xLabelPos = max(0, min(xCoor - t->getWidth() / 2, c->getWidth() - t->getWidth()));
        t->draw(xLabelPos + offsetX, plotArea->getBottomY() + 6 + offsetY, 0xffffff);
        t->destroy();

        // Iterate through all layers to draw the data labels
        for (int i = 0; i < c->getLayerCount(); ++i) {
            Layer *layer = c->getLayerByZ(i);

            // The data array index of the x-value
            int xIndex = layer->getXIndexOf(xValue);

            // Iterate through all the data sets in the layer
            for (int j = 0; j < layer->getDataSetCount(); ++j)
            {
                DataSet *dataSet = layer->getDataSetByZ(j);
                const char *dataSetName = dataSet->getDataName();

                // Get the color, name and position of the data label
                int color = dataSet->getDataColor();
                int yCoor = c->getYCoor(dataSet->getPosition(xIndex), dataSet->getUseYAxis());

                // Draw a track dot with a label next to it for visible data points in the plot area
                if ((yCoor >= plotArea->getTopY()) && (yCoor <= plotArea->getBottomY()) && (color !=
                    Chart::Transparent) && dataSetName && *dataSetName)
                {
                    d->circle(xCoor + offsetX, yCoor + offsetY, 4, 4, color, color);

                    ostringstream label;
                    label << "<*font,bgColor=" << hex << color << "*> "
                        << c->formatValue(dataSet->getValue(xIndex), "{value|P4}") << " <*font*>";
                    t = d->text(label.str().c_str(), "arialbd.ttf", 10);

                    // Draw the label on the right side of the dot if the mouse is on the left side the
                    // chart, and vice versa. This ensures the label will not go outside the chart image.
                    if (xCoor <= (plotArea->getLeftX() + plotArea->getRightX()) / 2)
                        t->draw(xCoor + 6 + offsetX, yCoor + offsetY, 0xffffff, Chart::Left);
                    else
                        t->draw(xCoor - 6 + offsetX, yCoor + offsetY, 0xffffff, Chart::Right);

                    t->destroy();
                }
            }
        }
    }

    return xValue;
}

