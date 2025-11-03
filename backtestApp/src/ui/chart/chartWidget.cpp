#include "ui/chart/chartWidget.h"
#include <QVBoxLayout>
#include <QDebug>
#include <QMouseEvent>
#include <algorithm>
#include <cmath>
#include <set>
#include "ui/chart/chartDataManager.h"

ChartWidget::ChartWidget(QWidget* parent)
    : QWidget(parent)
    , m_chartViewer(nullptr)
    , m_rulerToolEnabled(false)
    , m_rulerFirstPointSelected(false)
    , m_rulerStartX(0)
    , m_rulerStartY(0)
{
    // Configure the widget
    setObjectName("chartWidget");
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    
    // Create the layout
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    
    // Create the QChartViewer
    m_chartViewer = new QChartViewer(this);
    m_chartViewer->setObjectName("chartViewer");
    m_chartViewer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    // m_chartViewer->setUpdateInterval(5);
    
    // Configure the viewer
    m_chartViewer->setMouseUsage(Chart::MouseUsageScroll);
    m_chartViewer->setMouseTracking(true);
    m_chartViewer->setMouseWheelZoomRatio(1.4);
    m_chartViewer->setScrollDirection(Chart::DirectionHorizontalVertical);
    m_chartViewer->setZoomDirection(Chart::DirectionHorizontalVertical);
    m_chartViewer->setZoomInWidthLimit(0.00001); // Zoom limit to avoid excessively fine zooms
    
    // Connect signals
    connect(m_chartViewer, &QChartViewer::viewPortChanged, this, &ChartWidget::onViewPortChanged);
    connect(m_chartViewer, &QChartViewer::mousePressed, this, &ChartWidget::onMousePressed);
    connect(m_chartViewer, &QChartViewer::mouseMoveChart, this, &ChartWidget::onMouseMoveChart);
    connect(m_chartViewer, &QChartViewer::mouseMovePlotArea, this, &ChartWidget::onMouseMovePlotArea);
    connect(m_chartViewer, &QChartViewer::mouseReleased, this, &ChartWidget::onMouseReleased);
    connect(m_chartViewer, &QChartViewer::mouseDoubleClicked, this, &ChartWidget::onMouseDoubleClicked);

    // Add the viewer to the layout
    layout->addWidget(m_chartViewer);
    
    qDebug() << "ChartWidget created";
}

ChartWidget::~ChartWidget() {

}

void ChartWidget::setBacktestResults(const BacktestResults* results) {
    if (!results) {
        qWarning() << "Attempt to set null backtest results";
        return;
    }

    m_dataManager.setData(results->candles, results->stats.trades, results->stats.equityCurve);
    
    // Restore markers if present
    if (!results->userMarkers.empty()) {
        m_dataManager.setMarkers(results->userMarkers);
    }

    updateChartDisplay(ViewPortMode::USE_CURRENT);
}

void ChartWidget::setChartType(chart::ChartType chartType)
{
    if (m_config.chartType == chartType)
        return; // No change, nothing to do

    m_config.chartType = chartType;

    updateChartDisplay(ViewPortMode::USE_CURRENT);
}

bool ChartWidget::updateChartDisplay(ViewPortMode mode) {
    if (!m_dataManager.hasRawData() || !m_chartViewer)
        return false;

    // Determine start and end indices based on the viewport
    int startIndex = 0;
    int pointsToShow = static_cast<int>(m_dataManager.getTimestamps().size());
    
    if (mode == ViewPortMode::USE_CURRENT) {
        int totalPoints = pointsToShow;
        double viewPortLeft = m_chartViewer->getViewPortLeft();
        double viewPortWidth = m_chartViewer->getViewPortWidth();
        
        startIndex = (int)floor(viewPortLeft * totalPoints);
        int endIndex = (int)ceil((viewPortLeft + viewPortWidth) * totalPoints) - 1;
        
        // Ensure indices are within bounds
        startIndex = std::max(0, std::min(startIndex, totalPoints - 1));
        endIndex = std::max(startIndex, std::min(endIndex, totalPoints - 1));
        
        pointsToShow = endIndex - startIndex + 1;
    }

    chart::AggregationInfo newAggregation = m_dataManager.getOptimalAggregationInfo(
        DoubleArray(&m_dataManager.getTimestamps()[startIndex], pointsToShow)
    );

    // If aggregation changed, update and emit the signal
    if (newAggregation != m_currentAggregation) {
        m_currentAggregation = newAggregation;
        
        // Emit signal to sync the other widget if needed
        emit aggregationChanged(m_currentAggregation);
    }

    m_renderer.createOrUpdateChart(m_chartViewer, m_dataManager, m_config, m_currentAggregation);

    if (mode == ViewPortMode::FULL_CHART) {
        m_chartViewer->setViewPortLeft(0);
        m_chartViewer->setViewPortWidth(1.0);
    }
    
    return true;
}

void ChartWidget::onViewPortChanged() {
    if (m_chartViewer) {
        double left = m_chartViewer->getViewPortLeft();
        double width = m_chartViewer->getViewPortWidth();
        
        // Emit viewport changed signal
        emit viewportChanged(left, width);
    }

    updateChartDisplay(ViewPortMode::USE_CURRENT);
}

void ChartWidget::onMouseMovePlotArea(QMouseEvent* event)
{
    if (!m_chartViewer) return;

    if (m_verticalMoveMode && (event->buttons() & Qt::LeftButton)) {
        // Compute delta in pixels
        int deltaY = event->pos().y() - m_lastMousePos.y();
        
        if (deltaY != 0) {
            // Convert pixel delta to Y scale units
            double valueShift = deltaY * m_pixelToValueRatio;
            
            // Update offset in configuration
            m_config.yScaleOffset += valueShift; // Invert sign for a natural movement
            
            // Update reference position
            m_lastMousePos = event->pos();
        }
    }
    


    // Retrieve point information
    // if (m_financeChart->getChart()->getChartCount() > 1) {
    //     XYChart* mainChart = (XYChart*)m_financeChart->getChart(1);
    //     double xValue = mainChart->getNearestXValue(mouseX);
        
    //     // Find the corresponding index
    //     int dataIndex = -1;
    //     for (int i = 0; i < (int)m_dataManager.getTimestamps().size(); ++i) {
    //         if (fabs(m_dataManager.getTimestamps()[i] - xValue) < 1e-6) {
    //             dataIndex = i;
    //             break;
    //         }
    //     }

    //     if (dataIndex >= 0 && dataIndex < (int)m_dataManager.getBacktestData()->getClose().size()) {
    //         // Emit a signal with the point information
    //         emit mouseOverPoint(m_dataManager.getTimestamps()[dataIndex], m_dataManager.getBacktestData()->getClose()[dataIndex]);
    //     }
    // }
    
    // Update display

}

void ChartWidget::onMousePressed(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        // If marker drawing tool is enabled
        if (m_markerDrawingEnabled && m_chartViewer && m_chartViewer->getChart()) {
            int mouseX = m_chartViewer->getChartMouseX();
            int mouseY = m_chartViewer->getChartMouseY();
            
            // Get the main chart
            MultiChart* m = (MultiChart*)m_chartViewer->getChart();
            if (m && m->getChartCount() > 1) {
                XYChart* mainChart = (XYChart*)m->getChart(1);
                PlotArea* plotArea = mainChart->getPlotArea();
                
                // Check if the click is inside the plot area
                if (mouseX >= plotArea->getLeftX() && mouseX <= plotArea->getRightX() &&
                    mouseY >= plotArea->getTopY() && mouseY <= plotArea->getBottomY()) {
                    
                    // Convert pixel coordinates to chart values
                    // mainChart uses relative indices (0, 1, 2...) on the X axis
                    double relativeIndex = mainChart->getXValue(mouseX);
                    double price = mainChart->getYValue(mouseY);
                    
                    // Convert the relative index to absolute index (as for trades)
                    int index = static_cast<int>(std::round(relativeIndex));
                    size_t absoluteBarIndex = m_currentAggregation.startIndex + index;
                    
                    // Check that the index is valid
                    const auto& data = m_dataManager.getAggregatedData(m_currentAggregation.level);
                    if (absoluteBarIndex < data.timestamps.size()) {
                        // Add the marker with the absolute index
                        chart::ChartMarker marker;
                        marker.barIndex = absoluteBarIndex;
                        marker.price = price;
                        marker.type = m_currentMarkerType;
                        
                        m_dataManager.addMarker(marker);
                        
                        qDebug() << "Marker placed: barIndex=" << absoluteBarIndex 
                                << "price=" << price;
                        
                        // Update display
                        updateChartDisplay(ViewPortMode::USE_CURRENT);
                    }
                }
            }
            return;
        }
        
        m_lastMousePos = event->pos();

        // Instead of resetting the offset, update reference values
        // so they reflect the current scale with the offset applied
        m_config.yScaleMin = m_renderer.getYAxisMin();
        m_config.yScaleMax = m_renderer.getYAxisMax();
        m_config.yScaleOffset = 0.0;  // Now we can reset because min/max are up to date
        
        // Recompute the pixel/value ratio for the new scale
        double yRange = m_config.yScaleMax - m_config.yScaleMin;
        double plotAreaHeight = m_renderer.getPlotAreaHeight();
        m_pixelToValueRatio = yRange / plotAreaHeight;

        double relativeYPos = (m_lastMousePos.y() - m_config.equityHeight) / plotAreaHeight;
        if (m_lastMousePos.x() > m_config.chartWidth - m_yAxisMarginWidth && relativeYPos >= 0 && relativeYPos <= 1) {
            m_yAxisClickRelativePos = 1.0 - relativeYPos; // Invert so that 0 is bottom and 1 is top
            
            m_config.fixedYScale = true;

            m_isYAxisDragging = true;
            setCursor(Qt::SizeVerCursor);
        }
    }
}

void ChartWidget::onMouseDoubleClicked(QMouseEvent *event)
{
    if (m_verticalMoveMode) {
        m_config.fixedYScale = false;
        setCursor(Qt::ArrowCursor);

        updateChartDisplay(ViewPortMode::USE_CURRENT);
    }
}

void ChartWidget::onMouseMoveChart(QMouseEvent *event)
{
    // Manage vertical zoom on the Y axis
    if (m_isYAxisDragging && (event->buttons() & Qt::LeftButton)) {
        // Compute delta in pixels
        int deltaY = event->pos().y() - m_lastMousePos.y();
        
        if (deltaY != 0) {
            // Damping factor to control zoom speed
            double zoomFactor = 1.0 - (deltaY * 0.005); // Adjust according to desired sensitivity
            
            // Get current Y scale
            double currentMin = m_config.yScaleMin;
            double currentMax = m_config.yScaleMax;
            double currentRange = currentMax - currentMin;
            
            // Compute pivot point (value that stays fixed during the zoom)
            double pivotValue = currentMin + m_yAxisClickRelativePos * currentRange;
            
            // Compute the new range based on the zoom factor
            double newRange = currentRange / zoomFactor;
            
            // Calculate new limits while keeping the pivot position relative
            double newMin = pivotValue - (m_yAxisClickRelativePos * newRange);
            double newMax = pivotValue + ((1.0 - m_yAxisClickRelativePos) * newRange);
            
            // Update Y scale
            m_config.yScaleMin = newMin;
            m_config.yScaleMax = newMax;
            
            // Update reference position
            m_lastMousePos = event->pos();
            
            // Recalculate ratio for vertical movement
            m_pixelToValueRatio = (newMax - newMin) / m_renderer.getPlotAreaHeight();

            // Indicate that we are in vertical move mode
            m_verticalMoveMode = true;
            
            // Redraw the chart with the new configuration
            updateChartDisplay(ViewPortMode::USE_CURRENT);
        }
    }

    std::optional<std::pair<int, int>> mousePosition = m_renderer.updateDynamicLayer(
        m_chartViewer,
        m_rulerToolEnabled,
        m_rulerFirstPointSelected,
        m_rulerStartX, m_rulerStartY,
        m_dataManager,
        m_currentAggregation
    );

    m_chartViewer->updateDisplay();

    if (mousePosition) {
        emit trackFinanceUpdated(mousePosition->first, mousePosition->second);
    }
}

void ChartWidget::onMouseReleased(QMouseEvent *event)
{
    if (!m_chartViewer || !m_chartViewer->getChart())
        return;
    
    // If left button clicked and ruler tool is enabled
    if (m_rulerToolEnabled) {
        if (event->button() == Qt::LeftButton) {
            if (!m_rulerFirstPointSelected) {
                // Use plot area coordinates instead of chart coordinates
                m_rulerStartX = m_chartViewer->getPlotAreaMouseX();
                m_rulerStartY = m_chartViewer->getPlotAreaMouseY();
                m_rulerFirstPointSelected = true;
                
            } else {
                m_rulerFirstPointSelected = false;
            }
            m_chartViewer->updateDisplay();
        }

        return;
    }

    if (m_isYAxisDragging) {
        m_isYAxisDragging = false;
        // Keep the cursor in vertical mode since we remain in vertical mode
        setCursor(Qt::SizeVerCursor);
        event->accept();
    }
}

void ChartWidget::setMaxDisplayPoints(int value) {
    if (value == m_dataManager.getMaxDisplayPoints())
        return;

    m_dataManager.setMaxDisplayPoints(value);

    if (m_dataManager.hasRawData())
        updateChartDisplay(ViewPortMode::USE_CURRENT);

    emit maxDisplayPointsChanged(m_dataManager.getMaxDisplayPoints());
}

void ChartWidget::setRulerToolEnabled(bool enabled)
{
    m_rulerToolEnabled = enabled;
    
    // If the tool is disabled, reset state
    if (enabled) 
        return;

    m_rulerFirstPointSelected = false;
    
    // Update the chart to remove the ruler
    if (m_chartViewer && m_chartViewer->getChart())
        updateChartDisplay(ViewPortMode::USE_CURRENT);
}

void ChartWidget::setMarkerDrawingEnabled(bool enabled, chart::MarkerType type) {
    m_markerDrawingEnabled = enabled;
    m_currentMarkerType = type;
    
    // Disable the ruler if the drawing tool is activated
    if (enabled) {
        m_rulerToolEnabled = false;
        m_rulerFirstPointSelected = false;
    }
    
    // Change cursor if necessary
    if (m_chartViewer) {
        if (enabled) 
            m_chartViewer->setCursor(Qt::CrossCursor);
        else 
            m_chartViewer->setCursor(Qt::ArrowCursor);
    }
}

void ChartWidget::clearAllMarkers() {
    m_dataManager.clearAllMarkers();
    if (m_dataManager.hasRawData()) 
        updateChartDisplay(ViewPortMode::USE_CURRENT); 
}

bool ChartWidget::removeIndicator(int id) {
    if (!m_dataManager.removeIndicator(id)) return false;

    emit indicatorRemoved(id);

    if (m_dataManager.hasRawData())
        updateChartDisplay(ViewPortMode::USE_CURRENT);

    return true;
}

void ChartWidget::zoomToTrade(const be::TradeData& trade)
{
    if (!m_dataManager.hasRawData() || !m_chartViewer) {
        qDebug() << "Cannot zoom: invalid data or viewer not initialized";
        return;
    }
    
    // Convert trade dates to chart timestamps
    double entryTimestamp = dateToChartTimestamp(trade.entryDate);
    double exitTimestamp = dateToChartTimestamp(trade.exitDate);
    
    qDebug() << "Zoom to trade - Entry timestamp:" << entryTimestamp 
             << "Exit timestamp:" << exitTimestamp;
    
    // Get all timestamps to compute indices
    const std::vector<double>& timestamps = m_dataManager.getTimestamps();
    
    if (timestamps.empty()) {
        qDebug() << "No timestamps available for zoom";
        return;
    }
    
    // Find indices corresponding to entry and exit timestamps
    int entryIndex = -1;
    int exitIndex = -1;
    
    for (int i = 0; i < static_cast<int>(timestamps.size()); ++i) {
        if (entryIndex == -1 && timestamps[i] >= entryTimestamp) {
            entryIndex = i;
        }
        if (timestamps[i] >= exitTimestamp) {
            exitIndex = i;
            break;
        }
    }
    
    // If we didn't find the entry index, use the first
    if (entryIndex == -1) entryIndex = 0;
    
    // If we didn't find the exit index, use the last
    if (exitIndex == -1) exitIndex = static_cast<int>(timestamps.size()) - 1;
    
    // Add margin around the trade (20% of trade duration on each side)
    int tradeDuration = exitIndex - entryIndex;
    int margin = std::max(10, tradeDuration / 5); // At least 10 points margin
    
    int startIndex = std::max(0, entryIndex - margin);
    int endIndex = std::min(static_cast<int>(timestamps.size()) - 1, exitIndex + margin);
    
    // Compute viewport proportions
    double totalPoints = static_cast<double>(timestamps.size());
    double viewPortLeft = static_cast<double>(startIndex) / totalPoints;
    double viewPortWidth = static_cast<double>(endIndex - startIndex + 1) / totalPoints;
    
    // Ensure viewport width does not exceed 1.0
    if (viewPortLeft + viewPortWidth > 1.0) {
        viewPortWidth = 1.0 - viewPortLeft;
    }
    
    qDebug() << "Computed zoom - Entry index:" << entryIndex 
             << "Exit index:" << exitIndex
             << "ViewPort left:" << viewPortLeft
             << "ViewPort width:" << viewPortWidth;
    
    // Apply the zoom
    m_chartViewer->setViewPortLeft(viewPortLeft);
    m_chartViewer->setViewPortWidth(viewPortWidth);
    
    // Update display
    updateChartDisplay(ViewPortMode::USE_CURRENT);
}

void ChartWidget::zoomToPeriod(const QDateTime& startDate, const QDateTime& endDate)
{
    if (!m_dataManager.hasRawData() || !m_chartViewer) {
        qWarning() << "Cannot zoom: no data or no viewer";
        return;
    }
    
    // Convert QDateTime to ChartDirector timestamps
    double startTimestamp = Chart::chartTime(
        startDate.date().year(), startDate.date().month(), startDate.date().day(),
        startDate.time().hour(), startDate.time().minute(), startDate.time().second()
    );
    
    double endTimestamp = Chart::chartTime(
        endDate.date().year(), endDate.date().month(), endDate.date().day(),
        endDate.time().hour(), endDate.time().minute(), endDate.time().second()
    );
    
    qDebug() << "Zoom to period - Start timestamp:" << startTimestamp 
             << "End timestamp:" << endTimestamp;
    
    // Get all timestamps to compute indices
    const std::vector<double>& timestamps = m_dataManager.getTimestamps();
    
    if (timestamps.empty()) {
        qWarning() << "No timestamps available";
        return;
    }
    
    // Find indices corresponding to start and end timestamps
    int startIndex = -1;
    int endIndex = -1;
    
    for (int i = 0; i < static_cast<int>(timestamps.size()); ++i) {
        if (startIndex == -1 && timestamps[i] >= startTimestamp) {
            startIndex = i;
        }
        if (timestamps[i] <= endTimestamp) {
            endIndex = i;
        }
    }
    
    // If we didn't find the start index, use the first
    if (startIndex == -1) startIndex = 0;
    
    // If we didn't find the end index, use the last
    if (endIndex == -1) endIndex = static_cast<int>(timestamps.size()) - 1;
    
    // Add a small margin around the period (5% on each side)
    int periodDuration = endIndex - startIndex;
    int margin = std::max(5, periodDuration / 20);
    
    startIndex = std::max(0, startIndex - margin);
    endIndex = std::min(static_cast<int>(timestamps.size()) - 1, endIndex + margin);
    
    // Compute viewport proportions
    double totalPoints = static_cast<double>(timestamps.size());
    double viewPortLeft = static_cast<double>(startIndex) / totalPoints;
    double viewPortWidth = static_cast<double>(endIndex - startIndex + 1) / totalPoints;
    
    // Ensure viewport width does not exceed 1.0
    if (viewPortLeft + viewPortWidth > 1.0) {
        viewPortWidth = 1.0 - viewPortLeft;
    }
    
    qDebug() << "Computed zoom - Start index:" << startIndex 
             << "End index:" << endIndex
             << "ViewPort left:" << viewPortLeft
             << "ViewPort width:" << viewPortWidth;
    
    // Apply the zoom
    m_chartViewer->setViewPortLeft(viewPortLeft);
    m_chartViewer->setViewPortWidth(viewPortWidth);
    
    // Update display
    updateChartDisplay(ViewPortMode::USE_CURRENT);
}

void ChartWidget::setCurrentAggregation(const chart::AggregationInfo& aggregation) {
    // Only if aggregation is different
    if (m_currentAggregation.level == aggregation.level && 
        m_currentAggregation.startIndex == aggregation.startIndex && 
        m_currentAggregation.pointCount == aggregation.pointCount) {
        return;
    }

    m_currentAggregation = aggregation;
    
    // Don't emit the signal if coming from the partner to avoid infinite loops
    if (sender() != m_syncPartner) {
        emit aggregationChanged(m_currentAggregation);
    }
    
    // Update chart if necessary
    if (m_dataManager.hasRawData() && m_chartViewer) {
        m_renderer.createOrUpdateChart(m_chartViewer, m_dataManager, m_config, m_currentAggregation);
    }
}

void ChartWidget::setViewport(double left, double width) {
    if (m_chartViewer && sender() == m_syncPartner) {
        m_chartViewer->setViewPortLeft(left);
        m_chartViewer->setViewPortWidth(width);
    
        // Don't call updateChartDisplay directly to avoid the loop
        // The QChartViewer viewPortChanged signal will be emitted and connected to onViewPortChanged
        return;
    }
}

void ChartWidget::forceUpdateTrackFinance(int mouseX, int mouseY) {
    if (m_chartViewer && sender() == m_syncPartner) {
        m_renderer.updateTrackFinance(m_chartViewer, std::make_pair(mouseX, mouseY));

        m_chartViewer->updateDisplay();
        return;
    }
}

double ChartWidget::dateToChartTimestamp(const be::Date &date)
{
    return Chart::chartTime(date.year, date.month, date.day, date.hour, date.minute, date.second);
}

void ChartWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    QSize newSize = event->size();    
    
    // Update chart width according to widget width
    if (newSize.width() > 10) {
        m_config.chartWidth = newSize.width();
        m_config.chartHeight = newSize.height() - 20;
        
        // Update chart only if necessary
        if (m_dataManager.hasRawData() && m_chartViewer) {
            // Save current viewport state
            double currentLeft = m_chartViewer->getViewPortLeft();
            double currentWidth = m_chartViewer->getViewPortWidth();
            
            // Redraw chart
            updateChartDisplay(ViewPortMode::USE_CURRENT);
            
            // Restore viewport state
            m_chartViewer->setViewPortLeft(currentLeft);
            m_chartViewer->setViewPortWidth(currentWidth);
        }
    }
}

void ChartWidget::removeAllIndicators() {
    m_dataManager.removeAllIndicators();

    if (m_dataManager.hasRawData())
        updateChartDisplay(ViewPortMode::USE_CURRENT);
}
