#pragma once

#include <QWidget>
#include <QVector>
#include <QPointF>
#include <QCheckBox>
#include "ui/views/Stats/TitledWidget.h"
#include "stats.hpp"

// TODO: The widget needs to be given the dates corresponding to the X points to display the X-axis labels correctly (dates instead of point indices).
// NEED TO ADD IN THE stats the calculation of the equity curve as PnL in PERCENT and display the equity curve in percent.
class EquityWidget : public TitledWidget
{
    Q_OBJECT
public:
    explicit EquityWidget(const QString& title = QString(), QWidget *parent = nullptr);

    // Replace points (will be sorted by X and filtered for consecutive duplicates in Y)
    void setPoints(const QVector<QPointF>& pts);
    void setPoints(const std::vector<be::Date>& dates, const std::vector<be::EquityPoint>& equityCurve);
    QVector<QPointF> points() const { return m_points; }

protected:
    void paintContent(QPainter& painter, const QRect& contentRect) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    struct DateLabel {
        double position;  // X position on the axis
        QString text;     // Text to display
        int importance;   // Level of importance: 3=year, 2=month, 1=day
    };

    QCheckBox* m_checkBox;

    QVector<QPointF> m_points;
    QVector<QPointF> m_pointsPercent;
    std::vector<be::Date> m_dates;  // Dates corresponding to the points
    
    std::vector<DateLabel> generateDateLabels() const;
    void calculatePercentPoints(); // Calculate points in percentage
    void invalidateCache();        // Invalidate the cache when data changes

    void updateBounds();

    // Mouse crosshair
    bool m_showCrosshair;
    QPoint m_mousePos; // In widget pixel coordinates

    // Drawing helpers
    void drawGrid(QPainter &painter);
    void drawAxes(QPainter &painter);
    void drawFilledAreas(QPainter &painter);    // Draw the colored areas under the curve
    void drawEquityMarkers(QPainter &painter);  // Draw the initial/peak lines and highlight the final
    QPointF mapToWidget(const QPointF &pt) const;
    QPointF mapToWorld(const QPointF &pixel) const;
    const QVector<QPointF>& getCachedWidgetPoints() const;  // Return points in widget coordinates (with cache)
    
    // Helper methods for markers
    double getInitialEquity() const;
    QPointF getPeakPoint() const;  // Return the (x, y) point of the peak
    double getFinalEquity() const;

    QString formatValue(double value, bool useThousandsSeparator = true, bool isPercent = false, bool roundValue = true) const;

    double m_xmin, m_xmax, m_ymin, m_ymax;
    QRect m_contentRect;      // Total content area (excluding title)
    QRect m_plotRect;         // Graph area only (excluding margins for axes)

    // Cache for optimization
    mutable bool m_cacheValid = false;
    mutable double m_cachedInitialEquity = 0.0;
    mutable QPointF m_cachedPeakEquity = QPointF(0.0, 0.0);
    mutable double m_cachedFinalEquity = 0.0;
    mutable std::vector<DateLabel> m_cachedDateLabels;
    mutable QVector<QPointF> m_cachedWidgetPoints;  // Points already converted to widget coordinates
    mutable bool m_widgetPointsValid = false;
    mutable QSize m_cachedPlotSize;  // Size of the plotRect to detect changes

    // Layout margins to leave space for axis labels
    const int m_leftMargin = 10;      // Reduced as there are fewer labels on the left
    const int m_bottomMargin = 30;
    const int m_rightMargin = 80;     // Increased for Y labels on the right
    const int m_topMargin = 10;
    const int m_margin = 5;
};
