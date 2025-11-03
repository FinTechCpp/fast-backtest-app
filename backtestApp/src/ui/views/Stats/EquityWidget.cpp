#include "ui/views/Stats/EquityWidget.h"
#include <QPainter>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QtMath>
#include <algorithm>
#include <QPainterPath>
#include <set>

static constexpr double EPSILON_D = 1e-9;

EquityWidget::EquityWidget(const QString& title, QWidget *parent)
    : TitledWidget(title, parent),
      m_xmin(0), m_xmax(1), m_ymin(0), m_ymax(1),
      m_showCrosshair(false)
{
    setMinimumSize(160, 120);
    setMouseTracking(true); // necessary to receive mouseMoveEvent without pressing buttons

    // QCheckBox to switch between m_statText and m_statTextBis
    m_checkBox = new QCheckBox("Percentage", this);
    m_checkBox->setChecked(false);

    // Connect the toggled signal of the QCheckBox to a lambda slot
    connect(m_checkBox, &QCheckBox::toggled, this, [this](bool checked) {
        invalidateCache();  // Invalidate all caches (equities + widget points + labels)
        updateBounds();     // Recompute bounds with the correct set of points
        update();           // Redraw the widget
    });

    // Add the QCheckBox as a companion widget in the title
    setTitleCompanionWidget(m_checkBox);
}

void EquityWidget::setPoints(const QVector<QPointF>& pts)
{
    if (pts.isEmpty()) {
        m_points.clear();
        m_pointsPercent.clear(); // Also clear percentage points
        invalidateCache();
        updateBounds();
        update();
        return;
    }

    std::vector<QPointF> tmp;
    tmp.reserve(pts.size());
    for (const auto &p: pts) tmp.push_back(p);

    std::sort(tmp.begin(), tmp.end(), [](const QPointF &a, const QPointF &b){
        return a.x() < b.x();
    });

    m_points.clear();
    m_points.reserve(tmp.size());
    for (const auto &p: tmp) m_points.push_back(p);

    // Calculate percentage points
    calculatePercentPoints();
    
    invalidateCache();
    updateBounds();
    update();
}

void EquityWidget::setPoints(const std::vector<be::Date>& dates, const std::vector<be::EquityPoint>& equityCurve) {
    if (dates.empty() || equityCurve.empty()) {
        m_points.clear();
        m_pointsPercent.clear();
        updateBounds();
        update();
        return;
    }
    
    QVector<QPointF> newPoints;

    // This is not optimal but OK for now... to be fixed later
    m_dates = dates; // Store dates for future use
    
    // If the array contains only Y values
    if (!equityCurve.empty()) {
        // Create points with X = index and Y = value
        for (size_t i = 0; i < equityCurve.size(); ++i) {
            newPoints.append(QPointF(static_cast<double>(equityCurve[i].index), equityCurve[i].value));
        }
    }

    // Last point
    newPoints.append(QPointF(static_cast<double>(m_dates.size()), equityCurve.back().value));
    
    setPoints(newPoints);
}

void EquityWidget::invalidateCache()
{
    m_cacheValid = false;
    m_widgetPointsValid = false;
    m_cachedDateLabels.clear();
}

double EquityWidget::getInitialEquity() const
{
    if (!m_cacheValid) {
        // Compute and cache all values at once
        const QVector<QPointF>& activePoints = m_checkBox->isChecked() ? m_pointsPercent : m_points;
        
        if (!activePoints.isEmpty()) {
            m_cachedInitialEquity = activePoints.first().y();
            m_cachedFinalEquity = activePoints.last().y();
            
            // Compute the peak at the same time
            QPointF peak = activePoints.first();
            for (const auto& pt : activePoints) {
                if (pt.y() > peak.y()) {
                    peak = pt;
                }
            }
            m_cachedPeakEquity = peak;
        } else {
            m_cachedInitialEquity = 0.0;
            m_cachedPeakEquity = QPointF(0.0, 0.0);
            m_cachedFinalEquity = 0.0;
        }
        
        m_cacheValid = true;
    }
    
    return m_cachedInitialEquity;
}

QPointF EquityWidget::getPeakPoint() const
{
    if (!m_cacheValid) {
        getInitialEquity();  // Will compute and cache all values
    }
    return m_cachedPeakEquity;
}

double EquityWidget::getFinalEquity() const
{
    if (!m_cacheValid) {
        getInitialEquity();  // Will compute and cache all values
    }
    return m_cachedFinalEquity;
}

std::vector<EquityWidget::DateLabel> EquityWidget::generateDateLabels() const {
    // Use cache if valid
    if (!m_cachedDateLabels.empty()) {
        return m_cachedDateLabels;
    }

    std::vector<DateLabel> labels;
    
    if (m_dates.empty()) {
        return labels;
    }

    // Month conversion table -> text
    static const QStringList monthNames = {"", "Jan", "Feb", "Mar", "Apr", "May", "Jun", 
                                          "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    
    // Determine the total date range
    be::Date startDate;
    be::Date endDate;
    bool validDatesFound = false;

    for (const auto& date : m_dates) {
        if (date.year > 0) {
            if (!validDatesFound) {
                startDate = date;
                endDate = date;
                validDatesFound = true;
            } else {
                endDate = date;
            }
        }
    }
    
    // If no valid date found
    if (!validDatesFound) {
        return labels;
    }
    
    // Compute the duration between first and last date
    // New: threshold is 2 years instead of 1 year
    bool isMoreThanTwoYears = false;
    if (endDate.year > startDate.year + 2 || 
        (endDate.year == startDate.year + 2 && endDate.month >= startDate.month)) {
        isMoreThanTwoYears = true;
    }

    int totalMonthsDiff = (endDate.year - startDate.year) * 12 + (endDate.month - startDate.month);
    bool isMoreThanTwoMonths = totalMonthsDiff > 2;
    
    // Iterate dates to create labels
    int lastYear = m_dates[0].year;
    int lastMonth = m_dates[0].month;
    int lastDay = m_dates[0].day;

    for (size_t i = 0; i < m_dates.size(); ++i) {
        const be::Date& date = m_dates[i];
        if (date.year <= 0) continue;  // Invalid date
        
        int year = static_cast<int>(date.year);
        int month = static_cast<int>(date.month);
        int day = static_cast<int>(date.day);
        
        bool addLabel = false;
        QString labelText;
        int importance = 0;
        
        // If period > 2 years: years only
        if (isMoreThanTwoYears) {
            // Years only
            if (year != lastYear) {
                labelText = QString::number(year);
                importance = 3; // Year
                addLabel = true;
                lastYear = year;
                lastMonth = -1;
            }
        }
        // If period > 2 months and <= 2 years: all months AND years
        else if (isMoreThanTwoMonths) {
            // Year if it changes
            if (year != lastYear) {
                labelText = QString::number(year);
                importance = 3; // Year
                addLabel = true;
                lastYear = year;
                lastMonth = -1; // Reset to show the month that follows
            }
            
            // All months (if not the same as the last shown)
            else if (month != lastMonth) {
                labelText = monthNames[month];
                importance = 2; // Month
                addLabel = true;
            }
            
            // Remember the last displayed month
            if (addLabel) {
                lastMonth = month;
            }
        }
        // If period <= 2 months: all days, months and years
        else {
            // Year if it changes
            if (year != lastYear) {
                labelText = QString::number(year);
                importance = 3; // Year
                addLabel = true;
                lastYear = year;
                lastMonth = -1; // Reset to show the month that follows
            }
            // Month if it changes
            else if (month != lastMonth) {
                labelText = monthNames[month];
                importance = 2; // Month
                addLabel = true;
                lastMonth = month;
                lastDay = day;
            }
            // Day (always)
            else if (day != lastDay) {
                labelText = QString::number(static_cast<int>(date.day));
                importance = 1; // Day
                addLabel = true;
                lastDay = day;
            }
        }
        
        if (addLabel) {
            labels.push_back({static_cast<double>(i), labelText, importance});
        }
    }
    
    // Cache the result
    m_cachedDateLabels = labels;
    
    return labels;
}

void EquityWidget::calculatePercentPoints()
{
    if (m_points.isEmpty()) {
        m_pointsPercent.clear();
        return;
    }
    
    // First point (reference at 0%)
    double initialValue = m_points.first().y();
    if (std::abs(initialValue) < EPSILON_D) {
        initialValue = 1.0; // Avoid division by zero
    }
    
    m_pointsPercent.resize(m_points.size());
    for (int i = 0; i < m_points.size(); ++i) {
        double xval = m_points[i].x();
        double yval = m_points[i].y();
        double percentValue = (yval / initialValue - 1.0) * 100.0;
        m_pointsPercent[i] = QPointF(xval, percentValue);
    }
}

void EquityWidget::updateBounds()
{
    const QVector<QPointF>& activePoints = m_checkBox->isChecked() ? m_pointsPercent : m_points;
    
    if (activePoints.isEmpty()) {
        m_xmin = 0; m_xmax = 1;
        m_ymin = 0; m_ymax = 1;
        return;
    }
    
    m_xmin = m_xmax = activePoints[0].x();
    m_ymin = m_ymax = activePoints[0].y();
    for (const auto &p : activePoints) {
        m_xmin = qMin(m_xmin, p.x());
        m_xmax = qMax(m_xmax, p.x());
        m_ymin = qMin(m_ymin, p.y());
        m_ymax = qMax(m_ymax, p.y());
    }
}

void EquityWidget::paintContent(QPainter& painter, const QRect& contentRect)
{
    // 1. Define the content area (with outer margins)
    m_contentRect = contentRect.adjusted(m_margin, m_margin, -m_margin, -m_margin);
    
    // 2. Define the plot area (without margins for axes/labels)
    m_plotRect = QRect(
        m_contentRect.left() + m_leftMargin,
        m_contentRect.top() + m_topMargin,
        m_contentRect.width() - m_leftMargin - m_rightMargin,
        m_contentRect.height() - m_topMargin - m_bottomMargin
    );
    
    // 2b. Invalidate widget points cache if plotRect size changed
    if (m_plotRect.size() != m_cachedPlotSize) {
        m_widgetPointsValid = false;
        m_cachedPlotSize = m_plotRect.size();
    }
    
    // 3. Recompute bounds according to current mode
    updateBounds();
    
    painter.setRenderHint(QPainter::Antialiasing, true);

    // 4. Content background
    painter.fillRect(m_contentRect, Qt::white);

    // 5. Plot background (slightly different to see boundaries)
    painter.fillRect(m_plotRect, QColor(250, 250, 250));

    // 6. Draw the grid (inside m_plotRect only)
    drawGrid(painter);

    // 7. Draw axes (around m_plotRect)
    drawAxes(painter);

    // 8. Select the correct set of points according to mode
    const QVector<QPointF>& activePoints = m_checkBox->isChecked() ? m_pointsPercent : m_points;

    // 8b. Draw colored areas under the curve (BEFORE the curve itself)
    if (!activePoints.isEmpty()) {
        drawFilledAreas(painter);
    }

    // 9. Draw the curve (clipped in m_plotRect)
    if (!activePoints.isEmpty()) {
        painter.setClipRect(m_plotRect);
        
        // Use cached widget points (conversion done once)
        const QVector<QPointF>& widgetPoints = getCachedWidgetPoints();
        
        // Draw the polyline (without closing the path)
        QPen linePen(Qt::blue);
        linePen.setWidth(2);
        linePen.setCapStyle(Qt::RoundCap);
        linePen.setJoinStyle(Qt::RoundJoin);
        painter.setPen(linePen);
        painter.setBrush(Qt::NoBrush);  // Important: no fill
        painter.drawPolyline(widgetPoints.data(), widgetPoints.size());
        
        painter.setClipping(false);
    }

    // 10. Draw equity markers (initial/peak lines + final highlight)
    if (!activePoints.isEmpty()) {
        drawEquityMarkers(painter);
    }

    // 11. Draw the crosshair
    if (m_showCrosshair) {
        QPen crossPen(Qt::black);
        crossPen.setStyle(Qt::DashLine);
        crossPen.setWidth(1);
        painter.setPen(crossPen);

        // Mouse position is already in widget coordinates
        QPoint clampedPos = m_mousePos;
        
        // Clamp inside m_plotRect
        clampedPos.setX(qBound(m_plotRect.left(), clampedPos.x(), m_plotRect.right()));
        clampedPos.setY(qBound(m_plotRect.top(), clampedPos.y(), m_plotRect.bottom()));

        // Draw crosshair lines
        painter.drawLine(clampedPos.x(), m_plotRect.top(),
                         clampedPos.x(), m_plotRect.bottom());
        painter.drawLine(m_plotRect.left(), clampedPos.y(),
                         m_plotRect.right(), clampedPos.y());

        // Compute world coordinates
        QPointF world = mapToWorld(clampedPos);
        QString info;
        
        // Format differently depending on mode
        if (m_checkBox->isChecked()) {
            info = formatValue(world.y(), true, true, false);
        } else {
            info = formatValue(world.y(), true, false, false);
        }
        
        // Position the info box smartly
        QRect infoRect(clampedPos.x() + 10, clampedPos.y() - 25, 70, 20);
        
        // Adjust if it goes outside the plotRect
        if (infoRect.right() > m_plotRect.right()) {
            infoRect.moveLeft(clampedPos.x() - infoRect.width() - 10);
        }
        if (infoRect.top() < m_plotRect.top()) {
            infoRect.moveTop(clampedPos.y() + 10);
        }
        
        painter.fillRect(infoRect, QColor(255, 255, 224, 240));
        painter.setPen(Qt::transparent);
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(infoRect);
        painter.setPen(Qt::black);
        painter.drawText(infoRect.adjusted(4, 0, -4, 0), Qt::AlignVCenter | Qt::AlignLeft, info);
    }
}

QPointF EquityWidget::mapToWidget(const QPointF &pt) const
{
    // Map a world point to the plot area (m_plotRect)
    if (m_xmax - m_xmin < EPSILON_D || m_ymax - m_ymin < EPSILON_D) {
        return QPointF(m_plotRect.center());
    }
    
    double nx = (pt.x() - m_xmin) / (m_xmax - m_xmin);
    double ny = (pt.y() - m_ymin) / (m_ymax - m_ymin);
    
    double x = m_plotRect.left() + nx * m_plotRect.width();
    double y = m_plotRect.bottom() - ny * m_plotRect.height();
    
    return QPointF(x, y);
}

QPointF EquityWidget::mapToWorld(const QPointF &pixel) const
{
    // Map a widget pixel to world coordinates
    if (m_plotRect.width() <= 0 || m_plotRect.height() <= 0) {
        return QPointF(m_xmin, m_ymin);
    }
    
    double nx = (pixel.x() - m_plotRect.left()) / m_plotRect.width();
    double ny = (m_plotRect.bottom() - pixel.y()) / m_plotRect.height();
    
    double wx = m_xmin + nx * (m_xmax - m_xmin);
    double wy = m_ymin + ny * (m_ymax - m_ymin);
    
    return QPointF(wx, wy);
}

const QVector<QPointF>& EquityWidget::getCachedWidgetPoints() const
{
    // If cache is invalid, recompute widget points
    if (!m_widgetPointsValid) {
        const QVector<QPointF>& activePoints = m_checkBox->isChecked() ? m_pointsPercent : m_points;
        
        m_cachedWidgetPoints.clear();
        m_cachedWidgetPoints.reserve(activePoints.size());
        
        for (const auto& pt : activePoints) {
            m_cachedWidgetPoints.append(mapToWidget(pt));
        }
        
        m_widgetPointsValid = true;
    }
    
    return m_cachedWidgetPoints;
}

void EquityWidget::drawGrid(QPainter &painter)
{
    // Draw the grid only inside m_plotRect
    const int desiredLines = 8;
    double xrange = m_xmax - m_xmin;
    double yrange = m_ymax - m_ymin;
    if (xrange <= EPSILON_D || yrange <= EPSILON_D) return;

    // Compute a "nice" step for grid lines
    auto niceStep = [](double range, int target){
        if (range <= 0) return 1.0;
        double raw = range / target;
        double expv = qPow(10.0, qFloor(qLn(raw) / qLn(10.0)));
        double f = raw / expv;
        double nicef;
        if (f < 1.5) nicef = 1.0;
        else if (f < 3.0) nicef = 2.0;
        else if (f < 7.0) nicef = 5.0;
        else nicef = 10.0;
        return nicef * expv;
    };

    double xstep = niceStep(xrange, desiredLines);
    double ystep = niceStep(yrange, desiredLines);

    QPen gridPen(QColor(220, 220, 220));
    gridPen.setWidth(1);
    painter.setPen(gridPen);

    // Vertical lines (parallel to Y)
    double xstart = std::ceil(m_xmin / xstep) * xstep;
    for (double x = xstart; x <= m_xmax; x += xstep) {
        QPointF top = mapToWidget(QPointF(x, m_ymax));
        QPointF bottom = mapToWidget(QPointF(x, m_ymin));
        
        // Ensure lines stay inside m_plotRect
        top.setX(qBound((double)m_plotRect.left(), top.x(), (double)m_plotRect.right()));
        bottom.setX(qBound((double)m_plotRect.left(), bottom.x(), (double)m_plotRect.right()));
        
        painter.drawLine(top, bottom);
    }

    // Horizontal lines (parallel to X)
    double ystart = std::ceil(m_ymin / ystep) * ystep;
    for (double y = ystart; y <= m_ymax; y += ystep) {
        QPointF left = mapToWidget(QPointF(m_xmin, y));
        QPointF right = mapToWidget(QPointF(m_xmax, y));
        
        // Ensure lines stay inside m_plotRect
        left.setY(qBound((double)m_plotRect.top(), left.y(), (double)m_plotRect.bottom()));
        right.setY(qBound((double)m_plotRect.top(), right.y(), (double)m_plotRect.bottom()));
        
        painter.drawLine(left, right);
    }
}

QString EquityWidget::formatValue(double value, bool useThousandsSeparator, bool isPercent, bool roundValue) const {
    // Handle values close to zero
    if (std::abs(value) < 0.01) {
        return isPercent ? "0%" : "0";
    }

    // If in percent mode
    if (isPercent) {
        // Format with 1 decimal for percentages
        return QString::number(value, 'f', 1) + "%";
    }

    if (!roundValue) {
        // Do not round, just format directly
        if (useThousandsSeparator) {
            QLocale locale;
            return locale.toString(value, 'f', 1);
        } else {
            return QString::number(value, 'f', 1);
        }
    }

    // Find magnitude for rounding (rest of code unchanged)
    double absValue = std::abs(value);
    int digits = std::floor(std::log10(absValue));
    double factor;
    
    // Determine rounding factor based on magnitude
    if (absValue >= 100000) {
        factor = std::pow(10, digits - 1);  // Round to 10000s
    } else if (absValue >= 10000) {
        factor = 1000;  // Round to 1000s
    } else if (absValue >= 1000) {
        factor = 100;   // Round to 100s
    } else if (absValue >= 100) {
        factor = 10;    // Round to 10s
    } else {
        factor = 1;     // Round to units
    }
    
    // Round to determined precision
    double rounded = std::round(value / factor) * factor;
    
    // Format with QLocale for thousands separators
    if (useThousandsSeparator) {
        QLocale locale;
        return locale.toString(rounded, 'f', rounded < 10 ? 1 : 0);
    } else {
        return QString::number(rounded, 'f', rounded < 10 ? 1 : 0);
    }
}

void EquityWidget::drawAxes(QPainter &painter)
{
    QPen axisPen(Qt::black);
    axisPen.setWidth(1);
    painter.setPen(axisPen);

    // Draw X axis at bottom of m_plotRect
    painter.drawLine(m_plotRect.bottomLeft(), m_plotRect.bottomRight());
    
    // Draw Y axis at right of m_plotRect
    painter.drawLine(m_plotRect.topRight(), m_plotRect.bottomRight());

    QFontMetrics fm(font());

    // Y Axis - value labels (on the right)
    const int yTicks = 5;
    double yrange = m_ymax - m_ymin;
    if (yrange <= EPSILON_D) return;
    
    double ystep = yrange / yTicks;
    
    // Get final value for highlight
    double finalEquity = getFinalEquity();
    QPointF finalWidgetPos = mapToWidget(QPointF(m_xmax, finalEquity));
    
    for (int i = 0; i <= yTicks; ++i) {
        double yv = m_ymin + i * ystep;
        QPointF wp = mapToWidget(QPointF(m_xmax, yv));
        
        // Tick on the right of the Y axis
        painter.drawLine(QPointF(m_plotRect.right(), wp.y()), 
                         QPointF(m_plotRect.right() + 4, wp.y()));
        
        // Label aligned left after the tick
        QString txt = formatValue(yv, true, m_checkBox->isChecked());
        painter.drawText(QPointF(m_plotRect.right() + 8, wp.y() + fm.ascent() / 2 - 2), txt);
    }
    
    // ==================== Highlight for INITIAL value (without label) ====================
    // Draw a special label for the initial value (like the final)
    double initialEquity = getInitialEquity();
    QPointF initialWidgetPos = mapToWidget(QPointF(m_xmax, initialEquity));
    
    QString initialText = formatValue(initialEquity, true, m_checkBox->isChecked(), false);
    
    QFont boldFont = painter.font();
    boldFont.setWeight(QFont::DemiBold);
    painter.setFont(boldFont);
    QFontMetrics fmBold(boldFont);
    
    int textWidth = fmBold.horizontalAdvance(initialText);
    int textHeight = fmBold.height();
    
    // Rectangle for the highlight (aligned with Y labels)
    QRect initialHighlightRect(m_plotRect.right() + 6, initialWidgetPos.y() - textHeight / 2 - 3,
                              textWidth + 10, textHeight + 6);
    
    // Light grey background for the highlight
    painter.setPen(QPen(Qt::transparent));
    painter.setBrush(QColor(230, 230, 230));
    painter.drawRoundedRect(initialHighlightRect, 1, 1);
    
    painter.setPen(Qt::black);
    painter.drawText(initialHighlightRect, Qt::AlignCenter, initialText);
    
    // Restore normal font
    painter.setFont(font());
    painter.setPen(Qt::black);
    
    // ==================== Highlight for FINAL value ====================
    // Draw a special label for the final value
    QString finalText = formatValue(finalEquity, true, m_checkBox->isChecked(), false); // "Final: " + 
    
    boldFont = painter.font();
    boldFont.setWeight(QFont::DemiBold);
    painter.setFont(boldFont);
    fmBold = QFontMetrics(boldFont);
    
    textWidth = fmBold.horizontalAdvance(finalText);
    textHeight = fmBold.height();
    
    // Rectangle for the highlight (aligned with Y labels)
    QRect highlightRect(m_plotRect.right() + 6, finalWidgetPos.y() - textHeight / 2 - 3,
                       textWidth + 10, textHeight + 6);
    
    // Orange/red background for the highlight
    painter.setPen(QPen(Qt::transparent));
    painter.setBrush(QColor(255, 220, 200));
    painter.drawRoundedRect(highlightRect, 1, 1);
    
    painter.setPen(Qt::black);
    painter.drawText(highlightRect, Qt::AlignCenter, finalText);
    
    // Restore normal font
    painter.setFont(font());
    painter.setPen(Qt::black);

    // X Axis - smart date labels
    if (!m_dates.empty()) {
        auto dateLabels = generateDateLabels();
        
        for (const auto& label : dateLabels) {
            double xPos = label.position;
            if (xPos >= 0 && xPos < m_dates.size()) {
                QPointF wp = mapToWidget(QPointF(xPos, m_ymin));
                
                // Tick at the bottom of the X axis
                painter.drawLine(QPointF(wp.x(), m_plotRect.bottom()), 
                                QPointF(wp.x(), m_plotRect.bottom() + 4));
                
                // Adjust style based on importance
                QFont labelFont = painter.font();
                if (label.importance == 3) {
                    // Year: bold
                    labelFont.setBold(true);
                } else if (label.importance == 2) {
                    // Month: normal
                    labelFont.setBold(false);
                } else {
                    // Day: smaller
                    labelFont.setBold(false);
                    labelFont.setPointSize(qMax(6, labelFont.pointSize() - 1));
                }
                painter.setFont(labelFont);

                // Draw text centered under the tick
                int tw = fm.horizontalAdvance(label.text);
                painter.drawText(QPointF(wp.x() - tw / 2, m_plotRect.bottom() + 18), label.text);
                
                // Restore font
                painter.setFont(font());
            }
        }
    }
}

void EquityWidget::drawFilledAreas(QPainter &painter)
{
    const QVector<QPointF>& activePoints = m_checkBox->isChecked() ? m_pointsPercent : m_points;
    if (activePoints.isEmpty()) return;
    
    double initialEquity = getInitialEquity();
    
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setClipRect(m_plotRect);
    painter.setPen(Qt::NoPen);
    
    // We will create separate segments for each continuous zone
    // This avoids visual artifacts between discontinuous segments
    
    QVector<QPointF> currentSegment;
    bool isGainSegment = false;
    
    auto finishSegment = [&]() {
        if (currentSegment.size() < 2) return;
        
        QPainterPath path;
        
        // Start at the baseline of the first point
        QPointF firstBaseline = mapToWidget(QPointF(currentSegment.first().x(), initialEquity));
        path.moveTo(firstBaseline);
        
        // Follow the curve
        for (const auto& pt : currentSegment) {
            QPointF widgetPt = mapToWidget(pt);
            path.lineTo(widgetPt);
        }
        
        // Return to the baseline of the last point
        QPointF lastBaseline = mapToWidget(QPointF(currentSegment.last().x(), initialEquity));
        path.lineTo(lastBaseline);
        
        // Close the path (back to the start)
        path.closeSubpath();
        
        // Draw with appropriate color
        if (isGainSegment) {
            painter.setBrush(QColor(16, 124, 16, 40));  // Light green
        } else {
            painter.setBrush(QColor(196, 43, 28, 40));  // Light red
        }
        painter.drawPath(path);
        
        currentSegment.clear();
    };
    
    for (int i = 0; i < activePoints.size(); ++i) {
        const QPointF& pt = activePoints[i];
        bool isGain = (pt.y() >= initialEquity);
        
        if (i == 0) {
            // First point
            currentSegment.append(pt);
            isGainSegment = isGain;
        } else {
            const QPointF& prevPt = activePoints[i - 1];
            bool prevIsGain = (prevPt.y() >= initialEquity);
            
            if (isGain == prevIsGain) {
                // Still on the same side, continue the segment
                currentSegment.append(pt);
            } else {
                // Crossing happened
                // Compute the intersection point
                double t = (initialEquity - prevPt.y()) / (pt.y() - prevPt.y());
                double intersectX = prevPt.x() + t * (pt.x() - prevPt.x());
                QPointF intersectPt(intersectX, initialEquity);
                
                // Finish the previous segment with the intersection point
                currentSegment.append(intersectPt);
                finishSegment();
                
                // Start a new segment with the intersection point
                currentSegment.append(intersectPt);
                currentSegment.append(pt);
                isGainSegment = isGain;
            }
        }
    }
    
    // Finish the last segment
    finishSegment();
    
    painter.setClipping(false);
}

void EquityWidget::mouseMoveEvent(QMouseEvent *event)
{    
    // Mouse position in widget coordinates
    m_mousePos = event->pos();
    
    // Check if mouse is inside the plot area
    if (m_plotRect.contains(m_mousePos)) {
        m_showCrosshair = true;
    } else {
        m_showCrosshair = false;
    }
    
    update();
}

void EquityWidget::drawEquityMarkers(QPainter &painter)
{
    // Retrieve values automatically
    double initialEquity = getInitialEquity();
    double peakEquity = getPeakPoint().y();
    double finalEquity = getFinalEquity();
    
    bool isPercentMode = m_checkBox->isChecked();
    
    // Helper to format values
    auto formatValue = [isPercentMode](double value) -> QString {
        if (isPercentMode) {
            return QString("%1%").arg(value, 0, 'f', 2);
        } else {
            return QLocale().toString(value, 'f', 0);
        }
    };
    
    painter.setRenderHint(QPainter::Antialiasing, true);
    QFont labelFont = painter.font();
    labelFont.setPointSize(qMax(8, labelFont.pointSize() - 1));
    labelFont.setWeight(QFont::DemiBold);
    QFontMetrics fm(labelFont);
    
    // ==================== 1. INITIAL horizontal line (without label) ====================
    {
        QPointF leftPt = mapToWidget(QPointF(m_xmin, initialEquity));
        QPointF rightPt = mapToWidget(QPointF(m_xmax, initialEquity));
        
        // Dashed line
        QPen initialPen(QColor(100, 100, 100), 1, Qt::DashLine);
        painter.setPen(initialPen);
        painter.drawLine(leftPt, rightPt);
        
        // Note: The "Initial" label will be shown on the Y axis (see drawAxes)
    }
    
    // ==================== 2. PEAK horizontal line ====================
    {
        QPointF leftPt = mapToWidget(QPointF(m_xmin, peakEquity));
        QPointF rightPt = mapToWidget(QPointF(m_xmax, peakEquity));
        
        // Dashed line (green)
        QPen peakPen(QColor(16, 124, 16), 1, Qt::DashLine);
        painter.setPen(peakPen);
        painter.drawLine(leftPt, rightPt);
        
        // Vertical line at the peak (subtle, without legend)
        QPointF peakPoint = getPeakPoint();
        QPointF peakTopPt = mapToWidget(QPointF(peakPoint.x(), m_ymax));
        QPointF peakBottomPt = mapToWidget(QPointF(peakPoint.x(), m_ymin));

        QPen verticalPeakPen(QColor(16, 124, 16), 1, Qt::DotLine);  // Lighter and dotted
        painter.setPen(verticalPeakPen);
        painter.drawLine(peakTopPt, peakBottomPt);
        
        // Label on the line (centered)
        QString labelText = "Peak: " + formatValue(peakEquity);
        painter.setFont(labelFont);
        
        int textWidth = fm.horizontalAdvance(labelText);
        int textHeight = fm.height();
        
        QRect textRect(m_plotRect.center().x() - textWidth / 2 - 4, 
                      leftPt.y() - textHeight / 2 - 2, 
                      textWidth + 8, textHeight + 4);
        
        // Semi-transparent background
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(255, 255, 255));
        painter.drawRoundedRect(textRect, 3, 3);
        
        // Text in green
        painter.setPen(QColor(16, 124, 16));
        painter.drawText(textRect, Qt::AlignCenter, labelText);
    }
    
    // ==================== 3. FINAL highlight on the legend ====================
    // Note: The highlight will be drawn in drawAxes() directly on the last Y label
    // For now, we draw a small visual indicator on the chart
    {
        // QPointF finalPt = mapToWidget(QPointF(m_xmax, finalEquity));
        
        // Small circle on the final point
        // QPen finalPen(QColor(196, 43, 28), 2);
        // // painter.setPen(finalPen);
        // painter.setBrush(QColor(196, 43, 28, 100));
        // painter.drawEllipse(finalPt, 3, 3);
        
        // Arrow to the legend on the right
        // QPen arrowPen(QColor(196, 43, 28), 2);
        // arrowPen.setStyle(Qt::DotLine);
        // painter.setPen(arrowPen);
        
        // QPointF arrowEnd(m_plotRect.right() + 3, finalPt.y());
        // painter.drawLine(finalPt, arrowEnd);
        
        // // Small arrow head
        // painter.setPen(QPen(QColor(196, 43, 28), 2));
        // painter.drawLine(arrowEnd, arrowEnd + QPointF(-4, -3));
        // painter.drawLine(arrowEnd, arrowEnd + QPointF(-4, 3));
    }
    
    // Restore font
    painter.setFont(font());
}

void EquityWidget::leaveEvent(QEvent * /*event*/)
{
    m_showCrosshair = false;
    update();
}
