#include "ui/views/Stats/TradingHeatmapWidget.h"
#include <QDebug>
#include <QFont>
#include <QFontMetrics>
#include <QLinearGradient>
#include <QPainter>
#include <algorithm>
#include <cmath> // For std::fabs

TradingHeatmapWidget::TradingHeatmapWidget(const QString& title, QWidget* parent)
    : TitledWidget(title, parent),
      m_minValue(0.0),
      m_maxValue(0.0),
      m_minHour(24),
      m_maxHour(0)
{
    // Initialize data
    m_performanceData.resize(HOURS_IN_DAY);
    m_tradeCountData.resize(HOURS_IN_DAY);
    m_squaredSumData.resize(HOURS_IN_DAY);
    
    for (int h = 0; h < HOURS_IN_DAY; h++) {
        m_performanceData[h].resize(DAYS_IN_WEEK);
        m_tradeCountData[h].resize(DAYS_IN_WEEK);
        m_squaredSumData[h].resize(DAYS_IN_WEEK);
        
        for (int d = 0; d < DAYS_IN_WEEK; d++) {
            m_performanceData[h][d] = 0.0;
            m_tradeCountData[h][d] = 0;
            m_squaredSumData[h][d] = 0.0;
        }
    }

    // Initialize day names
    m_dayNames = {"Lun", "Mar", "Mer", "Jeu", "Ven", "Sam", "Dim"};

    // Set a recommended minimum size
    setMinimumSize(400, 380);
    setMouseTracking(true); // To track mouse even without click
}

void TradingHeatmapWidget::enterEvent(QEnterEvent* event)
{
    m_mouseOver = true;
    update();
}

void TradingHeatmapWidget::leaveEvent(QEvent*)
{
    m_mouseOver = false;
    m_activeCell_col = -1;
    m_activeCell_row = -1;
    update();
}

void TradingHeatmapWidget::mouseMoveEvent(QMouseEvent* event)
{
    m_mousePos = event->pos();

    // Check if the mouse is within the cell area
    if (m_cellsArea.contains(m_mousePos)) {
        // Calculate cell indices
        int cellSize = m_cellsArea.width() / std::max(1, static_cast<int>(m_activeDayIndices.size()));
        int numHoursToShow = m_maxHour - m_minHour + 1;
        
        m_activeCell_col = (m_mousePos.x() - m_cellsArea.left()) / cellSize;
        m_activeCell_row = (m_mousePos.y() - m_cellsArea.top()) / (m_cellsArea.height() / numHoursToShow);

        // Check that the indices are valid
        if (m_activeCell_col >= 0 && m_activeCell_col < static_cast<int>(m_activeDayIndices.size()) &&
            m_activeCell_row >= 0 && m_activeCell_row < numHoursToShow) {
            // Valid indices, we are still within the cell
        } else {
            m_activeCell_col = -1;
            m_activeCell_row = -1;
        }
    } else {
        m_activeCell_col = -1;
        m_activeCell_row = -1;
    }

    update(); // Redraw
}

int TradingHeatmapWidget::getDayOfWeek(const be::Date& date) {
    // Zeller's algorithm to determine the day of the week
    int q = date.day;   // day of the month
    int m = date.month; // month
    int y = date.year;  // year

    if (m < 3) {
        m += 12;
        y -= 1;
    }
    
    int h = (q + (13 * (m + 1)) / 5 + y + y / 4 - y / 100 + y / 400) % 7;

    // Convert from Zeller's algorithm (0=saturday) to our format (0=monday)
    return (h + 5) % 7;
}

void TradingHeatmapWidget::analyzeTradesByTimeAndDay(const std::vector<be::TradeData>& trades) {
    // Reset data
    for (int h = 0; h < HOURS_IN_DAY; h++) {
        for (int d = 0; d < DAYS_IN_WEEK; d++) {
            m_performanceData[h][d] = 0.0;
            m_tradeCountData[h][d] = 0;
            m_squaredSumData[h][d] = 0.0;
        }
    }

    // Reset min/max
    m_minValue = 0.0;
    m_maxValue = 0.0;
    m_minHour = 24;
    m_maxHour = 0;
    bool firstValue = true;

    // Initialize our active day array
    m_activeDays.clear();
    m_activeDays.resize(DAYS_IN_WEEK, false);
    m_activeDayIndices.clear();

    // Analyze each trade
    for (const auto& trade : trades) {
        // Use the entry date to determine the hour and day
        int hour = trade.entryDate.hour;
        int day = getDayOfWeek(trade.entryDate);

        // Check that the indices are in range
        if (hour < 0 || hour >= HOURS_IN_DAY || day < 0 || day >= DAYS_IN_WEEK)
            continue;

        // Add the trade's performance
        m_activeDays[day] = true;
        m_performanceData[hour][day] += trade.pl;
        m_tradeCountData[hour][day]++;
        m_squaredSumData[hour][day] += trade.pl * trade.pl; // For standard deviation

        // Update the hour range
        m_minHour = std::min(m_minHour, hour);
        m_maxHour = std::max(m_maxHour, hour);
    }

    // Build the mapping of active days
    for (int d = 0; d < DAYS_IN_WEEK; d++) 
        if (m_activeDays[d]) 
            m_activeDayIndices.push_back(d);

    // Ensure a minimum display space (at least 3h range)
    if (m_maxHour - m_minHour < 3) {
        int extension = (3 - (m_maxHour - m_minHour)) / 2;
        m_minHour = std::max(0, m_minHour - extension);
        m_maxHour = std::min(23, m_maxHour + extension);
    }

    // Find min/max values for color scaling
    for (int h = 0; h < HOURS_IN_DAY; h++) {
        for (int d = 0; d < DAYS_IN_WEEK; d++) {
            if (m_tradeCountData[h][d] <= 0)
                continue; // No trades for this cell

            // Calculate average performance
            double expectation = m_performanceData[h][d] / m_tradeCountData[h][d];

            // Update min/max
            if (firstValue || expectation < m_minValue) 
                m_minValue = expectation;

            if (firstValue || expectation > m_maxValue) 
                m_maxValue = expectation;
            
            firstValue = false;
        }
    }

    // CORRECTION: Balance the bounds so that the absolute values are equal
    // This ensures that the color intensity is symmetrical around zero
    // double absMax = std::max(std::fabs(m_minValue), std::fabs(m_maxValue));
    // m_minValue = -absMax;  // Force the minimum to be the opposite of the maximum in absolute value
    // m_maxValue = absMax;   // Maximum remains unchanged in absolute value

    // // Ensure that min and max are not identical to avoid division by zero
    // if (qFuzzyCompare(m_minValue, m_maxValue)) {
    //     m_minValue = -1.0;
    //     m_maxValue = 1.0;
    // }
}

QColor TradingHeatmapWidget::getColorForValue(double value) {
    // Calculate absolute bound for a zero-centered scale
    double absMax = std::max(std::fabs(m_minValue), std::fabs(m_maxValue));
    if (absMax < 1e-8) absMax = 1.0; // avoid division by zero

    // Normalize value between -1 and 1 based on this symmetric bound
    double normalizedValue = value / absMax;
    normalizedValue = std::clamp(normalizedValue, -1.0, 1.0);

    // Red (negative) -> Yellow (zero) -> Green (positive) gradient
    if (normalizedValue < 0) {
        double ratio = 1.0 + normalizedValue; // 0 to 1 (from -1 to 0)
        int red = 255;
        int green = static_cast<int>(165 * ratio);
        int blue = 0;
        return QColor(red, green, blue);
    } else {
        double ratio = normalizedValue; // 0 to 1 (from 0 to +1)
        int red = static_cast<int>(255 * (1.0 - ratio));
        int green = 255;
        int blue = 0;
        return QColor(red, green, blue);
    }
}

void TradingHeatmapWidget::paintContent(QPainter& painter, const QRect& contentRect) {
    // Calculate number of hours to display
    int numHoursToShow = m_maxHour - m_minHour + 1;

    // If no data, nothing to draw
    if (m_activeDayIndices.empty() || numHoursToShow <= 0) {
        painter.drawText(contentRect, Qt::AlignCenter, "No data to display");
        return;
    }

    // Margins and cell size
    int leftMargin = 60;  // Margin for hour labels
    int topMargin = 40;   // Margin for day labels
    int rightMargin = 80; // Margin for legend
    int bottomMargin = 20;

    // Calculate cell size based on available space
    int availableWidth = contentRect.width() - leftMargin - rightMargin;
    int availableHeight = contentRect.height() - topMargin - bottomMargin;
    
    int cellWidth = std::min(CELL_SIZE, availableWidth / std::max(1, static_cast<int>(m_activeDayIndices.size())));
    int cellHeight = std::min(CELL_SIZE, availableHeight / numHoursToShow);
    int cellSize = std::min(cellWidth, cellHeight);
    int cellSpacing = 0; // Space between cells

    // Calculate starting position (horizontal centering)
    int startX = contentRect.left() + leftMargin;
    int startY = contentRect.top() + topMargin;

    // Store cell area for mouse events
    int cellsWidth = m_activeDayIndices.size() * cellSize;
    int cellsHeight = numHoursToShow * cellSize;
    m_cellsArea = QRect(startX, startY, cellsWidth, cellsHeight);

    // Calculate total width of the heatmap
    int heatmapWidth = m_activeDayIndices.size() * (cellSize + cellSpacing) - cellSpacing;

    // Draw day labels (top)
    painter.save();
    QFont dayFont = painter.font();
    dayFont.setWeight(QFont::DemiBold);
    painter.setPen(QColor(Qt::black));
    painter.setFont(dayFont);


    for (size_t i = 0; i < m_activeDayIndices.size(); i++) {
        int d = m_activeDayIndices[i];
        QString dayName = m_dayNames[d];
        QRect textRect(startX + i * (cellSize + cellSpacing), 
                      startY - 25, 
                      cellSize, 
                      20);
        painter.drawText(textRect, Qt::AlignCenter, dayName);
    }
    painter.restore();

    // Draw hour labels (left)
    painter.save();
    QFont hourFont = painter.font();
    hourFont.setWeight(QFont::DemiBold);
    painter.setPen(QColor(Qt::black));
    painter.setFont(hourFont);
    
    for (int h = m_minHour; h <= m_maxHour; h++) {
        int rowIndex = h - m_minHour;
        int y = startY + rowIndex * (cellSize + cellSpacing) + cellSize/2;
        
        QString hourText = QString::number(h) + "h";
        QRect textRect(startX - 50, y - 10, 45, 20);
        painter.drawText(textRect, Qt::AlignRight | Qt::AlignVCenter, hourText);
    }
    painter.restore();

    // Draw heatmap
    for (int h = m_minHour; h <= m_maxHour; h++) {
        int rowIndex = h - m_minHour;
        
        for (size_t i = 0; i < m_activeDayIndices.size(); i++) {
            int d = m_activeDayIndices[i];

            // Cell position
            int x = startX + i * (cellSize + cellSpacing);
            int y = startY + rowIndex * (cellSize + cellSpacing);

            // Create cell rectangle
            QRect cellRect(x, y, cellSize, cellSize);

            // Check if there are trades for this cell
            if (m_tradeCountData[h][d] <= 0) {
                // Gray cell for periods without trades
                painter.fillRect(cellRect, QColor(240, 240, 240));
                continue;
            }

            // Calculate expectation and variance
            int count = m_tradeCountData[h][d];
            double totalPnL = m_performanceData[h][d];
            double expectation = totalPnL / count; // Expectation

            // Variance = E[X²] - (E[X])²
            // double meanOfSquares = m_squaredSumData[h][d] / count;
            // double variance = meanOfSquares - (expectation * expectation);
            // double stddev = variance > 0 ? std::sqrt(variance) : 0.0; // Standard deviation

            // Cell color based on expectation
            QColor cellColor = getColorForValue(expectation);
            painter.fillRect(cellRect, cellColor);

            // Text for expectation and standard deviation
            QString expText = QString("%1").arg(expectation, 0, 'f', 1);
            // QString stddevText = QString("%1").arg(stddev, 0, 'f', 0);

            // Adjust text color for readability
            QColor textColor = QColor::fromHsv(cellColor.hue(),
                                               cellColor.saturation(),
                                               cellColor.value() < 128 ? 240 : 30);

            // Draw expectation (top)
            painter.save();
            QFont expFont = painter.font();
            expFont.setWeight(QFont::DemiBold);
            expFont.setPointSize(10);
            painter.setFont(expFont);
            painter.setPen(textColor);
            painter.drawText(QRect(x, y, cellSize, cellSize), 
                            Qt::AlignCenter, 
                            expText);

            // Draw standard deviation (bottom)
            // QFont stddevFont = painter.font();
            // stddevFont.setPointSize(8);
            // painter.setFont(stddevFont);
            // painter.drawText(QRect(x, y + cellSize/2, cellSize, cellSize/2), 
            //                 Qt::AlignCenter, 
            //                 stddevText);
            // painter.restore();
        }
    }

    // Draw vertical legend
    int legendX = startX + heatmapWidth + 40;
    int legendY = startY;
    int legendWidth = 20;
    int legendHeight = numHoursToShow * (cellSize + cellSpacing) - cellSpacing;

    // Title of the legend
    painter.save();
    painter.setPen(QColor(Qt::black));
    painter.drawText(QRect(legendX, legendY - 25, 80, 20), 
                    Qt::AlignLeft | Qt::AlignVCenter, 
                    "Average (€)");

    // Gradient rectangle
    QLinearGradient gradient(0, legendY + legendHeight, 0, legendY);
    gradient.setColorAt(0.0, getColorForValue(m_minValue));
    gradient.setColorAt(0.25, getColorForValue(m_minValue/2));
    gradient.setColorAt(0.5, getColorForValue(0.0));
    gradient.setColorAt(0.75, getColorForValue(m_maxValue/2));
    gradient.setColorAt(1.0, getColorForValue(m_maxValue));
    
    QRect gradientRect(legendX, legendY, legendWidth, legendHeight);
    painter.fillRect(gradientRect, gradient);
    painter.drawRect(gradientRect);

    // Legend labels
    QFont valueFont = painter.font();
    painter.setPen(QColor(Qt::black));
    painter.setFont(valueFont);
    
    // Maximum
    painter.drawText(QRect(legendX + legendWidth + 5, legendY - 10, 80, 20),
                    Qt::AlignLeft | Qt::AlignVCenter,
                    QString("%1 €").arg(m_maxValue, 0, 'f', 2));

    // Zero line
    double range = m_maxValue - m_minValue;
    int zeroY = legendY + legendHeight - static_cast<int>(legendHeight * (0.0 - m_minValue) / (range > 1e-8 ? range : 1.0));
    if (zeroY > legendY)
        painter.drawText(QRect(legendX + legendWidth + 5, zeroY - 10, 80, 20),
                        Qt::AlignLeft | Qt::AlignVCenter,
                        "0 €");
    
    // Minimum
    painter.drawText(QRect(legendX + legendWidth + 5, legendY + legendHeight - 10, 80, 20),
                    Qt::AlignLeft | Qt::AlignVCenter,
                    QString("%1 €").arg(m_minValue, 0, 'f', 2));

    // Optional: actual quartiles (not necessarily at 1/4 and 3/4 if the scale is not symmetric)
    // double q1 = m_minValue + (m_maxValue - m_minValue) * 0.25;
    // double q3 = m_minValue + (m_maxValue - m_minValue) * 0.75;
    // int q1Y = legendY + legendHeight - static_cast<int>(legendHeight * 0.25);
    // int q3Y = legendY + legendHeight - static_cast<int>(legendHeight * 0.75);

    // painter.drawText(QRect(legendX + legendWidth + 5, q3Y - 10, 80, 20),
    //                 Qt::AlignLeft | Qt::AlignVCenter,
    //                 QString("%1 €").arg(q3, 0, 'f', 2));
    // painter.drawText(QRect(legendX + legendWidth + 5, q1Y - 10, 80, 20),
    //                 Qt::AlignLeft | Qt::AlignVCenter,
    //                 QString("%1 €").arg(q1, 0, 'f', 2));
    
    painter.restore();

    // At the end of the function, replace the cell detection code with:
    if (m_mouseOver && m_activeCell_col >= 0 && m_activeCell_row >= 0) {
        int d = m_activeDayIndices[m_activeCell_col];
        int h = m_minHour + m_activeCell_row;
        
        if (m_tradeCountData[h][d] > 0) {
            double expectation = m_performanceData[h][d] / m_tradeCountData[h][d];

            // Calculate position on the legend
            double range = m_maxValue - m_minValue;
            double yRatio = (expectation - m_minValue) / (range > 1e-8 ? range : 1.0);
            int yOnLegend = legendY + legendHeight - static_cast<int>(legendHeight * yRatio);

            // Clamp to avoid min/max
            int margin = 12;
            int yOnLegendLabel = std::clamp(yOnLegend, legendY + margin, legendY + legendHeight - margin);

            // Handle overlap with the 0 label
            int labelHeight = 24;
            int minLabelDist = labelHeight / 2 + 2; // minimum distance between label centers

            if (std::abs(yOnLegendLabel - zeroY) < minLabelDist) {
                if (expectation >= 0.0) {
                    // Shift up if positive
                    yOnLegendLabel = zeroY - minLabelDist;
                    // Ensure we don't go off the top
                    yOnLegendLabel = std::max(yOnLegendLabel, legendY + margin);
                } else {
                    // Shift down if negative
                    yOnLegendLabel = zeroY + minLabelDist;
                    // Ensure we don't go off the bottom
                    yOnLegendLabel = std::min(yOnLegendLabel, legendY + legendHeight - margin);
                }
            }

            // Highlight the active cell
            QRect activeRect(startX + m_activeCell_col * cellSize,
                             startY + m_activeCell_row * cellSize,
                             cellSize, cellSize);
            QPen highlightPen(Qt::black, 2);
            painter.save();
            painter.setPen(highlightPen);
            painter.drawRect(activeRect);

            int overflow = 4; // Overflow for the horizontal line
            // Draw the horizontal line on the legend
            QPen pen(Qt::black, 3);
            painter.setPen(pen);
            painter.drawLine(legendX - overflow, yOnLegend, legendX + legendWidth + overflow, yOnLegend);

            // Draw the value label with the trade count
            QFont labelFont = painter.font();
            labelFont.setWeight(QFont::DemiBold);
            painter.setPen(QColor(Qt::black));
            painter.setFont(labelFont);
            painter.drawText(QRect(legendX + legendWidth + overflow + 5, yOnLegendLabel - 12, 80, 24),
                            Qt::AlignLeft | Qt::AlignVCenter,
                            QString("%1 €").arg(expectation, 0, 'f', 2));
            painter.restore();
        }
    }
}

void TradingHeatmapWidget::updateContent(const std::vector<be::TradeData>& trades) {
    if (trades.empty()) {
        clear();
        return;
    }

    // Analyze trades by hour and day
    analyzeTradesByTimeAndDay(trades);
    update();
}

void TradingHeatmapWidget::clear() {
    // Reset all heatmap data
    for (int h = 0; h < HOURS_IN_DAY; h++) {
        for (int d = 0; d < DAYS_IN_WEEK; d++) {
            m_performanceData[h][d] = 0.0;
            m_tradeCountData[h][d] = 0;
            m_squaredSumData[h][d] = 0.0;
        }
    }
    m_minValue = 0.0;
    m_maxValue = 0.0;
    m_minHour = 24;
    m_maxHour = 0;
    m_activeDays.clear();
    m_activeDays.resize(DAYS_IN_WEEK, false);
    m_activeDayIndices.clear();
    m_mousePos = QPoint();
    m_mouseOver = false;
    m_activeCell_col = -1;
    m_activeCell_row = -1;
    m_cellsArea = QRect();
    update();
}