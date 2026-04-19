#pragma once

#include <QWidget>
#include <QLabel>
#include <vector>
#include <memory>
#include <array>
#include <unordered_map>
#include <iostream>

#include "components/Utils/IndicatorMathUtils.h"
#include "components/backtestResults.h"
#include "ui/chart/chartDataManager.h"
#include "ui/chart/chartRenderer.h"

// Include ChartDirector headers
#include "qchartviewer.h"
#include "chartdir.h"
#include "FinanceChart.h"

#include "beTypes.h"

/**
 * @brief Widget that encapsulates a ChartDirector financial chart
 */
class ChartWidget : public QWidget
{
    Q_OBJECT

public:
    // ======== Constructors and destructor ========
    explicit ChartWidget(QWidget* parent = nullptr);
    ~ChartWidget() override;
    
    // ======== Public API ========
    // Data initialization methods
    void setBacktestResults(const BacktestResults* results);

    // Chart configuration and control
    void setChartType(chart::ChartType chartType); // replace with a slot
    const std::vector<std::unique_ptr<indicators::IndicatorBase>>& getIndicators() const { return m_dataManager.getIndicators(); }

    void removeAllIndicators();

    // Method to enable/disable the ruler tool
    void setRulerToolEnabled(bool enabled); // replace with a slot
    void setMaxDisplayPoints(int value);
    
    // Methods for the marker drawing tool
    void setMarkerDrawingEnabled(bool enabled, chart::MarkerType type = chart::MarkerType::Check);
    void clearAllMarkers();
    const std::vector<chart::ChartMarker>& getMarkers() const { return m_dataManager.getMarkers(); }
    void setMarkers(const std::vector<chart::ChartMarker>& markers) { m_dataManager.setMarkers(markers); }
    
    // Method to zoom to a specific trade
    void zoomToTrade(const be::TradeData& trade);
    
    // Method to zoom to a specific period
    void zoomToPeriod(const QDateTime& startDate, const QDateTime& endDate);

    // Methods for synchronization
    const chart::AggregationInfo& getCurrentAggregation() const { return m_currentAggregation; }
    void setCurrentAggregation(const chart::AggregationInfo& aggregation);
    void setSyncPartner(ChartWidget* partner) { m_syncPartner = partner; }
    void setViewport(double left, double width);
    void forceUpdateTrackFinance(int mouseX, int mouseY);

    template<typename T>
    int addIndicator(const T& config) {
        QString displayName = config.getDisplayName();
        int id = m_dataManager.addIndicator(config);

        emit indicatorAdded(id, displayName);

        if (m_dataManager.hasRawData() && m_chartViewer)
            updateChartDisplay(ViewPortMode::USE_CURRENT);

        return id;
    }

    template<typename T>
    T* findIndicator(int id) const {
        return m_dataManager.findIndicator<T>(id);
    }

    template<typename T>
    bool updateIndicator(const T& config) {
        if (!m_dataManager.updateIndicator(config)) 
            return false;

        emit indicatorChanged(config.id, config.getDisplayName());

        if (m_dataManager.hasRawData() && m_chartViewer)
            updateChartDisplay(ViewPortMode::USE_CURRENT);

        return true;
    }

    bool removeIndicator(int id);
    void toggleIndicatorVisibility(int id); // New method
    
signals:
    void chartCreated();
    void viewportChanged(double left, double width);
    void trackFinanceUpdated(int mouseX, int mouseY);
    // void mouseOverPoint(double timestamp, double price);

    void indicatorAdded(int id, const QString& displayName);
    void indicatorChanged(int id, const QString& displayName);
    void indicatorRemoved(int id);

    void maxDisplayPointsChanged(int value);
    void aggregationChanged(const chart::AggregationInfo& aggregation);
    
protected:
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void onViewPortChanged();
    void onMousePressed(QMouseEvent* event);
    void onMouseDoubleClicked(QMouseEvent* event);
    void onMouseMoveChart(QMouseEvent* event);
    void onMouseMovePlotArea(QMouseEvent* event);
    void onMouseReleased(QMouseEvent* event);

private:
    enum class ViewPortMode {
        FULL_CHART,      // Display all data
        USE_CURRENT      // Use the current viewport
    };

    ChartDataManager m_dataManager;
    ChartRenderer m_renderer;
    chart::AggregationInfo m_currentAggregation;
    chart::ChartConfiguration m_config;
    ChartWidget* m_syncPartner = nullptr;

    // ======== Private methods ========
    // 1. Data processing and conversion
    double dateToChartTimestamp(const be::Date& date);

    bool updateChartDisplay(ViewPortMode mode = ViewPortMode::FULL_CHART);

    // 3. UI components
    QChartViewer* m_chartViewer = nullptr;
    

    QSize m_pendingResize;  ///< Pending resize size

    // Variables for the ruler tool
    bool m_rulerToolEnabled;           // If the ruler tool is enabled
    bool m_rulerFirstPointSelected;    // If the first point has been selected
    double m_rulerStartX;              // X coordinate of the start point
    double m_rulerStartY;              // Y coordinate of the start point

    // Variables for the marker drawing tool
    bool m_markerDrawingEnabled = false;
    chart::MarkerType m_currentMarkerType = chart::MarkerType::Check;

    bool m_yAxisZoomMode = false;
    int m_yAxisZoomStartY = 0;
    bool m_advancedNavigationMode = false;

    bool m_isDraggingVertically = false;
    int m_cumulativeVerticalDelta = 0; // For tracking cumulative movement

    // Properties for vertical movement
    bool m_verticalMoveMode = false;
    QPoint m_lastMousePos;
    double m_pixelToValueRatio = 0.0;

    // Variables for vertical zoom on the Y axis
    bool m_isYAxisDragging = false;     // Indicates if the Y axis is being dragged
    double m_yAxisClickRelativePos = 0.0; // Relative click position on the Y axis (0 = bottom, 1 = top)
    int m_yAxisMarginWidth = 50;        // Margin in pixels to detect clicks on the Y axis
};