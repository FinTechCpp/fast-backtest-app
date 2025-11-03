#include "ui/views/Stats/MonthlyPerformanceWidget.h"
#include <QDebug>
#include <QFont>
#include <QFontMetrics>
#include <QLinearGradient>
#include <QPainter>
#include <algorithm>
#include <cmath>

MonthlyPerformanceWidget::MonthlyPerformanceWidget(QWidget* parent)
    : StatsBaseWidget(parent),
      m_minYear(0),
      m_maxYear(0),
      m_minValue(0.0),
      m_maxValue(0.0)
{
    // Create the main layout
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    
    // Create the group box
    m_groupBox = new QGroupBox("Monthly Performance by Year");
    m_mainLayout->addWidget(m_groupBox);
    
    // Internal layout of the group box
    QVBoxLayout* groupLayout = new QVBoxLayout(m_groupBox);
    groupLayout->setContentsMargins(10, 20, 10, 10);
    
    // Create the scene and the view
    m_scene = new QGraphicsScene(this);
    m_view = new QGraphicsView(m_scene);
    m_view->setRenderHint(QPainter::Antialiasing, true);
    m_view->setMinimumHeight(250);
    m_view->setMinimumWidth(750);  // Wider to accommodate the legend
    m_view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_view->setAlignment(Qt::AlignCenter);
    
    // Add the view to the layout
    groupLayout->addWidget(m_view);
    
    // Install an event filter to handle resizing
    m_view->viewport()->installEventFilter(this);
}

MonthlyPerformanceWidget::~MonthlyPerformanceWidget()
{
    // Qt handles deletion of child objects
}

void MonthlyPerformanceWidget::analyzeTradesByMonthAndYear(const std::vector<be::TradeData>& trades)
{
    // Reset data
    m_performanceData.clear();
    m_tradeCountData.clear();
    m_squaredSumData.clear();
    
    // Reset min/max values
    m_minValue = 0.0;
    m_maxValue = 0.0;
    m_minYear = 0;
    m_maxYear = 0;
    bool firstTrade = true;
    
    // Analyze each trade
    for (const auto& trade : trades) {
        int year = trade.entryDate.year;
        int month = trade.entryDate.month;
        
        // Update min and max years
        if (firstTrade) {
            m_minYear = m_maxYear = year;
            firstTrade = false;
        } else {
            m_minYear = std::min(m_minYear, year);
            m_maxYear = std::max(m_maxYear, year);
        }
        
        // Add performance and increment the count
        m_performanceData[year][month] += trade.pl;
        m_tradeCountData[year][month]++;
        m_squaredSumData[year][month] += trade.pl * trade.pl;
    }
    
    // If no trades, exit
    if (firstTrade) return;
    
    // Find min and max values for the color scale
    bool firstValue = true;
    for (auto yearIt = m_performanceData.begin(); yearIt != m_performanceData.end(); ++yearIt) {
        for (auto monthIt = yearIt.value().begin(); monthIt != yearIt.value().end(); ++monthIt) {
            int year = yearIt.key();
            int month = monthIt.key();
            
            // Use the total PnL, not the expectation
            double monthlyTotal = monthIt.value();
            
            if (firstValue) {
                m_minValue = m_maxValue = monthlyTotal;
                firstValue = false;
            } else {
                m_minValue = std::min(m_minValue, monthlyTotal);
                m_maxValue = std::max(m_maxValue, monthlyTotal);
            }
        }
    }
    
    // Balance bounds so absolute values are symmetric
    double absMax = std::max(std::fabs(m_minValue), std::fabs(m_maxValue));
    m_minValue = -absMax;
    m_maxValue = absMax;
    
    // Ensure min and max are not identical to avoid division by zero
    if (std::abs(m_maxValue - m_minValue) < 1e-6) {
        m_minValue = -1.0;
        m_maxValue = 1.0;
    }
}

QColor MonthlyPerformanceWidget::getColorForValue(double value)
{
    // Ensure the value is within the [min, max] range
    value = std::max(m_minValue, std::min(value, m_maxValue));
    
    // Normalize the value between -1 and 1
    double range = m_maxValue - m_minValue;
    double normalizedValue = range != 0 ? 2.0 * (value - m_minValue) / range - 1.0 : 0.0;
    
    // Symmetric color scale
    if (normalizedValue < 0) {
        // From red to yellow for negative values
        double ratio = 1.0 + normalizedValue; // 0 to 1 (from min to 0)
        
        // Red always maximum for negative values
        int red = 255;
        
        // Green varies from 0 to 255
        int green = static_cast<int>(255 * ratio);
        
        // Blue always 0
        int blue = 0;
        
        return QColor(red, green, blue);
    } 
    else {
        // From yellow to green for positive values
        double ratio = normalizedValue; // 0 to 1 (from 0 to max)
        
        // Red varies from 255 to 0
        int red = static_cast<int>(255 * (1.0 - ratio));
        
        // Green always 255 for positive values
        int green = 255;
        
        // Blue always 0
        int blue = 0;
        
        return QColor(red, green, blue);
    }
}

void MonthlyPerformanceWidget::buildHeatmap()
{
    // Clear the scene
    m_scene->clear();
    
    // If no data, exit
    if (m_minYear == 0 || m_maxYear == 0) {
        m_groupBox->setTitle("Monthly Performance by Year (no data)");
        return;
    }
    
    // Number of years to display
    int yearCount = m_maxYear - m_minYear + 1;
    
    // Adjust cell size based on number of years
    int adjustedCellSize = std::min(CELL_SIZE, 300 / yearCount);
    
    // Margins for labels
    int leftMargin = 60;   // For year labels
    int topMargin = 40;    // For month labels
    int rightMargin = 180; // For the legend
    
    // Calculate total number of trades
    int totalTrades = 0;
    double totalPerformance = 0.0;
    
    for (auto yearIt = m_tradeCountData.begin(); yearIt != m_tradeCountData.end(); ++yearIt) {
        for (auto monthIt = yearIt.value().begin(); monthIt != yearIt.value().end(); ++monthIt) {
            totalTrades += monthIt.value();
        }
    }
    
    for (auto yearIt = m_performanceData.begin(); yearIt != m_performanceData.end(); ++yearIt) {
        for (auto monthIt = yearIt.value().begin(); monthIt != yearIt.value().end(); ++monthIt) {
            totalPerformance += monthIt.value();
        }
    }
    
    // Add a title
    // QGraphicsTextItem* titleText = m_scene->addText(
    //     QString("Distribution of %1 trades (%2 €)")
    //         .arg(totalTrades)
    //         .arg(totalPerformance, 0, 'f', 2)
    // );
    // QFont titleFont = titleText->font();
    // titleFont.setPointSize(10);
    // // titleFont.setBold(true);
    // titleText->setFont(titleFont);
    // titleText->setPos(leftMargin + 100, 5);
    
    // Draw month labels (top)
    for (int m = 0; m < MONTHS_IN_YEAR; m++) {
        QGraphicsTextItem* monthLabel = m_scene->addText(m_monthNames[m]);
        QFont monthFont = monthLabel->font();
        monthLabel->setFont(monthFont);
        
        // Center the text on the column
        QRectF textRect = monthLabel->boundingRect();
        monthLabel->setPos(leftMargin + m * (adjustedCellSize + CELL_SPACING) + 
                          (adjustedCellSize - textRect.width())/2, topMargin - 25);
    }
    
    // Draw year labels (left)
    for (int y = 0; y <= m_maxYear - m_minYear; y++) {
        int year = m_minYear + y;
        QGraphicsTextItem* yearLabel = m_scene->addText(QString::number(year));
        QFont yearFont = yearLabel->font();
        yearLabel->setFont(yearFont);
        
        // Align to the right
        QRectF textRect = yearLabel->boundingRect();
        yearLabel->setPos(leftMargin - textRect.width() - 10, 
                         topMargin + y * (adjustedCellSize + CELL_SPACING) + 
                         (adjustedCellSize - textRect.height())/2);
    }
    
    // Draw the heatmap
    for (int y = 0; y <= m_maxYear - m_minYear; y++) {
        int year = m_minYear + y;
        
        for (int m = 0; m < MONTHS_IN_YEAR; m++) {
            int month = m + 1; // Months start at 1
            
            // Cell position
            int x = leftMargin + m * (adjustedCellSize + CELL_SPACING);
            int y_pos = topMargin + y * (adjustedCellSize + CELL_SPACING);
            
            // Check if there are trades for this cell
            if (!m_tradeCountData.contains(year) || !m_tradeCountData[year].contains(month) || 
                m_tradeCountData[year][month] <= 0) {
                // Gray cell for periods without trades
                m_scene->addRect(
                    x, y_pos, adjustedCellSize, adjustedCellSize,
                    QPen(Qt::black, 0.5), QBrush(QColor(240, 240, 240))
                );
                continue;
            }
            
            // Compute expectation and standard deviation
            int count = m_tradeCountData[year][month];
            double totalPnL = m_performanceData[year][month];
            double squaredSum = m_squaredSumData[year][month];
            
            double monthlyTotal = totalPnL;
            double expectation = totalPnL / count;  // Expectation (mean)
            double variance = (squaredSum / count) - (expectation * expectation);  // Variance
            double stdDev = variance > 0 ? std::sqrt(variance) : 0;  // Standard deviation
            
            // Use total PnL to determine the color
            QColor cellColor = getColorForValue(monthlyTotal);

            // Add a rectangle with a thin border
            QGraphicsRectItem* cell = m_scene->addRect(
                x, y_pos, adjustedCellSize, adjustedCellSize,
                QPen(Qt::black, 0.5), QBrush(cellColor)
            );
            
            // Add a more detailed tooltip
            QString tooltipText = QString("%1 %2\nTotal: %3 €\nMean: %4 €\nStd dev: %5 €\nTrades: %6")
                                .arg(m_monthNames[m])
                                .arg(year)
                                .arg(monthlyTotal, 0, 'f', 2)
                                .arg(expectation, 0, 'f', 2)
                                .arg(stdDev, 0, 'f', 2)
                                .arg(count);
            cell->setToolTip(tooltipText);
            
            // Display the total PnL in the cell
            QGraphicsTextItem* totalText = m_scene->addText(QString("%1").arg(monthlyTotal, 0, 'f', 0));
            QFont totalFont = totalText->font();
            totalFont.setBold(true);
            totalFont.setPointSize(std::min(10, adjustedCellSize / 5));
            totalText->setFont(totalFont);
            
            // Display the std dev below (smaller)
            QGraphicsTextItem* stdText = m_scene->addText(QString("%1").arg(stdDev, 0, 'f', 0));
            QFont stdFont = stdText->font();
            stdFont.setPointSize(std::max(6, totalFont.pointSize() - 2));  // Smaller than the total
            stdText->setFont(stdFont);
            
            // Center and position the total at the top of the cell
            QRectF expRect = totalText->boundingRect();
            totalText->setPos(x + (adjustedCellSize - expRect.width())/2, 
                           y_pos + adjustedCellSize * 0.25 - expRect.height()/2);
            
            // Position the std dev at the bottom of the cell
            QRectF stdRect = stdText->boundingRect();
            stdText->setPos(x + (adjustedCellSize - stdRect.width())/2, 
                           y_pos + adjustedCellSize * 0.75 - stdRect.height()/2);
            
            // Adjust text color for readability
            QColor textColor = QColor::fromHsv(cellColor.hue(), 
                                              cellColor.saturation(),
                                              cellColor.value() < 128 ? 240 : 30);
            totalText->setDefaultTextColor(textColor);
            stdText->setDefaultTextColor(textColor);
        }
    }
    
    // *** VERTICAL LEGEND ON THE RIGHT ***
    // Legend starting position
    int legendX = leftMargin + MONTHS_IN_YEAR * (adjustedCellSize + CELL_SPACING) + 30;
    int legendY = topMargin + 10;
    int legendWidth = 30;
    int legendHeight = yearCount * (adjustedCellSize + CELL_SPACING) - 20;
    
    // Legend title
    QGraphicsTextItem* legendTitle = m_scene->addText("Monthly PnL (€)");
    QFont legendTitleFont = legendTitle->font();
    // legendTitleFont.setBold(true);
    legendTitle->setFont(legendTitleFont);
    legendTitle->setPos(legendX, 10);
    
    // Vertical gradient for the legend
    QLinearGradient gradient(0, legendY + legendHeight, 0, legendY);
    
    // Gradient stop points (red-orange-yellow-green)
    gradient.setColorAt(0.0, getColorForValue(m_minValue));      // Red for min
    gradient.setColorAt(0.25, getColorForValue(m_minValue/2));   // Orange-red
    gradient.setColorAt(0.5, getColorForValue(0.0));             // Yellow for zero
    gradient.setColorAt(0.75, getColorForValue(m_maxValue/2));   // Yellow-green
    gradient.setColorAt(1.0, getColorForValue(m_maxValue));      // Green for max
    
    // Gradient rectangle with border
    m_scene->addRect(legendX, legendY, legendWidth, legendHeight, 
                    QPen(Qt::black, 1), QBrush(gradient));
    
    // Value labels
    // Maximum (top)
    QGraphicsTextItem* maxText = m_scene->addText(QString("%1 €").arg(m_maxValue, 0, 'f', 2));
    QFont valueFont = maxText->font();
    valueFont.setPointSize(14);
    maxText->setFont(valueFont);
    maxText->setPos(legendX + legendWidth + 5, legendY - maxText->boundingRect().height()/2);
    
    // Positive quarter
    if (m_maxYear - m_minYear > 1) {
        QGraphicsTextItem* quarterPosText = m_scene->addText(QString("%1 €").arg(m_maxValue/2, 0, 'f', 2));
        quarterPosText->setFont(valueFont);
        quarterPosText->setPos(legendX + legendWidth + 5, legendY + legendHeight/4 - quarterPosText->boundingRect().height()/2);
    }

    // Zero (middle)
    QGraphicsTextItem* zeroText = m_scene->addText("0 €");
    zeroText->setFont(valueFont);
    zeroText->setPos(legendX + legendWidth + 5, legendY + legendHeight/2 - zeroText->boundingRect().height()/2);
    
    // Negative quarter
    if (m_minYear - m_maxYear < -1) {
        QGraphicsTextItem* quarterNegText = m_scene->addText(QString("%1 €").arg(m_minValue/2, 0, 'f', 2));
        quarterNegText->setFont(valueFont);
        quarterNegText->setPos(legendX + legendWidth + 5, legendY + 3*legendHeight/4 - quarterNegText->boundingRect().height()/2);
    }

    // Minimum (bottom)
    QGraphicsTextItem* minText = m_scene->addText(QString("%1 €").arg(m_minValue, 0, 'f', 2));
    minText->setFont(valueFont);
    minText->setPos(legendX + legendWidth + 5, legendY + legendHeight - minText->boundingRect().height()/2);
    
    // Adjust scene size to include all content
    QRectF boundingRect = m_scene->itemsBoundingRect();
    m_scene->setSceneRect(boundingRect);
    
    // Adjust view to show whole scene
    m_view->fitInView(boundingRect, Qt::KeepAspectRatio);
    m_view->centerOn(boundingRect.center());
    
    // Update the groupbox title
    m_groupBox->setTitle(QString("Monthly PnL Expectation (%1 - %2)").arg(m_minYear).arg(m_maxYear));
}

void MonthlyPerformanceWidget::updateContent(const be::Stats& stats)
{
    if (stats.trades.empty()) {
        clear();
        return;
    }
    
    analyzeTradesByMonthAndYear(stats.trades);
    buildHeatmap();
    
    qDebug() << "Monthly Performance Widget updated with" << stats.trades.size() << "trades"
             << "covering years" << m_minYear << "to" << m_maxYear
             << "PnL scale: [" << m_minValue << "," << m_maxValue << "]";
}

void MonthlyPerformanceWidget::clear()
{
    m_performanceData.clear();
    m_tradeCountData.clear();
    m_minYear = m_maxYear = 0;
    m_minValue = m_maxValue = 0.0;
    
    m_scene->clear();
    m_groupBox->setTitle("Monthly Performance by Year (no data)");
}

bool MonthlyPerformanceWidget::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_view->viewport() && event->type() == QEvent::Resize) {
        if (!m_scene->items().isEmpty()) {
            m_view->fitInView(m_scene->itemsBoundingRect(), Qt::KeepAspectRatio);
        }
    }
    return QWidget::eventFilter(watched, event);
}

void MonthlyPerformanceWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    
    // Readjust the view to the content after resizing
    if (!m_scene->items().isEmpty()) {
        QRectF bounds = m_scene->itemsBoundingRect();
        if (!bounds.isEmpty()) {
            m_view->fitInView(bounds, Qt::KeepAspectRatio);
        }
    }
}