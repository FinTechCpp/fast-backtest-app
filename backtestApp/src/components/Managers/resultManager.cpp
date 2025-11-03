#include "components/Managers/resultManager.h"
#include "ui/views/statsView.h"
#include "ui/views/tradesView.h"
#include "ui/views/chartView.h"
#include "ui/views/Stats/HistogramWidget.h"
#include <QDebug>
#include <QTime>
#include <QResizeEvent>
#include <QTimer>

ResultManager::ResultManager(QWidget* parent) : QWidget(parent) 
{
    qDebug() << "ResultManager created with parent:" << parent;
    
    // Initialize all attributes to nullptr first
    m_statsView = nullptr;
    m_tradesView = nullptr;
    m_chartView = nullptr;
    
    // Build the interface in the constructor
    setupUI();
    setupViews();
    setupConnections();
    
    qDebug() << "ResultManager initialized successfully";
}

void ResultManager::updateAllViews(BacktestResults* results)
{
    QTime start = QTime::currentTime();
    qDebug() << "updateAllViews called with BacktestResults:" << results;

    try {
        for (auto it = m_views.begin(); it != m_views.end(); ++it) {
            BaseView* view = it.value();
            if (view) {
                QTime viewStart = QTime::currentTime();
                view->updateData(results);  // Update to use the new signature
                int viewElapsed = viewStart.msecsTo(QTime::currentTime());
                std::cout << "View '" << it.key().toStdString() << "' updated in " << viewElapsed << " ms" << std::endl;
            }
        }

        qInfo() << "All views updated successfully";
    }
    catch (const std::exception& e) {
        qCritical() << "Error while updating views:" << e.what();
    }

    int elapsed = start.msecsTo(QTime::currentTime());
    qInfo() << "ResultManager::updateAllViews() took" << elapsed << "ms";
}

void ResultManager::clearAllViews()
{
    for (auto it = m_views.begin(); it != m_views.end(); ++it) {
        it.value()->clear();
    }
}

void ResultManager::setCurrentTab(int index)
{
    if (m_tabWidget && index >= 0 && index < m_tabWidget->count()) {
        m_tabWidget->setCurrentIndex(index);
    }
}

void ResultManager::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);  // Call the parent method
    
    qDebug() << "ResultManager resized:" << event->size();
    
    // Use the timer to avoid too many resizes
    if (m_resizeTimer) {
        m_resizeTimer->stop();
        m_resizeTimer->start(150);
    }
}

void ResultManager::onTabResized()
{
    qDebug() << "Handling tab resize";
    
    if (!m_tabWidget || !m_chartView) {
        return;
    }
    
    // Get the available size
    QSize availableSize = m_tabWidget->size();
    int tabWidth = availableSize.width();
    
    if (tabWidth > 100) {
        // Calculate the new width for the chart
        int newChartWidth = std::max(800, tabWidth - 40);
        
        qDebug() << "Resizing chart to:" << newChartWidth;
        
        // Resize the chart via the view
        // m_chartView->resizeChart(newChartWidth);
    }
}

void ResultManager::onTabChanged(int index)
{
    qDebug() << "Tab changed to index:" << index;
    
    // Optional: perform specific actions when the tab changes
    if (index >= 0 && index < m_tabWidget->count()) {
        // For example, refresh the active view
        QString currentViewName;
        if (index == 0) currentViewName = "stats";
        else if (index == 1) currentViewName = "histogram";
        else if (index == 2) currentViewName = "chart";
        
        qDebug() << "Active view:" << currentViewName;
    }
}

void ResultManager::setupUI()
{
    // Create the main layout for this widget
    m_mainLayout = new QVBoxLayout(this);  // 'this' becomes the parent widget
    
    // Create the TabWidget
    m_tabWidget = new QTabWidget(this);
    
    // Set the background color of the TabWidget to match the Window palette
    QPalette tabPalette = m_tabWidget->palette();
    QColor windowColor = tabPalette.color(QPalette::Window);
    m_tabWidget->setStyleSheet(QString("QTabWidget::pane { background-color: %1; border: none; }").arg(windowColor.name()));
    
    // Add the TabWidget to the layout
    m_mainLayout->addWidget(m_tabWidget);
    
    // Configure margins if necessary
    m_mainLayout->setContentsMargins(5, 5, 5, 5);
    
    qDebug() << "UI interface created";
}

void ResultManager::setupViews()
{
    // Create the views (they are widgets, so directly usable)
    m_statsView = new StatsView(this);
    m_tradesView = new TradesView(this);
    m_chartView = new ChartView(this);
    
    // Check that the views were created
    if (!m_statsView || !m_tradesView || !m_chartView) {
        qCritical() << "Error while creating views";
        return;
    }
    
    // Add the views as tabs (the views ARE widgets)
    m_tabWidget->addTab(m_statsView, "📊 Statistics");
    m_tabWidget->addTab(m_tradesView, "📋 Trades");
    m_tabWidget->addTab(m_chartView, "📈 Charts");
    
    // Add to the map for easy access
    m_views["stats"] = m_statsView;
    m_views["trades"] = m_tradesView;
    m_views["chart"] = m_chartView;
}

void ResultManager::setupConnections()
{
    // Connect tab change
    connect(m_tabWidget, &QTabWidget::currentChanged,
            this, &ResultManager::onTabChanged);
    
    // Connect the trade click signal from TradesView
    if (m_tradesView) {
        connect(m_tradesView, &TradesView::tradeClicked,
                this, &ResultManager::onTradeClicked);
    }
    
    // Connect the period click signal from StatsView (HistogramWidget)
    if (m_statsView) {
        // Find the histogramWidget in statsView and connect its signal
        HistogramWidget* histogramWidget = m_statsView->findChild<HistogramWidget*>();
        if (histogramWidget) {
            connect(histogramWidget, &HistogramWidget::periodClicked,
                    this, &ResultManager::onPeriodClicked);
            qDebug() << "Signal periodClicked connected from HistogramWidget";
        }
    }
    
    // Create a timer to monitor resizes
    QTimer* resizeTimer = new QTimer(this);
    resizeTimer->setSingleShot(true);
    connect(resizeTimer, &QTimer::timeout, this, &ResultManager::onTabResized);
    
    // Store the timer as a member if necessary
    m_resizeTimer = resizeTimer;
    
    qDebug() << "Connections established";
}

void ResultManager::onTradeClicked(const be::TradeData& trade)
{
    qDebug() << "Trade clicked in ResultManager - Entry:" << trade.entryDate.toString().c_str() 
             << "Exit:" << trade.exitDate.toString().c_str();
    
    // Switch to the Chart tab (index 2: Stats=0, Trades=1, Chart=2)
    int chartTabIndex = -1;
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        if (m_tabWidget->widget(i) == m_chartView) {
            chartTabIndex = i;
            break;
        }
    }
    
    if (chartTabIndex >= 0) {
        m_tabWidget->setCurrentIndex(chartTabIndex);
        
        // Ask ChartView to zoom in on this trade
        if (m_chartView) {
            m_chartView->zoomToTrade(trade);
        }
    }
}

void ResultManager::onPeriodClicked(const QDateTime& startDate, const QDateTime& endDate)
{
    qDebug() << "Period clicked in ResultManager - From:" << startDate.toString("dd/MM/yyyy hh:mm:ss")
             << "To:" << endDate.toString("dd/MM/yyyy hh:mm:ss");
    
    // Switch to the Chart tab
    int chartTabIndex = -1;
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        if (m_tabWidget->widget(i) == m_chartView) {
            chartTabIndex = i;
            break;
        }
    }
    
    if (chartTabIndex >= 0) {
        m_tabWidget->setCurrentIndex(chartTabIndex);
        
        // Ask ChartView to zoom in on this period
        if (m_chartView) {
            m_chartView->zoomToPeriod(startDate, endDate);
        }
    }
}
