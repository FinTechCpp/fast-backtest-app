#pragma once

#include <QWidget>
#include <QLabel>
#include <QVector>
#include <QPair>
#include <QColor>
#include <QPainter>
#include <QPen>
#include <QPainterPath>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QString>

class GaugeRenderWidget;
class FillGaugeRenderWidget;

enum class FillDirection {
    LeftToRight,
    RightToLeft
};

/**
 * @brief Widget displaying a gauge to visualize a financial ratio
 * 
 * This widget displays a horizontal colored gauge to visually contextualize
 * the value of a financial ratio (Sharpe, Sortino, Calmar, etc.)
 * with different colored zones depending on the quality of the ratio.
 */
class RatioGaugeWidget : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief Structure defining a zone of the gauge
     */
    struct GaugeZone {
        double minValue;     ///< Minimum value of the zone
        double maxValue;     ///< Maximum value of the zone
        QColor color;        ///< Color of the zone
        QColor labelColor;   ///< Text color in this zone
        QString description; ///< Textual description of the zone
    };

    /**
     * @brief Constructor
     * @param parent Parent widget
     */
    explicit RatioGaugeWidget(const QVector<GaugeZone>& zones, const QString& title, const QString& explanation, QWidget* parent = nullptr);

    /**
     * @brief Set the current value of the ratio
     * @param value Value to display
     */
    void setValue(double value);

    /**
     * @brief Get the current value of the ratio
     * @return Current value
     */
    double value() const;

    /**
     * @brief Set the display format of the value
     * @param precision Precision for display (number of decimals)
     * @param addPercentageSign Indicates whether the % sign should be added
     */
    void setValueFormat(unsigned int precision, bool addPercentageSign);

    // New: Set the reference value
    void setReferenceValue(double referenceValue);

    // New: Enable/disable the reference bar
    void showReference(bool show);

    // New: Set the fill direction
    void setFillDirection(FillDirection direction);

    /**
     * @brief Reset the widget and hide the cursor
     */
    void clear();

protected:
    /**
     * @brief Resize event
     * @param event Resize event information
     */
    void resizeEvent(QResizeEvent* event) override;

    /**
     * @brief Custom paint event
     * @param event Paint event information
     */
    void paintEvent(QPaintEvent* event) override;

private:
    /**
     * @brief Initialize the user interface
     */
    void setupUI();

    /**
     * @brief Update the gauge display
     */
    void updateGauge();

    /**
     * @brief Update the explanatory text based on the current value
     */
    void updateExplanation();

private:
    // Gauge data
    QVector<GaugeZone> m_zones;      ///< Gauge zones
    bool m_hasValue;
    double m_value;                  ///< Current value
    double m_minValue;               ///< Minimum displayable value
    double m_maxValue;               ///< Maximum displayable value
    unsigned int m_precision;        ///< Precision for value display
    bool m_addPercentageSign;        ///< Indicates whether the % sign should be added to the value
    double m_referenceValue;   // NEW: Reference value
    bool m_showReference;      // NEW: Show the reference line?
    FillDirection m_fillDirection; // NEW: Fill direction

    // UI Widgets
    QGridLayout* m_mainLayout;  ///< Main layout
    QLabel* m_metricNameLabel;       ///< Label for the metric name
    QString m_metricTitle;           ///< Metric title
    FillGaugeRenderWidget* m_gaugeWidget;          ///< Gauge widget
    QLabel* m_valueLabel;            ///< Label for the value
    QString m_baseExplanation;       ///< Base explanatory text
    int m_gaugeHeight;               ///< Gauge height
};


// Derived class for the custom gauge widget
class GaugeRenderWidget : public QWidget {
    Q_OBJECT

public:
    GaugeRenderWidget(QWidget* parent = nullptr) : QWidget(parent), 
        m_value(0.0), m_minValue(0.0), m_maxValue(1.0), m_showCursor(false) {}

    void setZones(const QVector<RatioGaugeWidget::GaugeZone>& zones) {
        m_zones = zones;
        update();
    }
    
    void setValue(double value) {
        m_value = value;
        m_showCursor = true;
        update();
    }
    
    void setRange(double min, double max) {
        m_minValue = min;
        m_maxValue = max;
        update();
    }

    void setShowCursor(bool show) {
        m_showCursor = show;
        update();
    }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        
        int width = this->width();
        int height = this->height();
        
        // Draw the gauge background with gradients for each zone
        QRect gaugeRect(0, 0, width, height);
        
        if (m_zones.isEmpty()) return;
        
        // Calculate the total range width
        double totalRange = m_maxValue - m_minValue;

        int startX = qRound(qBound(0.0, (m_zones[0].minValue - m_minValue) / totalRange, 1.0) * width);
        
        // Draw each zone
        for (const auto& zone : m_zones) {
            double endPos = (zone.maxValue - m_minValue) / totalRange;
            endPos = qBound(0.0, endPos, 1.0);
            int endX = qRound(endPos * width);
            
            // Draw the zone rectangle
            QRect zoneRect(startX, 0, endX - startX, height);
            QColor lighterColor = zone.color.lighter(130);
            painter.fillRect(zoneRect, lighterColor);

            startX = endX; // Update the start for the next zone
        }
        
        // Draw tick marks
        painter.setPen(QPen(QColor(60, 60, 60), 1, Qt::DotLine));
        
        // Font for tick marks - IMPROVED
        QFont tickFont = painter.font();
        tickFont.setPointSizeF(tickFont.pointSizeF() * 1.2); // Font 20% larger
        painter.setFont(tickFont);
        
        // Number of tick marks
        const int numTicks = 10;
        int padding = 5; // Margin to prevent tick marks from overflowing
        
        for (int i = 0; i <= numTicks; i++) {
            // Calculate the adjusted position to prevent overflow at the edges
            double ratio = (double)i / numTicks;
            int x = padding + (int)(ratio * (width - 2 * padding));
            
            // Add values to the main tick marks with better visibility
            if (i % 2 != 0)
                continue;
                
            // Calculate the corresponding value
            double tickValue = m_minValue + (ratio * totalRange);
            QString valueStr = QString::number(tickValue, 'f', 1);
            QFontMetrics fm = painter.fontMetrics();
            int textWidth = fm.horizontalAdvance(valueStr);
            int textHeight = fm.height();
            
            // Adjust X position based on position (first, last, or intermediate)
            int textX;
            if (i == 0) {
                // First label aligned to the left with a slight offset
                textX = padding + 2;
            } else if (i == numTicks) {
                // Last label aligned to the right with a slight offset
                textX = width - padding - textWidth - 2;
            } else {
                // Intermediate labels centered
                textX = x - textWidth / 2;
            }
            
            // Y position centered vertically
            int textY = height / 2 + textHeight / 3;
            
            // Darker and more visible text
            painter.setPen(QColor(0, 0, 0));
            painter.drawText(textX, textY, valueStr);
        }
        
        if (!m_showCursor || m_zones.isEmpty())
            return; // Do not draw the cursor if disabled or no zones
        
        // Calculate the horizontal position of the cursor
        double valuePos = (m_value - m_minValue) / totalRange;
        valuePos = qBound(0.0, valuePos, 1.0);
        int markerX = padding + (int)(valuePos * (width - 2 * padding));
        
        // Hourglass parameters
        int hourglassWidth = qMin(height / 3, 12); // Hourglass width (max 12px)
        int middleHeight = height / 2;

        // Create the hourglass path
        QPainterPath hourglassPath;

        // Upper triangle (base at the top, point downwards)
        hourglassPath.moveTo(markerX - hourglassWidth, 0);  // Top left corner
        hourglassPath.lineTo(markerX + hourglassWidth, 0);  // Top right corner
        hourglassPath.lineTo(markerX, middleHeight);        // Point in the middle
        hourglassPath.closeSubpath();

        // Lower triangle (base at the bottom, point upwards)
        hourglassPath.moveTo(markerX - hourglassWidth, height);  // Bottom left corner
        hourglassPath.lineTo(markerX + hourglassWidth, height);  // Bottom right corner
        hourglassPath.lineTo(markerX, middleHeight);             // Point in the middle
        hourglassPath.closeSubpath();

        // Neutral colors that stand out well
        QColor fillColor = QColor(55, 55, 55, 230);     // Dark gray with slight transparency
        QColor strokeColor = QColor(0, 0, 0);           // Black for the outline

        // Draw the hourglass with a contrasting border
        painter.setPen(QPen(strokeColor, 2));
        painter.setBrush(fillColor);
        painter.drawPath(hourglassPath);
    }

private:
    QVector<RatioGaugeWidget::GaugeZone> m_zones;
    double m_value;
    double m_minValue;
    double m_maxValue;
    bool m_showCursor;
};

// FillGaugeRenderWidget.h - to be added in the RatioGaugeWidget.h header file or in its own file

class FillGaugeRenderWidget : public QWidget {
    Q_OBJECT

public:
    FillGaugeRenderWidget(QWidget* parent = nullptr)
        : QWidget(parent), 
          m_value(0.0), 
          m_minValue(0.0), 
          m_maxValue(1.0),
          m_referenceValue(std::numeric_limits<double>::quiet_NaN()), // Reference value (NaN = no reference)
          m_showReference(false),
          m_fillDirection(FillDirection::LeftToRight) // Default: fill from left to right
    {
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        setMinimumHeight(65); // Minimum height to accommodate labels below
    }

    void setZones(const QVector<RatioGaugeWidget::GaugeZone>& zones) {
        m_zones = zones;
        update();
    }
    
    void setValue(double value) {
        m_value = value;
        update();
    }
    
    void setRange(double min, double max) {
        m_minValue = min;
        m_maxValue = max;
        update();
    }

    // New: Set the reference value
    void setReferenceValue(double referenceValue) {
        m_referenceValue = referenceValue;
        m_showReference = !std::isnan(referenceValue);
        update();
    }

    // New: Enable/disable the reference bar
    void showReference(bool show) {
        m_showReference = show;
        update();
    }

    // New: Set the fill direction
    void setFillDirection(FillDirection direction) {
        m_fillDirection = direction;
        update();
    }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        
        int w = width();
        int h = height();
        
        // Define the main dimensions and positions
        int gaugeHeight = h * 0.55;         // 55% of the height for the gauge and ticks
        int barHeight = gaugeHeight * 0.3;  // Fill bar = 35% of the background height
        int labelHeight = h - gaugeHeight;  // 45% for labels

        int gaugeY = 0;                     // Gauge starts at the top
        int barY = gaugeY + (gaugeHeight - barHeight) / 2; // Bar centered in the gauge
        int tickY = gaugeY + gaugeHeight;    // Y position of ticks (below the gauge)
        int labelY = tickY;              // Y position of labels (below the ticks)
        
        // Rectangle for the full gauge
        QRect gaugeRect(0, gaugeY, w, gaugeHeight);
        
        if (m_zones.isEmpty()) return;
        
        // 1. DRAW THE BACKGROUND OF COLORED ZONES
        double totalRange = m_maxValue - m_minValue;
        int startX = 0;
        
        for (const auto& zone : m_zones) {
            double zoneStart = (zone.minValue - m_minValue) / totalRange;
            double zoneEnd = (zone.maxValue - m_minValue) / totalRange;
            zoneStart = qBound(0.0, zoneStart, 1.0);
            zoneEnd = qBound(0.0, zoneEnd, 1.0);
            
            int zoneStartX = qRound(zoneStart * w);
            int zoneEndX = qRound(zoneEnd * w);
            int zoneWidth = zoneEndX - zoneStartX;
            
            if (zoneWidth > 0) {
                QRect zoneRect(zoneStartX, gaugeY, zoneWidth, gaugeHeight);
                QColor zoneColor = zone.color.lighter(115); // Slightly lighter
                painter.fillRect(zoneRect, zoneColor);
                
                // Thin border between zones
                painter.setPen(QPen(QColor(220, 220, 220), 1));
                painter.drawLine(zoneEndX, gaugeY, zoneEndX, gaugeY + gaugeHeight);
            }
            
            startX = zoneEndX;
        }
        
        // 2. DRAW THE BLACK FILL BAR
        double fillRatio = (m_value - m_minValue) / totalRange;
        
        QRect fillRect;
        
        // Draw the bar based on the chosen direction
        if (m_fillDirection == FillDirection::LeftToRight) {
            fillRatio = qBound(0.0, fillRatio, 1.0);
            int fillWidth = qRound(fillRatio * w);
            fillRect = QRect(0, barY, fillWidth, barHeight);
        } else {
            fillRatio = qBound(0.0, 1 - fillRatio, 1.0);
            int fillWidth = qRound(fillRatio * w);
            fillRect = QRect(w - fillWidth, barY, fillWidth, barHeight);
        }
        
        QColor fillColor(30, 30, 30);  // Semi-transparent black
        painter.fillRect(fillRect, fillColor);
        
        // 3. DRAW THE REFERENCE BAR (NEW)
        if (m_showReference && m_referenceValue >= m_minValue && m_referenceValue <= m_maxValue) {
            double refRatio = (m_referenceValue - m_minValue) / totalRange;
            refRatio = qBound(0.0, refRatio, 1.0);
            int refX = qRound(refRatio * w);
            
            // Draw a vertical line for the reference
            painter.setPen(QPen(QColor(0, 0, 0), 2)); // Black line of 2px
            painter.fillRect(QRect(refX - 1, gaugeY, 3, gaugeHeight), QColor(0, 0, 0));
        }
        
        // 4. DRAW THE TICKS AND LABELS
        painter.setPen(QPen(QColor(20, 20, 20), 1));
        
        QFont tickFont = painter.font();
        tickFont.setPointSizeF(tickFont.pointSizeF());
        painter.setFont(tickFont);
        
        // Number of main ticks
        const int numTicks = 5; // Reduced for better readability
        int tickLength = 5;
        
        for (int i = 0; i <= numTicks; i++) {
            double ratio = (double)i / numTicks;
            int x = qRound(ratio * w);
            
            // Tick mark
            painter.drawLine(x, tickY, x, tickY + tickLength);
            
            // Value label
            double tickValue = m_minValue + (ratio * totalRange);
            QString valueStr = QString::number(tickValue, 'f', 1);
            QFontMetrics fm = painter.fontMetrics();
            int textWidth = fm.horizontalAdvance(valueStr);
            
            // Adjust X position to prevent overflow
            int textX;
            if (i == 0) {
                textX = x;
            } else if (i == numTicks) {
                textX = x - textWidth;
            } else {
                textX = x - textWidth/2;
            }
            
            painter.drawText(textX, labelY + fm.height(), valueStr);
        }
        
        // 5. ADD INTERMEDIATE TICKS (optional)
        painter.setPen(QPen(QColor(120, 120, 120), 0.5, Qt::DotLine));
        for (int i = 1; i < numTicks * 2; i += 2) {
            double ratio = (double)i / (numTicks * 2);
            int x = qRound(ratio * w);
            
            // Shorter intermediate tick
            painter.drawLine(x, tickY - tickLength/2, x, tickY);
        }
        
        // 6. BORDER OF THE FULL GAUGE
        painter.setPen(QPen(QColor(100, 100, 100), 1));
        painter.drawRect(0, gaugeY, w, gaugeHeight);
    }

private:
    QVector<RatioGaugeWidget::GaugeZone> m_zones;
    double m_value;
    double m_minValue;
    double m_maxValue;
    double m_referenceValue;   // NEW: Reference value
    bool m_showReference;      // NEW: Show the reference line?
    FillDirection m_fillDirection; // NEW: Fill direction
};
