#include "ui/views/Stats/VerticalGaugeRenderWidget.h"
#include <algorithm>
#include <numeric>
#include <QDebug>
#include <QGroupBox>
#include <QFontMetrics>


VerticalGaugeRenderWidget::VerticalGaugeRenderWidget(const QString& title, QWidget* parent)
    : TitledWidget(title, parent),
      m_tpAvg(0.0),
      m_tpMax(0.0),
      m_tpMedian(0.0),
      m_slAvg(0.0),
      m_slMin(0.0),
      m_slMedian(0.0),
      m_tpAvgPrc(0.0),
      m_tpMaxPrc(0.0),
      m_tpMedianPrc(0.0),
      m_slAvgPrc(0.0),
      m_slMinPrc(0.0),
      m_slMedianPrc(0.0)
{
    // Initialize colors
    m_tpAvgColor = QColor(0, 150, 0);       // Dark green
    m_tpMaxColor = QColor(100, 255, 100);   // Light green
    m_slAvgColor = QColor(150, 0, 0);       // Dark red
    m_slMinColor = QColor(255, 100, 100);   // Light red
    m_lineColor = QColor(0, 0, 0);          // Black
    m_textColor = QColor(40, 40, 40);       // Dark gray
    m_medianTpColor = QColor(0, 100, 0);    // Dark green
    m_medianSlColor = QColor(100, 0, 0);    // Dark red

    // Set size and size policy - wider to include legend
    setMinimumSize(220, 300);  // Increased from 40 to 150 to make space for legend
    setMaximumWidth(240);      // Increased from 60 to 180
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);

    // This checkbox allows switching between absolute and percentage display
    m_showPercentageCheckbox = new QCheckBox("Percentage", this);
    m_showPercentageCheckbox->setChecked(false); // Default: show absolute values

    // Connect the toggled signal to a lambda that calls update()
    connect(m_showPercentageCheckbox, &QCheckBox::toggled, this, [this]() {
        this->update(); // Redraw the widget when the state changes
    });
    // Add the checkbox to the widget's title

    setTitleCompanionWidget(m_showPercentageCheckbox);

    m_fontSize = 10;
}

void VerticalGaugeRenderWidget::updateContent(const be::Stats& stats)
{
    if (stats.trades.empty()) {
        clear();
        return;
    }

    // Collect trade P&L based on their closing reason
    std::vector<double> tpPLs;
    std::vector<double> tpPLsPrc;
    std::vector<double> slPLs;
    std::vector<double> slPLsPrc;

    const be::TradeData* bestTrade = nullptr;
    const be::TradeData* worstTrade = nullptr;
    
    for (const auto& trade : stats.trades) {
        // Find the best and worst trade
        if (!bestTrade || trade.pl > bestTrade->pl) {
            bestTrade = &trade;
        }
        if (!worstTrade || trade.pl < worstTrade->pl) {
            worstTrade = &trade;
        }

        // Collect data by trade type
        if (trade.closeReason == be::CloseReason::TakeProfit) {
            tpPLs.push_back(trade.pl);
            tpPLsPrc.push_back(trade.plPercent);
        } else if (trade.closeReason == be::CloseReason::StopLoss) {
            slPLs.push_back(trade.pl);
            slPLsPrc.push_back(trade.plPercent);
        }
    }

    // Store the number of trades
    size_t tpCount = tpPLs.size();
    size_t slCount = slPLs.size();

    // Calculate statistics for winning trades (TP)
    if (!tpPLs.empty()) {
        m_tpAvg = std::accumulate(tpPLs.begin(), tpPLs.end(), 0.0) / double(tpCount);
        m_tpAvgPrc = std::accumulate(tpPLsPrc.begin(), tpPLsPrc.end(), 0.0) / double(tpPLsPrc.size());
        m_tpMax = *std::max_element(tpPLs.begin(), tpPLs.end());
        m_tpMaxPrc = *std::max_element(tpPLsPrc.begin(), tpPLsPrc.end());

        // Calculate median for TP trades
        std::vector<double> sortedTpPLs = tpPLs;
        std::vector<double> sortedTpPLsPrc = tpPLsPrc;
        std::sort(sortedTpPLs.begin(), sortedTpPLs.end());
        std::sort(sortedTpPLsPrc.begin(), sortedTpPLsPrc.end());

        if (sortedTpPLs.size() % 2 == 0) {
            m_tpMedian = (sortedTpPLs[sortedTpPLs.size() / 2 - 1] + 
                         sortedTpPLs[sortedTpPLs.size() / 2]) / 2.0;
            m_tpMedianPrc = (sortedTpPLsPrc[sortedTpPLsPrc.size() / 2 - 1] + 
                            sortedTpPLsPrc[sortedTpPLsPrc.size() / 2]) / 2.0;
        } else {
            m_tpMedian = sortedTpPLs[sortedTpPLs.size() / 2];
            m_tpMedianPrc = sortedTpPLsPrc[sortedTpPLsPrc.size() / 2];
        }
    } else {
        m_tpAvg = 0.0;
        m_tpAvgPrc = 0.0;
        m_tpMax = 0.0;
        m_tpMaxPrc = 0.0;
        m_tpMedian = 0.0;
        m_tpMedianPrc = 0.0;
    }

    // Calculate statistics for losing trades (SL)
    if (!slPLs.empty()) {
        m_slAvg = std::accumulate(slPLs.begin(), slPLs.end(), 0.0) / slPLs.size();
        m_slAvgPrc = std::accumulate(slPLsPrc.begin(), slPLsPrc.end(), 0.0) / slPLsPrc.size();
        m_slMin = *std::min_element(slPLs.begin(), slPLs.end());
        m_slMinPrc = *std::min_element(slPLsPrc.begin(), slPLsPrc.end());

        // Calculate median for SL trades
        std::vector<double> sortedSlPLs = slPLs;
        std::vector<double> sortedSlPLsPrc = slPLsPrc;
        std::sort(sortedSlPLs.begin(), sortedSlPLs.end());
        std::sort(sortedSlPLsPrc.begin(), sortedSlPLsPrc.end());

        if (sortedSlPLs.size() % 2 == 0) {
            m_slMedian = (sortedSlPLs[sortedSlPLs.size() / 2 - 1] + 
                         sortedSlPLs[sortedSlPLs.size() / 2]) / 2.0;
            m_slMedianPrc = (sortedSlPLsPrc[sortedSlPLsPrc.size() / 2 - 1] + 
                            sortedSlPLsPrc[sortedSlPLsPrc.size() / 2]) / 2.0;
        } else {
            m_slMedian = sortedSlPLs[sortedSlPLs.size() / 2];
            m_slMedianPrc = sortedSlPLsPrc[sortedSlPLsPrc.size() / 2];
        }
    } else {
        m_slAvg = 0.0;
        m_slAvgPrc = 0.0;
        m_slMin = 0.0;
        m_slMinPrc = 0.0;
        m_slMedian = 0.0;
        m_slMedianPrc = 0.0;
    }
    
    update();
}

void VerticalGaugeRenderWidget::clear()
{
    m_tpAvg = 0.0;
    m_tpMax = 0.0;
    m_tpMedian = 0.0;
    m_slAvg = 0.0;
    m_slMin = 0.0;
    m_slMedian = 0.0;
    
    m_tpAvgPrc = 0.0;
    m_tpMaxPrc = 0.0;
    m_tpMedianPrc = 0.0;
    m_slAvgPrc = 0.0;
    m_slMinPrc = 0.0;
    m_slMedianPrc = 0.0;
    
    update();
}

int VerticalGaugeRenderWidget::valueToY(double value, double minValue, double maxValue, int height)
{
    // Convert a value to a Y position
    double range = maxValue - minValue;
    if (range <= 0) return height / 2;  
    
    double normalizedValue = (value - minValue) / range;
    return height - static_cast<int>(normalizedValue * height);
}

void VerticalGaugeRenderWidget::paintContent(QPainter& painter, const QRect& contentRect)
{    
    painter.setRenderHint(QPainter::Antialiasing, false);

    // Define base dimensions and margins
    int gaugeWidth = 45;        // Fixed width for the gauge
    int legendSpaceLeft = 70;   // Space for legends on the left
    int legendSpaceRight = legendSpaceLeft;  // Space for legends on the right
    int textHeight = 20;        // Height of the max/min text
    int padding = 10;           // Reduced vertical margin

    // Total width required for the widget
    int totalRequiredWidth = legendSpaceLeft + gaugeWidth + legendSpaceRight;

    // Calculate offsets to center content horizontally
    int offsetX = (contentRect.width() - totalRequiredWidth) / 2;

    // Effective height for the gauge (with just the space for the texts, without extra margin)
    int effectiveHeight = contentRect.height() - 2 * padding - 2 * textHeight;

    // Position of the gauge, centered horizontally
    int gaugeX = contentRect.left() + offsetX + legendSpaceLeft;
    int gaugeTop = contentRect.top() + padding + textHeight; // Start of the gauge after the max text


    double displayTpAvg = m_tpAvg;
    double displayTpMax = m_tpMax;
    double displayTpMedian = m_tpMedian;

    double displaySlAvg = m_slAvg;
    double displaySlMin = m_slMin;
    double displaySlMedian = m_slMedian;

    QString suffix = "€";
    int precision = 1;

    if (m_showPercentageCheckbox->isChecked()) {
        displayTpAvg = m_tpAvgPrc;
        displayTpMax = m_tpMaxPrc;
        displayTpMedian = m_tpMedianPrc;

        displaySlAvg = m_slAvgPrc;
        displaySlMin = m_slMinPrc;
        displaySlMedian = m_slMedianPrc;

        suffix = "%";
        precision = 2;
    }


    // Retrieve max and min values for the scale
    double posMax = displayTpMax;
    double negMin = displaySlMin;

    // Avoid division by zero
    if (posMax == 0) posMax = 0.1;
    if (negMin == 0) negMin = -0.1;

    // Calculate total range
    double totalRange = posMax - negMin;

    // Calculate zero position as a proportion of the total range
    // Zero is located at |negMin| / (|negMin| + posMax) from the top
    double zeroRatio = std::abs(posMax) / totalRange;
    int zeroY = gaugeTop + (int)(effectiveHeight * zeroRatio);

    // Modified function to convert values to Y positions
    auto valueToYScaled = [gaugeTop, zeroY, effectiveHeight, posMax, negMin](double value) -> int {
        if (value >= 0) {
            // Above zero
            return zeroY - (int)(value / posMax * (zeroY - gaugeTop));
        } else {
            // Below zero
            return zeroY + (int)(value / negMin * (gaugeTop + effectiveHeight - zeroY));
        }
    };

    // Calculate Y positions of the values
    int tpMaxY = valueToYScaled(displayTpMax);
    int tpAvgY = valueToYScaled(displayTpAvg);
    int tpMedianY = valueToYScaled(displayTpMedian);
    int slAvgY = valueToYScaled(displaySlAvg);
    int slMinY = valueToYScaled(displaySlMin);
    int slMedianY = valueToYScaled(displaySlMedian);

    // Upper part of the gauge (TP)
    if (displayTpAvg > 0) {
        // Average part (dark green)
        QRect tpAvgRect(gaugeX, tpAvgY, gaugeWidth, zeroY - tpAvgY);
        painter.setPen(m_tpAvgColor);
        painter.setBrush(m_tpAvgColor);
        painter.drawRect(tpAvgRect);

        // Max part (light green)
        if (displayTpMax > displayTpAvg) {
            QRect tpMaxRect(gaugeX, tpMaxY, gaugeWidth, tpAvgY - tpMaxY);
            painter.setBrush(m_tpMaxColor);
            painter.setPen(m_tpAvgColor);
            painter.drawRect(tpMaxRect);
        }
    }

    // Lower part of the gauge (SL)
    if (displaySlAvg < 0) {
        // Average part (dark red)
        QRect slAvgRect(gaugeX, zeroY, gaugeWidth, slAvgY - zeroY);
        painter.setBrush(m_slAvgColor);
        painter.setPen(m_slAvgColor);
        painter.drawRect(slAvgRect);

        // Min part (light red)
        if (displaySlMin < displaySlAvg) {
            QRect slMinRect(gaugeX, slAvgY, gaugeWidth, slMinY - slAvgY);
            painter.setBrush(m_slMinColor);
            painter.setPen(m_slAvgColor);
            painter.drawRect(slMinRect);
        }
    }

    // Horizontal line for zero (optional)
    // painter.setPen(Qt::gray);
    // painter.drawLine(gaugeX, zeroY, gaugeX + gaugeWidth, zeroY);

    // Horizontal line for TP median (dashed green)
    // if (displayTpMedian > 0) {
    //     painter.setPen(QPen(m_medianTpColor, 1, Qt::DashLine));
    //     painter.drawLine(gaugeX, tpMedianY, gaugeX + gaugeWidth, tpMedianY);
    // }

    // // Horizontal line for SL median (dashed red)
    // if (displaySlMedian < 0) {
    //     painter.setPen(QPen(m_medianSlColor, 1, Qt::DashLine));
    //     painter.drawLine(gaugeX, slMedianY, gaugeX + gaugeWidth, slMedianY);
    // }
    
    // Police configuration for text
    painter.setRenderHint(QPainter::Antialiasing, true);
    QFont valueFont = painter.font();
    valueFont.setPointSize(m_fontSize);
    valueFont.setWeight(m_fontWeight);
    painter.setFont(valueFont);

    // --- Labels drawing for MAX and MIN (Above and Below) ---

    // Max Profit above the gauge - directly above
    if (displayTpMax > 0) {
        QString maxProfitText = QString("Max Profit: %1 %2").arg(displayTpMax, 0, 'f', precision).arg(suffix);
        QRect maxProfitRect(gaugeX - gaugeWidth, gaugeTop - textHeight, 
                          gaugeWidth * 3, textHeight);
        painter.setPen(m_tpMaxColor.darker(150));
        painter.drawText(maxProfitRect, Qt::AlignCenter, maxProfitText);
    }

    // Max Loss below the gauge - directly below
    if (displaySlMin < 0) {
        QString maxLossText = QString("Max Loss: %1 %2").arg(displaySlMin, 0, 'f', precision).arg(suffix);
        QRect maxLossRect(gaugeX - gaugeWidth, 
                        gaugeTop + effectiveHeight, 
                        gaugeWidth * 3, textHeight);
        painter.setPen(m_slMinColor.darker(150));
        painter.drawText(maxLossRect, Qt::AlignCenter, maxLossText);
    }

    // --- Labels drawing for AVERAGE (Side) ---

    // Define X positions for legends
    int leftLegendX = gaugeX - 10;
    int rightLegendX = gaugeX + gaugeWidth + 10;

    // Height of the average label  
    const int avgLabelHeight = 40;
    
    // Mean TP (côté gauche)
    if (displayTpAvg > 0) {
        //Make sure the text does not go out of the gauge area
        int avgY = qMax(
            gaugeTop + avgLabelHeight/2,        // Minimum Y
            tpAvgY                             // ideal Position
        );
        
        QRect avgRect(leftLegendX - 60, avgY - avgLabelHeight/2, 60, avgLabelHeight);
        painter.setPen(m_tpAvgColor.darker(120));
        painter.drawText(avgRect, Qt::AlignRight | Qt::AlignVCenter,
                      QString("Mean\n%1 %2").arg(displayTpAvg, 0, 'f', precision).arg(suffix));
    }

    // Mean SL (côté droit)
    if (displaySlAvg < 0) {
        //Make sure the text does not go out of the gauge area
        int avgY = qMin(
            slAvgY,                                   // ideal Position
            gaugeTop + effectiveHeight - avgLabelHeight/2  // Maximum Y
        );
        
        QRect avgRect(rightLegendX, avgY - avgLabelHeight/2, 60, avgLabelHeight);
        painter.setPen(m_slAvgColor.darker(120));
        painter.drawText(avgRect, Qt::AlignLeft | Qt::AlignVCenter,
                      QString("Mean\n%1 %2").arg(displaySlAvg, 0, 'f', precision).arg(suffix));
    }
}