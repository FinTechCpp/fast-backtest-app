#include "ui/chart/chartRenderer.h"
#include "ui/chart/chartDataManager.h"
#include <QDebug>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>

#include "beTypes.h"

ChartRenderer::ChartRenderer() : m_financeChart(nullptr) {
}

ChartRenderer::~ChartRenderer() {
}

void ChartRenderer::createOrUpdateChart(
    QChartViewer* viewer,
    const ChartDataManager& dataManager,
    const chart::ChartConfiguration& config,
    const chart::AggregationInfo& aggregationInfo)
{
    // Extract data based on the aggregation level
    DoubleArray timestamps, openData, highData, lowData, closeData, volumeData;

    {
        const auto& data = dataManager.getAggregatedData(aggregationInfo.level);
        size_t startIdx = aggregationInfo.startIndex;
        size_t count = aggregationInfo.pointCount;

        timestamps = DoubleArray(&data.timestamps[startIdx], static_cast<int>(count));
        volumeData = DoubleArray(&data.volume[startIdx], static_cast<int>(count));

        if (aggregationInfo.level == chart::AggregationLevel::Raw && config.chartType == chart::ChartType::HeikinAshi) {
            const auto& heikinAshiCache = dataManager.getHeikinAshiCache();
            openData = DoubleArray(&heikinAshiCache.open[startIdx], static_cast<int>(count));
            highData = DoubleArray(&heikinAshiCache.high[startIdx], static_cast<int>(count));
            lowData = DoubleArray(&heikinAshiCache.low[startIdx], static_cast<int>(count));
            closeData = DoubleArray(&heikinAshiCache.close[startIdx], static_cast<int>(count));
        } else {
            openData = DoubleArray(&data.open[startIdx], static_cast<int>(count));
            highData = DoubleArray(&data.high[startIdx], static_cast<int>(count));
            lowData = DoubleArray(&data.low[startIdx], static_cast<int>(count));
            closeData = DoubleArray(&data.close[startIdx], static_cast<int>(count));
        }
    }
    
    // Create a new chart
    m_financeChart = std::make_unique<FinanceChart>(config.chartWidth);
    
    // Configure the chart appearance
    m_financeChart->setPlotAreaStyle(0xE2F4FF, 0xCC999999, 0xCC999999, 0xCC999999, 0xCC999999);
    m_financeChart->setDateLabelFormat(
        "<*font=Arial Bold,size=12,color=800000*>{value|yyyy}<*/font*>",  // Years in bold and red
        "<*font=Arial Bold,size=10*>{value|MMM yyyy}<*/font*>",  // First month with year
        "<*font=Arial*>{value|MMM}<*/font*>",  // Other months
        "<*font=Arial Bold*>{value|d MMM}<*/font*>",  // First day with month
        "{value|d}",  // Other days
        "<*font=Arial Bold*>{value|d MMM hh:nn}<*/font*>",  // First hour with day
        "{value|hh:nn}"  // Other hours
    );
    m_financeChart->setMargins(0, 20, 50, 100);  // No margins
    m_financeChart->setDateLabelSpacing(80);
    
    // Configure the data
    m_financeChart->setData(timestamps, highData, lowData, openData, closeData, volumeData, 0);
    // 0 ms

    // Hide the default legend by making it transparent
    m_financeChart->setLegendStyle("normal", 8, Chart::Transparent, Chart::Transparent);
    m_financeChart->setPlotAreaBorder(Chart::Transparent, 0);

    // Add the chart title
    std::string chartTypeStr = chart::chartTypeToString(config.chartType);
    std::string aggregationStr = chart::aggregationLevelToString(aggregationInfo.level);
    std::string title = "Trading Chart (" + chartTypeStr + ", " + aggregationStr + ") - " + std::to_string(timestamps.len) + " points";
    m_financeChart->addTitle(title.c_str());
    
    // Determine the starting index for visible data
    int startIndex = static_cast<int>(aggregationInfo.startIndex);

    int subChartsTotalHeight = 30;
    
    // 1. Add the equity curve at the top if available and requested
    if (config.showEquity) {
        addEquityCurveSection(m_financeChart.get(), dataManager, timestamps, startIndex, config.equityHeight);
        subChartsTotalHeight += config.equityHeight;
    }

    for (const indicators::RSIInstance* rsi : dataManager.getIndicatorsOfType<indicators::RSIInstance>())
        if (rsi->visible)
            subChartsTotalHeight += rsi->height;
    
    // 3. Space for Stochastic
    for (const indicators::StochasticInstance* stochastic : dataManager.getIndicatorsOfType<indicators::StochasticInstance>())
        if (stochastic->visible)
            subChartsTotalHeight += stochastic->height;
    
    // 4. Space for ATR
    for (const indicators::ATRInstance* atr : dataManager.getIndicatorsOfType<indicators::ATRInstance>())
        if (atr->visible)
            subChartsTotalHeight += atr->height;

    // 5. Space for CCI
    for (const indicators::CCIInstance* cci : dataManager.getIndicatorsOfType<indicators::CCIInstance>())
        if (cci->visible)
            subChartsTotalHeight += cci->height;

    // 6. Space for MACD
    for (const indicators::MACDInstance* macd : dataManager.getIndicatorsOfType<indicators::MACDInstance>())
        if (macd->visible)
            subChartsTotalHeight += macd->height;

    // 2. Add the main chart
    int mainChartHeight = std::max(300, config.chartHeight - subChartsTotalHeight);
    XYChart* mainChart = m_financeChart->addMainChart(mainChartHeight);

    // Customize grid display
    mainChart->xAxis()->setWidth(2);  // Thicker axis
    mainChart->xAxis()->setTickLength(4, 2);  // More visible ticks
    mainChart->xAxis()->setLabelStyle("Arial Bold", 9);  // More readable labels
    mainChart->yAxis()->setAutoScale(0.01, 0.01, 0);
    // mainChart->xAxis()->setMinTickGap(20); // Minimum space between ticks (in pixels)
    // mainChart->xAxis()->setMaxTickGap(20);

    if (config.fixedYScale) {
        // Calculate new limits by applying the offset
        double newMin = config.yScaleMin + config.yScaleOffset;
        double newMax = config.yScaleMax + config.yScaleOffset;
        
        // Apply the limits to the main chart
        mainChart->yAxis()->setRounding(false, false);
        mainChart->yAxis()->setLinearScale(newMin, newMax);
    }
 
    // Add the appropriate chart type based on the current type
    if (config.chartType == chart::ChartType::CandleStick || 
        config.chartType == chart::ChartType::HeikinAshi) {
        CandleStickLayer* candleStickLayer = m_financeChart->addCandleStick(0x0d99001, 0xF30000); // Green/Red for candles
        candleStickLayer->setColors(0x0d9901, 0x0d9901, 0xF30000, 0xF30000);
        candleStickLayer->setDataWidth(50);
        candleStickLayer->setDataGap(0.1); // Space between candles
    } else if (config.chartType == chart::ChartType::OHLC) {
        m_financeChart->addHLOC(0x00CC00, 0xFF3333); // Green/Red for OHLC bars
    } else if (config.chartType == chart::ChartType::Close) {
        m_financeChart->addCloseLine(0x000088); // Blue line for close price
    }
    
    // 3. Add all active indicators
    // RSI
    for (const indicators::RSIInstance* rsi : dataManager.getIndicatorsOfType<indicators::RSIInstance>()) {
        if (rsi->visible) {
            addRSIToChart(m_financeChart.get(), *rsi, dataManager, aggregationInfo);
        }
    }
    
    // EMA
    for (const indicators::EMAInstance* ema : dataManager.getIndicatorsOfType<indicators::EMAInstance>()) {
        if (ema->visible) {
            addEMAToChart(m_financeChart.get(), *ema, dataManager, aggregationInfo);
        }
    }
    
    // Supertrend
    for (const indicators::SuperTrendInstance* supertrend : dataManager.getIndicatorsOfType<indicators::SuperTrendInstance>()) {
        if (supertrend->visible) {
            addSupertrendToChart(m_financeChart.get(), *supertrend, dataManager, aggregationInfo);
        }
    }
    
    // Stochastic
    for (const indicators::StochasticInstance* stochastic : dataManager.getIndicatorsOfType<indicators::StochasticInstance>()) {
        if (stochastic->visible) {
            addStochasticToChart(m_financeChart.get(), *stochastic, dataManager, aggregationInfo);
        }
    }
    
    // ATR
    for (const indicators::ATRInstance* atr : dataManager.getIndicatorsOfType<indicators::ATRInstance>()) {
        if (atr->visible) {
            addATRToChart(m_financeChart.get(), *atr, dataManager, aggregationInfo);
        }
    }
    // CCI
    for (const indicators::CCIInstance* cci : dataManager.getIndicatorsOfType<indicators::CCIInstance>()) {
        if (cci->visible) {
            addCCIToChart(m_financeChart.get(), *cci, dataManager, aggregationInfo);
        }
    }

    // MACD
    for (const indicators::MACDInstance* macd : dataManager.getIndicatorsOfType<indicators::MACDInstance>()) 
        if (macd->visible) 
            addMACDToChart(m_financeChart.get(), *macd, dataManager, aggregationInfo);

    // Pivot points
    for (const indicators::PivotPointsInstance* pivotPoints : dataManager.getIndicatorsOfType<indicators::PivotPointsInstance>()) {
        if (pivotPoints->visible) {
            addPivotPointsToChart(mainChart, *pivotPoints, dataManager, aggregationInfo);
        }
    }

    // BB
    for (const indicators::BBInstance* bb : dataManager.getIndicatorsOfType<indicators::BBInstance>()) {
        if (bb->visible) {
            addBBToChart(m_financeChart.get(), *bb, dataManager, aggregationInfo);
        }
    }

    // Super important, will allow me to do vertical zoom
    // mainChart->yAxis()->setLinearScale(200, 20000);

    
    // // 4. Add volume if requested
    // if (config.showVolume) {
    //     m_financeChart->addVolBars(config.volumeHeight, 0x99ff99, 0xff9999, 0x808080);
    // }

    // 5. Add trades if available and requested
    if (config.showTrades) {
        addTradeMarkers(mainChart, timestamps, dataManager, aggregationInfo);
        // 0 ms
    }

    // 6. Add user-drawn markers
    addUserMarkers(mainChart, dataManager.getMarkers(), aggregationInfo);

    // std::cout << "Before " << mainChart->getYCoor(20000) << std::endl;

    // Update the chart in the viewer
    if (viewer) {
        viewer->setChart(m_financeChart.get());
        // TAKES A LOT OF TIME HERE (50-100ms) I don't know how to optimize it
    }

    // std::cout << "After " << mainChart->getYCoor(20000) << std::endl;

    // Update the renderer dimensions
    m_lastYMin = mainChart->yAxis()->getMinValue();
    m_lastYMax = mainChart->yAxis()->getMaxValue();
    m_plotAreaHeight = mainChart->getPlotArea()->getHeight();
}

std::optional<std::pair<int, int>> ChartRenderer::updateDynamicLayer(QChartViewer *viewer, bool rulerEnabled, 
    bool rulerFirstPointSelected, int rulerStartX, int rulerStartY, 
    const ChartDataManager &dataManager, const chart::AggregationInfo &aggregationInfo)
{
    MultiChart* chart = dynamic_cast<MultiChart*>(viewer->getChart());

    // Verify that the chart is valid
    if (!chart || chart->getChartCount() == 0) 
        return std::nullopt;

    // Initialize the dynamic layer only once
    DrawArea* d = chart->initDynamicLayer();

    int mouseX = viewer->getPlotAreaMouseX();
    int mouseY = viewer->getPlotAreaMouseY();

    trackFinance(chart, mouseX, mouseY, d);

    if (rulerEnabled && rulerFirstPointSelected) {
        int rulerEndX = viewer->getPlotAreaMouseX();
        int rulerEndY = viewer->getPlotAreaMouseY();

        drawRuler(chart, rulerStartX, rulerStartY, rulerEndX, rulerEndY, d, 
                 dataManager, aggregationInfo);
    }

    return std::make_pair(mouseX, mouseY);
}

void ChartRenderer::updateTrackFinance(QChartViewer* viewer, std::pair<int, int> forcedMousePosition) 
{
    MultiChart* chart = dynamic_cast<MultiChart*>(viewer->getChart());

    if (!viewer || !chart || chart->getChartCount() == 0) 
        return;

    DrawArea* d = chart->initDynamicLayer();

    int mouseX = forcedMousePosition.first;
    int mouseY = forcedMousePosition.second;

    trackFinance(chart, mouseX, mouseY, d);
}

void ChartRenderer::addEquityCurveSection(FinanceChart *chart, const ChartDataManager& dataManager, const DoubleArray &timestamps, int startIndex, int equityHeight)
{
    const auto& equityData = dataManager.getEquityData();
    if (equityData.equity_values.empty() || timestamps.len == 0)
        return;
    
    // Get the visible time range
    double visibleStartTime = timestamps[0];
    double visibleEndTime = timestamps[timestamps.len - 1];

    enum class SegmentColor {
        Constant,
        Up,
        Down
    };
    
    // Create vectors for interpolated equity data
    std::vector<double> interpolatedTimes;
    std::vector<double> interpolatedValues;
    std::vector<SegmentColor> colorValues;
    
    interpolatedTimes.reserve(timestamps.len);
    interpolatedValues.reserve(timestamps.len);
    colorValues.reserve(timestamps.len);
    
    // Find the first equity point that precedes or equals visibleStartTime
    size_t equityIndex = 0;
    while (equityIndex + 1 < equityData.timestamps.size() && 
           equityData.timestamps[equityIndex + 1] < visibleStartTime) {
        equityIndex++;
    }
    
    // Equity value at the start of the visible window
    double currentEquityValue = equityData.equity_values[equityIndex];

    // For each visible timestamp, interpolate the equity value
    for (int i = 0; i < timestamps.len; ++i) {
        double currentTime = timestamps[i];
        
        // Advance in equity data if necessary
        while (equityIndex + 1 < equityData.timestamps.size() && 
               equityData.timestamps[equityIndex + 1] <= currentTime) {
            equityIndex++;
            currentEquityValue = equityData.equity_values[equityIndex];
        }
        
        // Add the interpolated point
        interpolatedTimes.push_back(i);
        interpolatedValues.push_back(currentEquityValue);
        
        // Determine the segment color
        if (i > 0) {
            double prev = interpolatedValues[i-1];
            double curr = currentEquityValue;
            double diff = curr - prev;
            
            if (std::abs(diff) < 1e-10) {
                colorValues.push_back(SegmentColor::Constant);
            } else if (diff > 0) {
                colorValues.push_back(SegmentColor::Up);
            } else {
                colorValues.push_back(SegmentColor::Down);
            }
        }
    }
    
    // Convert to DoubleArray
    DoubleArray equityTimes = ChartDataManager::vectorToDoubleArray(interpolatedTimes);
    DoubleArray equityValues = ChartDataManager::vectorToDoubleArray(interpolatedValues);
    
    // Add the indicator for the equity curve
    XYChart* equityChart = chart->addIndicator(equityHeight);
    
    // Configure title and labels
    equityChart->xAxis()->setColors(Chart::Transparent);
    
    // Define colors for segments
    int constColor = 0x999999;
    int upColor = 0x53DD00;
    int downColor = 0xFF0000;
    
    // Create three separate stepline layers
    StepLineLayer* equityLineLayer = equityChart->addStepLineLayer(equityValues, Chart::Transparent, "Equity");
    StepLineLayer* constantLayer = equityChart->addStepLineLayer();
    StepLineLayer* upLayer = equityChart->addStepLineLayer();
    StepLineLayer* downLayer = equityChart->addStepLineLayer(); 
    
    constantLayer->setFastLineMode(true);
    upLayer->setFastLineMode(true);
    downLayer->setFastLineMode(true);

    // Create a horizontal line for the initial cash
    double initialCash = equityData.equity_values.front();

    Mark* mark = equityChart->yAxis()->addMark(initialCash, 0x000000, "Initial Cash");
    mark->setLineWidth(2);
    mark->setMarkColor(equityChart->dashLineColor(0x000000), 0xffffff);
    mark->setAlignment(Chart::Left);
    mark->setBackground(0x000000, 0xffffff, 1);
    equityChart->addInterLineLayer(equityLineLayer->getLine(), mark->getLine(), 0xCC53DD00, 0xCCff0000);

    // Configure steplines alignment
    constantLayer->setAlignment(Chart::Left);
    upLayer->setAlignment(Chart::Left);
    downLayer->setAlignment(Chart::Left);
    
    // Create datasets for each type of segment
    std::vector<std::vector<double>> segmentX(3);
    std::vector<std::vector<double>> segmentY(3);
    
    // Iterate points and create colored segments
    for (size_t i = 1; i < interpolatedValues.size(); ++i) {
        size_t colorIndex = static_cast<size_t>(colorValues[i-1]);
        
        segmentX[colorIndex].push_back(interpolatedTimes[i-1]);
        segmentY[colorIndex].push_back(interpolatedValues[i-1]);
        
        segmentX[colorIndex].push_back(interpolatedTimes[i]);
        segmentY[colorIndex].push_back(interpolatedValues[i]);
        
        segmentX[colorIndex].push_back(Chart::NoValue);
        segmentY[colorIndex].push_back(Chart::NoValue);
    }
    
    // Add segments to their respective layers
    if (!segmentX[0].empty()) {
        DoubleArray x = ChartDataManager::vectorToDoubleArray(segmentX[0]);
        DoubleArray y = ChartDataManager::vectorToDoubleArray(segmentY[0]);
        constantLayer->setXData(x);
        DataSet* constDataSet = constantLayer->addDataSet(y, constColor, "Constant");
        constantLayer->setLineWidth(2);
    }
    
    if (!segmentX[1].empty()) {
        DoubleArray x = ChartDataManager::vectorToDoubleArray(segmentX[1]);
        DoubleArray y = ChartDataManager::vectorToDoubleArray(segmentY[1]);
        upLayer->setXData(x);
        DataSet* upDataSet = upLayer->addDataSet(y, upColor, "Up");
        upLayer->setLineWidth(5);
    }
    
    if (!segmentX[2].empty()) {
        DoubleArray x = ChartDataManager::vectorToDoubleArray(segmentX[2]);
        DoubleArray y = ChartDataManager::vectorToDoubleArray(segmentY[2]);
        downLayer->setXData(x);
        DataSet* downDataSet = downLayer->addDataSet(y, downColor, "Down");
        downLayer->setLineWidth(5);
    }
    
    // Add a point at the end of the curve to mark the current value
    if (!interpolatedValues.empty()) {
        std::vector<double> lastPointX = {(double)(interpolatedTimes.size() - 1)};
        std::vector<double> lastPointY = {interpolatedValues.back()};
        
        DoubleArray xPoint = ChartDataManager::vectorToDoubleArray(lastPointX);
        DoubleArray yPoint = ChartDataManager::vectorToDoubleArray(lastPointY);
        
        ScatterLayer* endPoint = equityChart->addScatterLayer(xPoint, yPoint, 
                                                            "Current", Chart::CircleShape, 7, 
                                                            0x000000, 0x000000);
        endPoint->moveFront();
    }
}

void ChartRenderer::addTradeMarkers(XYChart *mainChart, 
                                  const DoubleArray &timestamps,
                                  const ChartDataManager& dataManager,
                                  const chart::AggregationInfo& aggregationInfo)
{
    const std::vector<be::TradeData>& trades = dataManager.getTrades();
    int startIndex = static_cast<int>(aggregationInfo.startIndex);
    chart::AggregationLevel level = aggregationInfo.level;

    if (trades.empty() 
        || !dataManager.hasRawData()
        || !mainChart)
        return;

    // TODO maybe provide trades reference to the following methods to avoid them calling dataManager.getTrades()
    // Select display method based on aggregation level
    if (level == chart::AggregationLevel::Raw) {
        addRawTradeMarkers(mainChart, timestamps, dataManager, aggregationInfo);
    } else {
        addAggregatedTradeMarkers(mainChart, timestamps, dataManager, aggregationInfo);
    }
}

void ChartRenderer::addRawTradeMarkers(XYChart *mainChart, 
                                     const DoubleArray &timestamps,
                                     const ChartDataManager& dataManager,
                                     const chart::AggregationInfo& aggregationInfo)
{
    const std::vector<be::TradeData>& trades = dataManager.getTrades();
    size_t startIndex = aggregationInfo.startIndex;

    // Get the main chart
    if (!mainChart)
        return;

    // Constants for colors by result
    const int COLOR_TP = 0x00AA00;    // Green
    const int COLOR_SL = 0xCC0000;   // Red
    const int COLOR_BE = 0x0000CC; // Blue
    const int COLOR_NEUTRAL = 0x000000; // Black
    const int COLOR_UNKNOWN = 0x808080; // Gray

    // Square markers for exact position
    std::vector<std::pair<double, double>> entryMarkers;
    std::vector<std::pair<double, double>> exitMarkers;
    
    // Entry arrows (based on trade direction)
    std::vector<std::pair<double, double>> entryLongArrows;  // Buys (green arrow up)
    std::vector<std::pair<double, double>> entryShortArrows; // Sells (red arrow down)
    
    // Exit arrows (based on result and direction)
    std::vector<std::pair<double, double>> exitLongTPArrows;    // Long exit winning
    std::vector<std::pair<double, double>> exitLongSLArrows;   // Long exit losing
    std::vector<std::pair<double, double>> exitLongBEArrows;   // Long exit break-even
    std::vector<std::pair<double, double>> exitLongNeutralArrows; // Long exit neutral
    std::vector<std::pair<double, double>> exitShortTPArrows;   // Short exit winning
    std::vector<std::pair<double, double>> exitShortSLArrows;  // Short exit losing
    std::vector<std::pair<double, double>> exitShortBEArrows;  // Short exit break-even
    std::vector<std::pair<double, double>> exitShortNeutralArrows; // Short exit neutral
    
    // Preallocation
    size_t estimatedMarkers = std::min(size_t(100), trades.size());
    entryMarkers.reserve(estimatedMarkers);
    exitMarkers.reserve(estimatedMarkers);
    entryLongArrows.reserve(estimatedMarkers);
    entryShortArrows.reserve(estimatedMarkers);

    exitLongTPArrows.reserve(estimatedMarkers);
    exitLongSLArrows.reserve(estimatedMarkers);
    exitLongBEArrows.reserve(estimatedMarkers);
    exitLongNeutralArrows.reserve(estimatedMarkers/3);

    exitShortTPArrows.reserve(estimatedMarkers);
    exitShortSLArrows.reserve(estimatedMarkers);
    exitShortBEArrows.reserve(estimatedMarkers);
    exitShortNeutralArrows.reserve(estimatedMarkers/3);

    std::vector<TPSLBESegment> tpslbeSegments;
    tpslbeSegments.reserve(estimatedMarkers * 3);

    for (const auto& trade : trades) {
        bool isLong = trade.side == be::OrderSide::BUY;
        
        // Determine trade result
        be::CloseReason closeReason = trade.closeReason;

        // Indices for entry and exit
        size_t entryBarIndex = trade.entryBar;
        size_t exitBarIndex = trade.hasBeenClosed() ? trade.exitBar : 0;

        // Check if the trade is visible in the window (at least entry or exit visible)
        size_t length = static_cast<size_t>(timestamps.len);
        bool entryVisible = (entryBarIndex >= startIndex && entryBarIndex < startIndex + length);
        bool exitVisible = trade.hasBeenClosed() && (exitBarIndex >= startIndex && exitBarIndex < startIndex + length);

        // If neither entry nor exit is visible, skip this trade
        if (!entryVisible && !exitVisible) continue;


        if (entryVisible) {
            double relativeIndex = static_cast<double>(entryBarIndex - startIndex);
            
            // Square marker for entry position
            entryMarkers.push_back({relativeIndex, trade.entryPrice});

            const auto& data = dataManager.getAggregatedData(chart::AggregationLevel::Raw);
            if (entryBarIndex < data.timestamps.size()) {

                if (isLong) {
                    // Buy: upward arrow BELOW the candle
                    double arrowY = data.low[entryBarIndex];
                    entryLongArrows.push_back({relativeIndex, arrowY});
                } else {
                    // Sell: downward arrow ABOVE the candle
                    double arrowY = data.high[entryBarIndex];
                    entryShortArrows.push_back({relativeIndex, arrowY});
                }
            }
        }

        // Add TP/SL segments (unchanged)
        if (trade.hasBeenClosed()) {
            // Calculate relative indices as for pivot points
            // constraining them to visible window limits
            // double relativeEntryIndex = static_cast<double>(std::max(entryBarIndex - startIndex, static_cast<size_t>(0)));
            double relativeEntryIndex = entryBarIndex > startIndex ? static_cast<double>(entryBarIndex - startIndex) : 0.0;
            double relativeExitIndex = static_cast<double>(std::min(exitBarIndex - startIndex, aggregationInfo.pointCount - 1));
        
            int color;
            if (closeReason == be::CloseReason::TakeProfit) color = COLOR_TP;
            else if (closeReason == be::CloseReason::StopLoss) color = COLOR_SL;
            else if (closeReason == be::CloseReason::BreakEven) color = COLOR_BE;
            else if (closeReason == be::CloseReason::ManualClose) color = COLOR_NEUTRAL;
            else color = COLOR_UNKNOWN;

            double tpValue = trade.tpPrice;
            if (tpValue > 0) {
                tpslbeSegments.push_back({
                    relativeEntryIndex, relativeExitIndex, 
                    tpValue, TPSLBEType::TakeProfit,
                    color
                });
            }

            double slValue = (trade.initialSlPrice > 0) ? trade.initialSlPrice : trade.lastSlPrice;
            if (slValue > 0) {
                tpslbeSegments.push_back({
                    relativeEntryIndex, relativeExitIndex,
                    slValue, TPSLBEType::StopLoss,
                    color
                });
            }

            double beValue = trade.breakEvenTriggerPrice;
            if (beValue > 0) {
                tpslbeSegments.push_back({
                    relativeEntryIndex, relativeExitIndex,
                    beValue, TPSLBEType::BreakEven,
                    color
                });
            }

            if (exitVisible) {
                double relativeExitIndex = static_cast<double>(exitBarIndex - startIndex);
                
                // Square marker for exit position
                exitMarkers.push_back({relativeExitIndex, trade.exitPrice});

                const auto& data = dataManager.getAggregatedData(chart::AggregationLevel::Raw);

                if (exitBarIndex < static_cast<int>(data.timestamps.size())) {
                    
                    if (isLong) {
                        // Long exit: downward arrow ABOVE the candle
                        double arrowY = data.high[exitBarIndex];

                        if (closeReason == be::CloseReason::TakeProfit)
                            exitLongTPArrows.push_back({relativeExitIndex, arrowY});
                        else if (closeReason == be::CloseReason::StopLoss)
                            exitLongSLArrows.push_back({relativeExitIndex, arrowY});
                        else if (closeReason == be::CloseReason::BreakEven)
                            exitLongBEArrows.push_back({relativeExitIndex, arrowY});
                        else
                            exitLongNeutralArrows.push_back({relativeExitIndex, arrowY});
                    } else {
                        // Short exit: upward arrow BELOW the candle
                        double arrowY = data.low[exitBarIndex];

                        if (closeReason == be::CloseReason::TakeProfit)
                            exitShortTPArrows.push_back({relativeExitIndex, arrowY});
                        else if (closeReason == be::CloseReason::StopLoss)
                            exitShortSLArrows.push_back({relativeExitIndex, arrowY});
                        else if (closeReason == be::CloseReason::BreakEven)
                            exitShortBEArrows.push_back({relativeExitIndex, arrowY});
                        else
                            exitShortNeutralArrows.push_back({relativeExitIndex, arrowY});
                    }
                }
            }
        }
    }

    // Add square markers for exact positions
    addMarkers(mainChart, entryMarkers, "Entry", Chart::SquareSymbol, 7, 0x000000);
    addMarkers(mainChart, exitMarkers, "Exit", Chart::SquareSymbol, 7, 0x000000);
    addTPSLSegments(mainChart, tpslbeSegments);
    
    // Symbol sizes
    int symbolSize = (aggregationInfo.level == chart::AggregationLevel::Raw) ? 11 : 9;
    
    // Entry arrows
    if (!entryLongArrows.empty())
        addMarkers(mainChart, entryLongArrows, "Long Entry", Chart::ArrowShape(0, 1, 0.4, 0.4), symbolSize, COLOR_TP, 0, 20); // Green arrow up below the candle
    if (!entryShortArrows.empty())
        addMarkers(mainChart, entryShortArrows, "Short Entry", Chart::ArrowShape(180, 1, 0.4, 0.4), symbolSize, COLOR_SL, 0, -20); // Red arrow down above
    
    // Long exit arrows
    if (!exitLongTPArrows.empty())
        addMarkers(mainChart, exitLongTPArrows, "Long Exit Win", Chart::ArrowShape(180, 1, 0.4, 0.4), symbolSize, COLOR_TP, 0, -20); // Green arrow down above
    if (!exitLongSLArrows.empty())
        addMarkers(mainChart, exitLongSLArrows, "Long Exit Loss", Chart::ArrowShape(180, 1, 0.4, 0.4), symbolSize, COLOR_SL, 0, -20); // Red arrow down above
    if (!exitLongBEArrows.empty())
        addMarkers(mainChart, exitLongBEArrows, "Long Exit Break Even", Chart::ArrowShape(180, 1, 0.4, 0.4), symbolSize, COLOR_BE, 0, -20); // Blue arrow down above
    if (!exitLongNeutralArrows.empty())
        addMarkers(mainChart, exitLongNeutralArrows, "Long Exit Neutral", Chart::ArrowShape(180, 1, 0.4, 0.4), symbolSize, COLOR_NEUTRAL, 0, -20); // Black arrow down above
    
    // Short exit arrows
    if (!exitShortTPArrows.empty())
        addMarkers(mainChart, exitShortTPArrows, "Short Exit Win", Chart::ArrowShape(0, 1, 0.4, 0.4), symbolSize, COLOR_TP, 0, 20); // Green arrow up below the candle
    if (!exitShortSLArrows.empty())
        addMarkers(mainChart, exitShortSLArrows, "Short Exit Loss", Chart::ArrowShape(0, 1, 0.4, 0.4), symbolSize, COLOR_SL, 0, 20); // Red arrow up below the candle
    if (!exitShortBEArrows.empty())
        addMarkers(mainChart, exitShortBEArrows, "Short Exit Break Even", Chart::ArrowShape(0, 1, 0.4, 0.4), symbolSize, COLOR_BE, 0, 20); // Blue arrow up below the candle
    if (!exitShortNeutralArrows.empty())
        addMarkers(mainChart, exitShortNeutralArrows, "Short Exit Neutral", Chart::ArrowShape(0, 1, 0.4, 0.4), symbolSize, COLOR_NEUTRAL, 0, 20); // Black arrow up below the candle
}

void ChartRenderer::addAggregatedTradeMarkers(XYChart *mainChart, 
                                            const DoubleArray &timestamps,
                                            const ChartDataManager& dataManager,
                                            const chart::AggregationInfo& aggregationInfo)
{
    const std::vector<be::TradeData>& trades = dataManager.getTrades();
    size_t startIndex = aggregationInfo.startIndex;
    size_t pointCount = aggregationInfo.pointCount;
    chart::AggregationLevel level = aggregationInfo.level;

    // Get the main chart
    if (!mainChart)
        return;

    // Colors for arrows
    const int COLOR_LONG = 0x00AA00;    // Green for long entries
    const int COLOR_SHORT = 0xCC0000;   // Red for short entries

    // Fixed window size (number of candles per bucket)
    const size_t windowSize = std::max(static_cast<size_t>(5), pointCount / 40);

    // Calculate number of windows/buckets needed
    size_t numWindows = (pointCount + windowSize - 1) / windowSize; // Round up

    // Arrays to store counts per window
    std::vector<size_t> longCountByWindow(numWindows, 0);
    std::vector<size_t> shortCountByWindow(numWindows, 0);
    
    // Count trades per fixed window
    for (size_t i = 0; i < trades.size(); ++i) {
        const auto& trade = trades[i];
        bool isLong = trade.side == be::OrderSide::BUY;
        
        // Retrieve aggregated index for this trade
        std::optional<std::pair<size_t, size_t>> aggregatedIndices = dataManager.getTradeAggregatedIndices(i, level);

        // If no aggregated indices, continue to next trade
        if (!aggregatedIndices) continue;

        // Extract aggregated entry index
        size_t entryIndex = aggregatedIndices->first;
        
        // Check if entry is in displayed range
        if (entryIndex < startIndex || entryIndex >= startIndex + pointCount)
            continue;
            
        // Compute relative index and determine corresponding window
        int relativeIndex = static_cast<int>(entryIndex - startIndex);
        int windowIndex = relativeIndex / static_cast<int>(windowSize);

        // Ensure index is valid (safety)
        if (windowIndex >= 0 && windowIndex < static_cast<int>(numWindows)) {
            // Increment counter for this window
            if (isLong) {
                longCountByWindow[windowIndex]++;
            } else {
                shortCountByWindow[windowIndex]++;
            }
        }
    }
    
    // Create markers for each window with long trades
    std::vector<std::pair<double, double>> longEntryMarkers;
    std::vector<int> longEntryCounts;
    
    for (int i = 0; i < static_cast<int>(numWindows); i++) {
        if (longCountByWindow[i] > 0) {
            // Compute middle index of the window
            int windowStartIndex = i * static_cast<int>(windowSize);
            int windowEndIndex = std::min(windowStartIndex + static_cast<int>(windowSize) - 1, static_cast<int>(pointCount) - 1);
            int midIndex = (windowStartIndex + windowEndIndex) / 2;
            
            // Determine y position (bottom of candle)
            double y = 0;
            
            // In aggregated mode, use low price of the aggregated candle in middle of the window
            const auto& aggregatedData = dataManager.getAggregatedData(level);
            int midRealIndex = static_cast<int>(startIndex) + midIndex;
            if (midRealIndex < static_cast<int>(aggregatedData.low.size())) {
                y = aggregatedData.low[midRealIndex];
            }
            
            if (y > 0) {
                longEntryMarkers.push_back({midIndex, y});
                longEntryCounts.push_back(static_cast<int>(longCountByWindow[static_cast<size_t>(i)]));
            }
        }
    }
    
    // Add arrows for long entries
    if (!longEntryMarkers.empty()) {
        ScatterLayer* layer = addMarkers(mainChart, longEntryMarkers, "Long Entries", 
            Chart::ArrowShape(0, 1, 0.4, 0.4), 
            11, COLOR_LONG, 0, 20);
            
        // Add labels with number of entries
        if (layer) {
            for (size_t i = 0; i < longEntryMarkers.size(); i++) {
                std::string label = std::to_string(longEntryCounts[i]);
                
                // Add a custom label to the marker
                TextBox* countLabel = layer->addCustomDataLabel(0, static_cast<int>(i), 
                                                        label.c_str(), 
                                                        "Arial Bold", 8, 0x000000);
                
                // Configure label appearance
                countLabel->setAlignment(Chart::Bottom);
                countLabel->setPos(countLabel->getLeftX(), countLabel->getTopY() + 25);
                // countLabel->setBackground(0x90FFFFFF, 0x000000);
                // countLabel->setRoundedCorners(3);
                // countLabel->setMargin(3);
                
                // Adjust font size for large numbers
                if (longEntryCounts[i] > 99)
                    countLabel->setFontStyle("Arial Bold", 10);
            }
        }
    }

    // Create markers for each window with short trades
    std::vector<std::pair<double, double>> shortEntryMarkers;
    std::vector<int> shortEntryCounts;
    
    for (int i = 0; i < static_cast<int>(numWindows); i++) {
        if (shortCountByWindow[i] > 0) {
            // Compute middle index of the window
            int windowStartIndex = i * static_cast<int>(windowSize);
            int windowEndIndex = std::min(windowStartIndex + static_cast<int>(windowSize) - 1, static_cast<int>(pointCount) - 1);
            int midIndex = (windowStartIndex + windowEndIndex) / 2;
            
            // Determine y position (top of candle)
            double y = 0;
            
            // In aggregated mode, use high price of the aggregated candle in middle of the window
            const auto& aggregatedData = dataManager.getAggregatedData(level);
            int midRealIndex = static_cast<int>(startIndex) + midIndex;
            if (midRealIndex < static_cast<int>(aggregatedData.high.size())) {
                y = aggregatedData.high[midRealIndex];
            }
            
            if (y > 0) {
                shortEntryMarkers.push_back({midIndex, y});
                shortEntryCounts.push_back(static_cast<int>(shortCountByWindow[static_cast<size_t>(i)]));
            }
        }
    }
    
    // Add arrows for short entries
    if (!shortEntryMarkers.empty()) {
        ScatterLayer* layer = addMarkers(mainChart, shortEntryMarkers, "Short Entries", 
            Chart::ArrowShape(180, 1, 0.4, 0.4), 
            11, COLOR_SHORT, 0, -20);
            
        // Add labels with number of entries
        if (layer) {
            for (size_t i = 0; i < shortEntryMarkers.size(); i++) {
                std::string label = std::to_string(shortEntryCounts[i]);
                
                // Add a custom label to the marker
                TextBox* countLabel = layer->addCustomDataLabel(0, static_cast<int>(i), 
                                                        label.c_str(), 
                                                        "Arial Bold", 8, 0x000000);
                
                // Configure label appearance
                countLabel->setAlignment(Chart::Top);
                countLabel->setPos(countLabel->getLeftX(), countLabel->getTopY() - 25);
                // countLabel->setBackground(0x90FFFFFF, 0x000000);
                // countLabel->setRoundedCorners(3);
                // countLabel->setMargin(3);
                
                // Adjust font size for large numbers
                if (shortEntryCounts[i] > 99)
                    countLabel->setFontStyle("Arial Bold", 10);
            }
        }
    }
}

void ChartRenderer::addRSIToChart(FinanceChart* chart, 
                                const indicators::RSIInstance& rsi, 
                                const ChartDataManager& dataManager, 
                                const chart::AggregationInfo& aggregationInfo)
{
    size_t startIndex = aggregationInfo.startIndex;
    size_t pointsToShow = aggregationInfo.pointCount;

    // Use aggregated data
    const auto& rsiMap = dataManager.getAggregatedIndicators(aggregationInfo.level).rsiValues;
    auto it = rsiMap.find(rsi.id);
    
    // Check if aggregated data is available
    if (it == rsiMap.end()) return;

    const std::vector<double>& rsiData = it->second;

    if (rsiData.empty() || startIndex >= rsiData.size()) return;

    // Limit the number of points to display
    size_t endIndex = std::min(startIndex + pointsToShow, rsiData.size());
    if (endIndex < startIndex) return;
    size_t actualPoints = endIndex - startIndex;

    // Extract visible RSI data from cache
    DoubleArray rsiArray(&rsiData[startIndex], static_cast<int>(actualPoints));

    // Add the indicator chart
    XYChart* c = chart->addIndicator(rsi.height);
    
    // Configure and add the RSI
    char buffer[1024];
    snprintf(buffer, sizeof(buffer), "RSI (%d)", rsi.period);
    LineLayer* layer = chart->addLineIndicator2(c, rsiArray, rsi.color, buffer);
    layer->setFastLineMode(true);

    // Add thresholds
    chart->addThreshold(c, layer, rsi.overboughtLevel, rsi.upperColor, rsi.oversoldLevel, rsi.lowerColor);

    // Configure Y axis scale
    c->yAxis()->setLinearScale(0, 100);
}

void ChartRenderer::addEMAToChart(FinanceChart* chart, 
                                const indicators::EMAInstance& ema, 
                                const ChartDataManager& dataManager, 
                                const chart::AggregationInfo& aggregationInfo)
{
    // Determine which data source to use
    size_t startIndex = aggregationInfo.startIndex;
    size_t pointsToShow = aggregationInfo.pointCount;

    // Use aggregated data
    const auto& emaMap = dataManager.getAggregatedIndicators(aggregationInfo.level).emaValues;
    auto it = emaMap.find(ema.id);

    // Check if aggregated data is available
    if (it == emaMap.end()) return;

    const std::vector<double>& emaData = it->second;

    if (emaData.empty() || startIndex >= emaData.size()) return;

    // Limit the number of points to display
    size_t endIndex = std::min(startIndex + pointsToShow, emaData.size());
    if (endIndex < startIndex) return;
    size_t actualPoints = endIndex - startIndex;

    // Extract visible EMA data from cache
    DoubleArray emaArray(&emaData[startIndex], static_cast<int>(actualPoints));

    // Configure and add the EMA directly on the main chart
    char buffer[1024];
    snprintf(buffer, sizeof(buffer), "EMA (%d)", ema.period);
    LineLayer* layer = chart->addLineIndicator2((XYChart*)chart->getChart(1), emaArray, ema.color, buffer);
    layer->setFastLineMode(true);
}

void ChartRenderer::addSupertrendToChart(FinanceChart* chart, 
                                      const indicators::SuperTrendInstance& supertrend, 
                                      const ChartDataManager& dataManager, 
                                      const chart::AggregationInfo& aggregationInfo)
{
    size_t startIndex = aggregationInfo.startIndex;
    size_t pointsToShow = aggregationInfo.pointCount;

    const auto& superTrendMap = dataManager.getAggregatedIndicators(aggregationInfo.level).supertrendValues;
    auto it = superTrendMap.find(supertrend.id);

    if (it == superTrendMap.end()) return;

    const std::pair<std::vector<double>, std::vector<int>>& supertrendData = it->second;

    if (supertrendData.first.empty() || startIndex >= supertrendData.first.size())
        return;

    // Limit the number of points to display
    size_t endIndex = std::min(startIndex + pointsToShow, supertrendData.first.size());
    if (endIndex < startIndex) return;
    size_t actualPoints = endIndex - startIndex;


    // Extract visible data from cache
    std::vector<double> upValues(actualPoints, Chart::NoValue);
    std::vector<double> downValues(actualPoints, Chart::NoValue);

    for (size_t i = 0; i < actualPoints; ++i) {
        size_t index = startIndex + i;
        if ((supertrendData.second)[index] == 1) {
            upValues[i] = (supertrendData.first)[index];
        } else if ((supertrendData.second)[index] == -1) {
            downValues[i] = (supertrendData.first)[index];
        }
    }

    // Convert to DoubleArray
    DoubleArray upArray = ChartDataManager::vectorToDoubleArray(upValues);
    DoubleArray downArray = ChartDataManager::vectorToDoubleArray(downValues);

    // Add directly on the main chart
    char buffer[1024];
    snprintf(buffer, sizeof(buffer), "Supertrend (%d, %.1f)", supertrend.period, supertrend.multiplier);
    
    XYChart* mainChart = (XYChart*)chart->getChart(1);
    
    // Uptrend lines (green)
    LineLayer* upLayer = chart->addLineIndicator2(mainChart, upArray, supertrend.upColor, buffer);
    upLayer->setLineWidth(1);
    
    // Downtrend lines (red)
    LineLayer* downLayer = chart->addLineIndicator2(mainChart, downArray, supertrend.downColor, "");
    downLayer->setLineWidth(1);
}


void ChartRenderer::addStochasticToChart(FinanceChart* chart, 
                                       const indicators::StochasticInstance& stochastic, 
                                       const ChartDataManager& dataManager, 
                                       const chart::AggregationInfo& aggregationInfo)
{
    size_t startIndex = aggregationInfo.startIndex;
    size_t pointsToShow = aggregationInfo.pointCount;

    // Use aggregated data
    const auto& stochMap = dataManager.getAggregatedIndicators(aggregationInfo.level).stochasticValues;
    auto it = stochMap.find(stochastic.id);
    
    // Check if aggregated data is available and valid
    if (it == stochMap.end()) return;

    const std::pair<std::vector<double>, std::vector<double>>& stochData = it->second;

    if (stochData.first.empty() || startIndex >= stochData.first.size()) return;

    // Limit the number of points to display
    size_t endIndex = std::min(startIndex + pointsToShow, stochData.first.size());
    if (endIndex < startIndex) return;
    size_t actualPoints = endIndex - startIndex;

    // Extract visible Stochastic data from cache
    DoubleArray kArray(&stochData.first[startIndex], static_cast<int>(actualPoints));
    DoubleArray dArray(&stochData.second[startIndex], static_cast<int>(actualPoints));

    // Add the indicator chart
    XYChart* c = chart->addIndicator(stochastic.height);
    
    // Configure and add %K and %D lines
    char buffer[1024];
    snprintf(buffer, sizeof(buffer), "Stochastic %%K (%d, %d, %d)", 
             stochastic.fastKPeriod, stochastic.slowKPeriod, stochastic.slowDPeriod);

    LineLayer* kLayer = chart->addLineIndicator2(c, kArray, stochastic.kColor, buffer);
    kLayer->setFastLineMode(true);
    
    snprintf(buffer, sizeof(buffer), "%%D (%d)", stochastic.slowDPeriod);
    LineLayer* dLayer = chart->addLineIndicator2(c, dArray, stochastic.dColor, buffer);
    dLayer->setFastLineMode(true);
    
    // Configure Y axis scale
    c->yAxis()->setLinearScale(0, 100);
    
    // Add thresholds for overbought and oversold levels
    Mark* overboughtMark = c->yAxis()->addMark(stochastic.overboughtLevel, 0xff6666, 
                                              std::to_string(stochastic.overboughtLevel).c_str());
    Mark* oversoldMark = c->yAxis()->addMark(stochastic.oversoldLevel, 0x6666ff, 
                                           std::to_string(stochastic.oversoldLevel).c_str());

    c->addInterLineLayer(kLayer->getLine(), overboughtMark->getLine(), 0xCCff0000, Chart::Transparent);
    c->addInterLineLayer(kLayer->getLine(), oversoldMark->getLine(), Chart::Transparent, 0xCC0000ff);
}

void ChartRenderer::addATRToChart(FinanceChart* chart, 
                                const indicators::ATRInstance& atr, 
                                const ChartDataManager& dataManager, 
                                const chart::AggregationInfo& aggregationInfo)
{
    size_t startIndex = aggregationInfo.startIndex;
    size_t pointsToShow = aggregationInfo.pointCount;

    // Use aggregated data
    const auto& atrMap = dataManager.getAggregatedIndicators(aggregationInfo.level).atrValues;
    auto it = atrMap.find(atr.id);
    
    // Check if aggregated data is available and valid
    if (it == atrMap.end()) return;

    const std::vector<double>& atrData = it->second;

    if (atrData.empty() || startIndex >= atrData.size()) return;

    // Limit the number of points to display
    size_t endIndex = std::min(startIndex + pointsToShow, atrData.size());
    if (endIndex < startIndex) return;
    size_t actualPoints = endIndex - startIndex;

    // Extract visible ATR data from cache
    DoubleArray atrArray(&atrData[startIndex], static_cast<int>(actualPoints));

    // Add the indicator chart
    XYChart* c = chart->addIndicator(atr.height);
    
    // Configure and add ATR
    char buffer[1024];
    snprintf(buffer, sizeof(buffer), "ATR (%d)", atr.period);
    LineLayer* layer = chart->addLineIndicator2(c, atrArray, atr.color, buffer);
    layer->setFastLineMode(true);

    // Configure Y axis scale
    double maxATR = *std::max_element(atrData.begin() + startIndex, atrData.begin() + endIndex);
    c->yAxis()->setLinearScale(0, maxATR * 1.1); // 10% top margin
}

void ChartRenderer::addCCIToChart(FinanceChart* chart, 
                                const indicators::CCIInstance& cci, 
                                const ChartDataManager& dataManager, 
                                const chart::AggregationInfo& aggregationInfo)
{
    size_t startIndex = aggregationInfo.startIndex;
    size_t pointsToShow = aggregationInfo.pointCount;

    // Use aggregated data
    const auto& cciMap = dataManager.getAggregatedIndicators(aggregationInfo.level).cciValues;
    auto it = cciMap.find(cci.id);
    
    // Check if aggregated data is available
    if (it == cciMap.end()) return;

    const std::vector<double>& cciData = it->second;

    if (cciData.empty() || startIndex >= cciData.size()) return;

    // Limit the number of points to display
    size_t endIndex = std::min(startIndex + pointsToShow, cciData.size());
    if (endIndex < startIndex) return;
    size_t actualPoints = endIndex - startIndex;

    // Extract visible CCI data from cache
    DoubleArray cciArray(&cciData[startIndex], static_cast<int>(actualPoints));

    // Add the indicator chart
    XYChart* c = chart->addIndicator(cci.height);
    
    // Configure and add CCI
    char buffer[1024];
    snprintf(buffer, sizeof(buffer), "CCI (%d)", cci.period);
    LineLayer* layer = chart->addLineIndicator2(c, cciArray, cci.color, buffer);
    layer->setFastLineMode(true);

    // Add thresholds (horizontal lines for +100 and -100)
    chart->addThreshold(c, layer, cci.upperLevel, cci.upperColor, cci.lowerLevel, cci.lowerColor);

    // Configure Y axis scale with margins
    double minCCI = *std::min_element(cciData.begin() + startIndex, cciData.begin() + endIndex);
    double maxCCI = *std::max_element(cciData.begin() + startIndex, cciData.begin() + endIndex);
    
    // Extend limits to include standard levels (-200 to +200 with margin)
    double yMin = std::min(minCCI, static_cast<double>(cci.lowerLevel)) * 1.2;
    double yMax = std::max(maxCCI, static_cast<double>(cci.upperLevel)) * 1.2;
    c->yAxis()->setLinearScale(yMin, yMax);
}

void ChartRenderer::addMACDToChart(FinanceChart* finance, const indicators::MACDInstance &macd, const ChartDataManager &dataManager, const chart::AggregationInfo &aggregationInfo){
    if (!finance) return;

    size_t startIndex = aggregationInfo.startIndex;
    size_t pointsToShow = aggregationInfo.pointCount;

    // Retrieve aggregated MACD data
    const auto& macdMap = dataManager.getAggregatedIndicators(aggregationInfo.level).macdValues;
    auto it = macdMap.find(macd.id);
    if (it == macdMap.end()) return;

    // Expected format: tuple<macdLine, signalLine, histogram>
    const auto& macdTuple = it->second;
    const std::vector<double>& macdLine = std::get<0>(macdTuple);
    const std::vector<double>& signalLine = std::get<1>(macdTuple);
    const std::vector<double>& histLine   = std::get<2>(macdTuple);

    if (macdLine.empty() || signalLine.empty() || histLine.empty()) return;
    size_t available = std::min({ macdLine.size(), signalLine.size(), histLine.size() });
    if (startIndex >= available) return;

    size_t endIndex = std::min(startIndex + pointsToShow, available);
    if (endIndex <= startIndex) return;
    size_t actualPoints = endIndex - startIndex;

    // Extract visible portions
    DoubleArray macdArr(&macdLine[startIndex], static_cast<int>(actualPoints));
    DoubleArray signalArr(&signalLine[startIndex], static_cast<int>(actualPoints));

    // Build histograms separated for positive/negative
    std::vector<double> posHist(actualPoints, Chart::NoValue);
    std::vector<double> negHist(actualPoints, Chart::NoValue);
    for (size_t i = 0; i < actualPoints; ++i) {
        double v = histLine[startIndex + i];
        if (v > 0) posHist[i] = v;
        else if (v < 0) negHist[i] = v;
        // zero will be left as NoValue (or could be shown on either)
    }

    DoubleArray posHistArr = ChartDataManager::vectorToDoubleArray(posHist);
    DoubleArray negHistArr = ChartDataManager::vectorToDoubleArray(negHist);

    // Add MACD indicator area
    XYChart* c = finance->addIndicator(macd.height);
    if (!c) return;

    // Histogram: positive green, negative red (or use histogramColor for both)
    int positiveColor = 0x00aa00; // Green
    int negativeColor = 0xaa0000; // Red
    
    BarLayer* posLayer = c->addBarLayer(posHistArr, positiveColor);
    posLayer->setBarGap(Chart::TouchBar);
    posLayer->setBorderColor(Chart::Transparent);
    posLayer->set3D(0);

    BarLayer* negLayer = c->addBarLayer(negHistArr, negativeColor);
    negLayer->setBarGap(Chart::TouchBar);
    negLayer->setBorderColor(Chart::Transparent);
    negLayer->set3D(0);

    // MACD and Signal lines
    char labelBuf[128];
    snprintf(labelBuf, sizeof(labelBuf), "MACD (%d,%d,%d)", macd.fastPeriod, macd.slowPeriod, macd.signalPeriod);
    LineLayer* macdLineLayer = finance->addLineIndicator2(c, macdArr, macd.macdColor, labelBuf);
    if (macdLineLayer) macdLineLayer->setFastLineMode(true);

    snprintf(labelBuf, sizeof(labelBuf), "Signal (%d)", macd.signalPeriod);
    LineLayer* signalLineLayer = finance->addLineIndicator2(c, signalArr, macd.signalColor, labelBuf);
    if (signalLineLayer) signalLineLayer->setFastLineMode(true);

    // Add a mark/line at 0 as a reference
    Mark* zeroMark = c->yAxis()->addMark(0.0, 0x000000, "");
    if (zeroMark) {
        zeroMark->setLineWidth(1);
        zeroMark->setMarkColor(c->dashLineColor(0x000000, Chart::DashLine), 0xffffff);
    }

    // Let ChartDir handle the Y scale (optional: small margin)
    // Adjust if needed: c->yAxis()->setMargin(...);
}

void ChartRenderer::addBBToChart(FinanceChart *chart, const indicators::BBInstance &bb, const ChartDataManager &dataManager, const chart::AggregationInfo &aggregationInfo){
    size_t startIndex = aggregationInfo.startIndex;
    size_t pointsToShow = aggregationInfo.pointCount;

    // Use aggregated data
    const auto& bbMap = dataManager.getAggregatedIndicators(aggregationInfo.level).bbValues;
    auto it = bbMap.find(bb.id);
    
    // Check if aggregated data is available and valid
    if (it == bbMap.end()) return;

    const std::tuple<std::vector<double>, std::vector<double>, std::vector<double>>& bbData = it->second;

    const std::vector<double>& middleBand = std::get<0>(bbData);
    const std::vector<double>& upperBand = std::get<1>(bbData);
    const std::vector<double>& lowerBand = std::get<2>(bbData);

    if (middleBand.empty() || upperBand.empty() || lowerBand.empty() || startIndex >= middleBand.size()) return;

    // Limit the number of points to display
    size_t endIndex = std::min(startIndex + pointsToShow, middleBand.size());
    if (endIndex < startIndex) return;
    size_t actualPoints = endIndex - startIndex;

    // Extract visible BB data from cache
    DoubleArray middleArray(&middleBand[startIndex], static_cast<int>(actualPoints));
    DoubleArray upperArray(&upperBand[startIndex], static_cast<int>(actualPoints));
    DoubleArray lowerArray(&lowerBand[startIndex], static_cast<int>(actualPoints));

    // Add Bollinger bands directly on the main chart
    XYChart* mainChart = (XYChart*)chart->getChart(1);
    
    char buffer[1024];
    
    snprintf(buffer, sizeof(buffer), "BB Middle (%d)", bb.period);
    LineLayer* middleLayer = chart->addLineIndicator2(mainChart, middleArray, bb.middleBandColor, buffer);
    if (middleLayer) middleLayer->setFastLineMode(true);
    
    snprintf(buffer, sizeof(buffer), "BB Upper (%d)", bb.period);
    LineLayer* upperLayer = chart->addLineIndicator2(mainChart, upperArray, bb.upperBandColor, buffer);
    if (upperLayer) upperLayer->setFastLineMode(true);
    
    snprintf(buffer, sizeof(buffer), "BB Lower (%d)", bb.period);
    LineLayer* lowerLayer = chart->addLineIndicator2(mainChart, lowerArray, bb.lowerBandColor, buffer);
    if (lowerLayer) lowerLayer->setFastLineMode(true);
}

void ChartRenderer::addPivotPointsToChart(XYChart *mainChart, const indicators::PivotPointsInstance &pivotPoints, const ChartDataManager &dataManager, const chart::AggregationInfo &aggregationInfo)
{
    // Retrieve pivot points data from cache
    size_t startIndex = aggregationInfo.startIndex;
    size_t endIndex = startIndex + aggregationInfo.pointCount - 1;
    chart::AggregationLevel currentLevel = aggregationInfo.level;

    const std::map<int, std::vector<indicators::PivotPointsInstance::PivotPeriod>>& pivotPeriodsMap = dataManager.getPivotPeriods();
    auto it = pivotPeriodsMap.find(pivotPoints.id);
    if (it == pivotPeriodsMap.end())
        return; // No data for this pivot points ID
    
    // Get the vector of pivot periods
    const std::vector<indicators::PivotPointsInstance::PivotPeriod>& pivotPeriods = it->second;

    // Iterate all pivot periods
    for (const auto& period : pivotPeriods) {
        // Retrieve aggregated indices for the current aggregation level
        const std::pair<size_t, size_t>& indices = period.indices[static_cast<size_t>(currentLevel)];

        size_t aggStartIndex = indices.first;
        size_t aggEndIndex = indices.second;

        // Check if the period is visible in the current range
        if (aggEndIndex < startIndex || aggStartIndex > endIndex) {
            continue;  // Period out of visible range
        }
        
        // Compute relative indices for display
        size_t relativeStart = aggStartIndex < startIndex ? 0 : aggStartIndex - startIndex;
        size_t relativeEnd = std::min(aggEndIndex - startIndex, aggregationInfo.pointCount);
        
        // For each pivot level configured
        for (size_t levelType = 0; levelType < static_cast<size_t>(indicators::PivotPointsInstance::LevelType::Count); ++levelType) {
            // Check if this level should be displayed
            const auto& style = pivotPoints.levelStyles[levelType];
            if (!style.visible) continue;
            
            // Retrieve the level value for this period
            double value = period.levelValues[levelType];
            
            // Skip segments with invalid or zero values
            if (value == 0 || std::isnan(value)) continue;
            
            // Create a vector of points to draw the horizontal line
            std::vector<double> xData = {static_cast<double>(relativeStart), static_cast<double>(relativeEnd)};
            std::vector<double> yData = {value, value};
            
            // Convert to DoubleArray for ChartDir
            DoubleArray xArray = ChartDataManager::vectorToDoubleArray(xData);
            DoubleArray yArray = ChartDataManager::vectorToDoubleArray(yData);
            
            // Create a line layer
            LineLayer* layer = mainChart->addLineLayer(yArray, style.color);
            layer->setXData(xArray);
            layer->setLineWidth(style.thickness);

            // Set line style based on LineStyle
            int dashPatternColor;
            switch (style.lineStyle) {
                case indicators::PivotPointsInstance::LineStyle::Dash:
                    dashPatternColor = mainChart->dashLineColor(style.color, Chart::DashLine);
                    break;
                case indicators::PivotPointsInstance::LineStyle::Dot:
                    dashPatternColor = mainChart->dashLineColor(style.color, Chart::DotLine);
                    break;
                case indicators::PivotPointsInstance::LineStyle::DotDash:
                    dashPatternColor = mainChart->dashLineColor(style.color, Chart::DotDashLine);
                    break;
                case indicators::PivotPointsInstance::LineStyle::AltDash:
                    dashPatternColor = mainChart->dashLineColor(style.color, Chart::AltDashLine);
                    break;
                default: // LineStyle::Solid
                    dashPatternColor = style.color;
                    break;
            }

            // Apply the color to the DataSet
            DataSet* dataSet = layer->getDataSet(0);
            if (dataSet)
                dataSet->setDataColor(dashPatternColor);
            
            // NEW CODE: Add a custom label at the end point of the segment if requested
            if (pivotPoints.showLabels) {

                QString periodSuffix;
                switch (pivotPoints.periodType) {
                    case indicators::PivotPeriodType::FourHour: periodSuffix = "4H"; break;
                    case indicators::PivotPeriodType::Daily: periodSuffix = "D"; break;
                    case indicators::PivotPeriodType::Weekly: periodSuffix = "W"; break;
                    case indicators::PivotPeriodType::Monthly: periodSuffix = "M"; break;
                }

                // Format the label according to specified format or default format
                QString labelText;
                if (!style.labelFormat.isEmpty())
                    labelText = style.labelFormat.arg(periodSuffix);
                
                // Add a custom label to the end point of the segment (index 1)
                TextBox* label = layer->addCustomDataLabel(0, 1, labelText.toStdString().c_str(), "Arial Bold", 8, style.color);
                
                // Configure label appearance
                label->setAlignment(Chart::Right);
                label->setPos(label->getLeftX() - 5, label->getTopY() - 8); // Offset right and up
                label->setBackground(Chart::Transparent);
                label->setMargin(3);               // Inner margin
            }
        }
    }
}

void ChartRenderer::addUserMarkers(XYChart* mainChart, 
                                 const std::vector<chart::ChartMarker>& markers,
                                 const chart::AggregationInfo& aggregationInfo) {
    if (markers.empty() || !mainChart) {
        return;
    }

    size_t startIndex = aggregationInfo.startIndex;
    size_t length = aggregationInfo.pointCount;
    
    // Group markers by (shape type + color) to keep one layer per style.
    std::map<int, std::vector<std::pair<double, double>>> checkMarkersByColor;
    std::map<int, std::vector<std::pair<double, double>>> errorMarkersByColor;

    const int defaultCheckColor = 0x00BB00;
    const int defaultErrorColor = 0xBB0000;
    
    // For each marker, check if visible and convert to relative index
    for (const auto& marker : markers) {
        // Check if the marker is in the visible window
        bool isVisible = (marker.barIndex >= startIndex && 
                         marker.barIndex < startIndex + length);
        
        if (!isVisible) continue;
        
        // Convert absolute index to relative index (same as trades)
        double relativeIndex = static_cast<double>(marker.barIndex - startIndex);
        
        if (marker.type == chart::MarkerType::Check) {
            const int markerColor = (marker.color >= 0 && marker.color <= 0xFFFFFF) ? marker.color : defaultCheckColor;
            checkMarkersByColor[markerColor].push_back({relativeIndex, marker.price});
        } else if (marker.type == chart::MarkerType::Error) {
            const int markerColor = (marker.color >= 0 && marker.color <= 0xFFFFFF) ? marker.color : defaultErrorColor;
            errorMarkersByColor[markerColor].push_back({relativeIndex, marker.price});
        }
    }
    
    // Add Check markers (default green or custom color)
    for (const auto& [color, points] : checkMarkersByColor) {
        addMarkers(mainChart, points, "Check Markers", 
                  Chart::CircleShape, 16, color);
    }
    
    // Add Error markers (default red or custom color)
    for (const auto& [color, points] : errorMarkersByColor) {
        addMarkers(mainChart, points, "Error Markers", 
                  Chart::Cross2Shape(), 16, color);
    }

}

ScatterLayer* ChartRenderer::addMarkers(XYChart *chart, const std::vector<std::pair<double, double>> &markers,
                               const char *name, int symbolType, int symbolSize, int color, int offsetX, int offsetY)
{
    if (markers.empty()) return nullptr;

    std::vector<double> xValues;
    std::vector<double> yValues;
    xValues.reserve(markers.size());
    yValues.reserve(markers.size());
    
    for (const auto& pair : markers) {
        xValues.push_back(pair.first);
        yValues.push_back(pair.second);
    }

    // Convert to DoubleArray
    DoubleArray xArray = ChartDataManager::vectorToDoubleArray(xValues);
    DoubleArray yArray = ChartDataManager::vectorToDoubleArray(yValues);

    ScatterLayer* layer = chart->addScatterLayer(xArray, yArray, name, symbolType, symbolSize, color);

    if (offsetX != 0 || offsetY != 0)
        layer->getDataSet(0)->setSymbolOffset(offsetX, offsetY);

    layer->moveFront();

    return layer;
}

void ChartRenderer::addTPSLSegments(XYChart* chart, const std::vector<TPSLBESegment>& segments)
{
    if (segments.empty()) return;
    
    // Create separate vectors for TP and SL segments
    std::vector<double> tpXData, tpYData;
    std::vector<double> slXData, slYData;
    std::vector<double> beXData, beYData;
    
    // Iterate all segments and separate by type
    for (const auto& segment : segments) {
        std::vector<double>& xData = segment.type == TPSLBEType::TakeProfit ? tpXData : (segment.type == TPSLBEType::StopLoss ? slXData : beXData);
        std::vector<double>& yData = segment.type == TPSLBEType::TakeProfit ? tpYData : (segment.type == TPSLBEType::StopLoss ? slYData : beYData);
        
        // Add start point of horizontal segment
        xData.push_back(segment.startX);
        yData.push_back(segment.level);
        
        // Add end point of horizontal segment
        xData.push_back(segment.endX);
        yData.push_back(segment.level);
        
        // Add a NoValue point to create discontinuity
        xData.push_back(Chart::NoValue);
        yData.push_back(Chart::NoValue);
    }
    
    // Add Take Profit segments
    if (!tpXData.empty()) {
        LineLayer* tpLayer = chart->addLineLayer();
        tpLayer->setLineWidth(1);
        tpLayer->setFastLineMode(true);
        
        DoubleArray tpX = ChartDataManager::vectorToDoubleArray(tpXData);
        DoubleArray tpY = ChartDataManager::vectorToDoubleArray(tpYData);
        
        tpLayer->setXData(tpX);
        tpLayer->addDataSet(tpY, 0x00AA00, "Take Profit");
        tpLayer->moveFront();
    }
    
    // Add Stop Loss segments
    if (!slXData.empty()) {
        LineLayer* slLayer = chart->addLineLayer();
        slLayer->setLineWidth(1);
        slLayer->setFastLineMode(true);
        
        DoubleArray slX = ChartDataManager::vectorToDoubleArray(slXData);
        DoubleArray slY = ChartDataManager::vectorToDoubleArray(slYData);

        slLayer->setXData(slX);
        slLayer->addDataSet(slY, 0xCC0000, "Stop Loss");
        slLayer->moveFront();
    }

    // Add Break Even segments
    if (!beXData.empty()) {
        LineLayer* beLayer = chart->addLineLayer();
        beLayer->setLineWidth(1);
        beLayer->setFastLineMode(true);

        DoubleArray beX = ChartDataManager::vectorToDoubleArray(beXData);
        DoubleArray beY = ChartDataManager::vectorToDoubleArray(beYData);

        beLayer->setXData(beX);
        beLayer->addDataSet(beY, 0x0000CC, "Break Even");
        beLayer->moveFront();
    }
}

void ChartRenderer::drawRuler(MultiChart* m, int startX, int startY, int endX, int endY, DrawArea* d, 
                             const ChartDataManager& dataManager, 
                             const chart::AggregationInfo& aggregationInfo)
{
    // Verify that the chart is valid and has at least one chart
    if (!m || m->getChartCount() == 0) return;
    
    // Get the first XY chart (main chart)
    XYChart* c = (XYChart*)m->getChart(1);
    if (!c) return;

    // Get indices corresponding to cursor positions
    double xIndexStart = c->getNearestXValue(startX);
    double xIndexEnd = c->getNearestXValue(endX);
    double yValueStart = c->getYValue(startY);
    double yValueEnd = c->getYValue(endY);

    // Retrieve real timestamps from ChartDataManager
    double xValueStart, xValueEnd;

    // In aggregated mode, use aggregated data
    const std::vector<double>& timestamps = dataManager.getAggregatedData(aggregationInfo.level).timestamps;
    size_t startIndex = aggregationInfo.startIndex;
    
    // Convert relative indices to absolute indices
    size_t absIndexStart = startIndex + static_cast<size_t>(xIndexStart);
    size_t absIndexEnd = startIndex + static_cast<size_t>(xIndexEnd);

    // Check that indices are valid
    if (absIndexStart < timestamps.size() && absIndexEnd < timestamps.size()) {
        xValueStart = timestamps[absIndexStart];
        xValueEnd = timestamps[absIndexEnd];
    } else {
                             // Invalid indices, use default values
        xValueStart = xIndexStart;
        xValueEnd = xIndexEnd;
    }

    // Get formatted timestamps for display
    const char* startTimeStr = c->xAxis()->getFormattedLabel(xIndexStart, "yyyy-mm-dd hh:nn:ss");
    const char* endTimeStr = c->xAxis()->getFormattedLabel(xIndexEnd, "yyyy-mm-dd hh:nn:ss");

    // Compute time difference in seconds (real timestamps)
    double deltaX = fabs(xValueEnd - xValueStart);
    double deltaY = yValueEnd - yValueStart;

    // Set rectangle color based on deltaY
    int deltaColor = (deltaY < 0) ? 0xFF0000 : 0x008800;
    int alpha = 0xCC;
    int finalColor = (alpha << 24) | deltaColor;

    // Text for deltaX (above the rectangle)
    char bufferX[50];
    
    // Compute real duration in seconds (for TimeCharts)
    int totalSeconds = static_cast<int>(deltaX);
    int hours = totalSeconds / 3600;
    int minutes = (totalSeconds % 3600) / 60;
    int seconds = totalSeconds % 60;
    
    if (hours > 0) {
#ifdef _WIN32
        sprintf_s(bufferX, sizeof(bufferX), "%02dh%02dm%02ds", hours, minutes, seconds);
#else
        snprintf(bufferX, sizeof(bufferX), "%02dh%02dm%02ds", hours, minutes, seconds);
#endif
    } else if (minutes > 0) {
#ifdef _WIN32
        sprintf_s(bufferX, sizeof(bufferX), "%02dm%02ds", minutes, seconds);
#else
        snprintf(bufferX, sizeof(bufferX), "%02dm%02ds", minutes, seconds);
#endif
    } else {
#ifdef _WIN32
        sprintf_s(bufferX, sizeof(bufferX), "%02ds", seconds);
#else
        snprintf(bufferX, sizeof(bufferX), "%02ds", seconds);
#endif
    }
        
    // Text for deltaY (to the right of the rectangle)
    char bufferY[50];
    
    // Compute percentage change avoiding division by zero
    double percentChange = std::numeric_limits<double>::quiet_NaN();
    if (std::isfinite(yValueStart) && std::abs(yValueStart) > 1e-12) {
        percentChange = (deltaY / std::abs(yValueStart)) * 100.0;
    }

    // Format depending on whether we have a valid percentage
    if (std::isnan(percentChange)) {
        snprintf(bufferY, sizeof(bufferY), "%+.2f", deltaY);
    } else {
        snprintf(bufferY, sizeof(bufferY), "%+.2f \n(%.2f%%)", deltaY, percentChange);
    }

    // Draw the rectangle between the two points
    d->rect(startX, startY, endX, endY, deltaColor, finalColor);

    // Position for deltaX text (above the rectangle)
    int textXPosX = (startX + endX) / 2;
    int textXPosY = std::min(startX, endX) - 15;
    
    // Position for deltaY text (to the right of the rectangle)
    int textYPosX = std::max(startX, endX) + 10;
    int textYPosY = (startY + endY) / 2;
    
    // Create and display deltaX text
    TTFText* tForXDelta = d->text(bufferX, "Arial", 12);
    tForXDelta->draw(textXPosX, textXPosY, deltaColor, Chart::Bottom);
    tForXDelta->destroy();
    
    // Create and display deltaY text
    TTFText* tForYDelta = d->text(bufferY, "Arial", 12);
    tForYDelta->draw(textYPosX, textYPosY, deltaColor, Chart::Left);
    tForYDelta->destroy();
}

void ChartRenderer::trackFinance(MultiChart* m, int mouseX, int mouseY, DrawArea* d)
{
    // Verify that the chart is not empty
    if (m->getChartCount() == 0)
        return;
    
    // Get the nearest x value to the mouse
    int xValue = (int)(((XYChart*)m->getChart(0))->getNearestXValue(mouseX));
    
    // Iterate over all XY charts in the MultiChart
    XYChart *c = 0;
    
    for (int i = 0; i < m->getChartCount(); ++i) {
        c = (XYChart*)m->getChart(i);
        
        // Variables for legend entries
        std::ostringstream ohlcLegend;
        std::vector<std::string> legendEntries;
        
        // Iterate all layers to find the highest data point
        for (int j = 0; j < c->getLayerCount(); ++j) {
            Layer* layer = c->getLayerByZ(j);
            int xIndex = layer->getXIndexOf(xValue);
            int dataSetCount = layer->getDataSetCount();
            
            // In a FinanceChart, only layers showing OHLC data may have 4 datasets
            if (dataSetCount == 4) {
                double highValue = layer->getDataSet(0)->getValue(xIndex);
                double lowValue = layer->getDataSet(1)->getValue(xIndex);
                double openValue = layer->getDataSet(2)->getValue(xIndex);
                double closeValue = layer->getDataSet(3)->getValue(xIndex);
                
                if (closeValue != Chart::NoValue) {
                    // Build OHLC legend
                    ohlcLegend << "      <*block*>";
                    ohlcLegend << "Open: " << c->formatValue(openValue, "{value|P4}");
                    ohlcLegend << ", High: " << c->formatValue(highValue, "{value|P4}"); 
                    ohlcLegend << ", Low: " << c->formatValue(lowValue, "{value|P4}"); 
                    ohlcLegend << ", Close: " << c->formatValue(closeValue, "{value|P4}");
                    
                    // Add an arrow and the % change if possible
                    double lastCloseValue = (xIndex > 0) ? 
                        layer->getDataSet(3)->getValue(xIndex - 1) : 
                        Chart::NoValue;

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
            } else {
                // Iterate all datasets in the layer
                for (int k = 0; k < layer->getDataSetCount(); ++k) {
                    DataSet* dataSet = layer->getDataSetByZ(k);
                    
                    std::string name = dataSet->getDataName();
                    double value = dataSet->getValue(xIndex);
                    if ((0 != name.size()) && (value != Chart::NoValue)) {
                        
                        // Extract the unit
                        std::string unitChar;
                        
                        // The indicator name is the part of the name up to the colon
                        int delimiterPosition = (int)name.find(':');
                        if ((int)name.npos != delimiterPosition) {
                            
                            // The unit, if any, is the trailing non-digit character(s)
                            int lastDigitPos = (int)name.find_last_of("0123456789");
                            if (((int)name.npos != lastDigitPos) && (lastDigitPos + 1 < (int)name.size())
                                && (lastDigitPos > delimiterPosition))
                                unitChar = name.substr(lastDigitPos + 1);
                            
                            name.resize(delimiterPosition);
                        }
                        
                        // Special cases handling
                        if (dataSetCount == 2) {
                            // If two datasets, it's probably a range
                            value = layer->getDataSet(0)->getValue(xIndex);
                            double value2 = layer->getDataSet(1)->getValue(xIndex);
                            double minVal = (std::min)(value, value2);
                            double maxVal = (std::max)(value, value2);
                            
                            // Choose format based on values for better precision
                            std::string valueFormat;
                            if (std::abs(minVal) < 0.001 || std::abs(maxVal) < 0.001) {
                                valueFormat = "{value|P8}";  // 8 decimals for very small values
                            } else if (std::abs(minVal) < 0.01 || std::abs(maxVal) < 0.01) {
                                valueFormat = "{value|P6}";  // 6 decimals for small values  
                            } else if (std::abs(minVal) < 0.1 || std::abs(maxVal) < 0.1) {
                                valueFormat = "{value|P4}";  // 4 decimals for medium values
                            } else {
                                valueFormat = "{value|P3}";  // 3 decimals by default
                            }
                            
                            name = name + ": " + c->formatValue(minVal, valueFormat.c_str());
                            name = name + " - " + c->formatValue(maxVal, valueFormat.c_str());
                        } else {
                            // Special case: volume (3 datasets for up/down/flat)
                            if (dataSetCount == 3) {
                                // The real volume is the sum of the 3 datasets
                                value = layer->getDataSet(0)->getValue(xIndex) + layer->getDataSet(1
                                    )->getValue(xIndex) + layer->getDataSet(2)->getValue(xIndex);
                            }
                            
                            // Choose format based on value for better precision
                            std::string valueFormat;
                            if (std::abs(value) < 0.001) {
                                valueFormat = "{value|P8}";  // 8 decimals for very small values
                            } else if (std::abs(value) < 0.01) {
                                valueFormat = "{value|P6}";  // 6 decimals for small values  
                            } else if (std::abs(value) < 0.1) {
                                valueFormat = "{value|P4}";  // 4 decimals for medium values
                            } else {
                                valueFormat = "{value|P3}";  // 3 decimals by default
                            }
                            
                            name = name + ": " + c->formatValue(value, valueFormat.c_str()) + unitChar;
                        }
                        
                        // Build legend entry
                        std::ostringstream legendEntry;
                        legendEntry << "<*block*><*img=@square,width=8,edgeColor=000000,color="
                            << std::hex << dataSet->getDataColor() << "*> " << name << "<*/*>";
                        legendEntries.push_back(legendEntry.str());
                    }
                }
            }
        }
        
        // Get plot area position
        PlotArea* plotArea = c->getPlotArea();
        int plotAreaLeftX = plotArea->getLeftX() + c->getAbsOffsetX();
        int plotAreaTopY = plotArea->getTopY() + c->getAbsOffsetY();
        int plotAreaBottomY = plotAreaTopY + plotArea->getHeight();
        
        // Only if we could get the Y position
        if (mouseY >= plotAreaTopY && mouseY <= plotAreaBottomY) {
            double yValue = c->getYValue(mouseY - c->getAbsOffsetY());
            
            // Position of the Y axis tooltip (right side of the plot area)
            int yAxisTooltipX = plotAreaLeftX + plotArea->getWidth() + 5;
            int yAxisTooltipY = mouseY;
            
            // Create tooltip text with formatted Y value
            std::string yTooltipText = c->formatValue(yValue, "{value|P4}");
            
            // Draw background rectangle for Y tooltip
            int tooltipWidth = 60;
            int tooltipHeight = 20;
            d->rect(yAxisTooltipX - 2, yAxisTooltipY - tooltipHeight/2 - 2, 
                    yAxisTooltipX + tooltipWidth + 2, yAxisTooltipY + tooltipHeight/2 + 2, 
                    0x000000, 0xffffcc);
            
            // Display Y tooltip text
            TTFText* yTooltip = d->text(yTooltipText.c_str(), "Arial", 8);
            yTooltip->draw(yAxisTooltipX, yAxisTooltipY, 0x000000, Chart::Left);
            yTooltip->destroy();
            
            // Draw horizontal crosshair line for Y
            d->hline(plotAreaLeftX, plotAreaLeftX + plotArea->getWidth(), 
                    yAxisTooltipY, d->dashLineColor(0x000000, Chart::DashLine));
        }
        
        // Legend starts with the date label
        std::ostringstream legendText;
        legendText << "<*block,valign=top,maxWidth=" << (plotArea->getWidth() - 5)
            << "*><*font=Arial Bold*>[" << c->xAxis()->getFormattedLabel(xValue, "yyyy-mm-dd hh:nn:ss")
            << "]<*/font*>" << ohlcLegend.str();
        for (int i = ((int)legendEntries.size()) - 1; i >= 0; --i) {
            legendText << "      " << legendEntries[i];
        }
        legendText << "<*/*>";
        
        // Draw a vertical tracking line at x position
        d->vline(plotAreaTopY, plotAreaTopY + plotArea->getHeight(), c->getXCoor(xValue) +
            c->getAbsOffsetX(), d->dashLineColor(0x000000, Chart::DashLine));
        
        // Display the legend at top of the plot area
        TTFText* t = d->text(legendText.str().c_str(), "Arial", 8);
        t->draw(plotAreaLeftX + 5, plotAreaTopY + 5, 0x000000, Chart::TopLeft);
        t->destroy();
        
        // Only for the last chart (the bottom one with X axis visible)
        if (i == m->getChartCount() - 1) {
            // Get formatted timestamp text
            std::string timeStampText = c->xAxis()->getFormattedLabel(xValue, "yyyy-mm-dd hh:nn:ss");
            
            // Create a rectangular background for the text
            int textHeight = 16;
            int textWidth = 150;  // Adjust according to text length
            int xLabelPos = c->getXCoor(xValue) + c->getAbsOffsetX();
            int yLabelPos = plotAreaBottomY + 15;  // Position just below the X axis
            
            // Draw background for the text
            d->rect(xLabelPos - textWidth/2, yLabelPos - textHeight/2,
                   xLabelPos + textWidth/2, yLabelPos + textHeight/2,
                   0x000000, 0xffffcc);
            
            // Create and draw the text
            TTFText* timeLabel = d->text(timeStampText.c_str(), "Arial", 8);
            timeLabel->draw(xLabelPos, yLabelPos, 0x000000, Chart::Center);
            timeLabel->destroy();
            
            // Draw a small vertical tick on the X axis
            d->vline(plotAreaBottomY, plotAreaBottomY + 5, xLabelPos, 0x000000);
        }
    }
}
