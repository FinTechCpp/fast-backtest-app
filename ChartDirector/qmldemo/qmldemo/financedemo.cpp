#include "financedemo.h"
#include "FinanceChart.h"
#include <sstream>
#include <algorithm>


//
// Utlity to convert std::vector to a DoubleArray
//
static DoubleArray vectorToArray(const std::vector<double>& v, int startIndex = 0, int length = -1)
{
    if ((length < 0) || (length + startIndex > (int)v.size()))
        length = ((int)v.size()) - startIndex;
    return (length <= 0) ? DoubleArray() : DoubleArray(&(v[startIndex]), length);
}

//
// Utlity to convert DoubleArray to std::vector
//
static std::vector<double> arrayToVector(DoubleArray a)
{
    return std::vector<double>(a.data, a.data + a.len);
}




FinanceDemo::FinanceDemo(QObject *parent) : QObject(parent)
{
    m_currentChart = 0;
    m_currentTimeRange = 0;
}

//
// Destructor
//
FinanceDemo::~FinanceDemo()
{
    delete m_currentChart;
}

/////////////////////////////////////////////////////////////////////////////
// Data source

void FinanceDemo::loadData(QmlChartViewer *viewer, const QString& ticker, const QString& compare)
{
    // Check if data are already loaded   
    if ((m_currentTicker == ticker) && (m_currentCompare == compare))
        return;

    // The financial simulator
    if (m_currentTicker != ticker)
    {
        m_currentTicker = ticker;

        // Simulator to generate realistic random OHLC values
        FinanceSimulator db(ticker.toUtf8().data(), Chart::chartTime(2010, 1, 1),
            Chart::chartTime(2020, 12, 31), 86400);

        // Copy daily data to the data arrays
        m_dailyPrice.timeStamps = arrayToVector(db.getTimeStamps());
        m_dailyPrice.highData = arrayToVector(db.getHighData());
        m_dailyPrice.lowData = arrayToVector(db.getLowData());
        m_dailyPrice.openData = arrayToVector(db.getOpenData());
        m_dailyPrice.closeData = arrayToVector(db.getCloseData());
        m_dailyPrice.volData = arrayToVector(db.getVolData());

        // Set up the full range
        viewer->setFullRange("x", 0, (int)m_dailyPrice.timeStamps.size() - 1);
    }

    if (m_currentCompare != compare)
    {
        m_currentCompare = compare;
        if (m_currentCompare.isEmpty())
            m_dailyPrice.compareData.clear();
        else
        {
            // Simulator to generate realistic random OHLC values
            FinanceSimulator db2(compare.toUtf8().data(), Chart::chartTime(2010, 1, 1),
                Chart::chartTime(2020, 12, 31), 86400);
            m_dailyPrice.compareData = arrayToVector(db2.getCloseData());
        }
    }

    // In this example, we will compute the weekly and monthly prices on demand. We just
    // need to clear the old data here.
    m_weeklyPrice.timeStamps.clear();
    m_monthlyPrice.timeStamps.clear();
}

/// <summary>
/// An internal method used to aggregate daily data.
/// </summary>
void FinanceDemo::aggregateData(const ArrayMath& aggregator, PriceData& p)
{
    int size = aggregator.aggregate(vectorToArray(p.timeStamps), Chart::AggregateFirst).len;
    p.timeStamps.resize(size);
    size = aggregator.aggregate(vectorToArray(p.highData), Chart::AggregateMax).len;
    p.highData.resize(size);
    size = aggregator.aggregate(vectorToArray(p.lowData), Chart::AggregateMin).len;
    p.lowData.resize(size);
    size = aggregator.aggregate(vectorToArray(p.openData), Chart::AggregateFirst).len;
    p.openData.resize(size);
    size = aggregator.aggregate(vectorToArray(p.closeData), Chart::AggregateLast).len;
    p.closeData.resize(size);
    size = aggregator.aggregate(vectorToArray(p.volData), Chart::AggregateSum).len;
    p.volData.resize(size);
    size = aggregator.aggregate(vectorToArray(p.compareData), Chart::AggregateLast).len;
    p.compareData.resize(size);
}

/////////////////////////////////////////////////////////////////////////////
// Chart Creation

/// <summary>
/// Add a moving average line to the FinanceChart object.
/// </summary>
/// <param name="m">The FinanceChart object to add the line to.</param>
/// <param name="avgType">The moving average type (SMA/EMA/TMA/WMA).</param>
/// <param name="avgPeriod">The moving average period.</param>
/// <param name="color">The color of the line.</param>
static LineLayer* addMovingAvg(FinanceChart *m, QString avgType, int avgPeriod, int color)
{
    if (avgPeriod > 1)
    {
        if (avgType == "SMA")
            return m->addSimpleMovingAvg(avgPeriod, color);
        else if (avgType == "EMA")
            return m->addExpMovingAvg(avgPeriod, color);
        else if (avgType == "TMA")
            return m->addTriMovingAvg(avgPeriod, color);
        else if (avgType == "WMA")
            return m->addWeightedMovingAvg(avgPeriod, color);
    }

    return 0;
}

/// <summary>
/// Add an indicator chart to the FinanceChart object. In this demo example, the indicator
/// parameters (such as the period used to compute RSI, colors of the lines, etc.) are hard
/// coded to commonly used values. You are welcome to design a more complex user interface
/// to allow users to set the parameters.
/// </summary>
/// <param name="m">The FinanceChart object to add the line to.</param>
/// <param name="indicator">The selected indicator.</param>
/// <param name="height">Height of the chart in pixels</param>
static XYChart* addIndicator(FinanceChart *m, QString indicator, int height)
{
    if (indicator == "RSI")
        return m->addRSI(height, 14, 0x800080, 20, 0xff6666, 0x6666ff);
    else if (indicator == "StochRSI")
        return m->addStochRSI(height, 14, 0x800080, 30, 0xff6666, 0x6666ff);
    else if (indicator == "MACD")
        return m->addMACD(height, 26, 12, 9, 0xff, 0xff00ff, 0x8000);
    else if (indicator == "FStoch")
        return m->addFastStochastic(height, 14, 3, 0x6060, 0x606000);
    else if (indicator == "SStoch")
        return m->addSlowStochastic(height, 14, 3, 0x6060, 0x606000);
    else if (indicator == "ATR")
        return m->addATR(height, 14, 0x808080, 0xff);
    else if (indicator == "ADX")
        return m->addADX(height, 14, 0x8000, 0x800000, 0x80);
    else if (indicator == "DCW")
        return m->addDonchianWidth(height, 20, 0xff);
    else if (indicator == "BBW")
        return m->addBollingerWidth(height, 20, 2, 0xff);
    else if (indicator == "DPO")
        return m->addDPO(height, 20, 0xff);
    else if (indicator == "PVT")
        return m->addPVT(height, 0xff);
    else if (indicator == "Momentum")
        return m->addMomentum(height, 12, 0xff);
    else if (indicator == "Performance")
        return m->addPerformance(height, 0xff);
    else if (indicator == "ROC")
        return m->addROC(height, 12, 0xff);
    else if (indicator == "OBV")
        return m->addOBV(height, 0xff);
    else if (indicator == "AccDist")
        return m->addAccDist(height, 0xff);
    else if (indicator == "CLV")
        return m->addCLV(height, 0xff);
    else if (indicator == "WilliamR")
        return m->addWilliamR(height, 14, 0x800080, 30, 0xff6666, 0x6666ff);
    else if (indicator == "Aroon")
        return m->addAroon(height, 14, 0x339933, 0x333399);
    else if (indicator == "AroonOsc")
        return m->addAroonOsc(height, 14, 0xff);
    else if (indicator == "CCI")
        return m->addCCI(height, 20, 0x800080, 100, 0xff6666, 0x6666ff);
    else if (indicator == "EMV")
        return m->addEaseOfMovement(height, 9, 0x6060, 0x606000);
    else if (indicator == "MDX")
        return m->addMassIndex(height, 0x800080, 0xff6666, 0x6666ff);
    else if (indicator == "CVolatility")
        return m->addChaikinVolatility(height, 10, 10, 0xff);
    else if (indicator == "COscillator")
        return m->addChaikinOscillator(height, 0xff);
    else if (indicator == "CMF")
        return m->addChaikinMoneyFlow(height, 21, 0x8000);
    else if (indicator == "NVI")
        return m->addNVI(height, 255, 0xff, 0x883333);
    else if (indicator == "PVI")
        return m->addPVI(height, 255, 0xff, 0x883333);
    else if (indicator == "MFI")
        return m->addMFI(height, 14, 0x800080, 30, 0xff6666, 0x6666ff);
    else if (indicator == "PVO")
        return m->addPVO(height, 26, 12, 9, 0xff, 0xff00ff, 0x8000);
    else if (indicator == "PPO")
        return m->addPPO(height, 26, 12, 9, 0xff, 0xff00ff, 0x8000);
    else if (indicator == "UO")
        return m->addUltimateOscillator(height, 7, 14, 28, 0x800080, 20, 0xff6666, 0x6666ff);
    else if (indicator == "Vol")
        return m->addVolIndicator(height, 0x99ff99, 0xff9999, 0xc0c0c0);
    else if (indicator == "TRIX")
        return m->addTRIX(height, 12, 0xff);
    else
        return 0;
}

/// <summary>
/// Creates a dummy chart to show an error message.
/// </summary>
/// <param name="viewer">The WinChartViewer to display the error message.</param>
/// <param name="msg">The error message</param>
static void errMsg(QmlChartViewer* viewer, const char *msg)
{
     MultiChart m(400, 200);
     m.addTitle(Chart::Center, msg, "Arial", 10)->setMaxWidth(m.getWidth());
     viewer->setChart(&m);
}

/// <summary>
/// Draw the chart according to user selection and display it in the ChartViewer.
/// </summary>
/// <param name="viewer">The ChartViewer object to display the chart.</param>
void FinanceDemo::drawChart(QmlChartViewer *viewer)
{
    loadData(viewer, m_tickerSymbol, m_compareWith);

    // Update current time range if it has been changed
    if (m_currentTimeRange != m_timeRange)
    {
        m_currentTimeRange = m_timeRange;
        viewer->setViewPortWidth(m_timeRange / (double)m_dailyPrice.timeStamps.size());
        viewer->setViewPortLeft(1 - viewer->getViewPortWidth());
    }

    // The moving average periods
    int movAvgPeriod1 = (std::max)(0, (std::min)(300, m_movAvg1.toInt()));
    int movAvgPeriod2 = (std::max)(0, (std::min)(300, m_movAvg2.toInt()));

    // We need extra leading data points in order to compute moving averages.
    int extraPoints = (std::max)(25, (std::max)(movAvgPeriod1, movAvgPeriod2));

    // Get the start and end index visible on the viewport
    int startIndex = (int)floor(viewer->getValueAtViewPort("x", viewer->getViewPortLeft()));
    int endIndex = (int)ceil(viewer->getValueAtViewPort("x", viewer->getViewPortRight()));

    PriceData* p = &m_dailyPrice;
    if (endIndex - startIndex >= 400)
    {
        if (endIndex - startIndex < 2000)
        {
            p = &m_weeklyPrice;
            if (p->timeStamps.empty())
            {
                ArrayMath a(vectorToArray(m_dailyPrice.timeStamps));
                aggregateData(a.selectStartOfWeek(), *p = m_dailyPrice);
            }
        }
        else
        {
            p = &m_monthlyPrice;
            if (p->timeStamps.empty())
            {
                ArrayMath a(vectorToArray(m_dailyPrice.timeStamps));
                aggregateData(a.selectStartOfMonth(), *p = m_dailyPrice);
            }
        }

        // Re-compute the index based on weekly data
        DoubleArray ts = vectorToArray(p->timeStamps);
        startIndex = (int)floor(Chart::bSearch(ts, m_dailyPrice.timeStamps[startIndex]));
        endIndex = (int)ceil(Chart::bSearch(ts, m_dailyPrice.timeStamps[endIndex]));
    }

    //
    // At this stage, we have determined the price data to use. We need to obtain the part
    // of the arrays that are within the visible x-axis range.
    //

    // Add extra points before the startIndex, as moving averages and may technical indicators
    // requires data before the first visible point.
    extraPoints = (std::min)(extraPoints, startIndex);
    startIndex -= extraPoints;
    int duration = endIndex - startIndex + 1;

    // Obtain the visible part of the data plus the extra points
    DoubleArray timeStamps = vectorToArray(p->timeStamps, startIndex, duration);
    DoubleArray highData = vectorToArray(p->highData, startIndex, duration);
    DoubleArray lowData = vectorToArray(p->lowData, startIndex, duration);
    DoubleArray openData = vectorToArray(p->openData, startIndex, duration);
    DoubleArray closeData = vectorToArray(p->closeData, startIndex, duration);
    DoubleArray volData = vectorToArray(p->volData, startIndex, duration);
    DoubleArray compareData = vectorToArray(p->compareData, startIndex, duration);

    //
    // Now, we have obtain the data. We can plot the chart.
    //
    int width = 780;
    int mainHeight = 260;
    int indicatorHeight = 80;

    // Create the chart object using the selected size
    FinanceChart *m = new FinanceChart(width);

    // Disable default legend box, as we are using dynamic legend
    m->setLegendStyle("normal", 8, Chart::Transparent, Chart::Transparent);

    // Set the data into the chart object
    m->setData(timeStamps, highData, lowData, openData, closeData, volData, extraPoints);

    //
    // We configure the title of the chart. In this demo chart design, we put the
    // company name as the top line of the title with left alignment.
    //
    m->addPlotAreaTitle(Chart::BottomLeft, m_currentTicker.toUtf8().data());

    // A copyright message at the bottom left corner the title area
    m->addPlotAreaTitle(Chart::BottomRight,
        "<*font=Arial,size=8*>(c) Advanced Software Engineering");

    //
    // Add the main chart
    //
    m->addMainChart(mainHeight);

    //
    // Set log or linear scale according to user preference
    //
    m->setLogScale(m_logScale);

    //
    // Set axis labels to show data values or percentage change to user preference
    //
    if (m_percentageScale)
            m->setPercentageAxis();

    //
    // Draw the main chart depending on the chart type the user has selected
    //
    if (m_chartType == "Close")
        m->addCloseLine(0x000040);
    else if (m_chartType == "TP")
        m->addTypicalPrice(0x000040);
    else if (m_chartType == "WC")
        m->addWeightedClose(0x000040);
    else if (m_chartType == "Median")
        m->addMedianPrice(0x000040);

    //
    // Add comparison line if there is data for comparison
    //
    if (compareData.len > 0)
        m->addComparison(compareData, 0x0000ff, m_currentCompare.toUtf8().data());

    //
    // Add moving average lines.
    //
    addMovingAvg(m, m_avgType1, movAvgPeriod1, 0x663300);
    addMovingAvg(m, m_avgType2, movAvgPeriod2, 0x9900ff);

    //
    // Draw the main chart if the user has selected CandleStick or OHLC. We
    // draw it here to make sure it is drawn behind the moving average lines
    // (that is, the moving average lines stay on top.)
    //
    if (m_chartType == "CandleStick")
        m->addCandleStick(0x33ff33, 0xff3333);
    else if (m_chartType == "OHLC")
        m->addHLOC(0x8000, 0x800000);

    //
    // Add parabolic SAR if necessary
    //
    if (m_parabolicSAR)
        m->addParabolicSAR(0.02, 0.02, 0.2, Chart::DiamondShape, 5, 0x008800, 0x000000);

    //
    // Add price band/channel/envelop to the chart according to user selection
    //
    if (m_priceBand == "BB")
        m->addBollingerBand(20, 2, 0x9999ff, 0xc06666ff);
    else if (m_priceBand == "DC")
        m->addDonchianChannel(20, 0x9999ff, 0xc06666ff);
    else if (m_priceBand == "Envelop")
        m->addEnvelop(20, 0.1, 0x9999ff, 0xc06666ff);

    //
    // Add volume bars to the main chart if necessary
    //
    if (m_volumeBars)
        m->addVolBars(indicatorHeight, 0x99ff99, 0xff9999, 0xc0c0c0);

    //
    // Add additional indicators as according to user selection.
    //
    addIndicator(m, m_indicator1, indicatorHeight);
    addIndicator(m, m_indicator2, indicatorHeight);

    // We may need to update the track line. If the mouse is moving on the chart, the track line
    // will be updated in MouseMovePlotArea. Otherwise, we need to update the track line here.
    if (!viewer->isInMouseMoveEvent())
    {
        trackFinance(m, (0 == viewer->getChart()) ?
            ((XYChart *)m->getChart(0))->getPlotArea()->getRightX() : viewer->getPlotAreaMouseX());
    }

    // Set the chart to the viewer
    delete viewer->getChart();
    viewer->setChart(m_currentChart = m);
}

//
// Draw track cursor when mouse is moving over plotarea
//
void FinanceDemo::drawTrackCursor(QmlChartViewer *viewer, int mouseX)
{
    trackFinance((MultiChart *)viewer->getChart(), mouseX);
    viewer->updateDisplay();
}

//
// Draw finance chart track line with legend
//
void FinanceDemo::trackFinance(MultiChart* m, int mouseX)
{
    // Clear the current dynamic layer and get the DrawArea object to draw on it.
    DrawArea *d = m->initDynamicLayer();

    // It is possible for a FinanceChart to be empty, so we need to check for it.
    if (m->getChartCount() == 0)
        return;

    // Get the data x-value that is nearest to the mouse
    int xValue = (int)(((XYChart *)m->getChart(0))->getNearestXValue(mouseX));

    // Iterate the XY charts (main price chart and indicator charts) in the FinanceChart
    XYChart *c = 0;
    for (int i = 0; i < m->getChartCount(); ++i) {
        c = (XYChart *)m->getChart(i);

        // Variables to hold the legend entries
        std::ostringstream ohlcLegend;
        std::vector<std::string> legendEntries;

        // Iterate through all layers to find the highest data point
        for (int j = 0; j < c->getLayerCount(); ++j) {
            Layer *layer = c->getLayerByZ(j);
            int xIndex = layer->getXIndexOf(xValue);
            int dataSetCount = layer->getDataSetCount();

            // In a FinanceChart, only layers showing OHLC data can have 4 data sets
            if (dataSetCount == 4) {
                double highValue = layer->getDataSet(0)->getValue(xIndex);
                double lowValue = layer->getDataSet(1)->getValue(xIndex);
                double openValue = layer->getDataSet(2)->getValue(xIndex);
                double closeValue = layer->getDataSet(3)->getValue(xIndex);

                if (closeValue != Chart::NoValue) {
                    // Build the OHLC legend
                    ohlcLegend << "      <*block*>";
                    ohlcLegend << "Open: " << c->formatValue(openValue, "{value|P4}");
                    ohlcLegend << ", High: " << c->formatValue(highValue, "{value|P4}");
                    ohlcLegend << ", Low: " << c->formatValue(lowValue, "{value|P4}");
                    ohlcLegend << ", Close: " << c->formatValue(closeValue, "{value|P4}");

                    // We also draw an upward or downward triangle for up and down days and the %
                    // change
                    double lastCloseValue = layer->getDataSet(3)->getValue(xIndex - 1);
                    if (lastCloseValue != Chart::NoValue) {
                        double change = closeValue - lastCloseValue;
                        double percent = change * 100 / closeValue;
                        std::string symbol = (change >= 0) ?
                            "<*font,color=008800*><*img=@triangle,width=8,color=008800*>" :
                            "<*font,color=CC0000*><*img=@invertedtriangle,width=8,color=CC0000*>";

                        ohlcLegend << "  " << symbol << " " << c->formatValue(change, "{value|P4}");
                        ohlcLegend << " (" << c->formatValue(percent, "{value|2}") << "%)<*/font*>";
                    }

                    ohlcLegend << "<*/*>";
                }
            }
            else {
                // Iterate through all the data sets in the layer
                for (int k = 0; k < layer->getDataSetCount(); ++k) {
                    DataSet *dataSet = layer->getDataSetByZ(k);

                    std::string name = dataSet->getDataName();
                    double value = dataSet->getValue(xIndex);
                    if ((0 != name.size()) && (value != Chart::NoValue)) {

                        // In a FinanceChart, the data set name consists of the indicator name and
                        // its latest value. It is like "Vol: 123M" or "RSI (14): 55.34". As we are
                        // generating the values dynamically, we need to extract the indictor name
                        // out, and also the volume unit (if any).

                        // The volume unit
                        std::string unitChar;

                        // The indicator name is the part of the name up to the colon character.
                        int delimiterPosition = (int)name.find(':');
                        if (name.npos != delimiterPosition) {

                            // The unit, if any, is the trailing non-digit character(s).
                            int lastDigitPos = (int)name.find_last_of("0123456789");
                            if ((name.npos != lastDigitPos) && (lastDigitPos + 1 < (int)name.size()) &&
                                (lastDigitPos > delimiterPosition))
                                unitChar = name.substr(lastDigitPos + 1);

                            name.resize(delimiterPosition);
                        }

                        // In a FinanceChart, if there are two data sets, it must be representing a
                        // range.
                        if (dataSetCount == 2) {
                            // We show both values in the range in a single legend entry
                            value = layer->getDataSet(0)->getValue(xIndex);
                            double value2 = layer->getDataSet(1)->getValue(xIndex);
                            name = name + ": " + c->formatValue((std::min)(value, value2), "{value|P3}");
                            name = name + " - " + c->formatValue((std::max)(value, value2), "{value|P3}");
                        }
                        else {
                            // In a FinanceChart, only the layer for volume bars has 3 data sets for
                            // up/down/flat days
                            if (dataSetCount == 3) {
                                // The actual volume is the sum of the 3 data sets.
                                value = layer->getDataSet(0)->getValue(xIndex) + layer->getDataSet(1
                                    )->getValue(xIndex) + layer->getDataSet(2)->getValue(xIndex);
                            }

                            // Create the legend entry
                            name = name + ": " + c->formatValue(value, "{value|P3}") + unitChar;
                        }

                        // Build the legend entry, consist of a colored square box and the name (with
                        // the data value in it).
                        std::ostringstream legendEntry;
                        legendEntry << "<*block*><*img=@square,width=8,edgeColor=000000,color="
                            << std::hex << dataSet->getDataColor() << "*> " << name << "<*/*>";
                        legendEntries.push_back(legendEntry.str());
                    }
                }
            }
        }

        // Get the plot area position relative to the entire FinanceChart
        PlotArea *plotArea = c->getPlotArea();
        int plotAreaLeftX = plotArea->getLeftX() + c->getAbsOffsetX();
        int plotAreaTopY = plotArea->getTopY() + c->getAbsOffsetY();

        // The legend begins with the date label, then the ohlcLegend (if any), and then the
        // entries for the indicators.
        std::ostringstream legendText;
        legendText << "<*block,valign=top,maxWidth=" << (plotArea->getWidth() - 5)
            << "*><*font=Arial Bold*>[" << c->xAxis()->getFormattedLabel(xValue, "mmm dd, yyyy")
            << "]<*/font*>" << ohlcLegend.str();
        for (int i = ((int)legendEntries.size()) - 1; i >= 0; --i) {
            legendText << "      " << legendEntries[i];
        }
        legendText << "<*/*>";

        // Draw a vertical track line at the x-position
        d->vline(plotAreaTopY, plotAreaTopY + plotArea->getHeight(), c->getXCoor(xValue) +
            c->getAbsOffsetX(), d->dashLineColor(0x000000, 0x0101));

        // Display the legend on the top of the plot area
        TTFText *t = d->text(legendText.str().c_str(), "Arial", 8);
        t->draw(plotAreaLeftX + 5, plotAreaTopY + 3, 0x000000, Chart::TopLeft);
        t->destroy();
    }
}



