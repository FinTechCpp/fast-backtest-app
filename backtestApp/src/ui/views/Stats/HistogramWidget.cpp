#include "ui/views/Stats/HistogramWidget.h"
#include <QDebug>
#include <QDateTime>
#include <QTime>
#include <QGraphicsScene>
#include <QPen>
#include <QBrush>
#include <QFont>
#include <QResizeEvent>
#include <numeric>
#include <algorithm>

// Implementation of InteractiveChartView
InteractiveChartView::InteractiveChartView(QChart* chart, QWidget* parent)
    : QChartView(chart, parent)
    , m_horizontalLine(nullptr)
    , m_verticalLine(nullptr)
    , m_tooltipItem(nullptr)
    , m_crosshairVisible(false)
{
    setMouseTracking(true);
    setRubberBand(QChartView::NoRubberBand);
}

void InteractiveChartView::setupCrosshairElements()
{
    if (!scene()) {
        return;
    }
    
    // Create crosshair lines
    QPen crosshairPen(Qt::gray, 1, Qt::DashLine);
    
    if (!m_horizontalLine) {
        m_horizontalLine = new QGraphicsLineItem();
        m_horizontalLine->setPen(crosshairPen);
        m_horizontalLine->setVisible(false);
        scene()->addItem(m_horizontalLine);
    }
    
    if (!m_verticalLine) {
        m_verticalLine = new QGraphicsLineItem();
        m_verticalLine->setPen(crosshairPen);
        m_verticalLine->setVisible(false);
        scene()->addItem(m_verticalLine);
    }
    
    if (!m_tooltipItem) {
        m_tooltipItem = new QGraphicsTextItem();
        m_tooltipItem->setFont(QFont("Arial", 10));
        m_tooltipItem->setDefaultTextColor(Qt::black);
        m_tooltipItem->setFlag(QGraphicsItem::ItemIgnoresTransformations);
        m_tooltipItem->setVisible(false);
        scene()->addItem(m_tooltipItem);
    }
}

void InteractiveChartView::setTooltipData(const QStringList& categories, const QList<double>& values, 
                                         const QMap<QString, QDateTime>& fullDates)
{
    m_categories = categories;
    m_values = values;
    m_fullDates = fullDates;
}

void InteractiveChartView::mouseMoveEvent(QMouseEvent* event)
{
    QChartView::mouseMoveEvent(event);
    
    if (!m_horizontalLine) {
        setupCrosshairElements();
    }
    
    if (chart() && !m_categories.isEmpty()) {
        QPointF chartPos = chart()->mapToValue(event->pos());
        
        // Discretize X position
        int barIndex = qRound(chartPos.x());
        barIndex = qMax(0, qMin(barIndex, m_categories.size() - 1));
        
        // For crosshair, use mouse position converted to view coordinates
        QPointF discreteViewPos = chart()->mapToPosition(QPointF(barIndex, chartPos.y()));
        
        showCrosshair(discreteViewPos);
        updateTooltip(QPointF(barIndex, chartPos.y()));
    }
}

void InteractiveChartView::mousePressEvent(QMouseEvent* event)
{
    QChartView::mousePressEvent(event);
    
    if (event->button() == Qt::LeftButton && chart() && !m_categories.isEmpty()) {
        QPointF chartPos = chart()->mapToValue(event->pos());
        
        // Discretize X position to get the bar index
        int barIndex = qRound(chartPos.x());
        barIndex = qMax(0, qMin(barIndex, m_categories.size() - 1));
        
        // Emit the signal with the clicked bar index
        emit barClicked(barIndex);
    }
}

void InteractiveChartView::leaveEvent(QEvent* event)
{
    QChartView::leaveEvent(event);
    hideCrosshair();
}

void InteractiveChartView::showCrosshair(const QPointF& position)
{
    if (!chart() || !m_horizontalLine || !m_verticalLine) return;
    
    QRectF plotArea = chart()->plotArea();
    
    // Horizontal line
    m_horizontalLine->setLine(plotArea.left(), position.y(), 
                             plotArea.right(), position.y());
    m_horizontalLine->setVisible(true);
    
    // Vertical line
    m_verticalLine->setLine(position.x(), plotArea.top(), 
                           position.x(), plotArea.bottom());
    m_verticalLine->setVisible(true);
    
    m_crosshairVisible = true;
}

void InteractiveChartView::hideCrosshair()
{
    if (m_horizontalLine) m_horizontalLine->setVisible(false);
    if (m_verticalLine) m_verticalLine->setVisible(false);
    if (m_tooltipItem) m_tooltipItem->setVisible(false);
    m_crosshairVisible = false;
}

void InteractiveChartView::updateTooltip(const QPointF& chartPos)
{
    if (m_categories.isEmpty() || m_values.isEmpty()) return;
    
    int barIndex = qRound(chartPos.x());
    
    if (barIndex >= 0 && barIndex < m_categories.size()) {
        QString category = m_categories[barIndex];
        double value = m_values[barIndex];
        
        QString tooltipText = createTooltipText(category, value);
        m_tooltipItem->setHtml(tooltipText);
        
        QPointF scenePos = mapToScene(mapFromGlobal(QCursor::pos()));
        QRectF tooltipRect = m_tooltipItem->boundingRect();
        
        qreal x = scenePos.x() + 15;
        qreal y = scenePos.y() - tooltipRect.height() - 15;
        
        QRectF chartRect = chart()->plotArea();
        if (x + tooltipRect.width() > chartRect.right()) {
            x = scenePos.x() - tooltipRect.width() - 15;
        }
        if (y < chartRect.top()) {
            y = scenePos.y() + 15;
        }
        
        m_tooltipItem->setPos(x, y);
        m_tooltipItem->setVisible(true);
    }
}

QString InteractiveChartView::createTooltipText(const QString& category, double value)
{
    QString periodLabel;
    QString detailedInfo = category;
    
    if (m_fullDates.contains(category)) {
        QDateTime referenceDate = m_fullDates[category];
        
        if (category.contains("/") && category.length() <= 10) {
            periodLabel = "Day";
            detailedInfo = referenceDate.toString("dddd dd MMMM yyyy");
        }
        else if (category.startsWith("S") && category.contains("(")) {
            periodLabel = "Week";
            int weekNumber = referenceDate.date().weekNumber();
            int year = referenceDate.date().year();
            QDate weekStart = referenceDate.date().addDays(-(referenceDate.date().dayOfWeek() - 1));
            QDate weekEnd = weekStart.addDays(6);
            detailedInfo = QString("W%1 of %2 (%3 to %4)")
                          .arg(weekNumber)
                          .arg(year)
                          .arg(weekStart.toString("dd MMMM"))
                          .arg(weekEnd.toString("dd MMMM"));
        }
        else if (category.contains("/") && category.length() == 7) {
            periodLabel = "Month";
            detailedInfo = referenceDate.toString("MMMM yyyy");
        }
        else if (category.startsWith("T")) {
            periodLabel = "Quarter";
            int quarter = (referenceDate.date().month() - 1) / 3 + 1;
            QString quarterNames[] = {"", "Q1", "Q2", "Q3", "Q4"};
            detailedInfo = QString("%1 %2").arg(quarterNames[quarter]).arg(referenceDate.date().year());
        }
        else if (category.length() == 4) {
            periodLabel = "Year";
            detailedInfo = QString("Year %1").arg(category);
        }
    }
    
    QString tooltipText = QString(
        "<div style='background-color: rgba(255,255,224,240); "
        "border: 1px solid gray; padding: 8px; border-radius: 4px;'>"
        "<b>%1:</b> %2<br/>"
        "<b>Total P&L:</b> <span style='color: %4;'>%3€</span>"
        "</div>")
        .arg(periodLabel.isEmpty() ? "Period" : periodLabel)
        .arg(detailedInfo)
        .arg(QString::number(value, 'f', 2))
        .arg(value >= 0 ? "green" : "red");
    
    return tooltipText;
}



// Implementation of HistogramWidget
HistogramWidget::HistogramWidget(const QString& title, QWidget* parent)
    : TitledWidget(title, parent)
    , m_stackWidget(new QStackedWidget(this))
    , m_placeholderLabel(new QLabel("No trades to display", this))
    , m_timeUnitCombo(nullptr)
    , m_chartView(nullptr)
    , m_chart(nullptr)
    , m_currentResults(nullptr)
{
    // ComboBox to select time unit
    m_timeUnitCombo = new QComboBox(this);
    m_timeUnitCombo->addItems({"Day", "Week", "Month", "Quarter", "Year"});
    m_timeUnitCombo->setCurrentIndex(0);
    m_timeUnitCombo->setFixedWidth(100);
    connect(m_timeUnitCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &HistogramWidget::updateHistogram);
    setTitleCompanionWidget(m_timeUnitCombo);

    // Centered placeholder
    m_placeholderLabel->setAlignment(Qt::AlignCenter);
    m_placeholderLabel->setStyleSheet("color: gray; font-size: 16px;");
    m_placeholderLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // Create chart
    m_chart = new QChart();
    m_chart->setAnimationOptions(QChart::SeriesAnimations);
    m_chart->legend()->setVisible(false);
    m_chart->setBackgroundVisible(true);
    m_chart->setMargins(QMargins(0, 0, 0, 0));

    // Create chart view
    m_chartView = new InteractiveChartView(m_chart, this);
    m_chartView->setRenderHint(QPainter::Antialiasing);
    m_chartView->setBackgroundBrush(Qt::transparent);
    m_chartView->setContentsMargins(0, 0, 0, 0);

    // Add widgets to stack
    m_stackWidget->addWidget(m_chartView);        // index 0
    m_stackWidget->addWidget(m_placeholderLabel); // index 1
    m_stackWidget->setContentsMargins(1, 1, 1, 1);

    // Use stack as main layout
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(1, 1, 1, 1);
    layout->setSpacing(0);
    layout->addWidget(m_stackWidget);
    setLayout(layout);
    
    // Connect bar click signal
    connect(m_chartView, &InteractiveChartView::barClicked, 
            this, &HistogramWidget::onBarClicked);
}

HistogramWidget::~HistogramWidget()
{
    if (m_chart) {
        // Remove all series and axes
        const auto allSeries = m_chart->series();
        for (auto series : allSeries) {
            m_chart->removeSeries(series);
        }
        
        const auto allAxes = m_chart->axes();
        for (auto axis : allAxes) {
            m_chart->removeAxis(axis);
        }
        
        m_chart->deleteLater();
    }
}

void HistogramWidget::paintContent(QPainter& painter, const QRect& contentRect)
{
    if (m_stackWidget) {
        m_stackWidget->setGeometry(contentRect);
    }
}

void HistogramWidget::updateData(BacktestResults* results)
{
    m_currentResults = results;
    updateHistogram();
}

void HistogramWidget::clear()
{
    if (m_chart) {
        const auto allSeries = m_chart->series();
        for (auto series : allSeries) {
            m_chart->removeSeries(series);
        }
    }
    m_currentResults = nullptr;
}

void HistogramWidget::updateHistogram()
{
    if (!m_currentResults || !m_timeUnitCombo) {
        m_placeholderLabel->setText("No trades to display");
        m_stackWidget->setCurrentWidget(m_placeholderLabel);
        return;
    }
    
    // Extract trades from results
    std::vector<TradeInfo> trades = extractTradesFromResults(m_currentResults);
    
    if (trades.empty()) {
        m_placeholderLabel->setText("No trades to display");
        m_stackWidget->setCurrentWidget(m_placeholderLabel);
        return;
    }
    
    // Get selected time unit
    QString timeUnit = m_timeUnitCombo->currentText();

    // Group data by time unit
    GroupedData groupedData = groupDataByTimeUnit(trades, timeUnit);
    
    if (groupedData.categories.isEmpty()) {
        m_placeholderLabel->setText("Unable to group data");
        m_stackWidget->setCurrentWidget(m_placeholderLabel);
        return;
    }
    
    // Store grouped data for use on clicks
    m_currentGroupedData = groupedData;
    
    // Create the chart
    m_stackWidget->setCurrentWidget(m_chartView);
    createChart(groupedData);
}

std::vector<HistogramWidget::TradeInfo> HistogramWidget::extractTradesFromResults(BacktestResults* results)
{
    std::vector<TradeInfo> trades;
    
    if (!results) {
        return trades;
    }
    
    for (const auto& trade : results->stats.trades) {
        TradeInfo info;
        info.exitTime = QDateTime(
            QDate(trade.exitDate.year, trade.exitDate.month, trade.exitDate.day),
            QTime(trade.exitDate.hour, trade.exitDate.minute, trade.exitDate.second)
        );
        info.pnl = trade.pl;
        trades.push_back(info);
    }
    
    return trades;
}

HistogramWidget::GroupedData HistogramWidget::groupDataByTimeUnit(
    const std::vector<TradeInfo>& trades, const QString& timeUnit)
{
    GroupedData result;
    
    if (trades.empty()) {
        return result;
    }
    
    // Map to store PnL per period
    QMap<QString, double> periodPnL;
    QMap<QString, QDateTime> periodDates;
    
    for (const TradeInfo& trade : trades) {
        QDateTime exitTime = trade.exitTime;
        
        if (!exitTime.isValid()) {
            continue;
        }
        
        // Generate period key
        QString periodKey = generatePeriodKey(exitTime, timeUnit);
        
        if (periodKey.isEmpty()) {
            continue;
        }
        
        // Accumulate PnL
        periodPnL[periodKey] += trade.pnl;
        
        // Store a representative date
        if (!periodDates.contains(periodKey)) {
            QDateTime representativeDate = getRepresentativeDate(exitTime, timeUnit);
            periodDates[periodKey] = representativeDate;
            
            // Compute the date range for this period
            QPair<QDateTime, QDateTime> range = getPeriodRange(representativeDate, timeUnit);
            result.periodRanges[periodKey] = range;
        }
    }
    
    // Sort periods chronologically
    QList<QPair<QDateTime, QString>> sortedPeriods;
    for (auto it = periodDates.begin(); it != periodDates.end(); ++it) {
        sortedPeriods.append(qMakePair(it.value(), it.key()));
    }
    
    std::sort(sortedPeriods.begin(), sortedPeriods.end());
    
    // Build sorted results
    for (const auto& pair : sortedPeriods) {
        QString periodKey = pair.second;
        result.categories.append(periodKey);
        result.values.append(periodPnL[periodKey]);
        result.fullDates[periodKey] = pair.first;
    }
    
    return result;
}

void HistogramWidget::createChart(const GroupedData& data)
{
    if (!m_chart) {
        return;
    }

    // Disable animations for large datasets
    if (data.categories.size() > 50) {
        m_chart->setAnimationOptions(QChart::NoAnimation);
    } else {
        m_chart->setAnimationOptions(QChart::SeriesAnimations);
    }
    
    // Clear chart
    const auto seriesToRemove = m_chart->series();
    for (auto series : seriesToRemove) {
        m_chart->removeSeries(series);
    }
    
    const auto axesToRemove = m_chart->axes();
    for (auto axis : axesToRemove) {
        m_chart->removeAxis(axis);
    }
    
    // Create series
    QStackedBarSeries* barSeries = new QStackedBarSeries();
    
    // Create bar sets
    QBarSet* gainsSet = new QBarSet("Profits");
    QBarSet* lossesSet = new QBarSet("Losses");
    
    // Configure colors
    gainsSet->setColor(QColor(76, 175, 80));
    gainsSet->setBorderColor(QColor(56, 142, 60));
    
    lossesSet->setColor(QColor(244, 67, 54));
    lossesSet->setBorderColor(QColor(198, 40, 40));
    
    // Split data into gains and losses
    for (int i = 0; i < data.values.size(); ++i) {
        double value = data.values[i];
        
        if (value >= 0) {
            gainsSet->append(value);
            lossesSet->append(0);  
        } else {
            gainsSet->append(0);   
            lossesSet->append(value);
        }
    }
    
    barSeries->append(gainsSet);
    barSeries->append(lossesSet);
    m_chart->addSeries(barSeries);
    
    // Create axes
    QBarCategoryAxis* axisX = new QBarCategoryAxis();

    // Limit number of labels for readability
    const int maxLabels = 10;  // Maximum number of labels to show
    int categoryCount = data.categories.size();
    
    // Show all categories if their number is less than maxLabels
    // int off = 28;
    // axisX->append(data.categories[0 + off]);
    // axisX->append(data.categories[1 + off]);
    // axisX->append(data.categories[2 + off]);
    // axisX->append(data.categories[3 + off]);

    axisX->append(data.categories);

    // Keep labels hidden (can be enabled if needed)
    // if (categoryCount > maxLabels) {
    //     axisX->setLabelsAngle(-45.0);
    // }

    axisX->setLabelsVisible(false);

    
    QValueAxis* axisY = new QValueAxis();
    
    // Configure Y scale
    if (!data.values.isEmpty()) {
        double minValue = *std::min_element(data.values.begin(), data.values.end());
        double maxValue = *std::max_element(data.values.begin(), data.values.end());
        
        double range = maxValue - minValue;
        double margin = range > 0 ? range * 0.1 : 100.0;
        
        axisY->setRange(minValue - margin, maxValue + margin);
    }
    
    m_chart->addAxis(axisX, Qt::AlignBottom);
    m_chart->addAxis(axisY, Qt::AlignRight);
    
    barSeries->attachAxis(axisX);
    barSeries->attachAxis(axisY);
    
    // Configure title
    QString timeUnit = m_timeUnitCombo->currentText();
    double totalPnL = std::accumulate(data.values.begin(), data.values.end(), 0.0);
    
    // Configure appearance
    m_chart->setBackgroundRoundness(0);
    
    // Configure tooltip data
    if (m_chartView) {
        m_chartView->setTooltipData(data.categories, data.values, data.fullDates);
    }
}

QDateTime HistogramWidget::getRepresentativeDate(const QDateTime& dateTime, const QString& timeUnit)
{
    QDate date = dateTime.date();
    
    if (timeUnit == "Day") {
        return QDateTime(date, QTime(0, 0, 0));
    }
    else if (timeUnit == "Week") {
        QDate weekStart = date.addDays(-(date.dayOfWeek() - 1));
        return QDateTime(weekStart, QTime(0, 0, 0));
    }
    else if (timeUnit == "Month") {
        return QDateTime(QDate(date.year(), date.month(), 1), QTime(0, 0, 0));
    }
    else if (timeUnit == "Quarter") {
        int quarter = (date.month() - 1) / 3 + 1;
        int firstMonthOfQuarter = (quarter - 1) * 3 + 1;
        return QDateTime(QDate(date.year(), firstMonthOfQuarter, 1), QTime(0, 0, 0));
    }
    else if (timeUnit == "Year") {
        return QDateTime(QDate(date.year(), 1, 1), QTime(0, 0, 0));
    }
    
    return dateTime;
}

QString HistogramWidget::generatePeriodKey(const QDateTime& dateTime, const QString& timeUnit)
{
    if (!dateTime.isValid()) {
        return QString();
    }
    
    QDate date = dateTime.date();
    
    if (timeUnit == "Day") {
        // If it's the first of the month show the month name
        // if it's January 1st show the year
        // otherwise show the full day
        return date.toString("dd/MM/yyyy");
    }
    else if (timeUnit == "Week") {
        QDate weekStart = date.addDays(-(date.dayOfWeek() - 1));
        QDate weekEnd = weekStart.addDays(6);
        return QString("W%1 (%2 - %3)")
               .arg(weekStart.weekNumber())
               .arg(weekStart.toString("dd/MM"))
               .arg(weekEnd.toString("dd/MM"));
    }
    else if (timeUnit == "Month") {
        return date.toString("MM/yyyy");
    }
    else if (timeUnit == "Quarter") {
        int quarter = (date.month() - 1) / 3 + 1;
        return QString("Q%1 %2").arg(quarter).arg(date.year());
    }
    else if (timeUnit == "Year") {
        return date.toString("yyyy");
    }
    
    return QString();
}

QPair<QDateTime, QDateTime> HistogramWidget::getPeriodRange(const QDateTime& representativeDate, const QString& timeUnit)
{
    QDateTime startDate, endDate;
    QDate date = representativeDate.date();
    
    if (timeUnit == "Day") {
        startDate = QDateTime(date, QTime(0, 0, 0));
        endDate = QDateTime(date, QTime(23, 59, 59));
    }
    else if (timeUnit == "Week") {
        QDate weekStart = date.addDays(-(date.dayOfWeek() - 1));
        QDate weekEnd = weekStart.addDays(6);
        startDate = QDateTime(weekStart, QTime(0, 0, 0));
        endDate = QDateTime(weekEnd, QTime(23, 59, 59));
    }
    else if (timeUnit == "Month") {
        QDate monthStart(date.year(), date.month(), 1);
        QDate monthEnd(date.year(), date.month(), date.daysInMonth());
        startDate = QDateTime(monthStart, QTime(0, 0, 0));
        endDate = QDateTime(monthEnd, QTime(23, 59, 59));
    }
    else if (timeUnit == "Quarter") {
        int quarter = (date.month() - 1) / 3;
        int firstMonth = quarter * 3 + 1;
        QDate quarterStart(date.year(), firstMonth, 1);
        QDate quarterEnd(date.year(), firstMonth + 2, QDate(date.year(), firstMonth + 2, 1).daysInMonth());
        startDate = QDateTime(quarterStart, QTime(0, 0, 0));
        endDate = QDateTime(quarterEnd, QTime(23, 59, 59));
    }
    else if (timeUnit == "Year") {
        QDate yearStart(date.year(), 1, 1);
        QDate yearEnd(date.year(), 12, 31);
        startDate = QDateTime(yearStart, QTime(0, 0, 0));
        endDate = QDateTime(yearEnd, QTime(23, 59, 59));
    }
    
    return qMakePair(startDate, endDate);
}

void HistogramWidget::onBarClicked(int barIndex)
{
    if (barIndex < 0 || barIndex >= m_currentGroupedData.categories.size()) {
        return;
    }
    
    QString categoryKey = m_currentGroupedData.categories[barIndex];
    
    if (m_currentGroupedData.periodRanges.contains(categoryKey)) {
        QPair<QDateTime, QDateTime> range = m_currentGroupedData.periodRanges[categoryKey];
        
        qDebug() << "Clicked period:" << categoryKey 
                 << "From:" << range.first.toString("dd/MM/yyyy hh:mm:ss")
                 << "To:" << range.second.toString("dd/MM/yyyy hh:mm:ss");
        
        // Emit signal with the period
        emit periodClicked(range.first, range.second);
    }
}
